#include "define.h"

//UCHAR*  vmm_strncpy(UCHAR *dst, UCHAR *src, ULONG n);
VOID	vmm_strnrep(UCHAR *str1, UCHAR *str2);
ULONG  vmm_strncmp(UCHAR *str1, UCHAR *str2, ULONG n);
ULONG  vmm_strncmpi(UCHAR *str1, UCHAR *str2, ULONG n);
ULONG vmm_strlen(UCHAR *str);
BOOLEAN vmm_strtoul(char *str, PULONG out);
BOOLEAN vmm_strtoul_10(char *str, PULONG result);
BOOLEAN vmm_strtoul_64(char *str, PULONG64 result);
VOID    vmm_memset(VOID *s, int c, ULONG n);
int     vmm_snprintf(char*, size_t, const char*, ...);
int     vmm_vsnprintf(char*, size_t, const char*, va_list);
BOOLEAN vmm_islower(char c);
BOOLEAN vmm_isupper(char c);
BOOLEAN vmm_isalpha(char c);
UCHAR   vmm_tolower(UCHAR c);
UCHAR   vmm_toupper(UCHAR c);
ULONG	vmm_strcmpi(PCHAR str1,PCHAR str2);
