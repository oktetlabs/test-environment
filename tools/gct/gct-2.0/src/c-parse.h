/* A Bison parser, made by GNU Bison 1.875d.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     IDENTIFIER = 258,
     TYPENAME = 259,
     SCSPEC = 260,
     TYPESPEC = 261,
     TYPE_QUAL = 262,
     CONSTANT = 263,
     STRING = 264,
     ELLIPSIS = 265,
     SIZEOF = 266,
     ENUM = 267,
     STRUCT = 268,
     UNION = 269,
     IF = 270,
     ELSE = 271,
     WHILE = 272,
     DO = 273,
     FOR = 274,
     SWITCH = 275,
     CASE = 276,
     DEFAULT = 277,
     BREAK = 278,
     CONTINUE = 279,
     RETURN = 280,
     GOTO = 281,
     ASM_KEYWORD = 282,
     TYPEOF = 283,
     ALIGNOF = 284,
     ALIGN = 285,
     ATTRIBUTE = 286,
     EXTENSION = 287,
     LABEL = 288,
     REALPART = 289,
     IMAGPART = 290,
     ASSIGN = 291,
     OROR = 292,
     ANDAND = 293,
     EQCOMPARE = 294,
     ARITHCOMPARE = 295,
     RSHIFT = 296,
     LSHIFT = 297,
     MINUSMINUS = 298,
     PLUSPLUS = 299,
     UNARY = 300,
     HYPERUNARY = 301,
     POINTSAT = 302,
     INTERFACE = 303,
     IMPLEMENTATION = 304,
     END = 305,
     SELECTOR = 306,
     DEFS = 307,
     ENCODE = 308,
     CLASSNAME = 309,
     PUBLIC = 310,
     PRIVATE = 311,
     PROTECTED = 312,
     PROTOCOL = 313,
     OBJECTNAME = 314,
     CLASS = 315,
     ALIAS = 316,
     OBJC_STRING = 317
   };
#endif
#define IDENTIFIER 258
#define TYPENAME 259
#define SCSPEC 260
#define TYPESPEC 261
#define TYPE_QUAL 262
#define CONSTANT 263
#define STRING 264
#define ELLIPSIS 265
#define SIZEOF 266
#define ENUM 267
#define STRUCT 268
#define UNION 269
#define IF 270
#define ELSE 271
#define WHILE 272
#define DO 273
#define FOR 274
#define SWITCH 275
#define CASE 276
#define DEFAULT 277
#define BREAK 278
#define CONTINUE 279
#define RETURN 280
#define GOTO 281
#define ASM_KEYWORD 282
#define TYPEOF 283
#define ALIGNOF 284
#define ALIGN 285
#define ATTRIBUTE 286
#define EXTENSION 287
#define LABEL 288
#define REALPART 289
#define IMAGPART 290
#define ASSIGN 291
#define OROR 292
#define ANDAND 293
#define EQCOMPARE 294
#define ARITHCOMPARE 295
#define RSHIFT 296
#define LSHIFT 297
#define MINUSMINUS 298
#define PLUSPLUS 299
#define UNARY 300
#define HYPERUNARY 301
#define POINTSAT 302
#define INTERFACE 303
#define IMPLEMENTATION 304
#define END 305
#define SELECTOR 306
#define DEFS 307
#define ENCODE 308
#define CLASSNAME 309
#define PUBLIC 310
#define PRIVATE 311
#define PROTECTED 312
#define PROTOCOL 313
#define OBJECTNAME 314
#define CLASS 315
#define ALIAS 316
#define OBJC_STRING 317




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 132 "c-parse.y"
typedef union YYSTYPE {long itype; tree ttype; enum tree_code code;
	char *filename; int lineno; } YYSTYPE;
/* Line 1285 of yacc.c.  */
#line 164 "c-parse.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



