#ifndef _DEFINE_H_
#define _DEFINE_H_

typedef unsigned           char   UCHAR,*PUCHAR;
typedef   signed           char   CHAR,*PCHAR;
typedef unsigned short     int    USHORT,*PUSHORT;
typedef signed short       int    SHORT,*PSHORT;
typedef unsigned           int    ULONG,*PULONG;
typedef   signed           int    LONG,*PLONG;
typedef unsigned long long int    ULONG64,*PULONG64;
typedef   signed long long int    LONG64,*PLONG64;

typedef void    VOID,*PVOID;
typedef bool    BOOLEAN,*PBOOLEAN;

#define TRUE true
#define FALSE false

typedef struct _LARGE_INTEGER {
    union {
        ULONG64 QuadPart;
        struct{
            ULONG LowPart;
            ULONG HighPart;
        };
    };
}LARGE_INTEGER;

typedef LARGE_INTEGER PHYSICAL_ADDRESS;

#endif
