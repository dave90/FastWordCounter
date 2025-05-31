#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "commandParser.h"
#include "commandExecutor.h"
#include "log.h"
#include "constants.h"
#include "network.h"

DB *db;

void* parse_and_execute_commands(void* connfd_ptr){
    int connfd = *((int*)connfd_ptr);
    free(connfd_ptr);

    char *input =(char*) malloc(sizeof(char) * READ_SOCKET_BUFFER_SIZE);
    int size = READ_SOCKET_BUFFER_SIZE;

    ParsedCommand cmd = {CMD_INVALID, 0 ,0};

    while(1){

        int c = read_from_socket(connfd, input, size -1);
        if (c < 1) {
            LOG_WARNING("User close connection :(");
            break;
        }
        remove_end_token(input, c);

        LOG_DEBUG("Reading: %s", input);

        LOG_DEBUG("Parsing command [%s]...", input);

        int r = parse_command(input, &cmd);
        if(r<0){
            LOG_WARNING("Warning command [%s] not parsed", input);
            write_to_socket_fmt(connfd,  "Command not recognized: %s %s", input, END_TOKEN);
            free_cmd(&cmd);
            continue;
        }
        if(cmd.type == CMD_EXIT){
            LOG_INFO("User exit the connection");
            break;
        }
        execute_command(&cmd, db, connfd);
        free_cmd(&cmd);

    }
    free(input);
    return NULL;
}

void clients_loop(int sockfd){
    socklen_t len;
    SAI cli; 
    while(1){
        // Accept the data packet from client and verification 
        int *connfd = (int *) malloc(sizeof(int));
        LOG_INFO("Waiting client...\n"); 
        *connfd = accept(sockfd, (SA*)&cli, &len); 

        if (*connfd < 0) { 
            LOG_ERROR("server accept failed...\n"); 
            free(connfd);
            continue;
        } 
        LOG_INFO("server accept the client...\n"); 

        pthread_t tid;
        pthread_create(&tid, NULL, parse_and_execute_commands, connfd);
        pthread_detach(tid);
    }
}

int main(){
    init_logger("log.txt", 1);
    LOG_INFO("Init server...");

    db = create_db();

    int sockfd;
    if(start_server(DEFAULT_PORT,&sockfd) < 0){
        exit(1);
    }

    clients_loop(sockfd);

    LOG_INFO("Closing server...");
    LOG_INFO("Bye :)");

    close_logger();
    free_db(db);
}