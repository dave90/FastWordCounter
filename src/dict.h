#pragma once

#include <stdint.h>

typedef unsigned long hashType;

typedef struct entryDictStruct {
    struct entryDictStruct *next;
    char *word;
    unsigned count;
} entryDict;

typedef struct {
    char * filename;
    entryDict **htable;
    unsigned size;
    unsigned used_size;
    unsigned elements;
    unsigned mask;
    int32_t bloom;
} dict;

dict* create_dict(char* filename);
void resize(dict* d);
int dict_update(dict* d, char* key, unsigned value );
int get_dict_dimension_value(dict* d, char* key);
void free_dict(dict* d);
void print_dict(dict* d);
