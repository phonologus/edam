#include <ansi.h>
#include <posix.h>
#include "edam.h"

/*
 * Files are splayed out a factor of NDISC to reduce indirect block access
 */
Discdesc *files[NDISC];
Discdesc *transcripts[NDISC];
Buffer *undobuf;
int fcount;

enum{
   SKIP=50,   /* max dist between file changes folded together */
   MAXCACHE=25000   /* max length of cache. must be < 32K-BLOCKSIZE */
};


void Fstart(void);
File * Fopen(void);
void Fclose(File *f);
void Fmark(File *f, Mod m);
void Finsert(File *f, String *str, Posn p1);
void Fdelete(File *f, Posn p1, Posn  p2);
void Fflush(File *f);
void Fsetname(File *f);
int Fupdate(File *f, int isundo);
void puthdr_csP(Buffer *b, char c, short s, Posn p);
void puthdr_cs(Buffer *b, char c, short s);
void puthdr_M(Buffer *b, Posn p, Range dot, Range mk, Mod m, short s1);
void puthdr_cPP(Buffer *b, char c, Posn p1, Posn  p2);
int Fchars(File *f, char *addr, Posn p1, Posn  p2);
int Fgetcset(File *f, Posn p);
int Fbgetcset(File *f, Posn p);
int Fgetcload(File *f, Posn p);
int Fbgetcload(File *f, Posn p);
int Fgetc(File *f);
int Fbgetc(File *f);
int Fgetu8(File *f, char *p);
int Fbgetu8(File *f, char *p);

static String * ftempstr(char *s, int n);

void
Fstart(void)
{
   undobuf=Bopen(Dstart());
   unixbuf=Bopen(Dstart());
}
File *
Fopen(void)
{
   File *f;
   f=(File*)alloc(sizeof(File));
   if(files[fcount]==0){
      files[fcount]=Dstart();
      transcripts[fcount]=Dstart();
   }
   f->buf=Bopen(files[fcount]);
   f->transcript=Bopen(transcripts[fcount]);
   if(++fcount==NDISC)
      fcount=0;
   f->nbytes=0;
   f->markp=0;
   f->mod=0;
   f->dot.f=f;
   strinit(&f->name);
   straddc(&f->name, '\0');
   strinit(&f->cache);
   f->state=Unread;
   Fmark(f, (Mod)0);
   return f;
}
void
Fclose(File *f)
{
   Bclose(f->buf);
   Bclose(f->transcript);
   strclose(&f->name);
   strclose(&f->cache);
   free(f);
}
void
Fmark(File *f, Mod m)
{
   Buffer *t=f->transcript;
   Posn p;
   if(f->state==Unread)   /* this is implicit 'e' of a file */
      return;
   p=m==0? -1 : f->markp;
   f->markp=t->nbytes;
   puthdr_M(t, p, f->dot.r, f->mark, f->mod, f->state);
   f->marked=TRUE;
   f->mod=m;
   f->hiposn=-1;
   /* Safety first */
   f->cp1=f->cp2=0;
   strzero(&f->cache);
}
void
Finsert(File *f, String *str, Posn p1)
{
   Buffer *t=f->transcript;
   if(str->n==0)
      return;
   if(str->n<0 || str->n>32767)
      panic("Finsert");
   if(f->mod<modnum)
      Fmark(f, modnum);
   if(p1<f->hiposn)
      error(Esequence);
   if(str->n>=BLOCKSIZE){   /* don't bother with the cache */
      Fflush(f);
      puthdr_csP(t, 'i', str->n, p1);
      Binsert(t, str, t->nbytes);
   }else{   /* insert into the cache instead of the transcript */
      if(f->cp2==0 && f->cp1==0 && f->cache.n==0)   /* empty cache */
         f->cp1=f->cp2=p1;
      if(p1-f->cp2>SKIP || (long)f->cache.n+(long)str->n>MAXCACHE){
         Fflush(f);
         f->cp1=f->cp2=p1;
      }
      if(f->cp2!=p1){   /* grab the piece in between */
         char buf[SKIP];
         String s;
         Fchars(f, buf, f->cp2, p1);
         s.s=buf;
         s.n=p1-f->cp2;
         strinsert(&f->cache, &s, (long)f->cache.n);
         f->cp2=p1;
      }
      strinsert(&f->cache, str, (long)f->cache.n);
   }
   quitok=FALSE;
   f->closeok=FALSE;
   if(f->state==Clean)
      state(f, Dirty);
   f->hiposn=p1;
}
void
Fdelete(File *f, Posn p1, Posn  p2)
{
   if(p1==p2)
      return;
   if(f->mod<modnum)
      Fmark(f, modnum);
   if(p1<f->hiposn)
      error(Esequence);
   if(p1-f->cp2>SKIP)
      Fflush(f);
   if(f->cp2==0 && f->cp1==0 && f->cache.n==0)   /* empty cache */
      f->cp1=f->cp2=p1;
   if(f->cp2!=p1){   /* grab the piece in between */
      if(f->cache.n+(p1-f->cp2)>MAXCACHE){
         Fflush(f);
         f->cp1=f->cp2=p1;
      }else{
         char buf[SKIP];
         String s;
         Fchars(f, buf, f->cp2, p1);
         s.s=buf;
         s.n=p1-f->cp2;
         strinsert(&f->cache, &s, (long)f->cache.n);
      }
   }
   f->cp2=p2;
   quitok=FALSE;
   f->closeok=FALSE;
   if(f->state==Clean)
      state(f, Dirty);
   f->hiposn=p2;
}
void
Fflush(File *f)
{
   Buffer *t=f->transcript;
   Posn p1=f->cp1, p2=f->cp2;
   if(p1!=p2)
      puthdr_cPP(t, 'd', p1, p2);
   if(f->cache.n){
      puthdr_csP(t, 'i', f->cache.n, p2);
      Binsert(t, &f->cache, t->nbytes);
      strzero(&f->cache);
   }
   f->cp1=f->cp2=0;
}
void
Fsetname(File *f)
{
   Buffer *t=f->transcript;
   if(f->state==Unread){   /* This is setting initial file name */
      strdupstr(&f->name, &genstr);
      sortname(f);
   }else{
      if(f->mod<modnum)
         Fmark(f, modnum);
      if(genstr.n>BLOCKSIZE)
         error(Elong);
      puthdr_cs(t, 'f', genstr.n);
      Binsert(t, &genstr, t->nbytes);
   }
}
/*
 * The heart of it all. Fupdate will run along the transcript list, executing
 * the commands and converting them into their inverses for a later undo pass.
 * The pass runs top to bottom, so addresses in the transcript are tracked
 * (by the var. delta) so they stay valid during the operation.  This causes
 * all operations to appear to happen simultaneously, which is why the addresses
 * passed to Fdelete and Finsert never take into account other changes occurring
 * in this command (and is why things are done this way).
 */
int
Fupdate(File *f, int isundo)
{
   Buffer *t=f->transcript;
   Buffer *u=undobuf;
   int n, ni;
   Posn p0, p1, p2, p, deltadot=0, deltamark=0, delta=0;
   int changes=FALSE;
   char buf[32];
   char tmp[BLOCKSIZE];
   Fflush(f);
   if(f->marked)
      p0=f->markp+sizeof(Mark);
   else
      p0=0;
   while((n=Bread(t, buf, sizeof buf, p0))>0){
      switch(buf[0]){
      default:
         panic("unknown in Fupdate");
      case 'd':
         GETT(p1, buf+1, Posn);
         GETT(p2, buf+1+sizeof(Posn), Posn);
         p0+=1+2*sizeof(Posn);
         if(p2<=f->dot.r.p1)
            deltadot-=p2-p1;
         if(p2<=f->mark.p1)
            deltamark-=p2-p1;
         p1+=delta, p2+=delta;
         delta-=p2-p1;
         if(!isundo)
            for(p=p1; p<p2; p+=ni){
               if(p2-p>BLOCKSIZE)
                  ni=BLOCKSIZE;
               else
                  ni=p2-p;
               puthdr_csP(u, 'i', ni, p1);
               Bread(f->buf, tmp, ni, p);
               Binsert(u, ftempstr(tmp, ni), u->nbytes);
            }
         f->nbytes-=p2-p1;
         Bdelete(f->buf, p1, p2);
         changes=TRUE;
         break;
      case 'f':
         GETT(n, buf+1, short);
         p0+=1+sizeof(short);
         strinsure(&genstr, (ulong)n);
         Bread(t, tmp, n, p0);
         p0+=n;
         strdupl(&genstr, tmp);
         if(!isundo){
            puthdr_cs(u, 'f', f->name.n);
            Binsert(u, &f->name, u->nbytes);
         }
         strdupstr(&f->name, &genstr);
         sortname(f);
         changes=TRUE;
         break;
      case 'i':
         GETT(n, buf+1, short);
         GETT(p1, buf+1+sizeof(short), Posn);
         p0+=1+sizeof(short)+sizeof(Posn);
         if(p1<f->dot.r.p1)
            deltadot+=n;
         if(p1<f->mark.p1)
            deltamark+=n;
         p1+=delta;
         delta+=n;
         if(!isundo)
            puthdr_cPP(u, 'd', p1, p1+n);
         changes=TRUE;
         f->nbytes+=n;
         while(n>0){
            if(n>BLOCKSIZE)
               ni=BLOCKSIZE;
            else
               ni=n;
            Bread(t, tmp, ni, p0);
            Binsert(f->buf, ftempstr(tmp, ni), p1);
            n-=ni;
            p1+=ni;
            p0+=ni;
         }
         break;
      }
   }
   f->dot.r.p1+=deltadot;
   f->dot.r.p2+=deltadot;
   if(f->dot.r.p1>f->nbytes)
      f->dot.r.p1=f->nbytes;
   if(f->dot.r.p2>f->nbytes)
      f->dot.r.p2=f->nbytes;
   f->mark.p1+=deltamark;
   f->mark.p2+=deltamark;
   if(f->mark.p1>f->nbytes)
      f->mark.p1=f->nbytes;
   if(f->mark.p2>f->nbytes)
      f->mark.p2=f->nbytes;
   if(n<0)
      panic("Fupdate read");
   if(p0>f->markp+sizeof(Posn)){   /* for undo, this throws away the undo transcript */
      if(f->mod>0){   /* can't undo the dawn of time */
         Bdelete(t, f->markp+sizeof(Mark), t->nbytes);
         /* copy the undo list back into the transcript */
         for(p=0; p<u->nbytes; p+=ni){
            if(u->nbytes-p>BLOCKSIZE)
               ni=BLOCKSIZE;
            else
               ni=u->nbytes-p;
            Bread(u, tmp, ni, p);
            Binsert(t, ftempstr(tmp, ni), t->nbytes);
         }
      }
      Bdelete(u, (Posn)0, u->nbytes);
   }
   return changes;
}
void
puthdr_csP(Buffer *b, char c, short s, Posn p)
{
   char buf[1+sizeof(short)+sizeof p];
   char *a=buf;
   if(p<0)
      panic("puthdr_csP");
   PUTT(a, c, char);
   PUTT(a, s, short);
   PUTT(a, p, Posn);
   Binsert(b, ftempstr(buf, sizeof buf), b->nbytes);
}
void
puthdr_cs(Buffer *b, char c, short s)
{
   char buf[1+sizeof(short)];
   char *a=buf;
   PUTT(a, c, char);
   PUTT(a, s, short);
   Binsert(b, ftempstr(buf, sizeof buf), b->nbytes);
}
void
puthdr_M(Buffer *b, Posn p, Range dot, Range mk, Mod m, short s1)
{
   Mark mark;
   char buf[sizeof(Mark)];
   char *a=buf;
   static int first=1;
   if(!first && p<0)
      panic("puthdr_M");
   mark.p=p;
   mark.dot=dot;
   mark.mark=mk;
   mark.m=m;
   mark.s1=s1;
   PUTT(a, mark, Mark);
   Binsert(b, ftempstr(buf, sizeof buf), b->nbytes);
}
void
puthdr_cPP(Buffer *b, char c, Posn p1, Posn  p2)
{
   char buf[1+2*sizeof p1];
   char *a=buf;
   if(p1<0 || p2<0)
      panic("puthdr_cPP");
   PUTT(a, c, char);
   PUTT(a, p1, Posn);
   PUTT(a, p2, Posn);
   Binsert(b, ftempstr(buf, sizeof buf), b->nbytes);
}
int
Fchars(File *f, char *addr, Posn p1, Posn  p2)
{
   return Bread(f->buf, addr, (int)(p2-p1), p1);
}
int
Fgetcset(File *f, Posn p)
{
   if(p<0 || p>f->nbytes)
      panic("Fgetcset out of range");
   if((f->ngetc=Fchars(f, f->getcbuf, p, p+NGETC))<0)
      panic("Fgetcset Bread fail");
   f->getcp=p;
   f->getci=0;
   return f->ngetc;
}
int
Fbgetcset(File *f, Posn p)
{
   if(p<0 || p>f->nbytes)
      panic("Fbgetcset out of range");
   if((f->ngetc=Fchars(f, f->getcbuf, p<NGETC? (Posn)0 : p-NGETC, p))<0)
      panic("Fbgetcset Bread fail");
   f->getcp=p;
   f->getci=f->ngetc;
   return f->ngetc;
}
int
Fgetcload(File *f, Posn p)
{
   if(Fgetcset(f, p)){
      --f->ngetc;
      f->getcp++;
      return 0377&f->getcbuf[f->getci++];
   }
   return -1;
}
int
Fbgetcload(File *f, Posn p)
{
   if(Fbgetcset(f, p)){
      --f->getcp;
      return 0377&f->getcbuf[--f->getci];
   }
   return -1;
}
int
Fgetc(File *f)
{
   if(--f->ngetc<0)
      return Fgetcload(f, f->getcp);
   f->getcp++;
   return 0377&f->getcbuf[f->getci++];
}
int
Fbgetc(File *f)
{
   if(f->getci<=0)
      return Fgetcload(f, f->getcp);
   --f->getcp;
   return 0377&f->getcbuf[--f->getci];
}

static File *gfile;
int
Fgetter(void)
{
   return Fgetc(gfile);
}
int
Fbgetter(void)
{
   return Fbgetc(gfile);
}
int
Fgetu8(File *f, char *p)
{
   gfile=f;
   return u8getc(Fgetter,p);
}
int
Fbgetu8(File *f, char *p)
{
   gfile=f;
   return u8bgetc(Fbgetter,p);
}
static String *
ftempstr(char *s, int n)
{
   static String p;
   p.s=(char *)s;
   p.n=n;
   p.size=n;
   return &p;
}

