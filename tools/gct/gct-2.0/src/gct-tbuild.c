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
$Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-tbuild.c,v 1.1 2003/10/28 09:48:45 arybchik Exp $
$Log: gct-tbuild.c,v $
Revision 1.1  2003/10/28 09:48:45  arybchik
Initial revision

Revision 1.1  2003/08/18 16:38:03  pabolokh
new tree is being added and committed

 * Revision 1.15  1992/09/01  13:42:17  marick
 * Add RS/6000 compiler workaround routine.
 *
 * Revision 1.14  1992/08/29  18:12:04  marick
 * GCT 1.4 alpha - first working version.
 *
 * Revision 1.13  1992/08/02  23:55:12  marick
 * Mapfile revision, part 1:  each file uses its own pointer into Gct_table.
 * - equivalent change is made for race coverage.
 *
 * Revision 1.12  1992/07/28  14:29:09  marick
 * Miscellaneous tidying.
 *
 * Revision 1.11  1992/05/25  17:42:40  marick
 * Lint.
 *
 * Revision 1.10  1992/05/24  17:07:53  marick
 * replaced_statement was a kludge.  It's no longer needed.
 *
 * Revision 1.9  1992/05/21  01:12:49  marick
 * 1. comma now sets its gcc_type to the type of its last child.
 * 2. Switch and loop handling moved here from gct-utrans.c.
 * 3. switch_case_test has test_needed argument:  some instrumentation is
 * done whether or not the case/default is in a macro, but all
 * instrumentation is done only if the case is outside a macro.
 *
 * Revision 1.8  1992/02/13  17:48:48  marick
 * Copylefted.
 *
 * Revision 1.7  91/09/09  13:06:17  marick
 * No change.
 * 
 * Revision 1.6  91/08/21  11:14:19  marick
 * Added a couple of goodies to build commonly used structures:
 * make_unconditional_incr and make_simple_statement.
 * 
 * Revision 1.5  91/08/07  14:49:06  marick
 * Cretinous SysV filename limits (need to reduce further for RCS).
 * 
 * Revision 1.4  91/04/14  13:29:55  marick
 * Bugfix:  Mapfile filename was wrong for code in #included files.
 * 
 * Revision 1.3  91/02/01  14:50:13  marick
 * Added array-substitutions option.
 * 
 * Revision 1.2  90/12/30  18:44:55  marick
 * ne_test now correctly guards or_guards with the cumulative and-guards.
 * See the commentary for ne_test for details.
 * 
*/

#include <stdio.h>
#include "gct-const.h"
#include "c-decl.h"
#include "gct-tutil.h"
#include "gct-contro.h"
#include "gct-assert.h"
#include "gct-files.h"
#include "gct-trans.h"
#include <varargs.h>

#ifdef TEST
#define xmalloc malloc
#endif

/* These routines build instrumentation trees. */


/* Number of next instrumentation to add. */
int Gct_next_index = 0;

/* Define the end-of-argument-list marker. */
struct gct_node_structure END_value;
gct_node gct_node_arg_END = &END_value;

/* Creates a new node with type TYPE and whose text is a copy of TEXT. */
gct_node
makeroot(type, text)
     enum gct_node_type type;
     char *text;
{
  gct_node root = gct_placeholder();

  assert(TYPE_IN_RANGE(type));
  
  root->type = type;
  if (text)
    {
      root->text = permanent_string(text);
      /* This is incorrect if the text contains embedded nulls. */
      root->textlen = strlen(root->text);
    }
  
  return root;
}



/*
 * newtree(root, [arg]...) 
 *
 * This routine builds a new tree from a set of nodes that are not
 * currently linked into any tree (though they may each be trees themselves).
 */
/* VARARGS */
gct_node 
newtree(va_alist)
     va_dcl
{
  va_list args;
  gct_node root;
  gct_node child;

  va_start(args);
  root = va_arg(args, gct_node);
  assert(root);
  assert(!root->next);
  assert(root!=gct_node_arg_END);

  for (child = va_arg(args, gct_node); child!=gct_node_arg_END; child = va_arg(args, gct_node))
    {
      if (child)
	{
	  assert(!child->next);
	  GCT_ADD(root, child);
	}
    }
  va_end(args);
  return root;
}

/*
 * This creates a fresh copy of the node.  No pointer values are copied:
 * the node has no annotations, no children, and is not linked into any
 * list.  It is an error for the node to originally have children.  The
 * filename, lineno, and first_char are set to 0 to indicate that this
 * node is to be on the same line as any preceding node.
 */
gct_node 
copy(node)
     gct_node node;
{
  gct_node duplicate = gct_placeholder();

  assert(node);
  assert(!node->children);

  *duplicate = *node;	/* By default, copy everything. */
  duplicate->next = duplicate->prev = duplicate->children = GCT_NULL_NODE;
  duplicate->text = permanent_string(node->text);
  duplicate->note = GCT_NULL_ANNOTATION;
  duplicate->filename = (char *)0;
  duplicate->lineno = 0;
  duplicate->first_char = 0;
  return duplicate;
}

/*
 * This creates a fresh copy of an entire list.  The resulting list has
 * no annotations.  The filename, lineno, and first_char of all nodes are
 * set to 0.  List may be empty.
 *
 * This is named gct_copylist because there's a copy_list in gcc proper.
 * I should have used naming conventions consistently.
 */
static 
gct_node 
gct_copylist(list)
     gct_node list;
{
  static gct_node header = GCT_NULL_NODE; /* Holds the list */
  gct_node rover;

  if (GCT_NULL_NODE == header)
    {
      header = gct_placeholder();	
    }
  header->children = GCT_NULL_NODE;
  
  rover = list;
  if (rover)
    {
      do
	{
	  GCT_ADD(header, copytree(rover));
	  rover = rover->next;
	}
      while (rover != list);
    }
  
  return header->children;
}

/*
 * This creates a fresh copy of an entire tree.  The resulting tree has
 * no annotations and is not linked into any list.  The filename, lineno,
 * and first_char of all nodes are set to 0.
 *
 * It may seem stupid to have a separate copy() and copytree(), but the
 * no-children check in copy catches a lot of bugs.
 *
 * Note:  This routine cannot call gct_copylist because gct_copylist calls this
 * and gct_copylist is not reentrant.
 */
gct_node 
copytree(node)
     gct_node node;
{
  gct_node newroot = gct_placeholder();
  gct_node rover;

  assert(node);

  *newroot = *node;	/* By default, copy everything. */
  newroot->next = newroot->prev = newroot->children = GCT_NULL_NODE;
  if (node->text)
    {
      newroot->text = permanent_string(node->text);
    }
  newroot->note = GCT_NULL_ANNOTATION;
  newroot->filename = (char *)0;
  newroot->lineno = 0;
  newroot->first_char = 0;

  rover = node->children;
  if (rover)
    {
      do
	{
	  GCT_ADD(newroot, copytree(rover));
	  rover = rover->next;
	}
      while (rover != node->children);
    }
  
  return newroot;
}

/*
 * This returns a node representing an epsilon to add to NODE.
 * Currently, epsilon is always 1.
 */

gct_node
epsilon(node)
     gct_node node;
{
  return makeroot(GCT_CONSTANT, "1");
}


/*
 * Many tests require the conversion of a non-boolean into a boolean.
 * This is done by boolean-negating it twice.  This routine returns such
 * a tree.  Note that the passed-in node is used, not a copy.
 */
gct_node
notnot(node)
     gct_node node;
{
  return newtree(makeroot(GCT_TRUTH_NOT, "!"),
		 newtree(makeroot(GCT_TRUTH_NOT, "!"),
			 node,
			 gct_node_arg_END),
		 gct_node_arg_END);
}

/*
 * comma([arg]...)
 *
 * This routine links all the args as arguments to a newly-created COMMA
 * node.  None of the arguments may be linked into any list.
 *
 * As a special case, if the comma expression would have only a single
 * element, that single element is returned.
 *
 * No children returns an empty comma list.  This is used by routines
 * that build the comma list one element at a time.
 *
 * The tree that comma returns has the correct gct_type (the type of the
 * last expression in the comma list).
 */
/* VARARGS */
gct_node 
comma(va_alist)
     va_dcl
{
  va_list args;
  gct_node root = makeroot(GCT_COMMA, ",");
  gct_node child;

  va_start(args);
  for (child = va_arg(args, gct_node); child!=gct_node_arg_END; child = va_arg(args, gct_node))
    {
      if (child)
	{
	  assert(!child->next);
	  GCT_ADD(root, child);
	}
    }
  va_end(args);

  if (root->children)
    {
      root->gcc_type = GCT_LAST(root->children)->gcc_type;
    }

  if (root->children && root->children == root->children->next)
    {
      child = root->children;
      gct_unlink(child);
      free(root);
      root=child;
    }
  return root;
}


/*
 * compound([arg]...)
 *
 * This routine links all the args as statements below a newly-created
 * GCT_COMPOUND_STATEMENT node.  None of the arguments may be linked into
 * any list.  It is an error for there to be no arguments.
 */
/* VARARGS */
gct_node 
compound(va_alist)
     va_dcl
{
  va_list args;
  gct_node root = makeroot(GCT_COMPOUND_STMT, "");
  gct_node child;

  GCT_ADD(root, makeroot(GCT_OTHER, "{"));
			   
  va_start(args);
  for (child = va_arg(args, gct_node); child!=gct_node_arg_END; child = va_arg(args, gct_node))
    {
      if (child)
	{
	  assert(!child->next);
	  GCT_ADD(root, child);
	}
    }
  va_end(args);

  /* Something had better have been added. */
  assert(root->children->next != root->children);	

  GCT_ADD(root, makeroot(GCT_OTHER, "}"));

  return root;
}


/* See below. */
gct_node 
make_named_probe(index, test, name)
     int index;
     gct_node test;
     char *name;
{
  static char indexstring[12];

  assert(index>=0);
  assert(test);
  assert(!test->next);
  
  sprintf(indexstring, "%d", index);
  return newtree(makeroot(GCT_FUNCALL, (char *) 0),
		 makeroot(GCT_IDENTIFIER, name),
		 makeroot(GCT_CONSTANT, permanent_string(indexstring)),
		 test,
		 gct_node_arg_END);
}



/*
 * This function builds this function call:
 *
 * _G(index, <test>)
 *
 * _G is usually a macro, though.
 *
 * Test itself is used, not a copy.
 */
gct_node 
make_probe(index, test)
     int index;
     gct_node test;
{
  return make_named_probe(index, test, G_MARK1);
}


/*
 * This function builds this function call:
 *
 * _G2(index, <test>)
 *
 * Test itself is used, not a copy.
 */
gct_node 
make_binary_probe(index, test)
     int index;
     gct_node test;
{
  return make_named_probe(index, test, G_MARK2);
}


/*
 * This function builds this function call:
 *
 * GCT_SET(index, 1)
 *
 */
gct_node 
make_unconditional_incr(index)
     int index;
{
  static char indexstring[12];

  assert(index>=0);
  
  sprintf(indexstring, "%d", index);
  return newtree(makeroot(GCT_FUNCALL, (char *) 0),
		 makeroot(GCT_IDENTIFIER, "GCT_INC"),
		 makeroot(GCT_CONSTANT, permanent_string(indexstring)),
		 gct_node_arg_END);
}

/*
 * This function builds this function call:
 *
 * <name>(<logfile>)
 *
 */
gct_node 
make_logcall(name)
     char *name;
{
  extern char *Gct_log_filename;
  
  assert(name);
  return newtree(makeroot(GCT_FUNCALL, (char *) 0),
		 makeroot(GCT_IDENTIFIER, permanent_string(name)),
		 makeroot(GCT_CONSTANT, permanent_string(Gct_log_filename)),
		 gct_node_arg_END);
}


/*
 * This function builds a simple statement from a node.  Essentially,
 * it tacks a ";" on to the end.  The node must not be in any list.
 */

gct_node
make_simple_statement(node)
	gct_node node;
{
    assert(node);
    assert(!node->next);

    return newtree(makeroot(GCT_SIMPLE_STMT, (char *)0),
		   node,
		   gct_node_arg_END);
}



/*
 * This routine takes a list of nodes and combines them with a node of
 * the given type and string.
 */
gct_node
binarify(list, type, string)
     gct_node list;
     enum gct_node_type type;
     char *string;
{
  gct_node left;
  gct_node right;
  
  if (GCT_NULL_NODE == list)		/* Empty list -- caller handles. */
    return list;

  if (list == list->next)		/* singleton list, no combination */
    {
      gct_unlink(list);
      return list;
    }

  left = list;
  gct_remove_node(&list, list);
  right = binarify(list, type, string);
  return newtree(makeroot(type, string),
		 left, right, gct_node_arg_END);
}


/*
 * This returns true if the user wants this particular variable to be
 * dereferenced, even if the pointer might point to garbage.
 *
 * Currently, the decision is independent of the variable, set by control
 * options. 
 */
allow_dangerous_deref(ref)
     gct_node ref;
{
  return (ON == gct_option_value(OPT_DEREF));
}

/*
 * This is like allow_dangerous_deref, but more specific:  it applies
 * only to array substitution dereferences:
 *
 * 	A[<expr>] might be B[<expr>]
 *
 * It is (I think) often the case that turning these substitutions off
 * will eliminate all core-dump-causing dereferences, while still
 * allowing some dereferences.
 */
allow_array_substitution(ref)
     gct_node ref;
{
  return (ON == gct_option_value(OPT_ARRAY_SUBSTITUTION));
}

/*
 * NE_TEST
 *
 * This function builds this function call:
 *
 * _G(index, <test>)
 *
 * orig and new are used in the tree; copies are not made.  The STATE is
 * an I_STATE that controls the building of the test.
 *
 * In the simplest form, test is ORIG != NEW.  The more complex forms all
 * have the form F(ORIG) != F(NEW), where F is a function that returns a
 * tree structure.  ORIG and NEW have comparision-compatible types.
 * (That is, they are either the same identical type or ones that can be
 * compared with != -- for example, float and int.)  
 *
 * However, in some cases, ORIG and NEW are identical, but cannot
 * be compared: consider structures, for example.  In a later
 * enhancement, we will allow type-specific, user-supplied, comparison
 * routines.  For the moment, we build a test that always succeeds
 * (though one could argue that it should always fail, and that the user
 * should decide whether the variable is correct).
 *
 * Complex forms:
 *
 * Complex forms implement weak sufficiency.  Weak sufficiency has two forms:
 *
 * Comparison:  We generate code like A < var != B < var.  The comparison is
 * passed down in the state.
 *
 * Structure: We generate code like (*A)[1] != (*B)[1].  The surrounding
 * tree structure is passed down in the state, as is the type of the
 * resulting structure.  (And, if that type is not one with an inequality
 * test, we build an always-succeeding test, as described above.)
 *
 * Of course, comparison and structure may both hold.  However, the
 * comparison F is always the last thing to be built -- weak sufficiency
 * never extends across operators.  You'll not find anything like
 *
 * 	Array[A<1] != Array[B<1]
 *
 * That's because there's no evidence that would be useful, not because
 * it's particularly more difficult to do.
 *
 * Building complex structures presents problems with pointer
 * dereferencing: what if the NEW pointer has a garbage value?  How do we
 * avoid core dumps?  For safety, the user can control this -- see
 * allow_dangerous_deref and allow_array_substitutions.  This, however,
 * will also rule out many useful test conditions.  Therefore, the actual
 * structure references are guarded by short-circuiting tests.  These are
 * of two kinds:
 *
 * OR-Guards:  these are tests which, if they succeed, demonstrate that
 * ORIG and NEW are distinct.  For example, if ORIG is a valid pointer
 * and NEW is zero, we know that ORIG[A] != NEW[A] and the test condition
 * is ruled out.
 *
 * And-Guards:  these are tests which, if they fail, demonstrate that it
 * is not *known* to be safe to make the inequality comparison.  However,
 * we don't have the information to rule out the test condition.  For
 * example, suppose we were to have this substitution-test:
 *
 * 	Array[ORIG] != Array[NEW]
 *
 * We don't want to run this test if NEW > ORIG, since ORIG may be out of
 * the array bounds (and Array[NEW] may be in unallocated memory).
 *
 * Algorithm:
 *
 * The state consists of a stack of "suff" structures that contain
 * information about one level of weak sufficiency.  We climb up the
 * stack, building ever more complex versions of ORIG and NEW, also
 * accumulating OR-Guards and And-Guards.  These accumulating versions of 
 * ORIG and NEW might look like:
 *
 * 		ORIG
 * 		Array[ORIG]
 * 		*(Array[ORIG])
 * 		(*(Array[ORIG]))->field
 *
 * At any particular point in the building, we refer to Fo(ORIG) as the
 * value so far and Fn(ORIG) as the value built at that step.
 *
 * If the suff structure describes a relational operator, the tree to be
 * built is simple.
 *
 * If the suff structure describes a structure combining form, there are
 * several cases:
 *
 * ORIG/NEW are 
 *
 * 	to be dereferenced 		*ORIG
 *
 * Fn(ORIG) = *Fo(ORIG), similarly for NEW.  An Or-guard is added:
 * !Fo(New).  For example, we would arrive at this test:
 *
 * 	!NEW || *ORIG != *NEW
 *
 * (The story is not quite so simple, though: see Guarding the Guards below.)
 *
 * 	left-hand side of array 	ORIG[...]
 *
 * Fn(ORIG) = Fo[...], similarly for NEW.  The OR-guard is 
 * !Fo(NEW).  No And-guard.  The resulting test might be
 *
 * 	!NEW || ORIG[A] != NEW[A]
 *
 * Note that we're making a big assumption here -- that an index for ORIG
 * is valid index for NEW.  This is often untrue, but usually harmless.
 * It can, however, cause core dumps, in which case the user must
 * reinstrument turning array substitutions off (option -array-substitutions).
 *
 * 	right-hand side of array 	...[ORIG]
 *
 * Fn(ORIG) = ...[Fo(Orig)], similarly for NEW.
 * Or-Guard:  Fo(NEW)<0.  And-Guard:  Fo(NEW)<Fo(ORIG).  For example:
 *
 * NEW<0 || (NEW < ORIG && Array[NEW] != Array[ORIG])
 *
 * 	arrow-dereferenced		ORIG->field
 *
 * As with ordinary dereferencing.
 *
 * 	dot-dereferenced		ORIG.field
 *
 * As with arrow-dereferencing, except that no OR-guard is required since
 * ORIG is not a pointer.
 *
 *
 * Guarding the Guards:
 *
 * Guards are always added to the end of the current list.  In the case
 * of AND-Guards, it's easy to see that the earlier guards protect the
 * later guards.  For example, for a two dimensioned array, we'd get this
 * and-guard structure:
 *
 * 	A[B[ORIG]]
 *
 * 	NEW < ORIG && B[NEW] < B[ORIG] && A[B[NEW]] != A[B[ORIG]]
 *
 * This is not necessarily the case for OR-guards.  Consider
 *
 * 	*(A[ORIG])
 *
 * 	NEW < 0 || !A[NEW] || (NEW < ORIG && *(A[ORIG]) != *(A[NEW]))
 *
 * If NEW is, for example, a mask like 0xFFFFFFFF, we will likely get a
 * core dump.  The solutions is to guard every OR-guard with the
 * accumulated AND-guard (Fo(AND-GUARD)).  This would result in this test:
 *
 * NEW < 0 || (NEW < ORIG && !A[NEW]) || (NEW < ORIG && *(A[ORIG]) != *(A[NEW]))
 *
 * This is not as token-efficient as it could be, but the situation
 * arises rather seldom.
 */

/*
 * This updates the OR_GUARDS with the NEW_OR_GUARD, using AND_GUARDS as
 * described above. 
 */
add_or_guard(or_guards, and_guards, new_or_guard)
     gct_node *or_guards;
     gct_node and_guards;
     gct_node new_or_guard;
{
  if (GCT_NULL_NODE == and_guards)
    {
      gct_add_last(or_guards, new_or_guard);
    }
  else
    {
      gct_add_last(or_guards,
		   newtree(makeroot(GCT_ANDAND, "&&"),
			   binarify(gct_copylist(and_guards), GCT_ANDAND, "&&"),
			   new_or_guard,
			   gct_node_arg_END));
    }
}


gct_node 
ne_test(index, state, orig, new)
     int index;
     i_state state;
     gct_node orig, new;
{
  gct_node orig_so_far;	/* What we build from orig. */
  gct_node new_so_far;	/* What we build from new. */
  gct_node retval;
  tree result_type;	/* Type used in the comparison. */
  i_suff *suff;		/* A sufficiency node. */
  gct_node or_guards = GCT_NULL_NODE;	/* Check for array indices out of bounds below. */
  gct_node and_guards = GCT_NULL_NODE;	/* Check for array indices out of bounds above. */
  
  
  assert(orig);
  assert(new);
  assert(!orig->next);
  assert(!new->next);

  if (state.ref_type)
    result_type = state.ref_type;
  else
    result_type = orig->gcc_type;

  /* It's not at all clear what to do with arguments which are not
     comparable with !=.  For the moment, we generate an automatic
     true test, which will make it easier to add, e.g.,  flavor-specific
     code. */
  if (   result_type && NON_IMMEDIATE_P(result_type))
      {
	return make_probe(index, makeroot(GCT_CONSTANT, "1"));
      }

  /* Now we know that the result will be something that can be compared
     with != .*/
  orig_so_far = orig;
  new_so_far = new;

  /*
   * Suppose a pointerish variable is to be dereferenced.  By default,
   * this is allowed, although an OR-guard against null pointers is
   * added.  However, it may be turned off, in which case we
   * short-circuit weak-sufficiency.
   */

  suff = state.suff_stack;
  if (   suff
      && DEREFERENCE_NEEDED(suff))
    {
      /* We may turn off only array-name substitutions. */
      if (   GCT_ARRAYREF == WEAK_ROOT(suff)->type
	  && WEAK_ME_FIRST(suff)
	  && !allow_array_substitution(new))
	{
	  suff = (i_suff *) 0;
	}
      /* Or we may turn off everything */
      else if (!allow_dangerous_deref(new))
	{
	  suff = (i_suff *) 0;
	}
    }


  for (; suff; suff = suff->next)
    {
      if (WEAK_COMBINER_P(suff))
	{
	  /* Add guards if needed. */
	  if (WEAK_ARRAY_INDEX(suff))
	    {
	      add_or_guard(&or_guards, and_guards,
			   newtree(makeroot(GCT_LESS, "<"),
				   copytree(new_so_far),
				   makeroot(GCT_CONSTANT, "0"),
				   gct_node_arg_END));
	      gct_add_last(&and_guards,
			   newtree(makeroot(GCT_LESS, "<"),
				   copytree(new_so_far),
				   copytree(orig_so_far),
				   gct_node_arg_END));
	    }
	  else if (DEREFERENCE_NEEDED(suff))
	    {
	      assert(   GCT_ARRAYREF == WEAK_ROOT(suff)->type
		     || GCT_DEREFERENCE == WEAK_ROOT(suff)->type
		     || GCT_ARROWREF == WEAK_ROOT(suff)->type);
	      add_or_guard(&or_guards, and_guards,
			   newtree(makeroot(GCT_TRUTH_NOT, "!"),
				   copytree(new_so_far),
				   gct_node_arg_END));
	    }
	  

	  /* Build the expression. */
	  if (WEAK_OTHER_SIDE(suff))
	    {
	      if (WEAK_ME_FIRST(suff))
		{
		  orig_so_far = newtree(copy(WEAK_ROOT(suff)),
				      orig_so_far,
				      copy(WEAK_OTHER_SIDE(suff)),
				      gct_node_arg_END);
		  new_so_far = newtree(copy(WEAK_ROOT(suff)),
				       new_so_far,
				       copy(WEAK_OTHER_SIDE(suff)),
				       gct_node_arg_END);
		}
	      else
		{
		  orig_so_far = newtree(copy(WEAK_ROOT(suff)),
				      copy(WEAK_OTHER_SIDE(suff)),
				      orig_so_far,
				      gct_node_arg_END);
		  new_so_far = newtree(copy(WEAK_ROOT(suff)),
				       copy(WEAK_OTHER_SIDE(suff)),
				       new_so_far,
				       gct_node_arg_END);
		}
	    }
	  else
	    {
	      orig_so_far = newtree(copy(WEAK_ROOT(suff)), orig_so_far, gct_node_arg_END);
	      new_so_far = newtree(copy(WEAK_ROOT(suff)), new_so_far, gct_node_arg_END);
	    }
	}
      else if (WEAK_OPERATOR_P(suff))
	{
	  orig_so_far = newtree(copy(WEAK_OPERATOR(suff)),
			      orig_so_far,
			      copy(WEAK_VARIABLE(suff)),
			      gct_node_arg_END);
      
	  new_so_far = newtree(copy(WEAK_OPERATOR(suff)),
			       new_so_far,
			       copy(WEAK_VARIABLE(suff)),
			       gct_node_arg_END);
	  assert(!suff->next);	/* This better be the top of the stack. */
	}
    }

  or_guards = binarify(or_guards, GCT_OROR, "||");
  and_guards = binarify(and_guards, GCT_ANDAND, "&&");
  
  retval = newtree(makeroot(GCT_NOTEQUAL, "!="),
		   orig_so_far,
		   new_so_far,
		   gct_node_arg_END);

  if (and_guards)
    retval = newtree(makeroot(GCT_ANDAND, "&&"),
		     and_guards,
		     retval,
		     gct_node_arg_END);
  if (or_guards)
    retval = newtree(makeroot(GCT_OROR, "||"),
		     or_guards,
		     retval,
		     gct_node_arg_END);
  
  retval = make_probe(index, retval);
  return retval;
}


/*
 * It is often the case that tests are added one at a time.  This routine
 * is useful in such a case.  Precondition:  *tests must be GCT_NULL_NODE
 * if this is the first test to be added.  
 */

void
add_test(tests, new_test)
     gct_node *tests;
     gct_node new_test;
{
  if (GCT_NULL_NODE == new_test)
    return;
  
  if (GCT_NULL_NODE == *tests)
    {
      *tests = comma(gct_node_arg_END);
    }
  gct_add_last(&((*tests)->children), new_test);
}


/*
 * 	Loop instrumentation is somewhat complex.  We use four 
 * 	slots in the probe table, having these meanings:
 * 	loop not taken
 * 	loop taken at least once
 * 	loop taken exactly once
 * 	loop taken at least twice
 * 	Code (conceptually) looks like this:
 *
 * 	{
 *   	  counter = 0;
 * 	  if at-least-once	- Must have left with non-local exit.
 * 		at-least-once = 0
 * 		INCR(exactly-once)
 * 	  while(counter++, test
 * 		  counter==1&&!test ==> INCR(not-taken)
 * 		  counter==1&&test  ==> at-least-once = 1
 * 		  counter==2&&!test ==> at-least-once = 0, INCR(exactly-once)
 * 		  counter==2&&test  ==> at-least-once = 0, INCR(at-least-twice)
 * 	  if (at-least-once)	- Must have left with a break.
 * 		at-least-once = 0
 * 		INCR(exactly-once)
 * 	  counter = 0;
 *         }
 *
 * 	The reporting tool must also know about at-least-once.
 * 	NOTE:  at-least-once is used because we cannot rely on any of
 * 	the fields being larger than a bit (in which case the user will
 * 	have defined INCR(x) as calc-bit(x) = 1
 *
 * WARNING:  Some of the tests (instrumentation/branch/loop*.c) know that
 * this common code tests all looping constructs, so they use only while
 * in the test.  If you un-common-ize this code, you should write more tests.
 */
/*
 * NOTES:
 * 1.  The counter is declared in the OUTERMOST scope, not the CLOSEST,
 * because it is an initialized non-static.  The closest scope may be
 * within a switch, in which case the initializer will be in no case and
 * thus will never be executed.  Consider this:
 *
 * function(c)
 * {
 *   switch(c)
 *     {
 *     default:
 *       for(;c>0;c--)
 * 	{
 * 	  out(c);
 * 	}
 *     }
 * }
 *
 */

void
add_loop_test(parent, self, looptest, test_temp, first_index)
     gct_node parent, self, looptest, test_temp;
     int first_index;
{
  gct_node placeholder;		/* For remember_place */
  gct_node counter = temporary_id(Int_root, OUTERMOST, FORCE, NULL, "=0", WANT_BASE_TYPE);
  int not_taken = first_index;
  int at_least_once = first_index+1;
  int exactly_once = first_index+2;
  int at_least_twice = first_index+3;
  char not_taken_name[20];
  char at_least_once_name[20];
  char exactly_once_name[20];
  char at_least_twice_name[20];
  gct_node newlooptest;

  sprintf(not_taken_name, "%d", not_taken);
  sprintf(at_least_once_name, "%d", at_least_once);
  sprintf(exactly_once_name, "%d", exactly_once);
  sprintf(at_least_twice_name, "%d", at_least_twice);

  /* First, modify the loop test. */
  remember_place(self, looptest, placeholder);
  {
    gct_node test1, action1, test2, action2, test3, action3, test4, action4;

#ifdef GCT_AIX_BUG
    newlooptest =
      comma(looptest,
	    newtree(makeroot(GCT_FUNCALL, (char *) 0),
		    makeroot(GCT_IDENTIFIER, "gct_aix_bug_looptest"),
		    newtree(makeroot(GCT_ADDR, "&"),
			    copy(counter),
			    gct_node_arg_END),
		    copy(test_temp),
		    makeroot(GCT_IDENTIFIER, "GCT_TABLE_POINTER_FOR_THIS_FILE"),
		    makeroot(GCT_IDENTIFIER, not_taken_name),
		    gct_node_arg_END),
	    gct_node_arg_END);
		    
#else
    
    test1 =
      newtree(makeroot(GCT_ANDAND, "&&"),
	      newtree(makeroot(GCT_EQUALEQUAL, "=="),
		      copy(counter),
		      makeroot(GCT_CONSTANT, "1"),
		      gct_node_arg_END),
	      newtree(makeroot(GCT_TRUTH_NOT, "!"),
		      copy(test_temp),
		      gct_node_arg_END),
	      gct_node_arg_END);
    action1 =
      newtree(makeroot(GCT_FUNCALL, (char *) 0),
	      makeroot(GCT_IDENTIFIER, G_INC),
	      makeroot(GCT_CONSTANT, not_taken_name),
	      gct_node_arg_END);
    

    test2 =
      newtree(makeroot(GCT_ANDAND, "&&"),
	      newtree(makeroot(GCT_EQUALEQUAL, "=="),
		      copy(counter),
		      makeroot(GCT_CONSTANT, "1"),
		      gct_node_arg_END),
	      copy(test_temp),
	      gct_node_arg_END);
    action2 =
      newtree(makeroot(GCT_FUNCALL, (char *) 0),
	      makeroot(GCT_IDENTIFIER, G_SET),
	      makeroot(GCT_CONSTANT, at_least_once_name),
	      makeroot(GCT_CONSTANT, "1"),
	      gct_node_arg_END);
    

    test3 =
      newtree(makeroot(GCT_ANDAND, "&&"),
	      newtree(makeroot(GCT_EQUALEQUAL, "=="),
		      copy(counter),
		      makeroot(GCT_CONSTANT, "2"),
		      gct_node_arg_END),
	      newtree(makeroot(GCT_TRUTH_NOT, "!"),
		      copy(test_temp),
		      gct_node_arg_END),
	      gct_node_arg_END);

    action3 =
      comma(newtree(makeroot(GCT_FUNCALL, (char *) 0),
		    makeroot(GCT_IDENTIFIER, G_SET),
		    makeroot(GCT_CONSTANT, at_least_once_name),
		    makeroot(GCT_CONSTANT, "0"),
		    gct_node_arg_END),
	    newtree(makeroot(GCT_FUNCALL, (char *) 0),
		    makeroot(GCT_IDENTIFIER, G_INC),
		    makeroot(GCT_CONSTANT, exactly_once_name),
		    gct_node_arg_END),
	    gct_node_arg_END);

    test4 =
      newtree(makeroot(GCT_ANDAND, "&&"),
	      newtree(makeroot(GCT_EQUALEQUAL, "=="),
		      copy(counter),
		      makeroot(GCT_CONSTANT, "2"),
		      gct_node_arg_END),
	      copy(test_temp),
	      gct_node_arg_END);
    action4 =
      comma(newtree(makeroot(GCT_FUNCALL, (char *) 0),
		    makeroot(GCT_IDENTIFIER, G_SET),
		    makeroot(GCT_CONSTANT, at_least_once_name),
		    makeroot(GCT_CONSTANT, "0"),
		    gct_node_arg_END),
	    newtree(makeroot(GCT_FUNCALL, (char *) 0),
		    makeroot(GCT_IDENTIFIER, G_INC),
		    makeroot(GCT_CONSTANT, at_least_twice_name),
		    gct_node_arg_END),
	    gct_node_arg_END);


    
    newlooptest =
      comma(looptest,
	    newtree(makeroot(GCT_POSTINCREMENT, "++"),
		     copy(counter),
		     gct_node_arg_END),
	    newtree(makeroot(GCT_QUEST, "?"),
		     test1,
		     action1,
		     newtree(makeroot(GCT_QUEST, "?"),
			      test2,
			      action2,
			      newtree(makeroot(GCT_QUEST, "?"),
				       test3,
				       action3,
				       newtree(makeroot(GCT_QUEST, "?"),
						test4,
						action4,
						makeroot(GCT_CONSTANT, "0"),
						gct_node_arg_END),
				       gct_node_arg_END),
			      gct_node_arg_END),
		     gct_node_arg_END),
	    copy(test_temp),
	    gct_node_arg_END);
#endif
  }
  replace(self, newlooptest, placeholder);
  /*
   * Now surround this code with a compound statement and add supporting
   * code.  
   */
  {
    gct_node pre_loop_action =
      newtree(makeroot(GCT_IF, "if"),
	      newtree(makeroot(GCT_FUNCALL, (char *) 0),
		      makeroot(GCT_IDENTIFIER, G_GET),
		      makeroot(GCT_CONSTANT, at_least_once_name),
		      gct_node_arg_END),
	      newtree(makeroot(GCT_SIMPLE_STMT, (char *) 0),
		      comma(newtree(makeroot(GCT_FUNCALL, (char *) 0),
				    makeroot(GCT_IDENTIFIER, G_SET),
				    makeroot(GCT_CONSTANT, at_least_once_name),
				    makeroot(GCT_CONSTANT, "0"),
				    gct_node_arg_END),
			    newtree(makeroot(GCT_FUNCALL, (char *) 0),
				    makeroot(GCT_IDENTIFIER, G_INC),
				    makeroot(GCT_CONSTANT, exactly_once_name),
				    gct_node_arg_END),
			    gct_node_arg_END),
		      gct_node_arg_END),
	      gct_node_arg_END);

    gct_node post_loop_action = copytree(pre_loop_action);
    gct_node newcompound;

    remember_place(parent, self, placeholder);
    newcompound =
      compound(newtree(makeroot(GCT_SIMPLE_STMT, (char *) 0),
		       newtree(makeroot(GCT_SIMPLE_ASSIGN, "="),
			       copy(counter),
			       makeroot(GCT_CONSTANT, "0"),
			       gct_node_arg_END),
		       gct_node_arg_END),
	       pre_loop_action,
	       self,
	       post_loop_action,
	       newtree(makeroot(GCT_SIMPLE_STMT, (char *) 0),
		       newtree(makeroot(GCT_SIMPLE_ASSIGN, "="),
			       copy(counter),
			       makeroot(GCT_CONSTANT, "0"),
			       gct_node_arg_END),
		       gct_node_arg_END),
	       gct_node_arg_END);
      
    replace(parent, newcompound, placeholder);
  }
  
  free(counter);		   
}






/*
 * Switches are handles as follows:
 *
 * 1.  Each switch has a "needed" variable.  When set, this indicates that
 * no case for the switch has yet been executed.  (A separate var for
 * each switch makes the code more reentrant.)
 * 2.  Each case checks the "needed" variable to determine whether to
 * mark that branch as taken.
 * 3.  The same holds for defaults.
 * 4.  A default is added to the closing brace of the switch statement
 * (if there is one -- switches need not use compound statements, though
 * I doubt any real program doesn't) if the programmer didn't add one.
 *
 * Since cases can appear anywhere, a global data structure is needed to
 * keep track of what switch we're currently in, whether a default has
 * been seen for that switch, and what the switch's need variable is.
 */

struct gct_switch
{
  gct_node needed_var;
  int default_seen;
};


/*
 * These three variables control instrumentation of switches.  The
 * SWITCH_LEVEL is the switch currently being instrumented; the first
 * switch is at level 1.  SWITCHES holds the information for that switch
 * and all the switches that contain it.  SWITCH_ARRAY_SIZE is the amount
 * of room we have for switches -- the array grows as needed.
 */
static int Switch_level = 0;
static int Switch_array_size = 2;
static struct gct_switch *Switches = (struct gct_switch *)0;

void
push_switch()
{
  Switch_level++;

  if ((struct gct_switch *) 0 == Switches)
    Switches =
      (struct gct_switch *) xmalloc(Switch_array_size * sizeof(struct gct_switch));
  else if (Switch_level == Switch_array_size)
    {
      Switch_array_size *= 2;
      Switches =
	(struct gct_switch *) xrealloc(Switches, Switch_array_size * sizeof(struct gct_switch));
    }
  Switches[Switch_level].needed_var
    = temporary_id(Int_root, OUTERMOST, FORCE, NULL, "=0", WANT_BASE_TYPE);
  Switches[Switch_level].default_seen = 0;
}


/*
 * Note that the default_seen variable is not necessarily 0.  Consider
 * this legal C switch:
 *
 * 	switch(c)
 * 	   printf("Hello, world\n");
 */
void
pop_switch()
{
  assert(Switches[Switch_level].needed_var);
  free(Switches[Switch_level].needed_var);
  Switches[Switch_level].needed_var = GCT_NULL_NODE;
  Switch_level--;
}


/* Predicate:  has default been seen yet? */
int
switch_default_seen()
{
  return Switches[Switch_level].default_seen;
}

/* Use this to record that a default has been seen. */
void
now_switch_has_default()
{
  Switches[Switch_level].default_seen = 1;
}

/* Return the setter expression for the "needed" variable. */
gct_node
switch_needed_init()
{
  assert(Switches[Switch_level].needed_var);

  return newtree(makeroot(GCT_SIMPLE_ASSIGN, "="),
		 copy(Switches[Switch_level].needed_var),
		 makeroot(GCT_CONSTANT, "1"),
		 gct_node_arg_END);
}

/*
 * Return code to instrument a case or default.
 *
 * Always:  Code to reset the "needed" variable, indicating that a case
 * has been taken.
 * If test_needed set:  Code to set the condition count for this case.
 */
gct_node
switch_case_test(index, test_needed)
     int index;
     int test_needed;
{
  assert(Switches[Switch_level].needed_var);
  
  return newtree(makeroot(GCT_SIMPLE_STMT, (char *)0),
		 comma((test_needed? 
		          make_probe(index, copy(Switches[Switch_level].needed_var)):
		          GCT_NULL_NODE),
		       newtree(makeroot(GCT_SIMPLE_ASSIGN, "="),
			       copy(Switches[Switch_level].needed_var),
			       makeroot(GCT_CONSTANT, "0"),
			       gct_node_arg_END),
		       gct_node_arg_END),
		 gct_node_arg_END);
}

