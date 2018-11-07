
#include "type.h"
#include "uart_io_host.h"
#include "uart_hdlc.h"

#include "GuestConnector.h"
#include "Socket.h"
#include "ServerSocket.h"
#include "IoDevices.h"

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <iostream>

using namespace std;

mutex accessToPseudoDevice;

void GuestConnectorToGuest(string pseudoDevice, InputDevice *socket)
{
    size_t bufSize = 256;
    vector<unsigned char> buffer(bufSize);
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
            accessToPseudoDevice.lock();
            writtenBytes = guestConnector.Write(readBytes, &buffer[0]);
            writtenBytes = 0;
            accessToPseudoDevice.unlock();

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

void GuestConnectorFromGuest(string pseudoDevice, OutputDevice *socket)
{
    size_t bufSize = 1024;
    vector<unsigned char> buffer(bufSize);
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
        buffer.resize(bufSize);
        accessToPseudoDevice.lock();
        int readBytes = guestConnector.Read(buffer.size(), &buffer[0]);
        accessToPseudoDevice.unlock();
        if (readBytes > 0)
        {
            //dumpFrame(&buffer[0], readBytes);
            buffer.resize(readBytes);
            writtenBytes = socket->Write(buffer);
            if (writtenBytes < 0)
            {
                printf("GuestConnectorFromGuest: socket write failed; %s.\n", strerror(errno));
            }
            else
            {
                printf("GuestConnectorFromGuest: bytes written to socket: %d.\n", writtenBytes);
            }
        }
        else
        {
            // Not used because: not meaningful "Resource temporarily unavailable" 
            //printf("GuestConnectorFromGuest: guest read failed.\n");
        }
    }
}

class OutputLogger : public OutputDevice
{
    public:
    OutputLogger() {}

    int Write(vector<unsigned char> buf) 
    { 
        cout << string((char *)(&buf[0]), buf.size());
    }
};

class DeviceReader : public InputDevice
{
    public:
    DeviceReader(int fd) : fd(fd) {}

    int Read(std::vector<unsigned char> &buf)
    {
        return read(fd, &buf[0], buf.size());
    }

    private:
    int fd;
};

void ServerThread(DeviceReader reader, OutputLogger outputLogger)
{
    size_t bufSize = 1024;
    vector<unsigned char> buffer(bufSize);
    int readBytes, writtenBytes;

    while (true)
    {
        buffer.resize(bufSize);
        readBytes = reader.Read(buffer);
        if (readBytes > 0)
        {
            //dumpFrame(&buffer[0], readBytes);
            //printf("server thread received data: %d.\n", readBytes);
            writtenBytes = outputLogger.Write(buffer);
        }
        else
        {
            // Not used because: not meaningful "Resource temporarily unavailable" 
            //printf("ServerThread: guest read failed.\n");
        }
    }
}

void LanServer()
{
    ServerSocket serverSocket(7999);
    socklen_t clientLength;
    struct sockaddr_in clientAddress;

    serverSocket.Listen(5);

    vector<thread> threads;

    while (true)
    {
        clientLength = sizeof(clientAddress);
        int newsockfd = serverSocket.Accept((struct sockaddr *) &clientAddress, &clientLength);
        printf("start server thread: in port %d, in address %x\n", clientAddress.sin_port, clientAddress.sin_addr.s_addr);
        threads.push_back(thread{ServerThread, DeviceReader(newsockfd), OutputLogger()});
    }
}


#define SERVER_PORT 8883
#define SERVER_NAME "HAR-test-HUB.azure-devices.net"

int main(int argc, const char *argv[])
{
    LanServer();
    return 0;


    int port = SERVER_PORT;
    string hostName {SERVER_NAME};
    string pseudoDevice {argv[1]};

    Socket socket {port, hostName};

    thread t1 {GuestConnectorToGuest, pseudoDevice, &socket};
    thread t2 {GuestConnectorFromGuest, pseudoDevice, &socket};

    t1.join();
    t2.join();

#if 0
#endif    

    return 0;
}
