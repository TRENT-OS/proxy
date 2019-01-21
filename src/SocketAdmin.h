
#pragma once

#include "GuestListeners.h"
#include "SharedResource.h"
#include "IoDevices.h"
#include "uart_socket_guest_rpc_conventions.h"


#include <string>
#include <thread>
#include <mutex>

using namespace std;

class SocketAdmin
{
    public:
    SocketAdmin(SharedResource<string> *pseudoDevice) :
        pseudoDevice{pseudoDevice},
        guestListeners{UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_MAX},
        closeWasRequested(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_MAX),
        toGuestThreads(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_MAX),
        ioDevices(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_MAX)
    {
    }

    int ActivateSocket(unsigned int logicalChannel, IoDevice *ioDevice);
    int DeactivateSocket(unsigned int logicalChannel, bool unsolicited);
    OutputDevice *GetSocket(unsigned int logicalChannel) const;
    void SendDataToSocket(unsigned int logicalChannel, const vector<char> &buffer);
    SharedResource<string> *GetPseudoDevice() const { return pseudoDevice; }
    bool CloseWasRequested(unsigned int logicalChannel);
    void RequestClose(unsigned int logicalChannel);

    private:
    SharedResource<string> *pseudoDevice;
    GuestListeners guestListeners;
    vector<bool> closeWasRequested;
    vector<thread> toGuestThreads;
    vector<IoDevice *> ioDevices;
    mutable mutex lock;
};
