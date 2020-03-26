
#include "ChannelAdmin.h"
#include "GuestConnector.h"
#include "LibDebug/Debug.h"
#include "utils.h"

bool is_network_tap_channel(unsigned int logicalChannel)
{
    return (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW  ||  logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW_2);
}

void ToGuestThread(ChannelAdmin *channelAdmin, SharedResource<PseudoDevice> *pseudoDevice, unsigned int logicalChannel, InputDevice *channel)
{

    size_t bufSize = 2048;    //256;
    vector<char> buffer(bufSize);
    int readBytes, writtenBytes;

    GuestConnector guestConnector(pseudoDevice, GuestConnector::GuestDirection::TO_GUEST);

    if (!guestConnector.IsOpen())
    {
        Debug_LOG_FATAL("ToGuestThread[%1d]: pseudo device not open.\n", logicalChannel);
        return;
    }

    Debug_LOG_DEBUG("ToGuestThread[%1d]: starting...\n", logicalChannel);

    try
    {
        while (true)
        {
            /* This is a blocking call with a timeout. */
            readBytes = channel->Read(buffer);

            if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN)
            {
                if (channelAdmin->CloseWasRequested(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN))
                {
                   // Leave the endless loop in case a close of the wan
                   break;
                }
            }
            else if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW)
            {
                if (channelAdmin->CloseWasRequested(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW))
                {
                   // Leave the endless loop in case a close of the wan
                   break;
                }

             }

            else if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW_2)
            {
                if (channelAdmin->CloseWasRequested(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW_2))
                {
                   // Leave the endless loop in case a close of the wan
                   break;
                }

             }

            if (readBytes > 0)
            {
                Debug_LOG_DEBUG("ToGuestThread[%1d]: bytes received from socket: %d.\n", logicalChannel, readBytes);
                if (readBytes >= 2)
                {
                    Debug_LOG_DEBUG("ToGuestThread[%1d]: first bytes: %02x %02x\n", logicalChannel, buffer[0], buffer[1]);
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
                    Debug_LOG_DEBUG("ToGuestThread[%1d]: closing client connection thread. Read result: %d\n", logicalChannel, readBytes);
                    break;
                }
            }
        }
    }
    catch (...)
    {
        Debug_LOG_ERROR("ToGuestThread[%1d] exception\n", logicalChannel);
    }

    Debug_LOG_DEBUG("ToGuestThread[%1d]: closing socket\n", logicalChannel);

    if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN_CONTROL_CHANNEL) // TODO: remove (there is no control channel thread any more)
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
    else if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW)
    {
        Debug_LOG_INFO("ToGuestThread[%1d]: closing the current NW client connection\n", logicalChannel);
    }
    else if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW_2)
    {
        Debug_LOG_INFO("ToGuestThread[%1d]: closing the current NW-2 client connection\n", logicalChannel);
    }


    if (channelAdmin->GetChannel(logicalChannel) != nullptr)
    {
        bool unsolicited = true;
        if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN)
        {
            if (channelAdmin->CloseWasRequested(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN))
            {
                unsolicited = false;
            }
        }

        if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW)
        {
            if (channelAdmin->CloseWasRequested(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW))
            {
                unsolicited = false;
            }
        }
        if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW_2)
        {
            if (channelAdmin->CloseWasRequested(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW_2))
            {
                unsolicited = false;
            }
        }

        Debug_LOG_DEBUG("ToGuestThread[%1d]: deactivating the socket; unsolicited: %s \n", logicalChannel, (unsolicited ? "true" : "false"));
        channelAdmin->DeactivateChannel(logicalChannel, unsolicited);
    }

    Debug_LOG_DEBUG("ToGuestThread[%1d]: completed\n", logicalChannel);
}

// Possible contexts how to get here:
// - from guest thread: wants to activate the WAN socket
// - from the LAN server: wants to activate a newly created client socket
int ChannelAdmin::ActivateChannel(unsigned int logicalChannel, IoDevice *ioDevice)
{

    // Check the logical channel is valid
    if (logicalChannel >= UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_MAX)
    {
        Debug_LOG_INFO("ActivateChannel: %d\n", -1);
        return -1;
    }

    // Check valid arguments
    if (ioDevice == nullptr)
    {
        Debug_LOG_ERROR("ActivateChannel: bad input args\n");
        return -1;
    }


    int result = -1;

    lock.lock();

    /* Implicit check if the thread for this logical channel is already existing. */
    if (guestListeners.GetListener(logicalChannel) == nullptr)
    {

        result = ioDevice->Create();

        if (result >= 0)
        {
            // Store the io device;
            ioDevices[logicalChannel] = ioDevice;

            // Reset the close requested flag.
          	closeWasRequested[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN] = false;
            closeWasRequested[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW] = false;
            closeWasRequested[UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW_2] = false;

            // Register socket in GuestListeners
            guestListeners.SetListener(logicalChannel, ioDevice->GetOutputDevice());

            /* Only create a thread for network communication (using the tap devices) */
            if (is_network_tap_channel(logicalChannel))
            {
                // Create thread
                toGuestThreads[logicalChannel] =
                thread{ToGuestThread, this, pseudoDevice, PARAM(logicalChannel, logicalChannel), ioDevice->GetInputDevice()};
            }
        }
        else
        {
           Debug_LOG_ERROR("ActivateChannel: error creating io devices\n");
        }
    }
    else
    {
        Debug_LOG_ERROR("ActivateChannel: do not activate - logical channel already exisiting\n");
    }

    lock.unlock();

    Debug_LOG_DEBUG("ActivateChannel: %d\n", result);

    if (result  < 0)
    {
        delete ioDevice;
    }

    return result;
}

// Possible contexts how to get here:
// - to guest threads (LAN, WAN, control channel): at the end of their life time
int ChannelAdmin::DeactivateChannel(unsigned int logicalChannel, bool unsolicited)
{
    int result = 0;

    lock.lock();

    if (guestListeners.GetListener(logicalChannel) != nullptr)
    {
        if (unsolicited == false)
        {
            OutputDevice *outputDevice = GetChannel(logicalChannel);
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

    Debug_LOG_DEBUG("DeactivateChannel: %d\n", result);

    return result;
}

OutputDevice *ChannelAdmin::GetChannel(unsigned int logicalChannel) const
{
    // Use GuestListeners to map logical channel to socket.
    return guestListeners.GetListener(logicalChannel);
}

void ChannelAdmin::SendDataToChannel(unsigned int logicalChannel, const vector<char> &buffer)
{
    OutputDevice *outputDevice = GetChannel(logicalChannel);
    if (nullptr == outputDevice)
    {
        Debug_LOG_ERROR("[channel %u] no output device", logicalChannel);
        return;
    }

    lock.lock();
    int ret = outputDevice->Write(buffer);
    lock.unlock();

    if (ret < 0)
    {
        Debug_LOG_ERROR("[channel %u] otuput device write failed, error %d",
                        logicalChannel, ret);
        return;
    }
}

bool ChannelAdmin::CloseWasRequested(unsigned int logicalChannel)
{
    if ((logicalChannel != UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN) || (logicalChannel != UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW) \
       || (logicalChannel != UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW_2))
    {
        return false;
    }

    lock.lock();
    bool result = closeWasRequested[logicalChannel];
    lock.unlock();

    return result;
}

void ChannelAdmin::RequestClose(unsigned int logicalChannel)
{
    if ((logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN) || (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW) \
       || (logicalChannel != UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW_2))
    {
        lock.lock();
        closeWasRequested[logicalChannel] = true;
        lock.unlock();
    }
}
