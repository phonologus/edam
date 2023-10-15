#ifndef UTFINCLUDED
#define UTFINCLUDED

#define U8BYTES 4
#define ESCAPED  0x200000

enum {
  U8INVALID=0,
  U8ASCII,
  U82HDR,
  U83HDR,
  U84HDR,
  U8SEQ
};

int u8typ(char);
int u8len(char);
int u8decode(char *);
char *u8next(char *);
char *u8prev(char *);

int u8fgetc(File *f, char *p);
int u8fbgetc(File *f, char *p);

#endif
