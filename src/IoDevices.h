
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
};

class InputDevice
{
    public:
    virtual int Read(std::vector<char> &buf) = 0;
    virtual int Close() = 0;
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

    private:
    int fd;
};

