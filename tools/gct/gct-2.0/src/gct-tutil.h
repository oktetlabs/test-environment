/* Copyright (C) 1992 by Brian Marick.  All rights reserved. */
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
 * Header file for transformation utilities.   This corresponds to files
 *
 * gct-lookup.c:		Looking up comparison-compatible variables.
 * gct-temp.c:			Building assignment-compatible variables.
 * gct-tbuild.c:		Building transformed trees.
 * gct-strans.c:		Standard instrumentation utilities.
 * gct-utrans.c:		Weak instrumentation utilities.
 * gct-mapfil.c:		Mapfile utilities.
 */

/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-tutil.h,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */

/*
 * The state is passed down from parent to child to control the child's
 * instrumentation.  
 *
 * First use: weak mutation.  In a case where you're instrumenting A in
 * the expression (A < expr), two bits of state have been passed down to
 * A:
 *
 * 1.  The operator "<".
 * 2.  A temporary that will hold the value of A.
 *
 * Normally, instrumentation for A might be
 *
 *   _G(A!=B), _G(A!=C)
 *
 * Now it will be
 *
 *   _G(A<T123!=B<T123), _G(A<T123!=B<T123)
 *
 * Weak mutation also applies to "combining references" like arrays,
 * structs, dereferences, etc.   In this case, three bits of state are
 * passed down.
 * 1.  The combining root (arrayref, dotref, dereference)
 * 2.  The position of "self" in the result.
 * 3.  The other part of the combination (if applicable.)
 *
 * Hence, what would normally be
 *
 * 	_G(A!=B) 
 *
 * would become
 *
 * 	_G(T123[A]!=T123[B])
 *
 * or
 * 	_G(*A != *B)
 *
 *
 * The typical rule for a state is that it's passed down only one level.
 * The child may propagate it, but that's never done without a conscious
 * effort.
 *
 * FLAGS:
 * no_constant_checks:		if set, don't check whether the child
 * 				ever varies.  Applies only to identifiers.
 * no_substitutions:		if set, don't check a reference
 * 				against comparison-compatible variables.
 * identical_type:			When looking for matching variables,
 * 				only count identity.  Used by array
 * 				indices to avoid getting floats.
 * integer_only:		When doing a substitution, use only
 * 				integers:  used for array references,
 * 				the right hand side of shifts, etc.
 * 				Used both to prevent illegal substitutions
 * 				(array indices) and pointless substitutions
 * 				(intVar could be floatVar in context
 * 				of hand side of shift.)
 * OTHER VARS:
 *
 * ref_type:			The "root" type of a complex reference
 * 				like A[i].j->foo
 */


/*
 * Consider instrumenting an  expression like A[i].j->foo < M.  The
 * eventual instrumentation must compare that variable to, for example,
 * A[newvar].j->foo < M.  To do this, we must pass down a stack of
 * information about how to combine newvar.  This stack looks like
 *
 * <M ... ->foo ... .j ... A[ 	<= bottom
 *
 * This is passed down as a linked list of "suff" structures.
 */

enum suff_tag 		/* Which kind. */
{
  TAG_COMBINER,		/* A structure builder. */
  TAG_OPERATOR		/* A relational expression. */
};

struct suff_struct
{
  struct suff_struct *next;
  enum suff_tag tag;
  union
    {
      struct
	{
	  gct_node weak_operator;	/* The operator: <, <=, !=, etc. */
	  gct_node weak_variable;	/* The other side of the operator. */
	} operator;
      struct
	{
	  gct_node weak_root;		/* The kind of combiner:  array, etc. */
	  gct_node weak_other_side;	/* If binary combiner, other side. */
	  char weak_me_first;		/* Which side am I on? */
	} combiner;
    } kind;
};
typedef struct suff_struct i_suff;



#define WEAK_OPERATOR_P(X) (TAG_OPERATOR == (X)->tag)
#define WEAK_COMBINER_P(X) (TAG_COMBINER == (X)->tag)

#define WEAK_OPERATOR(X) ((X)->kind.operator.weak_operator)
#define WEAK_VARIABLE(X) ((X)->kind.operator.weak_variable)
#define WEAK_OTHER_SIDE(X) ((X)->kind.combiner.weak_other_side)
#define WEAK_ME_FIRST(X) ((X)->kind.combiner.weak_me_first)
#define WEAK_ROOT(X) ((X)->kind.combiner.weak_root)



/* This is a useful predicate made from above. */
#define WEAK_ARRAY_INDEX(X)			\
      (  WEAK_COMBINER_P(X)			\
       && GCT_ARRAYREF == WEAK_ROOT(X)->type	\
       && !WEAK_ME_FIRST(X))


#define DEREFERENCE_NEEDED(X)				\
  (   WEAK_COMBINER_P(X)				\
   && (   GCT_DEREFERENCE == WEAK_ROOT(suff)->type	\
       || GCT_ARROWREF == WEAK_ROOT(suff)->type		\
       || GCT_ARRAYREF == WEAK_ROOT(suff)->type	))

/* The state itself. */

/* Everything here defaults as 0. */
struct i_state_struct
{
  i_suff *suff_stack;
  tree ref_type;
  char no_constant_checks;
  char no_substitutions;
  char integer_only;
};
typedef struct i_state_struct i_state;

extern i_state Default_state;

/* Useful operations on the state.  These are macros not so much for
   efficiency, but for better type checking of arguments. */

extern i_suff *__TEMP_Suff;		/* Used when popping. */

#define push_combiner(root, other_side, me_first, state)	\
  __TEMP_Suff = (i_suff *) xmalloc(sizeof(i_suff));		\
  WEAK_OTHER_SIDE(__TEMP_Suff) = (other_side);		\
  WEAK_ME_FIRST(__TEMP_Suff) = (me_first);			\
  WEAK_ROOT(__TEMP_Suff) = (root);				\
  __TEMP_Suff->next = state.suff_stack;			\
  __TEMP_Suff->tag = TAG_COMBINER;				\
 state.suff_stack = __TEMP_Suff;



	

#define push_operator(operator, variable, state)		\
  __TEMP_Suff = (i_suff *) xmalloc(sizeof(i_suff));		\
  WEAK_OPERATOR(__TEMP_Suff) = (operator);		\
  WEAK_VARIABLE(__TEMP_Suff) = (variable);			\
  __TEMP_Suff->next = (state).suff_stack;			\
  __TEMP_Suff->tag = TAG_OPERATOR;				\
  (state).suff_stack = __TEMP_Suff;

/* The popped value is DISCARDED. */
#define pop_suff(state)			\
  __TEMP_Suff = (state).suff_stack;	\
  (state).suff_stack = __TEMP_Suff->next;	\
  free(__TEMP_Suff);

#define empty_suff(state)	((i_suff *) 0 == (state).suff_stack)

#define top_suff(state)		((state).suff_stack)


/*
 * This macro gives TARGET_STATE the same ref_type as the ORIG_STATE.
 * But if orig_state has no ref_type, the state of NODE is used.
 */

#define set_ref_type(target_state, orig_state, node)	\
((target_state).ref_type = \
  ((orig_state).ref_type)? (orig_state).ref_type : (node)->gcc_type)

/*
 * This variable stores the number of conditions (table entries) used so
 * far *in this file*.  It starts at 0 in each file.
 */

extern int Gct_next_index;

/*
 * This variable stores the number of table entries used since gct-init
 * was called.
 * It is initialized by reading from the gct-ps-defs.h file, which is where
 * the final value is placed. 
 */

extern int Gct_cumulative_index;

/* This variable is used to count the number of files processed.  It is
   initialized from the gct-ps-defs.h, which is where the incremented value
   is placed.  It is used to define a unique variable per file. */

extern int Gct_num_files;

/* This variable stores the number of race groups in this file.  Its use
   parallels Gct_next_index. */

extern int Gct_next_race_group;

/* This is the number of race groups seen so far in all files.  Used to
   allocate the race group table.  Stored in gct-ps-defs.c */
extern int Gct_cumulative_race_groups;



/* Some miscellaneous useful random macros */

/* IMMEDIATE types can be compared with equality. */
#define NON_IMMEDIATE_P(gcc_type)		\
      (   RECORD_TYPE == TREE_CODE(gcc_type)	\
       || UNION_TYPE == TREE_CODE(gcc_type))

/* POINTERISH types are pointers and arrays.  They can be dereferenced. */
#define POINTERISH_P(gcc_type)		\
      (   ARRAY_TYPE == TREE_CODE(gcc_type)	\
       || POINTER_TYPE == TREE_CODE(gcc_type))


/* TRUE if the given TYPE is a pointer to void. */
#define VOID_POINTER(type)	\
  	(   POINTER_TYPE == TREE_CODE(type)	\
	 && void_type_node == TREE_TYPE(type))

/*
 * These are used in arguments to *_map.  FIRST indicates that this is
 * the first instrumentation for this node; DUPLICATE indicates this is
 * one of the 2nd through Nth.  Note that this argument is a boolean
 * (and logical expressions are often used) -- these definitions are only
 * for convenience in those cases where the argument is a constant.
 */
#define FIRST	0
#define DUPLICATE	1


/*
 * This value marks the end of a variable number of gct_node arguments.  
 * We cannot use null as a marker because it is a valid (and common)
 * argument to the tree-building routines.
 */
extern gct_node gct_node_arg_END;


/*
 * These are used when a temporary variable is being used.  REFERENCE_OK
 * is passed in when the variable will be a reference to a value tree.
 * If multiple evaluations of the tree will have no side-effects,
 * internal_id may, at its discretion, just return its argument.
 *
 * If FORCE is set, the tree passed in is an lvalue in an assignment
 * statement.  The temporary is used to hold the old value of the lvalue;
 * hence a new temporary must be returned.
 *
 * If WANT_BASE_TYPE is set, the temporary is to be of the type passed in.
 * If WANT_POINTER_TYPE, it is to be of type pointer-to-base-type.
 */

#define REFERENCE_OK	1
#define FORCE		2

#define REF_IN_RANGE(ref)	(1==(ref)||2==(ref))

/*
 * These routines are used when deciding where to put a new variable.
 * IF DOING_DECL:
 * 	Put it before the declaration.
 * IF CURRENT_COMPOUND
 * 	Put it after the last declaration of the current compound statement.
 * IF FUNCTION_COMPOUND
 * 	Put it after the last declaration of the function's outermost stmt.
 */

#define CLOSEST		-1
#define OUTERMOST	-2 

#define WHERE_IN_RANGE(where)	((where)==CLOSEST || (where)== OUTERMOST)


#define WANT_POINTER_TYPE	-10
#define WANT_BASE_TYPE		-20 

#define POINTERNESS_IN_RANGE(X)	((X)==WANT_POINTER_TYPE || (X)== WANT_BASE_TYPE)



#define USE_GLOBAL		-10
#define DONT_USE_GLOBAL		-20

#define GLOBAL_IN_RANGE(x)	((x)==USE_GLOBAL || (x) == DONT_USE_GLOBAL)

/* Given a tree, these macros decide whether a temporary would be required
   for it.  Currently conservative:  only non-volatile identifiers and
   constants escape the need for temporaries. */

#define NO_TEMPORARY_NEEDED(node)	\
	(   (GCT_IDENTIFIER == (node)->type && !(node)->is_volatile)	\
          || GCT_CONSTANT == (node)->type)	/* Even strings */
#define TEMPORARY_NEEDED(node) (!(NO_TEMPORARY_NEEDED(node)))

		     /* SUMMARIZING FUNCTIONS */

/*
 * Each function is hashed and that hash value is placed in the mapfile.
 * We can then look at two mapfiles from two versions of a program and
 * see which functions have changed.
 */
/* This hashing function is adapted from GNU make. */
#define	GCT_HASH(var, c) \
  ((var += (c)), \
   (var = ((var) << 7) + ((var) >> 20)), \
   var = ((var) & 0x7fffffff))


/* Utility nodes (in gct-tgroups.c) */

extern gct_node Int_root;
extern gct_node Less_root;
extern gct_node Lesseq_root;
extern gct_node Greater_root;
extern gct_node Greatereq_root;


/* Group tables (int gct-tgroups.c) */

extern char Gct_relational[];	/* Is node a relational operator? */
extern char Gct_boolean[];	/* Is node a boolean operator? (weak routines) */
extern char Gct_true_boolean[];	/* Is node a boolean operator? (standard routines) */
extern char Gct_boolean_assign[];	/* Is node a boolean assignment operator? */
extern char Gct_integer_only[];	/* Does not require integer arguments. */
extern char Gct_nameable[];	/* Something we construct abbrev. names for? */


/* Utility functions. */

/* gct-lookup.c */
extern void gct_lookup_init();
extern void gct_lookup_finish();
extern int say_nesting_depth();
extern void gct_lookup_compound_init();
extern void gct_lookup_decl_init();
extern tree gct_lookup_decl_var();
extern void gct_lookup_decl_finish();
extern void gct_lookup_compound_finish();
extern tree name_iterate();
extern void end_iteration();

/* gct-temps.c */
extern void gct_temp_init();
extern void gct_temp_compound_init();
extern void gct_temp_decl_init();
extern void gct_temp_decl_finish();
extern gct_node temporary_id();

/* gct-tbuild.c */

extern void gct_initialize_roots();
extern gct_node gct_reverse_test();
extern gct_node makeroot();
extern gct_node newtree();
extern gct_node copy();
extern gct_node copytree();
extern gct_node copylist();
extern gct_node epsilon();
extern gct_node notnot();
extern gct_node comma();
extern gct_node compound();
extern gct_node make_probe();
extern gct_node make_binary_probe();
extern gct_node make_binary_probe();
extern gct_node make_unconditional_incr();
extern gct_node make_simple_statement();
extern gct_node make_logcall();
extern gct_node ne_test();
extern void add_test();
extern void add_loop_test();
extern void push_switch();
extern void pop_switch();
extern int switch_default_seen();
extern void now_switch_has_default();
extern gct_node switch_needed_init();
extern gct_node switch_case_test();

/* gct-tcompat.c */

extern int comparison_compatible();
extern int times_compatible();

/* gct-strans.c */

extern void standard_binary_test();
extern gct_node loop_binary_test();

/* gct-mapfil.c */

void init_mapfile();
void finish_mapfile();
void mapfile_function_start();
void mapfile_function_finish();
void dump_mapfile_buffer();
void assert_empty_mapfile_buffer();
void branch_map();
void multi_map();
void loop_map();
void operator_map();
void operand_map();
void routine_map();
void race_map();
void call_map();
void flavor_map();
void cliche_map();
void map_placeholder();
char *make_leftmost_name();
char *make_mapname();
void set_mapfile_name();
char *mapfile_name_to_print();
