#include "errors.h"

#define BLOCKSIZE 4096
#define NDISC 5
#define NBUFFILES 2+2*NDISC /* unix+undo+NDISC*(transcript+buf) */
#define NSUBEXP 10

#define TRUE 1
#define FALSE 0

#define INFTY ((Posn)0x7FFFFFFFFFFFFFFFL)
#define INCR 25

typedef unsigned short ushort;
typedef unsigned long ulong;
typedef long Posn; /* file position or address */
typedef ushort Mod; /* modification number */

enum State{
   Clean=' ',
   Dirty='\'',
   Unread='-'
};
typedef struct File File;
typedef struct Range{
   Posn   p1, p2;
}Range;
typedef struct Rangeset{
   Range   p[NSUBEXP];
}Rangeset;
typedef struct Address{
   Range   r;
   File   *f;
}Address;
typedef struct List{
   int   nalloc;
   int   nused;
   long   *ptr;      /* code depends on ptrs being same size as Posns */

}List;

/* code also depends on sizeof Block == sizeof long */
typedef struct Block{
   int   bnum;      /* absolute number on disk */
   int   nbytes;      /* bytes stored in this block */
}Block;
typedef struct Discdesc{
   int   fd;      /* unix file descriptor of temp file */
   int   nbk;      /* high water mark */
   List   free;      /* array of free blocks */
}Discdesc;
typedef struct Disc{
   Discdesc *desc;      /* descriptor of temp file */
   Posn   nbytes;      /* bytes on disc file */
   struct{
      int   nalloc;
      int   nused;
      Block   *ptr;
   }block;         /* array of used blocks (same shape as List) */
}Disc;
typedef struct String{
   ulong   n;
   ulong   size;
   char   *s;
}String;
typedef struct Buffer{
   Disc   *disc;      /* disc storage */
   Posn   nbytes;      /* total length of buffer */
   String   cache;      /* in-core storage for efficiency */
   Posn   c1, c2;      /* cache start and end positions in disc */
            /* note: if dirty, cache is really c0, c0+cache.n */
   int   dirty;      /* cache dirty */
}Buffer;
#define NGETC 128
struct File{
   Buffer   *buf;      /* cached disc storage */
   Buffer   *transcript;   /* what's been done */
   Posn   markp;      /* file pointer to start of latest change */
   Mod   mod;      /* modification stamp */
   Posn   nbytes;      /* total length of file */
   Posn   hiposn;      /* highest address touched this Mod */
   Address   dot;      /* current position */
   Range   mark;      /* tagged spot in text (don't confuse with Mark) */
   String   name;      /* file name */
   char   state;      /* Clean, Dirty, Unread */
   char   closeok;   /* ok to close file? */
	char	marked;		/* file has been Fmarked at least once; once
				 * set, this will never go off as undo doesn't
             * revert to the dawn of time */
   long   inumber;   /* file from which it was read */
   long   date;      /* time stamp of unix file */
   Posn   cp1, cp2;   /* Write-behind cache positions and */
   String   cache;      /* string */
   char   getcbuf[NGETC];
   int   ngetc;
   int   getci;
   Posn   getcp;
};
typedef struct Filelist{
   int   nalloc;
   int   nused;
   File   **ptr;
}Filelist;
typedef struct Mark{
   Posn   p;
   Range   dot;
   Range   mark;
   Mod   m;
   short   s1;
}Mark;
typedef struct Regexp Regexp;
struct Regexp{
   String   text;
};
typedef struct Lexeme {
   short type;
   int value;
} Lexeme;
typedef struct Inst{
   int   type;   /* < 0x200 ==> literal, otherwise action */
   union {
      int sid;
      struct Inst *other;
   }u;
   struct Inst *left;
}Inst;
typedef struct Ilist{
   Inst   *inst;      /* Instruction of the thread */
   Rangeset se;
   Posn   startp;      /* first char of match */
}Ilist;
typedef struct Node{
   Inst   *first;
   Inst   *last;
}Node;

#define PUTT(d,s,T) { char *P=(char *)&(s); int _n=sizeof(T);\
 while(_n-->0) *d++= *P++; }
#define GETT(p,s,T) { long L; char *P1=(s),*P2=(char *)&L;\
 int _n=sizeof(T); while(_n-->0) *P2++= *P1++; p = (T)L; }

#include "parse.h"
#include "funcs.h"
#include "utf.h"

extern char genbuf[];
extern char *home;
extern int io;
extern int patset;
extern int quitok;
extern Address addr;
extern Buffer *undobuf;
extern Buffer *unixbuf;
extern Filelist file;
extern Filelist tempfile;
extern File *curfile;
extern Mod modnum;
extern Posn cmdpt;
extern Posn cmdptadv;
extern Rangeset sel;
extern String cmdstr;
extern String genstr;
extern String lastpat;
extern String lastregexp;
extern String unixcmd;

