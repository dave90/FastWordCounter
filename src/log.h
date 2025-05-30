#pragma once

#include <stdio.h>


#if LOG_LEVEL <= LOG_LVL_DEBUG 
    #define LOG_DEBUG(...) log_message(__VA_ARGS__)
#else
    #define LOG_DEBUG(...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LVL_INFO 
    #define LOG_INFO(...) log_message(__VA_ARGS__)
#else
    #define LOG_INFO(...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LVL_WARNING
    #define LOG_WARNING(...) log_message(__VA_ARGS__)
#else
    #define LOG_WARNING(...) ((void)0)
#endif

#if LOG_LEVEL <= LOG_LVL_ERROR 
    #define LOG_ERROR(...) log_message(__VA_ARGS__)
#else
    #define LOG_ERROR(...) ((void)0)
#endif


// Initializes the logger with a file path
int init_logger(const char* filename, int print);

// Writes a message to the log file
void log_message(const char* format, ...);

// Closes the log file
void close_logger();

