/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
 */
#pragma once

#include "IoDevices.h"
#include "ChanMuxTest.h"

#include "LibDebug/Debug.h"

using namespace std;

class ChanMuxTestSocket : public IoDevice
{
private:
    ChanMuxTest* socket;

public:
    ChanMuxTestSocket() : socket()
    {
    }

    ~ChanMuxTestSocket()
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
