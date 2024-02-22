/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#pragma once

#include "byte_transfer.h"

#define UART_CONTROL                 0x00
#define UART_MODEM_CONTROL           0x24
#define UART_MODEM_OFFSET            0x28
#define UART_CHANNEL_STS             0x2C
#define UART_TX_RX_FIFO              0x30

/*
    Note: this module represents functionality of the UART contained on the zynq7000 platform.
    It has been created to provide a communication channel between a "host" process in Linux
    and the seL4 in case the zynq7000 is simulated by QEMU.

    It has been found that this specific UART is not completely implemented in QEMU: especially
    hardware flow control is missing. Because of this only polling access has been implemented
    so far.

    Up to now this code has only been tested with QEMU. In theory it could also be used in
    case the seL4 is running on a real zynq7000 board. (In which case we would want to use
    and implement hardware flow control.)
*/

typedef struct
{
    void *baseAddress;
} QemuUart;

typedef  void (*UartIoWaitFunc)(void);

void UartInit(QemuUart *qemuUart, void *baseAddress);
void UartDeInit(QemuUart *qemuUart);

unsigned int UartReadRegister(QemuUart *qemuUart, unsigned int registerOffset);
void UartWriteRegister(QemuUart *qemuUart, unsigned int registerOffset, unsigned int registerValue);

void UartEnable(QemuUart *qemuUart);
void UartReadRegisterAndPrint(QemuUart *qemuUart, unsigned int registerOffset);

void UartWriteCustomWait(QemuUart *qemuUart, unsigned int byte, UartIoWaitFunc wait);
void UartWriteBusyWait(QemuUart *qemuUart, unsigned int byte);
void UartReadCustomWait(QemuUart *qemuUart, ByteTransferOperation *byteTransferOperation, UartIoWaitFunc wait);
void UartReadBusyWait(QemuUart *qemuUart, ByteTransferOperation *byteTransferOperation);

void UartWriteIfFifoIsEmpty(QemuUart *qemuUart, ByteTransferOperation *byteTransferOperation);
void UartReadIfFifoIsNotEmpty(QemuUart *qemuUart, ByteTransferOperation *byteTransferOperation);
