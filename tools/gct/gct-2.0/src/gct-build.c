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

/* Routines for rearranging the node tree. */
#include <stdio.h>
#include "config.h"
#include "tree.h"
#include "gct-util.h"
#include "gct-assert.h"


/*
 * When a node is thrown away, its annotations must be promoted to the
 * previous node.  Note that this can shift tokens around annotations.
 * So be it.  Keeping parenthesis, semicolons, and the like around would 
 * complicate things.  The slight shifting will probably never be noticed.
 *
 * Because annotations are stored in reverse order, the promoted
 * annotations are at the head of the list.
 */

static void
promote_annotations(from, target_node)
     gct_node from, target_node;
{
  if (from->note && target_node->note)
    {
      gct_annotation rover;
      for (rover=from->note; rover->next; rover=rover->next)
	;
      rover->next = target_node->note;
      target_node->note = from->note;
    }
  else if (from->note)
    {
      target_node->note = from->note;
    }
  from->note = GCT_NULL_ANNOTATION;
}  


/*
 * Given a treelist of the form PAREN TREE PAREN, it removes and frees
 * the parens, promoting annotations.  We assume that there is a note
 * prior to the opening paren.  If not, annotations will be promoted to
 * the closing paren and thence to the tree.  This isn't horrible, but it
 * would be better to emit the annotation in this case.
 */
void
gct_flush_parens(first_paren)
     gct_node first_paren;
{
  gct_node exprtree = first_paren->next;
  gct_node last_paren = exprtree->next;
  
  promote_annotations(first_paren, first_paren->prev);
  promote_annotations(last_paren, exprtree);
  gct_remove_node(&Gct_all_nodes, first_paren);
  gct_remove_node(&Gct_all_nodes, last_paren);
  gct_recursive_free_node(first_paren);
  gct_recursive_free_node(last_paren);
}

void
gct_flush_semi()
{
  gct_free_sugar(GCT_LAST(Gct_all_nodes),
		 GCT_LAST(Gct_all_nodes)->prev);
}



/*
 * GCC doesn't always set volatile.  For example, when building a
 * comma-expression chain, volatility doesn't propagate up to the chain
 * node.  We play it safe:  a node is VOLATILE if
 * 1.  the gcc-node is TREE_VOLATILE or TREE_THIS_VOLATILE.
 * note: TREE_VOLATILE is undefined symbol in gcc-2.3.3!
 *       seems to have been replatec by TYPE_VOLATILE
 * 2.  Any of the children are volatile.
 * 3.  The node has already been set volatile for whatever reason.
 * Note that it does no particular harm to think a node is volatile --
 * just means extra rewriting.
 */
static void
set_volatile(root, gcctree)
     gct_node root;
     tree gcctree;
{
  if (root->is_volatile)
    return;
  
  if (TYPE_VOLATILE(gcctree) || TREE_THIS_VOLATILE(gcctree))
    {
      root->is_volatile = 1;
    }
#if 0
  /* This is inappropriate for my use.  I think. */
  else if (root->children)
    {
      gct_node rover = root->children;
      do
	{
	  if (rover->is_volatile)
	    {
	      root->is_volatile = 1;
	      return;
	    }
	  rover = rover->next;
	}
      while (rover != root->children);
    }
#endif
}



void
gct_free_sugar(sugar, annotation_destination)
     gct_node sugar, annotation_destination;
{
  promote_annotations(sugar, annotation_destination);
  gct_remove_node(&Gct_all_nodes, sugar);
  gct_recursive_free_node(sugar);
}


/* Note:  nodes must be moved below IN ORDER. */
move_below(root, new_child)
     gct_node root, new_child;
{
  gct_remove_node(&Gct_all_nodes, new_child);
  GCT_ADD(root, new_child);
}


/*
 * A hack is here.  The callers always assume that a lookahead token is
 * present.  This is not always true:  consider 1 * sizeof(int) -- the
 * times can be reduced immediately because nothing with higher
 * precedence can come after the sizeof.  (I do not know why the same is
 * NOT true of i*1, but it's not.)
 *
 * If the root is not a GCT_OTHER, an inappropriate lookahead was done.
 * and we have to undo it.  If that doesn't work, would be best to go to
 * some general solution.
 */
gct_build_binary(root, type, gcctree)
     gct_node root;
     enum gct_node_type type;
     tree gcctree;
{
  gct_node left, right;

  if (GCT_OTHER != root->type)
    {
      root=root->next;
    }
  
  
  right = root->next;
  left = root->prev;
  
  gct_remove_node(&Gct_all_nodes, right);
  gct_remove_node(&Gct_all_nodes, left);
  
  GCT_ADD(root, left);
  GCT_ADD(root, right);
  root->type = type;
  set_volatile(root, gcctree);
  root->gcc_type = TREE_TYPE(gcctree);
}  

/* Tensify later. */
gct_build_arithcompare(root, gcctree)
     gct_node root;
     tree gcctree;
{
  char *text = root->text;
  
  if ('<' == text[0] )
    {
      if ('=' == text[1])
	gct_build_binary(root, GCT_LESSEQ, gcctree);
      else if ('\0' == text[1])
	gct_build_binary(root, GCT_LESS, gcctree);
      else
	fatal("Bad arithcomparison");
    }
  else if ('>' == text[0])
    {
      if ('=' == text[1])
	gct_build_binary(root, GCT_GREATEREQ, gcctree);
      else if ('\0' == text[1])
	gct_build_binary(root, GCT_GREATER, gcctree);
      else
	fatal("Bad arithcomparison");
    }
  else
    fatal("Bad arithcomparison.");
}

gct_build_eqcompare(root, gcctree)
     gct_node root;
     tree gcctree;
{
  char *text = root->text;
  
  if ('=' == text[0])
    gct_build_binary(root, GCT_EQUALEQUAL, gcctree);
  else if ('!' == text[0])
    gct_build_binary(root, GCT_NOTEQUAL, gcctree);
  else
    fatal("Bad build_eqcompare");
}


gct_build_nonsimple_assign(root, gcctree)
     gct_node root;
     tree gcctree;
{
  switch((root->text)[0])
    {
    case '+':
      gct_build_binary(root, GCT_PLUS_ASSIGN, gcctree);
      break;
    case '-':
      gct_build_binary(root, GCT_MINUS_ASSIGN, gcctree);
      break;
    case '*':
      gct_build_binary(root, GCT_TIMES_ASSIGN, gcctree);
      break;
    case '/':
      gct_build_binary(root, GCT_DIV_ASSIGN, gcctree);
      break;
    case '%':
      gct_build_binary(root, GCT_MOD_ASSIGN, gcctree);
      break;
    case '<':
      gct_build_binary(root, GCT_LSHIFT_ASSIGN, gcctree);
      break;
    case '>':
      gct_build_binary(root, GCT_RSHIFT_ASSIGN, gcctree);
      break;
    case '&':
      gct_build_binary(root, GCT_BITAND_ASSIGN, gcctree);
      break;
    case '|':
      gct_build_binary(root, GCT_BITOR_ASSIGN, gcctree);
      break;
    case '^':
      gct_build_binary(root, GCT_BITXOR_ASSIGN, gcctree);
      break;
    default:
      fatal("Bad build_nonsimple_assign");
    }
}



gct_build_unary_by_gcctype(root, gcctype, gcctree)
     gct_node root;
     int gcctype;
     tree gcctree;
{
  enum gct_node_type type;
  
  switch(gcctype)
    {
    case ADDR_EXPR:
      type = GCT_ADDR;
      break;
    case NEGATE_EXPR:
      type = GCT_NEGATE;
      break;
    case CONVERT_EXPR:
      type = GCT_UNARY_PLUS;
      break;
    case PREINCREMENT_EXPR:
      type = GCT_PREINCREMENT;
      break;
    case PREDECREMENT_EXPR:
      type = GCT_PREDECREMENT;
      break;
    case BIT_NOT_EXPR:
      type = GCT_BIT_NOT;
      break;
    case TRUTH_NOT_EXPR:
      type = GCT_TRUTH_NOT;
      break;
    default:
      fatal("gct_build_unary");
    }
  gct_build_unary(root, type, gcctree);
}


gct_build_unary(root, type, gcctree)
     gct_node root;
     enum gct_node_type type;
     tree gcctree;
{
  gct_node expr = root->next;
  
  gct_remove_node(&Gct_all_nodes, expr);
  
  GCT_ADD(root, expr);
  root->type = type;
  set_volatile(root, gcctree);
  root->gcc_type = TREE_TYPE(gcctree);
}  

/* ( much type crud ) expr */
gct_build_cast(expr, gcctree)
     gct_node expr;
     tree gcctree;
{
  gct_node rparen = expr->prev;
  gct_node lparen = gct_find_earlier_match(rparen);
  gct_node cast = gct_located_placeholder(lparen);
  gct_node typenode = gct_located_placeholder(lparen);

  gct_add_before(&Gct_all_nodes, lparen, cast);

  /* CAST ( much type crud) expr */
  
  gct_cut_sublist(lparen, rparen);
  typenode->type = GCT_TYPECRUD;
  typenode->children = lparen;

  /* CAST expr */
  GCT_ADD(cast, typenode);
  move_below(cast, expr);

  cast->type = GCT_CAST;
  set_volatile(cast, gcctree);
  cast->gcc_type = TREE_TYPE(gcctree);
}  

gct_build_quest(root, gcctree)
     gct_node root;
     tree gcctree;
{
  gct_node test = root->prev;
  gct_node truecase = root->next;
  gct_node colon = truecase->next;
  gct_node falsecase = colon->next;
  
  gct_remove_node(&Gct_all_nodes, test);
  gct_remove_node(&Gct_all_nodes, truecase);
  gct_remove_node(&Gct_all_nodes, colon);
  gct_remove_node(&Gct_all_nodes, falsecase);
  
  promote_annotations(colon, truecase);
  gct_recursive_free_node(colon);
  
  GCT_ADD(root, test);
  GCT_ADD(root, truecase);
  GCT_ADD(root, falsecase);
  
  root->type = GCT_QUEST;
  set_volatile(root, gcctree);
  root->gcc_type = TREE_TYPE(gcctree);
}


/* There's a sort-of shift/reduce conflict for identifiers.  This handles
   it very ungracefully. */

#define CANT_BE_IDENTIFIER(text) \
      (!(text) || (   !((text)[0] >= 'a' && (text)[0] <= 'z') \
		   && !((text)[0] >= 'A' && (text)[0] <= 'Z') \
		   && (text)[0] != '_'))
gct_build_item(root, type, gcctree)
     gct_node root;
     enum gct_node_type type;
     tree gcctree;
{
  if (   GCT_IDENTIFIER == type)
    {
      if (CANT_BE_IDENTIFIER(root->text))
	{
	  /* if (root->text) fprintf(stderr, "SHIFTING %s\n", root->text); */
	  root = root->prev;
	  /* if (root->text) fprintf(stderr, "GOT %s\n", root->text); */
	  if (CANT_BE_IDENTIFIER(root->text))
	    {
	      warning("%s doesn't look like an identifier.", root->text);
	    }
	}
#if 0
      else
	{
	  if (root->text) fprintf(stderr, "DIDN'T SHIFT %s\n", root->text);
	}
#endif      
    }

  root->type = type;
  set_volatile(root, gcctree);
  root->gcc_type = TREE_TYPE(gcctree);
}


/*
 * GCC uses the same productions for function-parameter lists and
 * ordinary comma lists.  This causes some ugliness here, since this code
 * must do the right thing for both cases.  Unlike most combinings, we
 * don't need to fetch the type from the gcctree, we just bubble the type
 * up from the rightmost expression.  We retain the gcctree argument just
 * for consistency with other similar functions.
 *
 * Note that we punt and call all commas volatile.  They usually are. 
 */
gct_build_comma_list(comma, gcctree)
     gct_node comma;
     tree gcctree;
{
  gct_node leftexpr = comma->prev;
  gct_node rightexpr = comma->next;
  
  if (GCT_COMMA == leftexpr->type)	/* Continue a list. */
    {
      promote_annotations(comma, leftexpr);
      
      gct_remove_node(&Gct_all_nodes, comma);	/* This token not used. */
      assert(!comma->children);
      gct_recursive_free_node(comma);
      
      gct_remove_node(&Gct_all_nodes, rightexpr);
      gct_add_last(&(GCT_COMMA_OPERANDS(leftexpr)), rightexpr);

      /* Update type of entire comma list. */
      leftexpr->gcc_type = rightexpr->gcc_type;
    }
  else
    {
      gct_remove_node(&Gct_all_nodes, rightexpr);
      gct_remove_node(&Gct_all_nodes, leftexpr);
  
      GCT_ADD(comma, leftexpr);
      GCT_ADD(comma, rightexpr);
      comma->type = GCT_COMMA;
      comma->is_volatile = 1;
      comma->gcc_type = rightexpr->gcc_type;
    }
}

/*
 * This is needed to distinguish 2-element function call lists from
 * single-element lists that are commas.  Kind of a kludge.  When an
 * expression production is reduced, a top-level comma is guarded by
 * surrounding it with another comma-expression.  Thus, if that
 * comma-expression is used in a function call argument list, the arglist
 * will look like (, (, a b) <remaining arguments>), which is distinct
 * from a "normal" argument list, which looks like (, a b <remaining arguments>).
 */
gct_guard_comma(comma)
     gct_node comma;
{
  gct_node newcomma = gct_located_placeholder(comma);

  assert(GCT_COMMA == comma->type);

  gct_add_before(&Gct_all_nodes, comma, newcomma);
  move_below(newcomma, comma);

  newcomma->type = GCT_COMMA;
  newcomma->is_volatile = comma->is_volatile;
  newcomma->gcc_type = comma->gcc_type;
}


/*
 * expression ( 0-or-more-expressions ) 
 */
gct_build_function_call(call, exprlist, gcctree)
     gct_node call;
     gct_node exprlist;
     tree gcctree;
{
  gct_node root = gct_located_placeholder(call);
  gct_add_before(&Gct_all_nodes, call, root);

  /* Get rid of parens. */
  if (GCT_NULL_NODE == exprlist)
    {
      gct_free_sugar(call->next->next, call->next);
      gct_free_sugar(call->next, call);
    }
  else
    {
      gct_flush_parens(call->next);
    }
  
  
  if (GCT_NULL_NODE == exprlist)	/* No arguments. */
    {
      move_below(root, call);
    }
  else if (GCT_COMMA == exprlist->type)	/* Multiple arguments. */
    {
      promote_annotations(exprlist, call);

      gct_remove_node(&Gct_all_nodes, call);
      gct_remove_node(&Gct_all_nodes, exprlist);
      
      /* boom goes the modularity */
      root->children = GCT_COMMA_OPERANDS(exprlist);
      gct_add_first(&(root->children), call);

      GCT_COMMA_OPERANDS(exprlist) = GCT_NULL_NODE;
      gct_recursive_free_node(exprlist);
    }
  else					/* One argument. */
    {
      move_below(root, call);
      move_below(root, exprlist);
    }

  root->type = GCT_FUNCALL;
  set_volatile(root, gcctree);
/***  warning never used in gct
 ***  changed for update gct-1.5 -> gct-1.5
 ***/
  root->is_volatile = 1;  
/***  if (!root->is_volatile)
 ***   {
 ***     warning("Function isn't volatile?");
 ***   }
 ***/
  root->gcc_type = TREE_TYPE(gcctree);

  /*
   * All expression trees must have positions, so that we can test
   * whether they're in macros.
   */
  root->first_char = root->children->first_char;
}


/*
 * Given pointer to separator in primary.secondary, primary->secondary,
 * or primary[secondary]. 
 */
gct_build_ref(root, type, gcctree)
     gct_node root;
     enum gct_node_type type;
     tree gcctree;
{
  gct_node primary = root->prev;
  gct_node secondary = root->next;
  
  if (GCT_ARRAYREF == type)
    {
      gct_free_sugar(secondary->next, secondary);
    }
  
  move_below(root, primary);
  move_below(root, secondary);
  
  root->type = type;
  set_volatile(root, gcctree);
  root->gcc_type = TREE_TYPE(gcctree);
}  


/* postincrement and postdecrement */
gct_build_post(root, type, gcctree)
     gct_node root;
     enum gct_node_type type;
     tree gcctree;
{
  move_below(root, root->prev);
  root->type = type;
  set_volatile(root, gcctree);
  root->gcc_type = TREE_TYPE(gcctree);
}


/*
 * Build sizeof or alignof, where the argument is a typename:
 *
 * OPERATOR ( potentially many type tokens ) 
 */
gct_build_of(rparen, type, gcctree)
     gct_node rparen;
     enum gct_node_type type;
     tree gcctree;
{
  gct_node lparen = gct_find_earlier_match(rparen);
  gct_node operator = lparen->prev;
  gct_node typenode = gct_located_placeholder(operator);
  typenode->type = GCT_TYPECRUD;

  gct_cut_sublist(lparen, rparen);
  typenode->children = lparen;

  GCT_ADD(operator, typenode);
  operator->type = type;
  set_volatile(operator, gcctree);
  assert(!operator->is_volatile);
  operator->gcc_type = TREE_TYPE(gcctree);
}


/*
 * Each variable declaration is kept as a separate node, simply because
 * it's too hard later to find the end of the declarations.  
 *
 * The declaration is just a set of strung-together nodes, avoiding the
 * trouble of designing and building a parse tree that we'll never use.
 * However, simple-initializers (single values) are present as expression
 * nodes -- they can be instrumented.  Aggregate initializers are
 * ignored, since some C compilers cannot handle them.  (That's dumb.)
 *
 * The variable being declared is the only GCT_IDENTIFIER node in the
 * TYPCRUD list.  We need to be able to find it when instrumenting
 * initialization, since we must not use any variables declared later in
 * this block in that instrumentation.
 *
 * No gcctype is passed in because there isn't one for a declaration.
 */

/* Ignore declarations outside of functions, including parameters. */
int Gct_ignore_decls = 1;

gct_parse_decls()
{
  Gct_ignore_decls = 0;
}

gct_ignore_decls()
{
  Gct_ignore_decls = 1;
}


gct_build_decl(semi)
     gct_node semi;
{
  gct_node first_node;

  if (! Gct_ignore_decls)
    {
      gct_node declnode = gct_placeholder();
      
      declnode->type = GCT_DECLARATION;
      gct_add_after(&Gct_all_nodes, semi, declnode);
      
      /* Look for preceding semicolon or open brace. */
      first_node = gct_find_start_of_declaration(semi);
      gct_cut_sublist(first_node, semi);
      declnode->children = first_node;
      declnode->filename = first_node->filename;
      declnode->lineno = first_node->lineno;
      declnode->first_char = first_node->first_char;
    }
}


/* "{ 0-or-more items }" */
/* We retain the braces -- it's more convenient:  don't need to 
   worry about empty lists, whether there are annotations there.  */
gct_build_compound_stmt(closing_brace)
     gct_node closing_brace;
{
  gct_node opening_brace;
  gct_node compound = gct_placeholder();
  compound->type = GCT_COMPOUND_STMT;
  gct_add_after(&Gct_all_nodes, closing_brace, compound);

  for(opening_brace = closing_brace->prev;
      GCT_OTHER != opening_brace->type || '{' != opening_brace->text[0];
      opening_brace = opening_brace->prev)
    {
      /* Do nothing. */
    }
  gct_cut_sublist(opening_brace, closing_brace);
  compound->children = opening_brace;
  compound->filename = opening_brace->filename;
  compound->lineno = opening_brace->lineno;
  compound->first_char = opening_brace->first_char;
}

/*
 * This routine is called for the GNU C extension where compound
 * statements are allowed within expressions:  ({int a = 3; a * a;})+6;
 * is legal GNU C.  The compound statement has already been built.  All
 * we need to do is:
 * 1.  Strip the surrounding parens (as is consistently done throughout
 * GCT parsing).
 * 2.  Change the GCT-type to note this is a compound-expr.
 * 3.  Handle volatility and type consistently with expression trees.
 */
gct_build_compound_expr(compound, gcctree)
     gct_node compound;
     tree gcctree;
{
  compound->type = GCT_COMPOUND_EXPR;	/* Change type of node. It's not a statement. */

  set_volatile(compound, gcctree);
  compound->gcc_type = TREE_TYPE(gcctree);

  gct_flush_parens(compound->prev);
}



gct_build_simple_stmt(semi)
     gct_node semi;
{
  gct_node body = semi->prev;
  gct_node stmt = gct_located_placeholder(body);
  stmt->type = GCT_SIMPLE_STMT;
  gct_add_after(&Gct_all_nodes, semi, stmt);

  gct_free_sugar(semi, body);
  move_below(stmt, body);
}  


/*
 * For statements other than simple and compound, we have to worry about
 * the if / if-else ambiguity.  Resolving it may have required a shift,
 * meaning that the purported statement passed in may be the
 * disambiguating token, so we have to unshift it.
 */

#define expect_stmt(stmt)	\
  ((GCT_OTHER == (stmt)->type) ? (stmt)->prev : (stmt))


/* IF without else */
gct_build_simple_if(stmt)
     gct_node stmt;
{
  gct_node expr;
  gct_node if_node;

  stmt = expect_stmt(stmt);

  expr = stmt->prev->prev;
  if_node = expr->prev->prev;

  assert(if_node->text);
  assert(!strcmp(if_node->text, "if"));

  gct_flush_parens(if_node->next);
  move_below(if_node, expr);
  move_below(if_node, stmt);
  if_node->type = GCT_IF;
}

/* if ( expr ) stmt ELSE stmt */
gct_build_if_else(else_stmt)
     gct_node else_stmt;
{
  gct_node then_stmt, test, if_node;

  else_stmt = expect_stmt(else_stmt);

  then_stmt = else_stmt->prev->prev;
  test = then_stmt->prev->prev;
  if_node = test->prev->prev;

  assert(if_node->text);
  assert(!strcmp(if_node->text, "if"));
  assert(else_stmt->prev->text);
  assert(!strcmp(else_stmt->prev->text, "else"));

  gct_free_sugar(else_stmt->prev, else_stmt->prev->prev);
  gct_flush_parens(if_node->next);
  
  move_below(if_node, test);
  move_below(if_node, then_stmt);
  move_below(if_node, else_stmt);
  if_node->type = GCT_IF;
}


/* while ( expr ) stmt */

gct_build_while_stmt(stmt)
     gct_node stmt;
{
  gct_node expr;
  gct_node while_node;

  stmt = expect_stmt(stmt);
  
  expr = stmt->prev->prev;
  while_node = expr->prev->prev;

  assert(while_node->text);
  assert(!strcmp(while_node->text, "while"));

  gct_flush_parens(while_node->next);
  move_below(while_node, expr);
  move_below(while_node, stmt);
  while_node->type = GCT_WHILE;
}

/* do stmt while ( expr ) ; */
gct_build_do_stmt(semi)
     gct_node semi;
{
  gct_node expr = semi->prev->prev;
  gct_node stmt = expr->prev->prev->prev;
  gct_node do_node = stmt->prev;

  assert(do_node->text);
  assert(!strcmp(do_node->text, "do"));
  
  gct_free_sugar(semi, semi->prev);
  gct_flush_parens(expr->prev);
  gct_free_sugar(stmt->next, stmt);

  move_below(do_node, stmt);
  move_below(do_node, expr);
  do_node->type = GCT_DO;
}


/*
 * This routine is used to handle empty expressions in the control part
 * of a for loop.  It is given a pointer to a node.  That node may be
 * an expression, in which case it is returned.   If it is not an
 * expression, but rather a semicolon or paren, that is, an OTHER node, a
 * NULL_EXPR node is constructed, linked after the node, and returned.
 *
 * The routine is also suitable for "RETURN optional-expr".
 */
static gct_node
construct_null_expr_if_needed(possible)
     gct_node possible;
{
  
  if (GCT_OTHER == possible->type)
    {
      gct_node new_node = gct_located_placeholder(possible);
      new_node->type = GCT_NULL_EXPR;
      gct_add_after(&Gct_all_nodes, possible, new_node);
      /*
       * All expression trees must have positions, so that we can test
       * whether they're in macros.
       */
      new_node->first_char = possible->first_char;
      return new_node;
    }
  else
    {
      return possible;
    }
}

/* for ( opt-expr ; opt-expr; opt-expr ) stmt */
gct_build_for_stmt(stmt)
     gct_node stmt;
{
  gct_node for_node, init, test, incr;

  stmt = expect_stmt(stmt);
  
  gct_free_sugar(stmt->prev, stmt->prev->prev);

  			/* FOR ( OPT-EXPR ; OPT-EXPR ; OPT-EXPR STMT */

  incr = construct_null_expr_if_needed(stmt->prev);

  gct_free_sugar(incr->prev, incr->prev->prev);

			/* FOR ( OPT-EXPR; OPT-EXPR INCR STMT */

  test = construct_null_expr_if_needed(incr->prev);
  gct_free_sugar(test->prev, test->prev->prev);

			/* FOR ( OPT-EXPR TEST INCR STMT */

  init = construct_null_expr_if_needed(test->prev);
  gct_free_sugar(init->prev, init->prev->prev);

			/* FOR INIT TEST INCR STMT */
  
  for_node = init->prev;
  assert(for_node->text);
  assert(!strcmp(for_node->text, "for"));
  
  move_below(for_node, init);
  move_below(for_node, test);
  move_below(for_node, incr);
  move_below(for_node, stmt);
  for_node->type = GCT_FOR;
}


/* switch ( expr ) stmt */

gct_build_switch(stmt)
     gct_node stmt;
{
  gct_node expr;
  gct_node switch_node;
  
  stmt = expect_stmt(stmt);   /* Might be needed:  switch stmt need not be compound */

  expr = stmt->prev->prev;
  switch_node = expr->prev->prev;


  assert(switch_node->text);
  assert(!strcmp(switch_node->text, "switch"));

  gct_flush_parens(expr->prev);

  move_below(switch_node, expr);
  move_below(switch_node, stmt);
  switch_node->type = GCT_SWITCH;
}

/* case expr : stmt */
gct_build_case(stmt)
     gct_node stmt;
{
  gct_node expr;
  gct_node case_node;

  stmt = expect_stmt(stmt);
  
  expr = stmt->prev->prev;
  case_node = expr->prev;

  gct_free_sugar(expr->next, expr);
  move_below(case_node, expr);
  move_below(case_node, stmt);
  case_node->type = GCT_CASE;
}

/* default : stmt */
gct_build_default(stmt)
     gct_node stmt;
{
  gct_node default_node;
  
  stmt = expect_stmt(stmt);

  default_node = stmt->prev->prev;

  gct_free_sugar(default_node->next, default_node);
  move_below(default_node, stmt);
  default_node->type = GCT_DEFAULT;
}


/* break ; */
gct_build_break(semi)
     gct_node semi;
{
  gct_node break_node = semi->prev;

  gct_free_sugar(semi, break_node);
  break_node->type = GCT_BREAK;
}

/* continue ; */
gct_build_continue(semi)
     gct_node semi;
{
  gct_node continue_node = semi->prev;

  gct_free_sugar(semi, continue_node);
  continue_node->type = GCT_CONTINUE;
}

/* return opt-expr ; */
gct_build_return(semi)
     gct_node semi;
{
  gct_node expr, return_node;

  expr = construct_null_expr_if_needed(semi->prev);
  return_node = expr->prev;
  gct_free_sugar(semi, expr);	/* Safe even if expr is null node. */
  move_below(return_node, expr);
  return_node->type = GCT_RETURN;
}

/* ASM any kind of crud ; */
gct_build_asm(semi)
     gct_node semi;
{
  gct_node last_node = semi->prev;
  gct_node first_node;
  gct_node asm_node = gct_either_preceding_text(semi, "asm", "__asm__");

  if (GCT_NULL_NODE == asm_node)
    {
      fatal("asm production but no keyword.");
    }

  /* Remove the semicolon for consistency with other stmt types. */
  gct_free_sugar(semi, last_node);

  first_node = asm_node->next;
  gct_cut_sublist(first_node, last_node);
  asm_node->children = first_node;
  asm_node->type = GCT_ASM;
}

/* Goto identifier ; */
gct_build_goto(semi)
     gct_node semi;
{
  gct_node id = semi->prev;
  gct_node goto_node = id->prev;

  gct_free_sugar(semi, id);
  move_below(goto_node, id);
  goto_node->type = GCT_GOTO;
}

/* label : stmt */
gct_build_label(stmt)
     gct_node stmt;
{
  gct_node label;


  stmt = expect_stmt(stmt);
  
  label = stmt->prev->prev;

  gct_free_sugar(label->next, label);

  move_below(label, stmt);
  label->type = GCT_LABEL;
}

/* ; */
gct_build_null_stmt(semi)
     gct_node semi;
{
  gct_node expr = gct_located_placeholder(semi);
  expr->type = GCT_NULL_EXPR;
  GCT_ADD(semi, expr);
  
  semi->type = GCT_SIMPLE_STMT;
  /* The text hangs around, but that does no harm. */
}


/*
 * Argument points to the last (of two) strings in a list.  Note that
 * here, as elsewhere, strings are assumed to begin and end with ".
 * Wide-character strings are not currently allowed.  Strings with
 * embedded nulls are allowed.
 *
 * As far as GCT is concerned, the concatenated string is a single
 * string.  It is located wherever the first string in the series
 * started.
 *
 * A note on annotations.  Annotations are promoted from the second
 * string to the first.  Actually, because the end of a string is
 * well-defined, any line notes following the second string will not have
 * been read yet, hence there should be no annotations on the second
 * node.  It doesn't hurt to call promote_annotations, though.
 */
gct_combine_strings(second_string)
     gct_node second_string;
{
  gct_node first_string = second_string->prev;
  gct_node new_node = gct_alloc_node();
  

  assert(second_string->text);
  assert(first_string->text);
  
  assert(GCT_STRING_CONSTANT_P(second_string));
  assert(GCT_STRING_CONSTANT_P(first_string));

  assert('"' == second_string->text[second_string->textlen-1]);
  assert('"' == first_string->text[first_string->textlen-1]);


  /* The new node is a growing of the first_string, so it inherits
     the location and other fields of first_string. */
  *new_node = *first_string;

  /* New node now has annotations, and don't want two pointers to them. */
  first_string->note = GCT_NULL_ANNOTATION;

  /* The second_string is to be discarded, so promote annotations. */
  promote_annotations(second_string, new_node);	
  

  /* Allocate space, not counting two quotes we'll discard. */
  new_node->textlen = second_string->textlen + first_string->textlen - 2;
  new_node->text = (char *) xmalloc(new_node->textlen + 1);

  bcopy(first_string->text, new_node->text, first_string->textlen-1);
  bcopy(second_string->text+1, new_node->text+first_string->textlen-1,
	second_string->textlen-1);

  gct_add_before(&Gct_all_nodes, first_string, new_node);
  
  gct_remove_node(&Gct_all_nodes, first_string);
  gct_recursive_free_node(first_string);
  gct_remove_node(&Gct_all_nodes, second_string);
  gct_recursive_free_node(second_string);
  
}

  


		      /* CONSISTENCY CHECKING */

/*
 * This routine does consistency checking on the tree with root ROOT.  It
 * may be used before or after instrumentation, as indicated by the flag.
 * The following is checked:
 *
 * 1.  Before instrumentation, all nodes must have the filename, lineno,
 * and first_char arguments.  After instrumentation, a node that has any
 * must have all of them.
 * 2.  Tree structure (circular list of siblings) must be invariant.
 *
 * On failure, the broken tree is printed to stderr as a warning.  The
 * program continues.
 *
 * It would be nice to check that nodes retain the same order after
 * instrumentation, thus assuring us that annotations, especially
 * pragmas, won't get shuffled around.
 */

gct_build_consistency(root, instrumented)
	gct_node root;
	int instrumented;
{
    int problem = 0;

    if (!instrumented)
    {
	if (!root->filename || !root->lineno || !root->first_char)
	{
	    warning("Built tree has missing locations.");
	    problem++;
	}
    }
    else 
    {
	if (  (root->filename || root->lineno || root->first_char)
	    && !(root->filename && root->lineno && root->first_char))
	{
	    warning("Instrumented node has partial locations.");
	    problem++;
	}
    }

    if (problem)
    {
	gct_dump_tree(stderr, root, 0);
    }
    else if (root->children)
    {
	gct_node rover = root->children;
	do
	{
	    gct_build_consistency(rover, instrumented);
	    rover = rover->next;
	}
	while (rover != root->children);
    }
}
