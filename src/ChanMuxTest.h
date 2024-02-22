/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include "IoDevices.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fstream>

#include "lib_debug/Debug.h"
#include <ios>

#define CMD_TEST_OVERFLOW       0
#define CMD_TEST_FULL_DUPLEX    1
#define CMD_TEST_MAX_SIZE       2

#define MAX_MSG_LEN             4096

#define REQ_COMM_INDEX          0
#define REQ_ARG_INDEX           (REQ_COMM_INDEX + 1)
#define REQ_PAYLOAD_INDEX       (REQ_ARG_INDEX + 4)
//RETURN MESSAGES
#define RET_OK                  0


using namespace std;

class ChanMuxTest : public InputDevice, public OutputDevice
{
private:
    void m_cpyIntToBuf(uint32_t integer, unsigned char* buf){
        buf[0] = (integer >> 24) & 0xFF;
        buf[1] = (integer >> 16) & 0xFF;
        buf[2] = (integer >> 8) & 0xFF;
        buf[3] = integer & 0xFF;
    }

    size_t m_cpyBufToInt(unsigned char* buf){
        return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | (buf[3]);
    }

public:
    ChanMuxTest (void)
    {
        Debug_LOG_TRACE("%s", __func__);
    }

    int Read(vector<char> &buf)
    {
        Debug_LOG_WARNING("%s: not supported", __func__);
        return 0; // This is a command channel not a stream
    }

    int Write(vector<char> buf)
    {
        Debug_LOG_WARNING("%s: not supported", __func__);
        return 0;
    }

    int Close()
    {
        Debug_LOG_TRACE("%s", __func__);
        return 0;
    }

    std::vector<char> HandlePayload(vector<char> buffer)
    {
        uint32_t payloadLength = 0;
        vector<char> response;

        switch (buffer[REQ_COMM_INDEX])
        {
            case CMD_TEST_OVERFLOW:
            {
                Debug_LOG_DEBUG("%s: got a CMD_TEST_OVERFLOW", __func__);
                payloadLength = MAX_MSG_LEN;
                response.insert(response.begin(), payloadLength, 0);

                break;
            }
            case CMD_TEST_FULL_DUPLEX:
            {
                Debug_LOG_DEBUG("%s: got a CMD_TEST_FULL_DUPLEX", __func__);
                payloadLength =
                    m_cpyBufToInt((unsigned char*) &buffer[REQ_ARG_INDEX]);
                size_t offs = REQ_ARG_INDEX + sizeof(payloadLength);

                response.insert(response.begin(),
                                buffer.begin() + offs,
                                buffer.begin() + offs + payloadLength);
                break;
            }
            case CMD_TEST_MAX_SIZE:
            {
                Debug_LOG_DEBUG("%s: got a CMD_TEST_MAX_SIZE", __func__);
                payloadLength =
                    m_cpyBufToInt((unsigned char*) &buffer[REQ_ARG_INDEX]);

                size_t i = 0;
                while (i < payloadLength)
                {
                    char expected = i % 256;
                    if (buffer[i + REQ_PAYLOAD_INDEX] == expected)
                    {
                        i++;
                    }
                    else
                    {
                        break;
                    }
                }
                unsigned char data[4];
                m_cpyIntToBuf(i, data);

                response.insert(response.begin(),
                                std::begin(data),
                                std::end(data));

                break;
            }
            default:
                Debug_LOG_ERROR("Unsupported ChanMuxTest command 0x%02x", buffer[REQ_COMM_INDEX]);
        }
        return response;
    }

    ~ChanMuxTest()
    {
        Debug_LOG_TRACE("%s", __func__);
    }
};
