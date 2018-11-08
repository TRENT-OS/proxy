
#pragma once

#include "type.h"
#include "uart_io_host.h"
#include "uart_hdlc.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <string>
#include <mutex>

using namespace std;

/*
    Note: the QEMU implementation of the UART is not thread safe.
    For the Linux side the UART appears in to be a pseudo device.
    As a result all threads accessing the same UART (pseudo device)
    have to be synchronized.
*/

template <typename T>
class SharedResource
{
    public:
    SharedResource(T value) : value(value) {}

    void Lock() const { lock.lock(); }
    void UnLock() const { lock.lock(); }
    T GetResource() const { return value; }

    private:
    T value;
    mutable std::mutex lock;
};

class GuestConnector
{
    public:
    enum GuestDirection
    {
        FROM_GUEST,
        TO_GUEST
    };

    GuestConnector(SharedResource<string> *pseudoDevice, GuestDirection guestDirection)
        : pseudoDevice(pseudoDevice)
    {
        if (guestDirection == FROM_GUEST)
        {
            UartIoHostInit(
                &uartIoHost, 
                pseudoDevice->GetResource().c_str(), 
                PARAM(isBlocking, false),
                PARAM(readOnly, true),
                PARAM(writeOnly, false));
        }
        else
        {
            UartIoHostInit(
                &uartIoHost, 
                pseudoDevice->GetResource().c_str(), 
                PARAM(isBlocking, true),
                PARAM(readOnly, false),
                PARAM(writeOnly, true));
        }

        UartHdlcInit(&uartHdlc, &uartIoHost.implementation);

        isOpen = Open() >= 0;
    }

    ~GuestConnector() 
    { 
        Close();
        UartHdlcDeInit(&uartHdlc); 
    }

    int Open() { return UartHdlcOpen(&uartHdlc); }
    int Close() { return UartHdlcClose(&uartHdlc); }

    int Read(unsigned int length, char *buf, unsigned int *logicalChannel) 
    { 
        int result;

        pseudoDevice->Lock();
        result = UartHdlcRead(&uartHdlc, length, buf, logicalChannel); 
        pseudoDevice->UnLock();

        return result;
    } 

    int Write(unsigned int logicalChannel, unsigned int length, char *buf) 
    { 
        int result;

        pseudoDevice->Lock();
        result = UartHdlcWrite(&uartHdlc, logicalChannel, length, buf); 
        pseudoDevice->UnLock();

        return result;
    }

    bool IsOpen() const { return isOpen; }

    private:
    SharedResource<string> *pseudoDevice;
    UartIoHost uartIoHost;
    UartHdlc uartHdlc;
    bool isOpen;
};

