#include <ansi.h>
#include <posix.h>
#include "edam.h"

/*
 * Check that list has room for one more element.
 */

void growlist(List *l);
void dellist(List *l, int i);
void inslist(List *l, int i, long val);
void listfree(List *l);

void
growlist(List *l)
{
   if(l->ptr==0 || l->nalloc==0){
      l->ptr=(long *)alloc((l->nalloc=INCR)*sizeof(l->ptr[0]));
      l->nused=0;
   }else if(l->nused==l->nalloc){
      l->ptr=(long *)allocre(l->ptr,(l->nalloc+=INCR)*sizeof(l->ptr[0]));
      memset(&l->ptr[l->nused],0,INCR*sizeof(l->ptr[0])); 
   }
}
/*
 * Remove the ith element from the list
 */
void
dellist(List *l, int i)
{
   memmove(&l->ptr[i],&l->ptr[i+1],(l->nused-(i+1))*sizeof(l->ptr[0]));
   l->nused--;
}
/*
 * Add a new element, whose position is i, to the list
 */
void
inslist(List *l, int i, long val)
{
   growlist(l);
   memmove(&l->ptr[i+1],&l->ptr[i],(l->nused-i)*sizeof(l->ptr[0]));
   l->ptr[i]=val;
   l->nused++;
}
void
listfree(List *l)
{
   free(l->ptr);
   free(l);
}
