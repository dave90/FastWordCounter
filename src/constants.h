#pragma once

#define LOG_LVL_DEBUG 0
#define LOG_LVL_INFO 1
#define LOG_LVL_WARNING 2
#define LOG_LVL_ERROR 3

// -------------- Can be updated --------------
#ifndef LOG_LEVEL
    #define LOG_LEVEL LOG_LVL_DEBUG 
#endif

#define MAX_INPUT 1024 
#ifndef INIT_DICT_SIZE
    #define INIT_DICT_SIZE 1024  
#endif
#ifndef SMALL_DICT_SIZE_BYTES
    #define SMALL_DICT_SIZE_BYTES 256*(256+32+32)
#endif
#ifndef BLOOM
    #define BLOOM 1
#endif
#ifndef READ_THREADS
    #define READ_THREADS 5
#endif
#define RESIZE_DICT_T 0.8
#define MAX_FILES 1024
#define READ_WORD_SIZE 256
#define DEFAULT_PORT 8124
#define READ_SOCKET_BUFFER_SIZE 1024
#define END_TOKEN "END."
#define DEFAULT_ADDR "127.0.0.1"
#define ALIGNMENT sizeof(size_t)

