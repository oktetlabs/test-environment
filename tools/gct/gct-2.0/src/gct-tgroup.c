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
#include "gct-const.h"
#include "c-decl.h"
#include "gct-tutil.h"
#include "gct-contro.h"
#include "gct-assert.h"

/*
 * These are routines that deal with groups of node types.
 */

/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-tgroup.c,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */

/*
 * The following is a list of standard roots that are used as templates
 * in various of the transformation routines.
 */

static struct gct_node_structure Int_root_struct;
gct_node Int_root = &Int_root_struct;

static struct gct_node_structure Less_root_struct;
gct_node Less_root = &Less_root_struct;

static struct gct_node_structure Lesseq_root_struct;
gct_node Lesseq_root = &Lesseq_root_struct;

static struct gct_node_structure Greater_root_struct;
gct_node Greater_root = &Greater_root_struct;

static struct gct_node_structure Greatereq_root_struct;
gct_node Greatereq_root = &Greatereq_root_struct;



/* The following tables are used for quick tests of group membership. */
/* "True" booleans are used in standard coverage; plain booleans are used
   in weak mutation coverage.  Historical, mainly. */

char Gct_relational[NUM_GCT_TREE_CODES];
char Gct_boolean[NUM_GCT_TREE_CODES];
char Gct_true_boolean[NUM_GCT_TREE_CODES];
char Gct_boolean_assign[NUM_GCT_TREE_CODES];

/* Operators, both of whose operands must be integer. */
char Gct_integer_only[NUM_GCT_TREE_CODES];

/* The kind of things we build abbreviated names for.  See make_mapname. */
char Gct_nameable[NUM_GCT_TREE_CODES];


void
gct_initialize_groups()
{
  Int_root->gcc_type = integer_type_node;
  Less_root->text = "<";
  Less_root->type = GCT_LESS;
  
  Lesseq_root->text = "<=";
  Lesseq_root->type = GCT_LESSEQ;
  
  Greater_root->text = ">";
  Greater_root->type = GCT_GREATER;
  
  Greatereq_root->text = ">=";
  Greatereq_root->type = GCT_GREATEREQ;

  Gct_relational[GCT_LESS]=1;
  Gct_relational[GCT_LESSEQ]=1;
  Gct_relational[GCT_GREATER]=1;
  Gct_relational[GCT_GREATEREQ]=1;
  Gct_relational[GCT_EQUALEQUAL]=1;
  Gct_relational[GCT_NOTEQUAL]=1;

  Gct_boolean[GCT_BITAND]=1;
  Gct_boolean[GCT_BITOR]=1;
  Gct_boolean[GCT_BITXOR]=1;
  Gct_boolean[GCT_ANDAND]=1;
  Gct_boolean[GCT_OROR]=1;
  Gct_boolean[GCT_BIT_NOT]=1;
  Gct_boolean[GCT_TRUTH_NOT]=1;

  Gct_true_boolean[GCT_ANDAND]=1;
  Gct_true_boolean[GCT_OROR]=1;
  Gct_true_boolean[GCT_TRUTH_NOT]=1;

  Gct_boolean_assign[GCT_BITAND_ASSIGN]=1;
  Gct_boolean_assign[GCT_BITOR_ASSIGN]=1;
  Gct_boolean_assign[GCT_BITXOR_ASSIGN]=1;

  Gct_integer_only[GCT_LSHIFT]=1;
  Gct_integer_only[GCT_RSHIFT]=1;
  Gct_integer_only[GCT_RSHIFT]=1;
  Gct_integer_only[GCT_MOD]=1;
  Gct_integer_only[GCT_BITOR]=1;
  Gct_integer_only[GCT_BITAND]=1;
  Gct_integer_only[GCT_BITXOR]=1;
  Gct_integer_only[GCT_LSHIFT_ASSIGN]=1;
  Gct_integer_only[GCT_RSHIFT_ASSIGN]=1;
  Gct_integer_only[GCT_RSHIFT_ASSIGN]=1;
  Gct_integer_only[GCT_MOD_ASSIGN]=1;
  Gct_integer_only[GCT_BITOR_ASSIGN]=1;
  Gct_integer_only[GCT_BITAND_ASSIGN]=1;
  Gct_integer_only[GCT_BITXOR_ASSIGN]=1;
  Gct_integer_only[GCT_BIT_NOT]=1;


  Gct_nameable[GCT_ADDR]=1;
  Gct_nameable[GCT_DEREFERENCE]=1;
  Gct_nameable[GCT_IDENTIFIER]=1;
  Gct_nameable[GCT_CONSTANT]=1;
  Gct_nameable[GCT_FUNCALL]=1;
  Gct_nameable[GCT_ARRAYREF]=1;
  Gct_nameable[GCT_DOTREF]=1;
  Gct_nameable[GCT_ARROWREF]=1;
  Gct_nameable[GCT_SIZEOF]=1;
  Gct_nameable[GCT_ALIGNOF]=1;
  Gct_nameable[GCT_CAST]=1;
  Gct_nameable[GCT_COMPOUND_EXPR]=1;
}


gct_node
gct_reverse_test(test)
     gct_node test;
{
  assert(Gct_relational[test->type]);

  switch(test->type)
    {
    case GCT_LESS:
      return Greater_root;
    case GCT_LESSEQ:
      return Greatereq_root;
    case GCT_GREATER:
      return Less_root;
    case GCT_GREATEREQ:
      return Lesseq_root;
    }
  return test;
}



