
#include "uart_io_guest.h"
#include "type.h"
#include "uart.h"
#include "lib_compiler/compiler.h"

static int UartIoGuestOpen(IUartIo* instance)
{
    UartIoGuest* uartIoGuest = container_of(instance, UartIoGuest, implementation);

    UartEnable(&uartIoGuest->uart);

    UNUSED_VAR(uartIoGuest);
    return 0;
}

static int UartIoGuestClose(IUartIo* instance)
{
    UartIoGuest* uartIoGuest = container_of(instance, UartIoGuest, implementation);

    UNUSED_VAR(uartIoGuest);
    return 0;
}

static void UartIoGuestWriteByte(IUartIo* instance,
                                 UartByteTransferOperation* uartByteTransferOperation)
{
    UartIoGuest* uartIoGuest = container_of(instance, UartIoGuest, implementation);
    ByteTransferOperation byteTransferOperation;

    if (uartIoGuest->isBlocking)
    {
        UartWriteCustomWait(&uartIoGuest->uart, uartByteTransferOperation->byte,
                            uartIoGuest->wait);
        byteTransferOperation.result = 0;
    }
    else
    {
        byteTransferOperation.byte = uartByteTransferOperation->byte;
        UartWriteIfFifoIsEmpty(&uartIoGuest->uart, &byteTransferOperation);
    }

    uartByteTransferOperation->result = byteTransferOperation.result;
}

static void UartIoGuestReadByte(IUartIo* instance,
                                UartByteTransferOperation* uartByteTransferOperation)
{
    UartIoGuest* uartIoGuest = container_of(instance, UartIoGuest, implementation);
    ByteTransferOperation byteTransferOperation;

    if (uartIoGuest->isBlocking)
    {
        UartReadCustomWait(&uartIoGuest->uart, &byteTransferOperation,
                           uartIoGuest->wait);
        byteTransferOperation.result = 0;
    }
    else
    {
        UartReadIfFifoIsNotEmpty(&uartIoGuest->uart, &byteTransferOperation);
    }

    uartByteTransferOperation->byte = byteTransferOperation.byte;
    uartByteTransferOperation->result = byteTransferOperation.result;
}

void UartIoGuestInit(UartIoGuest* instance, unsigned int flags,
                     UartIoWaitFunc wait
#if !defined(CAMKES)
                     , void* baseAddress
#endif
                    )
{
    instance->implementation.Open = UartIoGuestOpen;
    instance->implementation.Close = UartIoGuestClose;
    instance->implementation.WriteByte = UartIoGuestWriteByte;
    instance->implementation.ReadByte = UartIoGuestReadByte;

    instance->isBlocking = (flags & UART_IO_GUEST_FLAG_NONBLOCKING) == 0;
    instance->wait = wait;

#if !defined(CAMKES)
    UartInit(&instance->uart, baseAddress);
#endif
}

