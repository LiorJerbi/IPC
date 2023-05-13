#ifndef SERVER_H
#define SERVER_H

int get_listener_socket(int port);
int get_listener6_socket(int port);

void server_chat_Handler(int port);
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count);
void addfds(struct pollfd *pfds[], int newfd,int *fd_count, int *fd_size);
void get_params(int,char*);
void performance_handler(int,int);
void ipv4_tcp(int,int);
void ipv6_tcp(int,int);
void ipv4_udp(int,int);
void ipv6_udp(int,int);
char* checksum(const char* , size_t );


#endif