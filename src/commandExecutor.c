#include "commandExecutor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "log.h"
#include "network.h"
#include "fileReader.h"


DB* create_db(){
    DB *db = (DB*) malloc(sizeof(DB));
    memset(db->dicts, 0, MAX_FILES*sizeof(dict*));
    for(unsigned i=0;i<MAX_FILES;++i){
        pthread_mutex_init( &(db->dictLocks[i]), NULL);
    }
    return db;
}

void free_db(DB* db){
    for(unsigned i=0;i<MAX_FILES;++i){
        LOG_INFO("Free dict %d", i);
        if(db->dicts[i] != 0){
            free_dict(db->dicts[i]);
        }
    }
    LOG_INFO("Free db ");

    free(db);
    return;
}

int __unload_file(DB* db, char* filename){
    int d = 0;
    for(unsigned i=0;i<MAX_FILES;++i){
        pthread_mutex_lock(&(db->dictLocks[i]));
        if(db->dicts[i] != 0 && strcmp(filename, db->dicts[i]->filename) == 0){
            d = 1;
            free_dict(db->dicts[i]);
            db->dicts[i] = 0;
        }
        pthread_mutex_unlock(&(db->dictLocks[i]));
    }

    if(d == 0){
        LOG_DEBUG("Filename not found %s", filename);
        return -1;
    }

    return 1;
}

// Helper function to adjust chunk size to next space
off_t __find_next_space(int fd, off_t offset, off_t max_offset) {
    char c;
    off_t current = offset;
    while (current < max_offset) {
        lseek(fd, current, SEEK_SET);
        if (read(fd, &c, 1) != 1)
            break;
        if (c == ' ')
            return current + 1; // include space in the previous chunk
        current++;
    }
    return max_offset; // fallback if no space found
}

int __load_file_mt(DB* db, char* filename){
    int fd = open(filename, O_RDONLY);
    if (fd<0) {
        LOG_ERROR("Error opening file %s",filename);
        return -1;
    }
    LOG_INFO("Loading file %s",filename);
    // clock_t start = clock();

    struct stat st;
    fstat(fd, &st);
    off_t file_size = st.st_size;

    LOG_INFO("File size  %s %d",filename, file_size);

    LOG_DEBUG("Finding dict");
    int dictIndex = -1;

    for(unsigned i=0;i<MAX_FILES;++i){
        pthread_mutex_lock(&(db->dictLocks[i]));
        if(db->dicts[i] != 0 && strcmp(filename, db->dicts[i]->filename) == 0){
            dictIndex = i;
            break;
        }else
            pthread_mutex_unlock(&(db->dictLocks[i]));
    }

    if(dictIndex == -1){
        LOG_DEBUG("Creating new dict");
        //new filename create a new entry
        char *nfilename = strdup(filename);
        dict* d = create_dict(nfilename);
        // insert in the first slot avaliable
        for(unsigned i=0;i<MAX_FILES;++i){
            pthread_mutex_lock(&(db->dictLocks[i]));
            if(db->dicts[i] == 0){
                db->dicts[i] = d;
                dictIndex = i;
                break;
            }else
                pthread_mutex_unlock(&(db->dictLocks[i]));
        }

        if(dictIndex == -1){
            LOG_WARNING("No space left for new dictionary. File is not possible to load.");
            free_dict(d);
            return -1;
        }
    }

    // FileReaderArgs args = {.fd = fd, .db = db->dicts[dictIndex], .id = 0, .size= file_size, .start = 0};
    // read_file_and_load( (void*)&args );

    // Multithreaded reading
    pthread_t threads[READ_THREADS];
    FileReaderArgs args[READ_THREADS];
    dict* temp_dicts[READ_THREADS];

    
    off_t starts[READ_THREADS + 1];
    starts[0] = 0;
    starts[READ_THREADS] = file_size;
    LOG_DEBUG("Read starts: %d\t",starts[0]);
    LOG_DEBUG("\t%d",starts[1]);
    for (int i = 1; i < READ_THREADS; ++i) {
        off_t tentative_end = file_size  / READ_THREADS * i;
        starts[i] = __find_next_space(fd, tentative_end, file_size);
        LOG_DEBUG("\t%d",starts[i]);
    }

    for (int i = 0; i < READ_THREADS; ++i) {
        off_t start = starts[i];
        off_t size = starts[i + 1] - start;

        char *nfilename = strdup(filename);
        temp_dicts[i] = create_dict(nfilename);
        args[i].fd = fd;
        args[i].start = start;
        args[i].size = size;
        args[i].db = temp_dicts[i];

        pthread_create(&threads[i], NULL, read_file_and_load, &args[i]);
    }

    for (int i = 0; i < READ_THREADS; ++i) {
        pthread_join(threads[i], NULL);
        merge_dict(db->dicts[dictIndex], temp_dicts[i]);
    }
    //resize 
    resize(db->dicts[dictIndex]);

    close(fd);
    pthread_mutex_unlock(&db->dictLocks[dictIndex]);

    // clock_t delta = clock() - start;
    // double time_spent = ((double)delta) / CLOCKS_PER_SEC;
    // printf("Time taken reading file: %.6f seconds\n", time_spent);

    return 1;
}


void __clear_db(DB* db){
    LOG_INFO("Clean db");
    for(unsigned i=0;i<MAX_FILES;++i){
        pthread_mutex_lock(&db->dictLocks[i]);
        if(db->dicts[i])
            free_dict(db->dicts[i]);
        pthread_mutex_unlock(&db->dictLocks[i]);
    }

}


int __query_file_db(DB* db,char* filename, char* word){
    int count = 0;
    LOG_DEBUG("Search word [%s] in file [%s]", word, filename);
    unsigned isAllFiles = strcmp(filename, "*") == 0;
    for(unsigned i=0;i<MAX_FILES;++i){
        pthread_mutex_lock(&(db->dictLocks[i]));
        if(db->dicts[i] && (isAllFiles || strcmp(db->dicts[i]->filename, filename ) == 0)){
            LOG_DEBUG("Dict found for %s", filename);
            int r = get_dict_dimension_value(db->dicts[i], word);
            if (r >= 0) {
                count += r;
            }
        }
        pthread_mutex_unlock(&(db->dictLocks[i]));
    }
    LOG_DEBUG("Dict search result for %s: %d", word, count);
    return count;
}

int execute_command(ParsedCommand *cmd, DB *db, int connfd){
    switch (cmd->type) {
        case CMD_CLEAR:
            __clear_db(db);
            write_to_socket_fmt(connfd,  "DB clean completed %s", END_TOKEN);
            return 1;
        case CMD_LOAD: {
            int r = __load_file_mt(db, cmd->filename);
            if (r<0){
                LOG_ERROR("Error loading file %s", cmd->filename);
                write_to_socket_fmt(connfd, "Error loading file %s %s", cmd->filename, END_TOKEN);
                return r;
            }
            write_to_socket_fmt(connfd, "Loading file %s completed %s", cmd->filename, END_TOKEN);
            LOG_INFO("Loading file %s completed ", cmd->filename);

            return r;
        }
        case CMD_UNLOAD: {
            int r = __unload_file(db, cmd->filename);
            if (r<0){
                LOG_ERROR("Error unloading file %s", cmd->filename);
                write_to_socket_fmt(connfd, "Error unloading file %s %s", cmd->filename, END_TOKEN);
                return r;
            }
            write_to_socket_fmt(connfd, "Unloading file %s completed %s", cmd->filename, END_TOKEN);
            LOG_INFO("Unloading file %s completed ", cmd->filename);
            return r;
        }
        case CMD_QUERY: {
            LOG_INFO("Query file %s ", cmd->word);
            int r = __query_file_db(db, cmd->filename, cmd->word);
            if (r<0){
                LOG_ERROR("Error query file %s", cmd->filename);
                write_to_socket_fmt(connfd, "Error query file %s %s", cmd->filename, END_TOKEN);
                return r;
            }
            write_to_socket_fmt(connfd, "%s: %d %s", cmd->word, r, END_TOKEN);
            LOG_INFO("Query file %s completed ", cmd->word);
            return r;
        }
        default:
            LOG_WARNING("Command %s not exeuted.", cmd->type);
            return -1;
    }

    return 0;
}