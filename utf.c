#include <ansi.h>
#include <posix.h>
#include "edam.h"

/*
 * UTF-8 
 */

/*
 * lookup table for byte status
 *
 */

static char u8typ_[]={
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
   5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
   5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
   5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
   2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
   3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
   4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0
};

int u8typ(char);
int u8len(char);
int u8decode(char *);
char *u8next(char *);
char *u8prev(char *);

int u8fgetc(File *f, char *p);
int u8fbgetc(File *f, char *p);

int
u8typ(char c)
{
   return u8typ_[(unsigned char)(c)];
}

int
u8len(char c)
{
   switch(u8typ(c)){
      case U8ASCII:
         return 1;
      case U82HDR:
         return 2;
      case U83HDR:
         return 3;
      case U84HDR:
         return 4;
   }
   return 0;
}

/*
 * u8decode() returns
 * the Unicode code point encoded by the UTF-8 its argument points
 * to, or -1 if the argument is invalid UTF-8.
 *
 */

int
u8decode(char *p)
{
  char c;
  int u;
  switch (u8typ(c=*p++)) {
    case U8ASCII:
      return c;
    case U82HDR:
      u=c&037;   /* initialise return value */
      goto two;
    case U83HDR:
      u=c&017;   /* initialise the return value */
      goto three;
    case U84HDR:
      u=c&07;    /* initialise the return value */
      goto four;
    default:
      return -1;
  }

  four:
    if(u8typ(c=*p++)!=U8SEQ)
      return -1;
    u=(u<<6) | (c&077);
  three:
    if(u8typ(c=*p++)!=U8SEQ)
      return -1;
    u=(u<<6) | (c&077);
  two:
    if(u8typ(c=*p++)!=U8SEQ)
      return -1;
    u=(u<<6) | (c&077);

  return u;

}


/*
 * u8next() points to the start of the next character, or
 * NULL if not valid UTF-8
 *
 */

char *
u8next(char *p)
{
  switch (u8typ(*p++)) {
    case U8ASCII:
      return p;       /* ascii byte */
    case U84HDR:
      if(u8typ(*p++)!=U8SEQ)
         return (char *)0;  /* didn't find expected sequence byte */
    case U83HDR:
      if(u8typ(*p++)!=U8SEQ)
         return (char *)0;  /* didn't find expected sequence byte */
    case U82HDR:
      if(u8typ(*p++)!=U8SEQ)
         return (char *)0;  /* didn't find expected sequence byte */
      break;
    default:
      return (char *)0;    /* no valid UTF-8 header byte at p */
  }
  return p;
}

/*
 * u8prev() points the start of the previous character
 *
 */

char *
u8prev(char *p)
{
  switch (u8typ(*--p)) {
    case U8ASCII:
      return p;        /* ascii byte */
    case U8SEQ:
      goto two;
    default:
      return (char *)0;        /* can't find a reverse UTF-8 sequence. */
  }

  two:
  switch (u8typ(*--p)) {
    case U82HDR:
      return p;
    case U8SEQ:
      goto three;
    default:
      return (char *)0;    /* can't find a revers UTF-8 sequence. */
  }

  three:
  switch (u8typ(*--p)) {
    case U83HDR:
      return p;
    case U8SEQ:
      goto four;
    default:
      return (char *)0;    /* can't find a reverse UTF-8 sequence. */
  }

  four:
  switch (u8typ(*--p)) {
    case U84HDR:
      return p;
    default:
      return (char *)0;        /* can't find a reverse UTF-8 sequence. */
  }

  /* not reached */
  return (char *)0;

}

/*
 * The number of bytes read is returned. If the bytes do
 * *not* form a valid UTF-8 sequence, the negative of the
 * number of bytes read is returned.
 *
 * 0 is returned on end of buffer
 *
 */

int
u8fgetc(File *f, char *p)
{
  int c;
  int n=1;
  if((c=Fgetc(f))==-1)
    return 0;
  switch (u8typ(*p++=c&0377)) {
    case U8ASCII:
      return n;       /* ascii byte */
    case U84HDR:
      if((c=Fgetc(f))==-1){
        n=1-n;
        break;
      }
      ++n;
      if(u8typ(*p++=c&0377)!=U8SEQ){
         n=-n;  /* didn't find expected sequence byte */
         break;
      }
    case U83HDR:
      if((c=Fgetc(f))==-1){
        n=1-n;
        break;
      }
      ++n;
      if(u8typ(*p++=c&0377)!=U8SEQ){
         n=-n;  /* didn't find expected sequence byte */
         break;
      }
    case U82HDR:
      if((c=Fgetc(f))==-1){
        n=1-n;
        break;
      }
      ++n;
      if(u8typ(*p++=c&0377)!=U8SEQ)
         n=-n;  /* didn't find expected sequence byte */
      break;
    default:
      n=-n;
      break;    /* no valid UTF-8 header byte at p */
  }

  return n; 

}

int
u8fbgetc(File *f,char *p)
{
   int c;
   int n=1;
   if((c=Fbgetc(f))==-1)
     return 0;
   switch (u8typ(*--p=c&0377)) {
     case U8ASCII:
       return n;        /* ascii byte */
     case U8SEQ:
       goto two;
     default:
       return -n;        /* can't find a reverse UTF-8 sequence. */
   }

   two:
   if((c=Fbgetc(f))==-1)
     return 1-n;
   ++n;
   switch (u8typ(*--p=c&0377)) {
     case U82HDR:
       return n;
     case U8SEQ:
       goto three;
     default:
       return -n;    /* can't find a revers UTF-8 sequence. */
   }

   three:
   if((c=Fbgetc(f))==-1)
     return 1-n;
   ++n;
   switch (u8typ(*--p=c&0377)) {
     case U83HDR:
       return n;
     case U8SEQ:
       goto four;
     default:
       return -n;    /* can't find a reverse UTF-8 sequence. */
   }

   four:
   if((c=Fbgetc(f))==-1)
     return 1-n;
   ++n;
   switch (u8typ(*--p=c&0377)) {
     case U84HDR:
       return n;
     default:
       return -n;        /* can't find a reverse UTF-8 sequence. */
   }
 
   return -n;    /* not reached */

}

