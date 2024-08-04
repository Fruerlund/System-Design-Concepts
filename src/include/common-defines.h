/**
 * @file common-defines.h
 * @author Fruerlund
 * @brief 
 * @version 0.1
 * @date 2024-08-03
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef COMMON_DEFINES_H
#define COMMON_DEFINES_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/queue.h>
#include <pthread.h>
#include <fcntl.h> 
#include <sys/types.h>
#include <unistd.h> 
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#endif