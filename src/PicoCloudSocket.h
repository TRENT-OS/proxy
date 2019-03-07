
#pragma once

#include "IoDevices.h"
#include "PicoSocket.h"

#include "LibDebug/Debug.h"

using namespace std;

class PicoCloudSocket : public IoDevice
{
    private:
    int port;
    string hostName;
    PicoSocket *socket;

    public:
    PicoCloudSocket(int port, string hostName) :
        port(port),
        hostName(hostName),
        socket(nullptr)
    {}

    ~PicoCloudSocket()
    {
        if (socket != nullptr)
        {
            delete socket;
        }
    }

    int Create()
    {
        socket = new PicoSocket{port, hostName};
        Debug_LOG_INFO("CloudSocket: create WAN socket: %s:%d\n", hostName.c_str(), port);

        // We set a timeout on the WAN socket: the thread will unblock on a regular basis and detect
        // if the socket is not existing any more - because it was closed by the from guest thread
        // on behalf of the guest - and run to completion.
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(socket->GetFileDescriptor(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

        return 1;
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
