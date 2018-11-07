
#pragma once

#include <vector>

class OutputDevice
{
    public:
    virtual int Write(std::vector<unsigned char> buf) = 0;
};

class InputDevice
{
    public:
    virtual int Read(std::vector<unsigned char> &buf) = 0;
};
