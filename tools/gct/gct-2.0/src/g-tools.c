/* Copyright (C) 1992 by Brian Marick.  All rights reserved. */
/*
 * When aggregated with works covered by the GNU General Public License,
 * that license applies to this file.  Brian Marick retains the right to
 * apply other terms when this file is distributed as a part of other
 * works.  See the GNU General Public License (version 2) for more
 * explanation.
 */

/* Code shared among the utilities. */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
extern int errno;	/* On some systems, it's not defined in errno.h. */
#include "gct-const.h"
#include "gct-files.h"
#include "config.h"
#include "g-tools.h"
#include "gct-assert.h"


		/* MAPFILE TRAVERSALS - Definitions */


/* This structure contains values that are set once and then never
   modified.  These values control the way the mapfile and logfile
   are read. */

static struct 
{
  int need_mapline_file_position;    /* Keep track of position for editing. */
  int using_logfile;		     /* Read data from logfile as well. */
} Control = 
{
  0,
  0
};


gct_entry Entry = 
{
  (char *) 0, 	/* Main Filename */
  (char *) 0, 	/* Inner Filename */
  (char *) 0, 	/* Routinename */
  -1, /* Means that the first expected entry is 0. */
  UNKNOWN,
};




		  /* ORDINARY FILE MANIPULATIONS */

/* The various filenames used, both in their original and expanded forms. */
char *Gct_test_dir = ".";

char *Gct_test_map = GCT_MAP;
char *Gct_full_map_file_name;
FILE *Gct_map_stream;

/* Gct_input is usually a log file, but it can be any stream. */
char *Gct_input = (char *)0;
FILE *Gct_input_stream;

/*
 * I/O errors are handled in a standard way. 
 * Corrupt files are not well diagnosed, since a particular index can be
 * hard to find.
 */

file_error(file, type)
     char *file;
     char *type;
{
  fprintf(stderr, "%s is %s at approximate location file=%s, index=%d.\n",
	  file, type,
	  Entry.main_filename ? Entry.main_filename : "unknown",
	  Entry.index);
  fprintf(stderr, "Mapfile line looks something like this:\n");
  fprintf(stderr, "%s\n", Entry.mapline);
  exit(1);
}

#define mapfile_truncation_if(test) \
  if (test) file_error("Mapfile", "truncated");
#define mapfile_corruption_if(test) \
  if (test) file_error("Mapfile", "corrupt");
#define logfile_truncation_if(test) \
  if (test) file_error("Logfile", "truncated");
#define logfile_corruption_if(test) \
  if (test) file_error("Logfile", "corrupt");

#define feither(stream) (feof(stream) || ferror(stream))



			 /* EXTERNAL EDITS */
/*
 * External edits are edits set on routines and files from outside the
 * mapfile, usually via command-line arguments.  (See the gsummary and
 * greport -vf option, for example.)  They take precedence
 * over whatever's in the mapfile, according to the following table.
 *
 * NOTES:
 *
 * 1.  External visibility takes precedence over external ignoring.
 * 2.  Generally, making a file visible erases the effect of !File edits.
 *     Making a routine visible erases the effect of !File edits.
 *     Externally ignoring a file supersedes all mapfile edits.
 * 3.  It is inconsistent to apply internal file edits when 
 *     visible-routine is set, but it turns out to be convenient
 *     for grammars.
 * 4.  The default is Don't Care for both edits.
 * 5.  The behavior of a SUPPRESSED external edit is undefined.
 *
 * Unfortunately, this doesn't fit into the "subsidiarity" framework.
 * It is implemented as special case code in cumulative_edit.
 */

/* 

External Routine Edit \ External File Edit				   

		Ignore		Visible		Don't Care

Ignore		always		internal-file,	always 
		ignore		routine, and	ignore
		everything	line edits	everything
				apply


Visible		internal-file	internal file	internal file
		and line edits	and line edits	and line edits
		apply		apply		apply	


Don't Care	always		internal file,	all mapfile edits
		ignore		routine, and 	apply
		everything	line edits 
				apply

*/




/*
 * Program-specific code
 *
 * There's some monkey business here with #ifndef GCT_KIT1.
 * This routine is used by the freeware GCT and the for-sale Expansion
 * Kit 1.  In GCT, we want to use regular-expression matching.  In KIT1,
 * we can't, because that code is copylefted.  (And it's useless for
 * KIT1, anyway.)  So we don't compile in the regular expression code
 * when this code is used within KIT1.
 */

/*
 * This structure holds the external edit values currently in use.  These
 * change as routines and files are entered and left.  They are
 * initialized to the value always used in KIT1.  In GCT, the value is
 * always explicitly set before use.
 */

static
struct
{
  t_edit routine_edit;
  t_edit file_edit;
} Current_external_edits = 
{
  DONT_CARE_COUNT, DONT_CARE_COUNT
};


#if !defined(GCT_KIT1)
#include "regex.h"

/*
 * This structure describes an element of a list of external edits.  
 * Here are the fields:
 *
 * NEXT		Usual forward pointer.
 * EDIT		The edit value.
 * NAME		The name.  Setting this value establishes another
 * 		pointer to some already-allocated storage.  The caller
 * 		should not free that storage.  This routine does not
 * 		free that storage.
 */
struct s_one_external_edit
{
  struct s_one_external_edit *next;
  char *name;
  struct re_pattern_buffer *matchbuf;
  t_edit edit;
};
typedef struct s_one_external_edit one_external_edit;

/*
 * This structure holds:
 *
 * the list of external edits for routines and files.
 *
 * the default values, which are what's used if no matching filename or
 * routine is used.  The, um, default default value is DONT_CARE_EDIT,
 * which allows the mapfile value to take precedence.
 */


static
struct
{
  t_edit default_routine_edit;
  t_edit default_file_edit;
  one_external_edit *routines;
  one_external_edit *files;
} External_edits = 
{
  DONT_CARE_COUNT, DONT_CARE_COUNT
};


void
set_default_routine_external_edit(edit)
     t_edit edit;
{
  External_edits.default_routine_edit = edit;
}

void
set_default_file_external_edit(edit)
     t_edit edit;
{
  External_edits.default_file_edit = edit;
}

/*
 * The following routines add the given NAME, with its associated EDIT,
 * to the External_edits structure.  When the mapfile is read, these
 * edits will be used for matching entries, instead of whatever's in the
 * mapfile.
 */
static one_external_edit *
add_edit_common(name, edit)
     char *name;
     t_edit edit;
{
  one_external_edit *new =
    (one_external_edit *) xmalloc(sizeof(one_external_edit));
  int namelength = strlen(name) + 1;	/* Including trailing $ we add. */
  
  new->name = (char *) xmalloc(namelength+1);	/* Trailing null. */
  strcpy(new->name, name);
  new->name[namelength-1] = '$';
  new->name[namelength] = '\0';
  new->edit = edit;

  new->matchbuf = (struct re_pattern_buffer *) xmalloc(sizeof(struct re_pattern_buffer));
  new->matchbuf->allocated = 100;
  new->matchbuf->buffer = (char *)xmalloc(new->matchbuf->allocated);
  new->matchbuf->fastmap = (char *)xmalloc(0400);
  new->matchbuf->translate = (char *)0;
  re_compile_pattern(new->name, namelength, new->matchbuf);

  return new;
}

/* Silently trim off leading "./", just like the mapfile does.  This
   is useful when the names are generated by find(1). */
void
add_file_external_edit(name, edit)
     char *name;
     t_edit edit;
{
  one_external_edit *new;

  while ('.' == name[0] && '/' == name[1])
    name +=2;
 
  new = add_edit_common(name, edit);
  new->next = External_edits.files;
  External_edits.files = new;
}

void
add_routine_external_edit(name, edit)
     char *name;
     t_edit edit;
{
  one_external_edit *new = add_edit_common(name, edit);
  new->next = External_edits.routines;
  External_edits.routines = new;
}

#endif	/* End what's not used in KIT1. */


/*
 * These two routines set External_edits to the edit values previously
 * associated with NAME.  In KIT1, these routines are still used, but
 * they are no-ops.  This is done to minimize the amount of #ifdefing.
 *
 * If no matching routine or file is found, the default value is used.
 */

static void
routine_external_edit(name)
     char *name;
{
#if !defined(GCT_KIT1)
  one_external_edit *rover = External_edits.routines;
  int len = strlen(name);

  for(;rover; rover = rover->next)
    {
      if (0 <= re_match(rover->matchbuf, name, len, 0, 0))
	{
	  Current_external_edits.routine_edit = rover->edit;
	  return;
	}
    }
  Current_external_edits.routine_edit = External_edits.default_routine_edit;
#endif
}

static void
file_external_edit(name)
     char *name;
{
#if !defined(GCT_KIT1)
  one_external_edit *rover = External_edits.files;
  int len = strlen(name);

  for(;rover; rover = rover->next)
    {
      if (0 <= re_match(rover->matchbuf, name, len, 0, 0))
	{
	  Current_external_edits.file_edit = rover->edit;
	  return;
	}
    }
  Current_external_edits.file_edit = External_edits.default_file_edit;
#endif
}


     

		/* ENTRY STRUCTURE MEMBER FUNCTIONS */

/*
 * Get the line where it can be manipulated.  This sets these fields in
 * Entry:
 * 	TYPE	(UNKNOWN or NONE if end-of-file was found)
 * 	MAPLINE
 * 	MAPLINE_NEXT (identical to MAPLINE).
 * 	MAPFILE_FILE_POSITION (if asked for by control file)
 *
 * The EOF argument causes an error exit if end-of-file was found.
 *
 * Note that Entry.index is not set here: it's the caller's
 * responsibility to determine (most of) the type of the entry.  INDEX
 * only applies to data lines.
 */
void
raw_mapfile_entry(eof)
     enum on_eof eof;
{
  Entry.type = UNKNOWN;
  if (Control.need_mapline_file_position)
    {
      Entry.mapline_file_position = ftell(Gct_map_stream);
    }
  
  fgets(Entry.mapline, FILE_BUFFER_SIZE, Gct_map_stream);
  Entry.mapline_next = Entry.mapline;
  mapfile_corruption_if(ferror(Gct_map_stream));
  if (feof(Gct_map_stream))
    {
      Entry.type = NONE;
      mapfile_truncation_if((ERROR_ON_EOF == eof));
    }
}

/*
 * Analogous to raw_mapfile_entry.  There is no such thing as EOF for
 * logfiles.  Once the actual entries run out, an unending stream of 0s
 * is returned.
 *
 * Sets LOGLINE, LOGLINE_NEXT (identical to LOGLINE).
 * Does not set INDEX:  this value is derived from the mapfile.
 */	
void
raw_logfile_entry()
{
  assert(Control.using_logfile);
  
  fgets(Entry.logline, FILE_BUFFER_SIZE, Gct_input_stream);
  if (feof(Gct_input_stream))
    {
      /* Trailing null for compatibility with fgets. */
      strcpy(Entry.logline, "0\n");
    }
  logfile_corruption_if(ferror(Gct_input_stream));
  Entry.logline_next = Entry.logline;
}
	
/*
 * PARSING THE LINES
 *
 * INVARIANT:  All parsing routines tokenize by inserting a null after a 
 * token.  (Tokens always are separated by white space.)  Backing up and
 * rescanning the line does not work.  Until the next line is read,
 * callers may assume strings are constant.
 */

/*
 * Set Entry.type
 *
 * Preconditions:
 * 1.  Mapline was initialized with raw_mapfile_entry.
 * 2.  Type is either NONE (if EOF found by raw_mapfile_entry) or UNKNOWN.
 * 3.  If type is UNKNOWN, line begins with either '!' or '0'.
 * 	On failure:  mapfile_corruption.
 * Postconditions:
 * 1.  If type is already known to be NONE, no change.
 * 2.  If the line begins with '!', type is HEADER.
 * 3.  If the line begins with '0', type is DATA.
 */
void
mapfile_type()
{
  if (UNKNOWN == Entry.type)
    {
      if (   '-' == Entry.mapline[0]
	  || 'S' == Entry.mapline[0]
	  || 'V' == Entry.mapline[0]
	  || 'I' == Entry.mapline[0])
	Entry.type = DATA;
      else
	{
	  mapfile_corruption_if('!' != Entry.mapline[0]);
	  Entry.type = HEADER;
	}
    }
}


/*
 * Skip white space in one of the lines to position at following token.
 * FIELD is the field to search (mapline_next or logline_next).
 * There need not be any white space, in which case the FIELD remains
 * unchanged.
 * Note that "the following token" may be a null string.  
 * If this is an error, the caller must notice it and handle it.
 * (In most cases, the best way to do that is to rely on findwhite
 * to discover an error when it tries to find the white space past the
 * token).
 */

#define skipwhite(field)		\
  { 								\
    for(; isspace(*Entry.field) ; Entry.field++)		\
      {								\
      }								\
  }


/*
 * As above, but skip non-white space instead of white space.  Because we
 * use fgets to build the mapline and logline, every line should end in a
 * newline (white space).  If the file is not newline-terminated
 * that trailing space might not be there.  But then the file is corrupt.
 * The file is also corrupt if we begin positioned at the end of the line
 * (e.g., a token is missing in the line).
 * Corruption is reported.
 */

#define findwhite(field, string)			\
  for(; !isspace(*Entry.field); Entry.field++)			\
    {								\
      if ('\0' == *Entry.field)					\
	{							\
	  file_error(string, "corrupt");			\
	}							\
    }



/*
 * Find a header in the current mapline.  On entry, MAPLINE_NEXT's value
 * is irrelevant.
 * Preconditions:
 * 1. Validated:  Line contains a colon.  On failure: corruption.
 *
 * Postconditions:
 * If the given header matches this line
 * 	MAPLINE_NEXT is now positioned at the next non-whitespace
 * 	character after the header and true is returned.
 * 	Header is tokenized (see above - null added).
 * else
 * 	MAPLINE_NEXT has unchanged positioning and false is returned.
 */
int 
mapfile_header_match(header)
     char *header;
{
  char *colon;
  for (colon = Entry.mapline+1; *colon != ':'; colon++)
    {
      mapfile_corruption_if('\0' == *colon);
    }
  *colon = '\0';
  if (0 != strcmp(Entry.mapline+1, header))
    {
      *colon = ':';
      return 0;
    }
  Entry.mapline_next = colon+1;
  skipwhite(mapline_next);
  return 1;
}

/*
 * Given one of the lines, identified by WHERE, save the current position
 * in START, then advance past this token and any trailing whitespace.
 * The character after the token is changed into a null (it may have been
 * one already, of course) so that START now points to a distinct token
 * string.  FILESTRING is a string to use to indicate which file is
 * corrupt, if it should turn out to be.
 */
#define SKIP_TOKEN(where, start, filestring)	\
   (start) = Entry.where;		\
   findwhite(where, filestring);	\
   *Entry.where = '\0';			\
   Entry.where++;			\
   skipwhite(where);

  
/*
 * The following routines all perform essentially the same actions.
 * I duplicate routines, instead of playing games to share code between
 * loglines and maplines, because the routines are small.
 *
 * All these routines "use up" part of a line.  If they consume a token,
 * the next call to a routine will get the next token.
 */

/* Return a token from logfile.  Error if token is not found. */
char *
logfile_token()
{
  char *retval;
   
  SKIP_TOKEN(logline_next, retval, "Logfile");
  return retval;
}

/* Return a token from mapfile.  Error if token is not found. */
char *
mapfile_token()
{
  char *retval;
   
  SKIP_TOKEN(mapline_next, retval, "Mapfile");
  return retval;
}

/*
 * Return integer value of token.  Error if token not found or not
 * integer.  Note that the return value is an unsigned long.  Logfile
 * entries can easily overflow 32 bits in some applications.  That's
 * *assumed* not to be true of any other integral values in GCT.
 */
unsigned long
logfile_token_as_unsigned_long()
{
  char *string;
  unsigned long value;

  SKIP_TOKEN(logline_next, string, "Logfile");
  logfile_corruption_if(1 != sscanf(string, "%lu", &value));
  return value;
}

/* Return integer value of token.  Error if token not found or not integer. */
int
mapfile_token_as_integer()
{
  char *string;
  int value;

  SKIP_TOKEN(mapline_next, string, "Mapfile");
  mapfile_corruption_if(1 != sscanf(string, "%d", &value));
  return value;
}

/* Return the rest of the logfile, without the trailing newline.
   It is a logfile-corruption for the trailing newline to be missing. */
char *
logfile_rest()
{
  int len = strlen(Entry.logline_next);
  logfile_corruption_if(Entry.logline_next[len-1] != '\n');
  Entry.logline_next[len-1] = '\0';
  return Entry.logline_next;
}

/*
 * Mapfile_rest is much like logfile_rest, except:
 * It is NOT an error for there to be no rest-text, though the reason
 * this works is obscure:
 * 1. There must have been a previous get-token.
 * 2. That get-token will have sucked up everything, leaving mapline_next
 * pointing at the trailing null.  (Since newline is white space.)
 * 3. len-1 will then refer to mapline_next[-1], which is the trailing
 * newline (or something else, if there was a corrupt mapfile).
 * 3. That newline is pointlesslessly changed to a null.
 * 4. The null string is returned.
 */

char *
mapfile_rest()
{
  int len = strlen(Entry.mapline_next);
  mapfile_corruption_if(Entry.mapline_next[len-1] != '\n');
  Entry.mapline_next[len-1] = '\0';
  return Entry.mapline_next;
}

/*
 * This tells what file position corresponds to the current mapline
 * position.  Seeking to this position will allow you to edit the token that
 * mapfile_token would return.
 */
long
mapfile_ftell()
{
  assert(Control.need_mapline_file_position);
  return Entry.mapline_file_position + Entry.mapline_next - Entry.mapline;
}

/*
 * Move to a particular position in the mapfile, not necessarily the
 * position given by mapfile_ftell().
 */
void
mapfile_moveto(position)
     long position;
{
  if (-1 == fseek(Gct_map_stream, position, 0))
    {
      fprintf(stderr, "Couldn't seek in %s.\n", Gct_full_map_file_name);
      perror("error: ");
      exit(1);
    }
}

char *
mapfile_timestamp()
{
  return Entry.map_timestamp;
}


/*
 * Read the edit tag as a token and return the appropriate t_edit value.
 * It is an error for the tag to be anything other than '-', 'I', 'V', or 'S'.
 */
t_edit
mapfile_raw_edit()
{
  char *str = mapfile_token();
  mapfile_corruption_if('\0' != str[1]);
  switch(str[0])
    {
    case 'S':
      return SUPPRESSED_COUNT;
    case '-':
      return DONT_CARE_COUNT;
    case 'I':
      return IGNORED_COUNT;
    case 'V':
      return VISIBLE_COUNT;
    default:
      mapfile_corruption_if(1);
    }
}

/*
 * Given a line edit, merge in the filename, internal-filename, and
 * routine edits to form a cumulative edit.  Subsidiarity applies
 * (see definition of t_edit).
 *
 * External edits are treated specially; see the definition of external
 * edits above.  (External edits are set by the -visible-file option, etc.)
 */
static t_edit
cumulative_edit(line_edit)
     t_edit line_edit;
{
  t_edit retval;

  if (VISIBLE_COUNT == Current_external_edits.routine_edit)
    {
      retval = COMBINE_EDIT_LEVELS(line_edit, Entry.internal_filename_edit);
      return retval;
    }

  if (VISIBLE_COUNT == Current_external_edits.file_edit)
    {
      retval = COMBINE_EDIT_LEVELS(Entry.routine_edit, Entry.internal_filename_edit);
      retval = COMBINE_EDIT_LEVELS(line_edit, retval);
      return retval;
    }

  if (   IGNORED_COUNT == Current_external_edits.routine_edit
      || IGNORED_COUNT == Current_external_edits.file_edit)
    {
      return IGNORED_COUNT;
    }

  assert(DONT_CARE_COUNT == Current_external_edits.routine_edit);
  assert(DONT_CARE_COUNT == Current_external_edits.file_edit);
  
  retval = COMBINE_EDIT_LEVELS(Entry.internal_filename_edit, Entry.filename_edit);
  retval = COMBINE_EDIT_LEVELS(Entry.routine_edit, retval);
  retval = COMBINE_EDIT_LEVELS(line_edit, retval);
  return retval;
}

  

/* I/O Utilities. */


/*
 * Initialize the mapfile stream.  The contents of
 * the mapfile are read up through the timestamp, which is stored.  
 *
 * OPEN_HOW should be either "r" or "r+", which is passed to fopen.
 * NEED_MAPLINE_FILE_POSITION instructs the mapfile code to keep track of
 * the ftell() position of the beginning of line (for mapfile_ftell).
 * Programs that reposition around in files should set this.
 */
void
init_mapstream(open_how, need_mapline_file_position)
     char *open_how;
     int need_mapline_file_position;
{
  Control.need_mapline_file_position = need_mapline_file_position;
  
  Gct_full_map_file_name = gct_expand_filename(Gct_test_map, Gct_test_dir);
  Gct_map_stream = fopen(Gct_full_map_file_name, open_how);
  if (NULL == Gct_map_stream)
    {
      fprintf(stderr,"Can't open mapfile %s\n",Gct_full_map_file_name);
      exit(1);
    }
  
  /* Check the version. */
  raw_mapfile_entry(ERROR_ON_EOF);
  if (strcmp(GCT_MAPFILE_VERSION, mapfile_rest()))
    {
      fprintf(stderr, "This program only works on mapfiles matching this header:\n%s\n",
	      GCT_MAPFILE_VERSION);
      exit(1);
    }
	
  /* Store timestamp. */
  raw_mapfile_entry(ERROR_ON_EOF);	/* Timestamp line. */
  mapfile_corruption_if(!mapfile_header_match("Timestamp"));
  strcpy(Entry.map_timestamp, mapfile_rest());
}

/*
 * Initialize the stream named by Gct_input.  If INPUT_IS_LOGFILE, the
 * timestamp is read (corruption if it's not available).
 */
void
init_other_stream(input_is_logfile)
     int input_is_logfile;
{
  Control.using_logfile = input_is_logfile;
  
  if (Gct_input == (char *)0)
    {
      Gct_input_stream = stdin;
      Gct_input = "<standard input>";
    }
  else
    {
      Gct_input_stream = fopen(Gct_input,"r");
    }

  if (NULL == Gct_input_stream)
    {
     fprintf(stderr,"Can't open %s\n",Gct_input);
     exit(1);
    }

  if (Control.using_logfile)
    {
      raw_logfile_entry();	/* Identifier line; format varies */
      raw_logfile_entry();	/* Timestamp line */
      strcpy(Entry.log_timestamp, logfile_rest());
    }
}

void
assert_logstream_empty()
{
  assert(Control.using_logfile);
  if (EOF != getc(Gct_input_stream))
    {
      fprintf(stderr, "Logfile has more entries than the mapfile.\n");
      exit(1);
    }
}

void
check_timestamps()
{
  if (strcmp(Entry.map_timestamp, Entry.log_timestamp))
    {
      fprintf(stderr, "The mapfile and logfile come from two different instrumentations.\n");
      fprintf(stderr, "The mapfile comes from one begun on %s.\n", Entry.map_timestamp);
      fprintf(stderr, "The logfile comes from one begun on %s.\n", Entry.log_timestamp);
      exit(1);
    }
}


			     /* COUNTS */

/*
 * From an integer and a t_edit, construct a
 * count.  The caller must free the return value.
 */

t_count 
build_count(value, edit)
     long value;
     t_edit edit;
{
  t_count retval = (t_count) xmalloc(sizeof(struct count));

  retval->val = value;
  retval->edit = edit;
  return retval;
}

/*
 * This adds two counts together and returns a count.  The caller must
 * later free the new count.
 */
t_count
add_count(first, second)
     t_count first, second;
{
  t_count retval = (t_count) xmalloc(sizeof(struct count));
  assert(first);
  assert(second);

  retval->val = first->val + second->val;
  retval->edit = COMBINE_LOCAL_EDITS(first->edit, second->edit);
  return retval;
}

/*
 * Return the a printable string denoting the count.  Caller must NOT
 * free the string; it is a static string that is reused on a later call.
 * However, the string will not be overwritten until the 5th call after
 * the call that created it.
 * 
 * If EDIT is true, the EDIT_EDIT_TOKEN is printed, not the USER_EDIT_TOKEN.
 * The former is more useful for editing.
 */
char *printable_count(count, edit)
     t_count count;
     int edit;
{
  static char buffer[5][100];	/* Have to be a pretty big integer to overflow. */
  static int which = 0;

  which++;
  if (5 == which) which = 0;
  sprintf(buffer[which], "%lu%s", count->val,
	  edit ? EDIT_EDIT_TOKEN(count->edit) : USER_EDIT_TOKEN(count->edit));
  return buffer[which];
}

  


	      /* HANDLING MAP AND LOG FILES TOGETHER  */


/* Internal Utilities. */


/*
 * next_entry(where, eof)
 *
 * This routine makes the mapfile and logfile line for the next DATA
 * entry available.
 *
 * The arguments control action on failure:
 *
 * If where is IMMEDIATE but no entry is immediately found, error out.
 * If eof is ERROR_ON_EOF but no entry is found, error out.
 *
 * If no entry is found and EOF_OK, 0 is returned.  Otherwise, the
 * non-error return is 1.
 *
 * The arguments apply only to the mapfile.  In particular, there is no
 * such thing as EOF on the logfile.  Once the actual entries run out, an
 * unending stream of "0" strings is returned.  Note that this will break
 * if the logfile format changes, but probably in a pretty obvious way.
 *
 * After this routine is called, the current position is set so that
 * the suppression tag is ready to read.
 */


static int
next_entry(where, eof)
     enum entry_where where;
     enum on_eof eof;
{
  for(;;)
    {
      
      raw_mapfile_entry(eof);
      mapfile_type();
      if (DATA == Entry.type)
	{
	  Entry.index++;
	  if (Control.using_logfile)
	    raw_logfile_entry();
	  return 1;
	}
      else if (NONE == Entry.type)
	{
	  mapfile_truncation_if(ERROR_ON_EOF == eof);
	  return 0;
	}
      else	/* We have a header line.  Check if it's of interest */
	{
	  char *rest_of_line;	/* Part of the line after the header. */
	  int file_matched;	/* It's a !File: line. */

	  mapfile_corruption_if(IMMEDIATE==where);
	  if (   (file_matched = mapfile_header_match("File"))
	      || mapfile_header_match("Internal-File"))
	    {
	      if (Entry.inner_filename)
		free(Entry.inner_filename);
	      Entry.inner_filename = permanent_string(mapfile_token());
	      if (file_matched)
		{
		  if (Entry.main_filename)
		    free(Entry.main_filename);
		  Entry.main_filename = permanent_string(Entry.inner_filename);
		  Entry.filename_edit = mapfile_raw_edit();
		  file_external_edit(Entry.main_filename);
		  Entry.internal_filename_edit = DONT_CARE_COUNT;
		}
	      else
		{
		  Entry.internal_filename_edit = mapfile_raw_edit();
		}
	    }
	  else if (mapfile_header_match("Routine"))
	    {
	      if (Entry.routinename)
		free(Entry.routinename);
	      Entry.routinename = permanent_string(mapfile_token());
	      Entry.routine_edit = mapfile_raw_edit();
	      routine_external_edit(Entry.routinename);
	    }
	}
    }
}

/*
 * Precondition:  next_entry has returned successfully.
 * Initialize a probe from this line.
 *
 * Edits are handled in two ways.  The Line edit is returned in the
 * LINE_COUNT field.  The cumulative edit, taking into account the
 * Routine and File and Internal_file edits, is returned in the COUNT
 * field.  Note that the count fields are only meaningful if the logfile
 * is opened.  If not, they're set to NULL.
 */
static 
fill_probe(probe)
     struct single_probe *probe;
{
  t_edit line_edit;		/* Is this entry affected by any edit? */
  
  probe->main_filename = Entry.main_filename;
  probe->inner_filename = Entry.inner_filename;
  probe->routinename = Entry.routinename;
  probe->index = Entry.index;

  line_edit = mapfile_raw_edit();
  probe->lineno = mapfile_token_as_integer();
  probe->kind = permanent_string(mapfile_token());
  probe->rest_text = permanent_string(mapfile_rest());
  
  if (Control.using_logfile)
    {
      unsigned long count = logfile_token_as_unsigned_long();
      probe->count = build_count(count, cumulative_edit(line_edit));
      probe->line_count = build_count(count, line_edit);
    }
  else
    {
      probe->count = (t_count) 0;
      probe->line_count = (t_count) 0;
    }
}
     

/* Exported functions */

/* A single instrumentation may take up this many mapfile entries. (loops) */
#define MAX_COMBINED_PROBES 4

/* So here are the probes that the caller may be using at one time. */
static struct single_probe probes[MAX_COMBINED_PROBES];
static int probes_in_use = 0;

/*
 * Filenames are not freed, since they are shared among several probes.
 * Ditto for routinenames.
 */
#define FREE_PROBE_FIELDS(probe)	\
  if ((probe).kind) free((probe).kind);		\
  if ((probe).rest_text) free ((probe).rest_text); \
  if ((probe).count) free((probe).count); \
  if ((probe).line_count) free((probe).line_count);


/* GET_PROBE()
 * Preconditions: (assumed)
 * 	1.  The Mapfile is open.
 * 	2.  The input file is open to a log file.
 * Postconditions:
 * 	1.  The return value is a pointer to a filled-in single_probe
 * 	    entry describing the next entry in the map and log file.
 *	2.  If there is no next entry, the null pointer is returned.
 * Obligations:
 * 	1.  Don't free the return value.
 * 	2.  Don't change any of the values.
 */

struct single_probe *
get_probe()
{
  while (--probes_in_use >= 0)
    {
      FREE_PROBE_FIELDS(probes[probes_in_use]);
    }
  probes_in_use = 0;
  
  if (next_entry(ANYWHERE, EOF_OK))
    {
      fill_probe(probes);
      probes_in_use = 1;
      return probes;
    }
  else
    {
      return (struct single_probe *)0;
    }
}


/*
 * SECONDARY_PROBE()
 * Preconditions: 
 * 	1.  The Mapfile is open. (assumed)
 * 	2.  The input file is open to a log file. (assumed)
 * 	3.  There is an immediate continuation mapfile entry and logfile
 * 	    entry.
 * 		On failure:  exit with error message.
 * Postconditions:
 * 	1.  The return value is a pointer to a filled-in single_probe
 * 	    entry describing the next entry in the map and log file.
 * Obligations:
 * 	1.  Don't free the return value.
 * 	2.  Don't change any of the values.
 */


struct single_probe *
secondary_probe()
{
  if (probes_in_use >= MAX_COMBINED_PROBES)
    {
      fprintf(stderr, "Program error:  too many probes used.\n");
      abort();
    }
  (void) next_entry(IMMEDIATE, ERROR_ON_EOF);
  fill_probe(probes + probes_in_use);
  probes_in_use++;
  return probes + probes_in_use - 1;
}


/*
 * Find the mapfile entry matching "index".  The logfile entry is read as
 * well (if open).  Calls to numbered_mapfile_entry must use strictly
 * increasing INDEX arguments.
 */
void
numbered_mapfile_entry(index)
{
  for(;;)
    {
      (void) next_entry(ANYWHERE, ERROR_ON_EOF);
      if (Entry.index == index)
	break;
      mapfile_corruption_if(Entry.index > index);
    }
}


/*
 * Caller must have used numbered_mapfile_entry to read the correct
 * Entry.
 *
 * This routine edits the edit text for that entry IN THE FILE, not in
 * the Entry structure.  The file position is unchanged.  The Entry is
 * unchanged.
 *
 * Remember that the entire line has already been read into the Entry.
 */
void
mark_suppressed(edit)
     t_edit edit;
{
  long here = ftell(Gct_map_stream);	/* Remember end-of-line position */
  mapfile_moveto(mapfile_ftell());	/* Move earlier in the line. */
  fprintf(Gct_map_stream, "%s", MAP_EDIT_TOKEN(edit));
  mapfile_moveto(here);			/* Move back to end of line. */
}

      

			 /* MEMORY AND MISC*/


char *
xmalloc(size)
     int size;
{
  char *buffer = (char *) malloc(size);
  if ((char *) 0 == buffer)
    {
      fprintf(stderr, "Program error: Ran out of virtual memory.\n");
      exit(1);
    }
  return buffer;
}

char *
xcalloc(nelem, elsize)
     int nelem;
     unsigned elsize;
{
  char *buffer = (char *) xmalloc(nelem * elsize);
  bzero(buffer, nelem * elsize);
  return buffer;
}

char *
xrealloc(ptr, size)
     char *ptr;
     unsigned size;
{
  char *buffer = (char *) realloc(ptr, size);
  if (NULL == buffer)
    {
      fprintf(stderr, "Program error: Ran out of virtual memory.\n");
      exit(1);
    }
  return buffer;
}


#ifdef USG

getwd(buf)
	char *buf;
{
    return (getcwd(buf, PATH_BUF_LEN) != NULL);
}

#endif
