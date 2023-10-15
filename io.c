#include <ansi.h>
#include <posix.h>
#include "edam.h"

void writef(File *f);
Posn readio(File *f, int setdate);
Posn writeio(File *f);
void closeio(Posn p);

void
writef(File *f)
{
   char c;
   Posn n;
   struct stat statb;
   int newfile=0;
   if(stat((char *)genstr.s, &statb)==-1)
      newfile++;
   else if(strcmp(genstr.s, f->name.s)==0 &&
    (f->inumber!=statb.st_ino || f->date<statb.st_mtime)){
      f->inumber=statb.st_ino;
      f->date=statb.st_mtime;
      warn_s(Wdate, (char *)genstr.s);
      return;
   }
   if((io=open((char *)genstr.s, O_CREAT | O_TRUNC | O_WRONLY ,0666))<0)
      error_s(Ecreate, (char *)genstr.s);
   dprint("%s: ", (char *)genstr.s);
   n=writeio(f);
   if(f->name.s[0]==0 || strcmp(genstr.s, f->name.s)==0)
      state(f, addr.r.p1==0 && addr.r.p2==f->nbytes? Clean : Dirty);
   if(newfile)
      dprint("(new file) ");
   if(addr.r.p2>0 && Fchars(f, &c, addr.r.p2-1, addr.r.p2) && c!='\n')
      warn(Wnotnewline);
   if(f->name.s[0]==0 || strcmp(genstr.s, f->name.s)==0){
      fstat(io, &statb);
      f->inumber=statb.st_ino;
      f->date=statb.st_mtime;
   }
   closeio(n);
}
Posn
readio(File *f, int setdate)
{
   int n;
   Posn nt;
   Posn p=addr.r.p2;
   struct stat statb;
   for(nt=0; (n=read(io, (char *)genbuf, BLOCKSIZE))>0; nt+=n){
      Finsert(f, tempstr(genbuf, n), p);
   }
   if(setdate){
      fstat(io, &statb);
      f->inumber=statb.st_ino;
      f->date=statb.st_mtime;
   }
   return nt;
}
Posn
writeio(File *f)
{
   int n;
   Posn p=addr.r.p1;
   while(p<addr.r.p2){
      if(addr.r.p2-p>BLOCKSIZE)
         n=BLOCKSIZE;
      else
         n=addr.r.p2-p;
      if(Fchars(f, genbuf, p, p+n)!=n)
         panic("writef read");
      Write(io, genbuf, n);
      p+=n;
   }
   return addr.r.p2-addr.r.p1;
}
void
closeio(Posn p)
{
   close(io);
   io=0;
   if(p>=0)
      dprint("#%lu\n", p);
}

