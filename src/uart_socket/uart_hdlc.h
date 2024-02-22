/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include "i_uart_io.h"
#include "TinyFrame.h"

typedef struct
{
    const char      *pseudoDevice;
    bool            rxOfMsgInProgress;
    unsigned int    rxBufferLength;
    char            *rxBuffer;
    unsigned char   rxMsgType;
    int             rxResult;
    IUartIo         *uartIo;
    TinyFrame       tinyFrame;
} UartHdlc;

void UartHdlcInit(UartHdlc *uartHdlc, IUartIo *uartIo);
void UartHdlcDeInit(UartHdlc *uartHdlc);

int UartHdlcOpen(UartHdlc *uartHdlc);
int UartHdlcClose(UartHdlc *uartHdlc);

/* The read operation yields the next physical hdlc frame plus its logical channel. */
int UartHdlcRead(UartHdlc *uartHdlc,
                 size_t length,
                 void *buf,
                 unsigned int *logicalChannel);

/* Write the hdlc frame using the given logical channel. */
int UartHdlcWrite(UartHdlc *uartHdlc,
                  unsigned int logicalChannel,
                  size_t length,
                  const void *buf);


