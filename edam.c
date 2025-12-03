#include <ansi.h>
#include <posix.h>
#include "edam.h"
#include <setjmp.h>
#include <signal.h>

typedef struct Patlist{
   int   nalloc;
   int   nused;
   String   **ptr;
}Patlist;

char genbuf[BLOCKSIZE];
char *tmpdir;
char *home;
int io;
int panicking;
int rescuing;
Mod modnum;
String genstr;
String rhs;
String wd;
String cmdstr;
File *curfile;
File *flist;
jmp_buf mainloop;
Filelist tempfile;
int quitok=TRUE;
Patlist globstr;
Patlist pattern;
int dounlock;


void panic(char *s);
void rescue(int s);
void hiccough(char *s);
void intr(int s);
void trytoclose(File *f);
void trytoquit(void);
void load(File *f);
void update(void);
File * current(File *f);
void edit(File *f, int cmd);
int getname(File *f, String *s, int save);
void filename(File *f);
void undo(void);
void undostep(File *f);
void cd(String *str);
void readcmd(String *s);
int loadflist(String *s);
File * readflist(int readall, int delete);
File * tofile(String *s);
File * getfile(String *s);
void closefiles(File *f, String *s);
void move(File *f, Address addr2);
void copy(File *f, Address addr2);
Posn nlcount(File *f, Posn p0, Posn  p1);
Posn charcount(File *f, Posn p0, Posn  p1);
Posn charposn(File *f, Posn p0, Posn  nc);
void printposn(File *f, int charsonly);
void settempfile(void);

int
main(int argc, char *argv[])
{
   int i;
   int optend=0;
   sig_t onintr;
if(sizeof(Block)!=sizeof(long))
panic("|Block| != |long|");
if(sizeof(Posn)!=sizeof(long*))
panic("|Posn| != |long*|");
   while(!optend && argc>1 && argv[1][0]=='-'){
      switch(argv[1][1]){
      case '-':
         optend++;
         break;
      case 'd':
         break;
      default:
         dprint("edam: unknown flag %c\n", argv[1][1]);
         return 1;
      }
      --argc, argv++;
   }
   Fstart();
   strinit(&cmdstr);
   strinit(&lastpat);
   strinit(&lastregexp);
   strinit(&genstr);
   strinit(&rhs);
   strinit(&wd);
/*
   tempfile.ptr=(File**)alloc(0);
*/
   strinit(&unixcmd);
   straddc(&unixcmd, '\0');
   home=getenv("HOME");
   if(home==0)
      home="/tmp";
   tmpdir=getenv("TMPDIR");
   if (tmpdir==0) 
      tmpdir="/tmp";
   if(argc>1){
      for(i=1; i<argc; i++)
{
         if(!setjmp(mainloop)){
            strdupl(&genstr, argv[i]);
            Fsetname(newfile());
         }
}
   }else{
      newfile()->state=Clean;
   }
   modnum++;
   onintr=signal(SIGINT, intr);
   if(onintr)
      signal(SIGINT, onintr);
   onintr=signal(SIGHUP, rescue);
   if(onintr)
      signal(SIGHUP, onintr);
   if(file.nused)
      current(file.ptr[0]);
   setjmp(mainloop);
   cmdloop();
   trytoquit();   /* if we already q'ed, quitok will be TRUE */
   return 0;
}
void
panic(char *s)
{
   if(!panicking++ && !setjmp(mainloop)){
      dprint("edam: panic: %s\n", s);
      rescue(0);
   }
   abort();
}
void
rescue(int s)
{
   int i, nblank=0;
   File *f;
   FILE *fio;
   char buf[128];
   (void)s;
   if(rescuing++)
      exit(1);
   signal(SIGHUP, SIG_IGN);
   io=-1;
   fio=NULL;
   for(i=0; i<file.nused; i++){
      f=file.ptr[i];
      if(f->nbytes==0 || f->state!=Dirty)
         continue;
      if(io==-1){
         strcpy(buf, (char *)home);
         strcpy(buf+strlen(buf), (char *)"/edam.save");
         io=open((char *)buf, O_CREAT | O_TRUNC | O_WRONLY, 0777);
         if(io<0)
            return;
         fio=fdopen(io,"w");
      }
      strcpy(buf, f->name.s);
      if(buf[0]==0)
         sprintf((char *)buf, "nameless.%d", nblank++);
      
      fprintf(fio, "/usr/bin/env edamsave '%s' $* <<'---%s'\n",
         (char *)buf, (char *)buf);
      fflush(fio);
      addr.r.p1=0, addr.r.p2=f->nbytes;
      writeio(f);
      fprintf(fio, "\n---%s\n", (char *)buf);
      fflush(fio);
   }
   if(panicking)
      abort();
   fclose(fio);
   exit(0);
}
void
hiccough(char *s)
{
   if(rescuing)
      exit(1);
   if(s)
      dprint("%s\n", s);
   resetcmd();
   resetxec();
   if(io>0)
      close(io);
   if(undobuf->nbytes)
      Bdelete(undobuf, (Posn)0, undobuf->nbytes);
   update();
   if(curfile && curfile->state==Unread)
      curfile->state=Clean;
   dounlock=TRUE;
   longjmp(mainloop, 1);
}
void
intr(int s)
{
   (void)s;
   signal(SIGINT, intr);
   error(Eintr);
}
void
trytoclose(File *f)
{
   if(f->state==Dirty && !f->closeok){
      f->closeok=TRUE;
      error_s(Emodified,
       f->name.s[0]?(char *)f->name.s : "nameless file");
   }
   delfile(f);
   if(f==curfile)
      current((File *)0);
}
void
trytoquit(void)
{
   int c;
   File *f;
   extern int eof;
   if(!quitok)
      for(c=0; c<file.nused; c++){
         f=file.ptr[c];
         if(f->state==Dirty){
            quitok=TRUE;
            eof=FALSE;
            error(Echanges);
         }
      }
}
void
load(File *f)
{
   Address saveaddr;
   strdupstr(&genstr, &f->name);
   filename(f);
   if(f->name.s[0]){
      saveaddr=addr;
      edit(f, 'I');
      addr=saveaddr;
   }else
      f->state=Clean;
   Fupdate(f, FALSE);
}
void
update(void)
{
   int i, anymod;
   File *f;
   settempfile();
   for(anymod=i=0; i<tempfile.nused; i++){
      f=tempfile.ptr[i];
      if(f->mod==modnum && Fupdate(f, FALSE))
         anymod++;
   }
   if(anymod)
      modnum++;
}
File *
current(File *f)
{
   return curfile=f;
}
void
edit(File *f, int cmd)
{
   int empty=TRUE;
   Posn p;
   if(cmd=='r')
      Fdelete(f, addr.r.p1, addr.r.p2);
   if(cmd=='e' || cmd=='I'){
      Fdelete(f, (Posn)0, f->nbytes);
      addr.r.p2=f->nbytes;
   }else if(f->nbytes!=0 || (f->name.s[0] && strcmp(genstr.s, f->name.s)!=0))
      empty=FALSE;
   if((io=open((char *)genstr.s, O_RDONLY))<0)
      error_s(Eopen, (char *)genstr.s);
   p=readio(f, empty);
   closeio((cmd=='e' || cmd=='I')? -1 : p);
   if(cmd=='r')
      f->dot.r.p1=addr.r.p2, f->dot.r.p2=addr.r.p2+p;
   else
      f->dot.r.p1=f->dot.r.p2=0;
   quitok=f->closeok=empty;
   state(f, empty ? Clean : Dirty);
   if(cmd=='e')
      filename(f);
}
int
getname(File *f, String *s, int save)
{
   int c, i;
   strzero(&genstr);
   if(s==0 || (c=s->s[0])==0){      /* no name provided */
      if(f)
         strdupstr(&genstr, &f->name);
      else
         straddc(&genstr, '\0');
      goto Return;
   }
   if(c!=' ' && c!='\t')
      error(Eblank);
   for(i=0; (c=s->s[i])==' ' || c=='\t'; i++)
      ;
   while(s->s[i]>' ')
      straddc(&genstr, s->s[i++]);
   if(s->s[i])
      error(Enewline);
   straddc(&genstr, '\0');
   if(f && (save || f->name.s[0]==0)){
      Fsetname(f);
      if(strcmp(f->name.s, genstr.s)){
         quitok=f->closeok=FALSE;
         f->inumber=0;
         f->date=0;
         state(f, Dirty); /* if it's 'e', fix later */
      }
   }
 Return:
   return genstr.n-1;   /* strlen(name) */
}
void
filename(File *f)
{
   dprint("%c%c%c %s\n", " '"[f->state==Dirty],
      "-+"[0], " ."[f==curfile], genstr.s);
}
void
undo(void)
{
   File *f;
   int i;
   Mod max;
   if((max=curfile->mod)==0)
      return;
   settempfile();
   for(i=0; i<tempfile.nused; i++){
      f=tempfile.ptr[i];
      if(f->mod==max)
         undostep(f);
   }
}
void
undostep(File *f)
{
   Buffer *t;
   int changes;
   Mark mark;
   t=f->transcript;
   changes=Fupdate(f, TRUE);
   Bread(t, (char *)&mark, sizeof mark, f->markp);
   Bdelete(t, f->markp, t->nbytes);
   f->markp=mark.p;
   f->dot.r=mark.dot;
   f->mark=mark.mark;
   f->mod=mark.m;
   f->closeok=mark.s1!=Dirty;
   if(mark.s1==Dirty)
      quitok=FALSE;
   if(f->state==Clean && mark.s1==Clean && changes)
      state(f, Dirty);
   else
      state(f, mark.s1);
}
void
cd(String *str)
{
   int i;
   File *f;
   readcmd(tempstr((char *)"/bin/pwd", 9));
   strdupstr(&wd, &genstr);
   if(wd.s[0]==0){
      wd.n=0;
      warn(Wpwd);
   }else if(wd.s[wd.n-2]=='\n'){
      --wd.n;
      wd.s[wd.n-1]='/';
   }
   if(chdir(getname((File *)0, str, FALSE)? (char *)genstr.s : home))
      syserror("chdir");
   settempfile();
   for(i=0; i<tempfile.nused; i++){
      f=tempfile.ptr[i];
      if(f->name.s[0]!='/' && f->name.s[0]!=0){
         strinsert(&f->name, &wd, (Posn)0);
         sortname(f);
      }
   }
}
void
readcmd(String *s)
{
   if(flist==0)
      (flist=Fopen())->state=Clean;
   addr.r.p1=0, addr.r.p2=flist->nbytes;
   Unix(flist, '<', s, FALSE);
   Fupdate(flist, FALSE);
   flist->mod=0;
   strzero(&genstr);
   strinsure(&genstr, flist->nbytes);
   Fchars(flist, genbuf, (Posn)0, flist->nbytes);
   memmove(genstr.s,genbuf,flist->nbytes);
   genstr.n=flist->nbytes;
   straddc(&genstr, '\0');
}
int
loadflist(String *s)
{
   int c, i;
   c=s->s[0];
   for(i=0; s->s[i]==' ' || s->s[i]=='\t'; i++)
      ;
   if((c==' ' || c=='\t') && s->s[i]!='\n'){
      if(s->s[i]=='<'){
         strdelete(s, 0L, (long)i+1);
         readcmd(s);
      }else{
         strzero(&genstr);
         while((c=s->s[i++]) && c!='\n')
            straddc(&genstr, c);
         straddc(&genstr, '\0');
      }
   }else{
      if(c!='\n')
         error(Eblank);
      strdupl(&genstr, (char *)"");
   }
   return genstr.s[0];
}
File *
readflist(int readall, int delete)
{
   Posn i;
   int c;
   File *f;
   for(i=0,f=0; f==0 || readall || delete; ){
      strdelete(&genstr, (Posn)0, i);
      for(i=0; (c=genstr.s[i])==' ' || c=='\t' || c=='\n'; i++)
         ;
      if(i>=genstr.n)
         break;
      strdelete(&genstr, (Posn)0, i);
      for(i=0; (c=genstr.s[i]) && c!=' ' && c!='\t' && c!='\n'; i++)
         ;
      if(i==0)
         break;
      genstr.s[i++]=0;
      f=lookfile();
      if(delete){
         if(f==0)
            warn_s(Wfile, (char *)genstr.s);
         else
            trytoclose(f);
      }else if(f==0 && readall)
         Fsetname(f=newfile());
   }
   return f;
}
File *
tofile(String *s)
{
   File *f;
   if(s->s[0]!=' ')
      error(Eblank);
   if(loadflist(s)==0){
      f=lookfile();   /* empty string ==> nameless file */
      if(f==0)
         error_s(Emenu, (char *)genstr.s);
   }else if((f=readflist(FALSE, FALSE))==0)
      error_s(Emenu, (char *)genstr.s);
   return current(f);
}
File *
getfile(String *s)
{
   File *f;
   if(loadflist(s)==0)
      Fsetname(f=newfile());
   else if((f=readflist(TRUE, FALSE))==0)
      error(Eblank);
   return current(f);
}
void
closefiles(File *f, String *s)
{
   if(s->s[0]==0){
      if(f==0)
         error(Enofile);
      trytoclose(f);
      return;
   }
   if(s->s[0]!=' ')
      error(Eblank);
   if(loadflist(s)==0)
      error(Enewline);
   readflist(FALSE, TRUE);
}
void
move(File *f, Address addr2)
{
   if(addr.r.p2<=addr2.r.p2){
      Fdelete(f, addr.r.p1, addr.r.p2);
      copy(f, addr2);
   }else if(addr.r.p1>=addr2.r.p2){
      copy(f, addr2);
      Fdelete(f, addr.r.p1, addr.r.p2);
   }else
      error(Eoverlap);
}
void
copy(File *f, Address addr2)
{
   Posn p;
   int ni;
   for(p=addr.r.p1; p<addr.r.p2; p+=ni){
      ni=addr.r.p2-p;
      if(ni>BLOCKSIZE)
         ni=BLOCKSIZE;
      Fchars(f, genbuf, p, p+ni);
      Finsert(addr2.f, tempstr(genbuf, ni), addr2.r.p2);
   }
   addr2.f->dot.r.p2=addr2.r.p2+(f->dot.r.p2-f->dot.r.p1);
   addr2.f->dot.r.p1=addr2.r.p2;
}
Posn
nlcount(File *f, Posn p0, Posn  p1)
{
   Posn nl=0;
   Fgetcset(f, p0);
   while(p0++<p1)
      if(Fgetc(f)=='\n')
         nl++;
   return nl;
}
Posn
charcount(File *f, Posn p0, Posn  p1)
{
   int i;
   char p[4];
   Posn nc=0;
   Fgetcset(f, p0);
   while(p0<p1){
      if((i=Fgetu8(f,p))>0){
         p0+=i;
         nc++;
      }else if(i<0){
         warn(Wnonutf);
         p0-=i;
      }else
         error(Erange);
   }
   return nc;
}
Posn
charposn(File *f, Posn p0, Posn  nc)
{
   int i;
   char p[4];
   Fgetcset(f, p0);
   if(nc>0){
      while(nc-->0){
         if((i=Fgetu8(f,p))>0){
            p0+=i;
         }else if(i<0){
            warn(Wnonutf);
            nc++;
            p0-=i;
         }else
            error(Erange);
      }
   }else{
      nc=-nc;
      while(nc-->0){
         if((i=Fbgetu8(f,p))>0){
            p0-=i;
         }else if(i<0){
            warn(Wnonutf);
            nc++;
            p0+=i;
         }else
            error(Erange);
      }
   }
   return p0;
}
void
printposn(File *f, int charsonly)
{
   Posn l1, l2;
   Posn c1, c2;
   if(!charsonly){
      l1=1+nlcount(f, (Posn)0, addr.r.p1);
      l2=l1+nlcount(f, addr.r.p1, addr.r.p2);
      /* check if addr ends with '\n' */
      if(addr.r.p2>0 && addr.r.p2>addr.r.p1 && (Fgetcset(f, addr.r.p2-1),Fgetc(f)=='\n'))
         --l2;
      dprint("%lu", l1);
      if(l2!=l1)
         dprint(",%lu", l2);
      dprint("; ");
   }
   c1=charcount(f, (Posn)0, addr.r.p1);
   c2=c1+charcount(f, addr.r.p1, addr.r.p2);
   dprint("#%lu", c1);
   if(c2!=c1)
      dprint(",#%lu", c2);
   dprint("\n");
}
void
settempfile(void)
{
   if(tempfile.nalloc<file.nused){
      free(tempfile.ptr);
      tempfile.ptr=(File**)alloc(sizeof(file.ptr[0])*file.nused);
      tempfile.nalloc=file.nused;
   }
   tempfile.nused=file.nused;
   memmove(&tempfile.ptr[0],&file.ptr[0],file.nused*sizeof(file.ptr[0]));
}
