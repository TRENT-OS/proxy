
#pragma once

#include "IoDevices.h"
#include "PicoCloudSocket.h"

#include "LibDebug/Debug.h"


using namespace std;

class PicoCloudSocketCreator : public IoDeviceCreator
{
    private:
    int port;
    string hostName;

    public:
    PicoCloudSocketCreator(int port, string hostName) :
        port(port),
        hostName(hostName)
    {}

    ~PicoCloudSocketCreator()
    {
    }

    IoDevice *Create()
    {
        return new PicoCloudSocket(port, hostName);
    }
};
