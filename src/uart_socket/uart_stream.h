/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include "i_uart_fifo.h"
#include "uart_hdlc.h"

#include <stdbool.h>

/*
    UART stream is a layer wrapping UART hdlc which uses a generic FIFO
    interface to provide segmentation / concatenation for a UART hdlc
    user in read direction.

    IMPORTANT: this module is intended to be used by a single thread.
*/

typedef struct
{
    UartHdlc *uartHdlc;

    /* The number of logical channels multiplexed on the UART hdlc physical channel. */
    unsigned int numLogicalChannels;

    /* The FIFO objects: one per logical channel. */
    IUartFifo** uartFifos;
} UartStream;

void UartStreamInit(UartStream *uartStream,
                    UartHdlc *uartHdlc,
                    unsigned int numLogicalChannels,
                    IUartFifo* uartFifos[]);

void UartStreamDeInit(UartStream *uartStream);

int UartStreamOpen(UartStream *uartStream);
int UartStreamClose(UartStream *uartStream);

/*
     This function always returns right away. If zero bytes have been read this
     indicates at least one more hdlc frame has to be received for this logical channel in
     order for a read of this length to be successful.
 */
int UartStreamRead(UartStream *uartStream,
                    unsigned int logicalChannel,
                    size_t length,
                    char *buf);

/*
    This function is blocking = it will return after all the bytes have been sent via hdlc.
*/
int UartStreamWrite(UartStream *uartStream,
                    unsigned int logicalChannel,
                    size_t length,
                    const char *buf);

/*
    This function is blocking = it will return after the next hdlc frame has been received completely.

    The intended usage of this function is: if the user has unsuccessfully tried to read from a logical
    channel with UartStreamRead then this function needs to be called. After its completion this function
    indicates to which logical channel the received hdlc frame belongs. This indicates to the user if
    it does make sense to retry a read on the wanted logical channel or if (at least) another hdlc
    frame needs to be received.
*/
int UartStreamFetchNextHdlcFrame(UartStream *uartStream,
                                 unsigned int *logicalChannel);
