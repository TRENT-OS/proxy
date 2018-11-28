
#include "GuestConnector.h"
#include "Socket.h"
#include "ServerSocket.h"
#include "IoDevices.h"
#include "GuestListeners.h"
#include "MqttCloud.h"
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

    try
    {
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
                printf("GuestConnectorToGuest: closing client connection thread (file descriptor: %d).\n", socket->GetFileDescriptor());
                if (logicalChannel == LOGICAL_CHANNEL_WAN)
                {
                    printf("GuestConnectorToGuest: the WAN socket was closed !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                    socket->Close();
                }

                break;
            }
        }
    }
    catch (...)
    {
        printf("GuestConnectorToGuest exception\n");
    }

    socket->Close();
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

    try
    {
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
                        printf("GuestConnectorFromGuest: bytes written to socket: %d. From logical channel: %d\n", writtenBytes, logicalChannel);
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
        printf("GuestConnectorFromGuest exception\n");
    }
}

void LanServer(SharedResource<string> *pseudoDevice, vector<thread> &allThreads, GuestListeners &guestListeners, unsigned int port)
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
            printf("start server thread: in port %d, in address %x (file descriptor: %x)\n", clientAddress.sin_port, clientAddress.sin_addr.s_addr, newsockfd);

            // Register the new LAN socket as (the new) listening device for the LAN logical channel.
            guestListeners.SetListener(LOGICAL_CHANNEL_LAN, new DeviceWriter(newsockfd));

            // A new "LAN thread" is started: it is waiting for data from the LAN and forwards it to the LAN logical channel.
            allThreads.push_back(thread{GuestConnectorToGuest, pseudoDevice, PARAM(logicalChannel, logicalChannel), new DeviceReader(newsockfd)});
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

    if (argc > 2)
    {
        hostName = string{argv[2]};
    }

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

    LanServer(&pseudoDevice, allThreads, guestListeners, SERVER_PORT);

    // We never get here -> no cleanup
    
    return 0;
}
