
#include "GuestConnector.h"
#include "Socket.h"
#include "ServerSocket.h"
#include "IoDevices.h"
#include "GuestListeners.h"
#include "MqttCloud.h"
#include "LogicalChannels.h"
#include "SocketAdmin.h"
#include "utils.h"

#include <thread>

using namespace std;

typedef enum
{
    UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN,
    UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN_CNF,
    UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_CLOSE,
    UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_CLOSE_CNF
} UartSocketGuestSocketCommand;

void SendDataToSocket(unsigned int logicalChannel, OutputDevice *outputDevice, vector<char> &buffer)
{
    int writtenBytes;

    if (outputDevice != nullptr)
    {
        writtenBytes = outputDevice->Write(buffer);
        if (writtenBytes < 0)
        {
            printf("FromGuestThread: socket write failed; %s.\n", strerror(errno));
        }
        else
        {
            printf("FromGuestThread: bytes written to socket: %d. From logical channel: %d\n", writtenBytes, logicalChannel);
        }
    }
}

void SendResponse(SocketAdmin *socketAdmin, UartSocketGuestSocketCommand command, unsigned int result)
{
    vector<char> response{2};

    if (command == UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN)
    {
        response[0] = static_cast<char>(UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN_CNF);
    }
    else
    {
        response[0] = static_cast<char>(UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_CLOSE_CNF);
    }

    response[1] = result;

    SendDataToSocket(LOGICAL_CHANNEL_CONTROL, socketAdmin->GetSocket(LOGICAL_CHANNEL_CONTROL), response);
}

void HandleSocketCommand(SocketAdmin *socketAdmin, vector<char> &buffer)
{
    UartSocketGuestSocketCommand command = static_cast<UartSocketGuestSocketCommand>(buffer[0]);
    unsigned int commandLogicalChannel = buffer[1];
    int result;

    printf("Handle socket command: cmd:%d channel:%d", command, commandLogicalChannel);
    if (commandLogicalChannel == LOGICAL_CHANNEL_LAN)
    {
        result = 0; // We do not allow the guest to handle the LAN socket -> fake success results
    }
    else if (commandLogicalChannel == LOGICAL_CHANNEL_CONTROL)
    {
        result = -1; // We do not allow the guest to handle the control channel -> return a failure
    }
    else if (commandLogicalChannel == LOGICAL_CHANNEL_WAN)
    {
        if (command == UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN)
        {
            result = socketAdmin->ActivateSocket(LOGICAL_CHANNEL_WAN, nullptr, nullptr);
        }
        else
        {
            result = socketAdmin->DeactivateSocket(LOGICAL_CHANNEL_WAN);
        }
    }

    printf("Handle socket command: result:%d", result);

    SendResponse(socketAdmin, command, PARAM(result, result < 0 ? 1 : 0));
}

void FromGuestThread(GuestConnector *guestConnector, SocketAdmin *socketAdmin)
{
    size_t bufSize = 1024;
    vector<char> buffer(bufSize);
    int readBytes, writtenBytes;

    printf("FromGuestThread: s00.\n");

    try
    {
        while (true)
        {
            unsigned int logicalChannel;
            buffer.resize(bufSize);
            int readBytes = guestConnector->Read(buffer.size(), &buffer[0], &logicalChannel);

            if (readBytes > 0)
            {
                //dumpFrame(&buffer[0], readBytes);
                buffer.resize(readBytes);

                if (logicalChannel == LOGICAL_CHANNEL_CONTROL)
                {
                    if (readBytes != 2)
                    {
                        printf("FromGuestThread: incoming socket command with wrong length.\n");
                    }
                    else
                    {
                        // Handle the commands arriving on the control channel
                        HandleSocketCommand(socketAdmin, buffer);
                    }
                }
                else
                {
                    // For LAN, WAN: write the received data to the according socket
                    OutputDevice *outputDevice = socketAdmin->GetSocket(logicalChannel);
                    SendDataToSocket(logicalChannel, outputDevice, buffer);
                }
            }
            else
            {
                // Not used because: not meaningful "Resource temporarily unavailable" 
                //printf("GuestConnectorFromGuest: guest read failed.\n");
            }
        }
    }
    catch (...)
    {
        printf("FromGuestThread exception\n");
    }
}

void LanServer(SocketAdmin *socketAdmin, unsigned int port)
{
    ServerSocket serverSocket(port);
    socklen_t clientLength;
    struct sockaddr_in clientAddress;
    unsigned int logicalChannel = LOGICAL_CHANNEL_LAN;

    serverSocket.Listen(5);

    try
    {
        while (true)
        {
            clientLength = sizeof(clientAddress);
            int newsockfd = serverSocket.Accept((struct sockaddr *) &clientAddress, &clientLength);

            if (socketAdmin->GetSocket(logicalChannel) == nullptr)
            {
                printf("LanServer: start server thread: in port %d, in address %x (file descriptor: %x)\n", clientAddress.sin_port, clientAddress.sin_addr.s_addr, newsockfd);
                socketAdmin->ActivateSocket(logicalChannel, new DeviceWriter{newsockfd}, new DeviceReader{newsockfd});
            }
            else
            {
                printf("LanServer: Error: do not start a new to-guest thread for LAN because such a thread is already active\n");
                close(newsockfd);
            }
        }
    }
    catch (...)
    {
        printf("LanServer exception\n");
    }
}

int main(int argc, const char *argv[])
{
    int port = SERVER_PORT;
    string hostName {SERVER_NAME};

    if (argc < 2)
    {
        printf("Usage: mqtt_proxy_demo QEMU_pseudo_terminal [cloud_host_name]\n");
        return 0;
    }

    string pseudoDeviceName{argv[1]};
    SharedResource<string> pseudoDevice{&pseudoDeviceName};

    GuestConnector guestConnector{&pseudoDevice, GuestConnector::GuestDirection::FROM_GUEST};
    if (!guestConnector.IsOpen())
    {
        printf("Could not open pseudo device.\n");
        return 0;
    }

    if (argc > 2)
    {
        hostName = string{argv[2]};
    }

    // Create the socket for the logical control channel thread.
    // The from guest thread will write the socket command confirms to this socket.
    Socket controlChannelSocket{7999, "127.0.0.1"};

    SocketAdmin socketAdmin{&pseudoDevice, hostName, port};

    // Activate the control channel socket.
    socketAdmin.ActivateSocket(LOGICAL_CHANNEL_CONTROL, &controlChannelSocket, &controlChannelSocket);

    // The "GUEST thread" is:
    // a) receiving all hdlc frames and distributing them to the sockets
    // b) handling the socket admin commands from the guest
    thread fromGuestThread{FromGuestThread, &guestConnector, &socketAdmin};

    // Handle the LAN socket
    LanServer(&socketAdmin, SERVER_PORT);

    // We never get here -> no cleanup

    return 0;
}
