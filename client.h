#ifndef CLIENT_H
#define CLIENT_H

int get_connection_socket(int ,char* ,int);
void client_chat_Handler(int ,char* );
char* get_chunkData();
char* cchecksum(const char* , size_t );
void perform_tcp_ipv4(int ,char* );
void perform_tcp_ipv6(int ,char* );
void send_params(int, char*,char*,char*);

#endif