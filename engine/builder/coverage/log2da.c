/** @file 
 *
 * @brief Test Environment Builder
 *
 *  Extracts GCOV data from a textual log
 * 
 *  Copyright (C) 2005 The authors of the Test Environment
 *  Copyright (C) 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
 *  2000, 2001, 2002  Free Software Foundation, Inc. 
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details. 
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 *
 *  @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 *  $Id$
 */

/* This code is based on libgcc2.c from GCC distribution.
 * It uses parts of gcov-related routines to convert log records
 * to gcov accepted DA files 
 */

#include <te_config.h>
#include <te_defs.h>
#include <limits.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "gcov-io.h"

enum PROCESSING_LEVELS { NO_LEVEL = -1, OBJECT_LEVEL, FUNCTION_LEVEL, ARC_LEVEL };

static char *agent_name;

const char *find_line(enum PROCESSING_LEVELS level)
{
    static char buffer[1024] = ""; 
    static enum PROCESSING_LEVELS level_in_buffer = NO_LEVEL;
    static te_bool line_block = FALSE;
    static char *bufptr;

    static const char *level_markers[] = 
        {"TCE total", "TCE function", "TCE arc", NULL};
    int l;

    while (level_in_buffer < 0)
    {
        if (!line_block)
        {
            if (fgets(buffer, sizeof(buffer) - 1, stdin) == NULL)
            {
                return NULL;
            }
            if (strncmp(buffer, "RING", 4) != 0 ||
                strstr(buffer, agent_name) == NULL)
            {
                    continue;
            }
        }
        
        if (fgets(buffer, sizeof(buffer) - 1, stdin) == NULL)
        {
            fputs("log is corrupted\n", stderr);
            exit(EXIT_FAILURE);
        }
        if (*buffer == '\n')
        {
            line_block = FALSE;
            continue;
        }
        line_block = TRUE;
        for (l = level; l >= 0; l--)
        {
            if ((bufptr = strstr(buffer, level_markers[l])) != NULL)
            {
                level_in_buffer = l;
                break;
            }
        }
    }
    
    
    if (level_in_buffer != level)
        return NULL;
    bufptr += strlen(level_markers[level_in_buffer]);
    level_in_buffer = NO_LEVEL;
    return bufptr;
}


int main(int argc, char *argv[])
{
    long long program_sum = 0;
    long long program_max = 0;
    long program_arcs = 0;
    char da_filename[PATH_MAX + 1];
    FILE *da_file;
    long long object_max = 0;
    long long object_sum = 0;
    long object_functions = 0;
    long ncounts = 0;
    const char *logline;
            

    agent_name = argv[1];
    for(;;)
    {
        if ((logline = find_line(OBJECT_LEVEL)) == NULL)
            break;

        if (sscanf(logline, "%s %ld:%ld:%Ld:%Ld:%ld:%Ld:%Ld\n", 
                   da_filename,
                   &object_functions, 
                   &program_arcs, 
                   &program_sum, 
                   &program_max, 
                   &ncounts, 
                   &object_sum, 
                   &object_max) != 8)
        {
            fprintf(stderr, "Cannot parse '%s'", logline);
            return EXIT_FAILURE;
        }

        /* Open for modification */
        da_file = fopen (da_filename, "wb");
      
        if (!da_file)
        {
            fprintf (stderr, "arc profiling: Can't open output file %s.\n",
                     da_filename);
            return EXIT_FAILURE;
        }

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
            || __write_long (ncounts, da_file, 4)
            /* sum of counters.  */
            || __write_gcov_type (object_sum, da_file, 8)
            /* maximal counter.  */
            || __write_gcov_type (object_max, da_file, 8))
        {
            fprintf (stderr, "arc profiling: Error writing output file %s.\n",
                     da_filename);
            return EXIT_FAILURE;
        }
        else
        {
            char name[128];
            long checksum;
            long arc_count; 
            long long counter;
           
            for (; object_functions != 0; object_functions--)
            {
                
                if ((logline = find_line(FUNCTION_LEVEL)) == NULL)
                {
                    fprintf(stderr, "function profiling: log corrupted in %s\n", da_filename);
                    break;
                }
                
                if (sscanf(logline, "%s %ld:%ld", name,
                           &checksum, &arc_count) != 3)
                {
                    fprintf(stderr, "Cannot parse function '%s'", logline);
                    return EXIT_FAILURE;
                }

                if (__write_gcov_string (name,
                                         strlen (name), da_file, -1)
                    || __write_long (checksum, da_file, 4)
                    || __write_long (arc_count, da_file, 4))
                {
                    fprintf (stderr, "arc profiling: Error writing output file %s.\n",
                             da_filename);
                    return EXIT_FAILURE;
                }
             
                for (; arc_count != 0; arc_count--)
                {
                    if ((logline = find_line(ARC_LEVEL)) == NULL)
                    {
                        fprintf(stderr, "arc profiling: log is corrupted near %s:'%s'\n", da_filename, name);
                        break;
                    }

                    if (sscanf(logline, "%Ld", &counter) != 1)
                    {
                        fprintf(stderr, "Cannot parse arc '%s'", logline);
                        return EXIT_FAILURE;
                    }


                    if (__write_gcov_type (counter, da_file, 8))
                    {
                        fprintf (stderr, "arc profiling: Error writing output file %s.\n",
                                 da_filename);
                        return EXIT_FAILURE;
                    }
                }
            }
        }

        if (fclose (da_file))
        {
            fprintf (stderr, "arc profiling: Error closing output file %s.\n",
                     da_filename);
            return EXIT_FAILURE;
        }
    }

    return 0;
}
