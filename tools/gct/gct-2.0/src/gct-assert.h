/* Copyright (C) 1992 by Brian Marick.  All rights reserved. */
/*
 * When aggregated with works covered by the GNU General Public License,
 * that license applies to this file.  Brian Marick retains the right to
 * apply other terms when this file is distributed as a part of other
 * works.  See the GNU General Public License (version 2) for more
 * explanation.
 */


/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-assert.h,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */

/* Sticky asserts are always on; ordinary asserts are turned off by NDEBUG. */

#define sticky_assert(expected)	\
{				\
  if (!(expected))		\
    {				\
      fprintf(stderr,"\"%s\", line %d: assertion failure\n", __FILE__, __LINE__);\
      abort();			\
    }				\
}

#ifndef NDEBUG
#	define assert(ex)	sticky_assert(ex)
#else
#	define assert(ex)
# endif
