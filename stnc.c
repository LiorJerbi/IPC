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



int main(int argc, char *argv[]){
    // char buff[256];     //buffer for client data
    if(argc==3){        
        if((!strcmp(argv[1],"-s"))){// Server chat side
            server_chat_Handler(atoi(argv[2]));
        }
        else{
        perror("Usage Server side: ./stnc -s PORT\nUsage Client side: ./stnc -c IP PORT");
        exit(1);
        }
    }

    
    else if(argc==4){       
        if(!strcmp(argv[1],"-c")){ // Client chat side
            client_chat_Handler(atoi(argv[3]),argv[2]);
        }
        else if(!strcmp(argv[1],"-s")){ //server test performance (without q mode)
            performance_handler(atoi(argv[2]),1);
        }
        else{
            perror("Usage Server side: ././stnc -s PORT\nUsage Client side: ./stnc -c IP PORT");
            exit(1);
        } 
    }

    else if(argc == 5){ //server test performance with q mode
        if(!strcmp(argv[4],"-q")){
            performance_handler(atoi(argv[2]),0);
        }
        else{
            printf("Usage: ././stnc -s port -p (p for performance test)-q (q for quiet)\n");
        }       
    }

    else if(argc == 7){ //client test preformance
        char* type = argv[5];
        char* param = argv[6];
        if(!strcmp(type,"ipv4")){
            if(!strcmp(param,"tcp")){
                send_params(atoi(argv[3]),param,type);
                perform_tcp_ipv4(atoi(argv[3]),argv[2]);
                return 0;
            }
            else if(!strcmp(param,"udp")){
                send_params(atoi(argv[3]),param,type);
                perform_udp_ipv4(atoi(argv[3]),argv[2]);
            }
            else{
                perror("Usage: ./stnc -c IP PORT -p <type> <param>\n");
                exit(1);            
            }
        }
        else if(!strcmp(type,"ipv6")){
            if(!strcmp(param,"tcp")){
                send_params(atoi(argv[3]),param,type);
                perform_tcp_ipv6(atoi(argv[3]),argv[2]);
            }
            else if(!strcmp(param,"udp")){
                send_params(atoi(argv[3]),param,type);
                perform_udp_ipv6(atoi(argv[3]),argv[2]);
            }
            else{
                perror("Usage: ./stnc -c IP PORT -p <type> <param>\n");
                exit(1);            
            }
        }
        else if(!strcmp(type,"uds")){
            if(!strcmp(param,"dgram")){
                send_params(atoi(argv[3]),param,type);
                perform_dgram_uds(argv[2]);

            }
            else if(!strcmp(param,"stream")){
                send_params(atoi(argv[3]),param,type);
                perform_stream_uds(argv[2]);

            }
            else{
                perror("Usage: ./stnc -c IP PORT -p <type> <param>\n");
                exit(1);            
            }
        }
        else if(!strcmp(type,"mmap")){
            send_params(atoi(argv[3]),param,type);
            perform_filename_mmap(param);
        }
        else if(!strcmp(type,"pipe")){
            send_params(atoi(argv[3]),param,type);
            perform_filename_pipe(param);         
        }
        else{
        perror("Usage: ./stnc -c IP PORT -p <type> <param>\n");
        exit(1);            
        }
    }
    else{
        perror("Usage Server side: ./stnc -s PORT\nUsage Client side: ./stnc -c IP PORT");
        exit(1);
    }
    return 0;
    }