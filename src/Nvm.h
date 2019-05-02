/**
 * Copyright (C) 2019, Hensoldt Cyber GmbH
 */
#pragma once

#include "IoDevices.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "LibDebug/Debug.h"
#include <ios>

#define CMD_GET_SIZE    0
#define CMD_WRITE       1
#define CMD_READ        2


using namespace std;

class Nvm : public InputDevice, public OutputDevice
{
private:
//    fstream m_file;
public:
    Nvm(unsigned chanNum)
    {
        Debug_LOG_TRACE("%s", __func__);

//        char filename[16];
//        snprintf(filename, "nvm_%u", chanNum);
//        m_file.open(filename, ios::out|ios::in);
//
//        if (m_file.failbit)
//        {
//            return false;
//        }
//        return true;
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
        vector<char> response(6);
        Debug_LOG_TRACE("%s: printing buffer...", __func__);

        for (unsigned i = 0; i < buffer.size(); i++)
        {
            Debug_PRINTF(" %02x", buffer[i]);
        }
        Debug_PRINTFLN("%s","");

        response[0] = CMD_WRITE;
        response[1] = 0;
        response[2] = buffer[5];
        response[3] = buffer[6];
        response[4] = buffer[7];
        response[5] = buffer[8];

        return response;
    }

    ~Nvm()
    {
        Debug_LOG_TRACE("%s", __func__);
    }
};
