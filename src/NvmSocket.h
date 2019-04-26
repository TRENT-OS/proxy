/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
 */
#pragma once

#include "IoDevices.h"
#include "Nvm.h"

#include "LibDebug/Debug.h"

using namespace std;

class NvmSocket : public IoDevice
{
    private:
    Nvm* socket;

    public:
    NvmSocket() : socket()
    {
    }

    ~NvmSocket()
    {
        if (socket != nullptr)
        {
            delete socket;
        }
    }

    int Create()
    {
    	socket = new Nvm();
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


