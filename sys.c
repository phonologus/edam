#include <ansi.h>
#include <posix.h>
#include "edam.h"
/*
 * A reasonable interface to the system calls
 */

void syserror(char *a);
int Read(int f, char *a, size_t n);
int Write(int f, char *a, size_t n);
void Lseek(int f, size_t n, int w);

void
syserror(char *a)
{
   dprint("%s: ", a);
   error_s(Eio, strerror(errno));
}
int
Read(int f, char *a, size_t n)
{
   if(read(f, (char *)a, n)!=n)
      syserror("read");
   return n;
}
int
Write(int f, char *a, size_t n)
{
   int m;
   if((m=write(f, (char *)a, n))!=n)
      syserror("write");
   return m;
}
void
Lseek(int f, size_t n, int w)
{
   if(lseek(f, n, w)==-1)
      syserror("lseek");
}
