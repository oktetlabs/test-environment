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

/* GCT code that handles declarations. */
/* 
$Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-decl.c,v 1.1 2003/10/28 09:48:45 arybchik Exp $
$Log: gct-decl.c,v $
Revision 1.1  2003/10/28 09:48:45  arybchik
Initial revision

Revision 1.1  2003/08/18 16:38:03  pabolokh
new tree is being added and committed

 * Revision 1.7  1992/07/28  14:28:23  marick
 * Miscellaneous tidying.
 *
 * Revision 1.6  1992/03/23  03:10:36  marick
 * Missing formal parameter.
 *
 * Revision 1.5  1992/02/13  17:44:11  marick
 * Copylefted.
 *
 * Revision 1.4  91/05/05  14:11:10  marick
 * General tidying up.
 * 
 * Revision 1.3  91/02/12  15:26:56  marick
 * GCT no longer prohibits unnamed structs and unions.
 * 
 * Revision 1.2  90/12/27  16:47:57  marick
 * 1.  If namestring() can't figure out the type, abort.
 * 2.  Unnamed enums are silently converted to ints.  A warning is given if
 * temporaries have to be made for unnamed structs or unions.
 * 
*/

#include "c-decl.h"
#include "gct-assert.h"


/* The last declaration seen by the parser. */
char *Gct_last_decl;


/* Used in printing -- converts null string to "(none)". */
#define STR(string) ((string)?(string):"(none)")

/*
 * This returns the name of a type, which is a single token.  
 * The storage returned is already in use
 * elsewhere:  it should not be freed.
 */
char *
namestring(type)
     tree type;
{
  if (NULL_TREE == TYPE_NAME(type))
    {
      /* For example, an unnamed structure. */
      fatal("GCT internal error:  Unnamed type slipped past parser.");
      return "!unnamed!";
    }
  else if (TREE_CODE (TYPE_NAME (type)) == IDENTIFIER_NODE)
    {
      /* fprintf(stderr, "CASE 1 for %s\n", IDENTIFIER_POINTER(TYPE_NAME(type))); */
      return IDENTIFIER_POINTER(TYPE_NAME(type));
    }
  else if (TREE_CODE(DECL_NAME(TYPE_NAME(type))) == IDENTIFIER_NODE)
    {
      /* fprintf(stderr, "CASE 2 for %s\n", IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(type)))); */
      return IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(type)));
    }
  else
    {
      warning("couldn't figure out type in namestring().");
      abort();
    }
}

  

/* Caller must free returned storage and also passed-in text.
   Passed-in-text is never what is returned. */

char *
make_decl(type, text_so_far, how_arrays)
     tree type;
     char * text_so_far;
     int how_arrays;
{
  char *string = (char *) xmalloc(1000);	/* Do this right someday. */
  char *substring;		/* What recursive invocations return. */
  char *typename;		/* The name of the type. */

  string[0] = '\0';

  switch(TREE_CODE(type))
    {
    case RECORD_TYPE:
      typename = namestring(type);
      sprintf(string, "struct %s %s", typename, text_so_far);
      break;
      
    case ENUMERAL_TYPE:
      typename = namestring(type);
      if ('!' == typename[0])
	{
	  /* Silently pretend that the enum is an int. */
	  sprintf(string, "int %s", text_so_far);
	}
      else
	{
	  sprintf(string, "enum %s %s", typename, text_so_far);
	}
      break;
      
    case UNION_TYPE:
      typename = namestring(type);
      sprintf(string, "union %s %s", typename, text_so_far);
      break;
      
    case ARRAY_TYPE:
      if (ARRAYS_AS_ARRAYS == how_arrays)
	sprintf(string, "(%s[])", text_so_far);
      else if (ARRAYS_AS_POINTERS == how_arrays)
	sprintf(string, "(*%s)", text_so_far);
      else
	fatal("make_decl called with bad third argument %d.", how_arrays);
      substring = make_decl(TREE_TYPE(type), string, how_arrays);
      strcpy(string, substring);
      free(substring);
      break;

    case POINTER_TYPE:
      sprintf(string, "(*%s)", text_so_far);
      substring = make_decl(TREE_TYPE(type), string, how_arrays);
      strcpy(string, substring);
      free(substring);
      break;
    case FUNCTION_TYPE:
      sprintf(string, "(%s())", text_so_far);
      substring = make_decl(TREE_TYPE(type), string, how_arrays);
      strcpy(string, substring);
      free(substring);
      break;

    default:
      sprintf(string, "%s %s", namestring(type), text_so_far);
      break;
    }
  return string;
}


/* This prints the variable declarations in a particular binding_level. */
print_contour(stream, contour)
     FILE *stream;
     struct binding_level *contour;
{
  tree rover;
  assert(contour);

  for (rover = contour->names; rover; rover=TREE_CHAIN(rover))
    if (   TREE_CODE(rover) == VAR_DECL
	|| TREE_CODE(rover) == PARM_DECL)
      {
	char *text = make_decl(TREE_TYPE(rover), DECL_PRINT_NAME(rover),
			       ARRAYS_AS_ARRAYS);
	fprintf(stream, "%s; /* flavor %s */\n", text,
		STR(DECL_GCT_FLAVOR(rover)));
	free(text);
      }
}

/*
 * This prints the current binding level and all enclosing binding
 * levels, most recent first.
 */
print_lexical_environment(stream)
     FILE *stream;
{
  print_lexical_environment_helper(stream, current_binding_level);
}

print_lexical_environment_helper(stream, stack)
     FILE *stream;
     struct binding_level *stack;
{
  if (stack)
    {
      fprintf(stream, "LEVEL:\n");
      print_contour(stream, stack);
      print_lexical_environment_helper(stream, stack->level_chain);
    }
}


/*
 * This prints all declarations in the global environment and in the
 * current environment and in all enclosed environments.  It is usually
 * called for an entire function body.
 */
print_declarations(stream)
     FILE *stream;
{
  fprintf(stream, "GLOBAL ENVIRONMENT:\n");
  print_contour(stream, global_binding_level);
  fprintf(stream, "CURRENT ENVIRONMENT:\n");
  print_contour(stream, current_binding_level);

  /* Binding_levels are changed to let statements as blocks are exited. */
  print_let_chain(stream, current_binding_level->blocks);
}


/*
 * This prints the variables in a chain of let statements.  It recurses
 * to enclosed let statements. 
 */
print_let_chain(stream, let_stmt)
     FILE *stream;
     tree let_stmt;
{
  static int level = 0;
  int block_count = 0;

  level++;
  for(block_count++; let_stmt; let_stmt=TREE_CHAIN(let_stmt), block_count++)
    {
      tree one_var;

      fprintf(stream, "LEVEL %d, BLOCK %d\n", level, block_count);
      for(one_var=BLOCK_VARS(let_stmt); one_var; one_var=TREE_CHAIN(one_var))
	{
	  if (   TREE_CODE(one_var) == VAR_DECL
	      || TREE_CODE(one_var) == PARM_DECL)
	    {
	      char *text = make_decl(TREE_TYPE(one_var), DECL_PRINT_NAME(one_var),
				     ARRAYS_AS_ARRAYS);
	      fprintf(stream, "%s; /* flavor %s */\n", text,
		      STR(DECL_GCT_FLAVOR(one_var)));
	      free(text);
	    }
	}
      print_let_chain(stream, BLOCK_SUBBLOCKS(let_stmt));
    }
  level--;
}


print_structure(stream, type)
     FILE *stream;
     tree type;
{
  tree fieldlist;
  
  fprintf(stream, "%s %s\n{\n",
	  RECORD_TYPE == TREE_CODE(type) ? "struct" : "union",
	  namestring(type));
  for(fieldlist = TYPE_FIELDS(type); fieldlist; fieldlist=TREE_CHAIN(fieldlist))
    {
      char *text;
      
      assert(TREE_CODE(fieldlist) == FIELD_DECL);
      text = make_decl(TREE_TYPE(fieldlist), DECL_PRINT_NAME(fieldlist),
		       ARRAYS_AS_ARRAYS);
      fprintf(stream, "%s; /* flavor %s */\n", text,
	      STR(DECL_GCT_FLAVOR(fieldlist)));
      free(text);
    }
  fprintf(stream, "};\n");
}
