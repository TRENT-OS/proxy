/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
 */
#pragma once

#include "IoDevices.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "LibDebug/Debug.h"

using namespace std;

class Nvm : public InputDevice, public OutputDevice
{
public:
    Nvm()
    {
        Debug_LOG_DEBUG("%s", __func__);
    }

    int Read(vector<char> &buf)
    {
        Debug_LOG_DEBUG("%s", __func__);
        return 0; // This is a command channel not a stream
    }

    int Write(vector<char> buf)
    {
        Debug_LOG_DEBUG("%s", __func__);
        return 0;
    }

    int Close()
    {
        Debug_LOG_DEBUG("%s", __func__);
        return 0;
    }

    std::vector<char> HandlePayload(vector<char> buffer)
    {
        Debug_LOG_DEBUG("%s", __func__);
        return buffer;
    }

    ~Nvm()
    {
        Debug_LOG_DEBUG("%s", __func__);
    }
};
