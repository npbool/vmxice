#include <linux/io.h>
#include <linux/sched.h>
#include "dbg.h"
#include "vmx.h"
#include "vmm.h"
#include "x86.h"
#include "video.h"
#include "console.h"
#include "codeview.h"
#include "scancode.h"
#include "mmu.h"

LARGE_INTEGER SpecCode;
int VmmCount;

//unwelcome vmexit :(
VOID HandleUnimplemented(PGUEST_CPU pCpu, ULONG ExitCode)
{
    ULONG InstructionLength;
    UCHAR str[128];
    ULONG IInfo,AState;

    IInfo = _ReadVMCS(GUEST_INTERRUPTIBILITY_INFO);
    AState = _ReadVMCS(GUEST_ACTIVITY_STATE);

    snprintf(str,128,"What the fuck?! VMExit Reason %d\n",ExitCode & 0xFF);
    ConsolePrintStr(str,0xC,1);
    snprintf(str,128,"EIP: %08X   ESP: %08X\n",_ReadVMCS(GUEST_RIP),_ReadVMCS(GUEST_RSP));
    ConsolePrintStr(str,0xE,0);
    snprintf(str,128,"CR0: %08X   CR3:%08X   CR4:%08X\n",_ReadVMCS(GUEST_CR0),_ReadVMCS(GUEST_CR3),_ReadVMCS(GUEST_CR4));
    ConsolePrintStr(str,0xE,0);
    snprintf(str,128,"GUEST_INTERRUPTIBILITY_INFO = %08X   GUEST_ACTIVITY_STATE = %08X\n",IInfo,AState);
    ConsolePrintStr(str,0xE,0);
    ConsolePrintCurrentPage();

    InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);
    _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP)+InstructionLength);
}

VOID HandleCpuid(PGUEST_CPU pCpu)
{
    ULONG Function;
    ULONG InstructionLength;

    Function = pCpu->pGuestRegs->eax;
    _CpuId(&pCpu->pGuestRegs->eax, &pCpu->pGuestRegs->ebx, &pCpu->pGuestRegs->ecx, &pCpu->pGuestRegs->edx);
    if(Function == 1)
    {
        pCpu->pGuestRegs->ecx &= ~0x20;
    }

    InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);
    _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP)+InstructionLength);
}

VOID HandleInvd(PGUEST_CPU pCpu)
{
    ULONG InstructionLength;

    _Invd();
    InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);
    _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP)+InstructionLength);
}

VOID HandleIoAccess(PGUEST_CPU pCpu)
{
    ULONG InstructionLength;
    ULONG Exit;
    ULONG Dir,Port;
    ULONG Size;
    BOOLEAN isRep, isString;
    ULONG LoopCount = 1;
    ULONG Value;

    Exit = _ReadVMCS(EXIT_QUALIFICATION);
    Size     = (Exit & 7) + 1;
    Dir      = (Exit & (1 << 3)) ? IO_IN : IO_OUT;
    isString = (Exit & (1 << 4)) != 0;
    isRep    = (Exit & (1 << 5)) != 0;
    Port     = (Exit >> 16);

    InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);
    _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP)+InstructionLength);

    if(isRep)
    {
        LoopCount = pCpu->pGuestRegs->ecx;
        pCpu->pGuestRegs->ecx = 0;
    }

    if(isString)
    {
        while(LoopCount)
        {
            if(Dir == IO_OUT)
            {
                switch(Size)
                {
                case 0:
                    Value = *((PUCHAR)(pCpu->pGuestRegs->esi));
                    WRITE_PORT_UCHAR(Port,Value);
                    pCpu->pGuestRegs->esi += 1;
                    break;
                case 1:
                    Value = *((PUSHORT)(pCpu->pGuestRegs->esi));
                    WRITE_PORT_USHORT(Port,Value);
                    pCpu->pGuestRegs->esi += 2;
                    break;
                case 3:
                    Value = *((PULONG)(pCpu->pGuestRegs->esi));
                    WRITE_PORT_ULONG(Port,Value);
                    pCpu->pGuestRegs->esi += 4;
                    break;
                default:
                    break;
                }
            }
            else if(Dir == IO_IN)
            {
                switch(Size)
                {
                case 0:
                    Value = READ_PORT_UCHAR(Port);
                    *((PUCHAR)(pCpu->pGuestRegs->edi)) = (UCHAR)Value;
                    pCpu->pGuestRegs->edi += 1;
                    break;
                case 1:
                    Value = READ_PORT_USHORT(Port);
                    *((PUSHORT)(pCpu->pGuestRegs->edi)) = (USHORT)Value;
                    pCpu->pGuestRegs->edi += 2;
                    break;
                case 3:
                    Value = READ_PORT_ULONG(Port);
                    *((PULONG)(pCpu->pGuestRegs->edi)) = (ULONG)Value;
                    pCpu->pGuestRegs->edi += 4;
                    break;
                default:
                    break;
                }
            }
            LoopCount--;
        }
    }
    else
    {
        if(Dir == IO_OUT)
        {
            switch(Size)
            {
            case 0:
                WRITE_PORT_UCHAR(Port,pCpu->pGuestRegs->eax);
                break;
            case 1:
                WRITE_PORT_USHORT(Port,pCpu->pGuestRegs->eax);
                break;
            case 3:
                WRITE_PORT_ULONG(Port,pCpu->pGuestRegs->eax);
                break;
            default:
                break;
            }
        }
        else if(Dir == IO_IN)
        {
            switch(Size)
            {
            case 0:
                Value = READ_PORT_UCHAR(Port);
                pCpu->pGuestRegs->eax = (pCpu->pGuestRegs->eax & 0xFFFFFF00) + (Value & 0xFF);
                break;
            case 1:
                Value = READ_PORT_USHORT(Port);
                pCpu->pGuestRegs->eax = (pCpu->pGuestRegs->eax & 0xFFFF0000) + (Value & 0xFFFF);

                if((UCHAR)Value == SCANCODE_F12)
                {
                    bDebuggerBreak = TRUE;
                }

                break;
            case 3:
                Value = READ_PORT_ULONG(Port);
                pCpu->pGuestRegs->eax = Value;
                break;
            default:
                break;
            }
        }
    }
}

VOID HandleVmCall(PGUEST_CPU pCpu)
{
    ULONG InjectEvent = 0;
    PINTERRUPT_INJECT_INFO_FIELD pInjectEvent = (PINTERRUPT_INJECT_INFO_FIELD)&InjectEvent;
    ULONG InstructionLength, Eip, Esp;

    Eip = _ReadVMCS(GUEST_RIP);

    InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);

    if ((pCpu->pGuestRegs->eax == SpecCode.LowPart) && (pCpu->pGuestRegs->edx == SpecCode.HighPart))
    {
        CmdClearBreakpoint(pCpu,"bc *");
        ClearStepBps();
        Eip = (ULONG)_GuestExit;
        Esp = pCpu->pGuestRegs->esp;
        _VmxOff(Esp,Eip);
    }
    else if(pCpu->pGuestRegs->eax == 0xdeadc0de)
    {
        FillScreen();
        InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);
        _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP)+InstructionLength);
    }
    else
    {
        InjectEvent = 0;
        pInjectEvent->Vector = INVALID_OPCODE_EXCEPTION;
        pInjectEvent->InterruptionType = HARDWARE_EXCEPTION;
        pInjectEvent->DeliverErrorCode = 0;
        pInjectEvent->Valid = 1;
        _WriteVMCS(VM_ENTRY_INTR_INFO_FIELD, InjectEvent);
    }
}

VOID HandleVmInstruction(PGUEST_CPU pCpu)
{
    ULONG InjectEvent = 0;
    PINTERRUPT_INJECT_INFO_FIELD pInjectEvent = (PINTERRUPT_INJECT_INFO_FIELD)&InjectEvent;

    InjectEvent = 0;
    pInjectEvent->Vector = INVALID_OPCODE_EXCEPTION;
    pInjectEvent->InterruptionType = HARDWARE_EXCEPTION;
    pInjectEvent->DeliverErrorCode = 0;
    pInjectEvent->Valid = 1;
    _WriteVMCS(VM_ENTRY_INTR_INFO_FIELD, InjectEvent);
}

VOID HandleMsrRead(PGUEST_CPU pCpu)
{
    LARGE_INTEGER Msr;
    ULONG ecx;
    ULONG InstructionLength;


    ecx = pCpu->pGuestRegs->ecx;
    Msr.QuadPart = _ReadMSR(ecx);
    pCpu->pGuestRegs->eax = Msr.LowPart;
    pCpu->pGuestRegs->edx = Msr.HighPart;

    if(ecx == MSR_IA32_FEATURE_CONTROL)
    {
        pCpu->pGuestRegs->eax &= ~4;
    }

    InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);
    _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP)+InstructionLength);
}

VOID HandleMsrWrite(PGUEST_CPU pCpu)
{
    ULONG InstructionLength;

    _WriteMSR(pCpu->pGuestRegs->ecx,pCpu->pGuestRegs->eax,pCpu->pGuestRegs->edx);
    InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);
    _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP)+InstructionLength);
}

ULONG AttachProcess(ULONG TargetCR3)
{
    PULONG vTargetCR3 = phys_to_virt(TargetCR3);
    PULONG vHostCR3 = phys_to_virt(_CR3());
    ULONG i;
    UCHAR str[128];

    if(TargetCR3 > 892*1024*1024)
        return 0;

    memset(vHostCR3,0,0xC00);
    for(i = 0; i < 1024; i++)
    {
        if(!*(vHostCR3+i))
        {
            *(vHostCR3+i) = *(vTargetCR3+i);
        }
        else
        {
            if(*(vTargetCR3+i) && *(vHostCR3+i) != *(vTargetCR3+i))
            {
                sprintf(str,"TargetCR3:%08x  Index:%d  Host:[%08x]  Guest:[%08x]\n",TargetCR3,i,*(vHostCR3+i),*(vTargetCR3+i));
                PrintStr(0,0,7,1,FALSE,str,TRUE);
            }
        }
    }
    _FlushTLB();
    return 1;
}

VOID HandleCrAccess(PGUEST_CPU pCpu)
{
    PMOV_CR_QUALIFICATION pExitQualification;
    ULONG Exit;
    ULONG Cr = 0;
    ULONG Reg = 0;
    ULONG InstructionLength;

    Exit =_ReadVMCS(EXIT_QUALIFICATION);
    pExitQualification = (PMOV_CR_QUALIFICATION)&Exit;

    switch (pExitQualification->ControlRegister)
    {
    case 0:
        Cr = _ReadVMCS(GUEST_CR0);
        break;

    case 3:
        Cr = _ReadVMCS(GUEST_CR3);
        break;

    case 4:
        Cr = _ReadVMCS(GUEST_CR4);
        break;

    default:
        //what the fuck?
        break;
    }

    switch (pExitQualification->Register)
    {
    case 0:
        Reg = pCpu->pGuestRegs->eax;
        break;

    case 1:
        Reg = pCpu->pGuestRegs->ecx;
        break;

    case 2:
        Reg = pCpu->pGuestRegs->edx;
        break;

    case 3:
        Reg = pCpu->pGuestRegs->ebx;
        break;

    case 4:
        Reg = pCpu->pGuestRegs->esp;
        break;

    case 5:
        Reg = pCpu->pGuestRegs->ebp;
        break;

    case 6:
        Reg = pCpu->pGuestRegs->esi;
        break;

    case 7:
        Reg = pCpu->pGuestRegs->edi;
        break;

    default:
        //what the fuck?
        break;
    }

    switch (pExitQualification->AccessType)
    {
    case 0://MOV_TO_CR
        switch (pExitQualification->ControlRegister)
        {
        case 0:
            _WriteVMCS(GUEST_CR0, Reg);
            break;

        case 3:
            _WriteVMCS(GUEST_CR3, Reg);

            //AttachGuestProcess();
            if(bDebuggerBreak)
            {
                ClearCurrentDisasm();
                RefreshOldRegister(pCpu);
                EnterHyperDebugger(pCpu);
            }
            break;

        case 4:
            _WriteVMCS(GUEST_CR4, Reg);
            break;

        default:
            break;
        }
        break;

    case 1://MOV_FROM_CR
        switch (pExitQualification->Register)
        {
        case 0:
            pCpu->pGuestRegs->eax = Cr;
            break;


        case 1:
            pCpu->pGuestRegs->ecx = Cr;
            break;

        case 2:
            pCpu->pGuestRegs->edx = Cr;
            break;

        case 3:
            pCpu->pGuestRegs->ebx = Cr;
            break;

        case 4:
            pCpu->pGuestRegs->esp = Cr;
            break;

        case 5:
            pCpu->pGuestRegs->ebp = Cr;
            break;

        case 6:
            pCpu->pGuestRegs->esi = Cr;
            break;

        case 7:
            pCpu->pGuestRegs->edi = Cr;
            break;

        default:
            break;
        }

        break;

    default:
        break;
    }

    InstructionLength = _ReadVMCS(VM_EXIT_INSTRUCTION_LEN);
    _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP)+InstructionLength);
}

VOID InjectPageFault(PGUEST_CPU pCpu,ULONG MemoryAddress)
{
    ULONG InjectEvent = 0;
    PINTERRUPT_INJECT_INFO_FIELD pInjectEvent = (PINTERRUPT_INJECT_INFO_FIELD)&InjectEvent;

    _SetCR2(MemoryAddress);
    _WriteVMCS(VM_ENTRY_EXCEPTION_ERROR_CODE, 0);
    InjectEvent = 0;
    pInjectEvent->Vector = PAGE_FAULT_EXCEPTION;
    pInjectEvent->InterruptionType = HARDWARE_EXCEPTION;
    pInjectEvent->DeliverErrorCode = 1;
    pInjectEvent->Valid = 1;
    _WriteVMCS(VM_ENTRY_INTR_INFO_FIELD, InjectEvent);
    _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP));
}

VOID HandleDebugException(PGUEST_CPU pCpu)
{
    ULONG64 ExitQualification;
    ULONG InjectEvent = 0;
    PINTERRUPT_INJECT_INFO_FIELD pInjectEvent = (PINTERRUPT_INJECT_INFO_FIELD)&InjectEvent;
    ULONG GuestEip = _ReadVMCS(GUEST_RIP);
    ULONG Eflags;

    if(bSingleStepping)
    {
        if(!IsAddressRangeExist((ULONG)GuestEip,15))
        {
            InjectPageFault(pCpu,GuestEip);
            return;
        }

        Eflags = _ReadVMCS(GUEST_RFLAGS);
        Eflags &= ~FLAGS_TF_MASK;
        _WriteVMCS(GUEST_RFLAGS,Eflags);

        if(bSingleStepPushfd)
        {
            ULONG GuestEflags;

            if(IsAddressRangeExist(pCpu->pGuestRegs->esp,4))
            {
                GuestEflags = *(PULONG)(pCpu->pGuestRegs->esp);
                GuestEflags &= ~FLAGS_TF_MASK;
                *(PULONG)(pCpu->pGuestRegs->esp) = GuestEflags;
            }
            bSingleStepPushfd = FALSE;
        }

        if(pCurrentSwBp)                        //ÕâÊÇ#BP¶ÏÏÂÀŽÖ®ºóÉèÖÃµÄµ¥²œÖÐ¶Ï£¬ÎÒÃÇ»ÖžŽÉÏÒ»žöÖžÁîµÄ¶Ïµã£¬ÐŽINT3
        {
            //OldCR3 = AttachGuestProcess();
            if(IsAddressExist(pCurrentSwBp->Address))
                *(PUCHAR)pCurrentSwBp->Address = INT3_OPCODE;
            //DetachTargetProcess(OldCR3);
            pCurrentSwBp = NULL;
        }

        if(bShowGUIwhileSingleStepping)
        {
            EnterHyperDebugger(pCpu);
        }
        else
        {
            bSingleStepping = FALSE;
            pCpu->bSingleStepping = FALSE;
        }
    }
    else
    {
        ExitQualification = _ReadVMCS(EXIT_QUALIFICATION);
        _SetDR6(ExitQualification);
        pInjectEvent->Vector = DEBUG_EXCEPTION;
        pInjectEvent->InterruptionType = HARDWARE_EXCEPTION;
        pInjectEvent->DeliverErrorCode = 0;
        pInjectEvent->Valid = 1;
        _WriteVMCS(VM_ENTRY_INTR_INFO_FIELD, InjectEvent);
        _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP));
    }
}

VOID HandleBreakpointException(PGUEST_CPU pCpu)
{
    ULONG GuestEip;
    ULONG Eflags;
    ULONG InjectEvent;
    PINTERRUPT_INJECT_INFO_FIELD pInjectEvent = (PINTERRUPT_INJECT_INFO_FIELD)&InjectEvent;
    ULONG i,j;
    PSW_BP pSoftBp,pStepBp;


    GuestEip = _ReadVMCS(GUEST_RIP);

    if(bSingleStepPushfd)        //žÕµ¥²œÁËÒ»žöpushfdÈ»ºóŸÍint3ÎÒÃÇÒªžãÒ»ÏÂ±£ŽæÔÚ¶ÑÕ»ÀïµÄeflags
    {
        ULONG GuestEflags;

        if(IsAddressRangeExist(pCpu->pGuestRegs->esp,4))
        {
            GuestEflags = *(PULONG)(pCpu->pGuestRegs->esp);        //°Ñ±£Žæµœ¶ÑÕ»ÖÐµÄEFLAGSÈ¡ÏûTF±êÖŸ »ÖžŽIF±êÖŸ
            GuestEflags &= ~FLAGS_TF_MASK;
            *(PULONG)(pCpu->pGuestRegs->esp) = GuestEflags;
        }

        bSingleStepPushfd = FALSE;
    }

    if(bSingleStepping)            //ÕýÔÚµ¥²œµÄÊ±ºòÖŽÐÐÁËÒ»žöint3£¬ŸÍÇå³ýÒ»ÏÂ£¬žúÉÏÃæµÄÒ»Ñù
    {
        Eflags = _ReadVMCS(GUEST_RFLAGS);
        Eflags &= ~FLAGS_TF_MASK;
        _WriteVMCS(GUEST_RFLAGS,Eflags);
        bSingleStepping = FALSE;
        pCpu->bSingleStepping = FALSE;
    }

    for(i = 0; i < MAX_SW_BP; i++)        //²éÕÒÊÇ²»ÊÇÎÒÃÇµÄint3¶Ïµã
    {
        pSoftBp = &SoftBPs[i];
        if(pSoftBp->Address == GuestEip)
        {
            break;
        }
    }

    for(j = 0; j < MAX_STEP_BP; j++)    //²éÕÒÊÇ²»ÊÇStepoverµÄint3¶Ïµã
    {
        pStepBp = &StepBPs[j];
        if(pStepBp->Address == GuestEip)
        {
            break;
        }
    }

    if(i != MAX_SW_BP)
    {
        SwBreakpointEvent(pCpu,pSoftBp);    //ÊÇÎÒÃÇµÄ¶Ïµã
    }
    else if(j != MAX_STEP_BP)
    {
        SwStepOverEvent(pCpu);        //ÊÇÎÒÃÇµÄ¶Ïµã for stepover
    }
    else                                    //²»ÊÇµÄ»°ŸÍœ»»Ø¿Í»§»úŽŠÀí
    {
        ULONG Eip = _ReadVMCS(GUEST_RIP);

        if(Eip >= 0xC0000000)
        {
            ClearCurrentDisasm();
			_WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP) + _ReadVMCS(VM_EXIT_INSTRUCTION_LEN));
			RefreshOldRegister(pCpu);
			EnterHyperDebugger(pCpu);
        }
        else
        {
            InjectEvent = 0;
            pInjectEvent->Vector = BREAKPOINT_EXCEPTION;
            pInjectEvent->InterruptionType = SOFTWARE_INTERRUPT;
            pInjectEvent->DeliverErrorCode = 0;
            pInjectEvent->Valid = 1;
            _WriteVMCS(VM_ENTRY_INTR_INFO_FIELD, InjectEvent);
            _WriteVMCS(VM_ENTRY_INSTRUCTION_LEN, 1);
            _WriteVMCS(GUEST_RIP, GuestEip);
        }
    }
}

VOID HandleException(PGUEST_CPU pCpu)
{
    ULONG Event, InjectEvent;
    ULONG64 ErrorCode, ExitQualification;
    PINTERRUPT_INFO_FIELD pEvent;
    PINTERRUPT_INJECT_INFO_FIELD pInjectEvent;

    Event = (ULONG)_ReadVMCS(VM_EXIT_INTR_INFO);
    pEvent = (PINTERRUPT_INFO_FIELD)&Event;

    InjectEvent = 0;
    pInjectEvent = (PINTERRUPT_INJECT_INFO_FIELD)&InjectEvent;
    
    switch (pEvent->InterruptionType)
    {
    case NMI_INTERRUPT:
        InjectEvent = 0;
        pInjectEvent->Vector = NMI_INTERRUPT;
        pInjectEvent->InterruptionType = NMI_INTERRUPT;
        pInjectEvent->DeliverErrorCode = 0;
        pInjectEvent->Valid = 1;
        _WriteVMCS(VM_ENTRY_INTR_INFO_FIELD, InjectEvent);
        break;

    case EXTERNAL_INTERRUPT:
        break;

    case HARDWARE_EXCEPTION:
        switch (pEvent->Vector)
        {
        case DEBUG_EXCEPTION:
            HandleDebugException(pCpu);
            break;

        case PAGE_FAULT_EXCEPTION:
            ErrorCode = _ReadVMCS(VM_EXIT_INTR_ERROR_CODE);
            ExitQualification = _ReadVMCS(EXIT_QUALIFICATION);
            _SetCR2(ExitQualification);
            _WriteVMCS(VM_ENTRY_EXCEPTION_ERROR_CODE, ErrorCode);
            InjectEvent = 0;
            pInjectEvent->Vector = PAGE_FAULT_EXCEPTION;
            pInjectEvent->InterruptionType = HARDWARE_EXCEPTION;
            pInjectEvent->DeliverErrorCode = 1;
            pInjectEvent->Valid = 1;
            _WriteVMCS(VM_ENTRY_INTR_INFO_FIELD, InjectEvent);
            _WriteVMCS(GUEST_RIP, _ReadVMCS(GUEST_RIP));
            break;

        default:
            break;
        }

        break;

    case SOFTWARE_EXCEPTION:
        /* #BP (int3) and #OF (into) */

        switch (pEvent->Vector)
        {
        case BREAKPOINT_EXCEPTION:
            HandleBreakpointException(pCpu);
            break;

        case OVERFLOW_EXCEPTION:
        default:
            break;
        }
        break;

    default:
        break;
    }
}

VOID asmlinkage VmExitHandler(PGUEST_CPU pCpu)
{
    ULONG ExitCode;
    //UCHAR str[128];

    _RegSetIdtr(_ReadVMCS(HOST_IDTR_BASE),0x7FF);
    _RegSetGdtr(_ReadVMCS(HOST_GDTR_BASE),0x3FF);
    pCpu->pGuestRegs->esp = _ReadVMCS(GUEST_RSP);
    ExitCode = _ReadVMCS(VM_EXIT_REASON);

    //snprintf(str,128,"[%d]VmExitHandler ExitCode %d..\n",VmmCount,ExitCode);
    //PrintStr(0,ExitCode,7,1,FALSE,str,TRUE);

    AttachProcess(_ReadVMCS(GUEST_CR3));

    switch (ExitCode)
    {
    case EXIT_REASON_EXCEPTION_NMI:
        HandleException(pCpu);
        break;

    case EXIT_REASON_EXTERNAL_INTERRUPT:
    case EXIT_REASON_TRIPLE_FAULT:
    case EXIT_REASON_INIT:
    case EXIT_REASON_SIPI:
    case EXIT_REASON_IO_SMI:
    case EXIT_REASON_OTHER_SMI:
    case EXIT_REASON_PENDING_INTERRUPT:
    case EXIT_REASON_TASK_SWITCH:
        HandleUnimplemented(pCpu, ExitCode);
        break;

    case EXIT_REASON_CPUID:
        HandleCpuid(pCpu);
        break;

    case EXIT_REASON_HLT:
        HandleUnimplemented(pCpu, ExitCode);
        break;

    case EXIT_REASON_INVD:
        HandleInvd(pCpu);
        break;

    case EXIT_REASON_INVLPG:
    case EXIT_REASON_RDPMC:
    case EXIT_REASON_RDTSC:
    case EXIT_REASON_RSM:
        HandleUnimplemented(pCpu, ExitCode);
        break;

    case EXIT_REASON_VMCALL:
        HandleVmCall(pCpu);
        break;

    case EXIT_REASON_VMCLEAR:
    case EXIT_REASON_VMLAUNCH:
    case EXIT_REASON_VMPTRLD:
    case EXIT_REASON_VMPTRST:
    case EXIT_REASON_VMREAD:
    case EXIT_REASON_VMRESUME:
    case EXIT_REASON_VMWRITE:
    case EXIT_REASON_VMXOFF:
    case EXIT_REASON_VMXON:
        HandleVmInstruction(pCpu);
        break;

    case EXIT_REASON_CR_ACCESS:
        HandleCrAccess(pCpu);
        break;

    case EXIT_REASON_DR_ACCESS:
        HandleUnimplemented(pCpu, ExitCode);
        break;
    case EXIT_REASON_IO_INSTRUCTION:
        HandleIoAccess(pCpu);
        break;

    case EXIT_REASON_MSR_READ:
        HandleMsrRead(pCpu);
        break;

    case EXIT_REASON_MSR_WRITE:
        HandleMsrWrite(pCpu);
        break;

    case EXIT_REASON_INVALID_GUEST_STATE:
    case EXIT_REASON_MSR_LOADING:
    case EXIT_REASON_MWAIT_INSTRUCTION:
    case EXIT_REASON_MONITOR_INSTRUCTION:
    case EXIT_REASON_PAUSE_INSTRUCTION:
    case EXIT_REASON_MACHINE_CHECK:
    case EXIT_REASON_TPR_BELOW_THRESHOLD:
        HandleUnimplemented(pCpu, ExitCode);
        break;

    default:
        HandleUnimplemented(pCpu, ExitCode);
        break;
    }
    _WriteVMCS(GUEST_RSP, pCpu->pGuestRegs->esp);

    //snprintf(str,128,"[%d]VmExitHandler ExitCode %d..ok\n",VmmCount,ExitCode);
    //PrintStr(0,ExitCode,7,1,FALSE,str,TRUE);
    VmmCount++;
}
