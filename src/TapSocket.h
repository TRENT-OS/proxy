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

class TapSocket : public IoDevice
{
    private:
    int tapfd;
    Tap *socket;

    public:
    TapSocket(int fd) :
        socket(nullptr)
    {
    	tapfd = fd;
    }

    ~TapSocket()
    {
        if (socket != nullptr)
        {
            delete socket;
        }
    }

    int Create()
    {
    	Debug_LOG_INFO("TapSocket: create TAP Dev socket start: %s:%d\n",__FUNCTION__,tapfd);
        socket = new Tap(tapfd);
        Debug_LOG_INFO("TapSocket: create TAP Dev socket end: %s:%p\n",__FUNCTION__,socket);

        return 0;
    }

    OutputDevice *GetOutputDevice()
    {
        return socket;
    }

    InputDevice *GetInputDevice()
    {
        return socket;
    }
};
