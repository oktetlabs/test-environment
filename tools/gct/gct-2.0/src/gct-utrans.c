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
 * This code was originally written in a mad dash as a prototype.  The
 * code has been "productized" into gct-strans.c, but weak-mutation
 * coverage was left behind.  I've retained this structure as a service
 * to those who want to use weak mutation coverage, but I'm not terribly
 * proud of this code.  Here are some old notes about cleaning it up:
 *
 * 2.  "Weak sufficiency" requires that either side of a relational
 * statement be able to use the value of the other side.  This is done in
 * the wrong way:  by generating temporaries in the parent procedure and
 * passing them down to the children.  This results (directly) in the
 * generation of many unused temporaries.  Indirectly, it results in
 * convoluted and hard-to-understand code here, and also the generation
 * of convoluted code in the output.  I originally thought that would be
 * acceptable -- let the optimizer optimize the code back to its original
 * form.  That's was arguably the right thing to do for a prototype;
 * however, in reality, not all optimizers are created equal and some of
 * them get *quite* bogged down on GCT output.
 *
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
#include "gct-macros.h"


/*
 * Instrumentation is complicated.  In general, we begin with a PARENT
 * node and are instrumenting its child, called SELF.  This child
 * recursively instruments its children; at any given time, the one being
 * instrumented is CHILD.
 *
 * Instrumentation must be performed such that the map file is in
 * increasing order.  If there are two instrumentation probes on a line,
 * the first one must be present before the second.  This can present
 * problems for inline operators, like 
 *
 * 		A+B
 *
 * because the instrumentation for + may depend on the instrumentation
 * for B.  Typically, A will be instrumented, slots for + will be
 * reserved, B will be instrumented, and the + expression will be rewritten.
 *
 * There are three classes of instrumentation functions:
 *
 * Class instrument:	this takes a parent and self and rewrites self.
 * Class expr_instrument:	this takes a variety of arguments and returns
 * 			several values that the caller assembles
 * 			appropriately.  (See below)
 * Class lvalue_instrument: Like expr_instrument, except that even more values
 * 			are returned.
 *
 * Before explaining the general structure of the routines, two terms:
 *
 * An expression is SIMPLE if
 * 1.  multiple evaluations always yield the same result.
 * 2.  The expression is small (temporary_id is the arbiter of this).
 *
 * As a general rule, we want to perturb the code as little as possible:
 * if an expression is not used for anything, it should appear unchanged
 * in the output.  This will make debugging tractable, but makes the
 * coding trickier.
 *
 * INSTRUMENT CLASS:	i_NAME(parent, self)
 *
 * SELF is instrumented in place.  The only responsibility of the caller
 * is not to use SELF again, since it may have been replaced by another
 * tree.  In particular, be careful when looping over the parent's children.
 *
 * EXPR_INSTRUMENT CLASS:
 * 	exp_NAME(parent, self, state, valuenode, setter_rh, tests)
 *
 * PARENT is self's original parent.
 * SELF is the node to instrument.  It must have been unlinked from PARENT.
 * 	(This is inefficient in many cases, since it will just be linked
 * 	back in, but will be more reliable.)
 * STATE tells how to test weak sufficiency (if the PARENT was a
 * 	relational operator node).
 * VALUENODE is a temporary variable that can be considered to hold the
 * 	value of SELF.  (The parent will build the code that ensures this.)
 * 	If SELF is simple, VALUENODE==SELF.
 *
 * The following are return values, passed as by-reference parameters:
 *
 * SETTER_RH is the right-hand side of the assignment statement that will
 * 	initialize VALUENODE.  It is the instrumented version of SELF.
 * TESTS contains a comma-list of tests.  These tests may or may not use
 * 	VALUENODE.  
 *
 * The splitting of return value into a setter part and a test part is
 * caused by weak mutation, which requires that SELF insert code between
 * the setter and test code from a particular CHILD.  In particular, it
 * must insert the SETTER code from the other child.  See exp_relational.
 *
 * The function also has a normal return value, for whose meaning see below.
 *
 *
 *
 *
 * The caller builds an expression from VALUENODE, SETTER_RH, and TESTS.
 * How it does so depends on these considerations:
 * 1.	Does the child need the setter to precede the tests?
 * 	This could mean one of two things:  that the SETTER_RH contains
 * 	assignment statements whose left-hand-sides are used in the
 * 	tests, or that the TESTS actually use the VALUENODE.
 * 	(This information is passed up to SELF in the return value.)
 *
 * 2	Does SELF need the setter to proceed its tests?  If not, more
 * 	easily understandable code will result from placing the setter
 * 	after the tests.
 * 	
 * 3.	Was CHILD simple?  If so, we do not want to use the setter_rh to
 * 	set it.
 *
 * EXAMPLES:
 * 	...
 *
 * LVALUE_INSTRUMENT CLASS:
 * 	lv_NAME(parent, self, state, originalvalue, setter_rh, tests, lvalue)
 *
 * The goal here is to take an expression like
 *
 * 	A = exp
 *
 * and convert it into
 *
 * 	original_a = A, <tests>, A = instrumented(exp).
 *
 * As above, though, if there is no instrumentation, we want this to
 * reduce to 
 *
 * 	A = exp
 *
 * This is quite like EXP_INSTRUMENT.  ORIGINALVALUE corresponds to 
 * VALUENODE.
 *
 *
 */

		/* SIMPLE INSTRUMENTATION UTILITIES */


/*
 * This may construct an assignment statement that gives temporary
 * TEMPVAR the value of SETTER_RH.  SETTER_FIRST tells this routine
 * whether the caller desires a simple value to use in tests.  SIMPLE
 * tells whether SETTER_RH is already simple, thus no assignment need be
 * made.
 */
#define SETTER(setter_first, simple, tempvar, setter_rh)	\
  ((setter_first) && !(simple) ?		\
      newtree(makeroot(GCT_SIMPLE_ASSIGN, "="), copy((tempvar)), (setter_rh), gct_node_arg_END) : \
      GCT_NULL_NODE)
	
/*
 * This constructs the value of a sub-expression.  SETTER has already
 * been called; it has either used the setter_lh side or not.  If it
 * didn't, we do, and vice-versa.  It might seem that we don't need the
 * value of simple; however, we must make sure to use the ACTUAL
 * setter_rh at least once, because copies do not retain line numbers.
 */
#define VALUE(setter_first, simple, tempvar, setter_rh)\
	((setter_first) && !(simple)? copy((tempvar)) : (setter_rh))

		    /* INSTRUMENTATION ROUTINES */

/*
 * These are the instrumentation routines that are easiest to use.  The
 * caller simply calls them.  The called routine does all the
 * instrumentation and tree editing; the caller simply accepts the results.
 */

/*
 * Instrument an expression, doing nothing if possible.
 *
 * This is the only case where a void-valued expression might be
 * instrumented, in exactly the case where this routine is called by
 * i_simple_statement for a call of a void function.  This special case
 * is handled here, rather than in the utility routines, to maximize the
 * chance of failure if this assumption is wrong.  (We want to know about it.)
 */
void
i_expr(parent, self)
     gct_node parent;
     gct_node self;
{
  gct_node setter_rh, tests;
  i_state state;
  gct_node temp;
  int tests_need_setter;
  gct_node new_self;
  gct_node placeholder;	/* for remember_place */
  int void_valued;	/* If self is a void-valued expression. */
  

  if (GCT_NULL_EXPR == self->type)	/* Null exprs have no gcc_type. */
    return;

  void_valued = (self->gcc_type == void_type_node);

  if (! void_valued)
    {
      temp = temporary_id(self, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
    }
  else
    {
      temp = GCT_NULL_NODE;
    }
  
  state = Default_state;

  remember_place(parent, self, placeholder);
  tests_need_setter = 
    DO_EXPR_INSTRUMENT(parent, self, state, temp, &setter_rh, &tests);

  new_self = comma(SETTER(tests_need_setter, temp==self, temp, setter_rh),
		   tests,
		   VALUE(tests_need_setter, temp==self, temp, setter_rh),
		   gct_node_arg_END);
  replace(parent, new_self, placeholder);
  if (!void_valued)
    {
      free_temp(temp, self);
    }
}

void
i_nothing(parent, self)
     gct_node parent;
     gct_node self;
{
  /* Do nothing. */
}


void
i_label(parent, self)
     gct_node parent;
     gct_node self;
{
  DO_INSTRUMENT(self, GCT_LABEL_STMT(self));
}


/*
 * General strategy:
 * 	Grovel through the argument list.  When variable names are
 * 	seen, inform the relevant routines.  When initializers are seen,
 * 	instrument them in place.
 *
 * Declarations are strings of OTHER tokens, punctuated by
 * IDENTIFIERS and possibly initializing expressions.  
 *
 * Curly braces are always ignored - when the opening brace is seen,
 * everything is ignored until a matching closing brace.  This means
 * no instrumentation is done for these cases:
 *
 * 1. Initialization of a structure
 * 	struct b = { 5 };
 * 2. GNU C compound-expressions:
 * 	int b = ({ int temp = 5; temp * 3;})
 *
 * In both of these cases, this is done to make my life simpler - I don't
 * have to parse compound initializers and I don't have to handle nested
 * declarations.  All I have to do is look for identifiers and equal signs.
 * Additional benefit:  I don't have to worry about contents of curly
 * braces in 
 *
 *   struct foo { int a; int b; };
 */

/*
 * In the GNU C compound-declaration case, we must guard against nested
 * declarations.  This variable is used.  i_compound_statement has the
 * responsibility.  Information set by the "lookup_decl" functions could
 * be used as well, but doesn't have quite the same meaning and isn't
 * quite as general a sanity check.
 */
static int Declaration_depth = 0;


void
i_declaration(parent, self)
     gct_node parent;
     gct_node self;
{
  gct_node identifier = GCT_NULL_NODE;
  gct_node rover;

  assert(0 == Declaration_depth);  Declaration_depth++;
  
  gct_temp_decl_init(self);
  rover = self->children;
  do	/* Loop depends on fact that there're at least 2 child nodes. */
    {
      gct_node work_rover = rover;	/* Saved value of rover. */
      rover=rover->next;		/* Rover is advanced to next
					 * node because work_rover may be
					 * instrumented in place. */
      switch(work_rover->type)
	{
	case GCT_IDENTIFIER:
	  /* Note that the next token is not necessarily an "=", even
	     if this variable is initialized. */
	  if (identifier)	/* Finished with this -- no initialization */
	    {
	      maybe_initialize(identifier, self);
	      gct_lookup_decl_finish(identifier->text);
	    }
	  identifier = work_rover;	/* Remember the identifier. */
	  gct_lookup_decl_init(identifier->text);
	  break;
	default:
	  if (work_rover->text && !strcmp(work_rover->text, "{"))
	  {
	      /* This is a struct or union; skip over it. */
	      rover = gct_find_later_match(work_rover)->next;
	  }
	  else if (work_rover->text && !strcmp(work_rover->text, "="))
	  {	  /* Thing following an equal sign is an initializer. */
	      gct_node equal_node = work_rover;	/* For later macro test, if needed. */
	    
	      assert(identifier);
	      work_rover = rover;
	      rover = rover->next;
	      if (ON == gct_option_value(OPT_SHOW_VISIBLE))
		{
		  fprintf(stderr, "Declaring %s:\n", identifier->text);
		  show_visible_variables(work_rover->gcc_type, "variable initialization", 0);
		}
	      if (ON == gct_option_value(OPT_TEST_TEMP))
		{
		  gct_node temp = temporary_id(work_rover, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
		  fprintf(stderr, "Declaring %s (closest, ref-ok)\n",
			  identifier->text);
		  fprintf(stderr, "Got %s\n", temp->text);
		  free_temp(temp, work_rover);
		  
		  temp = temporary_id(work_rover, OUTERMOST, FORCE, NULL, NULL, WANT_BASE_TYPE);
		  fprintf(stderr, "Declaring %s (outermost, force)\n",
			  identifier->text);
		  fprintf(stderr, "Got %s\n", temp->text);
		  free_temp(temp, work_rover);
		}
	      else if (work_rover->text && !strcmp(work_rover->text, "{"))
	        {
		  /* An aggregate initializer.  Skip it. */
		    rover = gct_find_later_match(work_rover)->next;
		}    
	      else if (! TREE_STATIC(gct_lookup_decl_var()))
		{
		  /*
		   * (Explaining the above test:)
		   * Can't instrument statics -- their initializers must
		   * be constant.  The code below converts ptr=0 to
		   * ptr=(ptrtype)0.  See also i_return.
		   */
		  int multi_on_p = assignish_multi_on(equal_node, work_rover);
		  int first_index;
		  
		  if (   GCT_CONSTANT == work_rover->type
		      && !strcmp("0", work_rover->text))
		    {
		      tree variable_type = TREE_TYPE(gct_lookup_decl_var());
		      char *make_decl();
		      char *decl = make_decl(variable_type, "", ARRAYS_AS_POINTERS);
		      char *cast = (char *) xmalloc(strlen(decl)+3);

		      sprintf(cast, "(%s)", decl);
		      free(decl);
		      gct_make_current_note(gct_misc_annotation(cast), equal_node);
		      work_rover->gcc_type = variable_type;
		    }

		  if (multi_on_p)
		    {
		      first_index = Gct_next_index;
		      multi_map(Gct_next_index++, self, "declaration", FIRST);
		      map_placeholder(Gct_next_index++);
		    }
		  
		  DO_INSTRUMENT(self, work_rover);

		  if (multi_on_p)
		    {
		      standard_binary_test(self, rover->prev, first_index);
		    }
		}
	      
	      gct_lookup_decl_finish(identifier->text);
	      identifier = GCT_NULL_NODE;
	    }
	  break;
	}
    }
  while (rover != self->children);
  if (identifier)
    {
      maybe_initialize(identifier, self);
      gct_lookup_decl_finish(identifier->text);
    }
  gct_temp_decl_finish(self);
  Declaration_depth--;
}


/*
 * For a compound statement, simply iterate through all the substatements
 * and transform them.  Compound expressons (GNU C ({..}) are handled the
 * same way).
 *
 * Compound expressions present an annoying problem:  a declaration may
 * be appear inside a compound expression that itself is the initializer
 * of a declaration:
 *
 * int x = ({int z; z = 1; z * z;});
 *
 *
 * Because the initial design assumed declarations would not be nested,
 * i_declaration is not reentrant.  Cleaning this up would probably make
 * for a better-written program, but I don't have time for that just now.
 * i_compound_statement, therefore, doesn't instrument nested compound
 * expressions.  It must register them with the "lookup" module, though,
 * so that GCT's correspondence of compound statements with GCC's
 * variable contours is maintained.
 *
 * This loss of instrumentation is irrelevant - such expressions are
 * almost always within macros.
 *
 * Special note:  GCC handles empty compound statements specially.  In
 * particular, they have no contours, so we must NOT descend into them.
 */
void
i_compound_statement(parent, self)
     gct_node parent;
     gct_node self;
{
  gct_node rover;
  
  if (!GCT_EMPTY_COMPOUND_STATEMENT(self))
    {
      gct_lookup_compound_init();
      if (ON == gct_option_value(OPT_SHOW_VISIBLE))	/* Test support */
	{
	  show_visible_variables((tree)0, "entry to compound statement", 0);
	}
      if (0 == Declaration_depth)	/* Not inside a declaration. */
	{
	  gct_temp_compound_init(self);
	  rover = self->children;
	  do       
	    {
	      /*
	       * This loop depends on fact that there're at least 2 child
	       * nodes, namely the curly braces.
	       */
	      rover=rover->next;
	      DO_INSTRUMENT(self, rover->prev);
	    }
	  while (rover != self->children);
	  gct_temp_compound_finish();
	}
      if (ON == gct_option_value(OPT_SHOW_VISIBLE))	/* Test support */
	show_visible_variables(0, "exit from compound statement", 0);
      gct_lookup_compound_finish();
    }
}

/* For a simple statement, simply instrument the statement's expression. */
void
i_simple_statement(parent, self)
     gct_node parent;
     gct_node self;
{
  gct_node child = GCT_SIMPLE_STMT_BODY(self);
  if (ON == gct_option_value(OPT_TEST_TEMP))
    {
      gct_node temp = temporary_id(child, CLOSEST, FORCE, NULL, NULL, WANT_BASE_TYPE);
      fprintf(stderr, "Simple statement (closest, force)\n");
      fprintf(stderr, "Got %s\n", temp->text);
      free_temp(temp, child);
      
      temp = temporary_id(child, OUTERMOST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
      fprintf(stderr, "Simple statement (outermost, ref-ok)\n");
      fprintf(stderr, "Got %s\n", temp->text);
      free_temp(temp, child);
    }
  else
    {
      i_expr(self, child);
    }
}

/* Instrument IF statements by instrumenting their tests and using the
   value as an argument to _G2. */

void
i_if(parent, self)
     gct_node parent;
     gct_node self;
{
  gct_node if_test = GCT_IF_TEST(self);	/* The test to instrument. */
  gct_node setter_rh, tests;	/* Results of same. */
  gct_node new_if_test;		/* Newly instrumented value. */
  int child_used;		/* Whether child needs its setter first. */
  gct_node test_temp;		/* Test variable in place of if_test. */
  gct_node placeholder;		/* For remember_place. */

				/* I use the variable if I instrument. */
  int i_use_test_temp = branch_on() && gct_outside_macro_p(self->first_char);
  int starting_index = Gct_next_index;

  remember_place(self, if_test, placeholder);
      
  test_temp = temporary_id(if_test, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  if (i_use_test_temp)
    {
      /* Emit before instrumenting test expression, which may be on a
	 different line. */
      branch_map(Gct_next_index++, self, FIRST);
      map_placeholder(Gct_next_index++);
    }
      
  child_used = DO_EXPR_INSTRUMENT(self, if_test, Default_state, test_temp, &setter_rh, &tests);
  new_if_test = comma(SETTER(child_used || i_use_test_temp, test_temp==if_test,
			     test_temp, setter_rh),
		      tests,
		      VALUE(child_used || i_use_test_temp, test_temp==if_test,
			    test_temp, setter_rh),
		      gct_node_arg_END);
      
  if (i_use_test_temp)
    {
      new_if_test = comma(new_if_test,
			  make_binary_probe(starting_index, copy(test_temp)),
			  copy(test_temp),
			  gct_node_arg_END);
    }
  free_temp(test_temp, if_test);
  replace(self, new_if_test, placeholder);
  
  DO_INSTRUMENT(self, GCT_IF_THEN(self));
  if (GCT_IF_HAS_ELSE(self))
    {
      DO_INSTRUMENT(self, GCT_IF_ELSE(self));
    }
}


/* LOOPS */					    
					    
void
i_while(parent, self)
     gct_node parent;
     gct_node self;
{
  gct_node while_test = GCT_WHILE_TEST(self);	/* The test to instrument. */
  gct_node setter_rh, tests;	/* Results of same. */
  gct_node new_while_test;		/* Newly instrumented value. */
  int child_used;		/* Whether child needs its setter first. */
  gct_node test_temp;		/* Test variable in place of while_test. */
  gct_node placeholder;		/* For remember_place. */
  int starting_index = Gct_next_index;	/* For all instrumentation */
  int loop_index;			/* For loop instrumentation. */

  int outside_macro_p = gct_outside_macro_p(self->first_char);
  int loop_on_p = loop_on() && outside_macro_p;
  int branch_on_p = branch_on() && outside_macro_p;
  int operator_on_p = operator_on() && outside_macro_p;
  int i_use_test_temp = loop_on_p || branch_on_p || operator_on_p;
  
  remember_place(self, while_test, placeholder);
      
  test_temp = temporary_id(while_test, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
      
  /* Emit mapfile entries before instrumenting test expression, which may be on a
     different line. */
  if (branch_on_p)
    {
      branch_map(Gct_next_index++, self, FIRST);
      map_placeholder(Gct_next_index++);
    }
  if (loop_on_p || operator_on_p)
    {
      loop_index = Gct_next_index;
      loop_map(Gct_next_index, self, FIRST);	/* Diff name than used above. */
      Gct_next_index++;
      map_placeholder(Gct_next_index++);
      map_placeholder(Gct_next_index++);
      map_placeholder(Gct_next_index++);
    }
        
  child_used = DO_EXPR_INSTRUMENT(self, while_test, Default_state, test_temp, &setter_rh, &tests);
  new_while_test = comma(SETTER(child_used || i_use_test_temp, test_temp==while_test,
				test_temp, setter_rh),
			 tests,
			 VALUE(child_used || i_use_test_temp, test_temp==while_test,
			       test_temp, setter_rh),
			 gct_node_arg_END);

  if (branch_on_p)
    {
      new_while_test = comma(new_while_test,
			     make_binary_probe(starting_index, copy(test_temp)),
			     copy(test_temp),
			     gct_node_arg_END);
    }

  /* Must replace now, because add_loop_test instruments in place. */
  replace(self, new_while_test, placeholder);

  if (loop_on_p || operator_on_p)
    {
      add_loop_test(parent, self, GCT_WHILE_TEST(self), test_temp, loop_index);
    }
      
  free_temp(test_temp, while_test);
  DO_INSTRUMENT(self, GCT_WHILE_BODY(self));
}

/*
 * do ... while.   Note that the instrumentation point is at the DO -- it
 * should probably be at the WHILE. 
 */

void
i_do(parent, self)
     gct_node parent;
     gct_node self;
{
  gct_node do_test = GCT_DO_TEST(self);		/* The test to instrument. */
  gct_node setter_rh, tests;	/* Results of same. */
  gct_node new_do_test;		/* Newly instrumented value. */
  int child_used;		/* Whether child needs its setter first. */
  gct_node test_temp;		/* Test variable in place of do_test. */
  gct_node placeholder;		/* For remember_place. */
  int starting_index = Gct_next_index;	/* For all instrumentation */
  int loop_index;			/* For loop instrumentation. */

  int outside_macro = gct_outside_macro_p(self->first_char);
  int loop_on_p = loop_on() && outside_macro;
  int branch_on_p = branch_on() && outside_macro;
  int i_use_test_temp = branch_on_p || loop_on_p;

  test_temp = temporary_id(do_test, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  if (branch_on_p)
    {
      /* Emit before instrumenting test expression, which may be on a
	 different line. */
      branch_map(Gct_next_index++, self, FIRST);
      map_placeholder(Gct_next_index++);
    }
  if (loop_on_p)
    {
      loop_index = Gct_next_index;
      loop_map(Gct_next_index, self, FIRST);	/* Diff name than used above. */
      Gct_next_index++;
      map_placeholder(Gct_next_index++);
      map_placeholder(Gct_next_index++);
      map_placeholder(Gct_next_index++);
    }

  DO_INSTRUMENT(self, GCT_DO_BODY(self));

  /* Note how remember_place goes after the DO_INSTRUMENT, which might
     change the structure of self. */
  remember_place(self, do_test, placeholder);
      
  child_used = DO_EXPR_INSTRUMENT(self, do_test, Default_state, test_temp, &setter_rh, &tests);
  new_do_test = comma(SETTER(child_used || i_use_test_temp, test_temp==do_test,
			     test_temp, setter_rh),
		      tests,
		      VALUE(child_used || i_use_test_temp, test_temp==do_test,
			    test_temp, setter_rh),
		      gct_node_arg_END);
        
  if (branch_on_p)
    {
      new_do_test = comma(new_do_test,
			  make_binary_probe(starting_index, copy(test_temp)),
			  copy(test_temp),
			  gct_node_arg_END);
    }
  
  /* Call replace now, because add_loop_test modifies in place. */
  replace(self, new_do_test, placeholder);
  if (loop_on_p)
    {
      add_loop_test(parent, self, GCT_DO_TEST(self), test_temp, loop_index);
    }
  
  free_temp(test_temp, do_test);
}

/*
 * There is some complexity involved in the case of FOR statements with
 * implicit tests [for (;;)].  
 *
 * 1.  We omit branch coverage, since the test always is taken TRUE.
 * 2.  We do NOT omit loop coverage -- most such for statements have
 * breaks and thus do go 1 or more-than-one time.  It is even possible
 * for them to go zero times, in the case of a goto into the loop (e.g.,
 * Duff's device).
 * 3.  To accomplish loop coverage, we add an implicit constant TRUE node.
 * We don't want that to be instrumented.  The only possible
 * instrumentation is substitution, and that is easily suppressed in the
 * passed-down state.
 *
 */
void
i_for(parent, self)
     gct_node parent;
     gct_node self;
{
  gct_node for_test;		/* The test to instrument. */
  gct_node setter_rh, tests;	/* Results of same. */
  gct_node new_for_test;		/* Newly instrumented value. */
  int child_used;		/* Whether child needs its setter first. */
  gct_node test_temp;		/* Test variable in place of for_test. */
  gct_node placeholder;		/* For remember_place. */
  int starting_index = Gct_next_index;	/* For all instrumentation */
  int loop_index;			/* For loop instrumentation. */
  int implicit_test = 0;		/* Is there no actual test? */
  i_state child_state;		/* State to pass down. */

  int outside_macro = gct_outside_macro_p(self->first_char);
  int loop_on_p = loop_on() && outside_macro;
  int branch_on_p = branch_on() && outside_macro;
  int i_use_test_temp = 0;	/* Set below, because of implicit test. */
      
  /* Retrieve the test, replacing an empty test with "1". */
  for_test = GCT_FOR_TEST(self);
  if (GCT_NULL_EXPR == for_test->type)
    {
      /* Kludge:  we must remember to preserve the location of this
	 imaginary test. */
      gct_node replacement = makeroot(GCT_CONSTANT, "1");
      replacement->first_char = for_test->first_char;
      replacement->gcc_type = integer_type_node;
      
      remember_place(self, for_test, placeholder);
      replace(self, replacement, placeholder);
      for_test = GCT_FOR_TEST(self);
      implicit_test = 1;
      branch_on_p = 0;
    }

  i_use_test_temp = branch_on_p || loop_on_p;
  test_temp = temporary_id(for_test, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
        
  if (branch_on_p)
    {
      /* Emit before instrumenting test expression, which may be on a
	 different line. */
      branch_map(Gct_next_index++, self, FIRST);
      map_placeholder(Gct_next_index++);
    }
  if (loop_on_p)
    {
      loop_index = Gct_next_index;
      loop_map(Gct_next_index, self, FIRST);		/* Diff name than used above. */
      Gct_next_index++;
      map_placeholder(Gct_next_index++);
      map_placeholder(Gct_next_index++);
      map_placeholder(Gct_next_index++);
    }
      
  DO_INSTRUMENT(self, GCT_FOR_INIT(self));
      
  remember_place(self, for_test, placeholder);

  child_state = Default_state;
  child_state.no_substitutions = implicit_test;
  child_used = DO_EXPR_INSTRUMENT(self, for_test, child_state, test_temp, &setter_rh, &tests);
  new_for_test = comma(SETTER(child_used || i_use_test_temp, test_temp==for_test,
			      test_temp, setter_rh),
		       tests,
		       VALUE(child_used || i_use_test_temp, test_temp==for_test,
			     test_temp, setter_rh),
		       gct_node_arg_END);
      
  if (branch_on_p)
    {
      new_for_test = comma(new_for_test,
			   make_binary_probe(starting_index, copy(test_temp)),
			   copy(test_temp),
			   gct_node_arg_END);
    }

  /* Replace now, because add_loop_test modifies in place. */
  replace(self, new_for_test, placeholder);
  if (loop_on_p)
    {
      add_loop_test(parent, self, GCT_FOR_TEST(self), test_temp, loop_index);
    }
      
  DO_INSTRUMENT(self, GCT_FOR_INCR(self));
  DO_INSTRUMENT(self, GCT_FOR_BODY(self));
  free_temp(test_temp, for_test);
}

/* SWITCHES */

/*
 * The effects of having the "switch" keyword in a macro are NOT to
 * suppress switch instrumentation.  The reason is that the cases are
 * where the instrumentation is actually placed, and they might not be in
 * the macro.  We could remember that those cases belong to a switch
 * that's not to be instrumented, but this is easier and probably
 * sufficient. 
 */

void
i_switch(parent, self)
     gct_node parent;
     gct_node self;
{
  gct_node switch_test;		/* The test to instrument. */
  gct_node setter_rh, tests;	/* Results of same. */
  gct_node new_switch_test;		/* Newly instrumented value. */
  int child_used;		/* Whether child needs its setter first. */
  gct_node test_temp;		/* Test variable in place of switch_test. */
  gct_node placeholder;		/* For remember_place. */
  int doing_instrumentation;	/* Remember whether instrumentation turned on. */
  
  switch_test = GCT_SWITCH_TEST(self);
  remember_place(self, switch_test, placeholder);
  
  test_temp = temporary_id(switch_test, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  
  doing_instrumentation = branch_on();

  child_used = DO_EXPR_INSTRUMENT(self, switch_test, Default_state,
				      test_temp, &setter_rh, &tests);
  new_switch_test = comma(SETTER(child_used || doing_instrumentation, test_temp==switch_test,
				 test_temp, setter_rh),
			  tests,
			  VALUE(child_used || doing_instrumentation, test_temp==switch_test,
				test_temp, setter_rh),
			  gct_node_arg_END);

  
  /* Note that we're in a switch, even if we end up not instrumenting it. */
  push_switch();

  if (doing_instrumentation)
    {
      new_switch_test = comma(new_switch_test,
			      switch_needed_init(),
			      copy(test_temp),
			      gct_node_arg_END);
    }
  
  free_temp(test_temp, switch_test);
  replace(self, new_switch_test, placeholder);
  
  DO_INSTRUMENT(self, GCT_SWITCH_BODY(self));
  
  if (!switch_default_seen() && doing_instrumentation)
    {
      if (GCT_COMPOUND_STMT != GCT_SWITCH_BODY(self)->type)
	{
	  warning("Switch statement is a simple statement; no default added.\n");
	}
      else 
	{
	  gct_node closing_brace = GCT_LAST(GCT_SWITCH_BODY(self)->children);
	  int outside_macro = gct_outside_macro_p(closing_brace->first_char);
	  
	  gct_node new_default = newtree(makeroot(GCT_DEFAULT, "default"),
					 switch_case_test(Gct_next_index,
							  outside_macro),
					 gct_node_arg_END);
	  gct_add_before(&(GCT_SWITCH_BODY(self)->children),
			 closing_brace,
			 new_default);
	  
	  if (outside_macro)
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

/*
 * See also i_case.
 */
void
i_default(parent, self)
     gct_node parent;
     gct_node self;
{
  int first_index = Gct_next_index;	/* Index of this probe. */
  int doing_instrumentation;	/* Whether branches are instrumented. */
  int outside_macro;		/* Avoid repeated calls to gct_outside_macro_p. */

  /* We record the default, even if it's not instrumented (because in a macro). */
  now_switch_has_default();
  
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
      gct_node default_stmt;	/* Statement after "default: */
      gct_node placeholder;		/* For remember_place. */
      gct_node new_compound;		/* Root of statement we build. */
	  
      default_stmt = GCT_DEFAULT_STMT(self);
      remember_place(self, default_stmt, placeholder);
      new_compound = compound(switch_case_test(first_index, outside_macro),
			      default_stmt,
			      gct_node_arg_END);
      replace(self, new_compound, placeholder);
    }
}

/*
 * The modification is in two parts:
 * 1.  Setting the variable that prevents further cases from being considered
 * taken.
 * 2.  Marking this case as taken.
 *
 * The second is done only if the case is outside a macro (and
 * appropriate instrumentation is on); the first must be done even within
 * a macro.  Consider:
 *
 * 	#define CASE case
 * 	...
 * 	CASE x:  (falls through)
 * 	case y:
 *
 * We don't want a jump to X to count as a taking of case Y.  (This is
 * admittedly a pretty bizarre use of macros, but might as well do it
 * right.)
 */
void
i_case(parent, self)
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
 * If the return is inside a macro, only the multi-conditional
 * instrumentation is turned off.
 */
void
i_return(parent, self)
     gct_node parent;
     gct_node self;
{
  gct_node expr = GCT_RETURN_EXPR(self);
  gct_node setter_rh, tests;		/* For expr */
  gct_node expr_temp;			/* Temporary that holds expr value. */
  gct_node placeholder;			/* For remember_place. */
  
  if (   multi_on()
      && gct_outside_macro_p(self->first_char)
      && (   Gct_relational[(int)expr->type]
	  || Gct_boolean[(int)expr->type]
	  || Gct_boolean_assign[(int)expr->type]))
    {
      int first_index = Gct_next_index;
      
      remember_place(self, expr, placeholder);
      expr_temp = temporary_id(expr, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
      multi_map(Gct_next_index, self, self->text, FIRST);
      map_placeholder(Gct_next_index+1);
      Gct_next_index+=2;
      (void) DO_EXPR_INSTRUMENT(self, expr, Default_state, expr_temp, &setter_rh, &tests);
      /* Note that expr_temp is currently always != expr, hence SETTER
	 will place it first.  This is more robust, though. */
      replace(self,
	      comma(SETTER(1, expr_temp==expr, expr_temp, setter_rh),
		    tests,
		    make_binary_probe(first_index, copy(expr_temp)),
		    VALUE(1, expr_temp==expr, expr_temp, setter_rh),
		    gct_node_arg_END),
	      placeholder);
      free_temp(expr_temp, expr);
    }
  else
    {
      /*
       * It is common to find "return 0" for pointer-typed functions.  If
       * the 0 is instrumented blindly, we'll end up with an
       * integer-valued comma expression, 
       * (like return (_G(i!=0), _G(j!=0), 0);) 
       * which will make the compiler complain about
       * mixing types and integers.  We must add a cast.
       * Further, we set the type of the constant 0 to the type of the
       * function, so that pointers, not integers, are compared to it.
       *
       * Note that we do all this even if the function is not
       * pointer-typed.  That does no harm -- certainly not now, while
       * integers and floats are considered compatible.  It may even be
       * the right thing in any case.
       */
      if (   GCT_CONSTANT == expr->type
	  && !strcmp("0", expr->text))
	{
	  tree function_type = TREE_TYPE(DECL_RESULT(current_function_decl));
	  char *make_decl();
	  char *decl = make_decl(function_type, "", ARRAYS_AS_POINTERS);
	  char *cast = (char *) xmalloc(strlen(decl)+3);

	  sprintf(cast, "(%s)", decl);
	  free(decl);
	  gct_make_current_note(gct_misc_annotation(cast), self);
	  expr->gcc_type = function_type;
	}
      i_expr(self, expr);
    }

  if (   add_writelog_on()
      && gct_entry_routine(DECL_PRINT_NAME(current_function_decl)))
    {
      if (GCT_NULL_EXPR == GCT_RETURN_EXPR(self)->type)
	{
	  gct_node placeholder;
	  gct_node newcompound;
	  
	  remember_place(parent, self, placeholder);
	  newcompound = compound(self, gct_node_arg_END);
	  replace(parent, newcompound, placeholder);
	  gct_add_before(&(newcompound->children), self,
			 make_simple_statement(make_logcall("gct_writelog")));
	}
      else
	{
	  standard_add_writelog(self, GCT_RETURN_EXPR(self));
	}
    }
}


	      /* EXPRESSION INSTRUMENTATION ROUTINES */

/*
 * NOTES:
 *
 * The valuenode:
 *
 * The valuenode passed to the the instrumentation routine is never
 * itself linked into any list.  Copies of it may be.  The caller is
 * responsible for freeing that node if it doesn't use it itself.
 */



/*
 * Instrumentation for standard binary operators, including arithmetic,
 * relational.
 */


/* NOTE:  This code assumes (via the incrementing of variables like
   first_index) that all instrumentation is done according to type.
   There is no instrumentation that applies to more than one type. */
/*
 * NOTE: Uses of state.integer_only are not strictly required; they
 * simply avoid test probes that provide useless information, like that a
 * shifted variable never has a different value from a float.
 */

int
exp_binary(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  gct_node left, right;	/* Our children */
  int first_index;	/* Index of first instrumentation of this node. */
  gct_node left_temp, right_temp;	/* Temporaries for result of
					   evaluating our children. */
  gct_node left_setter_rh, left_tests;	/* For recursive calls. */
  gct_node right_setter_rh, right_tests;	/* For recursive calls. */
  int left_temp_used, right_temp_used;	/* Did child need temp? */
  i_state child_state;			/* State to pass to child. */
  int i_use_setter;			/* Do I use the setter in my tests? */

  int outside_macro = gct_outside_macro_p(self->first_char);
  int operator_on_p = operator_on() && outside_macro;
  int relational_on_p = relational_on() && outside_macro;
  
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


  i_use_setter = operator_on_p || relational_on_p;

  *setter_rh = self;
  *tests = GCT_NULL_NODE;
  
  left = GCT_OP_LEFT(self);
  gct_remove_node(&(self->children), left);
  left_temp = temporary_id(left, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  
  right = GCT_OP_RIGHT(self);
  gct_remove_node(&(self->children), right);
  right_temp = temporary_id(right, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);

  child_state = Default_state;
  if (Gct_relational[(int)self->type])
    {
      push_operator(self, right_temp, child_state);
    }
  
  child_state.integer_only = Gct_integer_only[(int)self->type];
  left_temp_used = DO_EXPR_INSTRUMENT(self, left, child_state, left_temp, &left_setter_rh, &left_tests);
  if (Gct_relational[(int)self->type])
    {
      pop_suff(child_state);
    }

  
  /* We have to use up our indices here before we process lexically-later
     subtrees. */
  
  first_index = Gct_next_index;
  switch(self->type)
    {
    case GCT_PLUS:
      if (operator_on_p)
	{
	  operator_map(Gct_next_index++,
		       self, "might be -.", FIRST);
	  /* Beware of pointer addition. */
	  if (times_compatible(left->gcc_type, right->gcc_type))
	    {
	      operator_map(Gct_next_index++,
			   self, "might be *.", DUPLICATE);
	    }
	}
      break;
    case GCT_MINUS:
      if (operator_on_p)
	{
	  operator_map(Gct_next_index++,
		       self, "might be +.", FIRST);
	}
      break;
    case GCT_TIMES:
      if (operator_on_p)
	{
	  operator_map(Gct_next_index++,
		       self, "might be +.", FIRST);
	}
      break;
    case GCT_DIV:
      if (   operator_on_p
	  && (INTEGER_TYPE == TREE_CODE(left->gcc_type))
	  && (INTEGER_TYPE == TREE_CODE(right->gcc_type)))
	{
	  operator_map(Gct_next_index++,
		       self, "might be %.", FIRST);
	}
      break;
    case GCT_MOD:
      if (operator_on_p)
	{
	  operator_map(Gct_next_index++,
		       self, "might be /.", FIRST);
	}
      break;
    case GCT_LSHIFT:
      if (operator_on_p)
	{
	  operator_map(Gct_next_index++,
		       self, "might be >>.", FIRST);
	}
      break;
    case GCT_RSHIFT:
      if (operator_on_p)
	{
	  operator_map(Gct_next_index++,
		       self, "might be <<.", FIRST);
	}
      break;
    case GCT_LESS:
      if (operator_on_p || relational_on_p)
	{
	  operator_map(Gct_next_index++,
		       self, "might be >. (L!=R)", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be <=. (L==R)", DUPLICATE);
	  operator_map(Gct_next_index++, self,
		       "needs boundary L==R-1.", DUPLICATE);
	}
      break;
    case GCT_GREATER:
      if (operator_on_p || relational_on_p)
	{
	  operator_map(Gct_next_index++,
		       self, "might be <. (L!=R)", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be >=. (L==R)", DUPLICATE);
	  operator_map(Gct_next_index++, self,
		       "needs boundary L==R+1.", DUPLICATE);
	}
      break;
    case GCT_LESSEQ:
      if (operator_on_p || relational_on_p)
	{
	  operator_map(Gct_next_index++,
		       self, "might be >=. (L!=R)", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be <. (L==R)", DUPLICATE);
	  operator_map(Gct_next_index++, self,
		       "needs boundary L==R+1.", DUPLICATE);
	}
      break;
    case GCT_GREATEREQ:
      if (operator_on_p || relational_on_p)
	{
	  operator_map(Gct_next_index++,
		       self, "might be <=. (L!=R)", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be >. (L==R)", DUPLICATE);
	  operator_map(Gct_next_index++, self,
		       "needs boundary L==R-1.", DUPLICATE);
	}
      break;
    case GCT_EQUALEQUAL:
    case GCT_NOTEQUAL:
      /* No instrumentation.  However, we do issue a warning that
	 lint doesn't. */
      i_use_setter = 0;
      if (GCT_SIMPLE_STMT == parent->type)
	{
	  warning("(really line %d) \'<op> %s <op>;\' can have no effect.",
		  self->lineno, self->text);
	}
      break;
    default:
      error("Unknown exp_binary type.");
      abort();
      break;
    }
  if (Gct_relational[(int)self->type])
    {
      push_operator(gct_reverse_test(self), left_temp, child_state);
    }
  child_state.integer_only = Gct_integer_only[(int)self->type];
  right_temp_used = DO_EXPR_INSTRUMENT(self, right, child_state, right_temp, &right_setter_rh, &right_tests);
  if (Gct_relational[(int)self->type])
    {
      pop_suff(child_state);
    }

  /* At this point, we build the setter from the children's results.
     We put the setter first if the child wants it or we want it.
     We want it if we use it in our tests or if we passed
     down the other side for weak sufficiency (in which case the child
     wants it there but doesn't know it). */
  {
    int setter_arg = i_use_setter || Gct_relational[(int)self->type];
    
    *setter_rh =
      comma(SETTER(left_temp_used || setter_arg, left_temp == left,
		   left_temp, left_setter_rh),
	    SETTER(right_temp_used || setter_arg, right_temp == right,
		   right_temp, right_setter_rh),
	    left_tests,
	    right_tests,
	    newtree(self,
		    VALUE(left_temp_used || setter_arg, left_temp == left,
			  left_temp, left_setter_rh),
		    VALUE(right_temp_used || setter_arg, right_temp == right,
			  right_temp, right_setter_rh),
		    gct_node_arg_END),
	    gct_node_arg_END);
  }
  

  switch(self->type)
    {
    case GCT_PLUS:
      if (operator_on_p)
	{
	  add_test(tests,
		   ne_test(first_index, Default_state,
			   copy(right_temp),
			   makeroot(GCT_CONSTANT, "0")));
	  first_index++;
	  if (times_compatible(left->gcc_type, right->gcc_type))
	    {
	      add_test(tests,
		       ne_test(first_index, Default_state,
			       newtree(makeroot(GCT_TIMES, "*"),
				       copy(left_temp),
				       copy(right_temp),
				       gct_node_arg_END),
			       newtree(makeroot(GCT_PLUS, "+"),
				       copy(left_temp),
				       copy(right_temp),
				       gct_node_arg_END)));
	    }
	}
      break;
    case GCT_MINUS:
      if (operator_on_p)
	{
	  add_test(tests,
		   ne_test(first_index, Default_state,
			   copy(right_temp),
			   makeroot(GCT_CONSTANT, "0")));
	  first_index++;
	}
      break;
    case GCT_TIMES:
      if (operator_on_p)
	{
	  add_test(tests,
		   ne_test(first_index, Default_state,
			   newtree(makeroot(GCT_TIMES, "*"),
				   copy(left_temp),
				   copy(right_temp),
				   gct_node_arg_END),
			   newtree(makeroot(GCT_PLUS, "+"),
				   copy(left_temp),
				   copy(right_temp),
				   gct_node_arg_END)));
	  first_index++;
	}
      break;
    case GCT_DIV:
      if (   operator_on_p
	  && (INTEGER_TYPE == TREE_CODE(left->gcc_type))
	  && (INTEGER_TYPE == TREE_CODE(right->gcc_type)))
	{
	  add_test(tests,
		   ne_test(first_index, Default_state,
			   newtree(makeroot(GCT_TIMES, "/"),
				   copy(left_temp),
				   copy(right_temp),
				   gct_node_arg_END),
			   newtree(makeroot(GCT_PLUS, "%"),
				   copy(left_temp),
				   copy(right_temp),
				   gct_node_arg_END)));
	  first_index++;
	}
      break;
    case GCT_MOD:
      if (operator_on_p)
	{
	  add_test(tests,
		   ne_test(first_index, Default_state,
			   newtree(makeroot(GCT_TIMES, "/"),
				   copy(left_temp),
				   copy(right_temp),
				   gct_node_arg_END),
			   newtree(makeroot(GCT_PLUS, "%"),
				   copy(left_temp),
				   copy(right_temp),
				   gct_node_arg_END)));
	  first_index++;
	}
      break;
    case GCT_LSHIFT:
    case GCT_RSHIFT:
      /* These two have the same test. */
      if (operator_on_p)
	{
	  add_test(tests,
		   ne_test(first_index, Default_state,
			   copy(right_temp),
			   makeroot(GCT_CONSTANT, "0")));
	  first_index++;
	}
      break;
    case GCT_LESS:
    case GCT_GREATER:
    case GCT_LESSEQ:
    case GCT_GREATEREQ:
      if (operator_on_p || relational_on_p)
	{
	  gct_node op1, op2;
	  gct_node term1_1, term1_2, term2_1, term2_2;
	  
	  add_test(tests,
		   (make_probe(first_index++,
			       newtree(makeroot(GCT_NOTEQUAL, "!="),
				       copy(left_temp),
				       copy(right_temp),
				       gct_node_arg_END))));
	  add_test(tests,
		   make_probe(first_index++,
			      newtree(makeroot(GCT_EQUALEQUAL, "=="),
				      copy(left_temp),
				      copy(right_temp),
				      gct_node_arg_END)));
	  /* a < b && (a+1 >= b) */
	  /* a > b && (a-1 <= b) */
	  /* a-1 <= b && a > b */
	  /* a+1 >= b && a < b */
	  
	  
	  op1 = makeroot(self->type, self->text);
	  term1_2 = copy(right_temp);
	  term2_2 = copy(right_temp);
	  if (GCT_LESS == self->type)
	    {
	      term1_1 = copy(left_temp);
	      op2 = makeroot(GCT_GREATEREQ, ">=");
	      term2_1 = newtree(makeroot(GCT_PLUS, "+"),
				copy(left_temp),
				epsilon(left_temp),
				gct_node_arg_END);
	    }
	  else if (GCT_GREATER == self->type)
	    {
	      term1_1 = copy(left_temp);
	      op2 = makeroot(GCT_LESSEQ, "<=");
	      term2_1 = newtree(makeroot(GCT_MINUS, "-"),
				copy(left_temp),
				epsilon(left_temp),
				gct_node_arg_END);
	    }
	  else if (GCT_LESSEQ == self->type)
	    {
	      term1_1 = newtree(makeroot(GCT_MINUS, "-"),
				copy(left_temp),
				epsilon(left_temp),
				gct_node_arg_END);
	      op2 = makeroot(GCT_GREATER, ">");
	      term2_1 = copy(left_temp);
	    }
	  else
	    {
	      term1_1 = newtree(makeroot(GCT_PLUS, "+"),
				copy(left_temp),
				epsilon(left_temp),
				gct_node_arg_END);
	      op2 = makeroot(GCT_LESS, "<");
	      term2_1 = copy(left_temp);
	    }
	  
	  add_test(tests,
		   make_probe(first_index++,
			      newtree(makeroot(GCT_ANDAND, "&&"),
				      newtree(op1, term1_1, term1_2, gct_node_arg_END),
				      newtree(op2, term2_1, term2_2, gct_node_arg_END),
				      gct_node_arg_END)));
	}
      break;
    case GCT_EQUALEQUAL:
    case GCT_NOTEQUAL:
      /* No tests. */
      break;
    default:
      error("Unknown exp_binary type.");
      abort();
      break;
    }
  free_temp(right_temp, right);
  free_temp(left_temp, left);

  /* We force the caller to give the setter precedence if we used left_temp
     or right_temp in any tests.. */
  return (   GCT_NULL_NODE != *tests
	  && (left_temp != left || right_temp != right));
}


static int Multicondition_nesting_level = 0;

int
exp_boolean(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  gct_node left, right;	/* Our children */
  int first_index;	/* Index of first instrumentation of this node. */
  gct_node left_temp, right_temp;	/* Temporaries for result of
					   evaluating our children. */
  gct_node left_setter_rh, left_tests;	/* For recursive calls. */
  gct_node right_setter_rh, right_tests;	/* For recursive calls. */
  int left_temp_used, right_temp_used;	/* Did child need temp? */
  char *left_name, *right_name;		/* Names for multiconditional reporting. */

  int outside_macro = gct_outside_macro_p(self->first_char);
  int operator_on_p = operator_on() && outside_macro;
  int multi_on_p = multi_on() && outside_macro;
  int i_use_setter = operator_on_p || multi_on_p;

  *setter_rh = self;
  *tests = GCT_NULL_NODE;
    
  left = GCT_OP_LEFT(self);
  gct_remove_node(&(self->children), left);
  left_temp = temporary_id(left, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  
  right = GCT_OP_RIGHT(self);
  gct_remove_node(&(self->children), right);
  right_temp = temporary_id(right, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);

  Multicondition_nesting_level++;
  left_name = make_leftmost_name(left, Multicondition_nesting_level);
  right_name = make_leftmost_name(right, Multicondition_nesting_level);

  left_temp_used = DO_EXPR_INSTRUMENT(self, left, Default_state, left_temp, &left_setter_rh, &left_tests);

  
  /* We have to use up our indices here before we process lexically-later
     subtrees. */

  first_index = Gct_next_index;
  switch(self->type)
    {
    case GCT_ANDAND:
      if (operator_on_p)
	{
	  operator_map(Gct_next_index++,
		       self, "might be ||", FIRST);
	  
	  /* Currently omitted -- too hard to satisfy */
	  /* operator_map(Gct_next_index++,
	     self, "might be &", DUPLICATE); */
	}
      break;
    case GCT_OROR:
      if (operator_on_p)
	{
	  operator_map(Gct_next_index++,
		       self, "might be &&", FIRST);
	}
      break;
    default:
      error("Unknown exp_boolean type.");
      abort();
      break;
    }
  /* Applies to all boolean operators. */
  if (multi_on_p)
    {
      multi_map(Gct_next_index++, self, left_name, FIRST);	/* Diff name than used above. */
      map_placeholder(Gct_next_index++);
      multi_map(Gct_next_index++, self, right_name, DUPLICATE);
      map_placeholder(Gct_next_index++);
    }
  right_temp_used = DO_EXPR_INSTRUMENT(self, right, Default_state, right_temp, &right_setter_rh, &right_tests);

  *setter_rh =
    newtree(self,
	    comma(SETTER(left_temp_used || i_use_setter, left_temp == left,
			 left_temp, left_setter_rh),
		  left_tests,
		  VALUE(left_temp_used || i_use_setter, left_temp == left,
			left_temp, left_setter_rh),
		  gct_node_arg_END),
	    comma(SETTER(right_temp_used || i_use_setter, right_temp == right,
			 right_temp, right_setter_rh),
		  right_tests,
		  VALUE(right_temp_used || i_use_setter, right_temp == right,
			right_temp, right_setter_rh),
		      gct_node_arg_END),
	    gct_node_arg_END);

  switch(self->type)
    {
    case GCT_ANDAND:
      {
	if (operator_on_p)
	  {
	    /* (left & right) != (left && right) */
	    /* Not currently used. */
	    /* (left && right) != (left || right) */
	    add_test(tests,
		     make_probe(first_index, 
				newtree(makeroot(GCT_ANDAND, "&&"),
					copy(left_temp),
					newtree(makeroot(GCT_TRUTH_NOT, "!"),
						copy(right_temp),
						gct_node_arg_END),
					gct_node_arg_END)));
	    first_index++;
	  }
	if (multi_on_p)
	  {
	    add_test(tests, make_binary_probe(first_index, copy(left_temp)));
	    first_index+=2;
	    add_test(tests,
		     newtree(makeroot(GCT_QUEST, "?"),
			     copy(left_temp),
			     make_binary_probe(first_index,
					       copy(right_temp)),
			     makeroot(GCT_CONSTANT, "0"),
			     gct_node_arg_END));
	    first_index+=2;
	  }
      }
      break;
    case GCT_OROR:
      {
	if (operator_on_p)
	  {
	    add_test(tests,
		     make_probe(first_index, 
				newtree(makeroot(GCT_ANDAND, "&&"),
					newtree(makeroot(GCT_TRUTH_NOT, "!"),
						copy(left_temp),
						gct_node_arg_END),
					copy(right_temp),
					gct_node_arg_END)));
	    first_index++;
	  }
	if (multi_on_p)
	  {
	    add_test(tests, make_binary_probe(first_index, copy(left_temp)));
	    first_index+=2;
	    add_test(tests,
		     newtree(makeroot(GCT_QUEST, "?"),
			     copy(left_temp),
			     makeroot(GCT_CONSTANT, "0"),
			     make_binary_probe(first_index,
					       copy(right_temp)),
			     gct_node_arg_END));
	    first_index+=2;
	  }
      }
      break;
    default:
      error("Unknown exp_boolean type.");
      abort();
      break;
    }
  free_temp(right_temp, right);
  free_temp(left_temp, left);
  free(left_name);
  free(right_name);
  Multicondition_nesting_level--;

  /* We force the caller to give the setter precedence if we 
     wrote any tests - we use the temps in multi tests.*/
  return (GCT_NULL_NODE != *tests);
}


int
exp_bitwise(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  gct_node left, right;	/* Our children */
  int first_index;	/* Index of first instrumentation of this node. */
  gct_node left_temp, right_temp;	/* Temporaries for result of
					   evaluating our children. */
  gct_node left_setter_rh, left_tests;	/* For recursive calls. */
  gct_node right_setter_rh, right_tests;	/* For recursive calls. */
  int left_temp_used, right_temp_used;	/* Did child need temp? */
  char *left_name, *right_name;		/* Names for multiconditional reporting. */
  i_state child_state;			/* State to pass down. */

  int outside_macro = gct_outside_macro_p(self->first_char);
  int operator_on_p = operator_on() && outside_macro;
  int multi_on_p = multi_on() && outside_macro;
  int i_use_setter = operator_on_p || multi_on_p;

  *setter_rh = self;
  *tests = GCT_NULL_NODE;
    
  left = GCT_OP_LEFT(self);
  gct_remove_node(&(self->children), left);
  left_temp = temporary_id(left, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  
  right = GCT_OP_RIGHT(self);
  gct_remove_node(&(self->children), right);
  right_temp = temporary_id(right, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);

  Multicondition_nesting_level++;
  left_name = make_leftmost_name(left, Multicondition_nesting_level);
  right_name = make_leftmost_name(right, Multicondition_nesting_level);

  child_state = Default_state;
  child_state.integer_only = Gct_integer_only[(int) self->type];

  left_temp_used = DO_EXPR_INSTRUMENT(self, left, child_state, left_temp, &left_setter_rh, &left_tests);

  
  /* We have to use up our indices here before we process lexically-later
     subtrees. */

  first_index = Gct_next_index;
  switch(self->type)
    {
    case GCT_BITAND:
      if (operator_on_p)
	{
	  operator_map(Gct_next_index++,
		       self, "might be |", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be &&", DUPLICATE);
	}
      break;
    case GCT_BITOR:
      if (operator_on_p)
	{
	  operator_map(Gct_next_index++,
		       self, "might be &", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be ||", DUPLICATE);
	}
      break;
    case GCT_BITXOR:
      if (operator_on_p)
	{
	  operator_map(Gct_next_index++,
		       self, "might be |", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be &", DUPLICATE);
	}
      break;
    default:
      error("Unknown exp_bitwise type.");
      abort();
      break;
    }

  /* Applies to all bitwise operators. */
  if (multi_on_p)
    {
      multi_map(Gct_next_index++, self, left_name, FIRST);	/* Diff name than used above. */
      map_placeholder(Gct_next_index++);
      multi_map(Gct_next_index++, self, right_name, DUPLICATE);
      map_placeholder(Gct_next_index++);
    }

  right_temp_used = DO_EXPR_INSTRUMENT(self, right, child_state, right_temp, &right_setter_rh, &right_tests);

  /* At this point, we build the setter from the children's results.
     We put the setter first if the child wants it or we want it.
     We want it if we use it in our tests or if we passed
     down the other side for weak sufficiency (in which case the child
     wants it there but doesn't know it. */
  {
    *setter_rh =
      comma(SETTER(left_temp_used || i_use_setter, left_temp == left,
		   left_temp, left_setter_rh),
	    SETTER(right_temp_used || i_use_setter, right_temp == right,
		   right_temp, right_setter_rh),
	    left_tests,
	    right_tests,
	    newtree(self,
		    VALUE(left_temp_used || i_use_setter, left_temp == left,
			  left_temp, left_setter_rh),
		    VALUE(right_temp_used || i_use_setter, right_temp == right,
			  right_temp, right_setter_rh),
		    gct_node_arg_END),
	    gct_node_arg_END);
  }

  switch(self->type)
    {
    case GCT_BITAND:
      /* left != right rules out | */
      /* !!valuenode != (left && right) rules out && */
      if (operator_on_p)
	{
	  add_test(tests,
		   ne_test(first_index++, Default_state,
			   copy(left_temp),
			   copy(right_temp)));
	  add_test(tests,
		   ne_test(first_index++, Default_state,
			   notnot(copy(valuenode)),
			   newtree(makeroot(GCT_ANDAND, "&&"),
				   copy(left_temp),
				   copy(right_temp),
				   gct_node_arg_END)));
	}
      break;
    case GCT_BITOR:
      /* left != right rules out & */
      /* valuenode != (left || right) rules out || */
      if (operator_on_p)
	{
	  add_test(tests,
		   ne_test(first_index++, Default_state,
			   copy(left_temp),
			   copy(right_temp)));
	  add_test(tests,
		   ne_test(first_index++, Default_state,
			   copy(valuenode),
			   newtree(makeroot(GCT_OROR, "||"),
				   copy(left_temp),
				   copy(right_temp),
				   gct_node_arg_END)));
	}
      break;
    case GCT_BITXOR:
      if (operator_on_p)
	{
	  /* valuenode != (left | right) rules out | */
	  add_test(tests,
		   ne_test(first_index++, Default_state,
			   copy(valuenode),
			   newtree(makeroot(GCT_BITOR, "|"),
				   copy(left_temp),
				   copy(right_temp),
				   gct_node_arg_END)));
	  /* valuenode != (left & right) rules out & */
	  add_test(tests,
		   ne_test(first_index++, Default_state,
			   copy(valuenode),
			   newtree(makeroot(GCT_BITAND, "&"),
				   copy(left_temp),
				   copy(right_temp),
				   gct_node_arg_END)));
	}
      break;
    default:
      error("Unknown exp_bitwise type.");
      abort();
      break;
    }
  if (multi_on_p)
    {
      add_test(tests,
	       make_binary_probe(first_index, copy(left_temp)));
      add_test(tests,
	       make_binary_probe(first_index+2, copy(right_temp)));
    }
  
  free_temp(right_temp, right);
  free_temp(left_temp, left);
  free(left_name);
  free(right_name);
  Multicondition_nesting_level--;

  /* We force the caller to give the setter precedence if we 
     wrote any tests - we use the valuenode in operator tests
     and the temps in multi tests.  (Don't need to turn it on
     for some multi tests, but code's more robust this way. */
  return (GCT_NULL_NODE != *tests);
}


int
exp_assign(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  gct_node left, right;	/* Our children */
  gct_node left_temp, right_temp;	/* Temporaries for result of
					   evaluating our children. */
  gct_node left_setter_rh, left_tests;	/* For recursive calls. */
  gct_node right_setter_rh, right_tests;	/* For recursive calls. */
  int left_temp_used = 0, right_temp_used = 0;	/* By me or by child. */
  gct_node left_lvalue;			/* The thing to set. */
  i_state child_state;			/* State to pass to child. */
  int first_index;			/* Index of first instrumentation of this node. */

  int outside_macro = gct_outside_macro_p(self->first_char);
  int operator_on_p = operator_on() && outside_macro;
  int multi_on_p = multi_on() && outside_macro;
  
  *setter_rh = self;
  *tests = GCT_NULL_NODE;

  left = GCT_OP_LEFT(self);
  gct_remove_node(&(self->children), left);
  left_temp = temporary_id(left, CLOSEST, FORCE, NULL, NULL, WANT_BASE_TYPE);
  
  right = GCT_OP_RIGHT(self);
  gct_remove_node(&(self->children), right);
  right_temp = temporary_id(right, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);

  child_state = Default_state;
  child_state.integer_only = Gct_integer_only[(int)self->type];

  left_temp_used = DO_LVALUE_INSTRUMENT(self, left, child_state, left_temp, &left_setter_rh, &left_tests, &left_lvalue);
  
  /* We have to use up our indices here before we process lexically-later
     subtrees. */
  first_index = Gct_next_index;
  if (operator_on_p)
    {
      /* All of these use the left and right temporaries. */
      left_temp_used |= 1;
      right_temp_used |=1;
      switch(self->type)
	{
	case GCT_SIMPLE_ASSIGN:
	  operator_map(Gct_next_index++,
		       self, "is never needed.", FIRST);
	  if (!NON_IMMEDIATE_P(left_temp->gcc_type))
	    {
	      operator_map(Gct_next_index++,
			   self, "might be ==.", DUPLICATE);
	    }
	  break;
	case GCT_PLUS_ASSIGN:
	  operator_map(Gct_next_index++,
		       self, "is never needed (or might be -=).", FIRST);
	  if (times_compatible(left->gcc_type, right->gcc_type))
	    {
	      operator_map(Gct_next_index++,
			   self, "might be *=.", DUPLICATE);
	      /* One typo away */
	      /* Guarded by times_compatible to avoid pointer-int
		 comparison. */
	      operator_map(Gct_next_index++,
			   self, "might be ==.", DUPLICATE);
	    }
	  break;
	  
	case GCT_MINUS_ASSIGN:
	  operator_map(Gct_next_index++,
		       self, "is never needed (or might be +=).", FIRST);
	  /* Note that a == for -= probe is ALWAYS satisfied. */
	  break;
	  
	case GCT_TIMES_ASSIGN:
	  operator_map(Gct_next_index++,
		       self, "is never needed (or might be /=).", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be +=.", DUPLICATE);
	  /* One typo away */
	  if (   (INTEGER_TYPE == TREE_CODE(left->gcc_type))
	      && (INTEGER_TYPE == TREE_CODE(right->gcc_type)))
	    {
	      operator_map(Gct_next_index++,
			   self, "might be &=.", DUPLICATE);
	    }
	  break;
	  
	case GCT_DIV_ASSIGN:
	  operator_map(Gct_next_index++,
		       self, "is never needed (or might be *=).", FIRST);
	  if (   (INTEGER_TYPE == TREE_CODE(left->gcc_type))
	      && (INTEGER_TYPE == TREE_CODE(right->gcc_type)))
	    {
	      operator_map(Gct_next_index++,
			   self, "might be %=.", DUPLICATE);
	    }
	  break;
	  
	case GCT_MOD_ASSIGN:
	  operator_map(Gct_next_index++,
		       self, "is never needed.", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be /=.", DUPLICATE);
	  /* One typo away */
	  operator_map(Gct_next_index++,
		       self, "might be ^=.", DUPLICATE);
	  break;
	  
	case GCT_LSHIFT_ASSIGN:
	  operator_map(Gct_next_index++,
		       self, "is never needed (or might be >>=).", FIRST);
	  break;
	  
	case GCT_RSHIFT_ASSIGN:
	  operator_map(Gct_next_index++,
		       self, "is never needed (or might be <<=).", FIRST);
	  break;
	  
	case GCT_BITAND_ASSIGN:
	  operator_map(Gct_next_index++,
		       self, "is never needed.", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be |=.", DUPLICATE);
	  /* One typo away */
	  operator_map(Gct_next_index++,
		       self, "might be *=.", DUPLICATE);
	  /* One typo away */
	  operator_map(Gct_next_index++,
		       self, "might be ^=.", DUPLICATE);
	  break;
	  
	case GCT_BITOR_ASSIGN:
	  operator_map(Gct_next_index++,
		       self, "is never needed.", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be &=.", DUPLICATE);
	  /* One typo away */
	  operator_map(Gct_next_index++,
		       self, "might be +=.", DUPLICATE);
	  break;
	  
	case GCT_BITXOR_ASSIGN:
	  operator_map(Gct_next_index++,
		       self, "is never needed.", FIRST);
	  operator_map(Gct_next_index++,
		       self, "might be |=.", DUPLICATE);
	  /* One typo away */
	  operator_map(Gct_next_index++,
		       self, "might be %=.", DUPLICATE);
	  /* One typo away */
	  operator_map(Gct_next_index++,
		       self, "might be &=.", DUPLICATE);
	  break;
	default:
	  error("Unknown assignment type.");
	  abort();
	  break;
	}
    }
  
  if (multi_on_p)
    {
      if (Gct_boolean_assign[(int)self->type])
	{
	  multi_map(Gct_next_index, self, self->text, FIRST);	/* Diff name than used above. */
	  Gct_next_index++;
	  map_placeholder(Gct_next_index++);
	}
      if (   Gct_relational[(int)right->type]
	  || Gct_boolean[(int)right->type])
	{
	  char *name = (char *) xmalloc(strlen(self->text) + 20);
	  sprintf(name, "%s expression", self->text);
	  multi_map(Gct_next_index, self, name, FIRST);	/* Diff name than used above. */
	  free(name);
	  Gct_next_index++;
	  map_placeholder(Gct_next_index++);
	  right_temp_used |= 1;
	}
    }
  
  right_temp_used |= DO_EXPR_INSTRUMENT(self, right, child_state, right_temp, &right_setter_rh, &right_tests);

  {
    /* Force creation of a setter if I or child used child's temp. */
    /* The use of VALUE by itself causes the left child's tests to
       be inserted in the case when neither the child nor I used the
       left_temp. */
    *setter_rh =
      comma(SETTER(left_temp_used, left_temp == left,
		   left_temp, left_setter_rh),
	    SETTER(right_temp_used, right_temp == right,
		   right_temp, right_setter_rh),
	    left_tests,
	    right_tests,
	    VALUE(left_temp_used, left_temp == left,
 			  left_temp, left_setter_rh),
	    newtree(self,
		    left_lvalue,
		    VALUE(right_temp_used, right_temp == right,
			  right_temp, right_setter_rh),
		    gct_node_arg_END),
	    gct_node_arg_END);
  }
  if (operator_on_p)
    {
      /* This subroutine is used for all X= might be Y= probes. */
#     define MIGHTBE(MB_type, MB_text)				\
      add_test(tests,						\
	       ne_test(first_index++, Default_state,		\
		       copy(valuenode),				\
		       newtree(makeroot((MB_type), (MB_text)),	\
			       copy(left_temp),			\
			       copy(right_temp),		\
			       gct_node_arg_END)))
	
	/* Like the above, except the rhs is guarded with a check for 0. */
#     define ZGUARD_MIGHTBE(MB_type, MB_text)			\
	add_test(tests,						\
		 make_probe(first_index++,			\
			    newtree(makeroot(GCT_OROR, "||"),		\
				    newtree(makeroot(GCT_EQUALEQUAL, "=="),	\
					    copy(right_temp),		\
					    makeroot(GCT_CONSTANT, "0"),	\
					    gct_node_arg_END),				\
				    newtree(makeroot(GCT_NOTEQUAL, "!="),	\
					    copy(valuenode),		\
					    newtree(makeroot((MB_type), (MB_text)),\
						    copy(left_temp),	\
						    copy(right_temp),	\
						    gct_node_arg_END),			\
					    gct_node_arg_END),				\
				    gct_node_arg_END)));
      
      /* Like the above, except the comparison first makes the
	 valuenode into a boolean value, using !! */
#  define BOOL_MIGHTBE(MB_type, MB_text)				\
      add_test(tests,						\
	       ne_test(first_index++, Default_state,		\
		       notnot(copy(valuenode)),		\
		       newtree(makeroot((MB_type), (MB_text)),\
			       copy(left_temp),		\
			       copy(right_temp),		\
			       gct_node_arg_END)))
	
	/* This test is common to all types. */
	add_test(tests,
		 ne_test(first_index++, Default_state,
			 copy(valuenode),
			 copy(left_temp)));
      
      switch(self->type)
	{
	case GCT_SIMPLE_ASSIGN:
	  if (!NON_IMMEDIATE_P(left_temp->gcc_type))
	    {
	      BOOL_MIGHTBE(GCT_EQUALEQUAL, "==");
	    }
	  break;
	case GCT_PLUS_ASSIGN:
	  if (times_compatible(left->gcc_type, right->gcc_type))
	    {
	      MIGHTBE(GCT_TIMES, "*");
	      BOOL_MIGHTBE(GCT_EQUALEQUAL, "==");
	    }
	  break;
	  
	case GCT_MINUS_ASSIGN:
	  /* No additional tests. */
	  break;
	  
	case GCT_TIMES_ASSIGN:
	  MIGHTBE(GCT_PLUS, "+");
	  if (   (INTEGER_TYPE == TREE_CODE(left->gcc_type))
	      && (INTEGER_TYPE == TREE_CODE(right->gcc_type)))
	    {
	      MIGHTBE(GCT_BITAND, "&");
	    }
	  break;
	case GCT_DIV_ASSIGN:
	  if (   (INTEGER_TYPE == TREE_CODE(left->gcc_type))
	      && (INTEGER_TYPE == TREE_CODE(right->gcc_type)))
	    {
	      /* Don't need ZGUARD because / can't have 0 either. */
	      MIGHTBE(GCT_MOD, "%");
	    }
	  break;
	case GCT_MOD_ASSIGN:
	  MIGHTBE(GCT_DIV, "/");
	  MIGHTBE(GCT_BITXOR, "^");
	  break;
	case GCT_LSHIFT_ASSIGN:
	case GCT_RSHIFT_ASSIGN:
	  /* Nothing special. */
	  break;
	case GCT_BITAND_ASSIGN:
	  MIGHTBE(GCT_BITOR, "|");
	  MIGHTBE(GCT_TIMES, "*");
	  MIGHTBE(GCT_BITXOR, "^");
	  break;
	case GCT_BITOR_ASSIGN:
	  MIGHTBE(GCT_BITAND, "&");
	  MIGHTBE(GCT_PLUS, "+");
	  break;
	case GCT_BITXOR_ASSIGN:
	  MIGHTBE(GCT_BITOR, "|");
	  ZGUARD_MIGHTBE(GCT_MOD, "%");
	  MIGHTBE(GCT_BITAND, "&");
	  break;
	default:
	  error("Unknown assignment type.");
	  abort();
	  break;
	}
#	undef MIGHTBE
#	undef BOOL_MIGHTBE
#	undef ZGUARD_MIGHTBE
    }
  if (multi_on_p)
    {
      if (Gct_boolean_assign[(int)self->type])
	{
	  add_test(tests,
		   make_binary_probe(first_index, copy(valuenode)));
	  first_index +=2;
	  
	}
      if (   Gct_relational[(int)right->type]
	  || Gct_boolean[(int)right->type])
	{
	  add_test(tests,
		   make_binary_probe(first_index, copy(right_temp)));
	  first_index+=2;
	}
    }

  free_temp(right_temp, right);
  free_temp(left_temp, left);

  /* At present, every assignment test uses VALUENODE, so any test forces
     the setter. */
  return (   GCT_NULL_NODE != *tests);
}

int
exp_comma(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  gct_node rover = self->children;
  
  assert(rover);
  do	/* Loop depends on fact that there're at least 2 child nodes. */
    {
      rover=rover->next;	/* Old value about to be modified in place. */
      i_expr(self, rover->prev);
    }
  while (rover != self->children);
  *setter_rh = self;
  *tests = GCT_NULL_NODE;
  return 0;
}

/* unary -, unary +, !, and ~ */
int
exp_unary(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  gct_node expr, expr_temp;		/* left hand expression. */
  gct_node expr_setter_rh, expr_tests;	/* For recursive calls. */
  int expr_temp_used;			/* By child */
  int operator_on_p;
  int first_probe;
  int i_use_setter = 0;			/* I use child's tempvar & setter */
  i_state child_state;

  int outside_macro = gct_outside_macro_p(self->first_char);
  operator_on_p = operator_on() && outside_macro;
  
  *setter_rh = self;
  *tests = GCT_NULL_NODE;

  expr = GCT_OP_ONLY(self);
  gct_remove_node(&(self->children), expr);
  expr_temp = temporary_id(expr, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);

  if (operator_on_p)
    {
      first_probe = Gct_next_index;
      switch(self->type)
	{
	case GCT_NEGATE:
	  operator_map(Gct_next_index++,
		       self, "is never needed.", FIRST);
	  i_use_setter = 1;
	  break;
	case GCT_UNARY_PLUS:
	  /* Nothing. */
	  break;
	case GCT_BIT_NOT:
	  operator_map(Gct_next_index++,
		       self, "might be !.", FIRST);
	  i_use_setter = 1;
	  break;
	case GCT_TRUTH_NOT:
	  /* You can't apply ! to a float. */
	  if (INTEGER_TYPE == TREE_CODE(expr->gcc_type))
	    {
	      operator_map(Gct_next_index++,
			   self, "might be ~.", FIRST);
	      i_use_setter = 1;
	    }
	  break;
	default:
	  error("Unknown unary type.");
	  abort();
	  break;
	}
    }
  

  child_state = Default_state;
  child_state.integer_only = Gct_integer_only[(int)self->type];

  expr_temp_used = DO_EXPR_INSTRUMENT(self, expr, Default_state, expr_temp, &expr_setter_rh, &expr_tests);
  
  *setter_rh =
    comma(SETTER(expr_temp_used || i_use_setter, expr_temp == expr,
		 expr_temp, expr_setter_rh),
	  expr_tests,
	  newtree(self,
		  VALUE(expr_temp_used || i_use_setter, expr_temp == expr,
			expr_temp, expr_setter_rh),
		  gct_node_arg_END),
	  gct_node_arg_END);

  if (operator_on_p)
    {
      switch(self->type)
	{
	case GCT_NEGATE:
	  add_test(tests,
		   ne_test(first_probe, Default_state,
			   copy(expr_temp),
			   makeroot(GCT_CONSTANT, "0")));
	  first_probe++;
	  break;
	case GCT_UNARY_PLUS:
	  /* Nothing */
	  break;
	case GCT_BIT_NOT:
	case GCT_TRUTH_NOT:
	  if (INTEGER_TYPE == TREE_CODE(expr->gcc_type))
	    {
	      add_test(tests,
		       ne_test(first_probe, Default_state,
			       newtree(makeroot(GCT_BIT_NOT, "~"),
				       copy(expr_temp),
				       gct_node_arg_END),
			       newtree(makeroot(GCT_TRUTH_NOT, "!"),
				       copy(expr_temp),
				       gct_node_arg_END)));
	      first_probe++;
	    }
	  
	  break;
	default:
	  error("Unknown unary type.");
	  abort();
	  break;
	}
    }
  free_temp(expr_temp, expr);

  /* We never use the tempvar, but we do use the child's temp.  */
  return (i_use_setter);
}

/* ++, --, and unary &. */
int
exp_incdec(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  gct_node child;
  gct_node child_temp;
  gct_node child_setter_rh, child_tests;	/* For recursive calls. */
  int child_temp_used;				/* By child. */
  gct_node child_lvalue;			/* The thing to set. */

  *setter_rh = self;
  *tests = GCT_NULL_NODE;

  /* Need to avoid lvalue instrumentation of true arrays:  see file ARRAY.2D */
  if (   ARRAY_TYPE==TREE_CODE(GCT_OP_ONLY(self)->gcc_type)
      || FUNCTION_TYPE==TREE_CODE(GCT_OP_ONLY(self)->gcc_type))
    {
      return 0;
    }
  
  child = GCT_OP_ONLY(self);
  gct_remove_node(&(self->children), child);
  child_temp = temporary_id(child, CLOSEST, FORCE, NULL, NULL, WANT_BASE_TYPE);
  
  child_temp_used = DO_LVALUE_INSTRUMENT(self, child, Default_state, child_temp, &child_setter_rh, &child_tests, &child_lvalue);
  
  
  *setter_rh =
    comma(SETTER(child_temp_used, child_temp == child,
		 child_temp, child_setter_rh),
	  child_tests,
	  VALUE(child_temp_used, child_temp == child,
		child_temp, child_setter_rh),
	  newtree(self, child_lvalue, gct_node_arg_END),
	  gct_node_arg_END);
 
  free_temp(child_temp, child);

  return (0);
}



int
exp_cast(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  i_expr(self, GCT_CAST_EXPR(self));
  *setter_rh = self;
  *tests = GCT_NULL_NODE;
  return (0);
}


/*
 * exp_compound_expr is just a transducer that translates normal
 * (i_compound_stmt) instrumentation into a form useful to a caller
 * expecting expression instrumentation.  It is relevant only to 
 * GNU C compound statement ({...}) processing.
 */
int
exp_compound_expr(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  /* Assumption:  i_compound_statement does not change SELF (the compound node). */
  i_compound_statement(parent, self);
  *setter_rh = self;
  *tests = GCT_NULL_NODE;
  return (0);
}


/* The question mark operator.  Like an IF. */

int
exp_quest(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  gct_node quest_test;		/* The test to instrument. */
  gct_node test_setter_rh, test_tests;	/* Results of same. */
  gct_node new_quest_test;		/* Newly instrumented value. */
  int child_used;		/* Whether child needs its setter first. */
  gct_node test_temp;		/* Test variable in place of quest_test. */
  gct_node placeholder;		/* For remember_place. */

  int outside_macro = gct_outside_macro_p(self->first_char);
  int i_use_test_temp = branch_on() && outside_macro;
  
  *setter_rh = self;		/* We change nothing at top level. */
  *tests = GCT_NULL_NODE;	/* We add no tests at top level. */
    
  quest_test = GCT_QUEST_TEST(self);
  remember_place(self, quest_test, placeholder);
  
  test_temp = temporary_id(quest_test, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  
  child_used = DO_EXPR_INSTRUMENT(self, quest_test, Default_state, test_temp, &test_setter_rh, &test_tests);

  new_quest_test = comma(SETTER(child_used || i_use_test_temp, test_temp==quest_test,
			     test_temp, test_setter_rh),
		   test_tests,
		   VALUE(child_used || i_use_test_temp, test_temp==quest_test,
			 test_temp, test_setter_rh),
		   gct_node_arg_END);

  if (i_use_test_temp)
    {
      branch_map(Gct_next_index, self, FIRST);
      new_quest_test = comma(new_quest_test,
		      make_binary_probe(Gct_next_index++, copy(test_temp)),
		      copy(test_temp),
		      gct_node_arg_END);
      map_placeholder(Gct_next_index++);
    }


  free_temp(test_temp, quest_test);

  replace(self, new_quest_test, placeholder);
  DO_INSTRUMENT(self, GCT_QUEST_TRUE(self));
  DO_INSTRUMENT(self, GCT_QUEST_FALSE(self));

  return(0);		/* Setter does not need to go first. */
}


/* Utilities for simple and complex references. */

int 
reserve_substitution_tests(self, state, mapname, probes_so_far, duplicate)
     gct_node self;
     i_state state;
     char *mapname;
     int probes_so_far;
     int duplicate;
{
  tree matching_var;
  char *message;		/* For the mapfile. */

  while (matching_var = name_iterate(self->gcc_type, DONT_USE_GLOBAL))
    {
      char *match = DECL_PRINT_NAME(matching_var);
      
      if (self->text && !strcmp(match, self->text))
	continue;

      if (   state.integer_only
	  && INTEGER_TYPE != TREE_CODE(TREE_TYPE(matching_var)))
	continue;

      message = (char *) xmalloc(30+strlen(match));
      sprintf(message, "might be %s.", match);
      operand_map(probes_so_far, self, mapname, message, duplicate);
      free(message);
      probes_so_far++;
      duplicate = 1;
    }
  return probes_so_far;
}

int
add_substitution_tests(self, state, valuenode, first_probe, tests)
     gct_node self;
     i_state state;
     gct_node valuenode;
     int first_probe;
     gct_node *tests;
{
  tree matching_var;

  while (matching_var = name_iterate(self->gcc_type, DONT_USE_GLOBAL))
    {
      char *match = DECL_PRINT_NAME(matching_var);
      
      if (!strcmp(match, self->text))
	continue;

      if (   state.integer_only
	  && INTEGER_TYPE != TREE_CODE(TREE_TYPE(matching_var)))
	continue;

      add_test(tests,
	       ne_test(first_probe, state,
		       copy(valuenode),
		       makeroot(GCT_IDENTIFIER, match)));
      first_probe++;
    }
  return first_probe;
}

int
substitution_tests(self, state, valuenode, mapname, probes_so_far, tests, duplicate)
     gct_node self;
     i_state state;
     gct_node valuenode;
     char *mapname;
     int probes_so_far;
     gct_node *tests;
     int duplicate;
{
  int starting_probe;

  starting_probe = probes_so_far;
  probes_so_far = reserve_substitution_tests(self, state, mapname, probes_so_far, duplicate);
  add_substitution_tests(self, state, valuenode, starting_probe, tests);
  return probes_so_far;
}





/*
 * Add constancy test.  This is a simple version which ignores weak
 * sufficiency.  See the user's document.
 */

int
reserve_constancy_tests(self, mapname, probes_so_far, duplicate)
     gct_node self;
     char *mapname;
     int probes_so_far;
     int duplicate;
{
  if (   !NON_IMMEDIATE_P(self->gcc_type)
      && ON == gct_option_value(OPT_CONSTANTS))
    {
      operand_map(probes_so_far, self, mapname, "might be constant.",
		  duplicate);
      probes_so_far++;
    }
  return probes_so_far;
}


int
add_constancy_tests(self, state, valuenode, first_probe, tests)
     gct_node self;
     i_state state;
     gct_node valuenode;
     int first_probe;
     gct_node *tests;
{
  gct_node boolean;	/* Has this variable been seen before? */
  gct_node lastval;	/* What was its last value? */

  if (   !NON_IMMEDIATE_P(self->gcc_type)
      && ON==gct_option_value(OPT_CONSTANTS))
    {
      boolean = temporary_id(Int_root, CLOSEST, FORCE, "static", "=0", WANT_BASE_TYPE);
      lastval = temporary_id(self, CLOSEST, FORCE, "static", NULL, WANT_BASE_TYPE);
      
      /* (boolean?_G(lastval!=valuenode):boolean=1, lastval=valuenode) */
      
      add_test(tests,
	       comma(newtree(makeroot(GCT_QUEST, "?"),
			     copy(boolean),
			     ne_test(first_probe, Default_state,
				     copy(lastval), copy(valuenode)),
			     newtree(makeroot(GCT_SIMPLE_ASSIGN, "="),
				     copy(boolean),
				     makeroot(GCT_CONSTANT, "1"),
				     gct_node_arg_END),
			     gct_node_arg_END),
		     newtree(makeroot(GCT_SIMPLE_ASSIGN, "="),
			     copy(lastval),
			     copy(valuenode),
			     gct_node_arg_END),
		     gct_node_arg_END));
      first_probe++;
  
      free(boolean);
      free(lastval);
    }

  return first_probe;
}



int
constancy_tests(self, state, valuenode, mapname, probes_so_far, tests, duplicate)
     gct_node self;
     i_state state;
     gct_node valuenode;
     char *mapname;
     int probes_so_far;
     gct_node *tests;
     int duplicate;
{
  int starting_probe;

  starting_probe = probes_so_far;
  probes_so_far = reserve_constancy_tests(self, mapname, probes_so_far, duplicate);
  add_constancy_tests(self, state, valuenode, starting_probe, tests);
  return probes_so_far;
}


/* OPERAND INSTRUMENTATION */


/*
 * Cases:  
 *
 * 1. The tree is an IDENTIFIER leaf outside of a macro.  Perform operand
 * instrumentation.
 *
 * 2. The tree is within a macro, but is the root.  
 * 	
 * 	#define SIZE a
 * 	amount += SIZE;
 *
 * Perform operand instrumentation, but make sure the mapfile entry uses
 * SIZE instead of a.
 *
 * 3. The tree is an operand tree that's the root of a macro.  Treat it just
 * as if it were an atomic identifier.
 *
 * 	#define SIZE a[i].size
 * 	amount += SIZE;
 *
 * The appropriate instrumentation routine (exp_dotref in this case)
 * notices that it's in a macro and delegates to this routine.  The gct-map
 * entry will be SIZE.  The dotref tree is not descended, so any
 * descendents that are not in the macro are not instrumented.  This is
 * incorrect, strictly, but will probably never be noticed, because it
 * would require code like this:
 *
 * 	#define SIZE1 a
 * 	#define SIZE2 .size
 * 	amount += SIZE1[i>5?5:i]SIZE2;
 * 	
 * 4. The tree is within a macro, not the root.  This cannot happen if the
 * parent is an operand node (because of case 3).  It can happen if the
 * tree is an operator node:
 *
 * 	#define SIZE  a + 5
 * 	amount += SIZE;
 *
 * In this case, no instrumentation is done.  Note that this also applies
 * to a case like
 *
 * 	#define SIZE a[5] + 5
 *
 * because exp_arrayref will call this routine.
 */

int
exp_id(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  int first_index = Gct_next_index;
  int in_macro = gct_in_macro_p(self->first_char);
  /* Because of current kludgy implementation of macros, must get name now. */
  char *myname = in_macro ? gct_macro_name() : self->text;

  *setter_rh = self;
  *tests = GCT_NULL_NODE;

  if (in_macro && gct_in_macro_p(parent->first_char))
    {
      /* I'm embedded deep within a macro.  Don't do anything. */
      return 0;
    }

  if (operand_on())
    {
      if (!state.no_substitutions)
	{
	  Gct_next_index = substitution_tests(self, state, valuenode,
					      myname, Gct_next_index,
					      tests, 0);
	}
      if (!state.no_constant_checks)
	{
	  Gct_next_index = constancy_tests(self, state, valuenode,
					   myname, Gct_next_index, tests,
					   first_index < Gct_next_index);
	}
    }
  return (first_index < Gct_next_index && (valuenode!=self));
}


/*
 * This determines whether POSS_ZERO is a constant 0 being compared to a
 * pointer.  SUFF contains the comparison operator, if there is one.
 * At this point, the routine only works on constants, not on cast
 * expressions (which do not yet need it). 
 *
 * Notes:  we don't need to check whether the comparison operator is ==
 * or != (the cases of interest) -- if not, the c-typeck.c has already
 * printed
 *
 * 	warning: ordered comparison of pointer with integer zero
 *
 * so another couple won't hurt.  
 *
 */

int
zero_pointer_comparison(poss_zero, suff)
     gct_node poss_zero;
     i_suff *suff;
{
  if (   (i_suff *)0 == suff
      || 0 != strcmp(poss_zero->text, "0"))
    {
      return 0;
    }

  /*
   * DANGER:  if the other side is a temporary we made to use as a
   * pointer to some variable (as in exp_dotref), the gcc_type is 0.
   * (See temporary_id().)  Right now, there's no way that such a
   * temporary can be involved in a zero-pointer comparison.
   */
  
  if (   WEAK_OPERATOR_P(suff)
      && POINTER_TYPE == TREE_CODE(WEAK_VARIABLE(suff)->gcc_type))
    {
      /* Just in case this machine doesn't trap on zero dereferences. */
      assert(WEAK_VARIABLE(suff)->gcc_type);
      return 1;
    }
  
  return 0;
}

/* Constants, sizeof, alignof -- same as exp_constant. */
int
exp_constant(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  int first_index = Gct_next_index;
  int in_macro = gct_in_macro_p(self->first_char);
  /* Because of current kludgy implementation of macros, must get name now. */
  char *myname = in_macro ? gct_macro_name() : self->text;

  *setter_rh = self;
  *tests = GCT_NULL_NODE;

  if (in_macro && gct_in_macro_p(parent->first_char))
    {
      /* I'm embedded deep within a macro.  Don't do anything. */
      return 0;
    }

  assert(self->text);

  if (operand_on())
    {
      int myname_needs_freeing = 0;

      if (!in_macro && GCT_STRING_CONSTANT_P(self))
	{
	  myname = slashified_string(self->text, self->textlen);
	  myname_needs_freeing = 1;
	}
      
      if (   !state.no_substitutions
	  && GCT_CONSTANT == self->type)
	{
	  /* Special case:  don't do substition tests if 0 is being
	     compared to a pointer. */
	  if (!zero_pointer_comparison(self, top_suff(state)))
	    {
	      Gct_next_index = substitution_tests(self, state, valuenode,
						  myname, Gct_next_index,
						  tests, 0);
	    }
	}
      if (!empty_suff(state) && WEAK_OPERATOR_P(top_suff(state)))
	{
	  i_suff *suff = top_suff(state);
	  
	  operand_map(Gct_next_index, self, myname,
		      "might be another constant.",
		      first_index < Gct_next_index);
	  switch(WEAK_OPERATOR(suff)->type)
	    {
	    case GCT_LESSEQ:
	    case GCT_GREATER:
	    case GCT_GREATEREQ:
	    case GCT_LESS:
	      {
		gct_node equal = temporary_id(Int_root, CLOSEST, FORCE,
					      "static", "=0", WANT_BASE_TYPE);
		gct_node epsequal = temporary_id(Int_root, CLOSEST, FORCE,
					      "static", "=0", WANT_BASE_TYPE);

		gct_node epsilon_operator =
		  (   GCT_LESSEQ == WEAK_OPERATOR(suff)->type
		   || GCT_GREATER == WEAK_OPERATOR(suff)->type) ?
		     makeroot(GCT_MINUS, "-") : makeroot(GCT_PLUS, "+") ;

	        add_test(tests,
		  make_probe(Gct_next_index++,
		    comma(newtree(makeroot(GCT_BITOR_ASSIGN, "|="),
				  copy(equal),
				  newtree(makeroot(GCT_EQUALEQUAL, "=="),
					  copy(WEAK_VARIABLE(suff)),
					  copy(valuenode),
					  gct_node_arg_END),
				  gct_node_arg_END),
			  newtree(makeroot(GCT_BITOR_ASSIGN, "|="),
				  copy(epsequal),
				  newtree(makeroot(GCT_EQUALEQUAL, "=="),
					  copy(WEAK_VARIABLE(suff)),
					  newtree(epsilon_operator,
						  copy(valuenode),
						  epsilon(valuenode),
						  gct_node_arg_END),
					  gct_node_arg_END),
				  gct_node_arg_END),
			  newtree(makeroot(GCT_ANDAND, "&&"),
				  copy(epsequal),
				  copy(equal),
				  gct_node_arg_END),
			  gct_node_arg_END)));
		free(equal);
		free(epsequal);
	      }
	      break;
	      
	    case GCT_EQUALEQUAL:
	    case GCT_NOTEQUAL:
	      add_test(tests,
		 make_probe(Gct_next_index++,
		   newtree(makeroot(GCT_EQUALEQUAL, "=="),
			   copy(WEAK_VARIABLE(suff)),
			   copy(valuenode),
			   gct_node_arg_END)));
	      break;
	      
	    default:
	      error("Non-relational operator in state.weak_operator.");
	      abort();
	    }
	}

      if (myname_needs_freeing)
	{
	  free(myname);
	}
    }
  /* We used the valuenode if any tests were written.  */
  return (GCT_NULL_NODE != *tests);
}




int
exp_arrayref(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  int first_probe;	/* Index of first instrumentation of this node. */
  gct_node array, array_temp;		/* left hand expression. */
  gct_node index, index_temp;		/* Right hand expression */
  gct_node array_setter_rh, array_tests;	/* For recursive calls. */
  gct_node index_setter_rh, index_tests;	/* For recursive calls. */
  i_state child_state;	/* State to pass down. */
  char *myname;		/* Abbreviated name. */
  int operand_on_p;

  /* If we are in a macro, pretend we're an identifier. */
  if (gct_in_macro_p(self->first_char))
    {
      return exp_id(parent, self, state, valuenode, setter_rh, tests);
    }
  
  *setter_rh = self;
  *tests = GCT_NULL_NODE;

  if (   ARRAY_TYPE==TREE_CODE(self->gcc_type) /* See file ARRAY.2D */
      && TEMPORARY_NEEDED(GCT_ARRAY_ARRAY(self)))
  {
      return 0;
  }

  myname = make_mapname(self);

  array = GCT_ARRAY_ARRAY(self);
  gct_remove_node(&(self->children), array);
  array_temp = temporary_id(array, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  
  index = GCT_ARRAY_INDEX(self);
  gct_remove_node(&(self->children), index);
  index_temp = temporary_id(index, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);

  child_state = Default_state;
  child_state.suff_stack = state.suff_stack;
  set_ref_type(child_state, state, self);
  
  push_combiner(self, index_temp, 1, child_state);
  child_state.no_constant_checks = 1;	/* Arrays are often constant. */
  (void) DO_EXPR_INSTRUMENT(self, array, child_state, array_temp, &array_setter_rh, &array_tests);
  pop_suff(child_state);
  child_state.no_constant_checks = 0;
  

  operand_on_p = operand_on();
  if (operand_on_p)
    {
      /* Use up probe indices. */
      first_probe = Gct_next_index;

      /* Constancy checks obviously wanted.  For now, also doing
	 substitution tests. */

      Gct_next_index = reserve_substitution_tests(self, state, myname, Gct_next_index, 0);
      Gct_next_index = reserve_constancy_tests(self, myname, Gct_next_index, first_probe < Gct_next_index);

      if (!NON_IMMEDIATE_P(self->gcc_type))
	{
	  operand_map(Gct_next_index, self, myname,
		      "never differs from the previous element.",
		      first_probe<Gct_next_index);
	  Gct_next_index++;
	}
    }

  push_combiner(self, array_temp, 0, child_state);
  child_state.integer_only = 1;
  (void) DO_EXPR_INSTRUMENT(self, index, child_state,
				       index_temp, &index_setter_rh,
				       &index_tests);
  pop_suff(child_state);
  

  /*
   * We always put the setter first, because of weak sufficiency,
   * although there are cases where this is overkill.
   */
  {
    *setter_rh =
      comma(SETTER(1, array_temp == array,
		   array_temp, array_setter_rh),
	    SETTER(1, index_temp == index,
		   index_temp, index_setter_rh),
	    array_tests,
	    index_tests,
	    newtree(self,
		    VALUE(1, array_temp == array,
			  array_temp, array_setter_rh),
		    VALUE(1, index_temp == index,
			  index_temp, index_setter_rh),
		    gct_node_arg_END),
	    gct_node_arg_END);
  }
  
  if (operand_on_p)
    {
      first_probe = add_substitution_tests(self, state, valuenode,
				      first_probe, tests);
      first_probe = add_constancy_tests(self, state, valuenode,
				   first_probe, tests);


      if (!NON_IMMEDIATE_P(self->gcc_type))
	{
	  /* _G(index > 0 && array[index] != array[index-1]) */
	  add_test(tests,
		   make_probe(first_probe, 
			      newtree(makeroot(GCT_ANDAND, "&&"),
				      newtree(makeroot(GCT_GREATER, ">"),
					      copy(index_temp),
					      makeroot(GCT_CONSTANT, "0"),
					      gct_node_arg_END),
				      newtree(makeroot(GCT_NOTEQUAL, "!="),
					      copy(valuenode),
					      newtree(makeroot(GCT_ARRAYREF, "["),
						      copy(array_temp),
						      newtree(makeroot(GCT_MINUS, "-"),
							      copy(index_temp),
							      makeroot(GCT_CONSTANT, "1"),
							      gct_node_arg_END),
						      gct_node_arg_END),
					      gct_node_arg_END),
				      gct_node_arg_END)));
	  first_probe++;
	}
      
    }	
  free_temp(array_temp, array);
  free_temp(index_temp, index);
  free(myname);

  /* If we built any tests, we used the temp.  Force it. */
  return (GCT_NULL_NODE != *tests);
}

/* Note that the processing is stylized to follow the lexical order
   of the tree, although that's not really necessary. */

int
exp_deref(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  gct_node pointer, pointer_temp;		/* left hand expression. */
  gct_node pointer_setter_rh, pointer_tests;	/* For recursive calls. */
  i_state child_state;	/* State to pass down. */
  char *myname;		/* Abbreviated name. */
  int operand_on_p;
  
  /* If we are in a macro, pretend we're an identifier. */
  if (gct_in_macro_p(self->first_char))
    {
      return exp_id(parent, self, state, valuenode, setter_rh, tests);
    }
  
  *setter_rh = self;
  *tests = GCT_NULL_NODE;

  if (   ARRAY_TYPE==TREE_CODE(self->gcc_type) /* See file ARRAY.2D */
      && TEMPORARY_NEEDED(GCT_DEREFERENCE_ARG(self)))
  {
      return 0;
  }

  myname = make_mapname(self);

  pointer = GCT_DEREFERENCE_ARG(self);
  gct_remove_node(&(self->children), pointer);
  pointer_temp = temporary_id(pointer, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  
  operand_on_p = operand_on();

  child_state = Default_state;
  child_state.suff_stack = state.suff_stack;
  set_ref_type(child_state, state, self);
  
  push_combiner(self, GCT_NULL_NODE, 0, child_state);
  (void) DO_EXPR_INSTRUMENT(self, pointer, child_state, pointer_temp, &pointer_setter_rh, &pointer_tests);
  pop_suff(child_state);
  
  /*
   * We always put the setter first, because of weak sufficiency,
   * although there are cases where this is overkill.
   */
  {
    *setter_rh =
      comma(SETTER(1, pointer_temp == pointer,
		   pointer_temp, pointer_setter_rh),
	    pointer_tests,
	    newtree(self,
		    VALUE(1, pointer_temp == pointer,
			  pointer_temp, pointer_setter_rh),
		    gct_node_arg_END),
	    gct_node_arg_END);
  }

  if (operand_on_p)
    {
      /* Use up probe indices. */
      int first_probe = Gct_next_index;

      Gct_next_index = substitution_tests(self, state, valuenode, myname,
					  Gct_next_index, tests, 0);
      Gct_next_index = constancy_tests(self, state, valuenode, myname,
				       Gct_next_index, tests,
				       first_probe < Gct_next_index);
    }
  free_temp(pointer_temp, pointer);
  free(myname);

  /* If we built any tests, we used the temp.  Force it. */
  return (GCT_NULL_NODE != *tests);
}


/*
 * This creates field substitution tests for both dotrefs and arrowrefs.
 * (In the latter case the STRUCTURE argument is the pointer that's the
 * lefthand side of lh->rh.)
 *
 * No tests are created if LH is a union or union pointer.  If two fields
 * are the same type, they cannot be distinguished.  If they are of
 * different (but compatible) types, they are almost certain to have
 * different values (unless they're zero, which case is caught by
 * constancy tests [unless constancy tests are turned off]).
 */
int
field_substitution_tests(self, structure, field, state, valuenode, mapname, probes_so_far, tests, duplicate)
     gct_node self;
     gct_node structure;
     gct_node field;
     i_state state;
     gct_node valuenode;
     char *mapname;
     int probes_so_far;
     gct_node *tests;
     int duplicate;
{
  tree field_rover;
  tree this_field;
  tree structure_type;

  assert(self);
  assert(field);
  assert(valuenode);
  assert(mapname);
  assert(tests);
  assert(field->text);

  structure_type = structure->gcc_type;
  if (RECORD_TYPE != TREE_CODE(structure_type))
    {
      if (UNION_TYPE == TREE_CODE(structure_type))	/* Skip unions. */
	return probes_so_far;
      
      assert(POINTER_TYPE == TREE_CODE(structure_type));
      structure_type = TREE_TYPE(structure_type);

      if (UNION_TYPE == TREE_CODE(structure_type))	/* and pointers to unions */
	return probes_so_far;
      
      assert(RECORD_TYPE == TREE_CODE(structure_type));
    }
  
  /* First, we have to find type information for this field. */
  for(field_rover = TYPE_FIELDS(structure_type);
      field_rover;
      field_rover=TREE_CHAIN(field_rover))
    {
      assert(DECL_PRINT_NAME(field_rover));
      if (!strcmp(field->text, DECL_PRINT_NAME(field_rover)))
	{
	  this_field = field_rover;
	  break;
	}
    }

  /* Now, build the tests. */
  for(field_rover = TYPE_FIELDS(structure_type);
      field_rover;
      field_rover=TREE_CHAIN(field_rover))
    {
      if (field_rover == this_field)
	continue;

      if (   state.integer_only
	  && INTEGER_TYPE != TREE_CODE(TREE_TYPE(field_rover)))
	continue;

      if (comparison_compatible(TREE_TYPE(this_field), TREE_TYPE(field_rover)))
	  {
	    char *match = DECL_PRINT_NAME(field_rover);
	    char *message = (char *) xmalloc(30+strlen(match));
	    sprintf(message, "might use field %s.", match);
	    operand_map(probes_so_far, self, mapname, message, duplicate);
	    free(message);
	    add_test(tests,
		     ne_test(probes_so_far, state,
			     copy(valuenode),
			     newtree(copy(self),
				     copy(structure),
				     makeroot(GCT_IDENTIFIER, match),
				     gct_node_arg_END)));

	    probes_so_far++;
	    duplicate = 1;

	  }
	  
    }
  return probes_so_far;
}


/*
 * We must take care to return the original value, not a copy.  Consider,
 * for example, 
 *
 * 	A.f.k = 4;
 *
 * lv_dotref will be called on (A.f).k, and it will in turn call
 * exp_dotref on (A.f).  If A.f returned a copy, there would be no
 * side-effect to A in the instrumented program.
 *
 * This is also true of other compound operators:  arrowref, arrayref.
 * However, it falls out more naturally for them, since their
 * right-hand-side is always a pointer-to-object, never the object itself.
 */

/*
 * build_address_temporary(pointer_tmp, structure_setter)
 *
 * Given STRUCTURE_SETTER, this routine returns an assignment statement
 * that initializes POINTER_TMP.  The STRUCTURE_SETTER is perhaps
 * instrumented code; the main point of this routine is handling that.
 * See the postconditions.
 *
 * In below, call PREFIXED whatever tree we put the & before.
 *
 * PRECONDITIONS:
 * 1. STRUCTURE_SETTER is non-null.
 * 	On failure: assertion failure.
 * 2. POINTER_TMP is non-null
 * 	On failure: assertion failure.
 * 3. Neither node is linked into a list.
 * 	On failure: assertion failure.
 * 4. IF PREFIXED is a case-expression
 *    THEN note the design error.
 * 	((struct St)OTHER).i should be impossible.
 * POSTCONDITIONS
 * 1. IF STRUCTURE_SETTER is a comma operator of form (value..., structure)
 *    THEN transform to POINTER_TEMP = (value, &structure)
 *    ELSE transform to POINTER_TEMP = &STRUCTURE_SETTER.
 * 2. IF PREFIXED is a function
 *    THEN issue an error message (but produce the assignment described in 1).
 * 	This case cannot be handled with the current transformation
 * 	scheme.  It occurs when we're instrumenting code like
 * 	F().field.  This is rare, but it does occur.
 * 3. IF PREFIXED is an assignment
 *    THEN handle as case 2.
 * 	GCC accepts (struct1=struct2).i
 * 4. IF PREFIXED is a question-mark expression
 *    THEN handle as case 2
 * 	GCC accepts (test?struct1:struct2).i
 *
 * OBLIGATIONS:
 * 1. No copies are made of passed-in values; caller must not reuse them.
 * 2. Caller is responsible for freeing data.
 * 3. Caller should have no pointers into the structure_setter:  its structure
 *    may be changed.
 *
 * NOTES:
 * 1. The comma operator checked for above is created by GCT.  There
 * could also be a comma operator in the input (5, foo).i.  But I would
 * have to look inside the parentheses to see this, and that's past the
 * limit of what I'm willing to do.
 */
static gct_node
build_address_temporary(pointer_tmp, structure_setter)
	gct_node pointer_tmp;
	gct_node structure_setter;
{
  gct_node retval = GCT_NULL_NODE;	/* The pointer-setter expression. */
  gct_node prefixed = GCT_NULL_NODE;	/* What we put the address-of operator before. */
  
  assert(pointer_tmp);
  assert(structure_setter);
  assert(GCT_NULL_NODE == pointer_tmp->next);
  assert(GCT_NULL_NODE == structure_setter->next);

  if (GCT_COMMA == structure_setter->type)
  {
    prefixed = GCT_LAST(GCT_COMMA_OPERANDS(structure_setter));
    gct_remove_node(&GCT_COMMA_OPERANDS(structure_setter), prefixed);
    gct_add_last(&GCT_COMMA_OPERANDS(structure_setter),
		 newtree(makeroot(GCT_ADDR, "&"),
			 prefixed,
			 gct_node_arg_END));
    retval = newtree(makeroot(GCT_SIMPLE_ASSIGN, "="),
		     pointer_tmp,
		     structure_setter,
		     gct_node_arg_END);
  }
  else
  {
    prefixed = structure_setter;
    retval = newtree(makeroot(GCT_SIMPLE_ASSIGN, "="),
		   pointer_tmp,
		   newtree(makeroot(GCT_ADDR, "&"),
			   prefixed,
			   gct_node_arg_END),
		   gct_node_arg_END);
  }

  switch(prefixed->type)
  {
  case GCT_CASE:
    error("GCT design error:  ((struct structtype)<expr>).field thought impossible.");
    error("Resulting code should not compile.");
    break;
  case GCT_FUNCALL:
    error("GCT cannot handle function().field.");
    break;
  case GCT_SIMPLE_ASSIGN:
    error("GCT cannot handle (struct1=struct2).field.");
    break;
  case GCT_QUEST:
    error("GCT cannot handle (test?struct1:struct2).field.");
    break;
  }

  return retval;
}

int
exp_dotref(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  int first_probe;	/* Index of first instrumentation of this node. */
  gct_node structure, structure_temp;		/* left hand expression. */
  gct_node field;		/* Right hand expression */
  gct_node structure_setter_rh, structure_tests;	/* For recursive calls. */
  int structure_temp_used;	/* By child */
  int i_use_setter = 0;	/* Do I need the setter to come before tests? */
  i_state child_state;	/* State to pass down. */
  char *myname;		/* Abbreviated name. */
  int operand_on_p;
  
  
  /* If we are in a macro, pretend we're an identifier. */
  if (gct_in_macro_p(self->first_char))
    {
      return exp_id(parent, self, state, valuenode, setter_rh, tests);
    }
  
  *setter_rh = self;
  *tests = GCT_NULL_NODE;

  myname = make_mapname(self);
  structure = GCT_DOTREF_VAR(self);
  gct_remove_node(&(self->children), structure);
  structure_temp = temporary_id(structure, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  
  field = GCT_DOTREF_FIELD(self);
  gct_remove_node(&(self->children), field);

  
  child_state = Default_state;
  child_state.suff_stack = state.suff_stack;
  set_ref_type(child_state, state, self);

  push_combiner(self, field, 1, child_state);
  
  structure_temp_used = DO_EXPR_INSTRUMENT(self, structure, child_state, structure_temp, &structure_setter_rh, &structure_tests);
  pop_suff(child_state);
  

  operand_on_p = operand_on();
  if (operand_on_p)
    {
      
      i_use_setter = 1;
      
      /* Use up probe indices. */
      first_probe = Gct_next_index;

      Gct_next_index = constancy_tests(self, state, valuenode, myname, Gct_next_index, tests, FIRST);
      Gct_next_index = substitution_tests(self, state, valuenode, myname, Gct_next_index, tests, first_probe < Gct_next_index);
      
      Gct_next_index =
	field_substitution_tests(self, structure_temp, field, state, valuenode,
				 myname, Gct_next_index, tests,
				 first_probe < Gct_next_index);

    }


  if (structure_temp == structure)
    {
      *setter_rh =
	comma(structure_tests,
	      newtree(self,
		      structure_setter_rh,
		      field,
		      gct_node_arg_END),
	      gct_node_arg_END);
    }
  else
    {
      /* Construct an expression that returns the original value. */
      
      gct_node ptr_temp = temporary_id(structure_temp, CLOSEST, FORCE, NULL, NULL, WANT_POINTER_TYPE);
      

      /* Hack:  convert the node to preserve annotations. */
      self->type = GCT_ARROWREF;
      free(self->text);
      self->text = permanent_string("->");

      *setter_rh = comma(build_address_temporary(copy(ptr_temp),
						 structure_setter_rh),
			 /* Unneeded if there are no structure tests. */
			 (structure_temp_used || i_use_setter ? 
			   newtree(makeroot(GCT_SIMPLE_ASSIGN, "="),
				   copy(structure_temp),
				   newtree(makeroot(GCT_DEREFERENCE, "*"),
					   copy(ptr_temp),
					   gct_node_arg_END),
				   gct_node_arg_END) :
			   GCT_NULL_NODE),
			 structure_tests,
			 newtree(self, ptr_temp, field, gct_node_arg_END),
			 gct_node_arg_END);

    }
  
  
  free_temp(structure_temp, structure);
  free(myname);

  /* If we built any tests, we used the temp.  Force it. */
  /* If we had to create a temporary address, make sure it goes first. */
  return (GCT_NULL_NODE != *tests || structure_temp != structure);
}



int
exp_arrowref(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  int first_probe;	/* Index of first instrumentation of this node. */
  gct_node pointer, pointer_temp;		/* left hand expression. */
  gct_node field;		/* Right hand expression */
  gct_node pointer_setter_rh, pointer_tests;	/* For recursive calls. */
  int i_use_setter = 0;	/* Do I need the setter to come before tests? */
  i_state child_state;	/* State to pass down. */
  char *myname;		/* Abbreviated name. */
  int operand_on_p;
  
  
  /* If we are in a macro, pretend we're an identifier. */
  if (gct_in_macro_p(self->first_char))
    {
      return exp_id(parent, self, state, valuenode, setter_rh, tests);
    }
  
  *setter_rh = self;
  *tests = GCT_NULL_NODE;


  myname = make_mapname(self);

  pointer = GCT_ARROWREF_VAR(self);
  gct_remove_node(&(self->children), pointer);
  pointer_temp = temporary_id(pointer, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  
  field = GCT_ARROWREF_FIELD(self);
  gct_remove_node(&(self->children), field);

  
  child_state = Default_state;
  child_state.suff_stack = state.suff_stack;
  set_ref_type(child_state, state, self);

  push_combiner(self, field, 1, child_state);
  
  (void) DO_EXPR_INSTRUMENT(self, pointer, child_state, pointer_temp, &pointer_setter_rh, &pointer_tests);
  pop_suff(child_state);
  

  operand_on_p = operand_on();
  if (operand_on_p)
    {
      
      i_use_setter = 1;
      
      /* Use up probe indices. */
      first_probe = Gct_next_index;

      Gct_next_index = constancy_tests(self, state, valuenode, myname, Gct_next_index, tests, FIRST);
      Gct_next_index = substitution_tests(self, state, valuenode, myname, Gct_next_index, tests, first_probe < Gct_next_index);
      
      Gct_next_index =
	field_substitution_tests(self, pointer_temp, field, state, valuenode,
				 myname, Gct_next_index, tests,
				 first_probe < Gct_next_index);
    }

  /* Force setter first because of weak sufficiency. */
  *setter_rh =
      comma(SETTER(1, pointer_temp == pointer,
		   pointer_temp, pointer_setter_rh),
	    pointer_tests,
	    newtree(self,
		    VALUE(1, pointer_temp == pointer,
			  pointer_temp, pointer_setter_rh),
		    field,
		    gct_node_arg_END),
	    gct_node_arg_END);
  
  free_temp(pointer_temp, pointer);
  free(myname);

  return (i_use_setter);
}



int
exp_nothing(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  *tests = GCT_NULL_NODE;
  *setter_rh = self;
  return 0;
}


/*
 * Notes:
 *
 * 1. If the function is of void type, the valuenode must necessarily be NULL. 
 * 2. Call instrumentation cannot, at present, be on when this routine is
 * called, because of the separation between "weak" and "standard"
 * instrumentation.
 */
int
exp_funcall(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  gct_node rover;

  *setter_rh = self;
  *tests = GCT_NULL_NODE;
   
  if (GCT_IDENTIFIER != GCT_FUNCALL_FUNCTION(self)->type)	/* complex function name. */
    i_expr(self, self->children);

  /*
   * This loop works only because the function node is untouched
   * by it and thus is reliable for the stopping test.
   */
  rover = GCT_FUNCALL_ARGS(self);
  while (rover != self->children)
    {
      rover = rover->next;	/* Old value about to be modified in place */
      i_expr(self, rover->prev);
    }
  if (   add_writelog_on()
      && GCT_IDENTIFIER == GCT_FUNCALL_FUNCTION(self)->type
      && gct_exit_routine(GCT_FUNCALL_FUNCTION(self)->text))
    {
      if (GCT_FUNCALL_HAS_ARGS(self))
	{
	  standard_add_writelog(self, GCT_FUNCALL_LAST_ARG(self));
	}
      else
	{
	  *setter_rh = comma(make_logcall("gct_writelog"),
			     self,
			     gct_node_arg_END);
	}
    }
  return 0;	/* Setter does not need to precede tests. */
}


int
exp_impossible(parent, self, state, valuenode, setter_rh, tests)
     gct_node parent, self;
     i_state state;
     gct_node valuenode;
     gct_node *setter_rh, *tests;
{
  fprintf(stderr, "Impossible expression instrumentation.\n");
  abort();
}


		/* LVALUE INSTRUMENTATION ROUTINES */

/*
 * There's an exp_impossible and an lv_impossible so that debuggers, etc.
 * can get the argument list right.
 */

int
lv_impossible(parent, self, state, originalvalue, setter_rh, tests, lvalue)
     gct_node parent, self;
     i_state state;
     gct_node originalvalue;
     gct_node *setter_rh, *tests, *lvalue;
{
  fprintf(stderr, "Impossible lvalue instrumentation.\n");
  abort();
}

int
lv_id(parent, self, state, originalvalue, setter_rh, tests, lvalue)
     gct_node parent, self;
     i_state state;
     gct_node originalvalue;
     gct_node *setter_rh, *tests, *lvalue;
{
  *setter_rh = self;
  *tests = GCT_NULL_NODE;
  *lvalue = copy(self);
  return(0);
}

/*
 * We have to build setters even if no instrumentation is done, because
 * we're passing up both an expression to use to get the previous value
 * and an expression to set.
 */

int
lv_arrayref(parent, self, state, originalvalue, setter_rh, tests, lvalue)
     gct_node parent, self;
     i_state state;
     gct_node originalvalue;
     gct_node *setter_rh, *tests, *lvalue;
{
  gct_node array, array_temp;		/* left hand expression. */
  gct_node index, index_temp;		/* Right hand expression */
  gct_node array_setter_rh, array_tests;	/* For recursive calls. */
  gct_node index_setter_rh, index_tests;	/* For recursive calls. */
  i_state child_state;	/* State to pass down. */
  

  *setter_rh = self;
  *tests = GCT_NULL_NODE;

#ifdef WELL_BEHAVED_MACROS
  if (gct_in_macro_p(self->first_char))
    {
      fatal("Program error:  lv_arrayref called within a macro.");
    }
#endif

  array = GCT_ARRAY_ARRAY(self);
  gct_remove_node(&(self->children), array);
  array_temp = temporary_id(array, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  
  index = GCT_ARRAY_INDEX(self);
  gct_remove_node(&(self->children), index);
  index_temp = temporary_id(index, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);

  child_state = Default_state;
  child_state.no_constant_checks = 1;	/* Arrays are often constant. */
  DO_EXPR_INSTRUMENT(self, array, child_state, array_temp, &array_setter_rh, &array_tests);


  /* Note that, unlike arrays as values, the index expression is
     instrumented for any substitutions. */
  DO_EXPR_INSTRUMENT(self, index, Default_state,
		     index_temp, &index_setter_rh,
		     &index_tests);
  

  /* The setter must go first if it could have a side effect. */
  *setter_rh =
    comma(SETTER(1, array_temp == array, array_temp, array_setter_rh),
	  SETTER(1, index_temp == index, index_temp, index_setter_rh),
	  array_tests,
	  index_tests,
	  newtree(copy(self),
		  VALUE(1, array_temp == array, array_temp, array_setter_rh),
		  VALUE(1, index_temp == index, index_temp, index_setter_rh),
		  gct_node_arg_END),
	  gct_node_arg_END);
  *lvalue = newtree(self, copy(array_temp), copy(index_temp), gct_node_arg_END);
  
  free_temp(array_temp, array);
  free_temp(index_temp, index);

  /* No tests and we never use valuenode. */
  return (0);
}

int
lv_deref(parent, self, state, originalvalue, setter_rh, tests, lvalue)
     gct_node parent, self;
     i_state state;
     gct_node originalvalue;
     gct_node *setter_rh, *tests, *lvalue;
{
  gct_node pointer, pointer_temp;		/* The thing being deref'd. */
  gct_node pointer_setter_rh, pointer_tests;	/* For recursive calls. */
  i_state child_state;				/* For recursive calls. */
  
  
  *setter_rh = self;
  *tests = GCT_NULL_NODE;

#ifdef WELL_BEHAVED_MACROS
  if (gct_in_macro_p(self->first_char))
    {
      fatal("Program error:  lv_deref called within a macro.");
    }
#endif

  pointer = GCT_DEREFERENCE_ARG(self);
  gct_remove_node(&(self->children), pointer);
  pointer_temp = temporary_id(pointer, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  

  /*
   * We do not call for the child to do substitutions.   The act of
   * assignment produces the same information.  No weak sufficiency
   * information is passed down for the same reason.
   */ 
  child_state = Default_state;
  child_state.no_substitutions = 1;
  DO_EXPR_INSTRUMENT(self, pointer, child_state, pointer_temp, &pointer_setter_rh, &pointer_tests);

  /* The setter must go first if it could have a side effect. */
  *setter_rh =
    comma(SETTER(1, pointer_temp == pointer, pointer_temp, pointer_setter_rh),
	  pointer_tests,
	  newtree(copy(self),
		  VALUE(1, pointer_temp == pointer, pointer_temp, pointer_setter_rh),
		  gct_node_arg_END),
	  gct_node_arg_END);
  *lvalue = newtree(self, copy(pointer_temp), gct_node_arg_END);
  
  free_temp(pointer_temp, pointer);

  /* No tests and we never use valuenode. */
  return (0);
}





int
lv_dotref(parent, self, state, originalvalue, setter_rh, tests, lvalue)
     gct_node parent, self;
     i_state state;
     gct_node originalvalue;
     gct_node *setter_rh, *tests, *lvalue;
{
  gct_node structure, structure_temp;		/* left hand expression. */
  gct_node structure_setter_rh, structure_tests;	/* For recursive calls. */
  int structure_temp_used = 0;
  gct_node field;
  i_state child_state;	/* State to pass down. */
  
  *setter_rh = self;
  *tests = GCT_NULL_NODE;

  

#ifdef WELL_BEHAVED_MACROS
  if (gct_in_macro_p(self->first_char))
    {
      fatal("Program error:  lv_dotref called within a macro.");
    }
#endif

  structure = GCT_DOTREF_VAR(self);
  gct_remove_node(&(self->children), structure);
  structure_temp = temporary_id(structure, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  
  field = GCT_DOTREF_FIELD(self);
  gct_remove_node(&(self->children), field);

  /*
   * We instrument the structure part, we don't bother distinguishing
   * between identifiers.  There's no point in comparing S1.field to
   * S2.field, since the assignment itself will do so.
   *
   * Currently we don't do constant checks.  It's not clear when/if those
   * are appropriate.
   */
  child_state = Default_state;
  child_state.no_substitutions = 1;
  child_state.no_constant_checks = 1;
  structure_temp_used = DO_EXPR_INSTRUMENT(self, structure, child_state, structure_temp, &structure_setter_rh, &structure_tests);


  if (structure_temp == structure)
    {
      *setter_rh = comma(structure_tests,
			 newtree(copy(self),
				 structure_setter_rh,
				 copy(field),
				 gct_node_arg_END),
			 gct_node_arg_END);
      *lvalue = newtree(self, copy(structure), field, gct_node_arg_END);
      
      /* No tests and we never use valuenode. */
      return (0);
    }
  else
    {
      gct_node ptr_temp = temporary_id(structure_temp, CLOSEST, FORCE, NULL, NULL, WANT_POINTER_TYPE);
      

      /* Hack:  convert the node to preserve annotations. */
      self->type = GCT_ARROWREF;
      free(self->text);
      self->text = permanent_string("->");

      *setter_rh = comma(build_address_temporary(copy(ptr_temp),
						 structure_setter_rh),
			 /* Unneeded if there are no structure tests. */
			 (structure_temp_used ? 
			   newtree(makeroot(GCT_SIMPLE_ASSIGN, "="),
				   copy(structure_temp),
				   newtree(makeroot(GCT_DEREFERENCE, "*"),
					   copy(ptr_temp),
					   gct_node_arg_END),
				   gct_node_arg_END) :
			   GCT_NULL_NODE),
			 structure_tests,
			 newtree(copy(self), copy(ptr_temp), copy(field), gct_node_arg_END),
			 gct_node_arg_END);

      *lvalue = newtree(self, ptr_temp, field, gct_node_arg_END);

      free(structure_temp);
			 
      return (1);		/* We need the setter to go first. */
    }
}


int
lv_arrowref(parent, self, state, originalvalue, setter_rh, tests, lvalue)
     gct_node parent, self;
     i_state state;
     gct_node originalvalue;
     gct_node *setter_rh, *tests, *lvalue;
{
  gct_node pointer, pointer_temp;		/* left hand expression. */
  gct_node pointer_setter_rh, pointer_tests;	/* For recursive calls. */
  gct_node field;
  i_state child_state;	/* State to pass down. */
  
  
  *setter_rh = self;
  *tests = GCT_NULL_NODE;

#ifdef WELL_BEHAVED_MACROS
  if (gct_in_macro_p(self->first_char))
    {
      fatal("Program error:  lv_arrowref called within a macro.");
    }
#endif

  pointer = GCT_ARROWREF_VAR(self);
  gct_remove_node(&(self->children), pointer);
  pointer_temp = temporary_id(pointer, CLOSEST, REFERENCE_OK, NULL, NULL, WANT_BASE_TYPE);
  
  field = GCT_ARROWREF_FIELD(self);
  gct_remove_node(&(self->children), field);

  /*
   * We instrument the pointer part, we don't bother distinguishing
   * between identifiers.  There's no point in comparing S1->field to
   * S2->field, since the assignment itself will do so.
   */
  child_state = Default_state;
  child_state.no_substitutions = 1;
  (void) DO_EXPR_INSTRUMENT(self, pointer, child_state, pointer_temp, &pointer_setter_rh, &pointer_tests);


  /* Setter always goes first. */
  *setter_rh =
    comma(SETTER(1, pointer_temp == pointer,
		 pointer_temp, pointer_setter_rh),
	  pointer_tests,
	  newtree(copy(self),
		  VALUE(1, pointer_temp == pointer,
			pointer_temp, pointer_setter_rh),
		  copy(field),
		  gct_node_arg_END),
	  gct_node_arg_END);
  *lvalue = newtree(self, copy(pointer_temp), field, gct_node_arg_END);
  free_temp(pointer_temp, pointer);
      
  /* No tests and we never use valuenode. */
  return (0);
}
