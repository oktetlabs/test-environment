/*
 * Based on code Copyright (C) 1990 by Thomas Hoch and Wolfram Schafer,
 * Heavily modified for GCT by Brian Marick.
 * You may distribute under the terms of the GNU General Public License.
 * See the file named COPYING.
 */


#include "config.h"
#include "g-tools.h"

#ifdef USG
extern int getwd();	/* Defined in g-tools.c */
#endif

/* Saved value of the current working directory. */
char org_dir[PATH_BUF_LEN];


/* Control flags. */

/* If true, show filenames absolute, not relative to master directory. */
int absolute_names =0;

/* If true, show all non-ignored logfile entries, not just non-zero ones. */
int show_all = 0;

/* If true, ignored entries are not ignored. */
int must_show_ignored = 0;

/* If true, add [index #...] field to output for user to edit. */
int show_edit = 0;

/*
 * If true, show zero, non-suppressed counts.  If false, show only
 * satisfied counts (irrespective of suppression).
 */
int show_work_to_do = 1;

/* This macro implements the rules above. */

#define OF_INTEREST(count)	\
	( show_work_to_do ?	\
		(0 == (count)->val && SUPPRESSED_COUNT != (count->edit)) : \
	        ((count)->val > 0))


			   /* FILENAMES */

char *
full_filename(file_name)
     char *file_name;
{
  static char long_file_name[2*PATH_BUF_LEN];
  static char pathname[PATH_BUF_LEN], old_pathname[PATH_BUF_LEN];
  char *basename;
  static char new_dir[PATH_BUF_LEN];
  
  
  /* The absolute pathnames are evaluated by switching into the 
     directory specified in the control file and getting the directory
     name using the getwd command. This is done because it might be
     possible that symbolic links exist */
  
  if ((absolute_names) && (file_name[0] != '/'))
    {
      basename = strrchr(file_name, '/');
      if ((char *)0 != basename)
	{
	  basename++; /* over slash */
	  strncpy(pathname,file_name,basename-file_name);
	  pathname[basename-file_name] = '\0';
	  
	  if (strcmp(pathname,old_pathname)) 	/* if change */
	    {
	      
	      strcpy(old_pathname,pathname); 
	      if(chdir(pathname)==0) {
		if (!getwd(new_dir))
		  {
		    fprintf(stderr,"greport: getwd error: %s\n",new_dir);
		    exit(1);
		  }
		else
		  {
		    if (strlen(org_dir) == PATH_BUF_LEN -1)
		      {
			fprintf(stderr,"greport: Directory too long %s\n",
				new_dir);
			exit(1);
		      }
		    strcat(new_dir,"/");
		  }
	      }
	      else
		{
		  fprintf(stderr,"greport: chdir error: %s\n",pathname);
		  exit(1);
		}
	      if (chdir(org_dir)==-1)
		{
		  fprintf(stderr,"greport: chdir error: %s\n",org_dir);
		  exit(1);
		}
	    }
	  strcpy(long_file_name,new_dir);
	  strcat(long_file_name,basename);
	}
      else
	{
	  strcpy(long_file_name,org_dir);
	  strcat(long_file_name,file_name);
	}
    }
  else
    {
      strcpy(long_file_name,file_name);
    }
  return long_file_name;
}


		       /* STANDARD TEXT CRUD */

void
emit_line_id(probe)
     struct single_probe *probe;
{
  printf("\"%s\", line %d: ", probe->inner_filename, probe->lineno);
}


	     /* THE DIFFERENT KINDS OF INSTRUMENTATION */


void
branchish_map(probe)
     struct single_probe *probe;
{
  if (   !strcmp(probe->kind,"case")
      || !strcmp(probe->kind,"default"))
    {
      if (IGNORED_COUNT == probe->count->edit && !must_show_ignored)
	return;
      
      if (OF_INTEREST(probe->count) || show_all)
	{
	  emit_line_id(probe);
	  if (show_edit)
	    {
	      printf("[%d: %s] ",
		     probe->index, printable_count(probe->line_count, 1));
	    }
	  
	  printf("%s %s", probe->kind, probe->rest_text);
	  printf("was taken %s times.\n", printable_count(probe->count, 0));
	}
    }
  else if (   !strcmp(probe->kind, "if")
	   || !strcmp(probe->kind, "?")
	   || !strcmp(probe->kind, "do")
	   || !strcmp(probe->kind, "while")
	   || !strcmp(probe->kind, "for")
	   || !strcmp(probe->kind, "condition"))
    {
      t_count true_count = probe->count;
      struct single_probe *false_probe = secondary_probe();
      t_count false_count = false_probe->count;

      if (   DO_IGNORE_2(true_count->edit, false_count->edit)
	  && !must_show_ignored)
	{
	  return;
	}
      
      if (OF_INTEREST(true_count) || OF_INTEREST(false_count) || show_all)
	{
	  emit_line_id(probe);
	  if (show_edit)
	    {
	      printf("[%d: %s %s] ",
		     probe->index,
		     printable_count(probe->line_count, 1),
		     printable_count(false_probe->line_count, 1));
	    }
	  printf("%s %s", probe->kind, probe->rest_text);
	  printf("was taken TRUE %s, FALSE %s times.\n",
		 printable_count(true_count, 0),
		 printable_count(false_count, 0));
	}
    }
  else
    {
      fprintf(stderr, "Mapfile index %d is unknown '%s'\n", probe->index, probe->kind);
    }
}

void
loop_map(probe)
     struct single_probe *probe;
{
  t_count not_taken, at_least_once, exactly_once, sum_once, at_least_twice;
  struct single_probe *at_least_once_probe,
  			 *exactly_once_probe,
  			 *at_least_twice_probe;
  
  
  not_taken = probe->count;
  at_least_once_probe = secondary_probe();
  at_least_once = at_least_once_probe->count;
  exactly_once_probe = secondary_probe();
  exactly_once = exactly_once_probe->count;
  at_least_twice_probe = secondary_probe();
  at_least_twice = at_least_twice_probe->count;

  if (   !must_show_ignored
      && DO_IGNORE_4(not_taken->edit, at_least_once->edit, exactly_once->edit, at_least_twice->edit))
  {
    return;
  }
  
  sum_once = add_count(at_least_once, exactly_once);

  if (   OF_INTEREST(not_taken)
      || OF_INTEREST(sum_once)
      || OF_INTEREST(at_least_twice)
      || show_all)
    {
      emit_line_id(probe);
      if (!strcmp(probe->kind, "do-loop"))
	{
	  /* For "do" loop, not_taken means number of times body is
	     traversed and then immediately left - that is, taken once. */
	  t_count many  = add_count(sum_once, at_least_twice);
	  if (show_edit)
	    {
	      /* Must redo above calculations, but using line edits. */
	      t_count line_sum_once =
		add_count(at_least_once_probe->line_count,
			  exactly_once_probe->line_count);
	      t_count line_many =
		add_count(line_sum_once,
			  at_least_twice_probe->line_count);
	      
	      printf("[%d: %s %s] ", probe->index,
		     printable_count(probe->line_count, 1),
		     printable_count(line_many, 1));
	      free(line_sum_once);
	      free(line_many);
	    }
	  printf("loop %s", probe->rest_text);
	  printf("one time: %s, many times: %s.\n",
		 printable_count(not_taken, 0),  printable_count(many, 0));
	  free(many);
	}
      else
	{
	  if (show_edit)
	    {
	      /* Must redo calculations, but using line edits. */
	      t_count line_sum_once =
		add_count(at_least_once_probe->line_count,
			  exactly_once_probe->line_count);
	      printf("[%d: %s %s %s] ", probe->index,
		     printable_count(probe->line_count, 1),
		     printable_count(line_sum_once, 1),
		     printable_count(at_least_twice_probe->line_count, 1));
	      free(line_sum_once);
	    }
	  printf("loop %s", probe->rest_text);
	  printf("zero times: %s, one time: %s, many times: %s.\n",
		 printable_count(not_taken, 0),
		 printable_count(sum_once, 0),
		 printable_count(at_least_twice, 0));
	}
    }
  free(sum_once);
}

void
hide_if_set_map(probe)
     struct single_probe * probe;
{
  if (DO_IGNORE(probe->count->edit) && !must_show_ignored)
    return;
  
  if (OF_INTEREST(probe->count) || show_all)
    {
      emit_line_id(probe);
      if (show_edit)
	{
	  printf("[%d: %s] ",
		 probe->index, printable_count(probe->line_count, 1));
	}
      
      printf("%s %s", probe->kind, probe->rest_text);
      if (show_all || !show_work_to_do)
	printf(" [%s]", printable_count(probe->count, 0));
      putchar('\n');
    }
}



			      /* MAIN */


main(argc,argv)
     int argc;
     char *argv[];
{
  for(argv++; *argv; argv++)
    {
      if ((*argv)[0] == '-') 
      {
	if (!strcmp((*argv)+1, "l"))
	  {
	    absolute_names = 1;
	    if (!getwd(org_dir))
	      {
		fprintf(stderr,"greport: getwd error: %s\n",org_dir);
		exit(1);
	      }
	    else
	      {
		if (strlen(org_dir) == PATH_BUF_LEN -1)
		  {
		    fprintf(stderr,"greport: Directory too long %s\n",org_dir);
		    exit(1);
		  }
		strcat(org_dir,"/");
	      }
	  } 
	else if (!strcmp((*argv)+1, "test-map"))
	  {
	    argv++;
	    if (NULL == *argv)
	      {
		fprintf(stderr, "greport: -test-map requires an argument.\n");
		exit(1);
	      }
	    Gct_test_map = *argv;
	  }
	else if (!strcmp((*argv)+1, "test-dir"))
	  {
	    argv++;
	    if (NULL == *argv)
	      {
		fprintf(stderr, "greport: -test-dir requires an argument.\n");
		exit(1);
	      }
	    Gct_test_dir = (*argv);
	  }
	else if (   !strcmp((*argv)+1, "visible-file")
		 || !strcmp((*argv)+1, "vf"))
	  {
	    argv++;
	    if (NULL == *argv)
	      {
		fprintf(stderr, "greport: %s requires an argument.\n",
			*(argv-1));
		exit(1);
	      }
	    {
	      set_default_file_external_edit(IGNORED_COUNT);
	      add_file_external_edit(*argv, VISIBLE_COUNT);
	    }
	  }
	else if (   !strcmp((*argv)+1, "visible-routine")
		 || !strcmp((*argv)+1, "vr"))
	  {
	    argv++;
	    if (NULL == *argv)
	      {
		fprintf(stderr, "greport: %s requires an argument.\n",
			*(argv-1));
		exit(1);
	      }
	    {
	      set_default_routine_external_edit(IGNORED_COUNT);
	      add_routine_external_edit(*argv, VISIBLE_COUNT);
	    }
	  }
	else if (!strcmp((*argv)+1, "all"))
	  {
	    show_all = 1;
	  }
	else if (!strcmp((*argv)+1, "n"))
	  {
	    show_work_to_do = 0;
	  }
	else if (!strcmp((*argv)+1, "show-ignored"))
	  {
	    must_show_ignored = 1;
	  }
	else if (!strcmp((*argv)+1, "edit"))
	  {
	    show_edit = 1;
	  }
	else
	  {
	    fprintf(stderr,"greport: Unknown argument %s\n",(*argv));
	    exit(1);
	  }
      }
    else
      {
	if (Gct_input)		/* Already found. */
	  {
	    fprintf(stderr,"greport: greport takes only one file as argument.\n");
	    exit(1);
	  }
	else
	  {
	    Gct_input = (*argv);
	  }
      }
    }
  init_mapstream("r", 0);
  init_other_stream(1);
  check_timestamps();

  for(;;)
    {
      struct single_probe *probe;

      probe = get_probe();
      if ((struct single_probe *)0 == probe)
	{
	  assert_logstream_empty();
	  exit(0);
	}
      else if (   !strcmp(probe->kind, "loop")
	       || !strcmp(probe->kind, "do-loop"))
	{
	  loop_map(probe);
	}
      else if (   !strcmp(probe->kind, "case")
	       || !strcmp(probe->kind, "default")
	       || !strcmp(probe->kind, "if")
	       || !strcmp(probe->kind, "?")
	       || !strcmp(probe->kind, "do")
	       || !strcmp(probe->kind, "while")
	       || !strcmp(probe->kind, "for")
	       || !strcmp(probe->kind, "condition"))
	{
	  branchish_map(probe);
	}
      else
	{
	  hide_if_set_map(probe);
	}
    }
}

