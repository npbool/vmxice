#include "define.h"
#include "vmx.h"

#ifndef _APIC_H_
#define _APIC_H_

VOID MaskKeyboardInterrupt(PGUEST_CPU pCpu);
VOID RestoreKeyboardInterrupt(PGUEST_CPU pCpu);

#endif
