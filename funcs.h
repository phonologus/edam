#ifndef FUNCSDOTH
#define FUNCSDOTH

/* address.c */

Address address(Addr *ap, Address a, int sign);
void nextmatch(File *f, Regexp *r, Posn p, int sign);
File * matchfile(Regexp *r);
int filematch(File *f, Regexp *r);
Address charaddr(Posn l, Address addr, int sign);
Address lineaddr(Posn l, Address addr, int sign);

/* buffer.c */

Buffer * Bopen(Discdesc *dd);
void Bclose(Buffer *b);
int Bread(Buffer *b, char *addr, int n, Posn p0);
void Binsert(Buffer *b, String *s, Posn p0);
void Bdelete(Buffer *b, Posn p1, Posn  p2);
int incache(Buffer *b, Posn p1, Posn  p2);

/* cmd.c */

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

/* disc.c */

void Dremove(void);
Discdesc * Dstart(void);
void Dclosefd(void);
Disc * Dopen(Discdesc *dd);
void Dclose(Disc *d);
int Dread(Disc *d, char *addr, int n, Posn p1);
void Dinsert(Disc *d, char *addr, int n, Posn p0);
void Ddelete(Disc *d, Posn p1, Posn  p2);
void Dreplace(Disc *d, Posn p1, Posn  p2, char *addr, int n);

/* error.c */

void error(Error s);
void error_s(Error s, char *a);
void error_c(Error s, int c);
void warn(Warning s);
void warn_s(Warning s, char *a);
void dprint(char *z, ...);
void termwrite(char *s, int n);

/* file.c */

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

/* io.c */

void writef(File *f);
Posn readio(File *f, int setdate);
Posn writeio(File *f);
void closeio(Posn p);

/* list.c */

void growlist(List *l);
void dellist(List *l, int i);
void inslist(List *l, int i, long val);
void listfree(List *l);

/* malloc.c */

void * alloc(size_t n);
void * allocre(void *p, size_t n);

/* misc.c */

Posn getnum(void);
int skipbl(void);

/* multi.c */

File * newfile(void);
void delfile(File *f);
void sortname(File *f);
int whichmenu(File *f);
void state(File *f, enum State cleandirty);
File * lookfile(void);

/* regexp.c */

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
void dumpstack(void);
void dump(void);
void startlex(char *s, int n);
int * newclass(void);
int getunicode(void);
int fgetunicode(File *, int *);
int fbgetunicode(File *, int *);
Lexeme lex(void);
int nextrec(void);
void bldcclass(void);
int incclass(int, int *);
void addinst(Ilist *l, Inst *inst, Rangeset *sep);
int execute(File *f, Posn startp, Posn  eof);
void newmatch(Rangeset *sp);
int bexecute(File *f, Posn startp);
void bnewmatch(Rangeset *sp);

/* sam.c */

void panic(char *s);
void rescue(int s);
void hiccough(char *s);
void intr(int);
void trytoclose(File *f);
void trytoquit(void);
void load(File *f);
void cmdupdate(void);
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

/* string.c */

void strinit(String *p);
void strclose(String *p);
void strzero(String *p);
void strdupl(String *p, char *s);
void strdupstr(String *p, String  *q);
void straddc(String *p, int c);
void strinsure(String *p, ulong n);
void strinsert(String *p, String  *q, Posn p0);
void strdelete(String *p, Posn p1, Posn  p2);
String * tempstr(char *s, int n);

/* sys.c */

void syserror(char *a);
int Read(int f, char *a, size_t n);
int Write(int f, char *a, size_t n);
void Lseek(int f, size_t n, int w);

/* unix.c */

void Unix(File *f, int type, String *s, int nest);
void checkerrs(void);

/* xec.c */

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

#endif
