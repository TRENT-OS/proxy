
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
        struct hostent *server;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) 
            error("ERROR opening socket");

        // Handle the case that the given port may be in state TIME_WAIT because of some very recent activity involving this port.
        // http://www.softlab.ntua.gr/facilities/documentation/unix/unix-socket-faq/unix-socket-faq-2.html#time_wait
        // see: https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr
        if (useRebindProtection)
        {
            int enable = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
                error("ERROR using setsockopt");
        }

        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);
        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        {
            error("ERROR on binding");
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

    private:
    int sockfd;

    void error(const char *msg)
    {
        fprintf(stderr, "%s\n", msg);
    }
};
