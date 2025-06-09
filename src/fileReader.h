#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "dict.h"

typedef struct {
    int fd;
    off_t start;
    size_t size;
    int id;
    dict *db;
} FileReaderArgs;

void* read_file_and_load(void* args);
