#include <ansi.h>
#include <posix.h>
#include "edam.h"

static char linex[]="\n";
static char wordx[]=" \t\n";
struct cmdtab cmdtab[]={
/*   cmdc   text   regexp   addr   defcmd   defaddr   count   token    fn   */
   {'\n',   0,   0,   0,   0,   aDot,   0,   0,   nl_cmd},
   {'a',   1,   0,   0,   0,   aDot,   0,   0,   a_cmd},
   {'b',   0,   0,   0,   0,   aNo,   0,   linex,   b_cmd},
   {'B',   0,   0,   0,   0,   aNo,   0,   linex,   b_cmd},
   {'c',   1,   0,   0,   0,   aDot,   0,   0,   c_cmd},
   {'d',   0,   0,   0,   0,   aDot,   0,   0,   d_cmd},
   {'D',   0,   0,   0,   0,   aNo,   0,   linex,   D_cmd},
   {'e',   0,   0,   0,   0,   aNo,   0,   wordx,   e_cmd},
   {'f',   0,   0,   0,   0,   aNo,   0,   wordx,   f_cmd},
   {'g',   0,   1,   0,   'p',   aDot,   0,   0,   g_cmd},
   {'i',   1,   0,   0,   0,   aDot,   0,   0,   i_cmd},
   {'k',   0,   0,   0,   0,   aDot,   0,   0,   k_cmd},
   {'m',   0,   0,   1,   0,   aDot,   0,   0,   m_cmd},
   {'n',   0,   0,   0,   0,   aNo,   0,   0,   n_cmd},
   {'p',   0,   0,   0,   0,   aDot,   0,   0,   p_cmd},
   {'q',   0,   0,   0,   0,   aNo,   0,   0,   q_cmd},
   {'r',   0,   0,   0,   0,   aDot,   0,   wordx,   e_cmd},
   {'s',   0,   1,   0,   0,   aDot,   1,   0,   s_cmd},
   {'t',   0,   0,   1,   0,   aDot,   0,   0,   m_cmd},
   {'u',   0,   0,   0,   0,   aNo,   1,   0,   u_cmd},
   {'v',   0,   1,   0,   'p',   aDot,   0,   0,   g_cmd},
   {'w',   0,   0,   0,   0,   aAll,   0,   wordx,   w_cmd},
   {'x',   0,   1,   0,   'p',   aDot,   0,   0,   x_cmd},
   {'y',   0,   1,   0,   'p',   aDot,   0,   0,   x_cmd},
   {'X',   0,   1,   0,   'f',   aNo,   0,   0,   X_cmd},
   {'Y',   0,   1,   0,   'f',   aNo,   0,   0,   X_cmd},
   {'!',   0,   0,   0,   0,   aNo,   0,   linex,   unix_cmd},
   {'>',   0,   0,   0,   0,   aDot,   0,   linex,   unix_cmd},
   {'<',   0,   0,   0,   0,   aDot,   0,   linex,   unix_cmd},
   {'|',   0,   0,   0,   0,   aDot,   0,   linex,   unix_cmd},
   {'=',   0,   0,   0,   0,   aDot,   0,   linex,   eq_cmd},
   {'c'|0x100,0,   0,   0,   0,   aNo,   0,   wordx,   cd_cmd},
   {0,   0,   0,   0,   0,   0,   0,   0,   0},
};

char line[BLOCKSIZE];
char *linep=line;
List cmdlist;
List addrlist;
List relist;
List stringlist;
int eof;

void resetcmd(void);
int getch(void);
int nextch(void);
void ungetch(void);
int inputline(void);
int inputch(void);
void cmdloop(void);
Cmd * newcmd(void);
Addr * newaddr(void);
Regexp * newre(void);
String * newstring(void);
void freecmd(void);
int lookup(int c);
void okdelim(int c);
void atnl(void);
void getrhs(String *s, int delim, int cmd);
String * collecttoken(char *end);
String * collecttext(void);
Cmd * parsecmd(int nest);
Regexp * getregexp(int delim);
Addr * compoundaddr(void);
Addr * simpleaddr(void);

void
resetcmd(void)
{
   linep=line;
   *linep=0;
   freecmd();
}
int
getch(void)
{
   if(eof)
      return -1;
   if(*linep==0 && inputline()<0){
      eof=TRUE;
      return -1;
   }
   return 0377&*linep++;
}
int
nextch(void)
{
   if(*linep==0)
      return -1;
   return 0377&*linep;
}
void
ungetch(void)
{
   if(--linep<line)
      panic("ungetch");
}
int
inputline(void)
{
   int i, c;
   linep=line;
   i=0;
   do{
      if((c=inputch())<=0)
         return -1;
      if(i==(sizeof line)-1)
         error(Etoolong);
   }while((line[i++]=c)!='\n');
   line[i]=0;
   return 1;
}
int
inputch(void)
{
   char c;
   if(read(0, &c, 1)<=0)
      return -1;
   else
      return c&0377;
}
void
cmdloop(void)
{
   Cmd *cmdp;
   for(;;){
      if(curfile && curfile->state==Unread)
         load(curfile);
      if((cmdp=parsecmd(0))==0)
         break;
      if(cmdexec(curfile, cmdp)==0)
         break;
      freecmd();
      update();
   }
}
Cmd *
newcmd(void)
{
   Cmd *p;
   p=(Cmd*)alloc(sizeof(Cmd));
   inslist(&cmdlist, cmdlist.nused, (long)p);
   return p;
}
Addr *
newaddr(void)
{
   Addr *p;
   p=(Addr*)alloc(sizeof(Addr));
   inslist(&addrlist, addrlist.nused, (long)p);
   return p;
}
Regexp *
newre(void)
{
   Regexp *p;
   p=(Regexp*)alloc(sizeof(Regexp));
   inslist(&relist, relist.nused, (long)p);
   strinit(&p->text);
   return p;
}
String *
newstring(void)
{
   String *p;
   p=(String*)alloc(sizeof(String));
   inslist(&stringlist, stringlist.nused, (long)p);
   strinit(p);
   return p;
}
void
freecmd(void)
{
   int i;
   while(cmdlist.nused>0)
      free((char *)cmdlist.ptr[--cmdlist.nused]);
   while(addrlist.nused>0)
      free((char *)addrlist.ptr[--addrlist.nused]);
   while(relist.nused>0){
      i=--relist.nused;
      strclose(&((Regexp *)relist.ptr[i])->text);
      free((char *)relist.ptr[i]);   /* WARNING!! */
   }
   while(stringlist.nused>0){
      i=--stringlist.nused;
      strclose((String *)stringlist.ptr[i]);
      free((char *)stringlist.ptr[i]);
   }
}
int
lookup(int c)
{
   int i;
   for(i=0; cmdtab[i].cmdc; i++)
      if(cmdtab[i].cmdc==c)
         return i;
   return -1;
}
void
okdelim(int c)
{
   if(c=='\\' || ('a'<=c && c<='z') || ('A'<=c && c<='Z') || ('0'<=c && c<='9'))
      error_c(Edelim, c);
}
void
atnl(void)
{
   skipbl();
   if(getch()!='\n')
      error(Enewline);
}
void
getrhs(String *s, int delim, int cmd)
{
   int c;
   while((c=getch())>0 && c!=delim && c!='\n'){
      if(c=='\\'){
         if((c=getch())<=0)
            error(Ebadrhs);
         if(c=='\n'){
            ungetch();
            c='\\';
         }else if(c=='n')
            c='\n';
         else if(c!=delim && (cmd=='s' || c!='\\'))   /* s does its own */
            straddc(s, '\\');
      }
      straddc(s, c);
   }
   ungetch();   /* let client read whether delimeter, '\n' or whatever */
}
String *
collecttoken(char *end)
{
   String *s=newstring();
   int c;
   while((c=nextch())==' ' || c=='\t')
      straddc(s, getch()); /* blanks significant for getname() */
   while((c=getch())>0 && strchr(end, c)==0)
      straddc(s, c);
   straddc(s, 0);
   if(c!='\n')
      atnl();
   return s;
}
String *
collecttext(void)
{
   String *s=newstring();
   int begline, i;
   int c, delim;
   if(skipbl()=='\n'){
      getch();
      i=0;
      do{
         begline=i;
         while((c=getch())>0 && c!='\n')
            i++, straddc(s, c);
         i++, straddc(s, '\n');
         if(c<0)
            goto Return;
      }while(s->s[begline]!='.' || s->s[begline+1]!='\n');
      strdelete(s, (long)s->n-2, (long)s->n);
   }else{
      okdelim(delim=getch());
      getrhs(s, delim, 'a');
      if(nextch()==delim)
         getch();
      atnl();
   }
 Return:
   straddc(s, 0);      /* JUST FOR CMDPRINT() */
   return s;
}
Cmd *
parsecmd(int nest)
{
   int i, c;
   struct cmdtab *ct;
   Cmd *cp, *ncp;
   Cmd cmd;
   cmd.next=cmd.ccmd=0;
   cmd.re=0;
   cmd.flag=cmd.num=0;
   cmd.addr=compoundaddr();
   if(skipbl()==-1)
      return 0;
   if((c=getch())==-1)
      return 0;
   cmd.cmdc=c;
   if(cmd.cmdc=='c' && nextch()=='d'){   /* sleazy two-character case */
      getch();      /* the 'd' */
      cmd.cmdc='c'|0x100;
   }
   i=lookup(cmd.cmdc);
   if(i>=0){
      if(cmd.cmdc=='\n')
         goto Return;   /* let nl_cmd work it all out */
      ct=&cmdtab[i];
      if(ct->defaddr==aNo && cmd.addr)
         error(Enoaddr);
      if(ct->count)
         cmd.num=getnum();
      if(ct->regexp){
         /* x without pattern -> .*\n, indicated by cmd.re==0 */
         /* X without pattern is all files */
         if((ct->cmdc!='x' && ct->cmdc!='X') ||
          ((c=nextch())!=' ' && c!='\t' && c!='\n')){
            skipbl();
            if((c=getch())=='\n' || c<0)
               error(Enopattern);
            okdelim(c);
            cmd.re=getregexp(c);
            if(ct->cmdc=='s'){
               cmd.ctext=newstring();
               getrhs(cmd.ctext, c, 's');
               if(nextch()==c){
                  getch();
                  if(nextch()=='g')
                     cmd.flag=getch();
               }
         
            }
         }
      }
      if(ct->addr && (cmd.caddr=simpleaddr())==0)
         error(Eaddress);
      if(ct->defcmd){
         if(skipbl()=='\n'){
            getch();
            cmd.ccmd=newcmd();
            cmd.ccmd->cmdc=ct->defcmd;
         }else if((cmd.ccmd=parsecmd(nest))==0)
            panic("defcmd");
      }else if(ct->text)
         cmd.ctext=collecttext();
      else if(ct->token)
         cmd.ctext=collecttoken(ct->token);
      else
         atnl();
   }else
      switch(cmd.cmdc){
      case '{':
         cp=0;
         do{
            if(skipbl()=='\n')
               getch();
            ncp=parsecmd(nest+1);
            if(cp)
               cp->next=ncp;
            else
               cmd.ccmd=ncp;
         }while((cp=ncp));
         break;
      case '}':
         atnl();
         if(nest==0)
            error(Enolbrace);
         return 0;
      default:
         error_c(Eunk, cmd.cmdc);
      }
 Return:
   cp=newcmd();
   *cp=cmd;
   return cp;
}
Regexp * /* BUGGERED */
getregexp(int delim)
{
   Regexp *r=newre();
   int c;
   for(strzero(&genstr); ; straddc(&genstr, c))
      if((c=getch())=='\\'){
         if(nextch()==delim)
            c=getch();
         else if(nextch()=='\\'){
            straddc(&genstr, c);
            c=getch();
         }
      }else if(c==delim || c=='\n')
         break;
   if(c!=delim && c)
      ungetch();
   if(genstr.n>0){
      patset=TRUE;
      strdupstr(&lastpat, &genstr);
      straddc(&lastpat, '\0');
   }
   if(lastpat.n<=1)
      error(Epattern);
   strdupstr(&r->text, &lastpat);
   return r;
}
Addr *
compoundaddr(void)
{
   Addr addr;
   Addr *ap, *next;
   addr.aprev=simpleaddr();
   if((addr.type=skipbl())!=',' && addr.type!=';')
      return addr.aprev;
   getch();
   next=addr.next=compoundaddr();
   if(next && (next->type==',' || next->type==';') && next->aprev==0)
      error(Eaddress);
   ap=newaddr();
   *ap=addr;
   return ap;
}
Addr *
simpleaddr(void)
{
   Addr addr;
   Addr *ap, *nap;
   addr.next=0;
   addr.aprev=0;
   switch(skipbl()){
   case '#':
      addr.type=getch();
      addr.num=getnum();
      break;
   case '0': case '1': case '2': case '3': case '4':
   case '5': case '6': case '7': case '8': case '9': 
      addr.num=getnum();
      addr.type='l';
      break;
   case '/': case '?': case '"':
      addr.are=getregexp(addr.type=getch());
      break;
   case '.':
   case '$':
   case '+':
   case '-':
   case '\'':
      addr.type=getch();
      break;
   default:
      return 0;
   }
   if((addr.next=simpleaddr()))
      switch(addr.next->type){
      case '.':
      case '$':
      case '\'':
         if(addr.type!='"')
      case '"':
            error(Eaddress);
         break;
      case 'l':
      case '#':
         if(addr.type=='"')
            break;
         /* fall through */
      case '/':
      case '?':
         if(addr.type!='+' && addr.type!='-'){
            /* insert the missing '+' */
            nap=newaddr();
            nap->type='+';
            nap->next=addr.next;
            addr.next=nap;
         }
         break;
      case '+':
      case '-':
         break;
      default:
         panic("simpleaddr");
      }
   ap=newaddr();
   *ap=addr;
   return ap;
}
