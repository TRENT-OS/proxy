
#pragma once

#include "i_uart_fifo.h"
#include "lib_util/CharFifo.h"

typedef struct
{
    IUartFifo   uartFifo;
    CharFifo    charFifo;
}
UartFifo;

bool UartFifoInit(UartFifo* self, size_t capacity);
