#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "server.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>

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
        return -1;
    }
    return listener;

}

