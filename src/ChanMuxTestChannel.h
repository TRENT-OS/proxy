/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include "IoDevices.h"
#include "ChanMuxTest.h"

#include "lib_debug/Debug.h"

using namespace std;

class ChanMuxTestChannel : public IoDevice
{
private:
    ChanMuxTest* socket;

public:
    ChanMuxTestChannel() : socket()
    {
    }

    ~ChanMuxTestChannel()
    {
        if (socket != nullptr)
        {
            delete socket;
        }
    }

    int Create()
    {
    	socket = new ChanMuxTest();
        return 0;
    }

    OutputDevice *GetOutputDevice()
    {
        return socket;
    }

    InputDevice *GetInputDevice()
    {
        return socket;
    }
};
