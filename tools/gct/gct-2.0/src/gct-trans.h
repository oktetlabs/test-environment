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
 * This file contains common definitions for the collection of files that 
 * actually do instrumentation.
 */

/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-trans.h,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */
/* $Log: gct-trans.h,v $
 * Revision 1.1  2003/10/28 09:48:45  arybchik
 * Initial revision
 *
/* Revision 1.1  2003/08/18 16:38:03  pabolokh
/* new tree is being added and committed
/*
 * Revision 1.8  1992/08/30  16:51:27  marick
 * Cast enums to ints for HPUX.
 *
 * Revision 1.7  1992/07/28  14:29:32  marick
 * Miscellaneous tidying.
 *
 * Revision 1.6  1992/06/17  13:56:51  marick
 * Support compound statements within expressions (the GNU C ({...})
 * feature).  GCC's private include files sometimes use this (<varargs.h>
 * and <stdarg.h>, in particular).
 *
 * Revision 1.5  1992/05/21  01:31:23  marick
 * New externs declared.
 *
 * Revision 1.4  1992/05/13  21:34:43  marick
 * Changes supporting the move of standard instrumentation into gct-strans.c.
 *
 * Revision 1.3  1992/02/13  17:50:55  marick
 * Copylefted.
 *
 * Revision 1.2  91/09/09  13:11:53  marick
 * Race coverage.
 * 
 * Revision 1.1  91/09/03  12:47:28  marick
 * Header file that applies to both unit and system instrumentation.
 *  */



			/* TABLES (Part 1) */

/*
 * Instrumentation is via three funcall tables: Instrument,
 * Expr_instrument, and Lvalue_instrument.  The tables are instrumented
 * by gct_node.type.  They are used through three macros, DO_INSTRUMENT,
 * DO_EXPR_INSTRUMENT, DO_LVALUE_INSTRUMENT.
 *
 * The tables are defined at the bottom of gct-trans.c.  Two sets are used,
 * depending on the type of transformation:  weak or standard.
 */

extern void (**Instrument)();
extern int (**Expr_instrument)();
extern int (**Lvalue_instrument)();

#define DO_INSTRUMENT(parent, self)\
   ((Instrument[(int)self->type])((parent), (self)))

#define DO_EXPR_INSTRUMENT(parent, self, state,  valuenode, setter_rh, tests)\
   ((Expr_instrument[(int)self->type])((parent), (self), (state), (valuenode),\
				 (setter_rh), (tests)))

#define DO_LVALUE_INSTRUMENT(parent, self, state, originalvalue, setter_rh, tests, lvalue)\
   ((Lvalue_instrument[(int)self->type])((parent), (self), (state), \
				 (originalvalue),\
				 (setter_rh), (tests), (lvalue)))



		/* SIMPLE INSTRUMENTATION UTILITIES */


/* Free temporary storage if it was in fact allocated. */
#define free_temp(temp, unless_val)\
	if ((temp) != (unless_val)) free((temp));


/*
 * This remembers where the child is to go and unlinks it from the parent.
 * placeholder must be defined in the caller.  WARNING:  if a different
 * child of the parent is instrumented in place, placeholder may now be
 * remembering a now-defunct place to replace the node.  Don't let this
 * happen to you:  IT IS AN ERROR TO CALL DO_INSTRUMENT BETWEEN
 * REMEMBER_PLACE and REPLACE.
 */
#define remember_place(parent, child, placeholder)	\
{					\
  if ((parent)->children == (child))		\
    placeholder = GCT_NULL_NODE;		\
  else					\
    placeholder = (child)->prev;		\
  gct_remove_node(&((parent)->children), (child));\
}

/* This puts the newly-rewritten child back where it came from. */
#define replace(parent, child, placeholder)		\
{					\
  if (!placeholder)				\
    gct_add_first(&((parent)->children), (child));	\
  else					\
    gct_add_after(&((parent)->children), placeholder, (child));	\
}


		 /* THE INSTRUMENTATION FUNCTIONS */

/* Weak instrumentation */
extern void i_expr();
extern void i_nothing();
extern void i_label();
extern void i_declaration();
extern void i_compound_statement();
extern void i_simple_statement();
extern void i_if();
extern void i_while();
extern void i_do();
extern void i_for();
extern void i_switch();
extern void i_default();
extern void i_case();
extern void i_return();

extern int exp_binary();
extern int exp_boolean();
extern int exp_bitwise();
extern int exp_assign();
extern int exp_comma();
extern int exp_unary();
extern int exp_incdec();
extern int exp_cast();
extern int exp_compound_expr();
extern int exp_quest();
extern int exp_id();
extern int exp_constant();
extern int exp_arrayref();
extern int exp_deref();
extern int exp_dotref();
extern int exp_arrowref();
extern int exp_nothing();
extern int exp_funcall();
extern int exp_impossible();

extern int lv_impossible();
extern int lv_id();
extern int lv_arrayref();
extern int lv_deref();
extern int lv_dotref();
extern int lv_arrowref();


/* Standard instrumentation */
extern void i_std_descend();
extern void i_std_stop();
extern void i_std_routine();
extern void i_std_funcall();
/* extern void i_std_declaration();	gct_utrans.c version currently used. */
/* extern void i_std_compound_statement();	gct_utrans.c version currently used. */
extern void i_std_return();
extern void i_std_relational();
extern void i_std_boolean();
extern void i_std_assign();
extern void i_std_if();
extern void i_std_quest();
extern void i_std_while();
extern void i_std_do();
extern void i_std_for();
extern void i_std_switch();
extern void i_std_case();
extern void i_std_default();

/* Race utility routines */
extern gct_node race_entry_statement();
extern gct_node race_check_statement();
extern gct_node race_call_expression();
extern gct_node race_reenter_expression();
extern gct_node race_return_statement();

