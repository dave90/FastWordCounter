#include "dict.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "log.h"

#define ALIGN_UP(p, align) (((p) + ((align)-1)) & ~((align)-1))


#if BLOOM == 1 
    #define __bloom(...) __bloom_filter(__VA_ARGS__)
#else
    #define __bloom(...) (1)
#endif

static inline uint32_t murmur_32_scramble(uint32_t k) {
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
}

uint32_t murmur3_32(const char* key, uint32_t seed)
{
    size_t len = strlen(key);
	uint32_t h = seed;
    uint32_t k;
    /* Read in groups of 4. */
    for (size_t i = len >> 2; i; i--) {
        // Here is a source of differing results across endiannesses.
        // A swap here has no effects on hash properties though.
        memcpy(&k, key, sizeof(uint32_t));
        key += sizeof(uint32_t);
        h ^= murmur_32_scramble(k);
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }
    /* Read the rest. */
    k = 0;
    for (size_t i = len & 3; i; i--) {
        k <<= 8;
        k |= key[i - 1];
    }
    // A swap is *not* necessary here because the preceding loop already
    // places the low bytes in the low places according to whatever endianness
    // we use. Swaps only apply when the memory is copied in a chunk.
    h ^= murmur_32_scramble(k);
    /* Finalize. */
	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}

//  djb2 by Dan Bernstein
hashType djb2(char * key){
    unsigned long hash = 5381;
    int c;

    while ( (c = *key++) )
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

uint64_t sdbm(const char* str) {
    uint64_t hash = 0;
    int c;
    while ((c = *str++))
        hash = c + (hash << 6) + (hash << 16) - hash;
    return hash;
}

#define __hash_key(k) murmur3_32(k, 12600123) // 12600123 magic number


dict* create_dict(char* filename){
    LOG_DEBUG("Creating dict %s",filename);
    dict *d = (dict*) malloc(sizeof(dict));
    d->filename = filename;
    d->used_size = 0;
    memset(d->bloom, 0, sizeof(uint8_t)*BLOOM_SIZE);
    d->elements = 0;
    #if SMALL_DICT_SIZE_BYTES > 0
        d->size = 0;
        d->mask = 0;
        d->smallHtable = malloc(SMALL_DICT_SIZE_BYTES);
        d->htable = 0;
    #else
        d->size = INIT_DICT_SIZE;
        d->mask = d->size -1; // size is a power of 2
        d->htable = (entryDict*) malloc(sizeof(entryDict) * INIT_DICT_SIZE); 
        memset(d->htable, 0, sizeof(entryDict) * INIT_DICT_SIZE);
        d->smallHtable = 0;
    #endif

    return d;
}

void __init_entry_dict(entryDict* e, char* key, hashType h, unsigned value){
    free(e->word);
    e->count = value;
    e->word = key;
    e->metadata = ((uint8_t)(h & 0x7F) ) | 0x80;;  // 0x7F == 01111111
}

void __find_free_bucket(dict* d, unsigned* i ){
    while(d->htable[*i].metadata & 0x80){
        // probe
        *i = (*i+1) & d->mask;
    }
}

unsigned __find_element(dict* d, unsigned* i, uint8_t metadata, char* key){
    while(d->htable[*i].metadata & 0x80){
        LOG_DEBUG("I: %d, meta: %d , res: %d", *i, d->htable[*i].metadata, d->htable[*i].metadata & 0x80);
        // compare with the hash
        // compare with the string
        if(d->htable[*i].metadata == metadata && strcmp(key, d->htable[*i].word) == 0 ) {
            return *i;
        }
        // probe
        *i =  (*i+1) & d->mask;
    }
    return d->size;
}

// bloom function, 0 element does not exist, 1 exist
unsigned __bloom_filter(uint8_t* bloom, char* key){
    uint64_t h1 = djb2(key) % (BLOOM_SIZE * 8);
    uint64_t h2 = sdbm(key) % (BLOOM_SIZE * 8);

    unsigned b1 = bloom[h1 / 8] & (1 << (h1 % 8));
    unsigned b2 = bloom[h2 / 8] & (1 << (h2 % 8));

    unsigned exists = b1 && b2;

    if (!exists) {
        bloom[h1 / 8] |= (1 << (h1 % 8));
        bloom[h2 / 8] |= (1 << (h2 % 8));
    }

    return exists;
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
    d->htable = (entryDict*) malloc(sizeof(entryDict) * INIT_DICT_SIZE); 
    memset(d->htable, 0, sizeof(entryDict) * INIT_DICT_SIZE);

    uintptr_t ptr = (uintptr_t) d->smallHtable;
    size_t *t;
    char * w;
    unsigned small_used_size = d->used_size;
    LOG_DEBUG("Copy %d data from small to htable",small_used_size);

    for(unsigned i=0;i<small_used_size;++i){
        t = (size_t *)ptr;
        w = strdup((char *)(t+2));
        LOG_DEBUG("Copy %s",w);

        hashType hash = __hash_key(w);
        unsigned index = hash & d->mask;
        __find_free_bucket(d, &index);
        __init_entry_dict( &(d->htable[index]), w, hash, t[0]);
        ++(d->used_size);
        ++(d->elements);
        resize(d);

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
    // if(d->elements % 5000 == 0) 
    // LOG_ERROR("Elements: %d, Size: %d, Factor: %f", d->elements, d->size,(double)d->elements / (double)d->size );

    LOG_DEBUG("Elements: %d, Size: %d, Factor: %f", d->elements, d->size,(double)d->elements / (double)d->size );
    if( ((double)d->elements / (double)d->size) < RESIZE_DICT_T){
        return;
    }
    LOG_DEBUG("Resize dict %s", d->filename);


    // resize dict
    entryDict *htableOld = d->htable;
    unsigned oldSize = d->size;

    d->size = d->size << 1; //power of 2
    d->htable = (entryDict*) malloc(sizeof(entryDict) * d->size); 
    memset(d->htable, 0, sizeof(entryDict) * d->size);
    d->mask = d->size -1;

    for(unsigned i=0;i<oldSize;++i){
        if(!( htableOld[i].metadata & 0x80)) continue;
        hashType hash = __hash_key(htableOld[i].word);
        unsigned index = hash & d->mask;
        __find_free_bucket(d, &index);
        __init_entry_dict( &(d->htable[index]), htableOld[i].word, hash, htableOld[i].count);
    }
    free(htableOld);

}


int dict_update(dict* d, char* key, unsigned value ){

    unsigned bloomCheck = __bloom( d->bloom, key );

    if(d->htable == 0){
        // if htable is null use the small htable
        LOG_DEBUG("Using small htable");
        int r = __insert_small_vec_element(d, key, value);
        if(r > 0)return 0;
        __move_to_htable(d);
    }
    LOG_DEBUG("Using htable");

    resize(d);

    hashType hash = __hash_key(key);
    unsigned i=hash & d->mask;
    LOG_DEBUG("Bloom check %d",bloomCheck);
    if(bloomCheck == 0 ){
        //first time found key
        LOG_DEBUG("Insert new node");
        __find_free_bucket(d, &i);
        __init_entry_dict( &(d->htable[i]),key, hash, value);
        ++(d->used_size);
        ++(d->elements);
        return 0;
    }

    uint8_t metadata = (uint8_t)(hash & 0x7F) | 0x80;
    unsigned res = __find_element(d, &i, metadata, key);
    LOG_DEBUG("Find element result: %d", res);
    if(res != d->size){
        d->htable[i].count += value;
        free(key);
        return 0;
    }
    ++(d->used_size);
    ++(d->elements);
    __init_entry_dict( &(d->htable[i]),key, hash, value);

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

    hashType hash = __hash_key(key);
    unsigned i=hash & d->mask;
    uint8_t metadata = (uint8_t)(hash & 0x7F) | 0x80;
    unsigned res = __find_element(d, &i, metadata, key);
    if(res != d->size){
        return d->htable[i].count;
    }

    return 0;
}

void free_dict(dict* d){
    LOG_DEBUG("Free dict %s", d->filename);
    free(d->filename);
    if(d->htable != 0){
        for(unsigned i=0;i<d->size;++i){
            LOG_DEBUG("Free bucket");
            free(d->htable[i].word);
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
            LOG_DEBUG("[%s:%zu]\t", w, t[0]);
            ptr += (sizeof(size_t)*2) + t[1] +1;
            ptr = ALIGN_UP(ptr, ALIGNMENT);
        }
        return;
    }

    for(unsigned i =0;i<d->size;++i){
        if(d->htable[i].metadata & 0x80){
            LOG_DEBUG("%d: [%s:%d]\t",i, d->htable[i].word, d->htable[i].count);
        }
    }
}


void merge_dict(dict* final, dict* d){
    //init final htable if is none
    if(final->htable == 0 || final->size == INIT_DICT_SIZE){
        final->size = (INIT_DICT_SIZE > d->size)?INIT_DICT_SIZE:d->size;
        final->size = final->size << 1 ;
        final->mask = final->size -1; // size is a power of 2
        final->htable = (entryDict*) malloc(sizeof(entryDict) * final->size); 
        memset(final->htable, 0, sizeof(entryDict) * final->size);
        final->elements = 0;
        final->used_size = 0;
    }

    if(d->htable == 0){
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

        free(d->smallHtable);
        free(d->filename);
        free(d);
    
        return;
    }

    for(unsigned i =0;i<d->size;++i){
        if(!( d->htable[i].metadata & 0x80)) continue;

        __bloom( final->bloom, d->htable[i].word );
        hashType hash = __hash_key(d->htable[i].word);
        unsigned fi=hash & final->mask;        
        unsigned res = __find_element(final, &fi, d->htable[i].metadata, d->htable[i].word );
        if(res == final->size){
            __init_entry_dict( &(final->htable[fi]), d->htable[i].word, hash, d->htable[i].count);
            ++(final->used_size);
            ++(final->elements);
            resize(final);
        }else {
            final->htable[fi].count += d->htable[i].count;
            free(d->htable[i].word);
        }
    }

    // destroy d
    LOG_DEBUG("Free dict %s completed", d->filename);

    free(d->filename);
    free(d->htable);
    free(d->smallHtable);
    free(d);

}
