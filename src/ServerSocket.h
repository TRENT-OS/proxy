
#pragma once

#include "IoDevices.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

using namespace std;

class ServerSocket
{
public:
    ServerSocket(int port, bool useRebindProtection = true)
    {
        struct sockaddr_in serv_addr;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            fprintf(stderr, "ERROR opening socket %d ", sockfd);
            return;
        }

        // Handle the case that the given port may be in state TIME_WAIT because of some very recent activity involving this port.
        // http://www.softlab.ntua.gr/facilities/documentation/unix/unix-socket-faq/unix-socket-faq-2.html#time_wait
        // see: https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr
        if (useRebindProtection)
        {
            int enable = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            {
                fprintf(stderr, "ERROR using setsockopt");
                close(sockfd);
                sockfd = -1;
                return;
            }
        }

        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);
        if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            error("ERROR on binding");
            close(sockfd);
            sockfd = -1;
            return;
        }
    }

    ~ServerSocket()
    {
        close(sockfd);
    }

    int Listen(int backlog)
    {
        return listen(sockfd, backlog);
    }

    int Accept(struct sockaddr *addr, socklen_t *addrlen)
    {
        int fd = accept(sockfd, addr, addrlen);

        if (fd < 0)
        {
            error("ERROR on accept");
        }

        return fd;
    }

    bool IsOpen() const { return sockfd >= 0; }

private:
    int sockfd;

    void error(const char *msg) const
    {
        fprintf(stderr, "%s\n", msg);
    }
};
