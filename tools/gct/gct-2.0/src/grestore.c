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

/* $Header: /mnt/data/repos/cvsroot/test_environment.sav/tools/gct/gct-2.0/src/grestore.c,v 1.1 2003/10/28 09:48:45 arybchik Exp $ */
/*
 * Restoring instrumented files is simple -- gct has written the
 * instructions in the log file.  We merely need to execute it.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "gct-const.h"
#include <errno.h>

main(argc,argv)
     int argc;
     char *argv[];
{
  extern int errno;
  struct stat statbuf;		/* Check whether log exists. */
  char *system_buf;
  int i;
  
  for (i= 1; i < argc; i++) {
    if (!strcmp("-test-dir", argv[i]))
      {
	i++;
	if (-1 == chdir(argv[i]))
	  {
	    fprintf(stderr, "Couldn't change to master directory %s.\n", argv[i]);
	    perror("");
	    exit(1);
	  }
      }
    else
      {
	fprintf(stderr,"grestore: Unknown argument %s.\n",argv[i]);
	exit(1);
      }
  }

  /* Shell's message is less informative. */
  if (-1 == stat(GCT_RESTORE_LOG, &statbuf) && ENOENT == errno)
    {
      fprintf(stderr, "Log file %s does not exist.\n", GCT_RESTORE_LOG);
      fprintf(stderr, "Note:  gct-init removes this file.  But there's a backup in %s.bk.\n", GCT_RESTORE_LOG);
      fprintf(stderr, "Further, the original versions of instrumented files are still in the \n");
      fprintf(stderr, "%s directories.  See the manpage for more.\n", GCT_BACKUP_DIR);
      exit(1);
    }
  system_buf = (char *)malloc(strlen(GCT_RESTORE_LOG)+100);
  if ((char *)0 == system_buf)
    {
      fprintf(stderr, "Ran out of memory?\n");
      exit(1);
    }
  sprintf(system_buf, "/bin/sh %s", GCT_RESTORE_LOG);
  system(system_buf);
  exit(0);
}



