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

/* This file contains the code used to control what GCT does */

/*
 * Notes on filename handling.
 *
 * What about include files?
 *
 * Currently, the control file applies either to all text included in the
 * main_input_filename or only the text in the original
 * main_input_filename, depending on the instrument-included-files
 * option.  This is not really powerful enough; you would prefer to
 * control included files individually (much as you do routines).
 * Future enhancements.
 *
 * What about multiple directories?
 *
 * Pathnames must be absolute or relative to the master directory (where
 * the control file is).  Of course, the compiler may not be running in
 * the master directory.  If the file is named in the control file, it's
 * easy:  just use the control file name.  But the file may not be named:
 * 1.  the FILES option may be turned on.
 * 2.  the file may be an include file.
 *
 * How to construct the relative directory?  Because of symlinks, it's
 * not safe to run around splicing names together.  So we punt and
 * construct an absolute pathname.
 */

#include <stdio.h>
#include "gct-contro.h"
#include "gct-assert.h"
#include <sys/stat.h>
#include <sys/errno.h>
#include "gct-files.h"

/*
 * This array comprises the context stack.  It's extern, alas, so that
 * the fast-access routines in gct-contro.h can get at it.
 */

gct_option_context *(Context_stack[NUM_CONTEXTS]);

/*
 * NOTE: The global context (Context_stack[GLOBAL_CONTEXT]) is always the
 * root of the parsed control file.
 */

/* This array is used to map from option-ids to names */
char *Option_names[] =
{
#define GCTOPT(OPT, NAME, GS, GD, FS, FD, RS, RD, CS, CD, USE)	NAME,
#include "gct-opts.def"
#undef GCTOPT
  (char *)0		/* Because of trailing comma */
};

/* This array is used to map from option-ids to their uses */
char Option_use[] =
{
#define GCTOPT(OPT, NAME, GS, GD, FS, FD, RS, RD, CS, CD, USE)	USE,
#include "gct-opts.def"
#undef GCTOPT
  0			/* Because of trailing comma */
};


char *Gct_log_filename = "\"GCTLOG\"";	/* Argument of the logfile command. */


/*
 * This looks up an option and returns its id.  LAST_AND_UNUSED_OPTION is
 * returned to denote failure.  
 */
static enum gct_option_id
option_name_to_id(name)
     char *name;
{
  int i;

  for (i = 0; i < GCT_NUM_OPTIONS; i++)
    {
      if (!strcmp(Option_names[i], name))
	return (enum gct_option_id) i;
    }
  return LAST_AND_UNUSED_OPTION;
}  



		       /* Generic Utilities */


/* Look up the value of the option in the context stack. */
enum gct_option_value
gct_option_value(optid)
     enum gct_option_id optid;
{
  enum gct_option_value value;
  int i;

  assert(Context_stack[GLOBAL_CONTEXT]);
  assert(Context_stack[CACHE_CONTEXT]);

  for (i = NUM_CONTEXTS-1; i >=0; i--)
    {
      if (   Context_stack[i]
	  && NONE != (value = Context_stack[i]->options[(int)optid].value))
	{
	  return value;
	}
    }
  fatal("No value for option %d\n", optid);
}


/* Set the value of the given option in the given context */
gct_set_option(context_level, optid, value)
     int context_level;
     enum gct_option_id optid;
     enum gct_option_value value;
{
  assert(CONTEXT_IN_RANGE(context_level));
  assert(Context_stack[context_level]);
  
  Context_stack[context_level]->options[(int)optid].value = value;
}


/* Push a new value in the given context. */
gct_push_option(context_level, optid, value)
     int context_level;
     enum gct_option_id optid;
     enum gct_option_value value;
{
  gct_option *stacked = (gct_option *) xmalloc(sizeof(gct_option));

  assert(CONTEXT_IN_RANGE(context_level));
  assert(Context_stack[context_level]);

  *stacked = Context_stack[context_level]->options[(int)optid];
  Context_stack[context_level]->options[(int)optid].value = value;
  Context_stack[context_level]->options[(int)optid].next = stacked;
}

/* Pop the old value.  Error out if no corresponding push. */
gct_pop_option(context_level, optid)
     int context_level;
     enum gct_option_id optid;
{
  gct_option *stacked;	/* The old option stack. */

  assert(CONTEXT_IN_RANGE(context_level));
  assert(Context_stack[context_level]);

  stacked = Context_stack[context_level]->options[(int)optid].next;
  assert(stacked);

  Context_stack[context_level]->options[(int)optid] = *stacked;
  free(stacked);
}




/*
 * Determine if the given type of instrumentation is the only type turned
 * on.  This can be used to speed processing and - more importantly - to
 * avoid generating lots of useless code that has to be optimized away.
 * Initial use:  when only call instrumentation is on, don't do anything
 * to the routine.  Because of the simpleminded handling of weak
 * sufficiency, processing of the body of the routine generates lots of
 * useless temporaries.
 *
 * Note:  this routine assumes the caller has already checked whether
 * instrumentation is on and whether the argument instrumentation is on.
 */
int
gct_only_instrumentation(optid)
	enum gct_option_id optid;
{
  int i;

  for (i = 0; i < GCT_NUM_OPTIONS; i++)
  {
    if (   (enum gct_option_id) i != optid
	&& Option_use[i]!=OPT_OPTION
	&& gct_option_value((enum gct_option_id) i) == ON)
    {
      return 0;
    }
  }
  return 1;
}

/*
 * Determine whether any of the instrumentation types are set.
 * Note:  this routine assumes the caller has already checked whether
 * instrumentation is on in general.
 */
int
gct_any_instrumentation_on()
{
  int i;

  for (i = 0; i < GCT_NUM_OPTIONS; i++)
  {
    if (   Option_use[i]!=OPT_OPTION
	&& gct_option_value((enum gct_option_id) i) == ON)
    {
      return 1;
    }
  }
  return 0;
}
  
/*
 * Return whether standard instrumentation, weak instrumentation, or
 * neither, or both are turned on.  One or the other can be turned on
 * with either the "force_weak" or "force_standard" options, or by the
 * coverage directives in the control file.  
 *
 * No error is signalled if both are turned on; that's the caller's
 * responsibility.
 */
int 
gct_instrumentation_uses()
{
  int result = 0;
  int i;

  if (   ON == gct_option_value(OPT_FORCE_WEAK)
      || ON == gct_option_value(OPT_FORCE_STANDARD))
    {
      /* Allow options to override.  If both are set, the result is a
	 conflict, presumably handled later. */
      if (ON == gct_option_value(OPT_FORCE_WEAK))
	{
	  result |= OPT_NEED_WEAK;
	}
      
      if (ON == gct_option_value(OPT_FORCE_STANDARD))
	{
	  result |= OPT_NEED_STANDARD;
	}
    }

  for (i = 0; i < GCT_NUM_OPTIONS; i++)
    {
      if (   Option_use[i]!=OPT_OPTION	/* That is, instrumentation */
	  && gct_option_value((enum gct_option_id) i) == ON)
	{
	  result |= Option_use[i];
	}
    }
  return result;
}


		   /* Building the Control File */

/*
 * This structure is used to map from ("user-keyword", context_level) to a
 * default value for an option.
 *
 * Note:  the number of options is one larger than required, because we
 * have to shove in a dummy entry to soak up the last comma.
 */

static enum gct_option_value
Option_defaults[NUM_CONTEXTS][GCT_NUM_OPTIONS+1] =	
{
  {
#define GCTOPT(OPT, NAME, GS, GD, FS, FD, RS, RD, CS, CD, USE)	GD,
#include "gct-opts.def"
#undef GCTOPT
	NONE	/* Because of trailing comma */
  },
  {
#define GCTOPT(OPT, NAME, GS, GD, FS, FD, RS, RD, CS, CD, USE)	FD,
#include "gct-opts.def"
#undef GCTOPT
	NONE	/* Because of trailing comma */
  },
  {
#define GCTOPT(OPT, NAME, GS, GD, FS, FD, RS, RD, CS, CD, USE)	RD,
#include "gct-opts.def"
#undef GCTOPT
	NONE	/* Because of trailing comma */
  },
  {
#define GCTOPT(OPT, NAME, GS, GD, FS, FD, RS, RD, CS, CD, USE)	CD,
#include "gct-opts.def"
#undef GCTOPT
	NONE	/* Because of trailing comma */
  }
};





/*
 * This structure is used to map from (option_id, context_level) to a bit
 * telling us whether that option is valid at that level.
 *
 * Note:  the number of options is one larger than required, because we
 * have to shove in a dummy entry to soak up the last comma.
 */

char
Option_validity[NUM_CONTEXTS][GCT_NUM_OPTIONS+1] =	
{
  {
#define GCTOPT(OPT, NAME, GS, GD, FS, FD, RS, RD, CS, CD, USE)	GS,
#include "gct-opts.def"
#undef GCTOPT
	0	/* Because of trailing comma */
  },
  {
#define GCTOPT(OPT, NAME, GS, GD, FS, FD, RS, RD, CS, CD, USE)	FS,
#include "gct-opts.def"
#undef GCTOPT
	0	/* Because of trailing comma */
  },
  {
#define GCTOPT(OPT, NAME, GS, GD, FS, FD, RS, RD, CS, CD, USE)	RS,
#include "gct-opts.def"
#undef GCTOPT
	0	/* Because of trailing comma */
  },
  {
#define GCTOPT(OPT, NAME, GS, GD, FS, FD, RS, RD, CS, CD, USE)	CS,
#include "gct-opts.def"
#undef GCTOPT
	0	/* Because of trailing comma */
  }
};





gct_option_context *
gct_make_context(context_level)
     int context_level;
{
  gct_option_context *context;
  int index;
  
  assert(CONTEXT_IN_RANGE(context_level));

  context = (gct_option_context *) xmalloc(sizeof(gct_option_context));
  context->tag = (char *)0;
  context->inode = 0;			/* Cannot match any file. */
  context->next = NULL_GCT_OPTION_CONTEXT;
  context->children = NULL_GCT_OPTION_CONTEXT;
  for(index = 0; index < GCT_NUM_OPTIONS; index++)
    {
      /* Hoist that constant, compiler!  Heave that bale! */
      context->options[index].value =
	Option_defaults[context_level][index];
      context->options[index].next = NULL_GCT_OPTION;
    }
  return context;
}


static gct_option_context *lower_level_context();

/* Build the control file from FILENAME */
gct_control_init(control_file)
     char *control_file;
{
  FILE *stream;
  extern int errno;
  
  /* There is but one global context.  Build it. */
  Context_stack[GLOBAL_CONTEXT] = gct_make_context(GLOBAL_CONTEXT);
  Context_stack[GLOBAL_CONTEXT]->tag = "global context";
  
  /* Ditto for the cache context. */
  Context_stack[CACHE_CONTEXT] = gct_make_context(CACHE_CONTEXT);
  Context_stack[CACHE_CONTEXT]->tag = "cache context";

  /*
   * gct-write.c and gct-ps-defs.c are special - we don't instrument
   * them even if (options files).  Note that these files, like all
   * files, are relative to the master directory.
   */
  (void) lower_level_context(permanent_string("gct-write.c"),
			     Context_stack[GLOBAL_CONTEXT], GLOBAL_CONTEXT, 1);
  (void) lower_level_context(permanent_string("gct-ps-defs.c"),
			     Context_stack[GLOBAL_CONTEXT], GLOBAL_CONTEXT, 1);

  /* Time for the excitement of recursive descent parsing. */
  stream = fopen(control_file, "r");
  if (NULL == stream )
    fatal("Couldn't open control file %s\n", control_file);

  gct_parse(stream, Context_stack[GLOBAL_CONTEXT], GLOBAL_CONTEXT);
  fclose(stream);
}



/* Lexing the control file */

/* The lexer returns these tokens. */
#define TOK_ID		1
#define TOK_MINUS	2
#define TOK_LPAREN	3
#define TOK_RPAREN	4
#define TOK_EOF		5


#define MAX_TOKEN	1000	/* Kludge -- last_token should grow. */
static int Line_count = 1;
static char Last_token[MAX_TOKEN+1];


#define WHITE(char) \
       ('\n' == (char) || ' ' == (char) || '\t' == (char) || '#' == (char) ||\
	'\f' == (char) || '\r' == (char) || '\v'== (char) || '\b' == (char) )


/* Yes, Virginia, files beginning with - lose. */	
#define ID(c)	\
	(!WHITE(c) && (c) != EOF && (c) != '(' && (c) != ')')

static 
skip_white(stream)
     FILE *stream;
{
  int c;

  for (c = getc(stream); WHITE(c); c = getc(stream))
    {
      if ('#' == c)
	for (c = getc(stream); '\n' !=c && EOF != c; c = getc(stream))
	  {
	  }
      if ('\n' == c)
	Line_count++;
    }
  ungetc(c, stream);
}

/* First char is guaranteed non-white. */
static skip_token(stream)
     FILE *stream;
{
  int c;
  int count = 0;
  
  for (c = getc(stream); ID(c); c = getc(stream))
    {
      if (count == MAX_TOKEN)
	{
	  Last_token[MAX_TOKEN] = '\0';	/* There's room for a null. */
	  fatal_parse_error("token too large");
	}
      Last_token[count] = c;
      count++;
    }
  Last_token[count] = '\0';
  ungetc(c, stream);
}

int
gct_read_token(stream)
     FILE *stream;
{
  int c;

  Last_token[0] = '\0';
  skip_white(stream);
  c = getc(stream);
  switch(c)
    {
    case '-':
      Last_token[0] = '-';
      Last_token[1] = '\0';
      return(TOK_MINUS);
      break;
    case '(':
      Last_token[0] = '(';
      Last_token[1] = '\0';
      return(TOK_LPAREN);
      break;
    case ')':
      Last_token[0] = ')';
      Last_token[1] = '\0';
      return(TOK_RPAREN);
      break;
    case EOF:
      Last_token[0] = '\0';
      return(TOK_EOF);
      break;
    default:
      ungetc(c, stream);
      skip_token(stream);
      return(TOK_ID);
      break;
    }
}




/* Error handling */

/* First parse error causes program to bomb out. */
fatal_parse_error(string)
     char *string;
{
  fatal("control file, line %d: %s", Line_count, string);
}


/*
 * At a level of the hierarchy (global, file, routine, cache), a
 * lower-level entity can be named.  This routine checks whether a
 * lower-level entity is appropriate, creates it, sets the
 * instrumentation appropriately (depends on the level), and links it
 * into the parent context's list of children (in reverse order).
 */

static gct_option_context *
lower_level_context(tag, parent_context, parent_level, minus_seen)
     char *tag;
     gct_option_context *parent_context;
     int parent_level;
     int minus_seen;
{
  gct_option_context *new_context;
  
  if (parent_level != GLOBAL_CONTEXT && parent_level != FILE_CONTEXT)
    {
      fatal_parse_error("You have nested the control file too deeply.");
    }

  if (!strcmp(tag, "option"))
    warning("Creating file/routine named 'option'; did you mean 'options'?");
  if (!strcmp(tag, "instrument"))
    warning("Creating file/routine named 'instrument'; did you mean 'coverage'?");  if (!strcmp(tag, "instrumentation"))
    warning("Creating file/routine named 'instrumentation'; did you mean 'coverage'?");


  new_context = gct_make_context(parent_level+1);
  new_context->tag = tag;

  new_context->options[OPT_INSTRUMENT].value = minus_seen?OFF:ON;
  new_context->next = parent_context->children;
  parent_context->children = new_context;
  return new_context;
}


/* Parsing the control file. */

/*
 * This routine grovels the control file, building a substructure below
 * CONTEXT, which must be at CONTEXT_LEVEL.  In the first call, CONTEXT
 * is the global context and the level is GLOBAL_CONTEXT.  Children are
 * pushed on the context->children list in reverse order.  This matters
 * not at all.
 */
gct_parse(stream, context, context_level)
     FILE *stream;
     gct_option_context *context;
     int context_level;
{
  assert(CONTEXT_IN_RANGE(context_level));
  assert(stream);
  
  while(1)
    {
      int token = gct_read_token(stream);
      gct_option_context *new_context;
      
      switch (token)
	{
	case TOK_ID:
	  (void) lower_level_context(permanent_string(Last_token),
			      context, context_level, 0);
	  break;
	case TOK_MINUS:
	  token = gct_read_token(stream);
	  if (TOK_ID == token)
	    {
	      (void) lower_level_context(permanent_string(Last_token),
				  context, context_level, 1);
	    }
	  else
	    {
	      fatal_parse_error("identifier must follow -");
	    }
	  break;
	case TOK_LPAREN:
	  token = gct_read_token(stream);
	  if (   !strcmp(Last_token, "options")
	      || !strcmp(Last_token, "coverage"))
	    {
	      parse_options(stream, context, context_level);
	      break;	/* Done with parenthesized list. */
	    }
	  if (!strcmp(Last_token, "logfile"))
	    {
	      if (GLOBAL_CONTEXT != context_level)
		{
		  fatal_parse_error("'logfile' keyword only allowed at the top level.");
		}
	      parse_logfile(stream);
	      break;	/* Done with parenthesized list. */
	    }
	      
	  
	  /* Check for "routine" keyword and skip it. */
	  if (!strcmp(Last_token, "routine"))
	    {
	      if (FILE_CONTEXT != context_level)
		{
		  fatal_parse_error("'routine' keyword only allowed in a file description.");
		}
	      else	/* Skip it. */
		{
		  token = gct_read_token(stream);
		}
	    }
	  
	  if (TOK_ID == token)
	    {
	      new_context = lower_level_context(permanent_string(Last_token),
						context, context_level, 0);
	      gct_parse(stream, new_context, context_level+1);
	    }
	  else
	    {
	      fatal_parse_error("identifier must follow (");
	    }
	  break;
	case TOK_RPAREN:
	  if (context_level > GLOBAL_CONTEXT)
	    return;	/* Finished recursive call. */
	  else
	    fatal_parse_error("unexpected ')'");
	  break;
	  
	case TOK_EOF:
	  if (context_level != GLOBAL_CONTEXT)
	    fatal_parse_error("unexpected EOF");
	  else
	    return;	/* Finished main call */
	  break;
	  
	default:
	  fatal_parse_error("Parser internal error.");
	  break;
	}
    }
}


/*
 * A parenthesis has been read. Read option commands until a right paren
 * is read.
 */
parse_options(stream, context, context_level)
  FILE *stream;
  gct_option_context *context;
  int context_level;
{
  enum gct_option_id id;	/* The option specified. */

  assert(CONTEXT_IN_RANGE(context_level));
  assert(stream);

  while(1)
    {
      int token = gct_read_token(stream);
      
      switch (token)
	{
	case TOK_ID:
	  id = option_name_to_id(Last_token);
	  if (LAST_AND_UNUSED_OPTION == id)
	    {
	      fatal_parse_error("No such option or coverage type.");
	    }
	  else if (Option_validity[context_level][(int)id])
	    {
	      context->options[(int)id].value = ON;
	    }
	  else
	    {
	      fatal_parse_error("Illegal option in this context.");
	    }
	  break;
	case TOK_MINUS:
	  token = gct_read_token(stream);
	  if (TOK_ID == token)
	    {
	      id = option_name_to_id(Last_token);
	      if (LAST_AND_UNUSED_OPTION == id)
		{
		  fatal_parse_error("No such option or coverage type.");
		}
	      else if (Option_validity[context_level][(int)id])
		{
		  context->options[(int)id].value = OFF;
		}
	      else
		{
		  fatal_parse_error("Illegal option in this context.");
		}
	    }
	  else
	    {
	      fatal_parse_error("identifier must follow -");
	    }
	  break;
	case TOK_LPAREN:
	  fatal_parse_error("No paren allowed in option list.");
	  break;
	case TOK_RPAREN:
	  return;
	  break;

	case TOK_EOF:
	    fatal_parse_error("unexpected EOF");
	  break;
	  
	default:
	  fatal_parse_error("Parser internal error (2).");
	  break;
	}
    }
}



/*
 * The "logfile" keyword has been read.  Read the following token and
 * store it.  Expect a paren afterwards.
 */
parse_logfile(stream)
  FILE *stream;
{
  int token = gct_read_token(stream);
  
  if (TOK_ID != token)
    {
      fatal_parse_error("The 'logfile' keyword requires a following filename.");
    }
  if ('"' == Last_token[0])
    fatal_parse_error("The 'logfile' argument should not be in quotes.");
  
  Gct_log_filename = (char *) xmalloc(strlen(Last_token)+3);
  sprintf(Gct_log_filename, "\"%s\"", Last_token);
  /* Testing for success harder because sprintf has different type on
     different systems. */
     

  token = gct_read_token(stream);
  if (TOK_EOF == token)
    {
      fatal_parse_error("Unexpected EOF after 'logfile' keyword.");
    }
  else if (TOK_RPAREN != token)
    {
      fatal_parse_error("The 'logfile' keyword takes a single argument.");
    }
}


  


     /* Zipping along through the file, manipulating contexts */

/*
 * This finds the FILE_CONTEXT entry with a context whose tag matches
 * NAME.  Tags are stored as written in the control file (for clarity
 * when reporting); as such, they are relative to the master directory.
 * We may not be running in the master directory, so MASTER_DIR is used
 * to construct the filename that was intended.  This filename matches
 * NAME iff their i-numbers are the same.
 *
 * A NULL_GCT_OPTION_CONTEXT is returned if there is no match.
 *
 * It's unremarkable if the file being looked up does not exist - this
 * code can get called on object files that haven't been created yet (for
 * example).
 *
 * It's less usual for a file in the control file not to exist, but it
 * happens (the C file might be generated in the middle of the build), so
 * I don't warn in that case either.
 */

gct_option_context *
gct_find_file_context(name, master_dir)
     char *name;
     char *master_dir;
{
  gct_option_context *rover;
  char *full_control_file_name;	/* stattable control file name */
  struct stat statbuf;
  
  assert(Context_stack[GLOBAL_CONTEXT]);
  
  for(rover = Context_stack[GLOBAL_CONTEXT]->children; rover; rover=rover->next)
    {
      if (0 == rover->inode)	/* First, install inode. */
	{
	  if ('/' == rover->tag[0])	/* Absolute path */
	    {
	      full_control_file_name = rover->tag;
	    }
	  else
	    {
	      full_control_file_name =
		(char *) xmalloc(2 + strlen(master_dir) + strlen(rover->tag));
	      (void) strcpy(full_control_file_name, master_dir);
	      (void) strcat(full_control_file_name, "/");
	      (void) strcat(full_control_file_name, rover->tag);
	    }
	  if (-1 != stat(full_control_file_name, &statbuf))  /* File exists. */
	    {
	      rover->inode = statbuf.st_ino;
	    }
	  if ('/' != rover->tag[0])
	    free(full_control_file_name);
	}
      if (   -1 != stat(name, &statbuf)		/* File exists and ... */
          && statbuf.st_ino == rover->inode)	/* it matches a controlfile entry */
	{
	  return rover;
	}
    }
  return NULL_GCT_OPTION_CONTEXT;
}



/*
 * Just like find_gct_file_context, except that it sets
 * Context_stack[FILE_CONTEXT] by side effect.  Note that if nothing's
 * found, the FILE_CONTEXT is set to NULL.  
 *
 * OPT_IGNORE is calculated for the global context each time this is
 * called.  That's repetitive only in the case of gcc.c.  Note that
 * OPT_INSTRUMENT, OPT_READLOG, and OPT_WRITELOG dominate OPT_IGNORE at
 * this level -- it applies only when they are OFF.
 *
 * At the file level, OPT_IGNORE dominates the other options.  If it
 * wasn't specified (explicitly) by the user, ONness of the other values
 * will turn it off.
 *
 * If any routines are specified, I presume that the processing options
 * will be turned on, so OPT_IGNORE is turned off.  It's possible that
 * the control file looks like
 *
 * 	(foo.c (options -instrument -readlog -writelog)
 * 	   (myroutine (options -instrument -readlog -writelog)))
 *
 * but that would be pretty silly, and I don't mind erroneously
 * processing that file.  If anyone ever complains, I'll add the required
 * code.
 */
void
gct_set_file_context(name, master_dir)
     char *name;
     char *master_dir;
{
  gct_option_context *file;
  assert(Context_stack[GLOBAL_CONTEXT]);
  assert(!Context_stack[FILE_CONTEXT]);

  if (   ON == gct_option_value(OPT_INSTRUMENT)
      || ON == gct_option_value(OPT_READLOG)
      || ON == gct_option_value(OPT_WRITELOG))
    {
      gct_set_option(GLOBAL_CONTEXT, OPT_IGNORE, OFF);
    }

  file = gct_find_file_context(name, master_dir);
  Context_stack[FILE_CONTEXT] = file;
  
  if (file && (NONE == file->options[OPT_IGNORE].value))
    {
      if (   ON == gct_option_value(OPT_INSTRUMENT)
	  || ON == gct_option_value(OPT_READLOG)
	  || ON == gct_option_value(OPT_WRITELOG))
	{
	  gct_set_option(FILE_CONTEXT, OPT_IGNORE, OFF);
	}
      else if (file->children)
	{
	  gct_set_option(FILE_CONTEXT, OPT_IGNORE, OFF);
	}
    }
}


gct_no_file_context()
{
  Context_stack[FILE_CONTEXT] = NULL_GCT_OPTION_CONTEXT;
}



/*
 * For mapfile output, we want to know the name of the current file, as
 * it appears in the control file.  Note that there might not be one, if
 * the "files" option is turned on.
 */
char *
gct_current_control_filename()
{
  if (Context_stack[FILE_CONTEXT])
    {
      assert(Context_stack[FILE_CONTEXT]->tag);
      return Context_stack[FILE_CONTEXT]->tag;
    }
  else
    {
      return (char *)0;
    }
}

  


/*
 * This routine locates the ROUTINENAME in the control file and
 * establishes that as the current routine.
 *
 * Note that the file context may be null -- we can enter a routine
 * without having found an explicit mention of this filename in the
 * control file.  In this case the routine context is null.
 *
 * Even if the file context does exist, there might be no matching
 * routine.  In this case, the routine context is again null.
 *
 * In addition to setting the routine context (if appropriate), the
 * following options are cached in the cache context:
 *
 * 	All coverage options.  (branch, relational, etc.)
 * 	OPT_INSTRUMENT.
 * 	OPT_READLOG
 * 	OPT_WRITELOG
 *
 * This makes lookup faster.  The caller should feel free to change
 * the cached value.
 */
gct_set_routine_context(routinename)
     char *routinename;
{
  gct_option_context *rover;
  int i;
  
  assert(!Context_stack[ROUTINE_CONTEXT]);

  
  if (Context_stack[FILE_CONTEXT])
    {
      for(rover = Context_stack[FILE_CONTEXT]->children; rover; rover=rover->next)
	{
	  if (!strcmp(rover->tag, routinename))
	    {
	      Context_stack[ROUTINE_CONTEXT] = rover;
	      break;
	    }
	}
    }
  
  /*
   * There's no problem if the routinename is not found;
   * Context_stack[ROUTINE_CONTEXT] is null. 
   */

  /* Now cache instrumentation values. */
  for(i = 0; i < GCT_NUM_OPTIONS; i++)
  {
    assert(NONE == Context_stack[CACHE_CONTEXT]->options[i].value);
    
    if (Option_use[i]!=OPT_OPTION)
    {
      gct_set_option(CACHE_CONTEXT, (enum gct_option_id) i,
		     gct_option_value((enum gct_option_id) i));
    }
  }

  gct_set_option(CACHE_CONTEXT, OPT_INSTRUMENT,
		 gct_option_value(OPT_INSTRUMENT));
  gct_set_option(CACHE_CONTEXT, OPT_READLOG,
		 gct_option_value(OPT_READLOG));
  gct_set_option(CACHE_CONTEXT, OPT_WRITELOG,
		 gct_option_value(OPT_WRITELOG));
}

gct_no_routine_context()
{
  int i;
  
  Context_stack[ROUTINE_CONTEXT] = NULL_GCT_OPTION_CONTEXT;
  
  /* Uncache cached instrumentation values */
  for(i= 0; i < GCT_NUM_OPTIONS; i++)
  {
    gct_set_option(CACHE_CONTEXT, (enum gct_option_id) i, NONE);
  }
}





			    /* Printing */



static char *
Value_names[] = { "ON", "OFF", "NONE" };



static
indent(stream, count)
     FILE *stream;
     int count;
{
  for (;count>0;count--)
    putc(' ', stream);
  
}


gct_print_context(stream, context, indent_count)
     FILE *stream;
     gct_option_context *context;
     int indent_count;
{
  int i;
  

  if (! context)
    {
      fprintf(stream, "NO CONTEXT\n");
      return;
    }
  
  indent(stream, indent_count);
  fprintf(stream, "CONTEXT:  %s",
	  context->tag?context->tag: "unnamed");
  for (i = 0; i < FIRST_UNPRINTED_OPTION; i++)
    {
      gct_option *rover = &(context->options[i]);
      if (i % 5 == 0)
	{
	  putc('\n', stream);
	  indent(stream, indent_count);
	}
      fprintf(stream, "%s=%s ", Option_names[i],
	      Value_names[(int)(rover->value)]);
      rover = rover->next;
      if (rover)
	{
	  putc('(', stream);
	  for (; rover; rover= rover->next)
	    {
	      fprintf(stream, "%s ", Value_names[(int)(rover->value)]);
	    }
	  putc(')', stream);
	}
    }
  putc('\n', stream);
}

gct_print_context_stack(stream)
     FILE *stream;
{
  fprintf(stream, "CACHE CONTEXT:\n");
  gct_print_context(stream, Context_stack[CACHE_CONTEXT], 0);
  fprintf(stream, "ROUTINE CONTEXT:\n");
  gct_print_context(stream, Context_stack[ROUTINE_CONTEXT], 0);
  fprintf(stream, "FILE CONTEXT:\n");
  gct_print_context(stream, Context_stack[FILE_CONTEXT], 0);
  fprintf(stream, "GLOBAL CONTEXT:\n");
  gct_print_context(stream, Context_stack[GLOBAL_CONTEXT], 0);
}


gct_print_control_file(stream)
     FILE *stream;
{
  fprintf(stream, "Control file:\n");
  gct_print_context_list(stream, Context_stack[GLOBAL_CONTEXT], 0);
}


gct_print_context_list(stream, contextlist, indent_count)
     FILE *stream;
     gct_option_context *contextlist;
     int indent_count;
{
  for (; contextlist; contextlist = contextlist->next)
    {
      gct_print_context(stream, contextlist, indent_count);
      gct_print_context_list(stream, contextlist->children, indent_count+2);
    }
}
