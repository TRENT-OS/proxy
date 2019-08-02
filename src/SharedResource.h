
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


template <typename T>
class SharedResource
{
    public:
    SharedResource(T *value, DeviceType *type) : value(value), type(type){}

    void Lock() const { lock.lock(); }
    void UnLock() const { lock.unlock(); }
    T *GetResource() { return value; }
    DeviceType *GetType() { return type; }

    private:
    T *value;
    DeviceType *type;
    mutable std::mutex lock;
};


/*
typedef struct
{
    string pseudoDevice; 
    DeviceType deviceType;
} UartIoDevice;

 */
