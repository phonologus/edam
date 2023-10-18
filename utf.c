/*
 * Copyright (c) 2023 Sean Jensen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/*
 * UTF-8 
 */

#include "utf.h"

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
int u8needs(int);
int u8decode(char *);
char *u8next(char *);
char *u8prev(char *);

int u8get(getter get, char *p);
int u8bget(getter bget, char *p);

int
u8typ(char c)
{
   return u8typ_[(unsigned char)(c)];
}

/*
 * u8len(char c) returns the number of bytes of UTF-8 byte c promises,
 * or 0 if c is not a valid UTF-8 start byte.
 *
 * Note that no checking is done as to whether the ensuing bytes are
 * actually a valid UTF-8 sequence, or not.
 *
 */

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
 * u8needs(int u) returns the number of bytes needed
 * to encode Unicode codepoint u into UTF-8.
 *
 * returns 0 if u is not a valid Unicode codepoint.
 *
 */

int
u8needs(int u)
{
   unsigned int t=(unsigned int)u;
   if(!(t>>=7))
      return 1;      /* needs 1 byte */
   if(!(t>>=4))
      return 2;      /* needs 2 bytes */
   if(!(t>>=5))
      return 3;      /* needs 3 bytes */
   if(!(t>>=5))
      return 4;      /* needs 4 bytes */
   return 0;         /* out of range */
}

/*
 * u8encode(u,p) encodes the Unicode codepoint u into byte array p,
 * and returns the number of bytes used in the encoding, or 0 on error.
 *
 */ 
int
u8encode(int u, char *p)
{
  int n;
  switch (n=u8needs(u)) {
    case 1:
      *p++=(char)u; 
      return n;
    case 2:
      *p++=(char)((0300)|((u>>6)&077));
      goto two;
    case 3:
      *p++=(char)((0340)|((u>>12)&077));
      goto three;
    case 4:
      *p++=(char)((0360)|((u>>18)&077));
      goto four;
    default:
      return n;
  }
  four:
    *p++=(char)((0200)|((u>>12)&077));
  three:
    *p++=(char)((0200)|((u>>6)&077));
  two:
    *p++=(char)((0200)|(u&077));

  return n;
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
u8getc(getter get, char *p)
{
  int c;
  int n=1;
  if((c=get())==-1)
    return 0;
  switch (u8typ(*p++=c&0377)) {
    case U8ASCII:
      return n;       /* ascii byte */
    case U84HDR:
      if((c=get())==-1){
        n=1-n;
        break;
      }
      ++n;
      if(u8typ(*p++=c&0377)!=U8SEQ){
         n=-n;  /* didn't find expected sequence byte */
         break;
      }
    case U83HDR:
      if((c=get())==-1){
        n=1-n;
        break;
      }
      ++n;
      if(u8typ(*p++=c&0377)!=U8SEQ){
         n=-n;  /* didn't find expected sequence byte */
         break;
      }
    case U82HDR:
      if((c=get())==-1){
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
u8bgetc(getter bget,char *p)
{
   int c;
   int n=1;
   if((c=bget())==-1)
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
   if((c=bget())==-1)
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
   if((c=bget())==-1)
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
   if((c=bget())==-1)
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

