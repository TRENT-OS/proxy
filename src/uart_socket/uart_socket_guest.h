
#pragma once

#if !defined(CAMKES)
#   include "type.h"
#   include "uart_stream.h"
#   include "uart_fifo.h"
#   include "uart_hdlc.h"
#   include "uart_stream.h"
#endif

#include <stdbool.h>

/*
    UART socket guest provides a socket-like interface for a QEMU guest. Internally it uses
    UART stream to exchange data and socket related administration commands with the
    Linux proxy for the HAR.

    IMPORTANT: this module is intended to be used by a single thread.
*/

enum
{
    // The maximum number of supported sockets existing in parallel.
    UART_SOCKET_GUEST_MAX_NUM_SUPPORTED_SOCKETS = 2,

    // The number of logical channels we will use with UART stream: one for each supported socket plus the control channels.
    UART_SOCKET_GUEST_MAX_NUM_LOGICAL_CHANNELS = UART_SOCKET_GUEST_MAX_NUM_SUPPORTED_SOCKETS + 2,
};

typedef struct
{
#if !defined(CAMKES)
    // The pointers to all the FIFOs.
    IUartFifo* uartFifos[UART_SOCKET_GUEST_MAX_NUM_LOGICAL_CHANNELS];

    // The FIFO for the control channel.
    UartFifo controlChannelUartFifo;

    UartStream uartStream;
#endif
    // Mapping from file descriptors to logical channels
    unsigned int
    fileDescriptorToLogicalChannelId[UART_SOCKET_GUEST_MAX_NUM_LOGICAL_CHANNELS];
}
UartSocketGuest;

int UartSocketGuestInit(UartSocketGuest* uartSocketGuest
#if !defined(CAMKES)
                        ,
                        UartHdlc* uartHdlc,                 // Initialized uart hdlc object
                        unsigned int
                        maxNumSockets,         // Maximum number of sockets that will be created (opened) by the guest
                        IUartFifo*
                        uartFifos[]              // maxNumSockets FIFOs are required; one per socket
#endif
                       );

void UartSocketGuestDeInit(UartSocketGuest* uartSocketGuest);

/*
    Instruct the Linux proxy to create a socket.

    Note: this signature supports both cases: LAN server socket and Cloud client socket.
    At a later state (after the demo) this functionality can be evolved into dedicated functions better
    resembling the real socket interface of Linux.

    Note: this call is blocking and returns the result happened in the Linux proxy.

    Note: the host name can be NULL: this indicates the LAN side server socket of the HAR.

    Note: this function fails if the socket is already open.

    Note: In the first version of this module for the demo this feature will only be supported in part:
    a) the guest has to open the LAN socket in order to be able to use it, but
    b) no interaction with the proxy is initiated = the proxy opens the LAN socket after startup automatically

    Note: returns the "file descriptor of the socket" (in case of >= 0).
*/
int UartSocketGuestOpen(UartSocketGuest* uartSocketGuest,
                        const char* hostName,
                        unsigned int port);

/*
    Analog functionality to open. Similar restrictions for the LAN socket.
*/
int UartSocketGuestClose(UartSocketGuest* uartSocketGuest, int socketFd);

/*
    Socket read. Blocking.

    Note: in the first round of implementation (fore the demo) this call does not return with size 0
    in case the socket was closed by the remote party.
*/
int UartSocketGuestRead(UartSocketGuest* uartSocketGuest,
                        int socketFd,
                        size_t length,
                        char* buf);

/*
    Socket read. Non-blocking.

    Note: this call always returns right away. It either returns the requested data (if availabe in
    the internal buffers) or returns with error (in case not enough data is contained in the internal
    buffers).
*/
int UartSocketGuestReadNonBlocking(UartSocketGuest* uartSocketGuest,
                                   int socketFd,
                                   size_t length,
                                   char* buf);

/*
    Socket write. Always blocking.
*/
int UartSocketGuestWrite(UartSocketGuest* uartSocketGuest,
                         int socketFd,
                         size_t length,
                         const char* buf);
