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
int u8needs(int);
int u8encode(int, char*);
char *u8next(char *);
char *u8prev(char *);

/*
 * the `getter` type should return -1 on end of file, like
 * many of the low-level Unix reading/getting functions.
 *
 */ 

typedef int (*getter)(void);

int u8getc(getter g, char *p);
int u8bgetc(getter g, char *p);

#endif
