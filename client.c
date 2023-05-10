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



int get_connection_socket(int port,char* ip){

    int sender_sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sender_sockfd == -1){
        perror("Could not create socket.\n");
        return -1;
    }

    //attaching socket to reciver port.
    struct sockaddr_in serverAddr;
    memset(&serverAddr,0,sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    int ra = inet_pton(AF_INET,(const char*)ip,&serverAddr.sin_addr);
    if(ra<=0){
        perror("inet_pton() failed!\n");
        return -1;
    }
    //Make a connection to reciver.
    if(connect(sender_sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1){
        perror("Connection failed!\n");
        close(sender_sockfd);
        return -1;
    }

    return sender_sockfd;

}
void client_chat_Handler(int port,char* ip)
{
        char buff[256];
        int client_fd;
        if((client_fd = get_connection_socket(port,ip)) == -1){
            perror("connect");
            exit(4);
        }
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
        fd_count = 2;

        for(;;){ // for ever
            int poll_count = poll(pfds, fd_count, -1);
            if(poll_count==-1){
                perror("poll");
                exit(2);
            }
            memset(buff,0,sizeof(buff));
            if(pfds[1].revents && POLLIN){
                int nbytes = recv(pfds[1].fd,buff,sizeof(buff),0);
                int server_fd = pfds[1].fd;
                if (nbytes <= 0) {
                    // Got error or connection closed by client
                    if (nbytes == 0) {
                        // Connection closed
                        printf("pollserver: socket %d hung up\n", server_fd);
                        exit(0);
                    } 
                    else {
                        perror("recv");
                    }

                    close(pfds[1].fd); // Bye!
                } 
                else {
                    // We got some good data from a client
                    printf("Server: %s",buff);
                }                            
            }
            if(pfds[0].revents && POLLIN){
                char input[256];
                if(fgets(input, sizeof(input), stdin) != NULL) {
                    if(send(pfds[1].fd, input, strlen(input), 0) == -1) {
                        perror("send");
                    }
                }                            
            }

        }
}