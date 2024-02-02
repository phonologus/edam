.SUFFIXES: .o .c .h
.PHONY: all clean install uninstall

CFLAGS= -O2 -Wall -pedantic -fno-builtin -I.
LDFLAGS=
CC=cc

INSTALL=install
BUNDLE=bundle

PROG=edam

INSTALLD=/usr/local
BINDIR=$(INSTALLD)/bin
MANDIR=$(INSTALLD)/share/man/man

MSECTION=1

MANPAGE=$(PROG).$(MSECTION)

HDRS=\
  ansi.h \
  posix.h \
  funcs.h \
  errors.h \
  parse.h \
  edam.h \
  utf.h \

UNITS=\
  address \
  buffer \
  cmd \
  disc \
  error \
  file \
  io \
  list \
  malloc \
  misc \
  multi \
  regexp \
  edam \
  string \
  sys \
  unix \
  utf \
  xec \

OBJS=$(UNITS:=.o)
SRCS=$(UNITS:=.c)

.c.o:
	$(CC) -c $(CFLAGS) $<

all: $(PROG)

$(PROG): $(HDRS) $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS)

clean: 
	rm -f *.o $(PROG)

install: all $(MANPAGE) $(PROG)save s$(PROG) s$(MANPAGE)
	$(INSTALL) -d $(BINDIR) $(MANDIR)$(MSECTION)
	$(INSTALL) -s $(PROG) $(BINDIR)/
	$(INSTALL) $(PROG)save $(BINDIR)/
	$(INSTALL) $(MANPAGE) $(MANDIR)$(MSECTION)/
	$(INSTALL) s$(PROG)save $(BINDIR)/
	$(INSTALL) s$(MANPAGE) $(MANDIR)$(MSECTION)/

uninstall:
	rm -f $(BINDIR)/$(PROG)
	rm -f $(BINDIR)/$(PROG)save
	rm -f $(BINDIR)/s$(PROG)
	rm -f $(MANDIR)$(MSECTION)/$(MANPAGE)
	rm -f $(MANDIR)$(MSECTION)/s$(MANPAGE)

bundle: $(HDRS) $(SRCS) $(MANPAGE) Makefile $(PROG)save s$(PROG) s$(MANPAGE)
	rm -f $@
	$(BUNDLE) $^ > $@

