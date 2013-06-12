#include <linux/module.h>
#include "vmx.h"
#include "keyboard.h"

ULONG i8042ReadKeyboardData(PUCHAR pc, PBOOLEAN pisMouse)
{
    UCHAR port_status;

    port_status = READ_PORT_UCHAR(KEYB_REGISTER_STATUS);
    while (port_status & KEYB_STATUS_OBUFFER_FULL)
    {
        /* Data is available */
        *pc = READ_PORT_UCHAR(KEYB_REGISTER_DATA);
        /* Check if data is valid (i.e., no timeout, no parity error) */
        if ((port_status & KEYB_STATUS_PARITY_ERROR) == 0)
        {
            /* Check if this is a mouse event or not */
            *pisMouse = (port_status & KEYB_STATUS_TRANSMIT_TIMEOUT) != 0;
            return true;
        }
    }
    return false;
}

/* Inspired from ReactOS's i8042 keyboard driver */
ULONG KeyboardReadKeystroke(PUCHAR pc, PBOOLEAN pisMouse)
{
    ULONG counter;
    UCHAR port_status;
    UCHAR scancode;
    ULONG r;

    counter = POLL_STATUS_ITERATIONS;
    while (counter)
    {
        port_status = READ_PORT_UCHAR(KEYB_REGISTER_STATUS);
        r = i8042ReadKeyboardData(&scancode, pisMouse);
        if (r == true)
            break;
        
        _CpuSleep(1);
        counter--;
    }

    if (counter == 0)
        return false;

    *pc = scancode;
    return true;
}
