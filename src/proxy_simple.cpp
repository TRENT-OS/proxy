
#include "GuestConnector.h"
#include "Socket.h"
#include "ServerSocket.h"
#include "IoDevices.h"
#include "GuestListeners.h"
#include "MqttCloud.h"
#include "uart_socket_guest_rpc_conventions.h"
#include "SocketAdmin.h"
#include "utils.h"

#include <thread>

using namespace std;

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

    socketAdmin->SendDataToSocket(
        UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_CONTROL_CHANNEL,
        response);
}

void HandleSocketCommand(SocketAdmin *socketAdmin, vector<char> &buffer)
{
    if (buffer.size() != 2)
    {
        printf("FromGuestThread: incoming socket command with wrong length.\n");
        return;
    }

    UartSocketGuestSocketCommand command = static_cast<UartSocketGuestSocketCommand>(buffer[0]);
    unsigned int commandLogicalChannel = buffer[1];
    int result;

    printf("Handle socket command: cmd:%d channel:%d", command, commandLogicalChannel);

    if (commandLogicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN)
    {
        result = 0; // We do not allow the guest to handle the LAN socket -> fake success results
    }
    else if (commandLogicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_CONTROL_CHANNEL)
    {
        result = -1; // We do not allow the guest to handle the control channel -> return a failure
    }
    else if (commandLogicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN)
    {
        if (command == UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN)
        {
            result = socketAdmin->ActivateSocket(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN, nullptr, nullptr);
        }
        else
        {
            result = socketAdmin->DeactivateSocket(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN);
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

                if (logicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_CONTROL_CHANNEL)
                {
                    // Handle the commands arriving on the control channel
                    HandleSocketCommand(socketAdmin, buffer);
                }
                else
                {
                    // For LAN, WAN: write the received data to the according socket
                    socketAdmin->SendDataToSocket(logicalChannel, buffer);
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
    unsigned int logicalChannel = UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_LAN;

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
    socketAdmin.ActivateSocket(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_CONTROL_CHANNEL, &controlChannelSocket, &controlChannelSocket);

    // The "GUEST thread" is:
    // a) receiving all hdlc frames and distributing them to the sockets
    // b) handling the socket admin commands from the guest
    thread fromGuestThread{FromGuestThread, &guestConnector, &socketAdmin};

    // Handle the LAN socket
    LanServer(&socketAdmin, SERVER_PORT);

    // We never get here -> no cleanup

    return 0;
}
