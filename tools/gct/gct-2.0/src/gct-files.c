/* Copyright (C) 1992 by Brian Marick.  All rights reserved. */
/*
 * When aggregated with works covered by the GNU General Public License,
 * that license applies to this file.  Brian Marick retains the right to
 * apply other terms when this file is distributed as a part of other
 * works.  See the GNU General Public License (version 2) for more
 * explanation.
 */

/* File manipulation utilities. */
#include "config.h"
#include <string.h>
#include "gct-files.h"


/* String is in temporary memory.  Put it in malloc'd memory. */
char *
permanent_string(string)
     char *string;
{
  char *perm = (char *) xmalloc(strlen(string)+1);
  strcpy(perm, string);
  return perm;
}

/*
 * Split_file separates the given filename into two parts:  the directory
 * part and the file part.  If there is no directory part, "." is returned.
 * In all cases, the returned values are new storage and must be freed by
 * the caller.
 */

void
split_file(input, dir, file)
     char *input;
     char **dir;
     char **file;
{
  char *slash = strrchr(input, '/');
  char *permanent_string();
  
  if (slash)
    {
      *slash = '\0';
      *dir = (char *)xmalloc(strlen(input)+1);
      strcpy(*dir, input);
      *file = (char *)xmalloc(strlen(slash+1)+1);
      strcpy(*file, slash+1);
      *slash = '/';
    }
  else
    {
      *dir = permanent_string(".");
      *file = permanent_string(input);
    }
}


/*
 * This routine takes an ORIGINAL file name and a MASTER_DIR.  
 * If ORIGINAL is an absolute pathname, it is the return value.
 * Otherwise a pathname relative to the MASTER_DIR is constructed and
 * returned.
 *
 * New storage may or may not be allocated.  The caller is expected never
 * to free the returned value or either of the passed-in values.
 */

char *
gct_expand_filename(original, master_dir)
     char *original, *master_dir;
     
{
  if ('/' == original[0] )
    {
      return original;
    }
  else
    {
      char *retval = (char *) xmalloc(2 + strlen(master_dir) + strlen(original));
      strcpy(retval, master_dir);
      strcat(retval, "/");
      strcat(retval, original);
      return retval;
    }
}

