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
/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-commen.c,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */
/* $Log: gct-commen.c,v $
 * Revision 1.1  2003/10/28 09:48:45  arybchik
 * Initial revision
 *
/* Revision 1.1  2003/08/18 16:38:03  pabolokh
/* new tree is being added and committed
/*
 * Revision 1.4  1992/07/28  14:28:08  marick
 * Miscellaneous tidying.
 *
 * Revision 1.3  1992/02/13  17:42:41  marick
 * Copylefted.
 * */

/*
 * This file contains the code used to for the creation and manipulation
 * of option-commands.   See also gct-contro.c 
 */

#include <stdio.h>
#include "gct-contro.h"
#include "gct-assert.h"

/*
 * CCCP initially builds the comment list, growing it to contain however
 * many comments are needed.  It writes it out.  CC1 reads it in and
 * executes the commands as needed.
 */

#define DEFAULT_COMMENT_LIST_SIZE	10

/* In a production version, GCT would pass the name of a temp file to
   both CC1 and CCCP. */
#define COMMENT_FILE	"__GCT_COM"


/*
 * This variable points to the lists of comment-commands.  Remember that
 * the list can grow.
 */
gct_option_command *Gct_comment_commands;

/*
 * This is the number of entries in the comment list; used when deciding
 * whether to grow it.
 */
int Gct_comment_list_size;


/*
 * When building the list, this is the next unused element.  When
 * processing the list, this is the "low water mark" -- any comments
 * before here can be ignored (their effect has already been made
 * permanent.
 */
gct_option_command *Gct_next_command;



/* This creates a list, initially with NUMBER option commands. */
make_comment_list(number)
     int number;
{
  Gct_comment_commands = (gct_option_command *) xmalloc(number * sizeof(gct_option_command));
  Gct_comment_list_size = number;
}

/*
 * This reads in the comment list from the given file.  The file has the
 * following structure:  
 * 	long int number;		-- Number of commands.
 * 	gct_option_command array[number];
 * 	
 */
init_comment_list(file)
     char *file;
{
}

/* Write the comment list to the given file. */
write_comment_list(file)
     char *file;
{
}


/*
 * This returns an pointer to the next unused option_command in the
 * comment_commands.
 */

gct_option_command *
get_unused_command()
{
  /* Don't forget to grow. */
}


		       /* Applying Commands. */


commands_up_through(charno)
     long charno;
{
  /* It's an error if backup is not null. */
}


			    /* Printing */

print_comment_list(stream)
{
}


