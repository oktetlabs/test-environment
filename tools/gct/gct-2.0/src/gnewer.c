/* Copyright (C) 1992 by Brian Marick.  All rights reserved. */
/*
 * When aggregated with works covered by the GNU General Public License,
 * that license applies to this file.  Brian Marick retains the right to
 * apply other terms when this file is distributed as a part of other
 * works.  See the GNU General Public License (version 2) for more
 * explanation.
 */

#include <stdio.h>


/* Control flags. */

/* If true, show logical difference, not absolute difference.  */
int logical = 0;

/* First logfile */
char *oldname;
FILE *oldstream;

/* Second logfile */
char *newname;
FILE *newstream;

/* Size of buffer.  All the lines in a logfile are short, so this is plenty. */
#define LINESIZE	1000	


/* Print an error MESSAGE about FILE.  if DO_PERROR, call perror.  Exit(1) */
error(file, message, do_perror)
     char *file;
     char *message;
     int do_perror;
{
  fprintf(stderr, "gnewer:  %s %s\n", file, message);
  if (do_perror)
    perror("");
  exit(1);
}

#define check_truncation(stream, file) \
  if (feof(stream)) error((file), "is truncated.", 0);
#define check_ferror(stream, file) \
  if (ferror(stream)) error((file), "couldn't be read.", 1);
#define problem_if(test, file, message) \
  if ((test)) error((file), (message), 0);


/* Open the two files.  Read header.  Compare timestamps.  Write header. */
void
init_files()
{
  char oldbuffer[LINESIZE];
  char newbuffer[LINESIZE];

  oldstream = fopen(oldname, "r");
  problem_if(NULL==oldstream, oldname, "could not be opened.");
  
  newstream = fopen(newname, "r");
  problem_if(NULL==newstream, newname, "could not be opened.");

  /* Get the header line - contents are unspecified. */
  fgets(oldbuffer, LINESIZE, oldstream);
  fgets(newbuffer, LINESIZE, newstream);
  /* File errors checked below - they persist. */

  /* Get timestamp */
  fgets(oldbuffer, LINESIZE, oldstream);
  fgets(newbuffer, LINESIZE, newstream);

  check_truncation(oldstream, oldname);
  check_ferror(oldstream, oldname);
  check_truncation(newstream, newname);
  check_ferror(newstream, newname);
  

  /* Note that fgets retains newlines. */
  if (0 != strcmp(oldbuffer, newbuffer))
    {
      fprintf(stderr, "The two logfiles come from two different instrumentations.\n");

      fprintf(stderr, "The first comes from one begun on %s", oldbuffer);
      fprintf(stderr, "The second comes from one begun on %s", newbuffer);
      exit(1);
    }

  printf("GCT Log File (from gnewer)\n");
  printf("%s", oldbuffer);
}

/*
 * Process each pair of lines and print result to stdout.  Obvious error
 * checking is done.
 *
 * Error checking is cleaner if you read the line and then sscanf the
 * integer, so that's what I do.
 */
process()
{
  char oldbuffer[LINESIZE];
  char newbuffer[LINESIZE];

  static int warned = 0;	/* Warn once if files are inconsistent. */
  int i;			/* Number of lines read (for warning) */
  
  for(i=1;;i++)
    {
      unsigned long oldcount;
      unsigned long newcount;
      int old_scan_result;
      int new_scan_result;

      fgets(oldbuffer, LINESIZE, oldstream);
      fgets(newbuffer, LINESIZE, newstream);

      check_ferror(oldstream, oldname);
      check_ferror(newstream, newname);

      if (feof(oldstream) && feof(newstream))	/* Both files ended. */
	return;

      if (feof(oldstream))
	{
	  oldcount = 0;
	  old_scan_result = 1;
	}
      else
	{
	  old_scan_result = sscanf(oldbuffer, "%lu\n", &oldcount);
	}
      
	
      if (feof(newstream))
	{
	  newcount = 0;
	  new_scan_result = 1;
	}
      else
	{
	  new_scan_result = sscanf(newbuffer, "%lu\n", &newcount);
	}
      
      if (1 == old_scan_result && 1 == new_scan_result)
	{
	  if (oldcount > newcount && !warned)
	    {
	      warned = 1;
	      fprintf(stderr, "Warning:  The old file has a larger count than the new file for entry %d.\n", i);
	      fprintf(stderr, "The old count is %lu; the new is %lu.\n",
		      oldcount, newcount);
	      fprintf(stderr, "Further warnings will not be printed.\n");
	    }

	  /* Print as signed so that incorrect ordering has very
	     visible effects. */
	  if (!logical)
	    printf("%ld\n", newcount - oldcount);
	  else
	    printf("%ld\n", !!newcount - !!oldcount);
	}
      else
	{
	  problem_if((1 != old_scan_result), oldname, "is corrupt");
	  problem_if((1 != new_scan_result), newname, "is corrupt");
	  problem_if(1, "program error", "error checking failed.");
	}
    }
}


			      /* MAIN */


main(argc,argv)
     int argc;
     char *argv[];
{
  int files = 0;
  
  for(argv++; *argv; argv++)
    {
      if ((*argv)[0] == '-') 
      {
	if (!strcmp((*argv)+1, "logical"))
	  {
	    logical = 1;
	  }
	else
	  {
	    fprintf(stderr,"gnewer: Unknown argument %s\n",(*argv));
	    exit(1);
	  }
      }
    else
      {
	files++;
	if (1 == files)		/* Already found. */
	  {
	    oldname = *argv;
	  }
	else if (2 == files)
	  {
	    newname = *argv;
	  }
      }
    }

  if (files != 2)
    {
      fprintf(stderr, "gnewer takes 2 arguments; you gave %d.\n", files);
      exit(1);
    }
  
  init_files();
  process();
  exit(0);
}

