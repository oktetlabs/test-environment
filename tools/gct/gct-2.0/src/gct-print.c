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

/* ===%%SF%% output (Start)  === */


static void comma_list_write();
static void recursive_write();
static void write_annotations();

/*
 * We're writing contents that appeared on this line in the original
 * source file (via include file or main file).  This increments as you'd
 * expect.  #line directives reset it.
 */
static int Current_output_line = 1;

  /*
   * Add newlines until the current line is at least the line number in
   * the node.
   *
   * Note that 0 linenos are used to denote inserted nodes, and no
   * newlines are added for them.
   */
  /* It would make sense to emit #line directives if the difference
     is great. */
#define ADD_NEWLINES(node) 			\
  while (Current_output_line < (node)->lineno)	\
    {						\
      fputc('\n', Gct_textout);			\
      Current_output_line++;			\
    }

/*
 * Follow a list, descending and writing each tree in the list.  
 */

static
basic_write(node)
     gct_node node;
{
  ADD_NEWLINES(node);
  fprintf(Gct_textout, " %s ", node->text);
  write_annotations(node);
}


/*
 * This routine converts a string with embedded special characters into
 * its C language representation:  newlines are converted to \n, etc. 
 * New storage is allocated for the string; the caller must free it.
 */

/* GCT:  GCC doesn't use ctype.h, so I don't either. Dunno why. */
#define isprint(char) ((char >= ' ') && (char <= '~'))


char *
slashified_string(s, numchars)
     char *s;		/* String to slashify. */
     int numchars;	/* Number of characters, excluding final null */
{
  /* Worst case is that every character turns into '\NNN'. */
  char *retval = (char *) xmalloc(4 * numchars + 1);
  char *cp = retval;		/* Roving pointer. */

  *(cp++) = '"';	/* Init with starting double-quote */
  numchars--;

  for (s++; numchars>0; s++,numchars--)
    {
      if ('\\' == *s)
	{
	  *(cp++)='\\';
	  *(cp++)='\\';
	}
      else if ('"' == *s)
	{
	  if (numchars>1)	/* if more chars, this is not closing quote */
	    {
	      *(cp++)='\\';
	    }
	  *(cp++)='"';
	}
      else if ('\t' == *s)
	{
	  *(cp++)='\\';
	  *(cp++)='t';
	}
      else if ('\n' == *s)
	{
	  *(cp++)='\\';
	  *(cp++)='n';
	}
      else if (isprint(*s))
	{
	  *(cp++)= *s;
	}
      else
	{
	  sprintf(cp, "\\%o", (unsigned char) *s);
	  cp += strlen(cp);
	}
    }
  *cp = '\0';
  return retval;
}



/*
 * Strings are written slightly differently:  we convert newlines back
 * into their original form.  Should this be done at lexing time?
 * Probably not, since this makes it easier to write C code that
 * processes the string's characters.
 *
 */
   
static
string_write(node)
     gct_node node;
{
  char *string;

  assert(node->text);

  ADD_NEWLINES(node);

  string = slashified_string(node->text, node->textlen);
  fputs(string, Gct_textout);
  free(string);
  
  write_annotations(node);
}


static
lparen()
{
  putc('(', Gct_textout);
}



static
rparen()
{
  putc(')', Gct_textout);
}

static
semi()
{
  putc(';', Gct_textout);
}

static
colon()
{
  putc(':', Gct_textout);
}

/* To ensure that annotations are written only once, we now free the
   annotation. */
static void
write_annotations(node)
     gct_node node;
{
  gct_annotation note_rover;

  /* List is built in reverse order -- reverse it */
  if (node->note && node->note->next)
    {
      gct_annotation first, second, third;
      first = node->note;
      second = first->next;
      first->next = GCT_NULL_ANNOTATION;
      while (second)
	{
	  third = second->next;
	  second->next = first;

	  first=second;
	  second=third;
	}
      node->note = first;
    }
  
  for (note_rover=node->note; note_rover; note_rover = note_rover->next)
    {
      write_one_annotation(note_rover);
    }
  gct_recursive_free_annotation(node->note);
  node->note = GCT_NULL_ANNOTATION;
}

/*
 * Write an annotation.  #line numbers change the line numbering as you
 * expect.  Any other note beginning with a newline (e.g., pragmas)
 * increments the Current_output_line by 2 (since it also ends with a
 * newline).  Another other does not change the line numbering - these
 * are GCT internal notes (like declarations of temporaries).
 *
 * If several non-#line line directives appear in a row, the line
 * numbering will be temporarily wrong (because each of them will take up
 * two lines in the output, but only one in the input).  This is rare,
 * and corrects itself fairly quickly, so is not worth fixing.
 */
write_one_annotation(note)
     gct_annotation note;
{
  int newlineno;

  /* fprintf in case there's a % in the text. */
  fprintf(Gct_textout, "%s", note->text);
  /* What a kludge! */
  if (1 == sscanf(note->text, "\n#line %d", &newlineno))
    {
      Current_output_line = newlineno;
    }
  else if ('\n' == note->text[0])
    {
      Current_output_line += 2;
    }
}

static void
comma_list_write(list)
     gct_node list;
{
  gct_node rover;
  
  if (list)
    {
      recursive_write(list);
      for (rover = list->next; rover!=list; rover=rover->next)
	{
	  putc(',', Gct_textout);
	  putc(' ', Gct_textout);
	  recursive_write(rover);
	}
    }
}


/*
 * Write all nodes past first, including first, but not including
 * past_last.  If first==past_last, nothing is written. 
 */

static void
comma_sublist_write(first, past_last)
     gct_node first, past_last;
{
  gct_node rover;
  
  if (first!=past_last)
    {
      recursive_write(first);
      for (rover = first->next; rover!=past_last; rover=rover->next)
	{
	  putc(',', Gct_textout);
	  putc(' ', Gct_textout);
	  recursive_write(rover);
	}
    }
}

  
void
gct_write_list(list)
     gct_node list;
{
  assert(Gct_textout != NULL);

  if (list)
    {
      gct_node rover = list;
      do 
	{
	  recursive_write(rover);
	  rover=rover->next;
	}
      while (rover != list);
    }
}


/*
 * Annotation writing:  any case that doesn't use basic_write must
 * explicitly write out its own annotations.  I haven't been particularly
 * careful about when the annotations were written -- before or after
 * children.
 */

static void
recursive_write(node)
     gct_node node;
{

  switch(node->type)
    {
      /* Order these correctly later. */
    case GCT_PLUS:
    case GCT_MINUS:
    case GCT_TIMES:
    case GCT_DIV:
    case GCT_MOD:
    case GCT_LSHIFT:
    case GCT_RSHIFT:
    case GCT_LESS:
    case GCT_GREATER:
    case GCT_LESSEQ:
    case GCT_GREATEREQ:
    case GCT_EQUALEQUAL:
    case GCT_NOTEQUAL:
    case GCT_BITAND:
    case GCT_BITOR:
    case GCT_BITXOR:
    case GCT_ANDAND:
    case GCT_OROR:
    case GCT_SIMPLE_ASSIGN:
    case GCT_PLUS_ASSIGN:
    case GCT_MINUS_ASSIGN:
    case GCT_TIMES_ASSIGN:
    case GCT_DIV_ASSIGN:
    case GCT_MOD_ASSIGN:
    case GCT_LSHIFT_ASSIGN:
    case GCT_RSHIFT_ASSIGN:
    case GCT_BITAND_ASSIGN:
    case GCT_BITOR_ASSIGN:
    case GCT_BITXOR_ASSIGN:
      lparen();
      recursive_write(GCT_OP_LEFT(node));
      basic_write(node);
      recursive_write(GCT_OP_RIGHT(node));
      rparen();
      break;
      
    case GCT_ADDR:
    case GCT_NEGATE:
    case GCT_UNARY_PLUS:
    case GCT_PREINCREMENT:
    case GCT_PREDECREMENT:
    case GCT_BIT_NOT:
    case GCT_TRUTH_NOT:
    case GCT_DEREFERENCE:
    case GCT_SIZEOF:
    case GCT_ALIGNOF:
    case GCT_EXTENSION:
      lparen();
      basic_write(node);
      recursive_write(GCT_OP_ONLY(node));
      rparen();
      break;

    case GCT_CAST:
      lparen();
      recursive_write(GCT_CAST_TYPE(node));
      write_annotations(node);
      recursive_write(GCT_CAST_EXPR(node));
      rparen();
      break;

    case GCT_QUEST:
      lparen();
      recursive_write(GCT_QUEST_TEST(node));
      basic_write(node);
      recursive_write(GCT_QUEST_TRUE(node));
      colon();
      recursive_write(GCT_QUEST_FALSE(node));
      rparen();
      break;

    case GCT_IDENTIFIER:
    case GCT_CONSTANT:
    case GCT_OTHER:
      if (GCT_STRING_CONSTANT_P(node))
	{
	  string_write(node);
	}
      else
	{
	  basic_write(node);
	}
      assert(!(node->children));
      break;

    case GCT_COMMA:
      lparen();
      write_annotations(node);
      comma_list_write(GCT_COMMA_OPERANDS(node));
      rparen();
      break;

    case GCT_FUNCALL:
      write_annotations(node);
      recursive_write(GCT_FUNCALL_FUNCTION(node));
      lparen();
      comma_sublist_write(GCT_FUNCALL_ARGS(node),
			  GCT_FUNCALL_FUNCTION(node));
      rparen();
      break;

    case GCT_ARRAYREF:
    case GCT_DOTREF:
    case GCT_ARROWREF:
      /* Note:  no reason for parens, as these have highest precedence. */
      recursive_write(GCT_REF_PRIMARY(node));
      basic_write(node);
      recursive_write(GCT_REF_SECONDARY(node));
      if (GCT_ARRAYREF == node->type)
	{
	  putc(']', Gct_textout);
	}
      break;
      
    case GCT_POSTINCREMENT:
    case GCT_POSTDECREMENT:
      lparen();
      recursive_write(GCT_OP_ONLY(node));
      basic_write(node);
      rparen();
      break;

    case GCT_TYPECRUD:
    case GCT_DECLARATION:
    case GCT_COMPOUND_STMT:
      gct_write_list(GCT_TYPECRUD_CRUD(node));
      write_annotations(node);	/* These will only be annotations promoted from
				   discarded following nodes. */
      break;

    case GCT_COMPOUND_EXPR:
      lparen();
      gct_write_list(GCT_TYPECRUD_CRUD(node));
      write_annotations(node);	/* These will only be annotations promoted from
				   discarded following nodes. */
      rparen();
      break;

    case GCT_SIMPLE_STMT:
      recursive_write(GCT_SIMPLE_STMT_BODY(node));
      semi();
      write_annotations(node);
      break;

    case GCT_IF:
      basic_write(node);
      lparen();
      recursive_write(GCT_IF_TEST(node));
      rparen();
      recursive_write(GCT_IF_THEN(node));
      if (GCT_IF_HAS_ELSE(node))
	{
	  fprintf(Gct_textout, " else ");
	  recursive_write(GCT_IF_ELSE(node));
	}
      break;

    case GCT_WHILE:
      basic_write(node);
      lparen();
      recursive_write(GCT_WHILE_TEST(node));
      rparen();
      recursive_write(GCT_WHILE_BODY(node));
      break;
      
    case GCT_DO:
      basic_write(node);
      recursive_write(GCT_DO_BODY(node));
      fprintf(Gct_textout, " while ");
      lparen();
      recursive_write(GCT_DO_TEST(node));
      rparen();
      semi();
      break;

    case GCT_FOR:
      basic_write(node);
      lparen();
      recursive_write(GCT_FOR_INIT(node));
      semi();
      recursive_write(GCT_FOR_TEST(node));
      semi();
      recursive_write(GCT_FOR_INCR(node));
      rparen();
      recursive_write(GCT_FOR_BODY(node));
      break;

    case GCT_NULL_EXPR:
      write_annotations(node);
      break;

    case GCT_SWITCH:
      basic_write(node);
      lparen();
      recursive_write(GCT_SWITCH_TEST(node));
      rparen();
      recursive_write(GCT_SWITCH_BODY(node));
      break;

    case GCT_CASE:
      basic_write(node);
      recursive_write(GCT_CASE_EXPR(node));
      colon();
      recursive_write(GCT_CASE_STMT(node));
      break;

    case GCT_DEFAULT:
      basic_write(node);
      colon();
      recursive_write(GCT_DEFAULT_STMT(node));
      break;

    case GCT_BREAK:
    case GCT_CONTINUE:
      basic_write(node);
      semi();
      break;

    case GCT_RETURN:
      basic_write(node);
      recursive_write(GCT_RETURN_EXPR(node));
      semi();
      break;

    case GCT_ASM:
      basic_write(node);
      gct_write_list(GCT_ASM_CRUD(node));
      semi();
      break;

    case GCT_GOTO:
      basic_write(node);
      recursive_write(GCT_GOTO_LABEL(node));
      semi();
      break;

    case GCT_LABEL:
      basic_write(node);
      colon();
      recursive_write(GCT_LABEL_STMT(node));
      break;

    default:
      fatal("Unknown node type %d\n", node->type);
    }
  fflush(Gct_textout);
  
}

     

/* ===%%SF%% output (End)  === */



/* ===%%SF%% debug (Start)  === */

char * Gct_type_names[NUM_GCT_TREE_CODES+1] = 
{
#define DEF_GCT_WEAK_TREE(token, string, ifunc, expfunc, lvfunc) string,
#include "gct-tree.def"
#undef DEF_GCT_WEAK_TREE
  "something to follow the final comma"
};


indent(stream, count)
     FILE *stream;
     int count;
{
  for (;count>0;count--)
    putc(' ', stream);
}

debug_write_annotations(stream, node, indent_count)
     FILE *stream;
     gct_node node;
     int indent_count;
{
  gct_annotation note_rover;
  for (note_rover=node->note; note_rover; note_rover = note_rover->next)
    {
      indent(stream, indent_count);
      fprintf(stream, "%s\n", note_rover->text);
    }
}


gct_dump_tree(stream, root, show_gcc_tree)
     FILE *stream;
     gct_node root;
     int show_gcc_tree;
{
  static int indent_count = 0;

  indent(stream, indent_count);
  if ((int)root->type < 0 || (int)root->type >= NUM_GCT_TREE_CODES)
    {
      fprintf(stream, "node with unknown type %d\n", root->type);
    }
  else
    {
      fprintf(stream, "%s: name %s, pos %d/%d, vol %d\n",
	      Gct_type_names[(int)root->type],
	      root->text ? root->text : "(none)",
	      root->lineno, root->first_char, root->is_volatile);
	      
      if (root->note)
	{
	  debug_write_annotations(stream, root, indent_count+1);
	}
      if (show_gcc_tree && root->gcc_type)
	print_node (stream, "", root->gcc_type, 0);
    }

  if (root->children)
    {
      gct_node rover = root->children;
      indent_count += 2;
      gct_dump_tree(stream, rover, show_gcc_tree);
      for (rover = rover->next; rover!=root->children; rover=rover->next)
	gct_dump_tree(stream, rover, show_gcc_tree);
      indent_count -= 2;
    }      
}

gct_debug_dump_tree(root, show_gcc_tree)
     gct_node root;
     int show_gcc_tree;
{
  gct_dump_tree(stderr, root, show_gcc_tree);
  
}


/* ===%%SF%% debug (End)  === */

static int recursive_mccabe();
static int comma_list_mccabe();
static int comma_sublist_mccabe();
static int recursive_nonreducible();
static int comma_list_nonreducible();
static int comma_sublist_nonreducible();
static void simple_push_switch();
static void simple_pop_switch();
static int simple_switch_default_seen();
static void simple_now_switch_has_default();

int gct_compute_mccabe(list)
    gct_node list;
{
    int mccabe = 0;
    if (list) {
	gct_node rover = list;
	do {
	    mccabe += recursive_mccabe(rover);
	    rover=rover->next;
	} while (rover != list);
    }
    return mccabe;
}

static int recursive_mccabe(node)
    gct_node node;
{
  int mccabe = 0;
  switch(node->type)
    {
      /* Order these correctly later. */
    case GCT_PLUS:
    case GCT_MINUS:
    case GCT_TIMES:
    case GCT_DIV:
    case GCT_MOD:
    case GCT_LSHIFT:
    case GCT_RSHIFT:
    case GCT_LESS:
    case GCT_GREATER:
    case GCT_LESSEQ:
    case GCT_GREATEREQ:
    case GCT_EQUALEQUAL:
    case GCT_NOTEQUAL:
    case GCT_BITAND:
    case GCT_BITOR:
    case GCT_BITXOR:
    case GCT_ANDAND:
    case GCT_OROR:
    case GCT_SIMPLE_ASSIGN:
    case GCT_PLUS_ASSIGN:
    case GCT_MINUS_ASSIGN:
    case GCT_TIMES_ASSIGN:
    case GCT_DIV_ASSIGN:
    case GCT_MOD_ASSIGN:
    case GCT_LSHIFT_ASSIGN:
    case GCT_RSHIFT_ASSIGN:
    case GCT_BITAND_ASSIGN:
    case GCT_BITOR_ASSIGN:
    case GCT_BITXOR_ASSIGN:
      mccabe += recursive_mccabe(GCT_OP_LEFT(node));
      mccabe += recursive_mccabe(GCT_OP_RIGHT(node));
      break;
      
    case GCT_ADDR:
    case GCT_NEGATE:
    case GCT_UNARY_PLUS:
    case GCT_PREINCREMENT:
    case GCT_PREDECREMENT:
    case GCT_BIT_NOT:
    case GCT_TRUTH_NOT:
    case GCT_DEREFERENCE:
    case GCT_SIZEOF:
    case GCT_ALIGNOF:
    case GCT_EXTENSION:
      mccabe += recursive_mccabe(GCT_OP_ONLY(node));
      break;

    case GCT_CAST:
      mccabe += recursive_mccabe(GCT_CAST_TYPE(node));
      mccabe += recursive_mccabe(GCT_CAST_EXPR(node));
      break;

    case GCT_QUEST:
      mccabe += recursive_mccabe(GCT_QUEST_TEST(node));
      mccabe += recursive_mccabe(GCT_QUEST_TRUE(node));
      mccabe += recursive_mccabe(GCT_QUEST_FALSE(node));
      mccabe++;
      break;

    case GCT_IDENTIFIER:
    case GCT_CONSTANT:
    case GCT_OTHER:
      assert(!(node->children));
      break;

    case GCT_COMMA:
      mccabe += comma_list_mccabe(GCT_COMMA_OPERANDS(node));
      break;

    case GCT_FUNCALL:
      mccabe += recursive_mccabe(GCT_FUNCALL_FUNCTION(node));
      mccabe += comma_sublist_mccabe(GCT_FUNCALL_ARGS(node),
			   GCT_FUNCALL_FUNCTION(node));
      break;

    case GCT_ARRAYREF:
    case GCT_DOTREF:
    case GCT_ARROWREF:
      mccabe += recursive_mccabe(GCT_REF_PRIMARY(node));
      mccabe += recursive_mccabe(GCT_REF_SECONDARY(node));
      break;
      
    case GCT_POSTINCREMENT:
    case GCT_POSTDECREMENT:
      mccabe += recursive_mccabe(GCT_OP_ONLY(node));
      break;

    case GCT_TYPECRUD:
    case GCT_DECLARATION:
    case GCT_COMPOUND_STMT:
      mccabe += gct_compute_mccabe(GCT_TYPECRUD_CRUD(node));
      break;

    case GCT_COMPOUND_EXPR:
      mccabe += gct_compute_mccabe(GCT_TYPECRUD_CRUD(node));
      break;

    case GCT_SIMPLE_STMT:
      mccabe += recursive_mccabe(GCT_SIMPLE_STMT_BODY(node));
      break;

    case GCT_IF:
      mccabe += recursive_mccabe(GCT_IF_TEST(node));
      mccabe += recursive_mccabe(GCT_IF_THEN(node));
      if (GCT_IF_HAS_ELSE(node))
	{
	  mccabe += recursive_mccabe(GCT_IF_ELSE(node));
	}
      mccabe++;
      break;

    case GCT_WHILE:
      mccabe += recursive_mccabe(GCT_WHILE_TEST(node));
      mccabe += recursive_mccabe(GCT_WHILE_BODY(node));
      mccabe++;
      break;
      
    case GCT_DO:
      mccabe += recursive_mccabe(GCT_DO_BODY(node));
      mccabe += recursive_mccabe(GCT_DO_TEST(node));
      mccabe++;
      break;

    case GCT_FOR:
      mccabe += recursive_mccabe(GCT_FOR_INIT(node));
      mccabe += recursive_mccabe(GCT_FOR_TEST(node));
      mccabe += recursive_mccabe(GCT_FOR_INCR(node));
      mccabe += recursive_mccabe(GCT_FOR_BODY(node));
      mccabe++;
      break;

    case GCT_NULL_EXPR:
      break;

    case GCT_SWITCH:
      simple_push_switch();
      mccabe += recursive_mccabe(GCT_SWITCH_TEST(node));
      mccabe += recursive_mccabe(GCT_SWITCH_BODY(node));
      if (simple_switch_default_seen()) 
	  mccabe--;
      simple_pop_switch();
      break;

    case GCT_CASE:
      mccabe += recursive_mccabe(GCT_CASE_EXPR(node));
      mccabe += recursive_mccabe(GCT_CASE_STMT(node));
      mccabe++;
      break;

    case GCT_DEFAULT:
      mccabe += recursive_mccabe(GCT_DEFAULT_STMT(node));
      simple_now_switch_has_default();
      mccabe++;
      break;

    case GCT_BREAK:
    case GCT_CONTINUE:
      break;

    case GCT_RETURN:
      mccabe += recursive_mccabe(GCT_RETURN_EXPR(node));
      break;

    case GCT_ASM:
	break;

    case GCT_GOTO:
      mccabe += recursive_mccabe(GCT_GOTO_LABEL(node));
      break;

    case GCT_LABEL:
      mccabe += recursive_mccabe(GCT_LABEL_STMT(node));
      break;

    default:
      fatal("Unknown node type %d\n", node->type);
    }
  return mccabe;
  
}

static int comma_list_mccabe(list)
    gct_node list;
{
    int mccabe = 0;
    gct_node rover;
  
    if (list) {
	mccabe += recursive_mccabe(list);
	for (rover = list->next; rover!=list; rover=rover->next) {
	    mccabe += recursive_mccabe(rover);
	}
    }
    return mccabe;
}


static int comma_sublist_mccabe(first, past_last)
    gct_node first, past_last;
{
    gct_node rover;
    int mccabe=0;
  
    if (first!=past_last) {
	mccabe += recursive_mccabe(first);
	for (rover = first->next; rover!=past_last; rover=rover->next) {
	    mccabe += recursive_mccabe(rover);
	}
    }
    return mccabe;
}

int gct_compute_nonreducible(list, incase)
    gct_node list;
    int incase;
{
    int nonreducible = 0;
    if (list) {
	gct_node rover = list;
	do {
	    nonreducible += recursive_nonreducible(rover, incase);
	    rover=rover->next;
	} while (rover != list);
    }
    return nonreducible;
}

static int recursive_nonreducible(node, incase)
    gct_node node;
    int incase;
{
  int nonreducible = 0;
  switch(node->type)
    {
      /* Order these correctly later. */
    case GCT_PLUS:
    case GCT_MINUS:
    case GCT_TIMES:
    case GCT_DIV:
    case GCT_MOD:
    case GCT_LSHIFT:
    case GCT_RSHIFT:
    case GCT_LESS:
    case GCT_GREATER:
    case GCT_LESSEQ:
    case GCT_GREATEREQ:
    case GCT_EQUALEQUAL:
    case GCT_NOTEQUAL:
    case GCT_BITAND:
    case GCT_BITOR:
    case GCT_BITXOR:
    case GCT_ANDAND:
    case GCT_OROR:
    case GCT_SIMPLE_ASSIGN:
    case GCT_PLUS_ASSIGN:
    case GCT_MINUS_ASSIGN:
    case GCT_TIMES_ASSIGN:
    case GCT_DIV_ASSIGN:
    case GCT_MOD_ASSIGN:
    case GCT_LSHIFT_ASSIGN:
    case GCT_RSHIFT_ASSIGN:
    case GCT_BITAND_ASSIGN:
    case GCT_BITOR_ASSIGN:
    case GCT_BITXOR_ASSIGN:
      nonreducible += recursive_nonreducible(GCT_OP_LEFT(node), incase);
      nonreducible += recursive_nonreducible(GCT_OP_RIGHT(node), incase);
      break;
      
    case GCT_ADDR:
    case GCT_NEGATE:
    case GCT_UNARY_PLUS:
    case GCT_PREINCREMENT:
    case GCT_PREDECREMENT:
    case GCT_BIT_NOT:
    case GCT_TRUTH_NOT:
    case GCT_DEREFERENCE:
    case GCT_SIZEOF:
    case GCT_ALIGNOF:
    case GCT_EXTENSION:
      nonreducible += recursive_nonreducible(GCT_OP_ONLY(node), incase);
      break;

    case GCT_CAST:
      nonreducible += recursive_nonreducible(GCT_CAST_TYPE(node), incase);
      nonreducible += recursive_nonreducible(GCT_CAST_EXPR(node), incase);
      break;

    case GCT_QUEST:
      nonreducible += recursive_nonreducible(GCT_QUEST_TEST(node), incase);
      nonreducible += recursive_nonreducible(GCT_QUEST_TRUE(node), incase);
      nonreducible += recursive_nonreducible(GCT_QUEST_FALSE(node), incase);
      break;

    case GCT_IDENTIFIER:
    case GCT_CONSTANT:
    case GCT_OTHER:
      assert(!(node->children));
      break;

    case GCT_COMMA:
      nonreducible += comma_list_nonreducible(GCT_COMMA_OPERANDS(node), incase);
      break;

    case GCT_FUNCALL:
      nonreducible += recursive_nonreducible(GCT_FUNCALL_FUNCTION(node), incase);
      nonreducible += comma_sublist_nonreducible(GCT_FUNCALL_ARGS(node),
			   GCT_FUNCALL_FUNCTION(node), incase);
      if (strcmp(GCT_FUNCALL_FUNCTION(node)->text, "abort") == 0 ||
	  strcmp(GCT_FUNCALL_FUNCTION(node)->text, "exit") == 0) {
	  nonreducible++;
      }
      break;

    case GCT_ARRAYREF:
    case GCT_DOTREF:
    case GCT_ARROWREF:
      nonreducible += recursive_nonreducible(GCT_REF_PRIMARY(node), incase);
      nonreducible += recursive_nonreducible(GCT_REF_SECONDARY(node), incase);
      break;
      
    case GCT_POSTINCREMENT:
    case GCT_POSTDECREMENT:
      nonreducible += recursive_nonreducible(GCT_OP_ONLY(node), incase);
      break;

    case GCT_TYPECRUD:
    case GCT_DECLARATION:
    case GCT_COMPOUND_STMT:
      nonreducible += gct_compute_nonreducible(GCT_TYPECRUD_CRUD(node), incase);
      break;

    case GCT_COMPOUND_EXPR:
      nonreducible += gct_compute_nonreducible(GCT_TYPECRUD_CRUD(node), incase);
      break;

    case GCT_SIMPLE_STMT:
      nonreducible += recursive_nonreducible(GCT_SIMPLE_STMT_BODY(node), incase);
      break;

    case GCT_IF:
      nonreducible += recursive_nonreducible(GCT_IF_TEST(node), incase);
      nonreducible += recursive_nonreducible(GCT_IF_THEN(node), incase);
      if (GCT_IF_HAS_ELSE(node))
	{
	  nonreducible += recursive_nonreducible(GCT_IF_ELSE(node), incase);
	}
      break;

    case GCT_WHILE:
      nonreducible += recursive_nonreducible(GCT_WHILE_TEST(node), incase);
      nonreducible += recursive_nonreducible(GCT_WHILE_BODY(node), 0);
      break;
      
    case GCT_DO:
      nonreducible += recursive_nonreducible(GCT_DO_BODY(node), 0);
      nonreducible += recursive_nonreducible(GCT_DO_TEST(node), incase);
      break;

    case GCT_FOR:
      nonreducible += recursive_nonreducible(GCT_FOR_INIT(node), incase);
      nonreducible += recursive_nonreducible(GCT_FOR_TEST(node), incase);
      nonreducible += recursive_nonreducible(GCT_FOR_INCR(node), incase);
      nonreducible += recursive_nonreducible(GCT_FOR_BODY(node), 0);
      break;

    case GCT_NULL_EXPR:
      break;

    case GCT_SWITCH:
      nonreducible += recursive_nonreducible(GCT_SWITCH_TEST(node), incase);
      nonreducible += recursive_nonreducible(GCT_SWITCH_BODY(node), 1);
      break;

    case GCT_CASE:
      nonreducible += recursive_nonreducible(GCT_CASE_EXPR(node), incase);
      nonreducible += recursive_nonreducible(GCT_CASE_STMT(node), 1);
      break;

    case GCT_DEFAULT:
      nonreducible += recursive_nonreducible(GCT_DEFAULT_STMT(node), 1);
      break;

    case GCT_BREAK:
	if (! incase) {
	    nonreducible++;
	}
      break;
    case GCT_CONTINUE:
	nonreducible++;
      break;

    case GCT_RETURN:
      nonreducible += recursive_nonreducible(GCT_RETURN_EXPR(node), incase);
      nonreducible++;
      break;

    case GCT_ASM:
	break;

    case GCT_GOTO:
      nonreducible += recursive_nonreducible(GCT_GOTO_LABEL(node), incase);
      nonreducible++;
      break;

    case GCT_LABEL:
      nonreducible += recursive_nonreducible(GCT_LABEL_STMT(node), incase);
      break;

    default:
      fatal("Unknown node type %d\n", node->type);
    }
  return nonreducible;
  
}

static int comma_list_nonreducible(list, incase)
    gct_node list;
    int incase;
{
    int nonreducible = 0;
    gct_node rover;
  
    if (list) {
	nonreducible += recursive_nonreducible(list, incase);
	for (rover = list->next; rover!=list; rover=rover->next) {
	    nonreducible += recursive_nonreducible(rover, incase);
	}
    }
    return nonreducible;
}


static int comma_sublist_nonreducible(first, past_last, incase)
    gct_node first, past_last;
    int incase;
{
    gct_node rover;
    int nonreducible=0;
  
    if (first!=past_last) {
	nonreducible += recursive_nonreducible(first, incase);
	for (rover = first->next; rover!=past_last; rover=rover->next) {
	    nonreducible += recursive_nonreducible(rover, incase);
	}
    }
    return nonreducible;
}

static int Switch_level = 0;
static int Switch_array_size = 2;
static int *Switches = (int *)0;

static void simple_push_switch()
{
  Switch_level++;

  if ((int *) 0 == Switches) {
      Switches = (int *) xmalloc(Switch_array_size * sizeof(int));
  } else if (Switch_level == Switch_array_size) {
      Switch_array_size *= 2;
      Switches = (int *) xrealloc(Switches, Switch_array_size * sizeof(int));
  }
  Switches[Switch_level] = 0;
}


/*
 * Note that the default_seen variable is not necessarily 0.  Consider
 * this legal C switch:
 *
 * 	switch(c)
 * 	   printf("Hello, world\n");
 */
static void simple_pop_switch()
{
  Switch_level--;
}


/* Predicate:  has default been seen yet? */
static int simple_switch_default_seen()
{
  return Switches[Switch_level];
}

/* Use this to record that a default has been seen. */
static void simple_now_switch_has_default()
{
  Switches[Switch_level] = 1;
}

