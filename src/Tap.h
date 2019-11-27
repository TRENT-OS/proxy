/*
 * Tap.h
 *
 *  Created on: Mar 25, 2019
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

#define  ENABLE_TAP_FILTER 1  /* Enable or disable the filter */

/*
Two problems seen when tap allows all the traffic to flow are
    1.) Chanmux could start to overflow after sometime
    2.) The read from an application (e.g. a webpage) is slow or time
        consuming

To solve this,either we can ignore the problem in the application as drop
-ping of network packets is quite common

OR

Filter could be applied to filter out unncessary traffic for tap0 and tap1
interfaces based on the current usage scenarios which include using TCP/IP
traffic and ARP for incoming connection requests.

*/

#if (ENABLE_TAP_FILTER == 1)

typedef uint8_t eth_mac_addr_t[6];
typedef uint8_t ipv4_addr_t[4];

// Ethernet header
typedef struct {
    eth_mac_addr_t  dest_addr;   // Destination hardware address
    eth_mac_addr_t  src_addr;    // Source hardware address
    uint16_t        frame_type;  // Ethernet frame type */
} __attribute__((packed)) ethernet_header_t;

#define ETHERNET_FRAME_TYPE_ARP 0x0806

// Ethernet ARP packet from RFC 826
typedef struct {
    uint16_t        hw_type;          // 1 for Ethernet
    uint16_t        protocol_type;    // 0x8000 for IPv4
    uint8_t         hw_addr_len;      // 6 for Ethernet MACs
    uint8_t         prot_addr_len;    // 4 for IPv4 IP addresses
    uint16_t        operation;        // 1=request, 2=reply
    eth_mac_addr_t  sender_mac;
    ipv4_addr_t     sender_ip;
    eth_mac_addr_t  target_mac;
    ipv4_addr_t     target_ip;
} __attribute__((packed)) arp_ethernet_ipv4_t;

#define ARP_HW_TYPE_ETHERNET    1
#define ARP_PROTOCOL_TYPE_IPv4  0x8000
#define ARP_ETH_HW_ADDR_LEN     6
#define ARP_PROT_IPv4_ADDR_LEN  4

typedef struct {
    ethernet_header_t       eth_header;
    arp_ethernet_ipv4_t     arp_ipv4;
} packet_ethernet_arp_ipv4_t;

const ipv4_addr_t TAP1_IP_ADDR = {192,168,82,92};

#endif


#define TUN_MTU 1024


class Tap : public InputDevice, public OutputDevice
{
public:
    Tap(int fd)
    {
        tapfd = fd;
    }


#if (ENABLE_TAP_FILTER == 1)

    int Read(vector<char> &buf)
    {
        int is_tap0 = (0 == strcmp(devname,"tap0"));
        int is_tap1 = (0 == strcmp(devname,"tap1"));

        struct pollfd pfd;
        pfd.fd = tapfd;
        pfd.events = POLLIN;

        if (poll(&pfd, 1, 0) <= 0)
        {
            return -1;
        }

        // read a packet. We assume this will always read exactly one complete
        // Ethernet packet.
        int ret = (int)read(tapfd, &buf[0], buf.size());
        if(ret <= 0)
        {
            // error reading packet
            return -1;
        }
        size_t len = ret;

        ethernet_header_t* eth = (ethernet_header_t*)&buf[0];
        if (len < sizeof(*eth))
        {
           return -1;
        }

        static_assert( sizeof(mac_tap) == sizeof(eth->dest_addr) );
        int is_mac_ok  = (0 == memcmp(eth->dest_addr, &mac_tap[0], sizeof(mac_tap)));

        // all packets can pass where the destination MAC matches
        if (is_mac_ok)
        {
            return len;
        }

        // If we are here, the destination MAC did not match. Drop packet if we are
        // not tap1
        if ( !is_tap1)
        {
            if(devname[0] != '\0')
            {
                assert( is_tap0 ); // fail safe, we must be tap0
                return -1;
            }
        }

        // ToDo: do we support arbitrary MAC addresses or only those requests for
        // the broadcast address ff-ff-ff-ff-ff-ff? And why don't we simply allow
        // all broadcast packets to pass here, not just ARP?

        // check if this is an Ethernet ARP packet

        if (ETHERNET_FRAME_TYPE_ARP != ntohs(eth->frame_type)) // Network byte order is Big endian.
        {
            return -1;
        }

        // assume we have an Ethernet ARP IPv4 packet.
        packet_ethernet_arp_ipv4_t* packet_ethernet_arp_ipv4 = (packet_ethernet_arp_ipv4_t*)&buf[0];

        // check size. Note ARP packets can be less than the minimum size of an
        // Ethernet frame, thus there is additional padding after the ARP data.
        // Unfortunately, there is no length byte in the Ethernet header, so there
        // is no simple way to know how much padding there is. However, since we
        // assume that we always read exactly one Ethernet frame, we can consider
        // all data after the ARP data as padding and simply ignore this.
        if (len < sizeof(*packet_ethernet_arp_ipv4))
        {
           return -1;
        }

        // size is ok, check is payload is a valid ARP IPv4 packet.
        arp_ethernet_ipv4_t *arp = &(packet_ethernet_arp_ipv4->arp_ipv4);
        if ( (ARP_HW_TYPE_ETHERNET != arp->hw_type)
             && (ARP_PROTOCOL_TYPE_IPv4 != arp->protocol_type)
             && (ARP_ETH_HW_ADDR_LEN != arp->hw_addr_len)
             && (ARP_PROT_IPv4_ADDR_LEN != arp->prot_addr_len))

        {
            return -1;
        }

        // we have an ARP IPv4 packet, check that target IP address matches
        static_assert( sizeof(arp->target_ip) == sizeof(TAP1_IP_ADDR) );
        int is_ip_ok = (0 == memcmp( &(arp->target_ip), TAP1_IP_ADDR, sizeof(TAP1_IP_ADDR)));

        return is_ip_ok ? len : -1;

    } // end of read()

#else
    int Read(vector<char> &buf)
    {
          struct pollfd pfd;
             int len;
             pfd.fd = tapfd;
             pfd.events = POLLIN;
            if (poll(&pfd, 1, 0) <= 0)
            {
                return -1;
            }

            len = (int)read(tapfd, &buf[0], buf.size());
            if (len > 0)
            {
                return len;
            }
            else
            {
                return -1;
            }

    }
#endif


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
      char devname[10] = {0};
      void error(const char *msg) const
    {
          fprintf(stderr, "%s\n", msg);
    }

};

