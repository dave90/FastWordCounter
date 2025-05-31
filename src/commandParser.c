#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "commandParser.h"
#include "log.h"

int parse_command(const char* input, ParsedCommand* cmd) {
    if (!input || !cmd) return -1;

    cmd->filename = 0;
    cmd->word = 0;

    // Clear existing content
    memset(cmd, 0, sizeof(ParsedCommand));

    char buffer[READ_SOCKET_BUFFER_SIZE];
    
    strncpy(buffer, input, sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';
    
    char* token = strtok(buffer, " \t\n");
    if (!token) return -1;

    if (strcmp(token, "clear") == 0) {
        cmd->type = CMD_CLEAR;
        return 0;
    }  else if (strcmp(token, "exit") == 0) {
        cmd->type = CMD_EXIT;
        return 0;
    } else if (strcmp(token, "load") == 0) {
        token = strtok(NULL, " \t\n");
        if (!token) return -1;
        cmd->type = CMD_LOAD;
        int len = strlen(token);
        cmd->filename = (char*) malloc((len+1) * sizeof(char));
        strncpy(cmd->filename, token, len);
        return 0;
    } else if (strcmp(token, "unload") == 0) {
        token = strtok(NULL, " \t\n");
        if (!token) return -1;
        cmd->type = CMD_UNLOAD;
        int len = strlen(token);
        cmd->filename = (char*) malloc((len+1) * sizeof(char));
        strncpy(cmd->filename, token, len);
        return 0;
    } else if (strcmp(token, "query") == 0) {
        token = strtok(NULL, " \t\n");
        if (!token) return -1;
        int len = strlen(token);
        cmd->filename = (char*) malloc((len+1) * sizeof(char));
        strncpy(cmd->filename, token, len);

        token = strtok(NULL, " \t\n");     
        if (!token) return -1;
        len = strlen(token);
        cmd->word = (char*) malloc((len+1) * sizeof(char));
        strncpy(cmd->word, token, len);

        cmd->type = CMD_QUERY;
        return 0;
    }

    cmd->type = CMD_INVALID;
    return -1;
}

void free_cmd(ParsedCommand *p){
    free(p->filename);
    free(p->word);
}