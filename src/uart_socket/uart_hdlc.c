#include "uart_hdlc.h"
#include "type.h"
#include "lib_debug/Debug.h"

#include <stdio.h>
#include <stdbool.h>

static TF_Result TinyFrameListener(TinyFrame* tinyFrame, TF_Msg* msg)
{
    UartHdlc* uartHdlc = container_of(tinyFrame, UartHdlc, tinyFrame);

    uartHdlc->rxOfMsgInProgress = false;

    Debug_LOG_TRACE("%s: message received; size: %d", __func__, msg->len);

    if (msg->len <= uartHdlc->rxBufferLength)
    {
        memcpy(uartHdlc->rxBuffer, msg->data, msg->len);
        uartHdlc->rxResult = msg->len;
        uartHdlc->rxMsgType = msg->type;
    }
    else
    {
        uartHdlc->rxResult = -1;
    }

    return TF_STAY;
}

void TF_WriteImpl(TinyFrame* tf, const uint8_t* buff, uint32_t len)
{
    UartHdlc* uartHdlc = container_of(tf, UartHdlc, tinyFrame);
    UartByteTransferOperation uartByteTransferOperation;

    for (unsigned int k = 0; k < len; ++k)
    {
        uartByteTransferOperation.byte = buff[k];
        uartHdlc->uartIo->WriteByte(uartHdlc->uartIo, &uartByteTransferOperation);
    }
}

void UartHdlcInit(UartHdlc* uartHdlc, IUartIo* uartIo)
{
    uartHdlc->uartIo = uartIo;

    uartHdlc->rxOfMsgInProgress = false;
    TF_InitStatic(&uartHdlc->tinyFrame, TF_MASTER);

    TF_AddGenericListener(&uartHdlc->tinyFrame, TinyFrameListener);
}

void UartHdlcDeInit(UartHdlc* uartHdlc)
{
}

int UartHdlcOpen(UartHdlc* uartHdlc)
{
    return uartHdlc->uartIo->Open(uartHdlc->uartIo);
}

int UartHdlcClose(UartHdlc* uartHdlc)
{
    return uartHdlc->uartIo->Close(uartHdlc->uartIo);
}

int UartHdlcRead(UartHdlc* uartHdlc,
                 size_t length,
                 void* buf,
                 unsigned int* logicalChannel)
{
    UartByteTransferOperation uartByteTransferOperation;

    uartHdlc->rxOfMsgInProgress = true;
    uartHdlc->rxBufferLength = length;
    uartHdlc->rxBuffer = (char*)buf;
    uartHdlc->rxResult = -1;

    while (uartHdlc->rxOfMsgInProgress)
    {
        uartHdlc->uartIo->ReadByte(uartHdlc->uartIo, &uartByteTransferOperation);

        if (uartByteTransferOperation.result == 0)
        {
            Debug_LOG_TRACE("%s: received byte: %x", __func__,
                            uartByteTransferOperation.byte);
            TF_AcceptChar(&uartHdlc->tinyFrame, uartByteTransferOperation.byte);
        }
        else
        {
            uartHdlc->rxResult = -1;
            break;
        }
    }
    if (uartHdlc->rxResult != -1)
    {
        Debug_LOG_TRACE("%s: end receive operation of len %d with result: %zd",
                        __func__, uartHdlc->rxResult, length);
    }

    *logicalChannel = uartHdlc->rxMsgType;

    return uartHdlc->rxResult;
}

int UartHdlcWrite(UartHdlc* uartHdlc,
                  unsigned int logicalChannel,
                  size_t length,
                  const void* buf)
{
    TF_Msg msg;

    TF_ClearMsg(&msg);

    msg.type = logicalChannel;
    msg.data = (uint8_t*) buf;
    msg.len = length;

#if (Debug_Config_LOG_LEVEL ==  Debug_LOG_LEVEL_TRACE)
    for (size_t i = 0; i < length; i++)
    {
        Debug_LOG_TRACE("%s: transmitting byte: %x",
                        __func__, msg.data[i]);
    }
#endif
    if (TF_Send(&uartHdlc->tinyFrame, &msg))
    {
        return length;
    }
    else
    {
        return -1;
    }
}
