#include <ansi.h>
#include <posix.h>
#include "edam.h"

Rangeset sel;
String lastregexp;
/*
 * Machine Information
 */
#define next left /* Left branch is usually just next ptr */
#define subid u.sid
#define right u.other
#define charid u.sid
#define order u.uid
#define NPROG 1024
Inst program[NPROG];
Inst *progp;
Inst *startinst; /* First inst. of program; might not be program[0] */
Inst *bstartinst; /* same for backwards machine */

#define NLIST 128
Ilist *tl, *nl; /* This list, next list */
Ilist list[2][NLIST];
static Rangeset sempty;

/*
 * Actions and Tokens
 *
 *	02xx are operators, value == precedence
 *	03xx are tokens, i.e. operands for operators
 */
#define CHAR 0100 /* Character */
#define OPERATOR 0200 /* Bitmask of all operators */
#define START 0200 /* Start, used for marker on stack */
#define RBRA 0201 /* Right bracket, ) */
#define LBRA 0202 /* Left bracket, ( */
#define OR 0203 /* Alternation, | */
#define CAT 0204 /* Concatentation, implicit operator */
#define STAR 0205 /* Closure, * */
#define PLUS 0206 /* a+ == aa* */
#define QUEST 0207 /* a? == a|nothing, i.e. 0 or 1 a's */
#define OPERAND 0300 /* Bitmask of all operands */
#define OPMASK 0300
#define ANY 0300 /* Any character but newline, . */
#define ANYNL 0301 /* Any character, including newline, @ */
#define NOP 0302 /* No operation, internal use only */
#define BOL 0303 /* Beginning of line, ^ */
#define EOL 0304 /* End of line, $ */
#define CCLASS 0305 /* Character class, [] */
#define END 0377 /* Terminate: match found */

/*
 * Parser Information
 */
#define NSTACK 20
Node andstack[NSTACK];
Node *andp;
int atorstack[NSTACK];
int *atorp;
int lastwasand; /* Last token was operand */
int cursubid;
int subidstack[NSTACK];
int *subidp;
int backwards;
int nbra;
int nub;
char *exprp; /* pointer to next character in source expression */
int nexprp;  /* length of exprp */
#define NCLASS 20
#define SCLASS 128
int nclass;
int class[NCLASS][SCLASS];


Inst * newinst(int t);
Inst * realcompile(char *s, int n);
void compile(Regexp *r);
void operand(int t);
void operator(int t);
void character(int t);
void cant(char *s);
void pushand(Inst *f, Inst  *l);
void pushator(int t);
Node * popand(int op);
int popator(void);
void evaluntil(int pri);
void optimize(Inst *start);
#ifdef DEBUG
void dumpstack(void);
void dump(void);
#endif
void startlex(char *s, int n);
int * newclass(void);
Lexeme lex(void);
int nextrec(void);
int incclass(int, int *);
void bldcclass(void);
void addinst(Ilist *l, Inst *inst, Rangeset *sep);
int execute(File *f, Posn startp, Posn  eof);
void newmatch(Rangeset *sp);
int bexecute(File *f, Posn startp);
void bnewmatch(Rangeset *sp);

int getunicode(void);
int fgetunicode(File *f, int *u);
int fbgetunicode(File *f, int *u);

Inst *
newinst(int t)
{
   if(progp>=&program[NPROG])
      error(Etoolong);
   progp->type=t;
   progp->left=0;
   progp->right=0;
   return progp++;
}
Inst *
realcompile(char *s, int n)
{
   Lexeme token;

   startlex(s,n);
   atorp=atorstack;
   andp=andstack;
   subidp=subidstack;
   cursubid=0;
   lastwasand=FALSE;
   /* Start with a low priority operator to prime parser */
   pushator(START-1);
   for(;;){
      token=lex();
      if(token.value == END)
         break;
      if(token.type == OPERATOR)
         operator(token.value);
      else if(token.type == OPERAND)
         operand(token.value);
      else if(token.type == CHAR)
         character(token.value);
      else
         cant("unknown token type");
   }
   /* Close with a low priority operator */
   evaluntil(START);
   /* Force END */
   operand(END);
   evaluntil(START);
   if(nbra)
      error(Eleftpar);
   if(nub)
      error(Ebadutf);
   --andp;   /* points to first and only operand */
   return andp->first;
}
void
compile(Regexp *r)
{
   String *s=&r->text;
   Inst *oprogp;
   if(s->n==lastregexp.n &&
    strncmp((char *)s->s, (char *)lastregexp.s, lastregexp.n)==0)
      return;
   progp=program;
   backwards=FALSE;
   startinst=realcompile(s->s,s->n);
   optimize(program);
   oprogp=progp;
   backwards=TRUE;
   bstartinst=realcompile(s->s,s->n);
   optimize(oprogp);
   strdupstr(&lastregexp, s);
}
void
character(int t)
{
   Inst *i;
   if(lastwasand)
      operator(CAT);   /* catenate is implicit */
   i=newinst(CHAR);
   i->charid=t;
   pushand(i, i);
   lastwasand=TRUE;
}
void
operand(int t)
{
   Inst *i;
   if(lastwasand)
      operator(CAT);   /* catenate is implicit */
   i=newinst(t);
   if(t==CCLASS)   /* ugh */
      i->right=(Inst *)class[nclass-1];   /* UGH! */
   pushand(i, i);
   lastwasand=TRUE;
}
void
operator(int t)
{
   if(t==RBRA && --nbra<0)
      error(Erightpar);
   if(t==LBRA){
/*
 *		if(++cursubid >= NSUBEXP)
 *			error(Esubexp);
 */
      cursubid++;   /* silently ignored */
      nbra++;
      if(lastwasand)
         operator(CAT);
   }else
      evaluntil(t);
   if(t!=RBRA)
      pushator(t);
   lastwasand=FALSE;
   if(t==STAR || t==QUEST || t==PLUS || t==RBRA)
      lastwasand=TRUE;   /* these look like operands */
}
void
cant(char *s)
{
   static char buf[100];
   sprintf(buf, "can't happen: %s", s);
   panic(buf);
}
void
pushand(Inst *f, Inst  *l)
{
   if(andp >= &andstack[NSTACK])
      cant("operand stack overflow");
   andp->first=f;
   andp->last=l;
   andp++;
}
void
pushator(int t)
{
   if(atorp >= &atorstack[NSTACK])
      cant("operator stack overflow");
   *atorp++=t;
   if(cursubid >= NSUBEXP)
      *subidp++= -1;
   else
      *subidp++=cursubid;
}
Node *
popand(int op)
{
   if(andp <= &andstack[0])
      error_c(Emissop, op);
   return --andp;
}
int
popator(void)
{
   if(atorp <= &atorstack[0])
      cant("operator stack underflow");
   --subidp;
   return *--atorp;
}
void
evaluntil(int pri)
{
   Node *op1, *op2, *t;
   Inst *inst1, *inst2;
   int o;
   while(pri==RBRA || atorp[-1]>=pri){
      switch(o=popator()){
      case LBRA:
         op1=popand('(');
         inst2=newinst(RBRA);
         inst2->subid = *subidp;
         op1->last->next = inst2;
         inst1=newinst(LBRA);
         inst1->subid = *subidp;
         inst1->next=op1->first;
         pushand(inst1, inst2);
         return;      /* must have been RBRA */
      default:
         panic("unknown regexp operator");
         break;
      case OR:
         op2=popand('|');
         op1=popand('|');
         inst2=newinst(NOP);
         op2->last->next=inst2;
         op1->last->next=inst2;
         inst1=newinst(OR);
         inst1->right=op1->first;
         inst1->left=op2->first;
         pushand(inst1, inst2);
         break;
      case CAT:
         op2=popand(0);
         op1=popand(0);
         if(backwards && op2->first->type!=END)
            t=op1, op1=op2, op2=t;
         op1->last->next=op2->first;
         pushand(op1->first, op2->last);
         break;
      case STAR:
         op2=popand('*');
         inst1=newinst(OR);
         op2->last->next=inst1;
         inst1->right=op2->first;
         pushand(inst1, inst1);
         break;
      case PLUS:
         op2=popand('+');
         inst1=newinst(OR);
         op2->last->next=inst1;
         inst1->right=op2->first;
         pushand(op2->first, inst1);
         break;
      case QUEST:
         op2=popand('?');
         inst1=newinst(OR);
         inst2=newinst(NOP);
         inst1->left=inst2;
         inst1->right=op2->first;
         op2->last->next=inst2;
         pushand(inst1, inst2);
         break;
      }
   }
}
void
optimize(Inst *start)
{
   Inst *inst, *target;

   for(inst=start; inst->type!=END; inst++){
      target=inst->next;
      while(target->type == NOP)
         target=target->next;
      inst->next=target;
   }
}
#ifdef DEBUG
void
dumpstack(void)
{
   Node *stk;
   int *ip;

   dprint("operators\n");
   for(ip=atorstack; ip<atorp; ip++)
      dprint("%.2x\n", *ip);
   dprint("operands\n");
   for(stk=andstack; stk<andp; stk++)
      dprint("%.2x\t%.2x\n", stk->first->type, stk->last->type);
}
void
dump(void)
{
   Inst *l;

   l=program;
   do{
      dprint("%d:\t%.2x\t%.8x\t%.8x\t%.3o\n", l-program, l->type,
         l->left-program, l->right-program, l->charid);
   }while(l++->type);
}
#endif
void
startlex(char *s,int n)
{
   exprp=s;
   nexprp=n;
   nclass=0;
   nbra=0;
   nub=0;
}
int *
newclass(void)
{
   int *p;
   int n;

   if(nclass >= NCLASS)
      error(Eclass);
   p=class[nclass++];
   for(n=0; n<SCLASS; n++)
      p[n]=0;
   return p;
}
int
getunicode(void)
{
   char *p;
   if(nexprp<u8len(*exprp))
      return -1;
   p=exprp;
   if(!(exprp=u8next(exprp))){
      exprp=p;
      error(Ebadutf);
   }
   nexprp-=(exprp-p); 
   return u8decode(p);   
}
int
fgetunicode(File *f, int *u)
{
   int n;
   char p[U8BYTES];
   *u=-1;
   if((n=Fgetu8(f,p))<0)
      error(Ebadutf);
   if(n)
      *u=u8decode(p);
   return n;   
}
int
fbgetunicode(File *f, int *u)
{
   int n;
   char p[U8BYTES];
   *u=-1;
   if((n=Fbgetu8(f,p))<0)
      error(Ebadutf);
   if(n)
      *u=u8decode(p);
   return n;   
}
Lexeme
lex(void)
{
   Lexeme L;
   int c;
   short t=0;

   switch(c=getunicode()){
   case '\\':
      if(*exprp)
         if((c=getunicode())=='n')
            c='\n';
      t=CHAR;
      break;
   case 0:
   case -1:
      c=END;
      --exprp;   /* In case we come here again */
      ++nexprp;
      break;
   case '*':
      c=STAR;
      break;
   case '?':
      c=QUEST;
      break;
   case '+':
      c=PLUS;
      break;
   case '|':
      c=OR;
      break;
   case '.':
      c=ANY;
      break;
   case '@':
      c=ANYNL;
      break;
   case '(':
      c=LBRA;
      break;
   case ')':
      c=RBRA;
      break;
   case '^':
      c=BOL;
      break;
   case '$':
      c=EOL;
      break;
   case '[':
      c=CCLASS;
      bldcclass();
      break;
   default:
      t=CHAR;
   }
   if(!t)
      t=c&OPMASK;
   L.type=t;
   L.value=c;
   return L;
}
int
nextrec(void)
{
   int i;
   if(exprp[0]==0 || (exprp[0]=='\\' && exprp[1]==0))
      error(Ebadclass);
   if(exprp[0]=='\\'){
      exprp++;
      --nexprp;
      if(*exprp=='n'){
         exprp++;
         --nexprp;
         return '\n';
      }
      if((i=getunicode())<0)
         return -1;
      return i|ESCAPED;   /* needed to allow literal ^,- and ] */
   }
   return getunicode();
}
void
bldcclass(void)
{
   int c1, c2;
   int *classp;
   int negate=FALSE;
   int n = 2;

   classp=newclass();
   /* we have already seen the '[' */
   if(*exprp=='^'){
      negate=TRUE;
      exprp++;
      --nexprp;
   }
   while(n<SCLASS){
      if((c1=c2=nextrec()) == ']')
         break;
      if(*exprp=='-'){
         exprp++;   /* eat '-' */
         --nexprp;
         if((c2=nextrec()) == ']')
            error(Ebadclass);
      }
      if(c1>c2)
         continue;
      classp[n++]=c1&~ESCAPED;
      classp[n++]=c2&~ESCAPED;
   }
   classp[0]=n;
   if(negate)
      classp[1]=TRUE;
   else
      classp[1]=FALSE;
}

int
incclass(int c, int *cc)
{
   int n,i=0;
   int v;

   n=cc[i++];
   v=cc[i++];
   while(i<n)
      if(c>=cc[i++] && c<=cc[i++])
         return v ? 0:1;
   return v ? 1:0;
}

/*
 * Note optimization in addinst:
 * 	*l must be pending when addinst called; if *l has been looked
 *		at already, the optimization is a bug.
 */
void
addinst(Ilist *l, Inst *inst, Rangeset *sep)
{
   Ilist *p;

   for(p=l; p->inst; p++){
      if(p->inst==inst){
         if((sep)->p[0].p1 < p->se.p[0].p1)
            p->se= *sep;   /* this would be bug */
         return;   /* It's already there */
      }
   }
   p->inst=inst;
   p->se= *sep;
   (++p)->inst=0;
}
int
execute(File *f, Posn startp, Posn  eof)
{
   int flag=0;
   Inst *inst;
   Ilist *tlp;
   Posn p=startp;
   int nnl=0, ntl;
   int c,n=0;
   int wrapped=0;
   int startchar=startinst->type==CHAR? startinst->charid : 0;

   list[0][0].inst=list[1][0].inst=0;
   sel.p[0].p1 = -1;
   Fgetcset(f, startp);
   /* Execute machine once for each character */
   for(;;p+=n){
   doloop:
      n=fgetunicode(f,&c);
      if(p>=eof || c==-1){
         switch(wrapped++){
         case 0:      /* let loop run one more click */
         case 2:
            break;
         case 1:      /* expired; wrap to beginning */
            if(sel.p[0].p1>=0 || eof!=INFTY)
               goto Return;
            list[0][0].inst=list[1][0].inst=0;
            Fgetcset(f, (Posn)0);
            p=0;
            goto doloop;
         default:
            goto Return;
         }
      }else if(((wrapped && p>=startp) || sel.p[0].p1>0) && nnl==0)
         break;
      /* fast check for first char */
      if(startchar && nnl==0 && c!=startchar)
         continue;
      tl=list[flag];
      nl=list[flag^=1];
      nl->inst=0;
      ntl=nnl;
      nnl=0;
      if(sel.p[0].p1<0 && (!wrapped || p<startp || startp==eof)){
         /* Add first instruction to this list */
         if(++ntl >= NLIST)
   Overflow:
            error(Eoverflow);
         sempty.p[0].p1 = p;
         addinst(tl, startinst, &sempty);
      }
      /* Execute machine until this list is empty */
      for(tlp=tl; (inst=tlp->inst); tlp++){   /* assignment = */
   Switchstmt:
         switch(inst->type){
         case CHAR:   /* regular character */
            if(inst->charid==c){
   Addinst:
               if(++nnl >= NLIST)
                  goto Overflow;
               addinst(nl, inst->next, &tlp->se);
            }
            break;
         case LBRA:
            if(inst->subid>=0)
               tlp->se.p[inst->subid].p1 = p;
            inst=inst->next;
            goto Switchstmt;
         case RBRA:
            if(inst->subid>=0)
               tlp->se.p[inst->subid].p2 = p;
            inst=inst->next;
            goto Switchstmt;
         case ANYNL:
            goto Addinst;
         case ANY:
            if(c!='\n')
               goto Addinst;
            break;
         case BOL:
            if(p==0){
   Step:
               inst=inst->next;
               goto Switchstmt;
            }
            if(f->getci>1){
               if(f->getcbuf[f->getci-2]=='\n')
                  goto Step;
            }else{
               char c;
               if(Fchars(f, &c, p-1, p)==1 && c=='\n')
                  goto Step;
            }
            break;
         case EOL:
            if(c=='\n')
               goto Step;
            break;
         case CCLASS:
            if(incclass(c,(int *)inst->right))
               goto Addinst;
            break;
         case OR:
            /* evaluate right choice later */
            if(++ntl >= NLIST)
               goto Overflow;
            addinst(tlp, inst->right, &tlp->se);
            /* efficiency: advance and re-evaluate */
            inst=inst->left;
            goto Switchstmt;
         case END:   /* Match! */
            tlp->se.p[0].p2 = p;
            newmatch(&tlp->se);
            break;
         }
      }
   }
 Return:
   return sel.p[0].p1>=0;
}
void
newmatch(Rangeset *sp)
{
   int i;

   if(sel.p[0].p1<0 || sp->p[0].p1<sel.p[0].p1 ||
    (sp->p[0].p1==sel.p[0].p1 && sp->p[0].p2>sel.p[0].p2))
      for(i=0; i<NSUBEXP; i++)
         sel.p[i] = sp->p[i];
}
int
bexecute(File *f, Posn startp)
{
   int flag=0;
   Inst *inst;
   Ilist *tlp;
   Posn p=startp;
   int nnl=0, ntl;
   int c,n=0;
   int wrapped=0;
   int startchar=bstartinst->type==CHAR? bstartinst->charid : 0;

   list[0][0].inst=list[1][0].inst=0;
   sel.p[0].p1= -1;
   Fgetcset(f, startp);
   /* Execute machine once for each character, including terminal NUL */
   for(;;p-=n){
   doloop:
      n=fbgetunicode(f,&c);
      if(c==-1){
         switch(wrapped++){
         case 0:      /* let loop run one more click */
         case 2:
            break;
         case 1:      /* expired; wrap to end */
            if(sel.p[0].p1>=0)
         case 3:
               goto Return;
            list[0][0].inst=list[1][0].inst=0;
            Fgetcset(f, f->nbytes);
            p=f->nbytes;
            goto doloop;
         default:
            goto Return;
         }
      }else if(((wrapped && p<=startp) || sel.p[0].p1>0) && nnl==0)
         break;
      /* fast check for first char */
      if(startchar && nnl==0 && c!=startchar)
         continue;
      tl=list[flag];
      nl=list[flag^=1];
      nl->inst=0;
      ntl=nnl;
      nnl=0;
      if(sel.p[0].p1<0 && (!wrapped || p>startp)){
         /* Add first instruction to this list */
         if(++ntl >= NLIST)
   Overflow:
            error(Eoverflow);
         /* the minus is so the optimizations in addinst work */
         sempty.p[0].p1 = -p;
         addinst(tl, bstartinst, &sempty);
      }
      /* Execute machine until this list is empty */
      for(tlp=tl; (inst=tlp->inst); tlp++){   /* assignment = */
   Switchstmt:
         switch(inst->type){
         case CHAR:   /* regular character */
            if(inst->charid==c){
   Addinst:
               if(++nnl >= NLIST)
                  goto Overflow;
               addinst(nl, inst->next, &tlp->se);
            }
            break;
         case LBRA:
            if (inst->subid>=0){
               tlp->se.p[inst->subid].p1 = p;
            }
            inst=inst->next;
            goto Switchstmt;
         case RBRA:
            if (inst->subid>=0){
               tlp->se.p[inst->subid].p2 = p;
            }
            inst=inst->next;
            goto Switchstmt;
         case ANYNL:
            goto Addinst;
         case ANY:
            if(c!='\n')
               goto Addinst;
            break;
         case BOL:
            if(c=='\n' || p==0){
   Step:
               inst=inst->next;
               goto Switchstmt;
            }
            break;
         case EOL:
            if(f->getci<f->ngetc-1){
               if(f->getcbuf[f->getci+1]=='\n')
                  goto Step;
            }else if(p<f->nbytes-1){
               char c;
               if(Fchars(f, &c, p+1, p+2)==1 && c=='\n')
                  goto Step;
            }
            break;
         case CCLASS:
            if(incclass(c,(int *)inst->right))
               goto Addinst;
            break;
         case OR:
            /* evaluate right choice later */
            if(++ntl >= NLIST)
               goto Overflow;
            addinst(tlp, inst->right, &tlp->se);
            /* efficiency: advance and re-evaluate */
            inst=inst->left;
            goto Switchstmt;
         case END:   /* Match! */
            tlp->se.p[0].p1 = -tlp->se.p[0].p1; /* minus sign */
            tlp->se.p[0].p2 = p;
            bnewmatch(&tlp->se);
            break;
         }
      }
   }
 Return:
   return sel.p[0].p1>=0;
}
void
bnewmatch(Rangeset *sp)
{
 int i;
 if(sel.p[0].p1<0 || sp->p[0].p1>sel.p[0].p2 || (sp->p[0].p1==sel.p[0].p2 && sp->p[0].p2<sel.p[0].p1))
 for(i=0; i<NSUBEXP; i++){ /* note the reversal; p1<=p2 */
 sel.p[i].p1=sp->p[i].p2;
 sel.p[i].p2=sp->p[i].p1;
 }
}
