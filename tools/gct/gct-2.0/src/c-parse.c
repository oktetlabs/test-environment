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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



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




/* Copy the first part of user declarations.  */
#line 44 "c-parse.y"

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


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 132 "c-parse.y"
typedef union YYSTYPE {long itype; tree ttype; enum tree_code code;
	char *filename; int lineno; } YYSTYPE;
/* Line 191 of yacc.c.  */
#line 289 "c-parse.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */
#line 243 "c-parse.y"

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


/* Line 214 of yacc.c.  */
#line 325 "c-parse.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

# ifndef YYFREE
#  define YYFREE free
# endif
# ifndef YYMALLOC
#  define YYMALLOC malloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   define YYSTACK_ALLOC alloca
#  endif
# else
#  if defined (alloca) || defined (_ALLOCA_H)
#   define YYSTACK_ALLOC alloca
#  else
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2147

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  85
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  130
/* YYNRULES -- Number of rules. */
#define YYNRULES  350
/* YYNRULES -- Number of states. */
#define YYNSTATES  615

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   317

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    81,     2,     2,     2,    53,    44,     2,
      59,    77,    51,    49,    82,    50,    58,    52,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    39,    78,
       2,    36,     2,    38,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    60,     2,    84,    43,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    83,    42,    79,    80,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    37,    40,    41,    45,    46,    47,    48,    54,    55,
      56,    57,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     4,     6,     7,    10,    11,    15,    17,
      19,    25,    29,    34,    39,    42,    45,    48,    51,    53,
      54,    55,    63,    68,    69,    70,    78,    83,    84,    85,
      92,    96,    98,   100,   102,   104,   106,   108,   110,   112,
     114,   116,   117,   119,   121,   125,   127,   130,   131,   135,
     138,   141,   144,   149,   152,   157,   160,   163,   165,   170,
     178,   180,   184,   188,   192,   196,   200,   204,   208,   212,
     216,   220,   224,   228,   232,   236,   242,   246,   250,   252,
     254,   256,   260,   264,   265,   270,   275,   280,   284,   288,
     291,   294,   296,   299,   300,   302,   305,   309,   311,   313,
     316,   319,   324,   329,   332,   335,   339,   341,   343,   346,
     349,   350,   355,   360,   364,   368,   371,   374,   377,   381,
     382,   385,   388,   390,   392,   395,   398,   401,   405,   406,
     409,   411,   413,   415,   420,   425,   427,   429,   431,   433,
     437,   439,   443,   444,   449,   450,   457,   461,   462,   469,
     473,   474,   481,   483,   487,   489,   494,   499,   508,   510,
     513,   517,   522,   524,   526,   530,   537,   546,   551,   558,
     562,   568,   569,   573,   574,   578,   580,   582,   586,   590,
     595,   599,   603,   605,   609,   614,   618,   622,   624,   628,
     632,   636,   641,   645,   647,   648,   655,   656,   662,   665,
     666,   673,   674,   680,   683,   684,   692,   693,   700,   703,
     704,   706,   707,   709,   711,   714,   715,   719,   722,   726,
     728,   732,   734,   736,   738,   742,   747,   754,   760,   762,
     766,   768,   772,   775,   778,   779,   781,   783,   786,   787,
     790,   794,   798,   801,   805,   810,   814,   817,   821,   824,
     826,   829,   832,   833,   835,   838,   839,   840,   842,   844,
     847,   851,   853,   856,   859,   866,   872,   878,   881,   884,
     889,   890,   895,   896,   897,   901,   906,   910,   912,   914,
     916,   918,   921,   922,   927,   929,   933,   934,   935,   943,
     949,   952,   953,   954,   955,   968,   969,   976,   979,   982,
     985,   989,   996,  1005,  1016,  1029,  1033,  1038,  1040,  1042,
    1043,  1050,  1054,  1060,  1063,  1066,  1067,  1069,  1070,  1072,
    1073,  1075,  1077,  1081,  1086,  1088,  1092,  1093,  1096,  1099,
    1100,  1105,  1108,  1109,  1111,  1113,  1117,  1119,  1123,  1126,
    1129,  1132,  1135,  1138,  1139,  1142,  1144,  1147,  1149,  1153,
    1155
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
      86,     0,    -1,    -1,    87,    -1,    -1,    88,    90,    -1,
      -1,    87,    89,    90,    -1,    92,    -1,    91,    -1,    27,
      59,   101,    77,    78,    -1,   117,   127,    78,    -1,   121,
     117,   127,    78,    -1,   119,   117,   126,    78,    -1,   121,
      78,    -1,   119,    78,    -1,     1,    78,    -1,     1,    79,
      -1,    78,    -1,    -1,    -1,   119,   117,   142,    93,   111,
      94,   174,    -1,   119,   117,   142,     1,    -1,    -1,    -1,
     121,   117,   145,    95,   111,    96,   174,    -1,   121,   117,
     145,     1,    -1,    -1,    -1,   117,   145,    97,   111,    98,
     174,    -1,   117,   145,     1,    -1,     3,    -1,     4,    -1,
      44,    -1,    50,    -1,    49,    -1,    55,    -1,    54,    -1,
      80,    -1,    81,    -1,   103,    -1,    -1,   103,    -1,   107,
      -1,   103,    82,   107,    -1,   108,    -1,    51,   106,    -1,
      -1,    32,   105,   106,    -1,   100,   106,    -1,    41,    99,
      -1,    11,   104,    -1,    11,    59,   162,    77,    -1,    29,
     104,    -1,    29,    59,   162,    77,    -1,    34,   106,    -1,
      35,   106,    -1,   104,    -1,    59,   162,    77,   106,    -1,
      59,   162,    77,    83,   137,   153,    79,    -1,   106,    -1,
     107,    49,   107,    -1,   107,    50,   107,    -1,   107,    51,
     107,    -1,   107,    52,   107,    -1,   107,    53,   107,    -1,
     107,    48,   107,    -1,   107,    47,   107,    -1,   107,    46,
     107,    -1,   107,    45,   107,    -1,   107,    44,   107,    -1,
     107,    42,   107,    -1,   107,    43,   107,    -1,   107,    41,
     107,    -1,   107,    40,   107,    -1,   107,    38,   198,    39,
     107,    -1,   107,    36,   107,    -1,   107,    37,   107,    -1,
       3,    -1,     8,    -1,   110,    -1,    59,   101,    77,    -1,
      59,     1,    77,    -1,    -1,    59,   109,   175,    77,    -1,
     108,    59,   102,    77,    -1,   108,    60,   101,    84,    -1,
     108,    58,    99,    -1,   108,    61,    99,    -1,   108,    55,
      -1,   108,    54,    -1,     9,    -1,   110,     9,    -1,    -1,
     113,    -1,   113,    10,    -1,   180,   181,   114,    -1,   112,
      -1,   169,    -1,   113,   112,    -1,   112,   169,    -1,   119,
     117,   126,    78,    -1,   121,   117,   127,    78,    -1,   119,
      78,    -1,   121,    78,    -1,   180,   181,   118,    -1,   115,
      -1,   169,    -1,   116,   115,    -1,   115,   169,    -1,    -1,
     119,   117,   126,    78,    -1,   121,   117,   127,    78,    -1,
     119,   117,   138,    -1,   121,   117,   140,    -1,   119,    78,
      -1,   121,    78,    -1,   124,   120,    -1,   121,   124,   120,
      -1,    -1,   120,   125,    -1,   120,     5,    -1,     7,    -1,
       5,    -1,   121,     7,    -1,   121,     5,    -1,   124,   123,
      -1,   164,   124,   123,    -1,    -1,   123,   125,    -1,     6,
      -1,   146,    -1,     4,    -1,    28,    59,   101,    77,    -1,
      28,    59,   162,    77,    -1,     6,    -1,     7,    -1,   146,
      -1,   129,    -1,   126,    82,   129,    -1,   131,    -1,   127,
      82,   129,    -1,    -1,    27,    59,   110,    77,    -1,    -1,
     142,   128,   133,    36,   130,   136,    -1,   142,   128,   133,
      -1,    -1,   145,   128,   133,    36,   132,   136,    -1,   145,
     128,   133,    -1,    -1,    31,    59,    59,   134,    77,    77,
      -1,   135,    -1,   134,    82,   135,    -1,     3,    -1,     3,
      59,     3,    77,    -1,     3,    59,     8,    77,    -1,     3,
      59,     3,    82,     8,    82,     8,    77,    -1,   107,    -1,
      83,    79,    -1,    83,   137,    79,    -1,    83,   137,    82,
      79,    -1,     1,    -1,   136,    -1,   137,    82,   136,    -1,
      60,   107,    10,   107,    84,   136,    -1,   137,    82,    60,
     107,    10,   107,    84,   136,    -1,    60,   107,    84,   136,
      -1,   137,    82,    60,   107,    84,   136,    -1,    99,    39,
     136,    -1,   137,    82,    99,    39,   136,    -1,    -1,   142,
     139,   175,    -1,    -1,   145,   141,   175,    -1,   143,    -1,
     145,    -1,    59,   143,    77,    -1,   143,    59,   210,    -1,
     143,    60,   101,    84,    -1,   143,    60,    84,    -1,    51,
     165,   143,    -1,     4,    -1,   144,    59,   210,    -1,   144,
      60,   101,    84,    -1,   144,    60,    84,    -1,    51,   165,
     144,    -1,     4,    -1,   145,    59,   210,    -1,    59,   145,
      77,    -1,    51,   165,   145,    -1,   145,    60,   101,    84,
      -1,   145,    60,    84,    -1,     3,    -1,    -1,    13,    99,
      83,   147,   155,    79,    -1,    -1,    13,    83,   148,   155,
      79,    -1,    13,    99,    -1,    -1,    14,    99,    83,   149,
     155,    79,    -1,    -1,    14,    83,   150,   155,    79,    -1,
      14,    99,    -1,    -1,    12,    99,    83,   151,   160,   154,
      79,    -1,    -1,    12,    83,   152,   160,   154,    79,    -1,
      12,    99,    -1,    -1,    82,    -1,    -1,    82,    -1,   156,
      -1,   156,   157,    -1,    -1,   156,   157,    78,    -1,   156,
      78,    -1,   122,   117,   158,    -1,   122,    -1,   164,   117,
     158,    -1,   164,    -1,     1,    -1,   159,    -1,   158,    82,
     159,    -1,   180,   181,   142,   133,    -1,   180,   181,   142,
      39,   107,   133,    -1,   180,   181,    39,   107,   133,    -1,
     161,    -1,   160,    82,   161,    -1,    99,    -1,    99,    36,
     107,    -1,   122,   163,    -1,   164,   163,    -1,    -1,   166,
      -1,     7,    -1,   164,     7,    -1,    -1,   165,     7,    -1,
      59,   166,    77,    -1,    51,   165,   166,    -1,    51,   165,
      -1,   166,    59,   203,    -1,   166,    60,   101,    84,    -1,
     166,    60,    84,    -1,    59,   203,    -1,    60,   101,    84,
      -1,    60,    84,    -1,   183,    -1,   167,   183,    -1,   167,
     169,    -1,    -1,   167,    -1,     1,    78,    -1,    -1,    -1,
     172,    -1,   173,    -1,   172,   173,    -1,    33,   214,    78,
      -1,   175,    -1,     1,   175,    -1,    83,    79,    -1,    83,
     170,   171,   116,   168,    79,    -1,    83,   170,   171,     1,
      79,    -1,    83,   170,   171,   167,    79,    -1,   177,   182,
      -1,   177,     1,    -1,    15,    59,   101,    77,    -1,    -1,
      18,   179,   182,    17,    -1,    -1,    -1,   180,   181,   185,
      -1,   180,   181,   196,   182,    -1,   180,   181,   184,    -1,
     185,    -1,   196,    -1,   175,    -1,   193,    -1,   101,    78,
      -1,    -1,   176,    16,   186,   182,    -1,   176,    -1,   176,
      16,     1,    -1,    -1,    -1,    17,   187,    59,   101,    77,
     188,   182,    -1,   178,    59,   101,    77,    78,    -1,   178,
       1,    -1,    -1,    -1,    -1,    19,    59,   198,    78,   189,
     198,    78,   190,   198,    77,   191,   182,    -1,    -1,    20,
      59,   101,    77,   192,   182,    -1,    23,    78,    -1,    24,
      78,    -1,    25,    78,    -1,    25,   101,    78,    -1,    27,
     197,    59,   101,    77,    78,    -1,    27,   197,    59,   101,
      39,   199,    77,    78,    -1,    27,   197,    59,   101,    39,
     199,    39,   199,    77,    78,    -1,    27,   197,    59,   101,
      39,   199,    39,   199,    39,   202,    77,    78,    -1,    26,
      99,    78,    -1,    26,    51,   101,    78,    -1,    78,    -1,
     194,    -1,    -1,    19,    59,   108,    77,   195,   182,    -1,
      21,   107,    39,    -1,    21,   107,    10,   107,    39,    -1,
      22,    39,    -1,    99,    39,    -1,    -1,     7,    -1,    -1,
     101,    -1,    -1,   200,    -1,   201,    -1,   200,    82,   201,
      -1,     9,    59,   101,    77,    -1,   110,    -1,   202,    82,
     110,    -1,    -1,   204,   205,    -1,   207,    77,    -1,    -1,
     208,    78,   206,   205,    -1,     1,    77,    -1,    -1,    10,
      -1,   208,    -1,   208,    82,    10,    -1,   209,    -1,   208,
      82,   209,    -1,   119,   144,    -1,   119,   145,    -1,   119,
     163,    -1,   121,   145,    -1,   121,   163,    -1,    -1,   211,
     212,    -1,   205,    -1,   213,    77,    -1,     3,    -1,   213,
      82,     3,    -1,    99,    -1,   214,    82,    99,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,   270,   270,   273,   287,   287,   288,   288,   292,   293,
     294,   305,   310,   312,   314,   316,   318,   319,   320,   327,
     332,   326,   339,   342,   347,   341,   354,   357,   362,   356,
     369,   374,   375,   378,   380,   382,   384,   386,   388,   390,
     394,   402,   403,   407,   409,   415,   416,   423,   422,   430,
     435,   461,   469,   474,   480,   485,   488,   494,   495,   501,
     536,   537,   543,   549,   555,   561,   567,   573,   579,   585,
     590,   596,   602,   608,   614,   620,   625,   632,   642,   743,
     746,   750,   757,   760,   759,   793,   805,   810,   816,   823,
     828,   837,   838,   849,   851,   852,   863,   868,   869,   870,
     871,   875,   887,   899,   910,   927,   932,   933,   934,   935,
     943,   951,   963,   975,   987,   999,  1009,  1026,  1028,  1033,
    1034,  1036,  1048,  1051,  1053,  1056,  1070,  1072,  1077,  1078,
    1086,  1087,  1088,  1092,  1094,  1100,  1101,  1102,  1106,  1107,
    1111,  1112,  1117,  1118,  1126,  1125,  1131,  1139,  1138,  1144,
    1153,  1154,  1159,  1161,  1166,  1171,  1181,  1192,  1211,  1212,
    1216,  1218,  1220,  1227,  1229,  1234,  1238,  1243,  1245,  1247,
    1249,  1255,  1254,  1276,  1275,  1299,  1300,  1306,  1308,  1313,
    1315,  1317,  1319,  1329,  1334,  1336,  1338,  1340,  1347,  1352,
    1354,  1356,  1358,  1360,  1366,  1365,  1380,  1379,  1391,  1394,
    1393,  1404,  1403,  1413,  1416,  1415,  1428,  1427,  1439,  1443,
    1445,  1448,  1450,  1455,  1457,  1463,  1464,  1466,  1481,  1486,
    1491,  1496,  1501,  1506,  1507,  1512,  1515,  1519,  1530,  1531,
    1537,  1539,  1544,  1546,  1552,  1553,  1557,  1559,  1565,  1566,
    1571,  1574,  1576,  1578,  1580,  1582,  1584,  1586,  1588,  1597,
    1598,  1599,  1602,  1604,  1607,  1611,  1622,  1624,  1630,  1631,
    1635,  1649,  1651,  1654,  1658,  1665,  1672,  1688,  1692,  1696,
    1710,  1709,  1721,  1725,  1729,  1734,  1747,  1752,  1764,  1776,
    1778,  1779,  1786,  1785,  1794,  1803,  1806,  1816,  1805,  1828,
    1838,  1845,  1857,  1860,  1843,  1886,  1885,  1898,  1905,  1912,
    1918,  1924,  1938,  1947,  1956,  1965,  1977,  1981,  1986,  1992,
    1991,  2044,  2070,  2099,  2115,  2129,  2130,  2136,  2137,  2143,
    2144,  2148,  2149,  2154,  2159,  2161,  2168,  2168,  2178,  2180,
    2179,  2189,  2196,  2197,  2202,  2204,  2209,  2211,  2218,  2220,
    2222,  2224,  2226,  2234,  2234,  2244,  2245,  2255,  2257,  2263,
    2265
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "IDENTIFIER", "TYPENAME", "SCSPEC",
  "TYPESPEC", "TYPE_QUAL", "CONSTANT", "STRING", "ELLIPSIS", "SIZEOF",
  "ENUM", "STRUCT", "UNION", "IF", "ELSE", "WHILE", "DO", "FOR", "SWITCH",
  "CASE", "DEFAULT", "BREAK", "CONTINUE", "RETURN", "GOTO", "ASM_KEYWORD",
  "TYPEOF", "ALIGNOF", "ALIGN", "ATTRIBUTE", "EXTENSION", "LABEL",
  "REALPART", "IMAGPART", "'='", "ASSIGN", "'?'", "':'", "OROR", "ANDAND",
  "'|'", "'^'", "'&'", "EQCOMPARE", "ARITHCOMPARE", "RSHIFT", "LSHIFT",
  "'+'", "'-'", "'*'", "'/'", "'%'", "MINUSMINUS", "PLUSPLUS", "UNARY",
  "HYPERUNARY", "'.'", "'('", "'['", "POINTSAT", "INTERFACE",
  "IMPLEMENTATION", "END", "SELECTOR", "DEFS", "ENCODE", "CLASSNAME",
  "PUBLIC", "PRIVATE", "PROTECTED", "PROTOCOL", "OBJECTNAME", "CLASS",
  "ALIAS", "OBJC_STRING", "')'", "';'", "'}'", "'~'", "'!'", "','", "'{'",
  "']'", "$accept", "program", "extdefs", "@1", "@2", "extdef", "datadef",
  "fndef", "@3", "@4", "@5", "@6", "@7", "@8", "identifier", "unop",
  "expr", "exprlist", "nonnull_exprlist", "unary_expr", "@9", "cast_expr",
  "expr_no_commas", "primary", "@10", "string", "xdecls",
  "lineno_datadecl", "datadecls", "datadecl", "lineno_decl", "decls",
  "setspecs", "decl", "typed_declspecs", "reserved_declspecs", "declmods",
  "typed_typespecs", "reserved_typespecquals", "typespec",
  "typespecqual_reserved", "initdecls", "notype_initdecls", "maybeasm",
  "initdcl", "@11", "notype_initdcl", "@12", "maybe_attribute",
  "attribute_list", "attrib", "init", "initlist", "nested_function", "@13",
  "notype_nested_function", "@14", "declarator", "after_type_declarator",
  "parm_declarator", "notype_declarator", "structsp", "@15", "@16", "@17",
  "@18", "@19", "@20", "maybecomma", "maybecomma_warn",
  "component_decl_list", "component_decl_list2", "component_decl",
  "components", "component_declarator", "enumlist", "enumerator",
  "typename", "absdcl", "nonempty_type_quals", "type_quals", "absdcl1",
  "stmts", "xstmts", "errstmt", "pushlevel", "maybe_label_decls",
  "label_decls", "label_decl", "compstmt_or_error", "compstmt",
  "simple_if", "if_prefix", "do_stmt_start", "@21", "save_filename",
  "save_lineno", "lineno_labeled_stmt", "lineno_stmt_or_label",
  "stmt_or_label", "stmt", "@22", "@23", "@24", "@25", "@26", "@27", "@28",
  "all_iter_stmt", "all_iter_stmt_simple", "@29", "label",
  "maybe_type_qual", "xexpr", "asm_operands", "nonnull_asm_operands",
  "asm_operand", "asm_clobbers", "parmlist", "@30", "parmlist_1", "@31",
  "parmlist_2", "parms", "parm", "parmlist_or_identifiers", "@32",
  "parmlist_or_identifiers_1", "identifiers", "identifiers_or_typenames", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,    61,   291,    63,    58,
     292,   293,   124,    94,    38,   294,   295,   296,   297,    43,
      45,    42,    47,    37,   298,   299,   300,   301,    46,    40,
      91,   302,   303,   304,   305,   306,   307,   308,   309,   310,
     311,   312,   313,   314,   315,   316,   317,    41,    59,   125,
     126,    33,    44,   123,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    85,    86,    86,    88,    87,    89,    87,    90,    90,
      90,    91,    91,    91,    91,    91,    91,    91,    91,    93,
      94,    92,    92,    95,    96,    92,    92,    97,    98,    92,
      92,    99,    99,   100,   100,   100,   100,   100,   100,   100,
     101,   102,   102,   103,   103,   104,   104,   105,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   106,   106,   106,
     107,   107,   107,   107,   107,   107,   107,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   107,   107,   108,   108,
     108,   108,   108,   109,   108,   108,   108,   108,   108,   108,
     108,   110,   110,   111,   111,   111,   112,   113,   113,   113,
     113,   114,   114,   114,   114,   115,   116,   116,   116,   116,
     117,   118,   118,   118,   118,   118,   118,   119,   119,   120,
     120,   120,   121,   121,   121,   121,   122,   122,   123,   123,
     124,   124,   124,   124,   124,   125,   125,   125,   126,   126,
     127,   127,   128,   128,   130,   129,   129,   132,   131,   131,
     133,   133,   134,   134,   135,   135,   135,   135,   136,   136,
     136,   136,   136,   137,   137,   137,   137,   137,   137,   137,
     137,   139,   138,   141,   140,   142,   142,   143,   143,   143,
     143,   143,   143,   144,   144,   144,   144,   144,   145,   145,
     145,   145,   145,   145,   147,   146,   148,   146,   146,   149,
     146,   150,   146,   146,   151,   146,   152,   146,   146,   153,
     153,   154,   154,   155,   155,   156,   156,   156,   157,   157,
     157,   157,   157,   158,   158,   159,   159,   159,   160,   160,
     161,   161,   162,   162,   163,   163,   164,   164,   165,   165,
     166,   166,   166,   166,   166,   166,   166,   166,   166,   167,
     167,   167,   168,   168,   169,   170,   171,   171,   172,   172,
     173,   174,   174,   175,   175,   175,   175,   176,   176,   177,
     179,   178,   180,   181,   182,   182,   183,   184,   184,   185,
     185,   185,   186,   185,   185,   185,   187,   188,   185,   185,
     185,   189,   190,   191,   185,   192,   185,   185,   185,   185,
     185,   185,   185,   185,   185,   185,   185,   185,   193,   195,
     194,   196,   196,   196,   196,   197,   197,   198,   198,   199,
     199,   200,   200,   201,   202,   202,   204,   203,   205,   206,
     205,   205,   207,   207,   207,   207,   208,   208,   209,   209,
     209,   209,   209,   211,   210,   212,   212,   213,   213,   214,
     214
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     1,     0,     2,     0,     3,     1,     1,
       5,     3,     4,     4,     2,     2,     2,     2,     1,     0,
       0,     7,     4,     0,     0,     7,     4,     0,     0,     6,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     0,     1,     1,     3,     1,     2,     0,     3,     2,
       2,     2,     4,     2,     4,     2,     2,     1,     4,     7,
       1,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     5,     3,     3,     1,     1,
       1,     3,     3,     0,     4,     4,     4,     3,     3,     2,
       2,     1,     2,     0,     1,     2,     3,     1,     1,     2,
       2,     4,     4,     2,     2,     3,     1,     1,     2,     2,
       0,     4,     4,     3,     3,     2,     2,     2,     3,     0,
       2,     2,     1,     1,     2,     2,     2,     3,     0,     2,
       1,     1,     1,     4,     4,     1,     1,     1,     1,     3,
       1,     3,     0,     4,     0,     6,     3,     0,     6,     3,
       0,     6,     1,     3,     1,     4,     4,     8,     1,     2,
       3,     4,     1,     1,     3,     6,     8,     4,     6,     3,
       5,     0,     3,     0,     3,     1,     1,     3,     3,     4,
       3,     3,     1,     3,     4,     3,     3,     1,     3,     3,
       3,     4,     3,     1,     0,     6,     0,     5,     2,     0,
       6,     0,     5,     2,     0,     7,     0,     6,     2,     0,
       1,     0,     1,     1,     2,     0,     3,     2,     3,     1,
       3,     1,     1,     1,     3,     4,     6,     5,     1,     3,
       1,     3,     2,     2,     0,     1,     1,     2,     0,     2,
       3,     3,     2,     3,     4,     3,     2,     3,     2,     1,
       2,     2,     0,     1,     2,     0,     0,     1,     1,     2,
       3,     1,     2,     2,     6,     5,     5,     2,     2,     4,
       0,     4,     0,     0,     3,     4,     3,     1,     1,     1,
       1,     2,     0,     4,     1,     3,     0,     0,     7,     5,
       2,     0,     0,     0,    12,     0,     6,     2,     2,     2,
       3,     6,     8,    10,    12,     3,     4,     1,     1,     0,
       6,     3,     5,     2,     2,     0,     1,     0,     1,     0,
       1,     1,     3,     4,     1,     3,     0,     2,     2,     0,
       4,     2,     0,     1,     1,     3,     1,     3,     2,     2,
       2,     2,     2,     0,     2,     1,     2,     1,     3,     1,
       3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short int yydefact[] =
{
       4,     0,     6,     0,     1,     0,     0,   132,   123,   130,
     122,     0,     0,     0,     0,     0,    18,     5,     9,     8,
       0,   110,   110,   119,   131,     7,    16,    17,    31,    32,
     206,   208,   196,   198,   201,   203,     0,     0,   193,   238,
       0,     0,   140,     0,    15,     0,   125,   124,    14,     0,
     119,   117,     0,   204,   215,   194,   215,   199,    78,    79,
      91,     0,     0,    47,     0,     0,     0,    33,    35,    34,
       0,    37,    36,     0,    38,    39,     0,     0,    40,    57,
      60,    43,    45,    80,   236,     0,   234,   128,     0,   234,
       0,     0,    11,     0,    30,     0,   343,     0,     0,   150,
     182,   238,     0,     0,   138,     0,   175,   176,     0,     0,
     118,   121,   135,   136,   120,   137,   230,   211,   228,     0,
       0,     0,   215,     0,   215,     0,    51,     0,    53,     0,
      55,    56,    50,    46,     0,     0,     0,     0,    49,     0,
       0,     0,     0,   317,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    90,    89,
       0,    41,     0,     0,    92,   133,   238,   326,     0,   232,
     235,   126,   134,   237,   128,   233,   239,   190,   189,   141,
     142,     0,   188,     0,   192,     0,     0,    28,     0,   272,
      98,   273,     0,   149,     0,     0,    13,     0,    22,     0,
     150,   343,     0,    12,    26,     0,     0,   212,     0,   211,
     197,   222,   217,   110,   214,   110,     0,   202,     0,     0,
       0,    48,    82,    81,   255,     0,     0,    10,    44,    76,
      77,   318,     0,    74,    73,    71,    72,    70,    69,    68,
      67,    66,    61,    62,    63,    64,    65,    87,     0,    42,
       0,    88,   242,     0,   246,     0,   248,     0,   326,     0,
     129,   127,     0,     0,   347,   333,   234,   234,   345,     0,
     334,   336,   344,     0,   191,   254,     0,   100,    95,    99,
       0,     0,   147,   181,   177,   139,    20,   146,   178,   180,
       0,    24,   231,   229,   207,     0,   272,   216,   272,   195,
     200,    52,    54,   263,   256,    84,     0,    58,     0,    85,
      86,   241,   240,   327,   247,   243,   245,     0,   143,   331,
     187,   238,   326,   338,   339,   340,   238,   341,   342,   328,
     329,     0,   346,     0,     0,    29,   261,    96,   110,   110,
       0,     0,     0,   144,   179,     0,   205,   218,   223,   273,
     220,     0,     0,   257,   258,   162,    78,     0,     0,     0,
     158,   163,   209,    75,   244,   242,   343,     0,   242,     0,
     335,   337,   348,   262,   103,     0,   104,     0,   154,     0,
     152,   148,    21,     0,    25,   272,     0,   349,     0,     0,
       0,   272,     0,   107,   273,   249,   259,     0,   159,     0,
       0,     0,     0,   186,   183,   185,     0,   330,     0,     0,
     142,     0,     0,     0,   145,   224,     0,   150,   260,     0,
     265,   109,   108,     0,     0,   266,   251,   273,   250,     0,
       0,     0,   160,     0,   169,     0,     0,   164,    59,   184,
     101,   102,     0,     0,   151,   153,   150,     0,   225,   350,
     264,     0,   132,     0,   286,   270,     0,     0,     0,     0,
       0,     0,     0,     0,   315,   307,     0,     0,   105,   110,
     110,   279,   284,     0,     0,   276,   277,   280,   308,   278,
       0,   167,   161,     0,     0,   155,     0,   156,   227,   150,
       0,     0,   272,   317,     0,     0,   313,   297,   298,   299,
       0,     0,     0,   316,     0,   314,   281,   115,     0,   116,
       0,     0,   268,   273,   267,   290,     0,     0,     0,     0,
     170,     0,   226,     0,     0,     0,    45,     0,     0,     0,
     311,   300,     0,   305,     0,     0,   113,   142,     0,   114,
     142,   285,   272,     0,     0,   165,     0,   168,     0,   269,
       0,   271,   309,   291,   295,     0,   306,     0,   111,     0,
     112,     0,   283,   274,   272,     0,     0,     0,   287,   272,
     317,   272,   312,   319,     0,   172,   174,   275,   289,   166,
     157,   272,   310,     0,   296,     0,     0,   320,   321,   301,
     288,   292,     0,   319,     0,     0,   317,     0,     0,   302,
     322,     0,   323,     0,     0,   293,   324,     0,   303,   272,
       0,     0,   294,   304,   325
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,     1,     2,     3,     5,    17,    18,    19,   199,   342,
     205,   345,    98,   276,   116,    76,   231,   248,    78,    79,
     129,    80,    81,    82,   136,    83,   187,   188,   189,   337,
     390,   391,    20,   468,   266,    51,   267,    86,   171,    23,
     114,   103,    41,    99,   104,   383,    42,   341,   193,   379,
     380,   361,   362,   536,   559,   539,   561,   180,   106,   323,
     107,    24,   122,    54,   124,    56,   119,    52,   402,   208,
     120,   121,   214,   347,   348,   117,   118,    88,   169,    89,
      90,   170,   392,   424,   190,   304,   352,   353,   354,   335,
     336,   472,   473,   474,   492,   513,   280,   514,   395,   475,
     476,   542,   491,   581,   570,   596,   609,   571,   477,   478,
     569,   479,   504,   232,   586,   587,   588,   607,   254,   255,
     268,   369,   269,   270,   271,   182,   183,   272,   273,   388
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -449
static const short int yypact[] =
{
      42,    71,    86,   762,  -449,   762,   -21,  -449,  -449,  -449,
    -449,    45,    51,    62,   -26,    70,  -449,  -449,  -449,  -449,
     225,    65,    69,  -449,  -449,  -449,  -449,  -449,  -449,  -449,
    -449,    67,  -449,    90,  -449,   102,  1885,  1804,  -449,  -449,
     225,   132,  -449,  1243,  -449,   193,  -449,  -449,  -449,   225,
    -449,   654,   394,  -449,  -449,  -449,  -449,  -449,  -449,  -449,
    -449,  1913,  1947,  -449,  1885,  1885,   394,  -449,  -449,  -449,
    1885,  -449,  -449,  1002,  -449,  -449,  1885,   114,   120,  -449,
    -449,  2083,   542,   222,  -449,   145,    -7,  -449,   180,  1434,
     357,   159,  -449,   193,  -449,   204,  -449,  1319,   265,   242,
    -449,  -449,   193,   169,  -449,   504,   350,   387,   172,  1306,
     654,  -449,  -449,  -449,  -449,  -449,   247,   203,  -449,   394,
     196,   603,  -449,   218,  -449,  1002,  -449,  1002,  -449,  1885,
    -449,  -449,  -449,  -449,   233,   250,   280,   256,  -449,   299,
    1885,  1885,  1885,  1885,  1885,  1885,  1885,  1885,  1885,  1885,
    1885,  1885,  1885,  1885,  1885,  1885,  1885,  1885,  -449,  -449,
     394,  1885,  1885,   394,  -449,  -449,  -449,    -7,  1372,  -449,
     396,   275,  -449,  -449,  -449,  -449,  -449,   387,  -449,  -449,
     353,   374,  -449,   843,  -449,   307,   322,  -449,   158,    37,
    -449,  -449,   344,   392,   174,   249,  -449,   193,  -449,   265,
     242,  -449,  1425,  -449,  -449,   265,  1885,   394,   355,   203,
    -449,  -449,  -449,   389,   354,   328,   358,  -449,   369,   375,
     385,  -449,  -449,  -449,   408,   393,  1747,  -449,  2083,  2083,
    2083,  -449,   425,  1718,  1855,  2094,   477,  1345,  1248,  1084,
     316,   316,   391,   391,  -449,  -449,  -449,  -449,   414,   120,
     409,  -449,   123,   298,  -449,   924,  -449,   410,  -449,  1478,
    -449,   275,    84,   420,  -449,  -449,   302,   681,  -449,   422,
     261,  -449,  -449,    74,  -449,  -449,    40,  -449,  -449,  -449,
    1212,   442,  -449,   350,  -449,  -449,  -449,   466,  -449,  -449,
     419,  -449,  2083,  -449,  -449,   427,  -449,  -449,  -449,  -449,
    -449,  -449,  -449,  -449,   471,  -449,  1179,  -449,  1885,  -449,
    -449,   396,  -449,  -449,  -449,  -449,  -449,   429,  -449,  -449,
    -449,  -449,   141,   415,   387,  -449,  -449,   387,  -449,  -449,
    -449,  1067,  -449,   509,   280,  -449,  -449,  -449,   437,   310,
     511,  1234,    40,  -449,  -449,    40,  -449,   451,  -449,  -449,
     451,   394,   621,   471,  -449,  -449,   495,  1885,   370,   497,
    2083,  -449,   456,  1503,  -449,   239,  -449,  1531,   208,   924,
    -449,  -449,  -449,  -449,  -449,   193,  -449,   225,   480,   127,
    -449,  -449,  -449,  1234,  -449,  -449,   235,  -449,   294,   403,
     540,   463,   702,  -449,  -449,  -449,  -449,  1967,  -449,   267,
    1234,  1059,   491,   415,  -449,  -449,   487,  -449,   304,   312,
       9,   213,   496,   511,  -449,  -449,  1885,    28,  -449,   394,
    -449,  -449,  -449,   783,   498,  -449,  -449,  -449,  -449,  1613,
    1885,  1234,  -449,  1119,  -449,  1885,   537,  -449,  -449,  -449,
    -449,  -449,   179,   501,  -449,  -449,  2061,  1885,  -449,  -449,
    -449,  1694,   541,   520,  -449,  -449,   524,   526,  1885,   549,
     514,   515,  1832,   186,   591,  -449,   566,   528,  -449,   530,
     472,  -449,   595,   864,    33,  -449,  -449,  -449,  -449,  -449,
    1993,  -449,  -449,  2012,  1234,  -449,   604,  -449,  -449,  2061,
    1885,   554,  -449,  1885,  1885,  1557,  -449,  -449,  -449,  -449,
     536,  1885,   559,  -449,   592,  -449,  -449,  -449,   193,  -449,
     225,   945,  -449,  -449,  -449,  -449,  1885,  1234,  1885,  1234,
    -449,   570,  -449,   577,  1885,   640,  1285,   580,   586,  1885,
    -449,  -449,   596,  -449,  1885,   314,  -449,    63,   340,  -449,
     436,  -449,  -449,  1694,   587,  -449,  2038,  -449,   661,  -449,
     600,  -449,  -449,  -449,  -449,  1637,  -449,   110,  -449,   280,
    -449,   280,  -449,  -449,  -449,   601,  1234,   606,  -449,  -449,
    1885,  -449,  -449,   664,   611,  -449,  -449,  -449,  -449,  -449,
    -449,  -449,  -449,   612,  -449,   619,   136,   609,  -449,  -449,
    -449,  -449,  1885,   664,   614,   664,  1885,   620,   140,  -449,
    -449,   623,  -449,   374,   618,  -449,   222,   219,  -449,  -449,
     629,   374,  -449,  -449,   222
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -449,  -449,  -449,  -449,  -449,   693,  -449,  -449,  -449,  -449,
    -449,  -449,  -449,  -449,    -6,  -449,   -36,  -449,   547,   428,
    -449,   -14,   -46,   221,  -449,  -172,  -159,   523,  -449,  -449,
     324,  -449,   -10,  -449,    11,   666,    12,   597,   556,    -9,
    -136,  -350,   -41,  -101,   -66,  -449,  -449,  -449,  -182,  -449,
     320,  -264,   377,  -449,  -449,  -449,  -449,   -42,   -70,   373,
     -18,   -32,  -449,  -449,  -449,  -449,  -449,  -449,  -449,   533,
      31,  -449,  -449,   441,   359,   626,   543,    68,   -68,   627,
     -81,  -138,   356,  -449,  -158,  -449,  -449,  -449,   401,   -24,
    -126,  -449,  -449,  -449,  -449,   -72,  -306,  -429,  -353,  -449,
     206,  -449,  -449,  -449,  -449,  -449,  -449,  -449,  -449,  -449,
    -449,   212,  -449,  -448,   165,  -449,   164,  -449,   502,  -449,
    -231,  -449,  -449,  -449,   431,  -178,  -449,  -449,  -449,  -449
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -333
static const short int yytable[] =
{
      77,    85,    43,   105,   200,    31,    33,    35,   108,   262,
     225,    45,    49,    50,    21,    22,    21,    22,   287,   115,
     194,   175,    91,   288,   313,   408,   191,   179,    87,   253,
     277,   109,   195,    36,   515,   260,    95,   135,   -94,   428,
     286,   334,    -2,   386,   166,   527,   291,   278,    28,    29,
     130,   131,   167,   168,    28,    29,   133,    26,    27,   192,
     132,   185,   138,   525,    87,    28,    29,   447,    96,    97,
     428,     4,   177,     7,    46,     9,    47,   381,   115,   200,
     174,    11,    12,    13,    91,   252,    -3,   123,   429,   135,
      95,   135,   516,   164,   228,   229,   230,    15,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,    87,   562,   311,   221,    87,   191,    87,   414,
     -94,   451,   583,   224,   283,   260,   250,   191,    30,    37,
     176,   285,   257,   191,    32,   577,   434,   437,   407,   115,
     582,   137,   584,    44,    38,    34,  -171,    48,   601,   573,
      53,   332,   590,   216,   247,   218,   333,   251,   535,   186,
     292,   318,   -97,   -97,   -97,   -97,   290,   481,   -97,   437,
     -97,   -97,   -97,    55,   166,   593,   177,    38,   100,   603,
     612,   176,   167,   168,   253,    57,   -97,   574,   404,    28,
      29,   139,   326,   219,   393,   220,    38,   100,   325,   328,
     322,   168,   140,   296,   412,   298,   174,   543,   373,   413,
      92,    38,   307,   594,    93,   176,   442,   604,    96,    97,
     520,   443,   165,   317,   349,   101,   349,   311,    38,   115,
     311,   164,   421,   102,   426,   448,   178,   501,    38,   100,
     365,   -97,    38,   320,   101,   368,   176,   196,   324,   327,
     203,   197,   102,   545,    93,   547,   485,   172,    50,   326,
     360,   486,   363,   181,   488,   426,   186,   322,   168,  -272,
    -272,  -272,  -272,   192,   416,   210,    39,  -272,  -272,  -272,
     394,   112,   113,   206,    40,   207,   101,    11,    12,    13,
     321,   338,   339,  -272,   102,   360,   610,   217,   322,   168,
     359,   611,   579,   471,    91,    38,   320,   522,   201,   202,
     222,   397,   360,   349,     7,    46,     9,    47,   382,   394,
     427,   384,    11,    12,    13,   471,   284,   223,   375,   377,
      50,   406,     7,   226,     9,   173,   409,   360,    15,   330,
      11,    12,    13,   331,   417,   387,   432,   177,   -93,   433,
     177,   427,   359,   321,   360,   360,    15,   258,   259,   410,
      38,   322,   168,   224,   176,   153,   154,   155,   156,   157,
     446,   355,   418,   356,    29,   312,   419,   227,    59,    60,
      95,    61,   440,    60,   480,   360,   197,   360,   376,   483,
     441,   274,   558,   467,    93,   436,   197,    28,    29,    62,
     275,   489,    63,   281,    64,    65,  -221,  -221,    39,   201,
     202,    66,   495,   449,    67,   467,    40,   471,   560,    68,
      69,    70,    93,   466,    71,    72,   500,   436,   282,    73,
     357,   606,   297,   575,   294,   576,   200,   299,   360,   614,
     469,   470,   155,   156,   157,   466,    96,    97,   300,   398,
      74,    75,   301,   358,   523,   258,   259,   502,   528,   508,
     510,    50,   302,    95,   308,   532,   537,  -219,  -219,   538,
     305,   360,   546,   360,   366,   367,     7,    46,     9,    47,
     544,   275,   420,   555,    11,    12,    13,   303,   550,   126,
     128,   309,   540,   310,   314,    96,    97,   319,   557,   329,
      15,   340,   343,   344,   351,   198,   346,   467,   -19,   -19,
     -19,   -19,   372,   364,   378,   374,   -19,   -19,   -19,  -173,
     360,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,    95,   -19,   385,   -31,  -142,   400,   466,   401,   411,
    -142,   186,  -252,  -106,  -106,  -106,  -106,  -106,  -106,  -106,
     509,  -106,  -106,  -106,  -106,  -106,   597,  -106,  -106,  -106,
    -106,  -106,  -106,  -106,  -106,  -106,  -106,  -106,  -106,  -106,
     438,   439,  -106,   444,  -106,  -106,   484,   450,   487,   490,
     -32,  -106,  -142,   493,  -106,   494,  -142,   -19,   496,  -106,
    -106,  -106,   497,   498,  -106,  -106,   158,   159,   503,  -106,
     160,   161,   162,   163,   211,   505,   506,     7,   507,     9,
      84,   511,   521,   524,   531,    11,    12,    13,  -106,  -106,
    -106,  -106,   389,  -106,  -272,  -272,  -272,  -272,  -272,  -272,
    -272,    15,  -272,  -272,  -272,  -272,  -272,   533,  -272,  -272,
    -272,  -272,  -272,  -272,  -272,  -272,  -272,  -272,  -272,  -272,
    -272,   534,   548,  -272,   549,  -272,  -272,   551,   553,   111,
     112,   113,  -272,   554,   565,  -272,    11,    12,    13,   567,
    -272,  -272,  -272,   585,   556,  -272,  -272,   568,   592,   578,
    -272,   212,  -213,   580,    38,     7,    46,     9,    47,   589,
     591,   595,   599,    11,    12,    13,   608,   602,    25,  -272,
     605,  -272,  -272,   186,  -272,  -272,  -272,   613,   249,    15,
    -272,  -272,   279,  -272,   526,   422,   110,  -272,   213,  -272,
    -272,  -272,  -272,  -272,  -272,  -272,  -272,  -272,  -272,  -272,
     261,  -272,   326,   445,  -272,   399,  -272,  -272,   403,   350,
     322,   168,   295,  -272,   415,   209,  -272,   423,   215,   563,
     293,  -272,  -272,  -272,   396,   564,  -272,  -272,   598,   600,
     315,  -272,   371,     6,     0,  -110,     7,     8,     9,    10,
       0,     0,     0,     0,    11,    12,    13,     0,     0,     0,
    -272,   425,  -272,  -272,   186,  -272,  -272,  -272,     0,    14,
      15,  -272,  -272,     0,  -272,     0,     0,     0,  -272,     0,
    -272,  -272,  -272,  -272,  -272,  -272,  -272,  -272,  -272,  -272,
    -272,     0,  -272,  -110,     0,  -272,     0,  -272,  -272,     0,
       0,  -110,     0,     0,  -272,     0,     0,  -272,     0,     0,
       0,     0,  -272,  -272,  -272,     0,     0,  -272,  -272,     0,
      16,     0,  -272,     0,   263,     0,   264,     7,     8,     9,
      10,     0,     0,   265,     0,    11,    12,    13,     0,     0,
       0,  -272,  -253,  -272,  -272,   512,  -272,  -272,  -272,     0,
       0,    15,  -272,  -272,     0,  -272,     0,     0,     0,  -272,
       0,  -272,  -272,  -272,  -272,  -272,  -272,  -272,  -272,  -272,
    -272,  -272,     0,  -272,     0,     0,  -272,     0,  -272,  -272,
       0,     0,     0,     0,     0,  -272,     0,     0,  -272,     0,
       0,     0,     0,  -272,  -272,  -272,     0,     0,  -272,  -272,
    -332,     0,     0,  -272,     0,   263,     0,     0,     7,     8,
       9,    10,     0,     0,   265,     0,    11,    12,    13,     0,
       0,     0,  -272,     0,  -272,  -272,   541,  -272,  -282,  -282,
       0,     0,    15,  -282,  -282,     0,  -282,     0,     0,     0,
    -282,     0,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,     0,  -282,     0,     0,  -282,     0,  -282,
    -282,     0,     0,     0,     0,     0,  -282,     0,     0,  -282,
       0,     0,     0,     0,  -282,  -282,  -282,     0,     0,  -282,
    -282,  -332,     0,   134,  -282,    58,     7,     0,     9,    84,
      59,    60,     0,    61,    11,    12,    13,     0,     0,     0,
       0,     0,     0,  -282,     0,  -282,  -282,     0,  -282,     0,
      15,    62,     0,     0,    63,     0,    64,    65,     0,     0,
       0,     0,     0,    66,     0,     0,    67,     0,     0,     0,
       0,    68,    69,    70,     0,     0,    71,    72,     0,     0,
     355,    73,   356,    29,     0,     0,     0,    59,    60,     0,
      61,     7,     8,     9,    10,     0,     0,   370,     0,    11,
      12,    13,    74,    75,     0,   -83,     0,     0,    62,     0,
       0,    63,     0,    64,    65,    15,     0,     0,     0,     0,
      66,     0,     0,    67,     0,     0,     0,     0,    68,    69,
      70,     0,     0,    71,    72,     0,     0,     0,    73,   435,
     355,     0,   356,    29,     0,     0,     0,    59,    60,     0,
      61,   151,   152,   153,   154,   155,   156,   157,  -210,    74,
      75,     0,   358,     0,     0,     0,     0,     0,    62,     0,
       0,    63,     0,    64,    65,     0,     0,     0,     0,     0,
      66,     0,     0,    67,     0,     0,     0,     0,    68,    69,
      70,     0,     0,    71,    72,     0,     0,     0,    73,   435,
     355,     0,   356,    29,     0,     0,     0,    59,    60,     0,
      61,     0,     0,     0,     0,     0,     0,     0,   482,    74,
      75,     0,   358,     0,     0,     0,     0,     0,    62,     0,
       0,    63,     0,    64,    65,     0,     7,     8,     9,    10,
      66,     0,     0,    67,    11,    12,    13,     0,    68,    69,
      70,     0,     0,    71,    72,   355,     0,    58,    73,   357,
      15,     0,    59,    60,    94,    61,     0,   -27,   -27,   -27,
     -27,     0,     0,     0,     0,   -27,   -27,   -27,     0,    74,
      75,     0,   358,    62,     0,     0,    63,     0,    64,    65,
      95,   -27,     0,     0,  -142,    66,     0,     0,    67,  -142,
       0,     0,     0,    68,    69,    70,     0,     0,    71,    72,
       0,     0,     0,    73,   150,   151,   152,   153,   154,   155,
     156,   157,    96,    97,     0,     0,     0,   204,     0,     0,
     -23,   -23,   -23,   -23,    74,    75,     0,   358,   -23,   -23,
     -23,  -142,    58,     0,     0,  -142,   -27,    59,    60,     0,
      61,     0,     0,    95,   -23,     0,     0,  -142,     0,   158,
     159,     0,  -142,   160,   161,   162,   163,     0,    62,     0,
       0,    63,     0,    64,    65,     0,     0,     0,     0,     0,
      66,     0,   552,    67,     0,    96,    97,     0,    68,    69,
      70,     0,     0,    71,    72,    58,     0,     0,    73,     0,
      59,    60,     0,    61,  -142,     0,     0,     0,  -142,   -23,
     149,   150,   151,   152,   153,   154,   155,   156,   157,    74,
      75,    62,     0,   184,    63,     0,    64,    65,     0,     0,
       0,     0,     0,    66,     0,     0,    67,     0,     0,     0,
       0,    68,    69,    70,     0,     0,    71,    72,    58,     0,
       0,    73,     0,    59,    60,     0,    61,     0,     7,     0,
       9,   173,     0,     0,     0,     0,    11,    12,    13,     0,
       0,     0,    74,    75,    62,     0,   256,    63,     0,    64,
      65,     0,    15,     0,     0,     0,    66,     0,     0,    67,
       0,     0,     0,     0,    68,    69,    70,     0,     0,    71,
      72,    58,     0,     0,    73,   166,    59,    60,     0,    61,
       0,     0,     0,   167,   168,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    74,    75,    62,     0,   289,
      63,     0,    64,    65,     0,     0,     0,     0,     0,    66,
       0,     0,    67,     0,     0,     0,     0,    68,    69,    70,
       0,     0,    71,    72,    58,     0,     0,    73,     0,    59,
      60,   143,    61,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,     0,    74,    75,
      62,     0,   316,    63,     0,    64,    65,   529,     0,     0,
       0,     0,    66,     0,     0,    67,     0,     0,     0,     0,
      68,    69,    70,     0,     0,    71,    72,     0,     0,     0,
      73,     0,     0,   141,   142,   143,   530,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,    74,    75,     0,     0,   405,   356,   452,     8,     9,
      10,    59,    60,     0,    61,    11,    12,    13,   453,     0,
     454,   455,   456,   457,   458,   459,   460,   461,   462,   463,
     464,    15,    62,     0,     0,    63,     0,    64,    65,     0,
       0,     0,     0,     0,    66,     0,     0,    67,     0,     0,
       0,     0,    68,    69,    70,     0,     0,    71,    72,     0,
       0,     0,    73,   141,   142,   143,   572,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,   465,     0,    74,    75,     0,   224,   356,    29,     0,
       0,     0,    59,    60,     0,    61,     0,     0,     0,   453,
       0,   454,   455,   456,   457,   458,   459,   460,   461,   462,
     463,   464,     0,    62,     0,     0,    63,     0,    64,    65,
       0,     0,     0,     0,     0,    66,     0,     0,    67,     0,
       0,     0,     0,    68,    69,    70,     0,     0,    71,    72,
      58,     0,     0,    73,     0,    59,    60,     0,    61,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,   465,     0,    74,    75,    62,   224,     0,    63,
       0,    64,    65,     0,     0,     0,     0,     0,    66,     0,
       0,    67,     0,     0,     0,     0,    68,    69,    70,     0,
       0,    71,    72,     0,     0,     0,    73,    58,     7,     0,
       9,    84,    59,    60,     0,    61,    11,    12,    13,     0,
       0,     0,     0,     0,     0,     0,     0,    74,    75,     0,
     306,     0,    15,    62,     0,    58,    63,     0,    64,    65,
      59,    60,     0,    61,     0,    66,     0,     0,    67,     0,
       0,     0,     0,    68,    69,    70,     0,     0,    71,    72,
       0,    62,     0,    73,    63,     0,    64,    65,     0,     0,
       0,     0,     0,    66,     0,     0,    67,     0,     0,     0,
       0,    68,    69,    70,    74,    75,    71,    72,    58,     0,
       0,    73,     0,    59,    60,     0,    61,   146,   147,   148,
     149,   150,   151,   152,   153,   154,   155,   156,   157,     0,
     499,     0,    74,    75,    62,     0,    58,    63,     0,    64,
      65,    59,    60,     0,    61,     0,    66,     0,     0,    67,
       0,     0,     0,     0,    68,    69,    70,     0,     0,    71,
      72,     0,    62,     0,    73,    63,     0,    64,    65,     0,
      58,     0,     0,     0,    66,    59,    60,    67,    61,     0,
       0,     0,    68,    69,    70,    74,    75,    71,    72,     0,
       0,     0,   125,     0,     0,     0,    62,   430,     0,    63,
       0,    64,    65,     0,     0,     0,     0,     0,    66,     0,
       0,    67,     0,    74,    75,     0,    68,    69,    70,     0,
       0,    71,    72,   141,   142,   143,   127,   144,   145,   146,
     147,   148,   149,   150,   151,   152,   153,   154,   155,   156,
     157,     0,   518,     0,     0,     0,     0,    74,    75,   141,
     142,   143,     0,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,     0,   141,   142,
     143,   431,   144,   145,   146,   147,   148,   149,   150,   151,
     152,   153,   154,   155,   156,   157,     0,     0,     0,     0,
       0,     0,     0,     0,   141,   142,   143,   517,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,   192,     0,     0,     0,   519,   141,   142,   143,
       0,   144,   145,   146,   147,   148,   149,   150,   151,   152,
     153,   154,   155,   156,   157,     0,     0,     0,     0,   141,
     142,   143,   566,   144,   145,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   157,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   156,   157
};

static const short int yycheck[] =
{
      36,    37,    20,    45,   105,    11,    12,    13,    49,   181,
     136,    21,    22,    22,     3,     3,     5,     5,   200,    51,
     101,    89,    40,   201,   255,   375,    98,    93,    37,   167,
     188,    49,   102,    59,     1,   171,    27,    73,     1,   392,
     199,     1,     0,   349,    51,   493,   205,    10,     3,     4,
      64,    65,    59,    60,     3,     4,    70,    78,    79,    31,
      66,    97,    76,   492,    73,     3,     4,    39,    59,    60,
     423,     0,    90,     4,     5,     6,     7,   341,   110,   180,
      89,    12,    13,    14,   102,   166,     0,    56,   394,   125,
      27,   127,    59,     9,   140,   141,   142,    28,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,   121,   542,   252,   129,   125,   189,   127,   383,
      83,   427,   570,    83,   194,   261,   162,   199,    83,    59,
       7,   197,   168,   205,    83,   564,   400,   401,   369,   171,
     569,    73,   571,    78,     3,    83,    83,    78,   596,    39,
      83,    77,   581,   122,   160,   124,    82,   163,   508,     1,
     206,    77,     4,     5,     6,     7,   202,   431,    10,   433,
      12,    13,    14,    83,    51,    39,   194,     3,     4,    39,
     609,     7,    59,    60,   322,    83,    28,    77,   366,     3,
       4,    77,    51,   125,   352,   127,     3,     4,   266,   267,
      59,    60,    82,   213,    77,   215,   215,   513,   334,    82,
      78,     3,   226,    77,    82,     7,     3,    77,    59,    60,
     484,     8,    77,   259,   296,    51,   298,   365,     3,   261,
     368,     9,   390,    59,   392,   417,    77,    51,     3,     4,
     321,    83,     3,     4,    51,   326,     7,    78,   266,   267,
      78,    82,    59,   517,    82,   519,    77,    77,   267,    51,
     306,    82,   308,    59,   446,   423,     1,    59,    60,     4,
       5,     6,     7,    31,    39,    79,    51,    12,    13,    14,
     352,     6,     7,    36,    59,    82,    51,    12,    13,    14,
      51,   280,   280,    28,    59,   341,    77,    79,    59,    60,
     306,    82,   566,   429,   322,     3,     4,   489,    59,    60,
      77,   357,   358,   385,     4,     5,     6,     7,   342,   391,
     392,   345,    12,    13,    14,   451,    77,    77,   338,   339,
     339,   367,     4,    77,     6,     7,   377,   383,    28,    78,
      12,    13,    14,    82,   386,   351,    79,   365,    83,    82,
     368,   423,   358,    51,   400,   401,    28,    59,    60,   377,
       3,    59,    60,    83,     7,    49,    50,    51,    52,    53,
     416,     1,    78,     3,     4,    77,    82,    78,     8,     9,
      27,    11,    78,     9,   430,   431,    82,   433,    78,   435,
      78,    84,    78,   429,    82,   401,    82,     3,     4,    29,
      78,   447,    32,    59,    34,    35,    78,    79,    51,    59,
      60,    41,   458,   419,    44,   451,    59,   543,    78,    49,
      50,    51,    82,   429,    54,    55,   462,   433,    36,    59,
      60,   603,    78,   559,    79,   561,   537,    79,   484,   611,
     429,   429,    51,    52,    53,   451,    59,    60,    79,    79,
      80,    81,    77,    83,   490,    59,    60,   463,   494,   469,
     470,   470,    77,    27,    39,   501,   508,    78,    79,   510,
      77,   517,   518,   519,    59,    60,     4,     5,     6,     7,
     516,    78,    79,   529,    12,    13,    14,    79,   524,    61,
      62,    77,   510,    84,    84,    59,    60,    77,   534,    77,
      28,    59,    36,    84,    33,     1,    79,   543,     4,     5,
       6,     7,     3,    84,     3,    78,    12,    13,    14,    83,
     566,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    27,    28,    82,    39,    31,    39,   543,    82,    59,
      36,     1,    79,     3,     4,     5,     6,     7,     8,     9,
      78,    11,    12,    13,    14,    15,   592,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      79,    84,    32,    77,    34,    35,    39,    79,    77,    59,
      39,    41,    78,    59,    44,    59,    82,    83,    39,    49,
      50,    51,    78,    78,    54,    55,    54,    55,     7,    59,
      58,    59,    60,    61,     1,    39,    78,     4,    78,     6,
       7,    16,     8,    59,    78,    12,    13,    14,    78,    79,
      80,    81,     1,    83,     3,     4,     5,     6,     7,     8,
       9,    28,    11,    12,    13,    14,    15,    78,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    59,    82,    32,    77,    34,    35,    17,    78,     5,
       6,     7,    41,    77,    77,    44,    12,    13,    14,     8,
      49,    50,    51,     9,    78,    54,    55,    77,    59,    78,
      59,    78,    79,    77,     3,     4,     5,     6,     7,    78,
      78,    82,    78,    12,    13,    14,    78,    77,     5,    78,
      77,    80,    81,     1,    83,     3,     4,    78,   161,    28,
       8,     9,   189,    11,   493,   391,    50,    15,   121,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
     174,    29,    51,   413,    32,   358,    34,    35,   365,   298,
      59,    60,   209,    41,   385,   119,    44,   391,   121,   543,
     207,    49,    50,    51,   353,   543,    54,    55,   593,   595,
     258,    59,   331,     1,    -1,     3,     4,     5,     6,     7,
      -1,    -1,    -1,    -1,    12,    13,    14,    -1,    -1,    -1,
      78,    79,    80,    81,     1,    83,     3,     4,    -1,    27,
      28,     8,     9,    -1,    11,    -1,    -1,    -1,    15,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    -1,    29,    51,    -1,    32,    -1,    34,    35,    -1,
      -1,    59,    -1,    -1,    41,    -1,    -1,    44,    -1,    -1,
      -1,    -1,    49,    50,    51,    -1,    -1,    54,    55,    -1,
      78,    -1,    59,    -1,     1,    -1,     3,     4,     5,     6,
       7,    -1,    -1,    10,    -1,    12,    13,    14,    -1,    -1,
      -1,    78,    79,    80,    81,     1,    83,     3,     4,    -1,
      -1,    28,     8,     9,    -1,    11,    -1,    -1,    -1,    15,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    -1,    29,    -1,    -1,    32,    -1,    34,    35,
      -1,    -1,    -1,    -1,    -1,    41,    -1,    -1,    44,    -1,
      -1,    -1,    -1,    49,    50,    51,    -1,    -1,    54,    55,
      77,    -1,    -1,    59,    -1,     1,    -1,    -1,     4,     5,
       6,     7,    -1,    -1,    10,    -1,    12,    13,    14,    -1,
      -1,    -1,    78,    -1,    80,    81,     1,    83,     3,     4,
      -1,    -1,    28,     8,     9,    -1,    11,    -1,    -1,    -1,
      15,    -1,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    -1,    29,    -1,    -1,    32,    -1,    34,
      35,    -1,    -1,    -1,    -1,    -1,    41,    -1,    -1,    44,
      -1,    -1,    -1,    -1,    49,    50,    51,    -1,    -1,    54,
      55,    77,    -1,     1,    59,     3,     4,    -1,     6,     7,
       8,     9,    -1,    11,    12,    13,    14,    -1,    -1,    -1,
      -1,    -1,    -1,    78,    -1,    80,    81,    -1,    83,    -1,
      28,    29,    -1,    -1,    32,    -1,    34,    35,    -1,    -1,
      -1,    -1,    -1,    41,    -1,    -1,    44,    -1,    -1,    -1,
      -1,    49,    50,    51,    -1,    -1,    54,    55,    -1,    -1,
       1,    59,     3,     4,    -1,    -1,    -1,     8,     9,    -1,
      11,     4,     5,     6,     7,    -1,    -1,    10,    -1,    12,
      13,    14,    80,    81,    -1,    83,    -1,    -1,    29,    -1,
      -1,    32,    -1,    34,    35,    28,    -1,    -1,    -1,    -1,
      41,    -1,    -1,    44,    -1,    -1,    -1,    -1,    49,    50,
      51,    -1,    -1,    54,    55,    -1,    -1,    -1,    59,    60,
       1,    -1,     3,     4,    -1,    -1,    -1,     8,     9,    -1,
      11,    47,    48,    49,    50,    51,    52,    53,    79,    80,
      81,    -1,    83,    -1,    -1,    -1,    -1,    -1,    29,    -1,
      -1,    32,    -1,    34,    35,    -1,    -1,    -1,    -1,    -1,
      41,    -1,    -1,    44,    -1,    -1,    -1,    -1,    49,    50,
      51,    -1,    -1,    54,    55,    -1,    -1,    -1,    59,    60,
       1,    -1,     3,     4,    -1,    -1,    -1,     8,     9,    -1,
      11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,    80,
      81,    -1,    83,    -1,    -1,    -1,    -1,    -1,    29,    -1,
      -1,    32,    -1,    34,    35,    -1,     4,     5,     6,     7,
      41,    -1,    -1,    44,    12,    13,    14,    -1,    49,    50,
      51,    -1,    -1,    54,    55,     1,    -1,     3,    59,    60,
      28,    -1,     8,     9,     1,    11,    -1,     4,     5,     6,
       7,    -1,    -1,    -1,    -1,    12,    13,    14,    -1,    80,
      81,    -1,    83,    29,    -1,    -1,    32,    -1,    34,    35,
      27,    28,    -1,    -1,    31,    41,    -1,    -1,    44,    36,
      -1,    -1,    -1,    49,    50,    51,    -1,    -1,    54,    55,
      -1,    -1,    -1,    59,    46,    47,    48,    49,    50,    51,
      52,    53,    59,    60,    -1,    -1,    -1,     1,    -1,    -1,
       4,     5,     6,     7,    80,    81,    -1,    83,    12,    13,
      14,    78,     3,    -1,    -1,    82,    83,     8,     9,    -1,
      11,    -1,    -1,    27,    28,    -1,    -1,    31,    -1,    54,
      55,    -1,    36,    58,    59,    60,    61,    -1,    29,    -1,
      -1,    32,    -1,    34,    35,    -1,    -1,    -1,    -1,    -1,
      41,    -1,    77,    44,    -1,    59,    60,    -1,    49,    50,
      51,    -1,    -1,    54,    55,     3,    -1,    -1,    59,    -1,
       8,     9,    -1,    11,    78,    -1,    -1,    -1,    82,    83,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    80,
      81,    29,    -1,    84,    32,    -1,    34,    35,    -1,    -1,
      -1,    -1,    -1,    41,    -1,    -1,    44,    -1,    -1,    -1,
      -1,    49,    50,    51,    -1,    -1,    54,    55,     3,    -1,
      -1,    59,    -1,     8,     9,    -1,    11,    -1,     4,    -1,
       6,     7,    -1,    -1,    -1,    -1,    12,    13,    14,    -1,
      -1,    -1,    80,    81,    29,    -1,    84,    32,    -1,    34,
      35,    -1,    28,    -1,    -1,    -1,    41,    -1,    -1,    44,
      -1,    -1,    -1,    -1,    49,    50,    51,    -1,    -1,    54,
      55,     3,    -1,    -1,    59,    51,     8,     9,    -1,    11,
      -1,    -1,    -1,    59,    60,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    81,    29,    -1,    84,
      32,    -1,    34,    35,    -1,    -1,    -1,    -1,    -1,    41,
      -1,    -1,    44,    -1,    -1,    -1,    -1,    49,    50,    51,
      -1,    -1,    54,    55,     3,    -1,    -1,    59,    -1,     8,
       9,    38,    11,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    -1,    80,    81,
      29,    -1,    84,    32,    -1,    34,    35,    10,    -1,    -1,
      -1,    -1,    41,    -1,    -1,    44,    -1,    -1,    -1,    -1,
      49,    50,    51,    -1,    -1,    54,    55,    -1,    -1,    -1,
      59,    -1,    -1,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    80,    81,    -1,    -1,    84,     3,     4,     5,     6,
       7,     8,     9,    -1,    11,    12,    13,    14,    15,    -1,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    -1,    -1,    32,    -1,    34,    35,    -1,
      -1,    -1,    -1,    -1,    41,    -1,    -1,    44,    -1,    -1,
      -1,    -1,    49,    50,    51,    -1,    -1,    54,    55,    -1,
      -1,    -1,    59,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    78,    -1,    80,    81,    -1,    83,     3,     4,    -1,
      -1,    -1,     8,     9,    -1,    11,    -1,    -1,    -1,    15,
      -1,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    -1,    29,    -1,    -1,    32,    -1,    34,    35,
      -1,    -1,    -1,    -1,    -1,    41,    -1,    -1,    44,    -1,
      -1,    -1,    -1,    49,    50,    51,    -1,    -1,    54,    55,
       3,    -1,    -1,    59,    -1,     8,     9,    -1,    11,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    78,    -1,    80,    81,    29,    83,    -1,    32,
      -1,    34,    35,    -1,    -1,    -1,    -1,    -1,    41,    -1,
      -1,    44,    -1,    -1,    -1,    -1,    49,    50,    51,    -1,
      -1,    54,    55,    -1,    -1,    -1,    59,     3,     4,    -1,
       6,     7,     8,     9,    -1,    11,    12,    13,    14,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    81,    -1,
      83,    -1,    28,    29,    -1,     3,    32,    -1,    34,    35,
       8,     9,    -1,    11,    -1,    41,    -1,    -1,    44,    -1,
      -1,    -1,    -1,    49,    50,    51,    -1,    -1,    54,    55,
      -1,    29,    -1,    59,    32,    -1,    34,    35,    -1,    -1,
      -1,    -1,    -1,    41,    -1,    -1,    44,    -1,    -1,    -1,
      -1,    49,    50,    51,    80,    81,    54,    55,     3,    -1,
      -1,    59,    -1,     8,     9,    -1,    11,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    -1,
      78,    -1,    80,    81,    29,    -1,     3,    32,    -1,    34,
      35,     8,     9,    -1,    11,    -1,    41,    -1,    -1,    44,
      -1,    -1,    -1,    -1,    49,    50,    51,    -1,    -1,    54,
      55,    -1,    29,    -1,    59,    32,    -1,    34,    35,    -1,
       3,    -1,    -1,    -1,    41,     8,     9,    44,    11,    -1,
      -1,    -1,    49,    50,    51,    80,    81,    54,    55,    -1,
      -1,    -1,    59,    -1,    -1,    -1,    29,    10,    -1,    32,
      -1,    34,    35,    -1,    -1,    -1,    -1,    -1,    41,    -1,
      -1,    44,    -1,    80,    81,    -1,    49,    50,    51,    -1,
      -1,    54,    55,    36,    37,    38,    59,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    -1,    10,    -1,    -1,    -1,    -1,    80,    81,    36,
      37,    38,    -1,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    -1,    36,    37,
      38,    84,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    36,    37,    38,    84,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    31,    -1,    -1,    -1,    84,    36,    37,    38,
      -1,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    -1,    -1,    -1,    -1,    36,
      37,    38,    84,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    86,    87,    88,     0,    89,     1,     4,     5,     6,
       7,    12,    13,    14,    27,    28,    78,    90,    91,    92,
     117,   119,   121,   124,   146,    90,    78,    79,     3,     4,
      83,    99,    83,    99,    83,    99,    59,    59,     3,    51,
      59,   127,   131,   145,    78,   117,     5,     7,    78,   117,
     124,   120,   152,    83,   148,    83,   150,    83,     3,     8,
       9,    11,    29,    32,    34,    35,    41,    44,    49,    50,
      51,    54,    55,    59,    80,    81,   100,   101,   103,   104,
     106,   107,   108,   110,     7,   101,   122,   124,   162,   164,
     165,   145,    78,    82,     1,    27,    59,    60,    97,   128,
       4,    51,    59,   126,   129,   142,   143,   145,   127,   145,
     120,     5,     6,     7,   125,   146,    99,   160,   161,   151,
     155,   156,   147,   155,   149,    59,   104,    59,   104,   105,
     106,   106,    99,   106,     1,   101,   109,   162,   106,    77,
      82,    36,    37,    38,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      58,    59,    60,    61,     9,    77,    51,    59,    60,   163,
     166,   123,    77,     7,   124,   163,     7,   145,    77,   129,
     142,    59,   210,   211,    84,   101,     1,   111,   112,   113,
     169,   180,    31,   133,   165,   143,    78,    82,     1,    93,
     128,    59,    60,    78,     1,    95,    36,    82,   154,   160,
      79,     1,    78,   122,   157,   164,   155,    79,   155,   162,
     162,   106,    77,    77,    83,   175,    77,    78,   107,   107,
     107,   101,   198,   107,   107,   107,   107,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   107,    99,   102,   103,
     101,    99,   165,   166,   203,   204,    84,   101,    59,    60,
     125,   123,   110,     1,     3,    10,   119,   121,   205,   207,
     208,   209,   212,   213,    84,    78,    98,   169,    10,   112,
     181,    59,    36,   143,    77,   129,   111,   133,   210,    84,
     101,   111,   107,   161,    79,   154,   117,    78,   117,    79,
      79,    77,    77,    79,   170,    77,    83,   106,    39,    77,
      84,   166,    77,   205,    84,   203,    84,   101,    77,    77,
       4,    51,    59,   144,   145,   163,    51,   145,   163,    77,
      78,    82,    77,    82,     1,   174,   175,   114,   119,   121,
      59,   132,    94,    36,    84,    96,    79,   158,   159,   180,
     158,    33,   171,   172,   173,     1,     3,    60,    83,    99,
     107,   136,   137,   107,    84,   165,    59,    60,   165,   206,
      10,   209,     3,   175,    78,   117,    78,   117,     3,   134,
     135,   136,   174,   130,   174,    82,   181,    99,   214,     1,
     115,   116,   167,   169,   180,   183,   173,   107,    79,   137,
      39,    82,   153,   144,   210,    84,   101,   205,   126,   127,
     145,    59,    77,    82,   136,   159,    39,   142,    78,    82,
      79,   169,   115,   167,   168,    79,   169,   180,   183,   181,
      10,    84,    79,    82,   136,    60,    99,   136,    79,    84,
      78,    78,     3,     8,    77,   135,   107,    39,   133,    99,
      79,   181,     4,    15,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    78,    99,   101,   118,   119,
     121,   175,   176,   177,   178,   184,   185,   193,   194,   196,
     107,   136,    79,   107,    39,    77,    82,    77,   133,   107,
      59,   187,   179,    59,    59,   107,    39,    78,    78,    78,
     101,    51,    99,     7,   197,    39,    78,    78,   117,    78,
     117,    16,     1,   180,   182,     1,    59,    84,    10,    84,
     136,     8,   133,   101,    59,   182,   108,   198,   101,    10,
      39,    78,   101,    78,    59,   126,   138,   142,   127,   140,
     145,     1,   186,   181,   101,   136,   107,   136,    82,    77,
     101,    17,    77,    78,    77,   107,    78,   101,    78,   139,
      78,   141,   182,   185,   196,    77,    84,     8,    77,   195,
     189,   192,    39,    39,    77,   175,   175,   182,    78,   136,
      77,   188,   182,   198,   182,     9,   199,   200,   201,    78,
     182,    78,    59,    39,    77,    82,   190,   101,   199,    78,
     201,   198,    77,    39,    77,    77,   110,   202,    78,   191,
      77,    82,   182,    78,   110
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)		\
   ((Current).first_line   = (Rhs)[1].first_line,	\
    (Current).first_column = (Rhs)[1].first_column,	\
    (Current).last_line    = (Rhs)[N].last_line,	\
    (Current).last_column  = (Rhs)[N].last_column)
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if defined (YYMAXDEPTH) && YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  register short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;


  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 270 "c-parse.y"
    { if (pedantic)
		    pedwarn ("ANSI C forbids an empty source file");
		;}
    break;

  case 3:
#line 274 "c-parse.y"
    {
		  /* In case there were missing closebraces,
		     get us back to the global binding level.  */
		  while (! global_bindings_p ())
		    poplevel (0, 0, 0);
		;}
    break;

  case 4:
#line 287 "c-parse.y"
    {yyval.ttype = NULL_TREE; ;}
    break;

  case 6:
#line 288 "c-parse.y"
    {yyval.ttype = NULL_TREE; ;}
    break;

  case 10:
#line 295 "c-parse.y"
    { STRIP_NOPS (yyvsp[-2].ttype);
		  if ((TREE_CODE (yyvsp[-2].ttype) == ADDR_EXPR
		       && TREE_CODE (TREE_OPERAND (yyvsp[-2].ttype, 0)) == STRING_CST)
		      || TREE_CODE (yyvsp[-2].ttype) == STRING_CST)
		    assemble_asm (yyvsp[-2].ttype);
		  else
		    error ("argument of `asm' is not a constant string"); ;}
    break;

  case 11:
#line 306 "c-parse.y"
    { if (pedantic)
		    error ("ANSI C forbids data definition with no type or storage class");
		  else if (!flag_traditional)
		    warning ("data definition has no type or storage class"); ;}
    break;

  case 12:
#line 311 "c-parse.y"
    {;}
    break;

  case 13:
#line 313 "c-parse.y"
    {;}
    break;

  case 14:
#line 315 "c-parse.y"
    { pedwarn ("empty declaration"); ;}
    break;

  case 15:
#line 317 "c-parse.y"
    { shadow_tag (yyvsp[-1].ttype); ;}
    break;

  case 18:
#line 321 "c-parse.y"
    { if (pedantic)
		    pedwarn ("ANSI C does not allow extra `;' outside of a function"); ;}
    break;

  case 19:
#line 327 "c-parse.y"
    { if (! start_function (yyvsp[-2].ttype, yyvsp[0].ttype, 0))
		    YYERROR1;
		  Gct_function_hashval = 0;	/*** GCT ***/
		  reinit_parse_for_function (); ;}
    break;

  case 20:
#line 332 "c-parse.y"
    { store_parm_decls ();
		  gct_parse_decls();   ;}
    break;

  case 21:
#line 335 "c-parse.y"
    { /* debug_dump_tree(current_function_decl); */	/*** GCT ***/
		  gct_ignore_decls();				/*** GCT ***/
		  gct_transform_function(GCT_LAST(Gct_all_nodes)); /** GCT **/
		  finish_function (0); ;}
    break;

  case 22:
#line 340 "c-parse.y"
    { ;}
    break;

  case 23:
#line 342 "c-parse.y"
    { if (! start_function (yyvsp[-2].ttype, yyvsp[0].ttype, 0))
		    YYERROR1;
		  Gct_function_hashval = 0;	/*** GCT ***/
		  reinit_parse_for_function (); ;}
    break;

  case 24:
#line 347 "c-parse.y"
    { store_parm_decls ();
		  gct_parse_decls();   ;}
    break;

  case 25:
#line 350 "c-parse.y"
    { /* debug_dump_tree(current_function_decl); */	/*** GCT ***/
		  gct_transform_function(GCT_LAST(Gct_all_nodes)); /** GCT **/
		  gct_ignore_decls();				/*** GCT ***/
		  finish_function (0); ;}
    break;

  case 26:
#line 355 "c-parse.y"
    { ;}
    break;

  case 27:
#line 357 "c-parse.y"
    { if (! start_function (NULL_TREE, yyvsp[0].ttype, 0))
		    YYERROR1;
		  Gct_function_hashval = 0;	/*** GCT ***/
		  reinit_parse_for_function (); ;}
    break;

  case 28:
#line 362 "c-parse.y"
    { store_parm_decls ();
		  gct_parse_decls();  ;}
    break;

  case 29:
#line 365 "c-parse.y"
    { /* debug_dump_tree(current_function_decl); */	/*** GCT ***/
		  gct_transform_function(GCT_LAST(Gct_all_nodes)); /** GCT **/
		  gct_ignore_decls();				/*** GCT ***/
		  finish_function (0); ;}
    break;

  case 30:
#line 370 "c-parse.y"
    { ;}
    break;

  case 33:
#line 379 "c-parse.y"
    { yyval.code = ADDR_EXPR; ;}
    break;

  case 34:
#line 381 "c-parse.y"
    { yyval.code = NEGATE_EXPR; ;}
    break;

  case 35:
#line 383 "c-parse.y"
    { yyval.code = CONVERT_EXPR; ;}
    break;

  case 36:
#line 385 "c-parse.y"
    { yyval.code = PREINCREMENT_EXPR; ;}
    break;

  case 37:
#line 387 "c-parse.y"
    { yyval.code = PREDECREMENT_EXPR; ;}
    break;

  case 38:
#line 389 "c-parse.y"
    { yyval.code = BIT_NOT_EXPR; ;}
    break;

  case 39:
#line 391 "c-parse.y"
    { yyval.code = TRUTH_NOT_EXPR; ;}
    break;

  case 40:
#line 395 "c-parse.y"
    { if (GCT_COMMA == GCT_LAST(Gct_all_nodes)->prev->type)	/*** GCT ***/
		    { gct_guard_comma(GCT_LAST(Gct_all_nodes)->prev); }	/*** GCT ***/
		  yyval.ttype = build_compound_expr (yyvsp[0].ttype); ;}
    break;

  case 41:
#line 402 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 43:
#line 408 "c-parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 44:
#line 410 "c-parse.y"
    { chainon (yyvsp[-2].ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype));
		  gct_build_comma_list(GCT_LAST(Gct_all_nodes)->prev->prev,yyvsp[-2].ttype); ;}
    break;

  case 46:
#line 417 "c-parse.y"
    { yyval.ttype = build_indirect_ref (yyvsp[0].ttype, "unary *");
		  gct_build_unary(GCT_LAST(Gct_all_nodes)->prev->prev, 
				  GCT_DEREFERENCE, yyval.ttype); /*** GCT ***/
                ;}
    break;

  case 47:
#line 423 "c-parse.y"
    { yyvsp[0].itype = pedantic;
		  pedantic = 0; ;}
    break;

  case 48:
#line 426 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  pedantic = yyvsp[-2].itype; 
                  gct_build_unary(GCT_LAST(Gct_all_nodes)->prev->prev, 
                                  GCT_EXTENSION,yyval.ttype); /*** GCT ***/ ;}
    break;

  case 49:
#line 431 "c-parse.y"
    { yyval.ttype = build_unary_op (yyvsp[-1].code, yyvsp[0].ttype, 0);
		  gct_build_unary_by_gcctype(GCT_LAST_MAYBE_SHIFT(Gct_all_nodes)->prev, yyvsp[-1].code, yyval.ttype); /*** GCT ***/
                  overflow_warning (yyval.ttype); ;}
    break;

  case 50:
#line 436 "c-parse.y"
    { tree label = lookup_label (yyvsp[0].ttype);
		  if (label == 0)
		    yyval.ttype = null_pointer_node;
		  else
		    {
		      TREE_USED (label) = 1;
		      yyval.ttype = build1 (ADDR_EXPR, ptr_type_node, label);
		      TREE_CONSTANT (yyval.ttype) = 1;
		    }
		;}
    break;

  case 51:
#line 462 "c-parse.y"
    { if (TREE_CODE (yyvsp[0].ttype) == COMPONENT_REF
		      && DECL_BIT_FIELD (TREE_OPERAND (yyvsp[0].ttype, 1)))
		    error ("`sizeof' applied to a bit-field");
		  yyval.ttype = c_sizeof (TREE_TYPE (yyvsp[0].ttype));
		  gct_build_unary(GCT_LAST(Gct_all_nodes)->prev->prev, 
				  GCT_SIZEOF, yyval.ttype); /*** GCT ***/
                ;}
    break;

  case 52:
#line 470 "c-parse.y"
    { yyval.ttype = c_sizeof (groktypename (yyvsp[-1].ttype)); 
                  /*** GCT ***/
		  gct_build_of(GCT_LAST(Gct_all_nodes), GCT_SIZEOF, yyval.ttype); 
		;}
    break;

  case 53:
#line 475 "c-parse.y"
    { yyval.ttype = c_alignof_expr (yyvsp[0].ttype); 
		  /*** GCT ***/
		  gct_build_unary(GCT_LAST(Gct_all_nodes)->prev->prev,
				  GCT_ALIGNOF, yyval.ttype); 
                ;}
    break;

  case 54:
#line 481 "c-parse.y"
    { yyval.ttype = c_alignof (groktypename (yyvsp[-1].ttype)); 
		  /*** GCT ***/
                  gct_build_of(GCT_LAST(Gct_all_nodes), GCT_ALIGNOF, yyval.ttype);
                ;}
    break;

  case 55:
#line 486 "c-parse.y"
    { yyval.ttype = build_unary_op (REALPART_EXPR, yyvsp[0].ttype, 0); 
		  error("GCT does not yet understand complex numbers.");;}
    break;

  case 56:
#line 489 "c-parse.y"
    { yyval.ttype = build_unary_op (IMAGPART_EXPR, yyvsp[0].ttype, 0); 
		   error("GCT does not yet understand complex numbers.");;}
    break;

  case 58:
#line 496 "c-parse.y"
    { tree type = groktypename (yyvsp[-2].ttype);
		  yyval.ttype = build_c_cast (type, yyvsp[0].ttype); 
		  /*** GCT ***/
                  gct_build_cast(GCT_LAST_MAYBE_SHIFT(Gct_all_nodes), yyval.ttype); 
                ;}
    break;

  case 59:
#line 502 "c-parse.y"
    { tree type = groktypename (yyvsp[-5].ttype);
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
		  yyval.ttype = digest_init (type, build_nt (CONSTRUCTOR, NULL_TREE, nreverse (yyvsp[-2].ttype)),
				    NULL_PTR, 0, 0, name);
		  if (TREE_CODE (type) == ARRAY_TYPE && TYPE_SIZE (type) == 0)
		    {
		      int failure = complete_array_type (type, yyval.ttype, 1);
		      if (failure)
			abort ();
		    }
#endif /*** GCT ***/
		;}
    break;

  case 61:
#line 538 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev, 
                  GCT_PLUS, yyval.ttype); 
                ;}
    break;

  case 62:
#line 544 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_MINUS, yyval.ttype);
                ;}
    break;

  case 63:
#line 550 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_TIMES, yyval.ttype);
                ;}
    break;

  case 64:
#line 556 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_DIV, yyval.ttype);
                ;}
    break;

  case 65:
#line 562 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_MOD, yyval.ttype);
                ;}
    break;

  case 66:
#line 568 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_LSHIFT, yyval.ttype);
                ;}
    break;

  case 67:
#line 574 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype);
		/* GCT: There's a lookahead token on the tokenlist. */ 
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_RSHIFT, yyval.ttype);
                ;}
    break;

  case 68:
#line 580 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_arithcompare(GCT_LAST(Gct_all_nodes)->prev->prev, 
					 yyval.ttype);
                ;}
    break;

  case 69:
#line 586 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_eqcompare(GCT_LAST(Gct_all_nodes)->prev->prev, yyval.ttype);
                ;}
    break;

  case 70:
#line 591 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_BITAND, yyval.ttype);
                ;}
    break;

  case 71:
#line 597 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_BITOR, yyval.ttype);
                ;}
    break;

  case 72:
#line 603 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (yyvsp[-1].code, yyvsp[-2].ttype, yyvsp[0].ttype);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_BITXOR, yyval.ttype);
                ;}
    break;

  case 73:
#line 609 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (TRUTH_ANDIF_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_ANDAND, yyval.ttype);
                ;}
    break;

  case 74:
#line 615 "c-parse.y"
    { yyval.ttype = parser_build_binary_op (TRUTH_ORIF_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_OROR, yyval.ttype);
                ;}
    break;

  case 75:
#line 621 "c-parse.y"
    { yyval.ttype = build_conditional_expr (yyvsp[-4].ttype, yyvsp[-2].ttype, yyvsp[0].ttype);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_quest(GCT_LAST(Gct_all_nodes)->prev->prev->prev->prev, yyval.ttype);
                ;}
    break;

  case 76:
#line 626 "c-parse.y"
    { yyval.ttype = build_modify_expr (yyvsp[-2].ttype, NOP_EXPR, yyvsp[0].ttype);
		  C_SET_EXP_ORIGINAL_CODE (yyval.ttype, MODIFY_EXPR);
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_binary(GCT_LAST(Gct_all_nodes)->prev->prev,
				   GCT_SIMPLE_ASSIGN, yyval.ttype);
                ;}
    break;

  case 77:
#line 633 "c-parse.y"
    { yyval.ttype = build_modify_expr (yyvsp[-2].ttype, yyvsp[-1].code, yyvsp[0].ttype);
		  /* This inhibits warnings in truthvalue_conversion.  */
		  C_SET_EXP_ORIGINAL_CODE (yyval.ttype, ERROR_MARK); 
		  /* GCT: There's a lookahead token on the tokenlist. */
		  gct_build_nonsimple_assign(GCT_LAST(Gct_all_nodes)->prev->prev, yyval.ttype);
               ;}
    break;

  case 78:
#line 643 "c-parse.y"
    {
		  tree context;

		  yyval.ttype = lastiddecl;
		  if (!yyval.ttype || yyval.ttype == error_mark_node)
		    {
		      if (yychar == YYEMPTY)
			yychar = YYLEX;
		      if (yychar == '(')
			{
			    {
			      /* Ordinary implicit function declaration.  */
			      yyval.ttype = implicitly_declare (yyvsp[0].ttype);
			      assemble_external (yyval.ttype);
			      TREE_USED (yyval.ttype) = 1;
			    }
			}
		      else if (current_function_decl == 0)
			{
			  error ("`%s' undeclared here (not in a function)",
				 IDENTIFIER_POINTER (yyvsp[0].ttype));
			  yyval.ttype = error_mark_node;
			}
		      else
			{
			    {
			      if (IDENTIFIER_GLOBAL_VALUE (yyvsp[0].ttype) != error_mark_node
				  || IDENTIFIER_ERROR_LOCUS (yyvsp[0].ttype) != current_function_decl)
				{
				  error ("`%s' undeclared (first use this function)",
					 IDENTIFIER_POINTER (yyvsp[0].ttype));

				  if (! undeclared_variable_notice)
				    {
				      error ("(Each undeclared identifier is reported only once");
				      error ("for each function it appears in.)");
				      undeclared_variable_notice = 1;
				    }
				}
			      yyval.ttype = error_mark_node;
			      /* Prevent repeated error messages.  */
			      IDENTIFIER_GLOBAL_VALUE (yyvsp[0].ttype) = error_mark_node;
			      IDENTIFIER_ERROR_LOCUS (yyvsp[0].ttype) = current_function_decl;
			    }
			}
		    }
		  else if (TREE_TYPE (yyval.ttype) == error_mark_node)
		    yyval.ttype = error_mark_node;
		  else if (C_DECL_ANTICIPATED (yyval.ttype))
		    {
		      /* The first time we see a build-in function used,
			 if it has not been declared.  */
		      C_DECL_ANTICIPATED (yyval.ttype) = 0;
		      if (yychar == YYEMPTY)
			yychar = YYLEX;
		      if (yychar == '(')
			{
			  /* Omit the implicit declaration we
			     would ordinarily do, so we don't lose
			     the actual built in type.
			     But print a diagnostic for the mismatch.  */
			    if (TREE_CODE (yyval.ttype) != FUNCTION_DECL)
			      error ("`%s' implicitly declared as function",
				     IDENTIFIER_POINTER (DECL_NAME (yyval.ttype)));
			  else if ((TYPE_MODE (TREE_TYPE (TREE_TYPE (yyval.ttype)))
				    != TYPE_MODE (integer_type_node))
				   && (TREE_TYPE (TREE_TYPE (yyval.ttype))
				       != void_type_node))
			    pedwarn ("type mismatch in implicit declaration for built-in function `%s'",
				     IDENTIFIER_POINTER (DECL_NAME (yyval.ttype)));
			  /* If it really returns void, change that to int.  */
			  if (TREE_TYPE (TREE_TYPE (yyval.ttype)) == void_type_node)
			    TREE_TYPE (yyval.ttype)
			      = build_function_type (integer_type_node,
						     TYPE_ARG_TYPES (TREE_TYPE (yyval.ttype)));
			}
		      else
			pedwarn ("built-in function `%s' used without declaration",
				 IDENTIFIER_POINTER (DECL_NAME (yyval.ttype)));

		      /* Do what we would ordinarily do when a fn is used.  */
		      assemble_external (yyval.ttype);
		      TREE_USED (yyval.ttype) = 1;
		    }
		  else
		    {
		      assemble_external (yyval.ttype);
		      TREE_USED (yyval.ttype) = 1;
		    }

		  if (TREE_CODE (yyval.ttype) == CONST_DECL)
		    {
		      yyval.ttype = DECL_INITIAL (yyval.ttype);
		      /* This is to prevent an enum whose value is 0
			 from being considered a null pointer constant.  */
		      yyval.ttype = build1 (NOP_EXPR, TREE_TYPE (yyval.ttype), yyval.ttype);
		      TREE_CONSTANT (yyval.ttype) = 1;
		    }
		  gct_build_item(GCT_LAST(Gct_all_nodes), GCT_IDENTIFIER, yyval.ttype);
		;}
    break;

  case 79:
#line 745 "c-parse.y"
    { gct_build_item(GCT_LAST(Gct_all_nodes), GCT_CONSTANT, yyval.ttype); ;}
    break;

  case 80:
#line 747 "c-parse.y"
    { gct_build_item(GCT_LAST(Gct_all_nodes)->prev, GCT_CONSTANT, 
				 yyval.ttype);	/*** GCT ***/
		  yyval.ttype = combine_strings (yyvsp[0].ttype); ;}
    break;

  case 81:
#line 751 "c-parse.y"
    { char class = TREE_CODE_CLASS (TREE_CODE (yyvsp[-1].ttype));
		  if (class == 'e' || class == '1'
		      || class == '2' || class == '<')
		    C_SET_EXP_ORIGINAL_CODE (yyvsp[-1].ttype, ERROR_MARK);
		  gct_flush_parens(GCT_LAST(Gct_all_nodes)->prev->prev);	/*** GCT ***/
		  yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 82:
#line 758 "c-parse.y"
    { yyval.ttype = error_mark_node; ;}
    break;

  case 83:
#line 760 "c-parse.y"
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
		  yyval.ttype = expand_start_stmt_expr (); ;}
    break;

  case 84:
#line 774 "c-parse.y"
    { tree rtl_exp;
		  if (pedantic)
		    pedwarn ("ANSI C forbids braced-groups within expressions");
		  pop_iterator_stack ();
		  pop_label_level ();
		  rtl_exp = expand_end_stmt_expr (yyvsp[-2].ttype);
		  /* The statements have side effects, so the group does.  */
		  TREE_SIDE_EFFECTS (rtl_exp) = 1;

		  /* Make a BIND_EXPR for the BLOCK already made.  */
		  yyval.ttype = build (BIND_EXPR, TREE_TYPE (rtl_exp),
			      NULL_TREE, rtl_exp, yyvsp[-1].ttype);
	          /*** GCT ***/
		  gct_build_compound_expr(GCT_LAST(Gct_all_nodes)->prev, yyval.ttype);
                  /* Remove the block from the tree at this point.
		     It gets put back at the proper place
		     when the BIND_EXPR is expanded.  */
		/*** GCT: delete_block ($3); ***/
		;}
    break;

  case 85:
#line 794 "c-parse.y"
    {  /*** GCT ***/
                   gct_node primary, exprlist; 

                   yyval.ttype = build_function_call (yyvsp[-3].ttype, yyvsp[-1].ttype);
                   /*** GCT ***/
                   primary = yyvsp[-1].ttype ? GCT_LAST(Gct_all_nodes)->prev->prev->prev :
                                  GCT_LAST(Gct_all_nodes)->prev->prev;
                   exprlist = yyvsp[-1].ttype ? primary->next->next : GCT_NULL_NODE;
                   gct_build_function_call(primary, exprlist, yyval.ttype);
                   /*** END GCT ***/
                ;}
    break;

  case 86:
#line 806 "c-parse.y"
    { yyval.ttype = build_array_ref (yyvsp[-3].ttype, yyvsp[-1].ttype); 
                  gct_build_ref(GCT_LAST(Gct_all_nodes)->prev->prev, 
				GCT_ARRAYREF, yyval.ttype);
                ;}
    break;

  case 87:
#line 811 "c-parse.y"
    {
		    yyval.ttype = build_component_ref (yyvsp[-2].ttype, yyvsp[0].ttype);
		    gct_build_ref(GCT_LAST(Gct_all_nodes)->prev,
				  GCT_DOTREF, yyval.ttype);
		;}
    break;

  case 88:
#line 817 "c-parse.y"
    {
                  tree expr = build_indirect_ref (yyvsp[-2].ttype, "->");
                  yyval.ttype = build_component_ref (expr, yyvsp[0].ttype);
		  gct_build_ref(GCT_LAST(Gct_all_nodes)->prev, GCT_ARROWREF, 
				yyval.ttype); 
		;}
    break;

  case 89:
#line 824 "c-parse.y"
    { yyval.ttype = build_unary_op (POSTINCREMENT_EXPR, yyvsp[-1].ttype, 0); 
		  gct_build_post(GCT_LAST(Gct_all_nodes), GCT_POSTINCREMENT, 
				 yyval.ttype); 
                ;}
    break;

  case 90:
#line 829 "c-parse.y"
    { yyval.ttype = build_unary_op (POSTDECREMENT_EXPR, yyvsp[-1].ttype, 0); 
		  gct_build_post(GCT_LAST(Gct_all_nodes), GCT_POSTDECREMENT, 
				 yyval.ttype); 
                ;}
    break;

  case 92:
#line 839 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-1].ttype, yyvsp[0].ttype); 
		  /* GCT: Concatenate the strings.  Done here, instead of
		     in expr action, so that I don't have to worry
		     about shift-reduce conflicts - whether the last
		     element on the node list is really a string. */
		  gct_combine_strings(GCT_LAST(Gct_all_nodes)); 
                ;}
    break;

  case 95:
#line 854 "c-parse.y"
    { c_mark_varargs ();
		  if (pedantic)
		    pedwarn ("ANSI C does not permit use of `varargs.h'"); ;}
    break;

  case 96:
#line 864 "c-parse.y"
    { ;}
    break;

  case 101:
#line 876 "c-parse.y"
    { current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary (yyvsp[-2].itype);
	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                ;}
    break;

  case 102:
#line 888 "c-parse.y"
    { current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary (yyvsp[-2].itype);
	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                ;}
    break;

  case 103:
#line 900 "c-parse.y"
    { shadow_tag_warned (yyvsp[-1].ttype, 1);
		  pedwarn ("empty declaration");
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                ;}
    break;

  case 104:
#line 911 "c-parse.y"
    { pedwarn ("empty declaration"); 
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                ;}
    break;

  case 105:
#line 928 "c-parse.y"
    { ;}
    break;

  case 110:
#line 943 "c-parse.y"
    { yyval.itype = suspend_momentary ();
		  pending_xref_error ();
		  declspec_stack = tree_cons (NULL_TREE, current_declspecs,
					      declspec_stack);
		  current_declspecs = yyvsp[0].ttype; ;}
    break;

  case 111:
#line 952 "c-parse.y"
    { current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary (yyvsp[-2].itype);
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                 ;}
    break;

  case 112:
#line 964 "c-parse.y"
    { current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary (yyvsp[-2].itype); 
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                ;}
    break;

  case 113:
#line 976 "c-parse.y"
    { current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary (yyvsp[-1].itype); 
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                ;}
    break;

  case 114:
#line 988 "c-parse.y"
    { current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary (yyvsp[-1].itype); 
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                ;}
    break;

  case 115:
#line 1000 "c-parse.y"
    { shadow_tag (yyvsp[-1].ttype); 
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                ;}
    break;

  case 116:
#line 1010 "c-parse.y"
    { pedwarn ("empty declaration"); 
 	          /*** GCT ***/
		  if (NULL == current_function_decl)		
		    {
		      fatal("Decl production outside function.");
		    }
		  gct_build_decl(GCT_LAST(Gct_all_nodes)); 
		  /*** END GCT ***/
                ;}
    break;

  case 117:
#line 1027 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 118:
#line 1029 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[0].ttype, tree_cons (NULL_TREE, yyvsp[-1].ttype, yyvsp[-2].ttype)); ;}
    break;

  case 119:
#line 1033 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 120:
#line 1035 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype); ;}
    break;

  case 121:
#line 1037 "c-parse.y"
    { if (extra_warnings)
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype); ;}
    break;

  case 122:
#line 1049 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 123:
#line 1052 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 124:
#line 1054 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = 1; ;}
    break;

  case 125:
#line 1057 "c-parse.y"
    { if (extra_warnings && TREE_STATIC (yyvsp[-1].ttype))
		    warning ("`%s' is not at beginning of declaration",
			     IDENTIFIER_POINTER (yyvsp[0].ttype));
		  yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype);
		  TREE_STATIC (yyval.ttype) = TREE_STATIC (yyvsp[-1].ttype); ;}
    break;

  case 126:
#line 1071 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 127:
#line 1073 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[0].ttype, tree_cons (NULL_TREE, yyvsp[-1].ttype, yyvsp[-2].ttype)); ;}
    break;

  case 128:
#line 1077 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 129:
#line 1079 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype); ;}
    break;

  case 132:
#line 1089 "c-parse.y"
    { /* For a typedef name, record the meaning, not the name.
		     In case of `foo foo, bar;'.  */
		  yyval.ttype = lookup_name (yyvsp[0].ttype); ;}
    break;

  case 133:
#line 1093 "c-parse.y"
    { yyval.ttype = TREE_TYPE (yyvsp[-1].ttype); ;}
    break;

  case 134:
#line 1095 "c-parse.y"
    { yyval.ttype = groktypename (yyvsp[-1].ttype); ;}
    break;

  case 142:
#line 1117 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 143:
#line 1119 "c-parse.y"
    { if (TREE_CHAIN (yyvsp[-1].ttype)) yyvsp[-1].ttype = combine_strings (yyvsp[-1].ttype);
		  yyval.ttype = yyvsp[-1].ttype;
		;}
    break;

  case 144:
#line 1126 "c-parse.y"
    { yyval.ttype = start_decl (yyvsp[-3].ttype, current_declspecs, 1); ;}
    break;

  case 145:
#line 1129 "c-parse.y"
    { decl_attributes (yyvsp[-1].ttype, yyvsp[-3].ttype);
		  finish_decl (yyvsp[-1].ttype, yyvsp[0].ttype, yyvsp[-4].ttype); ;}
    break;

  case 146:
#line 1132 "c-parse.y"
    { tree d = start_decl (yyvsp[-2].ttype, current_declspecs, 0);
		  decl_attributes (d, yyvsp[0].ttype);
		  finish_decl (d, NULL_TREE, yyvsp[-1].ttype); ;}
    break;

  case 147:
#line 1139 "c-parse.y"
    { yyval.ttype = start_decl (yyvsp[-3].ttype, current_declspecs, 1); ;}
    break;

  case 148:
#line 1142 "c-parse.y"
    { decl_attributes (yyvsp[-1].ttype, yyvsp[-3].ttype);
		  finish_decl (yyvsp[-1].ttype, yyvsp[0].ttype, yyvsp[-4].ttype); ;}
    break;

  case 149:
#line 1145 "c-parse.y"
    { tree d = start_decl (yyvsp[-2].ttype, current_declspecs, 0);
		  decl_attributes (d, yyvsp[0].ttype);
		  finish_decl (d, NULL_TREE, yyvsp[-1].ttype); ;}
    break;

  case 150:
#line 1153 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 151:
#line 1155 "c-parse.y"
    { yyval.ttype = yyvsp[-2].ttype; ;}
    break;

  case 152:
#line 1160 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 153:
#line 1162 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-2].ttype); ;}
    break;

  case 154:
#line 1167 "c-parse.y"
    { if (strcmp (IDENTIFIER_POINTER (yyvsp[0].ttype), "packed"))
	    warning ("`%s' attribute directive ignored",
		     IDENTIFIER_POINTER (yyvsp[0].ttype));
	  yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 155:
#line 1172 "c-parse.y"
    { /* If not "mode (m)", then issue warning.  */
	  if (strcmp (IDENTIFIER_POINTER (yyvsp[-3].ttype), "mode") != 0)
	    {
	      warning ("`%s' attribute directive ignored",
		       IDENTIFIER_POINTER (yyvsp[-3].ttype));
	      yyval.ttype = yyvsp[-3].ttype;
	    }
	  else
	    yyval.ttype = tree_cons (yyvsp[-3].ttype, yyvsp[-1].ttype, NULL_TREE); ;}
    break;

  case 156:
#line 1182 "c-parse.y"
    { /* if not "aligned(n)", then issue warning */
	  if (strcmp (IDENTIFIER_POINTER (yyvsp[-3].ttype), "aligned") != 0
	      || TREE_CODE (yyvsp[-1].ttype) != INTEGER_CST)
	    {
	      warning ("`%s' attribute directive ignored",
		       IDENTIFIER_POINTER (yyvsp[-3].ttype));
	      yyval.ttype = yyvsp[-3].ttype;
	    }
	  else
	    yyval.ttype = tree_cons (yyvsp[-3].ttype, yyvsp[-1].ttype, NULL_TREE); ;}
    break;

  case 157:
#line 1193 "c-parse.y"
    { /* if not "format(...)", then issue warning */
	  if (strcmp (IDENTIFIER_POINTER (yyvsp[-7].ttype), "format") != 0
	      || TREE_CODE (yyvsp[-3].ttype) != INTEGER_CST
	      || TREE_CODE (yyvsp[-1].ttype) != INTEGER_CST)
	    {
	      warning ("`%s' attribute directive ignored",
		       IDENTIFIER_POINTER (yyvsp[-7].ttype));
	      yyval.ttype = yyvsp[-7].ttype;
	    }
	  else
	    yyval.ttype = tree_cons (yyvsp[-7].ttype,
			    tree_cons (yyvsp[-5].ttype,
				       tree_cons (yyvsp[-3].ttype, yyvsp[-1].ttype, NULL_TREE),
				       NULL_TREE),
			    NULL_TREE); ;}
    break;

  case 159:
#line 1213 "c-parse.y"
    { yyval.ttype = build_nt (CONSTRUCTOR, NULL_TREE, NULL_TREE);
		  if (pedantic)
		    pedwarn ("ANSI C forbids empty initializer braces"); ;}
    break;

  case 160:
#line 1217 "c-parse.y"
    { yyval.ttype = build_nt (CONSTRUCTOR, NULL_TREE, nreverse (yyvsp[-1].ttype)); ;}
    break;

  case 161:
#line 1219 "c-parse.y"
    { yyval.ttype = build_nt (CONSTRUCTOR, NULL_TREE, nreverse (yyvsp[-2].ttype)); ;}
    break;

  case 162:
#line 1221 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 163:
#line 1228 "c-parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 164:
#line 1230 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-2].ttype); ;}
    break;

  case 165:
#line 1235 "c-parse.y"
    { yyval.ttype = build_tree_list (tree_cons (yyvsp[-4].ttype, NULL_TREE,
						   build_tree_list (yyvsp[-2].ttype, NULL_TREE)),
					yyvsp[0].ttype); ;}
    break;

  case 166:
#line 1239 "c-parse.y"
    { yyval.ttype = tree_cons (tree_cons (yyvsp[-4].ttype, NULL_TREE,
					     build_tree_list (yyvsp[-2].ttype, NULL_TREE)),
				  yyvsp[0].ttype,
				  yyvsp[-7].ttype); ;}
    break;

  case 167:
#line 1244 "c-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 168:
#line 1246 "c-parse.y"
    { yyval.ttype = tree_cons (yyvsp[-2].ttype, yyvsp[0].ttype, yyvsp[-5].ttype); ;}
    break;

  case 169:
#line 1248 "c-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 170:
#line 1250 "c-parse.y"
    { yyval.ttype = tree_cons (yyvsp[-2].ttype, yyvsp[0].ttype, yyvsp[-4].ttype); ;}
    break;

  case 171:
#line 1255 "c-parse.y"
    { push_c_function_context ();
		  if (! start_function (current_declspecs, yyvsp[0].ttype, 1))
		    {
		      pop_c_function_context ();
		      YYERROR1;
		    }
		  reinit_parse_for_function ();
		  store_parm_decls (); ;}
    break;

  case 172:
#line 1270 "c-parse.y"
    { finish_function (1);
		  pop_c_function_context (); ;}
    break;

  case 173:
#line 1276 "c-parse.y"
    { push_c_function_context ();
		  if (! start_function (current_declspecs, yyvsp[0].ttype, 1))
		    {
		      pop_c_function_context ();
		      YYERROR1;
		    }
		  reinit_parse_for_function ();
		  store_parm_decls (); ;}
    break;

  case 174:
#line 1291 "c-parse.y"
    { finish_function (1);
		  pop_c_function_context (); ;}
    break;

  case 177:
#line 1307 "c-parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 178:
#line 1309 "c-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 179:
#line 1314 "c-parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 180:
#line 1316 "c-parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, yyvsp[-2].ttype, NULL_TREE); ;}
    break;

  case 181:
#line 1318 "c-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 182:
#line 1320 "c-parse.y"
    { GCT_LAST(Gct_all_nodes)->type = GCT_IDENTIFIER; ;}
    break;

  case 183:
#line 1330 "c-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 184:
#line 1335 "c-parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 185:
#line 1337 "c-parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, yyvsp[-2].ttype, NULL_TREE); ;}
    break;

  case 186:
#line 1339 "c-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 188:
#line 1348 "c-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 189:
#line 1353 "c-parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 190:
#line 1355 "c-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 191:
#line 1357 "c-parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 192:
#line 1359 "c-parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, yyvsp[-2].ttype, NULL_TREE); ;}
    break;

  case 193:
#line 1361 "c-parse.y"
    { GCT_LAST(Gct_all_nodes)->type = GCT_IDENTIFIER; ;}
    break;

  case 194:
#line 1366 "c-parse.y"
    { yyval.ttype = start_struct (RECORD_TYPE, yyvsp[-1].ttype);
		  /* Start scope of tag before parsing components.  */
		;}
    break;

  case 195:
#line 1370 "c-parse.y"
    { yyval.ttype = finish_struct (yyvsp[-2].ttype, yyvsp[-1].ttype);
		  /* Really define the structure.  */
		;}
    break;

  case 196:
#line 1380 "c-parse.y"
    {
		  gct_node dummy = gct_tempnode("_GCT_DUMMY_");
                  gct_add_before(&Gct_all_nodes,
                                 GCT_LAST(Gct_all_nodes), dummy);
                  yyval.ttype = start_struct (RECORD_TYPE, get_identifier(dummy->text));
		  /* Start scope of tag before parsing components.  */
		;}
    break;

  case 197:
#line 1388 "c-parse.y"
    { yyval.ttype = finish_struct (yyvsp[-2].ttype, yyvsp[-1].ttype);
		  /* Really define the structure.  */
		;}
    break;

  case 198:
#line 1392 "c-parse.y"
    { yyval.ttype = xref_tag (RECORD_TYPE, yyvsp[0].ttype); ;}
    break;

  case 199:
#line 1394 "c-parse.y"
    { yyval.ttype = start_struct (UNION_TYPE, yyvsp[-1].ttype); ;}
    break;

  case 200:
#line 1396 "c-parse.y"
    { yyval.ttype = finish_struct (yyvsp[-2].ttype, yyvsp[-1].ttype); ;}
    break;

  case 201:
#line 1404 "c-parse.y"
    {
		  gct_node dummy = gct_tempnode("_GCT_DUMMY_");
                  gct_add_before(&Gct_all_nodes,
                                 GCT_LAST(Gct_all_nodes), dummy);
                  yyval.ttype = start_struct (UNION_TYPE, get_identifier(dummy->text));
		  /* Start scope of tag before parsing components.  */
		;}
    break;

  case 202:
#line 1412 "c-parse.y"
    { yyval.ttype = finish_struct (yyvsp[-2].ttype, yyvsp[-1].ttype); ;}
    break;

  case 203:
#line 1414 "c-parse.y"
    { yyval.ttype = xref_tag (UNION_TYPE, yyvsp[0].ttype); ;}
    break;

  case 204:
#line 1416 "c-parse.y"
    { yyvsp[0].itype = suspend_momentary ();
		  yyval.ttype = start_enum (yyvsp[-1].ttype); ;}
    break;

  case 205:
#line 1419 "c-parse.y"
    { yyval.ttype = finish_enum (yyvsp[-3].ttype, nreverse (yyvsp[-2].ttype));
		  resume_momentary (yyvsp[-4].itype); ;}
    break;

  case 206:
#line 1428 "c-parse.y"
    { 
		  gct_node dummy = gct_tempnode("_GCT_DUMMY_");
		  yyvsp[0].itype = suspend_momentary ();
		  
                  gct_add_before(&Gct_all_nodes,
                                 GCT_LAST(Gct_all_nodes), dummy);
		  yyval.ttype = start_enum (get_identifier(dummy->text));
		;}
    break;

  case 207:
#line 1437 "c-parse.y"
    { yyval.ttype = finish_enum (yyvsp[-3].ttype, nreverse (yyvsp[-2].ttype));
		  resume_momentary (yyvsp[-4].itype); ;}
    break;

  case 208:
#line 1440 "c-parse.y"
    { yyval.ttype = xref_tag (ENUMERAL_TYPE, yyvsp[0].ttype); ;}
    break;

  case 212:
#line 1451 "c-parse.y"
    { if (pedantic) pedwarn ("comma at end of enumerator list"); ;}
    break;

  case 213:
#line 1456 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 214:
#line 1458 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-1].ttype, yyvsp[0].ttype);
		  pedwarn ("no semicolon at end of struct or union"); ;}
    break;

  case 215:
#line 1463 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 216:
#line 1465 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, yyvsp[-1].ttype); ;}
    break;

  case 217:
#line 1467 "c-parse.y"
    { if (pedantic)
		    pedwarn ("extra semicolon in struct or union specified"); ;}
    break;

  case 218:
#line 1482 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary (yyvsp[-1].itype); ;}
    break;

  case 219:
#line 1487 "c-parse.y"
    { if (pedantic)
		    pedwarn ("ANSI C forbids member declarations with no members");
		  shadow_tag(yyvsp[0].ttype);
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 220:
#line 1492 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  current_declspecs = TREE_VALUE (declspec_stack);
		  declspec_stack = TREE_CHAIN (declspec_stack);
		  resume_momentary (yyvsp[-1].itype); ;}
    break;

  case 221:
#line 1497 "c-parse.y"
    { if (pedantic)
		    pedwarn ("ANSI C forbids member declarations with no members");
		  shadow_tag(yyvsp[0].ttype);
		  yyval.ttype = NULL_TREE; ;}
    break;

  case 222:
#line 1502 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 224:
#line 1508 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 225:
#line 1513 "c-parse.y"
    { yyval.ttype = grokfield (yyvsp[-3].filename, yyvsp[-2].lineno, yyvsp[-1].ttype, current_declspecs, NULL_TREE);
		  decl_attributes (yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 226:
#line 1517 "c-parse.y"
    { yyval.ttype = grokfield (yyvsp[-5].filename, yyvsp[-4].lineno, yyvsp[-3].ttype, current_declspecs, yyvsp[-1].ttype);
		  decl_attributes (yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 227:
#line 1520 "c-parse.y"
    { yyval.ttype = grokfield (yyvsp[-4].filename, yyvsp[-3].lineno, NULL_TREE, current_declspecs, yyvsp[-1].ttype);
		  decl_attributes (yyval.ttype, yyvsp[0].ttype); ;}
    break;

  case 229:
#line 1532 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[0].ttype, yyvsp[-2].ttype); ;}
    break;

  case 230:
#line 1538 "c-parse.y"
    { yyval.ttype = build_enumerator (yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 231:
#line 1540 "c-parse.y"
    { yyval.ttype = build_enumerator (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 232:
#line 1545 "c-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 233:
#line 1547 "c-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 234:
#line 1552 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 236:
#line 1558 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 237:
#line 1560 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype); ;}
    break;

  case 238:
#line 1565 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 239:
#line 1567 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, yyvsp[0].ttype, yyvsp[-1].ttype); ;}
    break;

  case 240:
#line 1572 "c-parse.y"
    { yyval.ttype = yyvsp[-1].ttype; ;}
    break;

  case 241:
#line 1575 "c-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 242:
#line 1577 "c-parse.y"
    { yyval.ttype = make_pointer_declarator (yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 243:
#line 1579 "c-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, yyvsp[-2].ttype, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 244:
#line 1581 "c-parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 245:
#line 1583 "c-parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, yyvsp[-2].ttype, NULL_TREE); ;}
    break;

  case 246:
#line 1585 "c-parse.y"
    { yyval.ttype = build_nt (CALL_EXPR, NULL_TREE, yyvsp[0].ttype, NULL_TREE); ;}
    break;

  case 247:
#line 1587 "c-parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, NULL_TREE, yyvsp[-1].ttype); ;}
    break;

  case 248:
#line 1589 "c-parse.y"
    { yyval.ttype = build_nt (ARRAY_REF, NULL_TREE, NULL_TREE); ;}
    break;

  case 255:
#line 1611 "c-parse.y"
    { emit_line_note (input_filename, lineno);
		  pushlevel (0);
		  clear_last_expr ();
		  push_momentary ();
		  expand_start_bindings (0);
	          Gct_stmt_depth++
		;}
    break;

  case 257:
#line 1625 "c-parse.y"
    { if (pedantic)
		    pedwarn ("ANSI C forbids label declarations"); ;}
    break;

  case 260:
#line 1636 "c-parse.y"
    { tree link;
		  for (link = yyvsp[-1].ttype; link; link = TREE_CHAIN (link))
		    {
		      tree label = shadow_label (TREE_VALUE (link));
		      C_DECLARED_LABEL_FLAG (label) = 1;
		      declare_nonlocal_label (label);
		    }
		;}
    break;

  case 261:
#line 1650 "c-parse.y"
    {;}
    break;

  case 263:
#line 1655 "c-parse.y"
    { yyval.ttype = convert (void_type_node, integer_zero_node);
		  gct_build_compound_stmt(GCT_LAST(Gct_all_nodes));
                ;}
    break;

  case 264:
#line 1659 "c-parse.y"
    { emit_line_note (input_filename, lineno);
		  expand_end_bindings (getdecls (), 1, 0);
		  yyval.ttype = poplevel (1, 1, 0);
		  gct_build_compound_stmt(GCT_LAST(Gct_all_nodes));
	          Gct_stmt_depth--;
		  pop_momentary (); ;}
    break;

  case 265:
#line 1666 "c-parse.y"
    { emit_line_note (input_filename, lineno);
		  expand_end_bindings (getdecls (), kept_level_p (), 0);
		  yyval.ttype = poplevel (kept_level_p (), 0, 0);
		  gct_build_compound_stmt(GCT_LAST(Gct_all_nodes));
	          Gct_stmt_depth--;
		  pop_momentary (); ;}
    break;

  case 266:
#line 1673 "c-parse.y"
    { emit_line_note (input_filename, lineno);
		  expand_end_bindings (getdecls (), kept_level_p (), 0);
		  /*
		   * GCT:  We keep all levels of compound statements.
		   * $$ = poplevel (kept_level_p (), 0, 0);
		   */
		  yyval.ttype = poplevel (1, 0, 0);
		  gct_build_compound_stmt(GCT_LAST(Gct_all_nodes));
	          Gct_stmt_depth--;
		  pop_momentary (); 
                ;}
    break;

  case 269:
#line 1697 "c-parse.y"
    { emit_line_note (yyvsp[-5].filename, yyvsp[-4].lineno);
		  expand_start_cond (truthvalue_conversion (yyvsp[-1].ttype), 0);
		  yyvsp[-3].itype = stmt_count;
		  if_stmt_file = yyvsp[-5].filename;
		  if_stmt_line = yyvsp[-4].lineno;
		  position_after_white_space (); ;}
    break;

  case 270:
#line 1710 "c-parse.y"
    { stmt_count++;
		  emit_line_note (yyvsp[-2].filename, yyvsp[-1].lineno);
		  /* See comment in `while' alternative, above.  */
		  emit_nop ();
		  expand_start_loop_continue_elsewhere (1);
		  position_after_white_space (); ;}
    break;

  case 271:
#line 1717 "c-parse.y"
    { expand_loop_continue_here (); ;}
    break;

  case 272:
#line 1721 "c-parse.y"
    { yyval.filename = input_filename; ;}
    break;

  case 273:
#line 1725 "c-parse.y"
    { yyval.lineno = lineno; ;}
    break;

  case 274:
#line 1730 "c-parse.y"
    { ;}
    break;

  case 275:
#line 1735 "c-parse.y"
    { int Gct_have_case = GCT_LABEL_POP();
                if (Gct_have_case == CASE) {
                   gct_build_case(GCT_LAST(Gct_all_nodes));
                } else if (Gct_have_case == DEFAULT) {
                   gct_build_default(GCT_LAST(Gct_all_nodes));
                } else {
                   gct_build_label(GCT_LAST(Gct_all_nodes));
                }
              ;}
    break;

  case 276:
#line 1748 "c-parse.y"
    { ;}
    break;

  case 277:
#line 1753 "c-parse.y"
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
              ;}
    break;

  case 278:
#line 1765 "c-parse.y"
    { int next;
		  position_after_white_space ();
		  next = getc (finput);
		  ungetc (next, finput);
		  if (pedantic && next == '}')
		    pedwarn ("ANSI C forbids label at end of compound statement");
		;}
    break;

  case 279:
#line 1777 "c-parse.y"
    { stmt_count++; ;}
    break;

  case 281:
#line 1780 "c-parse.y"
    { stmt_count++;
		  emit_line_note (yyvsp[-3].filename, yyvsp[-2].lineno);
		  iterator_expand (yyvsp[-1].ttype);
		  gct_build_simple_stmt(GCT_LAST(Gct_all_nodes));
		  clear_momentary (); ;}
    break;

  case 282:
#line 1786 "c-parse.y"
    { expand_start_else ();
		  yyvsp[-1].itype = stmt_count;
		  position_after_white_space (); ;}
    break;

  case 283:
#line 1790 "c-parse.y"
    { expand_end_cond ();
		  gct_build_if_else(GCT_LAST(Gct_all_nodes));	/*** GCT ***/
		  if (extra_warnings && stmt_count == yyvsp[-3].itype)
		    warning ("empty body in an else-statement"); ;}
    break;

  case 284:
#line 1795 "c-parse.y"
    { expand_end_cond ();
		  gct_build_simple_if(GCT_LAST(Gct_all_nodes)->prev);
		  if (extra_warnings && stmt_count == yyvsp[0].itype)
		    warning_with_file_and_line (if_stmt_file, if_stmt_line,
						"empty body in an if-statement"); ;}
    break;

  case 285:
#line 1804 "c-parse.y"
    { expand_end_cond (); ;}
    break;

  case 286:
#line 1806 "c-parse.y"
    { stmt_count++;
		  emit_line_note (yyvsp[-2].filename, yyvsp[-1].lineno);
		  /* The emit_nop used to come before emit_line_note,
		     but that made the nop seem like part of the preceding line.
		     And that was confusing when the preceding line was
		     inside of an if statement and was not really executed.
		     I think it ought to work to put the nop after the line number.
		     We will see.  --rms, July 15, 1991.  */
		  emit_nop (); ;}
    break;

  case 287:
#line 1816 "c-parse.y"
    { /* Don't start the loop till we have succeeded
		     in parsing the end test.  This is to make sure
		     that we end every loop we start.  */
		  expand_start_loop (1);
		  emit_line_note (input_filename, lineno);
		  expand_exit_loop_if_false (NULL_PTR,
					     truthvalue_conversion (yyvsp[-1].ttype));
		  position_after_white_space (); ;}
    break;

  case 288:
#line 1825 "c-parse.y"
    { expand_end_loop ();
		  gct_build_while_stmt(GCT_LAST(Gct_all_nodes)); 
                ;}
    break;

  case 289:
#line 1830 "c-parse.y"
    { emit_line_note (input_filename, lineno);
		  expand_exit_loop_if_false (NULL_PTR,
					     truthvalue_conversion (yyvsp[-2].ttype));
		  expand_end_loop ();
		  clear_momentary ();
                  gct_build_do_stmt(GCT_LAST(Gct_all_nodes));  
                ;}
    break;

  case 290:
#line 1839 "c-parse.y"
    { expand_end_loop ();
		  clear_momentary ();
                  gct_build_do_stmt(GCT_LAST(Gct_all_nodes)); 
                ;}
    break;

  case 291:
#line 1845 "c-parse.y"
    { stmt_count++;
		  emit_line_note (yyvsp[-5].filename, yyvsp[-4].lineno);
		  /* See comment in `while' alternative, above.  */
		  emit_nop ();
		  if (yyvsp[-1].ttype) c_expand_expr_stmt (yyvsp[-1].ttype);
		  /* Next step is to call expand_start_loop_continue_elsewhere,
		     but wait till after we parse the entire for (...).
		     Otherwise, invalid input might cause us to call that
		     fn without calling expand_end_loop.  */
		;}
    break;

  case 292:
#line 1857 "c-parse.y"
    { yyvsp[0].lineno = lineno;
		  yyval.filename = input_filename; ;}
    break;

  case 293:
#line 1860 "c-parse.y"
    { 
		  /* Start the loop.  Doing this after parsing
		     all the expressions ensures we will end the loop.  */
		  expand_start_loop_continue_elsewhere (1);
		  /* Emit the end-test, with a line number.  */
		  emit_line_note (yyvsp[-2].filename, yyvsp[-3].lineno);
		  if (yyvsp[-4].ttype)
		    expand_exit_loop_if_false (NULL_PTR,
					       truthvalue_conversion (yyvsp[-4].ttype));
		  /* Don't let the tree nodes for $9 be discarded by
		     clear_momentary during the parsing of the next stmt.  */
		  push_momentary ();
		  yyvsp[-3].lineno = lineno;
		  yyvsp[-2].filename = input_filename;
		  position_after_white_space (); ;}
    break;

  case 294:
#line 1876 "c-parse.y"
    { /* Emit the increment expression, with a line number.  */
		  emit_line_note (yyvsp[-4].filename, yyvsp[-5].lineno);
		  expand_loop_continue_here ();
		  if (yyvsp[-3].ttype)
		    c_expand_expr_stmt (yyvsp[-3].ttype);
		  pop_momentary ();
		  expand_end_loop ();
		  gct_build_for_stmt(GCT_LAST(Gct_all_nodes)); 
                 ;}
    break;

  case 295:
#line 1886 "c-parse.y"
    { stmt_count++;
		  emit_line_note (yyvsp[-5].filename, yyvsp[-4].lineno);
		  c_expand_start_case (yyvsp[-1].ttype);
		  /* Don't let the tree nodes for $3 be discarded by
		     clear_momentary during the parsing of the next stmt.  */
		  push_momentary ();
		  position_after_white_space (); ;}
    break;

  case 296:
#line 1894 "c-parse.y"
    { expand_end_case (yyvsp[-3].ttype);
		  pop_momentary (); 
		  gct_build_switch(GCT_LAST(Gct_all_nodes)); 
                ;}
    break;

  case 297:
#line 1899 "c-parse.y"
    { stmt_count++;
		  emit_line_note (yyvsp[-3].filename, yyvsp[-2].lineno);
		  if ( ! expand_exit_something ())
		    error ("break statement not within loop or switch"); 
		  gct_build_break(GCT_LAST(Gct_all_nodes)); 
                ;}
    break;

  case 298:
#line 1906 "c-parse.y"
    { stmt_count++;
		  emit_line_note (yyvsp[-3].filename, yyvsp[-2].lineno);
		  if (! expand_continue_loop (NULL_PTR))
		    error ("continue statement not within a loop"); 
		  gct_build_continue(GCT_LAST(Gct_all_nodes)); 
                ;}
    break;

  case 299:
#line 1913 "c-parse.y"
    { stmt_count++;
		  emit_line_note (yyvsp[-3].filename, yyvsp[-2].lineno);
		  c_expand_return (NULL_TREE); 
		  gct_build_return(GCT_LAST(Gct_all_nodes)); 
                ;}
    break;

  case 300:
#line 1919 "c-parse.y"
    { stmt_count++;
		  emit_line_note (yyvsp[-4].filename, yyvsp[-3].lineno);
		  c_expand_return (yyvsp[-1].ttype); 
		  gct_build_return(GCT_LAST(Gct_all_nodes)); 
                ;}
    break;

  case 301:
#line 1925 "c-parse.y"
    { stmt_count++;
		  emit_line_note (yyvsp[-7].filename, yyvsp[-6].lineno);
		  STRIP_NOPS (yyvsp[-2].ttype);
		  if ((TREE_CODE (yyvsp[-2].ttype) == ADDR_EXPR
		       && TREE_CODE (TREE_OPERAND (yyvsp[-2].ttype, 0)) == STRING_CST)
		      || TREE_CODE (yyvsp[-2].ttype) == STRING_CST) 
		      {
			  expand_asm (yyvsp[-2].ttype);
			  gct_build_asm(GCT_LAST(Gct_all_nodes));	/*** GCT ***/
		      }
		  else
		    error ("argument of `asm' is not a constant string"); ;}
    break;

  case 302:
#line 1939 "c-parse.y"
    { stmt_count++;
		  emit_line_note (yyvsp[-9].filename, yyvsp[-8].lineno);
		  c_expand_asm_operands (yyvsp[-4].ttype, yyvsp[-2].ttype, NULL_TREE, NULL_TREE,
					 yyvsp[-6].ttype == ridpointers[(int)RID_VOLATILE],
					 input_filename, lineno);
		  gct_build_asm(GCT_LAST(Gct_all_nodes)); /*** GCT ***/
                ;}
    break;

  case 303:
#line 1948 "c-parse.y"
    { stmt_count++;
		  emit_line_note (yyvsp[-11].filename, yyvsp[-10].lineno);
		  c_expand_asm_operands (yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype, NULL_TREE,
					 yyvsp[-8].ttype == ridpointers[(int)RID_VOLATILE],
					 input_filename, lineno); 
		  gct_build_asm(GCT_LAST(Gct_all_nodes)); /*** GCT ***/
                ;}
    break;

  case 304:
#line 1958 "c-parse.y"
    { stmt_count++;
		  emit_line_note (yyvsp[-13].filename, yyvsp[-12].lineno);
		  c_expand_asm_operands (yyvsp[-8].ttype, yyvsp[-6].ttype, yyvsp[-4].ttype, yyvsp[-2].ttype,
					 yyvsp[-10].ttype == ridpointers[(int)RID_VOLATILE],
					 input_filename, lineno);
		  gct_build_asm(GCT_LAST(Gct_all_nodes)); /*** GCT ***/
                ;}
    break;

  case 305:
#line 1966 "c-parse.y"
    { tree decl;
		  stmt_count++;
		  emit_line_note (yyvsp[-4].filename, yyvsp[-3].lineno);
		  decl = lookup_label (yyvsp[-1].ttype);
		  if (decl != 0)
		    {
		      TREE_USED (decl) = 1;
		      expand_goto (decl);
		      gct_build_goto(GCT_LAST(Gct_all_nodes));	/*** GCT ***/
		    }
		;}
    break;

  case 306:
#line 1978 "c-parse.y"
    { stmt_count++;
		  emit_line_note (yyvsp[-5].filename, yyvsp[-4].lineno);
		  expand_computed_goto (convert (ptr_type_node, yyvsp[-1].ttype)); ;}
    break;

  case 307:
#line 1982 "c-parse.y"
    { gct_build_null_stmt(GCT_LAST(Gct_all_nodes)); ;}
    break;

  case 309:
#line 1992 "c-parse.y"
    {
	    /* The value returned by this action is  */
	    /*      1 if everything is OK */ 
	    /*      0 in case of error or already bound iterator */

	    yyval.itype = 0;
	    if (TREE_CODE (yyvsp[-1].ttype) != VAR_DECL)
	      error ("invalid `for (ITERATOR)' syntax");
	    if (! ITERATOR_P (yyvsp[-1].ttype))
	      error ("`%s' is not an iterator",
		     IDENTIFIER_POINTER (DECL_NAME (yyvsp[-1].ttype)));
	    else if (ITERATOR_BOUND_P (yyvsp[-1].ttype))
	      error ("`for (%s)' inside expansion of same iterator",
		     IDENTIFIER_POINTER (DECL_NAME (yyvsp[-1].ttype)));
	    else
	      {
		yyval.itype = 1;
		iterator_for_loop_start (yyvsp[-1].ttype);
	      }
	  ;}
    break;

  case 310:
#line 2013 "c-parse.y"
    {
	    if (yyvsp[-1].itype)
	      iterator_for_loop_end (yyvsp[-3].ttype);
	  ;}
    break;

  case 311:
#line 2045 "c-parse.y"
    { register tree value = check_case_value (yyvsp[-1].ttype);
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
                ;}
    break;

  case 312:
#line 2071 "c-parse.y"
    { register tree value1 = check_case_value (yyvsp[-3].ttype);
		  register tree value2 = check_case_value (yyvsp[-1].ttype);
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
                  position_after_white_space (); ;}
    break;

  case 313:
#line 2100 "c-parse.y"
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
                  position_after_white_space (); ;}
    break;

  case 314:
#line 2116 "c-parse.y"
    { tree label = define_label (input_filename, lineno, yyvsp[-1].ttype);
		  stmt_count++;
		  emit_nop ();
		  if (label)
		    expand_label (label);
		   GCT_LABEL_PUSH(1); /* placeholder */
                ;}
    break;

  case 315:
#line 2129 "c-parse.y"
    { emit_line_note (input_filename, lineno); ;}
    break;

  case 316:
#line 2131 "c-parse.y"
    { emit_line_note (input_filename, lineno); ;}
    break;

  case 317:
#line 2136 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 319:
#line 2143 "c-parse.y"
    { yyval.ttype = NULL_TREE; ;}
    break;

  case 322:
#line 2150 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, yyvsp[0].ttype); ;}
    break;

  case 323:
#line 2155 "c-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-3].ttype, yyvsp[-1].ttype); ;}
    break;

  case 324:
#line 2160 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, combine_strings (yyvsp[0].ttype), NULL_TREE); ;}
    break;

  case 325:
#line 2162 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, combine_strings (yyvsp[0].ttype), yyvsp[-2].ttype); ;}
    break;

  case 326:
#line 2168 "c-parse.y"
    { pushlevel (0);
		  clear_parm_order ();
		  declare_parm_level (0); ;}
    break;

  case 327:
#line 2172 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  parmlist_tags_warning ();
		  poplevel (0, 0, 0); ;}
    break;

  case 329:
#line 2180 "c-parse.y"
    { tree parm;
		  if (pedantic)
		    pedwarn ("ANSI C forbids forward parameter declarations");
		  /* Mark the forward decls as such.  */
		  for (parm = getdecls (); parm; parm = TREE_CHAIN (parm))
		    TREE_ASM_WRITTEN (parm) = 1;
		  clear_parm_order (); ;}
    break;

  case 330:
#line 2188 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype; ;}
    break;

  case 331:
#line 2190 "c-parse.y"
    { yyval.ttype = tree_cons (NULL_TREE, NULL_TREE, NULL_TREE); ;}
    break;

  case 332:
#line 2196 "c-parse.y"
    { yyval.ttype = get_parm_info (0); ;}
    break;

  case 333:
#line 2198 "c-parse.y"
    { yyval.ttype = get_parm_info (0);
		  if (pedantic)
		    pedwarn ("ANSI C requires a named argument before `...'");
		;}
    break;

  case 334:
#line 2203 "c-parse.y"
    { yyval.ttype = get_parm_info (1); ;}
    break;

  case 335:
#line 2205 "c-parse.y"
    { yyval.ttype = get_parm_info (0); ;}
    break;

  case 336:
#line 2210 "c-parse.y"
    { push_parm_decl (yyvsp[0].ttype); ;}
    break;

  case 337:
#line 2212 "c-parse.y"
    { push_parm_decl (yyvsp[0].ttype); ;}
    break;

  case 338:
#line 2219 "c-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-1].ttype, yyvsp[0].ttype)	; ;}
    break;

  case 339:
#line 2221 "c-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-1].ttype, yyvsp[0].ttype)	; ;}
    break;

  case 340:
#line 2223 "c-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 341:
#line 2225 "c-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-1].ttype, yyvsp[0].ttype)	; ;}
    break;

  case 342:
#line 2227 "c-parse.y"
    { yyval.ttype = build_tree_list (yyvsp[-1].ttype, yyvsp[0].ttype); ;}
    break;

  case 343:
#line 2234 "c-parse.y"
    { pushlevel (0);
		  clear_parm_order ();
		  declare_parm_level (1); ;}
    break;

  case 344:
#line 2238 "c-parse.y"
    { yyval.ttype = yyvsp[0].ttype;
		  parmlist_tags_warning ();
		  poplevel (0, 0, 0); ;}
    break;

  case 346:
#line 2246 "c-parse.y"
    { tree t;
		  for (t = yyvsp[-1].ttype; t; t = TREE_CHAIN (t))
		    if (TREE_VALUE (t) == NULL_TREE)
		      error ("`...' in old-style identifier list");
		  yyval.ttype = tree_cons (NULL_TREE, NULL_TREE, yyvsp[-1].ttype); ;}
    break;

  case 347:
#line 2256 "c-parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 348:
#line 2258 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;}
    break;

  case 349:
#line 2264 "c-parse.y"
    { yyval.ttype = build_tree_list (NULL_TREE, yyvsp[0].ttype); ;}
    break;

  case 350:
#line 2266 "c-parse.y"
    { yyval.ttype = chainon (yyvsp[-2].ttype, build_tree_list (NULL_TREE, yyvsp[0].ttype)); ;}
    break;


    }

/* Line 1010 of yacc.c.  */
#line 4466 "c-parse.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {
		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
		 yydestruct (yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
	  yydestruct (yytoken, &yylval);
	  yychar = YYEMPTY;

	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

  yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 2269 "c-parse.y"


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
     


