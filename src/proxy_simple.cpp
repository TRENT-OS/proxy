
#include "GuestConnector.h"
#include "Socket.h"
#include "ServerSocket.h"
#include "IoDevices.h"
#include "GuestListeners.h"
#include "utils.h"

#include <thread>

using namespace std;

enum LogicalChannels
{
    LOGICAL_CHANNEL_LAN = 0,
    LOGICAL_CHANNEL_WAN = 1,
    LOGICAL_CHANNEL_MAX
};

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
            //printf("GuestConnectorToGuest: socket read failed.\n");
        }
    }
}

void GuestConnectorFromGuest(SharedResource<string> *pseudoDevice, GuestListeners *guestListeners)
{
    size_t bufSize = 1024;
    vector<char> buffer(bufSize);
    int readBytes, writtenBytes;
    GuestConnector guestConnector(pseudoDevice, GuestConnector::GuestDirection::FROM_GUEST);

    if (!guestConnector.IsOpen())
    {
        printf("GuestConnectorFromGuest: pseudo device not open.\n");
        return;
    }

    printf("GuestConnectorFromGuest: s00.\n");

    while (true)
    {
        unsigned int logicalChannel;
        buffer.resize(bufSize);
        int readBytes = guestConnector.Read(buffer.size(), &buffer[0], &logicalChannel);
        if (readBytes > 0)
        {
            //dumpFrame(&buffer[0], readBytes);
            buffer.resize(readBytes);
            OutputDevice *outputDevice = guestListeners->GetListener(logicalChannel);
            if (outputDevice != nullptr)
            {
                writtenBytes = outputDevice->Write(buffer);
                if (writtenBytes < 0)
                {
                    printf("GuestConnectorFromGuest: socket write failed; %s.\n", strerror(errno));
                }
                else
                {
                    printf("GuestConnectorFromGuest: bytes written to socket: %d.\n", writtenBytes);
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

void LanServer(SharedResource<string> *pseudoDevice, vector<thread> &allThreads, GuestListeners &guestListeners)
{
    ServerSocket serverSocket(7999);
    socklen_t clientLength;
    struct sockaddr_in clientAddress;
    unsigned int logicalChannel = LOGICAL_CHANNEL_LAN;

    serverSocket.Listen(5);

    while (true)
    {
        clientLength = sizeof(clientAddress);
        int newsockfd = serverSocket.Accept((struct sockaddr *) &clientAddress, &clientLength);
        printf("start server thread: in port %d, in address %x\n", clientAddress.sin_port, clientAddress.sin_addr.s_addr);

        // Register the new LAN socket as (the new) listening device for the LAN logical channel.
        guestListeners.SetListener(LOGICAL_CHANNEL_LAN, new DeviceWriter(newsockfd));

        // TODO: we have to handle the destruction of client sockets and their corresponding threads.

        // A new "LAN thread" is started: it is waiting for data from the LAN and forwards it to the LAN logical channel.
        allThreads.push_back(thread{GuestConnectorToGuest, pseudoDevice, PARAM(logicalChannel, logicalChannel), new DeviceReader(newsockfd)});
    }
}

#define SERVER_PORT 8883
#define SERVER_NAME "HAR-test-HUB.azure-devices.net"

int main(int argc, const char *argv[])
{
    int port = SERVER_PORT;
    string hostName {SERVER_NAME};

    GuestListeners guestListeners{LOGICAL_CHANNEL_MAX};
    vector<thread> allThreads;

    string pseudoDeviceName{argv[1]};
    SharedResource<string> pseudoDevice{pseudoDeviceName};

    Socket wanSocket {port, hostName};

    // Register the WAN socket as listening device for the WAN logical channel.
    guestListeners.SetListener(LOGICAL_CHANNEL_WAN, &wanSocket);

    // The "WAN thread": is waiting for data from the WAN and forwards it to the WAN logical channel.
    allThreads.push_back(thread{GuestConnectorToGuest, &pseudoDevice, PARAM(logicalChannel, LOGICAL_CHANNEL_WAN), &wanSocket});

    // The "GUEST thread" is receiving all hdlc frames and distributing them to the according guest listeners.
    allThreads.push_back(thread{GuestConnectorFromGuest, &pseudoDevice, &guestListeners});

    LanServer(&pseudoDevice, allThreads, guestListeners);

    // We never get here -> no cleanup
    
    return 0;
}
