#include <ansi.h>
#include <posix.h>
#include "edam.h"

extern jmp_buf mainloop;

char errfile[64];
String unixcmd; /* null terminated */
Buffer *unixbuf;


void Unix(File *f, int type, String *s, int nest);
void checkerrs(void);

void
Unix(File *f, int type, String *s, int nest)
{
   int pid, rpid;
   long l;
   int m;
   sig_t onbpipe;
   int retcode;
   int pipe1[2], pipe2[2];
   if(s->s[0]==0 && unixcmd.s[0]==0)
      error(Enocmd);
   else if(s->s[0])
      strdupstr(&unixcmd, s);
   strcpy(errfile, (char *)"/dev/tty");
   if(type!='!' && pipe(pipe1)==-1)
      error(Epipe);
   if((pid=fork()) == 0){
      if(type!='!') {
         if(type=='<' || type=='|'){
            close(1);
            dup(pipe1[1]);
         }else if(type == '>'){
            close(0);
            dup(pipe1[0]);
         }
         close(pipe1[0]);
         close(pipe1[1]);
      }
      if(type=='|'){
         if(pipe(pipe2)==-1)
            exit(1);
         if((pid=fork())==0){
				/*
				 * It's ok if we get SIGPIPE here
             */
            close(pipe2[0]);
            if((retcode=!setjmp(mainloop)))   /* assignment = */
               for(l=0; l<unixbuf->nbytes; l+=m){
                  m=unixbuf->nbytes-l;
                  if(m>BLOCKSIZE)
                     m=BLOCKSIZE;
                  Bread(unixbuf, genbuf, m, l);
                  Write(pipe2[1], genbuf, m);
               }
            exit(retcode);
         }
         if(pid==-1){
            fprintf(stderr, "Can't fork?!\n");
            exit(1);
         }
         close(0);
         dup(pipe2[0]);
         close(pipe2[0]);
         close(pipe2[1]);
      }
      Dclosefd();
      if(type=='<'){
         close(0);   /* so it won't read from terminal */
         open("/dev/null", 0);
      }
      execlp("sh", "sh", "-c", unixcmd.s, (char *)0);
      exit(-1);
   }
   if(pid==-1)
      error(Efork);
   if(type=='<' || type=='|'){
      Fdelete(f, addr.r.p1, addr.r.p2);
      close(pipe1[1]);
      io=pipe1[0];
      f->dot.r.p2=addr.r.p2+readio(f, 0);
      f->dot.r.p1=addr.r.p2;
      closeio((Posn)-1);
   }else if(type=='>'){
      onbpipe = signal(SIGPIPE, SIG_IGN);
      close(pipe1[0]);
      io=pipe1[1];
      writeio(f);
      closeio((Posn)-1);
      signal(SIGPIPE, onbpipe);
   }
   do; while ((rpid = wait(&retcode)) != pid && rpid != -1);
   retcode=(retcode>>8)&0377;
   if(type=='|' || type=='<')
      if(retcode!=0)
         warn(Wbadstatus);
   if(!nest)
      dprint("!\n");
}
void
checkerrs(void)
{
   struct stat statb;
   char buf[256];
   int f;
   int n, nl;
   char *p;
   if(stat((char *)errfile, &statb)==0 && statb.st_size){
      if((f=open((char *)errfile, 0)) != -1){
         if((n=read(f, buf, sizeof buf-1))>0){
            for(nl=0,p=buf; nl<3 && p<&buf[n]; p++)
               if(*p=='\n')
                  nl++;
            *p=0;
            dprint("%s", buf);
            if(p-buf < statb.st_size-1)
               dprint("(edam: more in %s)\n", errfile);
         }
         close(f);
      }
   }else
      unlink((char *)errfile);
}
