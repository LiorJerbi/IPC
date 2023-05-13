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
#include <time.h>
#include <errno.h>
#include <openssl/evp.h>
#include <sys/un.h>
#include "client.h"

#define MSG_SIZE 100000000
#define BUFF_SIZE 32000
#define CHECKSUM_SIZE sizeof(char)*32
#define SOCK_PATH "server_uds"



int get_connection_socket(int port,char* ip,int flag){
    if(flag== 4){
        int sender_sockfd = socket(PF_INET,SOCK_STREAM,0);
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
        int sender_sockfd = socket(PF_INET6,SOCK_STREAM,0);
        if(sender_sockfd == -1){
            perror("Could not create socket.\n");
            return -1;
        }

        //attaching socket to reciver port.
        struct sockaddr_in6 serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin6_family = AF_INET6;
        serverAddr.sin6_port = htons(port);
        printf("%s\n",ip);
        if (inet_pton(AF_INET6, ip, &serverAddr.sin6_addr) <= 0) {
            if (errno == EAFNOSUPPORT) {
                perror("AF_INET6 not supported\n");
            } else if (errno == ENETUNREACH) {
                perror("The network is unreachable\n");
            } else if (errno == EINVAL) {
                perror("Invalid IPv6 address\n");
            } else {
                perror("inet_pton() failed\n");
            }
            close(sender_sockfd);
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
        return -1;
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

char* cchecksum(const char* data, size_t data_size)
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

void send_params(int port,char* parm,char* type){
    char buff[BUFF_SIZE];
    
    int client_fd;
    if((client_fd = get_connection_socket(port,"10.0.2.15",4)) == -1){
        perror("connect\n");
        exit(4);
    }

    strcat(buff,type);
    strcat(buff," ");
    strcat(buff,parm);
    printf("%s\n",buff);
    int bytesent = send(client_fd,buff,sizeof(buff),0);
    if(bytesent <  0){
        perror("inside send method\n");
        exit(0);
    }
    close(client_fd);
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

    char *chksum = cchecksum(msg,MSG_SIZE);
    int bytesent = send(client_fd,chksum,CHECKSUM_SIZE,0);
    if(bytesent < 0){
        perror("send checksum\n");
        free(msg);
        free(chksum);
        exit(0);
    }
    bytesent = send(client_fd,msg,MSG_SIZE,0);
    if(bytesent < 0){
        perror("send message\n");
        free(msg);
        free(chksum);
        exit(0);
    }
    free(msg);
    free(chksum);
    close(client_fd);

}
void perform_tcp_ipv6(int port,char* ip){
    char buff[BUFF_SIZE];
    int client_fd;
    if((client_fd = get_connection_socket(port,ip,6)) == -1){
        perror("connect_ipv6\n");
        exit(4);
    }
    memset(buff,0,sizeof(buff));
    char *msg = get_chunkData();

    char *chksum = cchecksum(msg,MSG_SIZE);
    int bytesent = send(client_fd,chksum,CHECKSUM_SIZE,0);
    if(bytesent < 0){
        perror("send checksum\n");
        free(msg);
        free(chksum);
        exit(0);
    }
    bytesent = send(client_fd,msg,MSG_SIZE,0);
    if(bytesent < 0){
        perror("send message\n");
        free(msg);
        free(chksum);
        exit(0);
    }
    free(msg);
    free(chksum);
    close(client_fd);
}

void perform_udp_ipv4(int port,char* ip){
    int sockfd, bytes_sent;
    struct sockaddr_in server_addr;

    // Create socket
    if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error creating socket\n");
        exit(EXIT_FAILURE);
    }

    // Set server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    char *msg = get_chunkData();
    char *chksum = cchecksum(msg,MSG_SIZE);


    // Send message
    bytes_sent = sendto(sockfd,chksum , CHECKSUM_SIZE, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (bytes_sent < 0) {
        perror("Error sending checksum\n");
        free(chksum);
        free(msg);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int sum_byte = 0;
    while(sum_byte < MSG_SIZE){
        bytes_sent = sendto(sockfd,msg+sum_byte , BUFF_SIZE, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (bytes_sent < 0) {
            perror("Error sending message\n");
            free(chksum);
            free(msg);
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        sum_byte += bytes_sent;
    }
    printf("\nCHECK_SUM: %s\nclient msg: %s\n",chksum,msg);
    free(chksum);
    free(msg);
    close(sockfd);
    exit(0);
}

void perform_udp_ipv6(int port,char* ip){
    int sockfd, bytes_sent;
    struct sockaddr_in6 server_addr;

    // Create socket
    if ((sockfd = socket(PF_INET6, SOCK_DGRAM, 0)) < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET;
    server_addr.sin6_port = htons(port);
    if (inet_pton(AF_INET6, ip, &server_addr.sin6_addr) <= 0) {
            if (errno == EAFNOSUPPORT) {
                perror("AF_INET6 not supported\n");
            } else if (errno == ENETUNREACH) {
                perror("The network is unreachable\n");
            } else if (errno == EINVAL) {
                perror("Invalid IPv6 address\n");
            } else {
                perror("inet_pton() failed\n");
            }
        close(sockfd);
        exit(-1);
    }

    char *msg = get_chunkData();
    char *chksum = cchecksum(msg,MSG_SIZE);

    // Send message
    bytes_sent = sendto(sockfd,chksum , CHECKSUM_SIZE, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (bytes_sent < 0) {
        perror("Error sending checksum");
        free(chksum);
        free(msg);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int sum_byte = 0;
    while(sum_byte < MSG_SIZE){
        bytes_sent = sendto(sockfd,msg+sum_byte , BUFF_SIZE, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (bytes_sent < 0) {
            perror("Error sending message\n");
            free(chksum);
            free(msg);
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        sum_byte += bytes_sent;
    }
    free(chksum);
    free(msg);
    close(sockfd);
    exit(0);


}

void perform_stream_uds(char* src){
    int clientfd, len;
    struct sockaddr_un remote = {
        .sun_family = AF_UNIX,
        // .sun_path = SOCK_PATH,   // Can't do assignment to an array
    };

    if ((clientfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    

    printf("Trying to connect...\n");

    strcpy(remote.sun_path,SOCK_PATH);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    sleep(1);
    if (connect(clientfd, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        exit(1);
    }

    int byte_sent;
    printf("Connected.\n");
    char* msg = get_chunkData();
    char* checksum = cchecksum(msg,MSG_SIZE);

    if ((byte_sent=send(clientfd, checksum, CHECKSUM_SIZE, 0)) == -1) {
        perror("send"); 
        close(clientfd);
        free(checksum);
        free(msg);
        exit(1);       
    }

    if ((byte_sent=send(clientfd, msg, MSG_SIZE, 0)) == -1) {
        perror("send"); 
        close(clientfd);
        free(checksum);

        free(msg);
        exit(1);       
    }
    free(checksum);
    close(clientfd);
    free(msg);
    exit(0); 
}

void perform_dgram_uds(char* src ){

    int clientfd, len;
    struct sockaddr_un remote = {
        .sun_family = AF_UNIX,
        // .sun_path = SOCK_PATH,   // Can't do assignment to an array
    };

    if ((clientfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    

  

    strcpy(remote.sun_path,SOCK_PATH);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    sleep(1);

    int byte_sent;
    char* msg = get_chunkData();
    char* checksum = cchecksum(msg,MSG_SIZE);
    
    if (sendto(clientfd,checksum , CHECKSUM_SIZE, 0, (struct sockaddr*)&remote, len) == -1) {
        perror("send"); 
        close(clientfd);
        free(checksum);
        free(msg);
        exit(1);       
    }

    int sum_byte = 0;
    while(sum_byte < MSG_SIZE){
        byte_sent = sendto(clientfd,msg+sum_byte , BUFF_SIZE, 0,(struct sockaddr*)&remote, len);
        if (byte_sent < 0) {
            perror("Error sending message\n");
            free(checksum);
            free(msg);
            close(clientfd);
            exit(EXIT_FAILURE);
        }
        sum_byte += byte_sent;
    }
    free(checksum);
    close(clientfd);
    free(msg);
    exit(0);     
}

void perform_filename_mmap(char* src){
    int fd;
    struct stat sbuf;


    if ((fd = open(src, O_WRONLY)) == -1) {
            perror("open");
            exit(1);
    }
    if (stat(src, &sbuf) == -1) {
            perror("stat");
            exit(1);
    }
    char *msg = get_chunkData();
    char *checksm = cchecksum(msg,MSG_SIZE);
    char* map = mmap((void*)0,CHECKSUM_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
    if(checksm == MAP_FAILED){
        perror("mmap\n");
        free(msg);
        free(checksm);
        exit(1);
    }
    strcpy(map,checksm);
    map = mmap((void*)0,MSG_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
    if(checksm == MAP_FAILED){
        perror("mmap\n");
        free(msg);
        free(checksm);  
        exit(1);
    }
    strcpy(map,msg);
    free(msg);
    free(checksm);
    close(fd);
    exit(0);

}
void perform_filename_pipe(char* src){
    int fd;

    mkfifo(src,0666);

    fd = open(src, O_WRONLY);
    
    int byte_sent;
    char* msg = get_chunkData();
    char* checksum = cchecksum(msg,MSG_SIZE);


    if ((byte_sent=write(fd, checksum, CHECKSUM_SIZE)) == -1) {
        perror("send"); 
        close(fd);
        free(checksum);
        free(msg);
        exit(1);       
    }

    if ((byte_sent=write(fd, msg, MSG_SIZE)) == -1) {
        perror("send"); 
        close(fd);
        free(checksum);

        free(msg);
        exit(1);       
    }
    free(checksum);
    close(fd);
    free(msg);
    exit(0); 
    
}




