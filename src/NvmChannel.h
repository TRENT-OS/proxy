/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include "IoDevices.h"
#include "Nvm.h"

#include "lib_debug/Debug.h"

using namespace std;

class NvmChannel : public IoDevice
{
private:
    Nvm* channel;
    unsigned m_chanNum;

public:
    NvmChannel(unsigned chanNum) : channel(), m_chanNum(chanNum)
    {
    }

    ~NvmChannel()
    {
        if (channel != nullptr)
        {
            delete channel;
        }
    }

    int Create()
    {
    	channel = new Nvm(m_chanNum);
        return 0;
    }

    OutputDevice *GetOutputDevice()
    {
        return channel;
    }

    InputDevice *GetInputDevice()
    {
        return channel;
    }
};


