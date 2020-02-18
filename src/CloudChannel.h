
#pragma once

#include "IoDevices.h"
#include "Channel.h"

#include "LibDebug/Debug.h"

using namespace std;

class CloudChannel : public IoDevice
{
    private:
    int port;
    string hostName;
    Channel *channel;

    public:
    CloudChannel(int port, string hostName) :
        port(port),
        hostName(hostName),
        channel(nullptr)
    {}

    ~CloudChannel()
    {
        if (channel != nullptr)
        {
            delete channel;
        }
    }

    int Create()
    {
        channel = new Channel{port, hostName};
        Debug_LOG_INFO("CloudSocket: create WAN socket: %s:%d\n", hostName.c_str(), port);

        // We set a timeout on the WAN socket: the thread will unblock on a regular basis and detect
        // if the socket is not existing any more - because it was closed by the from guest thread
        // on behalf of the guest - and run to completion.
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(channel->GetFileDescriptor(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

        return 1;
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
