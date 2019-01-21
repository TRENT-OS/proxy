
#pragma once

#include "IoDevices.h"
#include "CloudSocket.h"

#include "LibDebug/Debug.h"

#if 0
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include <vector>
#endif

using namespace std;

class CloudSocketCreator : public IoDeviceCreator
{
    private:
    int port;
    string hostName;

    public:
    CloudSocketCreator(int port, string hostName) :
        port(port),
        hostName(hostName)
    {}

    ~CloudSocketCreator()
    {
    }

    IoDevice *Create()
    {
        return new CloudSocket(port, hostName);
    }
};
