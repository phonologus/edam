#include <ansi.h>
#include <posix.h>
#include "edam.h"

static Discdesc desc[NBUFFILES];
static char tempfname[30];
static int inited=0;

void Dremove(void);
Discdesc * Dstart(void);
void Dclosefd(void);
Disc * Dopen(Discdesc *dd);
void Dclose(Disc *d);
int Dread(Disc *d, char *addr, int n, Posn p1);
void Dinsert(Disc *d, char *addr, int n, Posn p0);
void Ddelete(Disc *d, Posn p1, Posn  p2);
void Dreplace(Disc *d, Posn p1, Posn  p2, char *addr, int n);
static void bkread(Disc *d, char *loc, int n, int bk, size_t off);
static void bkwrite(Disc *d, char *loc, int n, int bk, size_t off);
static void bkalloc(Disc *d, int n);
static void bkfree(Disc *d, int n);

void
Dremove(void)
{
   int i;
   Discdesc *dd;
   for(i=0, dd=desc; dd->fd; i++, dd++){
      sprintf(tempfname, "/tmp/edam%d.%d", getpid(), i);
      unlink(tempfname);
   }
}
Discdesc *
Dstart(void)
{
   int i;
   Discdesc *dd;
   int fd;
   for(i=0, dd=desc; dd->fd; i++, dd++)
      if(i==NBUFFILES-1)
         panic("too many buffer files");
   sprintf(tempfname, "/tmp/edam%d.%d", getpid(), i);
   fd=open(tempfname, O_CREAT | O_TRUNC | O_WRONLY, 0600);
   if(fd<0){
      unlink(tempfname);
      fd=open(tempfname, O_CREAT | O_TRUNC | O_WRONLY, 0600);
   }
   if(fd<0 || (close(fd), fd=open(tempfname, O_RDWR | O_CLOEXEC))<0)
      panic("can't create buffer file");
   dd->fd=fd;
   if(!inited++)
      atexit(Dremove);
   return dd;
}
void
Dclosefd(void)
{
   int i;
   for(i=0; i<NBUFFILES; i++)
      if(desc[i].fd)
         close(desc[i].fd);
}
Disc *
Dopen(Discdesc *dd)
{
   Disc *d;
   d=(Disc *)alloc(sizeof(Disc));
   d->desc=dd;
   return d;
}
void
Dclose(Disc *d)
{
   int i;
   for(i=d->block.nused; --i>=0; )   /* backwards because bkfree() stacks */
      bkfree(d, i);
   free(d->block.ptr);
   free(d);
}
int
Dread(Disc *d, char *addr, int n, Posn p1)
{
   int i, nb, nr;
   Posn p=0, p2=p1+n;
   for(i=0; i<d->block.nused; i++){
      if((p+=d->block.ptr[i].nbytes)>p1){
         p-=d->block.ptr[i].nbytes;
         goto out;
      }
   }
   if(p==p1)
      return 0;   /* eof */
   return -1;      /* past eof */
 out:
   n=0;
   if(p!=p1){   /* trailing partial block */
      nb=d->block.ptr[i].nbytes;
      if(p2>p+nb)
         nr=nb-(p1-p);
      else
         nr=p2-p1;
      bkread(d, addr, nr, i, p1-p);
      /* advance to next block */
      p+=nb;
      addr+=nr;
      n+=nr;
      i++;
   }
   /* whole blocks */
   while(p<p2 && (nb=d->block.ptr[i].nbytes)<=p2-p){
      if(i>=d->block.nused)
         return n;   /* eof */
      bkread(d, addr, nb, i, 0);
      p+=nb;
      addr+=nb;
      n+=nb;
      i++;
   }
   if(p<p2){   /* any initial partial block left? */
      nr=p2-p;
      nb=d->block.ptr[i].nbytes;
      if(nr>nb)
         nr=nb;      /* eof */
      /* just read in the part that survives */
      bkread(d, addr, nr, i, 0);
      n+=nr;
   }
   return n;
}
void
Dinsert(Disc *d, char *addr, int n, Posn p0)
{
   int i, nb, ni;
   Posn p=0;
   char hold[BLOCKSIZE];
   int nhold;
   for(i=0; i<d->block.nused; i++){
      if((p+=d->block.ptr[i].nbytes)>=p0){
         p-=d->block.ptr[i].nbytes;
         goto out;
      }
   }
   if(p!=p0)
      panic("Dinsert");  /* beyond eof */
 out:
   d->nbytes+=n;
   nhold=0;
   if(i<d->block.nused && (nb=d->block.ptr[i].nbytes)>p0-p){
      nhold=nb-(p0-p);
      bkread(d, hold, nhold, i, p0-p);
      d->block.ptr[i].nbytes-=nhold;   /* no write necessary */
   }
   /* insertion point is now at end of block i (which may not exist) */
   while(n>0){
      if(i<d->block.nused && (nb=d->block.ptr[i].nbytes)<BLOCKSIZE/2){
         /* fill this block */
         if(nb+n>BLOCKSIZE)
            ni=BLOCKSIZE/2-nb;
         else
            ni=n;
         if(addr)
            bkwrite(d, addr, ni, i, nb);
         nb+=ni;
      }else{   /* make new block */
         if(i<d->block.nused)
            i++;   /* put after this block, if it exists */
         bkalloc(d, i);
         if(n>BLOCKSIZE)
            ni=BLOCKSIZE/2;
         else
            ni=n;
         if(addr)
            bkwrite(d, addr, ni, i, 0);
         nb=ni;
      }
      d->block.ptr[i].nbytes=nb;
      if(addr)
         addr+=ni;
      n-=ni;
   }
   if(nhold){
      if(i<d->block.nused && (nb=d->block.ptr[i].nbytes)+nhold<BLOCKSIZE){
         /* fill this block */
         bkwrite(d, hold, nhold, i, nb);
         nb+=nhold;
      }else{   /* make new block */
         if(i<d->block.nused)
            i++;   /* put after this block, if it exists */
         bkalloc(d, i);
         bkwrite(d, hold, nhold, i, 0);
         nb=nhold;
      }
      d->block.ptr[i].nbytes=nb;
   }
}
void
Ddelete(Disc *d, Posn p1, Posn  p2)
{
   int i, nb, nd;
   Posn p=0;
   char buf[BLOCKSIZE];
   for(i=0; i<d->block.nused; i++){
      if((p+=d->block.ptr[i].nbytes)>p1){
         p-=d->block.ptr[i].nbytes;
         goto out;
      }
   }
   if(p1!=d->nbytes || p2!=p1)
      panic("Ddelete");
   return;   /* beyond eof */
 out:
   d->nbytes-=p2-p1;
   if(p!=p1){   /* throw away partial block */
      nb=d->block.ptr[i].nbytes;
      bkread(d, buf, nb, i, 0);
      if(p2>=p+nb)
         nd=nb-(p1-p);
      else{
         nd=p2-p1;
         memmove(buf+(p1-p),buf+(p1-p)+nd,(p1-p)+nd-nb);
      }
      nb-=nd;
      bkwrite(d, buf, nb, i, 0);
      d->block.ptr[i].nbytes=nb;
      p2-=nd;
      /* advance to next block */
      p+=nb;
      i++;
   }
   /* throw away whole blocks */
   while(p<p2 && (nb=d->block.ptr[i].nbytes)<=p2-p){
      if(i>=d->block.nused)
         panic("Ddelete 2");
      bkfree(d, i);
      p2-=nb;
   }
   if(p>=p2)   /* any initial partial block left to delete? */
      return;   /* no */
   nd=p2-p;
   nb=d->block.ptr[i].nbytes;
   /* just read in the part that survives */
   bkread(d, buf, nb-=nd, i, nd);
   /* a little block merging */
   if(nb<BLOCKSIZE/2 && i>0 && (nd=d->block.ptr[i-1].nbytes)<BLOCKSIZE/2){
      if(nb>0)
         memmove(buf+nd,buf,nb);
      bkread(d, buf, nd, --i, 0);
      bkfree(d, i);
      nb+=nd;
   }
   bkwrite(d, buf, nb, i, 0);
   d->block.ptr[i].nbytes=nb;
}
void
Dreplace(Disc *d, Posn p1, Posn  p2, char *addr, int n)
{
   int i, nb, nr;
   Posn p=0;
   char buf[BLOCKSIZE];
   if(p2-p1>n)
      Ddelete(d, p1+n, p2);
   else if(p2-p1<n)
      Dinsert(d, (char *)0, (int)(n-(p2-p1)), p2);
   if(n==0)
      return;
   p2=p1+n;
   /* they're now conformal; replace in place */
   for(i=0; i<d->block.nused; i++){
      if((p+=d->block.ptr[i].nbytes)>p1){
         p-=d->block.ptr[i].nbytes;
         goto out;
      }
   }
   panic("Dreplace");
 out:
   if(p!=p1){   /* trailing partial block */
      nb=d->block.ptr[i].nbytes;
      bkread(d, buf, nb, i, 0);
      if(p2>p+nb)
         nr=nb-(p1-p);
      else
         nr=p2-p1;
      memmove(buf+p1-p,addr,nr);
      bkwrite(d, buf, nb, i, 0);
      /* advance to next block */
      p+=nb;
      addr+=nr;
      i++;
   }
   /* whole blocks */
   while(p<p2 && (nb=d->block.ptr[i].nbytes)<=p2-p){
      if(i>=d->block.nused)
         panic("Dreplace 2");
      bkwrite(d, addr, nb, i, 0);
      p+=nb;
      addr+=nb;
      i++;
   }
   if(p<p2){   /* any initial partial block left? */
      nr=p2-p;
      nb=d->block.ptr[i].nbytes;
      /* just read in the part that survives */
      bkread(d, buf+nr, nb-nr, i, nr);
      memmove(buf,addr,nr);
      bkwrite(d, buf, nb, i, 0);
   }
}
static void
bkread(Disc *d, char *loc, int n, int bk, size_t off)
{
   Lseek(d->desc->fd, BLOCKSIZE*(long)d->block.ptr[bk].bnum+off, 0);
   Read(d->desc->fd, loc, n);
}
static void
bkwrite(Disc *d, char *loc, int n, int bk, size_t off)
{
   Lseek(d->desc->fd, BLOCKSIZE*(long)d->block.ptr[bk].bnum+off, 0);
   Write(d->desc->fd, loc, n);
}
static void
bkalloc(Disc *d, int n)
{
   Discdesc *dd=d->desc;
   int bnum;
   if(dd->free.nused)
      bnum=dd->free.ptr[--dd->free.nused];
   else
      bnum=dd->nbk++;
   inslist((List *)&d->block, n, 0L);
   d->block.ptr[n].bnum=bnum;
}
static void
bkfree(Disc *d, int n)
{
   Discdesc *dd=d->desc;
   inslist(&dd->free, dd->free.nused, (long)d->block.ptr[n].bnum);
   dellist((List *)&d->block, n);
}
