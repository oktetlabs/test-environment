/* Copyright (C) 1992 by Brian Marick.  All rights reserved. */
/*
 * When aggregated with works covered by the GNU General Public License,
 * that license applies to this file.  Brian Marick retains the right to
 * apply other terms when this file is distributed as a part of other
 * works.  See the GNU General Public License (version 2) for more
 * explanation.
 */

/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-macros.h,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */


/*
 * This describes the format of in-core and on-disk entries about macros. 
 * Someday, we might want to merge macros and comment handling (which is
 * only partially implemented).  More likely, we'll want to throw out
 * comment handling entirely.
 */

/* In core: */

struct macro_data_struct
{
  char *name;		/*  Name of macro that was expanded. */
  int start;		/*  Offset of first character. */
  int end;		/*  Offset of first char not in macro. */
};

typedef struct macro_data_struct macro_data;
#define NULL_MACRO_DATA	((macro_data *)0)


/* On disk: */

/*
 * Macros are stored in this file unless the invoker specifies the -test-macro
 * argument (which gct always does).  The file may not exist (if, for
 * example, we're instrumenting a .i file).  In that case
 * Gct_macro_file_exists is set to 0. 
 */

#define DEFAULT_MACRO_FILENAME	"__gct-macros"
extern char *Gct_macro_file;
extern int Gct_macro_file_exists;

/*
 * On disk, macro data is stored as two lines of info:
 *
 * <namelen> <name> <start> <end>
 * <text>
 *
 * The <namelen> is the number of characters to allocate for <name>.
 * <name> is the name of the macro.  <start> is the first character
 * within the macro.  <end> is the first character NOT in the macro.
 * <text> is the expanded text of the macro.  It's for debugging.
 *
 * The list of macro data entries is preceded by a single line containing
 * the number of entries.
 */

int gct_in_macro_p();
int gct_outside_macro_p();
char *gct_macro_name();
