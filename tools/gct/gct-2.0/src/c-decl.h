/*
 * This file contains declarations, originally inside c-decl.c, which are
 * now used in both c-decl.c and gct-decl.c.
 */


/* Process declarations and variables for C compiler.
   Copyright (C) 1988 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */


/* Process declarations and symbol lookup for C front end.
   Also constructs types; the standard scalar types at initialization,
   and structure, union, array and enum types when they are declared.  */

/* ??? not all decl nodes are given the most useful possible
   line numbers.  For example, the CONST_DECLs for enum values.  */

#include "config.h"
#include "tree.h"
#include "flags.h"
#include "c-tree.h"
#include "c-lex.h"
#include <stdio.h>
#include "c-parse.h"
#include "gct-util.h"		/* GCT */

#define NULL 0
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

/* See c-decl.c for definitions of these variables. */

extern tree error_mark_node;

/* INTEGER_TYPE and REAL_TYPE nodes for the standard data types */

extern tree short_integer_type_node;
extern tree integer_type_node;
extern tree long_integer_type_node;
extern tree long_long_integer_type_node;

extern tree short_unsigned_type_node;
extern tree unsigned_type_node;
extern tree long_unsigned_type_node;
extern tree long_long_unsigned_type_node;

extern tree ptrdiff_type_node;

extern tree unsigned_char_type_node;
extern tree signed_char_type_node;
extern tree char_type_node;
extern tree wchar_type_node;
extern tree signed_wchar_type_node;
extern tree unsigned_wchar_type_node;

extern tree float_type_node;
extern tree double_type_node;
extern tree long_double_type_node;

extern tree complex_integer_type_node;
extern tree complex_float_type_node;
extern tree complex_double_type_node;
extern tree complex_long_double_type_node;

extern tree intQI_type_node;
extern tree intHI_type_node;
extern tree intSI_type_node;
extern tree intDI_type_node;

extern tree unsigned_intQI_type_node;
extern tree unsigned_intHI_type_node;
extern tree unsigned_intSI_type_node;
extern tree unsigned_intDI_type_node;

extern tree void_type_node;
extern tree ptr_type_node, const_ptr_type_node;
extern tree string_type_node, const_string_type_node;
extern tree char_array_type_node;
extern tree int_array_type_node;
extern tree wchar_array_type_node;
extern tree default_function_type;
extern tree double_ftype_double, double_ftype_double_double;
extern tree int_ftype_int, long_ftype_long;
extern tree void_ftype_ptr_ptr_int, int_ftype_ptr_ptr_int, void_ftype_ptr_int_int;
extern tree string_ftype_ptr_ptr, int_ftype_string_string;
extern tree int_ftype_cptr_cptr_sizet;
extern tree integer_zero_node;
extern tree null_pointer_node;
extern tree integer_one_node;
extern tree pending_invalid_xref;
extern char *pending_invalid_xref_file;
extern int pending_invalid_xref_line;
extern tree current_function_decl;
extern int current_function_returns_value;
extern int current_function_returns_null;


/* For each binding contour we allocate a binding_level structure
 * which records the names defined in that contour.
 * Contours include:
 *  0) the global one
 *  1) one for each function definition,
 *     where internal declarations of the parameters appear.
 *  2) one for each compound statement,
 *     to record its declarations.
 *
 * The current meaning of a name can be found by searching the levels from
 * the current one out to the global one.
 */

/* Note that the information in the `names' component of the global contour
   is duplicated in the IDENTIFIER_GLOBAL_VALUEs of all identifiers.  */

struct binding_level
  {
    /* A chain of _DECL nodes for all variables, constants, functions,
       and typedef types.  These are in the reverse of the order supplied.
     */
    tree names;

    /* A list of structure, union and enum definitions,
     * for looking up tag names.
     * It is a chain of TREE_LIST nodes, each of whose TREE_PURPOSE is a name,
     * or NULL_TREE; and whose TREE_VALUE is a RECORD_TYPE, UNION_TYPE,
     * or ENUMERAL_TYPE node.
     */
    tree tags;

    /* For each level, a list of shadowed outer-level local definitions
       to be restored when this level is popped.
       Each link is a TREE_LIST whose TREE_PURPOSE is an identifier and
       whose TREE_VALUE is its old definition (a kind of ..._DECL node).  */
    tree shadowed;

    /* For each level (except not the global one),
       a chain of BLOCK nodes for all the levels
       that were entered and exited one level down.  */
    tree blocks;

    /* The BLOCK node for this level, if one has been preallocated.
       If 0, the BLOCK is allocated (if needed) when the level is popped.  */
    tree this_block;

    /* The binding level which this one is contained in (inherits from).  */
    struct binding_level *level_chain;

    /* Nonzero for the level that holds the parameters of a function.  */
    /* 2 for a definition, 1 for a declaration.  */
    char parm_flag;

    /* Nonzero if this level "doesn't exist" for tags.  */
    char tag_transparent;

    /* Nonzero if sublevels of this level "don't exist" for tags.
       This is set in the parm level of a function definition
       while reading the function body, so that the outermost block
       of the function body will be tag-transparent.  */
    char subblocks_tag_transparent;

    /* Nonzero means make a BLOCK for this level regardless of all else.  */
    char keep;

    /* Nonzero means make a BLOCK if this level has any subblocks.  */
    char keep_if_subblocks;

    /* Number of decls in `names' that have incomplete 
       structure or union types.  */
    int n_incomplete;

    /* A list of decls giving the (reversed) specified order of parms,
       not including any forward-decls in the parmlist.
       This is so we can put the parms in proper order for assign_parms.  */
    tree parm_order;
  };

/* Definitions and comments in c-decl.c */  
extern struct binding_level *current_binding_level;
extern struct binding_level *global_binding_level;

