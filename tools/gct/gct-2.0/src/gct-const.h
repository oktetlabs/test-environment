/* Copyright (C) 1992 by Brian Marick.  All rights reserved. */
/*
 * When aggregated with works covered by the GNU General Public License,
 * that license applies to this file.  Brian Marick retains the right to
 * apply other terms when this file is distributed as a part of other
 * works.  See the GNU General Public License (version 2) for more
 * explanation.
 */

/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/gct-const.h,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */

/* Fundamental constants. */


#define GCT_BUFSIZE 1200	/* Random buffers for system, etc. */


#define GCT_BACKUP_DIR			"gct_backup"
#define GCT_RESTORE_LOG			"gct-rscript"
#define GCT_MAP				"gct-map"
#define GCT_PER_SESSION			"gct-ps-defs.h"
#define GCT_PER_SESSION_DEFINITIONS	"gct-ps-defs.c"
#define GCT_WRITE_SOURCE		"gct-write.c"

/* These are compiled files sometimes magically linked into an
   executable.  Note that object files are always in the current
   directory. */
#define GCT_COMPILED_PER_SESSION_DEFINITIONS	"gct-ps-defs.o"
#define GCT_COMPILED_WRITE_SOURCE	"gct-write.o"

/* The pseudo-function that marks a probe taken. */
#define G_MARK1	"_G"

/* The pseudo-function that marks probe i taken if the test is true, i+1 if
   the test is false. */
#define G_MARK2	"_G2"

/* Names of functions in gct-defs.h */
#define G_INC "GCT_INC"
#define G_SET "GCT_SET"
#define G_GET "GCT_GET"

/* The version of the mapfile we work with.  There is no corresponding
   definition for the logfile. */
#define GCT_MAPFILE_VERSION "!GCT-Mapfile-Version: 2.0"

/* Formats for the PER_SESSION file. (gct-ps-defs.h) */
#define PER_SESSION_INDEX_FORMAT "#define GCT_NUM_CONDITIONS %d\n"
#define PER_SESSION_RACE_FORMAT "#define GCT_NUM_RACE_GROUPS %d\n"
#define PER_SESSION_FILES_FORMAT "#define GCT_NUM_FILES %d\n"

/* Formats for the PER_SESSION_DEFINITION file. (gct-ps-defs.c) */
#define DEFINITION_TIMESTAMP_PREFIX	"char *Gct_timestamp = \""
#define DEFINITION_TIMESTAMP_FORMAT	"char *Gct_timestamp = \"%s\";\n"

#define DEFINITION_FILE_DATA_FORMAT "/* %s var number %d - %d entries start at %d - %d race start at %d */\n"
#define DEFINITION_FILE_LOCALDEF "GCT_CONDITION_TYPE *Gct_per_file_table_pointer_%d = Gct_table + %d;\n"
#define DEFINITION_FILE_RACEDEF  "long *Gct_per_file_race_table_pointer_%d = Gct_group_table + %d;\n"

