#include <ansi.h>
#include <posix.h>
#include "edam.h"

struct Filelist file;

File * newfile(void);
void delfile(File *f);
void sortname(File *f);
int whichmenu(File *f);
void state(File *f, enum State cleandirty);
File * lookfile(void);

File *
newfile(void)
{
   File *f;
   inslist((List *)&file, 0, (long)(f=Fopen()));
   return f;
}
void
delfile(File *f)
{
   int w=whichmenu(f);
   dellist((List *)&file, w);
   Fclose(f);
}
void
sortname(File *f)
{
   int i, cmp;
   int w=whichmenu(f);
   int dupwarned=FALSE;
   dellist((List *)&file, w);
   for(i=0; i<file.nused; i++){
      cmp=strcmp(f->name.s, file.ptr[i]->name.s);
      if(cmp==0 && !dupwarned){
         dupwarned=TRUE;
         warn_s(Wdupname, (char *)f->name.s);
      }else if(cmp<0 && (i>0))
         break;
   }
   inslist((List *)&file, i, (long)f);
}
int
whichmenu(File *f)
{
   int i;
   for(i=0; i<file.nused; i++)
      if(file.ptr[i]==f)
         return i;
   return -1;
}
void
state(File *f, enum State cleandirty)
{
   f->state=cleandirty;
}
File *
lookfile(void)
{
   int i;
   for(i=0; i<file.nused; i++)
      if(strcmp(file.ptr[i]->name.s, genstr.s)==0)
         return file.ptr[i];
   return 0;
}
