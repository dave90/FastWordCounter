#pragma once
#include <netinet/in.h> 

#define SA struct sockaddr 
#define SAI struct sockaddr_in 


int start_server(unsigned port, int* sockfdp);
unsigned read_from_socket(int connfd, char* buff, unsigned size_buff);
int write_to_socket(int connfd, char* buff);
int write_to_socket_fmt(int connfd, const char* format, ...);
int connect_to_server(int port, char* addr);
void close_connection(int connfd);
void remove_end_token(char* buffer, int size);