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
    unsigned m_chanNum;
    Nvm* socket;

public:
    NvmSocket(unsigned chanNum) : socket(), m_chanNum(chanNum)
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
    	socket = new Nvm(m_chanNum);
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


