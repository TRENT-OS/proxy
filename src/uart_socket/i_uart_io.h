
#pragma once

#include <stdbool.h>

typedef struct
{
    unsigned char byte;
    int result;
} UartByteTransferOperation;

struct _IUartIo
{
    int (*Open)(struct _IUartIo *instance);
    int (*Close)(struct _IUartIo *instance);
    void (*WriteByte)(struct _IUartIo *instance, UartByteTransferOperation *uartByteTransferOperation);
    void (*ReadByte)(struct _IUartIo *instance, UartByteTransferOperation *uartByteTransferOperation);
};

typedef struct _IUartIo IUartIo;
