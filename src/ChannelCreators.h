/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include "lib_debug/Debug.h"
#include "TapChannelCreator.h"
#include "NvmChannelCreator.h"
#include "ChanMuxTestChannelCreator.h"

#include <map>

using namespace std;

// Class that maps socket creators to their own channels. This is a kind
// of meta-creator. A creator of creators.
class ChannelCreators
{
private:
    map<int, IoDeviceCreator*>  creators;

public:
    ChannelCreators(bool use_tap)
    {
        // UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN has a separate handling
        if(use_tap)
        {
            creators[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW] =
                    new TapChannelCreator("tap0");

            creators[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW_2] =
                    new TapChannelCreator("tap1");

        }
        creators[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NVM] =
                new NvmChannelCreator(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NVM);

        creators[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NVM2] =
                new NvmChannelCreator(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NVM2);

        creators[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_CM_TEST_1] =
                new ChanMuxTestChannelCreator();

        creators[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_CM_TEST_2] =
                new ChanMuxTestChannelCreator();
    }

    ~ChannelCreators()
    {
        for (auto& kv : creators)
        {
            delete(kv.second);
        }
    }

    IoDeviceCreator* getCreator(unsigned logicalChannel)
    {
        Debug_LOG_TRACE("%s: called for channel %d", __func__, logicalChannel);
        return creators[logicalChannel];
    }
};
