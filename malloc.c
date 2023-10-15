#include <ansi.h>
#include <posix.h>
#include "edam.h"

void * alloc(size_t n);
void * allocre(void *p, size_t n);

void *
alloc(size_t n)
{
   void *r;
   if((r=malloc(n))==NULL)
      error(Ealloc);
   memset(r,0,n);
   return r;
}

void *
allocre(void *p, size_t n)
{
   void *r;
   if((r=realloc(p,n))==NULL)
      error(Ealloc);
   return r;
}
