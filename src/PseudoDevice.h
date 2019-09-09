
#pragma once

#include <mutex>
#include "device_type.h"

using namespace std;


class PseudoDevice
{
    public:
    PseudoDevice(string *pseudoDevice, DeviceType *type) : pseudoDevice(pseudoDevice), type(type){}

    string *GetResource() { return pseudoDevice; }
    DeviceType *GetType() { return type; }

    private:
    string *pseudoDevice;
    DeviceType *type;
};

