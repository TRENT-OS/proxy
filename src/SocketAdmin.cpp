
#include "SocketAdmin.h"

#include "GuestConnector.h"
#include "uart_socket_guest_rpc_conventions.h"
#include "MqttCloud.h"
#include "utils.h"

void ToGuestThread(SocketAdmin *socketAdmin, SharedResource<string> *pseudoDevice, unsigned int logicalChannel, InputDevice *socket)
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

                break;
            }
        }
    }
    catch (...)
    {
        printf("ToGuestThread[%1d] exception\n", logicalChannel);
    }

    printf("ToGuestThread[%1d]: closing socket\n", logicalChannel);

    if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_CONTROL_CHANNEL)
    {
        printf("ToGuestThread[%1d]: Unexpected stop of control channel thread !!!!!\n", logicalChannel);
    }
    else if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN)
    {
        printf("ToGuestThread[%1d]: the WAN socket was closed !!!!!\n", logicalChannel);
    }
    else if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN)
    {
        printf("ToGuestThread[%1d]: closing the current LAN client connection\n", logicalChannel);
    }

    socketAdmin->DeactivateSocket(logicalChannel);
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

    int result = -1;

    lock.lock();

    if (guestListeners.GetListener(logicalChannel) == nullptr)
    {
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
        toGuestThreads[logicalChannel] = thread{ToGuestThread, this, pseudoDevice, PARAM(logicalChannel, logicalChannel), inputDevice};

        result = 0;
    }

    lock.unlock();

    return result;
}

// Possible contexts how to get here:
// - from guest thread: wants to deactivate the WAN socket
// - to guest threads (LAN, WAN, control channel): at the end of their life time
int SocketAdmin::DeactivateSocket(unsigned int logicalChannel) 
{
    int result = 0;

    lock.lock();

    if (guestListeners.GetListener(logicalChannel) != nullptr)
    {
        guestListeners.SetListener(logicalChannel, nullptr);

        // Initial implementation: do not sync with WAN thread destruction and always indicate success - has to be tested

        if (logicalChannel != UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN)
        {
            // Note: if we were called from the from guest thread: 
            // Try not to communicate with the thread: try to close the socket and hope in the thread the read will fail and the thread closes itself
            wanSocket->Close();
            delete wanSocket;
        }
        else
        {
            OutputDevice *outputDevice = GetSocket(logicalChannel);
            outputDevice->Close();
        }
    }

    lock.unlock();

    return result;
}
    
OutputDevice *SocketAdmin::GetSocket(unsigned int logicalChannel) const
{
    // Use GuestListeners to map logical channel to socket.
    return guestListeners.GetListener(logicalChannel);
}

void SocketAdmin::SendDataToSocket(unsigned int logicalChannel, const vector<char> &buffer)
{
    int writtenBytes;
    OutputDevice *outputDevice = GetSocket(logicalChannel);

    lock.lock();

    if (outputDevice != nullptr)
    {
        writtenBytes = outputDevice->Write(buffer);
        if (writtenBytes < 0)
        {
            printf("logical channe: %d - socket write failed; %s.\n", logicalChannel, strerror(errno));
        }
        else
        {
            printf("logical channe: %d - bytes written to socket: %d.\n", logicalChannel, writtenBytes);
        }
    }

    lock.unlock();
}

