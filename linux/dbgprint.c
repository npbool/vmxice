#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>    /* For current */
#include <linux/tty.h>      /* For the tty declarations */
#include <linux/version.h>  /* For LINUX_VERSION_CODE */

#include "dbgprint.h"

void ttyprint(char *str)
{
    struct tty_struct *my_tty;

#if ( LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,5) )
    my_tty = current->tty;
#else
    my_tty = current->signal->tty;
#endif
    if (my_tty != NULL)
    {
        my_tty->ops->write (my_tty,str,strlen(str));
    }
}

void asmlinkage printf(const char* fmt, ...)
{
  va_list args;
  char str[256] = {0};

  va_start(args,fmt);
  vsprintf(str,fmt,args);
  va_end(args);

  ttyprint(str);
}
