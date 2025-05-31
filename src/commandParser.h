#pragma once

#include "constants.h"

typedef enum {
    CMD_INVALID,
    CMD_CLEAR,
    CMD_LOAD,
    CMD_UNLOAD,
    CMD_QUERY,
    CMD_EXIT
} CommandType;

typedef struct {
    CommandType type;
    char *filename;
    char *word;
} ParsedCommand;


int parse_command(const char* input, ParsedCommand* cmd);
void free_cmd(ParsedCommand* cmd);