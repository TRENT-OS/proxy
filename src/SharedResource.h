/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include <mutex>

using namespace std;


template <typename T>
class SharedResource
{
    public:
    SharedResource(T *value) : value(value){}

    void Lock() const { lock.lock(); }
    void UnLock() const { lock.unlock(); }
    T *GetResource() { return value; }

    private:
    T *value;
    mutable std::mutex lock;
};
