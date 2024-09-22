// Compile with the  -lws2_32 flag
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <stdint.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8765 // Replace with your server port
#define BUFFER_SIZE 1024

void mask_payload(uint8_t *payload, size_t length, uint8_t *mask) {
    for (size_t i = 0; i < length; i++) {
        payload[i] ^= mask[i % 4];
    }
}

int main() {
    WSADATA wsa;
    SOCKET sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    const char *message = "{\"path\":\"logs/test.log\",\"log_string\":\"test log string!\"}";
    uint8_t mask[4];

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation error. Error Code: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection failed. Error Code: %d\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    // Send WebSocket handshake
    snprintf(buffer, sizeof(buffer),
            "GET / HTTP/1.1\r\n"
            "Host: localhost:%d\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "Sec-WebSocket-Version: 13\r\n\r\n", PORT);
    send(sockfd, buffer, strlen(buffer), 0);

    // Receive handshake response
    recv(sockfd, buffer, sizeof(buffer), 0);

    // Prepare WebSocket frame
    size_t msg_len = strlen(message);
    buffer[0] = 0x81; // FIN + text frame
    buffer[1] = 0x80 | msg_len; // Masked + payload length

    // Generate masking key
    srand((unsigned int)time(NULL));
    for (int i = 0; i < 4; i++) {
        mask[i] = rand() % 256;
    }
    memcpy(&buffer[2], mask, 4);

    // Mask the payload
    memcpy(&buffer[6], message, msg_len);
    mask_payload((uint8_t *)&buffer[6], msg_len, mask);

    // Send the frame
    send(sockfd, buffer, msg_len + 6, 0);

    // Send close frame
    buffer[0] = 0x88; // FIN + close frame
    buffer[1] = 0x80; // Masked, no payload
    for (int i = 0; i < 4; i++) {
        mask[i] = rand() % 256;
    }
    memcpy(&buffer[2], mask, 4);
    send(sockfd, buffer, 6, 0);

    // Close connection
    closesocket(sockfd);
    WSACleanup();

    return 0;
}