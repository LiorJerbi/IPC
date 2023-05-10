#ifndef CLIENT_H
#define CLIENT_H

int get_connection_socket(int port,char* ip);
void client_chat_Handler(int port,char* ip);


#endif