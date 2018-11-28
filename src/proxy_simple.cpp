
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

            // Check for control channel

            // Check for correct channel (either lan or wan)

            if (readBytes > 0)
            {
                //dumpFrame(&buffer[0], readBytes);
                buffer.resize(readBytes);
                OutputDevice *outputDevice = socketAdmin->GetSocket(logicalChannel);
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

#if 0
            // Register the new LAN socket as (the new) listening device for the LAN logical channel.
            guestListeners.SetListener(LOGICAL_CHANNEL_LAN, new DeviceWriter(newsockfd));

            // A new "LAN thread" is started: it is waiting for data from the LAN and forwards it to the LAN logical channel.
            allThreads.push_back(thread{GuestConnectorToGuest, pseudoDevice, PARAM(logicalChannel, logicalChannel), new DeviceReader(newsockfd)});
#endif
        }
    }
    catch (...)
    {
        printf("LanServer exception\n");
    }
}

int main(int argc, const char *argv[])
{
    unsigned int port = SERVER_PORT;
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

    SocketAdmin socketAdmin{&pseudoDevice, hostName, port};

    // The "GUEST thread" is:
    // a) receiving all hdlc frames and distributing them to the sockets
    // b) handling the socket admin commands from the guest
    auto fromGuestThread{thread{FromGuestThread, &guestConnector, &socketAdmin}};

    // Handle the LAN socket
    LanServer(&socketAdmin, SERVER_PORT);

    // We never get here -> no cleanup

    return 0;
}
