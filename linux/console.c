#include <linux/module.h>
#include <linux/io.h>
#include <linux/slab.h>
#include "dbgprint.h"
#include "video.h"
#include "console.h"
#include "dbg.h"

ULONG    ConsoleHeight;
ULONG    ConsoleWidth;
PUCHAR   ConsoleBuffer;
ULONG    ConsoleHistoryHeight = 100;
ULONG    ConsoleHistoryTempHeight = 100;
PUCHAR   ConsoleEndPtr;
PUCHAR   ConsoleTempEndPtr;
PUCHAR   ConsoleCurrentPtr;
PUCHAR   ConsoleCurrentPagePtr;

ULONG ConsoleInit(void)
{
    ConsoleHeight = 12;
    ConsoleWidth = GUI_Width-2;
    ConsoleBuffer = (PUCHAR)kmalloc(ConsoleWidth*2*(ConsoleHistoryHeight+ConsoleHistoryTempHeight),GFP_ATOMIC);
    printf("console init\n");
    if(!ConsoleBuffer)
    {
        RED_FONT;
        printf("InitConsole: kmalloc for ConsoleBuffer failed\n");
        return false;
    }
    memset(ConsoleBuffer,0,(GUI_Width-2)*2*ConsoleHistoryHeight);
    ConsoleCurrentPtr = ConsoleBuffer;
    ConsoleCurrentPagePtr = ConsoleBuffer;
    ConsoleEndPtr = ConsoleBuffer + (GUI_Width-2)*2*ConsoleHistoryHeight;
    ConsoleTempEndPtr = ConsoleBuffer + (GUI_Width-2)*2*(ConsoleHistoryHeight+ConsoleHistoryTempHeight);
    ConsolePrintStr("VMXICE for linux - A Kernel Debugger Based On Intel VT-x\n",14,0);
    ConsolePrintStr(" \n",7,0);
    ConsolePrintStr("E-mail:  mxz@live.cn\n",13,0);
    ConsolePrintStr("Website: http://www.vxgate.net\n",13,0);
    ConsolePrintStr(" \n",7,0);

    return true;
}

VOID ConsoleRelease(void)
{
    if(ConsoleBuffer)
    {
        kfree(ConsoleBuffer);
        ConsoleBuffer = NULL;
    }
}

ULONG IsNewLine(PUCHAR ptr)
{
    return (((ptr-ConsoleBuffer) % (ConsoleWidth*2)) == 0);
}

VOID ConsolePrintChar(UCHAR c,ULONG ForeColor,ULONG BackColor)
{
    ConsoleCurrentPtr[0] = c;
    ConsoleCurrentPtr[1] = (ForeColor << 4) + (BackColor & 0xF);
    ConsoleCurrentPtr += 2;
}

VOID ConsolePrintCurrentPage(void)
{
    PUCHAR p = ConsoleCurrentPagePtr;
    ULONG i = 0;
    ULONG x = 1,y = 25;
    UCHAR c;
    ULONG ForeColor;
    ULONG BackColor;

    while(i < ConsoleWidth*ConsoleHeight*2)
    {
        c = p[0];
        ForeColor = p[1] >> 4;
        BackColor = p[1] & 0xF;
        PrintChar(x,y,ForeColor,BackColor,FALSE,c);
        x++;
        if(x == GUI_Width-1)
        {
            x = 1;
            y++;
        }
        i+=2;
        p+=2;
    }
}

VOID ConsolePrintPreviousPage(void)
{
    if(ConsoleCurrentPagePtr > ConsoleBuffer)
    {
        ConsoleCurrentPagePtr -= ConsoleWidth*2;
        ConsolePrintCurrentPage();
    }
}

VOID ConsolePrintNextPage(void)
{
    PUCHAR NewLinePtr = ConsoleCurrentPagePtr + ConsoleWidth*2;

    if((NewLinePtr < ConsoleEndPtr) && ((NewLinePtr + ConsoleWidth*ConsoleHeight*2)<ConsoleCurrentPtr))
    {
        ConsoleCurrentPagePtr += ConsoleWidth*2;
        ConsolePrintCurrentPage();
    }
}

VOID ConsolePrintStr(PUCHAR str,ULONG ForeColor,ULONG BackColor)
{
    CHAR c;
    ULONG OverflowBytes;

    c = *str;
    while(c)
    {
        if(c == '\n')
        {
            while(!IsNewLine(ConsoleCurrentPtr))
            {
                ConsolePrintChar(0,0,0);
            }
        }
        else
        {
            ConsolePrintChar(c,ForeColor,BackColor);
        }
        c = *(++str);
    }

    if(ConsoleCurrentPtr >= ConsoleEndPtr)
    {
        OverflowBytes = ((ConsoleCurrentPtr - ConsoleEndPtr + ConsoleWidth*2) / ConsoleWidth*2) * ConsoleWidth*2;
        ConsoleCurrentPtr -= OverflowBytes;
        memcpy(ConsoleBuffer, ConsoleBuffer+OverflowBytes, ConsoleWidth*2*(ConsoleHistoryHeight));
        memset(ConsoleCurrentPtr,0,ConsoleEndPtr-ConsoleCurrentPtr);
    }

    if((ConsoleCurrentPtr - ConsoleWidth*ConsoleHeight*2) <= ConsoleBuffer)
    {
        ConsoleCurrentPagePtr = ConsoleBuffer;
    }
    else
    {
        ConsoleCurrentPagePtr = ConsoleCurrentPtr - ConsoleWidth*ConsoleHeight*2;
        while(!IsNewLine(ConsoleCurrentPagePtr))
            ConsoleCurrentPagePtr += 2;
    }

    if(bScreenBackuped)
        ConsolePrintCurrentPage();
}

