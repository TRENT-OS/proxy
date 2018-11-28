
#include "GuestConnector.h"
#include "Socket.h"
#include "ServerSocket.h"
#include "SharedResource.h"
#include "IoDevices.h"
#include "GuestListeners.h"
#include "LogicalChannels.h"
#include "MqttCloud.h"
#include "SocketAdmin.h"
#include "utils.h"

#include <thread>

using namespace std;

void GuestConnectorToGuest(SharedResource<string> *pseudoDevice, unsigned int logicalChannel, InputDevice *socket)
{
    size_t bufSize = 256;
    vector<char> buffer(bufSize);
    int readBytes, writtenBytes;
    GuestConnector guestConnector(pseudoDevice, GuestConnector::GuestDirection::TO_GUEST);

    if (!guestConnector.IsOpen())
    {
        printf("GuestConnectorToGuest: pseudo device not open.\n");
        return;
    }

    try
    {
        while (true)
        {
            readBytes = socket->Read(buffer);
            if (readBytes > 0)
            {
                printf("GuestConnectorToGuest: bytes received from socket: %d.\n", readBytes);
                fflush(stdout);
                writtenBytes = guestConnector.Write(PARAM(logicalChannel, logicalChannel), readBytes, &buffer[0]);
                writtenBytes = 0;

                if (writtenBytes < 0)
                {
                    printf("GuestConnectorToGuest: guest write failed.\n");
                    fflush(stdout);
                }
            }
            else
            {
                printf("GuestConnectorToGuest: closing client connection thread (file descriptor: %d).\n", socket->GetFileDescriptor());
                if (logicalChannel == LOGICAL_CHANNEL_WAN)
                {
                    printf("GuestConnectorToGuest: the WAN socket was closed !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                    socket->Close();
                }

                break;
            }
        }
    }
    catch (...)
    {
        printf("GuestConnectorToGuest exception\n");
    }

    socket->Close();
}

#if 0
    SharedResource<string> pseudoDevice{pseudoDeviceName};

    Socket wanSocket {port, hostName};

    // Register the WAN socket as listening device for the WAN logical channel.
    guestListeners.SetListener(LOGICAL_CHANNEL_WAN, &wanSocket);

    // The "WAN thread": is waiting for data from the WAN and forwards it to the WAN logical channel.
    allThreads.push_back(thread{GuestConnectorToGuest, &pseudoDevice, PARAM(logicalChannel, LOGICAL_CHANNEL_WAN), &wanSocket});

    // The "GUEST thread" is receiving all hdlc frames and distributing them to the according guest listeners.
    allThreads.push_back(thread{GuestConnectorFromGuest, &pseudoDevice, &guestListeners});

    LanServer(&pseudoDevice, allThreads, guestListeners, SERVER_PORT);
#endif

int SocketAdmin::ActivateSocket(unsigned int logicalChannel, OutputDevice *outputDevice, InputDevice *inputDevice) 
{
    // Check the logical channel is valid
    if (logicalChannel >= LOGICAL_CHANNEL_MAX)
    {
        return -1;
    }

    // Check that logical channel is free
    if (guestListeners.GetListener(logicalChannel) != nullptr)
    {
        return -1;
    }

    // Register socket in GuestListeners
    guestListeners.SetListener(logicalChannel, outputDevice);

    // Create thread
    toGuestThreads[logicalChannel] = thread{GuestConnectorToGuest, pseudoDevice, PARAM(logicalChannel, LOGICAL_CHANNEL_WAN), inputDevice};
//void GuestConnectorToGuest(SharedResource<string> *pseudoDevice, unsigned int logicalChannel, InputDevice *socket)

    // Return success indication
    return 0;
}


int SocketAdmin::DeactivateSocket(unsigned int logicalChannel) 
{
    // Check the logical channel is valid
    if (logicalChannel >= LOGICAL_CHANNEL_MAX)
    {
        return -1;
    }

    // Check that logical channel is not free
    if (guestListeners.GetListener(logicalChannel) == nullptr)
    {
        return 0;
    }

    // First version: do not sync with thread destruction and always indicate success
    
#if 0

* signal to-guest thread: "kill yourself"
* wait for this operation to complete
* get the success indication of the thread
#endif

}
    
OutputDevice *SocketAdmin::GetSocket(unsigned int logicalChannel) const
{
    // Use GuestListeners to map logical channel to socket.
    return guestListeners.GetListener(logicalChannel);
}

