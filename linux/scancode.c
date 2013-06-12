#include <linux/module.h>
#include "scancode.h"

UCHAR ScancodeToAscii_NonShift(ULONG Scancode)
{
    switch(Scancode)
    {
    case SCANCODE_BACK:     return '\b'; //0x0E    // \b
    case SCANCODE_ENTER:    return '\n'; //0x1C    // \n
    case SCANCODE_PADENTER: return '\n'; //0x60    // \n
    case SCANCODE_SPACE:    return ' '; //0x39
    case SCANCODE_GACC:     return '`'; //0x58    // "`"
    case SCANCODE_1:        return '1'; //0x02
    case SCANCODE_2:        return '2'; //0x03
    case SCANCODE_3:        return '3'; //0x04
    case SCANCODE_4:        return '4'; //0x05
    case SCANCODE_5:        return '5'; //0x06
    case SCANCODE_6:        return '6'; //0x07
    case SCANCODE_7:        return '7'; //0x08
    case SCANCODE_8:        return '8'; //0x09
    case SCANCODE_9:        return '9'; //0x0A
    case SCANCODE_0:        return '0'; //0x0B
    case SCANCODE_SUB:      return '-'; //0x0C
    case SCANCODE_EQU:      return '='; //0x0D
    case SCANCODE_TAB:      return ' '; //0x0F
    case SCANCODE_Q:        return 'q'; //0x10
    case SCANCODE_W:        return 'w'; //0x11
    case SCANCODE_E:        return 'e'; //0x12
    case SCANCODE_R:        return 'r'; //0x13
    case SCANCODE_T:        return 't'; //0x14
    case SCANCODE_Y:        return 'y'; //0x15
    case SCANCODE_U:        return 'u'; //0x16
    case SCANCODE_I:        return 'i'; //0x17
    case SCANCODE_O:        return 'o'; //0x18
    case SCANCODE_P:        return 'p'; //0x19
    case SCANCODE_LSQU:     return '['; //0x1A    // "["
    case SCANCODE_RSQU:     return ']'; //0x1B    // "]"
    case SCANCODE_BSLASH:   return '\\'; //0x2B    // "\"
    case SCANCODE_A:        return 'a'; //0x1E
    case SCANCODE_S:        return 's'; //0x1F
    case SCANCODE_D:        return 'd'; //0x20
    case SCANCODE_F:        return 'f'; //0x21
    case SCANCODE_G:        return 'g'; //0x22
    case SCANCODE_H:        return 'h'; //0x23
    case SCANCODE_J:        return 'j'; //0x24
    case SCANCODE_K:        return 'k'; //0x25
    case SCANCODE_L:        return 'l'; //0x26
    case SCANCODE_SEMI:     return ';'; //0x27        // ";"
    case SCANCODE_APOS:     return '\''; //0x28        // "'"
    case SCANCODE_Z:        return 'z'; //0x2C
    case SCANCODE_X:        return 'x'; //0x2D
    case SCANCODE_C:        return 'c'; //0x2E
    case SCANCODE_V:        return 'v'; //0x2F
    case SCANCODE_B:        return 'b'; //0x30
    case SCANCODE_N:        return 'n'; //0x31
    case SCANCODE_M:        return 'm'; //0x32
    case SCANCODE_COMMA:    return ','; //0x33
    case SCANCODE_POINT:    return '.'; //0x34
    case SCANCODE_SLASH:    return '/'; //0x35    // "/"
    case SCANCODE_PAD0:     return '0'; //0x52
    case SCANCODE_PAD1:     return '1'; //0x4F
    case SCANCODE_PAD2:     return '2'; //0x50
    case SCANCODE_PAD3:     return '3'; //0x51
    case SCANCODE_PAD4:     return '4'; //0x4B
    case SCANCODE_PAD5:     return '5'; //0x4C
    case SCANCODE_PAD6:     return '6'; //0x4D
    case SCANCODE_PAD7:     return '7'; //0x47
    case SCANCODE_PAD8:     return '8'; //0x48
    case SCANCODE_PAD9:     return '9'; //0x49
    case SCANCODE_PADPOINT: return '.'; //0x53
    case SCANCODE_PADPLUS:  return '+'; //0xE035
    case SCANCODE_PADSUB:   return '-'; //0x4A
    case SCANCODE_PADMUL:   return '*'; //0x37
    case SCANCODE_PADDIV:   return '/'; //0x4E
    default:
        return 0;
    }
}

UCHAR ScancodeToAscii_Shift(ULONG Scancode)
{
    switch(Scancode)
    {
    case SCANCODE_BACK:     return '\b'; //0x0E    // \b
    case SCANCODE_ENTER:    return '\n'; //0x1C    // \n
    case SCANCODE_PADENTER: return '\n'; //0x60    // \n
    case SCANCODE_SPACE:    return ' '; //0x39
    case SCANCODE_GACC:     return '~'; //0x58    // "`"
    case SCANCODE_1:        return '!'; //0x02
    case SCANCODE_2:        return '@'; //0x03
    case SCANCODE_3:        return '#'; //0x04
    case SCANCODE_4:        return '$'; //0x05
    case SCANCODE_5:        return '%'; //0x06
    case SCANCODE_6:        return '^'; //0x07
    case SCANCODE_7:        return '&'; //0x08
    case SCANCODE_8:        return '*'; //0x09
    case SCANCODE_9:        return '('; //0x0A
    case SCANCODE_0:        return ')'; //0x0B
    case SCANCODE_SUB:      return '_'; //0x0C
    case SCANCODE_EQU:      return '+'; //0x0D
    case SCANCODE_TAB:      return ' '; //0x0F
    case SCANCODE_Q:        return 'Q'; //0x10
    case SCANCODE_W:        return 'W'; //0x11
    case SCANCODE_E:        return 'E'; //0x12
    case SCANCODE_R:        return 'R'; //0x13
    case SCANCODE_T:        return 'T'; //0x14
    case SCANCODE_Y:        return 'Y'; //0x15
    case SCANCODE_U:        return 'U'; //0x16
    case SCANCODE_I:        return 'I'; //0x17
    case SCANCODE_O:        return 'O'; //0x18
    case SCANCODE_P:        return 'P'; //0x19
    case SCANCODE_LSQU:     return '{'; //0x1A    // "["
    case SCANCODE_RSQU:     return '}'; //0x1B    // "]"
    case SCANCODE_BSLASH:   return '|'; //0x2B    // "\"
    case SCANCODE_A:        return 'A'; //0x1E
    case SCANCODE_S:        return 'S'; //0x1F
    case SCANCODE_D:        return 'D'; //0x20
    case SCANCODE_F:        return 'F'; //0x21
    case SCANCODE_G:        return 'G'; //0x22
    case SCANCODE_H:        return 'H'; //0x23
    case SCANCODE_J:        return 'J'; //0x24
    case SCANCODE_K:        return 'K'; //0x25
    case SCANCODE_L:        return 'L'; //0x26
    case SCANCODE_SEMI:     return ':'; //0x27        // ";"
    case SCANCODE_APOS:     return '"'; //0x28        // "'"
    case SCANCODE_Z:        return 'Z'; //0x2C
    case SCANCODE_X:        return 'X'; //0x2D
    case SCANCODE_C:        return 'C'; //0x2E
    case SCANCODE_V:        return 'V'; //0x2F
    case SCANCODE_B:        return 'B'; //0x30
    case SCANCODE_N:        return 'N'; //0x31
    case SCANCODE_M:        return 'M'; //0x32
    case SCANCODE_COMMA:    return '<'; //0x33
    case SCANCODE_POINT:    return '>'; //0x34
    case SCANCODE_SLASH:    return '?'; //0x35    // "/"
    case SCANCODE_PAD0:     return '0'; //0x52
    case SCANCODE_PAD1:     return '1'; //0x4F
    case SCANCODE_PAD2:     return '2'; //0x50
    case SCANCODE_PAD3:     return '3'; //0x51
    case SCANCODE_PAD4:     return '4'; //0x4B
    case SCANCODE_PAD5:     return '5'; //0x4C
    case SCANCODE_PAD6:     return '6'; //0x4D
    case SCANCODE_PAD7:     return '7'; //0x47
    case SCANCODE_PAD8:     return '8'; //0x48
    case SCANCODE_PAD9:     return '9'; //0x49
    case SCANCODE_PADPOINT: return '.'; //0x53
    case SCANCODE_PADPLUS:  return '+'; //0xE035
    case SCANCODE_PADSUB:   return '-'; //0x4A
    case SCANCODE_PADMUL:   return '*'; //0x37
    case SCANCODE_PADDIV:   return '/'; //0x4E
    default:
        return 0;
    }
}
