/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

typedef enum
{
    DEVICE_TYPE_PSEUDO_CONSOLE,
    DEVICE_TYPE_SOCKET,
    DEVICE_TYPE_RAW_SERIAL,
    DEVICE_TYPE_UNKOWN
} DeviceType;