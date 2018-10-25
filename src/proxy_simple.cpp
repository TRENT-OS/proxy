
#include "type.h"
#include "uart_io_host.h"
#include "uart_hdlc.h"

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

class GuestConnector
{
    public:
    enum GuestDirection
    {
        FROM_GUEST,
        TO_GUEST
    };

    GuestConnector(string pseudoDevice, GuestDirection guestDirection)
        : pseudoDevice(pseudoDevice)
    {
        if (guestDirection == FROM_GUEST)
        {
            UartIoHostInit(
                &uartIoHost, 
                pseudoDevice.c_str(), 
                PARAM(isBlocking, false),
                PARAM(readOnly, true),
                PARAM(writeOnly, false));
        }
        else
        {
            UartIoHostInit(
                &uartIoHost, 
                pseudoDevice.c_str(), 
                PARAM(isBlocking, true),
                PARAM(readOnly, false),
                PARAM(writeOnly, true));
        }


        UartHdlcInit(&uartHdlc, &uartIoHost.implementation);

        isOpen = Open() >= 0;
    }

    ~GuestConnector() 
    { 
        Close();
        UartHdlcDeInit(&uartHdlc); 
    }

    int Open() { return UartHdlcOpen(&uartHdlc); }
    int Close() { return UartHdlcClose(&uartHdlc); }
    int Read(unsigned int length, unsigned char *buf) { return UartHdlcRead(&uartHdlc, length, buf); } 
    int Write(unsigned int length, unsigned char *buf) { return UartHdlcWrite(&uartHdlc, length, buf); }

    bool IsOpen() const { return isOpen; }

    private:
    string pseudoDevice;
    UartIoHost uartIoHost;
    UartHdlc uartHdlc;
    bool isOpen;
};

class Socket
{
    public:
    Socket(int port, string hostName)
    {
        struct sockaddr_in serv_addr;
        struct hostent *server;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            error("ERROR opening socket");
        }
        else
        {
            server = gethostbyname(hostName.c_str());
            if (server == NULL) 
            {
                error("ERROR, no such host");
            }
            else
            {
                bzero((char *) &serv_addr, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                bcopy((char *)server->h_addr, 
                    (char *)&serv_addr.sin_addr.s_addr,
                    server->h_length);

                serv_addr.sin_port = htons(port);
                
                if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
                {
                    error("ERROR connecting");
                } 
            }
        }
    }

    ~Socket()
    {
        close(sockfd);
    }

    int Read(vector<unsigned char> &buf)
    {
        return read(sockfd, &buf[0], buf.size());
    }

    int Write(vector<unsigned char> buf)
    {
        return write(sockfd, &buf[0], buf.size());
    }

    private:
    int sockfd;

    void error(const char *msg)
    {
        fprintf(stderr,"msg\n");
    }
};

void GuestConnectorToGuest(string pseudoDevice, Socket *socket)
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

void GuestConnectorFromGuest(string pseudoDevice, Socket *socket)
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
