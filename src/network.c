
#include <sys/socket.h> 
#include <stdio.h> 
#include <arpa/inet.h> // inet_addr()
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> // read(), write(), close()
#include <pthread.h>
#include <stdarg.h>

#include "network.h"
#include "log.h"
#include "constants.h"

int start_server(unsigned port, int* sockfdp){
    LOG_INFO("Starting server...\n");

    int sockfd; 
    struct sockaddr_in servaddr; 
  
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        LOG_ERROR("socket creation failed...\n"); 
        return -1;
    } 
    LOG_INFO("Socket successfully created..\n"); 

    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(port); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        LOG_ERROR("socket bind failed...\n"); 
        return -1;
    } 
    LOG_INFO("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        LOG_ERROR("Listen failed...\n"); 
        return -1;
    } 
    LOG_INFO("Server listening..\n"); 
    *sockfdp = sockfd;

    return 1;

}



unsigned read_from_socket(int connfd, char* buff, unsigned size_buff){
    int n = read(connfd, buff, size_buff);
    if (n <= 0) {
        return -1;
    }
    buff[n] = '\0';    
    if (n <= 0) {
        LOG_WARNING("Client disconnected.\n");
        return -1;
    }
    LOG_DEBUG("Read from %d: %s\n", connfd, buff);
    return n;
}



int write_to_socket(int connfd, char* buff){
    write(connfd, buff, strlen(buff));
    LOG_DEBUG("Sent to %d: %s\n", connfd, buff);
    return 1;
}

int write_to_socket_fmt(int connfd, const char* format, ...){
    va_list args;
    va_start(args, format);
    char toSend[READ_SOCKET_BUFFER_SIZE];
    vsnprintf(toSend, sizeof(toSend), format, args);

    return write_to_socket(connfd, toSend);
}


int connect_to_server(int port, char* addr){
    int sockfd;
    struct sockaddr_in servaddr;
    LOG_INFO("Connecting to address %s:\n", addr);

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        LOG_ERROR("socket creation failed...\n");
        return -1;
    }
    LOG_INFO("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(addr);
    servaddr.sin_port = htons(port);

    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        LOG_ERROR("connection with the server failed...\n");
        return -1;
    }
    LOG_INFO("connected to the server!\n");

    return sockfd;
}

void close_connection(int connfd){
    close(connfd);
}



void remove_end_token(char* buffer, int size){
    int endTokenLen = strlen(END_TOKEN);
    if (size >= endTokenLen && strcmp(buffer + size - endTokenLen, END_TOKEN) == 0) {
        buffer[size - endTokenLen] = '\0';
    }
}