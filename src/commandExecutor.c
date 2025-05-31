#include "commandExecutor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "log.h"
#include "network.h"

DB* create_db(){
    DB *db = (DB*) malloc(sizeof(DB));
    memset(db->dicts, 0, MAX_FILES*sizeof(dict*));
    return db;
}


void free_db(DB* db){
    for(unsigned i=0;i<MAX_FILES;++i){
        LOG_INFO("Free dict %d", i);
        if(db->dicts[i] != 0)
            free_dict(db->dicts[i]);
    }
    LOG_INFO("Free db ");
    free(db);
    return;
}

int __unload_file(DB* db, char* filename){
    int d = 0;
    for(unsigned i=0;i<MAX_FILES;++i){
        if(db->dicts[i] != 0 && strcmp(filename, db->dicts[i]->filename) == 0){
           d = 1;
           free_dict(db->dicts[i]);
           db->dicts[i] = 0;
        }
    }
    if(d == 0){
        LOG_DEBUG("Filename not found %s", filename);
        return -1;
    }

    return 1;
}

int __load_file(DB* db, char* filename){
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        LOG_ERROR("Error opening file %s",filename);
        return -1;
    }
    LOG_INFO("Loading file %s",filename);


    LOG_DEBUG("Finding dict");
    dict* d = 0;
    for(unsigned i=0;i<MAX_FILES;++i){
        if(db->dicts[i] != 0 && strcmp(filename, db->dicts[i]->filename) == 0){
           d =  db->dicts[i];
           break;
        }
    }
    if(d == 0){
        LOG_DEBUG("Creating new dict");
        //new filename create a new entry
        char *nfilename = strdup(filename);
        d = create_dict(nfilename);
        // insert in the first slot avaliable
        for(unsigned i=0;i<MAX_FILES;++i)
            if(db->dicts[i] == 0){
                db->dicts[i] = d;
                break;
            }
    }

    char word[READ_WORD_SIZE]; 
    int ch, index = 0;

    while ((ch = fgetc(fp)) != EOF) {
        if (isspace(ch)) {
            if (index > 0) {
                word[index] = '\0';
                LOG_DEBUG("Adding word %s",word);
                char * key = strdup(word);
                dict_update(d, key, 1);
                index = 0;
            }
        } else if (index < READ_WORD_SIZE - 1) {
            word[index++] = (char)ch;
        } else {
            // Word too long; discard rest of it safely
            while ((ch = fgetc(fp)) != EOF && !isspace(ch));
            word[sizeof(word) - 1] = '\0';
            LOG_WARNING("Truncated %s...\n", word);  // truncated word
            index = 0;
            dict_update(d, word, 1);
        }
    }
    // insert last word parsed
    if(index != 0){
        word[index] = '\0';
        char * key = strdup(word);
        dict_update(d, key, 1);
    }
    print_dict(d);

    fclose(fp);
    return 1;
} 

void __clear_db(DB* db){
    LOG_INFO("Clean db");
    for(unsigned i=0;i<MAX_FILES;++i){
        if(db->dicts[i])
            free_dict(db->dicts[i]);
    }
}

int __query_file_db(DB* db,char* filename, char* word){
    int count = 0;
    LOG_DEBUG("Search word [%s] in file [%s]", word, filename);
    unsigned isAllFiles = strcmp(filename, "*") == 0;
    for(unsigned i=0;i<MAX_FILES;++i){
        if(db->dicts[i] && (isAllFiles || strcmp(db->dicts[i]->filename, filename ) == 0)){
            LOG_DEBUG("Dict found for %s", filename);
            int r = get_dict_dimension_value(db->dicts[i], word);
            if (r >= 0) {
                count += r;
            }
        }
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
            int r = __load_file(db, cmd->filename);
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