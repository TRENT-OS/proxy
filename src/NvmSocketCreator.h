#pragma once

#include "IoDevices.h"
#include "NvmSocket.h"

#include "LibDebug/Debug.h"


using namespace std;

class NvmSocketCreator : public IoDeviceCreator
{
    private:
        unsigned m_chanNum;
    public:
    NvmSocketCreator(unsigned chanNum) : m_chanNum(chanNum)
    {
    }

    ~NvmSocketCreator()
    {
    }

    IoDevice *Create()
    {
        return new NvmSocket(m_chanNum);
    }
};
