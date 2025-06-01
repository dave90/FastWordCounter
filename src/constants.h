#pragma once

#define LOG_LVL_DEBUG 0
#define LOG_LVL_INFO 1
#define LOG_LVL_WARNING 2
#define LOG_LVL_ERROR 3

// -------------- Can be updated --------------
#ifndef LOG_LEVEL
    #define LOG_LEVEL LOG_LVL_DEBUG  // default if not overridden
#endif

#define MAX_INPUT 1024 
#define INIT_DICT_SIZE 1024 
#define BLOOM 0
#define RESIZE_DICT_T 0.8
#define MAX_FILES 1024
#define READ_WORD_SIZE 256
#define DEFAULT_PORT 8124
#define READ_SOCKET_BUFFER_SIZE 1024
#define END_TOKEN "END."
#define DEFAULT_ADDR "127.0.0.1"
