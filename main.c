#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#define BUFFER_SIZE 1024

int sockfd;

void signal_handler(int sig) {
	printf("Closing socket and exiting...\n");
	close(sockfd);
	exit(0);
}

void send_data_to_websocket(const char *hostname, const char *port, const char *json_data) {
	struct addrinfo hints, *res;
	char buffer[BUFFER_SIZE];
	int bytes_sent;

	signal(SIGINT, signal_handler);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(hostname, port, &hints, &res) != 0) {
		perror("getaddrinfo");
		return;
	}

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd == -1) {
		perror("socket");
		freeaddrinfo(res);
		return;
	}

	if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
		perror("connect");
		close(sockfd);
		freeaddrinfo(res);
		return;
	}

	freeaddrinfo(res);

	// Send WebSocket handshake
	snprintf(buffer, sizeof(buffer),
	"GET / HTTP/1.1\r\n"
	"Host: %s:%s\r\n"
	"Upgrade: websocket\r\n"
	"Connection: Upgrade\r\n"
	"Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"
	"Sec-WebSocket-Version: 13\r\n\r\n",
	hostname, port);
	send(sockfd, buffer, strlen(buffer), 0);

	// Receive handshake response
	recv(sockfd, buffer, sizeof(buffer), 0);

	// Send JSON data frame
	snprintf(buffer, sizeof(buffer), "\x81%c%s", (unsigned char)strlen(json_data), json_data);
	bytes_sent = send(sockfd, buffer, strlen(buffer), 0);
	if (bytes_sent == -1) {
		perror("send");
	} else {
		printf("Sent %d bytes: %s\n", bytes_sent, json_data);
	}

	close(sockfd);
}

int main() {
	const char *hostname = "localhost";
	const char *port = "8765";
	const char *json_data = "{\"filename\": \"test_logs.log\", \"log_string\": \"Some log string, probably contains new lines\"}";

	send_data_to_websocket(hostname, port, json_data);

	return 0;
}
