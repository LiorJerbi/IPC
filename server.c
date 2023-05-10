#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include "server.h"

#define  IV4_SIZE sizeof(struct sockaddr_in) 



void addfds(struct pollfd *pfds[], int newfd,int *fd_count, int *fd_size){

    if(*fd_count==*fd_size){
        *fd_size *= 2;
        *pfds = realloc(*pfds,sizeof(**pfds)*(*fd_size));
    }
    
    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

    (*fd_count)++;
}
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
}
// Return a listening socket
int get_listener_socket(int port)
{   
    int listener;     // Listening socket descriptor
    listener = socket(AF_INET,SOCK_STREAM,0);
    if(listener == -1){
        perror("Could not create listening socket.\n");
        close(listener);
        return -1;
    }
    //attach the socket to the reciver details.
    struct sockaddr_in serverAddress;
    memset(&serverAddress,0,sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    inet_pton(AF_INET,"10.9.0.1",&serverAddress.sin_addr.s_addr);
    serverAddress.sin_port = htons(port);
    
    //Reuse the address and port.(prevents errors such as "address already in use")
    int yes = 1;
    if((setsockopt(listener,SOL_SOCKET,SO_REUSEADDR, &yes, sizeof(yes))) < 0){
        perror("setsockopt() failed\n");
        close(listener);
        exit(1);
    }
    if(bind(listener,(struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1){
        perror("Binding failed.\n");
        close(listener);
        return -1;
    }
    printf("Bind succeeded!\n");

    if(listen(listener,1) == -1){
        return -1;
    }
    return listener;

}
void server_chat_Handler(int port)
{   
    char buff[256];
    int newfd;
    int fd_count = 0, fd_size=3;
    struct sockaddr_in clientAddr;
    struct pollfd *pfds= malloc(sizeof(*pfds) * fd_size);   // poll for server fds connections
    if(pfds == NULL){
        printf("malloc error");
        exit(5);
    }

    int listener =  get_listener_socket(port);
    if(listener == -1){
        printf("error listener\n");
        exit(1);
    }  
    pfds[0].fd=listener;
    pfds[0].events=POLLIN;
    pfds[1].fd = STDIN_FILENO;
    pfds[1].events=POLLIN;

    fd_count = 2;
    for(;;){        //for ever
        int poll_count = poll(pfds, fd_count, -1);
        if(poll_count==-1){
            perror("poll");
            exit(2);
        }
        memset(buff,0,sizeof(buff));
        if(pfds[0].revents && POLLIN){
                socklen_t clilen=sizeof(clientAddr);
                newfd = accept(listener,(struct sockaddr *)&clientAddr,&clilen);         
                if(newfd == -1){
                    perror("accept");
                    exit(3);
                }
                else{
                    // addfds(&pfds,newfd,&fd_count,&fd_size);
                    fd_count++;
                    pfds[2].fd = newfd;
                    pfds[2].events = POLLIN;
                    char pbuff[IV4_SIZE];
                    printf("new connection %s\n", inet_ntop(AF_INET,&clientAddr.sin_addr,pbuff,IV4_SIZE));

                }
        }
        if(pfds[1].revents & POLLIN){
            char input[256];
            if(fgets(input, sizeof(input), stdin) != NULL) {
                if(send(pfds[2].fd, input, strlen(input), 0) == -1) {
                    perror("send");
                }
                // printf("msg sent from server\n");
            }
        }
        if(pfds[2].revents & POLLIN){
            // printf("reciveing msg from client:\n");

            int nbytes = recv(pfds[2].fd,buff,sizeof(buff),0);
            if (nbytes <= 0) {
                // Got error or connection closed by client
                if (nbytes == 0) {
                    // Connection closed
                    printf("pollserver: socket %d hung up\n", pfds[2].fd);
                } 
                else {
                    perror("recv");
                }
                close(pfds[2].fd); // Bye!
                del_from_pfds(pfds, 2, &fd_count);

            }
            else {
                // We got some good data from a client
                printf("Client: %s",buff);
            }
        }

    }

}
