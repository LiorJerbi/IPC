#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "server.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>





int main(int argc, char* argv[]){

    if(argc==3){        // Server side

    }
    else if(argc==4){        // Client side
        
    }
    else{
        printf("Usage Server side: stnc -s PORT\nUsage Client side: stnc -c IP PORT");
        exit(1);
    }
    }