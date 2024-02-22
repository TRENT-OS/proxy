/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#include "type.h"

#include "uart_socket_guest.h"
#include "uart_socket_guest_rpc_conventions.h"
#include "lib_debug/Debug.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include "lib_compiler/compiler.h"

#if defined(CAMKES)
#include "OS_Error.h"

extern void* chanMuxDataPort;
extern void* chanMuxCtrlDataPort;

extern OS_Error_t ChanMux_write(unsigned chan, size_t len, size_t* written);
extern OS_Error_t ChanMux_read(unsigned chan, size_t len, size_t* read);

extern void dataAvailable_wait(void);
#endif

enum
{
    // The capacity for the FIFO for the logical channel carrying the "control channel" to the Linux proxy.
    UART_SOCKET_GUEST_CONTROL_CHANNEL_FIFO_CAPACITY = 16 * 4096,
};

#if defined(CAMKES)

//------------------------------------------------------------------------------
static size_t
chanWriteSync(
    UartSocketLogicalChannelConvention chan,
    const void* buf,
    size_t len)
{
    void* chanDataportMap[] =
    {
        chanMuxDataPort,
        chanMuxDataPort,
        chanMuxCtrlDataPort,
        chanMuxCtrlDataPort
    };
    size_t written = 0;

    len = len < PAGE_SIZE ? len : PAGE_SIZE;
    // copy in the dataport
    memcpy(chanDataportMap[chan], buf, len);
    // tell the other side how much data we want to send and in which channel
    OS_Error_t err = ChanMux_write(chan, len, &written);
    Debug_ASSERT_PRINTFLN(!err, "OS err %d", err);

    return written;
}


//------------------------------------------------------------------------------
static size_t
chanRead(
    UartSocketLogicalChannelConvention chan,
    void* buf,
    size_t len)
{
    void* chanDataportMap[] =
    {
        chanMuxDataPort,
        chanMuxDataPort,
        chanMuxCtrlDataPort,
        chanMuxCtrlDataPort
    };
    size_t read = 0;
    OS_Error_t err = ChanMux_read(chan, len, &read);
    Debug_ASSERT_PRINTFLN(!err, "OS err %d", err);

    if (read)
    {
        Debug_ASSERT(read <= len);
        memcpy(buf, chanDataportMap[chan], read);
    }
    return read;
}


#endif // defined(CAMKES)


//------------------------------------------------------------------------------
static int UartSocketGuestSendSocketCommand(
    UartSocketGuest* uartSocketGuest,
    UartSocketGuestSocketCommand socketCommand,
    unsigned int logicalChannel)
{
    char command[2];
    char response[2];
    UartSocketLogicalChannelConvention chan = 0;

    Debug_ASSERT_SELF(uartSocketGuest);
    Debug_ASSERT(logicalChannel < UART_SOCKET_GUEST_MAX_NUM_SUPPORTED_SOCKETS);

    if ((socketCommand != UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN) &&
        (socketCommand != UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_CLOSE))
    {
        return -1;
    }
    command[0] = (char)socketCommand;
    command[1] = (char)logicalChannel;
#if !defined(CAMKES)
    int result =  UartStreamWrite(
                      &uartSocketGuest->uartStream,
                      PARAM(logicalChannel, UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_CONTROL_CHANNEL),
                      2,
                      command);
#else
    if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN
        ||
        logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN_CONTROL_CHANNEL)
    {
        chan = UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN_CONTROL_CHANNEL;
    }
    else if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN
             ||
             logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN_CONTROL_CHANNEL)
    {
        chan = UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN_CONTROL_CHANNEL;
    }
    else
    {
        Debug_LOG_DEBUG("cannot handle logical channel %d", logicalChannel);
        return -1;
    }
    unsigned result = chanWriteSync(chan, command, sizeof(command));
#endif
    if (result != sizeof(command))
    {
        Debug_LOG_DEBUG("wanted to write %d but got %d",
                        sizeof(command), result);
        return -1;
    }

    result =  UartSocketGuestRead(
                  uartSocketGuest,
#if !defined(CAMKES)
                  PARAM(logicalChannel,
                        UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_CONTROL_CHANNEL),
#else
                  chan,
#endif
                  sizeof(response),
                  response);
    if (result != sizeof(response))
    {
        Debug_LOG_DEBUG("On channel %d: wanted to read %d but got %d",
                        chan, result, sizeof(response));
        return -1;
    }

    unsigned int expectedResponse;
    if (socketCommand == UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN)
    {
        expectedResponse = UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN_CNF;
    }
    else
    {
        expectedResponse = UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_CLOSE_CNF;
    }

    if (response[0] == expectedResponse)
    {
        if (response[1] == 0)
        {
            return 0;
        }
        else
        {
            Debug_LOG_DEBUG("On channel %d: expected response argumment %d but got %d",
                            chan, 0, response[1]);
            return -1;
        }
    }
    else
    {
        Debug_LOG_DEBUG("On channel %d: expected response %d but got %d",
                        chan, expectedResponse, response[0]);
        return -1;
    }
}


//------------------------------------------------------------------------------
int UartSocketGuestInit(
    UartSocketGuest* uartSocketGuest
#if !defined(CAMKES)
    ,
    UartHdlc* uartHdlc,                 // Initialized uart hdlc object
    unsigned int
    maxNumSockets,         // Maximum number of sockets that will be created (opened) by the guest
    IUartFifo*
    uartFifos[]              // maxNumSockets FIFOs are required; one per socket
#endif
)
{
    Debug_ASSERT_SELF(uartSocketGuest);
#if !defined(CAMKES)
    Debug_ASSERT(uartHdlc != NULL);
    Debug_ASSERT(maxNumSockets == 2);
    Debug_ASSERT(uartFifos != NULL);

    // Initialize the control channel FIFO.
    bool ok = UartFifoInit(&uartSocketGuest->controlChannelUartFifo,
                           UART_SOCKET_GUEST_CONTROL_CHANNEL_FIFO_CAPACITY);
    Debug_ASSERT(ok);

    // Set the pointers to the FIFOs.
    for (unsigned k = 0; k < maxNumSockets; ++k)
    {
        uartSocketGuest->uartFifos[k] = uartFifos[k];
    }

    // Set the FIFO pointer of the control channel.
    uartSocketGuest->uartFifos[UART_SOCKET_GUEST_MAX_NUM_LOGICAL_CHANNELS - 1] =
        &uartSocketGuest->controlChannelUartFifo.uartFifo;
#endif
    // Initialize the mapping from file descriptor to logical channel id.
    uartSocketGuest->fileDescriptorToLogicalChannelId[0] =
        UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_UNDEFINED;
    uartSocketGuest->fileDescriptorToLogicalChannelId[1] =
        UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_UNDEFINED;
    uartSocketGuest->fileDescriptorToLogicalChannelId[2] =
        UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN_CONTROL_CHANNEL;
    uartSocketGuest->fileDescriptorToLogicalChannelId[3] =
        UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN_CONTROL_CHANNEL;

#if !defined(CAMKES)
    // Initialize the UART stream.
    UartStreamInit(
        &uartSocketGuest->uartStream,
        uartHdlc,
        UART_SOCKET_GUEST_MAX_NUM_LOGICAL_CHANNELS,
        uartSocketGuest->uartFifos);

    return UartStreamOpen(&uartSocketGuest->uartStream);
#else
    return 0;
#endif
}


//------------------------------------------------------------------------------
void UartSocketGuestDeInit(
UartSocketGuest* uartSocketGuest)
{
    Debug_ASSERT_SELF(uartSocketGuest);

#if !defined CAMKES
    UartStreamClose(&uartSocketGuest->uartStream);
    UartStreamDeInit(&uartSocketGuest->uartStream);
#endif
}


//------------------------------------------------------------------------------
int UartSocketGuestOpen(
    UartSocketGuest* uartSocketGuest,
    const char* hostName,
    unsigned int port)
{
    Debug_ASSERT_SELF(uartSocketGuest);

    // Currently we only support a maximum of one WAN and one LAN socket.
    UartSocketLogicalChannelConvention logicalChannelConventionToBeCreated;
    if (hostName != NULL)
    {
        logicalChannelConventionToBeCreated
            = UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN;
    }
    else
    {
        logicalChannelConventionToBeCreated
            = UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN;
    }

    if (uartSocketGuest->fileDescriptorToLogicalChannelId[logicalChannelConventionToBeCreated]
        == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_UNDEFINED)
    {
        int result;

        result = UartSocketGuestSendSocketCommand(
                     uartSocketGuest,
                     UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN,
                     PARAM(logicalChannel, logicalChannelConventionToBeCreated));

        if (result < 0)
        {
            return result;
        }
        else
        {
            uartSocketGuest->fileDescriptorToLogicalChannelId[logicalChannelConventionToBeCreated]
                = logicalChannelConventionToBeCreated;
            return logicalChannelConventionToBeCreated;
        }
    }
    return -1;
}


//------------------------------------------------------------------------------
int UartSocketGuestClose(
    UartSocketGuest* uartSocketGuest,
    int socketFd)
{
    Debug_ASSERT_SELF(uartSocketGuest);

    //Debug_ASSERT(socketFd < UART_SOCKET_GUEST_MAX_NUM_SUPPORTED_SOCKETS);
    //Debug_ASSERT(socketFd >= 0);

    // Succeed if the file descriptor is inavalid.
    if ((socketFd >= UART_SOCKET_GUEST_MAX_NUM_SUPPORTED_SOCKETS)
        || ((socketFd < 0)))
    {
        return 0;
    }

    // Succeed if socket is not open.
    if (uartSocketGuest->fileDescriptorToLogicalChannelId[socketFd]
        == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_UNDEFINED)
    {
        return 0;
    }

    int result;

    // Try to close the proxy socket belonging to the file descriptor's logical channel.
    result = UartSocketGuestSendSocketCommand(
                 uartSocketGuest,
                 UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_CLOSE,
                 PARAM(logicalChannel,
                       uartSocketGuest->fileDescriptorToLogicalChannelId[socketFd]));

    if (result >= 0)
    {
        uartSocketGuest->fileDescriptorToLogicalChannelId[socketFd]
            = UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_UNDEFINED;
    }

    return result;
}


//------------------------------------------------------------------------------
int UartSocketGuestRead(
    UartSocketGuest* uartSocketGuest,
    int socketFd,
    size_t length,
    char* buf)
{
    Debug_ASSERT_SELF(uartSocketGuest);
    Debug_ASSERT(socketFd < UART_SOCKET_GUEST_MAX_NUM_LOGICAL_CHANNELS);
    Debug_ASSERT(socketFd >= 0);
    Debug_ASSERT(buf != NULL);

#if !defined(CAMKES)
    int result = 0;

    while (result == 0)
    {
        unsigned int logicalChannel;

        // Non-blocking read.
        result = UartStreamRead(
                     &uartSocketGuest->uartStream,
                     PARAM(logicalChannel, socketFd),
                     length,
                     buf);

        if (result > 0)
        {
            break;
        }

        // Blocking fetch of the next hdlc frame.
        result = UartStreamFetchNextHdlcFrame(&uartSocketGuest->uartStream,
                                              &logicalChannel);

        if (result < 0)
        {
            break;
        }

        result = 0;
    }

    return result;
#else
    size_t totRead = 0;

    while (totRead < length)
    {
        DECL_UNUSED_VAR(unsigned int logicalChannel);

        // Non-blocking read.
        size_t read = chanRead(PARAM(logicalChannel, socketFd),
                               &buf[totRead],
                               length - totRead);
        if (!read)
        {
            dataAvailable_wait();
        }
        else
        {
            totRead += read;
        }
    }
    return totRead;
#endif
}


//------------------------------------------------------------------------------
int UartSocketGuestReadNonBlocking(
    UartSocketGuest* uartSocketGuest,
    int socketFd,
    size_t length,
    char* buf)
{
    Debug_ASSERT_SELF(uartSocketGuest);
    Debug_ASSERT(socketFd < UART_SOCKET_GUEST_MAX_NUM_SUPPORTED_SOCKETS);
    Debug_ASSERT(socketFd >= 0);
    Debug_ASSERT(buf != NULL);

#if !defined(CAMKES)
    return UartStreamRead(
               &uartSocketGuest->uartStream,
               PARAM(logicalChannel, socketFd),
               length,
               buf);
#else
    return chanRead(PARAM(logicalChannel, socketFd),
                    buf,
                    length);
#endif
}


//------------------------------------------------------------------------------
int UartSocketGuestWrite(
    UartSocketGuest* uartSocketGuest,
    int socketFd,
    size_t length,
    const char* buf)
{
    Debug_ASSERT_SELF(uartSocketGuest);
    Debug_ASSERT(socketFd < UART_SOCKET_GUEST_MAX_NUM_SUPPORTED_SOCKETS);
    Debug_ASSERT(socketFd >= 0);
    Debug_ASSERT(buf != NULL);

#if !defined(CAMKES)
    return UartStreamWrite(
               &uartSocketGuest->uartStream,
               PARAM(logicalChannel, socketFd),
               length,
               buf);
#else
    return chanWriteSync(PARAM(logicalChannel, socketFd),
                         buf,
                         length);
#endif
}
