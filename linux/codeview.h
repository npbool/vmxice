#include "define.h"
#include "vmx.h"

#ifndef _CODEVIEW_H_
#define _CODEVIEW_H_

typedef struct {
    ULONG pInstrAddr;
    ULONG HitCount;
}PREV_INSTR_HITTEST,*PPREV_INSTR_HITTEST;

VOID CodeViewInit(void);
VOID ShowDisasm(PGUEST_CPU pCpu,ULONG StartAddress,ULONG NeedPageChange);
VOID ShowCurrentDisasm(PGUEST_CPU pCpu);
VOID RefreshCurrentDisasm(PGUEST_CPU pCpu);
VOID ClearCurrentDisasm(void);
VOID ShowNextIpDisasm(PGUEST_CPU pCpu);
VOID ShowPrevIpDisasm(PGUEST_CPU pCpu);
ULONG OriDisasm(PUCHAR str,ULONG Eip);

#endif
