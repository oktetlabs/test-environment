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
 * The two files that do the actual instrumentation are gct-trans.c and
 * gct-utrans.c.  This file contains front-end code common to both of
 * them:  selecting which is to be used, initializing instrumentation,
 * finishing instrumentation, and the initial work done for a function.
 */


/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-trans.c,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */

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



			     /* States */
/* See gct-tutil.h for an explanation. */

/* The default state does nothing. */
i_state Default_state;
/* Used in gct-tutil.h macros; has to be defined somewhere. */
i_suff *__TEMP_Suff;


			     /* Tables */
void (**Instrument)();
int (**Expr_instrument)();
int (**Lvalue_instrument)();


/* Initialize the tables.  Also routines for selecting sets of tables. */

void (*(Weak_instrument[NUM_GCT_TREE_CODES+1]))() =
{
#define DEF_GCT_WEAK_TREE(token, string, ifunc, expfunc, lvfunc) ifunc,
#include "gct-tree.def"
#undef DEF_GCT_WEAK_TREE
  0  /* something to follow the final comma */
  }
;


int (*(Weak_expr_instrument[NUM_GCT_TREE_CODES+1]))() =
{
#define DEF_GCT_WEAK_TREE(token, string, ifunc, expfunc, lvfunc) expfunc,
#include "gct-tree.def"
#undef DEF_GCT_WEAK_TREE
  0  /* something to follow the final comma */
  }
;


int (*(Weak_lvalue_instrument[NUM_GCT_TREE_CODES+1]))() =
{
#define DEF_GCT_WEAK_TREE(token, string, ifunc, expfunc, lvfunc) lvfunc,
#include "gct-tree.def"
#undef DEF_GCT_WEAK_TREE
  0  /* something to follow the final comma */
  }
;



void (*(Standard_instrument[NUM_GCT_TREE_CODES+1]))() =
{
#define DEF_GCT_WEAK_TREE(token, string, ifunc, expfunc, lvfunc) 
#define GCT_WANT_STD_DEFNS
#define DEF_GCT_STD_TREE(token, ifunc)	ifunc,
#include "gct-tree.def"
#undef DEF_GCT_STD_TREE
#undef DEF_GCT_WEAK_TREE
#undef GCT_WANT_STD_DEFNS
  0  /* something to follow the final comma */
  }
;


/*
 * The race routines act differently depending on whether the main
 * declarations in the function have been processed.  They are told about
 * that via this variable.  It would be better to pass that information
 * down in the state structure, but that's not passed to these routines.
 *
 * The different actions could be eliminated if a function were
 * considered to begin with its first declaration instead of its first
 * statement.  There's not really a good reason for doing it this way,
 * but it's already done and it doesn't make any effective difference
 * in what the user sees.  The variable comes in handy in other ways, too.
 */
int In_function_body = 0;


/*
 * This routine selects one of the groups of tables.  Standard testing is
 * the default.  Weak mutation testing can be selected with the
 * force_weak option (used because some of the tests assume descent via
 * this testing path even if no coverage is turned on -- this also is
 * useful if you just want to do lint-like checks, like for "x != y;"
 */
gct_select_instrumentation_set(instrumentation_use)
	int instrumentation_use;
{
    /* We didn't inadvertently treat an option as instrumentation. */
    assert(0 == (~OPT_VALID_NEED_BITS  & instrumentation_use));
    assert(OPT_NEED_CONFLICT != (OPT_VALID_NEED_BITS & instrumentation_use));

    if (OPT_NEED_WEAK == instrumentation_use)
    {
	Instrument = Weak_instrument;
	Expr_instrument = Weak_expr_instrument;
	Lvalue_instrument = Weak_lvalue_instrument;
    }
    else
    {
	Instrument = Standard_instrument;
	Expr_instrument = 0;
	Lvalue_instrument = 0;
    }

    /* If requested, check that desired instrumentation set was gotten. */
    if (   ON == gct_option_value(OPT_CHECK_WEAK)
	&& Instrument == Standard_instrument)
    {
      error("Weak mutation coverage routines are not being used.");
    }
    else if (   ON == gct_option_value(OPT_CHECK_STANDARD)
	     && Instrument == Weak_instrument)
    {
      error("Standard coverage routines are not being used.");
    }
    /* If errors happened, we go ahead and instrument anyway.  No harm done. */
}




		      /* Starting and Ending */

/* These are read from gct-ps-defs.h.  They are used to communicate between
   successive runs of GCT. */

int Gct_cumulative_index = 0;	/* How big Gct_table should be. */
int Gct_num_files = 0;		/* How many files have been read. */

/*
 * This routine initializes the instrumentation state, which was saved in
 * files from the last run of GCT.
 *
 * See file STATE for a description of the various files maintained by GCT.
 */
init_instrumentation()
{
  extern char *Gct_per_session, *Gct_full_per_session_file;
  FILE *stream;
  
  stream = fopen(Gct_full_per_session_file, "r");
  if (NULL == stream)
    {
      fatal("Couldn't open per-session file %s.", Gct_per_session);
    }
  if (1 != fscanf(stream, PER_SESSION_INDEX_FORMAT, &Gct_cumulative_index))
    {
      fatal("Couldn't read condition count from per-session file %s.",
	    Gct_per_session);
    }
  if (1 != fscanf(stream, PER_SESSION_RACE_FORMAT, &Gct_cumulative_race_groups))
    {
      fatal("Couldn't read race group count from per-session file %s.",
	    Gct_per_session);
    }
  if (1 != fscanf(stream, PER_SESSION_FILES_FORMAT, &Gct_num_files))
    {
      fatal("Couldn't read file count from per-session file %s.",
	    Gct_per_session);
    }
  fclose(stream);
}


/*
 * Write out the information needed by the next invocation of gct-cc1.
 * This information is used to initialize the instrumentation state next
 * time.
 *
 * See file STATE for a description of the various files maintained by GCT.
 */
finish_instrumentation()
{
  FILE *stream;
  extern char *Gct_per_session, *Gct_full_per_session_file;
  extern char *Gct_per_session_definitions, *Gct_full_per_session_definitions_file;
  extern char *main_input_filename;
  
  stream = fopen(Gct_full_per_session_file, "w");
  if (NULL == stream)
    {
      fatal("Couldn't open per-session file %s.", Gct_per_session);
    }
  fprintf(stream, PER_SESSION_INDEX_FORMAT,
	  Gct_cumulative_index + Gct_next_index);
  fprintf(stream, PER_SESSION_RACE_FORMAT,
	  Gct_cumulative_race_groups + Gct_next_race_group);
  fprintf(stream, PER_SESSION_FILES_FORMAT, Gct_num_files+1);
  fclose(stream);

  /* Record this file. */
  set_mapfile_name(main_input_filename);
  
  stream = fopen(Gct_full_per_session_definitions_file, "a");
  if (NULL == stream)
    {
      fatal("Couldn't open per-session file %s.", Gct_per_session_definitions);
    }
  fprintf(stream, DEFINITION_FILE_DATA_FORMAT,
	  mapfile_name_to_print(), Gct_num_files,
	  Gct_next_index, Gct_cumulative_index,
	  Gct_next_race_group, Gct_cumulative_race_groups);
  fprintf(stream, DEFINITION_FILE_LOCALDEF,
	  Gct_num_files, Gct_cumulative_index);
  fprintf(stream, DEFINITION_FILE_RACEDEF,
	  Gct_num_files, Gct_cumulative_race_groups);
  fclose(stream);

  finish_mapfile(Gct_next_index);
}



/*
 * This is the main instrumentation routine.  It is called once per
 * function definition and is handed the compound statement that is the
 * body of the function.
 *
 * This is also a convenient place to put per-function debugging
 * routines.
 *
 * There are ugly special cases in this code:
 * 1.  Empty function bodies are ignored.
 * 2.  Function bodies with only declarations, no statements, are ignored. 
 * 3.  We add instrumentation when we fall off the end of a function, but we
 *     have to be careful not to put it after a return statement.
 */
gct_transform_function(compstmt)
     gct_node compstmt;
{
  gct_node rover;
  extern int errorcount;
  int instrumentation_use;	/* Standard or weak? */
  extern char *main_input_filename;


  /* Do no instrumentation if there's been a compilation error. */
  if (errorcount > 0)
    return;

  assert(main_input_filename);
  assert(compstmt->filename);
  gct_set_routine_context(DECL_PRINT_NAME(current_function_decl));
  if (   0 != strcmp(compstmt->filename, main_input_filename)
      && OFF == gct_option_value(OPT_INSTRUMENT_INCLUDED_FILES))
    {
      gct_set_option(CACHE_CONTEXT, OPT_INSTRUMENT, OFF);
    }
  
  /* The grammar treats empty compound statements specially.  It's safer
     to change our handling than to change the grammar.  We simply do not
     instrument empty compound statements. */
  if (GCT_EMPTY_COMPOUND_STATEMENT(compstmt))
  {
    if (routine_on() || race_on())
    {
      warning("Routine is empty, so no instrumentation added.");
    }
    goto out;
  }
  
  if (ON == gct_option_value(OPT_SHOW_DECLS))
    {
      fprintf(stderr, "\n%s:\n", DECL_PRINT_NAME(current_function_decl));
      print_declarations(stderr);
    }
  
  if (ON == gct_option_value(OPT_SHOW_TREE))
    {
      fprintf(stderr, "\n%s:\n", DECL_PRINT_NAME(current_function_decl));
      gct_dump_tree(stderr, compstmt, 0);
    }
  if (ON == gct_option_value(OPT_CHECK_TREE))
    {
      gct_build_consistency(compstmt, 0);
    }

  instrumentation_use = gct_instrumentation_uses();
  if (OPT_NEED_CONFLICT == instrumentation_use)
    {
      error("Weak mutation and race/routine/call instrumentation cannot be mixed, alas.");
    }
  else if (   ON == gct_option_value(OPT_FORCE_DESCEND)
	   || (   instrumentation_on()
	       && gct_any_instrumentation_on())
	   || add_writelog_on()
	   || add_readlog_on())
  {
      int routine_only = 0;
      int fell_off_end = 1;	/* Don't put instrumentation after return. */

      In_function_body = 0;
      gct_select_instrumentation_set(instrumentation_use);
      gct_lookup_init();
      gct_temp_init(compstmt);
      mapfile_function_start();
      
      if (ON == gct_option_value(OPT_SHOW_VISIBLE))
	{
	  show_visible_variables(0, "entry to function", 0);
	}
      
      /*
       * Now instrument the contents of the function.  As a special hack
       * that saves time, handle the case where only ROUTINE instrumentation
       * is asked for specially.  (It takes a LONG time to instrument the
       * kernel -- sure, most of that time is spent elsewhere, but this
       * is easy to add and doesn't hurt.
       *
       * We have to descend if writelog() is to be added to calls to exit().
       * We needn't descend if readlog() alone is set, since that's done
       * alongside routine instrumentation.
       */

      routine_only = !add_writelog_on() && routine_on() && gct_only_instrumentation(OPT_ROUTINE);

      rover = compstmt->children;
      do
      {
	if (   0 == In_function_body
	    && GCT_OTHER != rover->type
	    && GCT_DECLARATION != rover->type)
	{
	  In_function_body = 1;
	  i_std_routine(compstmt, rover);	/* Add routine instrumentation */
	  if (routine_only)
	      break;
	}
	rover=rover->next;	/* Advance in case node is changed in place */

	/* Remember if the last statement in the routine is a return. */
	if (   rover == compstmt->children->prev
	    && GCT_RETURN == rover->prev->type)
	{
	  fell_off_end = 0;
	}
	
	if (! routine_only)
	{
	  DO_INSTRUMENT(compstmt, rover->prev);
	}
      }
      while (rover != compstmt->children);

      /*
       * Do any "falling off the end instrumentation" that's required. 
       * Note that we don't instrument the end if there were no
       * statements in the function -- consistent with treatment of empty
       * function body above.  This is a very special case:  declarations,
       * but no statements in the function.
       */
      if (fell_off_end)
      {
	if (In_function_body)
	{
	  i_std_end_routine(compstmt, compstmt->children->prev);
	}
	else if (routine_on() || race_on())
	{
	  warning("Routine is empty, so no race or routine instrumentation added.");
	}
      }
      
      gct_lookup_finish();
      dump_mapfile_buffer();
      mapfile_function_finish();
      
      if (ON == gct_option_value(OPT_CHECK_TREE))
        {
	  gct_build_consistency(compstmt, 1);
        }
  }
 out:
  gct_no_routine_context();

#ifdef _DEBUG_MALLOC_INC
  /* This is a good place to look for problems. */
  malloc_chain_check(1);
#endif  
}
