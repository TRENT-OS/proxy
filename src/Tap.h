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
    std::vector<char> HandlePayload(vector<char> buffer)
    {
        UartSocketGuestSocketCommand command = static_cast<UartSocketGuestSocketCommand>(buffer[0]);
        unsigned int commandLogicalChannel = buffer[1];
        //int result;
        vector<char> result(7,0);
        Debug_LOG_DEBUG("%s:%d", __func__, __LINE__);

        if (command == UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_GETMAC)
        {
            Debug_LOG_DEBUG("%s:%d", __func__, __LINE__);
            printf("Get mac  command rx:\n");
             // handle get mac here.
            vector<char> mac(6,0);
            if(commandLogicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW)
            {
                getMac("tap0",&mac[0]);
            }
            else if(commandLogicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW_2)
            {
                getMac("tap1",&mac[0]);
            }

            printf("Mac read = %x %x %x %x %x %x\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
            result[0] = 0;
            memcpy(&result[1],&mac[0],6);
        }
        return result;
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

