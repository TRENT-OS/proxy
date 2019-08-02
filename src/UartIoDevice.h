
#pragma once

#include <mutex>

using namespace std;

typedef enum
{
    DEVICE_TYPE_PSEUDO_CONSOLE,
    DEVICE_TYPE_SOCKET,
    DEVICE_TYPE_RAW_SERIAL,
    DEVICE_TYPE_UNKOWN
} DeviceType;


class UartIoDevice
{
    public:
    UartIoDevice(string *pseudoDevice, DeviceType *type) : pseudoDevice(pseudoDevice), type(type){}

    void Lock() const { lock.lock(); }
    void UnLock() const { lock.unlock(); }
    string *GetResource() { return pseudoDevice; }
    DeviceType *GetType() { return type; }

    private:
    string *pseudoDevice;
    DeviceType *type;
    mutable std::mutex lock;
};

