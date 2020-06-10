
#include "uart_io_host.h"
#include "type.h"
#include "Channel.h"


#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netinet/tcp.h>



/*
    Note: the proper way of supporting sockets would have been to extend UartIoHost
    with a new member for the socket. This unfortunately was not possible because
    of QEMU - which is currently the only use case for the host side of a uart socket
    connection. QEMU does not support multiple clients to open a socket connection
    to it (via -serial tcp:localhost:4444). But the type UartIoHost was designed to
    support multiple instances. As a result UartIoHost currently only supports one
    socket which is shared across all UartIoHost instances.
*/

static unsigned int uartIoHostSocketInstances = 0;
static Channel *uartIoHostSocket = nullptr;

static int UartIoHostOpenStaticSocket(IUartIo *instance)
{
    UartIoHost *uartIoHost = container_of(instance, UartIoHost, implementation);

    if (uartIoHostSocket == nullptr)
    {
        uartIoHostSocket = new Channel(uartIoHost->port, "127.0.0.1");
        int fd = uartIoHostSocket->GetFileDescriptor();
        if (fd < 0)
        {
            printf("Could not create socket.\n");
            return -1;
        }

        if (uartIoHost->isBlocking == false)
        {
            int socketFlags = fcntl(fd, F_GETFL);
            if (socketFlags < 0)
            {
                printf("Could not read socket flags.");
                return -1;
            }

            int result = fcntl(fd, F_SETFL, socketFlags | O_NONBLOCK);
            if (result < 0)
            {
                printf("Could not set socket flags.");
                return -1;
            }
        }
    }

    ++uartIoHostSocketInstances;

    // Note: in case socket is already open: copy file descriptor of singleton socket to this instance
    uartIoHost->fd = uartIoHostSocket->GetFileDescriptor();

    //printf("Open: socket instances after open: %d - current fd: %d\n", uartIoHostSocketInstances, uartIoHost->fd);

   return (uartIoHost->fd == -1) ? -1 : 0;
}

static int UartIoHostCloseStaticSocket(IUartIo *instance)
{
    if (uartIoHostSocket != nullptr)
    {
        //printf("Close: current socket instances: %d\n", uartIoHostSocketInstances);

        if (--uartIoHostSocketInstances == 0)
        {
            delete uartIoHostSocket;
            uartIoHostSocket = nullptr;
        }
    }

    return 0;
}

static int UartIoHostOpen(IUartIo *instance)
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

static int UartIoHostClose(IUartIo *instance)
{
    UartIoHost *uartIoHost = container_of(instance, UartIoHost, implementation);

    return close(uartIoHost->fd);
}

static int UartIoHostOpenRawSerial(IUartIo *instance)
{
    UartIoHost *uartIoHost = container_of(instance, UartIoHost, implementation);

    uartIoHost->fd = open(uartIoHost->pseudoDevice, O_RDWR | O_NOCTTY);

    if (uartIoHost->fd == -1)
    {
        printf("Unable to open %s\n", uartIoHost->pseudoDevice);
        return -1;
    }

    int ret;
    struct termios ts;

    bzero(&ts, sizeof(ts));
    cfmakeraw(&ts);
    cfsetspeed(&ts, BAUD);
    ts.c_cflag |= (CLOCAL | CREAD | CSTOPB);
    tcflush(uartIoHost->fd, TCIOFLUSH);

    ret = tcsetattr(uartIoHost->fd, TCSANOW, &ts);
    if (ret == -1)
    {
        printf("Unable to set the parameters associated with the terminal ");
        return -1;
    }

    return (uartIoHost->fd == -1) ? -1 : 0;
}

static int UartIoHostCloseRawSerial(IUartIo *instance)
{
    UartIoHost *uartIoHost = container_of(instance, UartIoHost, implementation);

    return close(uartIoHost->fd);
}

static void UartIoHostWriteByte(IUartIo *instance, UartByteTransferOperation *uartByteTransferOperation)
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

static void UartIoHostReadByte(IUartIo *instance, UartByteTransferOperation *uartByteTransferOperation)
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

void UartIoHostInit(UartIoHost *instance, const char *pseudoDevice, DeviceType *type, unsigned int flags)
{
    char *str;
    long converted = strtol(pseudoDevice, &str, 10);

    // If the pseudo device is an interger: treat it as socket port rather than a pseudo device.

    switch (*type)
    {
    case DEVICE_TYPE_PSEUDO_CONSOLE:
        printf("Starting Pseudo Console\n");
        instance->port = 0;
        instance->implementation.Open = UartIoHostOpen;
        instance->implementation.Close = UartIoHostClose;
        break;

    case DEVICE_TYPE_SOCKET:
        printf("Starting Socket\n");
        instance->port = converted;
        instance->implementation.Open = UartIoHostOpenStaticSocket;
        instance->implementation.Close = UartIoHostCloseStaticSocket;
        break;

    case DEVICE_TYPE_RAW_SERIAL:
        printf("Starting Raw Serial\n");
        instance->port = 0;
        instance->implementation.Open = UartIoHostOpenRawSerial;
        instance->implementation.Close = UartIoHostCloseRawSerial;
        break;

    default:
        //return ("Unknown");
        break;
    }

    instance->implementation.WriteByte = UartIoHostWriteByte;
    instance->implementation.ReadByte = UartIoHostReadByte;

    instance->pseudoDevice = pseudoDevice;
    instance->isBlocking = (flags & UART_IO_HOST_FLAG_NONBLOCKING) == 0;
    instance->readOnly = (flags & UART_IO_HOST_FLAG_READ_ONLY) != 0;
    instance->writeOnly = (flags & UART_IO_HOST_FLAG_WRITE_ONLY) != 0;
    instance->fd = -1;
}
