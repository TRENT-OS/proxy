
#include "Socket.h"
#include "ServerSocket.h"
#include "IoDevices.h"
#include "utils.h"

#include <thread>

using namespace std;

#define SERVER_PORT 8883

void MqttServerThread(int socketFd)
{
    size_t bufSize = 4096;
    vector<char> buffer(bufSize);
    int readBytes, writtenBytes;
    unsigned int maxRxOperations = 5;

    try
    {
        struct timeval tv;
        tv.tv_sec = 120;
        tv.tv_usec = 0;
        setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

        while (true)
        {
            readBytes = read(socketFd, &buffer[0], buffer.size());
            if ((readBytes > 0) && (--maxRxOperations > 0))
            {
                printf("MqttServerThread: bytes received from socket: %d.\n", readBytes);
                fflush(stdout);
            }
            else
            {
                printf("MqttServerThread: closing client connection thread (file descriptor: %d).\n", socketFd);
                break;
            }
        }
    }
    catch (...)
    {
        printf("MqttServerThread exception\n");
    }

    close(socketFd);
}

void MqttServer()
{
    vector<thread> allThreads;

    ServerSocket serverSocket(SERVER_PORT);
    socklen_t clientLength;
    struct sockaddr_in clientAddress;

    serverSocket.Listen(5);

    try
    {
        while (true)
        {
            clientLength = sizeof(clientAddress);
            int newsockfd = serverSocket.Accept((struct sockaddr *) &clientAddress, &clientLength);
            printf("start server thread: in port %d, in address %x (file descriptor: %x)\n", clientAddress.sin_port, clientAddress.sin_addr.s_addr, newsockfd);

            // thread is started: it is waiting for MQTT publish messages.
            allThreads.push_back(thread{MqttServerThread, newsockfd});
        }
    }
    catch (...)
    {
        printf("MqttServer exception\n");
    }
}

int main(int argc, const char *argv[])
{
    MqttServer();

    // We never get here -> no cleanup
    
    return 0;
}
