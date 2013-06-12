#include <linux/module.h>
#include <linux/sched.h>
#include "vmm.h"
#include "vmx.h"
#include "video.h"
#include "console.h"
#include "keyboard.h"
#include "scancode.h"
#include "codeview.h"
#include "apic.h"
#include "x86.h"
#include "dbg.h"
#include "exp.h"
#include "vmmstring.h"
#include "mmu.h"
#include "sym.h"

ULONG bSingleStepping;
ULONG bSingleStepPushfd;

ULONG bNeedRestoreScreen;
ULONG bScreenBackuped;
ULONG bShowGUIwhileSingleStepping;

ULONG bDebuggerBreak;

#define MAX_CMD_HISTORY        10
CHAR szCmdHistory[MAX_CMD_HISTORY][128];
ULONG iCmdHistoryPos = 0;

PSW_BP pCurrentSwBp;					//µ±Ç°µÄint3ÖÐ¶Ï
ULONG iCurrentHwBp = -1;					//µ±Ç°µÄBPM XÖÐ¶Ï

SW_BP SoftBPs[MAX_SW_BP];				//¶ÏµãÊý×é
SW_BP StepBPs[MAX_SW_BP];				//¶ÏµãÊý×é
HW_BP HardBPs[4];						//Ó²Œþ¶Ïµã

CMD_HELP CmdHelp[] = {
	{"BPX","Breakpoint on execute","BPX [addr] if [condition] do [cmd]","bpx ntsetvaluekey if \"[[esp+8]+4]==\"imagepath\"\" do \"? byte [esp+4]\"\n",CmdSetSwBreakpoint},
	{"BC","Clear breakpoint","BC [*|id]",NULL,CmdClearBreakpoint},
	{"BL","List current breakpoints","No param for BL",NULL,CmdListBreakpoint},
	{"STACK","Display call stack","STACK","stack\n",CmdDisplayCallstack},
	{"U","Disassemble instructions","U [address|symbolname]","u eip\n",CmdDisplayDisasm},
	{"PAGE","Display page table information","PAGE [virtaddr]","page 0FFFE0000\n",CmdGetPageInfo},
	{"PHYS","Display all virtual addresses for physical address","PHYS [physaddr]","phys 0D0000000\n",CmdGetVirtAddr},
	{"CPU","Display cpu registers information","No param for CPU",NULL,CmdDisplayCpuReg},
	{"IDT","Display IDT","No param for IDT",NULL,CmdDisplayIdt},
	{"H","Display help information","H [cmd]","h bpx\n",CmdDisplayHelp},
	{"DB","Display memory(byte)","DB [address|symbolname]","db [esp+4]\n",CmdDisplayMemoryByte},
	{"DW","Display memory(word)","DW [address|symbolname]","dw [esp+4]\n",CmdDisplayMemoryWord},
	{"DD","Display memory(dword)","DD [address|symbolname]","dd [esp+4]\n",CmdDisplayMemoryDword},
	{"?","Compute expression","? [exp]","? [[esp+8]+4]\n",CmdCalcExp},
};
ULONG CmdCount = sizeof(CmdHelp) / sizeof(CMD_HELP);

HELP_INFO pHelpInfo[] = {
	{" Control Keys                                                                                     \n",1,7},
	{"F7        - Singlestep                                                                            \n",7,0},
	{"F8        - Stepover                                                                              \n",7,0},
	{"F9        - Execute until return                                                                  \n",7,0},
	{"F11       - Print screen                                                                          \n",7,0},
	{"F12       - Show/Hide VMXICE                                                                      \n",7,0},
	{"Shift + \x18 - Console line up                                                                    \n",7,0},
	{"Shift + \x19 - Console line down                                                                  \n",7,0},
	{"Ctrl  + \x18 - Disassembly line up                                                                \n",7,0},
	{"Ctrl  + \x19 - Disassembly line down                                                              \n",7,0},
	{" Commands for Breakpoints                                                                         \n",1,7},
	{"BC   - Clear Breakpoint                                                                           \n",7,0},
	{"BL   - List Breakpoint                                                                            \n",7,0},
	{"BPX  - Set software breakpoint on execute                                                         \n",7,0},
	{" Commands for Memory                                                                              \n",1,7},
	{"CPU  - Display cpu registers information                                                          \n",7,0},
	{"DB   - Display memory(byte)                                                                       \n",7,0},
	{"DD   - Display memory(dword)                                                                      \n",7,0},
	{"DW   - Display memory(word)                                                                       \n",7,0},
	{"PAGE - Display page table information                                                             \n",7,0},
	{"PHYS - Display all virtual addresses for physical address                                         \n",7,0},
	{"U    - Disassemble instructions                                                                   \n",7,0},
	{" Commands for Information                                                                         \n",1,7},
	{"?       - Compute expression                                                                      \n",7,0},
	{"H       - Display this help                                                                       \n",7,0},
	{"IDT     - Display IDT                                                                             \n",7,0},
	{"STACK   - Display call stack                                                                      \n",7,0},
};
ULONG HelpInfoCount = sizeof(pHelpInfo) / sizeof(HELP_INFO);

UCHAR GetConsolePageControlKey(void)
{
	UCHAR c;
	BOOLEAN isMouse;
	ULONG cursorState;
	ULONG x,y;

	x = 1;
	y = GUI_Height - 3;
	cursorState = 1;						//¹â±ê×ŽÌ¬£¬Ò»ÉÁÒ»ÉÁµÄ~~
	PrintChar(x,y,7,0,FALSE,':');			//ÌáÊŸ·û -0-
	x++;

	PrintStr(1,GUI_Height-2,0,3,FALSE,"  Enter: NextLine    Esc: Stop    Otherkey: NextPage",TRUE);

	while (1)
	{
		_CpuSleep(1000);	//ÎÒ¿ŽhyperdbgºÃÏñÓÐÕâžö
		if(cursorState == 1)				//»­¹â±ê
		{
			DrawCursor(x,y,FALSE,TRUE);
			cursorState = 0;
		}
		else
		{
			DrawCursor(x,y,FALSE,FALSE);
			cursorState = 1;
		}

		if(!(KeyboardReadKeystroke(&c, &isMouse)))	//¶ÁŒüÅÌ
		{
			continue;
		}

		if(isMouse)							//¶Á³öÀŽµÄÊÇPS2Êó±ê£¬ºöÂÔ
			continue;

		if(c & BREAK_CODE)
			continue;

		return c;
	}
}

ULONG GetCommandParam(PCHAR cmd,PCHAR retparam,ULONG paramid)
{
	ULONG IsQuota = 0;			//ÊÇ·ñÒýºÅÆðÍ·
	ULONG CurrentParamId = 0;
	ULONG QuotaIgnoreCount = 0;

	retparam[0] = 0;

	while(cmd[0])
	{
		if(paramid == CurrentParamId)	//µ±Ç°Î»ÓÚÄ¿±ê²ÎÊýidÖÐ£¬žŽÖÆ²ÎÊý
		{
			retparam[0] = cmd[0];
			retparam++;
		}
		if(cmd[0] == '"' && IsQuota)	//ÓöµœÁËË«ÒýºÅ£¬¶øÇÒÊÇË«ÒýºÅÆðÊŒ£¬ÐèÒªÅÐ¶Ï
		{
			if(cmd[1] == 0)				//ÏÂÒ»žö×Ö·ûÊÇœáÊø
			{
				if(paramid == CurrentParamId)	//ÅÐ¶ÏÊÇ·ñÈ¡µœÁËÎÒÃÇÏëÒªµÄ
				{
					retparam--;
					retparam[0] = 0;
					return 1;					//È¡µœÁË£¬·µ»Ø°É
				}
				break;					//ÒÑŸ­œáÊøÁË Ìø³öÑ­»·°É
			}
			else if(cmd[1] == ' ' && !QuotaIgnoreCount)	//ÏÂÒ»žö×Ö·ûÊÇ¿Õžñ£¬¶øÇÒÃ»ÓÐÐèÒªºöÂÔµÄË«ÒýºÅÁË
			{
				if(paramid == CurrentParamId)	//ÅÐ¶ÏÊÇ·ñÈ¡µœÁËÎÒÃÇÏëÒªµÄ
				{
					retparam--;
					retparam[0] = 0;
					return 1;					//È¡µœÁË£¬·µ»Ø°É
				}
				CurrentParamId++;				//Ã»È¡µœŸÍµÝÔöid
				cmd += 2;						//ºöÂÔË«ÒýºÅºÍ¿Õžñ
				IsQuota = 0;					//»ÖžŽË«ÒýºÅÆðÊŒ±êŒÇ
				if(cmd[0] == '"')				//ÏÂÒ»žö×Ö·ûÊÇË«ÒýºÅ
				{
					IsQuota = 1;				//ÉèÖÃË«ÒýºÅÆðÊŒ±êŒÇ
					cmd++;						//ºöÂÔÕâžöË«ÒýºÅ
				}
				continue;						//ŒÌÐøÑ­»·È¡×Ö·û
			}
			else
			{
				if(!QuotaIgnoreCount)			//Ë«ÒýºÅÏÂÒ»žö×Ö·û²»ÊÇ¿Õžñ
					QuotaIgnoreCount++;			//Ã»ÓÐÐèÒªºöÂÔµÄË«ÒýºÅ£¬ºöÂÔÐèÇó+1
				else
					QuotaIgnoreCount--;			//ÓÐÐèÒªºöÂÔµÄË«ÒýºÅ£¬ºöÂÔÐèÇó-1
			}
		}
		else if(cmd[0] == ' ')		//ÓöµœÁË¿Õžñ
		{
			if(!IsQuota)	//·ÇÒýºÅÆðÍ·µÄ¿Õžñ ±íÊŸ²ÎÊýœáÊø
			{
				if(paramid == CurrentParamId)	//ÅÐ¶ÏÊÇ·ñÈ¡µœÁËÎÒÃÇÏëÒªµÄ
				{
					retparam--;
					retparam[0] = 0;
					return 1;					//È¡µœÁË£¬·µ»Ø°É
				}
				CurrentParamId++;	//Ã»È¡µœŸÍµÝÔöid
				cmd++;				//ºöÂÔ¿Õžñ
				IsQuota = 0;		//»ÖžŽË«ÒýºÅÆðÊŒ±êŒÇ
				if(cmd[0] == '"')	//ÏÂÒ»žö×Ö·ûÊÇË«ÒýºÅ
				{
					IsQuota = 1;	//ÉèÖÃË«ÒýºÅÆðÊŒ±êŒÇ
					cmd++;			//ºöÂÔÕâžöË«ÒýºÅ
				}
				continue;			//ŒÌÐøÑ­»·È¡×Ö·û
			}
		}
		cmd++;
	}
	if(paramid == CurrentParamId)	//ÅÐ¶ÏÊÇ·ñÈ¡µœÁËÎÒÃÇÏëÒªµÄ
	{
		retparam[0] = 0;
		return 1;					//È¡µœÁË£¬·µ»Ø°É
	}
	return 0;
}

VOID ShowCmdTip(PCHAR cmdstring)
{
	ULONG i,iEqual = 0;
	CHAR cmd[16];
	CHAR cmdlist[90];
	ULONG hasSpace = 0;
	ULONG completelyEqual = 0;
	ULONG CmdListCount = 0;
	cmdlist[0] = 0;

	i = 0;
	while(i < 15)
	{
		cmd[i] = cmdstring[i];
		if(cmd[i] == ' ')
		{
			hasSpace = TRUE;
			break;
		}
		if(cmd[i] == 0)
			break;
		i++;
	}
	cmd[i] = 0;

	if(!i)
	{
		PrintStr(1,GUI_Height-2,0,3,FALSE,"Enter a command (H for help)",TRUE);
		return;
	}

	for(i = 0; i < CmdCount; i++)
	{
		if(!vmm_strncmpi(CmdHelp[i].Cmd,cmd,strlen(cmd)))
		{
			if(strlen(cmdlist) + strlen(CmdHelp[i].Cmd) + 2 < 90)
			{
				strcat(cmdlist,CmdHelp[i].Cmd);
				strcat(cmdlist,", ");
			}
			CmdListCount++;
			if(strlen(CmdHelp[i].Cmd) == strlen(cmd))
			{
				iEqual = i;
				completelyEqual = TRUE;
			}
		}
	}
	cmdlist[strlen(cmdlist)-2] = 0;

	if(completelyEqual)
	{
		if(CmdListCount > 1 && !hasSpace)
		{
			PrintStr(1,GUI_Height-2,0,3,FALSE,cmdlist,TRUE);
		}
		else
		{
			if(hasSpace)
				PrintStr(1,GUI_Height-2,0,3,FALSE,CmdHelp[iEqual].Usage,TRUE);
			else
				PrintStr(1,GUI_Height-2,0,3,FALSE,CmdHelp[iEqual].Desc,TRUE);
		}

	}
	else
	{
		if(CmdListCount)
			PrintStr(1,GUI_Height-2,0,3,FALSE,cmdlist,TRUE);
		else
			PrintStr(1,GUI_Height-2,0,3,FALSE,"Invalid command",TRUE);
	}
}

VOID RefreshOldRegister(PGUEST_CPU pCpu)
{
    ULONG Eip = _ReadVMCS(GUEST_RIP);

    pCpu->Old_eax = pCpu->pGuestRegs->eax;
    pCpu->Old_ebx = pCpu->pGuestRegs->ebx;
    pCpu->Old_ecx = pCpu->pGuestRegs->ecx;
    pCpu->Old_edx = pCpu->pGuestRegs->edx;
    pCpu->Old_esi = pCpu->pGuestRegs->esi;
    pCpu->Old_edi = pCpu->pGuestRegs->edi;
    pCpu->Old_esp = pCpu->pGuestRegs->esp;
    pCpu->Old_ebp = pCpu->pGuestRegs->ebp;
    pCpu->Old_eip = Eip;
    pCpu->Old_eflags = _ReadVMCS(GUEST_RFLAGS);
}

VOID ShowRegister(PGUEST_CPU pCpu)
{
    UCHAR str[512];
    ULONG Eflags;
    PEFLAGS pEflags;
    PEFLAGS pOldEflags;
    ULONG x;
    ULONG Eip = _ReadVMCS(GUEST_RIP);
    PGUEST_REGS pGuestReg = pCpu->pGuestRegs;
    ULONG ForeColor;
    CHAR flag;

    sprintf(str,"%08X",pGuestReg->eax);
    PrintStr(1,1,7,0,FALSE,"EAX=",FALSE);
    if(pCpu->Old_eax != pGuestReg->eax)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintStr(5,1,ForeColor,0,FALSE,str,FALSE);

    sprintf(str,"%08X",pGuestReg->ebx);
    PrintStr(16,1,7,0,FALSE,"EBX=",FALSE);
    if(pCpu->Old_ebx != pGuestReg->ebx)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintStr(20,1,ForeColor,0,FALSE,str,FALSE);

    sprintf(str,"%08X",pGuestReg->ecx);
    PrintStr(31,1,7,0,FALSE,"ECX=",FALSE);
    if(pCpu->Old_ecx != pGuestReg->ecx)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintStr(35,1,ForeColor,0,FALSE,str,FALSE);

    sprintf(str,"%08X",pGuestReg->edx);
    PrintStr(46,1,7,0,FALSE,"EDX=",FALSE);
    if(pCpu->Old_edx != pGuestReg->edx)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintStr(50,1,ForeColor,0,FALSE,str,FALSE);

    sprintf(str,"%08X",pGuestReg->esi);
    PrintStr(61,1,7,0,FALSE,"ESI=",FALSE);
    if(pCpu->Old_esi != pGuestReg->esi)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintStr(65,1,ForeColor,0,FALSE,str,FALSE);

    sprintf(str,"%08X",pGuestReg->edi);
    PrintStr(1,2,7,0,FALSE,"EDI=",FALSE);
    if(pCpu->Old_edi != pGuestReg->edi)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintStr(5,2,ForeColor,0,FALSE,str,FALSE);

    sprintf(str,"%08X",pGuestReg->ebp);
    PrintStr(16,2,7,0,FALSE,"EBP=",FALSE);
    if(pCpu->Old_ebp != pGuestReg->ebp)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintStr(20,2,ForeColor,0,FALSE,str,FALSE);

    sprintf(str,"%08X",pGuestReg->esp);
    PrintStr(31,2,7,0,FALSE,"ESP=",FALSE);
    if(pCpu->Old_esp != pGuestReg->esp)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintStr(35,2,ForeColor,0,FALSE,str,FALSE);

    sprintf(str,"%08X",Eip);
    PrintStr(46,2,7,0,FALSE,"EIP=",FALSE);
    if(pCpu->Old_eip != Eip)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintStr(50,2,ForeColor,0,FALSE,str,FALSE);

    sprintf(str,"CS=%04X   DS=%04X   SS=%04X   ES=%04X   FS=%04X   GS=%04X",_ReadVMCS(GUEST_CS_SELECTOR),_ReadVMCS(GUEST_DS_SELECTOR),_ReadVMCS(GUEST_SS_SELECTOR),_ReadVMCS(GUEST_ES_SELECTOR),_ReadVMCS(GUEST_FS_SELECTOR),_ReadVMCS(GUEST_GS_SELECTOR));
    PrintStr(1,3,7,0,FALSE,str,FALSE);

    Eflags = _ReadVMCS(GUEST_RFLAGS);
    pEflags = (PEFLAGS)&Eflags;
    pOldEflags = (PEFLAGS)&pCpu->Old_eflags;

    x = 61;
    flag = 'o' - 0x20*pEflags->OF;
    if(pEflags->OF != pOldEflags->OF)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintChar(x,2,ForeColor,0,0,flag);

    x += 2;
    flag = 'd' - 0x20*pEflags->DF;
    if(pEflags->DF != pOldEflags->DF)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintChar(x,2,ForeColor,0,0,flag);

    x += 2;
    flag = 'i' - 0x20*pEflags->IF;
    if(pEflags->IF != pOldEflags->IF)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintChar(x,2,ForeColor,0,0,flag);

    x += 2;
    flag = 's' - 0x20*pEflags->SF;
    if(pEflags->SF != pOldEflags->SF)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintChar(x,2,ForeColor,0,0,flag);

    x += 2;
    flag = 'z' - 0x20*pEflags->ZF;
    if(pEflags->ZF != pOldEflags->ZF)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintChar(x,2,ForeColor,0,0,flag);

    x += 2;
    flag = 'a' - 0x20*pEflags->AF;
    if(pEflags->AF != pOldEflags->AF)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintChar(x,2,ForeColor,0,0,flag);

    x += 2;
    flag = 'p' - 0x20*pEflags->PF;
    if(pEflags->PF != pOldEflags->PF)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintChar(x,2,ForeColor,0,0,flag);

    x += 2;
    flag = 'c' - 0x20*pEflags->CF;
    if(pEflags->CF != pOldEflags->CF)
        ForeColor = 0xb;
    else
        ForeColor = 7;
    PrintChar(x,2,ForeColor,0,0,flag);

}

VOID ShowStatus(PGUEST_CPU pCpu)
{
    ULONG Ring;
    CHAR szSeparate2[100];
//    CHAR str[128];

    memset(szSeparate2,0,100);
    memset(szSeparate2,196,98);

    Ring = _ReadVMCS(GUEST_CS_SELECTOR) & 3;

//    sprintf(str,"pid: %d  comm: %s",current->pid,current->comm);
//    memcpy(szSeparate2,str,strlen(str));
    PrintStr(1,24,2,0,FALSE,szSeparate2,FALSE);
}

VOID SwBreakpointEvent(PGUEST_CPU pCpu,PSW_BP pSoftBp)
{
	ULONG ret;
	ULONG value;

	ClearCurrentDisasm();
	pCurrentSwBp = pSoftBp;
	*(PUCHAR)pSoftBp->Address = pSoftBp->OldOpcode;	//°ÑINT3»ÖžŽÎªÔ­ÀŽµÄÖžÁî

	if(strlen(pCurrentSwBp->IfCondition))			//ÓÐÌõŒþ±íŽïÊœ
	{
		ret = CalcExp(pCpu,pCurrentSwBp->IfCondition,&value);
		if(!ret || !value)
		{
			CmdSetSingleStep(pCpu);					//ÉèÖÃµ¥²œ±êÖŸ£¬ÏÂÒ»ÌõÖžÁî¶ÏÏÂÀŽµÄÊ±ºò»ÖžŽÕâžö#BP¶ÏµãµÄ INT3 OPCODE BYTE
			bShowGUIwhileSingleStepping = FALSE;	//ÏÂÒ»ŽÎµ¥²œ¶ÏÏÂÀŽ²»ÏÔÊŸœçÃæ
			return;									//ÌõŒþ²»Âú×ã ·µ»Ø
		}
	}

	if(strlen(pCurrentSwBp->DoCmd))
		DebugCommandHandler(pCpu,pCurrentSwBp->DoCmd);

	RefreshOldRegister(pCpu);
	EnterHyperDebugger(pCpu);	//œøÈëµ÷ÊÔœçÃæ
}

//#BPÖÐ¶ÏºóÀŽµœÕâÀï
VOID SwStepOverEvent(PGUEST_CPU pCpu)
{
	ClearStepBps();		//»ÖžŽËùÓÐStepBP¶Ïµã

	RefreshOldRegister(pCpu);
	EnterHyperDebugger(pCpu);	//œøÈëµ÷ÊÔœçÃæ
}

VOID CmdSetStepBreakpoint(PGUEST_CPU pCpu,ULONG Address)
{
	ULONG i;
	PSW_BP pStepBp;

	for(i = 0; i < MAX_STEP_BP; i++)
	{
		if(!StepBPs[i].ProcessCR3)
		{
			pStepBp = &StepBPs[i];
			break;
		}
	}

	if(i != MAX_STEP_BP && IsAddressRangeExist(Address,1))
	{
		pStepBp->ProcessCR3 = _ReadVMCS(GUEST_CR3);
		pStepBp->CodeSeg = _ReadVMCS(GUEST_CS_SELECTOR);
		pStepBp->Address = Address;
		pStepBp->OldOpcode = *(PUCHAR)(pStepBp->Address);
		*(PUCHAR)(pStepBp->Address) = INT3_OPCODE;
		bNeedRestoreScreen = FALSE;
		RefreshOldRegister(pCpu);
		return;
	}
	else
	{
		ConsolePrintStr("What the fuck?! StepOver Limit!\n",7,0);
		return;
	}
}

VOID CmdStepOver(PGUEST_CPU pCpu)
{
	UCHAR str[128];
	ULONG Eip = _ReadVMCS(GUEST_RIP);
	ULONG Len;

	Len = OriDisasm(str,Eip);
	if(Len != -1)
	{
		if(!strncmp(str,"call ",5) || !strncmp(str,"rep ",4) || !strncmp(str,"loop ",5))
		{
			CmdSetStepBreakpoint(pCpu,Eip+Len);
		}
		else
		{
			CmdSetSingleStep(pCpu);
		}
	}
}

VOID ClearStepBps(void)
{
	ULONG i;
	PSW_BP pStepBp;

	for(i = 0; i < MAX_STEP_BP; i++)
	{
		pStepBp = &StepBPs[i];

		if(pStepBp->ProcessCR3)
		{
			//OldCR3 = AttachTargetProcess(pStepBp->ProcessCR3);

			if(IsAddressExist(pStepBp->Address))
				*(PUCHAR)pStepBp->Address = pStepBp->OldOpcode;

			memset(pStepBp,0,sizeof(SW_BP));
			//DetachTargetProcess(OldCR3);
		}
	}
}

VOID CmdSetSingleStep(PGUEST_CPU pCpu)
{
	ULONG Eflags = _ReadVMCS(GUEST_RFLAGS);
	ULONG Eip = _ReadVMCS(GUEST_RIP);

	bSingleStepPushfd = FALSE;
	if(!(Eflags & FLAGS_TF_MASK))
	{
		if(IsAddressRangeExist(Eip,1) && *(PUCHAR)Eip == PUSHFD_OPCODE)
		{
			bSingleStepPushfd = TRUE;
		}
		if(IsAddressRangeExist(Eip,2) && *(PUSHORT)Eip == SYSEXIT_OPCODE)
		{
 			CmdSetStepBreakpoint(pCpu,pCpu->pGuestRegs->edx);
		}
		else
		{
			_WriteVMCS(GUEST_INTERRUPTIBILITY_INFO, 0);
			_WriteVMCS(GUEST_ACTIVITY_STATE, 0);
		}
		Eflags |= FLAGS_TF_MASK;
		_WriteVMCS(GUEST_RFLAGS,Eflags);
	}

	bSingleStepping = TRUE;
	pCpu->bSingleStepping = TRUE;
	bShowGUIwhileSingleStepping = TRUE;
	bNeedRestoreScreen = FALSE;
	RefreshOldRegister(pCpu);
}

VOID DebugCommandHandler(PGUEST_CPU pCpu,PUCHAR cmd)
{
	ULONG i;
	PCMD_HELP pCmdHelp;

	for(i = 0; i < CmdCount; i++)
	{
		pCmdHelp = &CmdHelp[i];

		if(!vmm_strncmpi(pCmdHelp->Cmd,cmd,strlen(pCmdHelp->Cmd)) && (cmd[strlen(pCmdHelp->Cmd)] == ' ' || cmd[strlen(pCmdHelp->Cmd)] == 0))
		{
			if(pCmdHelp->pHandler)
			{
				pCmdHelp->pHandler(pCpu,cmd);
			}
			break;
		}
	}
}

ULONG InsertChar(PCHAR str,ULONG pos,CHAR c)
{
    CHAR temp[128];
    ULONG len = strlen(str);

    if(len > 95 || pos > len)
        return FALSE;

    strcpy(temp,&str[pos]);
    str[pos] = c;
    str[pos+1] = 0;
    strcat(str,temp);
    return TRUE;
}

VOID DeleteChar(PCHAR str,ULONG pos)
{
    CHAR temp[128];
    ULONG len = strlen(str);

    if(pos >= len)
        return;

    strcpy(temp,&str[pos+1]);
    str[pos] = 0;
    strcat(str,temp);
}

VOID InsertCmdHistory(PCHAR cmd)
{
    if(iCmdHistoryPos < MAX_CMD_HISTORY)    //0-19
    {
        strcpy(szCmdHistory[iCmdHistoryPos],cmd);
        iCmdHistoryPos++;
    }
    else                                    //20
    {
        memcpy(szCmdHistory[0],szCmdHistory[1],(MAX_CMD_HISTORY-1)*128);
        strcpy(szCmdHistory[MAX_CMD_HISTORY-1],cmd);
    }
}

extern ULONG TSCEvery1us;

VOID EnterCommandLoop(PGUEST_CPU pCpu)
{
    UCHAR c,k;
    ULONG scancode = 0;
    BOOLEAN isMouse;
    ULONG x,y;
    ULONG cursorState;
    UCHAR cmd[128];
    ULONG bExitLoop = 0;
    ULONG iCmdHistoryPosNow = 0;
    ULONG bCmdHistoryPaging = FALSE;
    ULONG ShiftState = 0;
    ULONG CtrlState = 0;
    ULONG AltState = 0;

    _ScaleTSCBasedTimer();  //for _CpuSleep
    //sprintf(str,"TSCEvery1us %d\n",TSCEvery1us);
    //ConsolePrintStr(str,7,0);

    x = 1;
    y = GUI_Height - 3;
    cursorState = 1;
    memset(cmd,0,128);
    ShowCmdTip(cmd);

    PrintChar(x,y,7,0,FALSE,':');
    x++;

    while (1)
    {
        _CpuSleep(1000);
        if(cursorState == 1)
        {
            DrawCursor(x,y,FALSE,TRUE);
            cursorState = 0;
        }
        else
        {
            DrawCursor(x,y,FALSE,FALSE);
            cursorState = 1;
            PrintStr(2,y,7,0,FALSE,cmd,TRUE);
        }

        if(!(KeyboardReadKeystroke(&c, &isMouse)))
        {
            continue;
        }

        if(isMouse)
            continue;

        if(c == SCANCODE_F11)
            PrintScreen();

        if(c == SCANCODE_F12)
            break;

        if(c == EXTEND_CODE)
            scancode = c << 8;
        else
            scancode += c;

        switch(scancode)
        {
        case EXTEND_CODE << 8:
            break;

        case SCANCODE_LSHIFT:
        case SCANCODE_RSHIFT:
            ShiftState = TRUE;
            break;

        case SCANCODE_LSHIFT + BREAK_CODE:
        case SCANCODE_RSHIFT + BREAK_CODE:
            ShiftState = FALSE;
            break;

        case SCANCODE_LCTRL:
        case SCANCODE_RCTRL:
            CtrlState = TRUE;
            break;

        case SCANCODE_LCTRL + BREAK_CODE:
        case SCANCODE_RCTRL + BREAK_CODE:
            CtrlState = FALSE;
            break;

        case SCANCODE_LALT:
        case SCANCODE_RALT:
            AltState = TRUE;
            break;

        case SCANCODE_LALT + BREAK_CODE:
        case SCANCODE_RALT + BREAK_CODE:
            AltState = FALSE;
            break;

        case SCANCODE_UP:
            if(ShiftState)
            {
                ConsolePrintPreviousPage();
                break;
            }
            else if(CtrlState)
            {
                ShowPrevIpDisasm(pCpu);
                break;
            }
            else
            {
                if(iCmdHistoryPos)
                {
                    if(!bCmdHistoryPaging)
                    {
                        iCmdHistoryPosNow = iCmdHistoryPos-1;
                        bCmdHistoryPaging = TRUE;
                    }
                    else
                    {
                        if(iCmdHistoryPosNow)
                            iCmdHistoryPosNow--;
                    }
                    strcpy(cmd,szCmdHistory[iCmdHistoryPosNow]);
                    PrintStr(2,y,7,0,FALSE,cmd,TRUE);
                    x = strlen(cmd) + 2;
                }
            }
            break;

        case SCANCODE_DOWN:
            if(ShiftState)
            {
                ConsolePrintNextPage();
                break;
            }
            else if(CtrlState)
            {
                ShowNextIpDisasm(pCpu);
                break;
            }
            else
            {
                if(iCmdHistoryPos)
                {
                    if(!bCmdHistoryPaging)
                    {
                        iCmdHistoryPosNow = iCmdHistoryPos-1;
                        bCmdHistoryPaging = TRUE;
                    }
                    else
                    {
                        if(iCmdHistoryPosNow < iCmdHistoryPos-1)
                            iCmdHistoryPosNow++;
                    }
                    strcpy(cmd,szCmdHistory[iCmdHistoryPosNow]);
                    PrintStr(2,y,7,0,FALSE,cmd,TRUE);
                    x = strlen(cmd) + 2;
                }
            }
            break;

        case SCANCODE_F7:
            ClearStepBps();
            CmdSetSingleStep(pCpu);
            bExitLoop = TRUE;
            break;

        case SCANCODE_F8:
            CmdStepOver(pCpu);
            bExitLoop = TRUE;
            break;

        case SCANCODE_F9:
            //CmdExecuteUntilReturn(pCpu);
            bExitLoop = TRUE;
            break;

        case SCANCODE_F5:
            ConsolePrintStr("123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\n",7,0);
            ConsolePrintStr("123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890\n",7,0);
            break;

        case SCANCODE_F6:
            break;

        case SCANCODE_LEFT:
            if(x > 2)
            {
                DrawCursor(x,y,FALSE,FALSE);
                x--;
                PrintStr(2,y,7,0,FALSE,cmd,TRUE);
            }
            break;

        case SCANCODE_RIGHT:
            if(x-2 < strlen(cmd))
            {
                DrawCursor(x,y,FALSE,FALSE);
                x++;
                PrintStr(2,y,7,0,FALSE,cmd,TRUE);
            }
            break;

        case SCANCODE_DEL:
            if(x-2 < strlen(cmd))
            {
                DeleteChar(cmd,x-2);
                PrintStr(2,y,7,0,FALSE,cmd,TRUE);
                ShowCmdTip(cmd);
            }
            break;

        default:
            break;
        }

        if(bExitLoop)
            break;

        if(!ShiftState)
            k = ScancodeToAscii_NonShift(scancode);
        else
            k = ScancodeToAscii_Shift(scancode);

        if(scancode != 0xE000)
            scancode = 0;

        if(k)
        {
            bCmdHistoryPaging = FALSE;
        }

        switch(k)
        {
        case 0:
            break;

        case '\b':
            if(x > 2)
            {
                DrawCursor(x,y,FALSE,FALSE);
                x--;
                DeleteChar(cmd,x-2);
                PrintStr(2,y,7,0,FALSE,cmd,TRUE);
                ShowCmdTip(cmd);
            }
            break;

        case '\n':
            //snprintf(str,128,":%s\n",cmd);
            //ConsolePrintStr(str,7,0);
            ConsolePrintStr(":",7,0);
            ConsolePrintStr(cmd,7,0);
            ConsolePrintStr(" \n",7,0);

            while(x != 2)
            {
                PrintChar(x,y,7,0,0,' ');
                x--;
            }
            PrintChar(x,y,7,0,0,' ');

            InsertCmdHistory(cmd);
            DebugCommandHandler(pCpu,cmd);
            memset(cmd,0,128);
            ShowCmdTip(cmd);
            break;

        default:
            if(x < GUI_Width-2)
            {
                if(InsertChar(cmd,x-2,k))
                {
                    PrintStr(2,y,7,0,FALSE,cmd,TRUE);
                    x++;
                    ShowCmdTip(cmd);
                }
            }
            break;
        }
    }
}

VOID EnterHyperDebugger(PGUEST_CPU pCpu)
{
    //sprintf(str,"EIP=%08x\n",_ReadVMCS(GUEST_RIP));
    //ConsolePrintStr(str,7,0);
    MaskKeyboardInterrupt(pCpu);

    bNeedRestoreScreen = TRUE;
    if(!bScreenBackuped)
    {
        BackupScreen();
        bScreenBackuped = TRUE;
        DrawBackground();
    }

    bSingleStepping = FALSE;
    pCpu->bSingleStepping = FALSE;

    DrawBorder();
    ShowRegister(pCpu);
    ShowCurrentDisasm(pCpu);
    ShowStatus(pCpu);

    if(pCurrentSwBp)				//ŽÓ#BPÖÐ¶ÏœøÀŽµÄ£¿
	{
	    const char *Ret;
        char SymName[256];
        char *pModName;
        unsigned long SymSize;
        unsigned long SymOffset;
		CHAR SymbolOffset[128];
        CHAR str[128];

		Ret = LookupAddress((int)pCurrentSwBp->Address,&SymSize,&SymOffset,&pModName,SymName);
        if(Ret)
            snprintf(SymbolOffset,128,"%s!%s + %04X",pModName,SymName,(ULONG)SymOffset);
        else
            strcpy(SymbolOffset,"unknown symbol");

		snprintf(str,128,"Break due to BPX %04X:%08X  %s\n",pCurrentSwBp->CodeSeg,pCurrentSwBp->Address,SymbolOffset);
		ConsolePrintStr(str,2,0);

		CmdSetSingleStep(pCpu);					//ÉèÖÃµ¥²œ±êÖŸ£¬ÏÂÒ»ÌõÖžÁî¶ÏÏÂÀŽµÄÊ±ºò»ÖžŽÕâžö#BP¶ÏµãµÄ INT3 OPCODE BYTE
		bShowGUIwhileSingleStepping = FALSE;	//ÏÂÒ»ŽÎµ¥²œ¶ÏÏÂÀŽ²»ÏÔÊŸœçÃæ
		bNeedRestoreScreen = TRUE;				//CmdSetSingleStepÀïÃæ bNeedRestoreScreen = FALSEÁË
	}

    ConsolePrintCurrentPage();

    EnterCommandLoop(pCpu);

    if(bNeedRestoreScreen)
    {
        RestoreScreen();
        bScreenBackuped = FALSE;
    }
    RestoreKeyboardInterrupt(pCpu);
    bDebuggerBreak = FALSE;
}

VOID CmdDisplayHelp(PGUEST_CPU pCpu,PCHAR cmd)
{
	ULONG i;
	UCHAR c;
	ULONG nextPage = 12;
	ULONG exitLoop = 0;
	CHAR p1[128];
	ULONG r1;

	r1 = GetCommandParam(cmd,p1,1);

	if(r1 && strlen(p1))
	{
		for(i = 0; i < CmdCount; i++)
		{
			if(!vmm_strcmpi(CmdHelp[i].Cmd,p1))
			{
				ConsolePrintStr(CmdHelp[i].Desc,7,0);
				ConsolePrintStr("\n",7,0);

				ConsolePrintStr(CmdHelp[i].Usage,7,0);
				ConsolePrintStr("\n",7,0);

				if(CmdHelp[i].Example)
				{
					ConsolePrintStr("\nEx: ",7,0);
					ConsolePrintStr(CmdHelp[i].Example,7,0);
				}

				break;
			}
		}

		return;
	}

	i = 0;
	while(i < HelpInfoCount)
	{
		if(i >= nextPage)
		{
			c = GetConsolePageControlKey();
			switch(c)
			{
			case SCANCODE_ESC:
				exitLoop = TRUE;
				break;

			case SCANCODE_ENTER:
				break;

			default:
				nextPage += 12;
				break;
			}
			if(exitLoop)
				break;
		}
		ConsolePrintStr(pHelpInfo[i].pszLine,pHelpInfo[i].ForeColor,pHelpInfo[i].BackColor);
		i++;
	}
	ShowCmdTip("");
}

VOID CmdCalcExp(PGUEST_CPU pCpu,PCHAR cmd)
{
	CHAR str[128];
	ULONG value;
	ULONG ret;
	ULONG b;
	CHAR szByte[32];
	CHAR szTemp[32];
	LONG i;

	ret = CalcExp(pCpu,&cmd[2],&value);
	if(!ret)
	{
		ConsolePrintStr("Invalid expression\n",7,0);
		return;
	}

	szByte[0] = 0;
	for(i = 3; i >= 0; i--)
	{
		b = ((PUCHAR)&value)[i];
		if(b)
		{
			snprintf(szTemp,2,"%c",b);
			strcat(szByte,szTemp);
		}
	}

	snprintf(str,128,"0x%08X, %d, %hu, '%s' \n",value,value,value,szByte);
	ConsolePrintStr(str,7,0);
}


ULONG ReadByte(ULONG Address,PULONG Value)
{
	if(!IsAddressExist(Address))
		return FALSE;

	*Value = *(PUCHAR)(Address);
	return TRUE;
}

ULONG ReadWord(ULONG Address,PULONG Value)
{
	if(!IsAddressExist(Address))
		return FALSE;

	*Value = *(PUSHORT)(Address);
	return TRUE;
}

ULONG ReadDword(ULONG Address,PULONG Value)
{
	if(!IsAddressExist(Address))
		return FALSE;

	*Value = *(PULONG)(Address);
	return TRUE;
}

VOID CmdDisplayMemoryByte(PGUEST_CPU pCpu,PCHAR cmd)
{
	CHAR str[128];
	CHAR p1[128];
	ULONG r1;
	ULONG address;
	ULONG Value;
	ULONG dRet;
	ULONG ret;
	ULONG i,j;

	r1 = GetCommandParam(cmd,p1,1);

	if(!r1 || !strlen(p1))
	{
		ConsolePrintStr("need param.\n",7,0);
		return;
	}

	ret = CalcExp(pCpu,p1,&address);
	if(!ret)
	{
		ConsolePrintStr("Invalid param\n",7,0);
		return;
	}

	for(i = 0; i < 8; i++)
	{
		snprintf(str,128,"%04X:%08X  ",_ReadVMCS(GUEST_DS_SELECTOR),address);
		ConsolePrintStr(str,7,0);

		for(j = 0; j < 16; j++)
		{
			dRet = ReadByte(address+j,&Value);
			if(dRet)
				snprintf(str,128,"%02X ",Value);
			else
				strcpy(str,"?? ");
			ConsolePrintStr(str,7,0);
		}

		ConsolePrintStr(" ",7,0);

		for(j = 0; j < 16; j++)
		{
			dRet = ReadByte(address+j,&Value);
			if(dRet)
				ConsolePrintChar((UCHAR)Value,7,0);
			else
				ConsolePrintChar('?',7,0);

		}
		ConsolePrintStr("\n",0,0);
		address += 16;
	}
}

VOID CmdDisplayMemoryWord(PGUEST_CPU pCpu,PCHAR cmd)
{
	CHAR str[128];
	CHAR p1[128];
	ULONG r1;
	ULONG address;
	ULONG Value;
	ULONG dRet;
	ULONG ret;
	ULONG i,j;

	r1 = GetCommandParam(cmd,p1,1);

	if(!r1 || !strlen(p1))
	{
		ConsolePrintStr("need param.\n",7,0);
		return;
	}

	ret = CalcExp(pCpu,p1,&address);
	if(!ret)
	{
		ConsolePrintStr("Invalid param\n",7,0);
		return;
	}

	for(i = 0; i < 8; i++)
	{
		snprintf(str,128,"%04X:%08X  ",_ReadVMCS(GUEST_DS_SELECTOR),address);
		ConsolePrintStr(str,7,0);

		for(j = 0; j < 8; j++)
		{
			dRet = ReadWord(address+j*2,&Value);
			if(dRet)
				snprintf(str,128,"%04X ",Value);
			else
				strcpy(str,"???? ");
			ConsolePrintStr(str,7,0);
		}

		ConsolePrintStr(" ",7,0);

		for(j = 0; j < 16; j++)
		{
			dRet = ReadByte(address+j,&Value);
			if(dRet)
				ConsolePrintChar((UCHAR)Value,7,0);
			else
				ConsolePrintChar('?',7,0);

		}
		ConsolePrintStr("\n",0,0);
		address += 16;
	}
}

VOID CmdDisplayMemoryDword(PGUEST_CPU pCpu,PCHAR cmd)
{
	CHAR str[128];
	CHAR p1[128];
	ULONG r1;
	ULONG address;
	ULONG Value;
	ULONG dRet;
	ULONG ret;
	ULONG i,j;

	r1 = GetCommandParam(cmd,p1,1);

	if(!r1 || !strlen(p1))
	{
		ConsolePrintStr("need param.\n",7,0);
		return;
	}

	ret = CalcExp(pCpu,p1,&address);
	if(!ret)
	{
		ConsolePrintStr("Invalid param\n",7,0);
		return;
	}

	for(i = 0; i < 8; i++)
	{
		snprintf(str,128,"%04X:%08X  ",_ReadVMCS(GUEST_DS_SELECTOR),address);
		ConsolePrintStr(str,7,0);

		for(j = 0; j < 4; j++)
		{
			dRet = ReadDword(address+j*4,&Value);
			if(dRet)
				snprintf(str,128,"%08X ",Value);
			else
				strcpy(str,"???????? ");
			ConsolePrintStr(str,7,0);
		}

		ConsolePrintStr(" ",7,0);

		for(j = 0; j < 16; j++)
		{
			dRet = ReadByte(address+j,&Value);
			if(dRet)
				ConsolePrintChar((UCHAR)Value,7,0);
			else
				ConsolePrintChar('?',7,0);

		}
		ConsolePrintStr("\n",0,0);
		address += 16;
	}
}

VOID CmdDisplayIdt(PGUEST_CPU pCpu,PCHAR cmd)
{
	PIDT_ENTRY pGuestIdt = (PIDT_ENTRY)_ReadVMCS(GUEST_IDTR_BASE);
	ULONG GuestIdtLimit = _ReadVMCS(GUEST_IDTR_LIMIT);
	ULONG i,iLine,nextPage;
	CHAR str[128];
	CHAR SymbolOffset[128];
	CHAR c;
	ULONG exitLoop = 0;
	CHAR GateType[8];
	CHAR Attributes[10];
	ULONG Offset;
    const char *Ret;
    char SymName[256];
    char *pModName;
    unsigned long SymSize;
    unsigned long SymOffset;

	snprintf(str,128,"IDTBase:%08X  IDTLimit:%04X\n",(ULONG)pGuestIdt,GuestIdtLimit);
	ConsolePrintStr(str,7,0);
	ConsolePrintStr("Int   Type      Sel:Offset     Attributes  Symbol                                        \n",1,7);
	iLine = 2;
	nextPage = 12;
	for(i = 0; i < (GuestIdtLimit+1)/sizeof(IDT_ENTRY); i++,iLine++)
	{
		switch(pGuestIdt[i].GateType)
		{
		case 5:
			strcpy(GateType,"TaskG");
			break;
		case 6:
			strcpy(GateType,"IntG");
			break;
		case 7:
			strcpy(GateType,"TrapG");
			break;
		default:
			strcpy(GateType,"????G");
			break;
		}

		if(pGuestIdt[i].GateSize)
			strcat(GateType,"32");

		Offset = pGuestIdt[i].LowOffset | (pGuestIdt[i].HighOffset << 16);
		Ret = LookupAddress((int)Offset,&SymSize,&SymOffset,&pModName,SymName);
        if(Ret)
            snprintf(SymbolOffset,128,"%s!%s + %04X",pModName,SymName,(ULONG)SymOffset);
        else
            strcpy(SymbolOffset,"unknown symbol");

		snprintf(Attributes,10,"DPL=%d P=%d",pGuestIdt[i].DPL,pGuestIdt[i].P);

		if(iLine >= nextPage)
		{
			c = GetConsolePageControlKey();
			switch(c)
			{
			case SCANCODE_ESC:
				exitLoop = TRUE;
				break;

			case SCANCODE_ENTER:
				break;

			default:
				nextPage += 12;
				break;
			}
			if(exitLoop)
				break;
		}
		snprintf(str,128,"%04X  %-8s  %04X:%08X  %-10s  %s\n",i,GateType,pGuestIdt[i].Selector,Offset,Attributes,SymbolOffset);
		ConsolePrintStr(str,7,0);
	}
	ShowCmdTip("");
}

VOID CmdDisplayCpuReg(PGUEST_CPU pCpu,PCHAR cmd)
{
	CHAR str[128];
	ULONG Cr0;
	PCR0 pCr0;
	ULONG Cr4;
	PCR4 pCr4;

	snprintf(str,128,"CS:EIP=%04X:%08X  SS:ESP=%04X:%08X\n",_ReadVMCS(GUEST_CS_SELECTOR),_ReadVMCS(GUEST_RIP),_ReadVMCS(GUEST_SS_SELECTOR),pCpu->pGuestRegs->esp);
	ConsolePrintStr(str,7,0);
	snprintf(str,128,"EAX=%08X  EBX=%08X  ECX=%08X  EDX=%08X\n",pCpu->pGuestRegs->eax,pCpu->pGuestRegs->ebx,pCpu->pGuestRegs->ecx,pCpu->pGuestRegs->edx);
	ConsolePrintStr(str,7,0);
	snprintf(str,128,"ESI=%08X  EDI=%08X  EBP=%08X  EFL=%08X\n \n",pCpu->pGuestRegs->esi,pCpu->pGuestRegs->edi,pCpu->pGuestRegs->ebp,_ReadVMCS(GUEST_RFLAGS));
	ConsolePrintStr(str,7,0);

	Cr0 = _ReadVMCS(GUEST_CR0);
	pCr0 = (PCR0)&Cr0;

	snprintf(str,128,"CR0=%08X  ",Cr0);
	ConsolePrintStr(str,7,0);
	str[0] = 0;

	if(pCr0->PE)
		strcat(str,"PE ");

	if(pCr0->MP)
		strcat(str,"MP ");

	if(pCr0->EM)
		strcat(str,"EM ");

	if(pCr0->TS)
		strcat(str,"TS ");

	if(pCr0->ET)
		strcat(str,"ET ");

	if(pCr0->NE)
		strcat(str,"NE ");

	if(pCr0->WP)
		strcat(str,"WP ");

	if(pCr0->AM)
		strcat(str,"AM ");

	if(pCr0->NW)
		strcat(str,"NW ");

	if(pCr0->CD)
		strcat(str,"CD ");

	if(pCr0->PG)
		strcat(str,"PG ");

	strcat(str,"\n");
	ConsolePrintStr(str,7,0);

	snprintf(str,128,"CR2=%08X  CR3=%08X\n",_CR2(),_ReadVMCS(GUEST_CR3));
	ConsolePrintStr(str,7,0);

	Cr4 = _ReadVMCS(GUEST_CR4);
	pCr4 = (PCR4)&Cr4;
	snprintf(str,128,"CR4=%08X  ",Cr4);
	ConsolePrintStr(str,7,0);
	str[0] = 0;

	if(pCr4->VME)
		strcat(str,"VME ");

	if(pCr4->PVI)
		strcat(str,"PVI ");

	if(pCr4->TSD)
		strcat(str,"TSD ");

	if(pCr4->DE)
		strcat(str,"DE ");

	if(pCr4->PSE)
		strcat(str,"PSE ");

	if(pCr4->PAE)
		strcat(str,"PAE ");

	if(pCr4->MCE)
		strcat(str,"MCE ");

	if(pCr4->PGE)
		strcat(str,"PGE ");

	if(pCr4->PCE)
		strcat(str,"PCE ");

	if(pCr4->OSFXSR)
		strcat(str,"OSFXSR ");

	if(pCr4->OSXMMEXCPT)
		strcat(str,"OSXMMEXCPT ");

	if(pCr4->VMXE)
		strcat(str,"VMXE ");

	if(pCr4->SMXE)
		strcat(str,"SMXE ");

	if(pCr4->PCIDE)
		strcat(str,"PCIDE ");

	if(pCr4->OSXSAVE)
		strcat(str,"OSXSAVE ");

	if(pCr4->SMEP)
		strcat(str,"SMEP ");

	strcat(str,"\n \n");
	ConsolePrintStr(str,7,0);

	snprintf(str,128,"DR0=%08X  DR1=%08X  DR2=%08X  DR3=%08X\n",_DR0(),_DR1(),_DR2(),_DR3());
	ConsolePrintStr(str,7,0);
	snprintf(str,128,"DR6=%08X\n",_DR6());
	ConsolePrintStr(str,7,0);
	snprintf(str,128,"DR7=%08X\n",_ReadVMCS(GUEST_DR7));
	ConsolePrintStr(str,7,0);
}

VOID FormatAttributesPde(PCHAR pBuf,ULONG64 PdeEntry)
{
	ULONG Cr4 = _ReadVMCS(GUEST_CR4);

	*pBuf = 0;

	if(PdeEntry & (1 << 0))
		strcat(pBuf,"P ");

	if(PdeEntry & (1 << 1))
		strcat(pBuf,"RW ");
	else
		strcat(pBuf,"R ");

	if(PdeEntry & (1 << 2))
		strcat(pBuf,"S ");
	else
		strcat(pBuf,"U ");

	if(PdeEntry & (1 << 3))
		strcat(pBuf,"PWT ");

	if(PdeEntry & (1 << 4))
		strcat(pBuf,"PCD ");

	if(PdeEntry & (1 << 5))
		strcat(pBuf,"A ");

	if(PdeEntry & (1 << 6))
		strcat(pBuf,"D ");


	if(PdeEntry & (1 << 7))
	{
		if(Cr4 & X86_CR4_PAE)
			strcat(pBuf,"2MB ");
		else
			strcat(pBuf,"4MB ");
	}



	if(PdeEntry & (1 << 8))
		strcat(pBuf,"G ");

	if(PdeEntry & (1 << 12))
		strcat(pBuf,"PAT ");
}

VOID FormatAttributesPte(PCHAR pBuf,ULONG64 PteEntry)
{
	*pBuf = 0;

	if(PteEntry & (1 << 0))
		strcat(pBuf,"P ");

	if(PteEntry & (1 << 1))
		strcat(pBuf,"RW ");
	else
		strcat(pBuf,"R ");

	if(PteEntry & (1 << 2))
		strcat(pBuf,"S ");
	else
		strcat(pBuf,"U ");

	if(PteEntry & (1 << 3))
		strcat(pBuf,"PWT ");

	if(PteEntry & (1 << 4))
		strcat(pBuf,"PCD ");

	if(PteEntry & (1 << 5))
		strcat(pBuf,"A ");

	if(PteEntry & (1 << 6))
		strcat(pBuf,"D ");

	if(PteEntry & (1 << 7))
		strcat(pBuf,"PAT ");

	if(PteEntry & (1 << 8))
		strcat(pBuf,"G ");
}

VOID NonPaeGetPageInfo(ULONG TargetVirtAddr)
{
	ULONG PhysAddr;
	ULONG Pte;
	ULONG Pde;
	CHAR str[128];
	CHAR Attributes[32];

	Pde = GetPde(_CR3(),TargetVirtAddr);
	if(!Pde)
	{
		ConsolePrintStr("Page is not exist.\n",7,0);
		return;
	}

	if(Pde & 1)
	{
		if(Pde & (1<<7))
		{
			PhysAddr = Pde & ~0x3FFFFF;
			PhysAddr |= TargetVirtAddr & 0x3FFFFF;
			FormatAttributesPde(Attributes,Pde);

			ConsolePrintStr("LinearAddress  PhysicalAddress  Attributes                 \n",7,0);
			snprintf(str,128,"%08X       %08X         %s\n",TargetVirtAddr,PhysAddr,Attributes);
			ConsolePrintStr(str,7,0);
			return;
		}
		else
		{
			Pte = GetPte(_CR3(),TargetVirtAddr);
			if(!Pte)
			{
				ConsolePrintStr("Page is not exist.\n",7,0);
				return;
			}

			if(Pte & 1)
			{
				PhysAddr = Pte & 0xFFFFF000;
				PhysAddr |= TargetVirtAddr & 0xFFF;
				FormatAttributesPte(Attributes,Pte);

				ConsolePrintStr("LinearAddress  PhysicalAddress  Attributes                 \n",7,0);
				snprintf(str,128,"%08X       %08X         %s\n",TargetVirtAddr,PhysAddr,Attributes);
				ConsolePrintStr(str,7,0);
				return;
			}
			else
			{
				ConsolePrintStr("Page is not present.\n",7,0);
				return;
			}
		}
	}
	else
	{
		ConsolePrintStr("Page is not present.\n",7,0);
		return;
	}
}

VOID CmdGetPageInfo(PGUEST_CPU pCpu,PCHAR cmd)
{
	ULONG address;
	BOOLEAN ret;

	ret = vmm_strtoul(&cmd[5],&address);
	if(!ret)
	{
		ConsolePrintStr("Invalid param\n",7,0);
		return;
	}

	NonPaeGetPageInfo(address);
}

VOID GetAllVirtualAddress(ULONG PhysAddress)
{
    ULONG PhysBaseRet;
    ULONG PhysBase = PhysAddress & 0xfffff000;
    ULONG PageOffset = PhysAddress & 0xfff;
    ULONG VirtBase;
    ULONG VirtAddress;
    ULONG Ret;

    ConsolePrintStr("LinearAddress  PhysicalAddress\n",7,0);
    for(VirtBase = 0x1000; VirtBase < 0xfffff000; VirtBase += 0x1000)
    {
        Ret = GetPhysAddress(_CR3(),VirtBase,&PhysBaseRet);
        if(Ret && PhysBaseRet == PhysBase)
        {
            UCHAR str[128];
            VirtAddress = VirtBase + PageOffset;
            snprintf(str,128,"%08X       %08X       \n",VirtAddress,PhysAddress);
            ConsolePrintStr(str,7,0);
        }
    }
}

VOID CmdGetVirtAddr(PGUEST_CPU pCpu,PCHAR cmd)
{
	ULONG64 address;
	BOOLEAN ret;

	ret = vmm_strtoul_64(&cmd[5],&address);
	if(!ret)
	{
		ConsolePrintStr("Invalid param\n",7,0);
		return;
	}

	GetAllVirtualAddress(address);
}

VOID CmdDisplayDisasm(PGUEST_CPU pCpu,PCHAR cmd)
{
	CHAR p1[128];
	ULONG r1;
	ULONG address;
	ULONG ret;

	r1 = GetCommandParam(cmd,p1,1);

	if(!r1 || !strlen(p1))
	{
		ConsolePrintStr("need param.\n",7,0);
		return;
	}

	ret = CalcExp(pCpu,p1,&address);
	if(!ret)
	{
		ConsolePrintStr("Invalid param\n",7,0);
		return;
	}

	ShowDisasm(pCpu,address,TRUE);
}

VOID CmdDisplayCallstack(PGUEST_CPU pCpu,PCHAR cmd)
{
	ULONG EbpReg = pCpu->pGuestRegs->ebp;
	ULONG RetEip;
	CHAR str[128];
	CHAR SymbolOffset[128];
	ULONG i,nextPage,exitLoop,c;
    const char *Ret;
    char SymName[256];
    char *pModName;
    unsigned long SymSize;
    unsigned long SymOffset;

	ConsolePrintStr("FrameEBP  RetEIP    Symbol                                        \n",1,7);
	i = 1;
	nextPage = 12;
	while(EbpReg)
	{
		if(!IsAddressExist(EbpReg))
			break;

		if(!IsAddressExist(EbpReg+4))
			break;

		if(i >= nextPage)
		{
			c = GetConsolePageControlKey();
			switch(c)
			{
			case SCANCODE_ESC:
				exitLoop = TRUE;
				break;

			case SCANCODE_ENTER:
				break;

			default:
				nextPage += 12;
				break;
			}
			if(exitLoop)
				break;
		}

		RetEip = *(PULONG)(EbpReg+4);
		if(!RetEip)
			break;

        Ret = LookupAddress((int)RetEip,&SymSize,&SymOffset,&pModName,SymName);
        if(Ret)
            snprintf(SymbolOffset,128,"%s!%s + %04X",pModName,SymName,(ULONG)SymOffset);
        else
            strcpy(SymbolOffset,"unknown symbol");

		snprintf(str,128,"%08X  %08X  %s\n",EbpReg,RetEip,SymbolOffset);
		ConsolePrintStr(str,7,0);

		if(!IsAddressExist(EbpReg+4))
			break;
		EbpReg = *((PULONG)EbpReg);
		i++;
	}
}

VOID CmdClearBreakpoint(PGUEST_CPU pCpu,PCHAR cmd)
{
	BOOLEAN ret;
	ULONG bp_id;
	PSW_BP pSoftBp;
	ULONG i;

	if(cmd[3] == '*')
	{
		for(i = 0; i < MAX_SW_BP; i++)
		{
			pSoftBp = &SoftBPs[i];

			if(pSoftBp->ProcessCR3)
			{
				//OldCR3 = AttachTargetProcess(pSoftBp->ProcessCR3);
				if(!IsAddressRangeExist(pSoftBp->Address,1))
				{
					ConsolePrintStr("Can not access target process.\n",7,0);
				}
				else
				{
					if(IsAddressExist(pSoftBp->Address))
						*(PUCHAR)pSoftBp->Address = pSoftBp->OldOpcode;

					if(pCurrentSwBp == pSoftBp)
						pCurrentSwBp = NULL;

					memset(pSoftBp,0,sizeof(SW_BP));
				}
				//DetachTargetProcess(OldCR3);
			}
		}

		RefreshCurrentDisasm(pCpu);
		return;
	}

	ret = vmm_strtoul_10(&cmd[3],&bp_id);
	if(ret && bp_id < MAX_SW_BP)
	{
		pSoftBp = &SoftBPs[bp_id];
		if(!pSoftBp->ProcessCR3)
		{
			ConsolePrintStr("Breakpoint is not exist.\n",7,0);
		}
		else
		{
			//OldCR3 = AttachTargetProcess(pSoftBp->ProcessCR3);
			if(!IsAddressRangeExist(pSoftBp->Address,1))
			{
				ConsolePrintStr("Can not access target process.\n",7,0);
			}
			else
			{
				if(IsAddressExist(pSoftBp->Address))
					*(PUCHAR)pSoftBp->Address = pSoftBp->OldOpcode;

				if(pCurrentSwBp == pSoftBp)
					pCurrentSwBp = NULL;

				memset(pSoftBp,0,sizeof(SW_BP));
				RefreshCurrentDisasm(pCpu);
			}
			//DetachTargetProcess(OldCR3);
		}
	}
	else
	{
		ConsolePrintStr("Invalid breakpoint id!\n",7,0);
	}
}

VOID CmdListBreakpoint(PGUEST_CPU pCpu,PCHAR cmd)
{
	CHAR str[128];
	CHAR SymbolOffset[128];
	ULONG i;
	PSW_BP pSoftBp;
    const char *Ret;
    char SymName[256];
    char *pModName;
    unsigned long SymSize;
    unsigned long SymOffset;

	for(i = 0; i < MAX_SW_BP; i++)
	{
		if(SoftBPs[i].ProcessCR3)
		{
			pSoftBp = &SoftBPs[i];

			Ret = LookupAddress((int)pSoftBp->Address,&SymSize,&SymOffset,&pModName,SymName);
            if(Ret)
                snprintf(SymbolOffset,128,"%s!%s + %04X",pModName,SymName,(ULONG)SymOffset);
            else
                strcpy(SymbolOffset,"unknown symbol");

			snprintf(str,128,"%2d)  BPX  %04X:%08X  %s\n",i,pSoftBp->CodeSeg,pSoftBp->Address,SymbolOffset);
			ConsolePrintStr(str,7,0);
			if(strlen(pSoftBp->IfCondition))
			{
				snprintf(str,128,"     if \"%s\" ",pSoftBp->IfCondition);
				ConsolePrintStr(str,7,0);
				if(strlen(pSoftBp->DoCmd))
				{
					snprintf(str,128,"do \"%s\"",pSoftBp->DoCmd);
					ConsolePrintStr(str,7,0);
				}
				ConsolePrintStr("\n",7,0);
			}
		}
	}
}

VOID CmdSetSwBreakpoint(PGUEST_CPU pCpu,PCHAR cmd)
{
	CHAR str[128];
	CHAR p1[128];
	CHAR p2[128];
	CHAR p3[128];
	CHAR p4[128];
	CHAR p5[128];
	ULONG r1,r2,r3,r4,r5;
	ULONG address;
	ULONG ret;
	ULONG i;
	PSW_BP pSoftBp;

	r1 = GetCommandParam(cmd,p1,1);
	r2 = GetCommandParam(cmd,p2,2);
	r3 = GetCommandParam(cmd,p3,3);
	r4 = GetCommandParam(cmd,p4,4);
	r5 = GetCommandParam(cmd,p5,5);

	if(!r1 || !strlen(p1))
	{
		ConsolePrintStr("BPX need param.\n",7,0);
		return;
	}

	ret = CalcExp(pCpu,p1,&address);
	if(!ret)
	{
		ConsolePrintStr("Invalid param.\n",7,0);
		return;
	}

	if(!IsAddressExist(address))
	{
		snprintf(str,128,"Page[%08X] is not present.\n",address);
		ConsolePrintStr(str,7,0);
		return;
	}

	for(i = 0; i < MAX_SW_BP; i++)
	{
		if(SoftBPs[i].Address == address)
		{
			ConsolePrintStr("breakpoint is already exist.\n",7,0);
			return;
		}
	}

	for(i = 0; i < MAX_SW_BP; i++)
	{
		if(!SoftBPs[i].ProcessCR3)
		{
			pSoftBp = &SoftBPs[i];
			break;
		}
	}

	if(i != MAX_SW_BP)
	{
		pSoftBp->ProcessCR3 = _ReadVMCS(GUEST_CR3);
		pSoftBp->CodeSeg = _ReadVMCS(GUEST_CS_SELECTOR);
		pSoftBp->Address = address;
		pSoftBp->OldOpcode = *(PUCHAR)(pSoftBp->Address);

		if(r2 && !vmm_strcmpi(p2,"if"))	//ÓÐifÓïŸä£¿
		{
			if(r3 && strlen(p3))
			{
				strcpy(pSoftBp->IfCondition,p3);			//žŽÖÆifÌõŒþ±íŽïÊœ
			}
		}

		if(r4 && !vmm_strcmpi(p4,"do"))	//ÓÐdoÓïŸä£¿
		{
			if(r5 && strlen(p5))
			{
				strcpy(pSoftBp->DoCmd,p5);			//žŽÖÆdoÓïŸä
			}
		}

		*(PUCHAR)(pSoftBp->Address) = INT3_OPCODE;
		RefreshCurrentDisasm(pCpu);
	}
	else
	{
		ConsolePrintStr("Breakpoint limit exceeded!",7,0);
	}
}
