#pragma once

#include "IoDevices.h"
#include "ChanMuxTestSocket.h"

#include "LibDebug/Debug.h"

using namespace std;

class ChanMuxTestSocketCreator : public IoDeviceCreator
{
    private:

    public:
    ChanMuxTestSocketCreator()
    {
    }

    ~ChanMuxTestSocketCreator()
    {
    }

    IoDevice *Create()
    {
        return new ChanMuxTestSocket();
    }
};
