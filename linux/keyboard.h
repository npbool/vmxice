#include "define.h"

#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

/* In READ mode. Can be read at any time */
#define KEYB_REGISTER_STATUS  0x64

/* In WRITE mode. Writing this port sets Bit 3 of the status register to 1 and
   the byte is treated as a controller command */
#define KEYB_REGISTER_COMMAND 0x64

/* In READ mode. Should be read if bit 0 of status register is 1 */
#define KEYB_REGISTER_OUTPUT  0x60

/* In WRITE mode. Data should only be written if Bit 1 of the status register
   is zero (register is empty) */
#define KEYB_REGISTER_DATA    0x60

#define POLL_STATUS_ITERATIONS 12000

/* 
   8042 Status Register (port 64h read)

	|7|6|5|4|3|2|1|0|  8042 Status Register
	 | | | | | | | `---- output register (60h) has data for system
	 | | | | | | `----- input register (60h/64h) has data for 8042
	 | | | | | `------ system flag (set to 0 after power on reset)
	 | | | | `------- data in input register is command (1) or data (0)
	 | | | `-------- 1=keyboard enabled, 0=keyboard disabled (via switch)
	 | | `--------- 1=transmit timeout (data transmit not complete)
	 | `---------- 1=receive timeout (data transmit not complete)
	 `----------- 1=even parity rec'd, 0=odd parity rec'd (should be odd)
 */

/* Status register bits */   
#define KEYB_STATUS_OBUFFER_FULL        (1 << 0)
#define KEYB_STATUS_IBUFFER_FULL        (1 << 1)
#define KEYB_STATUS_TRANSMIT_TIMEOUT    (1 << 5)
#define KEYB_STATUS_PARITY_ERROR        (1 << 7)

/* i8042 Commands */
#define KEYB_COMMAND_WRITE_OUTPUT      0xd2
#define KEYB_COMMAND_DISABLE_KEYBOARD  0xad
#define KEYB_COMMAND_ENABLE_KEYBOARD   0xae
#define KEYB_COMMAND_DISABLE_MOUSE     0xa7
#define KEYB_COMMAND_ENABLE_MOUSE      0xa8

/* Scancode flags */
#define IS_SCANCODE_RELEASE(c) (c & 0x80)
#define SCANCODE_RELEASE_FLAG  0x80


ULONG KeyboardReadKeystroke(PUCHAR pc, PBOOLEAN pisMouse);

#endif
