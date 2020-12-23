/*
 * TapSocket.h
 *
 *  Created on: Mar 25, 2019
 *      Author: yk
 */




#pragma once

#include "IoDevices.h"
#include "Tap.h"

#include "lib_debug/Debug.h"

using namespace std;

class TapChannel : public IoDevice
{
    private:
    int tapfd;
    Tap *channel;

    public:
    TapChannel(int fd) :
        channel(nullptr)
    {
    	tapfd = fd;
    }

    ~TapChannel()
    {
        if (channel != nullptr)
        {
            delete channel;
        }
    }

    int Create()
    {
        channel = new Tap(tapfd);
        if (channel == nullptr)
        {
            Debug_LOG_ERROR("TapChannel Create(): channel is NULL");
        }
        return 0;
    }

    OutputDevice *GetOutputDevice()
    {
        return channel;
    }

    InputDevice *GetInputDevice()
    {
        return channel;
    }
};
