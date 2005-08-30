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
                              void (*functor)(void *, unsigned long, unsigned long, object_coverage *, void *), 
                              void *extra);
static void do_gcov_sum(void *data, unsigned long start, unsigned long size, 
                        object_coverage *object, void *extra);
static void get_kernel_gcov_data(void *data, unsigned long start, unsigned long size,
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

typedef struct module_info
{
    char               *name;
    unsigned long       start;
    unsigned long       size;
    void               *kernel_data;
    struct module_info *next;
} module_info;

static module_info *module_list;

static te_bool
read_modules(void)
{
    FILE *modules = fopen("/proc/modules", "r");
    char buffer[256];
    char name[64];
    unsigned long size, start;
    module_info *the_module;
    
    if (modules == NULL)
    {
        tce_report_error("cannot open /proc/modules: %s", strerror(errno));
        return FALSE;
    }
    while (fgets(buffer, sizeof(buffer) - 1, modules) != NULL)
    {
        if (sscanf(buffer, "%63s %lu %*d %*s %*s 0x%lx", 
                   name, &size, &start) != 3)
        {
            tce_report_notice("malformed string in /proc/modules: %s", buffer);
            continue;
        }
        the_module = malloc(sizeof(*the_module));
        the_module->name        = strdup(name);
        the_module->start       = start;
        the_module->size        = size;
        the_module->kernel_data = NULL;
        the_module->next        = module_list;
        module_list             = the_module;
    }
    return TRUE;
}

static void *
read_module_data (const char *name, int core_file, unsigned long *start, unsigned long *size)
{
    module_info *mod;
    for (mod = module_list; mod != NULL; mod = mod->next)
    {
        if (strcmp(mod->name, name) == 0)
        {
            if (mod->kernel_data == NULL)
            {
                tce_print_debug("reading %lu bytes from %lx", mod->size, mod->start);
                mod->kernel_data = malloc(mod->size);
                lseek(core_file, mod->start, SEEK_SET);
                errno = 0;
                if (read(core_file, mod->kernel_data, mod->size) < (ssize_t)mod->size)
                {
                    tce_report_error("error reading module %s data: %s", name, strerror(errno));
                    free(mod->kernel_data);
                    mod->kernel_data = NULL;
                    return NULL;
                }
            }
            *start = mod->start;
            *size  = mod->size;
            return mod->kernel_data;
        }
    }
    tce_report_error("module %s not found", name);
    return NULL;
}

static ssize_t
read_safe(int fildes, void *buffer, size_t size)
{
    ssize_t nsize;
    nsize = read(fildes, buffer, size);
    tce_print_debug("read %d bytes", (int)nsize);
    if (nsize == (ssize_t)-1)
        tce_report_error("reading error: %s", strerror(errno));
    else if (nsize < (ssize_t)size)
        tce_report_error("read %ld < %ld", (long)nsize, (long)size);
    return nsize;
}

static void
detect_kernel_gcov_version(FILE *symfile)
{
    size_t offset;
    static char symbuf[256];
    char *token;
    int procfile = open("/proc/tce_gcov_magic", O_RDONLY);
    
    if (procfile >= 0)
    {
        read_safe(procfile, &kernel_gcov_version_magic, 
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
    tce_print_debug("starting kernel TCE");
    if (ksymtable != NULL)
    {
        if (read_modules())
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
            
            detect_kernel_gcov_version(symfile);
            process_gcov_syms(symfile, core_file, do_gcov_sum, &summary);
            process_gcov_syms(symfile, core_file, get_kernel_gcov_data, &summary);
            close(core_file);
            fclose(symfile);
        }
    }
}


static void
process_gcov_syms(FILE *symfile, int core_file,
                  void (*functor)(void *, unsigned long, unsigned long, object_coverage *, void *), void *extra)
{
    unsigned long offset;
    unsigned long mod_start, mod_size;
    char *mod_data;
    static char symbuf[256];
    char symname[128];
    char modname[64];
    
    rewind(symfile);
    while (fgets(symbuf, sizeof(symbuf) - 1, symfile) != NULL)
    {
        if (sscanf(symbuf, "%lx %*s %s [%[^]]]", &offset, symname, modname) != 3)
        {
            continue;
        }
        if (strstr(symname, "GCOV") != NULL)
        {
            /* This is a very crude hack relying on internals of GCC */
            /* It is necessary because GCOV symbols are pointing to
               constructors, rather than actual data. 
               On x86, the pointer to the actual data happen to be
               4 bytes later (an immediate argument to mov).
               Most probably, this won't work on x86-64 :(
            */
            unsigned long actual_offset = 0;
            object_coverage *bb_ptr;
            static int address_offset = -1;

            tce_print_debug("processing %s in module %s", symname, modname);
            mod_data = read_module_data(modname, core_file, &mod_start, &mod_size);
            if (mod_data == NULL)
            {
                continue;
            }
            if (offset < mod_start || offset >= mod_start + mod_size)
            {
                tce_report_error("invalid symbol offset %lx "
                                 "(must be in range %lx..%lx)", 
                                 offset, mod_start, mod_start + mod_size - 1);
                continue;
            }
            offset -= mod_start;

            tce_print_debug("offset is %lx for %d", 
                            (unsigned long)offset,
                            core_file);
            if (address_offset < 0)
            {
                for (address_offset = 0; address_offset < 16 && address_offset < (int)mod_size; 
                     address_offset++)
                {
                    tce_print_debug("trying offset %lx", offset + address_offset);
                    actual_offset = *(unsigned long *)(mod_data + offset + address_offset);
                    if (actual_offset >= mod_start && actual_offset < mod_start + mod_size)
                        break;
                }
                if (address_offset >= 16 || address_offset >= (int)mod_size)
                {
                    tce_report_error("data pointer not found");
                    address_offset = -1;
                    continue;
                }
                tce_report_notice("data pointer is %d bytes after the symbol",
                                  address_offset);
            }
            else
            {
                actual_offset = *(unsigned long *)(mod_data + offset + address_offset);
                if (actual_offset < mod_start || actual_offset >= mod_start + mod_size)
                {
                    tce_report_error("invalid data pointer %lx", actual_offset);
                }
            }
            actual_offset -= mod_start;
            tce_print_debug("new offset is %lu", actual_offset);
            bb_ptr = (object_coverage *)(mod_data + actual_offset);
            functor(mod_data, mod_start, mod_size, bb_ptr, extra);
        }
    }

}

static void
summarize_counters(long long *values, unsigned ncounts, summary_data *summary)
{
    unsigned i;

    for (i = 0; i < ncounts; i++, values++)
    {
        summary->sum += *values;
        
        if (*values > summary->max)
            summary->max = *values;
    }
    summary->arcs += ncounts;
}

#define NORMALIZE_OFFSET(offset, start, size, action)                 \
do {                                                                  \
    if ((offset) < (start) || (offset) >= (start) + (size))           \
    {                                                                 \
        tce_report_error("offset %lx out of range %lx..%lx (%s:%d)",  \
                         (offset), (start), (start) + (size) - 1,     \
                         __FUNCTION__, __LINE__);                     \
        action;                                                       \
    }                                                                 \
    offset -= start;                                                  \
} while(0)

static void
do_gcov_sum(void *data, unsigned long start, unsigned long size, object_coverage *object, void *extra)
{
    summary_data *summary = extra;

    if (kernel_gcov_version_magic != 0)
    {
        unsigned i;
        unsigned grp;
        unsigned long offset;
        
        if (object->new.ctr_mask & 1)
        {
            summary->groups[0] = object->new.counts[0];
            offset = (unsigned long)summary->groups[0].values;
            NORMALIZE_OFFSET(offset, start, size, return);
            summarize_counters((void *)((char *)data + offset), summary->groups->num, summary);
        }
        for (i = 1, grp = 1; i < GCOV_COUNTER_GROUPS; i++)
        {
            if ((1 << i) & object->new.ctr_mask)
            {
                summary->groups[i] = object->new.counts[grp];
                grp++;
            }
        }
    }
    else
    {
        unsigned long values = (unsigned long)object->old.counts;
        NORMALIZE_OFFSET(values, start, size, return);
        summarize_counters((void *)((char *)data + values), object->old.ncounts, summary);
    }
}


static void
get_kernel_gcov_data(void *data, unsigned long start, unsigned long size, object_coverage *object, void *extra)
{
    unsigned long offset;
    summary_data *summary = extra;
    summary_data object_summary = {0, 0, 0, {{0, 0, 0}}};
    long object_functions = 0;
    tce_function_info *fi;
    tce_object_info *oi;
    long long *target_counters;
    int i;
    char *name_ptr;
    char *real_start;
        

    if (kernel_gcov_version_magic != 0)
    {
        object_functions = object->new.n_functions;
    }
    else
    {
        struct bb_function_info *fn_info;

        offset = (unsigned long)object->old.function_infos;
        NORMALIZE_OFFSET(offset, start, size, return);
        for (fn_info = (void *)((char *)data + offset); fn_info->arc_count >= 0; fn_info++)
        {
            object_functions++;
        }
    }

    do_gcov_sum(data, start, size, object, &object_summary);
    offset = kernel_gcov_version_magic == 0 ? 
        (unsigned long)object->old.filename :
        (unsigned long)object->new.filename;
    NORMALIZE_OFFSET(offset, start, size, return);
    name_ptr = (char *)data + offset;
    real_start = strstr(name_ptr, "//");
    oi = tce_get_object_info(tce_obtain_principal_peer_id(), 
                             real_start ? real_start + 1 : name_ptr);
    tce_print_debug("accessing %s", name_ptr);
    oi->gcov_version = kernel_gcov_version_magic;
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
        struct gcov_fn_info *new_fn_info;
        unsigned n_counts = 0;
        unsigned n_sub_counts[GCOV_COUNTER_GROUPS];
        unsigned fi_stride;
        char name_buffer[32];
        int grp;

        oi->ncounts = object_summary.arcs;
        oi->program_ncounts = summary->arcs;
        oi->stamp = object->new.stamp;
        oi->ctr_mask = object->new.ctr_mask;

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
        
        offset = (unsigned long)object->new.functions;
        NORMALIZE_OFFSET(offset, start, size, return);
        new_fn_info = (void *)((char *)data + offset);

        for (fn = object_functions; fn != 0; fn--)
        {
            snprintf(name_buffer, sizeof(name_buffer), "%u",
                     new_fn_info->ident);
            for (n_counts = 0, i = 0, grp = 0; i < GCOV_COUNTER_GROUPS; i++)
            {
                if ((1 << i) & object->new.ctr_mask)
                {
                    n_sub_counts[i] = new_fn_info->n_ctrs[grp];
                    grp++;
                }           
            }

            fi = tce_get_function_info(oi, name_buffer, 
                                       n_counts, new_fn_info->checksum);
            for (i = 0; i < GCOV_COUNTER_GROUPS; i++)
            {
                if ((1 << i) & object->new.ctr_mask)
                {
                    fi->groups[i].number = n_sub_counts[i];
                }           
            }
            if (fi != NULL)
            {
                long long *source_counters;

                target_counters = fi->counts;              
                for (i = 0; i < GCOV_COUNTER_GROUPS; i++)
                {                  
                    if ((1 << i) & object->new.ctr_mask)
                    {
                        unsigned j;
        
                        offset = (unsigned long)summary->groups[i].values;
                        NORMALIZE_OFFSET(offset, start, size, continue);
                        source_counters = (void *)((char *)data + offset);

                        if ((size_t)summary->groups[i].merge == 
                            kernel_merge_functions[TCE_MERGE_ADD])
                        {                          
                            fi->groups[i].mode = TCE_MERGE_ADD;
                            for (j = 0; j < fi->groups[i].number; j++)
                            {
                                tce_print_debug("counter is %Ld", *source_counters);
                                *target_counters++ += *source_counters++;                              
                            }
                        }
                        else if ((size_t)summary->groups[i].merge == 
                                 kernel_merge_functions[TCE_MERGE_SINGLE])
                        {
                            fi->groups[i].mode = TCE_MERGE_SINGLE;
                            for (j = 0; j < fi->groups[i].number; j += 3)
                            {
                                if (target_counters[0] == source_counters[0])
                                    target_counters[1] += source_counters[1];
                                else if (source_counters[1] > target_counters[1])
                                {
                                    target_counters[0] = source_counters[0];
                                    target_counters[1] = source_counters[1] - target_counters[1];
                                }
                                else
                                    target_counters[1] -= source_counters[1];
                                target_counters[2] += source_counters[2];

                                target_counters += 3;
                                source_counters += 3;
                            }
                        }
                        else if ((size_t)summary->groups[i].merge == 
                                 kernel_merge_functions[TCE_MERGE_DELTA])
                        {
                            fi->groups[i].mode = TCE_MERGE_DELTA;
                            for (j = 0; j < fi->groups[i].number; j += 4)
                            {
                                if (target_counters[1] == source_counters[1])
                                    target_counters[2] += source_counters[2];
                                else if (source_counters[2] > target_counters[2])
                                {
                                    target_counters[1] = source_counters[1];
                                    target_counters[2] = source_counters[2] - target_counters[2];
                                }
                                else
                                    target_counters[2] -= source_counters[2];
                                target_counters[3] += source_counters[3];

                                target_counters += 4;
                                source_counters += 4;
                            }
                        }
                        else
                        {
                            tce_report_error("unknown merge function");
                        }
                       
                        summary->groups[i].values += fi->groups[i].number;                      
                    }                 
                }             
            }
            
            new_fn_info = (struct gcov_fn_info *) 
                ((char *)new_fn_info + fi_stride);
        }
    }
    else
    {
        struct bb_function_info *fn_info;
        oi->ncounts = object->old.ncounts;

        offset = (unsigned long)object->old.function_infos;
        NORMALIZE_OFFSET(offset, start, size, return);

        for (fn_info = (void *)((char *)data + offset); fn_info->arc_count >= 0; fn_info++)
        {          
            offset = (unsigned long)fn_info->name;
            NORMALIZE_OFFSET(offset, start, size, continue);
            name_ptr = (char *)data + offset;
            fi = tce_get_function_info(oi, name_ptr, 
                                       fn_info->arc_count, fn_info->checksum);
            if (fi != NULL)
            {
                long long *source_counters;
                
                offset = (unsigned long)object->old.counts;
                NORMALIZE_OFFSET(offset, start, size, continue);
                source_counters = (void *)((char *)data + offset);
                for (i = fn_info->arc_count, target_counters = fi->counts;
                     i > 0; 
                     i--)
                {
                    *target_counters++ += *source_counters++;
                }
            }
        }
    }
}

