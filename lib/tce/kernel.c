/** @file
 * @brief Tester Subsystem
 *
 * Kernel coverage stuff
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */

#include <te_config.h>
#include <te_defs.h>
#include "package.h"

#include <limits.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <te_errno.h>
#define IN_TCE_COLLECTOR 1
#include <tce_collector.h>
#include "tce_internal.h"

/* The following are data structures for GCC 3.3.x and earlier */

struct bb_raw_function_info 
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
    struct bb_raw_function_info *function_infos;
};


typedef struct summary_data
{
    long long sum;
    long long max;
    long arcs;
} summary_data;

/* Following are structures for GCC 3.4+ */

#define GCOV_COUNTERS_SUMMABLE  1  /* Counters which can be
                      summaried.  */

struct gcov_ctr_summary
{
  unsigned num;      /* number of counters.  */
  unsigned runs;     /* number of program runs */
  long long sum_all;        /* sum of all counters accumulated.  */
  long long run_max;        /* maximum value on a single run.  */
  long long sum_max;        /* sum of individual run max values.  */
};

/* Object & program summary record.  */
struct gcov_summary
{
  unsigned checksum; /* checksum of program */
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
  unsigned ident;    /* unique ident of function */
  unsigned checksum; /* function checksum */
  unsigned n_ctrs[0];       /* instrumented counters */
};

/* Information about counters.  */
struct gcov_ctr_info
{
  unsigned num;      /* number of counters.  */
  long long *values;        /* their values.  */
  void *merge;      /* The function used to merge them.  */
};

/* Information about a single object file.  */
struct gcov_info
{
  unsigned version;  /* expected version number */
  struct gcov_info *next;   /* link to next, used by libgcov */

  unsigned stamp;    /* uniquifying time stamp */
  const char *filename;     /* output file name */
  
  unsigned n_functions;     /* number of functions */
  const struct gcov_fn_info *functions; /* table of functions */

  unsigned ctr_mask;        /* mask of counters instrumented.  */
  struct gcov_ctr_info counts[0]; /* count data. The number of bits
                     set in the ctr_mask field
                     determines how big this array
                     is.  */
};

struct bb;
static void process_gcov_syms(FILE *symfile, int core_file,
                              void (*functor)(int, struct bb *, void *), 
                              void *extra);
static void do_gcov_sum(int core_file, struct bb *object, void *extra);
static void get_kernel_gcov_data(int core_file, 
                                 struct bb *object, void *extra);

static char *ksymtable;

void
tce_set_ksymtable(char *table)
{
    ksymtable = table;
}

static unsigned kernel_gcov_version_magic = 0;

static ssize_t
read_at(int fildes, off_t offset, void *buffer, size_t size)
{
    if(lseek(fildes, offset, SEEK_SET) == (off_t)-1)
        return (ssize_t)-1;
    return read(fildes, buffer, size);
}

static int
read_str(int fildes, off_t offset, char *buffer, size_t maxlen)
{
    if(lseek(fildes, offset, SEEK_SET) == (off_t)-1)
        return -1;
    while(--maxlen)
    {
        if(read(fildes, buffer, 1) <= 0)
            return -1;
        if(*buffer++ == '\0')
            return 0;
    }
    *buffer = '\0';
    errno = ENAMETOOLONG;
    return -1;
}

static void
detect_kernel_gcov_version(FILE *symfile, int core_file)
{
    size_t offset;
    static char symbuf[256];
    char *token;
    
    rewind(symfile);
    while (fgets(symbuf, sizeof(symbuf) - 1, symfile) != NULL)
    {
        offset = strtoul(symbuf, &token, 16);
        strtok(token, " \t\n");
        token = strtok(NULL, "\t\n");
        if (strcmp(token, "__gcov_version_magic") == 0)
        {
            read_at(core_file, offset, &kernel_gcov_version_magic,
                    sizeof(kernel_gcov_version_magic));
            return;
        }
    }
}

void
get_kernel_coverage(void)
{
    if (ksymtable != NULL)
    {
        FILE *symfile = fopen(ksymtable, "r");
        int core_file;
        summary_data summary = {0, 0, 0};
        
        if (symfile == NULL)
        {
            tce_report_error("Cannot open kernel symtable file %s: %s", 
                             ksymtable, strerror(errno));
            return;
        }
        core_file = open("/dev/kmem", O_RDONLY);
        if (core_file < 0)
        {
            tce_report_error("Cannot open kernel memory file: %s", strerror(errno));
            fclose(symfile);
            return;
        }
        
        detect_kernel_gcov_version(symfile, core_file);
        process_gcov_syms(symfile, core_file, do_gcov_sum, &summary);
        process_gcov_syms(symfile, core_file, get_kernel_gcov_data, &summary);
        close(core_file);
        fclose(symfile);
    }
}

static void
process_gcov_syms(FILE *symfile, int core_file,
                  void (*functor)(int, struct bb *, void *), void *extra)
{
    size_t offset;
    static char symbuf[256];
    char *token;
    
    rewind(symfile);
    while (fgets(symbuf, sizeof(symbuf) - 1, symfile) != NULL)
    {
        offset = strtoul(symbuf, &token, 16);
        strtok(token, " \t\n");
        token = strtok(NULL, "\t\n");
        if (strstr(token, "GCOV") != NULL)
        {
            /* This is a very crude hack relying on internals of GCC */
            /* It is necessary because GCOV symbols are pointing to
               constructors, rather than actual data. 
               On x86, the pointer to the actual data happen to be
               4 bytes later (an immediate argument to mov).
               Most probably, this won't work on x86-64 :(
            */
            unsigned long tmp;
            struct bb bb_buf;
            
            DEBUGGING(tce_report_notice("offset is %lu", 
                                    (unsigned long)offset));
            if(read_at(core_file, offset + 4, &tmp, sizeof(tmp)) < 
               (ssize_t)sizeof(tmp))
            {
                tce_report_error("read error on /dev/kmem: %s", 
                             strerror(errno));
                continue;
            }
            DEBUGGING(tce_report_notice("new offset is %lu", tmp));
            if(read_at(core_file, tmp, &bb_buf, sizeof(bb_buf)) < 
               (ssize_t)sizeof(bb_buf))
            {
                tce_report_error("read error on /dev/kmem: %s", 
                             strerror(errno));
                continue;
            }
            
            functor(core_file, &bb_buf, extra);
        }
    }

}

static void
do_gcov_sum(int core_file, struct bb *object, void *extra)
{
    summary_data *summary = extra;
    int i;
    long long counter;

    if(lseek(core_file, (size_t)object->counts, SEEK_SET) == (off_t)-1)
    {
        tce_report_error("seeking error on /dev/kmem: %s", strerror(errno));
        return;
    }
    for (i = 0; i < object->ncounts; i++)
    {
        if(read(core_file, &counter, sizeof(counter)) < 
           (ssize_t)sizeof(counter))
        {
            tce_report_error("error reading from /dev/kmem: %s", 
                         strerror(errno));
            return;
        }
        summary->sum += counter;
        
        if (counter > summary->max)
            summary->max = counter;
    }
    summary->arcs += object->ncounts;
}


static void
get_kernel_gcov_data(int core_file, struct bb *object, void *extra)
{
    summary_data *summary = extra;
    summary_data object_summary = {0, 0, 0};
    long object_functions = 0;
    struct bb_raw_function_info fn_info;
    bb_function_info *fi;
    bb_object_info *oi;
    long long *target_counters;
    long long counter;
    int i;
    static char name_buffer[PATH_MAX + 1];
    char *real_start;
        

    if (lseek(core_file, (size_t)object->function_infos, SEEK_SET) == 
        (off_t)-1)
    {
        tce_report_error("seeking error on /dev/kmem: %s", strerror(errno));
        return;
    }
    for (;;)
    {
        if(read(core_file, &fn_info, sizeof(fn_info)) < 
           (ssize_t)sizeof(fn_info))
        {
            tce_report_error("error reading from /dev/kmem: %s", 
                         strerror(errno));
            return;
        }
        if (fn_info.arc_count < 0)
            break;
        
        object_functions++;
    }

    do_gcov_sum(core_file, object, &object_summary);
    if (read_str(core_file, (size_t)object->filename, 
                 name_buffer, sizeof(name_buffer)) != 0)
    {
        tce_report_error("error reading from /dev/kmem: %s", strerror(errno));
        return;
    }
    real_start = strstr(name_buffer, "//");
    oi = get_object_info(obtain_principal_peer_id(), 
                         real_start ? real_start + 1 : name_buffer);
    oi->ncounts = object->ncounts;
    oi->object_functions = object_functions;
    oi->object_sum += object_summary.sum;
    oi->program_sum += summary->sum;
    oi->program_arcs += summary->arcs;
    if (object_summary.max > oi->object_max)
        oi->object_max = object_summary.max;
    if (summary->max > oi->program_max)
        oi->program_max = summary->max;

    for(;;)
    {          
        if(read_at(core_file, (size_t)object->function_infos++, 
                   &fn_info, sizeof(fn_info)) < (ssize_t)sizeof(fn_info))
        {
            tce_report_error("error reading from /dev/kmem: %s", 
                         strerror(errno));
            return;
        }
        if (fn_info.arc_count < 0)
            break;
        if (read_str(core_file, (size_t)fn_info.name, 
                     name_buffer, sizeof(name_buffer)) != 0)
        {
            tce_report_error("error reading from /dev/kmem: %s", 
                         strerror(errno));
            return;
        }
        fi = get_function_info(oi, name_buffer, 
                               fn_info.arc_count, fn_info.checksum);
        if (fi != NULL)
        {
            for (i = fn_info.arc_count, target_counters = fi->counts;
                 i > 0; 
                 i--, object->counts++, target_counters++)
            {
                read_at(core_file, (size_t)object->counts, 
                        &counter, sizeof(counter));
                *target_counters += counter;
            }
        }
    }

}

