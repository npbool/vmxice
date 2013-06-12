#include <linux/module.h>
#include <linux/io.h>
#include "mmu.h"
#include "vmmstring.h"

static UCHAR vmm_chartohex(char c);
static ULONG vmm_power(ULONG base, ULONG exp);

BOOLEAN vmm_islower(char c)
{
  return (c >= 'a' && c <= 'z');
}

BOOLEAN vmm_isupper(char c)
{
  return (c >= 'A' && c <= 'Z');
}

BOOLEAN vmm_isalpha(char c)
{
  return (vmm_islower(c) || vmm_isupper(c));
}

UCHAR vmm_tolower(UCHAR c)
{
  if (!vmm_isupper(c))
    return c;

  return (c - 'A' + 'a');
}

UCHAR vmm_toupper(UCHAR c)
{
  if (!vmm_islower(c))
    return c;

  return (c - 'a' + 'A');
}

VOID vmm_memset(VOID *s, int c, ULONG n)
{
  UCHAR *p;
  ULONG i;

  p = (UCHAR*) s;
  for (i=0; i<n; i++) {
    p[i] = (UCHAR) c;
  }
}

VOID vmm_strnrep(UCHAR *str1, UCHAR *str2)
{
	while(*str2)
	{
		*str1 = *str2;
		str1++;
		str2++;
	}
}

ULONG vmm_strncmpi(UCHAR *str1, UCHAR *str2, ULONG n)
{
  ULONG i;

  i = 0;
  while(i < n && str1[i] != 0 && str2[i] != 0) {
    if (vmm_tolower(str1[i]) != vmm_tolower(str2[i]))
      break;
    i++;
  }
  /* Strings match */
  if(i == n)
    return 0;
  else {
    /* str1 is "less than" str2 */
    if(vmm_tolower(str1[i]) < vmm_tolower(str2[i]))
      return -1;
    else   /* str1 is "more than" str2 */
      return 1;
  }
}

ULONG vmm_strncmp(UCHAR *str1, UCHAR *str2, ULONG n)
{
  ULONG i;

  i = 0;
  while (i < n && str1[i] != 0 && str2[i] != 0) {
    if (str1[i] != str2[i]) break;
    i++;
  }
  /* Strings match */
  if(i == n)
    return 0;
  else {
    /* str1 is "less than" str2 */
    if(vmm_tolower(str1[i]) < vmm_tolower(str2[i]))
      return -1;
    else   /* str1 is "more than" str2 */
      return 1;
  }

}

ULONG vmm_strlen(UCHAR *str)
{
  ULONG i;
  i = 0;
  while(str[i] != 0x00) i++;
  return i;
}

BOOLEAN vmm_strtoul_10(char *str, PULONG result)
{
  ULONG i, len;
  UCHAR tmp;
  len = 0;
  *result = 0;

  /* Get the length of the string */
  while(str[len] != 0) len++;

  for(i = 0; str[i] != 0; i++) {
    tmp = vmm_chartohex(str[len-1-i]);
    if (tmp > 9)
      return FALSE;

    *result = *result + tmp * vmm_power(10, i);
  }

  return TRUE;
}

BOOLEAN vmm_strtoul(char *str, PULONG result)
{
	ULONG i, len;
	UCHAR tmp;
	len = 0;
	*result = 0;
	if (str[0] == '0' && str[1] == 'x')
		str = &str[2];

	/* Get the length of the string */
	while(str[len] != 0) len++;

	for(i = 0; str[i] != 0; i++) {
		tmp = vmm_chartohex(str[len-1-i]);
		if (tmp > 15)
			return FALSE;

		*result = *result + tmp * vmm_power(16, i);
	}

	return TRUE;
}

BOOLEAN vmm_strtoul_64(char *str, PULONG64 result)
{
	ULONG i, len;
	UCHAR tmp;
	len = 0;
	*result = 0;
	if (str[0] == '0' && str[1] == 'x')
		str = &str[2];

	/* Get the length of the string */
	while(str[len] != 0) len++;

	for(i = 0; str[i] != 0; i++) {
		tmp = vmm_chartohex(str[len-1-i]);
		if (tmp > 15)
			return FALSE;

		*result = *result + tmp * vmm_power(16, i);
	}

	return TRUE;
}

static ULONG vmm_power(ULONG base, ULONG exp)
{
  if (exp == 0) return (ULONG) 1;
  else return (ULONG) (base * vmm_power(base, exp-1));
}

static UCHAR vmm_chartohex(char c)
{
  switch(c) {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    return c - '0';
  case 'a':
  case 'b':
  case 'c':
  case 'd':
  case 'e':
  case 'f':
    return c - 'a' + 10;
  case 'A':
  case 'B':
  case 'C':
  case 'D':
  case 'E':
  case 'F':
    return c - 'A' + 10;
  default:
    return -1;
  }
}

ULONG vmm_strcmpi(PCHAR str1,PCHAR str2)
{
	ULONG len1;

	if(!IsAddressExist((ULONG)str1))
		return 1;
	if(!IsAddressExist((ULONG)str2))
		return 1;

	len1 = strlen(str1);
	if(len1 != strlen(str2))
		return 1;

	if(!vmm_strncmpi(str1,str2,len1))
		return 0;
	return 1;
}
