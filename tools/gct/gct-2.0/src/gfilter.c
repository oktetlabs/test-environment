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



/* Gfilter -- filter out uninteresting entries from greport output. */
/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gfilter.c,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */

#include <stdio.h>

#define PATH_BUF_LEN	1025  /* Because there's no portable system define. */

char *Gct_input;		/* The input file */
FILE *Gct_input_stream;		/* The open input stream. */


/* These variables tell whether to show a particular kind of probe. */

int Show_branch = 0;
int Show_multi = 0;
int Show_loop = 0;
int Show_operator = 0;
int Show_operand = 0;
int Show_routine = 0;
int Show_call = 0;
int Show_race = 0;


main(argc,argv)
     int argc;
     char *argv[];
{

  /* Process args. */
  for(argv++; *argv; argv++)
    {
      if ((*argv)[0] == '-') 
      {
	if (!strcmp((*argv)+1, "branch"))
	  {
	    Show_branch = 1;
	  }
	else if (!strcmp((*argv)+1, "multi"))
	  {
	    Show_multi = 1;
	  }
	else if (!strcmp((*argv)+1, "loop"))
	  {
	    Show_loop = 1;
	  }
	else if (!strcmp((*argv)+1, "operator"))
	  {
	    Show_operator = 1;
	  }
	else if (!strcmp((*argv)+1, "operand"))
	  {
	    Show_operand = 1;
	  }
	else if (!strcmp((*argv)+1, "routine"))
	  {
	    Show_routine = 1;
	  }
	else if (!strcmp((*argv)+1, "call"))
	  {
	    Show_call = 1;
	  }
	else if (!strcmp((*argv)+1, "race"))
	  {
	    Show_race = 1;
	  }
	else
	  {
	    fprintf(stderr,"gfilter: Unknown argument -%s\n",(*argv));
	    exit(1);
	  }
      }
    else
      {
	if (Gct_input)		/* Already found. */
	  {
	    fprintf(stderr,"gfilter: gfilter takes only one file as argument.\n");
	    exit(1);
	  }
	else
	  {
	    Gct_input = (*argv);
	  }
      }
      
    }

  /* Open input file. */
  if (! Gct_input)
    {
      Gct_input_stream = stdin;
      Gct_input = "standard input";
    }
  else
    {
      Gct_input_stream = fopen(Gct_input, "r");
      if (NULL == Gct_input_stream)
	{
	  fprintf(stderr, "gfilter:  Could not open %s\n", Gct_input);
	  exit(1);
	}
    }


  /* Process the file. */
  for(;;)
    {
      char sourcefile[PATH_BUF_LEN], edit[100], probe_kind[100];
      int line;
      int hit = 0;
      /* Set to 1 if condition matches, so no branch probe can. */
      

      /* Get_report_line never returns when EOF is seen. */
      get_report_line(sourcefile, &line, edit, probe_kind);
      
      if (   ((hit = !strcmp(probe_kind, "condition")) && Show_multi)
	  || (!hit && (hit = !strcmp(probe_kind, "loop")) && Show_loop)
	  || (!hit && (hit = !strcmp(probe_kind, "operator")) && Show_operator)
	  || (!hit && (hit = !strcmp(probe_kind, "operand")) && Show_operand)
	  || (!hit && (hit = !strcmp(probe_kind, "routine")) && Show_routine)
	  || (!hit && (hit = !strcmp(probe_kind, "call")) && Show_call)
	  || (!hit && (hit = !strcmp(probe_kind, "race")) && Show_race)
	  || (!hit && Show_branch))	/* Anything else assumed to be branch. */
	{
	  emit_report_line_id(sourcefile, line, edit, probe_kind);
	  emit_report_rest();
	}
      else
	{
	  skip_report_rest();
	}
    }
}

