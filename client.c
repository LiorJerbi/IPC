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



int get_connection_socket(int port, char* sender_ip) {
    // Create the sender socket
    int sender_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sender_sockfd == -1) {
        perror("Could not create socket.\n");
        return -1;
    }

    // Bind the sender socket to the specified IP address and an arbitrary port
    struct sockaddr_in sender_addr;
    memset(&sender_addr, 0, sizeof(sender_addr));
    sender_addr.sin_family = AF_INET;
    sender_addr.sin_addr.s_addr = inet_addr(sender_ip);
    sender_addr.sin_port = 0; // Bind to an arbitrary port

    if (bind(sender_sockfd, (struct sockaddr*)&sender_addr, sizeof(sender_addr)) < 0) {
        perror("Bind failed.\n");
        close(sender_sockfd);
        return -1;
    }

    // Connect to the server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    int ra = inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    if (ra <= 0) {
        perror("inet_pton() failed!\n");
        return -1;
    }


    if (connect(sender_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed!\n");
        close(sender_sockfd);
        return -1;
    }

    return sender_sockfd;
}
