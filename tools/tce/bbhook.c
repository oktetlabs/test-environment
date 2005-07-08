/* Copyright (C) 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
   2000, 2001, 2002  Free Software Foundation, Inc. */
/* Copyright (C) 2005  The authors of the Test Environment */

/* This code is based on libgcc2.c from GCC distribution.
   It uses gcov-related routines from there modifying them to
   use TE logger instead of normal file I/O
*/

/* As a special exception, if you link this library with other files,
   some of which are compiled with GCC, to produce an executable,
   this library does not by itself cause the resulting executable
   to be covered by the GNU General Public License.
   This exception does not however invalidate any other reasons why
   the executable file might be covered by the GNU General Public License.  
*/

#include <te_config.h>
#include <stdio.h>

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#ifdef HAVE_NETINET_TCP_H
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#endif
#include <signal.h>
#include <unistd.h>


#if __GNUC__ > 3 || \
    (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)

typedef unsigned gcov_unsigned_t __attribute__ ((mode (SI)));
typedef unsigned gcov_position_t __attribute__ ((mode (SI)));
typedef signed gcov_type __attribute__ ((mode (DI)));

/* File suffixes.  */
#define GCOV_DATA_SUFFIX ".gcda"
#define GCOV_NOTE_SUFFIX ".gcno"

/* File magic. Must not be palindromes.  */
#define GCOV_DATA_MAGIC ((gcov_unsigned_t)0x67636461) /* "gcda" */
#define GCOV_NOTE_MAGIC ((gcov_unsigned_t)0x67636e6f) /* "gcno" */


/* FIXME: must be calculated like in original libgcov */
#define GCOV_VERSION ((gcov_unsigned_t)0x89abcdef)


/* Convert a magic or version number to a 4 character string.  */
#define GCOV_UNSIGNED2STRING(ARRAY,VALUE)   \
  ((ARRAY)[0] = (char)((VALUE) >> 24),      \
   (ARRAY)[1] = (char)((VALUE) >> 16),      \
   (ARRAY)[2] = (char)((VALUE) >> 8),       \
   (ARRAY)[3] = (char)((VALUE) >> 0))

/* The record tags.  Values [1..3f] are for tags which may be in either
   file.  Values [41..9f] for those in the note file and [a1..ff] for
   the data file.  */

#define GCOV_TAG_FUNCTION    ((gcov_unsigned_t)0x01000000)
#define GCOV_TAG_FUNCTION_LENGTH (2)
#define GCOV_TAG_BLOCKS      ((gcov_unsigned_t)0x01410000)
#define GCOV_TAG_BLOCKS_LENGTH(NUM) (NUM)
#define GCOV_TAG_BLOCKS_NUM(LENGTH) (LENGTH)
#define GCOV_TAG_ARCS        ((gcov_unsigned_t)0x01430000)
#define GCOV_TAG_ARCS_LENGTH(NUM)  (1 + (NUM) * 2)
#define GCOV_TAG_ARCS_NUM(LENGTH)  (((LENGTH) - 1) / 2)
#define GCOV_TAG_LINES       ((gcov_unsigned_t)0x01450000)
#define GCOV_TAG_COUNTER_BASE    ((gcov_unsigned_t)0x01a10000)
#define GCOV_TAG_COUNTER_LENGTH(NUM) ((NUM) * 2)
#define GCOV_TAG_COUNTER_NUM(LENGTH) ((LENGTH) / 2)
#define GCOV_TAG_OBJECT_SUMMARY  ((gcov_unsigned_t)0xa1000000)
#define GCOV_TAG_PROGRAM_SUMMARY ((gcov_unsigned_t)0xa3000000)
#define GCOV_TAG_SUMMARY_LENGTH  \
    (1 + GCOV_COUNTERS_SUMMABLE * (2 + 3 * 2))


/* Counters that are collected.  */
#define GCOV_COUNTER_ARCS   0  /* Arc transitions.  */
#define GCOV_COUNTERS_SUMMABLE  1  /* Counters which can be
                      summaried.  */
#define GCOV_FIRST_VALUE_COUNTER 1 /* The first of counters used for value
                      profiling.  They must form a consecutive
                      interval and their order must match
                      the order of HIST_TYPEs in
                      value-prof.h.  */
#define GCOV_COUNTER_V_INTERVAL 1  /* Histogram of value 
                                      inside an interval.*/
#define GCOV_COUNTER_V_POW2 2  /* Histogram of exact power2 logarithm
                      of a value.  */
#define GCOV_COUNTER_V_SINGLE   3  /* The most common value 
                                      of expression.  */
#define GCOV_COUNTER_V_DELTA    4  /* The most common difference between
                      consecutive values of expression.  */
#define GCOV_LAST_VALUE_COUNTER 4  /* The last of counters used for value
                      profiling.  */
#define GCOV_COUNTERS       5

#define GCOV_LOCKED 0

/* Cumulative counter data.  */
struct gcov_ctr_summary
{
  gcov_unsigned_t num;      /* number of counters.  */
  gcov_unsigned_t runs;     /* number of program runs */
  gcov_type sum_all;        /* sum of all counters accumulated.  */
  gcov_type run_max;        /* maximum value on a single run.  */
  gcov_type sum_max;        /* sum of individual run max values.  */
};

/* Object & program summary record.  */
struct gcov_summary
{
  gcov_unsigned_t checksum; /* checksum of program */
  struct gcov_ctr_summary ctrs[GCOV_COUNTERS_SUMMABLE];
};

/* Structures embedded in coveraged program.  The structures generated
   by write_profile must match these.  */

/* Information about a single function.  This uses the trailing array
   idiom. The number of counters is determined from the counter_mask
   in gcov_info.  We hold an array of function info, so have to
   explicitly calculate the correct array stride.  */
struct gcov_fn_info
{
  gcov_unsigned_t ident;    /* unique ident of function */
  gcov_unsigned_t checksum; /* function checksum */
  unsigned n_ctrs[0];       /* instrumented counters */
};

/* Type of function used to merge counters.  */
typedef void (*gcov_merge_fn) (gcov_type *, gcov_unsigned_t);

/* Information about counters.  */
struct gcov_ctr_info
{
  gcov_unsigned_t num;      /* number of counters.  */
  gcov_type *values;        /* their values.  */
  gcov_merge_fn merge;      /* The function used to merge them.  */
};

/* Information about a single object file.  */
struct gcov_info
{
  gcov_unsigned_t version;  /* expected version number */
  struct gcov_info *next;   /* link to next, used by libgcov */

  gcov_unsigned_t stamp;    /* uniquifying time stamp */
  const char *filename;     /* output file name */
  
  unsigned n_functions;     /* number of functions */
  const struct gcov_fn_info *functions; /* table of functions */

  unsigned ctr_mask;        /* mask of counters instrumented.  */
  struct gcov_ctr_info counts[0]; /* count data. The number of bits
                     set in the ctr_mask field
                     determines how big this array
                     is.  */
};

/* Chain of per-object gcov structures.  */
struct gcov_info *__gcov_list;

/* A program checksum allows us to distinguish program data for an
   object file included in multiple programs.  */
gcov_unsigned_t __gcov_crc32;

#else 

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
    long long *counts;
    long ncounts;
    struct bb *next;
    
    /* Older GCC's did not emit these fields.  */
    long sizeof_bb;
    struct bb_function_info *function_infos;
};

/* Chain of per-object file bb structures.  */
struct bb *__bb_head;

#endif


enum CONNECT_MODES { CONNECT_FIFO, CONNECT_UNIX, CONNECT_TCP };

static enum CONNECT_MODES connect_mode;
static int peer_id;
static char collector_path[PATH_MAX + 1];
static struct in_addr tcp_address;
static int tcp_port;

void
__bb_init_connection(const char *mode, int peer)
{
    if (strncmp(mode, "fifo:", 5) == 0)
    {
        connect_mode = CONNECT_FIFO;
        strcpy(collector_path, mode + 5);
    }
#ifdef HAVE_SYS_SOCKET_H
#ifdef HAVE_SYS_UN_H
    else if (strncmp(mode, "unix:", 5) == 0)
    {
        connect_mode = CONNECT_UNIX;
        strcpy(collector_path, mode + 5);
    }
#endif
#ifdef HAVE_NETINET_TCP_H
    else if (strncmp(mode, "tcp:", 4) == 0)
    {
        char *tmp;
        tcp_port = htons(strtoul(mode + 4, &tmp, 10));
                
        if (*tmp == '\0')
        {
            tcp_address.s_addr = 0;
        }
        else
        {
            inet_pton(AF_INET, tmp + 1, &tcp_address);
        }
    }
#endif
#endif /* HAVE_SYS_SOCKET_H */
    peer_id = peer;
}


/* Dump the coverage counts. We merge with existing counts when
   possible, to avoid growing the .da files ad infinitum.  */
#if __GNUC__ > 3 || \
    (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)

static void
__gcov_exit (void)
{
  struct gcov_info *gi_ptr;
  struct gcov_summary this_program;
  struct gcov_summary all;
  struct gcov_ctr_summary *cs_ptr;
  const struct gcov_ctr_info *ci_ptr;
  unsigned t_ix;
  gcov_unsigned_t c_num;

  memset (&all, 0, sizeof (all));
  /* Find the totals for this execution.  */
  memset (&this_program, 0, sizeof (this_program));
  for (gi_ptr = __gcov_list; gi_ptr; gi_ptr = gi_ptr->next)
    {
      ci_ptr = gi_ptr->counts;
      for (t_ix = 0; t_ix < GCOV_COUNTERS_SUMMABLE; t_ix++)
    {
      if (!((1 << t_ix) & gi_ptr->ctr_mask))
        continue;

      cs_ptr = &this_program.ctrs[t_ix];
      cs_ptr->num += ci_ptr->num;
      for (c_num = 0; c_num < ci_ptr->num; c_num++)
        {
              cs_ptr->sum_all += ci_ptr->values[c_num];
          if (cs_ptr->run_max < ci_ptr->values[c_num])
        cs_ptr->run_max = ci_ptr->values[c_num];
        }
      ci_ptr++;
    }
    }

  /* Now merge each file.  */
  for (gi_ptr = gcov_list; gi_ptr; gi_ptr = gi_ptr->next)
  {
      struct gcov_summary this_object;
      struct gcov_summary object, program;
      gcov_type *values[GCOV_COUNTERS];
      const struct gcov_fn_info *fi_ptr;
      unsigned fi_stride;
      unsigned c_ix, f_ix, n_counts;
      struct gcov_ctr_summary *cs_obj, *cs_tobj, *cs_prg, *cs_tprg, *cs_all;
      int error = 0;
      gcov_unsigned_t tag, length;
      gcov_position_t summary_pos = 0;

      memset (&this_object, 0, sizeof (this_object));
      memset (&object, 0, sizeof (object));
      
      /* Totals for this object file.  */
      ci_ptr = gi_ptr->counts;
      for (t_ix = 0; t_ix < GCOV_COUNTERS_SUMMABLE; t_ix++)
      {
          if (!((1 << t_ix) & gi_ptr->ctr_mask))
              continue;
          
          cs_ptr = &this_object.ctrs[t_ix];
          cs_ptr->num += ci_ptr->num;
          for (c_num = 0; c_num < ci_ptr->num; c_num++)
          {
              cs_ptr->sum_all += ci_ptr->values[c_num];
              if (cs_ptr->run_max < ci_ptr->values[c_num])
                  cs_ptr->run_max = ci_ptr->values[c_num];
          }
          
          ci_ptr++;
      }

      c_ix = 0;
      for (t_ix = 0; t_ix < GCOV_COUNTERS; t_ix++)
      {
          if ((1 << t_ix) & gi_ptr->ctr_mask)
          {
              values[c_ix] = gi_ptr->counts[c_ix].values;
              c_ix++;
          }
      }
      
      /* Calculate the function_info stride. This depends on the
         number of counter types being measured.  */
      fi_stride = sizeof (struct gcov_fn_info) + c_ix * sizeof (unsigned);
      if (__alignof__ (struct gcov_fn_info) > sizeof (unsigned))
      {
          fi_stride += __alignof__ (struct gcov_fn_info) - 1;
          fi_stride &= ~(__alignof__ (struct gcov_fn_info) - 1);
      }
      
      if (!gcov_open (gi_ptr->filename))
      {
          fprintf (stderr, "profiling:%s:Cannot open\n", gi_ptr->filename);
          continue;
      }
      
      tag = gcov_read_unsigned ();
      if (tag)
      {
          /* Merge data from file.  */
          if (tag != GCOV_DATA_MAGIC)
          {
              fprintf (stderr, "profiling:%s:Not a gcov data file\n",
                       gi_ptr->filename);
          read_fatal:;
              gcov_close ();
              continue;
          }
          length = gcov_read_unsigned ();
          if (!gcov_version (gi_ptr, length))
              goto read_fatal;
          
          length = gcov_read_unsigned ();
          if (length != gi_ptr->stamp)
          {
              /* Read from a different compilation. Overwrite the
                 file.  */
          gcov_truncate ();
          goto rewrite;
        }
      
      /* Merge execution counts for each function.  */
      for (f_ix = 0; f_ix < gi_ptr->n_functions; f_ix++)
        {
          fi_ptr = (const struct gcov_fn_info *)
              ((const char *) gi_ptr->functions + f_ix * fi_stride);
          tag = gcov_read_unsigned ();
          length = gcov_read_unsigned ();

          /* Check function.  */
          if (tag != GCOV_TAG_FUNCTION
          || length != GCOV_TAG_FUNCTION_LENGTH
          || gcov_read_unsigned () != fi_ptr->ident
          || gcov_read_unsigned () != fi_ptr->checksum)
        {
        read_mismatch:;
          fprintf (stderr, "profiling:%s:Merge mismatch for %s\n",
               gi_ptr->filename,
               f_ix + 1 ? "function" : "summaries");
          goto read_fatal;
        }

          c_ix = 0;
          for (t_ix = 0; t_ix < GCOV_COUNTERS; t_ix++)
        {
          gcov_merge_fn merge;

          if (!((1 << t_ix) & gi_ptr->ctr_mask))
            continue;
          
          n_counts = fi_ptr->n_ctrs[c_ix];
          merge = gi_ptr->counts[c_ix].merge;
            
          tag = gcov_read_unsigned ();
          length = gcov_read_unsigned ();
          if (tag != GCOV_TAG_FOR_COUNTER (t_ix)
              || length != GCOV_TAG_COUNTER_LENGTH (n_counts))
            goto read_mismatch;
          (*merge) (values[c_ix], n_counts);
          values[c_ix] += n_counts;
          c_ix++;
        }
          if ((error = gcov_is_error ()))
        goto read_error;
        }

      f_ix = ~0u;
      /* Check program & object summary */
      while (1)
        {
          gcov_position_t base = gcov_position ();
          int is_program;
          
          tag = gcov_read_unsigned ();
          if (!tag)
        break;
          length = gcov_read_unsigned ();
          is_program = tag == GCOV_TAG_PROGRAM_SUMMARY;
          if (length != GCOV_TAG_SUMMARY_LENGTH
          || (!is_program && tag != GCOV_TAG_OBJECT_SUMMARY))
        goto read_mismatch;
          gcov_read_summary (is_program ? &program : &object);
          if ((error = gcov_is_error ()))
        goto read_error;
          if (is_program && program.checksum == gcov_crc32)
        {
          summary_pos = base;
          goto rewrite;
        }
        }
    }
      
      if (!gcov_is_eof ())
    {
    read_error:;
      fprintf (stderr, error < 0 ? "profiling:%s:Overflow merging\n"
           : "profiling:%s:Error merging\n", gi_ptr->filename);
      goto read_fatal;
    }
    rewrite:;
      gcov_rewrite ();
      if (!summary_pos)
    memset (&program, 0, sizeof (program));

      /* Merge the summaries.  */
      f_ix = ~0u;
      for (t_ix = 0; t_ix < GCOV_COUNTERS_SUMMABLE; t_ix++)
    {
      cs_obj = &object.ctrs[t_ix];
      cs_tobj = &this_object.ctrs[t_ix];
      cs_prg = &program.ctrs[t_ix];
      cs_tprg = &this_program.ctrs[t_ix];
      cs_all = &all.ctrs[t_ix];

      if ((1 << t_ix) & gi_ptr->ctr_mask)
        {
          if (!cs_obj->runs++)
        cs_obj->num = cs_tobj->num;
          else if (cs_obj->num != cs_tobj->num)
        goto read_mismatch;
          cs_obj->sum_all += cs_tobj->sum_all;
          if (cs_obj->run_max < cs_tobj->run_max)
        cs_obj->run_max = cs_tobj->run_max;
          cs_obj->sum_max += cs_tobj->run_max;
          
          if (!cs_prg->runs++)
        cs_prg->num = cs_tprg->num;
          else if (cs_prg->num != cs_tprg->num)
        goto read_mismatch;
          cs_prg->sum_all += cs_tprg->sum_all;
          if (cs_prg->run_max < cs_tprg->run_max)
        cs_prg->run_max = cs_tprg->run_max;
          cs_prg->sum_max += cs_tprg->run_max;
        }
      else if (cs_obj->num || cs_prg->num)
        goto read_mismatch;
      
      if (!cs_all->runs && cs_prg->runs)
        memcpy (cs_all, cs_prg, sizeof (*cs_all));
      else if (!all.checksum
           && (!GCOV_LOCKED || cs_all->runs == cs_prg->runs)
           && memcmp (cs_all, cs_prg, sizeof (*cs_all)))
        {
          fprintf (stderr, "profiling:%s:Invocation mismatch - "
                   "some data files may have been removed%s",
                   gi_ptr->filename, GCOV_LOCKED
                   ? "" : " or concurrent update without locking support");
          all.checksum = ~0u;
        }
    }
      
      c_ix = 0;
      for (t_ix = 0; t_ix < GCOV_COUNTERS; t_ix++)
    if ((1 << t_ix) & gi_ptr->ctr_mask)
      {
        values[c_ix] = gi_ptr->counts[c_ix].values;
        c_ix++;
      }

      program.checksum = gcov_crc32;
      
      /* Write out the data.  */
      gcov_write_tag_length (GCOV_DATA_MAGIC, GCOV_VERSION);
      gcov_write_unsigned (gi_ptr->stamp);
      
      /* Write execution counts for each function.  */
      for (f_ix = 0; f_ix < gi_ptr->n_functions; f_ix++)
    {
      fi_ptr = (const struct gcov_fn_info *)
          ((const char *) gi_ptr->functions + f_ix * fi_stride);

      /* Announce function.  */
      gcov_write_tag_length (GCOV_TAG_FUNCTION, GCOV_TAG_FUNCTION_LENGTH);
      gcov_write_unsigned (fi_ptr->ident);
      gcov_write_unsigned (fi_ptr->checksum);

      c_ix = 0;
      for (t_ix = 0; t_ix < GCOV_COUNTERS; t_ix++)
        {
          gcov_type *c_ptr;

          if (!((1 << t_ix) & gi_ptr->ctr_mask))
        continue;

          n_counts = fi_ptr->n_ctrs[c_ix];
            
          gcov_write_tag_length (GCOV_TAG_FOR_COUNTER (t_ix),
                     GCOV_TAG_COUNTER_LENGTH (n_counts));
          c_ptr = values[c_ix];
          while (n_counts--)
        gcov_write_counter (*c_ptr++);

          values[c_ix] = c_ptr;
          c_ix++;
        }
    }

      /* Object file summary.  */
      gcov_write_summary (GCOV_TAG_OBJECT_SUMMARY, &object);

      /* Generate whole program statistics.  */
      gcov_seek (summary_pos);
      gcov_write_summary (GCOV_TAG_PROGRAM_SUMMARY, &program);
      if ((error = gcov_close ()))
      fprintf (stderr, error  < 0 ?
           "profiling:%s:Overflow writing\n" :
           "profiling:%s:Error writing\n",
           gi_ptr->filename);
    }
}


#else

void
__bb_exit_func (void)
{
    struct bb *ptr;
    int i;
    long long program_sum = 0;
    long long program_max = 0;
    long program_arcs = 0;
    int fd;
    static char buffer[PATH_MAX + 64];

    if (peer_id == 0) /* __bb_init_function has not been called */
    {
        const char *env = getenv("TCE_CONNECTION");
        char name[PATH_MAX + 1];
        char *space_at;
        int peer_id;
        if (env == NULL)
            return;
        strncpy(name, env, sizeof(name) - 1);
        space_at = strchr(name, ' ');
        if (space_at == NULL ||
            (peer_id = strtol(space_at, NULL, 0)) == 0)
        {
            fprintf(stderr, "invalid TCE_CONNECTION var '%s'\n", env);
            return;
        }
        *space_at = '\0';
        __bb_init_connection(name, peer_id);
    }

    signal(SIGPIPE, SIG_IGN);
    errno = 0;

    switch (connect_mode)
    {
#ifdef HAVE_NETINET_TCP_H
        case CONNECT_TCP:
        {
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_addr = tcp_address;
            addr.sin_port = tcp_port;
            fd = socket(PF_INET, SOCK_STREAM, 0);
            connect(fd, (struct sockaddr *)&addr, sizeof(addr));
            break;
        }
#endif
#ifdef HAVE_SYS_UN_H
        case CONNECT_UNIX:
        {
            struct sockaddr_un addr;
            addr.sun_family = AF_UNIX;
            strcpy(addr.sun_path, collector_path);
            fd = socket(PF_UNIX, SOCK_STREAM, 0);
            connect(fd, (struct sockaddr *)&addr, sizeof(addr));
            break;
        }
#endif
        case CONNECT_FIFO:
        {
            fd = open(collector_path, O_WRONLY);
            break;
        }
        default:
        {
            fputs("internal error: invalid connection mode\n", stderr);
            return;
        }
    }
    
    if (errno)
    {
        fprintf(stderr, "cannot connect to TCE collector: %s\n", 
                strerror(errno));
        return;
    }

    sprintf(buffer, "%d\n", peer_id);
    write(fd, buffer, strlen(buffer)); 
    
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
        long long object_max = 0;
        long long object_sum = 0;
        long object_functions = 0;
        int error = 0;
        struct bb_function_info *fn_info;
        long long *count_ptr;
        
        if (!ptr->filename)
            continue;
        
        for (fn_info = ptr->function_infos; 
             fn_info->arc_count != -1; 
             fn_info++)
        {
            object_functions++;
        }
        
        /* Calculate the per-object statistics.  */
        for (i = 0; i < ptr->ncounts; i++)
        {
            object_sum += ptr->counts[i];
            
            if (ptr->counts[i] > object_max)
                object_max = ptr->counts[i];
        }

        sprintf(buffer, "%s %d %d %Ld %Ld %d %Ld %Ld\n", 
                ptr->filename,
                object_functions, 
                program_arcs,
                program_sum, 
                program_max, 
                ptr->ncounts, 
                object_sum,
                object_max);
        write(fd, buffer, strlen(buffer));
      
        /* Write execution counts for each function.  */
        count_ptr = ptr->counts;
        
        for (fn_info = ptr->function_infos; fn_info->arc_count >= 0;
             fn_info++)
        {          
            sprintf(buffer, "*%s %d %d\n", fn_info->name,
                    fn_info->checksum, fn_info->arc_count);
            write(fd, buffer, strlen(buffer));
            for (i = fn_info->arc_count; i > 0; i--, count_ptr++)
            {
                sprintf(buffer, "+%Ld\n", *count_ptr);
                write(fd, buffer, strlen(buffer));
            }
        }
    }
    write(fd, "end\n", 4);
    close(fd);
}

#endif

/* Add a new object file onto the bb chain.  Invoked automatically
   when running an object file's global ctors.  */


#if __GNUC__ > 3 || \
    (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)

void
__gcov_init (struct gcov_info *info)
{
    if (!info->version)
        return;
    if (gcov_version (info, info->version))
    {
        const char *ptr = info->filename;
        gcov_unsigned_t crc32 = __gcov_crc32;
        
        do
        {
            unsigned ix;
            gcov_unsigned_t value = *ptr << 24;
            
            for (ix = 8; ix--; value <<= 1)
            {
                gcov_unsigned_t feedback;
                
                feedback = (value ^ crc32) & 0x80000000 ? 0x04c11db7 : 0;
                crc32 <<= 1;
                crc32 ^= feedback;
            }
        }
        while (*ptr++);
        
        __gcov_crc32 = crc32;
        
        if (!__gcov_list)
            atexit (__gcov_exit);
        
        info->next = __gcov_list;
        __gcov_list = info;
    }
    info->version = 0;
}

#else

void
__bb_init_func (struct bb *blocks)
{
    if (blocks->zero_word)
        return;

    /* Initialize destructor and per-thread data.  */
    if (!__bb_head)
    {
        atexit (__bb_exit_func);
    }

    /* Set up linked list.  */
    blocks->zero_word = 1;
    blocks->next = __bb_head;
    __bb_head = blocks;
}

#endif

/* Called before fork or exec - reset gathered coverage info to zero.  
   This avoids duplication or loss of the
   profile information gathered so far.  */

#if __GNUC__ > 3 || \
    (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)

void
__gcov_flush (void)
{
  const struct gcov_info *gi_ptr;

  __gcov_exit ();
  for (gi_ptr = __gcov_list; gi_ptr; gi_ptr = gi_ptr->next)
    {
      unsigned t_ix;
      const struct gcov_ctr_info *ci_ptr;
      
      for (t_ix = 0, ci_ptr = gi_ptr->counts; t_ix != GCOV_COUNTERS; t_ix++)
    if ((1 << t_ix) & gi_ptr->ctr_mask)
      {
        memset (ci_ptr->values, 0, sizeof (gcov_type) * ci_ptr->num);
        ci_ptr++;
      }
    }
}


#else

void
__bb_fork_func (void)
{
    struct bb *ptr;

    for (ptr = __bb_head; ptr != NULL; ptr = ptr->next)
    {
        long i;
        for (i = ptr->ncounts - 1; i >= 0; i--)
            ptr->counts[i] = 0;
    }
}

#endif

void _target_init(void) __attribute__ ((weak));
void _target_fini(void) __attribute__ ((weak));


void
_target_init(void) 
{
} 

void
_target_fini(void) 
{
} 

static void target_init_caller(void) __attribute__ ((constructor));
static void target_fini_caller(void) __attribute__ ((destructor));

static void
target_init_caller (void) 
{
    _target_init();
} 

static void
target_fini_caller (void) 
{
    _target_fini();
}
