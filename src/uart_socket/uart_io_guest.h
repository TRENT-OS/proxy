
#pragma once

#include "i_uart_io.h"
#include "uart.h"

#include <stdbool.h>

typedef struct
{
    IUartIo implementation;
    bool isBlocking;
    UartIoWaitFunc wait;
    QemuUart uart;
} UartIoGuest;

#define UART_IO_GUEST_FLAG_NONBLOCKING (1U << 0)

void UartIoGuestInit(UartIoGuest* instance,
                     unsigned int flags,
                     UartIoWaitFunc wait

#if !defined(CAMKES)
                     , void* baseAddress
#endif
                    );
