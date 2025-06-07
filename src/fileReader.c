#include "fileReader.h"

#include <ctype.h>
#include <string.h>

#include "constants.h"
#include "log.h"


void* read_file_and_load(void* args){
    FileReaderArgs* fra = (FileReaderArgs*) args;    

    char word[READ_WORD_SIZE+1]; // +1 for the null character at the end 
    char buffer[READ_WORD_SIZE+1];
    size_t bytes_remaining = fra->size;
    off_t offset = fra->start;
    int index = 0;

    while(bytes_remaining > 0){
        size_t to_read = (READ_WORD_SIZE > bytes_remaining)? bytes_remaining : READ_WORD_SIZE;
        pread(fra->fd, buffer, to_read, offset);
        buffer[to_read] = '\0';
        for(unsigned i=0;i<to_read;++i) {
            char ch = buffer[i];
            if (isspace(ch)) {
                if (index > 0) {
                    word[index] = '\0';
                    LOG_DEBUG("Adding word %s",word);
                    char * key = strdup(word);
                    dict_update(fra->db, key, 1);
                    index = 0;
                }
            } else if (index < READ_WORD_SIZE - 1) {
                word[index++] = (char)ch;
            } else {
                // Word too long; discard rest of it safely
                while (i < to_read && (ch = buffer[i++]) != '\0' && !isspace(ch));
                LOG_WARNING("Truncated %s...\n", word);  // truncated word
                word[index] = '\0';
                dict_update(fra->db, word, 1);
                index = 0;
            }
        }

        offset += to_read;
        bytes_remaining -= to_read;
    }

    // insert last word parsed
    if(index != 0){
        word[index] = '\0';
        char * key = strdup(word);
        dict_update(fra->db, key, 1);
    }

    return NULL;

}