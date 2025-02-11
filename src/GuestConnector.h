/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include "type.h"
#include "uart_io_host.h"
#include "uart_hdlc.h"
#include "PseudoDevice.h"
#include "SharedResource.h"

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

class GuestConnector
{
public:
    enum GuestDirection
    {
        FROM_GUEST,
        TO_GUEST
    };

    GuestConnector(SharedResource<PseudoDevice> *pseudoDevice, GuestDirection guestDirection)
        : pseudoDevice(pseudoDevice)
    {
        pseudoDevice->Lock();
        PseudoDevice tempDevice = *pseudoDevice->GetResource();

        if (guestDirection == FROM_GUEST)
        {
            UartIoHostInit(
                &uartIoHost,
                tempDevice.GetResource()->c_str(),
                tempDevice.GetType(),
                UART_IO_HOST_FLAG_NONBLOCKING | UART_IO_HOST_FLAG_READ_ONLY);
        }
        else
        {
            UartIoHostInit(
                &uartIoHost,
                tempDevice.GetResource()->c_str(),
                tempDevice.GetType(),
                UART_IO_HOST_FLAG_WRITE_ONLY);
        }

        UartHdlcInit(&uartHdlc, &uartIoHost.implementation);

        isOpen = Open() >= 0;

        pseudoDevice->UnLock();
    }

    ~GuestConnector()
    {
        Close();
        UartHdlcDeInit(&uartHdlc);
    }

    int Read(unsigned int length, char *buf, unsigned int *logicalChannel)
    {
        int result;

        pseudoDevice->Lock();
        result = UartHdlcRead(&uartHdlc, length, buf, logicalChannel);
        pseudoDevice->UnLock();

        return result;
    }

    int Write(unsigned int logicalChannel, unsigned int length, const char *buf)
    {
        int result;

        pseudoDevice->Lock();
        result = UartHdlcWrite(&uartHdlc, logicalChannel, length, buf);
        pseudoDevice->UnLock();

        return result;
    }

    bool IsOpen() const { return isOpen; }

private:
    SharedResource<PseudoDevice> *pseudoDevice;
    UartIoHost uartIoHost;
    UartHdlc uartHdlc;
    bool isOpen;

    int Open() { return UartHdlcOpen(&uartHdlc); }
    int Close() { return UartHdlcClose(&uartHdlc); }
};
