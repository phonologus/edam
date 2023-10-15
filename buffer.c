#include <ansi.h>
#include <posix.h>
#include "edam.h"

Buffer * Bopen(Discdesc *dd);
void Bclose(Buffer *b);
int Bread(Buffer *b, char *addr, int n, Posn p0);
void Binsert(Buffer *b, String *s, Posn p0);
void Bdelete(Buffer *b, Posn p1, Posn  p2);
static void flush(Buffer *b);
int incache(Buffer *b, Posn p1, Posn  p2);
/*
static void setcache(Buffer *b, String *s, Posn p0);
*/

Buffer *
Bopen(Discdesc *dd)
{
   Buffer *b;
   b=(Buffer*)alloc(sizeof(Buffer));
   b->disc=Dopen(dd);
   strinit(&b->cache);
   return b;
}
void
Bclose(Buffer *b)
{
   Dclose(b->disc);
   strclose(&b->cache);
   free(b);
}
int
Bread(Buffer *b, char *addr, int n, Posn p0)
{
   int m;
   if(b->c2>b->disc->nbytes || b->c1>b->disc->nbytes)
      panic("Bread cache");
   if(p0<0)
      panic("Bread p0<0");
   if(p0+n>b->nbytes){
      n=b->nbytes-p0;
      if(n<0)
         panic("Bread<0");
   }
   if(!incache(b, p0, p0+n)){
      flush(b);
      if(n>=BLOCKSIZE/2)
         return Dread(b->disc, addr, n, p0);
      else{
         Posn minp;
         if(b->nbytes-p0>BLOCKSIZE/2)
            m=BLOCKSIZE/2;
         else
            m=b->nbytes-p0;
         if(m<n)
            m=n;
         minp=p0-BLOCKSIZE/2;
         if(minp<0)
            minp=0;
         m+=p0-minp;
         strinsure(&b->cache, (ulong)m);
         if(Dread(b->disc, b->cache.s, m, minp)!=m)
            panic("Bread");
         b->cache.n=m;
         b->c1=minp;
         b->c2=minp+m;
         b->dirty=FALSE;
      }
   }
   memmove(addr,&b->cache.s[p0-b->c1],n);
   return n;
}
void
Binsert(Buffer *b, String *s, Posn p0)
{
   if(b->c2>b->disc->nbytes || b->c1>b->disc->nbytes)
      panic("binsert cache");
   if(p0<0)
      panic("Binsert p0<0");
   if(s->n==0)
      return;
   if(incache(b, p0, p0)){
      strinsert(&b->cache, s, p0-b->c1);
      b->dirty=TRUE;
      if(b->cache.n>BLOCKSIZE*2){
         b->nbytes+=s->n;
         flush(b);
         /* try to leave some cache around p0 */
         if(p0>=b->c1+BLOCKSIZE){
            /* first BLOCKSIZE can go */
            strdelete(&b->cache, 0, BLOCKSIZE);
            b->c1+=BLOCKSIZE;
         }else if(p0<=b->c2-BLOCKSIZE){
            /* last BLOCKSIZE can go */
            b->cache.n-=BLOCKSIZE;
            b->c2-=BLOCKSIZE;
         }else{
            /* too hard; negate the cache and pick up next time */
            b->cache.n=0;
            b->c1=b->c2=0;
         }
         return;
      }
   }else{
      flush(b);
      if(s->n>=BLOCKSIZE/2){
         b->cache.n=0;
         b->c1=b->c2=0;
         Dinsert(b->disc, s->s, s->n, p0);
      }else{
         int m;
         Posn minp;
         if(b->nbytes-p0>BLOCKSIZE/2)
            m=BLOCKSIZE/2;
         else
            m=b->nbytes-p0;
         minp=p0-BLOCKSIZE/2;
         if(minp<0)
            minp=0;
         m+=p0-minp;
         strinsure(&b->cache, (ulong)m);
         if(Dread(b->disc, b->cache.s, m, minp)!=m)
            panic("Bread");
         b->cache.n=m;
         b->c1=minp;
         b->c2=minp+m;
         strinsert(&b->cache, s, p0-b->c1);
         b->dirty=TRUE;
      }
   }
   b->nbytes+=s->n;
}
void
Bdelete(Buffer *b, Posn p1, Posn  p2)
{
   if(p1<0 || p2<0)
      panic("Bdelete p<0");
   if(b->c2>b->disc->nbytes || b->c1>b->disc->nbytes)
      panic("bdelete cache");
   if(p1==p2)
      return;
   if(incache(b, p1, p2)){
      strdelete(&b->cache, p1-b->c1, p2-b->c1);
      b->dirty=TRUE;
   }else{
      flush(b);
      Ddelete(b->disc, p1, p2);
      b->cache.n=0;
      b->c1=b->c2=0;
   }
   b->nbytes-=(p2-p1);
}
static void
flush(Buffer *b)
{
   if(b->dirty){
      Dreplace(b->disc, b->c1, b->c2, b->cache.s, b->cache.n);
      b->c2=b->c1+b->cache.n;
      b->dirty=FALSE;
      if(b->nbytes!=b->disc->nbytes)
         panic("flush");
   }
}
int hits, misses;
int
incache(Buffer *b, Posn p1, Posn  p2)
{
if(b->c1<=p1 && p2<=b->c1+b->cache.n)hits++; else misses++;
   return b->c1<=p1 && p2<=b->c1+b->cache.n;
}
/*
static void
setcache(Buffer *b, String *s, Posn p0)
{
   strdelete(&b->cache, (long)0, (long)b->cache.n);
   strinsert(&b->cache, s, (long)0);
   b->c1=p0;
   b->c2=p0+s->n;
   b->dirty=FALSE;
}
*/
