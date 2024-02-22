/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include "IoDevices.h"
#include "ChanMuxTestChannel.h"

#include "lib_debug/Debug.h"

using namespace std;

class ChanMuxTestChannelCreator : public IoDeviceCreator
{
    private:

    public:
    ChanMuxTestChannelCreator()
    {
    }

    ~ChanMuxTestChannelCreator()
    {
    }

    IoDevice *Create()
    {
        return new ChanMuxTestChannel();
    }
};
