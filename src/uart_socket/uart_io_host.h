/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

/*
    Note: this module represents the "host"-part (for Linux) of a uart socket connection.
*/

#include "i_uart_io.h"
#include <stdbool.h>


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "device_type.h"


typedef struct
{
    IUartIo implementation;
    const char *pseudoDevice; // Device name of pseudo terminal or socket port
    bool isBlocking;
    bool readOnly;
    bool writeOnly;
    int fd;
    int port;
} UartIoHost;

#define UART_IO_HOST_FLAG_NONBLOCKING (1U << 0)
#define UART_IO_HOST_FLAG_READ_ONLY (1U << 1)
#define UART_IO_HOST_FLAG_WRITE_ONLY (1U << 2)

#define BAUD B115200

void UartIoHostInit(UartIoHost *instance, const char *pseudoDevice, DeviceType *type, unsigned int flags);
