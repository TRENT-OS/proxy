#include "i_uart_fifo.h"
#include "uart_stream.h"
#include "lib_debug/Debug.h"

#include <stdio.h>
#include <stdbool.h>

void UartStreamInit(UartStream* uartStream,
                    UartHdlc* uartHdlc,
                    unsigned int numLogicalChannels,
                    IUartFifo* uartFifos[])
{
    Debug_ASSERT_SELF(uartStream);
    Debug_ASSERT(uartHdlc   != NULL && numLogicalChannels);
    Debug_ASSERT(uartFifos  != NULL);

    for (unsigned i = 0; i < numLogicalChannels; i++)
    {
        Debug_ASSERT(uartFifos[i]);
    }

    uartStream->uartHdlc            = uartHdlc;
    uartStream->numLogicalChannels  = numLogicalChannels;
    uartStream->uartFifos           = uartFifos;
}

void UartStreamDeInit(UartStream* uartStream)
{
}

int UartStreamOpen(UartStream* uartStream)
{
    return UartHdlcOpen(uartStream->uartHdlc);
}

int UartStreamClose(UartStream* uartStream)
{
    return UartHdlcClose(uartStream->uartHdlc);
}

int UartStreamRead(UartStream* uartStream,
                   unsigned int logicalChannel,
                   size_t length,
                   char* buf)
{
    IUartFifo* uartFifo = uartStream->uartFifos[logicalChannel];
    int bytesRead = 0;

    Debug_LOG_TRACE("%s: read operation with size: %d on channel: %d", __func__,
                    length, logicalChannel);

    unsigned int numberOfBytesInFifo = uartFifo->NumberOfBytesInFifo(uartFifo);

    Debug_LOG_TRACE("%s: FIFO current holds bytes: %u", __func__,
                    numberOfBytesInFifo);

    if (length - bytesRead <= numberOfBytesInFifo)
    {
        Debug_LOG_TRACE("%s: enough bytes in the FIFO to complete current read of size: %u",
                        __func__, length);

        if (uartFifo->RemoveBytes(uartFifo, length - bytesRead, buf + bytesRead) >= 0)
        {
            bytesRead += length - bytesRead;
        }
    }

    Debug_LOG_TRACE("%s: end with bytesRead: %d", __func__, bytesRead);

    return bytesRead;
}

int UartStreamWrite(UartStream* uartStream,
                    unsigned int logicalChannel,
                    size_t length,
                    const char* buf)
{
    Debug_LOG_TRACE("%s: write operation with size: %d on channel: %d", __func__,
                    length, logicalChannel);

    return (UartHdlcWrite(uartStream->uartHdlc, logicalChannel, length, buf));
}

int UartStreamFetchNextHdlcFrame(UartStream* uartStream,
                                 unsigned int* logicalChannel)
{
    /* This buffer size has to be greater than TinyFrame's internal read buffer size. */
    enum {MAX_HDLC_READ_BUF = 4096};

    static char hdlcReadBuf[MAX_HDLC_READ_BUF];
    int result = -1;

    /* Read the next HDLC frame. */
    int readBytes = UartHdlcRead(uartStream->uartHdlc,
                                 MAX_HDLC_READ_BUF,
                                 hdlcReadBuf,
                                 logicalChannel);
    if (readBytes > 0)
    {
        Debug_LOG_TRACE("%s: fetching next hdlc frame; readBytes: %u for channel %d of %d channels",
                        __func__,
                        readBytes,
                        *logicalChannel,
                        uartStream->numLogicalChannels);

        /* Write the number of read bytes to the FIFO. */
        if (*logicalChannel < uartStream->numLogicalChannels)
        {
            result = uartStream->uartFifos[*logicalChannel]->AddBytes(
                         uartStream->uartFifos[*logicalChannel], readBytes, hdlcReadBuf);
        }
    }

    return result;
}
