
#include "type.h"
#include "uart_io_host.h"
#include "uart_hdlc.h"
#include "GuestConnector.h"

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
