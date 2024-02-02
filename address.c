#include <ansi.h>
#include <posix.h>
#include "edam.h"

Address addr;
String lastpat;
int patset;
File *menu;


Address address(Addr *ap, Address a, int sign);
void nextmatch(File *f, Regexp *r, Posn p, int sign);
File * matchfile(Regexp *r);
int filematch(File *f, Regexp *r);
Address charaddr(Posn l, Address addr, int sign);
Address lineaddr(Posn l, Address addr, int sign);

Address
address(Addr *ap, Address a, int sign)
{
   File *f=a.f;
   Address a1, a2;
   do{
      switch(ap->type){
      case 'l':
      case '#':
         a=(*(ap->type=='#'?charaddr:lineaddr))((Posn)ap->num, a, sign);
         break;
      case '.':
         a=f->dot;
         break;
      case '$':
         a.r.p1=a.r.p2=f->nbytes;
         break;
      case '\'':
         a.r=f->mark;
         break;
      case '?':
         sign= -sign;
         if(sign==0)
            sign=-1;
         /* fall through */
      case '/':
         nextmatch(f, ap->are, sign>=0? a.r.p2 : a.r.p1, sign);
         a.r=sel.p[0];
         break;
      case '"':
         a=matchfile(ap->are)->dot;
         f=a.f;
         if(f->state==Unread)
            load(f);
         break;
      case '*':
         a.r.p1=0, a.r.p2=f->nbytes;
         return a;
      case ',':
      case ';':
         if(ap->aprev)
            a1=address(ap->aprev, a, 0);
         else
            a1.f=a.f, a1.r.p1=a1.r.p2=0;
         if(ap->type==';'){
            f=a1.f;
            f->dot=a=a1;
         }
         if(ap->next)
            a2=address(ap->next, a, 0);
         else
            a2.f=a.f, a2.r.p1=a2.r.p2=f->nbytes;
         if(a1.f!=a2.f)
            error(Eorder);
         a.f=a1.f, a.r.p1=a1.r.p1, a.r.p2=a2.r.p2;
         if(a.r.p2<a.r.p1)
            error(Eorder);
         return a;
      case '+':
      case '-':
         sign=1;
         if(ap->type=='-')
            sign=-1;
         if(ap->next==0 || ap->next->type=='+' || ap->next->type=='-')
            a=lineaddr(1L, a, sign);
         break;
      default:
         panic("address");
         return a;
      }
   }while((ap=ap->next));
   return a;
}
void
nextmatch(File *f, Regexp *r, Posn p, int sign)
{
   compile(r);
   if(sign>=0){
      if(!execute(f, p, INFTY))
         error(Esearch);
      if(sel.p[0].p1==sel.p[0].p2 && sel.p[0].p1==p){
         if(++p>f->nbytes)
            p=0;
         if(!execute(f, p, INFTY))
            panic("address");
      }
   }else{
      if(!bexecute(f, p))
         error(Esearch);
      if(sel.p[0].p1==sel.p[0].p2 && sel.p[0].p2==p){
         if(--p<0)
            p=f->nbytes;
         if(!bexecute(f, p))
            panic("address");
      }
   }
}
File *
matchfile(Regexp *r)
{
   File *f;
   File *match=0;
   int i;
   for(i=0; i<file.nused; i++){
      f=file.ptr[i];
      if(filematch(f, r)){
         if(match)
            error(Emanyfiles);
         match=f;
      }
   }
   if(!match)
      error(Efsearch);
   return match;
}
int
filematch(File *f, Regexp *r)
{
   sprintf((char *)genbuf, "%c%c%c %s\n", " '"[f->state==Dirty],
      "-+"[0], " ."[f==curfile], f->name.s);
   strdupl(&genstr, genbuf);
   /* A little dirty... */
   if(menu==0)
      (menu=Fopen())->state=Clean;
   Bdelete(menu->buf, (Posn)0, menu->buf->nbytes);
   Binsert(menu->buf, &genstr, (Posn)0);
   menu->nbytes=menu->buf->nbytes;
   compile(r);
   return execute(menu, (Posn)0, menu->nbytes);
}
Address
charaddr(Posn p, Address addr, int sign)
{
   if(sign==0)
      addr.r.p1=addr.r.p2=charposn(addr.f,(Posn)0,p);
   else if(sign<0)
      addr.r.p2=addr.r.p1=charposn(addr.f,addr.r.p1,-p);
   else if(sign>0)
      addr.r.p1=addr.r.p2=charposn(addr.f,addr.r.p2,p);
   
   if(addr.r.p1<0 || addr.r.p2>addr.f->nbytes)
      error(Erange);
   return addr;
}
Address
lineaddr(Posn p, Address addr, int sign)
{
   int n;
   int c=0;
   File *f=addr.f;
   Address a;
   a.f=f;
   if(sign>=0){
      if(p==0){
         if(sign==0 || addr.r.p2==0){
            a.r.p1=a.r.p2=0;
            return a;
         }
         a.r.p1=addr.r.p2;
         Fgetcset(f, addr.r.p2-1);
      }else{
         if(sign==0 || addr.r.p2==0){
            Fgetcset(f, (Posn)0);
            n=1;
         }else{
            Fgetcset(f, addr.r.p2-1);
            n=Fgetc(f)=='\n';
         }
         for(; n<p; ){
            c=Fgetc(f);
            if(c==-1)
               error(Erange);
            else if(c=='\n')
               n++;
         }
         a.r.p1=f->getcp;
      }
      do; while((c=Fgetc(f))!='\n' && c!=-1);
      a.r.p2=f->getcp;
   }else{
      Fbgetcset(f, addr.r.p1);
      if(p==0)
         a.r.p2=addr.r.p1;
      else{
         for(n=0; n<p; ){   /* always runs once */
            c=Fbgetc(f);
            if(c=='\n')
               n++;
            else if(c==-1){
               if(++n!=p)
                  error(Erange);
            }
         }
         a.r.p2=f->getcp;
         if(c=='\n')
            a.r.p2++;   /* lines start after a newline */
      }
      do; while((c=Fbgetc(f))!='\n' && c!=-1);
      a.r.p1=f->getcp;
      if(c=='\n')
         a.r.p1++;   /* lines start after a newline */
   }
   return a;
}
