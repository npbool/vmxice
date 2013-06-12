#include <linux/module.h>
#include <linux/fb.h>

#include "dbgprint.h"
#include "video.h"
#include "font_inc.c"

#define CHAR_WIDTH        8
#define CHAR_HEIGHT       14
#define CHAR_PIXELS       CHAR_WIDTH * CHAR_HEIGHT
#define CHAR_SIZE         (CHAR_WIDTH * CHAR_HEIGHT / 8)

//#define PRINT_IMAGE 1

PCHAR VideoBuffer_va;
ULONG VideoWidth;
ULONG VideoHeight;
ULONG VideoBitPerPixel;
ULONG VideoPitch;

ULONG StartX = 0;
ULONG StartY = 0;

ULONG GUI_Width = 100;
ULONG GUI_Height = 40;

PUCHAR pVideoBufferBak;
PUCHAR pVideoBufferImage;
PUCHAR pVideoBufferPrint;

ULONG ColorTbl[] = {0x000000,0x000080,0x008000,0x008080,0x800000,0x800080,0x808000,0xC0C0C0,0x808080,0x0000FF,0x00FF00,0x00FFFF,0xFF0000,0xFF00FF,0xFFFF00,0xFFFFFF};

ULONG PrintChar(ULONG x,ULONG y,ULONG ForeColor,ULONG BackColor,ULONG bTransparent,UCHAR CharAscii)
{
    ULONG CharForeColor;
    ULONG CharBackColor;
    register PUCHAR pFontBits;
    ULONG FontMask;
    ULONG x0,y0;
    register ULONG cx,cy;
    register PULONG p;
#ifdef PRINT_IMAGE
    register PULONG p_img;
#endif
    ULONG LinePitch1;

    if(x >= GUI_Width || y >= GUI_Height)
        return 0;

    CharForeColor = ColorTbl[ForeColor];
    CharBackColor = ColorTbl[BackColor];

    pFontBits = &cFontData[CharAscii*CHAR_SIZE];

    x0 = x*CHAR_WIDTH + StartX;
    y0 = y*CHAR_HEIGHT + StartY;

    p = (PULONG)&VideoBuffer_va[(y0 * VideoPitch) + (x0 * 4)];
#ifdef PRINT_IMAGE
    p_img = (PULONG)&pVideoBufferImage[(y0 * VideoPitch) + (x0 * 4)];
#endif
    LinePitch1 = (VideoPitch - (CHAR_WIDTH<<2)) >> 2;
    for(cy = 0; cy < CHAR_HEIGHT; cy++)
    {
        for(cx = 0,FontMask = 0x80; cx < CHAR_WIDTH; cx++)
        {
            if((*pFontBits) & FontMask)
            {
                *p = CharForeColor;
#ifdef PRINT_IMAGE
                *p_img = CharForeColor;
#endif
            }
            else
            {
                if(!bTransparent)
                {
                    *p = CharBackColor;
 #ifdef PRINT_IMAGE
                    *p_img = CharBackColor;
 #endif
                }
            }
            p++;
#ifdef PRINT_IMAGE
            p_img++;
#endif
            FontMask = FontMask >> 1;
        }
        p += LinePitch1;
#ifdef PRINT_IMAGE
        p_img += LinePitch1;
#endif
        pFontBits++;

    }
    return 1;
}

VOID DrawCursor(ULONG x,ULONG y,BOOLEAN isInsertState,BOOLEAN ShowState)
{
    ULONG x0,y0,cy;
    PUCHAR p;
#ifdef PRINT_IMAGE
    PUCHAR p_img;
#endif
    ULONG EmptyCharLineW[8] = {0xC0C0C0,0xC0C0C0,0xC0C0C0,0xC0C0C0,0xC0C0C0,0xC0C0C0,0xC0C0C0,0xC0C0C0};
    ULONG EmptyCharLineB[8] = {0x0};

    if(x >= GUI_Width || y >= GUI_Height)
        return;

    x0 = x*CHAR_WIDTH + StartX;
    y0 = y*CHAR_HEIGHT + StartY;

    p = &VideoBuffer_va[(y0 * VideoPitch) + (x0 * sizeof(ULONG))];
#ifdef PRINT_IMAGE
    p_img = &pVideoBufferImage[(y0 * VideoPitch) + (x0 * sizeof(ULONG))];
#endif
    for(cy = 0; cy < CHAR_HEIGHT; cy++)
    {
        if(cy < 11)
        {
            if(isInsertState)
            {
                if(ShowState)
                {
                    memcpy(p,EmptyCharLineW,sizeof(EmptyCharLineW));
#ifdef PRINT_IMAGE
                    memcpy(p_img,EmptyCharLineW,sizeof(EmptyCharLineW));
#endif
                }
                else
                {
                    memcpy(p,EmptyCharLineB,sizeof(EmptyCharLineB));
#ifdef PRINT_IMAGE
                    memcpy(p_img,EmptyCharLineW,sizeof(EmptyCharLineW));
#endif
                }
            }
        }
        else
        {
            if(ShowState)
            {
                memcpy(p,EmptyCharLineW,sizeof(EmptyCharLineW));
#ifdef PRINT_IMAGE
                memcpy(p_img,EmptyCharLineW,sizeof(EmptyCharLineW));
#endif
            }
            else
            {
                memcpy(p,EmptyCharLineB,sizeof(EmptyCharLineB));
#ifdef PRINT_IMAGE
                memcpy(p_img,EmptyCharLineB,sizeof(EmptyCharLineB));
#endif
            }
        }
        p += VideoPitch;
#ifdef PRINT_IMAGE
        p_img += VideoPitch;
#endif
    }
}

VOID PrintStr(ULONG x,ULONG y,ULONG ForeColor,ULONG BackColor,ULONG bTransparent,PUCHAR String,ULONG FillLine)
{
    ULONG MaxLen = 1024;

    while(*String)
    {
        PrintChar(x,y,ForeColor,BackColor,bTransparent,*String);
        x++;
        if(x >= GUI_Width)
        {
            x = 1;
            y++;
        }
        String++;
        MaxLen--;
        if(!MaxLen)
            break;
    }

    while(FillLine)
    {
        PrintChar(x,y,ForeColor,BackColor,bTransparent,' ');
        x++;
        if(x >= (GUI_Width-1))
            break;
    }
}

VOID DrawLineX(ULONG x0,ULONG x1,ULONG y,ULONG ForeColor,ULONG BackColor,UCHAR bChar)
{
    while(1)
    {
        PrintChar(x0,y,ForeColor,BackColor,FALSE,bChar);
        x0++;
        if(x0 >= x1)
            break;
    }
}


VOID DrawLineY(ULONG y0,ULONG y1,ULONG x,ULONG ForeColor,ULONG BackColor,UCHAR bChar)
{
    while(1)
    {
        PrintChar(x,y0,ForeColor,BackColor,FALSE,bChar);
        y0++;
        if(y0 >= y1)
            break;
    }
}

VOID DrawBackground(void)
{
    ULONG y;
    PUCHAR p;
#ifdef PRINT_IMAGE
    PUCHAR p_img;
#endif

    p = &VideoBuffer_va[(StartY * VideoPitch) + (StartX * sizeof(ULONG))];
#ifdef PRINT_IMAGE
    p_img = &pVideoBufferImage[(StartY * VideoPitch) + (StartX * sizeof(ULONG))];
#endif
    for(y = 0; y < GUI_Height * CHAR_HEIGHT; y++)
    {
        memset(p,0,GUI_Width * CHAR_WIDTH * sizeof(ULONG));
        p += VideoPitch;
#ifdef PRINT_IMAGE
        memset(p_img,0,GUI_Width * CHAR_WIDTH * sizeof(ULONG));
        p_img += VideoPitch;
#endif
    }
}

VOID FillScreen(void)
{
    ULONG i;
    for(i = 0; i < 40; i++)
        PrintStr(0,i,7,0,FALSE,"Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test Test ",FALSE);
}

VOID DrawBorder(void)
{
    PrintChar(0,0,3,0,0,218);
    PrintChar(GUI_Width-1,0,3,0,0,191);
    PrintChar(0,GUI_Height-1,3,0,0,192);
    PrintChar(GUI_Width-1,GUI_Height-1,3,0,0,217);

    DrawLineX(1,GUI_Width-1,0,3,0,196);
    DrawLineX(1,GUI_Width-1,GUI_Height-1,3,0,196);
    DrawLineY(1,GUI_Height-1,0,3,0,179);
    DrawLineY(1,GUI_Height-1,GUI_Width-1,3,0,179);

    //DrawLineX(1,GUI_Width-1,4,2,0,196);
    //DrawLineX(1,GUI_Width-1,24,2,0,196);
    DrawLineX(1,GUI_Width-1,GUI_Height-2,0,3,' ');

    PrintStr(GUI_Width-33,GUI_Height-1,15,0,0," [ VMXICE v0.1  MengXP Works ] ",FALSE);
}

VOID BackupScreen(void)
{
    ULONG y;

    for(y = 0; y < VideoHeight; y++)
    {
        memcpy(pVideoBufferBak+y*VideoWidth*4,VideoBuffer_va+VideoPitch*y,VideoWidth*4);
#ifdef PRINT_IMAGE
        memcpy(pVideoBufferImage+y*VideoWidth*4,VideoBuffer_va+VideoPitch*y,VideoWidth*4);
#endif
    }
}

VOID RestoreScreen(void)
{
    ULONG y;

    for(y = 0; y < VideoHeight; y++)
    {
        memcpy(VideoBuffer_va+VideoPitch*y,pVideoBufferBak+y*VideoWidth*4,VideoWidth*4);
    }
}

VOID PrintScreen(void)
{
    ULONG    y;

    for(y = 0; y < VideoHeight; y++)
    {
        memcpy(pVideoBufferPrint+y*VideoWidth*4,pVideoBufferImage+VideoPitch*y,VideoWidth*4);
    }
}

VOID VideoInit(void)
{
    struct fb_info *fbi = registered_fb[0];

    VideoBuffer_va = (char *)fbi->screen_base;
    VideoWidth = fbi->var.xres;
    VideoHeight = fbi->var.yres;
    VideoBitPerPixel = fbi->var.bits_per_pixel;
    VideoPitch = fbi->fix.line_length;

    printf("FrameBuffer Virt: 0x%p\r\n",(PVOID)VideoBuffer_va);
    printf("FrameBuffer Phys: 0x%p\r\n",(PVOID)fbi->fix.smem_start);
    printf("Resolution: %d x %d x %d\r\n",VideoWidth,VideoHeight,VideoBitPerPixel);
    printf("Pitch: %d\r\n\r\n",VideoPitch);

    StartX = (VideoWidth - GUI_Width * CHAR_WIDTH) / 2;
    StartY = (VideoHeight - GUI_Height * CHAR_HEIGHT) / 2;

    pVideoBufferBak = (PUCHAR)kmalloc(VideoHeight * VideoWidth * 4,GFP_ATOMIC);
    pVideoBufferPrint = (PUCHAR)kmalloc(VideoHeight * VideoWidth * 4,GFP_ATOMIC);
    pVideoBufferImage = (PUCHAR)kmalloc(VideoHeight * VideoWidth * 4,GFP_ATOMIC);
}

VOID VideoRelease(void)
{
    if(pVideoBufferBak)
        kfree(pVideoBufferBak);
    if(pVideoBufferPrint)
        kfree(pVideoBufferPrint);
    if(pVideoBufferImage)
        kfree(pVideoBufferImage);
}
