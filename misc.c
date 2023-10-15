#include <ansi.h>
#include <posix.h>
#include "edam.h"

Posn getnum(void);
int skipbl(void);

Posn
getnum(void)
{
   Posn n=0;
   int c;
   if((c=nextch())<'0' || '9'<c)   /* no number defaults to 1 */
      return 1;
   while('0'<=(c=getch()) && c<='9')
      n=n*10+c-'0';
   ungetch();
   return n;
}
int
skipbl(void)
{
   int c;
   do c=getch(); while(c==' ' || c=='\t');
   if(c>=0)
      ungetch();
   return c;
}
