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

/*
 * Limited instrumentation is done within macros.  The file named by
 * #define MACRO_FILENAME contains information about the extent of
 * macros.  This code reads that file and answers queries about it.
 */

/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-macros.c,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */
/* $Log: gct-macros.c,v $
 * Revision 1.1  2003/10/28 09:48:45  arybchik
 * Initial revision
 *
/* Revision 1.1  2003/08/18 16:38:03  pabolokh
/* new tree is being added and committed
/*
 * Revision 1.8  1992/07/31  16:32:01  marick
 * File for macro information now selected by gcc.c.
 * Missing macros files handled slightly more gracefully.
 *
 * Revision 1.7  1992/07/28  14:28:47  marick
 * Miscellaneous tidying.
 *
 * Revision 1.6  1992/05/24  20:17:37  marick
 * Bug (found by coverage testing):  Used wrong test to detect when a
 * backwards search hit the leftmost macro.
 *
 * Revision 1.5  1992/05/24  18:46:21  marick
 * Previous implementation was inefficient; made more efficient.
 * */

#include <stdio.h>
#include "gct-assert.h"
#include "gct-macros.h"
#include "gct-contro.h"

/* This is the list of macros from CCCP. */
static macro_data *Gct_macro_list = NULL_MACRO_DATA;

/* The number of macros. */
static int Gct_macro_count = 0;

/* Whether we've created the list (0-length list would otherwise be
   indistinguishable. */
static int Gct_macro_initialized = 0;


/* This is the macro last found by gct_in_macro_list.  It is valid only
   until the caller processes another node. */
static macro_data *Gct_current_macro = NULL_MACRO_DATA;

/* So we know if Gct_current_macro is valid and gct_macro_name may be called. */
static int Gct_macro_found = 0;



/* This creates a list from the named file. */
static 
make_macro_list(file)
     char *file;
{
  FILE *macro_stream;
  
  assert(NULL_MACRO_DATA == Gct_macro_list);
  assert(!Gct_macro_initialized);

  if (0 == Gct_macro_file_exists)
    {
      Gct_macro_count = 0;
    }
  else if (NULL == (macro_stream = fopen(file, "r")))
    {
      warning("Could not open macro file `%s'.", file);
      warning("Most likely GCT invoked the wrong preprocessor.");
      warning("This is probably an installation error.");
      warning("Try using 'gct -v' to diagnose the problem.");
      warning("In the meantime, the contents of macros will be instrumented.");
      Gct_macro_count = 0;
    }
  else
    {
      if (1 != fscanf(macro_stream, "%d\n", &Gct_macro_count))
	{
	  fatal("Couldn't read count of macros from macro data file.\n");
	}
    }
  
  if (Gct_macro_count > 0)
    {
      int index;	/* For reading successive entries. */
      int size;		/* Size of current entries name. */
      int c;		/* Random character. */
      
      
      Gct_macro_list = (macro_data *) xmalloc(Gct_macro_count * sizeof(macro_data));

      for(index = 0; index < Gct_macro_count; index++)
	{
	  if (1 != fscanf(macro_stream, "%d ", &size))
	    fatal("Couldn't read name size from macro file (index %d)\n",
		  index);
	  Gct_macro_list[index].name = (char *) xmalloc(size + 1);
	  if (3 != fscanf(macro_stream, "%s %d %d\n",
			  Gct_macro_list[index].name,
			  &(Gct_macro_list[index].start),
			  &(Gct_macro_list[index].end)))
	    fatal("Couldn't read data for macro %d.\n", index);
	  for (c = getc(macro_stream); '\n' != c; c = getc(macro_stream))
	    {
	      if (EOF == c)
		fatal("Unexpected EOF in macro file.\n");
	    }
	}
    }

  /* Start first search at the beginning. */
  Gct_current_macro = Gct_macro_list;		/* May be NULL */
  Gct_macro_found = 0;

  /* print_macro_list(); */
}


/*
 * This returns 1 if the given LOCATION is within a macro expansion; 0
 * otherwise.   If the MACROS option is turned on, macros are ignored,
 * which means this routine returns 0.
 *
 * Because the caller will be processing the tree preorder, roots before
 * branches, the LOCATION argument will not be steadily increasing.
 * We have to search in either direction from the last LOCATION given.
 */
gct_in_macro_p(location)
{
  if (! Gct_macro_initialized)
    {
      make_macro_list(Gct_macro_file);
      Gct_macro_initialized = 1;
    }

  /*
   * A likely program error is to pass in a tree without a location, one
   * that was created by GCT.  See gct_build_funcall for a case where
   * this might happen but is avoided.
   */
  if (0 == location)
    {
      warning("Program error: gct_in_macro_p called with unlocated tree.\n");
      abort();
    }

  Gct_macro_found = 0;
  
  if (   NULL_MACRO_DATA == Gct_macro_list		/* No macros. */
      || ON == gct_option_value(OPT_MACROS))
    return 0;

  if (Gct_current_macro->end <= location)
    {
      do
	{
	  if (Gct_macro_count == Gct_current_macro - Gct_macro_list + 1)
	    return 0;
	  Gct_current_macro++;
	}
      while (Gct_current_macro->end <= location);
    }
  else if (Gct_current_macro->start > location)
    {
      do
	{
	  if (Gct_macro_list == Gct_current_macro)
	    return 0;
	  Gct_current_macro--;
	}
      while (Gct_current_macro->start > location);
    }
  
  if (location >= Gct_current_macro->start && location < Gct_current_macro->end)
    {
      Gct_macro_found = 1;
      return 1;
    }
  else
    {
      return 0;
    }
}

/* The inverse of previous routine; for clarity. */
gct_outside_macro_p(location)
{
  return !gct_in_macro_p(location);
}

/*
 * This routine should only be called after gct_in_macro_p has returned 1
 * (or gct_outside_macro_p has returned 0).  It returns the name of the
 * macro located by that call.
 *
 * The caller should not mess with the return value; call
 * permanent_string() to get a value to modify.  However, the value can
 * be considered constant.
 */
char *
gct_macro_name()
{
  assert(Gct_current_macro);
  assert(Gct_macro_list);
  assert(Gct_current_macro - Gct_macro_list < Gct_macro_count);
  assert(Gct_macro_found);

  return Gct_current_macro->name;
}

  

/* Debugging */
static
print_macro_list()
{
  int index;

  for (index = 0; index < Gct_macro_count; index++)
    {
      printf("%d %s %d %d\n",
	      strlen(Gct_macro_list[index].name),
	      Gct_macro_list[index].name,
	      Gct_macro_list[index].start,
	      Gct_macro_list[index].end);
    }
}
