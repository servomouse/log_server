#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// Compile with the  -lws2_32 flag!!!

void log_server_init(uint32_t port);
void log_server_send(const char *log_file, char *message);
void log_server_close(void);
