/*
 * Tap.h
 *
 *  Created on: Mar 25, 2019
 *      Author: yogesh kulkarni
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
#include "LibDebug/Debug.h"

using namespace std;

#define TUN_MTU 1024
#define ARP_MIN_SIZE 42     /* min packet size of ARP */
#define ARP_ETHERTYPE_BYTE1 0x08 /* Ether type of ARP packet */
#define ARP_ETHERTYPE_BYTE2 0x06 /* Ether type of ARP packet */
#define ARP_OFFSET_ETHERTYPE 12 /* First 12 bytes are occupied by mac address */


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
             int is_mac_ok;
             int is_ip_ok;
             uint8_t ARP_IP_ADDR[4] = {192,168,82,92};  /* This is the IP '192.168.82.92' of tap1 interface*/
    	     pfd.fd = tapfd;
    	     pfd.events = POLLIN;
    	     do  {
                     if (poll(&pfd, 1, 0) <= 0)
                     {
                        return -1;
                     }
                     len = (int)read(tapfd, &buf[0], buf.size());

                     if(len <= 0)
                     {
                         return -1;
                     }

                   /* For tap0 no ARP is needed as it is configured for client,hence filter based on mac only
                      For tap1 ARP is needed as it is configured for server,hence filter based on mac or IP addr
                      ARP request in brief: The size of ARP request is 42 bytes (28 bytes of ARP msg and 14 bytes Ethernet header).
                      Each frame is padded to Ethernet minimum :60 bytes data.
                      In the use case when NW server and client app run on the same device ARP request length is 42 bytes.No padding seen.
                      In the use case when NW server and client app run on different devices, ARP request is 60 bytes.
                      In both these cases the IP addr of destination "TAP1" with 4 bytes would start from offset 38 */
                    if(len >= sizeof(mac_tap))
                     {
                        is_mac_ok  = (0 == memcmp(&buf[0], &mac_tap[0],sizeof(mac_tap)));  /* For Ethernet frames (with TCP traffic), first 6 bytes are always destination mac*/
                     }

                    if(len >= ARP_MIN_SIZE)
                     {
                        /* To confirm its an ARP packet, ethertype must be 0x0806*/
                          if(buf[ARP_OFFSET_ETHERTYPE] != ARP_ETHERTYPE_BYTE1)
                          {
                            is_ip_ok = false;
                          }
                          else
                          {
                            is_ip_ok = (0 == memcmp(&buf[ARP_MIN_SIZE-sizeof(ARP_IP_ADDR)],&ARP_IP_ADDR[0],sizeof(ARP_IP_ADDR)));  /* For ARP,last 4 bytes are always IP addr */
                          }

                     }
                      /* Block excess traffic for tap0. Send only data which starts with MAC of tap0 */

                     if(strcmp(devname,"tap0") == 0)
                     {
                        /* Compare the 6 bytes of mac, read only data meant for tap0 mac addr.
                          Filter out other data i.e. Return len only when is_mac_ok equals 0  */

                           if(is_mac_ok != true)
                           {
                              return -1;
                           }

                          return len;
                      }
                      else if(strcmp(devname,"tap1") == 0)
                      {
                            /* For tap1, compare 6 bytes of mac or IP addr. Both must be allowed to pass.
                           i.e. Return len only when either one of them is_mac_ok OR is_ip_ok equals true*/

                           if((is_mac_ok != true) && (is_ip_ok != true))
                           {
                              return -1;
                           }

                           return len;
                      }


                } while(0);

     } // end of read

      int Write(vector<char> buf)
      {
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
                memcpy(&mac_tap[0],&mac[0],6);
                strncpy(devname, "tap0",5);
                mac_tap[5]++;
            }
            else if(commandLogicalChannel == UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW_2)
            {
                getMac("tap1",&mac[0]);
                memcpy(&mac_tap[0],&mac[0],6);
                strncpy(devname, "tap1",5);
                mac_tap[5]++;
            }
            else
            {
                printf("%s() Unsupported Logical channel\n",__FUNCTION__);
                result[0] = -1;
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
      uint8_t mac_tap[6];  /* Save tap mac addr and name for later use for filtering data */
      char devname[10];
      void error(const char *msg) const
      {
          fprintf(stderr, "%s\n", msg);
      }

};

