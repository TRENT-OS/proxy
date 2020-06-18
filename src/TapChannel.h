/*
 * TapSocket.h
 *
 *  Created on: Mar 25, 2019
 *      Author: yk
 */




#pragma once

#include "IoDevices.h"
#include "Tap.h"

#include "LibDebug/Debug.h"

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
    	Debug_LOG_INFO("TapSocket: create TAP Dev socket start: %s:%d",__FUNCTION__,tapfd);
        channel = new Tap(tapfd);
        Debug_LOG_INFO("TapSocket: create TAP Dev socket end: %s:%p",__FUNCTION__,channel);

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
