/*
 * Based on code Copyright (C) 1990 by Thomas Hoch and Wolfram Schafer,
 * Heavily modified for GCT by Brian Marick.
 * You may distribute under the terms of the GNU General Public License.
 * See the file named COPYING.
 */

#include "g-tools.h"
#include "config.h"
#include "gct-assert.h"
#undef index
#undef rindex


			/* DATA STRUCTURES */

/* PER CONDITION */

/*
 * The different ways a condition may be satisfied.  Using an enum is a
 * hangover from a now-deleted feature.
 */
enum sat_enum
{
  FULLY,
  NOT,
  LAST_AND_UNUSED_SAT
};
#define NUM_SAT ((int)LAST_AND_UNUSED_SAT)

typedef enum sat_enum sat;




/* PER COVERAGE TYPE */

/* Here are the different coverage types. */
enum coverage_types 
{
  BINARY_BRANCH,
  SWITCH,
  LOOP,
  MULTIPLE,
  OPERATOR,
  OPERAND,
  ROUTINE,
  CALL,
  RACE,
  OTHER,	/* Currently unused. */
  LAST_AND_UNUSED_COVERAGE_TYPE
};

#define NUM_COVERAGE_TYPES ((int)LAST_AND_UNUSED_COVERAGE_TYPE)

/* These are the names used when describing coverage types under the 
   -file or -routine option. */

char *Terse_names[] = 
{  
  "BR",	/* branch */
  "SW",	/* switch */
  "LP",	/* loop */
  "ML",	/* multi */
  "<",	/* operator */
  "x",	/* operand */
  "ROUT",	/* routine */
  "CALL",	/* call */
  "RACE",	/* race */
  "OTHER",	/* other - not currently used*/
};



/*
 * Here's what we record for each of them.
 *
 * - the total number of conditions,
 * - the number satisfied
 * - the number satisfied because the user marked them as "suppressed" in the
 *   mapfile.
 *
 * These counts do not include ignored conditions.
 */
struct record_struct
{
  int use_count;		/* Total number of conditions. */
  int satisfied[NUM_SAT];	/* Number satisfied by any type of satisfaction. */
  int suppressed[NUM_SAT];	/* Number satisfied because of suppression. */
};

typedef struct record_struct record;


  


/* COMPLETE SUMMARIES */

  
/*
 * The summary structure summarizes everything we need for output. 
 *
 * There are RECORD structures for each of the coverage types.  There is
 * a record field that holds the TOTAL for all the coverage types.  For
 * historical reasons, this is not kept up-to-date during the calculation
 * of the individual coverage type values.  Rather, it's calculated at
 * reporting time.
 *
 * The NAME field gives the name of the structure (a main_filename or routine
 * name to which these records apply).
 *
 * There are three types of entries:
 * 	A GRAND_SUMMARY holds the totals for the entire logfile.
 * 	A MARKER_SUMMARY just holds a main_filename - this is used when the
 * 		printing is done per-routine.
 * 	A DATA_SUMMARY entry holds totals for some part of the logfile
 * 		(a file or routine).
 *
 * When per-file or per-routine summaries are desired, summaries are
 * chained together through the NEXT field, rather than being printed
 * immediately.  This allows prettier output.  The NEXT field is used to
 * make a circular list where the "head" points to the
 * final element.
 */

/*
 * Note that DATA_SUMMARY is the default.  I'm not very trusting, so I'll
 * sticky_assert that summaries are initialized to the correct type.
 */
enum summary_type { DATA_SUMMARY, GRAND_SUMMARY, MARKER_SUMMARY };
  
  

struct summary_struct
{
  char *name;
  enum summary_type type;
  record records[NUM_COVERAGE_TYPES];
  record total;
  struct summary_struct *next;
};

typedef struct summary_struct summary;


/*
 * These variables contain the complete information.
 *
 * TOTAL contains the total for the entire logfile. 
 * BUILDING_TOTAL contains the total for a single routine, single file,
 * or current value of Total, depending on what breakdown the user
 * desires to see.
 * HISTORY contains a list of entries for previous routines or files, if
 * desired.  
 */

summary Total = { "TOTAL", GRAND_SUMMARY };
summary Building_total;
summary *History;


/* MEMBER FUNCTIONS FOR SUMMARY_STRUCTS */

/* Link this summary into the history list.  All fields should be
   permanently allocated and never freed. */
void
add_to_history(a_summary)
     summary *a_summary;
{
  if ((summary *)0 == History)
    {
      History = a_summary;
      a_summary->next = a_summary;
    }
  else
    {
      a_summary->next = History->next;
      History->next = a_summary;
      History = a_summary;
    }
}


/*
 * Make a copy of the given summary and add it to the history list.
 * The copy is totalled before it is added.  
 *
 * A special case: A DATA record with a zero use_count is not remembered
 * - you don't want to see that output.  There are usually many such
 * entries in a mapfile for uninstrumented files and routines.
 * GRAND_SUMMARIES (e.g., the grand total) are remembered regardless.
 *
 *
 *
 * Assumption:  The argument's NAME field is permanently allocated.
 * It is not copied when linked into the history, so it must not
 * be overwritten or freed.
 */  
static void
remember_copy(a_summary)
     summary *a_summary;
{
  summary *new;
  void calculate_total();

  calculate_total(a_summary);
  if (   0 == a_summary->total.use_count
      && DATA_SUMMARY == a_summary->type)
    return;

  new = (summary *) xmalloc(sizeof(summary));
  *new = *a_summary;
  add_to_history(new);
}


/*
 * Link a marker entry into the history
 * 	If the MAIN and INNER names are the same, the format is
 * 		main_name
 * 	If they are different, it is 
 * 		inner_name (in main_name)
 * 		(If you change this, make sure there's still enough space.)
 * The NAMEs may be freed or overwritten after this call.
 */
static void
file_marker(main_name, inner_name)
     char *main_name, *inner_name;
{
  summary *new = (summary *) xmalloc(sizeof(summary));

  if (!strcmp(main_name, inner_name))
    {
      new->name = permanent_string(main_name);
    }
  else
    {
      new->name = xmalloc(strlen(main_name)+strlen(inner_name) + 10);
      sprintf(new->name, "%s (in %s)", inner_name, main_name);
    }
  
  new->type = MARKER_SUMMARY;
  add_to_history(new);
}


/*
 * The numeric fields in TARGET are incremented by the corresponding
 * fields in ADDITIONS.  Note that the totals are not updated - they're
 * typically calculated once at the very end.  
 */
static void
update(target, additions)
     summary *target;
     summary *additions;
{
  int sat_index;
  int coverage_index;

  for(coverage_index = 0; coverage_index < NUM_COVERAGE_TYPES; coverage_index++)
    {
      target->records[coverage_index].use_count
	+= additions->records[coverage_index].use_count;

      for (sat_index = 0; sat_index < NUM_SAT; sat_index++)
	{
	  target->records[coverage_index].satisfied[sat_index] +=
	    additions->records[coverage_index].satisfied[sat_index];
	  target->records[coverage_index].suppressed[sat_index] +=
	    additions->records[coverage_index].suppressed[sat_index];
	}
    }
}

/*
 * After this call, all numeric fields are zero, the summary has no name,
 * and the summary is not a marker.  The NAME field is NOT freed
 * before the pointer is zeroed; if that is to be done, the caller must
 * do it before the call.
 */
zero_summary(a_summary)
     summary *a_summary;
{
  bzero(a_summary, sizeof(summary));
  sticky_assert (DATA_SUMMARY == a_summary->type);
}





			 /* RECORDING INFORMATION */

/* Utilities */

/*
 * Utilities for setting record entries.  Note: the caller is expected to
 * filter out IGNORED entries before these routines are called.  In
 * multiple-coverage-condition lines (like branches), only the caller
 * knows how many conditions an IGNORE affects.  Instead of having some
 * callers do the checking and some relying on utilities doing checking,
 * I'll consistently make the callers do it.
 */


/*
 * Set either the NOT or FULLY entries in the given record according to
 * the count passed in.  The use-count for that record is also incremented.
 */
void
set_noset_with_count(rec, count)
     record *rec;
     t_count count;
{
  rec->use_count++;
  if (0 == count->val && SUPPRESSED_COUNT != count->edit)
    {
      rec->satisfied[NOT]++;
    }
  else
    {
      rec->satisfied[FULLY]++;
      if (0 == count->val && SUPPRESSED_COUNT == count->edit)
	rec->suppressed[FULLY]++;
    }
}

/*
 * Set either the NOT or FULLY entries in the record REC, using the
 * count from PROBE.  The use-count for that record is also incremented.
 */
void
set_noset(probe, rec)
     struct single_probe *probe;
     record *rec;
{
  set_noset_with_count(rec, probe->count);
}



/*
 * This routine is called only if per-routine or per-file summaries are
 * desired.  If a transition was made, it records the Building_total in
 * the Total, saves the Building_total in the history list, and zeroes
 * the Building_total for the new routine/file.
 *
 * In the case of per-routine coverage, we may enter a FILE_MARKER 
 * before the transition.  They are appropriate
 * 1.  if the main_filename has changed
 * 2.  If the inner_filename has changed
 * See the inclusion tests (*-sumr.ref) for scenarios.
 *
 * On the very first call, it will appear the main_filename has changed
 * from "nothing" to the first file.  No saving is done in that case.
 * However, a file marker should be saved, if appropriate.
 *
 * Memory management, as always, is a pain.
 * 1.  We get the filenames and routinename from a probe, but we do not
 * assume pointers to that data are valid when the next probe is returned,
 * so we must make copies.
 * 2.  So comparisons to detect changes must be strcmp instead of pointer
 * equality tests.  (It is possible that the probe could use the same allocated
 * space to return a new filename.)
 * 3.  When we initialize the Building_total (with one of the routine
 * name or the main_filename) we must initialize it with memory that will
 * never be deleted - this is an obligation of remember_copy.  Rather
 * than remember which of the two can be freed, we make a separate copy.
 * 4.  Whenever the filename/routinename changes, the old saved values
 * must be freed.
 */

void
note_transition(probe, per_routine)
     struct single_probe *probe;
     int per_routine;
{
  static char *last_main_filename = (char *)0;
  static char *last_inner_filename = (char *)0;
  static char *last_routinename = (char *)0;
  int main_filename_change;
  int routinename_change;
  

  if ((char *)0 == last_main_filename)	/* First call */
    {
      last_main_filename = permanent_string(probe->main_filename);
      last_inner_filename = permanent_string(probe->inner_filename);
      last_routinename = permanent_string(probe->routinename);
      Building_total.name =
	permanent_string(per_routine ? probe->routinename : probe->main_filename);
      if (per_routine)
	{
	  file_marker(probe->main_filename, probe->inner_filename);
	}
      return;
    }

  main_filename_change = strcmp(last_main_filename, probe->main_filename);
  routinename_change = strcmp(last_routinename, probe->routinename);
  
  if (!main_filename_change && !routinename_change)
    {
      return;
    }

  /* If the filename didn't change, and all we care about is filenames */
  if (!main_filename_change && !per_routine)
    {
      return;
    }

  remember_copy(&Building_total);
  if (   per_routine
      && (   main_filename_change
	  || strcmp(last_inner_filename, probe->inner_filename)))
    {
      file_marker(probe->main_filename, probe->inner_filename);
    }

  update(&Total, &Building_total);
  zero_summary(&Building_total);

  Building_total.name =
    permanent_string(per_routine ? probe->routinename : probe->main_filename);
  
  free(last_main_filename);
  last_main_filename = permanent_string(probe->main_filename);
  free(last_inner_filename);
  last_inner_filename = permanent_string(probe->inner_filename);
  free(last_routinename);
  last_routinename = permanent_string(probe->routinename);
}



/* Per-Coverage-Type Routines. */

/*
 * All of these routines add data to Building_total.  They all ignore
 * IGNORED conditions as appropriate for their type.
 *
 * There's a separate routine for each coverage type, though many of them
 * do the same thing.
 */

void
branch_record(probe)
     struct single_probe *probe;
{
  if (   !strcmp(probe->kind, "if")
      || !strcmp(probe->kind, "?")
      || !strcmp(probe->kind, "while")
      || !strcmp(probe->kind, "do")
      || !strcmp(probe->kind, "for"))
    {
      struct single_probe *false_probe = secondary_probe();
      
      if (DO_IGNORE_2(probe->count->edit, false_probe->count->edit))
	return;
      
      set_noset(probe, &Building_total.records[BINARY_BRANCH]);
      set_noset(false_probe, &Building_total.records[BINARY_BRANCH]);
    }
  else
    {
      fprintf(stderr, "Mapfile index %d is unknown condition '%s'\n",
	      probe->index,  probe->kind);
    }
}

void
switch_record(probe)
     struct single_probe *probe;
{
  if (DO_IGNORE(probe->count->edit))
    return;
  
  set_noset(probe, &Building_total.records[SWITCH]);
}


void
multi_record(probe)
     struct single_probe *probe;
{
  struct single_probe *false_probe = secondary_probe();

  if (DO_IGNORE_2(probe->count->edit, false_probe->count->edit))
    return;
  
  set_noset(probe, &Building_total.records[MULTIPLE]);
  set_noset(false_probe, &Building_total.records[MULTIPLE]);
}


void
loop_record(probe)
     struct single_probe *probe;
{
  if (!strcmp(probe->kind, "do-loop"))
    {
      t_count once_count, twice_1_count, twice_2_count, more_than_twice_count;
      t_count partial_sum, total_sum;

      once_count = probe->count;
      twice_1_count = secondary_probe()->count;
      twice_2_count = secondary_probe()->count;
      more_than_twice_count = secondary_probe()->count;

      if (DO_IGNORE_4(once_count->edit, twice_1_count->edit, twice_2_count->edit, more_than_twice_count->edit))
	return;
  
      partial_sum = add_count(twice_1_count, twice_2_count);
      total_sum = add_count(partial_sum, more_than_twice_count);

      set_noset_with_count(&Building_total.records[LOOP], once_count);
      set_noset_with_count(&Building_total.records[LOOP], total_sum);

      free(partial_sum);
      free(total_sum);
    }
  else
    {
      t_count never_count, at_least_once_count, once_count, sum_once, many_count;
      
      never_count = probe->count;
      at_least_once_count = secondary_probe()->count;
      once_count = secondary_probe()->count;
      many_count = secondary_probe()->count;

      if (DO_IGNORE_4(never_count->edit, at_least_once_count->edit, once_count->edit, many_count->edit))
	return;

      sum_once = add_count(at_least_once_count, once_count);

      set_noset_with_count(&Building_total.records[LOOP], never_count);
      set_noset_with_count(&Building_total.records[LOOP], sum_once);
      set_noset_with_count(&Building_total.records[LOOP], many_count);

      free(sum_once);
    }
}

void
operator_record(probe)
     struct single_probe *probe;
{
  if (DO_IGNORE(probe->count->edit))
    return;
  
  set_noset(probe, &Building_total.records[OPERATOR]);
}


void
operand_record(probe)
     struct single_probe *probe;
{
  if (DO_IGNORE(probe->count->edit))
    return;
  
  set_noset(probe, &Building_total.records[OPERAND]);
}


void
routine_record(probe)
     struct single_probe *probe;
{
  if (DO_IGNORE(probe->count->edit))
    return;
  
  set_noset(probe, &Building_total.records[ROUTINE]);
}

void
call_record(probe)
     struct single_probe *probe;
{
  if (DO_IGNORE(probe->count->edit))
    return;
  
  set_noset(probe, &Building_total.records[CALL]);
}

void
race_record(probe)
     struct single_probe *probe;
{
  if (DO_IGNORE(probe->count->edit))
    return;
  
  set_noset(probe, &Building_total.records[RACE]);
}

void
other_record(probe)
     struct single_probe *probe;
{
  if (DO_IGNORE(probe->count->edit))
    return;
  
  set_noset(probe, &Building_total.records[OTHER]);
}

/*
 * This fills in the TOTAL field in the given summary, based on the
 * values already present in the RECORDS fields.   
 */

void
calculate_total(a_summary)
     summary *a_summary;
{
  int sat_index;
  int coverage_index;

  a_summary->total.use_count = 0;
  for(coverage_index = 0;
      coverage_index < NUM_COVERAGE_TYPES;
      coverage_index++)
    {
      a_summary->total.use_count
	+= a_summary->records[coverage_index].use_count;
    }
  
  for (sat_index = 0; sat_index < NUM_SAT; sat_index++)
    {
      a_summary->total.satisfied[sat_index] = 0;
      a_summary->total.suppressed[sat_index] = 0;
      for (coverage_index = 0;
	   coverage_index < NUM_COVERAGE_TYPES;
	   coverage_index ++)
	{
	  a_summary->total.satisfied[sat_index]
	    += a_summary->records[coverage_index].satisfied[sat_index];
	  a_summary->total.suppressed[sat_index]
	    += a_summary->records[coverage_index].suppressed[sat_index];
	}
    }
}

			/* DISPLAYING INFORMATION */


/* UTILITIES */

/*
 * Calculate the percentage, avoiding divide-by-zero.  If the denominator
 * is 0, the percentage is 100%.  This is useful in terse reports,
 * unneeded in long-form reports.
 */
float
percent(num, denom)
     int num, denom;
{
  if (0 == denom)
    return (float) 100.0;
  else
    return (float) (num * 100.0/denom);
}

/* LONG-FORM DISPLAYS */

/*
 * Print out the contents of the given RECORD.  
 */
generic_report(data)
     record *data;
{
  printf("%d (%5.2f%%) not satisfied.\n", data->satisfied[NOT],
	 percent(data->satisfied[NOT], data->use_count));
  printf("%d (%5.2f%%) fully satisfied.",
	 data->satisfied[FULLY],
	 percent(data->satisfied[FULLY], data->use_count));
  if (data->suppressed[FULLY] > 0)
    {
      printf(" [%d (%5.2f%%) suppressed]",
	     data->suppressed[FULLY], 
	     percent(data->suppressed[FULLY], data->use_count));
    }
  putchar('\n');
  putchar('\n');
}



/* Per-Coverage-Type Reporting */
/* All of these routines presume they are to report on Total. */


void
branch_report()
{
  if (Total.records[BINARY_BRANCH].use_count > 0)
    {
      printf("BINARY BRANCH INSTRUMENTATION (%d conditions total)\n",
	     Total.records[BINARY_BRANCH].use_count);
      generic_report(&Total.records[BINARY_BRANCH]);
    }
}

void
switch_report()
{
  if (Total.records[SWITCH].use_count > 0)
    {
      printf("SWITCH INSTRUMENTATION (%d conditions total)\n",
	     Total.records[SWITCH].use_count);
      generic_report(&Total.records[SWITCH], 0);
    }
}

void
multi_report()
{
  if (Total.records[MULTIPLE].use_count > 0)
    {
      printf("MULTIPLE CONDITION INSTRUMENTATION (%d conditions total)\n",
	     Total.records[MULTIPLE].use_count);
      generic_report(&Total.records[MULTIPLE]);
    }
}

void
loop_report()
{
  if (Total.records[LOOP].use_count > 0)
    {
      printf("LOOP INSTRUMENTATION (%d conditions total)\n",
	     Total.records[LOOP].use_count);
      generic_report(&Total.records[LOOP]);
    }
}

void
operator_report()
{
  if (Total.records[OPERATOR].use_count > 0)
    {
      printf("OPERATOR INSTRUMENTATION (%d conditions total)\n",
	     Total.records[OPERATOR].use_count);
      generic_report(&Total.records[OPERATOR], 0);
    }
}

void
operand_report()
{
  if (Total.records[OPERAND].use_count > 0)
    {
      printf("OPERAND INSTRUMENTATION (%d conditions total)\n",
	     Total.records[OPERAND].use_count);
      generic_report(&Total.records[OPERAND], 0);
    }
}

void
routine_report()
{
  if (Total.records[ROUTINE].use_count > 0)
    {
      printf("ROUTINE INSTRUMENTATION (%d conditions total)\n",
	     Total.records[ROUTINE].use_count);
      generic_report(&Total.records[ROUTINE], 0);
    }
}

void
call_report()
{
  if (Total.records[CALL].use_count > 0)
    {
      printf("CALL INSTRUMENTATION (%d conditions total)\n",
	     Total.records[CALL].use_count);
      generic_report(&Total.records[CALL], 0);
    }
}

void
race_report()
{
  if (Total.records[RACE].use_count > 0)
    {
      printf("RACE INSTRUMENTATION (%d conditions total)\n",
	     Total.records[RACE].use_count);
      generic_report(&Total.records[RACE], 0);
    }
}

void
other_report()
{
  if (Total.records[OTHER].use_count > 0)
    {
      printf("OTHER INSTRUMENTATION (%d conditions total)\n",
	     Total.records[OTHER].use_count);
      generic_report(&Total.records[OTHER], 0);
    }
}

/* This prints a summary of all the information collected. */
void
summary_report()
{
  calculate_total(&Total);
  printf("SUMMARY OF ALL CONDITION TYPES (%d total)\n", Total.total.use_count);
  generic_report(&Total.total);
}

void
long_report_all()
{
  branch_report();
  switch_report();
  loop_report();
  multi_report();
  operator_report();
  operand_report();
  routine_report();
  call_report();
  race_report();
  other_report();
  summary_report();
}



/* TERSE DISPLAYS */

/*
 * Function to avoid multiple evaluations, with net effect of more
 * function calls...
 */
int max(a, b)
     int a, b;
{
  return (a > b ? a : b);
}

int min(a, b)
     int a, b;
{
  return (a < b ? a : b);
}


/*
 * Given an integer I, return the amount of space needed for its printed
 * (base 10) representation.  Number should never be less than CURRENT_MAXIMUM.
 */
max_numeric_field_width(i, current_maximum)
     int i;
     int current_maximum;
{
  int width = 0;

  do
    {
      width++;
      i /= 10;
    }
  while (i > 0);

  return (width>current_maximum ? width : current_maximum);
}

/*
 * Print a terse report about a single summary, A_SUMMARY, which may be
 * for a routine, a file, or a grand total.  The terse report is a single
 * line, giving the name, the percent for each printed coverage type, the
 * percent for all coverage types, and the total number of conditions.
 *
 * Coverage types are omitted from the printout if there were no coverage
 * conditions for that type (typically because instrumentation was not
 * turned on for that type).  To know this, the routine must be passed
 * the grand summary in TOTAL (which may be pointer-identical to
 * A_SUMMARY).
 *
 * The caller must have previously calculated the width of the name
 * column and the condition count column.  These are passed in in
 * NAME_WIDTH and COUNT_WIDTH.  Being percentages, the other columns have
 * a fixed width.
 *
 * This routine should not be called for a marker summary.
 */
terse_report_one(a_summary, total, name_width, count_width)
     summary *a_summary;
     summary *total;
     int name_width, count_width;
{
  int index;

  sticky_assert(MARKER_SUMMARY != a_summary->type);
  
  printf("%-*s ", name_width, a_summary->name);
  printf("%3.0f=ALL ",
	 percent(a_summary->total.satisfied[FULLY],
		 a_summary->total.use_count));
  for(index = 0; index < NUM_COVERAGE_TYPES; index++)
    {
      if (total->records[index].use_count > 0)
	printf("%3.0f=%s ",
	       percent(a_summary->records[index].satisfied[FULLY],
		       a_summary->records[index].use_count),
	       Terse_names[index]);
    }
  printf("%*d#\n", count_width, a_summary->total.use_count);
}

/*
 * This routine prints a summary of an entire history list.
 *
 * The TOTAL is added to the history list (making at least one entry),
 * then every element in the list is printed.  Maximum field widths are
 * calculated before printing.
 */

terse_report_all()
{
  summary *stop;	/* Last element to print */
  int count_width = 0, name_width = 0;
  
  remember_copy(&Total);
  stop = History;

  /* Calculate maximum widths of fields. */
  do
    {
      History = History->next;
      if (MARKER_SUMMARY != History->type)
	{
	  count_width = max_numeric_field_width(History->total.use_count,
						count_width);
	}
      name_width = max(strlen(History->name), name_width);
    }
  while (stop != History);

  /* But don't allow very long lines to dominate. */
  name_width = min(name_width, 31);

  /*
   * Print each file, provided it has any instrumentation.
   * (Terse_print_one handles the case where a particular type of
   * coverage is never used.)  Skip file markers immediately followed by
   * other file markers.
   */
  do
    {
      History = History->next;
      if (MARKER_SUMMARY == History->type)
	{
	  /* Note that the Total field is not a MARKER_SUMMARY, which
	     handles the boundary condition. */
	  if (DATA_SUMMARY == History->next->type)
	    printf("%s\n", History->name);
	}
      else 
	{
	  terse_report_one(History, &Total, name_width, count_width);
	}
    }
  while (History != stop);
}


			      /* MAIN */


main(argc,argv)
     int argc;
     char *argv[];
{
  int per_routine = 0;
  int per_file = 0;
  
  
  for(argv++; *argv; argv++)
    {
      if ((*argv)[0] == '-') 
      {
	if (!strcmp((*argv)+1, "test-map"))
	  {
	    argv++;
	    if (NULL == *argv)
	      {
		fprintf(stderr, "gsummary: -test-map requires an argument.\n");
		exit(1);
	      }
	    Gct_test_map = *argv;
	  }
	else if (!strcmp((*argv)+1, "test-dir"))
	  {
	    argv++;
	    if (NULL == *argv)
	      {
		fprintf(stderr, "gsummary: -test-dir requires an argument.\n");
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
		fprintf(stderr, "gsummary: %s requires an argument.\n",
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
		fprintf(stderr, "gsummary: %s requires an argument.\n",
			*(argv-1));
		exit(1);
	      }
	    {
	      set_default_file_external_edit(IGNORED_COUNT);
	      add_routine_external_edit(*argv, VISIBLE_COUNT);
	    }
	  }
	else if (   !strcmp((*argv)+1, "files")
		 || !strcmp((*argv)+1, "f"))
	  {
	    per_file = 1;
	  }
	else if (   !strcmp((*argv)+1, "routines")
		 || !strcmp((*argv)+1, "r"))
	  {
	    per_routine = 1;
	  }
	else
	  {
	    fprintf(stderr,"gsummary: Unknown argument %s\n",(*argv));
	    exit(1);
	  }
      }
    else
      {
	if (Gct_input)		/* Already found. */
	  {
	    fprintf(stderr,"gsummary: gsummary takes only one file as argument.\n");
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

  sticky_assert(GRAND_SUMMARY == Total.type);
  sticky_assert(DATA_SUMMARY == Building_total.type);
  
  for(;;)
    {
      struct single_probe *probe;
      void (*fn)();

      probe = get_probe();
      if ((struct single_probe *)0 == probe)	/* EOF on mapfile */
	{
	  assert_logstream_empty();
	  update(&Total, &Building_total);
	  sticky_assert(GRAND_SUMMARY == Total.type);
	  sticky_assert(DATA_SUMMARY == Building_total.type);
  
	  if (per_file || per_routine)
	    {
	      remember_copy(&Building_total);
	      terse_report_all();
	    }
	  else
	    long_report_all();
	  exit(0);
	}

      if (per_routine || per_file)
	{
	  note_transition(probe, per_routine);
	}
	  

      if (   !strcmp(probe->kind, "loop")
	       || !strcmp(probe->kind, "do-loop"))
	{
	  fn = loop_record;
	}
      else if (   !strcmp(probe->kind, "condition"))
	{
	  fn = multi_record;
	}
      else if (   !strcmp(probe->kind, "operator"))
	{
	  fn = operator_record;
	}
      else if (!strcmp(probe->kind, "operand"))
	{
	  fn = operand_record;
	}
      else if (   !strcmp(probe->kind, "routine"))
	{
	  fn = routine_record;
	}
      else if (   !strcmp(probe->kind, "call"))
	{
	  fn = call_record;
	}
      else if (   !strcmp(probe->kind, "race"))
	{
	  fn = race_record;
	}
      else if (   !strcmp(probe->kind, "other"))
	{
	  fn = other_record;
	}
      else if (   !strcmp(probe->kind, "case")
	       || !strcmp(probe->kind, "default"))
	{
	  fn = switch_record;
	}
      else
	{
	  fn = branch_record;
	}
      (*fn)(probe);
    }
}
