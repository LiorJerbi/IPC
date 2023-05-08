#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "server.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct addrinfo hints;
struct addrinfo *serverinfo;

int open_socket(int port_num){
    memset(&hints,0,sizeof(&hints));

}
