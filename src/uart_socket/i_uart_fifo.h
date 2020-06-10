
#pragma once

#include <stdlib.h>

/* FIFO abstraction to be used by UART stream wrapper of UART hdlc module. */
struct _IUartFifo
{
    /* Return the (static) capacity of the FIFO. */
    size_t (*FifoCapacity)(struct _IUartFifo *instance);

    /* Return the current number of bytes stored in the FIFO. */
    size_t (*NumberOfBytesInFifo)(struct _IUartFifo *instance);

    /* Add bytes to the FIFO which are defined by the given length, buf. */
    /* Return: 0 = success, -1 = failure. In case of failure no bytes have been added. */
    int (*AddBytes)(struct _IUartFifo *instance, size_t length, char *buf);

    /* Remove length bytes from the FIFO and store them in buf. */
    /* Return: 0 = success, -1 = failure. In case of failure no bytes will have been removed. */
    int (*RemoveBytes)(struct _IUartFifo *instance, size_t length, char *buf);
};

typedef struct _IUartFifo IUartFifo;
