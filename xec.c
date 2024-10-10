#include <ansi.h>
#include <posix.h>
#include "edam.h"

int Glooping;
int nest;

void resetxec(void);
int cmdexec(File *f, Cmd *cp);
int a_cmd(File *f, Cmd *cp);
int b_cmd(File *f, Cmd *cp);
int c_cmd(File *f, Cmd *cp);
int d_cmd(File *f, Cmd *cp);
int D_cmd(File *f, Cmd *cp);
int e_cmd(File *f, Cmd *cp);
int f_cmd(File *f, Cmd *cp);
int g_cmd(File *f, Cmd *cp);
int i_cmd(File *f, Cmd *cp);
int k_cmd(File *f, Cmd *cp);
int m_cmd(File *f, Cmd *cp);
int n_cmd(File *f, Cmd *cp);
int p_cmd(File *f, Cmd *cp);
int q_cmd(File *f, Cmd *cp);
int s_cmd(File *f, Cmd *cp);
int u_cmd(File *f, Cmd *cp);
int w_cmd(File *f, Cmd *cp);
int x_cmd(File *f, Cmd *cp);
int X_cmd(File *f, Cmd *cp);
int unix_cmd(File *f, Cmd *cp);
int eq_cmd(File *f, Cmd *cp);
int nl_cmd(File *f, Cmd *cp);
int cd_cmd(File *f, Cmd *cp);
int append(File *f, Cmd *cp, Posn p);
int display(File *f);
void looper(File *f, Cmd *cp, int xy);
void linelooper(File *f, Cmd *cp);
void filelooper(Cmd *cp, int XY);

void
resetxec(void)
{
   Glooping=nest=0;
}
int
cmdexec(File *f, Cmd *cp)
{
   int i;
   Addr *ap;
   Address a;
   if(f && f->state==Unread)
      load(f);
   if(f==0 && (cp->addr==0 || cp->addr->type!='"') &&
    !strchr("bBnqUXY!{", cp->cmdc) &&
    cp->cmdc!=('c'|0x100) && !(cp->cmdc=='D' && cp->ctext))
      error(Enofile);
   i=lookup(cp->cmdc);
   if(cmdtab[i].defaddr!=aNo){
      if((ap=cp->addr)==0 && cp->cmdc!='\n'){
         cp->addr=ap=newaddr();
         ap->type='.';
         if(cmdtab[i].defaddr==aAll)
            ap->type='*';
      }else if(ap && ap->type=='"' && ap->next==0 && cp->cmdc!='\n'){
         ap->next=newaddr();
         ap->next->type='.';
         if(cmdtab[i].defaddr==aAll)
            ap->next->type='*';
      }
      if(cp->addr){   /* may be false for '\n' (only) */
         addr=address(ap, f->dot, 0);
         f=addr.f;
      }
   }
   current(f);
   switch(cp->cmdc){
   case '{':
      a=cp->addr? address(cp->addr, f->dot, 0): f->dot;
      for(cp=cp->ccmd; cp; cp=cp->next){
         a.f->dot=a;
         cmdexec(a.f, cp);
      }
      break;
   default:
      i=(*cmdtab[i].fn)(f, cp);
      return i;
   }
   return 1;
}
int
a_cmd(File *f, Cmd *cp)
{
   return append(f, cp, addr.r.p2);
}
int
b_cmd(File *f, Cmd *cp)
{
   (void)f;
   f=cp->cmdc=='b'? tofile(cp->ctext) : getfile(cp->ctext);
   if(f->state==Unread)
      load(f);
   else if(nest==0)
      filename(f);
   return TRUE;
}
int
c_cmd(File *f, Cmd *cp)
{
   Fdelete(f, addr.r.p1, addr.r.p2);
   f->dot.r.p1=f->dot.r.p2=addr.r.p2;
   return append(f, cp, addr.r.p2);
}
int
d_cmd(File *f, Cmd *cp)
{
   (void)cp;
   Fdelete(f, addr.r.p1, addr.r.p2);
   f->dot.r.p1=f->dot.r.p2=addr.r.p1;
   return TRUE;
}
int
D_cmd(File *f, Cmd *cp)
{
   closefiles(f, cp->ctext);
   return TRUE;
}
int
e_cmd(File *f, Cmd *cp)
{
   if(getname(f, cp->ctext, cp->cmdc=='e')==0)
      error(Enoname);
   edit(f, cp->cmdc);
   return TRUE;
}
int
f_cmd(File *f, Cmd *cp)
{
   getname(f, cp->ctext, TRUE);
   filename(f);
   return TRUE;
}
int
g_cmd(File *f, Cmd *cp)
{
   if(f!=addr.f)panic("g_cmd f!=addr.f");
   compile(cp->re);
   if(execute(f, addr.r.p1, addr.r.p2) ^ (cp->cmdc=='v')){
      f->dot=addr;
      return cmdexec(f, cp->ccmd);
   }
   return TRUE;
}
int
i_cmd(File *f, Cmd *cp)
{
   return append(f, cp, addr.r.p1);
}
int
k_cmd(File *f, Cmd *cp)
{
   (void)cp;
   f->mark=addr.r;
   return TRUE;
}
int
m_cmd(File *f, Cmd *cp)
{
   Address addr2;
   addr2=address(cp->caddr, f->dot, 0);
   if(cp->cmdc=='m')
      move(f, addr2);
   else
      copy(f, addr2);
   current(addr2.f);
   return TRUE;
}
int
n_cmd(File *f, Cmd *cp)
{
   int i;
   (void)f;
   (void)cp;
   for(i=0; i<file.nused; i++){
      strdupstr(&genstr, &file.ptr[i]->name);
      filename(file.ptr[i]);
   }
   return TRUE;
}
int
p_cmd(File *f, Cmd *cp)
{
   (void)cp;
   return display(f);
}
int
q_cmd(File *f, Cmd *cp)
{
   (void)cp;
   (void)f;
   trytoquit();
   return FALSE;
}
int
s_cmd(File *f, Cmd *cp)
{
   int i, j, c, n;
   Posn p1, op, didsub=0, delta=0;
   n=cp->num;
   op=-1;
   compile(cp->re);
   for(p1=addr.r.p1; p1<=addr.r.p2 && execute(f, p1, addr.r.p2); ){
      if(sel.p[0].p1==sel.p[0].p2){   /* empty match? */
         if(sel.p[0].p1==op){
            p1++;
            continue;
         }
         p1=sel.p[0].p2+1;
      }else
         p1=sel.p[0].p2;
      op=sel.p[0].p2;
      if(--n>0)
         continue;
      strzero(&genstr);
      for(i=0; i<cp->ctext->n; i++)
         if((c=cp->ctext->s[i])=='\\' && i<cp->ctext->n-1){
            c=cp->ctext->s[++i];
            if('1'<=c && c<='9') {
               j=c-'0';
               if(sel.p[j].p2-sel.p[j].p1>BLOCKSIZE)
                  error(Elongsub);
               Fchars(f, genbuf, sel.p[j].p1, sel.p[j].p2);
               strinsert(&genstr, tempstr(genbuf, (ulong)(sel.p[j].p2-sel.p[j].p1)), (ulong)genstr.n);
            }else
                straddc(&genstr, c);
         }else if(c!='&')
            straddc(&genstr, c);
         else{
            if(sel.p[0].p2-sel.p[0].p1>BLOCKSIZE)
               error(Elongrhs);
            Fchars(f, genbuf, sel.p[0].p1, sel.p[0].p2);
            strinsert(&genstr,
               tempstr(genbuf, (int)(sel.p[0].p2-sel.p[0].p1)),
               (ulong)genstr.n);
         }
      if(sel.p[0].p1!=sel.p[0].p2){
         Fdelete(f, sel.p[0].p1, sel.p[0].p2);
         delta-=sel.p[0].p2-sel.p[0].p1;
      }
      if(genstr.n){
         Finsert(f, &genstr, sel.p[0].p2);
         delta+=genstr.n;
      }
      didsub=1;
      if(!cp->flag)
         break;
   }
   if(!didsub && nest==0)
      error(Enosub);
   f->dot.r.p1=addr.r.p1, f->dot.r.p2=addr.r.p2+delta;
   return TRUE;
}
int
u_cmd(File *f, Cmd *cp)
{
   int n;
   (void)f;
   (void)cp;
   n=cp->num;
   while(n--)
      undo();
   return TRUE;
}
int
w_cmd(File *f, Cmd *cp)
{
   if(getname(f, cp->ctext, FALSE)==0)
      error(Enoname);
   writef(f);
   return TRUE;
}
int
x_cmd(File *f, Cmd *cp)
{
   if(cp->re)
      looper(f, cp, cp->cmdc=='x');
   else
      linelooper(f, cp);
   return TRUE;
}
int
X_cmd(File *f, Cmd *cp)
{
   (void)f;
   filelooper(cp, cp->cmdc=='X');
   return TRUE;
}
int
unix_cmd(File *f, Cmd *cp)
{
   Unix(f, cp->cmdc, cp->ctext, nest);
   return TRUE;
}
int
eq_cmd(File *f, Cmd *cp)
{
   int charsonly=0;
   switch(cp->ctext->n){
   case 1:
      charsonly=FALSE;
      break;
   case 2:
      if(cp->ctext->s[0]=='#'){
         charsonly=TRUE;
         break;
      }
   default:
      error(Enewline);
   }
   printposn(f, charsonly);
   return TRUE;
}
int
nl_cmd(File *f, Cmd *cp)
{
   if(cp->addr==0){
      /* First put it on newline boundaries */
      addr=lineaddr((Posn)0, f->dot, -1);
      addr.r.p2=lineaddr((Posn)0, f->dot, 1).r.p2;
      if(addr.r.p1==f->dot.r.p1 && addr.r.p2==f->dot.r.p2)
         addr=lineaddr((Posn)1, f->dot, 1);
      display(f);
   }else
      display(f);
   return TRUE;
}
int
cd_cmd(File *f, Cmd *cp)
{
   (void)f;
   cd(cp->ctext);
   return TRUE;
}
int
append(File *f, Cmd *cp, Posn p)
{
   if(cp->ctext->n>0 && cp->ctext->s[cp->ctext->n-1]==0)
      --cp->ctext->n;
   if(cp->ctext->n>0)
      Finsert(f, cp->ctext, p);
   f->dot.r.p1=p;
   f->dot.r.p2=p+cp->ctext->n;
   return TRUE;
}
int
display(File *f)
{
   Posn p1=addr.r.p1, p2=addr.r.p2;
   int np, n;
   if(f!=addr.f)panic("display f!=addr.f");
   while(p1<p2){
      np=p2-p1;
      if(np>BLOCKSIZE)
         np=BLOCKSIZE;
      n=Fchars(f, genbuf, p1, p1+np);
      if(n<=0)
         panic("display");
      Write(1, genbuf, n);
      p1+=n;
   }
   f->dot=addr;
   return TRUE;
}
void
looper(File *f, Cmd *cp, int xy)
{
   Posn p, op;
   Range r;
   r=addr.r;
   op= xy? -1 : r.p1;
   nest++;
   compile(cp->re);
   for(p=r.p1; p<=r.p2; ){
      if(!execute(f, p, r.p2)){ /* no match, but y should still run */
         if(xy || op>r.p2)
            break;
         f->dot.r.p1=op, f->dot.r.p2=r.p2;
         p=r.p2+1;   /* exit next loop */
      }else{
         if(sel.p[0].p1==sel.p[0].p2){   /* empty match? */
            if(sel.p[0].p1==op){
               p++;
               continue;
            }
            p=sel.p[0].p2+1;
         }else
            p=sel.p[0].p2;
         if(xy)
            f->dot.r=sel.p[0];
         else
            f->dot.r.p1=op, f->dot.r.p2=sel.p[0].p1;
      }
      op=sel.p[0].p2;
      cmdexec(f, cp->ccmd);
      compile(cp->re);
   }
   --nest;
}
void
linelooper(File *f, Cmd *cp)
{
   Posn p;
   Range r, linesel;
   Address a3;
   nest++;
   r=addr.r;
   a3.f=f;
   a3.r.p1=a3.r.p2=r.p1;
   for(p=r.p1; p<r.p2; p=a3.r.p2){
      a3.r.p1=a3.r.p2;
/*pjw      if(p!=r.p1 || (linesel=lineaddr((Posn)0, a3, 1)).r.p2==p)*/
      if(p!=r.p1 || ((linesel=lineaddr((Posn)0, a3, 1).r), linesel.p2==p))
         linesel=lineaddr((Posn)1, a3, 1).r;
      if(linesel.p1>=r.p2)
         break;
      if(linesel.p2>=r.p2)
         linesel.p2=r.p2;
      if(linesel.p2>linesel.p1)
         if(linesel.p1>=a3.r.p2 && linesel.p2>a3.r.p2){
            f->dot.r=linesel;
            cmdexec(f, cp->ccmd);
            a3.r=linesel;
            continue;
         }
      break;
   }
   --nest;
}
void
filelooper(Cmd *cp, int XY)
{
   File *f, *cur;
   int i;
   if(Glooping++)
      error(EnestXY);
   nest++;
   settempfile();
   cur=curfile;
   for(i=0; i<tempfile.nused; i++){
      f=tempfile.ptr[i];
      if(cp->re==0 || filematch(f, cp->re)==XY)
         cmdexec(f, cp->ccmd);
   }
   if(cur && whichmenu(cur)>=0)   /* check that cur is still a file */
      current(cur);
   --Glooping;
   --nest;
}
