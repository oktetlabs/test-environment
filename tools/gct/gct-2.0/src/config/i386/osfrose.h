/* Definitions of target machine for GNU compiler.
   Intel 386 (OSF/1 with OSF/rose) version.
   Copyright (C) 1991 Free Software Foundation, Inc.

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

/* Put leading underscores in front of names. */
#define YES_UNDERSCORES

#include "halfpic.h"
#include "i386/gstabs.h"

/* Get perform_* macros to build libgcc.a.  */
#include "i386/perform.h"

#define OSF_OS

#undef  WORD_SWITCH_TAKES_ARG
#define WORD_SWITCH_TAKES_ARG(STR)					\
 (DEFAULT_WORD_SWITCH_TAKES_ARG (STR) || !strcmp (STR, "pic-names"))

#define MASK_HALF_PIC     	0x40000000	/* Mask for half-pic code */
#define MASK_HALF_PIC_DEBUG	0x20000000	/* Debug flag */
#define MASK_ELF		0x10000000	/* ELF not rose */
#define MASK_NO_IDENT		0x08000000	/* suppress .ident */

#define TARGET_HALF_PIC	(target_flags & MASK_HALF_PIC)
#define TARGET_DEBUG	(target_flags & MASK_HALF_PIC_DEBUG)
#define HALF_PIC_DEBUG	TARGET_DEBUG
#define TARGET_ELF	(target_flags & MASK_ELF)
#define TARGET_ROSE	((target_flags & MASK_ELF) == 0)
#define TARGET_IDENT	((target_flags & MASK_NO_IDENT) == 0)

#undef	SUBTARGET_SWITCHES
#define SUBTARGET_SWITCHES \
     { "half-pic",	 MASK_HALF_PIC},				\
     { "no-half-pic",	-MASK_HALF_PIC},				\
     { "debugb",	 MASK_HALF_PIC_DEBUG},				\
     { "elf",		 MASK_ELF},					\
     { "no-elf",	-MASK_ELF},					\
     { "rose",		-MASK_ELF},					\
     { "ident",		-MASK_NO_IDENT},				\
     { "no-ident",	 MASK_NO_IDENT},

/* OSF/rose uses stabs, not dwarf.  */
#define PREFERRED_DEBUGGING_TYPE DBX_DEBUG

#ifndef DWARF_DEBUGGING_INFO
#define DWARF_DEBUGGING_INFO	/* enable dwarf debugging for testing */
#endif

/* Handle #pragma weak and #pragma pack.  */

#define HANDLE_SYSV_PRAGMA

/* Change default predefines.  */
#undef	CPP_PREDEFINES
#define CPP_PREDEFINES "-DOSF -DOSF1 -Dunix -Di386 -Asystem(unix) -Acpu(i386) -Amachine(i386)"

#undef  CPP_SPEC
#define CPP_SPEC "\
%{!melf: -D__ROSE__} %{melf: -D__ELF__} \
%{.S:	%{!ansi:%{!traditional:%{!traditional-cpp:%{!ftraditional: -traditional}}}}} \
%{.S:	-D__LANGUAGE_ASSEMBLY %{!ansi:-DLANGUAGE_ASSEMBLY}} \
%{.cc:	-D__LANGUAGE_C_PLUS_PLUS} \
%{.cxx:	-D__LANGUAGE_C_PLUS_PLUS} \
%{.C:	-D__LANGUAGE_C_PLUS_PLUS} \
%{.m:	-D__LANGUAGE_OBJECTIVE_C} \
%{!.S:	-D__LANGUAGE_C %{!ansi:-DLANGUAGE_C}}"

/* Turn on -mpic-extern by default.  */
#undef  CC1_SPEC
#define CC1_SPEC "\
%{!melf: %{!mrose: %{!mno-elf: -mrose }}} \
%{gline:%{!g:%{!g0:%{!g1:%{!g2: -g1}}}}} \
%{pic-none:   -mno-half-pic} \
%{fpic:	      -mno-half-pic} \
%{fPIC:	      -mno-half-pic} \
%{pic-lib:    -mhalf-pic} \
%{pic-extern: -mhalf-pic} \
%{pic-calls:  -mhalf-pic} \
%{pic-names*: -mhalf-pic} \
%{!pic-*: %{!fpic: %{!fPIC: -mhalf-pic}}}"

#undef	ASM_SPEC
#define ASM_SPEC       "%{v*: -v}"

#undef  LINK_SPEC
#define LINK_SPEC      "%{v*: -v}                           \
	               %{!noshrlib: %{pic-none: -noshrlib} %{!pic-none: -warn_nopic}} \
	               %{nostdlib} %{noshrlib} %{glue}"

#undef  LIB_SPEC
#define LIB_SPEC "-lc"

#undef  LIBG_SPEC
#define LIBG_SPEC ""

#undef  STARTFILE_SPEC
#define STARTFILE_SPEC "%{pg:gcrt0.o%s}%{!pg:%{p:mcrt0.o%s}%{!p:crt0.o%s}}"

#undef TARGET_VERSION_INTERNAL
#undef TARGET_VERSION

#define I386_VERSION " 80386, OSF/rose objects"

#define TARGET_VERSION_INTERNAL(STREAM) fputs (I386_VERSION, STREAM)
#define TARGET_VERSION TARGET_VERSION_INTERNAL (stderr)

#undef  MD_EXEC_PREFIX
#define MD_EXEC_PREFIX		"/usr/ccs/gcc/"

#undef  MD_STARTFILE_PREFIX
#define MD_STARTFILE_PREFIX	"/usr/ccs/lib/"

/* Specify size_t, ptrdiff_t, and wchar_t types.  */
#undef	SIZE_TYPE
#undef	PTRDIFF_TYPE
#undef	WCHAR_TYPE
#undef	WCHAR_TYPE_SIZE

#define SIZE_TYPE	"long unsigned int"
#define PTRDIFF_TYPE	"int"
#define WCHAR_TYPE	"unsigned int"
#define WCHAR_TYPE_SIZE BITS_PER_WORD

/* Temporarily turn off long double being 96 bits.  */
#undef LONG_DOUBLE_TYPE_SIZE

/* Tell final.c we don't need a label passed to mcount.  */
#define NO_PROFILE_DATA

#undef  FUNCTION_PROFILER
#define FUNCTION_PROFILER(FILE, LABELNO) fprintf (FILE, "\tcall _mcount\n")

/* A C expression that is 1 if the RTX X is a constant which is a
   valid address.  On most machines, this can be defined as
   `CONSTANT_P (X)', but a few machines are more restrictive in
   which constant addresses are supported.

   `CONSTANT_P' accepts integer-values expressions whose values are
   not explicitly known, such as `symbol_ref', `label_ref', and
   `high' expressions and `const' arithmetic expressions, in
   addition to `const_int' and `const_double' expressions.  */

#define CONSTANT_ADDRESS_P_ORIG(X)					\
  (GET_CODE (X) == LABEL_REF || GET_CODE (X) == SYMBOL_REF		\
   || GET_CODE (X) == CONST_INT || GET_CODE (X) == CONST		\
   || GET_CODE (X) == HIGH)

#undef	CONSTANT_ADDRESS_P
#define CONSTANT_ADDRESS_P(X)                                           \
  ((CONSTANT_ADDRESS_P_ORIG (X)) && (!HALF_PIC_P () || !HALF_PIC_ADDRESS_P (X)))

/* Nonzero if the constant value X is a legitimate general operand.
   It is given that X satisfies CONSTANT_P or is a CONST_DOUBLE.  */

#undef	LEGITIMATE_CONSTANT_P
#define LEGITIMATE_CONSTANT_P(X)					\
  (!HALF_PIC_P ()							\
   || GET_CODE (X) == CONST_DOUBLE					\
   || GET_CODE (X) == CONST_INT						\
   || !HALF_PIC_ADDRESS_P (X))

/* GO_IF_LEGITIMATE_ADDRESS recognizes an RTL expression
   that is a valid memory address for an instruction.
   The MODE argument is the machine mode for the MEM expression
   that wants to use this address. */

#define GO_IF_LEGITIMATE_ADDRESS_ORIG(MODE, X, ADDR)			\
{									\
  if (CONSTANT_ADDRESS_P (X)						\
      && (! flag_pic || LEGITIMATE_PIC_OPERAND_P (X)))			\
    goto ADDR;								\
  GO_IF_INDEXING (X, ADDR);						\
  if (GET_CODE (X) == PLUS && CONSTANT_ADDRESS_P (XEXP (X, 1)))		\
    {									\
      rtx x0 = XEXP (X, 0);						\
      if (! flag_pic || ! SYMBOLIC_CONST (XEXP (X, 1)))			\
	{ GO_IF_INDEXING (x0, ADDR); }					\
      else if (x0 == pic_offset_table_rtx)				\
	goto ADDR;							\
      else if (GET_CODE (x0) == PLUS)					\
	{								\
	  if (XEXP (x0, 0) == pic_offset_table_rtx)			\
	    { GO_IF_INDEXABLE_BASE (XEXP (x0, 1), ADDR); }		\
	  if (XEXP (x0, 1) == pic_offset_table_rtx)			\
	    { GO_IF_INDEXABLE_BASE (XEXP (x0, 0), ADDR); }		\
	}								\
    }									\
}

#undef	GO_IF_LEGITIMATE_ADDRESS
#define GO_IF_LEGITIMATE_ADDRESS(MODE, X, ADDR)				\
{									\
  if (! HALF_PIC_P ())							\
    {									\
      GO_IF_LEGITIMATE_ADDRESS_ORIG(MODE, X, ADDR);			\
    }									\
  else									\
    {									\
      if (CONSTANT_P (X) && ! HALF_PIC_ADDRESS_P (X))			\
	goto ADDR;							\
									\
      GO_IF_INDEXING (X, ADDR);						\
      if (GET_CODE (X) == PLUS)						\
	{								\
	  rtx x1 = XEXP (X, 1);						\
									\
	  if (CONSTANT_P (x1) && ! HALF_PIC_ADDRESS_P (x1))		\
	    {								\
	      rtx x0 = XEXP (X, 0);					\
	      GO_IF_INDEXING (x0, ADDR);				\
	    }								\
	}								\
    }									\
}

/* Sometimes certain combinations of command options do not make sense
   on a particular target machine.  You can define a macro
   `OVERRIDE_OPTIONS' to take account of this.  This macro, if
   defined, is executed once just after all the command options have
   been parsed.  */

#define OVERRIDE_OPTIONS						\
{									\
  /*									\
  if (TARGET_ELF && TARGET_HALF_PIC)					\
    {									\
      target_flags &= ~MASK_HALF_PIC;					\
      flag_pic = 1;							\
    }									\
  */									\
									\
  if (TARGET_ROSE && flag_pic)						\
    {									\
      target_flags |= MASK_HALF_PIC;					\
      flag_pic = 0;							\
    }									\
									\
  if (TARGET_HALF_PIC)							\
    half_pic_init ();							\
}

/* Define this macro if references to a symbol must be treated
   differently depending on something about the variable or
   function named by the symbol (such as what section it is in).

   The macro definition, if any, is executed immediately after the
   rtl for DECL has been created and stored in `DECL_RTL (DECL)'. 
   The value of the rtl will be a `mem' whose address is a
   `symbol_ref'.

   The usual thing for this macro to do is to a flag in the
   `symbol_ref' (such as `SYMBOL_REF_FLAG') or to store a modified
   name string in the `symbol_ref' (if one bit is not enough
   information).

   The best way to modify the name string is by adding text to the
   beginning, with suitable punctuation to prevent any ambiguity. 
   Allocate the new name in `saveable_obstack'.  You will have to
   modify `ASM_OUTPUT_LABELREF' to remove and decode the added text
   and output the name accordingly.

   You can also check the information stored in the `symbol_ref' in
   the definition of `GO_IF_LEGITIMATE_ADDRESS' or
   `PRINT_OPERAND_ADDRESS'. */

#undef	ENCODE_SECTION_INFO
#define ENCODE_SECTION_INFO(DECL)					\
do									\
  {									\
   if (HALF_PIC_P ())						        \
      HALF_PIC_ENCODE (DECL);						\
  }									\
while (0)


/* Given a decl node or constant node, choose the section to output it in
   and select that section.  */

#undef	SELECT_RTX_SECTION
#define SELECT_RTX_SECTION(MODE, RTX)					\
do									\
  {									\
    if (MODE == Pmode && HALF_PIC_P () && HALF_PIC_ADDRESS_P (RTX))	\
      data_section ();							\
    else								\
      readonly_data_section ();						\
  }									\
while (0)

#undef	SELECT_SECTION
#define SELECT_SECTION(DECL, RELOC)					\
{									\
  if (RELOC && HALF_PIC_P ())						\
    data_section ();							\
									\
  else if (TREE_CODE (DECL) == STRING_CST)				\
    {									\
      if (flag_writable_strings)					\
	data_section ();						\
      else								\
	readonly_data_section ();					\
    }									\
									\
  else if (TREE_CODE (DECL) != VAR_DECL)				\
    readonly_data_section ();						\
									\
  else if (!TREE_READONLY (DECL))					\
    data_section ();							\
									\
  else									\
    readonly_data_section ();						\
}


/* Define the strings used for the special svr4 .type and .size directives.
   These strings generally do not vary from one system running svr4 to
   another, but if a given system (e.g. m88k running svr) needs to use
   different pseudo-op names for these, they may be overridden in the
   file which includes this one.  */

#define TYPE_ASM_OP	".type"
#define SIZE_ASM_OP	".size"
#define WEAK_ASM_OP	".weak"

/* The following macro defines the format used to output the second
   operand of the .type assembler directive.  Different svr4 assemblers
   expect various different forms for this operand.  The one given here
   is just a default.  You may need to override it in your machine-
   specific tm.h file (depending upon the particulars of your assembler).  */

#define TYPE_OPERAND_FMT	"@%s"

/* A C statement (sans semicolon) to output to the stdio stream
   STREAM any text necessary for declaring the name NAME of an
   initialized variable which is being defined.  This macro must
   output the label definition (perhaps using `ASM_OUTPUT_LABEL'). 
   The argument DECL is the `VAR_DECL' tree node representing the
   variable.

   If this macro is not defined, then the variable name is defined
   in the usual manner as a label (by means of `ASM_OUTPUT_LABEL').  */

#undef	ASM_DECLARE_OBJECT_NAME
#define ASM_DECLARE_OBJECT_NAME(STREAM, NAME, DECL)			\
do									\
 {									\
   ASM_OUTPUT_LABEL(STREAM,NAME);                                       \
   HALF_PIC_DECLARE (NAME);						\
   if (TARGET_ELF)							\
     {									\
       fprintf (STREAM, "\t%s\t ", TYPE_ASM_OP);			\
       assemble_name (STREAM, NAME);					\
       putc (',', STREAM);						\
       fprintf (STREAM, TYPE_OPERAND_FMT, "object");			\
       putc ('\n', STREAM);						\
       if (!flag_inhibit_size_directive)				\
	 {								\
	   fprintf (STREAM, "\t%s\t ", SIZE_ASM_OP);			\
	   assemble_name (STREAM, NAME);				\
	   fprintf (STREAM, ",%d\n",  int_size_in_bytes (TREE_TYPE (decl))); \
	 }								\
     }									\
 }									\
while (0)

/* This is how to declare a function name. */

#undef	ASM_DECLARE_FUNCTION_NAME
#define ASM_DECLARE_FUNCTION_NAME(STREAM,NAME,DECL)			\
do									\
 {									\
   ASM_OUTPUT_LABEL(STREAM,NAME);					\
   HALF_PIC_DECLARE (NAME);						\
   if (TARGET_ELF)							\
     {									\
       fprintf (STREAM, "\t%s\t ", TYPE_ASM_OP);			\
       assemble_name (STREAM, NAME);					\
       putc (',', STREAM);						\
       fprintf (STREAM, TYPE_OPERAND_FMT, "function");			\
       putc ('\n', STREAM);						\
       ASM_DECLARE_RESULT (STREAM, DECL_RESULT (DECL));			\
     }									\
 }									\
while (0)

/* Write the extra assembler code needed to declare a function's result.
   Most svr4 assemblers don't require any special declaration of the
   result value, but there are exceptions.  */

#ifndef ASM_DECLARE_RESULT
#define ASM_DECLARE_RESULT(FILE, RESULT)
#endif

/* This is how to declare the size of a function.  */

#define ASM_DECLARE_FUNCTION_SIZE(FILE, FNAME, DECL)			\
do									\
  {									\
    if (TARGET_ELF && !flag_inhibit_size_directive)			\
      {									\
        char label[256];						\
	static int labelno;						\
	labelno++;							\
	ASM_GENERATE_INTERNAL_LABEL (label, "Lfe", labelno);		\
	ASM_OUTPUT_INTERNAL_LABEL (FILE, "Lfe", labelno);		\
	fprintf (FILE, "/\t%s\t ", SIZE_ASM_OP);			\
	assemble_name (FILE, (FNAME));					\
        fprintf (FILE, ",");						\
	assemble_name (FILE, label);					\
        fprintf (FILE, "-");						\
	assemble_name (FILE, (FNAME));					\
	putc ('\n', FILE);						\
      }									\
  }									\
while (0)

/* Attach a special .ident directive to the end of the file to identify
   the version of GCC which compiled this code.  The format of the
   .ident string is patterned after the ones produced by native svr4
   C compilers.  */

#define IDENT_ASM_OP ".ident"

/* Allow #sccs in preprocessor.  */

#define SCCS_DIRECTIVE

/* This says what to print at the end of the assembly file */
#define ASM_FILE_END(STREAM)						\
do									\
  {									\
    if (HALF_PIC_P ())							\
      HALF_PIC_FINISH (STREAM);						\
									\
    if (TARGET_IDENT)							\
      {									\
	fprintf ((STREAM), "\t%s\t\"GCC: (GNU) %s -O%d",		\
		 IDENT_ASM_OP, version_string, optimize);		\
									\
	if (write_symbols == PREFERRED_DEBUGGING_TYPE)			\
	  fprintf ((STREAM), " -g%d", (int)debug_info_level);		\
									\
	else if (write_symbols == DBX_DEBUG)				\
	  fprintf ((STREAM), " -gstabs%d", (int)debug_info_level);	\
									\
	else if (write_symbols == DWARF_DEBUG)				\
	  fprintf ((STREAM), " -gdwarf%d", (int)debug_info_level);	\
									\
	else if (write_symbols != NO_DEBUG)				\
	  fprintf ((STREAM), " -g??%d", (int)debug_info_level);		\
									\
	if (flag_omit_frame_pointer)					\
	  fprintf ((STREAM), " -fomit-frame-pointer");			\
									\
	if (flag_strength_reduce)					\
	  fprintf ((STREAM), " -fstrength-reduce");			\
									\
	if (flag_unroll_loops)						\
	  fprintf ((STREAM), " -funroll-loops");			\
									\
	if (flag_force_mem)						\
	  fprintf ((STREAM), " -fforce-mem");				\
									\
	if (flag_force_addr)						\
	  fprintf ((STREAM), " -fforce-addr");				\
									\
	if (flag_inline_functions)					\
	  fprintf ((STREAM), " -finline-functions");			\
									\
	if (flag_caller_saves)						\
	  fprintf ((STREAM), " -fcaller-saves");			\
									\
	if (flag_pic)							\
	  fprintf ((STREAM), (flag_pic > 1) ? " -fPIC" : " -fpic");	\
									\
	if (flag_inhibit_size_directive)				\
	  fprintf ((STREAM), " -finhibit-size-directive");		\
									\
	if (flag_gnu_linker)						\
	  fprintf ((STREAM), " -fgnu-linker");				\
									\
	if (profile_flag)						\
	  fprintf ((STREAM), " -p");					\
									\
	if (profile_block_flag)						\
	  fprintf ((STREAM), " -a");					\
									\
	if (TARGET_IEEE_FP)						\
	  fprintf ((STREAM), " -mieee-fp");				\
									\
	if (TARGET_HALF_PIC)						\
	  fprintf ((STREAM), " -mhalf-pic");				\
									\
	fprintf ((STREAM), (TARGET_486) ? " -m486" : " -m386");		\
	fprintf ((STREAM), (TARGET_ELF) ? " -melf\"\n" : " -mrose\"\n"); \
      }									\
  }									\
while (0)

/* Tell collect that the object format is OSF/rose.  */
#define OBJECT_FORMAT_ROSE

/* Tell collect where the appropriate binaries are.  */
#define REAL_LD_FILE_NAME	"/usr/ccs/gcc/gld"
#define REAL_NM_FILE_NAME	"/usr/ccs/bin/nm"
#define REAL_STRIP_FILE_NAME	"/usr/ccs/bin/strip"

/* Use atexit for static constructors/destructors, instead of defining
   our own exit function.  */
#define HAVE_ATEXIT

/* Define this macro meaning that gcc should find the library 'libgcc.a'
   by hand, rather than passing the argument '-lgcc' to tell the linker
   to do the search */
#define LINK_LIBGCC_SPECIAL

/* A C statement to output assembler commands which will identify the object
  file as having been compile with GNU CC. We don't need or want this for
  OSF1. GDB doesn't need it and kdb doesn't like it */
#define ASM_IDENTIFY_GCC(FILE)

/* Identify the front-end which produced this file.  To keep symbol
   space down, and not confuse kdb, only do this if the language is
   not C.  */

#define ASM_IDENTIFY_LANGUAGE(STREAM)					\
{									\
  if (strcmp (lang_identify (), "c") != 0)				\
    output_lang_identify (STREAM);					\
}

/* This is how to output an assembler line defining a `double' constant.
   Use "word" pseudos to avoid printing NaNs, infinity, etc.  */

/* This is how to output an assembler line defining a `double' constant.  */

#undef	ASM_OUTPUT_DOUBLE

#ifndef	CROSS_COMPILE
#define	ASM_OUTPUT_DOUBLE(STREAM, VALUE)				\
do									\
  {									\
    long value_long[2];							\
    REAL_VALUE_TO_TARGET_DOUBLE (VALUE, value_long);			\
									\
    fprintf (STREAM, "\t.long\t0x%08lx\t\t# %.20g\n\t.long\t0x%08lx\n",	\
	   value_long[0], VALUE, value_long[1]);			\
  }									\
while (0)

#else
#define	ASM_OUTPUT_DOUBLE(STREAM, VALUE)				\
  fprintf (STREAM, "\t.double\t%.20g\n", VALUE)
#endif

/* This is how to output an assembler line defining a `float' constant.  */

#undef	ASM_OUTPUT_FLOAT

#ifndef	CROSS_COMPILE
#define	ASM_OUTPUT_FLOAT(STREAM, VALUE)					\
do									\
  {									\
    long value_long;							\
    REAL_VALUE_TO_TARGET_SINGLE (VALUE, value_long);			\
									\
    fprintf (STREAM, "\t.long\t0x%08lx\t\t# %.12g (float)\n",		\
	   value_long, VALUE);						\
  }									\
while (0)

#else
#define	ASM_OUTPUT_FLOAT(STREAM, VALUE)					\
  fprintf (STREAM, "\t.float\t%.12g\n", VALUE)
#endif


/* Generate calls to memcpy, etc., not bcopy, etc. */
#define TARGET_MEM_FUNCTIONS

/* Don't default to pcc-struct-return, because gcc is the only compiler, and
   we want to retain compatibility with older gcc versions.  */
#define DEFAULT_PCC_STRUCT_RETURN 0

/* Map i386 registers to the numbers dwarf expects.  Of course this is different
   from what stabs expects.  */

#define DWARF_DBX_REGISTER_NUMBER(n) \
((n) == 0 ? 0 \
 : (n) == 1 ? 2 \
 : (n) == 2 ? 1 \
 : (n) == 3 ? 3 \
 : (n) == 4 ? 6 \
 : (n) == 5 ? 7 \
 : (n) == 6 ? 5 \
 : (n) == 7 ? 4 \
 : ((n) >= FIRST_STACK_REG && (n) <= LAST_STACK_REG) ? (n)+3 \
 : (-1))

/* Now what stabs expects in the register.  */
#define STABS_DBX_REGISTER_NUMBER(n) \
((n) == 0 ? 0 : \
 (n) == 1 ? 2 : \
 (n) == 2 ? 1 : \
 (n) == 3 ? 3 : \
 (n) == 4 ? 6 : \
 (n) == 5 ? 7 : \
 (n) == 6 ? 4 : \
 (n) == 7 ? 5 : \
 (n) + 4)

#undef	DBX_REGISTER_NUMBER
#define DBX_REGISTER_NUMBER(n) ((write_symbols == DWARF_DEBUG)		\
				? DWARF_DBX_REGISTER_NUMBER(n)		\
				: STABS_DBX_REGISTER_NUMBER(n))
