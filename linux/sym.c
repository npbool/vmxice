#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include "dbgprint.h"
#include "sym.h"

kallsyms_lookup_name LookupName;
kallsyms_lookup LookupAddress;

int char2bin(char c)
{
    switch(c)
    {
        case '0':return 0;
        case '1':return 1;
        case '2':return 2;
        case '3':return 3;
        case '4':return 4;
        case '5':return 5;
        case '6':return 6;
        case '7':return 7;
        case '8':return 8;
        case '9':return 9;
        case 'A':
        case 'a':return 10;
        case 'B':
        case 'b':return 11;
        case 'C':
        case 'c':return 12;
        case 'D':
        case 'd':return 13;
        case 'E':
        case 'e':return 14;
        case 'F':
        case 'f':return 15;
        default:
            return 0;
    }
}

int hex2bin_dword(char *lpszHex)
{
    int value = 0;
    int i;

    for(i = 0; i < 8; i++)
    {
        value <<= 4;
        value |= char2bin(lpszHex[i]);
    }
    return value;
}

int Get_kallsyms_lookup_name(void)
{
    int lpApiAddress = 0;
    mm_segment_t old_fs;
    struct file *fp = NULL;
    char *lpBuffer;
    int dwTotalRead = 0;
    int dwReadBytes;
    char szApiName[] = " kallsyms_lookup_name";
    int dwApiNameLen = strlen(szApiName);
    int i;
    char szApiAddress[16];

    printf("Lookup %s..\r\n",szApiName);

    lpBuffer = vmalloc(0x400000);
    memset(szApiAddress,0,16);

    fp = filp_open("/proc/kallsyms",O_RDWR,644);
    if(!fp)
        return 0;

    old_fs = get_fs();
    set_fs(get_ds());
    while(1)
    {
        dwReadBytes = fp->f_op->read(fp,&lpBuffer[dwTotalRead],0x1000,&fp->f_pos);
        if(!dwReadBytes)
            break;
        dwTotalRead += dwReadBytes;
    }
    set_fs(old_fs);
    filp_close(fp,NULL);
    printf("Total read %d bytes\r\n",dwTotalRead);

    for(i = 0; i < dwTotalRead-dwApiNameLen; i++)
    {
        if(!memcmp(&lpBuffer[i],szApiName,dwApiNameLen))
        {
            memcpy(szApiAddress,&lpBuffer[i-10],8);
            lpApiAddress = hex2bin_dword(szApiAddress);
            printf("Found %s @ %08x\r\n",szApiName,lpApiAddress);
            break;
        }
    }

    vfree(lpBuffer);
    return lpApiAddress;
}

ULONG SymbolInit(void)
{
    LookupName = (kallsyms_lookup_name)Get_kallsyms_lookup_name();
    if(!LookupName)
    {
        printf("Can't get kallsyms_lookup_name!\r\n");
        return 0;
    }
    LookupAddress = (kallsyms_lookup)LookupName("kallsyms_lookup");
    printf("Found kallsyms_lookup @ %08x\r\n\r\n",LookupAddress);

    return 1;
}
