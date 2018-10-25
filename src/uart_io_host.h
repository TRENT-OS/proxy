
#pragma once

#include "i_uart_io.h"

#include <stdbool.h>

typedef struct
{
    IUartIo implementation;
    const char *pseudoDevice;
    bool isBlocking;
    bool readOnly;
    bool writeOnly;
    int fd;
} UartIoHost;

void UartIoHostInit(UartIoHost *instance, const char *pseudoDevice, bool isBlocking, bool readOnly, bool writeOnly);
