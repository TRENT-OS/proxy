/*
 * Copyright (C) 2019-2024, HENSOLDT Cyber GmbH
 * 
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * For commercial licensing, contact: info.cyber@hensoldt.net
 */

#include "uart.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


#define UART_REG(baseAddress, x) ((volatile uint32_t *)(baseAddress + (x)))

#define UART_CONTROL_RX_EN           (1U << 2)
#define UART_CONTROL_RX_DIS          (1U << 3)

#define UART_CONTROL_TX_EN           (1U << 4)
#define UART_CONTROL_TX_DIS          (1U << 5)

#define UART_CHANNEL_STS_TXEMPTY     (1U << 3)
#define UART_CHANNEL_STS_RXEMPTY     (1U << 1)

#define UART_MODEMCR_FCM             (1U << 5)

#define BIT(x) (1 << (x))

typedef struct
{
    bool isWrite;
    unsigned int registerOffset;
    unsigned int registerValue;
} RegisterAccessOperation;

#if defined(CAMKES)
extern unsigned UartDrv_readReg(unsigned offset);
extern void     UartDrv_writeReg(unsigned offset, unsigned value);
#endif

static unsigned int HwUartReadRegister(QemuUart* qemuUart,
                                       unsigned int registerOffset)
{
#if !defined(CAMKES)
    unsigned char offset = (unsigned char)(registerOffset & 0xff);

    unsigned int value = (unsigned int)(*UART_REG(qemuUart->baseAddress, offset));

    return value;
#else
    return UartDrv_readReg(registerOffset);
#endif
}

static void HwUartWriteRegister(QemuUart* qemuUart, unsigned int registerOffset,
                                unsigned int registerValue)
{
#if !defined(CAMKES)
    unsigned char offset = (unsigned char)(registerOffset & 0xff);

    *UART_REG(qemuUart->baseAddress, offset) = registerValue;
#else
    UartDrv_writeReg(registerOffset, registerValue);
#endif
}

static void HwUartReadRegisterOperation(QemuUart* qemuUart,
                                        RegisterAccessOperation* registerAccessOperation)
{
    registerAccessOperation->registerValue = HwUartReadRegister(qemuUart,
                                                                registerAccessOperation->registerOffset);
}

static void HwUartWriteRegisterOperation(QemuUart* qemuUart,
                                         RegisterAccessOperation* registerAccessOperation)
{
    HwUartWriteRegister(qemuUart, registerAccessOperation->registerOffset,
                        registerAccessOperation->registerValue);
}

void UartInit(QemuUart* qemuUart, void* baseAddress)
{
    qemuUart->baseAddress = baseAddress;
}

void UartDeInit(QemuUart* qemuUart)
{
}

unsigned int UartReadRegister(QemuUart* qemuUart, unsigned int registerOffset)
{
    RegisterAccessOperation registerAccessOperation;

    registerAccessOperation.registerOffset = registerOffset;
    HwUartReadRegisterOperation(qemuUart, &registerAccessOperation);

    return registerAccessOperation.registerValue;
}

void UartWriteRegister(QemuUart* qemuUart, unsigned int registerOffset,
                       unsigned int registerValue)
{
    RegisterAccessOperation registerAccessOperation;

    registerAccessOperation.registerOffset = registerOffset;
    registerAccessOperation.registerValue = registerValue;

    HwUartWriteRegisterOperation(qemuUart, &registerAccessOperation);
}

void UartEnable(QemuUart* qemuUart)
{
    uint32_t v = UartReadRegister(qemuUart, UART_CONTROL);

    /* Enable writing. */
    v |= UART_CONTROL_TX_EN;
    v &= ~UART_CONTROL_TX_DIS;

    /* Enable reading. */
    v |= UART_CONTROL_RX_EN;
    v &= ~UART_CONTROL_RX_DIS;

    UartWriteRegister(qemuUart, UART_CONTROL, v);

#if 0
    /* Hardware flow control is not supported by the QEMU serial port of the zynq platform. */
    /* Hardware flow control. */
    v = *UART_REG(UART_MODEM_CONTROL);
    v |= UART_MODEMCR_FCM;

    *UART_REG(UART_MODEM_CONTROL) = v;
#endif
}

void UartReadRegisterAndPrint(QemuUart* qemuUart, unsigned int registerOffset)
{
    printf("UART register %02x: %08x\n", registerOffset, UartReadRegister(qemuUart,
            registerOffset));
}

void UartWriteCustomWait(QemuUart* qemuUart, unsigned int byte,
                         UartIoWaitFunc wait)
{
    ByteTransferOperation byteTransferOperation;

    byteTransferOperation.byte = byte;

    do
    {
        UartWriteIfFifoIsEmpty(qemuUart, &byteTransferOperation);
        if (byteTransferOperation.result == 0)
        {
            break;
        }

        if (wait != NULL)
        {
            wait();
        }
    }
    while (byteTransferOperation.result != 0);
}

void UartWriteBusyWait(QemuUart* qemuUart, unsigned int byte)
{
    UartWriteCustomWait(qemuUart, byte, NULL);
}

void UartReadCustomWait(QemuUart* qemuUart,
                        ByteTransferOperation* byteTransferOperation, UartIoWaitFunc wait)
{
    do
    {
        UartReadIfFifoIsNotEmpty(qemuUart, byteTransferOperation);
        if (byteTransferOperation->result == 0)
        {
            break;
        }
        else
        {
            if (wait != NULL)
            {
                wait();
            }
        }
    }
    while (byteTransferOperation->result != 0);
}

void UartReadBusyWait(QemuUart* qemuUart,
                      ByteTransferOperation* byteTransferOperation)
{
    UartReadCustomWait(qemuUart, byteTransferOperation, NULL);
}

void UartWriteIfFifoIsEmpty(QemuUart* qemuUart,
                            ByteTransferOperation* byteTransferOperation)
{
    /* Transmit if FIFO is empty. */
    if ((UartReadRegister(qemuUart,
                          UART_CHANNEL_STS) & UART_CHANNEL_STS_TXEMPTY) != 0)
    {
        UartWriteRegister(qemuUart, UART_TX_RX_FIFO, byteTransferOperation->byte);
        byteTransferOperation->result = 0;
    }
    else
    {
        byteTransferOperation->result = -1;
    }
}

void UartReadIfFifoIsNotEmpty(QemuUart* qemuUart,
                              ByteTransferOperation* byteTransferOperation)
{
    if ((UartReadRegister(qemuUart,
                          UART_CHANNEL_STS) & UART_CHANNEL_STS_RXEMPTY) == 0)
    {
        byteTransferOperation->byte = (char)((UartReadRegister(qemuUart,
                                                               UART_TX_RX_FIFO)) & 0xff);
        byteTransferOperation->result = 0;
    }
    else
    {
        byteTransferOperation->result = -1;
    }
}
