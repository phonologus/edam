The _Edam_ editor
=================

_Edam_ is the Research Unix Version 10 editor `sam` without
the graphical terminal driver. It only has the `ed`-like
line editor terminal interface. It should be able to serve
as a drop-in replacement for `sam -d`.

The name comes from `ed` + headless `sam`,
or, `edam` (which is a bit cheesy).

The Unix and Plan9 _Sam_
editor was written by Rob Pike. _Edam_ is derived from the
Research Unix Version 10 _Sam_ sourcecode, with all the
graphical terminal code removed, and a lightweight UTF-8
processing library added.

_Edam_ is described completely in the `edam(1)` manpage. It is a line
editor, in the tradition of _ed_, with multi-file editing, with
structural regular expressions, and an elegant command language.
_Edam_ works with UTF-8 encoded text.

Included with _Edam_ is a wrapper script `sedam`, and its associated manpage
`sedam.1`, which provides a `sed`-like interface to `edam`. Thus one
can do things like

    cat foo | sedam 's/a/z/g'

Building
========

Building and installing _Edam_ should be as easy as:

    make clean && make && make install

Installation is into `/usr/local/bin`. The `edam` binary is
self-contained, and can be moved anywhere. The manpage is
installed to `/usr/local/share/man/man1`. The `sedam` script
and its manpage are also installed.

To uninstall, `make uninstall`.

New Features
============

UTF-8
-----

Although the Plan9 _Sam_ editor processes UTF-8, it uses
a completely different implementation strategy. _Edam_
converts UTF-8 text into Unicode code points, on the fly,
but only when it absolutely has to. This drastically reduces the memory
and on-disk storage requirements, at the expense of a little extra
processing. 

_Edam_ will refuse to work with malformed UTF-8. This can be annoying! 

Sources
=======

_Edam_ is derived from the Research Unix Version 10 _Sam_
sources available from the Unix Archive [here][1], and located
in directory `/jerq/src/sam/`.

[1]: https://www.tuhs.org/Archive/Distributions/Research/Norman_v10/

_Sedam_, and both manpages, are derived from the [Plan9 port][2]
versions of these files.

[2]: https://github.com/9fans/plan9port

Authors
=======

_Sam_ was written by Rob Pike in the late 1980s. It survives to this day
in the [Plan 9 port][2].

In 2023, Sean Jensen reformatted the original 1980s Unix
sourcecode to be ANSI-compliant C, and made changes to (i) make
it compile on an up-to-date ANSI/POSIX system; (ii) to add new
capabilities for processing UTF-8 encoded text; and (iii) to
remove the graphical terminal/GUI dependencies.

The source files `utf.[ch]` were written entirely by Sean Jensen.

