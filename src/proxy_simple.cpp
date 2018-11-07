
#include "type.h"
#include "uart_io_host.h"
#include "uart_hdlc.h"

#include "GuestConnector.h"
#include "Socket.h"
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

#define SERVER_PORT 8883
#define SERVER_NAME "HAR-test-HUB.azure-devices.net"

int main(int argc, const char *argv[])
{
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
