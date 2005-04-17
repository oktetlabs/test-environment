/* Copyright (C) 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
   2000, 2001, 2002  Free Software Foundation, Inc. */

/* As a special exception, if you link this library with other files,
   some of which are compiled with GCC, to produce an executable,
   this library does not by itself cause the resulting executable
   to be covered by the GNU General Public License.
   This exception does not however invalidate any other reasons why
   the executable file might be covered by the GNU General Public License.  */

#include <stdio.h>

#include "gcov-io.h"
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

struct bb_function_info 
{
    long checksum;
    int arc_count;
    const char *name;
};

/* Structure emitted by --profile-arcs  */
struct bb
{
    long zero_word;
    const char *filename;
    gcov_type *counts;
    long ncounts;
    struct bb *next;
    
    /* Older GCC's did not emit these fields.  */
    long sizeof_bb;
    struct bb_function_info *function_infos;
};


static const char *make_coverage_path(const char *name)
{
    static char path_buf[PATH_MAX + 1];
    const char *te_tmp = getenv("TE_TMP");
    const char *next_slash, *iter;
    
    if (!te_tmp)
        return name;
    memset(path_buf, 0, sizeof(path_buf));
    strcpy(path_buf, te_tmp);
    strcat(path_buf, "/coverage/");
    mkdir(path_buf);
    for (iter = name + (*name == '/'), next_slash = strchr(iter, '/'); next_slash != NULL;
         iter = next_slash + 1, next_slash = strchr(iter, '/'))
    {
        strncat(path_buf, iter, next_slash - iter);
        mkdir(path_buf);
        strcat(path_buf, "/");
    }
    strcat(path_buf, iter);
    return path_buf;
}


/* Chain of per-object file bb structures.  */
struct bb *__bb_head;

/* Dump the coverage counts. We merge with existing counts when
   possible, to avoid growing the .da files ad infinitum.  */

void
__bb_exit_func (void)
{
    struct bb *ptr;
    int i;
    gcov_type program_sum = 0;
    gcov_type program_max = 0;
    long program_arcs = 0;
    gcov_type merged_sum = 0;
    gcov_type merged_max = 0;
    long merged_arcs = 0;
    const char *da_filename;

  
#if 1
  struct flock s_flock;

  s_flock.l_type = F_WRLCK;
  s_flock.l_whence = SEEK_SET;
  s_flock.l_start = 0;
  s_flock.l_len = 0; /* Until EOF.  */
  s_flock.l_pid = getpid ();
#endif

  /* Non-merged stats for this program.  */
  for (ptr = __bb_head; ptr; ptr = ptr->next)
  {
      for (i = 0; i < ptr->ncounts; i++)
      {
          program_sum += ptr->counts[i];

          if (ptr->counts[i] > program_max)
              program_max = ptr->counts[i];
      }
      program_arcs += ptr->ncounts;
  }
  
  for (ptr = __bb_head; ptr; ptr = ptr->next)
  {
      FILE *da_file;
      gcov_type object_max = 0;
      gcov_type object_sum = 0;
      long object_functions = 0;
      int error = 0;
      struct bb_function_info *fn_info;
      gcov_type *count_ptr;
      
      if (!ptr->filename)
          continue;
      
      da_filename = make_coverage_path(ptr->filename);
      /* Try for appending */
      da_file = fopen (da_filename, "ab");
      /* Some old systems might not allow the 'b' mode modifier.
         Therefore, try to open without it.  This can lead to a
         race condition so that when you delete and re-create the
         file, the file might be opened in text mode, but then,
         you shouldn't delete the file in the first place.  */
      if (!da_file)
          da_file = fopen (da_filename, "a");
      
      if (!da_file)
      {
          fprintf (stderr, "arc profiling: Can't open output file %s.\n",
                   da_filename);
          ptr->filename = 0;
          continue;
      }

#if 1
      /* After a fork, another process might try to read and/or write
         the same file simultanously.  So if we can, lock the file to
         avoid race conditions.  */
      while (fcntl (fileno (da_file), F_SETLKW, &s_flock)
             && errno == EINTR)
          continue;
#endif
      for (fn_info = ptr->function_infos; fn_info->arc_count != -1; fn_info++)
          object_functions++;

      /* Calculate the per-object statistics.  */
      for (i = 0; i < ptr->ncounts; i++)
      {
          object_sum += ptr->counts[i];

          if (ptr->counts[i] > object_max)
              object_max = ptr->counts[i];
      }
      merged_sum += object_sum;
      if (merged_max < object_max)
          merged_max = object_max;
      merged_arcs += ptr->ncounts;
      
      /* Write out the data.  */
      if (/* magic */
          __write_long (-123, da_file, 4)
          /* number of functions in object file.  */
          || __write_long (object_functions, da_file, 4)
          /* length of extra data in bytes.  */
          || __write_long ((4 + 8 + 8) + (4 + 8 + 8), da_file, 4)

          /* whole program statistics. If merging write per-object
             now, rewrite later */
          /* number of instrumented arcs.  */
          || __write_long (program_arcs, da_file, 4)
          /* sum of counters.  */
          || __write_gcov_type (program_sum, da_file, 8)
          /* maximal counter.  */
          || __write_gcov_type (program_max, da_file, 8)

          /* per-object statistics.  */
          /* number of counters.  */
          || __write_long (ptr->ncounts, da_file, 4)
          /* sum of counters.  */
          || __write_gcov_type (object_sum, da_file, 8)
          /* maximal counter.  */
          || __write_gcov_type (object_max, da_file, 8))
      {
          fprintf (stderr, "arc profiling: Error writing output file %s.\n",
                   da_filename);
      }
      else
      {
          /* Write execution counts for each function.  */
          count_ptr = ptr->counts;

          for (fn_info = ptr->function_infos; fn_info->arc_count != -1;
               fn_info++)
          {
              if (__write_gcov_string (fn_info->name,
                                       strlen (fn_info->name), da_file, -1)
                  || __write_long (fn_info->checksum, da_file, 4)
                  || __write_long (fn_info->arc_count, da_file, 4))
              {
                  fprintf (stderr, "arc profiling: Error writing output file %s.\n",
                           da_filename);

              }
              else
              {
                  for (i = fn_info->arc_count; i > 0; i--, count_ptr++)
                  {
                      if (__write_gcov_type (*count_ptr, da_file, 8))
                          fprintf (stderr, "arc profiling: Error writing output file %s.\n",
                                   da_filename);
                  }
              }
          }
      }

      if (fclose (da_file))
      {
          fprintf (stderr, "arc profiling: Error closing output file %s.\n",
                   da_filename);
      }
  }


}

/* Add a new object file onto the bb chain.  Invoked automatically
   when running an object file's global ctors.  */

void
__bb_init_func (struct bb *blocks)
{
    if (blocks->zero_word)
        return;

    /* Initialize destructor and per-thread data.  */
    if (!__bb_head)
        atexit (__bb_exit_func);

    /* Set up linked list.  */
    blocks->zero_word = 1;
    blocks->next = __bb_head;
    __bb_head = blocks;
}

/* Called before fork or exec - write out profile information gathered so
   far and reset it to zero.  This avoids duplication or loss of the
   profile information gathered so far.  */

/* ??? Will this be called multiple times or not ? */

void
__bb_fork_func (void)
{
    struct bb *ptr;

    __bb_exit_func ();
    for (ptr = __bb_head; ptr != NULL; ptr = ptr->next)
    {
        long i;
        for (i = ptr->ncounts - 1; i >= 0; i--)
            ptr->counts[i] = 0;
    }
}

