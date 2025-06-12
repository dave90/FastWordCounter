#pragma once

#include <stdint.h>
#include <stdlib.h>

#include "constants.h"

typedef size_t hashType;

typedef struct entryDictStruct {
    char *word;
    unsigned count;
    uint8_t metadata; // metadata of the entry, first bit if it is free, 7 bits first part of the hash
} entryDict;

typedef struct {
    char * filename;
    entryDict *htable;
    void* smallHtable;
    unsigned size;
    unsigned used_size;
    unsigned elements;
    unsigned mask;
    uint8_t bloom[BLOOM_SIZE];
} dict;

dict* create_dict(char* filename);
void resize(dict* d);
int dict_update(dict* d, char* key, unsigned value );
int get_dict_dimension_value(dict* d, char* key);
void free_dict(dict* d);
void print_dict(dict* d);
void merge_dict(dict* final, dict* d);
