
#pragma once

#include <unistd.h>

#include <vector>
#include <iostream>

using namespace std;

class OutputDevice
{
    public:
    virtual int Write(std::vector<char> buf) = 0;
    virtual int Close() = 0;
    virtual std::vector<char> HandlePayload(vector<char> payload) = 0;
    virtual int getMac(const char* name,char *mac) =0;
};

class InputDevice
{
    public:
    virtual int Read(std::vector<char> &buf) = 0;
    virtual int Close() = 0;

};

// Used by SocketAdmin.cpp. Up to now represents a Linux socket. Later on: wrapper for TUN interface; PICO socket.
class IoDevice
{
    public:
    // Return value = success indication of creation
    virtual int Create() = 0;
    virtual OutputDevice *GetOutputDevice() = 0;
    virtual InputDevice *GetInputDevice() = 0;
    virtual ~IoDevice() {}
};

// Indirection for the from guest thread; currently it yields Linux sockets
class IoDeviceCreator
{
    public:
    virtual IoDevice *Create() = 0;
    virtual ~IoDeviceCreator() {}
};

class OutputLogger : public OutputDevice
{
    public:
    OutputLogger() {}

    int Write(vector<char> buf) 
    { 
        cout << string(&buf[0], buf.size());
    }

    int Close()
    {
        return 0;
    }
    int getMac(const char* name,char *mac)
    {
      return 0;
    }
    std::vector<char> HandlePayload(vector<char> payload)
    {
        return payload;
    }
};

class DeviceReader : public InputDevice
{
    public:
    DeviceReader(int fd) : fd(fd) {}

    int Read(std::vector<char> &buf)
    {
        return read(fd, &buf[0], buf.size());
    }

    int Close()
    {
        return close(fd);
    }


    private:
    int fd;
};

class DeviceWriter : public OutputDevice
{
    public:
    DeviceWriter(int fd) : fd(fd) {}

    int Write(std::vector<char> buf)
    {
        return write(fd, &buf[0], buf.size());
    }

    int Close()
    {
        return close(fd);
    }
    int getMac(const char* name,char *mac)
    {
      return 0;
    }

    std::vector<char> HandlePayload(vector<char> payload)
    {
        return payload;
    }

    private:
    int fd;
};

