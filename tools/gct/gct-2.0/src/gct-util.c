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
#include "config.h"
#include "tree.h"
#include "gct-util.h"
#include "gct-assert.h"	/* Don't use gcc's; it uses gnulib routine. */
#include "gct-files.h"

/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-util.c,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */

#ifdef TEST

#define xmalloc malloc
#define xcalloc calloc
#define gct_fgetc fgetc
#define xrealloc realloc

#endif /* TEST */

#define PRAGMA_HEADER	"\n#pragma "	/* Start of a pragma annotation. */

gct_node Gct_all_nodes = GCT_NULL_NODE;
FILE *Gct_textout;


/* ===%%SF%% annotations (Start)  === */


/* This should be smarter than just using malloc; should use the GCC obstack
   routines.  */
gct_annotation
gct_alloc_annotation()
{
  return (gct_annotation) xcalloc(1, sizeof(struct gct_annotation_structure));
}


/* Note that null pointer is valid argument. */
void
gct_recursive_free_annotation(first_annotation)
  gct_annotation first_annotation;
{
  gct_annotation rover;

  while (first_annotation)	/* Well, tail recursion, anyway. */
    {
      rover = first_annotation->next;
      free(first_annotation);
      first_annotation = rover;
    }
}


/*
 * Build a pragma annotation from the current input.  The "#pragma" has
 * already been read. 
 */
gct_annotation 
gct_build_pragma(stream)
     FILE *stream;
{
  gct_annotation node = gct_alloc_annotation();
  char *current = (char *)0;	/* Next character goes here. */
  char *past_end = (char *)0;	/* So we know when to grow. */
  int len = 50;			/* Length of string */
  static int warned = 0;

  node->text = (char *) xmalloc(len + 1);
  node->next = GCT_NULL_ANNOTATION;
  strcpy(node->text, PRAGMA_HEADER);
  current = node->text + sizeof(PRAGMA_HEADER)-1;   /* Don't include null */
  past_end = node->text + len;

  do
  {
    *current = gct_fgetc(stream);
    current++;
    if (current == past_end)
    {
      len *= 2;
      node->text = (char *) xrealloc(node->text, len + 1);
      current = node->text + len / 2;
      past_end = node->text + len;
    }
  } while ('\n' != *(current-1) && EOF != *(current-1));
  *current = '\0';

  if (NULL != current_function_decl)
  {
    warning("#pragmas within functions are dangerous.");
  }
  
  return node;
}


/*
 * Test if the annotation is a pragma.  This is disgusting -- pragmas
 * should be a separate type.
 */
int
annotation_pragma_p(note)
	gct_annotation note;
{
  return (0 == strncmp(PRAGMA_HEADER, note->text, sizeof(PRAGMA_HEADER)));
}

	
/*
 * Build a line note, given LINE and FILE.
 * Format is #line N "file" as in K&R2.  The trailing enter/exit symbols
 * are suppressed because older versions of GCC choke on it.
 */
gct_annotation
gct_build_line_note(filename, line)
     char *filename;
     int line;
{
  gct_annotation node = gct_alloc_annotation();
  node->text = (char *) xmalloc(50 + strlen(filename));
  node->next = GCT_NULL_ANNOTATION;
  sprintf(node->text, "\n#line %d \"%s\"\n", line, filename);
  return node;
}

/*
 * Build an annotation containing random text.  Text is not copied; the
 * caller must retain no pointers to it.
 */
gct_annotation
gct_misc_annotation(text)
     char *text;
{
  gct_annotation node = gct_alloc_annotation();
  node->text = text;
  node->next = GCT_NULL_ANNOTATION;
  return node;
}

     
/*
 * LIST points to the first gct_node in a list.  An annotation is
 * pushed onto the last node in that list.  Note that the annotation list
 * is constructed in reverse time-order; the printing routines will later
 * print it in reverse.
 *
 * If the list is empty, the annotation is immediately written to the output.
 */
void 
gct_make_end_note(note, list)
     gct_annotation note;
     gct_node list;
{
  if (list)
    {
      list = list->prev;	/* Attach to last node in list. */
      note->next = list->note;
      list->note = note;
    }
#ifndef TEST	/* Avoid dragging in all sorts of code for test routines. */
  else
    {
      write_one_annotation(note);
      free(note);
    }
#endif /* NOT TEST */
}

/*
 * ATTACH_TO must point to a gct_node.  An annotation is pushed onto that
 * node.  Note that the annotation list is constructed in reverse
 * time-order; the printing routines will later print it in reverse.
 */
void 
gct_make_current_note(note, attach_to)
     gct_annotation note;
     gct_node attach_to;
{
  assert(attach_to);

  note->next = attach_to->note;
  attach_to->note = note;
}

/* ===%%SF%% annotations (End)  === */


/* ===%%SF%% nodes (Start)  === */



/*    ===%%SF%% nodes/creation-destruction (Start)  === */

/*
 * Allocate a node structure.  Right now, uses malloc, which is
 * incredibly inefficient.  Ought to maintain a freelist.
 */
gct_node
gct_alloc_node()
{
  return (gct_node) xcalloc(1, sizeof(struct gct_node_structure));
}

/*
 * Free all the nodes reachable from the root.  Works for nodes in doubly
 * linked lists, and also for unlinked nodes.  Frees all children and
 * texts.  It is a program error if any annotation is present, for that
 * means that the annotation was not written or not promoted when a node
 * is discarded.
 */

void
gct_recursive_free_node(root)
     gct_node root;
{
  if (! root)
    {
      return;
    }
  
  if (root->prev)	/* linked in list */
    {
      root->prev->next = GCT_NULL_NODE;
    }
  while(root)
    {
      gct_node next_root = root->next; /* Free might scribble over NEXT field. */
      gct_recursive_free_node(root->children);
      if (root->note)
	{
	  warning("On node with type %d, text %s, line %d:", root->type,
		  root->text ? root->text : "(none)", root->lineno);
	  warning("Line note or pragma not written or promoted.");
	  warning("First annotation says '%s'.\n", root->note->text);
	  gct_recursive_free_annotation(root->note);
	}
      if (root->text) free(root->text);
      free(root);
      root = next_root;
    }
}


/* Allocate a node that is only the root of a tree.  Setting the type is
   the caller's responsibility. */
gct_node
gct_placeholder()
{
  gct_node node = gct_alloc_node();
  return node;
}

/*
 * Allocate a node that is only the root of a tree.  The filename,
 * lineno, and first_char fields are filled in.  The filename should be
 * set on all nodes generated from the original C code, though it's
 * strictly required on only those that might generate a mapfile entry.
 * Ditto for the lineno.  The first_char is also set everywhere, though
 * it's only required at instrumentation points and boundaries of
 * expressions (where the node is tested to see if it's in a macro).
 *
 * Setting the type is the caller's responsibility.
 */
gct_node
gct_located_placeholder(located)
	gct_node located;	
{
  gct_node node = gct_alloc_node();
  node->filename = located->filename;
  node->lineno = located->lineno;
  node->first_char = located->first_char;
  return node;
}

/*
 * NOTES:
 * 1.  All nodes up until a #line directive use the same string as their
 * filename.   The space for this string is NOT reclaimed; code in
 * gct-mapfil.c depends on it remaining around.  Thus we waste one string
 * per included file.  Big deal.
 *
 * 2.  This code is careful to handle strings with embedded nulls.
 * However, use of strcpy and friends is pervasive in GCT, so copies of
 * the node created here are likely not to preserve this careful
 * handling.  By inspection, I believe this will only affect people using
 * operand coverage.  By test, it's confirmed that it is a problem (see
 * testdir/known-bugs/null-broke).
 *
 * The intersection of people using operand coverage and people having
 * programs with strings with embedded nulls is, I hope, ignorably small.
 */

gct_node
gct_node_from_string(string, number_nulls, filename, lineno, first_char)
     char *string;
     int number_nulls;	/* Number of nulls in string (not final one) */
     char *filename;
     int lineno, first_char;
{
  int length = 0;     /* Number of characters in string, not including null. */
  char *string_rover = string;
  gct_node node = gct_alloc_node();
  static char *current_filename = (char *)0;	/* Last filename used */


  node->textlen = 0;	/* Give sane values if string doesn't exist. */
  node->text = (char *)0;
  if (string)
    {
      do
	{
	  /* Count Nth segment, including null */
	  length += strlen(string_rover) + 1;
	  string_rover = string + length;
	} while (number_nulls-- > 0);
      length--;	/* Omit trailing null */

      node->textlen = length;
      node->text = (char *) xmalloc(length+1);
      for(;length >= 0; length--)     /* This copies trailing null as well. */
	{
	  node->text[length] = string[length];
	}
    }

  if (!filename)
    {
      node->filename = filename;	/* Node not from a file. */
    }
  else if (   !current_filename		
	   || 0 != strcmp(filename, current_filename))
    {
      /* Node from new file. */
      current_filename = permanent_string(filename);
      node->filename = current_filename;
    }
  else
    {
      /* Node from current file. */
      node->filename = current_filename;
    }
  
  node->lineno = lineno;
  node->first_char = first_char;
  node->type = GCT_OTHER;
  return node;
}


/* Create an unlocated temporary node with prefix prefix. */
gct_node
gct_tempnode(prefix)
     char *prefix;
{
  static int counter = 0;
  static char text[100];

  assert(prefix);
  assert(strlen(prefix)+10 >= 100);

  sprintf(text, "%s%d", prefix, counter);
  counter++;
  return gct_node_from_string(text, 0, (char *)0, 0, 0);
}


  

/*    ===%%SF%% nodes/creation-destruction (End)  === */


/*    ===%%SF%% nodes/list-manipulation (Start)  === */

/* Retrieve nth element of LIST, wrapping around if necessary. */ 
gct_node
gct_nth_node(list, n)
     gct_node list;
     int n;
{
  assert(list != GCT_NULL_NODE);
  assert(n >= 0);

  while (n > 0)
    {
      --n;
      list = list->next;
    }
  return list;
}

/* Unlink node from its NEXT and PREV neighbors. */
void
gct_unlink(node)
     gct_node node;
{
  gct_node next, prev;

  assert(GCT_NULL_NODE != node);

  next = node->next;
  prev = node->prev;
  next->prev = prev;
  prev->next = next;

  /* To more readily catch errors, clear link fields of node. */
  node->next = GCT_NULL_NODE;
  node->prev = GCT_NULL_NODE;
}


/* Remove a node from the list pointed to by header.  Header is updated 
   if necessary. */
void
gct_remove_node(header, node)
     gct_node *header;
     gct_node node;
{
  assert(GCT_NULL_NODE != node);
  assert(GCT_NULL_NODE != *header);

  if (*header == node)	/* Have to change header */
    {
      *header = node->next;
      if (*header == node)	/* single-element list */
	{
	  *header = GCT_NULL_NODE;
	}
    }
  gct_unlink(node);
  assert(gct_ok_list(*header));
}

#define MAKE_SINGLETON(node)	\
{				\
  (node)->prev = (node);	\
  (node)->next = (node);	\
}

#define ADD_BEFORE(existing, to_add)	\
{					\
  (to_add)->prev = (existing)->prev;	\
  (to_add)->next = (existing);		\
  (existing)->prev = (to_add);		\
  (to_add)->prev->next = (to_add);	\
}



/* Add node as first element of list pointed to by HEADER */
void
gct_add_first(header, node)
     gct_node *header;
     gct_node node;
{
  if (*header == GCT_NULL_NODE)
    {
      MAKE_SINGLETON(node);
    }
  else
    {
      ADD_BEFORE(*header, node);
    }
  *header = node;
  assert(gct_ok_list(*header));
}


/* Add node as last element of list pointed to by HEADER. */
void
gct_add_last(header, node)
     gct_node *header;
     gct_node node;
{
  if (GCT_NULL_NODE == *header)
    {
      MAKE_SINGLETON(node);
      *header = node;
    }
  else
    {
      gct_node fake_header = *header;
      gct_add_first(&fake_header, node);
    }
  assert(gct_ok_list(*header));
}

/* Add node before another node in list pointed to by header. */
void
gct_add_before(header, node_in_list, new_node)
     gct_node *header;
     gct_node node_in_list, new_node;
{
  assert(GCT_NULL_NODE != *header);
  assert(GCT_NULL_NODE != node_in_list);
  
  if (node_in_list == *header)
    {
      gct_add_first(header, new_node);
    }
  else
    {
      ADD_BEFORE(node_in_list, new_node);
    }
  assert(gct_ok_list(*header));
}

/*
 * Add node after another node in list pointed to by header. 
 * Note that the header isn't used; just there for consistency with
 * similar routines.
 */
void
gct_add_after(header, node_in_list, new_node)
     gct_node *header;
     gct_node node_in_list, new_node;
{
  assert(GCT_NULL_NODE != *header);
  assert(GCT_NULL_NODE != node_in_list);
  ADD_BEFORE(node_in_list->next, new_node);
  assert(gct_ok_list(*header));
}

/* Replace the OLD node with the new node in list pointed to by header. */
/* The old node is NOT freed. */
void
gct_replace_node(header, old_node, new_node)
     gct_node *header;
     gct_node old_node, new_node;
{
  assert(GCT_NULL_NODE != *header);
  ADD_BEFORE(old_node, new_node);
  gct_unlink(old_node);
  if (*header == old_node)
    {
      *header = new_node;
    }
  assert(gct_ok_list(*header));
}

/*
 * gct_find_earlier_match finds an earlier match for TOKEN, which must be
 * a "}" or ")" node.   gct_find_later_match looks in the other
 * direction.  They're different routines to make the sanity checking
 * cleaner.  The sanity checking has paid off in the past.
 */

#define BODY						\
      if (! rover->text)	/* Some tree. */	\
	continue;					\
      							\
      if (rover->text[0] == started)			\
	{						\
	  excess++;					\
	}						\
      else if (rover->text[0] == needed)		\
	{						\
	  excess--;					\
	  if (0 == excess)				\
	    {						\
	      return rover;				\
	    }						\
	}

gct_node
gct_find_earlier_match(token)
gct_node token;     
{
  gct_node rover;
  int excess = 1;	/* Number of opening characters we have seen. */
  char started = token->text[0];	/* The opening character */
  char needed = (started == ')' ? '(' : '{');	/* The closing character */

  sticky_assert(')' == token->text[0] || '}' == token->text[0]);
  
  for(rover = token->prev;rover!=token;rover=rover->prev)
    {
	BODY
    }
  fatal("gct_find_earlier_match looped without finding match %c\n", needed);
  /*NOTREACHED*/
}


gct_node
gct_find_later_match(token)
gct_node token;     
{
  gct_node rover;
  int excess = 1;	/* Number of opening characters we have seen. */
  char started = token->text[0];	/* The opening character */
  char needed = (started == '(' ? ')' : '}');	/* The closing character */

  sticky_assert('(' == token->text[0] || '{' == token->text[0]);
  
  for(rover = token->next;rover!=token;rover=rover->next)
    {
	BODY
    }
  fatal("gct_find_later_match looped without finding match %c\n", needed);
  /*NOTREACHED*/
}

/*
 * This routine finds the start of a simple declaration, one
 * ending in a SEMICOLON.  The start is the node after a preceding
 * open brace or declaration.  Brace-pairs are skipped.  Note that, since
 * statements are processed left-to-right, we will never see the
 * semicolon of the preceding declaration.
 */
gct_node
gct_find_start_of_declaration(semicolon)
gct_node semicolon;     
{
  gct_node rover;
  int braces = 1;	/* Number of opening braces we need. */
  
  for(rover = semicolon->prev;rover!=semicolon;rover=rover->prev)
    {
      if (GCT_DECLARATION == rover->type)
	{
	  return rover->next;
	}
      if (! rover->text)	/* A cast node or something else. */
	{
	  continue;
	}
      if (rover->text[0] == '}')
	{
	  braces++;
	}
      else if (rover->text[0] == '{')
	{
	  braces--;
	  if (0 == braces)
	    {
	      return rover->next;
	    }
	}
    }
  fatal("gct_find_start_of_declaration looped without finding match.\n");
  /*NOTREACHED*/
}


/* This routine finds a token preceding the LAST token whose text field
   is identically TEXT.  Returns NULL node if none is present. */

gct_node
gct_preceding_text(last, text)
     gct_node last;
     char *text;
{
  gct_node rover;

  for(rover=last->prev; rover!=last; rover=rover->prev)
    {
      if (rover->text && !strcmp(rover->text, text))
	{
	  return rover;
	}
    }
  return GCT_NULL_NODE;
}

  
/*
 * This routine finds a token preceding the LAST token whose text field
 * is identically TEXT or TEXT2.  Returns NULL node if none is present.
 * Calling gct_preceding_text twice may not be what you want -- the first
 * call may search too far backward and miss an instance of the second text.
 */

gct_node
gct_either_preceding_text(last, text, text2)
     gct_node last;
     char *text, *text2;
{
  gct_node rover;

  for(rover=last->prev; rover!=last; rover=rover->prev)
    {
      if (   rover->text
	  && (   !strcmp(rover->text, text)
	      || !strcmp(rover->text, text2)))
	{
	  return rover;
	}
    }
  return GCT_NULL_NODE;
}

  


/* Remove the sublist delimited (inclusively) by FIRST_NODE and LAST_NODE.
   The sublist does NOT include the header.   The sublist is formed into 
   a circular list.  The sublist must be a proper sublist. */

void
gct_cut_sublist(first_node, last_node)
     gct_node first_node;
     gct_node last_node;
{
  gct_node prev = first_node->prev;
  gct_node next = last_node->next;

  prev->next = next;
  next->prev = prev;

  first_node ->prev = last_node;
  last_node->next = first_node;
}


gct_ok_list(list)
     gct_node list;
{
  if (list)
    {
      gct_node rover = list;
      do
	{
	  if (   !rover
	      || !rover->next
	      || !rover->prev
	      || rover->next->prev != rover
	      || rover->prev->next != rover)
	    return 0;
	  rover = rover->next;
	}
      while (rover != list);
    }
  return 1;      
}

/*    ===%%SF%% nodes/list-manipulation (End)  === */



/* ===%%SF%% nodes (End)  === */
