
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

using namespace std;

class GuestConnector
{
    public:
    enum GuestDirection
    {
        FROM_GUEST,
        TO_GUEST
    };

    GuestConnector(string pseudoDevice, GuestDirection guestDirection)
        : pseudoDevice(pseudoDevice)
    {
        if (guestDirection == FROM_GUEST)
        {
            UartIoHostInit(
                &uartIoHost, 
                pseudoDevice.c_str(), 
                PARAM(isBlocking, false),
                PARAM(readOnly, true),
                PARAM(writeOnly, false));
        }
        else
        {
            UartIoHostInit(
                &uartIoHost, 
                pseudoDevice.c_str(), 
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
        return UartHdlcRead(&uartHdlc, length, buf, logicalChannel); 
    } 

    int Write(unsigned int logicalChannel, unsigned int length, char *buf) 
    { 
        return UartHdlcWrite(&uartHdlc, logicalChannel, length, buf); 
    }

    bool IsOpen() const { return isOpen; }

    private:
    string pseudoDevice;
    UartIoHost uartIoHost;
    UartHdlc uartHdlc;
    bool isOpen;
};

