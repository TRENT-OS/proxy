#pragma once

#include "IoDevices.h"
#include "ChanMuxTestChannel.h"

#include "LibDebug/Debug.h"

using namespace std;

class ChanMuxTestChannelCreator : public IoDeviceCreator
{
    private:

    public:
    ChanMuxTestChannelCreator()
    {
    }

    ~ChanMuxTestChannelCreator()
    {
    }

    IoDevice *Create()
    {
        return new ChanMuxTestChannel();
    }
};
