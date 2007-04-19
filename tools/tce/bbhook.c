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

#define _ISOC99_SOURCE
#include <stdio.h>

#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>


#if __GNUC__ > 3 || \
    (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define GCC_IS_3_4P 1
#define INIT_GCOV(arg) void __gcov_init (struct gcov_info *arg)
#define EXIT_GCOV() void __gcov_exit (void)
#define FLUSH_GCOV() void __gcov_flush (void)
#else
#undef  GCC_IS_3_4P
#define INIT_GCOV(arg) void __bb_init_func (struct bb *arg)
#define EXIT_GCOV() void __bb_exit_func (void)
#define FLUSH_GCOV() void __bb_fork_func (void)
#endif

#ifdef GCC_IS_3_4P
typedef unsigned gcov_unsigned_t;
typedef unsigned gcov_position_t;
typedef signed long long gcov_type;

/* File suffixes.  */
#define GCOV_DATA_SUFFIX ".gcda"
#define GCOV_NOTE_SUFFIX ".gcno"

/* File magic. Must not be palindromes.  */
#define GCOV_DATA_MAGIC ((gcov_unsigned_t)0x67636461) /* "gcda" */
#define GCOV_NOTE_MAGIC ((gcov_unsigned_t)0x67636e6f) /* "gcno" */


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

void __gcov_merge_add (__attribute__ ((unused)) gcov_type *counters, 
                       __attribute__ ((unused)) unsigned n_counters)
{
}

void __gcov_merge_single (__attribute__ ((unused)) gcov_type *counters, 
                          __attribute__ ((unused)) unsigned n_counters)
{
}

void __gcov_merge_delta (__attribute__ ((unused)) gcov_type *counters, 
                         __attribute__ ((unused)) unsigned n_counters)
{
}

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

#endif


static int gcov_program_id = 0;

/* Add a new object file onto the bb chain.  Invoked automatically
   when running an object file's global ctors.  */

#define SYS_TCE_PREFIX "/sys/tce/"

static int
open_tce_info(int objno, const char *functr, int functrno, int arcno,
              const char *attrname, int mode)
{
    char path[32] = SYS_TCE_PREFIX;
    int pos = strlen(path);
    int size = sizeof(path) - pos - 1;
    char *ptr = path + pos;
#define ADJPOS \ 
    if (pos < 0 || pos > size) return -1; else ptr += pos, size -= pos

    pos = snprintf(ptr, size, "%d/", gcov_program_id);
    ADJPOS;

    if (objno >= 0)
    {
        pos = snprintf(ptr, size, "%d/", objno);
        ADJPOS;
        if (functr != NULL)
        {
            pos = strlen(functr);
            if (pos > size) 
                return -1;
            memcpy(ptr, functr, pos);
            ptr += pos;
            size -= pos;
            
            pos = snprintf(ptr, size, "%d/", functrno);
            ADJPOS;
            if (arcno >= 0)
            {
                pos = snprintf(ptr, size, "%d/", functrno);
                ADJPOS;
            }
        }
    }
    strncpy(ptr, attrname, size);

#undef ADJPOS
    
    return open(path, mode);
}


static void
write_tce_info(int objno, const char *functr, int functrno, int arcno, 
               const char *attrname, const char *fmt, ...)
{
    va_list args;
    int fd;
    char buffer[128];
    
    va_start(args, fmt);
    fd = open_tce_info(objno, functr, functrno, arcno, 
                       attrname, O_WRONLY);
    if (fd >= 0)
    {
        int len;
        len = vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
        if (len > 0)
        {
            write(fd, buffer, len > (int)sizeof(buffer) - 1 ? 
                  (int)sizeof(buffer) - 1 : len);
        }
        close(fd);
    }
    va_end(args);
}

static int
read_tce_info(int objno, const char *functr, int functrno, int arcno, 
              const char *attrname, const char *fmt, ...)
{
    va_list args;
    int fd;
    int n_fields = 0;
    char buffer[128];
    
    va_start(args, fmt);
    fd = open_tce_info(objno, functr, functrno, arcno, 
                       attrname, O_RDONLY);
    if (fd >= 0)
    {
        int len;
        
        len = read(fd, buffer, sizeof(buffer) - 1);
        if (len > 0)
        {
            buffer[len] = '\0';
            n_fields = vsscanf(buffer, fmt, args);
        }
        close(fd);
    }
    return n_fields;
}

#define TCE_GLOBAL -1, NULL, -1, -1
#define TCE_OBJ(_objno) _objno, NULL, -1, -1
#define TCE_FUN(_objno, _funno) _objno, "fun", _funno, -1
#define TCE_CTR(_objno, _ctrno) _objno, "ctr", _ctrno, -1
#define TCE_ARC(_objno, _funno, _arcno) _objno, "fun", _funno, _arcno

static void
get_program_number(void)
{
    if (gcov_program_id == 0)
    {
        read_tce_info(TCE_GLOBAL, "seq", "%d", &gcov_program_id);
        if (gcov_program_id != 0)
        {
            write_tce_info(TCE_GLOBAL, "version", "%s",  __VERSION__);
        }
    }
}

#ifdef GCC_IS_3_4P

void
__gcov_init (struct gcov_info *info)
{
    if (!info->version)
        return;
    else
    {
        int objno = -1;

        get_program_number();
        read_tce_info(TCE_GLOBAL, "next", "%d", -1, &objno);
        if (objno >= 0)
        {
            unsigned i;
            unsigned fi_stride;
            int actual_counters = 0;
            const struct gcov_fn_info *fi_ptr;

            write_tce_info(TCE_OBJ(objno), "filename", "%s", 
                           info->filename);
            write_tce_info(TCE_OBJ(objno), "stamp", "%x", info->stamp);
            write_tce_info(TCE_OBJ(objno), "ctr_mask", "%x", 
                           info->ctr_mask);
            write_tce_info(TCE_OBJ(objno), "n_functions", "%u", 
                           info->n_functions);

            for (i = 0; i < GCOV_COUNTERS; i++)
            {
                int cnno = -1;
                
                if ((info->ctr_mask & (1 << i)) != 0)
                {
                    read_tce_info(TCE_OBJ(objno), "next_cn", "%d", &cnno);
                    
                    if (cnno >= 0)
                    {
                        write_tce_info(TCE_CTR(objno, cnno), "n_counters",
                                       "%u", info->counts[i].num);
                        write_tce_info(TCE_CTR(objno, cnno), 
                                       "merger", "%d",
                                       info->counts[i].merge == 
                                       __gcov_merge_add ? 0 :
                                       (info->counts[i].merge == 
                                        __gcov_merge_single ? 1 :
                                        (info->counts[i].merge == 
                                         __gcov_merge_delta ? 2 : -1)));
                        memset(info->counts[i].values, 0, 
                               sizeof(*info->counts[i].values) * 
                               info->counts[i].num);
                        write_tce_info(TCE_CTR(objno, cnno), "data", "%p", 
                                       info->counts[i].values);
                        actual_counters++;
                    }
                }
            }

            fi_stride = sizeof (struct gcov_fn_info) + actual_counters * 
                sizeof (unsigned);
            if (__alignof__ (struct gcov_fn_info) > sizeof (unsigned))
            {
                fi_stride += __alignof__ (struct gcov_fn_info) - 1;
                fi_stride &= ~(__alignof__ (struct gcov_fn_info) - 1);
            }
      

            for (i = 0, fi_ptr = info->functions; 
                 i < info->n_functions; 
                 i++, fi_ptr = (const struct gcov_fn_info *)
                     ((const char *)fi_ptr + fi_stride))
            {
                int fnno = -1;
                int j;
                int cn;
                
                read_tce_info(TCE_OBJ(objno), "next_fn", "%d", &fnno);

                if (fnno >= 0)
                {
                    write_tce_info(TCE_FUN(objno, fnno), "ident", "%x",
                                   fi_ptr->ident);
                    write_tce_info(TCE_FUN(objno, fnno), "checksum", "%x", 
                                   fi_ptr->checksum);
                    
                    for (j = 0, cn = 0; j < GCOV_COUNTERS; j++)
                    {
                        int arcno = -1;

                        if ((info->ctr_mask & (1 << j)) != 0)
                        {
                            read_tce_info(TCE_FUN(objno, fnno), "next", 
                                          "%d", &arcno);
                            if (arcno >= 0)
                            {
                                write_tce_info(TCE_ARC(objno, fnno, arcno), 
                                               "count", "%u", 
                                               fi_ptr->n_ctrs[cn++]);
                            }
                        }
                    }
                }
            }

        }
    }
    info->version = 0;
}

#else

void
__bb_init_func (struct bb *blocks)
{
    int objno = -1;

    if (blocks->zero_word)
        return;

    get_program_number();

    read_tce_info(TCE_GLOBAL, "next", "%d", -1, &objno);
    if (objno >= 0)
    {
        unsigned i;
        unsigned n_funcs = 0;
        int cnno = -1;
        
        write_tce_info(TCE_OBJ(objno), "filename", "%s", blocks->filename);
        write_tce_info(TCE_OBJ(objno), "ctr_mask", "%x", 1);

        for (n_funcs = 0; 
             blocks->function_infos[n_funcs].arc_count >= 0; 
             n_funcs++)
            ;

        write_tce_info(TCE_OBJ(objno), "n_functions", "%u", n_funcs);
        
        for (i = 0; i < n_funcs; i++)
        {
            int fnno = -1;

            read_tce_info(TCE_OBJ(objno), "next_fn", "%d", &fnno);
            
            if (fnno >= 0)
            {
                int arcno = -1;

                write_tce_info(TCE_FUN(objno, fnno), "name", "%s",
                               blocks->function_infos[i].name);
                    
                read_tce_info(TCE_FUN(objno, fnno), "next", "%d", &arcno);
                if (arcno >= 0)
                {
                    write_tce_info(TCE_ARC(objno, fnno, arcno), 
                                   "count", "%u", 
                                   blocks->function_infos[i].arc_count);
                }
            }
        }

        read_tce_info(TCE_OBJ(objno), "next_cn", "%d", &cnno);

        if (cnno >= 0)
        {
            write_tce_info(TCE_CTR(objno, cnno), "n_counters", "%u", 
                           blocks->ncounts);
            write_tce_info(TCE_CTR(objno, cnno), "merger", "%d", 0);
            memset(blocks->counts, 0, sizeof(*blocks->counts) * 
                   blocks->ncounts);
            write_tce_info(TCE_CTR(objno, cnno), "data", "%p", 
                           blocks->counts);
        }
        
    }
    
    /* Set up linked list.  */
    blocks->zero_word = 1;
}

#endif

/* We do not need to clear data before fork or exec, 
 * because, unlike the standard libgcov, there is no
 * possibility the data get garbled by two concurrent
 * processes.
 *
 * However, currently a fork()'ed process will not participate
 * in TCE gathering (until exec*() is called, of course, and
 * a new TCE initialization is performed)
 */

#ifdef GCC_IS_3_4P

void __gcov_flush (void)
{
}


#else

void __bb_fork_func (void)
{
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
