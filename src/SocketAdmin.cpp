
#include "GuestConnector.h"
#include "Socket.h"
#include "ServerSocket.h"
#include "SharedResource.h"
#include "IoDevices.h"
#include "GuestListeners.h"
#include "uart_socket_guest_rpc_conventions.h"
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
        printf("ToGuestThread[%1d]: pseudo device not open.\n", logicalChannel);
        return;
    }

    try
    {
        while (true)
        {
            readBytes = socket->Read(buffer);
            if (readBytes > 0)
            {
                printf("ToGuestThread[%1d]: bytes received from socket: %d.\n", logicalChannel, readBytes);
                fflush(stdout);
                writtenBytes = guestConnector.Write(PARAM(logicalChannel, logicalChannel), readBytes, &buffer[0]);
                writtenBytes = 0;

                if (writtenBytes < 0)
                {
                    printf("ToGuestThread[%1d]: guest write failed.\n", logicalChannel);
                    fflush(stdout);
                }
            }
            else
            {
                printf("ToGuestThread[%1d]: closing client connection thread (file descriptor: %d).\n", logicalChannel, socket->GetFileDescriptor());
                if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN)
                {
                    printf("ToGuestThread[%1d]: the WAN socket was closed !!!!!\n", logicalChannel);
                    socket->Close();
                }

                break;
            }
        }
    }
    catch (...)
    {
        printf("ToGuestThread[%1d] exception\n", logicalChannel);
    }

    printf("ToGuestThread[%1d]: closing socket\n", logicalChannel);

    socket->Close();
}

// Possible contexts how to get here:
// - main: wants to activate the control channel socket
// - from guest thread: wants to activate the WAN socket
// - from the LAN server: wants to activate a newly created client socket
int SocketAdmin::ActivateSocket(unsigned int logicalChannel, OutputDevice *outputDevice, InputDevice *inputDevice) 
{
    // Check the logical channel is valid
    if (logicalChannel >= UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_MAX)
    {
        return -1;
    }

    // In case the logical channel is not free: fail
    if (guestListeners.GetListener(logicalChannel) != nullptr)
    {
        return -1;
    }

    // In case of WAN: create the real socket
    if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN)
    {
        wanSocket = new Socket{wanPort, wanHostName};
        outputDevice = wanSocket;
        inputDevice = wanSocket;
    }

    // Register socket in GuestListeners
    guestListeners.SetListener(logicalChannel, outputDevice);

    // Create thread
    toGuestThreads[logicalChannel] = thread{ToGuestThread, pseudoDevice, PARAM(logicalChannel, logicalChannel), inputDevice};

    // Return success indication
    return 0;
}

// Possible contexts how to get here:
// - from guest thread: wants to deactivate the WAN socket
int SocketAdmin::DeactivateSocket(unsigned int logicalChannel) 
{
    // Currently only WLAN is supported
    if (logicalChannel != UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN)
    {
        return -1;
    }

    // Return with success in case the logical channel is free (= nothing to do)
    if (guestListeners.GetListener(logicalChannel) == nullptr)
    {
        return 0;
    }

    // Try not to communicate with the thread: try to close the socket and hope in the thread the read will fail and the thread closes itself

    guestListeners.SetListener(logicalChannel, nullptr);
    wanSocket->Close();
    delete wanSocket;
    
#if 0
    // First version: do not sync with thread destruction and always indicate success
* signal to-guest thread: "kill yourself"
* wait for this operation to complete
* get the success indication of the thread
* reset the pointer in guestlisteners
#endif
}
    
OutputDevice *SocketAdmin::GetSocket(unsigned int logicalChannel) const
{
    // Use GuestListeners to map logical channel to socket.
    return guestListeners.GetListener(logicalChannel);
}
