/* GNU Objective C Runtime forward tests program
   Copyright (C) 1993 Free Software Foundation, Inc.

Author: Kresten Krab Thorup

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify it under the
   terms of the GNU General Public License as published by the Free Software
   Foundation; either version 2, or (at your option) any later version.

GNU CC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
   details.

You should have received a copy of the GNU General Public License along with
   GNU CC; see the file COPYING.  If not, write to the Free Software
   Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */


#include <stdio.h>
#include "gstddef.h"

static char* flags = "";

void try_func (a, b)
     size_t a;
     size_t b;
{
   if ((a==5432) && (b==12))
     printf(flags);
   exit (0);
}

void do_apply (xx, y)
     void* xx;
     size_t y;
{
  __builtin_apply((void*)try_func, xx, 200) ;
}

#define try(what)       \
      what[0] = 5432;   \
      what[1] = 12;     \
      do_apply(af, 1234)

void test_apply (a, b) 
     size_t a;
     size_t b;
{
  union frame {
    struct { size_t* args; } a;
    struct { size_t* args; size_t regs[2]; } b;
    struct { size_t* args; void* struct_return; size_t regs[2]; } c;
  } *af;
  af = __builtin_apply_args();
  if ((af->b.regs[0] == 2345) && (af->b.regs[1] == 6789))
    { flags=" -DREG_ARGS\n";  try (af->b.regs); }
  if ((af->c.regs[0] == 2345) && (af->c.regs[1] == 6789))
    { flags=" -DREG_ARGS -DSTRUCT_RETURN\n"; try (af->c.regs); }
  if ((af->a.args[0] == 2345) && (af->a.args[1] == 6789))
    { flags=" -DSTACK_ARGS\n"; try (af->a.args); }
  exit (0);
}

void main () 
{
  test_apply (2345, 6789);
}
