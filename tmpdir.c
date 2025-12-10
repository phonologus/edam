#include <ansi.h>
#include <posix.h>
#include <edam.h>

char *tempfname;
char *tmpdir;
char *tx = "edam-XXXXXXXX";
char *sep = "/";

static char *tmplate;

void
mktempfname(int i)
{
   if(!tempfname) {
      tempfname = alloc(strlen(tmpdir) + strlen(sep) + 6 + 1);
      if(!tempfname)
         panic("can;t alloc tempfname");
   }
   sprintf(tempfname, "%s%s%.6d", tmpdir, sep, i);
}

void
rmtmpdir(void)
{
   rmdir(tmpdir);
   free(tmpdir);
   free(tempfname);
}

char *
mktmpdir(char *td)
{
   char *tx = "edam-XXXXXXXX";
   char *sep = "/";
   tmplate = alloc(strlen(td)+strlen(sep)+strlen(tx)+1);
   if(!tmplate)
       panic("can't create temporary dirname");
   sprintf(tmplate, "%s%s%s", td, sep, tx);
   if(!mkdtemp(tmplate))
       panic("can't create temporary directory");
   atexit(rmtmpdir);
   return tmplate;
}
