/* Copyright (C) 1992 by Brian Marick. */
/*
 * Permission is granted to use this file for any purpose whatsoever.
 * There are no restrictions on distribution, use, or modification.  In
 * particular, inclusion of this file in any instrumented program is
 * freely permitted, and you may distribute that program to third parties
 * under any terms.
 *
 * This file is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.
 */

/* This file contains defines that never change. */

/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-defs.h,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */

/* The type of a counter for a single target.  Change to char, for
example, to reduce the amount of space needed.  If you change the table
to an array of bits, you'll have to edit gct-ps-defs.c as well.  */

#define GCT_CONDITION_TYPE	unsigned long

/*
 * The table that keeps track of how often a particular target has been
 * hit.  Defined in gct-ps-defs.c.  We don't use this directly -- rather,
 * we go through GCT_TABLE_POINTER_FOR_THIS_FILE, which is a define
 * expanding to a variable pointing into the middle of Gct_table.
 * Gct_table is declared here so that tools that extract the log from a
 * running image only have to include one file.
 *
 * Gct_table is always defined to be size GCT_NUM_CONDITIONS+1.  If there was
 * no instrumentation, the table still has a positive size, which
 * prevents loader errors.  Almost invariably, a zero GCT_NUM_CONDITIONS means
 * a user error.  gct-remap will print a warning.
 */
extern GCT_CONDITION_TYPE Gct_table[];

/*  The size of the table.  This variable exists so that a generic tool to
    extract the table from a core file or running image can be built without
    having to include gct-ps-defs.h */

extern long Gct_table_size;

/*  An in-core version of the define GCT_NUM_CONDITIONS. */

extern long Gct_num_conditions;

/*
 * This table stores an entry per race group.  Its use parallels
 * Gct_table.  Routines refer to this through GCT_RACE_TABLE_POINTER_FOR_THIS_FILE, 
 * which is defined to be a different offset into Gct_group_table for
 * each file.
 */

extern long Gct_group_table[];

/*
 * This variable contains the index about to be tested.  If the
 * instrumented program dumps core, this can be dug out to pinpoint where
 * the error happened.  (GDB isn't so useful because there are usually
 * many tests per line.)
 *
 * Later:  Gct_current index is only used when weak-mutation coverage is
 * turned on.
 */
extern long Gct_current_index;


/* Get the value of INDEX. */
#define GCT_GET(index)	(GCT_TABLE_POINTER_FOR_THIS_FILE[index])

/* Increment the value of INDEX */
#define GCT_INC(index)	(GCT_TABLE_POINTER_FOR_THIS_FILE[index]++)

/* Set the value of INDEX */
#define GCT_SET(index, value)	(GCT_TABLE_POINTER_FOR_THIS_FILE[index] = (value))

/* _G and _G2 (explained below) can keep track of which index is being processed,
 * in a way that is useful to gcorefrom(1).  By default they don't, for speed.
 * To turn this feature on, you must uncomment the folowing define and recompile
 * the instrumented code.  (Note:  you'll probably just want to reinstrument
 * everything from scratch; note that gct-init will not overwrite a changed
 * gct-defs.h.
 */

/*
#define GCT_WEAK_MUTATION 
*/
#ifdef GCT_WEAK_MUTATION
/*
 * This functoid is used when GCT adds instrumentation.  The brevity makes the
 * instrumented code slightly easier to read.
 */

#define _G(index, test) \
  (Gct_current_index = GCT_TABLE_POINTER_FOR_THIS_FILE - Gct_table + (index),	\
   (test)?(GCT_INC(index)):0)

/*
 * This functiod is used for binary tests -- multiconditional, etc.  By
 * convention, the first index is the TRUE branch, the second the FALSE.
 */
#define _G2(index, test)	\
  (Gct_current_index = GCT_TABLE_POINTER_FOR_THIS_FILE - Gct_table + (index),
   (test)?(GCT_INC(index)):GCT_INC(index+1))

#else

/* Standard coverage */
#define _G(index, test) 	((test)?(GCT_INC(index)):0)
#define _G2(index, test)	((test)?(GCT_INC(index)):GCT_INC(index+1))

#endif

#if GCT_NUM_RACE_GROUPS > 0

<<<TAG>>>:  You may need to declare externs here.

/* This yields a thread number from 0 to bits-in-word (for this simple
   implementation) */
#define GCT_THREAD	<<<you must define this>>>

/* The value of a group/thread - for debugging and testing */
#define GCT_RACE_GROUP_VALUE(group, thread) \
    (GCT_RACE_TABLE_POINTER_FOR_THIS_FILE[(group)] & (1 << (thread)))

/* Use this if an expression is syntactically required, but you don't
   want it to do anything. */
#define GCT_NO_OP 49

/*
 * Enter a race group at the top.  This must be an expression, not a
 * statement.  
 */
#define GCT_RACE_GROUP_ENTER(group) \
    (GCT_RACE_TABLE_POINTER_FOR_THIS_FILE[(group)] |= (1 << GCT_THREAD))

/* Return from a function call -- this is the same as entering at the top. */
#define GCT_RACE_GROUP_REENTER(group) \
    (GCT_RACE_GROUP_ENTER(group))

/*
 * Leave a race group via return or falling off end -- this must be an
 * expression, not a statement.
 */
#define GCT_RACE_GROUP_EXIT(group) \
    (GCT_RACE_TABLE_POINTER_FOR_THIS_FILE[(group)] &= ~(1 << GCT_THREAD))

/* Leave a race group via a call -- same as returning. */
#define GCT_RACE_GROUP_CALL(group) \
    (GCT_RACE_GROUP_EXIT(group))

/* Test whether another thread is in the same race group. */
#define GCT_RACING(group)\
    (GCT_RACE_TABLE_POINTER_FOR_THIS_FILE[(group)])

/* Check for races -- the check is always made before the entry. */
#define GCT_RACE_GROUP_CHECK(index, group)\
    (_G(index, GCT_RACING(group)))

/*
 * NOTE:  If you define RACE_GROUP_CALL to be a no-op (so that subroutines
 * can race with their caller), the bit for this thread in the
 * Gct_group_table will already be set when a routine is called
 * recursively.  The default GCT_RACE_GROUP_CHECK would then count a
 * recursive call as a race.  In some cases, that might indeed flush out
 * a locking error, but it's a probably not as effective as a true
 * two-thread race and shouldn't be counted as such.
 *
 * To avoid that problem, define GCT_RACING as 
 *
 * #define GCT_RACING(group)\
 *     (GCT_RACE_TABLE_POINTER_FOR_THIS_FILE[(group)] & ~(1 << GCT_THREAD))
 */

#endif
