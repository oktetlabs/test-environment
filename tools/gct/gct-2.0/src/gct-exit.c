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
/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-exit.c,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */

#include <string.h>

/*
 * This list contains the names of all routines that cause a process
 * to exit.  If (options writelog) is set, a call to gct_writelog is
 * added as part of the last argument to the function, as in:
 *
 * exit ((( _G0_0  = ( a  <  45 )),  gct_writelog ("GCTLOG"),  _G0_0 ));
 *
 * Warning:  If the process_ending_routine takes more than one argument, 
 * it is still the last argument that is modified.  Remember that C does
 * not guarantee the order of evaluation, so you may lose some coverage.
 */

static char *(Process_ending_routines[]) =
{
  "exit",
  "abort",
  (char *)0	/* Add new names before this line. */
};


/* Return TRUE iff ROUTINENAME is a routine that exits the process. */
int
gct_exit_routine(routinename)
     char *routinename;
{
  int i = 0;

  for (i=0; (char *)0 != Process_ending_routines[i]; i++)
    {
      if (!strcmp(Process_ending_routines[i], routinename))
	{
	  return 1;
	}
    }
  return 0;
}

/* Return TRUE iff ROUTINENAME is the first routine in the process. */
int gct_entry_routine(routinename)
     char *routinename;
{
  return !strcmp(routinename, "main");
}
