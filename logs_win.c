#include <string.h>
#include <winsock2.h>
#include <stdint.h>
#include <time.h>
#include <windows.h>
// #include <pthread.h>
#include "logs_win.h"

#define BUFFER_SIZE 1024

static SOCKET sockfd = INVALID_SOCKET;

int n_times = 1;
HANDLE *h_threads;
DWORD *thread_ids;

volatile int term = 0;

void mask_payload(uint8_t *payload, size_t length, uint8_t *mask) {
    for (size_t i = 0; i < length; i++) {
        payload[i] ^= mask[i % 4];
    }
}

DWORD WINAPI thread_func(LPVOID lp_param) {
    char buffer[BUFFER_SIZE];
    while(1) {
        int bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
        if(bytes_received <=0) {
            break;
        }
        uint8_t opcode = buffer[0] & 0x0F;
        if(opcode == 9) {   // Ping frame received
            char pong_frame[BUFFER_SIZE] = {0};  // Max pong length is 125
            size_t frame_len = bytes_received - 2;
            pong_frame[0] = 0x8A;   // Create pong frame
            pong_frame[1] = 0x80 + frame_len;

            uint8_t mask[4];
            for(int i=0; i<4; i++) {
                mask[i] = rand() % 256;
            }
            memcpy(&pong_frame[2], mask, 4);
            memcpy(&pong_frame[6], &buffer[2], frame_len);
            mask_payload((uint8_t *)&pong_frame[6], frame_len, mask);

            send(sockfd, pong_frame, frame_len+6, 0);
        }
    }
    return 0;
}

void log_server_init(uint32_t port) {
    srand((unsigned int)time(NULL));

    WSADATA wsa;
    // SOCKET sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    // uint8_t mask[4];

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error Code: %d\n", WSAGetLastError());
        return;
    }

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation error. Error Code: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection failed. Error Code: %d\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        return;
    }

    snprintf(buffer, sizeof(buffer),    // Send WebSocket handshake
            "GET / HTTP/1.1\r\n"
            "Host: localhost:%d\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
            "Sec-WebSocket-Version: 13\r\n\r\n", port);
    send(sockfd, buffer, strlen(buffer), 0);

    // Receive handshake response
    recv(sockfd, buffer, sizeof(buffer), 0);

    h_threads = (HANDLE*)malloc(n_times * sizeof(HANDLE));
    thread_ids = (DWORD*)malloc(n_times * sizeof(DWORD));

    for(int i=0; i<n_times; i++) {
        h_threads[i] = CreateThread(NULL, 0, thread_func, NULL, 0, &thread_ids[i]);
    }
}

void log_server_send(const char *log_file, char *message) {
    if(sockfd == INVALID_SOCKET) {
        printf("ERROR: connection to log server failed");
        return;
    }
    // Prepare WebSocket frame
    char buffer[BUFFER_SIZE] = {0};
    size_t msg_len = snprintf(&buffer[6], sizeof(buffer), "{\"path\":\"%s\",\"log_string\":\"%s\"}", log_file, message);
    // printf("Sending string %s\n", message);

    buffer[0] = 0x81;           // FIN + text frame
    buffer[1] = 0x80 | msg_len; // Masked + payload length

    // Generate masking key
    uint8_t mask[4];
    for (int i = 0; i < 4; i++) {
        mask[i] = rand() % 256;
    }
    memcpy(&buffer[2], mask, 4);
    mask_payload((uint8_t *)&buffer[6], msg_len, mask);    // Mask the payload
    send(sockfd, buffer, msg_len + 6, 0);
    // if(term == 1) exit(1);
}

void log_server_close() {
    uint8_t mask[4];
    char buffer[BUFFER_SIZE];
    // Send close frame
    buffer[0] = 0x88; // FIN + close frame
    buffer[1] = 0x80; // Masked, no payload
    for (int i = 0; i < 4; i++) {
        mask[i] = rand() % 256;
    }
    memcpy(&buffer[2], mask, 4);
    send(sockfd, buffer, 6, 0);
    if(SOCKET_ERROR == shutdown(sockfd, SD_SEND)) {
        printf("Websocket shutdown failed with error %d\n", WSAGetLastError());
    }

    // Close connection
    closesocket(sockfd);
    WSACleanup();

    // Wait for thread to complete, it will gracefully complete when the connection is closed
    WaitForMultipleObjects(n_times, h_threads, TRUE, INFINITE);

    // Clean up
    for(int i=0; i<n_times; i++) {
        CloseHandle(h_threads[i]);
    }
    free(h_threads);
    free(thread_ids);
}

// Example:
// #define PORT 8765 // Replace with your server port
// int main() {
//     log_server_init(PORT);
//     uint8_t counter = 0;
//     while(counter < 100) {
//         log_server_send("logs/test.log", "new test log string! La-La-La");
//         Sleep(10000);
//     }
//     log_server_close();
//     return 0;
// }