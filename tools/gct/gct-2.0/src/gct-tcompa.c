/* Copyright (C) 1992 by Brian Marick */
/*
This file is part of GCT.

GCT is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GCT is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-tcompa.c,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */
/* $Log: gct-tcompa.c,v $
 * Revision 1.1  2003/10/28 09:48:45  arybchik
 * Initial revision
 *
/* Revision 1.1  2003/08/18 16:38:03  pabolokh
/* new tree is being added and committed
/*
 * Revision 1.4  1992/07/28  14:29:12  marick
 * Miscellaneous tidying.
 *
 * Revision 1.3  1992/07/24  19:18:55  marick
 * RS/6000 diffs
 *
 * Revision 1.2  1992/02/13  17:49:10  marick
 * Copylefted.
 * */

/*
 * These routines determine whether two GCC types can be compatibly
 * combined with a particular kind of operator.  Note that GCC proper of
 * course does this type of type checking, but it's interwoven with other
 * code.
 */

#include <stdio.h>
#include "config.h"
#include "tree.h"
#include "gct-util.h"
#include "gct-tutil.h"
#include "gct-assert.h"

/*
 * Return 1 if type1 and type2 are comparison-compatible.  The wrong
 * variable must of course be type compatible, else the compiler would
 * have caught it.  However, not every type-compatible variable is
 * appropriate; that'd lead to too many test cases.  We prune by assuming
 * that only certain types of variables are erroneously substituted.  Of
 * course, in the presence of typos, that's not really valid.  Time will
 * tell (and, even if typos abound, how about reasonable variable
 * names?)
 *
 * all integral types (longs, shorts, and chars) are compatible.
 * floats and doubles are compatible with integral types.
 * Pointers and arrays are compatible if their base types are identical.
 * All other types must be identical to be compatible.
 *
 * Special case: All types are compatible to NULL_TREE (for testing).
 *  	(NULL_TREE must be the second argument.)
 *
 * We don't call pointers compatible with integers because their values
 * are so rarely the same.  Although, given weak sufficiency, that may
 * not be valid.
 *
 * Note: the name "comparison compatible" was chosen to emphasize that
 * we're NOT just testing for equality-compatibility, although the two
 * trees will be combined with !=.
 */


int
comparison_compatible(type1, type2)
     tree type1, type2;
{
  if (NULL_TREE == type2)
    {
      return 1;
    }

  if (type1 == type2)
    {
      return 1;
    }

  if (   ((INTEGER_TYPE == TREE_CODE(type1)) ||(REAL_TYPE == TREE_CODE(type1)))
      && ((INTEGER_TYPE == TREE_CODE(type2)) ||(REAL_TYPE == TREE_CODE(type2))))
    {
      return 1;
    }

  /* Handle structured types:  arrays and pointers. */

  if (POINTERISH_P(type1))
    {
      type1 = TREE_TYPE(type1);
    }
  else
    {
      return 0;
    }
  
  if (POINTERISH_P(type2))
    {
      type2 = TREE_TYPE(type2);
    }
  else
    {
      return 0;
    }
  
  return type1 == type2;
}
#undef DESCEND_TYPE

/* Can the two types be combined by multiplication? */
int
times_compatible(type1, type2)
     tree type1, type2;
{
  return (   (   (INTEGER_TYPE == TREE_CODE(type1))
	      || (REAL_TYPE == TREE_CODE(type1)))
	  && (   (INTEGER_TYPE == TREE_CODE(type2))
	      || (REAL_TYPE == TREE_CODE(type2))));
  
}
