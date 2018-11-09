
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
    ServerSocket(int port)
    {
        struct sockaddr_in serv_addr;
        struct hostent *server;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) 
            error("ERROR opening socket");
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
        fprintf(stderr,"msg\n");
    }
};
