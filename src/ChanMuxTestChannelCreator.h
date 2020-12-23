#pragma once

#include "IoDevices.h"
#include "ChanMuxTestChannel.h"

#include "lib_debug/Debug.h"

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
