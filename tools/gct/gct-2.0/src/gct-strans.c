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
 * This file contains "standard instrumentation" -- everything except
 * weak mutation.  If operator or operand instrumentation are called for,
 * gct-utrans.c is used instead.
 */

			     /* DESIGN */

/*
 * The routines in this file are called via DO_INSTRUMENT.  See
 * gct-tree.def for the table that selects which routines to call.
 * This comment describes what's generally true for all routines; the 
 * comments for individual routines give particulars.
 *
 * The routines are given trees.  They replace these trees with trees
 * that yield the same value and side effects, but also call _G or _G2.
 * (See gct-defs.h.)  Each routine is responsible for calling
 * DO_INSTRUMENT on each of its children.  Whether it does that before or
 * after adding its own instrumentation is a free choice.  (Generally,
 * it's less error-prone to instrument the children, then instrument the
 * root.)
 *
 * Routines take two arguments:
 * SELF is the node to be instrumented.  In i_std_if, it would be the IF
 * node, which has the if's test, the if's THEN statement, and (optionally)
 * the if's ELSE statement as children.
 * PARENT is the parent of SELF.  It's required for the replacement.
 *
 * Instrumentation generally means using the value of a tree twice:  once
 * to record (for example) whether it was true or false, and once to
 * yield the value of the tree.  For this reason, temporaries must be
 * created by temporary_id(), which returns a declared identifier.
 * As a convention, only copies of the temporary are used, not the
 * original value, which is freed at the end of the routine.  This is
 * less prone to memory leaks, using the same node twice, etc.,
 * especially during maintenance. 
 *
 * The original SELF node must be built into the resulting tree (only
 * once, of course).  This is needed so that any annotations on SELF
 * (pragmas, etc) are placed in the instrumented file.  The SETTER and
 * VALUE macros (explained where they're declared) make this easier.
 *
 * Instrumentation is controlled by the control file, manifest in
 * routines like branch_on(), routine_on(), etc.  (These are actually
 * macros.) 
 *
 * Gct_next_index is the index of the next line in the mapfile.  People
 * want those entries to read from left to right.  Because
 * instrumentation is top-down or bottom-up, that's often not the natural way
 * to increment Gct_next_index.  We'll often do the following:
 * 	1. Emit mapfile entries for SELF, reserving values of Gct_next_index.
 * 	2. Instrument subtrees.
 * 	3. Add instrumentation for SELF, using reserved values.
 *
 * SELF is the "instrumentation point" referred to in the user's manual.
 * With a few exceptions (routines and races), if SELF is in a macro,
 * instrumentation is not done.  The mechanism is to pretend that the
 * appropriate instrumentation was turned off.  Macros only affect the
 * instrumentation of SELF; the subtrees take care of themselves.  A
 * typical code pattern is:
 *
 * 	do_instrumentation = branch_on() && gct_outside_macro_p(self);
 * 	if (do_instrumentation)
 * 		emit mapfile entries.
 * 	DO_INSTRUMENT(self, A_SUBTREE(self));
 * 	if (do_instrumentation)
 * 		remember_place... 	Remembers where subtree came from
 * 		edit the subtree.
 * 		replace...		Replace subtree.
 *
 * Note the use of remember_place() and replace().  All tree manipulation
 * should use these; see the code for examples.
 *
 * Because the parent may manipulate this tree after the call returns -
 * in particular, may call temporary_id() on it - it is important that
 * the instrumented tree have the same type as the original tree.  This
 * is typically done by reusing SELF as the root of the original tree, or
 * by constructing the instrumented tree with a call to comma(), which
 * produces a comma-expression whose type is the type of the last element
 * in the comma-list.
 *
 * Be careful about using local variables to store pointers to parts of a
 * tree or values derived from a tree.
 * 1. Pointers to parts of a tree are not updated by DO_INSTRUMENT, so
 * they may not point to where you want.  A common error: 
 * 	child = SOME_SUBTREE(self);
 * 	DO_INSTRUMENT(self, child);
 * 	... build child into a tree ...
 * This is an error because you probably want to build the instrumented
 * subtree (retrievable by SOME_SUBTREE(self)) into a tree; child may be
 * deep within that instrumented subtree.
 * 2.  An instrumented tree does not have all the properties of the
 * original tree (such as location).  This code is in error:
 * 	DO_INSTRUMENT(self, SOME_SUBTREE(self));
 * 	outside_macro = gct_outside_macro_p(SOME_SUBTREE(self));
 * The call to gct_outside_macro_p must be made before the instrumentation.
 *
 *
 *
 * CODE READ CHECKLIST: (NO = bug)
 *
 * Note:  this checklist also applies to weak mutation instrumentation
 * (gct-utrans.c), which is more complicated.
 *
 * 1. Have only copies of temporaries been used?
 * 2. Has the original value of temporary_id been freed?
 * 3. Does the instrumented tree have the same type as the original tree?
 * 4. If locals are set pointing to parts of a tree, is the tree still the same when
 *    the locals are used (else they might point to the wrong place)?
 * 5. Are locals set to values derived from the root of an instrumented tree?
 * 6. Do the mapfile entries read from left to right?
 * 7. Is SELF built into the resulting instrumented tree?
 * 8. Is any temporary_id that is an initialized non-static declared
 *    OUTERMOST?  (See gct-temps.c.)
 */


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

			    /* GLOBALS */

/*
 * In race coverage, we surround function calls with CALL and REENTER
 * macros.  We don't want to require the macro writer to make them nest
 * properly, so we guarantee that they won't be nested.  That means we
 * have to keep track of nested calls with this global.  Could avoid the
 * issue by currying the arguments like is done in weak instrumentation:
 *
 * 	F(g(h(x)))
 *
 * becomes
 * 	T1= h(x), T2=g(T1), F(T1)
 *
 * (with the appropriate CALLs and REENTERS), but that really doesn't
 * seem worthwhile.
 */
static int Call_depth = 0;


		   /* INSTRUMENTATION UTILITIES */

/*
 * SETTER and VALUE
 *
 * The general pattern of instrumentation is to construct a provisional
 * temporary variable for an expression (using temporary_id).
 * temporary_id will return the root of the original tree if that tree
 * doesn't need a temporary (e.g., it is a non-volatile identifier)
 *
 * The new instrumentation is constructed using these macros.
 *
 * SETTER(tempvar, setter_rh) constructs the expression "tempvar =
 * setter_rh" if the temporary was needed; otherwise, it returns GCT_NULL_NODE.
 *
 * VALUE(tempvar, setter_rh) returns either tempvar or setter_rh,
 * depending on whether the tempvar was needed.  
 *
 * The two expressions must be used in tandem.  If used, the following
 * are true:
 *
 * 1. The original tempvar has the value of setter_rh.  Further, it is 
 * safe to copy.  (Remember:  the convention is that tempvars are always
 * copied - the original value is never used.)
 *
 * 2. The setter_rh has been used exactly once.  The caller is now
 * obliged never to use setter_rh (or a copy) elsewhere.
 *
 * A typical example of use is this:
 *
 *       gct_node test_temp = temporary_id(if_test, CLOSEST, REFERENCE_OK, ...);
 *       new_if_test = comma(SETTER(test_temp, if_test),
 * 			  make_binary_probe(starting_index, copy(test_temp)),
 * 			  VALUE(test_temp, if_test),	
 * 			  gct_node_arg_END);	
 *
 * comma() will elide the GCT_NULL_NODE, if necessary, so the above
 * yields one of two expressions:
 *
 * 	Temp = <expr>, _G2(#, Temp), Temp
 * 	_G2(#, <expr>), <expr>
 *
 * NOTE:  These macros have a similar function to SETTER and VALUE in gct-utrans.c,
 * though those routines are more complicated.
 */

#define SETTER(tempvar, setter_rh) \
  ((tempvar) != (setter_rh) ? \
   newtree(makeroot(GCT_SIMPLE_ASSIGN, "="), copy((tempvar)), (setter_rh), gct_node_arg_END) : \
   GCT_NULL_NODE)
	
#define VALUE(tempvar, setter_rh)\
	((tempvar) != (setter_rh) ? copy(tempvar) : (setter_rh))


/*
 * This routine is called for assignment expressions, return statements,
 * and declarations.  All of these can add instrumentation to check that
 * their argument has been both true and false (a variant of
 * multi-condition coverage) when the argument is boolean.  This routine
 * returns TRUE if such instrumentation is appropriate:
 *
 * 1.  multi-condition coverage is turned on for this routine.
 * 2.  the instrumentation point (SELF) is outside a macro.
 * 3.  the expression returns a boolean.
 * 4.  the expression is not && or || (in which case, coverage of the
 *     components will force the entire expression to be both true and
 *     false, so no special instrumentation is needed).
 *
 * Note:  it is not required that EXPR be a child of SELF.
 */
int
assignish_multi_on(self, expr)
     gct_node self;
     gct_node expr;
{
  if (   GCT_ANDAND == expr->type
      || GCT_OROR == expr->type)
    {
      return 0;
    }
  return (   (   Gct_relational[(int)expr->type]
	      || Gct_true_boolean[(int)expr->type])
	  && multi_on()
	  && gct_outside_macro_p(self->first_char));
}


/*
 * A common replacement is to convert EXPR into T=EXPR, _G2(#, T), T.
 * This routine does that.  The resulting expression is optimized into 
 * _G2(#, EXPR), EXPR if the expression does not require a temporary.
 *
 * The instrumentation is done in place (standard instrumentation).
 * After this call, EXPR is no longer the child of PARENT; the added code
 * is between them.
 */
void
standard_binary_test(parent, expr, index)
     gct_node parent;	/* Node above the expr */
     gct_node expr;	/* Expression we're replacing. */
     int index;		/* Mapfile index for the instrumentation */
{
  gct_node new_expr;		/* The instrumented version */
  gct_node expr_temp = temporary_id(expr, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  gct_node placeholder;	/* For remember_place. */

  remember_place(parent, expr, placeholder);
  new_expr = comma(SETTER(expr_temp, expr),
		   make_binary_probe(index, copy(expr_temp)),
		   VALUE(expr_temp, expr),
		   gct_node_arg_END);
  replace(parent, new_expr, placeholder);
  free_temp(expr_temp, expr);
}



/*
 * This routine is like standard_binary_test, except that it inserts a
 * call to gct_writelog into the expression:
 *
 * 	T1 = expr, gct_writelog(), T1
 *
 * The instrumentation is done in place (standard instrumentation).
 * After this call, EXPR is no longer the child of PARENT; the added code
 * is between them.
 */
void
standard_add_writelog(parent, expr)
     gct_node parent;	/* Node above the expr */
     gct_node expr;	/* Expression we're replacing. */
{
  gct_node new_expr;		/* The instrumented version */
  gct_node expr_temp = temporary_id(expr, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  gct_node placeholder;	/* For remember_place. */

  remember_place(parent, expr, placeholder);
  new_expr = comma(SETTER(expr_temp, expr),
		   make_logcall("gct_writelog"),
		   VALUE(expr_temp, expr),
		   gct_node_arg_END);
  replace(parent, new_expr, placeholder);
  free_temp(expr_temp, expr);
}



/*
 * This routine is related to standard_binary_test, but specialized for
 * use in loops.  There are two changes:
 *
 * 1.  The created temporary is returned to the caller, who is
 * responsible for freeing it (using free_temp()).
 * 2.  The binary probe is inserted only if the argument BINARY_ON_P is
 * true.  (When it's false, the return value will be T=EXPR,T.  This
 * leads to redundant code of this form when the temporary is used in a
 * later test:
 * 	(T=EXPR,T), TEST(T), T
 * but we expect the compiler to optimize out the one dead reference.
 */
gct_node
loop_binary_test(parent, loop_test, index, branch_on_p)
     gct_node parent;		/* Node above the expr */
     gct_node loop_test;	/* Expression we're replacing. */
     int index;			/* Mapfile index for the instrumentation */
     int branch_on_p;		/* Whether to insert branch instrumention. */
{
  gct_node new_loop_test;	/* Newly instrumented value. */
  gct_node test_temp;		/* Test variable in place of loop_test. */
  gct_node placeholder;		/* For remember_place. */

  test_temp = temporary_id(loop_test, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);

  remember_place(parent, loop_test, placeholder);
  new_loop_test = comma(SETTER(test_temp, loop_test),
			(branch_on_p ? 
			 make_binary_probe(index, copy(test_temp)) :
			 GCT_NULL_NODE),
			VALUE(test_temp, loop_test),
			gct_node_arg_END);
  replace(parent, new_loop_test, placeholder);
  return test_temp;
}

		    /* INSTRUMENTATION ROUTINES */

/*
 * This routine is called when no instrumentation is to be done for this
 * type of tree.  It simply calls DO_INSTRUMENT on all the children.
 *
 * An instrumenting routine that just wants to instrument all its
 * children may also call this routine.  More typically, it will call
 * DO_INSTRUMENT directly.
 *
 * ASSUMED PRECONDITION:  SELF must have at least one child.
 * SPECIAL POSTCONDITION: The parent node is unused. (Must exist because all
 * DO_INSTRUMENT-called routines have same interface.)
 */  
void 
i_std_descend(parent, self)
     gct_node parent;
     gct_node self;
{
    gct_node rover = self->children;

    assert(rover);
    if (rover->next == rover)
    {
      DO_INSTRUMENT(self, rover);
    }
    else	/* Loop requires two elements to work. */
    {
      do
      {
	rover = rover->next;	/* Old value about to be modified in place.*/
	DO_INSTRUMENT(self, rover->prev);
      } while (rover != self->children);
    }
}

/*
 * This routine is called when neither this node, nor its children (if
 * any), are to be instrumented.
 * SPECIAL POSTCONDITION: The self and parent nodes are unused. (Must
 * exist because all DO_INSTRUMENT-called routines have same interface.)
 */
void 
i_std_stop(parent, self)
     gct_node parent;
     gct_node self;
{
    /* Do nothing */
}

/*
 * This routine is called on entry to a routine.  To be precise, it is
 * called when the first non-declaration within the function is
 * recognized. (This makes the line numbering rather odd, but that is of
 * no consequence.)  The instrumentation is done by adding statements
 * before the current node.
 *
 * SPECIAL NOTES:
 *
 * Routine and race instrumentation are not affected by macros.
 * Normally, macros have an effect at a particular "instrumentation
 * point".  That idea doesn't really make sense for routine or race
 * instrumentation.  Further, the reason for ignoring macros (suppressing
 * messages about operators that you don't see in the source code)
 * doesn't really apply to routine and race instrumentation, which
 * generate messages about large, obvious chunks of code, even if people
 * are playing games with macros that define functions.
 *
 * This routine is called by gct_transform_function, not through the DO_INSTRUMENT
 * function table - there's no special parse tree node for start-of-function.
 *
 * If this is the first routine in the process (e.g., main), a call to
 * gct_readlog is added, if desired (via options readlog).
 */
void
i_std_routine(parent, self)
     gct_node parent;
     gct_node self;
{
  gct_node statement = GCT_NULL_NODE;		/* Statement to add. */
  
  /*
   * Note that readlog is added before the first line of "true" code in
   * the program, although results of instrumentation of declaration
   * initializers will be lost.
   *
   * It is also added before any instrumentation, so that the
   * instrumentation isn't overwritten.
   */
  if (   add_readlog_on()
      && gct_entry_routine(DECL_PRINT_NAME(current_function_decl)))
    {
      statement = 
	make_simple_statement(make_logcall("gct_readlog"));
      gct_add_before(&(parent->children), self, statement);
    }

  if (routine_on())
    {
      statement
	= make_simple_statement(make_unconditional_incr(Gct_next_index));
      gct_add_before(&(parent->children), self, statement);
      routine_map(Gct_next_index, self, DECL_PRINT_NAME(current_function_decl),
		  "is never entered.", FIRST);
      Gct_next_index++;
    }
  
  if (race_on())
    {
      /* Add code to check if someone else is in the routine. */
      statement = race_check_statement(self, &Gct_next_index);
      gct_add_before(&(parent->children), self, statement);
      /* Gct_next_index has been incremented as necessary. */
      
      /* Add code to enter the routine myself. */
      statement = race_entry_statement();
      gct_add_before(&(parent->children), self, statement);
    }
}

/*
 * This routine is called when the closing brace of the routine is seen.
 * If race instrumentation is on, it adds exiting-routine instrumentation
 * before the closing brace.  Note that would add it even if the previous
 * statement was a return.  Since this may cause warnings about
 * unreachable code from some compilers, the caller should not call it in
 * this case.
 *
 * If we're "falling off the end of main", a call to gct_writelog is
 * added (if desired).  Just as in the above case, it's inappropriate to
 * add that call if the previous statement was a return.  Note that it
 * *will* get added if the previous statement is an exit() -- that's also
 * inappropriate, but harmless.
 *
 * This routine is called by gct_transform_function, not through the DO_INSTRUMENT
 * function table - there's no special parse tree node for end-of-function.
 *
 */
void
i_std_end_routine(parent, closebrace)
	gct_node parent;
	gct_node closebrace;
{
  if (race_on())
    {
      gct_add_before(&(parent->children), closebrace, race_return_statement());
    }
  
  if (   add_writelog_on()
      && gct_entry_routine(DECL_PRINT_NAME(current_function_decl)))
    {
      gct_add_before(&(parent->children),
		     closebrace, 
		     make_simple_statement(make_logcall("gct_writelog")));
    }
}

	
/*
 * Surround a function call with whatever code is required.  
 *
 * Race coverage: 
 *
 * Surround the call with GCT_RACE_GROUP_CALL and GCT_RACE_GROUP_RETURN.
 * No mapfile entries are generated.  (That's been done at entry to the
 * routine.)
 *
 * Recall that race coverage is not affected by macros.
 *
 * In the case where a function calls a function (f(g())) we're careful
 * to only surround the outermost function with race instrumentation.
 * We also have to avoid putting race instrumentation around function calls
 * inside declarations (because we're not "in" the function yet).
 *
 * Call coverage:
 *
 * Funcall instrumentation comes before instrumentation of the function
 * object.  In the case of something like callarray[f(a)](), that yields
 * a left-to-right order of mapfile entries: the first is for
 * callarray[...], the second is for f.
 *
 * Calling exit or abort or some similar routine:
 *
 * If gct_writelog is to be added, it can be put in one of two places.
 * If the function takes no arguments, it has to be placed before the
 * call:
 *
 * 	<call instrumentation, if any>, gct_writelog(), abort();
 *
 * Note that it is placed after call instrumentation, so that that is
 * recorded.  It is placed before race instrumentation, because it's
 * convenient to do so and makes no difference at this point anyway.
 *
 * If it takes any argument, it's placed after evaluation of the last
 * argument (note that order of evaluation is undefined in C, so coverage
 * may be lost):
 *
 * 	exit(gct_writelog(),3);
 * 	exit((t=f(), gct_writelog(), t));
 *
 * The insertion is not affected by macros.
 */

/*
 * Sub-function to create function mapnames.  Caller must free storage. 
 * In most cases, this routine is the same as make_mapname().  However,
 * expressions that cannot be given to make_mapname can be given to this
 * routine.  (Best example is a ?-expression that yields a function.) 
 */
char *
make_function_mapname(function_node)
     gct_node function_node;
{
  if (Gct_nameable[(int)function_node->type])
    {
      return make_mapname(function_node);
    }
  else if (function_node->text)
    {
      char *retval = (char *)xmalloc(100+strlen(function_node->text));
      sprintf(retval, "%s-expression that yields a function", function_node->text);
      return retval;
    }
  else	/* I believe this to be impossible. */
    {
      return permanent_string("function-yielding expression");
    }
}

      
void 
i_std_funcall(parent, self)
     gct_node parent;
     gct_node self;
{
  gct_node placeholder;	/* Where to put back after instrumenting */
  gct_node working_node = self;	/* What to put back */
  char *function_name = make_function_mapname(GCT_FUNCALL_FUNCTION(self));
  extern int In_function_body;	/* otherwise, in declarations. */
  int needs_writelog = add_writelog_on() && gct_exit_routine(function_name);
  
  Call_depth++;
  assert(Call_depth>0);

  remember_place(parent, working_node, placeholder);	/* unlink child. */

  if (race_on() && 1 == Call_depth && In_function_body)
    {
      gct_node temp = GCT_NULL_NODE;	/* temporary_id() */
      
      if (   void_type_node != self->gcc_type	/* function returns value */
	  && GCT_SIMPLE_STMT != parent->type)	/* Value is used. */
	{
	  temp = temporary_id(self, CLOSEST, FORCE, NULL, NULL, WANT_BASE_TYPE);
	  working_node = newtree(makeroot(GCT_SIMPLE_ASSIGN, "="),
				 copy(temp),
				 working_node,
				 gct_node_arg_END);
	}
      working_node = comma(race_call_expression(),
			   working_node,
			   race_reenter_expression(),
			   gct_node_arg_END);
      if (temp)
	{
	  working_node = comma(working_node, copy(temp), gct_node_arg_END);
	  free(temp);
	}
    }
  
  if (needs_writelog && !GCT_FUNCALL_HAS_ARGS(self))
    {
      working_node = comma(make_logcall("gct_writelog"),
			   working_node,
			   gct_node_arg_END);
    }
  
  if (call_on() && gct_outside_macro_p(self->first_char))
    {
      call_map(Gct_next_index, self, function_name,
	       DECL_PRINT_NAME(current_function_decl), FIRST);
      working_node = comma(make_unconditional_incr(Gct_next_index),
			   working_node,
			   gct_node_arg_END);
      Gct_next_index++;
    }
  
  replace(parent, working_node, placeholder);

  /*
   * Do normal instrumentation: note that first argument is unused.  Note
   * that, by using self, we avoid incorrectly instrumenting all the code
   * that we just added.
   */
  i_std_descend(GCT_NULL_NODE, self);

  if (needs_writelog && GCT_FUNCALL_HAS_ARGS(self))
    {
      standard_add_writelog(self, GCT_FUNCALL_LAST_ARG(self));
    }
  free(function_name);

  Call_depth--;
}


/*
 * Add race and multi instrumentation for RETURN statements.
 * Add gct_writelog if we're returning from main.
 *
 * MULTI:
 *
 * We must take care to notice whether multi-conditional coverage is
 * required before instrumenting the returned expression.
 * Instrumentation may turn the returned expression into a
 * comma-expression, which does not look boolean.
 *
 * RACE:
 *
 * All returns are preceded by the GCT_RACE_GROUP_EXIT macro:
 *
 * 	{GCT_RACE_GROUP_EXIT(5); return f(3);}
 *
 * We must prevent further race instrumentation, in particular around
 * function calls in the returned expression.  This is done by
 * incrementing the Call_depth.  That might seem to be a kludge, but,
 * uh... it's because returning is calling the continuation!  Yeah!
 * That's the ticket!
 *
 * Thus, the return expression is not "in" the race group, exactly as
 * initializers are not.  An alternative would be to handle the two types
 * of returns differently:
 *
 * 	return;
 *
 * becomes
 *
 * 	{GCT_RACE_GROUP_EXIT(5); return;}
 *
 * whereas
 *
 * 	return <expr>
 *
 * becomes
 *
 * 	return (_G343 = <expr>, GCT_RACE_GROUP_EXIT(5), _G343);
 *
 * I don't see that being a win; perhaps even a slight loss, since
 * locking problems are likely to be restricted to the code before the
 * return, so race-measurement should as well.
 */
void 
i_std_return(parent, self)
     gct_node parent;
     gct_node self;
{
  gct_node orig_expr = GCT_RETURN_EXPR(self);
  gct_node newcompound;	/* The compound we build */
  gct_node placeholder; /* Where it's to go */
  int multi_on_p = assignish_multi_on(self, orig_expr);
  int starting_index = Gct_next_index;
  int needs_writelog =
    add_writelog_on() && gct_entry_routine(DECL_PRINT_NAME(current_function_decl));
  int needs_writelog_stmt = needs_writelog &&
    (GCT_NULL_EXPR == GCT_RETURN_EXPR(self)->type);
  int needs_expr_writelog = needs_writelog &&
    (GCT_NULL_EXPR != GCT_RETURN_EXPR(self)->type);
  
  if (race_on() || needs_writelog_stmt)	/* Not affected by macros. */
    {
      remember_place(parent, self, placeholder);
      newcompound = compound(self, gct_node_arg_END);
      replace(parent, newcompound, placeholder);

      /* The order in which these are added does not matter. */
      if (race_on())
	{
	  gct_add_before(&(newcompound->children), self, race_return_statement());
	}
      if (needs_writelog_stmt)
	{
	  gct_add_before(&(newcompound->children), self,
			 make_simple_statement(make_logcall("gct_writelog")));
	}
    }

  if (multi_on_p) 
    {
      multi_map(Gct_next_index++, self, self->text, FIRST);
      map_placeholder(Gct_next_index++);
    }

  Call_depth++;
  DO_INSTRUMENT(self, orig_expr);
  Call_depth--;

  if (multi_on_p)
    {
      standard_binary_test(self, GCT_RETURN_EXPR(self), starting_index);
    }

  if (needs_expr_writelog)
    {
      standard_add_writelog(self, GCT_RETURN_EXPR(self));
    }
}

/*
 * Relational instrumentation typically creates three tests:
 *
 * From
 * 	left < right
 *
 * We need (worst case)
 *
 * 	Tleft=left, Tright=right,
 * 	<test1>(Tleft, Tright),	<test2>(Tleft, Tright), <test3>(Tleft, Tright),
 * 	Tleft < Tright
 *
 * The particular tests depend on the operator.  Instrumentation is all
 * in accord with the DESIGN at the head of this file.
 *
 * <, >, <=, >= are all instrumented.  == and != just print a 
 * warning if their value cannot be used.  (This warning is printed even
 * if relational operator coverage isn't turned on.)
 */

void 
i_std_relational(parent, self)
     gct_node parent;
     gct_node self;
{
  int first_index;	/* Index of first instrumentation of this node. */
  int relational_on_p = relational_on() && gct_outside_macro_p(self->first_char);

  /*
   * No relational tests for void types.  Testing both sides because not
   * all compilers complain about void comparisons to non-void.
   */
  if  (   VOID_POINTER(GCT_OP_LEFT(self)->gcc_type)
       || VOID_POINTER(GCT_OP_RIGHT(self)->gcc_type))
    {
      relational_on_p = 0;
    }

  /*
   * With some compilers, additions to enum types are disallowed, causing
   * compile errors with GCT's instrumentation.  Note:  enumerated
   * constants are indistinguishable from integers, so this test is not
   * infallible.
   */
  if (   (   ENUMERAL_TYPE == TREE_CODE(GCT_OP_LEFT(self)->gcc_type)
	  || ENUMERAL_TYPE == TREE_CODE(GCT_OP_RIGHT(self)->gcc_type))
      && OFF == gct_option_value(OPT_ENUM_RELATIONAL))
    {
      relational_on_p = 0;
    }

  /* Issue this warning regardless of whether instrumentation is turned on. */
  if (	 (GCT_EQUALEQUAL == self->type || GCT_NOTEQUAL == self->type)
      && GCT_SIMPLE_STMT == parent->type)
    {
      /* Error routine's saved line number is that generated during lexing --
	 at this point, it's the line number of the routine's closing brace. */
      warning("(really line %d) \'<op> %s <op>;\' can have no effect.",
	      self->lineno, self->text);
    }

  DO_INSTRUMENT(self, GCT_OP_LEFT(self));
  
  /* We have to use up our indices here before we process lexically-later
     subtrees. */
  if (relational_on_p)
    {
      first_index = Gct_next_index;
      switch(self->type)
	{
	case GCT_LESS:
	  operator_map(Gct_next_index++,
		       self, "might be >. (L!=R)", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be <=. (L==R)", DUPLICATE);
	  operator_map(Gct_next_index++, self,
		       "needs boundary L==R-1.", DUPLICATE);
	  break;
	case GCT_GREATER:
	  operator_map(Gct_next_index++,
		       self, "might be <. (L!=R)", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be >=. (L==R)", DUPLICATE);
	  operator_map(Gct_next_index++, self,
		       "needs boundary L==R+1.", DUPLICATE);
	  break;
	case GCT_LESSEQ:
	  operator_map(Gct_next_index++,
		       self, "might be >=. (L!=R)", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be <. (L==R)", DUPLICATE);
	  operator_map(Gct_next_index++, self,
		       "needs boundary L==R+1.", DUPLICATE);
	  break;
	case GCT_GREATEREQ:
	  operator_map(Gct_next_index++,
		       self, "might be <=. (L!=R)", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be >. (L==R)", DUPLICATE);
	  operator_map(Gct_next_index++, self,
		       "needs boundary L==R-1.", DUPLICATE);
	  break;
	case GCT_EQUALEQUAL:
	case GCT_NOTEQUAL:
	  /* No instrumentation. */
	  break;
	default:
	  error("Unknown i_std_relational type.");
	  abort();
	  break;
	}
    }

  DO_INSTRUMENT(self, GCT_OP_RIGHT(self));

  if (relational_on_p)
    {
      gct_node left, right;	/* Short-hand for left and right expressions. */
      gct_node left_temp;	/* Temporary for left-hand side of expression. */
      gct_node right_temp;	/* Temporary for right-hand side of expression. */
      gct_node tests = GCT_NULL_NODE;	/* The tests we'll add. */
      gct_node placeholder;	/* We unlink self from parent. */
      gct_node new_self;	/* The instrumented version of self. */
  
      /* Take apart the relational tree. */
      left = GCT_OP_LEFT(self);
      gct_remove_node(&(self->children), left);
      left_temp = temporary_id(left, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
      
      right = GCT_OP_RIGHT(self);
      gct_remove_node(&(self->children), right);
      right_temp = temporary_id(right, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
      
      /* Build the tests. */
      switch(self->type)
	{
	case GCT_LESS:
	case GCT_GREATER:
	case GCT_LESSEQ:
	case GCT_GREATEREQ:
	  /* The first two tests are the same for all cases. */
	  /* left != right rules out < for >, <= for >=, etc. */
	  add_test(&tests,
		   (make_probe(first_index++,
			       newtree(makeroot(GCT_NOTEQUAL, "!="),
				       copy(left_temp),
				       copy(right_temp),
				       gct_node_arg_END))));
	  /* left == right rules out <= for <, etc. */
	  add_test(&tests,
		   make_probe(first_index++,
			      newtree(makeroot(GCT_EQUALEQUAL, "=="),
				      copy(left_temp),
				      copy(right_temp),
				      gct_node_arg_END)));
	  {
	    /*
	     * The "almost true" and "almost false" tests are particular to the
	     * operator type:
	     * 	<:  a < b && (a+1 >= b) 
	     * 	>:  a > b && (a-1 <= b) 
	     * 	<=: a-1 <= b && a > b 
	     * 	>=: a+1 >= b && a < b
	     */

	    gct_node op1, op2;   /* Tests have two comparisons. */
	    gct_node op1_left, op1_right, op2_left, op2_right;

	    /*
	     * First operator is always a copy of SELF. Right-hand sides
	     * are always copies of the right_temp. 
	     */
	    op1 = makeroot(self->type, self->text);
	    op1_right = copy(right_temp);
	    op2_right = copy(right_temp);

	    switch(self->type)
	      {
	      case GCT_LESS:
		op1_left = copy(left_temp);
		op2 = makeroot(GCT_GREATEREQ, ">=");
		op2_left = newtree(makeroot(GCT_PLUS, "+"),
				  copy(left_temp),
				  epsilon(left_temp),
				  gct_node_arg_END);
		break;
	      case GCT_GREATER:
		op1_left = copy(left_temp);
		op2 = makeroot(GCT_LESSEQ, "<=");
		op2_left = newtree(makeroot(GCT_MINUS, "-"),
				  copy(left_temp),
				  epsilon(left_temp),
				  gct_node_arg_END);
		break;
	      case GCT_LESSEQ:
		op1_left = newtree(makeroot(GCT_MINUS, "-"),
				  copy(left_temp),
				  epsilon(left_temp),
				  gct_node_arg_END);
		op2 = makeroot(GCT_GREATER, ">");
		op2_left = copy(left_temp);
		break;
	      case GCT_GREATEREQ:
		op1_left = newtree(makeroot(GCT_PLUS, "+"),
				  copy(left_temp),
				  epsilon(left_temp),
				  gct_node_arg_END);
		op2 = makeroot(GCT_LESS, "<");
		op2_left = copy(left_temp);
		break;
	      }
	    
	    add_test(&tests,
		     make_probe(first_index++,
				newtree(makeroot(GCT_ANDAND, "&&"),
					newtree(op1, op1_left, op1_right, gct_node_arg_END),
					newtree(op2, op2_left, op2_right, gct_node_arg_END),
					gct_node_arg_END)));
	  }
	  break;
	case GCT_EQUALEQUAL:
	case GCT_NOTEQUAL:
	  /* No tests. */
	  break;
	default:
	  error("Unknown i_std_relational type.");
	  abort();
	  break;
	}
      
      /* Conglomerate everything into a new tree. */
      remember_place(parent, self, placeholder);
      
      new_self = 
	comma(SETTER(left_temp, left),
	      SETTER(right_temp, right),
	      tests,
	      newtree(self,
		      VALUE(left_temp, left),
		      VALUE(right_temp, right),
		      gct_node_arg_END),
	      gct_node_arg_END);
      replace(parent, new_self, placeholder);
      
      /* Clean up */
      free_temp(right_temp, right);
      free_temp(left_temp, left);
    }
}


/*
 * Boolean-operator instrumentation typically creates two tests:
 *
 * From
 *   left || right
 *
 * We get (worst case)
 *   (Tleft=left, <test1>(Tleft), Tleft) || (Tright=right, <test2>(Tright), Tright)
 *
 * Instrumentation of the subexpressions is done by recursive calls to
 * DO_INSTRUMENT.  The left subexpression must have its mapfile entries
 * before SELF.  Note that self is not changed in place.
 *
 * Each mapfile entry contains a hint about what condition was
 * instrumented.  That hint contains the leftmost operand in the
 * condition and the condition's nesting level.
 *
 * 1.  The leftmost operand must be found before the left subexpression
 * is instrumented, since that instrumentation may rearrange the tree.
 * 2.  The nesting level is stored in a static, recursively incremented
 * variable.  Note that, in (a || (b < (a && c))), there are two
 * multicondition nesting levels, though the tree has three levels of
 * operators.  I think that's reasonable.
 */

void 
i_std_boolean(parent, self)
     gct_node parent;
     gct_node self;
{
  int first_index;	/* Index of first instrumentation of this node. */
  char *left_name, *right_name;		/* Names for multiconditional reporting. */
  static int nesting_level = 0;	/* For level info in mapfile messages. */
  int multi_on_p = multi_on() && gct_outside_macro_p(self->first_char);

  nesting_level++;
  if (multi_on_p)	/* Needs be done before left side instrumented. */
    {
      left_name = make_leftmost_name(GCT_OP_LEFT(self), nesting_level);
      right_name = make_leftmost_name(GCT_OP_RIGHT(self), nesting_level);
    }
  
  DO_INSTRUMENT(self, GCT_OP_LEFT(self));

  if (multi_on_p)
    {
      first_index = Gct_next_index;
      multi_map(Gct_next_index++, self, left_name, FIRST);
      map_placeholder(Gct_next_index++);
      multi_map(Gct_next_index++, self, right_name, DUPLICATE);
      map_placeholder(Gct_next_index++);
    }
  
  DO_INSTRUMENT(self, GCT_OP_RIGHT(self));
  
  if (multi_on_p)
    {
      standard_binary_test(self, GCT_OP_LEFT(self), first_index);
      standard_binary_test(self, GCT_OP_RIGHT(self), first_index+2);
      free(left_name);
      free(right_name);
    }
  
  nesting_level--;
}

/*
 * Assignment instrumentation (multicondition) is very standard:
 *
 * 	left = right
 *
 * becomes
 *
 * 	instrumented-left = (T = instrumented-right, _G2(#, T), T)
 *
 * In standard instrumentation, operands are never instrumented.
 * Therefore, instrumented-left will still be the same node (perhaps with
 * chanced childen), and that node will still be an lvalue.
 *
 * We must take care to notice whether multi-conditional coverage is
 * required before instrumenting the right-hand expression.
 * Instrumentation may turn the returned expression into a
 * comma-expression, which does not look boolean.
 */
void 
i_std_assign(parent, self)
     gct_node parent;
     gct_node self;
{
  int first_index;	/* Index of first instrumentation of this node. */
  int multi_on_p = assignish_multi_on(self, GCT_OP_RIGHT(self));

  DO_INSTRUMENT(self, GCT_OP_LEFT(self));

  if (multi_on_p)
    {
      char *name = (char *) xmalloc(strlen(self->text) + 20);
      first_index = Gct_next_index;
      sprintf(name, "%s expression", self->text);
      multi_map(Gct_next_index++, self, name, FIRST);
      map_placeholder(Gct_next_index++);
      free(name);
    }
  
  DO_INSTRUMENT(self, GCT_OP_RIGHT(self));
  
  if (multi_on_p)
    {
      standard_binary_test(self, GCT_OP_RIGHT(self), first_index);
    }
}

/*
 * IF instrumentation is very standard.
 *
 * 	if (test) then-stmt;
 *
 * becomes
 *
 * 	if (T=instrumented-test, _G2(#, T), T) instrumented-then-stmt
 */

void 
i_std_if(parent, self)
     gct_node parent;
     gct_node self;
{
  int branch_on_p = branch_on() && gct_outside_macro_p(self->first_char);
  int starting_index = Gct_next_index;

  /* Emit mapfile entry before instrumenting children. */
  if (branch_on_p)
    {
      branch_map(Gct_next_index++, self, FIRST);
      map_placeholder(Gct_next_index++);
    }
  
  i_std_descend(parent, self);	/* Instrument children. */

  if (branch_on_p)
    {
      standard_binary_test(self, GCT_IF_TEST(self), starting_index);
    }
}

/*
 * Question-mark instrumentation is normal:
 *
 *   test ? then-expr : else-expr
 *
 * becomes
 *
 *   (T=instrumented-test, _G2(#, T), T) ?
 * 	instrumented-then-expr : instrumented-else-expr
 *
 * Note that self is not changed.
 * The ? is the point of instrumentation, so the test must be instrumented
 * before mapfile entries are generated for SELF.
 */

void 
i_std_quest(parent, self)
     gct_node parent;
     gct_node self;
{
  int branch_on_p = branch_on() && gct_outside_macro_p(self->first_char);

  DO_INSTRUMENT(self, GCT_QUEST_TEST(self));

  /* Instrumentation officially takes place at the ? operator. */
  if (branch_on_p)
    {
      int starting_index = Gct_next_index;

      /* Mapfile entry */
      branch_map(Gct_next_index++, self, FIRST);
      map_placeholder(Gct_next_index++);

      /* Replace the just-instrumented test. */
      standard_binary_test(self, GCT_QUEST_TEST(self), starting_index);
    }

  /* Instrument remainder. */
  DO_INSTRUMENT(self, GCT_QUEST_TRUE(self));
  DO_INSTRUMENT(self, GCT_QUEST_FALSE(self));
}

/*
 * While statements may be instrumented by branch or loop.
 * Instrumentation is straightforward. All the complexity is in the
 * utility routine add_loop_test().   
 *
 * When branch and loop instrumentation are on, there are calls to
 * branch_map and loop_map.  Unlike, for example, i_std_relational, the
 * second map-function call is not labelled as a DUPLICATE.  This is
 * because different names are used in the mapfile.  See gct-mapfil.c for more.
 *
 */

void 
i_std_while(parent, self)
     gct_node parent;
     gct_node self;
{
  int outside_macro_p = gct_outside_macro_p(self->first_char);
  int loop_on_p = loop_on() && outside_macro_p;
  int branch_on_p = branch_on() && outside_macro_p;
  
  int starting_index = Gct_next_index;		/* First instrumentation */
  int loop_index;				/* First loop instrumentation */

  /* Emit mapfile entries before instrumenting test expression, which may be on a
     different line. */
  if (branch_on_p)
    {
      branch_map(Gct_next_index++, self, FIRST); /* Name used is "while". */
      map_placeholder(Gct_next_index++);
    }
  if (loop_on_p)
    {
      loop_index = Gct_next_index;
      loop_map(Gct_next_index, self, FIRST);	/* Name used is "loop" */
      Gct_next_index++;
      map_placeholder(Gct_next_index++);
      map_placeholder(Gct_next_index++);
      map_placeholder(Gct_next_index++);
    }
  
  i_std_descend(parent, self);	/* Instrument children. */

  if (branch_on_p || loop_on_p)
    {
      gct_node while_test = GCT_WHILE_TEST(self);
      gct_node test_temp = loop_binary_test(self, while_test,
					    starting_index, branch_on_p);
      
      if (loop_on_p)
	{
	  add_loop_test(parent, self, GCT_WHILE_TEST(self), test_temp, loop_index);
	}
      free_temp(test_temp, while_test);
    }
}

/*
 * Do instrumentation is almost identical to while.  Note that the
 * instrumentation point is the DO, the first token in the statement.
 */

void 
i_std_do(parent, self)
     gct_node parent;
     gct_node self;
{
  int outside_macro_p = gct_outside_macro_p(self->first_char);
  int loop_on_p = loop_on() && outside_macro_p;
  int branch_on_p = branch_on() && outside_macro_p;
  
  int starting_index = Gct_next_index;		/* First instrumentation */
  int loop_index;				/* First loop instrumentation */

  /* Emit mapfile entries before instrumenting test expression, which may be on a
     different line. */
  if (branch_on_p)
    {
      branch_map(Gct_next_index++, self, FIRST);	/* Name used is "do" */
      map_placeholder(Gct_next_index++);
    }
  if (loop_on_p)
    {
      loop_index = Gct_next_index;
      loop_map(Gct_next_index, self, FIRST);		/* Name used is "loop" */
      Gct_next_index++;
      map_placeholder(Gct_next_index++);
      map_placeholder(Gct_next_index++);
      map_placeholder(Gct_next_index++);
    }
  
  i_std_descend(parent, self);	/* Instrument children. */

  if (branch_on_p || loop_on_p)
    {
      gct_node do_test = GCT_DO_TEST(self);
      gct_node test_temp = loop_binary_test(self, do_test,
					    starting_index, branch_on_p);
      
      if (loop_on_p)
	{
	  add_loop_test(parent, self, GCT_DO_TEST(self), test_temp, loop_index);
	}
      free_temp(test_temp, do_test);
    }
}

/*
 * FOR loops
 *
 * There is some complexity involved in the case of FOR statements with
 * implicit tests [for (;;)].  
 *
 * 1.  We omit branch coverage, since the test always is taken TRUE.
 * 2.  We do NOT omit loop coverage -- most such for statements have
 * breaks and thus go 1 or more-than-one time.  It is even possible
 * for them to go zero times, in the case of a goto into the loop (e.g.,
 * Duff's device).
 *
 * The mechanism is to replace the null test with a constant 1.  Since
 * that constant will be instrumented (in loop coverage), it has to have
 * a location (character number in file) and a type.
 *
 * Other than this, FOR loops are the same as WHILE and DO loops.
 */
void 
i_std_for(parent, self)
     gct_node parent;
     gct_node self;
{
  int outside_macro_p = gct_outside_macro_p(self->first_char);
  int loop_on_p = loop_on() && outside_macro_p;
  int branch_on_p = branch_on() && outside_macro_p;
  
  int starting_index = Gct_next_index;		/* First instrumentation */
  int loop_index;				/* First loop instrumentation */

  /* Retrieve the test, replacing an empty test with "1". */
  if (GCT_NULL_EXPR == GCT_FOR_TEST(self)->type)
    {
      gct_node for_test = GCT_FOR_TEST(self);
      gct_node placeholder;		/* For remember_place. */

      /* We must remember to give the imaginary test a location and type. */
      gct_node replacement = makeroot(GCT_CONSTANT, "1");
      replacement->first_char = for_test->first_char;
      replacement->gcc_type = integer_type_node;
      
      remember_place(self, for_test, placeholder);
      replace(self, replacement, placeholder);
      branch_on_p = 0;
    }

  /* Emit mapfile entries before instrumenting test expression */
  if (branch_on_p)
    {
      branch_map(Gct_next_index++, self, FIRST);	/* Name used is "for" */
      map_placeholder(Gct_next_index++);
    }
  if (loop_on_p)
    {
      loop_index = Gct_next_index;
      loop_map(Gct_next_index, self, FIRST);		/* Name used is "loop" */
      Gct_next_index++;
      map_placeholder(Gct_next_index++);
      map_placeholder(Gct_next_index++);
      map_placeholder(Gct_next_index++);
    }
  
  i_std_descend(parent, self);	/* Instrument children. */

  if (branch_on_p || loop_on_p)
    {
      gct_node for_test = GCT_FOR_TEST(self);
      gct_node test_temp = loop_binary_test(self, for_test,
					    starting_index, branch_on_p);
      if (loop_on_p)
	{
	  add_loop_test(parent, self, GCT_FOR_TEST(self), test_temp, loop_index);
	}
      free_temp(test_temp, for_test);
    }
}

/*
 * Switches must be handled oddly, because they're not very structured:
 * a case or default can occur anywhere in the statement following the
 * switch test.
 *
 * (The switch test doesn't even need to be a compound statement.  We
 * handle that by ignoring it.  There's one case in which we don't behave
 * correctly: if instrumentation of a simple-statement switch body wrapped
 * the statement in a compound statement (as loop instrumentation does,
 * for example), an "erroneous" default will be added to that new
 * compound statement.  Of course, if the switch is ever taken, the
 * default will be taken, so the incorrectness is harmless.  Anyway, I'm
 * sure no C program uses simple-statement switches; at least some
 * compilers complain about them.)
 *
 * Consequently, switches can't be handled like other statements: add the
 * instrumentation at the instrumentation point, a readily-available
 * token.  The instrumentation points are numerous (the cases and
 * defaults) and hard to find.  Instead, we keep a stack of seen
 * switches.  Our job is to set up the top of the stack, call
 * DO_INSTRUMENT on the switch's statement, and then pop the top of the
 * stack.  Whenever a case or default is seen, it uses the top of the
 * stack to decide what to do.
 *
 * We do a little bit of instrumentation.  The code to add comes from
 * utility routines, but we do the work of adding it.
 *
 * 1. When the switch is taken, only the first case should increment an
 * entry in the GCT_table - fallthroughs should not.  So the switch test
 * is instrumented to set a per-switch variable that says the first case
 * has not been seen yet.  Each case and the default tests that variable.
 *
 * This instrumentation is immune to macros, since the variable must be
 * set if any of the cases is outside a macro.  This instrumentation does
 * not produce any mapfile entries.  If branch instrumentation is not
 * turned on, we don't do any instrumentation; it can't be turned on
 * before the cases and default are seen.
 *
 * 2. If the closing curly brace of the switch is seen without an
 * explicit default, we add the explicit default.  The curly brace must
 * be outside a macro and branch instrumentation must be on.
 *
 * Obscure note:  You'll notice that even the implicit default zeros the
 * "first case not seen" variable.  This is so that that variable is true
 * only if the statement was entered via the switch.  Were it not set,
 * the switch could exit through the implicit default, then a later goto
 * into the switch would cause the first case encountered to think it was
 * taken.
 */


void 
i_std_switch(parent, self)
     gct_node parent;
     gct_node self;
{
  int doing_instrumentation = branch_on();
  DO_INSTRUMENT(self, GCT_SWITCH_TEST(self));

  if (doing_instrumentation)
    {
      gct_node switch_test;		/* The test for this switch. */
      gct_node new_switch_test;		/* Newly instrumented value. */
      gct_node placeholder;		/* For remember_place. */

      push_switch();
      switch_test = GCT_SWITCH_TEST(self);
      remember_place(self, switch_test, placeholder);
      new_switch_test = comma(switch_needed_init(), switch_test, gct_node_arg_END);
      replace(self, new_switch_test, placeholder);
    }
  
  DO_INSTRUMENT(self, GCT_SWITCH_BODY(self));
  
  if (doing_instrumentation)
    {
      if (!switch_default_seen())
	{
	  if (GCT_COMPOUND_STMT != GCT_SWITCH_BODY(self)->type)
	    {
	      warning("Switch statement is a simple statement; no default added.\n");
	    }
	  else 
	    {
	      gct_node closing_brace = GCT_LAST(GCT_SWITCH_BODY(self)->children);
	      int outside_macro = gct_outside_macro_p(closing_brace->first_char);

	      /* Always clear the "case needed" variable; maybe add instrumentation */
	      gct_node new_default = newtree(makeroot(GCT_DEFAULT, "default"),
					     switch_case_test(Gct_next_index,
							      outside_macro),
					     gct_node_arg_END);
	      gct_add_before(&(GCT_SWITCH_BODY(self)->children),
			     closing_brace,
			     new_default);
	  
	      if (outside_macro)	/* If instrumentation added. */
		{
		  /* The default goes on same line as closing brace. */
		  new_default->lineno = closing_brace->lineno;
		  new_default->filename = closing_brace->filename;
		  branch_map(Gct_next_index++, new_default, FIRST);
		}
	    }
	}
      pop_switch();
    }
}

/*
 * Cases are handled much like the implicit defaults in i_std_switch:
 *
 * 1. Nothing happens unless branch_on() is true.
 * 2. Macros suppress instrumentation, but they do not suppress zeroing
 * the "case not seen" variable.
 */

void 
i_std_case(parent, self)
     gct_node parent;
     gct_node self;
{
  int first_index = Gct_next_index;	/* Index of this probe. */
  int doing_instrumentation;	/* Whether branches are instrumented. */
  int outside_macro;		/* Avoid repeated calls to gct_outside_macro_p. */

  /* Saving value avoids problems were self to be replaced. */
  outside_macro = gct_outside_macro_p(self->first_char);
  doing_instrumentation = branch_on();
  
  if (doing_instrumentation && outside_macro)
    {
      branch_map(Gct_next_index++, self, FIRST);
    }
  DO_INSTRUMENT(self, GCT_CASE_STMT(self));

  if (doing_instrumentation)
    {
      gct_node case_stmt;	/* Statement after "case: " */
      gct_node placeholder;		/* For remember_place. */
      gct_node new_compound;		/* Root of statement we build. */
	  
      case_stmt = GCT_CASE_STMT(self);
      remember_place(self, case_stmt, placeholder);
      new_compound = compound(switch_case_test(first_index, outside_macro),
			      case_stmt,
			      gct_node_arg_END);
      replace(self, new_compound, placeholder);
    }
}

/*
 * Default instrumentation is just like case instrumentation, with one
 * addition:  the code notes that a default was seen, so that an implicit
 * default is not generated by i_std_switch.
 */

void 
i_std_default(parent, self)
     gct_node parent;
     gct_node self;
{
  int first_index = Gct_next_index;	/* Index of this probe. */
  int doing_instrumentation;	/* Whether branches are instrumented. */
  int outside_macro;		/* Avoid repeated calls to gct_outside_macro_p. */

  /* Saving value avoids problems were self to be replaced. */
  outside_macro = gct_outside_macro_p(self->first_char);
  doing_instrumentation = branch_on();
  
  if (doing_instrumentation && outside_macro)
    {
      branch_map(Gct_next_index++, self, FIRST);
    }
  DO_INSTRUMENT(self, GCT_DEFAULT_STMT(self));

  if (doing_instrumentation)
    {
      gct_node default_stmt;	/* Statement after "default: " */
      gct_node placeholder;		/* For remember_place. */
      gct_node new_compound;		/* Root of statement we build. */

      now_switch_has_default();
      default_stmt = GCT_DEFAULT_STMT(self);
      remember_place(self, default_stmt, placeholder);
      new_compound = compound(switch_case_test(first_index, outside_macro),
			      default_stmt,
			      gct_node_arg_END);
      replace(self, new_compound, placeholder);
    }
}


