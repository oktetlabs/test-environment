/* Copyright (C) 1992 by Brian Marick.  All rights reserved. */
/*
 * When aggregated with works covered by the GNU General Public License,
 * that license applies to this file.  Brian Marick retains the right to
 * apply other terms when this file is distributed as a part of other
 * works.  See the GNU General Public License (version 2) for more
 * explanation.
 */


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "gct-const.h"
#include "gct-files.h"


#define PATH_BUF_LEN	2025  /* Because there's no portable system define. */


		     /* FILES AND DIRECTORIES. */

/* The various filenames used, both in their original and expanded forms. */
extern char *Gct_test_dir;		/* Master directory. */

extern char *Gct_test_map;
extern char *Gct_full_map_file_name;
extern FILE *Gct_map_stream;

extern char *Gct_input;		/* Log file name, usually */
extern FILE *Gct_input_stream;	/* Its open stream. */

extern void init_mapstream();
extern void init_other_stream();

  

			     /* COUNTS */

/*
 * These tools work on counts, which contain two parts: a numeric part
 * and a suppression tag, called an "edit".  The numeric part is read
 * from the logfile.  The edit is one of
 *
 * DONT_CARE_COUNT
 * 	Display by default, but let one of the others override.
 * SUPPRESSED_COUNT
 * 	Treat the count as if it were non-zero, even if it's zero.
 * 	Only appears in normal greport output if some other component of a
 * 	greport output line (e.g., the other case of an IF) is
 * 	zero and not suppressed.  Does appear in greport -all output.
 * IGNORED_COUNT
 * 	Line does not ever appear in greport output or gsummary totals.
 * 	Ignoring one component of a greport output line (such as one
 * 	half of an IF) has the effect of ignoring all of them.
 * VISIBLE_COUNT
 * 	This is like DONT_CARE_COUNT, in that it allows a condition to
 * 	be visible.  It differs in that you want it to override other
 * 	edits.
 *
 * Edits are combined with subsidiarity:  the more local the edit, the
 * higher the precedence.  A line edit SUPPRESSED will override a file
 * IGNORED, for example.  DONT_CARE, of course, doesn't override
 * anything.
 *
 * When combining two edits at the same level, precedence increases with
 * order in above list.
 *
 * Subsidiarity is a problem for internal-file edits, since routines may
 * span internal-files.  Nevertheless, routines are usually nested within
 * them, so routines will be considered more local.
 *
 * My thanks to the European Economic Community for "subsidiarity".
 */

typedef enum
{
  DONT_CARE_COUNT,
  SUPPRESSED_COUNT,
  IGNORED_COUNT,
  VISIBLE_COUNT
} t_edit;


typedef struct count
{
  unsigned long val;
  t_edit edit;
} *t_count;


extern t_count build_count();
extern t_count add_count();
extern char *printable_count();

/*
 * What an edit value should look like to a user viewing coverage results.
 * Using code may depend on the fact that count tokens are one character
 * at most.  No distinction is made between DONT_CARE and VISIBLE.
 *
 */
#define USER_EDIT_TOKEN(edit)	\
	(IGNORED_COUNT == (edit) ? "I" :	\
	  (SUPPRESSED_COUNT == (edit) ? "S" : ""))

/* This is used to describe what's done with an edit.  The distinction
   between DONT_CARE and VISIBLE is important. */
#define LONG_USER_EDIT_TOKEN(edit)	\
	(IGNORED_COUNT == (edit) ? "ignored" :	\
	  (SUPPRESSED_COUNT == (edit) ? "suppressed" : \
	   (VISIBLE_COUNT == (edit) ? "forced to be visible" : \
	      "handled normally")))

/*
 * What an edit value should look like to a user who might want to change
 * it.  The distinction between VISIBLE and DONT_CARE is now important.
 */
#define EDIT_EDIT_TOKEN(edit)	\
	(IGNORED_COUNT == (edit) ? "I" :	\
	  (SUPPRESSED_COUNT == (edit) ? "S" :	\
	   (VISIBLE_COUNT == (edit) ? "V" : "")))

/* What an edit value should look like in the mapfile. */
#define MAP_EDIT_TOKEN(edit)	\
	(IGNORED_COUNT == (edit) ? "I" :	\
	  (SUPPRESSED_COUNT == (edit) ? "S" :	\
	   (VISIBLE_COUNT == (edit) ? "V" : "-")))

/*
 * Merge two edits.  The first is supposed to be more local, so takes
 * priority.
 */
#define COMBINE_EDIT_LEVELS(local, national) \
  (DONT_CARE_COUNT == local ? national : local)

/* Merge two edits at the same level.  Note:  no side effects. */
#define COMBINE_LOCAL_EDITS(edit1, edit2) \
  ((VISIBLE_COUNT == (edit1) || VISIBLE_COUNT == (edit2))	\
      ? VISIBLE_COUNT \
      : ((IGNORED_COUNT == (edit1) || IGNORED_COUNT == (edit2))	\
	 ? IGNORED_COUNT \
         : ((SUPPRESSED_COUNT == (edit1) || SUPPRESSED_COUNT == (edit2)) \
	      ? SUPPRESSED_COUNT \
              : DONT_CARE_COUNT)))

  
/*
 * If IGNORE_COUNT is set and not overridden with VISIBLE_COUNT, we
 * display; otherwise we completely ignore the line.  These utilities
 * make the checks easier.
 */

#define DO_IGNORE(edit)   (IGNORED_COUNT == (edit))

#define DO_IGNORE_2(edit1, edit2) \
  (DO_IGNORE(COMBINE_LOCAL_EDITS((edit1), (edit2))))

#define DO_IGNORE_4(edit1, edit2, edit3, edit4)	\
  (DO_IGNORE(COMBINE_LOCAL_EDITS(COMBINE_LOCAL_EDITS((edit1), (edit2)),	\
				 COMBINE_LOCAL_EDITS((edit3), (edit4)))))
		     

			   /* PROBE DATA */

/*
 * This structure defines all that's known about a single probe (that is,
 * a data entry in the mapfile).  Note that the count field includes
 * ANY editing data that applies to this line, be it a line edit, routine
 * edit, whatever. 
 */
struct single_probe
{
  char *main_filename;	/* The file it's from - gsummary uses this.*/
  char *inner_filename;	/* Included file it's from - string-equal to
			 * main_filename when not from an include file.
			 * greport uses this. */
  char *routinename;	/* The routine it's from */
  int lineno;		/* The line it's from */
  int index;		/* Position in the log file. */
  t_count count;	/* Its count (including all suppression info) */
  t_count line_count;	/* Line count, including only this line's suppression info */
  char *kind;		/* The "tag" printed into the mapfile. */
  char *rest_text;	/* Whatever other text belongs to the probe */
};

extern struct single_probe * get_probe();
extern struct single_probe * secondary_probe();

  
			     /* MEMORY */

extern char *xmalloc();
extern char *xrealloc();
extern char *xcalloc();


		/* MAPFILE TRAVERSALS - Definitions */

/*
 * There are two types of entries - data entries and header entries, but
 * we may also treat EOF as a type of entry.
 */
enum entry_type
{
  UNKNOWN,	/* Not determined yet */
  DATA,
  HEADER,
  NONE		/* EOF found */
};

/* Where a caller expects to find a data entry. */
enum entry_where 
{
  IMMEDIATE,	/* Without any intervening headers. */
  ANYWHERE
};

/*
 * What a caller wants done if EOF is encountered before an entry.
 * Also used to indicate what should happen when an unexpected
 * end-of-line is encountered.
 */
enum on_eof
{
  ERROR_ON_EOF,	/* It is an error to not find a next entry. */
  EOF_OK	/* It's OK for there to be no more entries. */
};



/*
 * This structure is needed for two reasons.
 * 1. Some information about an entry that a caller will want - the
 *    filename, for example - is not on a data line, but on earlier
 *    header lines.  It has to be stored somewhere.
 *
 * 2. It is also convenient to suck a mapfile or logfile entry up into
 *    a buffer and use string operations to process it.
 *
 * Fields:
 *
 * MAIN_FILENAME
 * 		The name of the main file this entry is from.
 * 		Always the file originally being compiled.
 * INNER_FILENAME
 * 		The name of the file directly including this line.
 * 		If not from an include file, string-equal to MAIN_FILENAME.
 *
 * 		Note that the MAIN_FILENAME/INNER_FILENAME pair 
 * 		has a slightly different interpretation than 
 * 		the way filenames and internal-filenames appear in the
 * 		mapfile.  The names differ to make sure you think
 * 		about which you really want.
 * 		
 * FILENAME_EDIT
 * 		A t_edit entry indicating suppression that applies
 * 		to the entire file.
 * INTERNAL_FILENAME_EDIT
 * 		Suppression applying to an internal filename.
 * ROUTINENAME	The name of the routine this entry is from.
 * ROUTINE_EDIT
 * 		A t_edit entry for the function.
 * INDEX	The index of this (or previous) data entry, beginning at 0.
 * 		No gaps are allowed in the mapfile or logfile.
 * TYPE		The type of the mapline we're currently looking at, including
 * 		NONE if we've run out of maplines (hit eof).
 * MAPLINE,
 *  MAPLINE_NEXT	MAPLINE is a complete (newline-terminated) line from
 * 		the mapfile.  It may be either a header or data entry.
 * 		As parts of the line are consumed, MAPLINE_NEXT is advanced
 * 		to point to the next token.  Return values from
 * 		token-returning "member functions" are constructed in place
 * 		by sticking nulls into MAPLINE.
 * MAPLINE_FILE_POSITION
 * 		If we want to edit the mapfile, we need to know where
 * 		to position the file pointer.
 * 		This is the file position (as told by ftell()) of the
 * 		first character of an entry.  Positions within the
 * 		line are constructed by pointer arithmetic.
 *
 * LOGLINE,
 *  LOGLINE_NEXT	These are analogous to MAPLINE and MAPLINE_NEXT.
 *
 * MAP_TIMESTAMP,	We retain the timestamps from each file for possible later
 *  LOG_TIMESTAMP  comparison.
 */

#define FILE_BUFFER_SIZE (100 + PATH_BUF_LEN)	/* Big enough for either file's lines. */


typedef struct entry
{
  char *main_filename;
  char *inner_filename;
  char *routinename;
  int index;

  enum entry_type type;

  t_edit filename_edit,
  	 internal_filename_edit,
  	 routine_edit;
  
  char mapline[FILE_BUFFER_SIZE];
  long mapline_file_position;	/* From ftell */
  char *mapline_next;
  char map_timestamp[FILE_BUFFER_SIZE];

  char logline[FILE_BUFFER_SIZE];
  char *logline_next;
  char log_timestamp[FILE_BUFFER_SIZE];
} gct_entry;

extern gct_entry Entry;

/* "Member" functions - see g-tools.c for definitions. */

void assert_logstream_empty();
void check_timestamps();
char *mapfile_timestamp();
void raw_mapfile_entry();
void raw_logfile_entry();
void mapfile_type();
int mapfile_header_match();
char * logfile_token();
char * mapfile_token();
unsigned long logfile_token_as_unsigned_long();
int mapfile_token_as_integer();
char * logfile_rest();
char * mapfile_rest();
long mapfile_ftell();
void mapfile_moveto();
t_edit mapfile_raw_edit();

#ifndef GCT_KIT1
void set_default_routine_external_edit();
void set_default_file_external_edit();
void add_file_external_edit();
void add_routine_external_edit();
#endif
