#include "define.h"

#ifndef _SCANCODE_H_
#define _SCANCODE_H_

#define BREAK_CODE              0x80
#define EXTEND_CODE             0xE0

/*
 * ·ÇASCIIÇøÓò
 */
#define SCANCODE_ESC            0x01
#define SCANCODE_F1             0x3B
#define SCANCODE_F2             0x3C
#define SCANCODE_F3             0x3D
#define SCANCODE_F4             0x3E
#define SCANCODE_F5             0x3F
#define SCANCODE_F6             0x40
#define SCANCODE_F7             0x41
#define SCANCODE_F8             0x42
#define SCANCODE_F9             0x43
#define SCANCODE_F10            0x44
#define SCANCODE_F11            0x57
#define SCANCODE_F12            0x58
#define SCANCODE_CAPSLOCK       0x3A
#define SCANCODE_LSHIFT         0x2A
#define SCANCODE_RSHIFT         0x36
#define SCANCODE_LCTRL          0x1D
#define SCANCODE_LWIN           0xE05B
#define SCANCODE_LALT           0x38
#define SCANCODE_RALT           0xE038
#define SCANCODE_RMENU          0xE05D
#define SCANCODE_RCTRL          0xE01D
#define SCANCODE_INSERT         0xE052
#define SCANCODE_DEL            0xE053
#define SCANCODE_LEFT           0xE04B
#define SCANCODE_RIGHT          0xE04D
#define SCANCODE_UP             0xE048
#define SCANCODE_DOWN           0xE050
#define SCANCODE_PGUP           0xE049
#define SCANCODE_PGDN           0xE051
#define SCANCODE_HOME           0xE047
#define SCANCODE_END            0xE04F

/*
 * ASCIIÇøÓò
 */
#define SCANCODE_BACK           0x0E    // \b
#define SCANCODE_ENTER          0x1C    // \n
#define SCANCODE_PADENTER       0x60    // \n
#define SCANCODE_SPACE          0x39
#define SCANCODE_GACC           0x29    // "`"
#define SCANCODE_1              0x02
#define SCANCODE_2              0x03
#define SCANCODE_3              0x04
#define SCANCODE_4              0x05
#define SCANCODE_5              0x06
#define SCANCODE_6              0x07
#define SCANCODE_7              0x08
#define SCANCODE_8              0x09
#define SCANCODE_9              0x0A
#define SCANCODE_0              0x0B
#define SCANCODE_SUB            0x0C
#define SCANCODE_EQU            0x0D
#define SCANCODE_TAB            0x0F
#define SCANCODE_Q              0x10
#define SCANCODE_W              0x11
#define SCANCODE_E              0x12
#define SCANCODE_R              0x13
#define SCANCODE_T              0x14
#define SCANCODE_Y              0x15
#define SCANCODE_U              0x16
#define SCANCODE_I              0x17
#define SCANCODE_O              0x18
#define SCANCODE_P              0x19
#define SCANCODE_LSQU           0x1A    // "["
#define SCANCODE_RSQU           0x1B    // "]"
#define SCANCODE_BSLASH         0x2B    // "\"
#define SCANCODE_A              0x1E
#define SCANCODE_S              0x1F
#define SCANCODE_D              0x20
#define SCANCODE_F              0x21
#define SCANCODE_G              0x22
#define SCANCODE_H              0x23
#define SCANCODE_J              0x24
#define SCANCODE_K              0x25
#define SCANCODE_L              0x26
#define SCANCODE_SEMI           0x27        // ";"
#define SCANCODE_APOS           0x28        // "'"
#define SCANCODE_Z              0x2C
#define SCANCODE_X              0x2D
#define SCANCODE_C              0x2E
#define SCANCODE_V              0x2F
#define SCANCODE_B              0x30
#define SCANCODE_N              0x31
#define SCANCODE_M              0x32
#define SCANCODE_COMMA          0x33
#define SCANCODE_POINT          0x34
#define SCANCODE_SLASH          0x35    // "/"
#define SCANCODE_PAD0           0x52
#define SCANCODE_PAD1           0x4F
#define SCANCODE_PAD2           0x50
#define SCANCODE_PAD3           0x51
#define SCANCODE_PAD4           0x4B
#define SCANCODE_PAD5           0x4C
#define SCANCODE_PAD6           0x4D
#define SCANCODE_PAD7           0x47
#define SCANCODE_PAD8           0x48
#define SCANCODE_PAD9           0x49
#define SCANCODE_PADPOINT       0x53
#define SCANCODE_PADPLUS        0x4E
#define SCANCODE_PADSUB         0x4A
#define SCANCODE_PADMUL         0x37
#define SCANCODE_PADDIV         0xE035

UCHAR ScancodeToAscii_NonShift(ULONG Scancode);
UCHAR ScancodeToAscii_Shift(ULONG Scancode);

#endif
