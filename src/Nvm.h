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
        Debug_LOG_TRACE("%s: printing buffer...", __func__);

        for (unsigned i = 0; i < buffer.size(); i++)
        {
            Debug_PRINTF(" %02x", (unsigned char)buffer[i]);
        }
        Debug_PRINTFLN("%s","");

        switch (buffer[0])
        {
            case CMD_GET_SIZE:
            {
                vector<char> response(6);

                response[0] = CMD_GET_SIZE;
                response[1] = 0;
                response[2] = 0;
                response[3] = 0;
                response[4] = 0x10;
                response[5] = 0x00;

                return response; 
            }        
            case CMD_WRITE:
            {
                vector<char> response(6);

                response[0] = CMD_WRITE;
                response[1] = 0;
                response[2] = (unsigned char)buffer[5];
                response[3] = (unsigned char)buffer[6];
                response[4] = (unsigned char)buffer[7];
                response[5] = (unsigned char)buffer[8];

                return response;
            }
            case CMD_READ:
            {
                vector<char> response(1024);
                int length = ((unsigned char)buffer[5] << 24) | ((unsigned char)buffer[6] << 16) | ((unsigned char)buffer[7] << 8) | ((unsigned char)buffer[8]);

                response[0] = CMD_READ;
                response[1] = 0;
                response[2] = (unsigned char)buffer[5];
                response[3] = (unsigned char)buffer[6];
                response[4] = (unsigned char)buffer[7];
                response[5] = (unsigned char)buffer[8];

                for(int i = 0; i < length; i++){
                    response[i+6] = i;
                }

                return response;
            }
            default:
                vector<char> response(1024);
                return response;
        }
    }

    ~Nvm()
    {
        Debug_LOG_TRACE("%s", __func__);
    }
};
