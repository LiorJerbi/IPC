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
    int connectionfd;
    //Make a connection to reciver.
    if((connectionfd=connect(sender_sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr))) == -1){
        perror("Connection failed!\n");
        close(sender_sockfd);
        return -1;
    }

    return connectionfd;
   
}