
#pragma once

#include "GuestListeners.h"
#include "PseudoDevice.h"
#include "SharedResource.h"
#include "IoDevices.h"
#include "uart_socket_guest_rpc_conventions.h"


#include <string>
#include <thread>
#include <mutex>

using namespace std;

class ChannelAdmin
{
    public:
    ChannelAdmin(SharedResource<PseudoDevice> *pseudoDevice) :
        pseudoDevice{pseudoDevice},
        guestListeners{UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_MAX},
        closeWasRequested(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_MAX),
        toGuestThreads(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_MAX),
        ioDevices(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_MAX)
    {
    }

    int ActivateChannel(unsigned int logicalChannel, IoDevice *ioDevice);
    int DeactivateChannel(unsigned int logicalChannel, bool unsolicited);
    OutputDevice *GetChannel(unsigned int logicalChannel) const;
    void SendDataToChannel(unsigned int logicalChannel, const vector<char> &buffer);
    SharedResource<PseudoDevice> *GetPseudoDevice() const { return pseudoDevice; }

    /* These two members are the result of integration experience: use case: connection closed by cloud server. */
    bool CloseWasRequested(unsigned int logicalChannel);
    void RequestClose(unsigned int logicalChannel);

    private:
    SharedResource<PseudoDevice> *pseudoDevice;
    GuestListeners guestListeners;
    vector<bool> closeWasRequested;
    vector<thread> toGuestThreads;
    vector<IoDevice *> ioDevices;
    mutable mutex lock;
};
