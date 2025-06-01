#include "dict.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "constants.h"
#include "log.h"

//  djb2 by Dan Bernstein
hashType __hash_key(char * key){
    unsigned long hash = 5381;
    int c;

    while ( (c = *key++) )
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

hashType  __bloom_hash(char * key){
    unsigned long hash = 0;
    int c;

    while ( (c = *key++))
        hash = c + (hash << 6) + (hash << 16) - hash;

    return hash;
}

dict* create_dict(char* filename){
    dict *d = (dict*) malloc(sizeof(dict));
    d->filename = filename;
    d->size = INIT_DICT_SIZE;
    d->mask = d->size -1; // size is a power of 2
    d->used_size = 0;
    d->htable = (entryDict**) malloc(sizeof(entryDict*) * INIT_DICT_SIZE); 
    d->bloom = 0;
    memset(d->htable, 0, sizeof(entryDict*) * INIT_DICT_SIZE);
    return d;
}

entryDict* __create_entry_dict(char* key, unsigned value){
    entryDict* e = (entryDict*) malloc(sizeof(entryDict));
    e->count = value;
    e->word = key;
    e->next = 0;
    return e;
}

// bloom function, 0 element does not exist, 1 exist
unsigned __bloom_filter(int* bloom, char* key){
    hashType hash = __bloom_hash(key) % 32;
    unsigned i = (1 << hash) & *bloom;
    if(i==0){
        //set to 1
        *bloom |= ((1 << hash));
    }
    return i;
}


#if BLOOM == 1 
    #define __bloom(...) __bloom_filter(__VA_ARGS__)
#else
    #define __bloom(...) (1)
#endif


void __insert_dict_element(dict* d, unsigned hash, char* key, unsigned value ){
    entryDict *ned = __create_entry_dict(key, value);
    entryDict *ced = d->htable[hash];
    ++(d->elements);
    if(ced == 0){
        d->htable[hash] = ned;
        ++(d->used_size);
        return;
    }
    d->htable[hash] = ned;
    ned->next = ced;
}

void resize(dict* d){
    if( (d->elements / d->size) < RESIZE_DICT_T){
        return;
    }
    // resize dict
    entryDict **htableOld = d->htable;
    unsigned oldSize = d->size;

    d->size = d->size << 1; //power of 2
    d->htable = (entryDict**) malloc(sizeof(entryDict*) * d->size); 
    memset(d->htable, 0, sizeof(entryDict*) * d->size);
    d->used_size = 0;
    d->mask = d->size -1;
    entryDict* oed; // old entry        
    entryDict* ned ;  // next old entry
    entryDict *ced; // current entry in htable

    for(unsigned i =0;i<oldSize;++i){
        oed = htableOld[i];
        while(oed){
            ned = oed->next;  
            hashType hash = __hash_key(oed->word) & d->mask;
            ced = d->htable[hash];
            if(ced == 0){
                // new entry in new table
                d->htable[hash] = oed;
                oed->next = 0;
                ++(d->used_size);
            }else{
                // replace first position
                d->htable[hash] = oed;
                oed->next = ced;
            }
            oed = ned; // next element
        }
    }
    free(htableOld);
}

int dict_update(dict* d, char* key, unsigned value ){
    resize(d);

    hashType hash = __hash_key(key) & d->mask;

    if(__bloom( &(d->bloom), key ) == 0 ){
        //first time found key
        __insert_dict_element(d, hash, key, value);
        return 0;
    }

    
    // find the element
    entryDict* ed = d->htable[hash];

    // find if exist word
    while(ed){
        if(strcmp(ed->word, key)==0){
            //update the value and exit
            ed->count += value;
            free(key); // free key already exist in dictionary
            return 1;
        }
        ed = ed->next;
    }
    __insert_dict_element(d, hash, key, value);
    return 0;
}

int get_dict_dimension_value(dict* d, char* key){
    hashType hash = __hash_key(key) & d->mask;
    entryDict* ed = d->htable[hash];

    // find if exist word
    while(ed){
        if(strcmp(ed->word, key)==0){
            //update the value and exit
            return ed->count;
        }
        ed = ed->next;
    }
    return 0;
}

void free_dict(dict* d){
    LOG_DEBUG("Free dict %s", d->filename);
    free(d->filename);
    entryDict *ced;
    entryDict *ned;
    for(unsigned i=0;i<d->size;++i){
        LOG_DEBUG("Free bucket");
        ced = d->htable[i];
        while(ced){
            ned = ced->next;
            LOG_DEBUG("Free node");
            free(ced->word);
            free(ced);
            ced = ned;
        }
    }
    LOG_DEBUG("Free dict %s completed", d->filename);
    free(d->htable);
    free(d);

}

void print_dict(dict* d){

    entryDict *ced; // current entry in htable

    for(unsigned i =0;i<d->size;++i){
        ced = d->htable[i];
        printf("%d: ",i);
        while(ced){
            printf("[%s:%d]\t", ced->word, ced->count);
            ced = ced->next;
        }
        printf("\n");
    }
}