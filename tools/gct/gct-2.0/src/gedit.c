/*
 * Based on code Copyright (C) 1990 by Thomas Hoch and Wolfram Schafer,
 * Heavily modified for GCT by Brian Marick.
 * You may distribute under the terms of the GNU General Public License.
 * See the file named COPYING.
 */

#include "g-tools.h"
#include "gct-assert.h"

/* These are the values set by get_report_line() - used everywhere. */

char Sourcefile[PATH_BUF_LEN];	/* Filename  - unused */
int Lineno;			/* Line number - unused. */
char Edit_complete[100];		/* [12: 0S] portion:  consumed in stages. */
char *Edit_pointer;		/* This points to each index in Edit_complete. */
char Probe_kind[100];		/* Type of instrumentation - used */

int Verbose = 0;		/* -v option */


			   /* UTILITIES */


#define malformed_if(value)	\
{				\
  if (value)			\
    {				\
      fprintf(stderr, "gedit:  Malformed edit text '%s'.\n", Edit_complete);\
      exit(1);			\
    }				\
}

#define program_error_if(value)	\
{				\
  if (value)			\
    {				\
      fprintf(stderr, "Program error:  Edit text '%s' found corrupt AFTER checking.\n", Edit_complete);\
      exit(1);			\
    }				\
}

      

/* Copy the existing file into a backup file. */

void
backup(name)
{
  char system_buf[100 + 2 * PATH_BUF_LEN];	/* Long enough for both names. */

  sprintf(system_buf, "/bin/cp %s %s.gbk", name, name);
  if (0 != system(system_buf))
    {
      fprintf(stderr, "Could not back up %s\n", name);
      perror("Problem: ");
      fprintf(stderr, "Original file is unchanged.\n");
      exit(1);
    }
  else if (Verbose)
    {
      fprintf(stderr, "Backup file is %s.gbk\n", name);
    }
}


/*
 * Validated:  Head of Edit_pointer matches " [%d: "
 * The value of the embedded integer is returned.
 * Edit_pointer is set pointing to the end of the above string.
 */
int
get_report_index()
{
  int retval;
  int scan_count;

  scan_count = sscanf(Edit_pointer, " [%d:", &retval);
  malformed_if (1 != scan_count);
  for(Edit_pointer+=2; *Edit_pointer != ':'; Edit_pointer++)
    {
      malformed_if('\0' == *Edit_pointer);
    }
  Edit_pointer++;
  return retval;
}

/*
 * This routine must be called after get_report_index or itself.  It
 * returns a t_count for the next count in the input stream.  (The actual
 * value is irrelevant, so 0 is used.)  's' and 'S' denote suppression;
 * 'i' and 'I' denote ignoring; 'v' and 'V' denote forced visibility, and
 * numerals denote no editing.
 *
 * If, instead of a count, the closing bracket is found, NULL is returned.
 * It is an error for EOF or any other character but bracket to terminate the
 * count.
 *
 * Caller must free the non-null return value.
 */
t_count
get_report_count()
{
  char raw_count[1000];
  static int just_returned_final_count = 0;
  int number_read;

  if (just_returned_final_count)
    {
      just_returned_final_count = 0;	/* Ready for next line */
      return NULL;
    }
  
  number_read = sscanf(Edit_pointer," %[0123456789sSiIvV]", raw_count);
  malformed_if(1 != number_read);

  /* Skip to next count. */
  for(Edit_pointer+=2;
      *Edit_pointer != ' ' && *Edit_pointer != ']';
      Edit_pointer++)
    {
      program_error_if('\0' == *Edit_pointer);
    }
  just_returned_final_count = (']' == *Edit_pointer);
  
  switch(raw_count[strlen(raw_count)-1])
    {
    case 's':
    case 'S':
      return build_count(0, SUPPRESSED_COUNT);
      break;
    case 'v':
    case 'V':
      return build_count(0, VISIBLE_COUNT);
      break;
    case 'i':
    case 'I':
      return build_count(0, IGNORED_COUNT);
      break;
    default:
      /* Checking for numerals occurs above. */
      return build_count(0, DONT_CARE_COUNT);
      break;
    }
}


			      /* MAIN */

main(argc,argv)
     int argc;
     char *argv[];
{
  int last_index = -1;	/* Last index read from mapfile. */
  int index = -1;	/* This index. */

  for(argv++; *argv; argv++)
    {
      if ((*argv)[0] == '-') 
      {
	if (!strcmp((*argv)+1, "test-map"))
	  {
	    argv++;
	    if (NULL == *argv)
	      {
		fprintf(stderr, "gedit: -test-map requires an argument.\n");
		exit(1);
	      }
	    Gct_test_map = *argv;
	  }
	else if (!strcmp((*argv)+1, "test-dir"))
	  {
	    argv++;
	    if (NULL == *argv)
	      {
		fprintf(stderr, "gedit: -test-dir requires an argument.\n");
		exit(1);
	      }
	    Gct_test_dir = (*argv);
	  }
	else if (!strcmp((*argv)+1, "v"))
	  {
	    Verbose=1;
	  }
	else
	  {
	    fprintf(stderr,"gedit: Unknown argument %s\n",(*argv));
	    exit(1);
	  }
      }
    else
      {
	if (Gct_input)		/* Already found. */
	  {
	    fprintf(stderr,"gedit: gedit takes only one file as argument.\n");
	    exit(1);
	  }
	else
	  {
	    Gct_input = (*argv);
	  }
      }
    }

  init_mapstream("r+", 1);
  init_other_stream(0);
  backup(gct_expand_filename(Gct_test_map, Gct_test_dir));

  for(;;)
    {
      get_report_line(Sourcefile, &Lineno, Edit_complete, Probe_kind);
      if (Verbose)
	{
	  fprintf(stderr, "\"%s\", line %d:%s%s\n",
		  Sourcefile, Lineno, Edit_complete, Probe_kind);
	}
      
      Edit_pointer = Edit_complete;
      last_index = index;
      index = get_report_index();
      if (last_index > index)
	{
	  fprintf(stderr, "The following entry has an index smaller than a previous entry.\n");
	  fprintf(stderr, "\"%s\", line %d:%s%s\n",
		  Sourcefile, Lineno, Edit_complete, Probe_kind);
	  fprintf(stderr, "gedit input must be in the same order as greport output.\n");
	  fprintf(stderr, "Exiting.  Edits before this line have been applied to the mapfile.\n");
	  exit(1);
	}
      
      
      if (!strcmp(Probe_kind, "loop"))
	{
	  /* There will be either two entries (for a do loop), or
	     three.  Fetch them all first. */
	  t_count first, second, third;

	  first = get_report_count();	  malformed_if(NULL == first);
	  second = get_report_count();	  malformed_if(NULL == second);
	  third = get_report_count();
	  if (NULL != third)	/* Ordinary loop */
	    {
	      malformed_if(NULL != get_report_count());
	      
	      if (DONT_CARE_COUNT != first->edit)
		{
		  numbered_mapfile_entry(index);
		  mark_suppressed(first->edit);
		}
	      /* For an ordinary loop, set both of the "one time" lines. */
	      if (DONT_CARE_COUNT != second->edit)
		{
		  numbered_mapfile_entry(index+1);
		  mark_suppressed(second->edit);
		  numbered_mapfile_entry(index+2);
		  mark_suppressed(second->edit);
		}
	      if (DONT_CARE_COUNT != third->edit)
		{
		  numbered_mapfile_entry(index+3);
		  mark_suppressed(third->edit);
		}
	      free(first); free(second); free(third);
	    }
	  else			/* DO loop */
	    {
	      if (DONT_CARE_COUNT != first->edit)
		{
		  numbered_mapfile_entry(index);
		  mark_suppressed(first->edit);
		}
	      /* For do-loops, the second entry applies to the last three
		 mapfile entries.  Set all */
	      if (DONT_CARE_COUNT != second->edit)
		{
		  numbered_mapfile_entry(index+1);
		  mark_suppressed(second->edit);
		  numbered_mapfile_entry(index+2);
		  mark_suppressed(second->edit);
		  numbered_mapfile_entry(index+3);
		  mark_suppressed(second->edit);
		}
	      free(first); free(second);
	    }
	}
      else	/* No special cases - just suppress each one. */
	{
	  t_count count;
	  
	  while (NULL != (count = get_report_count()))
	    {
	      if (DONT_CARE_COUNT != count->edit)
		{
		  numbered_mapfile_entry(index);
		  mark_suppressed(count->edit);
		}
	      index++;
	      free(count);
	    }
	}
      
      skip_report_rest();
    }
  /* Never reached - get_report_line exits. */
}

