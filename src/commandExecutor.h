#pragma once

#include "dict.h"
#include "constants.h"
#include "commandParser.h"
#include <pthread.h>

typedef struct {
    dict* dicts[MAX_FILES];

    // lock of element inside dicts: modify dicts[i]
    pthread_mutex_t dictLocks[MAX_FILES];
} DB;


DB* create_db();
void free_db(DB* db);
int execute_command(ParsedCommand *cmd, DB *db, int connfd);
