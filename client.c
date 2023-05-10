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
#include <time.h>
#include <openssl/md5.h>
#include "client.h"

#define MSG_SIZE 100000000
#define BUFF_SIZE 32000


int get_connection_socket(int port,char* ip,int flag){
    if(flag== 4){
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
    else if(flag == 6){
        int sender_sockfd = socket(AF_INET6,SOCK_STREAM,0);
        if(sender_sockfd == -1){
            perror("Could not create socket.\n");
            return -1;
        }

        //attaching socket to reciver port.
        struct sockaddr_in6 serverAddr;
        memset(&serverAddr,0,sizeof(serverAddr));
        serverAddr.sin6_family = AF_INET6;
        serverAddr.sin6_port = htons(port);
        int ra = inet_pton(AF_INET6,(const char*)ip,&serverAddr.sin6_addr);
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
    else{
        return NULL;
    }
}

char* get_chunkData()
{
    char* data = malloc(sizeof(char) * MSG_SIZE);
    if (data == NULL) {
        perror("Malloc");
        return NULL;
    }

    srand(time(NULL));

    for (int i = 0; i < MSG_SIZE; i++) {
        data[i] = rand() % 256; // generate random byte
    }

    return data;
}

unsigned char* checksum(const char* data, size_t data_size)
{
    MD5_CTX mdContext;
    MD5_Init(&mdContext);
    MD5_Update(&mdContext, data, data_size);

    unsigned char* checksum = malloc(MD5_DIGEST_LENGTH);
    if (!checksum) {
        perror("malloc");
        return NULL;
    }

    MD5_Final(checksum, &mdContext);

    return checksum;
}


void client_chat_Handler(int port,char* ip)
{
        char buff[256];
        int client_fd;
        if((client_fd = get_connection_socket(port,ip,4)) == -1){
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
                    } 
                    else {
                        perror("recv");
                    }
                   
                    close(pfds[1].fd); // Bye!
                    free(pfds);
                    exit(0);
                } 
                else {
                    // We got some good data from a client
   
                    printf("Server: %s",buff);
                }                            
            }
            if(pfds[0].revents && POLLIN){
                char input[256];
                if(fgets(input, sizeof(input), stdin) != NULL) {
                    if(!strcmp(input,"exit")){
                        close(pfds[1].fd); // Bye!
                        free(pfds);
                    }
                    if(send(pfds[1].fd, input, strlen(input), 0) == -1) {
                        perror("send\n");
                    }
                }                            
            }

        }
}

void send_params(int port, char* ip){
    char buff[BUFF_SIZE];
    
    int client_fd;
    if((client_fd = get_connection_socket(port,ip,4)) == -1){
        perror("connect\n");
        exit(4);
    }
    strcpy(buff,ip);
    int bytesent = send(client_fd,buff,sizeof(buff),0);
    if(bytesent <  0){
        perror("inside send method\n");
        exit(0);
    }
    sprintf(buff,"%d",port);
    int bytesent = send(client_fd,buff,sizeof(buff),0);
    if(bytesent <  0){
        perror("inside send method\n");
        exit(0);
    }
    return;
    
}

void perform_tcp_ipv4(int port,char* ip){
    char buff[BUFF_SIZE];
    int client_fd;
    if((client_fd = get_connection_socket(port,ip,4)) == -1){
        perror("connect\n");
        exit(4);
    }
    memset(buff,0,sizeof(buff));
    char *msg = get_chunkData();

    char *chksum = checksum(msg,MSG_SIZE);
    int bytesent = send(client_fd,chksum,sizeof(chksum),0);
    if(bytesent < 0){
        perror("send checksum\n");
        free(msg);
        free(chksum);
        exit(0);
    }
    printf("Sent checksum, sending message\n");
    bytesent = send(client_fd,msg,MSG_SIZE,0);
    if(bytesent < 0){
        perror("send message\n");
        free(msg);
        free(chksum);
        exit(0);
    }

}
void perform_tcp_ipv6(int port,char* ip){
    char buff[BUFF_SIZE];
    int client_fd;
    if((client_fd = get_connection_socket(port,ip,6)) == -1){
        perror("connect\n");
        exit(4);
    }
    memset(buff,0,sizeof(buff));
    char *msg = get_chunkData();

    char *chksum = checksum(msg,MSG_SIZE);
    int bytesent = send(client_fd,chksum,sizeof(chksum),0);
    if(bytesent < 0){
        perror("send checksum\n");
        free(msg);
        free(chksum);
        exit(0);
    }
    printf("Sent checksum, sending message\n");
    bytesent = send(client_fd,msg,MSG_SIZE,0);
    if(bytesent < 0){
        perror("send message\n");
        free(msg);
        free(chksum);
        exit(0);
    }
}