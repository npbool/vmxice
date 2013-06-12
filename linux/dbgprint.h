#include "define.h"

#ifndef _DBGPRINT_H_
#define _DBGPRINT_H_

void asmlinkage printf(const char* fmt, ...);
void ttyprint(char *str);

#define RED_FONT ttyprint("\033[40;31m")
#define GREEN_FONT ttyprint("\033[40;32m")
#define YELLOW_FONT ttyprint("\033[40;33m")
#define DEFAULT_FONT ttyprint("\033[0m")

#endif
