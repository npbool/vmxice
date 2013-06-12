#include "define.h"

#ifndef SYM_H
#define SYM_H

typedef unsigned long (*kallsyms_lookup_name)(const char *name);
typedef const char *(*kallsyms_lookup)(unsigned long addr,
                unsigned long *symbolsize,
                unsigned long *offset,
                char **modname, char *namebuf);

extern ULONG SymbolInit(void);

extern kallsyms_lookup_name  LookupName;
extern kallsyms_lookup       LookupAddress;

#endif
