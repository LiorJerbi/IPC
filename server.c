#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <sys/time.h>
#include <openssl/evp.h>
#include <errno.h>
#include <sys/un.h>
#include "server.h"


#define SOCK_PATH "server_uds"
#define IV4_SIZE sizeof(struct sockaddr_in)
#define IV6_SIZE INET6_ADDRSTRLEN
#define MSG_SIZE 100000000
#define BUFF_SIZE 32000
#define CHECKSUM_SIZE sizeof(char)*32



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

//Return a ipv6 listen socket
int get_listener6_socket(int port){
    int listener;     // Listening socket descriptor
    listener = socket(PF_INET6,SOCK_STREAM,0);
    if(listener == -1){
        perror("Could not create listening socket.\n");
        close(listener);
        return -1;
    }
    //attach the socket to the reciver details.
    struct sockaddr_in6 serverAddress;
    memset(&serverAddress,0,sizeof(serverAddress));
    serverAddress.sin6_family = AF_INET6;
    serverAddress.sin6_addr = in6addr_any;
    serverAddress.sin6_port = htons(port);
    
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
        perror("listen_faild");
        exit(1);
    }
    return listener;
}

// Return a listening socket
int get_listener_socket(int port)
{   
    int listener;     // Listening socket descriptor
    listener = socket(PF_INET,SOCK_STREAM,0);
    if(listener == -1){
        perror("Could not create listening socket.\n");
        close(listener);
        return -1;
    }
    //attach the socket to the reciver details.
    struct sockaddr_in serverAddress;
    memset(&serverAddress,0,sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
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
        perror("listen_faild");
        exit(1);
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
                if(!strcmp(input,"exit")){
                    free(pfds);
                    printf("Server exit");
                    exit(0);
                }
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


void get_params(int port,char* params){
    memset(params,0,strlen(params));

    int listenfd = get_listener_socket(port);
    struct sockaddr_in clientAddr;
    socklen_t clilen=sizeof(clientAddr);
    int newfd = accept(listenfd,(struct sockaddr *)&clientAddr,&clilen);
    char pbuff[IV4_SIZE];
    printf("new connection %s\n", inet_ntop(AF_INET,&clientAddr.sin_addr,pbuff,IV4_SIZE));

    if(newfd == -1){
        perror("get_params\n");
        exit(-1);
    }
    int byte_rec = recv(newfd,params,256,0);
    if(byte_rec == -1){
        perror("get_first_msg\n");
        exit(-1);
    }
    close(newfd);
    close(listenfd);  

}

char* checksum(const char* data, size_t data_size)
{
    EVP_MD_CTX *mdContext;
    mdContext = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdContext, EVP_md5(), NULL);
    EVP_DigestUpdate(mdContext, data, data_size);

    char* checksum = malloc(EVP_MD_size(EVP_md5()));
    if (!checksum) {
        perror("malloc");
        return NULL;
    }

    unsigned int checksum_size;
    EVP_DigestFinal_ex(mdContext, (unsigned char*)checksum, &checksum_size);
    EVP_MD_CTX_free(mdContext);

    return checksum;
}


void ipv4_tcp(int port,int flag){
    char* checksm=malloc(sizeof(char)*CHECKSUM_SIZE);
    char* msg =malloc(sizeof(char)*MSG_SIZE);
    if(msg == NULL || checksm == NULL){
        perror("malloc\n");
        exit(-1);
    }
    memset(checksm,0,CHECKSUM_SIZE);


    int listen_fd = get_listener_socket(port);
    struct sockaddr_in clientAddr;
    socklen_t clilen=sizeof(&clientAddr); 
    int newfd = accept(listen_fd,(struct sockaddr *)&clientAddr,&clilen);
    if(newfd == -1){
        perror("tcp_ipv4_accept\n");
        free(msg);
        free(checksm);
        close(listen_fd);
        exit(-1);
    }
    char pbuff[IV4_SIZE];
    printf("new connection %s\n", inet_ntop(AF_INET,&clientAddr.sin_addr,pbuff,IV4_SIZE));
    
    
    //Recive checksum
    int byte_rec;  
    byte_rec = recv(newfd,checksm,CHECKSUM_SIZE,0);
    
    if(byte_rec == -1){
        perror("tcp_ipv4_checksum msg\n");
        free(msg);
        free(checksm);
        close(listen_fd);
        close(newfd);
        exit(-1);
    }
    
    


    // Start timer
    struct timeval start_time = {0};
    gettimeofday(&start_time, NULL);
    // Receive data
    int msg_bytes_received = 0;
    while (msg_bytes_received < MSG_SIZE) {
        byte_rec = recv(newfd, msg + msg_bytes_received, MSG_SIZE - msg_bytes_received, 0);
        if (byte_rec == -1) {
            perror("tcp_ipv4_recv_msg");
            free(msg);
            free(checksm);
            close(listen_fd);
            close(newfd);
            exit(-1);
        }
        msg_bytes_received += byte_rec;
    }
    // Stop timer
    struct timeval end_time = {0};
    gettimeofday(&end_time, NULL);
    // Calculate elapsed time
    double elapsed_time = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) + ((end_time.tv_usec - start_time.tv_usec)/1000.0);
    
    //checksum compere
    char* checksm_cmp = checksum(msg,MSG_SIZE);

    if(strcmp(checksm,checksm_cmp) != 0 ){
        printf("checksum inccorect!\n");
    }
    else{
        printf("checksum correct!\n");
    }
    if(flag == 1){
        printf("%s\n",msg);
    }
    printf("ipv4_tcp,%f\n",elapsed_time);
    free(msg);
    free(checksm);
    free(checksm_cmp);
    close(listen_fd);
    close(newfd);
    exit(0);

}

void ipv6_tcp(int port,int flag){
    char* checksm=malloc(sizeof(char)*CHECKSUM_SIZE);
    char* msg =malloc(sizeof(char)*MSG_SIZE);
    if(msg == NULL || checksm == NULL){
        perror("malloc\n");
        exit(-1);
    }
    memset(checksm,0,CHECKSUM_SIZE);
    int listen_fd = get_listener6_socket(port);
    struct sockaddr_in6 clientAddr;
    socklen_t clilen=sizeof(&clientAddr); 
    int newfd = accept(listen_fd,(struct sockaddr *)&clientAddr,&clilen);
    if(newfd == -1){
        perror("tcp_ipv6_accept\n");
        free(msg);
        free(checksm);
        close(listen_fd);
        exit(-1);
    }
    char pbuff[IV6_SIZE];
    printf("new connection %s\n", inet_ntop(AF_INET6,&clientAddr.sin6_addr,pbuff,INET6_ADDRSTRLEN));
    int byte_rec = recv(newfd,checksm,CHECKSUM_SIZE,0);
    if(byte_rec == -1){
        perror("tcp_ipv6_checksum msg\n");
        free(msg);
        free(checksm);
        close(listen_fd);
        close(newfd);
        exit(-1);
    }

    // Start timer
    struct timeval start_time = {0};
    gettimeofday(&start_time, NULL);
    // Receive data
    int msg_bytes_received = 0;
    while (msg_bytes_received < MSG_SIZE) {
        byte_rec = recv(newfd, msg + msg_bytes_received, MSG_SIZE - msg_bytes_received, 0);
        if (byte_rec == -1) {
            perror("tcp_ipv6_recv_msg");
            free(msg);
            free(checksm);
            close(listen_fd);
            close(newfd);
            exit(-1);
        }
        msg_bytes_received += byte_rec;
    }
    // Stop timer
    struct timeval end_time = {0};
    gettimeofday(&end_time, NULL);
    // Calculate elapsed time
    double elapsed_time = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) + ((end_time.tv_usec - start_time.tv_usec)/1000.0);
    
    //checksum comper
    char* checksm_cmp = checksum(msg,MSG_SIZE);
    if(strcmp(checksm,checksm_cmp) != 0 ){
        printf("checksum inccorect!\n");
    }
    else{
        printf("checksum correct!\n");
    }
    if(flag == 1){
        printf("%s\n",msg);
    }
    printf("ipv6_tcp,%f\n",elapsed_time);
    free(msg);
    free(checksm);
    free(checksm_cmp);
    close(listen_fd);
    close(newfd);
    exit(0);  

}

void ipv4_udp(int port,int flag){
    int udp_sock;     // Listening socket descriptor
    udp_sock = socket(PF_INET,SOCK_DGRAM,0);
    if(udp_sock == -1){
        perror("Could not create udp socket.\n");
        close(udp_sock);
        exit(-1);
    }
    //attach the socket to the reciver details.
    struct sockaddr_in serverAddress;
  

    memset(&serverAddress,0,sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);
    socklen_t serlen=sizeof(&serverAddress); 
    //Reuse the address and port.(prevents errors such as "address already in use")
    int yes = 1;
    if((setsockopt(udp_sock,SOL_SOCKET,SO_REUSEADDR, &yes, sizeof(yes))) < 0){
        perror("setsockopt() failed\n");
        close(udp_sock);
        exit(1);
    }

    if(bind(udp_sock,(struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1){
        perror("Binding failed.\n");
        close(udp_sock);
        exit(-1);
    }
    printf("Bind succeeded!\n");
    int byte_num = 0;
    char* checksm=malloc(sizeof(char)*CHECKSUM_SIZE);
    char* msg =malloc(sizeof(char)*MSG_SIZE);
    if(msg == NULL || checksm == NULL){
        perror("malloc\n");
        exit(-1);
    }
    memset(checksm,0,CHECKSUM_SIZE);


    //get the checksum
    byte_num = recvfrom(udp_sock,checksm,CHECKSUM_SIZE,0,(struct sockaddr *)&serverAddress,&serlen);
    if(byte_num < 0){
        perror("checksum\n");
        free(checksm);
        free(msg);
        close(udp_sock);
    }

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec =0;
    if (setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }
    // Start timer
    struct timeval start_time = {0};
    gettimeofday(&start_time, NULL);
    int msg_byte_num = 0;
    while (msg_byte_num < MSG_SIZE) {
        byte_num = recvfrom(udp_sock, msg + msg_byte_num, MSG_SIZE - msg_byte_num, 0, (struct sockaddr *)&serverAddress, &serlen);
        if (byte_num < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printf("Timeout reached. Exiting loop...\n");
                break;
            } else {
                perror("recvfrom() failed\n");
                free(checksm);
                free(msg);
                close(udp_sock);
                exit(1);
            }
        } else if (byte_num == 0) {
            printf("Got a broken msg\n");
            break;
        }
        msg_byte_num += byte_num;
        // printf("msg bytes: %d, byte_num: %d\n", msg_byte_num, byte_num);
    }
    printf("Total bytens: %d\n",msg_byte_num);
    // Stop timer
    struct timeval end_time = {0};
    gettimeofday(&end_time, NULL);
    // Calculate elapsed time
    double elapsed_time = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) + ((end_time.tv_usec - start_time.tv_usec)/1000.0);
    
    //checksum comper
    char* checksm_cmp = checksum(msg,MSG_SIZE);
    printf("client checksum %s\nthe checksum i get: %s\n",checksm,checksm_cmp);

    if(strcmp(checksm,checksm_cmp) != 0 ){
        printf("checksum inccorect!\n");
    }
    else{
        printf("checksum correct!\n");
    }
    if(flag == 1){
        printf("%s\n",msg);
    }
    printf("ipv4_udp,%f\n",elapsed_time);
    free(msg);
    free(checksm);
    free(checksm_cmp);
    close(udp_sock);
    exit(0);

}

void ipv6_udp(int port,int flag){
    int udp_sock;     // Listening socket descriptor
    udp_sock = socket(PF_INET6,SOCK_DGRAM,0);
    if(udp_sock == -1){
        perror("Could not create udp socket.\n");
        close(udp_sock);
        exit(-1);
    }
    //attach the socket to the reciver details.

    struct sockaddr_in6 serverAddress;
    memset(&serverAddress,0,sizeof(serverAddress));
    serverAddress.sin6_family = AF_INET6;
    serverAddress.sin6_addr = in6addr_any;
    serverAddress.sin6_port = htons(port);
    socklen_t serlen=sizeof(&serverAddress); 

    //Reuse the address and port.(prevents errors such as "address already in use")
    int yes = 1;
    if((setsockopt(udp_sock,SOL_SOCKET,SO_REUSEADDR, &yes, sizeof(yes))) < 0){
        perror("setsockopt() failed\n");
        close(udp_sock);
        exit(1);
    }
    if(bind(udp_sock,(struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1){
        perror("Binding failed.\n");
        close(udp_sock);
        exit(-1);
    }
    printf("Bind succeeded!\n");
    int byte_num = 0;
    char* checksm=malloc(sizeof(char)*CHECKSUM_SIZE);
    char* msg =malloc(sizeof(char)*MSG_SIZE);
    if(msg == NULL || checksm == NULL){
        perror("malloc\n");
        exit(-1);
    }

    //get the checksum
    byte_num = recvfrom(udp_sock,checksm,CHECKSUM_SIZE,0,(struct sockaddr *)&serverAddress,&serlen);
    if(byte_num < 0){
        perror("checksum\n");
        free(checksm);
        free(msg);
        close(udp_sock);
    }
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec =0;
    if (setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }
    // Start timer
    struct timeval start_time = {0};
    gettimeofday(&start_time, NULL);
    int msg_byte_num = 0;
    while (msg_byte_num < MSG_SIZE) {
        byte_num = recvfrom(udp_sock, msg + msg_byte_num, MSG_SIZE - msg_byte_num, 0, (struct sockaddr *)&serverAddress, &serlen);
        if (byte_num < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printf("Timeout reached. Exiting loop...\n");
                break;
            } else {
                perror("recvfrom() failed\n");
                free(checksm);
                free(msg);
                close(udp_sock);
                exit(1);
            }
        } 
        else if (byte_num == 0) {
            printf("Got a broken msg\n");
            break;
        }
        msg_byte_num += byte_num;
        // printf("msg bytes: %d, byte_num: %d\n", msg_byte_num, byte_num);
    }
    printf("Total bytens: %d\n",msg_byte_num);   
    // Stop timer
    struct timeval end_time = {0};
    gettimeofday(&end_time, NULL);
    // Calculate elapsed time
    double elapsed_time = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) + ((end_time.tv_usec - start_time.tv_usec)/1000.0);
    
    //checksum comper
    char* checksm_cmp = checksum(msg,MSG_SIZE);
    if(strcmp(checksm,checksm_cmp) != 0 ){
        printf("checksum inccorect!\n");
    }
    else{
        printf("checksum correct!\n");
    }
    if(flag == 1){
        printf("%s\n",msg);
    }
    printf("ipv6_udp,%f\n",elapsed_time);
    free(msg);
    free(checksm);
    free(checksm_cmp);
    close(udp_sock);
    exit(0);
}

void stream_uds(int flag){
    int len,servfd,client_fd;
    struct sockaddr_un remote, local = {
        .sun_family = AF_UNIX,
        // .sun_path = SOCK_PATH,   // Can't do assignment to an array
    };
    if ((servfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
    }
    strcpy(local.sun_path, SOCK_PATH);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(servfd, (struct sockaddr *)&local, len) == -1) {
            perror("bind");
            exit(1);
    }

    if (listen(servfd,1 ) == -1) {
            perror("listen");
            exit(1);
    }
    printf("Waiting for a connection...\n");
    socklen_t slen = sizeof(remote);
    if ((client_fd = accept(servfd, (struct sockaddr *)&remote, &slen)) == -1) {
            perror("accept");
            exit(1);
    }
    printf("Connected.\n");

    char* checksm = malloc(sizeof(char)*CHECKSUM_SIZE);
    char* msg = malloc(sizeof(char)*MSG_SIZE);
    if(msg == NULL || checksm == NULL){
        perror("Malloc\n");
        exit(1);
    }


    int byte_rec;  
    byte_rec = recv(client_fd,checksm,CHECKSUM_SIZE,0);
    
    if(byte_rec == -1){
        perror("uds_stream_checksum msg\n");
        free(msg);
        free(checksm);
        close(client_fd);
        close(servfd);
        exit(-1);
    }
    
    // Start timer
    struct timeval start_time = {0};
    gettimeofday(&start_time, NULL);
    int max_byte_rec = 0;

    while(max_byte_rec < MSG_SIZE){    
        if((byte_rec=recv(client_fd,msg+max_byte_rec,MSG_SIZE,0)) <0){
            perror("recv\n");
            free(msg);
            free(checksm);
            close(client_fd);
            close(servfd);
            exit(-1);
        }
        max_byte_rec += byte_rec;
        
    }
    printf("Byte rec: %d\n",max_byte_rec);

    // Stop timer
    struct timeval end_time = {0};
    gettimeofday(&end_time, NULL);
    // Calculate elapsed time
    double elapsed_time = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) + ((end_time.tv_usec - start_time.tv_usec)/1000.0);
    
    //checksum compere
    char* checksm_cmp = checksum(msg,MSG_SIZE);

    if(strcmp(checksm,checksm_cmp) != 0 ){
        printf("checksum inccorect!\n");
    }
    else{
        printf("checksum correct!\n");
    }
    if(flag == 1){
        printf("%s\n",msg);
    }
    printf("stream_uds,%f\n",elapsed_time);
    free(msg);
    free(checksm);
    free(checksm_cmp);
    close(client_fd);
    close(servfd);
    exit(0);
}

void dgram_uds(int flag){
    int servfd;
    struct sockaddr_un  local = {
        .sun_family = AF_UNIX,
        // .sun_path = SOCK_PATH,   // Can't do assignment to an array
    };
    if ((servfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
            perror("socket");
            exit(1);
    }
    strcpy(local.sun_path, SOCK_PATH);
    unlink(local.sun_path);
    socklen_t len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(servfd, (struct sockaddr *)&local, len) == -1) {
            perror("bind");
            exit(1);
    }

    char* checksm = malloc(sizeof(char)*CHECKSUM_SIZE);
    char* msg = malloc(sizeof(char)*MSG_SIZE);
    if(msg == NULL || checksm == NULL){
        perror("Malloc\n");
        exit(1);
    }


    int byte_rec;  
    byte_rec = recvfrom(servfd,checksm,CHECKSUM_SIZE,0,(struct sockaddr *)&local,&len);
    
    if(byte_rec == -1){
        perror("uds_stream_checksum msg\n");
        free(msg);
        free(checksm);
        close(servfd);
        exit(-1);
    }
    
    // Start timer
    struct timeval start_time = {0};
    gettimeofday(&start_time, NULL);
    int max_byte_rec = 0;

    while(max_byte_rec < MSG_SIZE){    
        if((byte_rec=recvfrom(servfd,msg+max_byte_rec,MSG_SIZE,0,(struct sockaddr *)&local,&len)) <0){
            perror("recv\n");
            free(msg);
            free(checksm);
            close(servfd);
            exit(-1);
        }
        max_byte_rec += byte_rec;
        
    }
    printf("Byte rec: %d\n",max_byte_rec);

    // Stop timer
    struct timeval end_time = {0};
    gettimeofday(&end_time, NULL);
    // Calculate elapsed time
    double elapsed_time = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) + ((end_time.tv_usec - start_time.tv_usec)/1000.0);
    
    //checksum compere
    char* checksm_cmp = checksum(msg,MSG_SIZE);

    if(strcmp(checksm,checksm_cmp) != 0 ){
        printf("checksum inccorect!\n");
    }
    else{
        printf("checksum correct!\n");
    }
    if(flag == 1){
        printf("%s\n",msg);
    }
    printf("dgram_uds,%f\n",elapsed_time);
    free(msg);
    free(checksm);
    free(checksm_cmp);
    close(servfd);
    exit(0);
}

void filename_mmap(int flag,char* filename){
    int fd;
    struct stat sbuf;


    if ((fd = open(filename, O_RDONLY)) == -1) {
            perror("open");
            exit(1);
    }

    if (stat(filename, &sbuf) == -1) {
            perror("stat");
            exit(1);
    }


    // char* checksm = malloc(sizeof(char)*CHECKSUM_SIZE);
    // char* msg = malloc(sizeof(char)*MSG_SIZE);
    // if(msg == NULL || checksm == NULL){
    //     perror("Malloc\n");
    //     exit(1);
    // }

    // int byte_rec;  
    sleep(1);
    char* checksm = mmap((void*)0,CHECKSUM_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if(checksm == MAP_FAILED){
        perror("mmap\n");
        exit(1);
    }
    // if(byte_rec == -1){
    //     perror("pipe_checksum msg\n");
    //     // free(msg);
    //     // free(checksm);
    //     close(fd);
    //     exit(-1);
    // }
    
    // Start timer
    struct timeval start_time = {0};
    gettimeofday(&start_time, NULL);
    // int max_byte_rec = 0;
    char* msg;
    // while(max_byte_rec < MSG_SIZE){    
    //     if((msg=mmap((void*),MSG_SIZE,PROT_READ, MAP_SHARED, fd, max_byte_rec)) <0){
    //         perror("recv\n");
    //         free(msg);
    //         free(checksm);
    //         close(fd);
    //         exit(-1);
    //     }
    //     max_byte_rec += byte_rec;
        
    // }
    if((msg=mmap((void*)0,MSG_SIZE,PROT_READ, MAP_SHARED, fd, 0)) <0){
            perror("recv\n");
            // free(msg);
            // free(checksm);
            close(fd);
            exit(-1);
    }
    // printf("Byte rec: %d\n",max_byte_rec);

    // Stop timer
    struct timeval end_time = {0};
    gettimeofday(&end_time, NULL);
    // Calculate elapsed time
    double elapsed_time = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) + ((end_time.tv_usec - start_time.tv_usec)/1000.0);

    //checksum compere
    char* checksm_cmp = checksum(msg,MSG_SIZE);

    if(strcmp(checksm,checksm_cmp) != 0 ){
        printf("checksum inccorect!\n");
    }
    else{
        printf("checksum correct!\n");
    }
    if(flag == 1){
        printf("%s\n",msg);
    }
    printf("mmap,%f\n",elapsed_time);
    // free(msg);
    // free(checksm);
    free(checksm_cmp);
    close(fd);
    exit(0);



}


void filename_pipe(int flag,char* filename){
    int fd;

    mkfifo(filename, 0666);

    printf("waiting for writers...\n");
    fd = open(filename, O_RDONLY);
    printf("got a writer\n");

    char* checksm = malloc(sizeof(char)*CHECKSUM_SIZE);
    char* msg = malloc(sizeof(char)*MSG_SIZE);
    if(msg == NULL || checksm == NULL){
        perror("Malloc\n");
        exit(1);
    }

    int byte_rec;  
    byte_rec = read(fd,checksm,CHECKSUM_SIZE);
    
    if(byte_rec == -1){
        perror("pipe_checksum msg\n");
        free(msg);
        free(checksm);
        close(fd);
        exit(-1);
    }
    
    // Start timer
    struct timeval start_time = {0};
    gettimeofday(&start_time, NULL);
    int max_byte_rec = 0;

    while(max_byte_rec < MSG_SIZE){    
        if((byte_rec=read(fd,msg+max_byte_rec,MSG_SIZE)) <0){
            perror("recv\n");
            free(msg);
            free(checksm);
            close(fd);
            exit(-1);
        }
        max_byte_rec += byte_rec;
        
    }
    printf("Byte rec: %d\n",max_byte_rec);

    // Stop timer
    struct timeval end_time = {0};
    gettimeofday(&end_time, NULL);
    // Calculate elapsed time
    double elapsed_time = ((end_time.tv_sec - start_time.tv_sec) * 1000.0) + ((end_time.tv_usec - start_time.tv_usec)/1000.0);

    //checksum compere
    char* checksm_cmp = checksum(msg,MSG_SIZE);

    if(strcmp(checksm,checksm_cmp) != 0 ){
        printf("checksum inccorect!\n");
    }
    else{
        printf("checksum correct!\n");
    }
    if(flag == 1){
        printf("%s\n",msg);
    }
    printf("pipe,%f\n",elapsed_time);
    free(msg);
    free(checksm);
    free(checksm_cmp);
    close(fd);
    exit(0);
}


void performance_handler(int port,int flag){
    char* params[2];
    char paramst[256] = {0};
    char type[128]={0};
    char param[128]={0};
    get_params(port,paramst);

    for(int i=0;i<256;++i){
        if(paramst[i]==' '){
            strncpy(type,paramst,i);
            strncpy(param,(paramst+i+1),strlen(paramst)-strlen(type)+1);

            break;
        }
    }
    params[0]=type;
    params[1]=param;
    printf("Type: %s\nParam: %s\n",type,param);
    if(!strcmp(params[0],"ipv4")){
        if(!strcmp(params[1],"tcp")){
            ipv4_tcp(port,flag);
        }
        else if(!strcmp(params[1],"udp")){
            ipv4_udp(port,flag);
        }
        else{
            perror("Usage: ./stnc -c IP PORT -p <type> <param>\n");
            exit(1);
        }
    }
    else if(!strcmp(params[0],"ipv6")){
        if(!strcmp(params[1],"tcp")){
            ipv6_tcp(port,flag);

        }
        else if(!strcmp(params[1],"udp")){
            ipv6_udp(port,flag);
        }
        else{
            perror("Usage: ./stnc -c IP PORT -p <type> <param>\n");
            exit(1);           
        }
    }
    else if(!strcmp(params[0],"uds")){
        if(!strcmp(params[1],"dgram")){
            dgram_uds(flag);
        }
        else if(!strcmp(params[1],"stream")){
            stream_uds(flag);
        }
        else{
            perror("Usage: ./stnc -c server PORT -p <type> <param>\n");
            exit(1);
        }
    }
    else if(!strcmp(params[0],"mmap")){
        filename_mmap(flag,params[1]);
        
    }
    else if(!strcmp(params[0],"pipe")){
        filename_pipe(flag,params[1]);
        
    }
    else{
    perror("Usage: ./stnc -c IP PORT -p <type> <param>\n");
    exit(1);            
    }
}

