
#pragma once

#include "GuestListeners.h"
#include "SharedResource.h"

#include <thread>

using namespace std;

class SocketAdmin
{
    public:
    SocketAdmin(SharedResource<string> *pseudoDevice, string wanHostName, int wanPort) :
        wanPort{wanPort},
        wanHostName{wanHostName},
        pseudoDevice{pseudoDevice},
        guestListeners{LOGICAL_CHANNEL_MAX},
        toGuestThreads{LOGICAL_CHANNEL_MAX}
    {
    }

    int ActivateSocket(unsigned int logicalChannel, OutputDevice *outputDevice , InputDevice *inputDevice);
    int DeactivateSocket(unsigned int logicalChannel);
    OutputDevice *GetSocket(unsigned int logicalChannel) const;

    private:
    int wanPort;
    string wanHostName;
    SharedResource<string> *pseudoDevice;
    Socket *wanSocket;

    GuestListeners guestListeners;
    vector<thread> toGuestThreads;
};
