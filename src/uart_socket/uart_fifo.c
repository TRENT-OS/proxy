#include "uart_fifo.h"
#include "lib_debug/Debug.h"

size_t FifoCapacity(struct _IUartFifo *instance)
{
    UartFifo* self = (UartFifo*) instance;

    return CharFifo_getCapacity(&self->charFifo);
}

size_t NumberOfBytesInFifo(struct _IUartFifo *instance)
{
    UartFifo* self = (UartFifo*) instance;

    return CharFifo_getSize(&self->charFifo);
}

int AddBytes(struct _IUartFifo *instance, size_t length, char *buf)
{
    UartFifo* self = (UartFifo*) instance;
    int retval = 0;

    if (CharFifo_getCapacity(&self->charFifo) -
            CharFifo_getSize(&self->charFifo) >= length)
    {
        for (size_t i = 0; i < length; i++)
        {
            bool ok = CharFifo_push(&self->charFifo, &buf[i]);
            Debug_ASSERT(ok);
        }
    }
    else
    {
        retval = -1;
    }
    return retval;
}

int RemoveBytes(struct _IUartFifo *instance, size_t length, char *buf)
{
    UartFifo* self = (UartFifo*) instance;
    int retval = 0;

    if (CharFifo_getSize(&self->charFifo) >= length)
    {
        for (size_t i = 0; i < length; i++)
        {
            buf[i] = CharFifo_getAndPop(&self->charFifo);
        }
    }
    else
    {
        retval = -1;
    }
    return retval;
}

bool UartFifoInit(UartFifo* self, size_t capacity)
{
    self->uartFifo.FifoCapacity         = FifoCapacity;
    self->uartFifo.NumberOfBytesInFifo  = NumberOfBytesInFifo;
    self->uartFifo.AddBytes             = AddBytes;
    self->uartFifo.RemoveBytes          = RemoveBytes;

    return CharFifo_ctor(&self->charFifo, capacity);
}
