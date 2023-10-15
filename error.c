#include <ansi.h>
#include <posix.h>
#include "edam.h"

static char *emsg[]={
   /* error_s */
   "can't open",
   "can't create",
   "not in menu:",
   "changes to",
   "I/O error:",
   /* error_c */
   "unknown command",
   "no operand for",
   "bad delimiter",
   /* error */
   "can't fork",
   "out of memory",
   "interrupt",
   "address",
   "search",
   "pattern",
   "newline expected",
   "blank expected",
   "pattern expected",
   "can't nest X or Y",
   "unmatched `}'",
   "command takes no address",
   "addresses overlap",
   "substitution",
   "substitution too long",
   "& match too long",
   "bad \\ in rhs",
   "address range",
   "changes not in sequence",
   "file name too long",
   "addresses out of order",
   "no file name",
   "unmatched `('",
   "unmatched `)'",
   "too many char classes",
   "malformed `[]'",
   "reg. exp. list overflow",
   "unix command",
   "can't pipe",
   "no current file",
   "string too long",
   "changed files",
   "empty string",
   "file search",
   "non-unique match for \"\"",
   "too many subexpressions",
   "malformed UTF-8"
};
static char *wmsg[]={
   /* warn_s */
   "duplicate file name",
   "no such file",
   "write might change good version of",
   /* warn */
   "non UTF-8 chars elided",
   "can't run pwd",
   "last char not newline",
   "exit status not 0",
};

void error(Error s);
void error_s(Error s, char *a);
void error_c(Error s, int c);
void warn(Warning s);
void warn_s(Warning s, char *a);
void dprint(char *z, ...);

void
error(Error s)
{
   char buf[512];
   sprintf(buf, "?%s", emsg[s]);
   hiccough(buf);
}
void
error_s(Error s, char *a)
{
   char buf[512];
   sprintf(buf, "?%s \"%s\"", emsg[s], a);
   hiccough(buf);
}
void
error_c(Error s, int c)
{
   char buf[512];
   sprintf(buf, "?%s `%c'", emsg[s], c);
   hiccough(buf);
}
void
warn(Warning s)
{
   dprint("?warning: %s\n", wmsg[s]);
}
void
warn_s(Warning s, char *a)
{
   dprint("?warning: %s `%s'\n", wmsg[s], a);
}
void
dprint(char *z, ...)
{
   va_list args;
   va_start(args,z);
   vfprintf(stderr, z, args);
   va_end(args);
}

