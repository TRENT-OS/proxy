/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
 */
#pragma once

#include "IoDevices.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fstream>

#include "LibDebug/Debug.h"
#include <ios>

#define CMD_TEST_OVERFLOW       0
#define CMD_TEST_FULL_DUPLEX    1

#define MAX_MSG_LEN             4096
#define RESP_HEADER_LEN         0

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
        Debug_LOG_TRACE("%s", __func__);
        return 0; // This is a command channel not a stream
    }

    int Write(vector<char> buf)
    {
        Debug_LOG_TRACE("%s", __func__);
        return 0;
    }

    int Close()
    {
        Debug_LOG_TRACE("%s", __func__);
        return 0;
    }

    std::vector<char> HandlePayload(vector<char> buffer)
    {
        uint32_t length, address, payloadLength = 0;
        vector<char> response;
        char data[MAX_MSG_LEN];

        switch (buffer[REQ_COMM_INDEX])
        {
            case CMD_TEST_OVERFLOW:
            {
                Debug_LOG_DEBUG("%s: got a CMD_TEST_OVERFLOW", __func__);
                payloadLength = MAX_MSG_LEN;
                break;
            }
            default:
                Debug_LOG_ERROR("\nUnsupported ChanMuxTest command!\n");
        }

        for (unsigned int i = 0; i < payloadLength; i++)
        {
            response.push_back(data[i]);
        }

        return response;
    }

    ~ChanMuxTest()
    {
        Debug_LOG_TRACE("%s", __func__);
    }
};
