#include <linux/module.h>
#include "apic.h"

VOID MaskKeyboardInterrupt(PGUEST_CPU pCpu)
{
    _WritePRT(pCpu->IOAPIC_va,1*2,_ReadPRT(pCpu->IOAPIC_va,1*2) | 0x10000);
}

VOID RestoreKeyboardInterrupt(PGUEST_CPU pCpu)
{
    _WritePRT(pCpu->IOAPIC_va,1*2,_ReadPRT(pCpu->IOAPIC_va,1*2) & ~0x10000);
}
