#include <ansi.h>
#include <posix.h>
#include "edam.h"

#define MINSIZE 16 /* minimum number of chars allocated */
#define MAXSIZE BLOCKSIZE /* maximum number of chars for an empty string */
#define STRREALLOCSIZE 512
#define STRLIMIT ULONG_MAX-STRREALLOCSIZE


void strinit(String *p);
void strclose(String *p);
void strzero(String *p);
void strdupl(String *p, char *s);
void strdupstr(String *p, String  *q);
void straddc(String *p, int c);
void strinsure(String *p, ulong n);
void strinsert(String *p, String  *q, Posn p0);
void strdelete(String *p, Posn p1, Posn  p2);
String * tempstr(char *s, ulong n);

void
strinit(String *p)
{
   p->s=(char*)alloc(MINSIZE*sizeof(char));
   p->n=0;
   p->size=MINSIZE;
}
void
strclose(String *p)
{
   free(p->s);
}
void
strzero(String *p)
{
   if(p->size>MAXSIZE){
      p->s=(char*)allocre(p->s, MAXSIZE);   /* throw away the garbage */
      p->size=MAXSIZE;
   }
   p->n=0;
}
void
strdupl(String *p, char *s)
{
   strinsure(p, (ulong)(p->n=strlen(s)+1));
   memmove(p->s,s,p->n);
}
void
strdupstr(String *p, String  *q)
{
   strinsure(p, (ulong)q->n);
   p->n=q->n;
   memmove(p->s,q->s,q->n);
}
void
straddc(String *p, int c)
{
   strinsure(p, (ulong)p->n+1);
   p->s[p->n++]=c;
}
void
strinsure(String *p, ulong n)
{
   ulong i;

   if(n>=STRLIMIT)
     error(Etoolong);

   if(p->size<n){   /* p needs to grow */
      i=n+STRREALLOCSIZE;
      p->s=(char*)allocre(p->s, i);
      p->size=i;
   }
}
void
strinsert(String *p, String  *q, Posn p0)
{
   strinsure(p, (ulong)(p->n+q->n));
   if(p->n-p0>0)
      memmove(p->s+p0+q->n,p->s+p0,p->n-p0);
   memmove(p->s+p0,q->s,q->n);
   p->n+=q->n;
}
void
strdelete(String *p, Posn p1, Posn  p2)
{
   memmove(p->s+p1,p->s+p2,p->n-p2);
   p->n-=(p2-p1);
}
String *
tempstr(char *s, ulong n)
{
   static String p;
   p.s=s;
   p.n=n;
   p.size=n;
   return &p;
}

