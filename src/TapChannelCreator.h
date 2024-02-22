/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include "IoDevices.h"
#include "TapChannel.h"

#include "lib_debug/Debug.h"


using namespace std;

class TapChannelCreator : public IoDeviceCreator
{
    private:
    int tapfd;

    public:
    TapChannelCreator(const char *name)
    {
	    struct ifreq ifr;


	    if((tapfd = open("/dev/net/tun", O_RDWR)) < 0) {
	    	printf("ERROR opening tap dev");
	    	tapfd=-1;
	    }

	    memset(&ifr, 0, sizeof(ifr));
	    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	    strncpy(ifr.ifr_name, name, IFNAMSIZ);
	    if(ioctl(tapfd, TUNSETIFF, &ifr) < 0) {

	    	 printf("ERROR reading ioctl");
	    	 tapfd=-1;
	    }
	   printf("%s() tapfd = %d\n", __FUNCTION__,tapfd);


    }

    ~TapChannelCreator()
    {
    }

    IoDevice *Create()
    {
        printf("Create Tap socket...%s\n",__FUNCTION__);
    	return new TapChannel(tapfd);
    }

};
