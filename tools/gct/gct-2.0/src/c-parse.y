/* YACC parser for C syntax and for Objective C.  -*-c-*-
   Copyright (C) 1987, 1988, 1989, 1992 Free Software Foundation, Inc.

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* This file defines the grammar of C and that of Objective C.
   ifobjc ... end ifobjc  conditionals contain code for Objective C only.
   ifc ... end ifc  conditionals contain code for C only.
   Sed commands in Makefile.in are used to convert this file into
   c-parse.y and into objc-parse.y.  */

/* To whomever it may concern: I have heard that such a thing was once
written by AT&T, but I have never seen it.  */

%expect 8

/* These are the 8 conflicts you should get in parse.output;
   the state numbers may vary if minor changes in the grammar are made.

State 41 contains 1 shift/reduce conflict.  (Two ways to recover from error.)
State 92 contains 1 shift/reduce conflict.  (Two ways to recover from error.)
State 99 contains 1 shift/reduce conflict.  (Two ways to recover from error.)
State 103 contains 1 shift/reduce conflict.  (Two ways to recover from error.)
State 119 contains 1 shift/reduce conflict.  (See comment at component_decl.)
State 183 contains 1 shift/reduce conflict.  (Two ways to recover from error.)
State 193 contains 1 shift/reduce conflict.  (Two ways to recover from error.)
State 199 contains 1 shift/reduce conflict.  (Two ways to recover from error.)
*/

%{
#include <stdio.h>
#include <errno.h>
#include <setjmp.h>

#include "config.h"
#include "tree.h"
#include "input.h"
#include "c-lex.h"
#include "c-tree.h"
#include "flags.h"

#ifdef MULTIBYTE_CHARS
#include <stdlib.h>
#include <locale.h>
#endif

#include <sys/types.h>          /* GCT */
#include <sys/stat.h>		/* GCT */
#include "gct-const.h"		/* GCT */
#include "gct-util.h"		/* GCT */
#include "gct-contro.h"		/* GCT */
#include <sys/file.h>		/* GCT */
#include "gct-tutil.h"		/* GCT */
#include "gct-assert.h"		/* GCT */

/* There are occasional "shift-reduce conflicts" in which the last node
   on the list may be an OTHER (unprocessed) node that stops the parse, 
   but it may also be a processed node.  An example of this is 
   "(unsigned) sizeof(int)".  The handling of such problems is 
   special-casey, quick-and-dirty, kludgy, and ought to be rethought. */
   
#define GCT_LAST_MAYBE_SHIFT(list)		\
    ((GCT_LAST(list)->type == GCT_OTHER)?	\
	    (GCT_LAST(list)->prev) :		\
	    (GCT_LAST(list)))

/*
 * Each function is hashed and that hash value is placed in the mapfile.
 * This hashval is calculated on all the tokens of a function.
 */
long Gct_function_hashval = 0;
typedef struct Gct_label {
    int type;
    int depth;
};

static int Gct_stmt_depth = 0;

static struct Gct_label Gct_label_buff[256];
static struct Gct_label *Gct_label_type = Gct_label_buff;
static int Gct_label_type_ptr = 0;
static int Gct_label_size = 256;

static void GCT_LABEL_PUSH();
static int GCT_LABEL_POP();

/* The code that does the hashing used to be here.  It had to be
   moved down below because AIX is confused. */


#ifdef USG			
#define R_OK 4			/* From gcc.c */
#define W_OK 2
#define X_OK 1
#define vfork fork
#endif /* USG */


/* Since parsers are distinct for each language, put the language string
   definition here.  */
char *language_string = "GNU C";

#ifndef errno
extern int errno;
#endif

void yyerror ();

/* Like YYERROR but do call yyerror.  */
#define YYERROR1 { yyerror ("syntax error"); YYERROR; }

/* Cause the `yydebug' variable to be defined.  */
#define YYDEBUG 1
%}

%start program

%union {long itype; tree ttype; enum tree_code code;
	char *filename; int lineno; }

/* All identifiers that are not reserved words
   and are not declared typedefs in the current block */
%token IDENTIFIER

/* All identifiers that are declared typedefs in the current block.
   In some contexts, they are treated just like IDENTIFIER,
   but they can also serve as typespecs in declarations.  */
%token TYPENAME

/* Reserved words that specify storage class.
   yylval contains an IDENTIFIER_NODE which indicates which one.  */
%token SCSPEC

/* Reserved words that specify type.
   yylval contains an IDENTIFIER_NODE which indicates which one.  */
%token TYPESPEC

/* Reserved words that qualify type: "const" or "volatile".
   yylval contains an IDENTIFIER_NODE which indicates which one.  */
%token TYPE_QUAL

/* Character or numeric constants.
   yylval is the node for the constant.  */
%token CONSTANT

/* String constants in raw form.
   yylval is a STRING_CST node.  */
%token STRING

/* "...", used for functions with variable arglists.  */
%token ELLIPSIS

/* the reserved words */
/* SCO include files test "ASM", so use something else. */
%token SIZEOF ENUM STRUCT UNION IF ELSE WHILE DO FOR SWITCH CASE DEFAULT
%token BREAK CONTINUE RETURN GOTO ASM_KEYWORD TYPEOF ALIGNOF ALIGN
%token ATTRIBUTE EXTENSION LABEL
%token REALPART IMAGPART

/* Add precedence rules to solve dangling else s/r conflict */
%nonassoc IF
%nonassoc ELSE

/* Define the operator tokens and their precedences.
   The value is an integer because, if used, it is the tree code
   to use in the expression made from the operator.  */

%right <code> ASSIGN '='
%right <code> '?' ':'
%left <code> OROR
%left <code> ANDAND
%left <code> '|'
%left <code> '^'
%left <code> '&'
%left <code> EQCOMPARE
%left <code> ARITHCOMPARE
%left <code> LSHIFT RSHIFT
%left <code> '+' '-'
%left <code> '*' '/' '%'
%right <code> UNARY PLUSPLUS MINUSMINUS
%left HYPERUNARY
%left <code> POINTSAT '.' '(' '['

/* The Objective-C keywords.  These are included in C and in
   Objective C, so that the token codes are the same in both.  */
%token INTERFACE IMPLEMENTATION END SELECTOR DEFS ENCODE
%token CLASSNAME PUBLIC PRIVATE PROTECTED PROTOCOL OBJECTNAME CLASS ALIAS

/* Objective-C string constants in raw form.
   yylval is an OBJC_STRING_CST node.  */
%token OBJC_STRING


%type <code> unop

%type <ttype> identifier IDENTIFIER TYPENAME CONSTANT expr nonnull_exprlist exprlist
%type <ttype> expr_no_commas cast_expr unary_expr primary string STRING
%type <ttype> typed_declspecs reserved_declspecs
%type <ttype> typed_typespecs reserved_typespecquals
%type <ttype> declmods typespec typespecqual_reserved
%type <ttype> SCSPEC TYPESPEC TYPE_QUAL nonempty_type_quals maybe_type_qual
%type <ttype> initdecls notype_initdecls initdcl notype_initdcl
%type <ttype> init initlist maybeasm
%type <ttype> asm_operands nonnull_asm_operands asm_operand asm_clobbers
%type <ttype> maybe_attribute attribute_list attrib

%type <ttype> compstmt

%type <ttype> declarator
%type <ttype> notype_declarator after_type_declarator
%type <ttype> parm_declarator

%type <ttype> structsp component_decl_list component_decl_list2
%type <ttype> component_decl components component_declarator
%type <ttype> enumlist enumerator
%type <ttype> typename absdcl absdcl1 type_quals
%type <ttype> xexpr parms parm identifiers

%type <ttype> parmlist parmlist_1 parmlist_2
%type <ttype> parmlist_or_identifiers parmlist_or_identifiers_1
%type <ttype> identifiers_or_typenames

%type <itype> setspecs

%type <filename> save_filename
%type <lineno> save_lineno


%{
/* Number of statements (loosely speaking) seen so far.  */
static int stmt_count;

/* Input file and line number of the end of the body of last simple_if;
   used by the stmt-rule immediately after simple_if returns.  */
static char *if_stmt_file;
static int if_stmt_line;

/* List of types and structure classes of the current declaration.  */
static tree current_declspecs;

/* Stack of saved values of current_declspecs.  */
static tree declspec_stack;

/* 1 if we explained undeclared var errors.  */
static int undeclared_variable_notice;


/* Tell yyparse how to print a token's value, if yydebug is set.  */

#define YYPRINT(FILE,YYCHAR,YYLVAL) yyprint(FILE,YYCHAR,YYLVAL)
extern void yyprint ();
%}

%%
program: /* empty */
		{ if (pedantic)
		    pedwarn ("ANSI C forbids an empty source file");
		}
	| extdefs
		{
		  /* In case there were missing closebraces,
		     get us back to the global binding level.  */
		  while (! global_bindings_p ())
		    poplevel (0, 0, 0);
		}
	;

/* the reason for the strange actions in this rule
 is so that notype_initdecls when reached via datadef
 can find a valid list of type and sc specs in $0. */

extdefs:
	{$<ttype>$ = NULL_TREE; } extdef
	| extdefs {$<ttype>$ = NULL_TREE; } extdef
	;

extdef:
	fndef
	| datadef
	| ASM_KEYWORD '(' expr ')' ';'
		{ STRIP_NOPS ($3);
		  if ((TREE_CODE ($3) == ADDR_EXPR
		       && TREE_CODE (TREE_OPERAND ($3, 0)) == STRING_CST)
		      || TREE_CODE ($3) == STRING_CST)
		    assemble_asm ($3);
		  else
		    error ("argument of `asm' is not a constant string"); }
	;

datadef:
	  setspecs notype_initdecls ';'
		{ if (pedantic)
		    error ("ANSI C forbids data definition with no type or storage class");
		  else if (!flag_traditional)
		    warning ("data definition has no type or storage class"); }
        | declmods setspecs notype_initdecls ';'
	  {}
	| typed_declspecs setspecs initdecls ';'
	  {}
        | declmods ';'
	  { pedwarn ("empty declaration"); }
	| typed_declspecs ';'
	  { shadow_tag ($1); }
	| error ';'
	| error '}'
	| ';'
		{ if (pedantic)
		    pedwarn ("ANSI C does not allow extra `;' outside of a function"); }
	;

fndef:
	  typed_declspecs setspecs declarator
		{ if (! start_function ($1, $3, 0))
		    YYERROR1;
		  Gct_function_hashval = 0;	/*** GCT ***/
		  reinit_parse_for_function (); }
	  xdecls
		{ store_parm_decls ();
		  gct_parse_decls();   }	/*** GCT ***/
	  compstmt_or_error
		{ /* debug_dump_tree(current_function_decl); */	/*** GCT ***/
		  gct_ignore_decls();				/*** GCT ***/
		  gct_transform_function(GCT_LAST(Gct_all_nodes)); /** GCT **/
		  finish_function (0); }
	| typed_declspecs setspecs declarator error
		{ }
	| declmods setspecs notype_declarator
		{ if (! start_function ($1, $3, 0))
		    YYERROR1;
		  Gct_function_hashval = 0;	/*** GCT ***/
		  reinit_parse_for_function (); }
	  xdecls
		{ store_parm_decls ();
		  gct_parse_decls();   }	/*** GCT ***/
	  compstmt_or_error
		{ /* debug_dump_tree(current_function_decl); */	/*** GCT ***/
		  gct_transform_function(GCT_LAST(Gct_all_nodes)); /** GCT **/
		  gct_ignore_decls();				/*** GCT ***/
		  finish_function (0); }
	| declmods setspecs notype_declarator error
		{ }
	| setspecs notype_declarator
		{ if (! start_function (NULL_TREE, $2, 0))
		    YYERROR1;
		  Gct_function_hashval = 0;	/*** GCT ***/
		  reinit_parse_for_function (); }
	  xdecls
		{ store_parm_decls ();
		  gct_parse_decls();  }	/*** GCT ***/
	  compstmt_or_error
		{ /* debug_dump_tree(current_function_decl); */	/*** GCT ***/
		  gct_transform_function(GCT_LAST(Gct_all_nodes)); /** GCT **/
		  gct_ignore_decls();				/*** GCT ***/
		  finish_function (0); }
	| setspecs notype_declarator error
		{ }
	;

identifier:
	IDENTIFIER
	| TYPENAME
	;

unop:     '&'
		{ $$ = ADDR_EXPR; }
	| '-'
		{ $$ = NEGATE_EXPR; }
	| '+'
		{ $$ = CONVERT_EXPR; }
	| PLUSPLUS
		{ $$ = PREINCREMENT_EXPR; }
	| MINUSMINUS
		{ $$ = PREDECREMENT_EXPR; }
	| '~'
		{ $$ = BIT_NOT_EXPR; }
	| '!'
		{ $$ = TRUTH_NOT_EXPR; }
	;

expr:	nonnull_exprlist
		{ if (GCT_COMMA == GCT_LAST(Gct_all_nodes)->prev->type)	/*** GCT ***/
		    { gct_guard_comma(GCT_LAST(Gct_all_nodes)->prev); }	/*** GCT ***/
		  $$ = build_compound_expr ($1); }
	;

exprlist:
	  /* empty */
		{ $$ = NULL_TREE; }
	| nonnull_exprlist
	;

nonnull_exprlist:
	expr_no_commas
		{ $$ = build_tree_list (NULL_TREE, $1); }
	| nonnull_exprlist ',' expr_no_commas
		{ chainon ($1, build_tree_list (NULL_TREE, $3));
		  gct_build_comma_list(GCT_LAST(Gct_all_nodes)->prev->prev,$1); }	/*** GCT ***/
	;

unary_expr:
	primary
	| '*' cast_expr   %prec UNARY
		{ $$ = build_indirect_ref ($2, "unary *");
		  gct_build_unary(GCT_LAST(Gct_all_nodes)->prev->prev, 
				  GCT_DEREFERENCE, $$); /*** GCT ***/
                }
	/* __extension__ turns off -pedantic for following primary.  */
	| EXTENSION
		{ $<itype>1 = pedantic;
		  pedantic = 0; }
	  cast_expr	  %prec UNARY
		{ $$ = $3;
		  pedantic = $<itype>1; 
                  gct_build_unary(GCT_LAST(Gct_all_nodes)->prev->prev, 
                                  GCT_EXTENSION,$$); /*** GCT ***/ }
	| unop cast_expr  %prec UNARY
		{ $$ = build_unary_op ($1, $2, 0);
		  gct_build_unary_by_gcctype(GCT_LAST_MAYBE_SHIFT(Gct_all_nodes)->prev, $1, $$); /*** GCT ***/
                  overflow_warning ($$); }
	/* Refer to the address of a label as a pointer.  */
	| ANDAND identifier
		{ tree label = lookup_label ($2);
		  if (label == 0)
		    $$ = null_pointer_node;
		  else
		    {
		      TREE_USED (label) = 1;
		      $$ = build1 (ADDR_EXPR, ptr_type_node, label);
		      TREE_CONSTANT ($$) = 1;
		    }
		}
/* This seems to be impossible on some machines, so let's turn it off.
   You can use __builtin_next_arg to find the anonymous stack args.
	| '&' ELLIPSIS
		{ tree types = TYPE_ARG_TYPES (TREE_TYPE (current_function_decl));
		  $$ = error_mark_node;
		  if (TREE_VALUE (tree_last (types)) == void_type_node)
		    error ("`&...' used in function with fixed number of arguments");
		  else
		    {
		      if (pedantic)
			pedwarn ("ANSI C forbids `&...'");
		      $$ = tree_last (DECL_ARGUMENTS (current_function_decl));
		      $$ = build_unary_op (ADDR_EXPR, $$, 0);
		    } }
*/
	| SIZEOF unary_expr  %prec UNARY
		{ if (TREE_CODE ($2) == COMPONENT_REF
		      && DECL_BIT_FIELD (TREE_OPERAND ($2, 1)))
		    error ("`sizeof' applied to a bit-field");
		  $$ = c_sizeof (TREE_TYPE ($2));
		  gct_build_unary(GCT_LAST(Gct_all_nodes)->prev->prev, 
				  GCT_SIZEOF, $$); /*** GCT ***/
                }
	| SIZEOF '(' typename ')'  %prec HYPERUNARY
		{ $$ = c_sizeof (groktypename ($3)); 
                  /*** GCT ***/
		  gct_build_of(GCT_LAST(Gct_all_nodes), GCT_SIZEOF, $$); 
		}
	| ALIGNOF unary_expr  %prec UNARY
		{ $$ = c_alignof_expr ($2); 
		  /*** GCT ***/
		  gct_build_unary(GCT_LAST(Gct_all_nodes)->prev->prev,
				  GCT_ALIGNOF, $$); 
                }
	| ALIGNOF '(' typename ')'  %prec HYPERUNARY
		{ $$ = c_alignof (groktypename ($3)); 
		  /*** GCT ***/
                  gct_build_of(GCT_LAST(Gct_all_nodes), GCT_ALIGNOF, $$);
                }
	| REALPART cast_expr %prec UNARY
		{ $$ = build_unary_op (REALPART_EXPR, $2, 0); 
		  error("GCT does not yet understand complex numbers.");}
	| IMAGPART cast_expr %prec UNARY
		{ $$ = build_unary_op (IMAGPART_EXPR, $2, 0); 
		   error("GCT does not yet understand complex numbers.");}
	;

cast_expr:
	unary_expr
	| '(' typename ')' cast_expr  %prec UNARY
		{ tree type = groktypename ($2);
		  $$ = build_c_cast (type, $4); 
		  /*** GCT ***/
                  gct_build_cast(GCT_LAST_MAYBE_SHIFT(Gct_all_nodes), $$); 
                }
	| '(' typename ')' '{' initlist maybecomma '}'  %prec UNARY
		{ tree type = groktypename ($2);
		/* GCT:  Don't mess with this yet.			 *** GCT ***
		  if (pedantic)
		    pedwarn ("ANSI C forbids constructor expressions");
		*/							/*** GCT ***/
		  error("GCT doesn't handle constructor expressions.");	/*** GCT ***/
		  YYERROR;						/*** GCT ***/
		  /*NOTREACHED*/					/*** GCT ***/
#if 0									/*** GCT ***/
		  char *name;
		  if (pedantic)
		    pedwarn ("ANSI C forbids constructor expressions");
		  if (TYPE_NAME (type) != 0)
		    {
		      if (TREE_CODE (TYPE_NAME (type)) == IDENTIFIER_NODE)
			name = IDENTIFIER_POINTER (TYPE_NAME (type));
		      else
			name = IDENTIFIER_POINTER (DECL_NAME (TYPE_NAME (type)));
		    }
		  else
		    name = "";
		  $$ = digest_init (type, build_nt (CONSTRUCTOR, NULL_TREE, nreverse ($5)),
				    NULL_PTR, 0, 0, name);
		  if (TREE_CODE (type) == ARRAY_TYPE && TYPE_SIZE (type) == 0)
		    {
		      int failure = complete_array_type (type, $$, 1);
		      if (failure)
			abort ();
		    }
#endif /*** GCT ***/
		}
	;

expr_no_commas:
	  cast_expr
	| expr_no_commas '+' expr_no_commas
		{ $$ = parser_build_binary_op ($2, $1, $3);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev, 
                  GCT_PLUS, $$); 
                }
	| expr_no_commas '-' expr_no_commas
		{ $$ = parser_build_binary_op ($2, $1, $3);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_MINUS, $$);
                }
	| expr_no_commas '*' expr_no_commas
		{ $$ = parser_build_binary_op ($2, $1, $3);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_TIMES, $$);
                }
	| expr_no_commas '/' expr_no_commas
		{ $$ = parser_build_binary_op ($2, $1, $3);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_DIV, $$);
                }
	| expr_no_commas '%' expr_no_commas
		{ $$ = parser_build_binary_op ($2, $1, $3);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_MOD, $$);
                }
	| expr_no_commas LSHIFT expr_no_commas
		{ $$ = parser_build_binary_op ($2, $1, $3);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_LSHIFT, $$);
                }
	| expr_no_commas RSHIFT expr_no_commas
		{ $$ = parser_build_binary_op ($2, $1, $3);
		/* GCT: There's a lookahead token on the tokenlist. */ 
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_RSHIFT, $$);
                }
	| expr_no_commas ARITHCOMPARE expr_no_commas
		{ $$ = parser_build_binary_op ($2, $1, $3);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_arithcompare(GCT_LAST(Gct_all_nodes)->prev->prev, 
					 $$);
                }
	| expr_no_commas EQCOMPARE expr_no_commas
		{ $$ = parser_build_binary_op ($2, $1, $3);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_eqcompare(GCT_LAST(Gct_all_nodes)->prev->prev, $$);
                }
	| expr_no_commas '&' expr_no_commas
		{ $$ = parser_build_binary_op ($2, $1, $3);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_BITAND, $$);
                }
	| expr_no_commas '|' expr_no_commas
		{ $$ = parser_build_binary_op ($2, $1, $3);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_BITOR, $$);
                }
	| expr_no_commas '^' expr_no_commas
		{ $$ = parser_build_binary_op ($2, $1, $3);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_BITXOR, $$);
                }
	| expr_no_commas ANDAND expr_no_commas
		{ $$ = parser_build_binary_op (TRUTH_ANDIF_EXPR, $1, $3);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_ANDAND, $$);
                }
	| expr_no_commas OROR expr_no_commas
		{ $$ = parser_build_binary_op (TRUTH_ORIF_EXPR, $1, $3);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_OROR, $$);
                }
	| expr_no_commas '?' xexpr ':' expr_no_commas
		{ $$ = build_conditional_expr ($1, $3, $5);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_quest(GCT_LAST(Gct_all_nodes)->prev->prev->prev->prev, $$);
                }
	| expr_no_commas '=' expr_no_commas
		{ $$ = build_modify_expr ($1, NOP_EXPR, $3);
		  C_SET_EXP_ORIGINAL_CODE ($$, MODIFY_EXPR);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_SIMPLE_ASSIGN, $$);
                }
	| expr_no_commas ASSIGN expr_no_commas
		{ $$ = build_modify_expr ($1, $2, $3);
		  /* This inhibits warnings in truthvalue_conversion.  */
		  C_SET_EXP_ORIGINAL_CODE ($$, ERROR_MARK); 
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_nonsimple_assign(GCT_LAST(Gct_all_nodes)->prev->prev, $$);
               }
	;

primary:
	IDENTIFIER
		{
		  tree context;

		  $$ = lastiddecl;
		  if (!$$ || $$ == error_mark_node)
		    {
		      if (yychar == YYEMPTY)
			yychar = YYLEX;
		      if (yychar == '(')
			{
			    {
			      /* Ordinary implicit function declaration.  */
			      $$ = implicitly_declare ($1);
			      assemble_external ($$);
			      TREE_USED ($$) = 1;
			    }
			}
		      else if (current_function_decl == 0)
			{
			  error ("`%s' undeclared here (not in a function)",
				 IDENTIFIER_POINTER ($1));
			  $$ = error_mark_node;
			}
		      else
			{
			    {
			      if (IDENTIFIER_GLOBAL_VALUE ($1) != error_mark_node
				  || IDENTIFIER_ERROR_LOCUS ($1) != current_function_decl)
				{
				  error ("`%s' undeclared (first use this function)",
					 IDENTIFIER_POINTER ($1));

				  if (! undeclared_variable_notice)
				    {
				      error ("(Each undeclared identifier is reported only once");
				      error ("for each function it appears in.)");
				      undeclared_variable_notice = 1;
				    }
				}
			      $$ = error_mark_node;
			      /* Prevent repeated error messages.  */
			      IDENTIFIER_GLOBAL_VALUE ($1) = error_mark_node;
			      IDENTIFIER_ERROR_LOCUS ($1) = current_function_decl;
			    }
			}
		    }
		  else if (TREE_TYPE ($$) == error_mark_node)
		    $$ = error_mark_node;
		  else if (C_DECL_ANTICIPATED ($$))
		    {
		      /* The first time we see a build-in function used,
			 if it has not been declared.  */
		      C_DECL_ANTICIPATED ($$) = 0;
		      if (yychar == YYEMPTY)
			yychar = YYLEX;
		      if (yychar == '(')
			{
			  /* Omit the implicit declaration we
			     would ordinarily do, so we don't lose
			     the actual built in type.
			     But print a diagnostic for the mismatch.  */
			    if (TREE_CODE ($$) != FUNCTION_DECL)
			      error ("`%s' implicitly declared as function",
				     IDENTIFIER_POINTER (DECL_NAME ($$)));
			  else if ((TYPE_MODE (TREE_TYPE (TREE_TYPE ($$)))
				    != TYPE_MODE (integer_type_node))
				   && (TREE_TYPE (TREE_TYPE ($$))
				       != void_type_node))
			    pedwarn ("type mismatch in implicit declaration for built-in function `%s'",
				     IDENTIFIER_POINTER (DECL_NAME ($$)));
			  /* If it really returns void, change that to int.  */
			  if (TREE_TYPE (TREE_TYPE ($$)) == void_type_node)
			    TREE_TYPE ($$)
			      = build_function_type (integer_type_node,
						     TYPE_ARG_TYPES (TREE_TYPE ($$)));
			}
		      else
			pedwarn ("built-in function `%s' used without declaration",
				 IDENTIFIER_POINTER (DECL_NAME ($$)));

		      /* Do what we would ordinarily do when a fn is used.  */
		      assemble_external ($$);
		      TREE_USED ($$) = 1;
		    }
		  else
		    {
		      assemble_external ($$);
		      TREE_USED ($$) = 1;
		    }

		  if (TREE_CODE ($$) == CONST_DECL)
		    {
		      $$ = DECL_INITIAL ($$);
		      /* This is to prevent an enum whose value is 0
			 from being considered a null pointer constant.  */
		      $$ = build1 (NOP_EXPR, TREE_TYPE ($$), $$);
		      TREE_CONSTANT ($$) = 1;
		    }
		  gct_build_item(GCT_LAST(Gct_all_nodes), GCT_IDENTIFIER, $$);
		}
	| CONSTANT
	/*** GCT ***/
		{ gct_build_item(GCT_LAST(Gct_all_nodes), GCT_CONSTANT, $$); }
	| string
		{ gct_build_item(GCT_LAST(Gct_all_nodes)->prev, GCT_CONSTANT, 
				 $$);	/*** GCT ***/
		  $$ = combine_strings ($1); }
	| '(' expr ')'
		{ char class = TREE_CODE_CLASS (TREE_CODE ($2));
		  if (class == 'e' || class == '1'
		      || class == '2' || class == '<')
		    C_SET_EXP_ORIGINAL_CODE ($2, ERROR_MARK);
		  gct_flush_parens(GCT_LAST(Gct_all_nodes)->prev->prev);	/*** GCT ***/
		  $$ = $2; }
	| '(' error ')'
		{ $$ = error_mark_node; }
	| '('
		{ if (current_function_decl == 0)
		    {
		      error ("braced-group within expression allowed only inside a function");
		      YYERROR;
		    }
		  /* We must force a BLOCK for this level
		     so that, if it is not expanded later,
		     there is a way to turn off the entire subtree of blocks
		     that are contained in it.  */
		  keep_next_level ();
		  push_iterator_stack ();
		  push_label_level ();
		  $<ttype>$ = expand_start_stmt_expr (); }
	  compstmt ')'
		{ tree rtl_exp;
		  if (pedantic)
		    pedwarn ("ANSI C forbids braced-groups within expressions");
		  pop_iterator_stack ();
		  pop_label_level ();
		  rtl_exp = expand_end_stmt_expr ($<ttype>2);
		  /* The statements have side effects, so the group does.  */
		  TREE_SIDE_EFFECTS (rtl_exp) = 1;

		  /* Make a BIND_EXPR for the BLOCK already made.  */
		  $$ = build (BIND_EXPR, TREE_TYPE (rtl_exp),
			      NULL_TREE, rtl_exp, $3);
	          /*** GCT ***/
		  gct_build_compound_expr(GCT_LAST(Gct_all_nodes)->prev, $$);
                  /* Remove the block from the tree at this point.
		     It gets put back at the proper place
		     when the BIND_EXPR is expanded.  */
		/*** GCT: delete_block ($3); ***/
		}
	| primary '(' exprlist ')'   %prec '.'
                {  /*** GCT ***/
                   gct_node primary, exprlist; 

                   $$ = build_function_call ($1, $3);
                   /*** GCT ***/
                   primary = $3 ? GCT_LAST(Gct_all_nodes)->prev->prev->prev :
                                  GCT_LAST(Gct_all_nodes)->prev->prev;
                   exprlist = $3 ? primary->next->next : GCT_NULL_NODE;
                   gct_build_function_call(primary, exprlist, $$);
                   /*** END GCT ***/
                }
	| primary '[' expr ']'   %prec '.'
		{ $$ = build_array_ref ($1, $3); 
                  gct_build_ref(GCT_LAST(Gct_all_nodes)->prev->prev, 
				GCT_ARRAYREF, $$);
                }
	| primary '.' identifier
		{
		    $$ = build_component_ref ($1, $3);
		    gct_build_ref(GCT_LAST(Gct_all_nodes)->prev,
				  GCT_DOTREF, $$);
		}
	| primary POINTSAT identifier
		{
                  tree expr = build_indirect_ref ($1, "->");
                  $$ = build_component_ref (expr, $3);
		  gct_build_ref(GCT_LAST(Gct_all_nodes)->prev, GCT_ARROWREF, 
				$$); 
		}
	| primary PLUSPLUS
		{ $$ = build_unary_op (POSTINCREMENT_EXPR, $1, 0); 
		  gct_build_post(GCT_LAST(Gct_all_nodes), GCT_POSTINCREMENT, 
				 $$); 
                }
	| primary MINUSMINUS
		{ $$ = build_unary_op (POSTDECREMENT_EXPR, $1, 0); 
		  gct_build_post(GCT_LAST(Gct_all_nodes), GCT_POSTDECREMENT, 
				 $$); 
                }
	;

/* Produces a STRING_CST with perhaps more STRING_CSTs chained onto it.  */
string:
	  STRING
	| string STRING
		{ $$ = chainon ($1, $2); 
		  /* GCT: Concatenate the strings.  Done here, instead of
		     in expr action, so that I don't have to worry
		     about shift-reduce conflicts - whether the last
		     element on the node list is really a string. */
		  gct_combine_strings(GCT_LAST(Gct_all_nodes)); 
                }
	;


xdecls:
	/* empty */
	| datadecls
	| datadecls ELLIPSIS
		/* ... is used here to indicate a varargs function.  */
		{ c_mark_varargs ();
		  if (pedantic)
		    pedwarn ("ANSI C does not permit use of `varargs.h'"); }
	;

/* The following are analogous to lineno_decl, decls and decl
   except that they do not allow nested functions.
   They are used for old-style parm decls.  */
lineno_datadecl:
	  save_filename save_lineno datadecl
		{ }
	;

datadecls:
	lineno_datadecl
	| errstmt
	| datadecls lineno_datadecl
	| lineno_datadecl errstmt
	;

datadecl:
	typed_declspecs setspecs initdecls ';'
		{ current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary ($2);
	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                }
	| declmods setspecs notype_initdecls ';'
		{ current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary ($2);
	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                }
	| typed_declspecs ';'
		{ shadow_tag_warned ($1, 1);
		  pedwarn ("empty declaration");
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                }
	| declmods ';'
		{ pedwarn ("empty declaration"); 
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                }
	;

/* This combination which saves a lineno before a decl
   is the normal thing to use, rather than decl itself.
   This is to avoid shift/reduce conflicts in contexts
   where statement labels are allowed.  */
lineno_decl:
	  save_filename save_lineno decl
		{ }
	;

decls:
	lineno_decl
	| errstmt
	| decls lineno_decl
	| lineno_decl errstmt
	;

/* records the type and storage class specs to use for processing
   the declarators that follow.
   Maintains a stack of outer-level values of current_declspecs,
   for the sake of parm declarations nested in function declarators.  */
setspecs: /* empty */
		{ $$ = suspend_momentary ();
		  pending_xref_error ();
		  declspec_stack = tree_cons (NULL_TREE, current_declspecs,
					      declspec_stack);
		  current_declspecs = $<ttype>0; }
	;

decl:
	typed_declspecs setspecs initdecls ';'
		{ current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary ($2);
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                 }
	| declmods setspecs notype_initdecls ';'
		{ current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary ($2); 
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                }
	| typed_declspecs setspecs nested_function
		{ current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary ($2); 
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                }
	| declmods setspecs notype_nested_function
		{ current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary ($2); 
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                }
	| typed_declspecs ';'
		{ shadow_tag ($1); 
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                }
	| declmods ';'
		{ pedwarn ("empty declaration"); 
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                }
	;

/* Declspecs which contain at least one type specifier or typedef name.
   (Just `const' or `volatile' is not enough.)
   A typedef'd name following these is taken as a name to be declared.  */

typed_declspecs:
	  typespec reserved_declspecs
		{ $$ = tree_cons (NULL_TREE, $1, $2); }
	| declmods typespec reserved_declspecs
		{ $$ = chainon ($3, tree_cons (NULL_TREE, $2, $1)); }
	;

reserved_declspecs:  /* empty */
		{ $$ = NULL_TREE; }
	| reserved_declspecs typespecqual_reserved
		{ $$ = tree_cons (NULL_TREE, $2, $1); }
	| reserved_declspecs SCSPEC
		{ if (extra_warnings)
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER ($2));
		  $$ = tree_cons (NULL_TREE, $2, $1); }
	;

/* List of just storage classes and type modifiers.
   A declaration can start with just this, but then it cannot be used
   to redeclare a typedef-name.  */

declmods:
	  TYPE_QUAL
		{ $$ = tree_cons (NULL_TREE, $1, NULL_TREE);
		  TREE_STATIC ($$) = 1; }
	| SCSPEC
		{ $$ = tree_cons (NULL_TREE, $1, NULL_TREE); }
	| declmods TYPE_QUAL
		{ $$ = tree_cons (NULL_TREE, $2, $1);
		  TREE_STATIC ($$) = 1; }
	| declmods SCSPEC
		{ if (extra_warnings && TREE_STATIC ($1))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER ($2));
		  $$ = tree_cons (NULL_TREE, $2, $1);
		  TREE_STATIC ($$) = TREE_STATIC ($1); }
	;


/* Used instead of declspecs where storage classes are not allowed
   (that is, for typenames and structure components).
   Don't accept a typedef-name if anything but a modifier precedes it.  */

typed_typespecs:
	  typespec reserved_typespecquals
		{ $$ = tree_cons (NULL_TREE, $1, $2); }
	| nonempty_type_quals typespec reserved_typespecquals
		{ $$ = chainon ($3, tree_cons (NULL_TREE, $2, $1)); }
	;

reserved_typespecquals:  /* empty */
		{ $$ = NULL_TREE; }
	| reserved_typespecquals typespecqual_reserved
		{ $$ = tree_cons (NULL_TREE, $2, $1); }
	;

/* A typespec (but not a type qualifier).
   Once we have seen one of these in a declaration,
   if a typedef name appears then it is being redeclared.  */

typespec: TYPESPEC
	| structsp
	| TYPENAME
		{ /* For a typedef name, record the meaning, not the name.
		     In case of `foo foo, bar;'.  */
		  $$ = lookup_name ($1); }
	| TYPEOF '(' expr ')'
		{ $$ = TREE_TYPE ($3); }
	| TYPEOF '(' typename ')'
		{ $$ = groktypename ($3); }
	;

/* A typespec that is a reserved word, or a type qualifier.  */

typespecqual_reserved: TYPESPEC
	| TYPE_QUAL
	| structsp
	;

initdecls:
	initdcl
	| initdecls ',' initdcl
	;

notype_initdecls:
	notype_initdcl
	| notype_initdecls ',' initdcl
	;

maybeasm:
	  /* empty */
		{ $$ = NULL_TREE; }
	| ASM_KEYWORD '(' string ')'
		{ if (TREE_CHAIN ($3)) $3 = combine_strings ($3);
		  $$ = $3;
		}
	;

initdcl:
	  declarator maybeasm maybe_attribute '='
		{ $<ttype>$ = start_decl ($1, current_declspecs, 1); }
	  init
/* Note how the declaration of the variable is in effect while its init is parsed! */
		{ decl_attributes ($<ttype>5, $3);
		  finish_decl ($<ttype>5, $6, $2); }
	| declarator maybeasm maybe_attribute
		{ tree d = start_decl ($1, current_declspecs, 0);
		  decl_attributes (d, $3);
		  finish_decl (d, NULL_TREE, $2); }
	;

notype_initdcl:
	  notype_declarator maybeasm maybe_attribute '='
		{ $<ttype>$ = start_decl ($1, current_declspecs, 1); }
	  init
/* Note how the declaration of the variable is in effect while its init is parsed! */
		{ decl_attributes ($<ttype>5, $3);
		  finish_decl ($<ttype>5, $6, $2); }
	| notype_declarator maybeasm maybe_attribute
		{ tree d = start_decl ($1, current_declspecs, 0);
		  decl_attributes (d, $3);
		  finish_decl (d, NULL_TREE, $2); }
	;
/* the * rules are dummies to accept the Apollo extended syntax
   so that the header files compile. */
maybe_attribute:
    /* empty */
		{ $$ = NULL_TREE; }
    | ATTRIBUTE '(' '(' attribute_list ')' ')'
		{ $$ = $4; }
    ;

attribute_list
    : attrib
	{ $$ = tree_cons (NULL_TREE, $1, NULL_TREE); }
    | attribute_list ',' attrib
	{ $$ = tree_cons (NULL_TREE, $3, $1); }
    ;

attrib
    : IDENTIFIER
	{ if (strcmp (IDENTIFIER_POINTER ($1), "packed"))
	    warning ("`%s' attribute directive ignored",
		     IDENTIFIER_POINTER ($1));
	  $$ = $1; }
    | IDENTIFIER '(' IDENTIFIER ')'
	{ /* If not "mode (m)", then issue warning.  */
	  if (strcmp (IDENTIFIER_POINTER ($1), "mode") != 0)
	    {
	      warning ("`%s' attribute directive ignored",
		       IDENTIFIER_POINTER ($1));
	      $$ = $1;
	    }
	  else
	    $$ = tree_cons ($1, $3, NULL_TREE); }
    | IDENTIFIER '(' CONSTANT ')'
	{ /* if not "aligned(n)", then issue warning */
	  if (strcmp (IDENTIFIER_POINTER ($1), "aligned") != 0
	      || TREE_CODE ($3) != INTEGER_CST)
	    {
	      warning ("`%s' attribute directive ignored",
		       IDENTIFIER_POINTER ($1));
	      $$ = $1;
	    }
	  else
	    $$ = tree_cons ($1, $3, NULL_TREE); }
    | IDENTIFIER '(' IDENTIFIER ',' CONSTANT ',' CONSTANT ')'
	{ /* if not "format(...)", then issue warning */
	  if (strcmp (IDENTIFIER_POINTER ($1), "format") != 0
	      || TREE_CODE ($5) != INTEGER_CST
	      || TREE_CODE ($7) != INTEGER_CST)
	    {
	      warning ("`%s' attribute directive ignored",
		       IDENTIFIER_POINTER ($1));
	      $$ = $1;
	    }
	  else
	    $$ = tree_cons ($1,
			    tree_cons ($3,
				       tree_cons ($5, $7, NULL_TREE),
				       NULL_TREE),
			    NULL_TREE); }
    ;

init:
	expr_no_commas
	| '{' '}'
		{ $$ = build_nt (CONSTRUCTOR, NULL_TREE, NULL_TREE);
		  if (pedantic)
		    pedwarn ("ANSI C forbids empty initializer braces"); }
	| '{' initlist '}'
		{ $$ = build_nt (CONSTRUCTOR, NULL_TREE, nreverse ($2)); }
	| '{' initlist ',' '}'
		{ $$ = build_nt (CONSTRUCTOR, NULL_TREE, nreverse ($2)); }
	| error
		{ $$ = NULL_TREE; }
	;

/* This chain is built in reverse order,
   and put in forward order where initlist is used.  */
initlist:
	  init
		{ $$ = build_tree_list (NULL_TREE, $1); }
	| initlist ',' init
		{ $$ = tree_cons (NULL_TREE, $3, $1); }
	/* These are for labeled elements.  The syntax for an array element
	   initializer conflicts with the syntax for an Objective-C message,
	   so don't include these productions in the Objective-C grammer.  */
	| '[' expr_no_commas ELLIPSIS expr_no_commas ']' init
		{ $$ = build_tree_list (tree_cons ($2, NULL_TREE,
						   build_tree_list ($4, NULL_TREE)),
					$6); }
	| initlist ',' '[' expr_no_commas ELLIPSIS expr_no_commas ']' init
		{ $$ = tree_cons (tree_cons ($4, NULL_TREE,
					     build_tree_list ($6, NULL_TREE)),
				  $8,
				  $1); }
	| '[' expr_no_commas ']' init
		{ $$ = build_tree_list ($2, $4); }
	| initlist ',' '[' expr_no_commas ']' init
		{ $$ = tree_cons ($4, $6, $1); }
	| identifier ':' init
		{ $$ = build_tree_list ($1, $3); }
	| initlist ',' identifier ':' init
		{ $$ = tree_cons ($3, $5, $1); }
	;

nested_function:
	  declarator
		{ push_c_function_context ();
		  if (! start_function (current_declspecs, $1, 1))
		    {
		      pop_c_function_context ();
		      YYERROR1;
		    }
		  reinit_parse_for_function ();
		  store_parm_decls (); }
/* This used to use compstmt_or_error.
   That caused a bug with input `f(g) int g {}',
   where the use of YYERROR1 above caused an error
   which then was handled by compstmt_or_error.
   There followed a repeated execution of that same rule,
   which called YYERROR1 again, and so on.  */
	  compstmt
		{ finish_function (1);
		  pop_c_function_context (); }
	;

notype_nested_function:
	  notype_declarator
		{ push_c_function_context ();
		  if (! start_function (current_declspecs, $1, 1))
		    {
		      pop_c_function_context ();
		      YYERROR1;
		    }
		  reinit_parse_for_function ();
		  store_parm_decls (); }
/* This used to use compstmt_or_error.
   That caused a bug with input `f(g) int g {}',
   where the use of YYERROR1 above caused an error
   which then was handled by compstmt_or_error.
   There followed a repeated execution of that same rule,
   which called YYERROR1 again, and so on.  */
	  compstmt
		{ finish_function (1);
		  pop_c_function_context (); }
	;

/* Any kind of declarator (thus, all declarators allowed
   after an explicit typespec).  */

declarator:
	  after_type_declarator
	| notype_declarator
	;

/* A declarator that is allowed only after an explicit typespec.  */

after_type_declarator:
	  '(' after_type_declarator ')'
		{ $$ = $2; }
	| after_type_declarator '(' parmlist_or_identifiers  %prec '.'
		{ $$ = build_nt (CALL_EXPR, $1, $3, NULL_TREE); }
/*	| after_type_declarator '(' error ')'  %prec '.'
		{ $$ = build_nt (CALL_EXPR, $1, NULL_TREE, NULL_TREE);
		  poplevel (0, 0, 0); }  */
	| after_type_declarator '[' expr ']'  %prec '.'
		{ $$ = build_nt (ARRAY_REF, $1, $3); }
	| after_type_declarator '[' ']'  %prec '.'
		{ $$ = build_nt (ARRAY_REF, $1, NULL_TREE); }
	| '*' type_quals after_type_declarator  %prec UNARY
		{ $$ = make_pointer_declarator ($2, $3); }
	| TYPENAME
		{ GCT_LAST(Gct_all_nodes)->type = GCT_IDENTIFIER; }
	;

/* Kinds of declarator that can appear in a parameter list
   in addition to notype_declarator.  This is like after_type_declarator
   but does not allow a typedef name in parentheses as an identifier
   (because it would conflict with a function with that typedef as arg).  */

parm_declarator:
	  parm_declarator '(' parmlist_or_identifiers  %prec '.'
		{ $$ = build_nt (CALL_EXPR, $1, $3, NULL_TREE); }
/*	| parm_declarator '(' error ')'  %prec '.'
		{ $$ = build_nt (CALL_EXPR, $1, NULL_TREE, NULL_TREE);
		  poplevel (0, 0, 0); }  */
	| parm_declarator '[' expr ']'  %prec '.'
		{ $$ = build_nt (ARRAY_REF, $1, $3); }
	| parm_declarator '[' ']'  %prec '.'
		{ $$ = build_nt (ARRAY_REF, $1, NULL_TREE); }
	| '*' type_quals parm_declarator  %prec UNARY
		{ $$ = make_pointer_declarator ($2, $3); }
	| TYPENAME
	;

/* A declarator allowed whether or not there has been
   an explicit typespec.  These cannot redeclare a typedef-name.  */

notype_declarator:
	  notype_declarator '(' parmlist_or_identifiers  %prec '.'
		{ $$ = build_nt (CALL_EXPR, $1, $3, NULL_TREE); }
/*	| notype_declarator '(' error ')'  %prec '.'
		{ $$ = build_nt (CALL_EXPR, $1, NULL_TREE, NULL_TREE);
		  poplevel (0, 0, 0); }  */
	| '(' notype_declarator ')'
		{ $$ = $2; }
	| '*' type_quals notype_declarator  %prec UNARY
		{ $$ = make_pointer_declarator ($2, $3); }
	| notype_declarator '[' expr ']'  %prec '.'
		{ $$ = build_nt (ARRAY_REF, $1, $3); }
	| notype_declarator '[' ']'  %prec '.'
		{ $$ = build_nt (ARRAY_REF, $1, NULL_TREE); }
	| IDENTIFIER
		{ GCT_LAST(Gct_all_nodes)->type = GCT_IDENTIFIER; }
	;

structsp:
	  STRUCT identifier '{'
		{ $$ = start_struct (RECORD_TYPE, $2);
		  /* Start scope of tag before parsing components.  */
		}
	  component_decl_list '}'
		{ $$ = finish_struct ($<ttype>4, $5);
		  /* Really define the structure.  */
		}
/*
  In GCT, all structs must be named.  We fake this by adding a name.
	| STRUCT '{' component_decl_list '}'
		{ $$ = finish_struct (start_struct (RECORD_TYPE, NULL_TREE),
				      $3); }
*/
	|  STRUCT '{'
		{
		  gct_node dummy = gct_tempnode("_GCT_DUMMY_");
                  gct_add_before(&Gct_all_nodes,
                                 GCT_LAST(Gct_all_nodes), dummy);
                  $$ = start_struct (RECORD_TYPE, get_identifier(dummy->text));
		  /* Start scope of tag before parsing components.  */
		}
	  component_decl_list '}'
		{ $$ = finish_struct ($<ttype>3, $4);
		  /* Really define the structure.  */
		}
	| STRUCT identifier
		{ $$ = xref_tag (RECORD_TYPE, $2); }
	| UNION identifier '{'
		{ $$ = start_struct (UNION_TYPE, $2); }
	  component_decl_list '}'
		{ $$ = finish_struct ($<ttype>4, $5); }
/*
  GCT:  Unions must be named, so we name them ourselves.
	| UNION '{' component_decl_list '}'
		{ $$ = finish_struct (start_struct (UNION_TYPE, NULL_TREE),
				      $3); }
*/
	| UNION  '{'
		{
		  gct_node dummy = gct_tempnode("_GCT_DUMMY_");
                  gct_add_before(&Gct_all_nodes,
                                 GCT_LAST(Gct_all_nodes), dummy);
                  $$ = start_struct (UNION_TYPE, get_identifier(dummy->text));
		  /* Start scope of tag before parsing components.  */
		}
	  component_decl_list '}'
		{ $$ = finish_struct ($<ttype>3, $4); }
	| UNION identifier
		{ $$ = xref_tag (UNION_TYPE, $2); }
	| ENUM identifier '{'
		{ $<itype>3 = suspend_momentary ();
		  $$ = start_enum ($2); }
	  enumlist maybecomma_warn '}'
		{ $$ = finish_enum ($<ttype>4, nreverse ($5));
		  resume_momentary ($<itype>3); }
/*
  We fudge enum names, just like structs and unions. 
	| ENUM '{'
		{ $<itype>2 = suspend_momentary ();
		  $$ = start_enum (NULL_TREE); }
*/
	| ENUM '{'
		{ 
		  gct_node dummy = gct_tempnode("_GCT_DUMMY_");
		  $<itype>2 = suspend_momentary ();
		  
                  gct_add_before(&Gct_all_nodes,
                                 GCT_LAST(Gct_all_nodes), dummy);
		  $$ = start_enum (get_identifier(dummy->text));
		}
	  enumlist maybecomma_warn '}'
		{ $$ = finish_enum ($<ttype>3, nreverse ($4));
		  resume_momentary ($<itype>2); }
	| ENUM identifier
		{ $$ = xref_tag (ENUMERAL_TYPE, $2); }
	;

maybecomma:
	  /* empty */
	| ','
	;

maybecomma_warn:
	  /* empty */
	| ','
		{ if (pedantic) pedwarn ("comma at end of enumerator list"); }
	;

component_decl_list:
	  component_decl_list2
		{ $$ = $1; }
	| component_decl_list2 component_decl
		{ $$ = chainon ($1, $2);
		  pedwarn ("no semicolon at end of struct or union"); }
	;

component_decl_list2:	/* empty */
		{ $$ = NULL_TREE; }
	| component_decl_list2 component_decl ';'
		{ $$ = chainon ($1, $2); }
	| component_decl_list2 ';'
		{ if (pedantic)
		    pedwarn ("extra semicolon in struct or union specified"); }
	;

/* There is a shift-reduce conflict here, because `components' may
   start with a `typename'.  It happens that shifting (the default resolution)
   does the right thing, because it treats the `typename' as part of
   a `typed_typespecs'.

   It is possible that this same technique would allow the distinction
   between `notype_initdecls' and `initdecls' to be eliminated.
   But I am being cautious and not trying it.  */

component_decl:
	  typed_typespecs setspecs components
		{ $$ = $3;
		  current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary ($2); }
	| typed_typespecs
		{ if (pedantic)
		    pedwarn ("ANSI C forbids member declarations with no members");
		  shadow_tag($1);
		  $$ = NULL_TREE; }
	| nonempty_type_quals setspecs components
		{ $$ = $3;
		  current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary ($2); }
	| nonempty_type_quals
		{ if (pedantic)
		    pedwarn ("ANSI C forbids member declarations with no members");
		  shadow_tag($1);
		  $$ = NULL_TREE; }
	| error
		{ $$ = NULL_TREE; }
	;

components:
	  component_declarator
	| components ',' component_declarator
		{ $$ = chainon ($1, $3); }
	;

component_declarator:
	  save_filename save_lineno declarator maybe_attribute
		{ $$ = grokfield ($1, $2, $3, current_declspecs, NULL_TREE);
		  decl_attributes ($$, $4); }
	| save_filename save_lineno
	  declarator ':' expr_no_commas maybe_attribute
		{ $$ = grokfield ($1, $2, $3, current_declspecs, $5);
		  decl_attributes ($$, $6); }
	| save_filename save_lineno ':' expr_no_commas maybe_attribute
		{ $$ = grokfield ($1, $2, NULL_TREE, current_declspecs, $4);
		  decl_attributes ($$, $5); }
	;

/* We chain the enumerators in reverse order.
   They are put in forward order where enumlist is used.
   (The order used to be significant, but no longer is so.
   However, we still maintain the order, just to be clean.)  */

enumlist:
	  enumerator
	| enumlist ',' enumerator
		{ $$ = chainon ($3, $1); }
	;


enumerator:
	  identifier
		{ $$ = build_enumerator ($1, NULL_TREE); }
	| identifier '=' expr_no_commas
		{ $$ = build_enumerator ($1, $3); }
	;

typename:
	typed_typespecs absdcl
		{ $$ = build_tree_list ($1, $2); }
	| nonempty_type_quals absdcl
		{ $$ = build_tree_list ($1, $2); }
	;

absdcl:   /* an absolute declarator */
	/* empty */
		{ $$ = NULL_TREE; }
	| absdcl1
	;

nonempty_type_quals:
	  TYPE_QUAL
		{ $$ = tree_cons (NULL_TREE, $1, NULL_TREE); }
	| nonempty_type_quals TYPE_QUAL
		{ $$ = tree_cons (NULL_TREE, $2, $1); }
	;

type_quals:
	  /* empty */
		{ $$ = NULL_TREE; }
	| type_quals TYPE_QUAL
		{ $$ = tree_cons (NULL_TREE, $2, $1); }
	;

absdcl1:  /* a nonempty absolute declarator */
	  '(' absdcl1 ')'
		{ $$ = $2; }
	  /* `(typedef)1' is `int'.  */
	| '*' type_quals absdcl1  %prec UNARY
		{ $$ = make_pointer_declarator ($2, $3); }
	| '*' type_quals  %prec UNARY
		{ $$ = make_pointer_declarator ($2, NULL_TREE); }
	| absdcl1 '(' parmlist  %prec '.'
		{ $$ = build_nt (CALL_EXPR, $1, $3, NULL_TREE); }
	| absdcl1 '[' expr ']'  %prec '.'
		{ $$ = build_nt (ARRAY_REF, $1, $3); }
	| absdcl1 '[' ']'  %prec '.'
		{ $$ = build_nt (ARRAY_REF, $1, NULL_TREE); }
	| '(' parmlist  %prec '.'
		{ $$ = build_nt (CALL_EXPR, NULL_TREE, $2, NULL_TREE); }
	| '[' expr ']'  %prec '.'
		{ $$ = build_nt (ARRAY_REF, NULL_TREE, $2); }
	| '[' ']'  %prec '.'
		{ $$ = build_nt (ARRAY_REF, NULL_TREE, NULL_TREE); }
	;

/* at least one statement, the first of which parses without error.  */
/* stmts is used only after decls, so an invalid first statement
   is actually regarded as an invalid decl and part of the decls.  */

stmts:
	  lineno_stmt_or_label
	| stmts lineno_stmt_or_label
	| stmts errstmt
	;

xstmts:
	/* empty */
	| stmts
	;

errstmt:  error ';'
	;

pushlevel:  /* empty */
		{ emit_line_note (input_filename, lineno);
		  pushlevel (0);
		  clear_last_expr ();
		  push_momentary ();
		  expand_start_bindings (0);
	          Gct_stmt_depth++
		}
	;

/* Read zero or more forward-declarations for labels
   that nested functions can jump to.  */
maybe_label_decls:
	  /* empty */
	| label_decls
		{ if (pedantic)
		    pedwarn ("ANSI C forbids label declarations"); }
	;

label_decls:
	  label_decl
	| label_decls label_decl
	;

label_decl:
	  LABEL identifiers_or_typenames ';'
		{ tree link;
		  for (link = $2; link; link = TREE_CHAIN (link))
		    {
		      tree label = shadow_label (TREE_VALUE (link));
		      C_DECLARED_LABEL_FLAG (label) = 1;
		      declare_nonlocal_label (label);
		    }
		}
	;

/* This is the body of a function definition.
   It causes syntax errors to ignore to the next openbrace.  */
compstmt_or_error:
	  compstmt
		{}
	| error compstmt
	;

compstmt: '{' '}'
		{ $$ = convert (void_type_node, integer_zero_node);
		  gct_build_compound_stmt(GCT_LAST(Gct_all_nodes));
                }
	| '{' pushlevel maybe_label_decls decls xstmts '}'
		{ emit_line_note (input_filename, lineno);
		  expand_end_bindings (getdecls (), 1, 0);
		  $$ = poplevel (1, 1, 0);
		  gct_build_compound_stmt(GCT_LAST(Gct_all_nodes));
	          Gct_stmt_depth--;
		  pop_momentary (); }
	| '{' pushlevel maybe_label_decls error '}'
		{ emit_line_note (input_filename, lineno);
		  expand_end_bindings (getdecls (), kept_level_p (), 0);
		  $$ = poplevel (kept_level_p (), 0, 0);
		  gct_build_compound_stmt(GCT_LAST(Gct_all_nodes));
	          Gct_stmt_depth--;
		  pop_momentary (); }
	| '{' pushlevel maybe_label_decls stmts '}'
		{ emit_line_note (input_filename, lineno);
		  expand_end_bindings (getdecls (), kept_level_p (), 0);
		  /*
		   * GCT:  We keep all levels of compound statements.
		   * $$ = poplevel (kept_level_p (), 0, 0);
		   */
		  $$ = poplevel (1, 0, 0);
		  gct_build_compound_stmt(GCT_LAST(Gct_all_nodes));
	          Gct_stmt_depth--;
		  pop_momentary (); 
                }
	;

/* Value is number of statements counted as of the closeparen.  */
simple_if:
	  if_prefix lineno_labeled_stmt
/* Make sure expand_end_cond is run once
   for each call to expand_start_cond.
   Otherwise a crash is likely.  */
	| if_prefix error
	;

if_prefix:
	  IF '(' expr ')'
		{ emit_line_note ($<filename>-1, $<lineno>0);
		  expand_start_cond (truthvalue_conversion ($3), 0);
		  $<itype>1 = stmt_count;
		  if_stmt_file = $<filename>-1;
		  if_stmt_line = $<lineno>0;
		  position_after_white_space (); }
	;

/* This is a subroutine of stmt.
   It is used twice, once for valid DO statements
   and once for catching errors in parsing the end test.  */
do_stmt_start:
	  DO
		{ stmt_count++;
		  emit_line_note ($<filename>-1, $<lineno>0);
		  /* See comment in `while' alternative, above.  */
		  emit_nop ();
		  expand_start_loop_continue_elsewhere (1);
		  position_after_white_space (); }
	  lineno_labeled_stmt WHILE
		{ expand_loop_continue_here (); }
	;

save_filename:
		{ $$ = input_filename; }
	;

save_lineno:
		{ $$ = lineno; }
	;

lineno_labeled_stmt:
	  save_filename save_lineno stmt
                { }
/*	| save_filename save_lineno error
		{ }
*/
	| save_filename save_lineno label lineno_labeled_stmt
              { int Gct_have_case = GCT_LABEL_POP();
                if (Gct_have_case == CASE) {
                   gct_build_case(GCT_LAST(Gct_all_nodes));
                } else if (Gct_have_case == DEFAULT) {
                   gct_build_default(GCT_LAST(Gct_all_nodes));
                } else {
                   gct_build_label(GCT_LAST(Gct_all_nodes));
                }
              }
	;

lineno_stmt_or_label:
	  save_filename save_lineno stmt_or_label
		{ }
	;

stmt_or_label:
	  stmt
              { int Gct_have_case;
                while (Gct_have_case = GCT_LABEL_POP()) {
                   if (Gct_have_case == CASE) {
                      gct_build_case(GCT_LAST(Gct_all_nodes));
                   } else if (Gct_have_case == DEFAULT) {
                      gct_build_default(GCT_LAST(Gct_all_nodes));
                   } else {
                      gct_build_label(GCT_LAST(Gct_all_nodes));
                   }
                }
              }
	| label
		{ int next;
		  position_after_white_space ();
		  next = getc (finput);
		  ungetc (next, finput);
		  if (pedantic && next == '}')
		    pedwarn ("ANSI C forbids label at end of compound statement");
		}
	;

/* Parse a single real statement, not including any labels.  */
stmt:
	  compstmt
		{ stmt_count++; }
        | all_iter_stmt 
	| expr ';'
		{ stmt_count++;
		  emit_line_note ($<filename>-1, $<lineno>0);
		  iterator_expand ($1);
		  gct_build_simple_stmt(GCT_LAST(Gct_all_nodes));
		  clear_momentary (); }
	| simple_if ELSE
		{ expand_start_else ();
		  $<itype>1 = stmt_count;
		  position_after_white_space (); }
	  lineno_labeled_stmt
		{ expand_end_cond ();
		  gct_build_if_else(GCT_LAST(Gct_all_nodes));	/*** GCT ***/
		  if (extra_warnings && stmt_count == $<itype>1)
		    warning ("empty body in an else-statement"); }
	| simple_if %prec IF
		{ expand_end_cond ();
		  gct_build_simple_if(GCT_LAST(Gct_all_nodes)->prev);
		  if (extra_warnings && stmt_count == $<itype>1)
		    warning_with_file_and_line (if_stmt_file, if_stmt_line,
						"empty body in an if-statement"); }
/* Make sure expand_end_cond is run once
   for each call to expand_start_cond.
   Otherwise a crash is likely.  */
	| simple_if ELSE error
		{ expand_end_cond (); }
	| WHILE
		{ stmt_count++;
		  emit_line_note ($<filename>-1, $<lineno>0);
		  /* The emit_nop used to come before emit_line_note,
		     but that made the nop seem like part of the preceding line.
		     And that was confusing when the preceding line was
		     inside of an if statement and was not really executed.
		     I think it ought to work to put the nop after the line number.
		     We will see.  --rms, July 15, 1991.  */
		  emit_nop (); }
	  '(' expr ')'
		{ /* Don't start the loop till we have succeeded
		     in parsing the end test.  This is to make sure
		     that we end every loop we start.  */
		  expand_start_loop (1);
		  emit_line_note (input_filename, lineno);
		  expand_exit_loop_if_false (NULL_PTR,
					     truthvalue_conversion ($4));
		  position_after_white_space (); }
	  lineno_labeled_stmt
		{ expand_end_loop ();
		  gct_build_while_stmt(GCT_LAST(Gct_all_nodes)); 
                }
	| do_stmt_start
	  '(' expr ')' ';'
		{ emit_line_note (input_filename, lineno);
		  expand_exit_loop_if_false (NULL_PTR,
					     truthvalue_conversion ($3));
		  expand_end_loop ();
		  clear_momentary ();
                  gct_build_do_stmt(GCT_LAST(Gct_all_nodes));  
                }
/* This rule is needed to make sure we end every loop we start.  */
	| do_stmt_start error
		{ expand_end_loop ();
		  clear_momentary ();
                  gct_build_do_stmt(GCT_LAST(Gct_all_nodes)); 
                }
	| FOR
	  '(' xexpr ';'
		{ stmt_count++;
		  emit_line_note ($<filename>-1, $<lineno>0);
		  /* See comment in `while' alternative, above.  */
		  emit_nop ();
		  if ($3) c_expand_expr_stmt ($3);
		  /* Next step is to call expand_start_loop_continue_elsewhere,
		     but wait till after we parse the entire for (...).
		     Otherwise, invalid input might cause us to call that
		     fn without calling expand_end_loop.  */
		}
	  xexpr ';'
		/* Can't emit now; wait till after expand_start_loop...  */
		{ $<lineno>7 = lineno;
		  $<filename>$ = input_filename; }
	  xexpr ')'
		{ 
		  /* Start the loop.  Doing this after parsing
		     all the expressions ensures we will end the loop.  */
		  expand_start_loop_continue_elsewhere (1);
		  /* Emit the end-test, with a line number.  */
		  emit_line_note ($<filename>8, $<lineno>7);
		  if ($6)
		    expand_exit_loop_if_false (NULL_PTR,
					       truthvalue_conversion ($6));
		  /* Don't let the tree nodes for $9 be discarded by
		     clear_momentary during the parsing of the next stmt.  */
		  push_momentary ();
		  $<lineno>7 = lineno;
		  $<filename>8 = input_filename;
		  position_after_white_space (); }
	  lineno_labeled_stmt
		{ /* Emit the increment expression, with a line number.  */
		  emit_line_note ($<filename>8, $<lineno>7);
		  expand_loop_continue_here ();
		  if ($9)
		    c_expand_expr_stmt ($9);
		  pop_momentary ();
		  expand_end_loop ();
		  gct_build_for_stmt(GCT_LAST(Gct_all_nodes)); 
                 }
	| SWITCH '(' expr ')'
		{ stmt_count++;
		  emit_line_note ($<filename>-1, $<lineno>0);
		  c_expand_start_case ($3);
		  /* Don't let the tree nodes for $3 be discarded by
		     clear_momentary during the parsing of the next stmt.  */
		  push_momentary ();
		  position_after_white_space (); }
	  lineno_labeled_stmt
		{ expand_end_case ($3);
		  pop_momentary (); 
		  gct_build_switch(GCT_LAST(Gct_all_nodes)); 
                }
	| BREAK ';'
		{ stmt_count++;
		  emit_line_note ($<filename>-1, $<lineno>0);
		  if ( ! expand_exit_something ())
		    error ("break statement not within loop or switch"); 
		  gct_build_break(GCT_LAST(Gct_all_nodes)); 
                }
	| CONTINUE ';'
		{ stmt_count++;
		  emit_line_note ($<filename>-1, $<lineno>0);
		  if (! expand_continue_loop (NULL_PTR))
		    error ("continue statement not within a loop"); 
		  gct_build_continue(GCT_LAST(Gct_all_nodes)); 
                }
	| RETURN ';'
		{ stmt_count++;
		  emit_line_note ($<filename>-1, $<lineno>0);
		  c_expand_return (NULL_TREE); 
		  gct_build_return(GCT_LAST(Gct_all_nodes)); 
                }
	| RETURN expr ';'
		{ stmt_count++;
		  emit_line_note ($<filename>-1, $<lineno>0);
		  c_expand_return ($2); 
		  gct_build_return(GCT_LAST(Gct_all_nodes)); 
                }
	| ASM_KEYWORD maybe_type_qual '(' expr ')' ';'
		{ stmt_count++;
		  emit_line_note ($<filename>-1, $<lineno>0);
		  STRIP_NOPS ($4);
		  if ((TREE_CODE ($4) == ADDR_EXPR
		       && TREE_CODE (TREE_OPERAND ($4, 0)) == STRING_CST)
		      || TREE_CODE ($4) == STRING_CST) 
		      {
			  expand_asm ($4);
			  gct_build_asm(GCT_LAST(Gct_all_nodes));	/*** GCT ***/
		      }
		  else
		    error ("argument of `asm' is not a constant string"); }
	/* This is the case with just output operands.  */
	| ASM_KEYWORD maybe_type_qual '(' expr ':' asm_operands ')' ';'
		{ stmt_count++;
		  emit_line_note ($<filename>-1, $<lineno>0);
		  c_expand_asm_operands ($4, $6, NULL_TREE, NULL_TREE,
					 $2 == ridpointers[(int)RID_VOLATILE],
					 input_filename, lineno);
		  gct_build_asm(GCT_LAST(Gct_all_nodes)); /*** GCT ***/
                }
	/* This is the case with input operands as well.  */
	| ASM_KEYWORD maybe_type_qual '(' expr ':' asm_operands ':' asm_operands ')' ';'
		{ stmt_count++;
		  emit_line_note ($<filename>-1, $<lineno>0);
		  c_expand_asm_operands ($4, $6, $8, NULL_TREE,
					 $2 == ridpointers[(int)RID_VOLATILE],
					 input_filename, lineno); 
		  gct_build_asm(GCT_LAST(Gct_all_nodes)); /*** GCT ***/
                }
	/* This is the case with clobbered registers as well.  */
	| ASM_KEYWORD maybe_type_qual '(' expr ':' asm_operands ':'
  	  asm_operands ':' asm_clobbers ')' ';'
		{ stmt_count++;
		  emit_line_note ($<filename>-1, $<lineno>0);
		  c_expand_asm_operands ($4, $6, $8, $10,
					 $2 == ridpointers[(int)RID_VOLATILE],
					 input_filename, lineno);
		  gct_build_asm(GCT_LAST(Gct_all_nodes)); /*** GCT ***/
                }
	| GOTO identifier ';'
		{ tree decl;
		  stmt_count++;
		  emit_line_note ($<filename>-1, $<lineno>0);
		  decl = lookup_label ($2);
		  if (decl != 0)
		    {
		      TREE_USED (decl) = 1;
		      expand_goto (decl);
		      gct_build_goto(GCT_LAST(Gct_all_nodes));	/*** GCT ***/
		    }
		}
	| GOTO '*' expr ';'
		{ stmt_count++;
		  emit_line_note ($<filename>-1, $<lineno>0);
		  expand_computed_goto (convert (ptr_type_node, $3)); }
	| ';'
		{ gct_build_null_stmt(GCT_LAST(Gct_all_nodes)); } /*** GCT ***/
	;

all_iter_stmt:
	  all_iter_stmt_simple
/*	| all_iter_stmt_with_decl */
	;

all_iter_stmt_simple:
	  FOR '(' primary ')' 
	  {
	    /* The value returned by this action is  */
	    /*      1 if everything is OK */ 
	    /*      0 in case of error or already bound iterator */

	    $<itype>$ = 0;
	    if (TREE_CODE ($3) != VAR_DECL)
	      error ("invalid `for (ITERATOR)' syntax");
	    if (! ITERATOR_P ($3))
	      error ("`%s' is not an iterator",
		     IDENTIFIER_POINTER (DECL_NAME ($3)));
	    else if (ITERATOR_BOUND_P ($3))
	      error ("`for (%s)' inside expansion of same iterator",
		     IDENTIFIER_POINTER (DECL_NAME ($3)));
	    else
	      {
		$<itype>$ = 1;
		iterator_for_loop_start ($3);
	      }
	  }
	  lineno_labeled_stmt
	  {
	    if ($<itype>5)
	      iterator_for_loop_end ($3);
	  }

/*  This really should allow any kind of declaration,
    for generality.  Fix it before turning it back on.

all_iter_stmt_with_decl:
	  FOR '(' ITERATOR pushlevel setspecs iterator_spec ')' 
	  {
*/	    /* The value returned by this action is  */
	    /*      1 if everything is OK */ 
	    /*      0 in case of error or already bound iterator */
/*
	    iterator_for_loop_start ($6);
	  }
	  lineno_labeled_stmt
	  {
	    iterator_for_loop_end ($6);
	    emit_line_note (input_filename, lineno);
	    expand_end_bindings (getdecls (), 1, 0);
	    $<ttype>$ = poplevel (1, 1, 0);
	    pop_momentary ();	    
	  }
*/

/* Any kind of label, including jump labels and case labels.
   ANSI C accepts labels only before statements, but we allow them
   also at the end of a compound statement.  */

label:	  CASE expr_no_commas ':'
		{ register tree value = check_case_value ($2);
		  register tree label
		    = build_decl (LABEL_DECL, NULL_TREE, NULL_TREE);

		  stmt_count++;

		  if (value != error_mark_node)
		    {
		      tree duplicate;
		      int success = pushcase (value, label, &duplicate);
		      if (success == 1)
			error ("case label not within a switch statement");
		      else if (success == 2)
			{
			  error ("duplicate case value");
			  error_with_decl (duplicate, "this is the first entry for that value");
			}
		      else if (success == 3)
			warning ("case value out of range");
		      else if (success == 5)
			error ("case label within scope of cleanup or variable array");
		     }
                  GCT_LABEL_PUSH(CASE);
		  position_after_white_space();
                }
	| CASE expr_no_commas ELLIPSIS expr_no_commas ':'
		{ register tree value1 = check_case_value ($2);
		  register tree value2 = check_case_value ($4);
		  register tree label
		    = build_decl (LABEL_DECL, NULL_TREE, NULL_TREE);

		  stmt_count++;

		  if (value1 != error_mark_node && value2 != error_mark_node)
		    {
		      tree duplicate;
		      int success = pushcase_range (value1, value2, label,
						    &duplicate);
		      if (success == 1)
			error ("case label not within a switch statement");
		      else if (success == 2)
			{
			  error ("duplicate case value");
			  error_with_decl (duplicate, "this is the first entry for that value");
			}
		      else if (success == 3)
			warning ("case value out of range");
		      else if (success == 4)
			warning ("empty case range");
		      else if (success == 5)
			error ("case label within scope of cleanup or variable array");
		    }
                  GCT_LABEL_PUSH(CASE);
                  position_after_white_space (); }
	| DEFAULT ':'
		{
		  tree duplicate;
		  register tree label
		    = build_decl (LABEL_DECL, NULL_TREE, NULL_TREE);
		  int success = pushcase (NULL_TREE, label, &duplicate);
		  stmt_count++;
		  if (success == 1)
		    error ("default label not within a switch statement");
		  else if (success == 2)
		    {
		      error ("multiple default labels in one switch");
		      error_with_decl (duplicate, "this is the first default label");
		    }
                  GCT_LABEL_PUSH(DEFAULT);
                  position_after_white_space (); }
	| identifier ':'
		{ tree label = define_label (input_filename, lineno, $1);
		  stmt_count++;
		  emit_nop ();
		  if (label)
		    expand_label (label);
		   GCT_LABEL_PUSH(1); /* placeholder */
                }
	;

/* Either a type-qualifier or nothing.  First thing in an `asm' statement.  */

maybe_type_qual:
	/* empty */
		{ emit_line_note (input_filename, lineno); }
	| TYPE_QUAL
		{ emit_line_note (input_filename, lineno); }
	;

xexpr:
	/* empty */
		{ $$ = NULL_TREE; }
	| expr
	;

/* These are the operands other than the first string and colon
   in  asm ("addextend %2,%1": "=dm" (x), "0" (y), "g" (*x))  */
asm_operands: /* empty */
		{ $$ = NULL_TREE; }
	| nonnull_asm_operands
	;

nonnull_asm_operands:
	  asm_operand
	| nonnull_asm_operands ',' asm_operand
		{ $$ = chainon ($1, $3); }
	;

asm_operand:
	  STRING '(' expr ')'
		{ $$ = build_tree_list ($1, $3); }
	;

asm_clobbers:
	  string
		{ $$ = tree_cons (NULL_TREE, combine_strings ($1), NULL_TREE); }
	| asm_clobbers ',' string
		{ $$ = tree_cons (NULL_TREE, combine_strings ($3), $1); }
	;

/* This is what appears inside the parens in a function declarator.
   Its value is a list of ..._TYPE nodes.  */
parmlist:
		{ pushlevel (0);
		  clear_parm_order ();
		  declare_parm_level (0); }
	  parmlist_1
		{ $$ = $2;
		  parmlist_tags_warning ();
		  poplevel (0, 0, 0); }
	;

parmlist_1:
	  parmlist_2 ')'
	| parms ';'
		{ tree parm;
		  if (pedantic)
		    pedwarn ("ANSI C forbids forward parameter declarations");
		  /* Mark the forward decls as such.  */
		  for (parm = getdecls (); parm; parm = TREE_CHAIN (parm))
		    TREE_ASM_WRITTEN (parm) = 1;
		  clear_parm_order (); }
	  parmlist_1
		{ $$ = $4; }
	| error ')'
		{ $$ = tree_cons (NULL_TREE, NULL_TREE, NULL_TREE); }
	;

/* This is what appears inside the parens in a function declarator.
   Is value is represented in the format that grokdeclarator expects.  */
parmlist_2:  /* empty */
		{ $$ = get_parm_info (0); }
	| ELLIPSIS
		{ $$ = get_parm_info (0);
		  if (pedantic)
		    pedwarn ("ANSI C requires a named argument before `...'");
		}
	| parms
		{ $$ = get_parm_info (1); }
	| parms ',' ELLIPSIS
		{ $$ = get_parm_info (0); }
	;

parms:
	parm
		{ push_parm_decl ($1); }
	| parms ',' parm
		{ push_parm_decl ($3); }
	;

/* A single parameter declaration or parameter type name,
   as found in a parmlist.  */
parm:
	  typed_declspecs parm_declarator
		{ $$ = build_tree_list ($1, $2)	; }
	| typed_declspecs notype_declarator
		{ $$ = build_tree_list ($1, $2)	; }
	| typed_declspecs absdcl
		{ $$ = build_tree_list ($1, $2); }
	| declmods notype_declarator
		{ $$ = build_tree_list ($1, $2)	; }
	| declmods absdcl
		{ $$ = build_tree_list ($1, $2); }
	;

/* This is used in a function definition
   where either a parmlist or an identifier list is ok.
   Its value is a list of ..._TYPE nodes or a list of identifiers.  */
parmlist_or_identifiers:
		{ pushlevel (0);
		  clear_parm_order ();
		  declare_parm_level (1); }
	  parmlist_or_identifiers_1
		{ $$ = $2;
		  parmlist_tags_warning ();
		  poplevel (0, 0, 0); }
	;

parmlist_or_identifiers_1:
	  parmlist_1
	| identifiers ')'
		{ tree t;
		  for (t = $1; t; t = TREE_CHAIN (t))
		    if (TREE_VALUE (t) == NULL_TREE)
		      error ("`...' in old-style identifier list");
		  $$ = tree_cons (NULL_TREE, NULL_TREE, $1); }
	;

/* A nonempty list of identifiers.  */
identifiers:
	IDENTIFIER
		{ $$ = build_tree_list (NULL_TREE, $1); }
	| identifiers ',' IDENTIFIER
		{ $$ = chainon ($1, build_tree_list (NULL_TREE, $3)); }
	;

/* A nonempty list of identifiers, including typenames.  */
identifiers_or_typenames:
	identifier
		{ $$ = build_tree_list (NULL_TREE, $1); }
	| identifiers_or_typenames ',' identifier
		{ $$ = chainon ($1, build_tree_list (NULL_TREE, $3)); }
	;

%%

/* GCT:  We wish to count character position on line as well as lineno. */

/*
 * Charno is the number of the character in the current file.  (This will
 * be convenient if we ever use emacs to mark characters.  It will be
 * inconvenient if line-oriented tools are used.  Take your pick.)
 */

int charno = 0;

int Gct_initialized = 0;

/* The name of the temporary file we put rewritten output into. */
char *Gct_tempname = "GCT-TEMP";	/* Normally overwritten, except when debugging. */

/* check_newline calls yylex to pull in tokens.  Such calls shouldn't
   add to the parse tree.  Neither should the initial call to check_newline
   made in toplev.c.
 */

int Gct_ignore_tokens = 0;


#define gct_getc(filep)   	(charno++, getc(filep))
#define gct_ungetc(c, filep)	(charno--, ungetc((c), (filep)))

/* Note:  because getc is a macro, can't use #define getc gct_getc hack 
   to rename uses of getc below.  Have to do it by hand. */


/* Functions for use outside this file */

int
gct_fgetc(filep)
     FILE *filep;
{
  return gct_getc(filep);
}

int
gct_fungetc(c, filep)
     int c;
     FILE *filep;
{
  return gct_ungetc(c, filep);
}



/*
 * Initialize GCT.  The Gct_textout file is passed in as -o argument.
 *
 * The instrumented file contains a header that
 * - marks it as instrumented
 * - includes gct-ps-defs.h and gct-defs.h
 * - declares the local pointers into Gct_table and Gct_group_table
 *
 * This routine requires that main_input_filename be known.  That means
 * it can't be called until the source file has been opened and the
 * filename discovered.
 *
 * This function also calls other initialization routines for other
 * modules.  See file STATE for more about what initialization is needed
 * and why.
 */

gct_init()
{
  extern char *Gct_full_defs_file;
  extern char *Gct_full_per_session_file;
  extern char *Gct_full_map_file_name;

  assert(!Gct_initialized);
  Gct_initialized = 1;

  init_instrumentation();	/* Retrieve per-session instrumentation */
  gct_initialize_groups();	/* Set up utility tables, vars. */
  init_mapfile(Gct_full_map_file_name);

  fprintf(Gct_textout, "/* __GCT_INSTRUMENTATION_TAG */\n");
  fprintf(Gct_textout, "#define GCT_TABLE_POINTER_FOR_THIS_FILE Gct_per_file_table_pointer_%d\n",
	  Gct_num_files);
  fprintf(Gct_textout, "#define GCT_RACE_TABLE_POINTER_FOR_THIS_FILE Gct_per_file_race_table_pointer_%d\n",
	  Gct_num_files);
  fprintf(Gct_textout, "#include \"%s\"\n", Gct_full_per_session_file);
  fprintf(Gct_textout, "#include \"%s\"\n", Gct_full_defs_file);
  fprintf(Gct_textout, "extern GCT_CONDITION_TYPE *Gct_per_file_table_pointer_%d;\n",
	  Gct_num_files);
  fprintf(Gct_textout, "extern long *Gct_per_file_race_table_pointer_%d;\n",
	  Gct_num_files);
  fprintf(Gct_textout, "#line 1\n");
}

/*
 * Finish processing of the instrumented file.  This depends on the style
 * of instrumentation:
 *
 * In the new style of instrumentation (where GCT calls the compiler),
 * this routine does nothing.  The driver program (gcc) is responsible
 * for the next step.
 *
 * In the old style of instrumentation, the temporary file must be placed
 * in the source file's directory.  Normally, it replaces the original
 * source.  If OPT_REPLACE is turned off, the instrumented file has the
 * name of the original file, prefixed with 'T'.
 *
 * There's more to finishing a GCT invocation than just mucking with the
 * instrumented file.  Handling of the instrumentation state is done by
 * finish_instrumentation().  See file STATE for more.
 */
extern void gct_write_metrics();
gct_finish()
{
  extern int errorcount;
  
  if (Gct_initialized)
    {
      Gct_initialized = 0;
      gct_write_metrics();
      gct_write_list(Gct_all_nodes);	/* GCT */
      gct_recursive_free_node(Gct_all_nodes);
      fputc('\n', Gct_textout);
      fflush(Gct_textout);	/* About to copy contents - make sure all in file */
      /* Probably should close the file, but I'm letting original GCC code do it. */

      finish_instrumentation();

      if (OFF == gct_option_value(OPT_PRODUCE_OBJECT))
	{
	  /*
	   * Replace the original source with the temp file.  If
	   * OPT_PRODUCE_OBJECT, the compiler driver will immediately
	   * compile the temp file and the source file is untouched.
	   */
	  char *system_buffer;

	  /* Sloppy size calculation. */
	  system_buffer =
	    (char *)xmalloc(1000+strlen(Gct_tempname)+2*strlen(main_input_filename));

	  if (errorcount > 0)
	    {
	      /* Would rather use a "note" function, but there isn't one.
		 Using warning would be misleading. */
	      error("The original file is unchanged.");
	    }
	  else if (OFF == gct_option_value(OPT_REPLACE))
	    {
	      /* Of course, on SysV, this will run into filename limits.  But this
		 is not for general user's use. */
	      sprintf(system_buffer, "cp %s T%s", Gct_tempname,
		      main_input_filename);
	      if (0 != system(system_buffer))
		{
		  error("Couldn't create 'T' file.");
		  fatal("Failed:  %s\n", system_buffer);
		}
	    }
	  else
	    {
	      extern char *Gct_full_restore_log_file;
	      char *main_directory;
	      char *main_file;
	      char *full_backup;		/* Full name of backup directory. */
	      struct stat backup_statbuf;	/* Stat of backup directory */
	      struct stat orig_statbuf;	/* Stat of original file. */
	      
	      /* Find current modes of file. */
	      if (-1==stat(main_input_filename, &orig_statbuf))
		{
		  fatal ("Can't find current modes for %s.", main_input_filename);
		}
	      
	      
	      split_file(main_input_filename, &main_directory, &main_file);
	      
	      full_backup = (char *)xmalloc(strlen(main_directory)+1+strlen(GCT_BACKUP_DIR)+1);
	      sprintf(full_backup, "%s/%s", main_directory, GCT_BACKUP_DIR);
	      
	      /* Make the backup directory if needed. */
	      if (-1==stat(full_backup, &backup_statbuf))
		{
		  if (-1==mkdir (full_backup,00777))
		    fatal ("Can't create backup directory %s.", full_backup);
		}
	      
	      
	      /* Backup the file. */
	      sprintf (system_buffer,"/bin/rm -f %s/%s \n", full_backup, main_file);
	      /* printf (system_buffer); */
	  
	      if (0!=system (system_buffer))
		fatal ("Already an unremovable backup file for %s",
		       main_input_filename);
	  
	      sprintf (system_buffer,"/bin/mv %s %s \n", main_input_filename,
		       full_backup);
	      
	      if (0!=system (system_buffer))
		fatal ("Can't backup source file %s", main_input_filename);
	  
	      /*
	       * Make the replaced file.   Note: I don't know what's portable for
	       * the mode bits, so I'll only preserve the bottom part, which
	       * has been other-group-owner rwx since time immemorial.
	       *
	       * Copy is used so that the .c file is newer than the .o file.
	       * I might delete the backup copy now, but in case something
	       * goes wrong, having a copy of the original file in the backup
	       * directory might save the day.
	       */
	      sprintf(system_buffer, "echo \"cd `pwd`;\" 'cp %s/%s %s/%s; chmod %o %s/%s' >> %s\n",
		      full_backup, main_file,
		      main_directory, main_file,
		      orig_statbuf.st_mode & 0777,
		      main_directory, main_file,
		      Gct_full_restore_log_file);
	      /* printf(system_buffer); */
	      if (0!=system (system_buffer))
		fatal ("Can't update  %s", Gct_full_restore_log_file);
	      
	      /* Replace the file. */
	      sprintf(system_buffer, "cp %s %s", Gct_tempname, main_input_filename);
	      if (0 != system(system_buffer))
		{
		  error("Couldn't replace original source with instrumented source.");
		  fatal("Failed:  %s\n", system_buffer);
		}
	  
	      free(main_directory);
	      free(main_file);
	    }
      
	  free(system_buffer);
	}
    }
}


static void GCT_LABEL_PUSH(x)
int x;
{
    struct Gct_label *tmp;                                 
    Gct_label_type[Gct_label_type_ptr].type = x; 
    Gct_label_type[Gct_label_type_ptr++].depth = Gct_stmt_depth;
    
    if (Gct_label_type_ptr == Gct_label_size) {
	tmp = (struct Gct_label *)malloc(sizeof(struct Gct_label) *
	                                 2 * Gct_label_size); 
	assert(tmp);
	memcpy(tmp, Gct_label_type, Gct_label_size*sizeof(int));
	if (Gct_label_type != Gct_label_buff) {
	    free(Gct_label_type);
	} 
	Gct_label_type = tmp;
	Gct_label_size *= 2;
    }
}

static int GCT_LABEL_POP()
{
    if (Gct_label_type_ptr < 1 
	|| (Gct_label_type[Gct_label_type_ptr-1].depth != Gct_stmt_depth)) {
	return 0;
    }
    return Gct_label_type[--Gct_label_type_ptr].type;
}



/* End GCT */
     

