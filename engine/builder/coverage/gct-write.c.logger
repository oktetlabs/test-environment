/* Copyright (C) 1992 by Brian Marick.  All rights reserved.  */
/* v 1.5
 * Permission is granted to use this file for any purpose whatsoever.
 * There are no restrictions on distribution, use, or modification.  In
 * particular, inclusion of this file in any instrumented program is
 * freely permitted, and you may distribute that program to third parties
 * under any terms.
 *
 * This file is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.
 */


#include "gct-defs.h"
#include <stdio.h>
#include <errno.h>
extern int errno;	/* On some systems, it's not defined in errno.h. */
extern char *Gct_timestamp;	/* Defined in gct-ps-defs.c */


/*
 * What you need to know if you're modifying this code:
 *
 * 1. To make a change to the version that gct-init installs, edit
 * <gct-root>/src/gct-write.G, not <gct-root>/src/gct-write.c.  If you're
 * making a change to a private copy, edit gct-write.c in your directory.
 * gct-init will not overwrite it.
 *
 * 2. The first line of the logfile identifies it as a GCT logfile.  This
 * is purely for the convenience of humans.  No program depends on its
 * exact contents (though they do require the timestamp to be on the
 * second line).
 *
 * 3. gct_readlog doesn't actually read the log.  That's done as a part
 * of writing the log.  It instead (1) records that the log should be
 * read and (2) checks to see that the logfile matches this executable
 * (by comparing timestamps).  A mismatch means the log will not be
 * written.
 *
 * The check is done at program startup so that the the user can kill a
 * useless test run (or quickly delete the bad logfile - which works
 * because the check is made again at writelog time).
 *
 * 4. This executable has an idea of the size of the log.  Different
 * matching executables may have different ideas, and they may have
 * written logfiles that are shorter or longer than expected.  (This
 * happens if they are instrumented and linked earlier or later than this
 * executable).  If the executable is too short, the missing entries are
 * treated as if they were 0.  If it is too long, they are preserved (if
 * readlog was used) or erased (if it was not).
 */ 




			   /* Utilities */

/*
 * This routine is given a file, presumed to be the name of a GCT log
 * file.  That file is opened for reading and the stream returned.  If
 * that fails because the file does not exist, NULL is returned.  If it
 * fails for some other reason, an error message is printed 
 * and NULL is returned.  This last may lead to printing of essentially
 * the same error message again, but it's not the business of this code
 * to make the program exit.
 */

static FILE *
openlog(file)
     char *file;
{
  FILE *stream = fopen(file, "r");
  if (!stream)
    {
      if (ENOENT != errno)
	{
	  fprintf(stderr, "gct logging routines: Couldn't open log %s.\n", file);
	  perror("gct logging routines");
	}
    }
  return stream;
}

  
/*
 * This routine is given a stream, presumed to be open for reading to a
 * GCT logfile.
 *
 * If the stream has a corrupt or missing timestamp,
 * 	The stream is closed.
 * 	NULL is returned.
 * 	An error message is printed.
 * If the file exists and has a consistent timestamp,
 * 	the file pointer is returned.
 * 	The file is still open for reading.
 * 	The file pointer is positioned at the first index entry.
 *
 * This routine may be called several times (in between which the user
 * may have fixed any problems discovered).  It prints error messages
 * only once (because usually the user won't have).
 */
static FILE *
checklog(stream, file)
     FILE *stream;
     char *file;	/* Stream's file - for error messages. */
{
  static int error_printed = 0;	/* Show errors only once. */
#define TIMESTAMP_LEN 500	/* Very sufficient */
  char timestamp[TIMESTAMP_LEN];
  

  /* Suck up the identifying header and timestamp. */
  if (   fgets(timestamp, TIMESTAMP_LEN, stream) == NULL
      || fgets(timestamp, TIMESTAMP_LEN, stream) == NULL)
    {
      if (! error_printed)
	{
	  fprintf(stderr, "gct logging routines: %s has only a partial header.\n",
		  file);
	  fprintf(stderr, "gct logging routines: The logfile will not be updated.\n");
	  error_printed = 1;
	}
      goto error;
    }

  timestamp[strlen(timestamp)-1] = '\0';	/* Zorch newline */
  if (strcmp(timestamp, Gct_timestamp))
    {
      if (! error_printed)
	{
	  fprintf(stderr, "gct logging routines: Warning: logfile %s is out of date.\n", file);
	  fprintf(stderr, "gct logging routines: The executable is from '%s'.\n", Gct_timestamp);
	  fprintf(stderr, "gct logging routines: The logfile is from '%s'.\n", timestamp);
	  fprintf(stderr, "gct logging routines: The logfile will not be updated.\n");
	  error_printed = 1;
	}
      goto error;
    }
  
  if (error_printed)
    {
      fprintf(stderr, "gct logging routines: Earlier reported error has been corrected.\n", file);
      fprintf(stderr, "gct logging routines: The logfile will be updated.\n");
    }
  
  return stream;

error:
  fclose(stream);
  return NULL;
}


/*
 * This routine writes the named FILE without regard to its existing
 * contents.  Called if (options -readlog) or no FILE exists.
 */

static void
gct_write_new_log(file)
     char *file;
{
  FILE *stream;
  int index;

  stream = fopen(file, "w");
  if (!stream)
    {
      fprintf(stderr, "gct logging routines: Couldn't create log %s.\n", file);
      perror("gct logging routines");
      return;
    }

  fprintf(stream, "GCT Log File\n");
  fprintf(stream, "%s\n", Gct_timestamp);
  for (index = 0; index < Gct_num_conditions; index++)
    fprintf(stream, "%lu\n", Gct_table[index]);

  if (ferror(stream))
    {
      fprintf(stderr, "gct logging routines: Failed to write log file %s.\n", file);
      perror("gct logging routines");
    }
  fclose(stream);
}

/*
 * This routine writes the named FILE.  Entries are the sum of the
 * Gct_table and entries read from OLDSTREAM.  Gct_table is updated with
 * the old entries so that later calls to gct_writelog include them.
 *
 * Assumed:  OLDSTREAM is opened to a logfile for this executable,
 * positioned at the first entry.  Overwriting the FILE will not
 * affect the OLDSTREAM.
 */
static void
gct_update_old_log(file, oldstream)
     char *file;	/* File we're updating. */
     FILE *oldstream;	/* old values come from this stream. */
{
  FILE *newstream;	/* new values go to this stream. */
  int index;		/* Counter for our GCT_table. */
  int fscanf_retval;		/* Was read successful. */
  unsigned long old_count;	/* If so, here's the value. */

  newstream = fopen(file, "w");
  if (!newstream)
    {
      fprintf(stderr, "gct logging routines: Couldn't open log %s for update.\n", file);
      return;
    }

  fprintf(newstream, "GCT Log File\n");
  fprintf(newstream, "%s\n", Gct_timestamp);
  for (index = 0; index < Gct_num_conditions; index++)
    {
      fscanf_retval = fscanf(oldstream, "%lu\n", &old_count);
      if (fscanf_retval != 1)	/* Ran out of entries. */
	{
	  old_count = 0;	/* Missing entries taken to be 0. */
	}
      Gct_table[index] += old_count;
      fprintf(newstream, "%lu\n", Gct_table[index]);
    }
  /* If old file log is longer than in-core log, preserve entries. */
  for(;;)
    {
      fscanf_retval = fscanf(oldstream, "%lu\n", &old_count);
      if (fscanf_retval != 1)
	break;
      fprintf(newstream, "%lu\n", old_count);
    }

  if (ferror(oldstream))
    {
      fprintf(stderr, "gct logging routines: Failed to read log file %s for updating.\n", file);
      perror("gct logging routines");
    }
  if (ferror(newstream))
    {
      fprintf(stderr, "gct logging routines: Failed to write log file %s.\n", file);
      perror("gct logging routines");
    }
  fclose(newstream);
}


			    /* Locking */

/*
 * By default, locking is turned off.
 * Turn it on by defining GCT_LOCKING (in this file).
 * This implementation is believed to be POSIX-compliant, but you should
 * look it over to see if it does what you want.
 *
 * 1. We lock on a file in /usr/tmp.  There is a potential race if someone 
 *    deletes that file while processes are blocked on it.  A new process
 *    may recreate the file, be unaffected by old locks, and update GCTLOG.
 *    Pretty low probability.
 *
 *    Can't lock on GCTLOG itself, because that file is being deleted and
 *    created during update.  See gct_writelog().
 *
 *    You might also consider locking on an omnipresent readable/writable
 *    file (/dev/null).
 *
 * 2. If the program fails to acquire the read lock, the calling routine
 *    returns noisily.  However, this failure does not prevent the logfile
 *    from being updated if the write lock can later be acquired.
 *    The read lock is only used to protect the initial
 *    check of the logfile header, which is checked again later.  Thus,
 *    an early return causes no data loss.
 *
 * 3. If the program fails to acquire the write lock, the calling routine
 *    fails noisily and the logfile is not updated.
 *
 * 4. Receipt of a signal will cause a locking failure.  Might be preferable
 *    to retry.
 *
 *
 * An implementation could use semaphores - I suspect those are less
 * portable, though.
 */

#define GCT_LOCKFILE  "/usr/tmp/__gct_lock__"
/* #define GCT_LOCKING */
#ifndef GCT_LOCKING

/* Null operations. */
#define GCT_READ_LOCK_OR_RETURN()
#define GCT_WRITE_LOCK_OR_RETURN()
#define GCT_UNLOCK()

#else	

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define GCT_READ_LOCK_OR_RETURN() \
   if (!gct_get_lock(F_RDLCK, "read")) return;
     
#define GCT_WRITE_LOCK_OR_RETURN() \
   if (! gct_get_lock(F_WRLCK, "write")) \
   { \
     fprintf(stderr, "gct logging routines: The logfile will not be updated.\n"); \
     return; \
   }

/* For unlocking, error handling doesn't make much sense -- any
   damage has already been done. */
#define GCT_UNLOCK() gct_unlock()

int Lock_fid = -1;

static int
gct_get_lock(command, tag)
     int command;
     char *tag;
     
{
  struct flock arg;

  Lock_fid = open(GCT_LOCKFILE, O_RDWR | O_CREAT, 0666);
  if (Lock_fid < 0)
    {
      fprintf(stderr, "gct logging routines: Couldn't open lock file.\n");
      perror("gct logging routines");
      return 0;
    }

  /* open's mode is affected by umask, so it might not be 666. */
  if (-1 == chmod(GCT_LOCKFILE, 0666))
    {
      fprintf(stderr, "gct logging routines: Couldn't change mode of lock file.\n");
      perror("gct logging routines");
      return 0;
    }

  arg.l_type = command;
  arg.l_whence = SEEK_SET;
  arg.l_start = 0;
  arg.l_len = 0; 	/*to EOF*/

  if (-1 == fcntl(Lock_fid, F_SETLKW, (int) &arg))
    {
      fprintf(stderr, "gct logging routines: Attempt to get %s lock failed.\n", tag);
      perror("gct logging routines");
      return 0;
    }
  return 1;
}


static void
gct_unlock()
{
  struct flock arg;

  if (-1 == Lock_fid)
    {
      fprintf(stderr, "gct logging routines program error: unlocking with no lock.\n");
      return;
    }

  /* Closing a file removes the locks. */
  if (-1 == close(Lock_fid))
    {
      fprintf(stderr, "gct logging routines: Couldn't close lock file.\n");
      perror("gct logging routines");
    }
  Lock_fid = -1;
  
  /* Note that deleting the file would delete the whole point of locking. */
}


#endif	/* GCT_LOCKING */



		    /* The Routines Themselves */

/*
 * Old logfile entries are only merged if (options readlog), which
 * manifests itself as a call to gct_readlog.  This variable remembers
 * that call.  A logfile is merged in only once; more calls to
 * gct_writelog just overwrite it.
 */

static int Do_merge = 0;



gct_writelog(file)
     char *file;
{
  FILE *oldstream;

  GCT_WRITE_LOCK_OR_RETURN();	/* Failure causes return. */
  
  if (   0 == Do_merge
      || NULL == (oldstream = openlog(file)))
    {
      gct_write_new_log(file);
      Do_merge = 0;
    }
  else if (NULL != checklog(oldstream, file))
    {
      /*
       * Update.  Note the standard trick: unlinking an open file only
       * deletes the directory entry, it doesn't make the contents
       * unavailable.  It's now safe to recreate that file.
       */
      if (-1 == unlink(file))
	{
	  fprintf(stderr, "gct logging routines: Couldn't remove old log.\n");
	  fprintf(stderr, "gct logging routines: The logfile will not be updated.\n");
	  perror("gct logging routines");
	}
      else
	{
	  gct_update_old_log(file, oldstream);
	  Do_merge = 0;
	}
      fclose(oldstream);
    }
  GCT_UNLOCK();
}

/*
 * Note that Do_merge is set even if the openlog() fails.  Suppose 
 * FOO is an executable that forks BAR.  Both are instrumented.  Both use
 * readlog and writelog.  When FOO is first executed, GCTLOG does not
 * exist.  But BAR creates it when it exits.  When FOO then exits, we
 * want it to read in BAR's log.
 *
 * Note that a locking failure will not prevent a later successful call
 * of writelog from reading the logfile and then updating it.
 * (Presumably the user has scurried off and fixed the problem before the
 * program exited.)
 */
gct_readlog(file)
     char *file;
{     
  FILE *stream;

  Do_merge = 1;
  
  GCT_READ_LOCK_OR_RETURN();	/* Failure causes return. */
  
  stream = openlog(file);
  if (NULL == stream)
    {
      GCT_UNLOCK();
      return;
    }

  if (NULL != checklog(stream, file))
    fclose(stream);
  GCT_UNLOCK();
}
