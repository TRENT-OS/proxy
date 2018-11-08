
#pragma once

#include <vector>
#include <mutex>

using namespace std;

class GuestListeners
{
    public:
    GuestListeners(unsigned int numListeners) : 
        numListeners(numListeners),
        listeners(numListeners, nullptr)
    {
    }

    void SetListener(unsigned int listenerIndex, OutputDevice *listener)
    {
        lock.lock();

        if (listenerIndex < numListeners)
        {
            listeners[listenerIndex] = listener;
        }

        lock.unlock();
    }

    OutputDevice *GetListener(unsigned int listenerIndex) const
    {
        OutputDevice *result = nullptr;

        lock.lock();
        
        if (listenerIndex < numListeners)
        {
            result = listeners[listenerIndex];
        }

        lock.unlock();

        return result;
    }

    private:
    unsigned int numListeners;
    vector<OutputDevice *> listeners;
    mutable mutex lock;
};

