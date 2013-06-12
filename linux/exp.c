#include <linux/module.h>
#include <linux/io.h>
#include "mmu.h"
#include "exp.h"
#include "sym.h"
#include "vmx.h"
#include "vmmstring.h"
#include "console.h"

#define TYPE_DWORD   0
#define TYPE_WORD    1
#define TYPE_BYTE    2

#define STACK_SIZE   48

typedef struct {
    ULONG Stack[STACK_SIZE];
    ULONG Top;
}STACK,*PSTACK;

VOID StackInit(PSTACK s)
{
    s->Top = -1;
}

ULONG StackIsEmpty(PSTACK s)
{
    return (s->Top == -1);
}

ULONG StackGetTop(PSTACK s,PULONG e)
{
    *e = 0;
    if(StackIsEmpty(s))
        return 0;
    *e = s->Stack[s->Top];
    return 1;
}

VOID StackPush(PSTACK s,ULONG e)
{
    s->Top++;
    s->Stack[s->Top] = e;
}

ULONG StackPop(PSTACK s,PULONG e)
{
    *e = 0;
    if(StackIsEmpty(s))
        return 0;
    *e = s->Stack[s->Top--];
    return 1;
}

ULONG GetNumber(PCHAR ptr,PULONG v)
{
    ULONG ret = 0;
    ULONG i = 0;
    CHAR c;

    while(1)
    {
        c = *ptr;
        if(c >= 'A' && c <= 'F')
            c = c - 'A' + 10;
        else if(c >= 'a' && c <= 'f')
            c = c - 'a' + 10;
        else if(c >= '0' && c <= '9')
            c = c - '0';
        else
            break;

        ret = ret * 16 + c;
        ptr++;
        i++;
    }
    *v = ret;
    return i;
}

ULONG GetSymbol(PCHAR ptr,PCHAR str)
{
    CHAR c;
    ULONG i = 0;

    while(*ptr)
    {
        c = *ptr;
        if((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '@' || c == '?' || c == '_' || (c =='!' && *(ptr+1) != '='))
            *str = c;
        else
            break;
        ptr++;
        str++;
        i++;
    }
    *str = 0;
    return i;
}

ULONG GetString(PCHAR ptr,PCHAR str)
{
    ULONG i = 1;

    ptr++;
    while(*ptr)
    {
        *str = *ptr;
        if(*str == '"')
            break;
        str++;
        ptr++;
        i++;
    }
    *str = 0;
    return i+1;        //×îºóÒ»¸öÒýºÅ
}

ULONG IsNumber(CHAR c)
{
    return (c >= '0' && c <= '9');
}

ULONG IsSpecOp(ULONG c)
{
    PUCHAR pc = (PUCHAR)&c;
    switch(pc[0])
    {
    case '=':
        if(pc[1] == '=')
            return 1;
        break;
    case '>':
        return 1;
    case '<':
        return 1;
    case '!':
        if(pc[1] == '=')
            return 1;
        break;
    case '|':
        if(pc[1] == '|')
            return 1;
        break;
    case '&':
        return 1;
        break;
    }
    return 0;
}

ULONG SafeStrCopy(PCHAR str1,PCHAR str2)
{
    if(!IsAddressExist((ULONG)str2))
        return FALSE;

    while(*str2)
    {
        *str1 = *str2;
        str1++;
        str2++;
    }
    *str1 = 0;
    return TRUE;
}

//È¡µØÖ·Öµ
ULONG ReadValue(ULONG addr,ULONG type,PULONG value)
{
    ULONG bRet = FALSE;

    *value = 0;
    if(!IsAddressExist(addr))
        return FALSE;

    switch(type)
    {
    case TYPE_BYTE:
        *value = *(PUCHAR)addr;
        bRet = TRUE;
        break;

    case TYPE_WORD:
        *value = *(PUSHORT)addr;
        bRet = TRUE;
        break;

    case TYPE_DWORD:
        *value = *(PULONG)addr;
        bRet = TRUE;
        break;

    default:
        break;
    }

    return bRet;
}

//¼ÆËãÕ»ÖÐÒ»¸ö±í´ïÊ½µÄÖµ²¢ÈëÕ»
//·µ»Ø0Ê§°Ü 1³É¹¦
ULONG CalcInStack(PSTACK ss,PSTACK sr)
{
    ULONG a,b,r;
    ULONG s;
    PCHAR pc = (PCHAR)&s;
    CHAR str1[128];
    CHAR str2[128];
    ULONG ret;

    r = StackPop(sr,&b);
    if(!r)
        return 0;
    r = StackPop(sr,&a);
    if(!r)
        return 0;
    r = StackPop(ss,&s);
    if(!r)
        return 0;

    //printf("%d %c%c %d\n",a,pc[0],pc[1]?pc[1]:' ',b);

    switch(pc[0])
    {
    case '+':
        StackPush(sr,a+b);
        break;
    case '-':
        StackPush(sr,a-b);
        break;
    case '*':
        StackPush(sr,a*b);
        break;
    case '/':
        if(!b)
        {
            //printf("³ý0´íÎó£¡\n");
            return 0;
        }
        StackPush(sr,a/b);
        break;

    case '>':
        if(pc[1] == '=')
            StackPush(sr,a>=b);
        else
            StackPush(sr,a>b);
        break;
    case '<':
        if(pc[1] == '=')
            StackPush(sr,a<=b);
        else
            StackPush(sr,a<b);
        break;

    case '=':
        if(pc[1] == '=')
        {
            //Ê×ÏÈ³¢ÊÔ×Ö·û´®±È½Ï ANSI
            ret = SafeStrCopy(str1,(PCHAR)a);
            if(ret)
            {
                ret = SafeStrCopy(str2,(PCHAR)b);
                if(ret)
                {
                    if(!vmm_strcmpi(str1,str2))
                    {
                        StackPush(sr,1);
                        break;
                    }
                }
            }
            //Ö±½Ó±È½Ï
            StackPush(sr,a==b);
        }
        break;
    case '!':
        if(pc[1] == '=')
            StackPush(sr,a!=b);
        break;
    case '|':
        if(pc[1] == '|')
            StackPush(sr,a||b);
        break;
    case '&':
        if(pc[1] == '&')
            StackPush(sr,a&&b);
        else
            StackPush(sr,a&b);
        break;
    default:
        return 0;
    }
    return 1;
}

//È¡Ò»¸ö×Ö·û»ò²Ù×÷·û
//·µ»Ø×Ö·û³¤¶È
ULONG GetChar(PCHAR exp,PULONG c)
{
    PUCHAR pc = (PUCHAR)c;

    *c = *(PSHORT)(exp);
    switch(pc[0])
    {
    case '=':
        if(pc[1] == '=')
            return 2;
        break;
    case '>':
        if(pc[1] == '=')
            return 2;
        break;
    case '<':
        if(pc[1] == '=')
            return 2;
        break;
    case '!':
        if(pc[1] == '=')
            return 2;
        break;
    case '|':
        if(pc[1] == '|')
            return 2;
        break;
    case '&':
        if(pc[1] == '&')
            return 2;
        break;

    default:
        break;
    }

    *c = 0;
    *c = *exp;
    return 1;
}

ULONG GetAddressForSymbol(PGUEST_CPU pCpu,PCHAR FullSymbolName)
{
    if(!vmm_strcmpi(FullSymbolName,"eax"))
        return pCpu->pGuestRegs->eax;
    else if(!vmm_strcmpi(FullSymbolName,"ebx"))
        return pCpu->pGuestRegs->ebx;
    else if(!vmm_strcmpi(FullSymbolName,"ecx"))
        return pCpu->pGuestRegs->ecx;
    else if(!vmm_strcmpi(FullSymbolName,"edx"))
        return pCpu->pGuestRegs->edx;
    else if(!vmm_strcmpi(FullSymbolName,"esi"))
        return pCpu->pGuestRegs->esi;
    else if(!vmm_strcmpi(FullSymbolName,"edi"))
        return pCpu->pGuestRegs->edi;
    else if(!vmm_strcmpi(FullSymbolName,"esp"))
        return pCpu->pGuestRegs->esp;
    else if(!vmm_strcmpi(FullSymbolName,"ebp"))
        return pCpu->pGuestRegs->ebp;
    else if(!vmm_strcmpi(FullSymbolName,"eip"))
        return _ReadVMCS(GUEST_RIP);

    return LookupName(FullSymbolName);
}

/*
Start:
1.Èç¹ûµ±Ç°ÊäÈë´®ÖÐµÃµ½µÄÊÇÊý×Ö,ÔòÖ±½ÓÑ¹ÈëÖµÕ».È»ºó×ªµ½Start.
2.Èç¹ûµ±Ç°ÊäÈë´®ÖÐµÃµ½µÄÊÇ·ûºÅ,ÄÇÃ´¶Ô·ûºÅ½øÐÐÅÐ¶Ï.
    1)Èç¹û·ûºÅÊÇ'+'»òÕß'-',ÔòÒÀ´Îµ¯³ö·ûºÅÕ»µÄ·ûºÅ,¼ÆËãÕ»ÖÐÊýÖµ,Ö±µ½µ¯³öµÄ·ûºÅ²»ÊÇ*,/,+,-¡£×îºóÑ¹Èë·ûºÅ¡£
    2)Èç¹û·ûºÅÊÇ'*'»òÕß'/',ÔòÑ¹Èë·ûºÅÕ»
    3)Èç¹û·ûºÅÊÇ'(' '[' ,ÔòÖ±½ÓÑ¹'(' '['Èë·ûºÅÕ»
    4)Èç¹û·ûºÅÊÇ')',ÔòÒÀÕÕ·ûºÅÕ»µÄË³Ðòµ¯³ö·ûºÅ,¼ÆËãÕ»ÖÐÊýÖµ,°Ñ½á¹ûÑ¹ÈëÖµÕ»,Ö±µ½·ûºÅÕ»¶¥ÊÇ'(',×îºóÔÙµ¯³ö'('
    5)Èç¹û·ûºÅÊÇ']',ÔòÒÀÕÕ·ûºÅÕ»µÄË³Ðòµ¯³ö·ûºÅ,¼ÆËãÕ»ÖÐÊýÖµ,°Ñ½á¹ûÑ¹ÈëÖµÕ»,Ö±µ½·ûºÅÕ»¶¥ÊÇ'[',×îºóÔÙµ¯³ö'[' È»ºóµ¯³öµØÖ·£¬È¡µØÖ·ÖµÑ¹Èë¶ÑÕ»
    6)Èç¹û·ûºÅÊÇ == >= <= != || && & > < ÄÇÃ´ÔòÒÀ´Îµ¯³ö·ûºÅÕ»µÄ·ûºÅ,¼ÆËãÕ»ÖÐÊýÖµ,Ö±µ½µ¯³öµÄ·ûºÅ²»ÊÇ ( && ||¡£×îºóÑ¹Èë·ûºÅ¡£
    ×îºó×ªµ½Start.
3.Èç¹ûµ±Ç°ÊäÈë´®µÃµ½µÄÊÇEOF(×Ö·û´®½áÊø·ûºÅ),Ôò¼ÆËãÕ»ÖÐÊýÖµ,ÖªµÀ·ûºÅÕ»Ã»ÓÐ·ûºÅ.
*/
ULONG CalcExp(PVOID pCpu2,PCHAR exp,PULONG ret)
{
    STACK ss,sr,st;        //·ûºÅÕ»¡¢ÖµÕ»¡¢Àà±ðÕ»
    ULONG c;
    ULONG v,i,r;
    PUCHAR pv = (PUCHAR)&v;
    CHAR str[128];
    CHAR sym[128];
    ULONG type = 0;
    ULONG type2;
    PGUEST_CPU pCpu = (PGUEST_CPU)pCpu2;

    StackInit(&ss);
    StackInit(&sr);
    StackInit(&st);

    while(1)
    {
        i = GetChar(exp,&c);            //È¡Ò»¸ö×Ö·û
        if(c == ' ')                    //ºöÂÔ¿Õ×Ö·û
        {
            exp++;
            continue;
        }
        if(IsNumber((CHAR)c))        //Êý×Ö£¿
        {
            i = GetNumber(exp,&v);        //Ö±½ÓÑ¹ÈëÖµÕ»
            StackPush(&sr,v);
            exp += i;
            continue;
        }
        else if(IsSpecOp(c))        //ÌØÊâ²Ù×÷·û£¿
        {
            r = StackGetTop(&ss,&v);
            while(r && v != '(' && pv[0]!='&' && pv[1]!='&' && pv[0]!='|' && pv[1]!='|')        //¼ÆËãÕ»ÖÐ±í´ïÊ½Ö±µ½È¡³öµÄÊÇ ( && ||
            {
                v = CalcInStack(&ss,&sr);
                if(!v)
                    return 0;
                r = StackGetTop(&ss,&v);
            }
            StackPush(&ss,c);            //Ñ¹ÈëÌØÊâ²Ù×÷·û
        }
        else if(c == '+' || c == '-')    //+-ÔËËã·û
        {
            r = StackGetTop(&ss,&v);
            while(v == '+' || v == '-' || v == '*' || v == '/')    //¼ÆËãÕ»ÖÐ±í´ïÊ½Ö±µ½È¡³öµÄ²»ÊÇ +-*/
            {
                v = CalcInStack(&ss,&sr);
                if(!v)
                    return 0;
                r = StackGetTop(&ss,&v);
            }
            StackPush(&ss,c);            //+-ÔËËã·ûÈëÕ»
        }
        else if(c == '*' || c == '/' || c == '(' || c == '[')    // * / ( [ Ö±½ÓÈëÕ»
        {
            StackPush(&ss,c);
            if(c == '[')
            {
                //printf("type = %d\n",type);
                StackPush(&st,type);
                type = 0;
            }
        }
        else if(c == ')')            // ×Ö·û)
        {
            r = StackGetTop(&ss,&v);
            if(!r)
                return 0;
            while(v != '(')                //¼ÆËãÕ»ÖÐ±í´ïÊ½Ö±µ½È¡³öµÄÊÇ (
            {
                v = CalcInStack(&ss,&sr);
                if(!v)
                    return 0;
                r = StackGetTop(&ss,&v);
            }
            StackPop(&ss,&v);            //µ¯³ö(
        }
        else if(c == ']')            //×Ö·û]
        {
            r = StackGetTop(&ss,&v);
            if(!r)
                return 0;
            while(v != '[')                //¼ÆËãÕ»ÖÐ±í´ïÊ½Ö±µ½È¡³öµÄÊÇ [
            {
                v = CalcInStack(&ss,&sr);
                if(!v)
                    return 0;
                r = StackGetTop(&ss,&v);
            }
            StackPop(&ss,&v);                //µ¯³ö[
            r = StackPop(&sr,&v);            //µ¯³öµØÖ·
            if(!r)
                return 0;
            r = StackPop(&st,&type2);
            if(!r)
                return 0;
            r = ReadValue(v,type2,&v);
            if(!r)
                return 0;
            StackPush(&sr,v);    //È¡µØÖ·ÖµÈëÕ»
        }
        else if(c == 0)                //½áÊø·û
        {
            while(!StackIsEmpty(&ss))    //¼ÆËãÕ»ÖÐ±í´ïÊ½Ö±µ½Õ»¿Õ
            {
                v = CalcInStack(&ss,&sr);
                if(!v)
                    return 0;
            }
            r = StackPop(&sr,ret);    //µ¯³ö±í´ïÊ½½á¹ûÖµ
            if(!r)
                return 0;
            return 1;
        }
        else if(c == '"')            //ÒýºÅ
        {
            i = GetString(exp,str);
            //printf("È¡µ½×Ö·û´® %s\n",str);
            StackPush(&sr,(ULONG)&str);
            exp += i;
            continue;
        }
        else
        {
            i = GetSymbol(exp,sym);    //·ûºÅ»òÕß¼Ä´æÆ÷Ãû£¿
            if(!i)
            {
                //printf("ÎÞ·¨Ê¶±ðµÄ×Ö·û£º%c\n",*exp);
                return 0;
            }
            if(!vmm_strncmpi(sym,"dword",5))
            {
                type = TYPE_DWORD;
            }
            else if(!vmm_strncmpi(sym,"word",4))
            {
                type = TYPE_WORD;
            }
            else if(!vmm_strncmpi(sym,"byte",4))
            {
                type = TYPE_BYTE;
            }
            else
            {
                v = GetAddressForSymbol(pCpu,sym);
                StackPush(&sr,v);
            }
            exp += i;
            continue;
        }
        exp += i;
    }
    return 0;
}
