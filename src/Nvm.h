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

#define FILENAME_LEN            7

#define CMD_GET_SIZE            0
#define CMD_WRITE               1
#define CMD_READ                2

#define DEFAULT_MEM_SIZE        1024*1024 //1 MB of memory
#define CHAN_6_MEM_SIZE         128*1024 //128 kB of memory

#define MAX_MSG_LEN             4096
#define RESP_HEADER_LEN         6

//INDEXES OF DIFFERENT PARTS OF THE REQUEST MESSAGE (IN A BUFFER)
#define REQ_COMM_INDEX          0
#define REQ_ADDR_INDEX          1
#define REQ_LEN_INDEX           5
#define REQ_PAYLD_INDEX         9

//INDEXES OF DIFFERENT PARTS OF THE RESPONSE MESSAGE (IN A BUFFER)
#define RESP_COMM_INDEX         0
#define RESP_RETVAL_INDEX       1
#define RESP_BYTES_INDEX        2
#define RESP_PAYLD_INDEX        6

//RETURN MESSAGES
#define RET_OK                   0
#define RET_GENERIC_ERR         -1
#define RET_FILE_OPEN_ERR       -2
#define RET_WRITE_ERR           -3
#define RET_READ_ERR            -4
#define RET_LEN_OUT_OF_BOUNDS   -5
#define RET_ADDR_OUT_OF_BOUNDS  -6


using namespace std;

class Nvm : public InputDevice, public OutputDevice
{
private:
    char m_filename[FILENAME_LEN];
    size_t m_memorySize;

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
    Nvm(unsigned chanNum)
    {
        Debug_LOG_TRACE("%s", __func__);
        snprintf(m_filename, sizeof(m_filename), "nvm_%02u", chanNum);
        fstream file (m_filename, ios::app | std::ios::binary);

        if(!file.is_open())
        {
            Debug_LOG_ERROR("\nFile could not be opened!\n");
            return;
        }
        if(!file.good()){
            Debug_LOG_ERROR("\nFile is corrupted!\n");
            return;
        }

        Debug_LOG_INFO("\nFile opened succesfully!\n");

        switch (chanNum)
        {
        case 6:
            m_memorySize = CHAN_6_MEM_SIZE;
            break;
        
        default:
            m_memorySize = DEFAULT_MEM_SIZE;
            break;
        }

        file.seekg(0, file.end);
        int length = file.tellg();
        file.seekg(0, file.beg);

        if (length == 0){
            Debug_LOG_INFO("\nLength = 0, initializing!\n");
            for(int i = 0; i < m_memorySize; i++){
                file.put(0);
            }
        }

        file.close();
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
        int length, address;
        vector<char> response;
        char data[MAX_MSG_LEN];

        if(buffer[REQ_COMM_INDEX] != CMD_GET_SIZE){
            length = m_cpyBufToInt((unsigned char*)&buffer[REQ_LEN_INDEX]);
            address = m_cpyBufToInt((unsigned char*)&buffer[REQ_ADDR_INDEX]);
        }

        switch (buffer[REQ_COMM_INDEX])
        {
            case CMD_GET_SIZE:
            {
                data[RESP_COMM_INDEX] = CMD_GET_SIZE;
                data[RESP_RETVAL_INDEX] = RET_OK;
                m_cpyIntToBuf(m_memorySize, (unsigned char*)&data[RESP_BYTES_INDEX]);

                for(int i = 0; i < RESP_HEADER_LEN; i++){
                    response.push_back(data[i]);
                }
 
                return response;
            }        
            case CMD_WRITE:
            {
                std::copy(buffer.begin(), buffer.end(), data);

                //check if the address is outside of the memory bounds
                //if it is read 0 bytes
                if (address > m_memorySize){
                    Debug_LOG_ERROR("Request size is outside of memory bounds! Adress: %d, Length: %d, Memory size: %lu", address, length, m_memorySize);
                    data[RESP_COMM_INDEX] = CMD_WRITE;
                    data[RESP_RETVAL_INDEX] = RET_ADDR_OUT_OF_BOUNDS;
                    m_cpyIntToBuf(0, (unsigned char*)&data[RESP_BYTES_INDEX]);
                }
                //check if length exceeds the available room (from specified address to the end)
                //if it does read just the readable amount of bytes
                else if (length > (m_memorySize - address)){
                    Debug_LOG_ERROR("Address outside of memory bounds! Adress: %d, Memory size: %lu", address, m_memorySize);
                    data[RESP_COMM_INDEX] = CMD_WRITE;
                    data[RESP_RETVAL_INDEX] = RET_LEN_OUT_OF_BOUNDS;
                    m_cpyIntToBuf(0, (unsigned char*)&data[RESP_BYTES_INDEX]);
                }
                else{
                    fstream file (m_filename, ios::in | ios::out | std::ios::binary);
                    if(file.is_open()){
                        file.seekg(address, ios::beg);
                        uint32_t pos_before = (uint32_t)file.tellp();
                        file.write(&data[REQ_PAYLD_INDEX], length * sizeof(char));
                        uint32_t written_length = (uint32_t)file.tellp() - pos_before;
                        file.close();

                        data[RESP_COMM_INDEX] = CMD_WRITE;
                        data[RESP_RETVAL_INDEX] = written_length == length ? RET_OK : RET_WRITE_ERR;
                        m_cpyIntToBuf(written_length, (unsigned char*)&data[RESP_BYTES_INDEX]);
                    }
                    else{
                        Debug_LOG_ERROR("Could not open file: %s", m_filename);
                        data[RESP_COMM_INDEX] = CMD_WRITE;
                        data[RESP_RETVAL_INDEX] = RET_FILE_OPEN_ERR;
                        m_cpyIntToBuf(int(0), (unsigned char*)&data[RESP_BYTES_INDEX]);
                    }
                }

                for(int i = 0; i < RESP_HEADER_LEN; i++){
                    response.push_back(data[i]);
                }

                return response;
            }
            case CMD_READ:
            {   
                uint32_t read_length = 0;
                //check if the address is outside of the memory bounds
                //if it is read 0 bytes
                if (address > m_memorySize){
                    Debug_LOG_ERROR("Request size is outside of memory bounds! Adress: %d, Length: %d, Memory size: %lu", address, length, m_memorySize);
                    data[RESP_COMM_INDEX] = CMD_READ;
                    data[RESP_RETVAL_INDEX] = RET_ADDR_OUT_OF_BOUNDS;
                    m_cpyIntToBuf(0, (unsigned char*)&data[RESP_BYTES_INDEX]);
                }
                //check if length exceeds the available room (from specified address to the end)
                //if it does read just the readable amount of bytes
                else if (length > (m_memorySize - address)){
                    Debug_LOG_ERROR("Address outside of memory bounds! Adress: %d, Memory size: %lu", address, m_memorySize);
                    data[RESP_COMM_INDEX] = CMD_READ;
                    data[RESP_RETVAL_INDEX] = RET_LEN_OUT_OF_BOUNDS;
                    m_cpyIntToBuf(0, (unsigned char*)&data[RESP_BYTES_INDEX]);
                }
                else{
                    fstream file (m_filename, ios::in | ios::out | std::ios::binary);
                    if(file.is_open()){
                        file.seekg(address, ios::beg);
                        uint32_t pos_before = (uint32_t)file.tellp();
                        file.read(&data[RESP_PAYLD_INDEX], length * sizeof(char));
                        read_length = (uint32_t)file.tellp() - pos_before;
                        file.close();

                        data[RESP_COMM_INDEX] = CMD_READ;
                        data[RESP_RETVAL_INDEX] = read_length == length ? RET_OK : RET_READ_ERR;
                        m_cpyIntToBuf(read_length, (unsigned char*)&data[RESP_BYTES_INDEX]);
                    }
                    else{
                        Debug_LOG_ERROR("Could not open file: %s", m_filename);
                        data[RESP_COMM_INDEX] = CMD_READ;
                        data[RESP_RETVAL_INDEX] = RET_FILE_OPEN_ERR;
                        m_cpyIntToBuf(int(0), (unsigned char*)&data[RESP_BYTES_INDEX]);                   
                    }
                }
                
                for(int i = 0; i < read_length + RESP_HEADER_LEN; i++){
                    response.push_back(data[i]);
                }

                return response;
            }
            default:
                Debug_LOG_ERROR("\nUnsupported NVM command!\n");

                return response;
        }
    }

    ~Nvm()
    {
        Debug_LOG_TRACE("%s", __func__);
    }
};
