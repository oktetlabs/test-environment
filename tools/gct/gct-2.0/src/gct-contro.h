/* Copyright (C) 1992 by Brian Marick.  All rights reserved. */
/*
 * When aggregated with works covered by the GNU General Public License,
 * that license applies to this file.  Brian Marick retains the right to
 * apply other terms when this file is distributed as a part of other
 * works.  See the GNU General Public License (version 2) for more
 * explanation.
 */

#include <sys/types.h>

/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-contro.h,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */

/*
 * This file contains the code used to control what GCT does.   The
 * general design is as follows.
 *
 * 1.  There are four contexts for options.   Options can apply globally,
 * to a particular file, to a particular routine within a particular
 * file, or from a particular point in a file until the end of the file.
 *
 * 2.  The first three contexts are specified by the control file; the
 * last serves as a cache during the instrumentation of a routine.
 *
 * 3.  The contexts are arranged in a "binding stack".  The cache
 * context is searched first, the routine context next, the file context
 * next, and the global context last.
 *
 * 4.  The compiler front end (gct) uses the global context to decide
 * what files to instrument.  CC1 is called on a particular file, which
 * it pushes on the context stack.  As it discovers routines, it pushes
 * their context on the context stack; however, it always pushes them
 * *under* the cache context.  This is for historical reasons -- the
 * cache context was once intended to be manipulated by in-line comments:
 * could involve pushing and popping values of particular options.
 *
 * 6.  Options have three values, ON, OFF, and NONE.  NONE means to
 * search up the context stack.  It is a program error for any option in
 * the global context to have a NONE value.
 *
 * 7.  The cache context caches only instrumentation options.  Other
 * options have value NONE, so the search continues in higher-level
 * context.  Thus, the cache can be used invisibly.  If you KNOW you're
 * inside a routine -- and thus that the cache is active -- and you KNOW
 * the value you're looking for is cached, the cache context can be
 * accessed directly.
 *
 */


/* There are four elements, with distinct names, in the context stack. */

#define GLOBAL_CONTEXT	0
#define FILE_CONTEXT	1
#define ROUTINE_CONTEXT	2
#define CACHE_CONTEXT 3
#define NUM_CONTEXTS	4

#define CONTEXT_IN_RANGE(context)\
	((context)>=0 && (context)<NUM_CONTEXTS)

/*
 * Options have these values.  NONE means use the value from a
 * higher-level context.  There's an array, Value_names, which depends on 
 * the order of these entries.
 */
enum gct_option_value
{
  ON,		/* option is set. */
  OFF,		/* option is cleared */
  NONE		/* Use value from enclosing context. */
};


/*
 * Options are divided into four categories.  
 * OPT_OPTION means the option is really an option
 * OPT_NEED_WEAK means it's a type of instrumentation that forces
 * weak-mutation-style instrumentation (gct-utrans.c). [operand and operator]
 * OPT_NEED_STANDARD means it's a type of instrumentation that forces
 * standard instrumentation (gct-strans.c) [race, routine, and call]
 * OPT_NEED_NEITHER is for types of instrumentation that allow us to choose.
 */

/* Note that these are bitmasks -- we'll OR them together. */
#define OPT_NEED_NEITHER	0x0
#define OPT_NEED_WEAK		0x1
#define OPT_NEED_STANDARD	0x2
#define OPT_NEED_CONFLICT	0x3	/* If both are set */
#define OPT_VALID_NEED_BITS	0x3	/* For sanity checking */

#define OPT_OPTION		0x4	/* A true option */

/* Define the option type. */

enum gct_option_id
{
#define GCTOPT(OPT, NAME, GS, GD, FS, FD, RS, RD, CS, CD, USE)	OPT,
#include "gct-opts.def"
#undef GCTOPT
   LAST_AND_UNUSED_OPTION
};

#define GCT_NUM_OPTIONS  ((int) LAST_AND_UNUSED_OPTION)


/*
 * Options are stored as stacks; users can set the top value, push
 * values, or pop values.  The stacks were intended to be used for the
 * comment context, since comments could explicitly push on new option
 * values.  It is retained, now that the comment context is the cache
 * context, just in case I decide the comment context should reappear.
 */
struct gct_option_struct
{
  enum gct_option_value value;		/* The value of this option. */
  struct gct_option_struct *next;	/* The rest of the stack. */
};

typedef struct gct_option_struct gct_option;
#define NULL_GCT_OPTION	((gct_option *)0)




/*
 * Options are often processed in groups.   For example, each file has a
 * group of options.  Functions within a file may have their own,
 * overriding, options.  Such groups, which are handled as a whole, are
 * all stored in their own structure.
 *
 * Matching:  The global and cache contexts are static. The file and
 * routine contexts are installed and deinstalled as needed:  the file or
 * routine is looked up and made the value of the appropriate location in
 * the stack.  Lookup for routines is simple:  by string comparison.
 * It's a bit more peculiar for filenames.  The problem is that control
 * file names are relative to the master directory, which may not be the
 * directory the tool is being run in.  We could absolutize the two
 * names, but that might break in the presence of symlinks.  It seems
 * safer to say that two files are the same if their i-numbers are the
 * same.  The INODE field in the structure below is used as a cache --
 * it's not filled in until the first file search.
 */
struct gct_option_context_struct
{
  gct_option options[GCT_NUM_OPTIONS];		/* All the options. */
  char *tag;					/* Name of this group. */
  ino_t	inode;					/* I-number for file checks. */
  struct gct_option_context_struct *next;	/* Next context at this level */
  struct gct_option_context_struct *children;	/* Subcontexts */
};


typedef struct gct_option_context_struct gct_option_context;
#define NULL_GCT_OPTION_CONTEXT	((gct_option_context *)0)




/* Utilities (defined in gct-control.c) */

extern enum gct_option_value gct_option_value();
extern gct_set_option();
extern gct_push_option();
extern gct_pop_option();


/* Fast lookup of options.  Use ONLY within a routine. */

/*
 * These are miscellaneous macros for looking up *cached* option values.  
 */
extern gct_option_context *Context_stack[];

#define instrumentation_on() (ON == Context_stack[CACHE_CONTEXT]->options[OPT_INSTRUMENT].value)
#define add_writelog_on() (ON == Context_stack[CACHE_CONTEXT]->options[OPT_WRITELOG].value)
#define add_readlog_on() (ON == Context_stack[CACHE_CONTEXT]->options[OPT_READLOG].value)

/*
 * Instrumentation values.  Note that they return false if
 * instrumentation_on() is false.  This is because a routine may be
 * processed when instrumentation is OFF - to insert calls to gct_readlog
 * and gct_writelog.  Therefore, you cannot add branch coverage when you
 * see an IF unless you know that instrumentation is also on.
 *
 */
#define branch_on() (   instrumentation_on() \
		     && ON == Context_stack[CACHE_CONTEXT]->options[OPT_BRANCH].value)
#define multi_on() (   instrumentation_on() \
		    && ON == Context_stack[CACHE_CONTEXT]->options[OPT_MULTI].value)
#define loop_on() (   instrumentation_on() \
		   && ON == Context_stack[CACHE_CONTEXT]->options[OPT_LOOP].value)
#define operator_on() (   instrumentation_on() \
		       && ON == Context_stack[CACHE_CONTEXT]->options[OPT_OPERATOR].value)
#define operand_on() (   instrumentation_on() \
		      && ON == Context_stack[CACHE_CONTEXT]->options[OPT_OPERAND].value)
#define routine_on() (   instrumentation_on() \
		      && ON == Context_stack[CACHE_CONTEXT]->options[OPT_ROUTINE].value)
#define relational_on() (   instrumentation_on() \
			 && ON == Context_stack[CACHE_CONTEXT]->options[OPT_RELATIONAL].value)
#define call_on() (   instrumentation_on() \
		   && ON == Context_stack[CACHE_CONTEXT]->options[OPT_CALL].value)
#define race_on() (   instrumentation_on() \
		   && ON == Context_stack[CACHE_CONTEXT]->options[OPT_RACE].value)

