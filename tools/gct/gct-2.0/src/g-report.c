/* Copyright (C) 1992 by Brian Marick.  All rights reserved.  */
/*
 * When aggregated with works covered by the GNU General Public License,
 * that license applies to this file.  Brian Marick retains the right to
 * apply other terms when this file is distributed as a part of other
 * works.  See the GNU General Public License (version 2) for more
 * explanation.
 */

/* Code used to read and rewrite greport output. */

/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/g-report.c,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */

#include <stdio.h>
#include "g-report.h"


/*
 * Given 
 *
 * "test.c", line 3: if was taken TRUE 1, FALSE 0 times.
 *
 * This returns these values:  
 * 	the string "test.c"
 * 	the integer 3
 * 	the string " "
 * 	the string "if"
 *
 * Given 
 *
 * "test.c", line 3: [4: 0] operator < might be <=.  [0]
 *
 * it returns
 * 	the string "test.c"
 * 	the integer 3
 * 	the string " [4: 0] "
 * 	the string "operator"
 *
 * The bracketed string is added by greport -edit.
 *
 * Syntax errors cause an error message and program exit with status 1.
 * EOF causes an exit with status 0.
 *
 * Assumptions:  
 * 1.  The arguments are large enough to hold the values.
 */

void
get_report_line(sourcefile, line, edit, probe_kind)
     char *sourcefile;
     int *line;
     char *edit;
     char *probe_kind;
{
  int number;		/* Return value from scanf. */
  int c;		/* Lookahead character. */
  

  /* First two args are always there. */
  number = fscanf(Gct_input_stream, "\"%[^\"]\", line %d: ",
		  sourcefile, line);
  if (number != 2 && feof(Gct_input_stream))
    {
      exit(0);
    }
  else if (number != 2)
    {
      goto syntax_error;
    }

  /* Third arg is optional. */
  c = getc(Gct_input_stream);
  ungetc(c, Gct_input_stream);
  *edit = '\0';
  strcat(edit, " ");
  if (c == '[')	/* greport -edit was used. */
    {
      number = fscanf(Gct_input_stream, "%[^]]] ", edit+1);
      if (number != 1)
	goto syntax_error;
      strcat(edit, "] ");
    }

  /* Fourth arg is always there. */
  number = fscanf(Gct_input_stream, "%s", probe_kind);
  if (number != 1)
    goto syntax_error;
  return;

 syntax_error:
  fprintf(stderr,"Syntax error in file %s\n", Gct_input);
  exit(1);
}


/*
 * Given the return values from get_report_line, this recreates exactly the
 * characters that get_report_line used up.
 */
void
emit_report_line_id(filename, lineno, edit, probe)
     char *filename;
     int lineno;
     char *edit;
     char *probe;
{
  printf("\"%s\", line %d:%s%s", filename, lineno, edit, probe);
}

/*
 * get_report_line uses up only part of a line; this routine echoes the
 * remainder to standard output. 
 *
 * Note:  if the file is not newline terminated, this will add a newline.
 */
void
emit_report_rest()
{
  char c;

  while((c = getc(Gct_input_stream))!= '\n' && c != EOF)
    putchar(c);
  putchar('\n');
}

/*
 * get_report_line uses up only part of a line; this silently swallows
 * the remainder.  
 */
void skip_report_rest()
{
  char c;
  
  while((c = getc(Gct_input_stream))!= '\n' && c != EOF)
    {
    }
}

