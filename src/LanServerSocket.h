
#pragma once

#include "IoDevices.h"
#include "Socket.h"

#include "LibDebug/Debug.h"

using namespace std;

class LanServerSocket : public IoDevice
{
    private:
    int fd;
    OutputDevice *outputDevice;
    InputDevice *inputDevice;

    public:
    LanServerSocket(int fd) :
        fd(fd),
        outputDevice(nullptr),
        inputDevice(nullptr)
    {}

    ~LanServerSocket()
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
