#include "define.h"

extern ULONG GUI_Width;
extern ULONG GUI_Height;

extern ULONG VideoWidth;
extern ULONG VideoHeight;

extern PUCHAR pVideoBufferBak;
extern PUCHAR pVideoBufferPrint;

VOID VideoInit(void);
VOID VideoRelease(void);
VOID BackupScreen(void);
VOID RestoreScreen(void);
ULONG PrintChar(ULONG x,ULONG y,ULONG ForeColor,ULONG BackColor,ULONG bTransparent,UCHAR CharAscii);
VOID DrawCursor(ULONG x,ULONG y,BOOLEAN isInsertState,BOOLEAN ShowState);
VOID PrintStr(ULONG x,ULONG y,ULONG ForeColor,ULONG BackColor,ULONG bTransparent,PUCHAR String,ULONG FillLine);
VOID DrawBackground(void);
VOID FillScreen(void);
VOID DrawBorder(void);
VOID PrintScreen(void);


