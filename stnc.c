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
#include "client.h"
#include "server.h"

#define  IV4_SIZE sizeof(struct sockaddr_in) 

int castPort(char* port){
    int len = strlen(port);
    int ans=0;
    for(int i=0;i<len;i++){
        if(port[i] > 47 && port[i] < 58){
            ans=ans*10+(port[i]-48);
        }
        else{
            perror("Usage Server side: stnc -s PORT\nUsage Client side: stnc -c IP PORT");
            exit(5);
        }
    }
    return ans;
}
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

int main(int argc, char *argv[]){
    char buff[256];     //buffer for client data
    if(argc==3){        // Server side
        int newfd;
        int fd_count = 0, fd_size=3;
        struct sockaddr_in clientAddr;
        struct pollfd *pfds= malloc(sizeof(*pfds) * fd_size);   // poll for server fds connections
        if(pfds == NULL){
            printf("malloc error");
            exit(5);
        }

        int listener =  get_listener_socket(castPort(argv[2]));
        if(listener == -1){
            printf("error listener");
            exit(1);
        }
        pfds[0].fd=listener;
        pfds[0].events=POLLIN;
        pfds[1].fd = STDIN_FILENO;
        pfds[1].events=POLLIN;

        ++fd_count;

        for(;;){        //for ever
            int poll_count = poll(pfds, fd_count, -1);

            if(poll_count==-1){
                perror("poll");
                exit(2);
            }

            for(int i=0; i<fd_count;i++){
                if(pfds[i].revents & POLLIN){
                    if(pfds[i].fd == listener){
                        socklen_t clilen=sizeof(clientAddr);
                        newfd = accept(listener,(struct sockaddr *)&clientAddr,&clilen);         
                        if(newfd == -1){
                            perror("accept");
                            exit(3);
                        }
                        else{
                            addfds(&pfds,newfd,&fd_count,&fd_size);
                            char pbuff[IV4_SIZE];
                            printf("new connection %s\n", inet_ntop(AF_INET,&clientAddr.sin_addr,pbuff,IV4_SIZE));
                            
                        }
                    }
                    else if(pfds[i].fd == STDIN_FILENO){
                        printf("input comingn\n");
                        char input[256];
                        if(fgets(input, sizeof(input), stdin) != NULL) {
                            for(int j=0; j<fd_count; j++) {
                                if(pfds[j].fd != listener && pfds[j].fd != STDIN_FILENO) {
                                    if(send(pfds[j].fd, input, strlen(input), 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                    }
                    else{
                        printf("reciveing msg from client:\n");
                        int nbytes = recv(pfds[i].fd,buff,sizeof(buff),0);
                        int sender_fd = pfds[i].fd;
                        if (nbytes <= 0) {
                            // Got error or connection closed by client
                            if (nbytes == 0) {
                                // Connection closed
                                printf("pollserver: socket %d hung up\n", sender_fd);
                            } else {
                                perror("recv");
                            }

                            close(pfds[i].fd); // Bye!
                            del_from_pfds(pfds, i, &fd_count);

                        } else {
                            // We got some good data from a client
                            printf("%s\n",buff);

                        }
                    }

                }

            }
        }

    }
    else if(argc==4){        // Client side
        int client_fd;
        if((client_fd = get_connection_socket(castPort(argv[3]),argv[2])) == -1){
            perror("connect");
            exit(4);
        }
        else{
            int fd_count =0,fd_size = 2;
            struct pollfd *pfds= malloc(sizeof(*pfds) * fd_size);   // poll for server fds connections
            if(pfds == NULL){
                printf("malloc error");
                exit(5);
            }
            
            pfds[0].fd=STDIN_FILENO;
            pfds[0].events= POLLIN;
            pfds[1].fd = client_fd;
            pfds[1].events = POLLIN;

            for(;;){ // for ever
                int poll_count = poll(pfds, fd_count, -1);
                if(poll_count==-1){
                    perror("poll");
                    exit(2);
                }
                if(pfds[1].revents && POLLIN){
                    printf("msg comingn\n");
                    int nbytes = recv(pfds[0].fd,buff,sizeof(buff),0);
                    int server_fd = pfds[1].fd;
                    if (nbytes <= 0) {
                        // Got error or connection closed by client
                        if (nbytes == 0) {
                            // Connection closed
                            printf("pollserver: socket %d hung up\n", server_fd);
                        } else {
                            perror("recv");
                        }

                        close(pfds[1].fd); // Bye!
                        del_from_pfds(pfds, 1, &fd_count);

                    } else {
                        // We got some good data from a client
                        printf("%s\n",buff);
                    }                            
                }
                else if(pfds[0].revents && POLLIN){
                    printf("client writing\n");
                    char input[256];
                    if(fgets(input, sizeof(input), stdin) != NULL) {
                        if(send(pfds[0].fd, input, strlen(input), 0) == -1) {
                            perror("send");
                        }
                    }                            
                }

            }
        }
    }
    else{
        perror("Usage Server side: stnc -s PORT\nUsage Client side: stnc -c IP PORT");
        exit(1);
    }
    return 0;
    }