# edam
Edam is a port of Research Unix `sam -d`. It is ed-like sam. It is UTF-8 aware.

Unlike the `Plan9` version of `sam`, this version works directly with UTF-8 bytestreams,
converting to Unicode code points on-the-fly only when strictly necessary. I have no
idea if this is a good or bad thing, but it was a fun exercise, and it seems to work.

The port was developed on an M2 Mac with the CommandLine Developer Tools. It is
a typical Unix commandline utility of its time, so should be reasonably portable
to Linux.

There are a couple of _gothcas_ that porters should be aware of (scantily commented in `edam.h`),
thanks to some low-level type-aliasing.
