#include "define.h"
#include "vmx.h"

#ifndef _DBG_H_
#define _DBG_H_

#define INT3_OPCODE      0xCC
#define PUSHFD_OPCODE    0x9C
#define SYSEXIT_OPCODE   0x350F

#define MAX_SW_BP        10
#define MAX_STEP_BP      10

typedef struct{
	ULONG ProcessCR3;
	USHORT CodeSeg;
	ULONG Address;
	CHAR OldOpcode;
	CHAR IfCondition[128];
	CHAR DoCmd[128];
}SW_BP, *PSW_BP;

typedef struct{
	ULONG BpType;	//0:BPMB 1:BPMW 2:BPIO 3:BPMD
	ULONG BpRWE;	//0:X 1:W 2:R 3:RW
	ULONG Address;
	CHAR IfCondition[128];
	CHAR DoCmd[128];
}HW_BP, *PHW_BP;


extern SW_BP SoftBPs[MAX_SW_BP];
extern SW_BP StepBPs[MAX_STEP_BP];
extern HW_BP HardBPs[4];
extern PSW_BP pCurrentSwBp;
extern ULONG iCurrentHwBp;


extern ULONG IsAddressRangeExist(ULONG Addr,ULONG Len);

extern ULONG bSingleStepping;
extern ULONG bSingleStepPushfd;

extern ULONG bNeedRestoreScreen;
extern ULONG bScreenBackuped;
extern ULONG bShowGUIwhileSingleStepping;

extern ULONG bDebuggerBreak;

VOID RefreshOldRegister(PGUEST_CPU pCpu);
VOID EnterHyperDebugger(PGUEST_CPU pCpu);
VOID CmdSetSingleStep(PGUEST_CPU pCpu);

VOID SwBreakpointEvent(PGUEST_CPU pCpu,PSW_BP pSoftBp);
VOID SwStepOverEvent(PGUEST_CPU pCpu);

VOID CmdStepOver(PGUEST_CPU pCpu);
VOID ClearStepBps(void);
VOID DebugCommandHandler(PGUEST_CPU pCpu,PUCHAR cmd);

typedef VOID CMD_HANDLER (PGUEST_CPU pCpu,PCHAR cmd);
typedef CMD_HANDLER *PCMD_HANDLER;

typedef struct{
	CHAR *Cmd;
	CHAR *Desc;
	CHAR *Usage;
	CHAR *Example;
	PCMD_HANDLER pHandler;
}CMD_HELP, *PCMD_HELP;

typedef struct{
	PCHAR pszLine;
	ULONG ForeColor;
	ULONG BackColor;
}HELP_INFO;

VOID CmdDisplayHelp(PGUEST_CPU pCpu,PCHAR cmd);
VOID CmdCalcExp(PGUEST_CPU pCpu,PCHAR cmd);
VOID CmdDisplayMemoryByte(PGUEST_CPU pCpu,PCHAR cmd);
VOID CmdDisplayMemoryWord(PGUEST_CPU pCpu,PCHAR cmd);
VOID CmdDisplayMemoryDword(PGUEST_CPU pCpu,PCHAR cmd);
VOID CmdDisplayIdt(PGUEST_CPU pCpu,PCHAR cmd);
VOID CmdDisplayCpuReg(PGUEST_CPU pCpu,PCHAR cmd);
VOID CmdGetPageInfo(PGUEST_CPU pCpu,PCHAR cmd);
VOID CmdGetVirtAddr(PGUEST_CPU pCpu,PCHAR cmd);
VOID CmdDisplayDisasm(PGUEST_CPU pCpu,PCHAR cmd);
VOID CmdDisplayCallstack(PGUEST_CPU pCpu,PCHAR cmd);
VOID CmdClearBreakpoint(PGUEST_CPU pCpu,PCHAR cmd);
VOID CmdListBreakpoint(PGUEST_CPU pCpu,PCHAR cmd);
VOID CmdSetSwBreakpoint(PGUEST_CPU pCpu,PCHAR cmd);

#endif
