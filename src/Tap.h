/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
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
#include "LibDebug/Debug.h"
#include "network/OS_Ethernet.h"

using namespace std;

#define ENABLE_TAP_FILTER 0 /* Enable or disable the filter */

#define FRAME_LENGTH_IN_BYTES 2 /* First 2 bytes contain frame length*/

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
typedef struct
{
    eth_mac_addr_t dest_addr; // Destination hardware address
    eth_mac_addr_t src_addr;  // Source hardware address
    uint16_t frame_type;      // Ethernet frame type */
} __attribute__((packed)) ethernet_header_t;

#define ETHERNET_FRAME_TYPE_ARP 0x0806

// Ethernet ARP packet from RFC 826
typedef struct
{
    uint16_t hw_type;       // 1 for Ethernet
    uint16_t protocol_type; // 0x8000 for IPv4
    uint8_t hw_addr_len;    // 6 for Ethernet MACs
    uint8_t prot_addr_len;  // 4 for IPv4 IP addresses
    uint16_t operation;     // 1=request, 2=reply
    eth_mac_addr_t sender_mac;
    ipv4_addr_t sender_ip;
    eth_mac_addr_t target_mac;
    ipv4_addr_t target_ip;
} __attribute__((packed)) arp_ethernet_ipv4_t;

#define ARP_HW_TYPE_ETHERNET 1
#define ARP_PROTOCOL_TYPE_IPv4 0x8000
#define ARP_ETH_HW_ADDR_LEN 6
#define ARP_PROT_IPv4_ADDR_LEN 4

typedef struct
{
    ethernet_header_t eth_header;
    arp_ethernet_ipv4_t arp_ipv4;
} packet_ethernet_arp_ipv4_t;

// this IP address is assigned by an OS to a network interface. We use
// a the well know default from the OS Network Test here, it must be adopted
// if another system uses something else. And actually, the NIC driver in
// the system should be able to enable this filter on demand with a
// command, so we don't have anything to hard-coded here at all. The Proxy
// should be agnostic of how a system uses the network interface.
const ipv4_addr_t TAP1_IP_ADDR = {192, 168, 82, 92};

#endif

class Tap : public InputDevice, public OutputDevice
{
public:
    Tap(int fd)
    {
        tapfd = fd;
    }

    int Read(vector<char> &buf)
    {
        int ret;
        struct pollfd pfd = {.fd = tapfd, .events = POLLIN};
        // By setting timeout to -1, we will only be woken up when there is
        // data to be read
        ret = poll(&pfd, 1, -1);
        // Poll should never return 0 when timeout is -1. In case it happens,
        // treat it as error.
        if (ret <= 0)
        {
            Debug_LOG_ERROR("[%s] poll() failed, error %d", devname, ret);
            return -1;
        }

        // read a packet. We assume this will always read exactly one complete
        // Ethernet packet.
        ret = read(tapfd, &buf[FRAME_LENGTH_IN_BYTES], buf.size());
        // we consider 0 as an error, since poll() above must have returned
        // with a non-zero value if we arrive here. So there must be data.
        if (ret <= 0)
        {
            Debug_LOG_ERROR("[%s] read() failed, error %d", devname, ret);
            return -1;
        }

        // We read from the TAP device even when not forwarding to QEMU,
        // to prevent stale data in the FIFO. If we haven't reveiced a START yet
        // we ignore the packet we read into the buffer and return -1.
        if (!read_start)
        {
            if (!printed_to_log)
            {
                Debug_LOG_INFO("Not reading from TAP device %s until START is received", devname);
                printed_to_log = true;
            }
            return -1; // Dont read anything until read_start is true
        }

        size_t frame_len = ret;

        // first two byte are the frame length, we follow the network byte
        // order and use big endian
        buf[0] = (frame_len >> 8) & 0xFF;
        buf[1] = frame_len & 0xFF;

#if (ENABLE_TAP_FILTER == 1)

        ethernet_header_t *eth = (ethernet_header_t *)&buf[FRAME_LENGTH_IN_BYTES];
        if (frame_len < sizeof(*eth))
        {
            return -1;
        }

        static_assert(sizeof(mac_tap) == sizeof(eth->dest_addr));
        int is_mac_ok = (0 == memcmp(eth->dest_addr, &mac_tap[0], sizeof(mac_tap)));

        // all packets can pass where the destination MAC matches
        if (is_mac_ok)
        {
            return FRAME_LENGTH_IN_BYTES + frame_len;
        }

        // If we are here, the destination MAC did not match. Drop packet if we
        // are not tap1
        int is_tap0 = (0 == strcmp(devname, "tap0"));
        int is_tap1 = (0 == strcmp(devname, "tap1"));
        if (!is_tap1)
        {
            if (devname[0] != '\0')
            {
                assert(is_tap0); // fail safe, we must be tap0
                return -1;
            }
        }

        // ToDo: do we support arbitrary MAC addresses or only those requests
        //       for the broadcast address ff-ff-ff-ff-ff-ff? And why don't we
        //       simply allow all broadcast packets to pass here, not just ARP?

        // check if this is an Ethernet ARP packet. Note that network byte
        // order is big endian, so we need ntohs()  for all integers
        if (ntohs(ETHERNET_FRAME_TYPE_ARP) != eth->frame_type)
        {
            return -1;
        }

        // assume we have an Ethernet ARP IPv4 packet.
        packet_ethernet_arp_ipv4_t *packet_ethernet_arp_ipv4 = (packet_ethernet_arp_ipv4_t *)&buf[FRAME_LENGTH_IN_BYTES];

        // check size. Note ARP packets can be less than the minimum size of an
        // Ethernet frame, thus there is additional padding after the ARP data.
        // Unfortunately, there is no length byte in the Ethernet header, so
        // there is no simple way to know how much padding there is. However,
        // since we assume that we always read exactly one Ethernet frame, we
        // can consider all data after the ARP data as padding and simply
        // ignore this.
        if (frame_len < sizeof(*packet_ethernet_arp_ipv4))
        {
            return -1;
        }

        // size is ok, check if payload is a valid ARP IPv4 packet. Note that
        // network byte order is big endian, so we need ntohs() for all
        // integers
        arp_ethernet_ipv4_t *arp = &(packet_ethernet_arp_ipv4->arp_ipv4);
        if ((ntohs(ARP_HW_TYPE_ETHERNET) != arp->hw_type) && (ntohs(ARP_PROTOCOL_TYPE_IPv4) != arp->protocol_type) && (ntohs(ARP_ETH_HW_ADDR_LEN) != arp->hw_addr_len) && (ntohs(ARP_PROT_IPv4_ADDR_LEN) != arp->prot_addr_len))

        {
            return -1;
        }
        // we have an ARP IPv4 packet, check that target IP address matches
        static_assert(sizeof(arp->target_ip) == sizeof(TAP1_IP_ADDR));
        int is_ip_ok = (0 == memcmp(&(arp->target_ip), TAP1_IP_ADDR, sizeof(TAP1_IP_ADDR)));
        if (!is_ip_ok)
        {
            return -1;
        }

#endif

        return FRAME_LENGTH_IN_BYTES + frame_len;
    }

    int Write(vector<char> buf)
    {
        // if we return 0 everything was ok, -1 indicated an error

        size_t buffer_offset = 0;
        size_t buffer_len = buf.size();
        Debug_LOG_DEBUG("[%s] incomming len %zu", devname, buffer_len);

        // the frame prtocoll is: 2 byte frame length | frame data | .....

        // state machine
        static enum {
            RECEIVE_ERROR = 0,
            RECEIVE_FRAME_START,
            RECEIVE_FRAME_LEN,
            RECEIVE_FRAME_DATA
        } state = RECEIVE_FRAME_START;
        static size_t size_len = 0;
        static size_t frame_len = 0;
        static size_t frame_offset = 0;
        static int doDropFrame = true;
        static uint8_t out_buffer[ETHERNET_FRAME_MAX_SIZE];

        int partialFrame = (frame_offset > 0);
        if (partialFrame)
        {
            Debug_ASSERT(RECEIVE_FRAME_DATA == state);
        }

        for (;;)
        {
            switch (state)
            {
            //----------------------------------------------------------------------
            case RECEIVE_ERROR:
                Debug_LOG_ERROR("[%s] state RECEIVE_ERROR, drop %zu bytes",
                                devname, buffer_len);
                return -1;

            //----------------------------------------------------------------------
            case RECEIVE_FRAME_START:
                size_len = 2;
                frame_len = 0;
                frame_offset = 0;
                state = RECEIVE_FRAME_LEN;
                break; // could also fall through

            //----------------------------------------------------------------------
            case RECEIVE_FRAME_LEN:
                Debug_ASSERT(0 != size_len);

                do
                {
                    if (0 == buffer_len)
                    {
                        return 0;
                    }

                    // read a byte
                    uint8_t len_byte = buf[buffer_offset];
                    buffer_offset++;
                    buffer_len--;

                    // frame length is send in network byte order (big endian),
                    // so we build the value as: 0x0000 -> 0x00AA -> 0xAABB
                    frame_len <<= 8;
                    frame_len |= len_byte;

                } while (0 != --size_len);

                // we have read the length, make some sanity check and then
                // change state to read the frame data
                Debug_LOG_DEBUG("[%s] expecting ethernet frame of %zu bytes",
                                devname, frame_len);

                // if the frame is too big for our buffer, then the only option
                // is dropping it
                doDropFrame = (frame_len > ETHERNET_FRAME_MAX_SIZE);
                if (doDropFrame)
                {
                    Debug_LOG_WARNING("[%s] frame length %zu exceeds max length",
                                      devname, frame_len);
                }

                // is there any frame data?
                if (0 == frame_len)
                {
                    state = RECEIVE_FRAME_START;
                    break;
                }

                Debug_ASSERT(0 == frame_offset);
                state = RECEIVE_FRAME_DATA;
                break; // could also fall through

            //----------------------------------------------------------------------
            case RECEIVE_FRAME_DATA:
            {
                Debug_ASSERT(buffer_offset + buffer_len <= buf.size());
                Debug_ASSERT(0 == size_len);
                Debug_ASSERT(0 != frame_len);

                if (0 == buffer_len)
                {
                    return 0;
                }

                size_t chunk_len = frame_len - frame_offset;
                if (chunk_len > buffer_len)
                {
                    chunk_len = buffer_len;
                    partialFrame = true;
                }

                Debug_ASSERT(0 != chunk_len);

                if ((!doDropFrame) && partialFrame)
                {
                    memcpy(&out_buffer[frame_offset],
                           &buf[buffer_offset],
                           chunk_len);
                }

                Debug_ASSERT(buffer_len >= chunk_len);
                buffer_len -= chunk_len;
                buffer_offset += chunk_len;
                frame_offset += chunk_len;

                // check if we have received the full frame. If not then we
                // are done
                if (frame_offset < frame_len)
                {
                    Debug_LOG_ERROR("[%s] partial frame received, %zu of %zu bytes",
                                    devname, frame_offset, frame_len);
                    Debug_ASSERT(partialFrame);
                    Debug_ASSERT(0 == buffer_len);
                    return 0;
                }

                // we have received a full frame.
                if (!doDropFrame)
                {
                    void *b = out_buffer;
                    if (!partialFrame)
                    {
                        Debug_ASSERT(buffer_offset >= frame_len);
                        b = &buf[buffer_offset - frame_len];
                    }

                    Debug_LOG_DEBUG("[%s] writing frame of %zu bytes",
                                   devname, frame_len);
                    int ret = write(tapfd, b, frame_len);
                    if (ret < 0)
                    {
                        Debug_LOG_ERROR("[%s] write() failed for frame, error %d",
                                        devname, ret);
                        state = RECEIVE_ERROR;
                        break;
                    }

                    if ((unsigned)ret != frame_len)
                    {
                        Debug_LOG_ERROR("[%s] write() did write only %d",
                                        devname, ret);
                        state = RECEIVE_ERROR;
                        break;
                    }
                }

                state = RECEIVE_FRAME_START;
            }
            break;

            //----------------------------------------------------------------------
            default:
                Debug_LOG_ERROR("[%s] invalid state %d, drop %zu bytes",
                                devname, state, buffer_len);
                state = RECEIVE_ERROR;
                Debug_ASSERT(false); // we should never be here
                break;
            } // end switch (state)
        }     // end for(;;)

        return 0;
    } // end of write

    int Close()
    {
        Debug_LOG_DEBUG("Tap Close called");
        return close(tapfd);
    }

    int GetFileDescriptor() const
    {
        return tapfd;
    }

    int getMac(const char *name, uint8_t *mac)
    {
        Debug_LOG_DEBUG("device '%s'", name);

        int sck = socket(AF_INET, SOCK_DGRAM, 0);
        if (sck < 0)
        {
            Debug_LOG_ERROR("socket() failed, error %d", sck);
            return -1;
        }

        struct ifreq eth = {0};
        strcpy(eth.ifr_name, name);
        int ret = ioctl(sck, SIOCGIFHWADDR, &eth);
        if (ret < 0)
        {
            Debug_LOG_ERROR("ioctl(SIOCGIFHWADDR) failed, error %d", ret);
            close(sck);
            return -1;
        }

        close(sck);

        memcpy(mac, &eth.ifr_hwaddr.sa_data, 6);

        // We get the MAC address of the TAP device and we need to generate
        // a new one for the nic driver which avoids an address conflict
        // The original mac is generated randomly by the Linux kernel and we
        // just increment it.
        mac[5]++;

        // crude hack to keep to allow some filtering later. Actually none of
        // this should be hard coded, but a feature that can be set for each
        // TAP channel by whoever is using it.
        memcpy(mac_tap, mac, 6);
        strncpy(devname, name, 5);

        Debug_LOG_DEBUG("MAC %02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        return 0;
    }

    std::vector<char> HandlePayload(vector<char> buffer)
    {
        UartSocketGuestSocketCommand command = static_cast<UartSocketGuestSocketCommand>(buffer[0]);
        vector<char> result(7, 0);
        unsigned int channel = buffer[1];
        switch (command)
        {
        //-----------------------------------------------------------
        case UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_GETMAC:
        {
            Debug_LOG_DEBUG("command GET_MAC for channel %d", channel);
            vector<uint8_t> mac(6, 0);
            switch (channel)
            {
            //-----------------------------------------------------------
            case UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW:
                getMac("tap0", &mac[0]);
                break;

            //-----------------------------------------------------------
            case UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NW_2:
                getMac("tap1", &mac[0]);
                break;

            //-----------------------------------------------------------
            default:
                Debug_LOG_ERROR("unsupported channel %d", channel);
                result[0] = -1;
                return result;
            } // end switch(command)

            result[0] = 0;
            memcpy(&result[1], &mac[0], 6);
            return result;
        }

        //-----------------------------------------------------------
        case UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_START_READ:
            Debug_LOG_DEBUG("command START_READ for channel %d", channel);
            read_start = true;
            result[0] = 0;
            return result;

        //-----------------------------------------------------------
        case UART_SOCKET_GUEST_CONTROL_SOCKET_COMMAND_STOP_READ:
            Debug_LOG_DEBUG("command STOP_READ for channel %d", channel);
            read_start = false;
            printed_to_log = false;
            result[0] = 0;
            return result;

        //-----------------------------------------------------------
        default:
            break;
        } // end switch(command)

        Debug_LOG_ERROR("unknown command 0x%02x", command);
        result[0] = -1;
        return result;
    }

    ~Tap()
    {
        close(tapfd);
    }

private:
    int tapfd;
    uint8_t mac_tap[6]; /* Save tap mac addr and name for later use for filtering data */
    char devname[10] = {0};
    bool read_start = false;
    bool printed_to_log = false;

    void error(const char *msg) const
    {
        Debug_LOG_ERROR("%s", msg);
    }
};
