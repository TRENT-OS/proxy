/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include "i_uart_fifo.h"
#include "lib_utils/CharFifo.h"

typedef struct
{
    IUartFifo   uartFifo;
    CharFifo    charFifo;
}
UartFifo;

bool UartFifoInit(UartFifo* self, size_t capacity);
