
#pragma once

#include "IoDevices.h"
#include "Channel.h"

#include "LibDebug/Debug.h"

using namespace std;

class LanServerChannel : public IoDevice
{
    private:
    int fd;
    OutputDevice *outputDevice;
    InputDevice *inputDevice;

    public:
    LanServerChannel(int fd) :
        fd(fd),
        outputDevice(nullptr),
        inputDevice(nullptr)
    {}

    ~LanServerChannel()
    {
        if (outputDevice != nullptr)
        {
            delete outputDevice;
        }

        if (inputDevice != nullptr)
        {
            delete inputDevice;
        }
    }

    int Create()
    {
        outputDevice = new DeviceWriter{fd};
        inputDevice = new DeviceReader{fd};

        return 1;
    }

    OutputDevice *GetOutputDevice()
    {
        return outputDevice;
    }

    InputDevice *GetInputDevice()
    {
        return inputDevice;
    }
};
