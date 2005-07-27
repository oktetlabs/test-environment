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

#define _FILE_OFFSET_BITS 64
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

typedef struct summary_data
{
    long long sum;
    long long max;
    long arcs;
    struct gcov_ctr_info groups[GCOV_COUNTER_GROUPS];
} summary_data;

typedef union object_coverage
{
    struct bb old;
    struct gcov_info new;
} object_coverage;

static void process_gcov_syms(FILE *symfile, int core_file,
                              void (*functor)(int, object_coverage *, void *), 
                              void *extra);
static void do_gcov_sum(int core_file, object_coverage *object, void *extra);
static void get_kernel_gcov_data(int core_file, 
                                 object_coverage *object, void *extra);

static char *ksymtable;

void
tce_set_ksymtable(char *table)
{
    ksymtable = table;
}

static unsigned kernel_gcov_version_magic = 0;
static size_t sizeof_object_coverage = sizeof(struct bb);
static size_t kernel_merge_functions[TCE_MERGE_MAX];

static ssize_t
read_at(int fildes, off_t offset, void *buffer, size_t size)
{
    ssize_t nsize;
    
    tce_print_debug("reading %lu at %lx", (unsigned long)size, (unsigned long)offset);
    if(lseek(fildes, offset, SEEK_SET) == (off_t)-1)
    {
        tce_report_error("seek error: %s", strerror(errno));
        return (ssize_t)-1;
    }
    nsize = read(fildes, buffer, size);
    tce_print_debug("read %d bytes", (int)nsize);
    if (nsize < (ssize_t)size)
        tce_report_error("reading error: %s", strerror(errno));
    return nsize;
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
    int procfile = open("/proc/tce_gcov_magic", O_RDONLY);
    
    if (procfile >= 0)
    {
        read(procfile, &kernel_gcov_version_magic, 
             sizeof(kernel_gcov_version_magic));
        close(procfile);
        rewind(symfile);
        while (fgets(symbuf, sizeof(symbuf) - 1, symfile) != NULL)
        {
            offset = strtoul(symbuf, &token, 16);
            strtok(token, " \t\n");
            token = strtok(NULL, " \t\n");
            if (strcmp(token, "__gcov_merge_add") == 0)
            {
                kernel_merge_functions[TCE_MERGE_ADD] = offset;
            }
            else if (strcmp(token, "__gcov_merge_single") == 0)
            {
                kernel_merge_functions[TCE_MERGE_SINGLE] = offset;
            }
            else if (strcmp(token, "__gcov_merge_delta") == 0)
            {
                kernel_merge_functions[TCE_MERGE_DELTA] = offset;
            }            
        }
    }
    if (kernel_gcov_version_magic != 0)
    {
        tce_report_notice("kernel GCOV version is %#lx, record size is %d",
                          kernel_gcov_version_magic ,
                          (int)sizeof_object_coverage);
    }
    else
    {
        tce_report_notice("kernel GCOV is pre-3.4, record size is %d",
                          (int)sizeof_object_coverage);
    }
}

void
tce_obtain_kernel_coverage(void)
{
    if (ksymtable != NULL)
    {
        FILE *symfile = fopen(ksymtable, "r");
        int core_file;
        summary_data summary = {0, 0, 0, {{0, 0, 0}}};
        
        if (symfile == NULL)
        {
            tce_report_error("Cannot open kernel symtable file %s: %s", 
                             ksymtable, strerror(errno));
            return;
        }
        core_file = open("/dev/tce_kmem", O_RDONLY);
        if (core_file < 0)
        {
            core_file = open("/dev/kmem", O_RDONLY);
            if (core_file < 0)
            {
                tce_report_error("Cannot open kernel memory file: %s", strerror(errno));
                fclose(symfile);
                return;
            }
        }
        
        detect_kernel_gcov_version(symfile, core_file);
        process_gcov_syms(symfile, core_file, do_gcov_sum, &summary);
        process_gcov_syms(symfile, core_file, get_kernel_gcov_data, &summary);
        close(core_file);
        fclose(symfile);
    }
}

static te_bool 
check_module_offset(const char *module, unsigned long offset)
{
    FILE *modules = fopen("/proc/modules", "r");
    char buffer[256];
    char *token;
    unsigned long size, start;
    
    if (modules == NULL)
    {
        tce_report_error("cannot open /proc/modules: %s", strerror(errno));
        return FALSE;
    }
    while (fgets(buffer, sizeof(buffer) - 1, modules) != NULL)
    {
        token = strtok(buffer, " \t\n");
        if (token != NULL && strcmp(token, module) == 0)
        {
            token = strtok(NULL, " \t\n");
            size = strtoul(token, NULL, 10);
            strtok(NULL, " \t\n"); /* skip refcnt */
            strtok(NULL, " \t\n");
            strtok(NULL, " \t\n"); /* skip alive status */
            token = strtok(NULL, " \t\n");
            if (token != NULL)
            {
                start = strtoul(token, NULL, 0);
                tce_print_debug("module code is at %lx..%lx", start, start + size);
                return offset >= start && offset < start + size;
            }
        }
    }
    tce_report_error("module %s not found", module);
    return FALSE;
}

static void
process_gcov_syms(FILE *symfile, int core_file,
                  void (*functor)(int, object_coverage *, void *), void *extra)
{
    size_t offset;
    static char symbuf[256];
    char *token;
    
    rewind(symfile);
    while (fgets(symbuf, sizeof(symbuf) - 1, symfile) != NULL)
    {
        offset = strtoul(symbuf, &token, 16);
        strtok(token, " \t\n");
        token = strtok(NULL, " \t\n");
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
            object_coverage bb_buf;
            static int address_offset = -1;
            
            token = strtok(NULL, " \t\n"); 
            if (token == NULL) /* not a module symbol */
            {
                tce_report_notice("GCOV symbol not in a module, ignoring");
                continue; 
            }
            token[strlen(token) - 1] = '\0'; /* eliminate brackets */
            token++;

            tce_print_debug("offset is %lx for %d", 
                            (unsigned long)offset,
                            core_file);
            if (address_offset < 0)
            {
                for (address_offset = 0; address_offset < 16; 
                     address_offset++)
                {
                    tce_print_debug("trying offset %lx", offset + address_offset);
                    if(read_at(core_file, offset + address_offset, &tmp, sizeof(tmp)) < 
                       (ssize_t)sizeof(tmp))
                    {
                        tce_report_error("read error on /dev/kmem");
                        tmp = 0;
                        break;
                    }
                    if (check_module_offset(token, tmp))
                        break;
                }
                if (address_offset >= 16)
                {
                    tce_report_error("data pointer not found");
                    address_offset = -1;
                    continue;
                }
                if (tmp == 0) 
                {
                    address_offset = -1;
                    continue;
                }
                tce_report_notice("data pointer is %d bytes after the symbol",
                                  address_offset);
            }
            else
            {
                if(read_at(core_file, offset + address_offset, &tmp, sizeof(tmp)) < 
                   (ssize_t)sizeof(tmp))
                {
                    tce_report_error("read error on /dev/kmem");
                    continue;
                }
            }
            tce_print_debug("new offset is %lu", tmp);
            if(read_at(core_file, tmp, &bb_buf, sizeof_object_coverage) < 
               (ssize_t)sizeof_object_coverage)
            {
                tce_report_error("read error on /dev/kmem");
                continue;
            }
            
            functor(core_file, &bb_buf, extra);
        }
    }

}

static void
summarize_counters(int core_file, unsigned ncounts, off_t values, summary_data *summary)
{
    unsigned i;

    for (i = 0; i < ncounts; i++, values += sizeof(long long))
    {
        long long counter;

        if(read_at(core_file, values, &counter, sizeof(counter)) < 
           (ssize_t)sizeof(counter))
        {
            tce_report_error("read error on /dev/kmem");
            return;
        }
        summary->sum += counter;
        
        if (counter > summary->max)
            summary->max = counter;
    }
    summary->arcs += ncounts;
}

static void
do_gcov_sum(int core_file, object_coverage *object, void *extra)
{
    summary_data *summary = extra;

    if (kernel_gcov_version_magic != 0)
    {
        unsigned i;
        
        if (object->new.ctr_mask & 1)
        {
            read(core_file, &summary->groups, sizeof(*summary->groups));
            summarize_counters(core_file, summary->groups->num, 
                               (size_t)summary->groups->values, summary);
        }
        for (i = 1; i < GCOV_COUNTER_GROUPS; i++)
        {
            if ((1 << i) & object->new.ctr_mask)
            {
                read(core_file, summary->groups + i, sizeof(*summary->groups));             
            }
        }
    }
    else
    {
        summarize_counters(core_file, object->old.ncounts, 
                           (size_t)object->old.counts, summary);
    }
}


static void
get_kernel_gcov_data(int core_file, object_coverage *object, void *extra)
{
    summary_data *summary = extra;
    summary_data object_summary = {0, 0, 0, {{0, 0, 0}}};
    long object_functions = 0;
    struct bb_function_info fn_info;
    tce_function_info *fi;
    tce_object_info *oi;
    long long *target_counters;
    long long counter;
    int i;
    static char name_buffer[1024];
    char *real_start;
        

    if (kernel_gcov_version_magic != 0)
    {
        object_functions = object->new.n_functions;
    }
    else
    {
        if (lseek(core_file, (size_t)object->old.function_infos, SEEK_SET) == 
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
    }

    do_gcov_sum(core_file, object, &object_summary);
    if (read_str(core_file, kernel_gcov_version_magic == 0 ? 
                 (size_t)object->old.filename :
                 (size_t)object->new.filename, 
                 name_buffer, sizeof(name_buffer)) != 0)
    {
        tce_report_error("error reading from /dev/kmem: %s", strerror(errno));
        return;
    }
    real_start = strstr(name_buffer, "//");
    oi = tce_get_object_info(tce_obtain_principal_peer_id(), 
                             real_start ? real_start + 1 : name_buffer);
    oi->gcov_version = kernel_gcov_version_magic;
    oi->ncounts = object->old.ncounts;
    oi->object_functions = object_functions;
    oi->object_sum += object_summary.sum;
    oi->program_sum += summary->sum;
    oi->program_arcs += summary->arcs;
    oi->program_runs = 1;
    oi->object_runs  = 1;
    if (object_summary.max > oi->object_max)
        oi->object_max = object_summary.max;
    if (summary->max > oi->program_max)
        oi->program_max = summary->max;
    oi->program_sum_max += summary->max;
    oi->object_sum_max  += object_summary.max;

    if (kernel_gcov_version_magic != 0)
    {
        unsigned fn;
        struct gcov_fn_info new_fn_info;
        unsigned n_counts = 0;
        unsigned n_sub_counts[GCOV_COUNTER_GROUPS] = {0};
        unsigned fi_stride;

        oi->stamp = object->new.stamp;

        for (i = 0; i < GCOV_COUNTER_GROUPS; i++)
        {
            if ((1 << i) & object->new.ctr_mask)
                n_counts++;
        }
        fi_stride = sizeof (struct gcov_fn_info) + 
            n_counts * sizeof (unsigned);
        if (__alignof__ (struct gcov_fn_info) > sizeof (unsigned))
        {
            fi_stride += __alignof__ (struct gcov_fn_info) - 1;
            fi_stride &= ~(__alignof__ (struct gcov_fn_info) - 1);
        }
        
        for (fn = object_functions; fn != 0; fn--)
        {
            if(read_at(core_file, (size_t)object->new.functions, 
                       &new_fn_info, sizeof(struct gcov_fn_info)) < 
               (ssize_t)sizeof(struct gcov_fn_info))
            {
                tce_report_error("error reading from /dev/kmem");
                return;
            }            
            for (n_counts = 0, i = 0; i < GCOV_COUNTER_GROUPS; i++)
            {
                if ((1 << i) & object->new.ctr_mask)
                {
                    read(core_file, n_sub_counts + i, sizeof(*n_sub_counts));
                    n_counts += n_sub_counts[i];               
                }           
            }
            snprintf(name_buffer, sizeof(name_buffer), "%u",
                     new_fn_info.ident);
            fi = tce_get_function_info(oi, name_buffer, 
                                       n_counts, new_fn_info.checksum);
            if (fi != NULL)
            {
                target_counters = fi->counts;              
                for (i = 0; i < GCOV_COUNTER_GROUPS; i++)
                {                  
                    if ((1 << i) & object->new.ctr_mask)
                    {
                        unsigned j;
                        long long ctr_value[4];                       
        
                        lseek(core_file, (size_t)summary->groups[i].values, SEEK_SET);
                        if ((size_t)summary->groups[i].merge == 
                            kernel_merge_functions[TCE_MERGE_ADD])
                        {                          
                            for (j = 0; j < n_sub_counts[i]; j++)
                            {
                                read(core_file, ctr_value, sizeof(*ctr_value));
                                (*target_counters++) += ctr_value[0];                              
                            }
                        }
                        else if ((size_t)summary->groups[i].merge == 
                                 kernel_merge_functions[TCE_MERGE_SINGLE])
                        {
                            for (j = 0; j < n_sub_counts[i]; j += 3)
                            {
                                read(core_file, ctr_value, 3 * sizeof(ctr_value));
                                if (target_counters[0] == ctr_value[0])
                                    target_counters[1] += ctr_value[1];
                                else if (ctr_value[1] > target_counters[1])
                                {
                                    target_counters[0] = ctr_value[0];
                                    target_counters[1] = ctr_value[1] - target_counters[1];
                                }
                                else
                                    target_counters[1] -= ctr_value[1];
                                target_counters[2] += ctr_value[2];

                                target_counters += 3;                               
                            }
                        }
                        else if ((size_t)summary->groups[i].merge == 
                                 kernel_merge_functions[TCE_MERGE_DELTA])
                        {
                            for (j = 0; j < n_sub_counts[i]; j += 4)
                            {
                                read(core_file, ctr_value, 4 * sizeof(ctr_value));
                                if (target_counters[1] == ctr_value[1])
                                    target_counters[2] += ctr_value[2];
                                else if (ctr_value[2] > target_counters[2])
                                {
                                    target_counters[1] = ctr_value[1];
                                    target_counters[2] = ctr_value[2] - target_counters[2];
                                }
                                else
                                    target_counters[2] -= ctr_value[2];
                                target_counters[3] += ctr_value[3];

                                target_counters += 4;
                            }
                        }
                       
                        summary->groups[i].values += n_sub_counts[i];                      
                    }                 
                }             
            }
            
            object->new.functions = (struct gcov_fn_info *) 
                ((char *)object->new.functions + fi_stride);
        }
    }
    else
    {
        for (;;)
        {          
            if(read_at(core_file, (size_t)object->old.function_infos++, 
                       &fn_info, sizeof(fn_info)) < (ssize_t)sizeof(fn_info))
            {
                tce_report_error("error reading from /dev/kmem");
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
            fi = tce_get_function_info(oi, name_buffer, 
                                       fn_info.arc_count, fn_info.checksum);
            if (fi != NULL)
            {
                for (i = fn_info.arc_count, target_counters = fi->counts;
                     i > 0; 
                     i--, object->old.counts++, target_counters++)
                {
                    read_at(core_file, (size_t)object->old.counts, 
                            &counter, sizeof(counter));
                    *target_counters += counter;
                }
            }
        }
    }
}

