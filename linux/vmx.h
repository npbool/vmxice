#include "define.h"

#ifndef _VMX_H_
#define _VMX_H_

enum {
    GUEST_ES_SELECTOR = 0x00000800,
    GUEST_CS_SELECTOR = 0x00000802,
    GUEST_SS_SELECTOR = 0x00000804,
    GUEST_DS_SELECTOR = 0x00000806,
    GUEST_FS_SELECTOR = 0x00000808,
    GUEST_GS_SELECTOR = 0x0000080a,
    GUEST_LDTR_SELECTOR = 0x0000080c,
    GUEST_TR_SELECTOR = 0x0000080e,
    HOST_ES_SELECTOR = 0x00000c00,
    HOST_CS_SELECTOR = 0x00000c02,
    HOST_SS_SELECTOR = 0x00000c04,
    HOST_DS_SELECTOR = 0x00000c06,
    HOST_FS_SELECTOR = 0x00000c08,
    HOST_GS_SELECTOR = 0x00000c0a,
    HOST_TR_SELECTOR = 0x00000c0c,
    IO_BITMAP_A = 0x00002000,
    IO_BITMAP_A_HIGH = 0x00002001,
    IO_BITMAP_B = 0x00002002,
    IO_BITMAP_B_HIGH = 0x00002003,
    MSR_BITMAP = 0x00002004,
    MSR_BITMAP_HIGH = 0x00002005,
    VM_EXIT_MSR_STORE_ADDR = 0x00002006,
    VM_EXIT_MSR_STORE_ADDR_HIGH = 0x00002007,
    VM_EXIT_MSR_LOAD_ADDR = 0x00002008,
    VM_EXIT_MSR_LOAD_ADDR_HIGH = 0x00002009,
    VM_ENTRY_MSR_LOAD_ADDR = 0x0000200a,
    VM_ENTRY_MSR_LOAD_ADDR_HIGH = 0x0000200b,
    TSC_OFFSET = 0x00002010,
    TSC_OFFSET_HIGH = 0x00002011,
    VIRTUAL_APIC_PAGE_ADDR = 0x00002012,
    VIRTUAL_APIC_PAGE_ADDR_HIGH = 0x00002013,
    VMCS_LINK_POINTER = 0x00002800,
    VMCS_LINK_POINTER_HIGH = 0x00002801,
    GUEST_IA32_DEBUGCTL = 0x00002802,
    GUEST_IA32_DEBUGCTL_HIGH = 0x00002803,
    PIN_BASED_VM_EXEC_CONTROL = 0x00004000,
    CPU_BASED_VM_EXEC_CONTROL = 0x00004002,
    EXCEPTION_BITMAP = 0x00004004,
    PAGE_FAULT_ERROR_CODE_MASK = 0x00004006,
    PAGE_FAULT_ERROR_CODE_MATCH = 0x00004008,
    CR3_TARGET_COUNT = 0x0000400a,
    VM_EXIT_CONTROLS = 0x0000400c,
    VM_EXIT_MSR_STORE_COUNT = 0x0000400e,
    VM_EXIT_MSR_LOAD_COUNT = 0x00004010,
    VM_ENTRY_CONTROLS = 0x00004012,
    VM_ENTRY_MSR_LOAD_COUNT = 0x00004014,
    VM_ENTRY_INTR_INFO_FIELD = 0x00004016,
    VM_ENTRY_EXCEPTION_ERROR_CODE = 0x00004018,
    VM_ENTRY_INSTRUCTION_LEN = 0x0000401a,
    TPR_THRESHOLD = 0x0000401c,
    SECONDARY_VM_EXEC_CONTROL = 0x0000401e,
    VM_INSTRUCTION_ERROR = 0x00004400,
    VM_EXIT_REASON = 0x00004402,
    VM_EXIT_INTR_INFO = 0x00004404,
    VM_EXIT_INTR_ERROR_CODE = 0x00004406,
    IDT_VECTORING_INFO_FIELD = 0x00004408,
    IDT_VECTORING_ERROR_CODE = 0x0000440a,
    VM_EXIT_INSTRUCTION_LEN = 0x0000440c,
    VMX_INSTRUCTION_INFO = 0x0000440e,
    GUEST_ES_LIMIT = 0x00004800,
    GUEST_CS_LIMIT = 0x00004802,
    GUEST_SS_LIMIT = 0x00004804,
    GUEST_DS_LIMIT = 0x00004806,
    GUEST_FS_LIMIT = 0x00004808,
    GUEST_GS_LIMIT = 0x0000480a,
    GUEST_LDTR_LIMIT = 0x0000480c,
    GUEST_TR_LIMIT = 0x0000480e,
    GUEST_GDTR_LIMIT = 0x00004810,
    GUEST_IDTR_LIMIT = 0x00004812,
    GUEST_ES_AR_BYTES = 0x00004814,
    GUEST_CS_AR_BYTES = 0x00004816,
    GUEST_SS_AR_BYTES = 0x00004818,
    GUEST_DS_AR_BYTES = 0x0000481a,
    GUEST_FS_AR_BYTES = 0x0000481c,
    GUEST_GS_AR_BYTES = 0x0000481e,
    GUEST_LDTR_AR_BYTES = 0x00004820,
    GUEST_TR_AR_BYTES = 0x00004822,
    GUEST_INTERRUPTIBILITY_INFO = 0x00004824,
    GUEST_ACTIVITY_STATE = 0x00004826,
    GUEST_SM_BASE = 0x00004828,
    GUEST_SYSENTER_CS = 0x0000482A,
    HOST_IA32_SYSENTER_CS = 0x00004c00,
    CR0_GUEST_HOST_MASK = 0x00006000,
    CR4_GUEST_HOST_MASK = 0x00006002,
    CR0_READ_SHADOW = 0x00006004,
    CR4_READ_SHADOW = 0x00006006,
    CR3_TARGET_VALUE0 = 0x00006008,
    CR3_TARGET_VALUE1 = 0x0000600a,
    CR3_TARGET_VALUE2 = 0x0000600c,
    CR3_TARGET_VALUE3 = 0x0000600e,
    EXIT_QUALIFICATION = 0x00006400,
    GUEST_LINEAR_ADDRESS = 0x0000640a,
    GUEST_CR0 = 0x00006800,
    GUEST_CR3 = 0x00006802,
    GUEST_CR4 = 0x00006804,
    GUEST_ES_BASE = 0x00006806,
    GUEST_CS_BASE = 0x00006808,
    GUEST_SS_BASE = 0x0000680a,
    GUEST_DS_BASE = 0x0000680c,
    GUEST_FS_BASE = 0x0000680e,
    GUEST_GS_BASE = 0x00006810,
    GUEST_LDTR_BASE = 0x00006812,
    GUEST_TR_BASE = 0x00006814,
    GUEST_GDTR_BASE = 0x00006816,
    GUEST_IDTR_BASE = 0x00006818,
    GUEST_DR7 = 0x0000681a,
    GUEST_RSP = 0x0000681c,
    GUEST_RIP = 0x0000681e,
    GUEST_RFLAGS = 0x00006820,
    GUEST_PENDING_DBG_EXCEPTIONS = 0x00006822,
    GUEST_SYSENTER_ESP = 0x00006824,
    GUEST_SYSENTER_EIP = 0x00006826,
    HOST_CR0 = 0x00006c00,
    HOST_CR3 = 0x00006c02,
    HOST_CR4 = 0x00006c04,
    HOST_FS_BASE = 0x00006c06,
    HOST_GS_BASE = 0x00006c08,
    HOST_TR_BASE = 0x00006c0a,
    HOST_GDTR_BASE = 0x00006c0c,
    HOST_IDTR_BASE = 0x00006c0e,
    HOST_IA32_SYSENTER_ESP = 0x00006c10,
    HOST_IA32_SYSENTER_EIP = 0x00006c12,
    HOST_RSP = 0x00006c14,
    HOST_RIP = 0x00006c16,
};


#define CPU_BASED_VIRTUAL_INTR_PENDING  0x00000004
#define CPU_BASED_USE_TSC_OFFSETING     0x00000008
#define CPU_BASED_HLT_EXITING           0x00000080
#define CPU_BASED_INVDPG_EXITING        0x00000200
#define CPU_BASED_MWAIT_EXITING         0x00000400
#define CPU_BASED_RDPMC_EXITING         0x00000800
#define CPU_BASED_RDTSC_EXITING         0x00001000
#define CPU_BASED_CR8_LOAD_EXITING      0x00080000
#define CPU_BASED_CR8_STORE_EXITING     0x00100000
#define CPU_BASED_TPR_SHADOW            0x00200000
#define CPU_BASED_MOV_DR_EXITING        0x00800000
#define CPU_BASED_UNCOND_IO_EXITING     0x01000000
#define CPU_BASED_ACTIVATE_IO_BITMAP    0x02000000
#define CPU_BASED_ACTIVATE_MSR_BITMAP   0x10000000
#define CPU_BASED_MONITOR_EXITING       0x20000000
#define CPU_BASED_PAUSE_EXITING         0x40000000

#define VM_EXIT_IA32E_MODE              0x00000200
#define VM_EXIT_ACK_INTR_ON_EXIT        0x00008000


typedef struct _VMX_BASIC_MSR {
    unsigned RevId:32;
    unsigned szVmxOnRegion:12;
    unsigned ClearBit:1;
    unsigned Reserved:3;
    unsigned PhysicalWidth:1;
    unsigned DualMonitor:1;
    unsigned MemoryType:4;
    unsigned VmExitInformation:1;
    unsigned Reserved2:9;
} VMX_BASIC_MSR, *PVMX_BASIC_MSR;



#pragma pack (push, 1)

/*
* Attribute for segment selector. This is a copy of bit 40:47 & 52:55 of the
* segment descriptor.
*/
typedef union
{
    USHORT UCHARs;
    struct
    {
        USHORT type:4;              /* 0;  Bit 40-43 */
        USHORT s:1;                 /* 4;  Bit 44 */
        USHORT dpl:2;               /* 5;  Bit 45-46 */
        USHORT p:1;                 /* 7;  Bit 47 */
        // gap!
        USHORT avl:1;               /* 8;  Bit 52 */
        USHORT l:1;                 /* 9;  Bit 53 */
        USHORT db:1;                /* 10; Bit 54 */
        USHORT g:1;                 /* 11; Bit 55 */
        USHORT Gap:4;
    } fields;
} SEGMENT_ATTRIBUTES;

typedef struct _SEGMENT_SELECTOR
{
    USHORT sel;
    SEGMENT_ATTRIBUTES attributes;
    ULONG limit;
    ULONG64 base;
} SEGMENT_SELECTOR, *PSEGMENT_SELECTOR;

typedef struct
{
    unsigned    LimitLo:16;
    unsigned    BaseLo:16;
    unsigned    BaseMid:8;
    unsigned    Type:4;
    unsigned    System:1;
    unsigned    DPL:2;
    unsigned    Present:1;
    unsigned    LimitHi:4;
    unsigned    AVL:1;
    unsigned    L:1;
    unsigned    DB:1;
    unsigned    Gran:1;    // Granularity
    unsigned    BaseHi:8;
} SEGMENT_DESCRIPTOR, *PSEGMENT_DESCRIPTOR;

typedef struct
{
    USHORT limit0;
    USHORT base0;
    UCHAR  base1;
    UCHAR  attr0;
    UCHAR  limit1attr1;
    UCHAR  base2;
} SEGMENT_DESCRIPTOR2, *PSEGMENT_DESCRIPTOR2;

typedef struct _INTERRUPT_GATE_DESCRIPTOR
{
    USHORT TargetOffset1500;
    USHORT TargetSelector;
    UCHAR  InterruptStackTable;
    UCHAR  Attributes;
    USHORT TargetOffset3116;
    ULONG  TargetOffset6332;
    ULONG  Reserved;
} INTERRUPT_GATE_DESCRIPTOR,*PINTERRUPT_GATE_DESCRIPTOR;

#pragma pack (pop)



#define LA_ACCESSED        0x01
#define LA_READABLE        0x02    // for code segments
#define LA_WRITABLE        0x02    // for data segments
#define LA_CONFORMING      0x04    // for code segments
#define LA_EXPANDDOWN      0x04    // for data segments
#define LA_CODE            0x08
#define LA_STANDARD        0x10
#define LA_DPL_0           0x00
#define LA_DPL_1           0x20
#define LA_DPL_2           0x40
#define LA_DPL_3           0x60
#define LA_PRESENT         0x80

#define LA_LDT64           0x02
#define LA_ATSS64          0x09
#define LA_BTSS64          0x0b
#define LA_CALLGATE64      0x0c
#define LA_INTGATE64       0x0e
#define LA_TRAPGATE64      0x0f

#define HA_AVAILABLE       0x01
#define HA_LONG            0x02
#define HA_DB              0x04
#define HA_GRANULARITY     0x08

typedef enum _SEGREGS {
    R_ES = 0,
    R_CS,
    R_SS,
    R_DS,
    R_FS,
    R_GS,
    R_LDTR,
    R_TR,
}SEGREGS;


typedef struct _GUEST_REGS {
    ULONG eax;
    ULONG ebx;
    ULONG ecx;
    ULONG edx;
    ULONG esp;
    ULONG ebp;
    ULONG esi;
    ULONG edi;
} GUEST_REGS, *PGUEST_REGS;

typedef struct _GUEST_CPU {
    ULONG               ProcessorNumber;
    PGUEST_REGS         pGuestRegs;
    ULONG               bSingleStepping;

    PVOID               VMXON_va;
    PHYSICAL_ADDRESS    VMXON_pa;
    PVOID               VMCS_va;
    PHYSICAL_ADDRESS    VMCS_pa;
    PVOID               MSR_bitmap_va;
    PHYSICAL_ADDRESS    MSR_bitmap_pa;
    PVOID               IO_Bitmap_A_va;
    PHYSICAL_ADDRESS    IO_Bitmap_A_pa;
    PVOID               IO_Bitmap_B_va;
    PHYSICAL_ADDRESS    IO_Bitmap_B_pa;
    PVOID               VMMIDT;
    PVOID               Host_Stack_va;

    PVOID               IOAPIC_va;
    PVOID               LocalAPIC_va;

    ULONG               Old_eax;
    ULONG               Old_ebx;
    ULONG               Old_ecx;
    ULONG               Old_edx;
    ULONG               Old_esi;
    ULONG               Old_edi;
    ULONG               Old_esp;
    ULONG               Old_ebp;
    ULONG               Old_eip;
    ULONG               Old_eflags;

} GUEST_CPU, *PGUEST_CPU;

VOID asmlinkage _ScaleTSCBasedTimer(void);
VOID asmlinkage _CpuSleep(ULONG MicroSecond);
VOID asmlinkage _FlushTLB(void);

ULONG asmlinkage READ_PORT_UCHAR(ULONG Port);
ULONG asmlinkage READ_PORT_USHORT(ULONG Port);
ULONG asmlinkage READ_PORT_ULONG(ULONG Port);
VOID asmlinkage WRITE_PORT_UCHAR(ULONG Port,ULONG Value);
VOID asmlinkage WRITE_PORT_USHORT(ULONG Port,ULONG Value);
VOID asmlinkage WRITE_PORT_ULONG(ULONG Port,ULONG Value);
ULONG asmlinkage _ReadPRT(PVOID IoApicBase,ULONG PRTIndex);
VOID asmlinkage _WritePRT(PVOID IoApicBase,ULONG PRTIndex,ULONG PRTData);

VOID asmlinkage _Return1(void);

ULONG64 asmlinkage _ReadMSR(ULONG MSRIndex);
ULONG64 asmlinkage _WriteMSR(ULONG MSRIndex,ULONG LowPart,ULONG HighPart);

VOID asmlinkage _VmCallFillScreen(void);
VOID asmlinkage _BeepOn(void);
VOID asmlinkage _BeepOff(void);
VOID asmlinkage _Reboot(void);
ULONG64 asmlinkage _TSC(void);

ULONG asmlinkage _CS(void);
ULONG asmlinkage _DS(void);
ULONG asmlinkage _ES(void);
ULONG asmlinkage _FS(void);
ULONG asmlinkage _GS(void);
ULONG asmlinkage _SS(void);
ULONG asmlinkage _ES(void);

ULONG asmlinkage _CR0(void);
ULONG asmlinkage _CR2(void);
ULONG asmlinkage _CR3(void);
ULONG asmlinkage _CR4(void);

ULONG asmlinkage _DR0(void);
ULONG asmlinkage _DR1(void);
ULONG asmlinkage _DR2(void);
ULONG asmlinkage _DR3(void);
ULONG asmlinkage _DR6(void);
ULONG asmlinkage _DR7(void);

VOID asmlinkage _SetCR0(ULONG value);
VOID asmlinkage _SetCR2(ULONG value);
VOID asmlinkage _SetCR3(ULONG value);
VOID asmlinkage _SetCR4(ULONG value);

VOID asmlinkage _SetDR0(ULONG value);
VOID asmlinkage _SetDR1(ULONG value);
VOID asmlinkage _SetDR2(ULONG value);
VOID asmlinkage _SetDR3(ULONG value);
VOID asmlinkage _SetDR6(ULONG value);
VOID asmlinkage _SetDR7(ULONG value);

VOID asmlinkage _CpuId(PULONG pEax,PULONG pEbx,PULONG pEcx,PULONG pEdx);

ULONG asmlinkage _VmxOn(ULONG PtrLowPart,ULONG PtrHighPart);
ULONG asmlinkage _VmClear(ULONG PtrLowPart,ULONG PtrHighPart);
ULONG asmlinkage _VmPtrLd(ULONG PtrLowPart,ULONG PtrHighPart);
ULONG asmlinkage _VmFailValid(ULONG Eflags);
ULONG asmlinkage _VmFailInvalid(ULONG Eflags);
ULONG asmlinkage _ReadVMCS(ULONG Field);
VOID asmlinkage _WriteVMCS(ULONG Field, ULONG Value);
ULONG asmlinkage _VmLaunch(void);

ULONG asmlinkage _Eflags(void);

ULONG asmlinkage _GdtBase(void);
ULONG asmlinkage _IdtBase(void);
USHORT asmlinkage _GdtLimit(void);
USHORT asmlinkage _IdtLimit(void);

USHORT asmlinkage _Ldtr(void);
USHORT asmlinkage _TrSelector(void);


ULONG asmlinkage _StartVirtualization(void);
VOID asmlinkage _GuestEntryPoint(void);
VOID asmlinkage _VmExitHandler(void);

VOID asmlinkage _Invd(void);
VOID asmlinkage _StopVirtualization(ULONG SpecCodeLow,ULONG SpecCodeHigh);
VOID asmlinkage _GuestExit(void);
VOID asmlinkage _VmxOff(ULONG RegEsp,ULONG RegEip);
ULONG asmlinkage _VmxOff_NoGuest(void);

VOID asmlinkage _RegSetIdtr(ULONG Base,ULONG Limit);
VOID asmlinkage _RegSetGdtr(ULONG Base,ULONG Limit);

VOID asmlinkage _DisableWP(void);
VOID asmlinkage _EnableWP(void);


BOOLEAN CheckForVirtualizationSupport(void);
BOOLEAN CheckIfVMXIsEnabled(void);
BOOLEAN SetupVMX(PGUEST_CPU pCpu);
BOOLEAN SetupVMCS(PGUEST_CPU pCpu, PVOID GuestEsp);
BOOLEAN Virtualize(PGUEST_CPU pCpu);

#endif

