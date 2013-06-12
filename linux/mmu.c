#include <linux/kernel.h>
#include <linux/io.h>

#include "mmu.h"
#include "mmu.h"
#include "vmx.h"

ULONG GetPhysAddress(ULONG CR3,ULONG VirtAddress,PULONG PhysAddress)
{
    ULONG PdeBaseVirs;
    ULONG Pde;
    PPDE pPde = (PPDE)&Pde;
    ULONG PteBaseVirt;
    ULONG Pte;

    *PhysAddress = 0;
    PdeBaseVirs = (ULONG)phys_to_virt(CR3);
    Pde = *(PULONG)(PdeBaseVirs + ((VirtAddress >> 22) << 2));
    if(!(Pde & 1))
        return 0;

    if(pPde->LargePage)
    {
        *PhysAddress = (Pde & 0xffc00000) + (VirtAddress & 0x3fffff);
        return 1;
    }

    if(Pde > 896*1024*1024)    //Why PTE table may appear VMALLOC region..fuck..
    {
        //printk("Can't directly map Pde: %x\n",Pde);
        return 0;
    }

    PteBaseVirt = (ULONG)phys_to_virt(Pde & 0xfffff000);
    Pte = *(PULONG)(PteBaseVirt + (((VirtAddress >> 12) & 0x3ff) << 2));
    if(!(Pte & 1))
        return 0;

    *PhysAddress = (Pte & 0xfffff000) + (VirtAddress & 0xfff);
    return 1;
}

ULONG IsAddressExist(ULONG VirtAddress)
{
    ULONG PhysAddress;

    return GetPhysAddress(_CR3(),VirtAddress,&PhysAddress);
}

ULONG IsAddressRangeExist(ULONG VirtAddress,ULONG Len)
{
    ULONG VirtAddress2 = VirtAddress + Len;
    ULONG PhysAddress;
    ULONG Ret1,Ret2;

    Ret1 = GetPhysAddress(_CR3(),VirtAddress,&PhysAddress);
    if(!Ret1)
        return 0;

    Ret2 = GetPhysAddress(_CR3(),VirtAddress2,&PhysAddress);
    if(!Ret2)
        return 0;

    return 1;
}


VOID GetVirtAddress(ULONG CR3,ULONG PhysAddress)
{
    ULONG PhysBaseRet;
    ULONG PhysBase = PhysAddress & 0xfffff000;
    ULONG PageOffset = PhysAddress & 0xfff;
    ULONG VirtBase;
    ULONG VirtAddress;
    ULONG Ret;

    for(VirtBase = 0x1000; VirtBase < 0xfffff000; VirtBase += 0x1000)
    {
        Ret = GetPhysAddress(CR3,VirtBase,&PhysBaseRet);
        if(Ret && PhysBaseRet == PhysBase)
        {
            VirtAddress = VirtBase + PageOffset;
            printk("GetVirtAddress found %x [%x]\n",VirtAddress,*(PULONG)VirtAddress);
        }
    }
}

ULONG GetPde(ULONG CR3,ULONG VirtAddress)
{
    ULONG PdeBaseVirs;
    ULONG Pde;

    PdeBaseVirs = (ULONG)phys_to_virt(CR3);
    Pde = *(PULONG)(PdeBaseVirs + ((VirtAddress >> 22) << 2));
    if(!(Pde & 1))
        return 0;

    if(Pde > 896*1024*1024)    //Why PTE table may appear VMALLOC region..fuck..
    {
        //printk("Can't directly map Pde: %x\n",Pde);
        return 0;
    }

    return Pde;
}

ULONG GetPte(ULONG CR3,ULONG VirtAddress)
{
    ULONG PdeBaseVirs;
    ULONG Pde;
    PPDE pPde = (PPDE)&Pde;
    ULONG PteBaseVirt;
    ULONG Pte;

    PdeBaseVirs = (ULONG)phys_to_virt(CR3);
    Pde = *(PULONG)(PdeBaseVirs + ((VirtAddress >> 22) << 2));
    if(!(Pde & 1))
        return 0;

    if(pPde->LargePage)
    {
        return 0;
    }

    if(Pde > 896*1024*1024)    //Why PTE table may appear VMALLOC region..fuck..
    {
        //printk("Can't directly map Pde: %x\n",Pde);
        return 0;
    }

    PteBaseVirt = (ULONG)phys_to_virt(Pde & 0xfffff000);
    Pte = *(PULONG)(PteBaseVirt + (((VirtAddress >> 12) & 0x3ff) << 2));
    return Pte;
}
