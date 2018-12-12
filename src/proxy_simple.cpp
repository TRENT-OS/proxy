

#include "GuestConnector.h"
#include "ServerSocket.h"
#include "Socket.h"
#include "MqttCloud.h"
#include "SocketAdmin.h"
#include "LibDebug/Debug.h"
#include "uart_socket_guest_rpc_conventions.h"
#include "utils.h"

#include <chrono>
#include <thread>

using namespace std;

string UartSocketCommand(UartSocketGuestSocketCommand command)
{
    switch (command)
    {
        case UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN:
        return ("Open");

        case UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN_CNF:
        return ("OpenCnf");

        case UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_CLOSE:
        return ("Close");

        case UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_CLOSE_CNF:
        return ("CloseCnf");

        default:
        return ("Unknown");
    }
}

void WriteToGuest(SharedResource<string> *pseudoDevice, unsigned int logicalChannel, const vector<char> &buffer)
{
    int writtenBytes;
    GuestConnector guestConnector(pseudoDevice, GuestConnector::GuestDirection::TO_GUEST);

    if (!guestConnector.IsOpen())
    {
        Debug_LOG_FATAL("WriteToGuest[%1d]: pseudo device not open.\n", logicalChannel);
        return;
    }

    try
    {
        writtenBytes = guestConnector.Write(PARAM(logicalChannel, logicalChannel), buffer.size(), &buffer[0]);
        if (writtenBytes < 0)
        {
            Debug_LOG_ERROR("WriteToGuest[%1d]: guest write failed.\n", logicalChannel);
        }
    }
    catch (...)
    {
        Debug_LOG_ERROR("WriteToGuest[%1d] exception\n", logicalChannel);
    }

    guestConnector.Close();
}

void SendResponse(SocketAdmin *socketAdmin, UartSocketGuestSocketCommand command, unsigned int result)
{
    vector<char> response(2);

    if (command == UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN)
    {
        response[0] = static_cast<char>(UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_OPEN_CNF);
    }
    else
    {
        response[0] = static_cast<char>(UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_CLOSE_CNF);
    }

    response[1] = result;

    Debug_LOG_INFO("Socket command response: cmd: %s result: %d\n",
        UartSocketCommand(static_cast<UartSocketGuestSocketCommand>(response[0])).c_str(),
        response[1]);

    WriteToGuest(
        socketAdmin->GetPseudoDevice(),
        UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_CONTROL_CHANNEL,
        response);
}

void HandleSocketCommand(SocketAdmin *socketAdmin, vector<char> &buffer)
{
    if (buffer.size() != 2)
    {
        Debug_LOG_ERROR("FromGuestThread: incoming socket command with wrong length.\n");
        return;
    }

    UartSocketGuestSocketCommand command = static_cast<UartSocketGuestSocketCommand>(buffer[0]);
    unsigned int commandLogicalChannel = buffer[1];
    int result;

    Debug_LOG_INFO("Handle socket command: cmd: %s channel: %d\n",
        UartSocketCommand(command).c_str(),
        commandLogicalChannel);

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
            //result = socketAdmin->DeactivateSocket(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN, false);
            socketAdmin->RequestClose(UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_WAN);
            result = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 2));
        }
    }

    // Debug_LOG_INFO("Handle socket command: result:%d\n", result);

    SendResponse(socketAdmin, command, PARAM(result, result < 0 ? 1 : 0));
}

void FromGuestThread(GuestConnector *guestConnector, SocketAdmin *socketAdmin)
{
    size_t bufSize = 1024;
    vector<char> buffer(bufSize);
    int readBytes, writtenBytes;

    Debug_LOG_INFO("FromGuestThread: starting.\n");

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
                //Debug_LOG_ERROR("GuestConnectorFromGuest: guest read failed.\n");
            }
        }
    }
    catch (...)
    {
        Debug_LOG_ERROR("FromGuestThread exception\n");
    }
}

void LanServer(SocketAdmin *socketAdmin, unsigned int lanPort)
{
    ServerSocket serverSocket(lanPort);
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
                Debug_LOG_INFO("LanServer: start server thread: in port %d, in address %x (file descriptor: %x)\n", clientAddress.sin_port, clientAddress.sin_addr.s_addr, newsockfd);
                socketAdmin->ActivateSocket(logicalChannel, new DeviceWriter{newsockfd}, new DeviceReader{newsockfd});
            }
            else
            {
                Debug_LOG_ERROR("LanServer: Error: do not start a new to-guest thread for LAN because such a thread is already active\n");
                close(newsockfd);
            }
        }
    }
    catch (...)
    {
        Debug_LOG_ERROR("LanServer exception\n");
    }
}

int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: mqtt_proxy_demo QEMU_pseudo_terminal [lan port] [cloud_host_name] [cloud_port]\n");
        return 0;
    }

    string pseudoDeviceName{argv[1]};

    int lanPort = SERVER_PORT;
    if (argc > 2)
    {
        lanPort = atoi(argv[2]);
    }

    string hostName {SERVER_NAME};
    if (argc > 3)
    {
        hostName = string{argv[3]};
    }

    int port = SERVER_PORT;
    if (argc > 4)
    {
        port = atoi(argv[4]);
    }

    printf("Starting mqtt proxy on lan port: %d with pseudo device: %s using cloud host: %s port: %d\n", 
        lanPort, 
        pseudoDeviceName.c_str(),
        hostName.c_str(), 
        port);

    SharedResource<string> pseudoDevice{&pseudoDeviceName};

    GuestConnector guestConnector{&pseudoDevice, GuestConnector::GuestDirection::FROM_GUEST};
    if (!guestConnector.IsOpen())
    {
        Debug_LOG_FATAL("Could not open pseudo device.\n");
        return 0;
    }

    SocketAdmin socketAdmin{&pseudoDevice, hostName, port};

    // The "GUEST thread" is:
    // a) receiving all hdlc frames and distributing them to the sockets
    // b) handling the socket admin commands from the guest
    thread fromGuestThread{FromGuestThread, &guestConnector, &socketAdmin};

    // Handle the LAN socket
    LanServer(&socketAdmin, lanPort);

    // We never get here -> no cleanup

    return 0;
}
