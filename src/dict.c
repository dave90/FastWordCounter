#include "dict.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "constants.h"
#include "log.h"

#define ALIGN_UP(p, align) (((p) + ((align)-1)) & ~((align)-1))


#if BLOOM == 1 
    #define __bloom(...) __bloom_filter(__VA_ARGS__)
#else
    #define __bloom(...) (1)
#endif


//  djb2 by Dan Bernstein
hashType __hash_key(char * key){
    unsigned long hash = 5381;
    int c;

    while ( (c = *key++) )
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

uint64_t  __bloom_hash(char * key){
    uint64_t hash = 0;
    int c;

    while ( (c = *key++))
        hash = c + (hash << 6) + (hash << 16) - hash;

    return hash;
}

dict* create_dict(char* filename){
    LOG_DEBUG("Creating dict %s",filename);
    dict *d = (dict*) malloc(sizeof(dict));
    d->filename = filename;
    d->size = 0;
    d->mask = 0;
    d->used_size = 0;
    d->smallHtable = malloc(SMALL_DICT_SIZE_BYTES);
    d->htable = 0;
    d->bloom = 0;
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
unsigned __bloom_filter(uint64_t* bloom, char* key){
    uint64_t hash = __bloom_hash(key) % 32;
    unsigned i = (1 << hash) & *bloom;
    if(i==0){
        //set to 1
        *bloom |= ((1 << hash));
    }
    return i;
}




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


int __insert_small_vec_element(dict* d, char* key, unsigned value ){
    // search into the vector if the key exist
    size_t len = strlen(key);
    uintptr_t ptr = (uintptr_t) d->smallHtable;
    size_t *t;
    char * w;

    for(unsigned i=0;i<d->used_size;++i){
        t = (size_t *)ptr;
        w = (char *)(t+2);
        if(t[1] == len && strcmp(w,key) == 0){
            // element found update the value
            t[0] += (size_t)value;
            free(key);
            return 1;
        }
        ptr += (sizeof(size_t)*2) + t[1] +1;
        ptr = ALIGN_UP(ptr, ALIGNMENT);
    }
    // check if there is space
    uintptr_t nextPtr = ptr + (sizeof(size_t)*2) + len + 1;
    nextPtr = ALIGN_UP(nextPtr, ALIGNMENT);
    if(nextPtr - ( (uintptr_t) d->smallHtable)  >= SMALL_DICT_SIZE_BYTES){
        return -1;
    }

    // insert the new string
    t = (size_t *)ptr;
    w = (char*) (t+2); 
    t[0] = (size_t) value;
    t[1] = len;
    memcpy(w,key,len+1);
    free(key);
    d->used_size += 1;
    return 1;
}

void __move_to_htable(dict *d){
    LOG_DEBUG("Move from small to htable");
    d->size = INIT_DICT_SIZE;
    d->mask = d->size -1; // size is a power of 2
    d->htable = (entryDict**) malloc(sizeof(entryDict*) * INIT_DICT_SIZE); 
    memset(d->htable, 0, sizeof(entryDict*) * INIT_DICT_SIZE);

    uintptr_t ptr = (uintptr_t) d->smallHtable;
    size_t *t;
    char * w;
    unsigned small_used_size = d->used_size;
    LOG_DEBUG("Copy %d data from small to htable",small_used_size);

    for(unsigned i=0;i<small_used_size;++i){
        t = (size_t *)ptr;
        w = strdup((char *)(t+2));
        LOG_DEBUG("Copy %s",w);

        hashType hash = __hash_key(w) & d->mask;
        __insert_dict_element(d, hash, w ,t[0]);
        ptr += (sizeof(size_t)*2) + t[1] +1;
        ptr = ALIGN_UP(ptr, ALIGNMENT);
    }

    LOG_DEBUG("Call a resize");
    resize(d);
    // free small table
    LOG_DEBUG("Free small table");
    free(d->smallHtable);
    d->smallHtable = 0;
}


void resize(dict* d){
    if( (d->elements / d->size) < RESIZE_DICT_T){
        return;
    }
    LOG_DEBUG("Resize dict %s", d->filename);
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

    unsigned bloomCheck = __bloom( &(d->bloom), key );

    if(d->htable == 0){
        // if htable is null use the small htable
        LOG_DEBUG("Using small htable");
        int r = __insert_small_vec_element(d, key, value);
        if(r > 0)return 0;
        __move_to_htable(d);
    }
    LOG_DEBUG("Using htable");

    resize(d);

    hashType hash = __hash_key(key) & d->mask;
    if(bloomCheck == 0 ){
        //first time found key
        LOG_DEBUG("Insert new node");
        __insert_dict_element(d, hash, key, value);
        return 0;
    }

    
    // find the element
    entryDict* ed = d->htable[hash];

    // find if exist word
    while(ed){
        if(strcmp(ed->word, key)==0){
            LOG_DEBUG("Update node");
            //update the value and exit
            ed->count += value;
            free(key); // free key already exist in dictionary
            return 1;
        }
        ed = ed->next;
    }
    LOG_DEBUG("Insert new node");
    __insert_dict_element(d, hash, key, value);
    return 0;
}

int get_dict_dimension_value(dict* d, char* key){

    if(d->htable == 0){
        // search in the small htable
        uintptr_t ptr = (uintptr_t)d->smallHtable;
        size_t *t;
        char * w;
        size_t len = strlen(key);
        for(unsigned i=0;i<d->used_size;++i){
            t = (size_t*) (ptr);
            w =  (char*)(t+2);
            if( t[1] == len && strcmp(w,key) == 0) {
                return t[0];
            }
            ptr += (sizeof(size_t)*2) + t[1] +1;
            ptr = ALIGN_UP(ptr, ALIGNMENT);
        }
        return 0;
    }

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
    if(d->htable != 0){
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
    }
    LOG_DEBUG("Free dict %s completed", d->filename);
    free(d->htable);
    free(d->smallHtable);

    free(d);

}

void print_dict(dict* d){
    if(d->htable == 0){
        // print in the small htable
        uintptr_t ptr = (uintptr_t)d->smallHtable;
        size_t *t;
        char * w;
        for(unsigned i=0;i<d->used_size;++i){
            t = (size_t*) (ptr);
            w =  (char*)(t+2);
            printf("[%s:%zu]\t\n", w, t[0]);
            ptr += (sizeof(size_t)*2) + t[1] +1;
            ptr = ALIGN_UP(ptr, ALIGNMENT);
        }
        return;
    }

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



void merge_dict(dict* final, dict* d){

    //init final htable if is none
    if(final->htable == 0){
        final->size = (INIT_DICT_SIZE > d->size)?INIT_DICT_SIZE:d->size;
        final->size = final->size * READ_THREADS;
        final->mask = final->size -1; // size is a power of 2
        final->htable = (entryDict**) malloc(sizeof(entryDict*) * final->size); 
        memset(final->htable, 0, sizeof(entryDict*) * final->size);
    }

    if(d->htable == 0){
        LOG_WARNING("IS A SMALL TABLE");
        // iterate in small table
        uintptr_t ptr = (uintptr_t)d->smallHtable;
        size_t *t;
        char * w;
        for(unsigned i=0;i<d->used_size;++i){
            t = (size_t*) (ptr);
            w =  (char*)(t+2);
            dict_update(final, strdup(w), (unsigned)(t[0]));

            ptr += (sizeof(size_t)*2) + t[1] +1;
            ptr = ALIGN_UP(ptr, ALIGNMENT);
        }
        LOG_WARNING("FREE D");

        free(d->smallHtable);
        free(d->filename);
        free(d);
    
        return;
    }

    // merge using htable
    entryDict *ced; // current entry in htable
    entryDict *ned; // next entry in htable
    for(unsigned i =0;i<d->size;++i){
        ced = d->htable[i];
        if(ced == 0)continue;
        
        while(ced){
            ned = ced->next;
            hashType hash = __hash_key(ced->word) & final->mask;
            entryDict* ed = final->htable[hash];
            __bloom( &(final->bloom), ced->word );

            while(ed){
                if(strcmp(ed->word, ced->word)==0){
                    //update the value and exit
                    ed->count += ced->count;
                    free(ced->word);
                    free(ced);
                    ced = 0;
                    break;
                }
                ed = ed->next;
            }
            // if ced is not 0 this node is not present so append it 
            if(ced){
                ed = final->htable[hash];
                final->htable[hash] = ced;
                ced->next = ed;
            }
            ced = ned;
        }
    }

    // destroy d
    LOG_DEBUG("Free dict %s completed", d->filename);
    free(d->filename);
    free(d->htable);
    free(d->smallHtable);
    free(d);

}
