#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 1234
#define BUFFER_SIZE 1024

int main(void) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    const char *message = "Hello WebSocket!";

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return 1;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return 1;
    }

    //Send websocket handshake
    snprintf(buffer, sizeof(buffer), "GET / HTTP/1.1\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key:dGh1IHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version:13\r\n\r\n", PORT);
    send(sockfd, buffer, strlen(buffer), 0);
    // Receive handshake responce
    recv(sockfd, buffer, sizeof(buffer), 0);
    // Send message
    snprintf(buffer, sizeof(buffer), "\x81\x0F%s", message); // 0x81 for text frame, 0x0F for text len
    send(sockfd, buffer, strlen(message) + 2, 0);
    // Close connection
    close(sockfd);
    return 0;
}