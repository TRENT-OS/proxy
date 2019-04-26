#pragma once

#include "LibDebug/Debug.h"
#include "CloudSocketCreator.h"
#include "PicoCloudSocketCreator.h"
#include "TapSocketCreator.h"
#include "NvmSocketCreator.h"

#include <map>

using namespace std;

// Class that maps socket creators to their own channels. This is a kind
// of meta-creator. A creator of creators.
class SocketCreators
{
private:
    map<int, IoDeviceCreator*>  creators;

public:
    SocketCreators(string hostName, int port, bool use_pico, bool use_tap)
    {
        // UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN has a separate handling
        if (use_pico)
        {
            creators[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN] =
                    new PicoCloudSocketCreator(port, hostName);
        }
        else if(use_tap == 0 &&  use_pico ==0)
        {
            creators[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN] =
                    new CloudSocketCreator{port, hostName};
        }
        else
        {
            creators[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN] =
                    new TapSocketCreator("tap0");
        }
        creators[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW] =
                    creators[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN];
        creators[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NVM] =
                new NvmSocketCreator();
    }

    ~SocketCreators()
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
