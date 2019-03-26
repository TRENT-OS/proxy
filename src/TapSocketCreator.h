
#pragma once

#include "IoDevices.h"
#include "TapSocket.h"

#include "LibDebug/Debug.h"


using namespace std;

class TapSocketCreator : public IoDeviceCreator
{
    private:
    int tapfd;

    public:
    TapSocketCreator(const char *name)
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
	   printf("[yk]%s\n", __FUNCTION__);


    }

    ~TapSocketCreator()
    {
    }

    IoDevice *Create()
    {
        return new TapSocket(tapfd);
    }

};
