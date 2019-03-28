/*
 * Tap.h
 *
 *  Created on: Mar 25, 2019
 *      Author: yk
 */


#pragma once

#include "IoDevices.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include <vector>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <signal.h>
#include <linux/if_tun.h>
#include <sys/poll.h>
#include "uart_socket_guest_rpc_conventions.h"
#include "utils.h"


using namespace std;

#define TUN_MTU 1024

class Tap : public InputDevice, public OutputDevice
{
public:
	Tap(int fd)
      {
		tapfd = fd;
      }


      int Read(vector<char> &buf)
      {
    	  struct pollfd pfd;
    	     int len;
    	     pfd.fd = tapfd;
    	     pfd.events = POLLIN;
    	     do  {
    	           if (poll(&pfd, 1, 0) <= 0)
    	           {
    	        	   return -1;
    	           }
      	    	    printf("[yk]%s....2\n", __FUNCTION__);
    	           len = (int)read(tapfd, &buf[0], buf.size());
    	            if (len > 0)
    	             {
    	               return len;
    	             }
    	            else
    	            {
    	        	 return -1;
    	            }

    	       } while(0);



    	  //return read(tapfd, &buf[0], buf.size());
      }

      int Write(vector<char> buf)
      {
    		printf("[yk]%s\n", __FUNCTION__);
    	    return write(tapfd, &buf[0], buf.size());

      }

      int Close()
      {
    	  printf("Tap Close called %s\n",__FUNCTION__);
          return close(tapfd);
      }


      int GetFileDescriptor() const
      {
          return tapfd;
      }


      int getMac(const char* name,char *mac)
      {
    	    int sck;
    	    struct ifreq eth;
    	    int retval = -1;

    	    sck = socket(AF_INET, SOCK_DGRAM, 0);
    	    if(sck < 0) {
    	        return retval;
    	    }

    	    memset(&eth, 0, sizeof(struct ifreq));
    	    strcpy(eth.ifr_name, name);
    	    /* call the IOCTL */
    	    if (ioctl(sck, SIOCGIFHWADDR, &eth) < 0) {
    	        perror("ioctl(SIOCGIFHWADDR)");
    	        return -1;
    	    }

    	    memcpy (mac, &eth.ifr_hwaddr.sa_data, 6);
    	    close(sck);
    	    return 0;

      }

    ~Tap()
      {
          close(tapfd);
      }

private:
      int tapfd;
      void error(const char *msg) const
      {
          fprintf(stderr, "%s\n", msg);
      }

};

