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

#include "lib_debug/Debug.h"
#include <ios>

#define FILENAME_LEN            7

#define CMD_GET_SIZE            0
#define CMD_WRITE               1
#define CMD_READ                2

#define DEFAULT_MEM_SIZE        (1024*1024) //1 MiB of memory
#define CHANNEL_NVM_1_MEM_SIZE  (36*1024*1024)  //36 MiB of memory
#define CHANNEL_NVM_2_MEM_SIZE  (128*1024)  //128 KiB of memory

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

    char m_checkMemoryBoundaries(uint32_t length, uint32_t address){
        //check if the address is outside of the memory bounds
        //if it is read 0 bytes
        if (address > m_memorySize){
            Debug_LOG_ERROR("Request size is outside of memory bounds! Adress: %d, Length: %d, Memory size: %lu", address, length, m_memorySize);
            return RET_ADDR_OUT_OF_BOUNDS;
        }
        //check if length exceeds the available room (from specified address to the end)
        //if it does read just the readable amount of bytes
        else if (length > (m_memorySize - address)){
            Debug_LOG_ERROR("Address outside of memory bounds! Adress: %d, Memory size: %lu", address, m_memorySize);
            return RET_LEN_OUT_OF_BOUNDS;
        }

        return RET_OK;
    }

    void m_handleGetSizeReq(char* data){
        data[RESP_COMM_INDEX] = CMD_GET_SIZE;
        data[RESP_RETVAL_INDEX] = RET_OK;
        m_cpyIntToBuf(m_memorySize, (unsigned char*)&data[RESP_BYTES_INDEX]);
    }

    void m_handleWriteReq(char* data, uint32_t length, uint32_t address){
        data[RESP_COMM_INDEX] = CMD_WRITE;
        m_cpyIntToBuf(0, (unsigned char*)&data[RESP_BYTES_INDEX]);

        char retValue = m_checkMemoryBoundaries(length, address);
        if(retValue != RET_OK){
            Debug_LOG_ERROR("Memory boundaries check failed!");
            data[RESP_RETVAL_INDEX] = retValue;
            return;
        }

        fstream file (m_filename, ios::in | ios::out | std::ios::binary);
        if(!file.is_open()){
            Debug_LOG_ERROR("Could not open file: %s", m_filename);
            data[RESP_RETVAL_INDEX] = RET_FILE_OPEN_ERR;
            return;
        }

        file.seekg(address, ios::beg);
        uint32_t pos_before = (uint32_t)file.tellp();
        file.write(&data[REQ_PAYLD_INDEX], length);
        uint32_t written_length = (uint32_t)file.tellp() - pos_before;
        file.close();

        data[RESP_RETVAL_INDEX] = ((written_length == length) ? RET_OK : RET_WRITE_ERR);
        m_cpyIntToBuf(written_length, (unsigned char*)&data[RESP_BYTES_INDEX]);
    }

    uint32_t m_handleReadReq(char* data, uint32_t length, uint32_t address){
        data[RESP_COMM_INDEX] = CMD_READ;
        m_cpyIntToBuf(0, (unsigned char*)&data[RESP_BYTES_INDEX]);

        char retValue = m_checkMemoryBoundaries(length, address);
        if(retValue != RET_OK){
            Debug_LOG_ERROR("Memory boundaries check failed!");
            data[RESP_RETVAL_INDEX] = retValue;
            return 0;
        }

        fstream file (m_filename, ios::in | ios::out | std::ios::binary);
        if(!file.is_open()){
            Debug_LOG_ERROR("Could not open file: %s", m_filename);
            data[RESP_RETVAL_INDEX] = RET_FILE_OPEN_ERR;
            return 0;
        }

        file.seekg(address, ios::beg);
        uint32_t pos_before = (uint32_t)file.tellp();
        file.read(&data[RESP_PAYLD_INDEX], length);
        uint32_t read_length = (uint32_t)file.tellp() - pos_before;
        file.close();

        data[RESP_RETVAL_INDEX] = ((read_length == length) ? RET_OK : RET_READ_ERR);
        m_cpyIntToBuf(read_length, (unsigned char*)&data[RESP_BYTES_INDEX]);
        return read_length;

    }

public:
    Nvm(unsigned chanNum)
    {
        Debug_LOG_TRACE("%s", __func__);
        snprintf(m_filename, sizeof(m_filename), "nvm_%02u", chanNum);
        fstream file (m_filename, ios::app | std::ios::binary);

        if(!file.is_open())
        {
            Debug_LOG_ERROR("File could not be opened!");
            return;
        }
        if(!file.good()){
            Debug_LOG_ERROR("File is corrupted!");
            return;
        }

        Debug_LOG_INFO("File opened succesfully!");

        switch (chanNum)
        {
        case UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NVM:
            m_memorySize = CHANNEL_NVM_1_MEM_SIZE;
            break;
        case UART_SOCKET_LOGICAL_CHANNEL_CONVENTION_NVM2:
            m_memorySize = CHANNEL_NVM_2_MEM_SIZE;
            break;

        default:
            m_memorySize = DEFAULT_MEM_SIZE;
            break;
        }

        file.seekg(0, file.end);
        int length = file.tellg();
        file.seekg(0, file.beg);

        if (length == 0){
            Debug_LOG_INFO("Length = 0, initializing!");
            for(unsigned int i = 0; i < m_memorySize; i++){
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
        uint32_t length, address, payloadLength = 0;
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
                m_handleGetSizeReq(data);
                break;
            }
            case CMD_WRITE:
            {
                std::copy(buffer.begin(), buffer.end(), data);
                m_handleWriteReq(data, length, address);
                break;
            }
            case CMD_READ:
            {
                payloadLength = m_handleReadReq(data, length, address);
                break;
            }
            default:
                Debug_LOG_ERROR("Unsupported NVM command 0x%02x", buffer[REQ_COMM_INDEX]);
        }

        for(unsigned int i = 0; i < payloadLength + RESP_HEADER_LEN; i++){
            response.push_back(data[i]);
        }

        return response;
    }

    ~Nvm()
    {
        Debug_LOG_TRACE("%s", __func__);
    }
};
