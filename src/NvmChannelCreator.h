/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include "IoDevices.h"
#include "NvmChannel.h"

#include "lib_debug/Debug.h"


using namespace std;

class NvmChannelCreator : public IoDeviceCreator
{
    private:
        unsigned m_chanNum;
    public:
    NvmChannelCreator(unsigned chanNum) : m_chanNum(chanNum)
    {
    }

    ~NvmChannelCreator()
    {
    }

    IoDevice *Create()
    {
        return new NvmChannel(m_chanNum);
    }
};
