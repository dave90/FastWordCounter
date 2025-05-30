#pragma once

#include "dict.h"
#include "constants.h"
#include "commandParser.h"

typedef struct {
    dict* dicts[MAX_FILES];
    unsigned size;
} DB;


DB* create_db();
void free_db(DB* db);
int execute_command(ParsedCommand *cmd, DB *db, int connfd);
