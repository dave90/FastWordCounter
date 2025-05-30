#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "log.h"
#include "constants.h"
#include "network.h"

typedef struct {
    char addr[125];
    int port;
    int print;
} input_args;


void extract_address(char *str, char* addr, int* port){
    char *token = strtok(str, ":");

    if (token != NULL) {
        printf("Address: %s\n", token);
        strcpy(addr, token);

        // Get the second token (after the colon)
        token = strtok(NULL, ":");
        if (token != NULL) {
            *port = atoi(token);
            printf("Port: %d\n", *port);
        }else{
            printf("Port not parsed\n");
            exit(1);
        }
    }
}


input_args parse_args(int argc, char** argv){

    input_args ia;

    if(argc > 1)
        extract_address(argv[1], ia.addr, &(ia.port));
    else{
        strcpy(ia.addr, DEFAULT_ADDR);
        ia.port = DEFAULT_PORT;
        ia.print = 1;
    }
    return ia;
}

int main(int argc, char** argv){
    init_logger("log_cli.txt", 0);
    input_args ia = parse_args(argc, argv);
    LOG_INFO("Connecting to the server %s:%d", ia.addr, ia.port);
    int sockfd = connect_to_server(ia.port, ia.addr);
    if(sockfd < 0){
        LOG_ERROR("Connection to the server failed");
        if(ia.print)printf("Connection to the server failed\n");
        exit(1);
    }

    int ch;
    char buffer[READ_SOCKET_BUFFER_SIZE];
    int buf_index = 0;

    while (1) {

        while ((ch = getchar()) != EOF) {
            if(ch == '\n')ch = ' ';
            buffer[buf_index++] = ch;
            if(ch == ';'){
                // replace with END TOKEN
                buffer[buf_index-1] = '\0';
                strcat(buffer,END_TOKEN);
                LOG_DEBUG("Send to socket: %s", buffer);
                write_to_socket(sockfd, buffer);
                LOG_DEBUG("Waiting response");
                int c = read_from_socket(sockfd, buffer, READ_SOCKET_BUFFER_SIZE -1);
                if(c<1){
                    LOG_ERROR("Server closed connection :(");
                    printf("Server closed connection :(");
                }
                remove_end_token(buffer, c);
                LOG_DEBUG("Server response: %s",buffer);
                printf("%s\n",buffer);
                buf_index = 0;
                fflush(stdout);
                break;
            }
            if(buf_index >= READ_SOCKET_BUFFER_SIZE - 1){
                // reaching buffer send to socket
                LOG_WARNING("Warning command too long, comand was not sent.");
                printf("Warning command too long, comand was not sent.");
                fflush(stdout);
                buf_index = 0;
                break;
            }
        }
    }


    close_logger();
    return 0;
}