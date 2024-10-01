#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// Compile with the  -lws2_32 flag!!!

void init_connection(uint32_t port);
void send_data(uint8_t *log_file, uint8_t *message);
void close_connection(void);
