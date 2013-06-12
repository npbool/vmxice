#include <linux/module.h>
#include "x86.h"
#include "dbg.h"
#include "video.h"
#include "console.h"
#include "codeview.h"
#include "udis86.h"
#include "sym.h"
#include "mmu.h"

#define MAX_DISASM_LINES  30
#define OPCODE_MAX_LEN    15

ULONG CodeViewHeight;
UCHAR DisasmInstrs[MAX_DISASM_LINES][128];
ULONG DisasmIPS[MAX_DISASM_LINES];

VOID CodeViewInit(void)
{
    CodeViewHeight = 19;
}

VOID FormatInstruction(PUCHAR dst,PUCHAR src)
{
    PUCHAR s;
    ULONG i=0;
    UCHAR c;

    s = src;
    while(*s)
    {
        c = *s;
        if(c >= 'a' && c<= 'z')
            c -= 32;
        *s = c;
        s++;
    }

    while(*src)
    {
        c = *src++;
        *dst++ = c;
        if(c == ' ')
        {
            while(i < 10)
            {
                *dst++ = ' ';
                i++;
            }
        }
        i++;
    }
    *dst = 0;
}

ULONG OriDisasm(PUCHAR str,ULONG Eip)
{
    UDIS ud_obj;
    ULONG len;

    ud_init(&ud_obj);
    ud_set_mode(&ud_obj, 32);
    ud_set_syntax(&ud_obj, UD_SYN_INTEL);
    ud_set_pc(&ud_obj,(int)Eip);
    ud_set_input_buffer(&ud_obj, (uint8_t*)Eip, 32);
    len = ud_disassemble(&ud_obj);
    strcpy(str,ud_insn_asm(&ud_obj));
    return len;
}

ULONG FastDisasm(ULONG Eip)
{
    UDIS ud_obj;
    ULONG len;

    ud_init(&ud_obj);
    ud_set_mode(&ud_obj, 32);
    ud_set_syntax(&ud_obj, UD_SYN_INTEL);
    ud_set_pc(&ud_obj,(int)Eip);
    ud_set_input_buffer(&ud_obj, (uint8_t*)Eip, 32);
    len = ud_decode(&ud_obj);
    return len;
}

ULONG GetRegValue(PGUEST_CPU pCpu,ULONG RegIndex)
{
    CHAR str[128];

    switch(RegIndex)
    {
    case 0:
        return 0;
    case UD_R_EAX:
        return pCpu->pGuestRegs->eax;
    case UD_R_ECX:
        return pCpu->pGuestRegs->ecx;
    case UD_R_EDX:
        return pCpu->pGuestRegs->edx;
    case UD_R_EBX:
        return pCpu->pGuestRegs->ebx;
    case UD_R_ESP:
        return pCpu->pGuestRegs->esp;
    case UD_R_EBP:
        return pCpu->pGuestRegs->ebp;
    case UD_R_ESI:
        return pCpu->pGuestRegs->esi;
    case UD_R_EDI:
        return pCpu->pGuestRegs->edi;
    default:
        snprintf(str,128,"GetRegValue %x\n",RegIndex);
        ConsolePrintStr(str,7,0);
        return 0;
    }
}

ULONG GetCallReg(PGUEST_CPU pCpu,PUDIS pUdis) //call [reg]
{
    if(pUdis->operand[0].type == UD_OP_REG)
        return GetRegValue(pCpu,pUdis->operand[0].base);
    else if(pUdis->operand[0].type == UD_OP_MEM)
    {
        ULONG BaseReg = GetRegValue(pCpu,pUdis->operand[0].base);
        ULONG IndexReg = GetRegValue(pCpu,pUdis->operand[0].index);
        ULONG Scale = GetRegValue(pCpu,pUdis->operand[0].scale);
        ULONG Address = 0;
        CHAR Buf[256];

        switch(pUdis->operand[0].offset)
        {
            case 0:
                Address = (BaseReg+IndexReg*Scale);
            case 8:
                Address = (BaseReg+IndexReg*Scale + pUdis->operand[0].lval.sbyte);
            case 16:
                Address = (BaseReg+IndexReg*Scale + pUdis->operand[0].lval.sword);
            case 32:
                Address = (BaseReg+IndexReg*Scale + pUdis->operand[0].lval.sdword);
        }

        if(!Address)
            return 0;

        if(!IsAddressExist(Address))
        {
            sprintf(Buf,"callmem [%08X] is invalid!!\n",Address);
            ConsolePrintStr(Buf,7,0);
            return 0;
        }

        return *(PULONG)Address;
    }
    return 0;
}

ULONG GetCallImm(PGUEST_CPU pCpu,PUDIS pUdis) //call xxx
{
    if(pUdis->operand[0].type == UD_OP_IMM)
        return pUdis->operand[0].lval.udword;
    else if(pUdis->operand[0].type == UD_OP_JIMM)
    {
        return pUdis->pc + pUdis->operand[0].lval.sdword;
    }
    return 0;
}

ULONG GetSymbolAndOffset(ULONG CallAddr,ULONG ShowOffset,PCHAR Buf,ULONG BufLen)
{
    const char *Ret;
    char SymName[256];
    char *pModName;
    unsigned long SymSize;
    unsigned long Offset;

    Buf[0] = 0;

    Ret = LookupAddress((int)CallAddr,&SymSize,&Offset,&pModName,SymName);
    if(!Ret)
        return 0;

    if(pModName)
    {
        if(Offset || ShowOffset)
        {
            snprintf(Buf,BufLen,"%s!%s + %04Xh",pModName,SymName,(ULONG)Offset);
            return 1;
        }
        else
        {
            snprintf(Buf,BufLen,"%s!%s",pModName,SymName);
            return 1;
        }
    }
    else
    {
        if(Offset || ShowOffset)
        {
            snprintf(Buf,BufLen,"%s + %04Xh",SymName,(ULONG)Offset);
            return 1;
        }
        else
        {
            snprintf(Buf,BufLen,"%s",SymName);
            return 1;
        }
    }
}

ULONG DisasmInstruction(PGUEST_CPU pCpu,PUCHAR str,ULONG Eip,ULONG Detail)
{
    UDIS ud_obj;
    UCHAR instr[80];
    ULONG len;
    UCHAR CodeImage[16];
    ULONG i,j;

    if(!IsAddressRangeExist((ULONG)Eip,OPCODE_MAX_LEN))
    {
        snprintf(str,96,"%04X:%08X  INVALID PAGE",_ReadVMCS(GUEST_CS_SELECTOR),Eip);
        return -1;
    }


    for(i = 0; i < MAX_SW_BP; i++)
    {
        if(SoftBPs[i].Address == Eip)
        {
            memcpy(CodeImage,(PVOID)Eip,16);
            CodeImage[0] = SoftBPs[i].OldOpcode;
            break;
        }
    }

    for(j = 0; j < MAX_STEP_BP; j++)
    {
        if(StepBPs[j].Address == Eip)
        {
            memcpy(CodeImage,(PVOID)Eip,16);
            CodeImage[0] = StepBPs[i].OldOpcode;
            break;
        }
    }
/*
    RtlZeroMemory(&dis,sizeof(dis));
    if(i != MAX_SW_BP || j != MAX_STEP_BP)
        dis.EIP = CodeImage;
    else
        dis.EIP = Eip;
    dis.VirtualAddr = Eip;
    dis.Archi = 32;
    len = Disasm(&dis);
    if(len == -1)
    {
        snprintf(str,96,"%04X:%08X  INVALID OPCODE",_ReadVMCS(GUEST_CS_SELECTOR),Eip);
        return -1;
    }
*/
    ud_init(&ud_obj);
    ud_set_mode(&ud_obj, 32);
    ud_set_syntax(&ud_obj, UD_SYN_INTEL);
    ud_set_pc(&ud_obj,(int)Eip);
    if(i != MAX_SW_BP || j != MAX_STEP_BP)
        ud_set_input_buffer(&ud_obj, (uint8_t*)CodeImage, 32);
    else
        ud_set_input_buffer(&ud_obj, (uint8_t*)Eip, 32);
    len = ud_disassemble(&ud_obj);
    if(len == -1)
    {
        snprintf(str,96,"%04X:%08X  INVALID OPCODE",_ReadVMCS(GUEST_CS_SELECTOR),(ULONG)Eip);
        return -1;
    }

    if(!strncmp(ud_insn_asm(&ud_obj),"call",4))
    {
        ULONG CallAddr;
        CHAR SymName[256];
        ULONG Ret;

        if(Detail)
        {
            CallAddr = GetCallReg(pCpu,&ud_obj);
            if(CallAddr)
            {
                Ret = GetSymbolAndOffset(CallAddr,FALSE,SymName,256);
                if(Ret)
                {
                    FormatInstruction(instr,ud_insn_asm(&ud_obj));
                    snprintf(str,96,"%04X:%08X  %s  ;%s",_ReadVMCS(GUEST_CS_SELECTOR),(ULONG)Eip,instr,SymName);
                    return len;
                }
            }
        }

        CallAddr = GetCallImm(pCpu,&ud_obj);
        if(CallAddr)
        {
            Ret = GetSymbolAndOffset(CallAddr,FALSE,SymName,256);
            if(Ret)
            {
                snprintf(str,96,"%04X:%08X  CALL       %s",_ReadVMCS(GUEST_CS_SELECTOR),(ULONG)Eip,SymName);
                return len;
            }
        }
    }

    FormatInstruction(instr,ud_insn_asm(&ud_obj));
    snprintf(str,96,"%04X:%08X  %s",_ReadVMCS(GUEST_CS_SELECTOR),(ULONG)Eip,instr);
    return len;
}

VOID AddHit(PPREV_INSTR_HITTEST pHitTest,ULONG HitTestArrCount,ULONG PrevAddr)
{
    ULONG i;

    for(i = 0; i < HitTestArrCount && pHitTest[i].pInstrAddr; i++)
    {
        if(pHitTest[i].pInstrAddr == PrevAddr)
        {
            pHitTest[i].HitCount++;
            return;
        }
    }
    pHitTest[i].pInstrAddr = PrevAddr;
    pHitTest[i].HitCount = 1;
}

ULONG GetPrevIp(ULONG Eip)
{
    PREV_INSTR_HITTEST HitTest[16];
    ULONG CurrentAddr = Eip - 1;
    ULONG PrevAddr;
    ULONG DisasmLimit = 0x100;
    ULONG len;
    ULONG i;
    ULONG PrevAddr_MaxHit = 0;
    ULONG MaxHit = 0;

    if(!Eip)
        return FALSE;

    memset(&HitTest,0,sizeof(HitTest));
    while(DisasmLimit)
    {
        PrevAddr = CurrentAddr;
        if(!IsAddressExist(PrevAddr))    //ÄÚ´æ¿É¶ÁÐÔ²âÊÔ
            break;

        while(1)
        {
            len = FastDisasm(PrevAddr);
            if(len != -1 && len)
            {
                if(len + PrevAddr >= Eip)
                {
                    AddHit(&HitTest[0],16,PrevAddr);
                    break;
                }
                else if(len + PrevAddr > Eip)
                {
                    break;
                }
            }
            else
            {
                break;
            }
            PrevAddr += len;
        }

        DisasmLimit--;
        CurrentAddr--;
    }

    for(i = 0; i < 16; i++)
    {
        if(HitTest[i].HitCount > MaxHit)
        {
            MaxHit = HitTest[i].HitCount;
            PrevAddr_MaxHit = HitTest[i].pInstrAddr;
        }
    }
    return PrevAddr_MaxHit;
}

ULONG IsInstrBeSetBp(ULONG Address)
{
	ULONG i;
	for(i = 0; i < MAX_SW_BP; i++)
	{
		if(Address == SoftBPs[i].Address)
			return TRUE;
	}
	return FALSE;
}

VOID ShowDisasm(PGUEST_CPU pCpu,ULONG StartAddress,ULONG NeedPageChange)
{
    ULONG i;
    ULONG Eip;
    ULONG Len;
    ULONG CurrentEip;
    CHAR str[128];
    CHAR szSeparate1[100];
    ULONG Ring;
    ULONG ForeColor,BackColor;
    PCHAR pDisasmStr;

    if(!bScreenBackuped)
        return;

    memset(szSeparate1,0,100);
    memset(szSeparate1,196,98);

    CurrentEip = _ReadVMCS(GUEST_RIP);

    GetSymbolAndOffset(StartAddress,TRUE,str,50);
    memcpy(&szSeparate1[0],str,strlen(str));

    Ring = _ReadVMCS(GUEST_CS_SELECTOR) & 3;
    snprintf(str,128,"Ring%d",Ring);
    memcpy(&szSeparate1[GUI_Width-15],str,strlen(str));
    snprintf(str,128,"CPU%d",pCpu->ProcessorNumber);
    memcpy(&szSeparate1[GUI_Width-7],str,strlen(str));
    PrintStr(1,4,2,0,FALSE,szSeparate1,FALSE);

    for(i = 0; i < CodeViewHeight; i++)
    {
        if(DisasmIPS[i] == StartAddress)
        {
            break;
        }
    }

    if(NeedPageChange)
        i = CodeViewHeight;

    if(i == CodeViewHeight)
    {
        Eip = StartAddress;
        for(i = 0; i < CodeViewHeight; i++)
        {
            pDisasmStr = DisasmInstrs[i];
            if(Eip == CurrentEip)
            {
                Len = DisasmInstruction(pCpu,DisasmInstrs[i],Eip,FALSE);
                Len = DisasmInstruction(pCpu,str,Eip,TRUE);
                pDisasmStr = str;
            }
            else
            {
                Len = DisasmInstruction(pCpu,DisasmInstrs[i],Eip,FALSE);
            }

            if(Eip == CurrentEip)
            {
                ForeColor = 1;
                BackColor = 7;
            }
            else
            {
                if(IsInstrBeSetBp(Eip))
                {
                    ForeColor = 0xb;
                    BackColor = 0;
                }
                else
                {
                    ForeColor = 7;
                    BackColor = 0;
                }
            }

            PrintStr(1,5+i,ForeColor,BackColor,FALSE,pDisasmStr,TRUE);

            if(Len != -1)
            {
                DisasmIPS[i] = Eip;
                Eip += Len;
            }
            else
            {
                DisasmIPS[i] = -1;
                Eip += 1;
            }
        }
        return;
    }

    for(i = 0; i < CodeViewHeight; i++)
    {
        pDisasmStr = DisasmInstrs[i];
        if(DisasmIPS[i] == CurrentEip)
        {
            Len = DisasmInstruction(pCpu,str,CurrentEip,TRUE);
            pDisasmStr = str;
            ForeColor = 1;
            BackColor = 7;
        }
        else
        {
            if(IsInstrBeSetBp(DisasmIPS[i]))
            {
                ForeColor = 0xb;
                BackColor = 0;
            }
            else
            {
                ForeColor = 7;
                BackColor = 0;
            }
        }
        PrintStr(1,5+i,ForeColor,BackColor,FALSE,pDisasmStr,TRUE);
    }
}

VOID ShowCurrentDisasm(PGUEST_CPU pCpu)
{
    ShowDisasm(pCpu,_ReadVMCS(GUEST_RIP),FALSE);
}

VOID RefreshCurrentDisasm(PGUEST_CPU pCpu)
{
    ULONG i;
    ULONG Len;
    ULONG CurrentEip;
    CHAR str[128];
    ULONG ForeColor,BackColor;
    PCHAR pDisasmStr;

    if(!bScreenBackuped)
        return;

    CurrentEip = _ReadVMCS(GUEST_RIP);
    for(i = 0; i < CodeViewHeight; i++)
    {
        pDisasmStr = DisasmInstrs[i];
        if(DisasmIPS[i] == CurrentEip)
        {
            Len = DisasmInstruction(pCpu,str,CurrentEip,TRUE);
            pDisasmStr = str;
            ForeColor = 1;
            BackColor = 7;
        }
        else
        {
            if(IsInstrBeSetBp(DisasmIPS[i]))
            {
                ForeColor = 0xb;
                BackColor = 0;
            }
            else
            {
                ForeColor = 7;
                BackColor = 0;
            }
        }
        PrintStr(1,5+i,ForeColor,BackColor,FALSE,pDisasmStr,TRUE);
    }
}

VOID ClearCurrentDisasm()
{
    memset(DisasmInstrs,0,sizeof(DisasmInstrs));
    memset(DisasmIPS,0,sizeof(DisasmIPS));
}

VOID ShowNextIpDisasm(PGUEST_CPU pCpu)
{
    ULONG Eip;

    Eip = DisasmIPS[1];
    ShowDisasm(pCpu,Eip,TRUE);
}

VOID ShowPrevIpDisasm(PGUEST_CPU pCpu)
{
    ULONG Eip;

    Eip = GetPrevIp(DisasmIPS[0]);
    if(!Eip)
        return;

    ShowDisasm(pCpu,Eip,TRUE);
}
