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

#include <stdio.h>
#ifdef MALLOC_TESTING
#	include <sys/stdtypes.h>
#	include <malloc.h>	/* Use Conner Cahill's dbmalloc. */
#endif

/* ===%%SF%% structures (Start)  === */
/*
 * A gct_annotation structure is attached to a gct_node.  It represents
 * an extra-syntactic annotation (like a #pragma) that must be emitted
 * after the node.  Annotations are chained in reverse order.
 *
 * There are two types of annotations:
 * - line notes that come from the original source, such as #line and
 * #pragma directives.  These begin and end with newlines, so that they
 * appear on their own line in the instrumented output.
 * - inline notes, such as declarations added by GCT.  They neither begin
 * nor end with a newline, so that they don't change line numbering.
 */

struct gct_annotation_structure
{
  struct gct_annotation_structure *next;
  char *text;
};

typedef struct gct_annotation_structure *gct_annotation;
#define GCT_NULL_ANNOTATION	((gct_annotation) 0)

/*
 * The tree codes are defined using gct-tree.def, in the manner of
 * tree.def and tree.h.
 *
 * Each of the non-atomic nodes has accessors and setters for its
 * components.  See below.
 */

enum gct_node_type
{
#define DEF_GCT_WEAK_TREE(token, string, ifunc, expfunc, lvfunc) token,
#include "gct-tree.def"
  LAST_AND_UNUSED_GCT_TREE_CODE
#undef DEF_GCT_WEAK_TREE
};

#define NUM_GCT_TREE_CODES	((int)LAST_AND_UNUSED_GCT_TREE_CODE)


#define TYPE_IN_RANGE(type)\
	((type)>=0 && (type)<NUM_GCT_TREE_CODES)


/* Declaration-type tree nodes containing arrays may be processed in one of
   two ways:  as arrays, or as pointers.  */

#define ARRAYS_AS_ARRAYS		-5 /* Print in foo[] format */
#define ARRAYS_AS_POINTERS		-7 /* Print in *foo format. */


/*
 * The gct_node is the main structure used for building GCT parse trees. 
 * The fields are as follows:
 *
 * NEXT, PREV:	Nodes are chained in doubly-linked lists.
 * 		The lists have headers, which always point to the first elt.
 * CHILDREN:	Pointer to a chained list of children.
 * TYPE:	As above.
 * TEXT:	What's to be printed to represent this node.
 * TEXTLEN:	Length of that text (since strings may contain nulls)
 * 		The length does not include the trailing null.
 * NOTE:	Misc non-syntactic text to print after this node.
 * FILENAME:	The file this node comes from.
 * LINENO:	When printing a tree, this tells when to spit out a newline.
 * FIRST_CHAR:	The location of the first character of the
 * 		text of this node.  It is used when deciding whether
 * 		to instrument a node.
 * IS_VOLATILE:	True if tree cannot freely be evaluated multiple times.
 * GCC_TYPE:	The GCC type corresponding to this node.
 */

/*
 * NOTE: The NOTE field is a quick and dirty place to store #pragmas and #line 
 * notes.  They would be better stored in an auxiliary data structure,
 * since few nodes will actually use the space.
 */

struct gct_node_structure
{
  struct gct_node_structure *next, *prev, *children;
  enum gct_node_type type;
  char *text;
  int textlen;
  gct_annotation note;
  char *filename;
  int lineno, first_char;
  int is_volatile;
  tree gcc_type;
};
typedef struct gct_node_structure *gct_node;
#define GCT_NULL_NODE		((gct_node)0)



/* This is the node list we build while parsing. */
extern gct_node Gct_all_nodes;

/* This is where it gets printed to. */
extern FILE* Gct_textout;

extern int Gct_initialized;
extern int Gct_current_line;

/* ===%%SF%% structures (End)  === */

/* ===%%SF%% node-accessors (Start)  === */

/* Accessor and setter functions for the different types of nodes. */
#define GCT_FIRST(header)		(header)
#define GCT_LAST(header)		((header)->prev)

/* Generic fiddling with children */
#define GCT_CHILD_COUNT(node)		(gct_length(node->children))

#define GCT_LEFT_CHILD(node)  		((node)->children)
#define GCT_RIGHT_CHILD(node)		((node)->children->prev)
#define GCT_NTH_CHILD(node, n)		(gct_nth_node((node)->children, (n)))

/* Distinguish strings from other constants. */
#define GCT_STRING_CONSTANT_P(node)	('"' == (node)->text[0])

/* Trees built using the ADD function are build left to right (according
   to the grammar).  Modifications to the existing tree are done with
   gct_replace_node(GETTER(root)). */

#define GCT_ADD(root, thing)	(gct_add_last(&((root)->children), (thing)))

#define GCT_OP_LEFT(node)			GCT_LEFT_CHILD(node)
#define GCT_OP_RIGHT(node)			GCT_RIGHT_CHILD(node)
#define GCT_OP_ONLY(node)			GCT_LEFT_CHILD(node)

#define GCT_CAST_TYPE(node)			GCT_LEFT_CHILD(node)
#define GCT_CAST_EXPR(node)			GCT_RIGHT_CHILD(node)

#define GCT_COMMA_OPERANDS(node)		(node)->children

#define GCT_FUNCALL_FUNCTION(node)		(node)->children
#define GCT_FUNCALL_ARGS(node)			(node)->children->next
#define GCT_FUNCALL_IS_ARG(root, node)		((node)->children != (root))
#define GCT_FUNCALL_HAS_ARGS(node)\
  (GCT_FUNCALL_ARGS(node) != GCT_FUNCALL_FUNCTION(node))
#define GCT_FUNCALL_FIRST_ARG(node)		GCT_FUNCALL_ARGS(node)
#define GCT_FUNCALL_LAST_ARG(node)		((node)->children->prev)

/*
 * NOTE:  For some of these, we'll want accessors for the node itself.
 * For example, there ought to be a GCT_LABEL_LABEL to hide the fact that
 * the label node is the root of the label statement.  Probably.
 */

#define GCT_IF_TEST(node)		((node)->children)
#define GCT_IF_THEN(node)		((node)->children->next)
#define GCT_IF_ELSE(node)		((node)->children->prev)
#define GCT_IF_HAS_ELSE(node)		((node)->children->prev != \
					 (node)->children->next)

#define GCT_WHILE_TEST(node)		((node)->children)
#define GCT_WHILE_BODY(node)		((node)->children->next)

#define GCT_DO_BODY(node)		((node)->children)
#define GCT_DO_TEST(node)		((node)->children->next)

#define GCT_FOR_INIT(node)		((node)->children)
#define GCT_FOR_TEST(node)		((node)->children->next)
#define GCT_FOR_INCR(node)		((node)->children->next->next)
#define GCT_FOR_BODY(node)		((node)->children->prev)


#define GCT_QUEST_TEST(node)		((node)->children)
#define GCT_QUEST_TRUE(node)		((node)->children->next)
#define GCT_QUEST_FALSE(node)		((node)->children->prev)

#define GCT_REF_PRIMARY(node)		GCT_LEFT_CHILD(node)
#define GCT_REF_SECONDARY(node)		GCT_RIGHT_CHILD(node)

#define GCT_TYPECRUD_CRUD(node)		((node)->children)

#define GCT_SIMPLE_STMT_BODY(node)	((node)->children)

#define GCT_EMPTY_COMPOUND_STATEMENT(compstmt)\
  ((compstmt)->children->next == (compstmt)->children->prev)

#define GCT_SWITCH_TEST(node)		((node)->children)
#define GCT_SWITCH_BODY(node)		((node)->children->next)

#define GCT_CASE_EXPR(node)		((node)->children)
#define GCT_CASE_STMT(node)		((node)->children->next)

#define GCT_DEFAULT_STMT(node)		((node)->children)

#define GCT_RETURN_EXPR(node)		((node)->children)

#define GCT_ASM_CRUD(node)		((node)->children)

#define GCT_GOTO_LABEL(node)		((node)->children)

#define GCT_LABEL_STMT(node)		((node)->children)

#define GCT_ADDR_ARG(node)		((node)->children)
#define GCT_DEREFERENCE_ARG(node)	((node)->children)

#define GCT_ARRAY_ARRAY(node)		((node)->children)
#define GCT_ARRAY_INDEX(node)		((node)->children->next)

#define GCT_DOTREF_VAR(node)		GCT_LEFT_CHILD(node)
#define GCT_DOTREF_FIELD(node)		GCT_RIGHT_CHILD(node)

#define GCT_ARROWREF_VAR(node)		GCT_LEFT_CHILD(node)
#define GCT_ARROWREF_FIELD(node)		GCT_RIGHT_CHILD(node)

/* AND SO ON */

/* ===%%SF%% node-accessors (End)  === */


/* ===%%SF%% functions (Start)  === */

/* Descriptions of these functions are given in the C file. */

extern gct_annotation gct_alloc_annotation();
extern void gct_recursive_free_annotation();
extern gct_annotation gct_build_pragma();
extern gct_annotation gct_build_line_note();
extern void gct_make_end_note();
extern void gct_make_current_note();
extern gct_annotation gct_misc_annotation();

extern gct_node gct_alloc_node();
extern gct_node gct_placeholder();
extern gct_node gct_located_placeholder();
extern void gct_recursive_free_node();
extern gct_node gct_node_from_string();
extern gct_node gct_tempnode();

extern gct_node gct_nth_node();
extern void gct_unlink();
extern void gct_remove_node();

extern void gct_add_first();
extern void gct_add_last();
extern void gct_add_before();
extern void gct_add_after();
extern void gct_replace_node();

extern void gct_write_list();

extern void gct_free_sugar();
extern gct_node gct_find_earlier_match();
extern gct_node gct_find_later_match();
extern gct_node gct_find_start_of_declaration();
extern gct_node gct_preceding_text();
extern gct_node gct_either_preceding_text();

extern char *slashified_string();		/* Defined in gct-print.c */



/*    ===%%SF%% functions/nodes (End)  === */


/* ===%%SF%% functions (End)  === */

