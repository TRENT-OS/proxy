/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
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
