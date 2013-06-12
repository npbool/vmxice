#include <linux/module.h>
#include <linux/io.h>
#include <linux/slab.h>
#include "vmx.h"
#include "vmm.h"
#include "x86.h"
#include "video.h"
#include "dbgprint.h"

#define KMALLOC_FLAGS GFP_ATOMIC

PHYSICAL_ADDRESS HostPDE_pa;
PVOID HostPDE_va;
PVOID GuestPDE_va;

VOID HostCr3Init(void)
{
    GuestPDE_va = phys_to_virt(_CR3());
    HostPDE_va = kmalloc(0x1000,KMALLOC_FLAGS);
    HostPDE_pa.QuadPart = virt_to_phys(HostPDE_va);
    memcpy(HostPDE_va,GuestPDE_va,0x1000);
    printf("Guest CR3: 0x%p\r\n",(PVOID)_CR3());
    printf("Host CR3: 0x%p\r\n",(PVOID)HostPDE_pa.LowPart);
}

BOOLEAN IsBitSet(ULONG64 v, UCHAR bitNo)
{
    ULONG64 mask = (ULONG64) 1 << bitNo;
    return (BOOLEAN) ((v & mask) != 0);
}

BOOLEAN CheckIfVMXIsEnabled(void)
{
    ULONG64 msr;

    msr = _ReadMSR(MSR_IA32_FEATURE_CONTROL);
    if (!(msr & 4))
    {
        return false;
    }
    return true;
}

BOOLEAN CheckForVirtualizationSupport(void)
{
    ULONG eax, ebx, ecx, edx;

    eax = 0;
    _CpuId(&eax, &ebx, &ecx, &edx);
    if (eax < 1)
    {
        return false;
    }

    /* Intel Genuine */
    if (!(ebx == 0x756e6547 && ecx == 0x6c65746e && edx == 0x49656e69))
    {
        return false;
    }

    eax = 1;
    _CpuId(&eax, &ebx, &ecx, &edx);
    if (!IsBitSet(ecx, 5))
    {
        return false;
    }
    return true;
}

BOOLEAN SetupVMX(PGUEST_CPU pCpu)
{
    PHYSICAL_ADDRESS pa;
    ULONG64 msr;
    PVMX_BASIC_MSR pvmx;
    PVOID va;
    ULONG size;
    //ULONG i;
    ULONG cr4;
    ULONG Eflags;

    DEFAULT_FONT;

    /* vmxon supported ? */
    _SetCR4(_CR4() | X86_CR4_VMXE);
    cr4 = _CR4();

    if (!(cr4 & X86_CR4_VMXE))
    {
        RED_FONT;
        printf("error: VMXON not supported\r\n");
        return false;
    }

    //i = KeGetCurrentProcessorNumber();
    //pCpu->ProcessorNumber = i;

    msr = _ReadMSR(MSR_IA32_VMX_BASIC);
    pvmx = (PVMX_BASIC_MSR)&msr;
    size = pvmx->szVmxOnRegion;
    printf("VMXON region size: 0x%x\r\n", size);
    printf("VMX revision ID: 0x%x\r\n", pvmx->RevId);

    va = kmalloc(0x1000,KMALLOC_FLAGS);
    if (va == NULL)
    {
        RED_FONT;
        printf("error: can't allocate vmxon region\r\n");
        return false;
    }
    memset(va,0,0x1000);
    *(ULONG *)va = pvmx->RevId;
    pa.QuadPart = virt_to_phys(va);
    pCpu->VMXON_pa = pa;
    pCpu->VMXON_va = va;

    Eflags = _VmxOn(pa.LowPart,pa.HighPart);
    if (_VmFailInvalid(Eflags))
    {
        RED_FONT;
        printf("_VmxOn failed\r\n");
        return false;
    }

    va = kmalloc(0x1000,KMALLOC_FLAGS);
    if (va == NULL)
    {
        RED_FONT;
        printf("error: can't allocate vmcs region\r\n");
        return false;
    }
    memset(va,0,0x1000);
    *(ULONG *)va = pvmx->RevId;
    pa.QuadPart = virt_to_phys(va);
    pCpu->VMCS_pa = pa;
    pCpu->VMCS_va = va;

    va = kmalloc(0x1000,KMALLOC_FLAGS);
    if (va == NULL)
    {
        RED_FONT;
        printf("error: can't allocate io bitmap a\r\n");
        return false;
    }
    memset(va,0,0x1000);
    pa.QuadPart = virt_to_phys(va);
    pCpu->IO_Bitmap_A_pa = pa;
    pCpu->IO_Bitmap_A_va = va;

    va = kmalloc(0x1000,KMALLOC_FLAGS);
    if (va == NULL)
    {
        RED_FONT;
        printf("error: can't allocate io bitmap b\r\n");
        return false;
    }
    memset(va,0,0x1000);
    pa.QuadPart = virt_to_phys(va);
    pCpu->IO_Bitmap_B_pa = pa;
    pCpu->IO_Bitmap_B_va = va;

    va = kmalloc(0x1000,KMALLOC_FLAGS);
    if (va == NULL)
    {
        RED_FONT;
        printf("error: can't allocate msr bitmap\r\n");
        return false;
    }
    memset(va,0,0x1000);
    pa.QuadPart = virt_to_phys(va);
    pCpu->MSR_bitmap_pa = pa;
    pCpu->MSR_bitmap_va = va;

    va = kmalloc(0x8000,KMALLOC_FLAGS);
    if (va == NULL)
    {
        RED_FONT;
        printf("error: can't allocate stack for host\r\n");
        return false;
    }
    memset(va,0,0x8000);
    pCpu->Host_Stack_va = va;

    va = kmalloc(0x1000,KMALLOC_FLAGS);
    if (va == NULL)
    {
        RED_FONT;
        printf("error: can't allocate IDT for host\r\n");
        return false;
    }
    memset(va,0,0x1000);
    pCpu->VMMIDT = va;

    pa.LowPart = 0xFEE00000;
    pa.HighPart = 0;
    va = ioremap_nocache(pa.QuadPart,256);
    if (va == NULL)
    {
        RED_FONT;
        printf("error: can't map LocalAPIC\r\n");
        return false;
    }
    pCpu->LocalAPIC_va = va;

    pa.LowPart = 0xFEC00000;
    pa.HighPart = 0;
    va = ioremap_nocache(pa.QuadPart,256);
    if (va == NULL)
    {
        RED_FONT;
        printf("error: can't map IOAPIC\r\n");
        return false;
    }
    pCpu->IOAPIC_va = va;

    printf("LocalAPIC map to 0x%p\r\n",pCpu->LocalAPIC_va);
    printf("IOAPIC map to 0x%p\r\n",pCpu->IOAPIC_va);
    return true;
}

BOOLEAN InitializeSegmentSelector(PSEGMENT_SELECTOR SegmentSelector,USHORT Selector,ULONG GdtBase)
{
    PSEGMENT_DESCRIPTOR2 SegDesc;

    if (!SegmentSelector)
        return false;

    if (Selector & 0x4)
    {
        RED_FONT;
        printf("WARNING: InitializeSegmentSelector(): Given selector(%d) points to LDT\r\n", Selector);
        return false;
    }

    SegDesc = (PSEGMENT_DESCRIPTOR2) ((PUCHAR) GdtBase + (Selector & ~0x7));

    SegmentSelector->sel = Selector;
    SegmentSelector->base = SegDesc->base0 | SegDesc->base1 << 16 | SegDesc->base2 << 24;
    SegmentSelector->limit = SegDesc->limit0 | (SegDesc->limit1attr1 & 0xf) << 16;
    SegmentSelector->attributes.UCHARs = SegDesc->attr0 | (SegDesc->limit1attr1 & 0xf0) << 4;

    if (!(SegDesc->attr0 & LA_STANDARD))
    {
        ULONG64 tmp;
        // this is a TSS or callgate etc, save the base high part
        tmp = (*(PULONG64) ((PUCHAR) SegDesc + 8));
        SegmentSelector->base = (SegmentSelector->base & 0xffffffff) | (tmp << 32);
    }

    if (SegmentSelector->attributes.fields.g)
    {
        // 4096-bit granularity is enabled for this segment, scale the limit
        SegmentSelector->limit = (SegmentSelector->limit << 12) + 0xfff;
    }

    return true;
}

ULONG GetSegmentDescriptorBase(ULONG gdt_base, USHORT seg_selector)
{
    ULONG base = 0;
    SEGMENT_DESCRIPTOR segDescriptor = {0};

    memcpy(&segDescriptor, (ULONG *)(gdt_base + (seg_selector >> 3) * 8), 8 );
    base = segDescriptor.BaseHi;
    base <<= 8;
    base |= segDescriptor.BaseMid;
    base <<= 16;
    base |= segDescriptor.BaseLo;

    return base;
}

ULONG GetSegmentDescriptorDPL(ULONG gdt_base, USHORT seg_selector)
{
    SEGMENT_DESCRIPTOR segDescriptor = {0};

    memcpy(&segDescriptor, (ULONG *)(gdt_base + (seg_selector >> 3) * 8), 8);

    return segDescriptor.DPL;
}

ULONG GetSegmentDescriptorLimit(ULONG gdt_base, USHORT selector)
{
    SEGMENT_SELECTOR SegmentSelector = { 0 };

    InitializeSegmentSelector(&SegmentSelector, selector, gdt_base);

    return SegmentSelector.limit;
}

ULONG GetSegmentDescriptorAR(ULONG gdt_base, USHORT selector)
{
    SEGMENT_SELECTOR SegmentSelector = { 0 };
    ULONG uAccessRights;

    InitializeSegmentSelector(&SegmentSelector, selector, gdt_base);

    uAccessRights = ((PUCHAR) & SegmentSelector.attributes)[0] + (((PUCHAR) & SegmentSelector.attributes)[1] << 12);

    if (!selector)
        uAccessRights |= 0x10000;

    return uAccessRights;
}

ULONG VmxAdjustControls(ULONG Ctl, ULONG Msr)
{
    LARGE_INTEGER MsrValue;

    MsrValue.QuadPart = _ReadMSR(Msr);
    Ctl &= MsrValue.HighPart;     /* bit == 0 in high word ==> must be zero */
    Ctl |= MsrValue.LowPart;      /* bit == 1 in low word  ==> must be one  */
    return Ctl;
}

VOID IntHandler(void)
{

}

VOID PageFaultHandler(void)
{

}

VOID InitVMMIDT(PIDT_ENTRY pidt)
{
    ULONG i;
    IDT_ENTRY idte_null;
    //PIDT_ENTRY pGuestIdt = (PIDT_ENTRY)_IdtBase();

    memset(&idte_null,0,sizeof(idte_null));
    idte_null.Selector   = _CS();

    /* Present, DPL 0, Type 0xe (INT gate) */
    idte_null.P = 1;
    idte_null.GateSize = 1;
    idte_null.GateType = 6;

    idte_null.LowOffset  = (ULONG)_BeepOn & 0xffff;
    idte_null.HighOffset = (ULONG)_BeepOn >> 16;
    for (i=0; i<256; i++)
        pidt[i] = idte_null;

    //#PF异常自动重启
    idte_null.LowOffset  = (ULONG)_Reboot & 0xffff;
    idte_null.HighOffset = (ULONG)_Reboot >> 16;
    pidt[14] = idte_null;
}

BOOLEAN SetupVMCS(PGUEST_CPU pCpu, PVOID GuestEsp)
{
    ULONG ExceptionBitmap;
    ULONG GdtBase;
    PHYSICAL_ADDRESS pa;
    ULONG64 msr;
    ULONG ioPort;
//    ULONG msrIndex;
    ULONG Eflags;

    DEFAULT_FONT;

    pa = pCpu->VMCS_pa;
    Eflags = _VmClear(pa.LowPart,pa.HighPart);
    if(_VmFailInvalid(Eflags))
    {
        RED_FONT;
        printf("VmClear failed.\r\n");
        return false;
    }

    Eflags = _VmPtrLd(pa.LowPart,pa.HighPart);
    if(_VmFailInvalid(Eflags))
    {
        RED_FONT;
        printf("VmPtrLd failed.\r\n");
        return false;
    }

    //
    // ***********************************
    // * H.1.1 16-Bit Guest-State Fields *
    // ***********************************
    _WriteVMCS(GUEST_CS_SELECTOR, _CS() & 0xfff8);
    _WriteVMCS(GUEST_SS_SELECTOR, _SS() & 0xfff8);
    _WriteVMCS(GUEST_DS_SELECTOR, _DS() & 0xfff8);
    _WriteVMCS(GUEST_ES_SELECTOR, _ES() & 0xfff8);
    _WriteVMCS(GUEST_FS_SELECTOR, _FS() & 0xfff8);
    _WriteVMCS(GUEST_GS_SELECTOR, _GS() & 0xfff8);
    _WriteVMCS(GUEST_LDTR_SELECTOR, _Ldtr() & 0xfff8);
    _WriteVMCS(GUEST_TR_SELECTOR, _TrSelector() & 0xfff8);

    //  **********************************
    //  *    H.1.2 16-Bit Host-State Fields *
    //  **********************************

    /* FIXME: Host DS, ES & FS mascherati con 0xFFFC? */
    _WriteVMCS(HOST_CS_SELECTOR, _CS() & 0xfff8);
    _WriteVMCS(HOST_SS_SELECTOR, _SS() & 0xfff8);
    _WriteVMCS(HOST_DS_SELECTOR, _DS() & 0xfff8);
    _WriteVMCS(HOST_ES_SELECTOR, _ES() & 0xfff8);
    _WriteVMCS(HOST_FS_SELECTOR, _FS() & 0xfff8);
    _WriteVMCS(HOST_GS_SELECTOR, _GS() & 0xfff8);
    _WriteVMCS(HOST_TR_SELECTOR, _TrSelector() & 0xfff8);

    //  ***********************************
    //  *    H.2.2 64-Bit Guest-State Fields *
    //  ***********************************

    _WriteVMCS(VMCS_LINK_POINTER, 0xFFFFFFFF);
    _WriteVMCS(VMCS_LINK_POINTER_HIGH, 0xFFFFFFFF);


    /* Reserved Bits of IA32_DEBUGCTL MSR must be 0 */
    msr = _ReadMSR(MSR_IA32_DEBUGCTL);
    _WriteVMCS(GUEST_IA32_DEBUGCTL, (ULONG)(msr & 0xFFFFFFFF));
    _WriteVMCS(GUEST_IA32_DEBUGCTL_HIGH, (ULONG)(msr >> 32));

    //    *******************************
    //    * H.3.1 32-Bit Control Fields *
    //    *******************************

    /* Pin-based VM-execution controls */
    _WriteVMCS(PIN_BASED_VM_EXEC_CONTROL, VmxAdjustControls(0, MSR_IA32_VMX_PINBASED_CTLS));
    _WriteVMCS(CPU_BASED_VM_EXEC_CONTROL, VmxAdjustControls(CPU_BASED_ACTIVATE_MSR_BITMAP + CPU_BASED_ACTIVATE_IO_BITMAP,MSR_IA32_VMX_PROCBASED_CTLS));

    /* I/O bitmap */
    _WriteVMCS(IO_BITMAP_A_HIGH, pCpu->IO_Bitmap_A_pa.HighPart);
    _WriteVMCS(IO_BITMAP_A,      pCpu->IO_Bitmap_A_pa.LowPart);
    _WriteVMCS(IO_BITMAP_B_HIGH, pCpu->IO_Bitmap_B_pa.HighPart);
    _WriteVMCS(IO_BITMAP_B,      pCpu->IO_Bitmap_B_pa.LowPart);

    ioPort = 0x60;
    ((PUCHAR)(pCpu->IO_Bitmap_A_va))[ioPort / 8] = 1<<(ioPort%8);

    _WriteVMCS(MSR_BITMAP, pCpu->MSR_bitmap_pa.LowPart);
    _WriteVMCS(MSR_BITMAP_HIGH, pCpu->MSR_bitmap_pa.HighPart);

    //msrIndex = MSR_IA32_FEATURE_CONTROL;
    //((PUCHAR)(pCpu->MSR_bitmap_va))[msrIndex / 8] = 1<<(msrIndex%8);


    /* Exception bitmap */
    ExceptionBitmap = 0;
    ExceptionBitmap |= 1<<DEBUG_EXCEPTION;
    ExceptionBitmap |= 1<<BREAKPOINT_EXCEPTION;
    //ExceptionBitmap |= 1<<PAGE_FAULT_EXCEPTION;
    _WriteVMCS(EXCEPTION_BITMAP, ExceptionBitmap);

    /* Time-stamp counter offset */
    _WriteVMCS(TSC_OFFSET, 0);
    _WriteVMCS(TSC_OFFSET_HIGH, 0);

    _WriteVMCS(PAGE_FAULT_ERROR_CODE_MASK, 0);
    _WriteVMCS(PAGE_FAULT_ERROR_CODE_MATCH, 0);
    _WriteVMCS(CR3_TARGET_COUNT, 0);
    _WriteVMCS(CR3_TARGET_VALUE0, 0);
    _WriteVMCS(CR3_TARGET_VALUE1, 0);
    _WriteVMCS(CR3_TARGET_VALUE2, 0);
    _WriteVMCS(CR3_TARGET_VALUE3, 0);

    /* VM-exit controls */
    _WriteVMCS(VM_EXIT_CONTROLS, VmxAdjustControls(VM_EXIT_ACK_INTR_ON_EXIT, MSR_IA32_VMX_EXIT_CTLS));

    /* VM-entry controls */
    _WriteVMCS(VM_ENTRY_CONTROLS, VmxAdjustControls(0, MSR_IA32_VMX_ENTRY_CTLS));

    _WriteVMCS(VM_EXIT_MSR_STORE_COUNT, 0);
    _WriteVMCS(VM_EXIT_MSR_LOAD_COUNT, 0);

    _WriteVMCS(VM_ENTRY_MSR_LOAD_COUNT, 0);
    _WriteVMCS(VM_ENTRY_INTR_INFO_FIELD, 0);



    //  ***********************************
    //  *    H.3.3 32-Bit Guest-State Fields *
    //  ***********************************
    GdtBase = _GdtBase();
    _WriteVMCS(GUEST_CS_LIMIT, GetSegmentDescriptorLimit(GdtBase, _CS()));
    _WriteVMCS(GUEST_SS_LIMIT, GetSegmentDescriptorLimit(GdtBase, _SS()));
    _WriteVMCS(GUEST_DS_LIMIT, GetSegmentDescriptorLimit(GdtBase, _DS()));
    _WriteVMCS(GUEST_ES_LIMIT, GetSegmentDescriptorLimit(GdtBase, _ES()));
    _WriteVMCS(GUEST_FS_LIMIT, GetSegmentDescriptorLimit(GdtBase, _FS()));
    _WriteVMCS(GUEST_GS_LIMIT, GetSegmentDescriptorLimit(GdtBase, _GS()));
    _WriteVMCS(GUEST_LDTR_LIMIT, GetSegmentDescriptorLimit(GdtBase, _Ldtr()));
    _WriteVMCS(GUEST_TR_LIMIT, GetSegmentDescriptorLimit(GdtBase, _TrSelector()));

    /* Guest GDTR/IDTR limit */
    _WriteVMCS(GUEST_GDTR_LIMIT, _GdtLimit());
    _WriteVMCS(GUEST_IDTR_LIMIT, _IdtLimit());

    /* DR7 */
    _WriteVMCS(GUEST_DR7, 0x400);

    /* Guest interruptibility and activity state */
    _WriteVMCS(GUEST_INTERRUPTIBILITY_INFO, 0);
    _WriteVMCS(GUEST_ACTIVITY_STATE, 0);

    /* Set segment access rights */
    _WriteVMCS(GUEST_CS_AR_BYTES,   GetSegmentDescriptorAR(GdtBase, _CS()));
    _WriteVMCS(GUEST_DS_AR_BYTES,   GetSegmentDescriptorAR(GdtBase, _DS()));
    _WriteVMCS(GUEST_SS_AR_BYTES,   GetSegmentDescriptorAR(GdtBase, _SS()));
    _WriteVMCS(GUEST_ES_AR_BYTES,   GetSegmentDescriptorAR(GdtBase, _ES()));
    _WriteVMCS(GUEST_FS_AR_BYTES,   GetSegmentDescriptorAR(GdtBase, _FS()));
    _WriteVMCS(GUEST_GS_AR_BYTES,   GetSegmentDescriptorAR(GdtBase, _GS()));
    _WriteVMCS(GUEST_LDTR_AR_BYTES, GetSegmentDescriptorAR(GdtBase, _Ldtr()));
    _WriteVMCS(GUEST_TR_AR_BYTES,   GetSegmentDescriptorAR(GdtBase, _TrSelector()));

    /* Guest IA32_SYSENTER_CS */
    msr = _ReadMSR(MSR_IA32_SYSENTER_CS);
    _WriteVMCS(GUEST_SYSENTER_CS, (ULONG)(msr & 0xFFFFFFFF));


    //  ******************************************
    //  * H.4.3 Natural-Width Guest-State Fields *
    //  ******************************************

    /* Guest CR0 */
    _WriteVMCS(GUEST_CR0, _CR0());

    /* Guest CR3 */
    _WriteVMCS(GUEST_CR3, _CR3());

    _WriteVMCS(GUEST_CR4, _CR4());
    _WriteVMCS(CR4_GUEST_HOST_MASK, X86_CR4_VMXE);

    /* Guest segment base addresses */
    _WriteVMCS(GUEST_CS_BASE,   GetSegmentDescriptorBase(GdtBase, _CS()));
    _WriteVMCS(GUEST_SS_BASE,   GetSegmentDescriptorBase(GdtBase, _SS()));
    _WriteVMCS(GUEST_DS_BASE,   GetSegmentDescriptorBase(GdtBase, _DS()));
    _WriteVMCS(GUEST_ES_BASE,   GetSegmentDescriptorBase(GdtBase, _ES()));
    _WriteVMCS(GUEST_FS_BASE,   GetSegmentDescriptorBase(GdtBase, _FS()));
    _WriteVMCS(GUEST_GS_BASE,   GetSegmentDescriptorBase(GdtBase, _GS()));
    _WriteVMCS(GUEST_LDTR_BASE, GetSegmentDescriptorBase(GdtBase, _Ldtr()));
    _WriteVMCS(GUEST_TR_BASE,   GetSegmentDescriptorBase(GdtBase, _TrSelector()));

    /* Guest GDTR/IDTR base */
    _WriteVMCS(GUEST_GDTR_BASE, _GdtBase());
    _WriteVMCS(GUEST_IDTR_BASE, _IdtBase());

    /* Guest RFLAGS */
    _WriteVMCS(GUEST_RFLAGS, _Eflags());

    /* Guest IA32_SYSENTER_ESP */
    msr = _ReadMSR(MSR_IA32_SYSENTER_ESP);
    _WriteVMCS(GUEST_SYSENTER_ESP, (ULONG)(msr & 0xFFFFFFFF));

    /* Guest IA32_SYSENTER_EIP */
    msr = _ReadMSR(MSR_IA32_SYSENTER_EIP);
    _WriteVMCS(GUEST_SYSENTER_EIP, (ULONG)(msr & 0xFFFFFFFF));


    //    *****************************************
    //    * H.4.4 Natural-Width Host-State Fields *
    //    *****************************************

    /* Host CR0, CR3 and CR4 */
    _WriteVMCS(HOST_CR0, _CR0() & ~(0x10000));  //Disable Host Write-Protect
    //_WriteVMCS(HOST_CR3, _CR3());
    HostCr3Init();
    _WriteVMCS(HOST_CR3, HostPDE_pa.LowPart);

    _WriteVMCS(HOST_CR4, _CR4());

    /* Host FS, GS and TR base */
    _WriteVMCS(HOST_FS_BASE, GetSegmentDescriptorBase(GdtBase, _FS()));
    _WriteVMCS(HOST_GS_BASE, GetSegmentDescriptorBase(GdtBase, _GS()));
    _WriteVMCS(HOST_TR_BASE, GetSegmentDescriptorBase(GdtBase, _TrSelector()));


    /* Host GDTR/IDTR base (they both hold *linear* addresses) */
    _WriteVMCS(HOST_GDTR_BASE, _GdtBase());


    printf("VMMIDT: 0x%p\r\n",pCpu->VMMIDT);
    //_WriteVMCS(HOST_IDTR_BASE, _IdtBase());
    _WriteVMCS(HOST_IDTR_BASE, (ULONG)pCpu->VMMIDT);
    InitVMMIDT((PIDT_ENTRY)pCpu->VMMIDT);

    /* Host IA32_SYSENTER_ESP/EIP/CS */
    msr = _ReadMSR(MSR_IA32_SYSENTER_ESP);
    _WriteVMCS(HOST_IA32_SYSENTER_ESP, (ULONG)(msr & 0xFFFFFFFF));

    msr = _ReadMSR(MSR_IA32_SYSENTER_EIP);
    _WriteVMCS(HOST_IA32_SYSENTER_EIP, (ULONG)(msr & 0xFFFFFFFF));

    msr = _ReadMSR(MSR_IA32_SYSENTER_CS);
    _WriteVMCS(HOST_IA32_SYSENTER_CS, (ULONG)(msr & 0xFFFFFFFF));

    /* Clear the VMX Abort Error Code prior to VMLAUNCH */
    memset((PULONG)pCpu->VMCS_va + 1,0,4);

    /* Set EIP, ESP for the Guest right before calling VMLAUNCH */
    printf("Setting Guest ESP 0x%p\r\n", GuestEsp);
    _WriteVMCS(GUEST_RSP, (ULONG) GuestEsp);

    printf("Setting Guest EIP 0x%p\r\n", _GuestEntryPoint);
    _WriteVMCS(GUEST_RIP, (ULONG)_GuestEntryPoint);

    /* Set EIP, ESP for the Host right before calling VMLAUNCH */
    printf("Setting Host ESP 0x%p\r\n", (PVOID)((PUCHAR) pCpu->Host_Stack_va + 0x7000));
    _WriteVMCS(HOST_RSP, (ULONG)((PUCHAR) pCpu->Host_Stack_va + 0x7000));

    memcpy((PUCHAR)pCpu->Host_Stack_va + 0x7000,pCpu,sizeof(GUEST_CPU));
    pCpu = (PGUEST_CPU)((PUCHAR)pCpu->Host_Stack_va + 0x7000);
    pCpu->pGuestRegs = (PGUEST_REGS)((PUCHAR)pCpu - 0x20);

    printf("Setting Host EIP 0x%p\r\n", _VmExitHandler);
    _WriteVMCS(HOST_RIP, (ULONG) _VmExitHandler);

    return true;
}

BOOLEAN Virtualize(PGUEST_CPU pCpu)
{
    //ULONG i;
    ULONG Eflags;

    //i = KeGetCurrentProcessorNumber();

    _Return1();

    Eflags = _VmLaunch();
    if (_VmFailInvalid(Eflags))
    {
        RED_FONT;
        printf("no current VMCS\r\n");
        return false;
    }

    if (_VmFailValid(Eflags))
    {
        RED_FONT;
        printf("vmlaunch failed: 0x%x\r\n", _ReadVMCS(VM_INSTRUCTION_ERROR));
        return false;
    }
    return false;
}
