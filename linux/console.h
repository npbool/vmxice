#include "define.h"

ULONG ConsoleInit(void);
VOID ConsoleRelease(void);
VOID ConsolePrintChar(UCHAR c,ULONG ForeColor,ULONG BackColor);
VOID ConsolePrintStr(PUCHAR str,ULONG ForeColor,ULONG BackColor);
//VOID ConsolePrintStr2(PUCHAR str,ULONG ForeColor,ULONG BackColor,ULONG RefreshScreen);
VOID ConsolePrintCurrentPage(void);
VOID ConsolePrintPreviousPage(void);
VOID ConsolePrintNextPage(void);
