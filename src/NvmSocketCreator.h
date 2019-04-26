#pragma once

#include "IoDevices.h"
#include "NvmSocket.h"

#include "LibDebug/Debug.h"


using namespace std;

class NvmSocketCreator : public IoDeviceCreator
{
    private:

    public:
    NvmSocketCreator()
    {
    }

    ~NvmSocketCreator()
    {
    }

    IoDevice *Create()
    {
        return new NvmSocket();
    }
};
