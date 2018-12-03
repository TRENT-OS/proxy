
#pragma once

#include "GuestListeners.h"
#include "SharedResource.h"
#include "Socket.h"
#include "uart_socket_guest_rpc_conventions.h"


#include <string>
#include <thread>
#include <mutex>

using namespace std;

class SocketAdmin
{
    public:
    SocketAdmin(SharedResource<string> *pseudoDevice, string wanHostName, int wanPort) :
        wanPort{wanPort},
        wanHostName{wanHostName},
        pseudoDevice{pseudoDevice},
        wanSocket(nullptr),
        guestListeners{UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_MAX},
        closeWasRequested(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_MAX),
        toGuestThreads(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_MAX)
    {
    }

    int ActivateSocket(unsigned int logicalChannel, OutputDevice *outputDevice , InputDevice *inputDevice);
    int DeactivateSocket(unsigned int logicalChannel, bool unsolicited);
    OutputDevice *GetSocket(unsigned int logicalChannel) const;
    void SendDataToSocket(unsigned int logicalChannel, const vector<char> &buffer);
    SharedResource<string> *GetPseudoDevice() const { return pseudoDevice; }
    bool CloseWasRequested(unsigned int logicalChannel);
    void RequestClose(unsigned int logicalChannel);

    private:
    int wanPort;
    string wanHostName;
    SharedResource<string> *pseudoDevice;
    Socket *wanSocket;
    GuestListeners guestListeners;
    vector<bool> closeWasRequested;
    vector<thread> toGuestThreads;
    mutable mutex lock;
};
