
#include "SocketAdmin.h"
#include "GuestConnector.h"
#include "LibDebug/Debug.h"
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
        Debug_LOG_FATAL("ToGuestThread[%1d]: pseudo device not open.\n", logicalChannel);
        return;
    }

    Debug_LOG_INFO("ToGuestThread[%1d]: starting...\n", logicalChannel);

    try
    {
        while (true)
        {
            readBytes = socket->Read(buffer);

            if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN)
            {
                if (socketAdmin->CloseWasRequested(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN))
                {
                   // Leave the endless loop in case a close of the wan
                   break;
                }
            }

            if (readBytes > 0)
            {
                Debug_LOG_INFO("ToGuestThread[%1d]: bytes received from socket: %d.\n", logicalChannel, readBytes);
                if (readBytes >= 2)
                {
                    Debug_LOG_INFO("ToGuestThread[%1d]: first bytes: %02x %02x\n", logicalChannel, buffer[0], buffer[1]);
                }

                fflush(stdout);
                writtenBytes = guestConnector.Write(PARAM(logicalChannel, logicalChannel), readBytes, &buffer[0]);
                writtenBytes = 0;

                if (writtenBytes < 0)
                {
                    Debug_LOG_ERROR("ToGuestThread[%1d]: guest write failed.\n", logicalChannel);
                    fflush(stdout);
                }
            }
            else
            {
                // Has the cloud server closed the connection?
                if (readBytes == 0)
                {
                    Debug_LOG_INFO("ToGuestThread[%1d]: closing client connection thread. Read result: %d\n", logicalChannel, readBytes);
                    break;
                }
            }
        }
    }
    catch (...)
    {
        Debug_LOG_ERROR("ToGuestThread[%1d] exception\n", logicalChannel);
    }

    Debug_LOG_INFO("ToGuestThread[%1d]: closing socket\n", logicalChannel);

    if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_CONTROL_CHANNEL)
    {
        Debug_LOG_ERROR("ToGuestThread[%1d]: Unexpected stop of control channel thread !!!!!\n", logicalChannel);
    }
    else if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN)
    {
        Debug_LOG_INFO("ToGuestThread[%1d]: the WAN socket was closed\n", logicalChannel);
    }
    else if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN)
    {
        Debug_LOG_INFO("ToGuestThread[%1d]: closing the current LAN client connection\n", logicalChannel);
    }

    if (socketAdmin->GetSocket(logicalChannel) != nullptr)
    {
        bool unsolicited = true;
        if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN)
        {
            if (socketAdmin->CloseWasRequested(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN))
            {
                unsolicited = false;
            }
        }

        Debug_LOG_INFO("ToGuestThread[%1d]: deactivating the socket; unsolicited: %s \n", logicalChannel, (unsolicited ? "true" : "false"));
        socketAdmin->DeactivateSocket(logicalChannel, unsolicited);
    }

    Debug_LOG_INFO("ToGuestThread[%1d]: completed\n", logicalChannel);
}

// Possible contexts how to get here:
// - from guest thread: wants to activate the WAN socket
// - from the LAN server: wants to activate a newly created client socket
int SocketAdmin::ActivateSocket(unsigned int logicalChannel, IoDevice *ioDevice)
{
    // Check the logical channel is valid
    if (logicalChannel >= UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_MAX)
    {
        Debug_LOG_INFO("ActivateSocket: %d\n", -1);
        return -1;
    }

    // Check valid arguments
    if (ioDevice == nullptr)
    {
        Debug_LOG_ERROR("ActivateSocket: bad input args\n");
        return -1;
    }

    int result = -1;

    lock.lock();

    if (guestListeners.GetListener(logicalChannel) == nullptr)
    {
        result = ioDevice->Create();
        if (result >= 0)
        {
            // Store the io device;
            ioDevices[logicalChannel] = ioDevice;


            // Reset the close requested flag.
            if(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_TAP == logicalChannel)
            {
            	closeWasRequested[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_TAP] = false;

            	InputDevice* socket =  ioDevice->GetInputDevice();
            	int res = socket->getMac("tap0");

            }
            else
            {
            	closeWasRequested[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN] = false;

            }

            // Register socket in GuestListeners
            guestListeners.SetListener(logicalChannel, ioDevice->GetOutputDevice());

            // Create thread
            toGuestThreads[logicalChannel] =
            thread{ToGuestThread, this, pseudoDevice, PARAM(logicalChannel, logicalChannel), ioDevice->GetInputDevice()};
        }
        else
        {
           Debug_LOG_ERROR("ActivateSocket: error creating io devices\n");
        }
    }
    else
    {
        Debug_LOG_ERROR("ActivateSocket: do not activate - logical channel already exisiting\n");
    }

    lock.unlock();

    Debug_LOG_INFO("ActivateSocket: %d\n", result);

    if (result  < 0)
    {
        delete ioDevice;
    }

    return result;
}

// Possible contexts how to get here:
// - to guest threads (LAN, WAN, control channel): at the end of their life time
int SocketAdmin::DeactivateSocket(unsigned int logicalChannel, bool unsolicited) 
{
    int result = 0;

    lock.lock();

    if (guestListeners.GetListener(logicalChannel) != nullptr)
    {
        if (unsolicited == false)
        {
            OutputDevice *outputDevice = GetSocket(logicalChannel);
            outputDevice->Close();
        }

        delete ioDevices[logicalChannel];

        guestListeners.SetListener(logicalChannel, nullptr);

        // Wait until the according to guest thread has run to completion.
        if (unsolicited == false)
        {
            //toGuestThreads[logicalChannel].join();
            toGuestThreads[logicalChannel].detach();
        }
        else
        {
            toGuestThreads[logicalChannel].detach();
        }
    }

    lock.unlock();

    Debug_LOG_INFO("DeactivateSocket: %d\n", result);

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
            Debug_LOG_ERROR("logical channel: %d - socket write failed; %s.\n", logicalChannel, strerror(errno));
        }
        else
        {
            Debug_LOG_INFO("logical channel: %d - bytes written to socket: %d.\n", logicalChannel, writtenBytes);
        }
    }

    lock.unlock();
}

bool SocketAdmin::CloseWasRequested(unsigned int logicalChannel)
{
    if (logicalChannel != UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN)
    {
        return false;
    }

    lock.lock();
    bool result = closeWasRequested[logicalChannel];
    lock.unlock();

    return result;
}

void SocketAdmin::RequestClose(unsigned int logicalChannel)
{
    if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN)
    {
        lock.lock();
        closeWasRequested[logicalChannel] = true;
        lock.unlock();
    }
}
