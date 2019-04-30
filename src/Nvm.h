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
        Debug_LOG_TRACE("%s: printing buffer...", __func__);

        for (unsigned i = 0; i < buffer.size(); i++)
        {
            Debug_PRINTF(" %02x", buffer[i]);
        }
        Debug_PRINTFLN("%s","");
        return buffer;
    }

    ~Nvm()
    {
        Debug_LOG_TRACE("%s", __func__);
    }
};
