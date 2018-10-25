
#include "uart_io_host.h"
#include "type.h"
#include "utils.h"

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

static int Open(IUartIo *instance)
{
    UartIoHost *uartIoHost = container_of(instance, UartIoHost, implementation);
    int flags = O_NOCTTY;

    if (uartIoHost->readOnly)
    {
        flags |= O_RDONLY;
    }
    else if (uartIoHost->readOnly)
    {
        flags |= O_WRONLY;
    }
    else 
    {
        flags |= O_RDWR;
    }

    if (!uartIoHost->isBlocking)
    {
        flags |= O_NDELAY;
    }

    uartIoHost->fd = open(uartIoHost->pseudoDevice, flags);

    return (uartIoHost->fd == -1) ? -1 : 0;
}

static int Close(IUartIo *instance)
{
    UartIoHost *uartIoHost = container_of(instance, UartIoHost, implementation);

    return close(uartIoHost->fd);
}

static void WriteByte(IUartIo *instance, UartByteTransferOperation *uartByteTransferOperation)
{
    UartIoHost *uartIoHost = container_of(instance, UartIoHost, implementation);
    char buf[1];
    int n;


    buf[0] = uartByteTransferOperation->byte;

    //printf("write: %02x ", (unsigned char)buf[0]);
    //fflush(stdout);

    n = write(uartIoHost->fd, buf, 1);
    if (n > 0)
    {
        uartByteTransferOperation->result = 0;
    }
    else
    {
        uartByteTransferOperation->result = -1;
    }
}

static void ReadByte(IUartIo *instance, UartByteTransferOperation *uartByteTransferOperation)
{
    UartIoHost *uartIoHost = container_of(instance, UartIoHost, implementation);
    char buf[1];
    int n;

    n = read(uartIoHost->fd, buf, 1);
    if (n > 0)
    {
        uartByteTransferOperation->byte = buf[0];
        uartByteTransferOperation->result = 0;

        //printf("%02x ", (unsigned char)buf[0]);
        //fflush(stdout);
    }
    else
    {
        uartByteTransferOperation->result = -1;
    }
}

void UartIoHostInit(UartIoHost *instance, const char *pseudoDevice, bool isBlocking, bool readOnly, bool writeOnly)
{
    instance->implementation.Open = Open;
    instance->implementation.Close = Close;
    instance->implementation.WriteByte = WriteByte;
    instance->implementation.ReadByte = ReadByte;

    instance->pseudoDevice = pseudoDevice;
    instance->isBlocking = isBlocking;
    instance->readOnly = readOnly;
    instance->writeOnly = writeOnly;
}

