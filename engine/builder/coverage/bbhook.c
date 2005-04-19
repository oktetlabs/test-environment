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
#include <ta_logfork.h>

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

/* Dump the coverage counts. We merge with existing counts when
   possible, to avoid growing the .da files ad infinitum.  */

void
__bb_exit_func (void)
{
    struct bb *ptr;
    int i;
    long long program_sum = 0;
    long long program_max = 0;
    long program_arcs = 0;

    if (logfork_register_user("TCE"))
        return;
    
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
        
        for (fn_info = ptr->function_infos; fn_info->arc_count != -1; fn_info++)
            object_functions++;
        
        /* Calculate the per-object statistics.  */
        for (i = 0; i < ptr->ncounts; i++)
        {
            object_sum += ptr->counts[i];
            
            if (ptr->counts[i] > object_max)
                object_max = ptr->counts[i];
        }

        LOGF_RING("TCE", "total %s %ld:%Ld:%Ld:%ld:%Ld:%Ld\n", 
                  ptr->filename,
                  object_functions, 
                  program_arcs, 
                  program_sum, 
                  program_max, 
                  ptr->ncounts, 
                  object_sum, 
                  object_max);
      
        /* Write execution counts for each function.  */
        count_ptr = ptr->counts;
        
        for (fn_info = ptr->function_infos; fn_info->arc_count != -1;
             fn_info++)
        {          
            LOGF_RING("TCE", "function %s %ld:%ld\n", fn_info->name,
                      fn_info->checksum, fn_info->arc_count);
            for (i = fn_info->arc_count; i > 0; i--, count_ptr++)
            {
                LOGF_RING("TCE", "arc %Ld\n", *count_ptr);
            }
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

