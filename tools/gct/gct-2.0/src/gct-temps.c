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
$Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-temps.c,v 1.1 2003/10/28 09:48:45 arybchik Exp $
$Log: gct-temps.c,v $
Revision 1.1  2003/10/28 09:48:45  arybchik
Initial revision

Revision 1.1  2003/08/18 16:38:03  pabolokh
new tree is being added and committed

 * Revision 1.12  1992/07/28  14:29:19  marick
 * Miscellaneous tidying.
 *
 * Revision 1.11  1992/05/24  17:11:22  marick
 * Replaced_statement was a kludge.  It's no longer necessary.  While I
 * was at it, I rewrote the comments and changed temporary_id so that
 * calls for an OUTERMOST id while instrumenting an initializer in the
 * function body's context puts the temporary declaration in the right
 * place (even though such calls never happen).
 *
 * Revision 1.10  1992/05/21  16:29:05  marick
 * Remove short_stack option.
 *
 * Revision 1.9  1992/02/13  17:49:33  marick
 * Copylefted.
 *
 * Revision 1.8  91/12/13  09:57:08  marick
 * Handle problems generating temporaries for 2D arrays.  See ARRAY.2D
 * for details.
 * 
 * Revision 1.7  91/10/21  10:03:47  marick
 * Added short_stack option (see gct-utrans.c for discussion).
 * 
 * Revision 1.6  91/08/07  14:49:39  marick
 * Cretinous SysV filename limits (need to reduce further for RCS).
 * 
 * Revision 1.5  91/05/05  14:10:12  marick
 * General tidying up.
 * 
 * Revision 1.4  90/12/28  16:40:19  marick
 * Added a helpful comment.
 * 
 * Revision 1.3  90/12/27  16:50:34  marick
 * Bugfix:  replaced_statement updated only the current compound, not the
 * outermost compound.
 * 
*/

#include <stdio.h>
#include "c-decl.h"
#include "gct-tutil.h"
#include "gct-contro.h"
#include "gct-assert.h"
#include <varargs.h>

#ifdef TEST
#define xmalloc malloc
#endif

/*
 * SUMMARY: 
 *
 * When preparing to instrument a function:
 * 		Call gct_temp_init.
 * When finished instrumenting a function:
 * 		Call gct_temp_finish
 * When entering a compound statement:
 * 		Call gct_temp_compound_init
 * When leaving a compound statement:
 * 		Call gct_temp_compound_finish
 * When processing a declaration:
 * 		Call gct_temp_decl_init.
 * When finished with a declaration:
 * 		Call gct_temp_decl_finish.
 *
 * To make a temporary:
 * 		Call temporary_id
 */


/*
 * Temporaries may be declared in one of three places:
 *
 * - In the innermost context (compound statement opening brace) enclosing
 * the code being instrumented, at the end of any declarations from the
 * original program.
 *
 * - In the outermost context (the compound that comprises the body of
 * the function), at the end of any declarations from the original program.
 *
 * - In the middle of a declaration list from the original program.
 *
 * The caller of temporary_id chooses among these. See temporary_id for
 * how and why the choice is made.
 *
 * Temporaries are annotations, attached to curly-brace nodes or
 * declaration nodes.  The following three variables point to the
 * appropriate node:
 *
 * - Function_compound_where is used to put declarations at the end of the
 * outermost declaration list.
 *
 * - Innermost_compound_where is a vector of pointers that changes as
 * compound statements are entered and left.
 * Innermost_compound_where[Current_compound] is the place to put
 * temporaries at any given moment.
 *
 * - Decl_where is used to put declarations within a declaration list.
 * For example, if instrumenting the initializer for B,
 *
 * 	int a;
 * 	int b = 43 + arg;
 *
 * declarations are put after A.
 */

static gct_node Function_compound_where = GCT_NULL_NODE;

/* A stack of contexts, and its top-of-stack index. */
static gct_node *Innermost_compound_where = (gct_node *) 0;
int Current_compound;

static gct_node Decl_where = GCT_NULL_NODE;



			   /* Functions */
/*
 * Call this routine when entering the outermost compound statement of a
 * function.  It initializes the places where temporary variables are to
 * be declared.
 */
void
gct_temp_init(compound)
     gct_node compound;
{
  assert(!Decl_where);

  if (Innermost_compound_where)
      free(Innermost_compound_where);
  Innermost_compound_where = (gct_node *) xcalloc(say_nesting_depth(),
						 sizeof(gct_node));
  Current_compound = -1;
  
  gct_temp_compound_init(compound);
  Function_compound_where = Innermost_compound_where[0];
  
}

		      /* Compound Statements */
/*
 * Upon entering a compound statement, call this function, which will
 * ensure that tempories declared while processing the sub-statements
 * are placed at the end of the declaration list.  Putting them at the
 * end is useful if there are embedded declarations like
 *
 * {
 *   enum foo { i , j} myvar;
 *
 * This routine knows the structure of compound statements. 
 *
 */
void
gct_temp_compound_init(compound)
     gct_node compound;
{
  gct_node possible = compound->children->next;

  assert(!Decl_where);
  assert(Innermost_compound_where);
  
  for(;GCT_DECLARATION == possible->type; possible= possible->next)
    {
    }

  /* Now past end of declaration list. */
  Current_compound++;
  Innermost_compound_where[Current_compound] = possible->prev;
}

/* This function "pops" the module's notion of where to put new
   declarations. */
gct_temp_compound_finish()
{
  assert(Current_compound > 0);	/* Never leave outermost compound statement. */
  Current_compound--;
}


			  /* Declarations */

/*
 * When processing a declaration, issue this call, which causes all
 * temporaries created during the instrumentation of the declaration's
 * initializer (if any) to be placed before the declaration.
 */
void
gct_temp_decl_init(decl)
     gct_node decl;
{
  assert(!Decl_where);
  
  Decl_where = decl->prev;
}

void
gct_temp_decl_finish(decl)
     gct_node decl;
{
  assert(decl == Decl_where->next);
  Decl_where = GCT_NULL_NODE;
}

		      /* Making Temporaries. */


/*
 * temporary_id(root, where, ref, prefix, suffix, pointerness)
 *
 * This routine creates a unique temporary identifier which is assignment
 * compatible with ROOT.
 *
 * WHERE is either CLOSEST or OUTERMOST to tell whether the declaration
 * should be placed as close as possible to the current instrumentation
 * point or at the outermost enclosing compound statement.  CLOSEST is
 * preferable, since the type you're making a temporary for might have
 * been declared in the closest scope:
 *
 * 	{
 * 	  enum foo { val1, val2} myvar = val1;
 * 	  <code that requires a temporary of type foo>
 * 	}
 *
 * OUTERMOST is sometimes needed, consider:
 * 	switch(c)
 *     	{			<= added here under CLOSEST
 * 	    default:
 * 	      for(;c>0;c--)	<= needed here
 * 		{
 * 		  out(c);
 * 		}
 * 	}
 *
 * If an initialized non-static temporary is to be declared at the point
 * marked with "needed here", it would be added at the point marked
 * "added here" -- but the initializing code would never be executed.
 * So OUTERMOST is needed to put it where it is executed.
 *
 * Note:  if the temporary is needed for instrumentation of a declaration
 * initializer, it will be put before the initializer in the declaration
 * list.  
 *
 *
 *
 * if REF is REFERENCE_OK, the same node MAY be returned if repeated
 * evaluations would not change its value.  We return the same node iff
 * 1.  It is a non-volatile identifier.
 * 2.  It is a constant.
 *
 * The PREFIX is usually used for passing in "static".  The SUFFIX is
 * usually used for passing in an initializer.  POINTERNESS is used to
 * create a temporary of type pointer-to-root-type instead of just
 * root-type.  (BUG:  In this case, the gcc-type is set to 0, which is
 * incorrect but currently harmless.)
 *
 * The name of the resulting temporary is unique.
 * The caller must free the temporary.  It is conventional to always make
 * copies of the temporary, never to splice it into a tree.
 */

gct_node
temporary_id(root, where, ref, prefix, suffix, pointerness)
     gct_node root;
     int where;
     int ref;
     char *prefix, *suffix;
     int pointerness;
{
  static long last_index;	/* Last value of Gct_next_index -- keeps
				   strings shorter. */
  static long counter;
  static char name[20];		/* The name of the variable. */
  static char declname[21];	/* Name to use in declaration. */
  char *text;			/* Part of the real declaration. */
  char *decl;			/* The actual declaration. */
  gct_node new_id;		/* What we return. */
  char *make_decl();
  gct_node which_location;	/* Of three locations, where to put declaration */

  assert(root);
  assert(WHERE_IN_RANGE(where));
  assert(REF_IN_RANGE(ref));
  assert(POINTERNESS_IN_RANGE(pointerness));
  
  assert(Function_compound_where);	/* These must always be defined. */
  assert(Innermost_compound_where);
  assert(Innermost_compound_where[Current_compound]);

  if (!prefix) prefix = "";
  if (!suffix) suffix = "";

  if (   REFERENCE_OK == ref
      && NO_TEMPORARY_NEEDED(root))
    {
      return root;
    }

  /*
   * Arrays as values generate incorrect temporaries.  Strategy:  make
   * the temporary REALLY incorrect -- a float.  If the temporary isn't
   * used (most of the time) this is just a warning.  If it is, the
   * instrumented file will not compile.  See also ARRAY.2D.
   *
   * Note that this code can never be true in standard instrumentation,
   * which doesn't make temporaries for arrays.
   */
  if (root->gcc_type &&  ARRAY_TYPE==TREE_CODE(root->gcc_type)
      && ARRAY_TYPE==TREE_CODE(TREE_TYPE(root->gcc_type)))
  {
    struct gct_node_structure float_root_struct;
    float_root_struct.gcc_type = float_type_node;
    
    warning("GCT may mishandle 2D arrays like the one on line %d.", root->lineno);
    warning("If this file later fails to compile, you must");
    warning("turn off instrumentation of this routine.");
    return temporary_id(&float_root_struct, where, ref, prefix, suffix, pointerness);
  }

  /* We have to make a variable. */
  if (Gct_next_index != last_index)
    {
      last_index = Gct_next_index;
      counter = 0;
    }
  
  sprintf(name, "_G%d_%d",
	  Gct_next_index, counter);
  if (   WANT_POINTER_TYPE == pointerness
      && FUNCTION_TYPE == TREE_CODE(root->gcc_type))
    {
      sprintf(declname, "(**%s)", name);
    }
  else if (   WANT_POINTER_TYPE == pointerness
      || FUNCTION_TYPE == TREE_CODE(root->gcc_type))
    {
      sprintf(declname, "(*%s)", name);
    }
  else
    {
      strcpy(declname, name);
    }
  counter++;

  text = make_decl(root->gcc_type, declname, ARRAYS_AS_POINTERS);
  decl = (char *) xmalloc(strlen(text)+4+strlen(prefix)+strlen(suffix));
  sprintf(decl, "%s %s %s;", prefix, text, suffix);
  free(text);
  

  if (   Decl_where
      && (   CLOSEST == where
	  || Function_compound_where == Innermost_compound_where[Current_compound]))
    {
      /* Working on declaration list where temporary will go. */
      which_location = Decl_where;
    }
  else if (OUTERMOST == where)
    {
      which_location = Function_compound_where;
    }
  else
    {
      which_location = Innermost_compound_where[Current_compound];
    }
  gct_make_current_note(gct_misc_annotation(decl), which_location);

  new_id = makeroot(GCT_IDENTIFIER, name);
  new_id->gcc_type = (WANT_POINTER_TYPE == pointerness? NULL_TREE : root->gcc_type);
  return new_id;
}
