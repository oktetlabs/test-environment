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
 * This routine contains code that handles the construction of race
 * instrumentation.   It is service code for gct-strans.c, which is
 * responsible for actually inserting the instrumentation.  This merely
 * returns the instrumentation to be inserted.
 *
 * The routines in the file (see gct-trans.h for a list) are called
 * to return statements or expressions to be spliced in as described in
 * the user's manual.  Note that any of these may (eventually) return a
 * comma-list, since a routine may be part of several race groups.
 * The routines are also responsible for maintaining the count of race
 * groups and for making mapfile entries.
 *
 * In the current implementation, these routines always return constant
 * strings. 
 */


/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-race.c,v 1.1 2003/10/28 09:48:45 arybchik Exp $
/* $Log: gct-race.c,v $
 * Revision 1.1  2003/10/28 09:48:45  arybchik
 * Initial revision
 *
/* Revision 1.1  2003/08/18 16:38:03  pabolokh
/* new tree is being added and committed
/*
 * Revision 1.6  1992/08/02  23:55:04  marick
 * Mapfile revision, part 1:  each file uses its own pointer into Gct_table.
 * - equivalent change is made for race coverage.
 *
 * Revision 1.5  1992/07/28  14:28:58  marick
 * Miscellaneous tidying.
 *
 * Revision 1.4  1992/07/24  19:18:46  marick
 * RS/6000 diffs
 *
 * Revision 1.3  1992/05/23  04:49:23  marick
 * Put CHECK_RACE_GROUP before RACE_GROUP_ENTER -- reasons for putting it
 * second were not good.  That means initialization is done there instead
 * of in RACE_GROUP_ENTER.
 *
 * Revision 1.2  1992/02/13  17:47:58  marick
 * Copylefted.
 *
 * Revision 1.1  91/09/09  12:47:19  marick
 * First version.
 *  */

#include <stdio.h>
#include "config.h"
#include "tree.h"
#include "gct-util.h"
#include "gct-tutil.h"
#include "gct-assert.h"
#include "gct-contro.h"
#include "gct-const.h"
#include "gct-files.h"
#include "gct-trans.h"


/* Number of next race group we see.  Starts at zero for each new file. */
int Gct_next_race_group = 0;

/* This is the number of race groups seen so far in all files.  Used to
   allocate the race group table.  Stored in gct-ps-defs.c */
int Gct_cumulative_race_groups = 0;


/*
 * The string name of the current index.  It always trails the next index
 * by one (when it has any value at all).
 */
static char Group_string[12];



/*
 * This routine returns a statement that contains the ENTER expressions for all
 * the race groups *** in the initial version, just one, the default. ***
 */
gct_node
race_entry_statement()
{
  gct_node enter_expr;

  enter_expr = newtree(makeroot(GCT_FUNCALL, (char *)0),
		       makeroot(GCT_IDENTIFIER, "GCT_RACE_GROUP_ENTER"),
		       makeroot(GCT_CONSTANT, permanent_string(Group_string)),
		       gct_node_arg_END);
  return make_simple_statement(enter_expr);
}

/*
 * This routine 
 * 1.  constructs mapfile entries for every race group this routine is in
 * (if this is the first entry to that race group) *** In the initial
 * version, a single mapfile entry is always built ***
 * 2.  returns a statement that contains the CHECK expressions for all
 * the race groups *** in the initial version, just one, the default. ***
 *
 * The by-reference argument is the number of the first mapfile entry; it
 * will be incremented by the number of mapfile entries issued.  The node
 * argument is a located node that is used to provide the filename and
 * linenumber. 
 *
 * Since this is the first race-instrumentation call made for a new
 * routine, it does any per-routine instrumentation.
 */
gct_node
race_check_statement(node, next_map_index)
	long *next_map_index;
{
  static char indexstring[12];
  gct_node check_expr;
  

  /* Initialize race globals, if needed. */
  if (   0 == Gct_next_race_group
      && 0 == Gct_cumulative_race_groups)
  {
    /* 0 is reserved as "no such group".  This currently has no use. */
    Gct_next_race_group = 1;	
  }

  /* This is a new routine; update globals. */
  sprintf(Group_string, "%d", Gct_next_race_group);
  Gct_next_race_group++;

  assert(*next_map_index >= 0);

  race_map(*next_map_index, node, DECL_PRINT_NAME(current_function_decl),
	   "is never probed.", FIRST);

  sprintf(indexstring, "%d", *next_map_index);
  (*next_map_index)++;

  check_expr = newtree(makeroot(GCT_FUNCALL, (char *)0),
		       makeroot(GCT_IDENTIFIER, "GCT_RACE_GROUP_CHECK"),
		       makeroot(GCT_CONSTANT, permanent_string(indexstring)),
		       makeroot(GCT_CONSTANT, permanent_string(Group_string)),
		       gct_node_arg_END);
  
  return make_simple_statement(check_expr);
}

/*
 * This routine returns an expression or comma-list that contains all the CALL
 * expressions needed before a function call.  *** In the initial version,
 * just one, the default, is returned. ***
 */
gct_node
race_call_expression()
{
  return newtree(makeroot(GCT_FUNCALL, (char *)0),
		 makeroot(GCT_IDENTIFIER, "GCT_RACE_GROUP_CALL"),
		 makeroot(GCT_CONSTANT, permanent_string(Group_string)),
		 gct_node_arg_END);
}

/*
 * This routine returns an expression or comma-list that contains all the
 * REENTER expressions needed after a function call.  *** In the initial version,
 * just one, the default, is returned. ***
 */
gct_node
race_reenter_expression()
{
  return newtree(makeroot(GCT_FUNCALL, (char *)0),
		 makeroot(GCT_IDENTIFIER, "GCT_RACE_GROUP_REENTER"),
		 makeroot(GCT_CONSTANT, permanent_string(Group_string)),
		 gct_node_arg_END);
}

/*
 * This routine returns a statement to be placed before any return
 * statements and before the end of the function.
 */
gct_node
race_return_statement()
{
  gct_node expr;
  
  expr = newtree(makeroot(GCT_FUNCALL, (char *)0),
		 makeroot(GCT_IDENTIFIER, "GCT_RACE_GROUP_EXIT"),
		 makeroot(GCT_CONSTANT, permanent_string(Group_string)),
		 gct_node_arg_END);
  
  return make_simple_statement(expr);
}

