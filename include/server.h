#ifndef COMP4981A2_SERVER_H

#define COMP4981A2_SERVER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define FULL_PATH_LENGTH 1024
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define UINT16_MAX 65535
#define BASE_TEN 10

static const char *const end_msg = "\n--- Command Execution Complete ---\n";

#endif    // COMP4981A2_SERVER_H
