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
 * Routines that write the mapfile. 
 *
 * General rules:
 *
 * There is a mapfile logging routine for each type of instrumentation.
 * They all take the index and node-being-instrumented as an argument.
 * In addition, they may take one or more of the following arguments:
 *
 * TAG:  the token identifying the kind of instrumentation.  This is
 * constant, except for branch instrumentation, where the tag is the name
 * of the node.
 *
 * NAME: An additional identifying name for the user's convenience.
 *
 * REST_TEXT:  Random text that's put in the map file; to be blindly
 * printed by greport.
 *
 * If the TAG and NAME match the tag and name of a previous entry on this
 * line, the mapfile entry will contain a parenthesized numbering which
 * is one greater than the previous entry's (starting with 2).  HOWEVER,
 * if the additional argument DUPLICATE is true, a particular mapfile
 * entry refers to the same operand (operator, etc) as the previous
 * entry, so the number is duplicated (rather than incremented).
 *
 * Note that DUPLICATE refers to instrumentation for the same operator using the
 * same tag.  Suppose both loop and branch instrumentation are on.  Then a 
 * FOR loop will have two mapfile entries.  However, the first will have
 * the tag "for", and the second will have the tag "loop".  So the
 * duplicate arg should be FIRST for both of these.  That's ugly, since
 * it means the caller needs to know too much about what these routines
 * do.  It will suffice for now.
 *
 */


#include <stdio.h>
#include "config.h"
#include "tree.h"
#include "gct-util.h"
#include "gct-assert.h"
#include "gct-tutil.h"
#include "gct-files.h"
#include "gct-macros.h"
#include "input.h"



#ifdef MAXPATHLEN
#  define PATH_BUF_LEN MAXPATHLEN
#else
#  define PATH_BUF_LEN	1025  /* Because there's no portable system define. */
#endif

#ifdef USG

getwd(buf)
	char *buf;
{
    return (getcwd(buf, PATH_BUF_LEN) != NULL);	/* TEMP */
}

#endif


/*
 * A single data entry is described by this structure.
 *
 * The TAG field is never to be freed.  The other two strings are always
 * allocated within this module and must be freed by it.
 */
struct mapfile_entry
{
  int index;		/* Index (relative to first index for file). */
  char *tag;		/* loop, if, condition, etc.  */
  char *name;		/* Identifying name of probe. */
  int count;		/* Number of (tag,name) pairs on this line. */
  char *rest_text;	/* Some extra text of interest to user. */
};

/*
 * This hashval is calculated on all the tags and names of the probes
 * for a function.  That is sufficient to distinguish different
 * instrumentations of the same function.  Note that the value is
 * independent of the line number, which often changes.
 */
static long Gct_map_hashval = 0;

/*
 * This structure describes all the information needed to buffer up some
 * number of entries and later print them out.  Some of the fields are
 * not obvious:
 *
 * BUFFER, FREE, SIZE:
 *
 * This contains a vector of mapfile entries for a single line.
 * It is "written" (see below) when the line or file change.
 *
 * FILENAME_CHANGE: 
 *
 * If this is true, these entries are from a different file than the last
 * set.  The consequence is that dump_mapfile_buffer must insert a
 * !Internal-File header line in the mapfile.
 *
 * FILENAME:  
 *
 * This is the name of the file as it was given to GCT
 * (main_input_filename) or taken from a #line directive.  This is NOT
 * the file that goes into the mapfile.  It is merely used to check when
 * the filename changes.  Note that we depend on two facts:
 *
 * 1. All nodes created from a file have pointer-equal node->filename fields.
 * 2. This filename data is never freed.
 *
 * This allows us to use pointer-equality to check for filename changes,
 * even if we have moved on to a new function and all the previous
 * function's nodes have been freed.
 *
 * EXPANDED_FILENAME:
 *
 * This is the name that is printed into the mapfile.  It will either be
 * relative to the mapfile's directory or absolute.  It is not
 * necessarily the name given to GCT or the name in a #line directive.
 *
 * Both FILENAME and EXPANDED_FILENAME are valid only after
 * set_mapfile_name is called.  Its argument is the unexpanded name.
 *
 * TEXT, TEXT_NEXT, TEXT_PAST_END:
 *
 * The BUFFER buffers up a single line.  When the line number or
 * filename changes, the buffer is "written out".  The writing out,
 * however, caches the text in these variables.  It is only actually
 * flushed to the stream at the end of the function.  The reason is that
 * we'd like to put the checksums at the head of all the entries, not as
 * a trailer.  It's easier to have this one routine do buffering than to
 * have all mapfile-reading programs do it (or something equivalent)
 * because they have to find trailers.
 */
struct 
{
  int lineno;			/* Linenumber for all these entries. */
  int filename_change;		/* Reason for creation of this buffer. */
  char *filename;		/* Filename as given. */
  char *expanded_filename;	/* Filename relative to master directory */
  struct mapfile_entry *buffer;	/* The buffer. */
  struct mapfile_entry *free;	/* Next available entry */
  int size;			/* Total number of available entries. */
  char *text;			/* Where data entries are temporarily written. */
  char *text_next;		/* Next available byte. */
  char *text_past_end;		/* First UNavailable byte. */
  
} Mapfile_buffer;


FILE *Mapstream;	/* Where we write output. */


static char *barrier;	/* Text_space management routines use this for bounds checking. */

/*
 * To write text: 
 * 1.  Call ensure_text_space with some number larger than what you
 *     intend to write.
 * 2.  sprintf your text into Mapfile_buffer.text_next.
 * 3.  Call advance_text_space() when finished.
 *
 * If you use several sprintfs, advance the text space after each one.
 */


static void
ensure_text_space(amount)
{
  int needed = amount + 2;	/* + 1 for null and + 1 for barrier */
  int available = Mapfile_buffer.text_past_end - Mapfile_buffer.text_next;

  if (available < needed)
    {
      int len = Mapfile_buffer.text_past_end - Mapfile_buffer.text;
      int used = Mapfile_buffer.text_next - Mapfile_buffer.text;
      
      for (len *= 2; len-used < needed; len *= 2)
	;
      Mapfile_buffer.text = (char *)xrealloc(Mapfile_buffer.text, len);
      Mapfile_buffer.text_next = Mapfile_buffer.text + used;
      Mapfile_buffer.text_past_end = Mapfile_buffer.text + len;
    }
  assert(Mapfile_buffer.text_next + amount + 2 <= Mapfile_buffer.text_past_end);
  
  /* Erect barrier to check calculations. */
  barrier = Mapfile_buffer.text_next + amount + 1;
  *barrier = '\177';	/* Expect unchanged. */
}

static void
advance_text_space()
{
  Mapfile_buffer.text_next += strlen(Mapfile_buffer.text_next);
  assert('\177' == *barrier);
}


    


/*
 * This routine initializes the mapfile state for an invocation of gct-cc1.
 * The mapfile state is the contents of variables used in these routines,
 * as well as what's actually appended to the mapfile.  The mapfile must
 * exist; whether it has contents is irrelevant.
 *
 * Note:  the Mapfile_buffer.lineno is initialized to 0.  Thus the
 * first mapfile entry seen will cause a dump (of an empty) buffer
 * and the correct initialization of a new buffer.
 *
 * See file STATE for more information on what's stored in files.
 *
 * Note:  do not be tempted to save space in the mapfile by not printing
 * out the !File header if nothing in the file is instrumented.  That
 * would cause two problems:
 * 1.  The mapfile would not be consistent with gct-ps-defs.c, which
 * contains information for all files.
 * 2.  If a file used to be instrumented, then had instrumentation turned
 * off, the second !File header is the signal to delete the old version.
 */ 

/* Size of buffer for saved text.  Grows if needed. */
#ifdef TESTING
#	define MAP_TEXT_BUFLEN	5	/* Make small so boundaries probed */
#else
#	define MAP_TEXT_BUFLEN	1000
#endif

void
init_mapfile(map_filename)
     char *map_filename;
{
  assert(main_input_filename);
  
  Mapfile_buffer.size = 10;
  Mapfile_buffer.buffer =
    (struct mapfile_entry *) xmalloc(Mapfile_buffer.size * sizeof(struct mapfile_entry));
  Mapfile_buffer.free = Mapfile_buffer.buffer;
  Mapfile_buffer.lineno = 0;

  Mapfile_buffer.text = (char *)xmalloc(MAP_TEXT_BUFLEN);
  Mapfile_buffer.text_next = Mapfile_buffer.text;
  Mapfile_buffer.text_next[0] = '\0';
  Mapfile_buffer.text_past_end = Mapfile_buffer.text + MAP_TEXT_BUFLEN;

  Mapstream = fopen(map_filename, "a");
  if (NULL == Mapstream)
    {
      fatal("Can't open mapfile %s\n", map_filename);
    }

  set_mapfile_name(main_input_filename);
  fprintf(Mapstream, "!File: %s -\n", mapfile_name_to_print());
}

/*
 * Clean up: 
 * 1.  Close the mapstream.
 *
 * Note that the mapfile buffer is empty:  it is always dumped whenever a
 * function ends, and instrumentation happens only within functions.
 */

void
finish_mapfile(num_entries)
     int num_entries;	/* Ignored in this version. */
{
  assert(Mapfile_buffer.free == Mapfile_buffer.buffer);
  fclose(Mapstream);
}


/*
 * This routine is responsible for per-function initialization.
 * Only one thing needs to be done:
 * 1. The instrumentation checksum needs to be initialized.
 *
 * The actual printing of the !Routine header is deferred until we know
 * whether there are any data lines.
 */
void mapfile_function_start()
{
  Gct_map_hashval = 0;
}


/*
 * The variable header is printed.  It contains per-function mapfile
 * state that is gradually discovered during the instrumentation of the
 * function.  Then all the function's data entries are printed.
 *
 * Nothing is printed if there are no data entries.
 */
void mapfile_function_finish()
{
  extern int Gct_function_hashval;

  assert(Mapfile_buffer.free == Mapfile_buffer.buffer);	/* Nothing queued up. */

  /* Any instrumentation for this function? */
  if (Mapfile_buffer.text_next != Mapfile_buffer.text)
    {
      fprintf(Mapstream, "!Routine: %s -\n", 
	      DECL_PRINT_NAME(current_function_decl));
      fprintf(Mapstream, "!Checksum: %ld\n",
	      Gct_function_hashval);
      fprintf(Mapstream, "!Instr-Checksum: %ld\n",
	      Gct_map_hashval);
      fputs(Mapfile_buffer.text, Mapstream);
      if (ferror(Mapstream))
	{
	  fatal("I/O error writing mapfile.");
	}
      
      Mapfile_buffer.text_next = Mapfile_buffer.text;
      Mapfile_buffer.text_next[0] = '\0';
    }
}


/* Get next mapfile entry, growing the buffer if needed. */
static
struct mapfile_entry *
getmessage()
{
  int offset = Mapfile_buffer.free - Mapfile_buffer.buffer;
  if (offset == Mapfile_buffer.size)
    {
      Mapfile_buffer.size *= 2;
      Mapfile_buffer.buffer  =
	(struct mapfile_entry *) xrealloc(Mapfile_buffer.buffer,
					   Mapfile_buffer.size * sizeof(struct mapfile_entry));
      Mapfile_buffer.free = Mapfile_buffer.buffer + offset;
    }
  return(Mapfile_buffer.free ++);
}



/*
 * Dump if this node is on a different line than the previous one or in a
 * different file.  Consider:  it's possible for the filename to change
 * but the linenumber not to.
 */
static
maybe_dump(node)
      gct_node node;
{
  if (!node->filename)
  { /* Panic */
    fprintf(stderr, "Node has no associated filename.\n");
    gct_dump_tree(stderr, node, 0);
    assert(0);
  }

  if (   node->lineno != Mapfile_buffer.lineno
      || node->filename != Mapfile_buffer.filename)
    {
      dump_mapfile_buffer();		/* Flush buffered entries. */
      Mapfile_buffer.lineno = node->lineno;
      set_mapfile_name(node->filename);
    }
}


/*
 * Call this routine to identify the file being instrumented.  This is
 * the file whose name is used in the mapfile, not necessarily the
 * main_input_filename (in the case of #included files).
 *
 * Internal to gct-mapfil.c, this routine is used to maintain the
 * Mapfile_buffer.  Externally, it may be used to make the expanded
 * mapfile name available.  External routines should call it only before
 * any instrumentation or after any instrumentation.
 *
 * In sum, this routine should be called under these circumstances:
 *
 * 1. This routine should be called once at the start of the file, with
 *    main_input_filename.
 * 2. A mapfile DATA entry belongs to a different file or line than the 
 *    previous.  Let this routine decide whether the filename has changed.
 * 3. If mapfile_name_to_print is to be called after instrumentation is
 *    finished, call this routine again with main_input_filename, just to
 *    be sure.
 *
 * NOTES:
 * 1.  We waste memory in certain cases, but never more than one string
 *     per file (main or included).  
 * 2.  An explanation of filename expansion may be found at the head of
 *     gct-control.c.
 * 3.  Handling of Mapfile_buffer.filename_change is complicated by two
 *     special cases:
 *
 *     A. Since the first call is for main_input_filename, we shouldn't
 *        spit out a !Internal-file header.  That would be redundant.
 *     B. Suppose we have
 * 	<instrumentation>
 * 	#include "foo.h"	<no instrumentation>
 * 	<instrumentation here>
 *
 * 	In this case, the filename will have changed (pointers will
 * 	be different), but we don't want to emit !Internal_file
 * 	unless it would actually have an effect on a DATA line.
 */

void
set_mapfile_name(file)
     char *file;
{
  if (file != Mapfile_buffer.filename)	/* File has changed. */
    {
      extern char *gct_current_control_filename();
      extern char *Gct_test_dir;
      char *expanded_filename;	/* Use as working variable. */
      

      /* Avoid redundant printing - but we still need to record
	 change of pointer and do appropriate recalculations. */

      Mapfile_buffer.filename_change = 1;	/* Default */

      /* Note 3A: initial call. */
      if (! Mapfile_buffer.filename)
	{
	  assert(!strcmp(file, main_input_filename)); /* As per case 1 */
	  Mapfile_buffer.filename_change = 0;
	}

      /* Note 3B:  changed back without any instrumentation. */
      if (Mapfile_buffer.filename && !strcmp(Mapfile_buffer.filename, file))
	{
	  Mapfile_buffer.filename_change = 0;
	}

      Mapfile_buffer.filename = file;	/* Record for new pointer test. */

      /*
       * New file to worry about.  This work is redundant in some of the
       * cases where there's no filename_change, but not worth changing.
       */
      if (0 == strcmp(Gct_test_dir, "."))
	{
	  /* No multiple directory funniness. */
	  expanded_filename = file;
	}
      else if (   0 == strcmp(main_input_filename, file)
	       && (expanded_filename = gct_current_control_filename()))
	{
	  /* Dealing with the main file, and it's in the control file. */
	  ;
	}
      else
	{
	  /* Either an include file or a main file not in control file. */
	  static this_dir[PATH_BUF_LEN];

	  if (!getwd(this_dir))
	    {
	      error("Couldn't read current working directory.");
	      error(this_dir);
	      expanded_filename = file;	/* punt */
	    }
	  else
	    {
	      expanded_filename =
		gct_expand_filename(file, this_dir);
	    }
	}
	  
      /* Flush ugly leading "./" */
      while ('.' == expanded_filename[0] && '/' == expanded_filename[1])
	{
	  expanded_filename += 2;
	}
      
      Mapfile_buffer.expanded_filename = expanded_filename;
    }
  else
    {
      Mapfile_buffer.filename_change = 0;
    }
}



/*
 * This returns a string for the filename currently being instrumented.
 * The string is either relative to the mapfile's directory or is
 * absolute.  Note that this filename will change as files are included
 * (that is, it's the file on a #line directive, not just the
 * main_input_filename).
 *
 * OBLIGATIONS: 
 * 1. Do not free the return value.
 * 2. This routine will return an incorrect value unless set_mapfile_name
 *    has been called previously.
 */
char *
mapfile_name_to_print()
{
  assert(Mapfile_buffer.expanded_filename);
  return Mapfile_buffer.expanded_filename;
}



/*
 * Dump the contents of the mapfile buffer and prepare for the next set
 * of related mapfile entries.
 *
 * Recall that the buffer is dumped whenever the line number or filename
 * changes.  This routine only handles dumping the buffer; it doesn't deal
 * with the lineno or filename fields.
 */

void
dump_mapfile_buffer()
{
  if (Mapfile_buffer.buffer  != Mapfile_buffer.free)
    {
      struct mapfile_entry *rover;

      if (Mapfile_buffer.filename_change)
	{
	  ensure_text_space(100+strlen(Mapfile_buffer.expanded_filename));
	  sprintf(Mapfile_buffer.text_next, "!Internal-File: %s -\n",
		  Mapfile_buffer.expanded_filename);
	  advance_text_space();
	}
  
      for (rover = Mapfile_buffer.buffer; rover != Mapfile_buffer.free; rover++)
	{
	  char *c;	/* Rover through string */
	  int required_space = 100;	/* room for constant text. */
	  
	  /*
	   * Update the hash value.  Note that strings with embedded
	   * nulls will have the trailing portion ignored.  Of no concern.
	   */
	  assert(rover->tag);
	  for (c = rover->tag; *c; c++)
	    {
	      GCT_HASH(Gct_map_hashval, (int) *c);
	      required_space++;
	    }

	  /*
	   * Peculiar test is so that multicondition coverage stops after
	   * the condition number.  Multicondition coverage is the only
	   * type of coverage with embedded spaces in the name.
	   */
	  if (rover->name)
	    for (c = rover->name; *c && (' ' != *c); c++)
	      {
		GCT_HASH(Gct_map_hashval, (int) *c);
		required_space++;
	      }

	  if (rover->rest_text) required_space += strlen(rover->rest_text);

	  ensure_text_space(required_space);	/* Enough for whole line */
	  sprintf(Mapfile_buffer.text_next, "- %d %s ",
		  Mapfile_buffer.lineno,
		  rover->tag);
	  advance_text_space();
	  
	  if (rover->name)
	    {
	      sprintf(Mapfile_buffer.text_next, "%s ", rover->name);
	      advance_text_space();
	      free(rover->name);
	    }
	  if (rover->count > 1)
	    {
	      sprintf(Mapfile_buffer.text_next, "(%d) ", rover->count);
	      advance_text_space();
	    }
	  if (rover->rest_text)
	    {
	      sprintf(Mapfile_buffer.text_next, "%s ", rover->rest_text);
	      advance_text_space();
	      free(rover->rest_text);
	    }
	  sprintf(Mapfile_buffer.text_next, "\n");
	  advance_text_space();
	}
      Mapfile_buffer.free = Mapfile_buffer.buffer;
    }
}


void
assert_empty_mapfile_buffer()
{
  assert(Mapfile_buffer.buffer == Mapfile_buffer.free);
}






/* Use instead of strcmp when a string can be null. */
#define match_strings(s1, s2) ((s1) == (s2) || ((s1) && (s2) && !strcmp((s1), (s2))))

/*
 * Assign a count for the (tag,name) pair.  If there's not a matching
 * pair on the line, the count is 1.  If there is, and this is a
 * DUPLICATE, return the matching pair's count.  If this is not a
 * duplicate, return 1+ the matching pair's count.
 *
 * Do loops are internally tagged with "do-loop", and ordinary loops are
 * tagged with "loop".  Externally, both are tagged with "loop", which is
 * more natural for the user.  In order to get the counts right, we must
 * make sure that "do-loop" matches "loop".
 */
static int
assign_count(tag, name, duplicate)
     char *tag, *name;
     int duplicate;
{
  struct mapfile_entry *rover;
  char *tag2 = (char *)0;

  assert(tag);
  
  if (match_strings(tag, "loop"))
    tag2 = "do-loop";
  if (match_strings(tag, "do-loop"))
    tag2 = "loop";

  /* Be sure to skip self. */
  for (rover = Mapfile_buffer.free-2; rover>= Mapfile_buffer.buffer; rover--)
    {
      if (   (   !strcmp(tag, rover->tag)
	      || match_strings(tag2, rover->tag))
	  && match_strings(name, rover->name))
	{
	  if (duplicate)
	    return rover->count;
	  else
	    return 1+(rover->count);
	}
    }
  return 1;
}


/*
 * Return number of elements on line with matching tag.  This is gross.
 */
static int
count_of_matching_tag(tag)
     char *tag;
{
  struct mapfile_entry *rover;
  int result = 0;

  /* Be sure to skip self. */
  for (rover = Mapfile_buffer.free-2; rover>= Mapfile_buffer.buffer; rover--)
    {
      if (!strcmp(tag, rover->tag))
	++result;
    }
  return result;
}




/*
 * Note: we know that the tag field will outlive the mapfile entry, so we
 * don't need to make a copy.
 */
void
branch_map(index, node, duplicate)
     int index;
     gct_node node;
     int duplicate;
{
  struct mapfile_entry *message;
  maybe_dump(node);

  message = getmessage();
  message->index = index;
  assert(node->text);
  message->tag = node->text;
  message->name = (char *)0;
  message->count = assign_count(message->tag, message->name, duplicate);
  message->rest_text = (char *)0;
}

void
multi_map(index, node, name, duplicate)
     int index;
     gct_node node;
     char *name;
     int duplicate;	/* ignored */
{
  struct mapfile_entry *message;
  maybe_dump(node);

  message = getmessage();
  message->index = index;
  message->tag = "condition";
  message->name = (char *) 0;
  assert(name);
  message->name = (char *) xmalloc(20+strlen(name));
  sprintf(message->name, "%d (%s)",
	  1+count_of_matching_tag(message->tag), name);
  message->count = 1;		/* Conditions are always unique. */
  message->rest_text = (char *)0;
}

void
loop_map(index, node, duplicate)
     int index;
     gct_node node;
     int duplicate;
{
  struct mapfile_entry *message;
  maybe_dump(node);

  message = getmessage();
  message->index = index;
  if (GCT_DO == node->type)
    message->tag = "do-loop";	/* Interpretation is different */
  else
    message->tag = "loop";

  message->name = (char *)0;
  message->count = assign_count(message->tag, message->name, duplicate);
  message->rest_text = (char *)0;
}

void 
operator_map(index, node, rest_text, duplicate)
     int index;
     gct_node node;
     char *rest_text;
     int duplicate;
{
  struct mapfile_entry *message;
  maybe_dump(node);

  message = getmessage();
  message->index = index;
  message->tag = "operator";
  assert(node->text);
  message->name = permanent_string(node->text);
  message->count = assign_count(message->tag, message->name, duplicate);
  assert(rest_text);
  message->rest_text = permanent_string(rest_text);

}

void
operand_map(index, node, name, rest_text, duplicate)
     int index;
     gct_node node;
     char *name;
     char *rest_text;
     int duplicate;
{
  struct mapfile_entry *message;
  maybe_dump(node);

  message = getmessage();
  message->index = index;
  message->tag = "operand";
  assert(name);
  message->name = permanent_string(name);
  message->count = assign_count(message->tag, message->name, duplicate);
  assert(rest_text);
  message->rest_text = permanent_string(rest_text);
}

void
routine_map(index, node, name, rest_text, duplicate)
     int index;
     gct_node node;
     char *name;
     char *rest_text;
     int duplicate;
{
  struct mapfile_entry *message;
  maybe_dump(node);

  message = getmessage();
  message->index = index;
  message->tag = "routine";
  assert(name);
  message->name = permanent_string(name);
  message->count = assign_count(message->tag, message->name, duplicate);
  assert(rest_text);
  message->rest_text = permanent_string(rest_text);
}

void
race_map(index, node, name, rest_text, duplicate)
     int index;
     gct_node node;
     char *name;
     char *rest_text;
     int duplicate;
{
  struct mapfile_entry *message;
  maybe_dump(node);

  message = getmessage();
  message->index = index;
  message->tag = "race in";
  assert(name);
  message->name = permanent_string(name);
  message->count = assign_count(message->tag, message->name, duplicate);
  assert(rest_text);
  message->rest_text = permanent_string(rest_text);
}

void
call_map(index, node, name, containing_routine, duplicate)
     int index;
     gct_node node;
     char *name;
     char *containing_routine;
     int duplicate;
{
  struct mapfile_entry *message;
  char *rest_text;
# define FORMAT "(in %s) never made."
  
  maybe_dump(node);

  message = getmessage();
  message->index = index;
  message->tag = "call of";
  assert(name);
  message->name = permanent_string(name);
  message->count = assign_count(message->tag, message->name, duplicate);
  rest_text = (char *) xmalloc(strlen(containing_routine) + sizeof(FORMAT));
  sprintf(rest_text, FORMAT, containing_routine);
  message->rest_text = rest_text;
# undef FORMAT
}

/*
 * This is used to "fill in the blanks" for instrumentation types that
 * use up a single message but several entries in the test flag table:
 * loops, branches, multi-conditionals.  The extra entries in the mapfile
 * make it easy for test programs to combine the map and log files.
 *
 * The caller of other mapping routines is responsible for also calling
 * this.  It might make sense to have other mapping routines create the
 * placeholders, but then they'd have to be responsible for incrementing
 * Gct_next_index.  
 *
 * At some future point, this may also be called to insert dummy messages
 * in the logfile to make the numbering of, e.g., variables more obvious.
 *
 */
void
map_placeholder(index)
     int index;
{
  struct mapfile_entry *message;

  message = getmessage();
  message->index = index;
  message->tag = "&";
  message->name = (char *)0;
  message->count = 1;
  message->rest_text = (char *)0;
}
     

/* Miscellaneous utilities */

/*
 * This routine makes a name, suitable for the mapfile, out of the given
 * node.  The caller must free the space later.  
 */
char *
make_mapname(node)
     gct_node node;
{
  char *myname;			/* The return value */
  gct_node child;		/* A child whose type is of interest */

  assert(Gct_nameable[(int)(node->type)]);
  
  switch(node->type)
    {
    case GCT_ADDR:
      child = GCT_ADDR_ARG(node);
      if (GCT_IDENTIFIER == child->type || GCT_CONSTANT == child->type)
	{
	  myname = (char *)xmalloc(2+strlen(child->text));
	  sprintf(myname, "&%s", child->text);
	}
      else
	{
	  myname = permanent_string("&<...>");
	}
      
      break;
      
    case GCT_DEREFERENCE:
      child = GCT_DEREFERENCE_ARG(node);
      if (GCT_IDENTIFIER == child->type || GCT_CONSTANT == child->type)
	{
	  myname = (char *) xmalloc(2 + strlen(child->text));
	  sprintf(myname, "*%s", child->text);
	}
      else
	{
	  myname = permanent_string("*<...>");
	}
      break;
    case GCT_IDENTIFIER:
    case GCT_CONSTANT:
      myname = permanent_string(node->text);
      break;

    case GCT_FUNCALL:
      child = GCT_FUNCALL_FUNCTION(node);
      if (GCT_IDENTIFIER == child->type || GCT_CONSTANT == child->type)
	{
	  myname = (char *)xmalloc(6+strlen(child->text));
	  sprintf(myname, "%s(...)", child->text);
	}
      else
	{
	  myname = permanent_string("<...>(...)");
	}	  
      break;
      
    case GCT_ARRAYREF:
      child = GCT_ARRAY_ARRAY(node);
      if (GCT_IDENTIFIER == child->type || GCT_CONSTANT == child->type)
	{
	  myname = (char *) xmalloc(6 + strlen(child->text));
	  sprintf(myname, "%s[...]", child->text);
	}
      else
	{
	  myname = permanent_string("<...>[...]");
	}
      break;
    case GCT_DOTREF:
      child = GCT_DOTREF_VAR(node);
      if (GCT_IDENTIFIER == child->type || GCT_CONSTANT == child->type)
	{
	  myname = (char *) xmalloc(2 + strlen(child->text) + strlen(GCT_DOTREF_FIELD(node)->text));
	  sprintf(myname, "%s.%s", child->text, GCT_DOTREF_FIELD(node)->text);
	}
      else
	{
	  myname = (char *) xmalloc(7 + strlen(GCT_DOTREF_FIELD(node)->text));
	  sprintf(myname, "<...>.%s", GCT_DOTREF_FIELD(node)->text);
	}
      break;
    case GCT_ARROWREF:
      child = GCT_ARROWREF_VAR(node);
      if (GCT_IDENTIFIER == child->type || GCT_CONSTANT == child->type)
	{
	  myname = (char *) xmalloc(3 + strlen(child->text) + strlen(GCT_ARROWREF_FIELD(node)->text));
	  sprintf(myname, "%s->%s", child->text, GCT_ARROWREF_FIELD(node)->text);
	}
      else
	{
	  myname = (char *) xmalloc(8 + strlen(GCT_ARROWREF_FIELD(node)->text));
	  sprintf(myname, "<...>->%s", GCT_ARROWREF_FIELD(node)->text);
	}
      break;
    case GCT_SIZEOF:
    case GCT_ALIGNOF:
      child = GCT_OP_ONLY(node);
      if (GCT_IDENTIFIER == child->type || GCT_CONSTANT == child->type)
	{
	  myname = (char *) xmalloc(3 + strlen(child->text) + strlen(node->text));
	  sprintf(myname, "%s(%s)", node->text, child->text);
	}
      else
	{
	  myname = (char *) xmalloc(6 + strlen(node->text));
	  sprintf(myname, "%s(...)", node->text);
	}
      break;
    case GCT_CAST:
      child = GCT_CAST_EXPR(node);
      if (GCT_IDENTIFIER == child->type || GCT_CONSTANT == child->type)
	{
	  myname = (char *) xmalloc(7 + strlen(child->text));
	  sprintf(myname, "<cast>%s", child->text);
	}
      else
	{
	  myname = (char *) xmalloc(10);
	  sprintf(myname, "<cast>...");
	}
      break;
    case GCT_COMPOUND_EXPR:
      myname = (char *) xmalloc(10);
      sprintf(myname, "({...})");
      break;
      
	  
    default:
      error("make_mapname called with wrong type.");
      abort();
    }
  return myname;
}


/*
 * This returns a string of the form "<name>, <int-tag>", where <name> is
 * a name like that returned by make_mapname for the leftmost reference in
 * the tree.  <int-tag> is whatever was passed in.  Caller is responsible
 * for freeing storage.
 */

char *make_leftmost_name(node_tree, tag)
     gct_node node_tree;
     int tag;
{
  char *name;
  char *retval;
  
  for(; !Gct_nameable[(int)(node_tree->type)]; node_tree=node_tree->children)
    {
    }
  if (gct_in_macro_p(node_tree->first_char))
    name = permanent_string(gct_macro_name());
  else
    name = make_mapname(node_tree);

  retval = (char *)xmalloc(20+strlen(name));
  sprintf(retval, "%s, %d", name, tag);
  free(name);
  return(retval);
}
