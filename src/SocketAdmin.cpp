
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

void ToGuestThread(SharedResource<string> *pseudoDevice, unsigned int logicalChannel, InputDevice *socket)
{
    size_t bufSize = 256;
    vector<char> buffer(bufSize);
    int readBytes, writtenBytes;
    GuestConnector guestConnector(pseudoDevice, GuestConnector::GuestDirection::TO_GUEST);

    if (!guestConnector.IsOpen())
    {
        printf("ToGuestThread: pseudo device not open.\n");
        return;
    }

    try
    {
        while (true)
        {
            readBytes = socket->Read(buffer);
            if (readBytes > 0)
            {
                printf("ToGuestThread: bytes received from socket: %d.\n", readBytes);
                fflush(stdout);
                writtenBytes = guestConnector.Write(PARAM(logicalChannel, logicalChannel), readBytes, &buffer[0]);
                writtenBytes = 0;

                if (writtenBytes < 0)
                {
                    printf("ToGuestThread: guest write failed.\n");
                    fflush(stdout);
                }
            }
            else
            {
                printf("ToGuestThread: closing client connection thread (file descriptor: %d).\n", socket->GetFileDescriptor());
                if (logicalChannel == LOGICAL_CHANNEL_WAN)
                {
                    printf("ToGuestThread: the WAN socket was closed !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                    socket->Close();
                }

                break;
            }
        }
    }
    catch (...)
    {
        printf("ToGuestThread exception\n");
    }

    socket->Close();
}

int SocketAdmin::ActivateSocket(unsigned int logicalChannel, OutputDevice *outputDevice, InputDevice *inputDevice) 
{
    // Check the logical channel is valid
    if (logicalChannel >= LOGICAL_CHANNEL_MAX)
    {
        return -1;
    }

    // In case of LAN: do nothing and return with success
    if (logicalChannel == LOGICAL_CHANNEL_LAN)
    {
        return 0;
    }

    // In case the logical channel is not free: fail
    if (guestListeners.GetListener(logicalChannel) != nullptr)
    {
        return -1;
    }

    // Register socket in GuestListeners
    guestListeners.SetListener(logicalChannel, outputDevice);

    // Create thread
    toGuestThreads[logicalChannel] = thread{ToGuestThread, pseudoDevice, PARAM(logicalChannel, LOGICAL_CHANNEL_WAN), inputDevice};

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

    // In case of LAN: do nothing and return with success
    if (logicalChannel == LOGICAL_CHANNEL_LAN)
    {
        return 0;
    }

    // Return with success in case the logical channel is free (= nothing to do)
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

