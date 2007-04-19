/** @file
 * retrieve TCE data for gcc < 3.4
 *
 * Copyright (C) 2007 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 * Copyright (C) 2000, 2001  Free Software Foundation, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * In addition to the permissions in the GNU General Public License, the
 * Free Software Foundation and the Test Environment authors 
 * give you unlimited permission to link the
 * compiled version of this file into combinations with other programs,
 * and to distribute those combinations without any restriction coming
 * from the use of this file.  (The General Public License restrictions
 * do apply in other respects; for example, they cover modification of
 * the file, and distribution when not linked into a combine
 * executable.)
 *
 * @author Artem V. Andreev <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 *
 */

/* This file is based on a libgcc2.[ch] files that are part of the GCC 3.3 */

#include "te_config.h"

#include <stdio.h>
#include "te_errno.h"
#include "tapi_rpc_unistd.h"
#include "tce_internal.h"

#define PARAMS(x) x
#define ATTRIBUTE_UNUSED __attribute__ ((unused))
typedef long gcov_type;

static int __fetch_long PARAMS ((long *, char *, size_t))
    ATTRIBUTE_UNUSED;
static int __read_long  PARAMS ((long *, FILE *, size_t))
    ATTRIBUTE_UNUSED;
static int __write_long PARAMS ((long, FILE *, size_t))
    ATTRIBUTE_UNUSED;
static int __fetch_gcov_type PARAMS ((gcov_type *, char *, size_t))
    ATTRIBUTE_UNUSED;
static int __store_gcov_type PARAMS ((gcov_type, char *, size_t))
    ATTRIBUTE_UNUSED;
static int __read_gcov_type  PARAMS ((gcov_type *, FILE *, size_t))
    ATTRIBUTE_UNUSED;
static int __write_gcov_type PARAMS ((gcov_type, FILE *, size_t))
    ATTRIBUTE_UNUSED;
static int __write_gcov_string PARAMS ((const char *, size_t, FILE*, long))
    ATTRIBUTE_UNUSED;
static int __read_gcov_string PARAMS ((char *, size_t, FILE*, long))
    ATTRIBUTE_UNUSED;

/* These routines only work for signed values.  */

/* Store a portable representation of VALUE in DEST using BYTES*8-1 bits.
   Return a nonzero value if VALUE requires more than BYTES*8-1 bits
   to store.  */

static int
__store_gcov_type (value, dest, bytes)
     gcov_type value;
     char *dest;
     size_t bytes;
{
  int upper_bit = (value < 0 ? 128 : 0);
  size_t i;

  if (value < 0)
    {
      gcov_type oldvalue = value;
      value = -value;
      if (oldvalue != -value)
    return 1;
    }

  for(i = 0 ; i < (sizeof (value) < bytes ? sizeof (value) : bytes) ; i++) {
    dest[i] = value & (i == (bytes - 1) ? 127 : 255);
    value = value / 256;
  }

  if (value && value != -1)
    return 1;

  for(; i < bytes ; i++)
    dest[i] = 0;
  dest[bytes - 1] |= upper_bit;
  return 0;
}

/* Retrieve a quantity containing BYTES*8-1 bits from SOURCE and store
   the result in DEST. Returns a nonzero value if the value in SOURCE
   will not fit in DEST.  */

static int
__fetch_gcov_type (dest, source, bytes)
     gcov_type *dest;
     char *source;
     size_t bytes;
{
  gcov_type value = 0;
  int i;

  for (i = bytes - 1; (size_t) i > (sizeof (*dest) - 1); i--)
    if (source[i] & ((size_t) i == (bytes - 1) ? 127 : 255 ))
      return 1;

  for (; i >= 0; i--)
  {
      value = value * 256 + 
          (source[i] & ((size_t)i == (bytes - 1) ? 127 : 255));
  }
  

  if ((source[bytes - 1] & 128) && (value > 0))
    value = - value;

  *dest = value;
  return 0;
}

static int
__fetch_long (dest, source, bytes)
     long *dest;
     char *source;
     size_t bytes;
{
  long value = 0;
  int i;

  for (i = bytes - 1; (size_t) i > (sizeof (*dest) - 1); i--)
    if (source[i] & ((size_t) i == (bytes - 1) ? 127 : 255 ))
      return 1;

  for (; i >= 0; i--)
  {
    value = value * 256 + 
        (source[i] & ((size_t)i == (bytes - 1) ? 127 : 255));
  }
  

  if ((source[bytes - 1] & 128) && (value > 0))
    value = - value;

  *dest = value;
  return 0;
}

/* Write a BYTES*8-bit quantity to FILE, portably. Returns a nonzero
   value if the write fails, or if VALUE can't be stored in BYTES*8
   bits.

   Note that VALUE may not actually be large enough to hold BYTES*8
   bits, but BYTES characters will be written anyway.

   BYTES may be a maximum of 10.  */

static int
__write_gcov_type (value, file, bytes)
     gcov_type value;
     FILE *file;
     size_t bytes;
{
  char c[10];

  if (bytes > 10 || __store_gcov_type (value, c, bytes))
    return 1;
  else
    return fwrite(c, 1, bytes, file) != bytes;
}

static int
__write_long (value, file, bytes)
     long value;
     FILE *file;
     size_t bytes;
{
  char c[10];

  if (bytes > 10 || __store_gcov_type ((gcov_type)value, c, bytes))
    return 1;
  else
    return fwrite(c, 1, bytes, file) != bytes;
}

/* Read a quantity containing BYTES bytes from FILE, portably. Return
   a nonzero value if the read fails or if the value will not fit
   in DEST.

   Note that DEST may not be large enough to hold all of the requested
   data, but the function will read BYTES characters anyway.

   BYTES may be a maximum of 10.  */

static int
__read_gcov_type (dest, file, bytes)
     gcov_type *dest;
     FILE *file;
     size_t bytes;
{
  char c[10];

  if (bytes > 10 || fread(c, 1, bytes, file) != bytes)
    return 1;
  else
    return __fetch_gcov_type (dest, c, bytes);
}

static int
__read_long (dest, file, bytes)
     long *dest;
     FILE *file;
     size_t bytes;
{
  char c[10];

  if (bytes > 10 || fread(c, 1, bytes, file) != bytes)
    return 1;
  else
    return __fetch_long (dest, c, bytes);
}


/* Writes string in gcov format.  */

static int
__write_gcov_string (string, length, file, delim)
     const char *string;
     size_t length;
     FILE *file;
     long delim;
{
  size_t temp = length + 1;

  /* delimiter */
  if (__write_long (delim, file, 4) != 0)
    return 1;

  if (__write_long (length, file, 4) != 0)
    return 1;

  if (fwrite (string, temp, 1, file) != 1)
    return 1;

  temp &= 3;

  if (temp)
    {
      char c[4];

      c[0] = c[1] = c[2] = c[3] = 0;

      if (fwrite (c, sizeof (char), 4 - temp, file) != 4 - temp)
    return 1;
    }

  if (__write_long (delim, file, 4) != 0)
    return 1;

  return 0;
}

/* Reads string in gcov format.  */


static int
__read_gcov_string (string, max_length, file, delim)
     char *string;
     size_t max_length;
     FILE *file;
     long delim;
{
  long delim_from_file;
  long length;
  long read_length;
  long tmp;

  if (__read_long (&delim_from_file, file, 4) != 0)
    return 1;

  if (delim_from_file != delim)
    return 1;

  if (__read_long (&length, file, 4) != 0)
    return 1;

  if (length > (long) max_length)
    read_length = max_length;
  else
    read_length = length;

  tmp = (((length + 1) - 1) / 4 + 1) * 4;
  /* This is the size occupied by the string in the file */

  if (fread (string, read_length, 1, file) != 1)
    return 1;

  string[read_length] = 0;

  if (fseek (file, tmp - read_length, SEEK_CUR) < 0)
    return 1;

  if (__read_long (&delim_from_file, file, 4) != 0)
    return 1;

  if (delim_from_file != delim)
    return 1;

  return 0;
}


te_errno
tce_save_data_gcc33(rcf_rpc_server *rpcs, int progno)
{
    unsigned n_objs = 0;
    unsigned i;
    unsigned objno;
    gcov_type program_sum = 0;
    gcov_type program_max = 0;
    long program_arcs = 0;
    gcov_type merged_sum = 0;
    gcov_type merged_max = 0;
    long merged_arcs = 0;
    tce_counter *obj_ctrs;

    tce_read_value(rpcs, TCE_GLOBAL(progno), "n_objects", "%u", &n_objs);
    obj_ctrs = calloc(n_objs, sizeof(*obj_ctrs));

    /* Non-merged stats for this program.  */
    for (objno = 0; objno < n_objs; objno++)
    {
        tce_read_counters(rpcs, progno, objno, 0, &obj_ctrs[objno]);
        for (i = 0; i < obj_ctrs[objno].num; i++)
        {
            program_sum += obj_ctrs[objno].values[i];

            if (obj_ctrs[objno].values[i] > program_max)
                program_max = obj_ctrs[objno].values[i];
        }
        program_arcs += obj_ctrs[objno].num;
    }
  
    for (objno = 0; objno < n_objs; objno++)
    {
        FILE *da_file;
        gcov_type object_max = 0;
        gcov_type object_sum = 0;
        long object_functions = 0;
        int merging = 0;
        int error = 0;
        int fn;
        
        char filename[RCF_MAX_NAME + 1] = "";
      
        tce_read_value(rpcs, TCE_OBJ(progno, objno), "filename", "%s", filename);

        /* Open for modification */
        da_file = fopen (filename, "r+b");
      
        if (da_file)
            merging = 1;
        else
        {
            /* Try for appending */
            da_file = fopen (filename, "ab");
            /* Some old systems might not allow the 'b' mode modifier.
               Therefore, try to open without it.  This can lead to a
               race condition so that when you delete and re-create the
               file, the file might be opened in text mode, but then,
               you shouldn't delete the file in the first place.  */
            if (!da_file)
                da_file = fopen (filename, "a");
        }
      
        if (!da_file)
        {
            ERROR("cannot open TCE file: %s", filename);
            continue;
        }

        tce_read_value(rpcs, TCE_OBJ(progno, objno), "n_functions", 
                       "%ld", &object_functions);

        if (merging)
        {
            /* Merge data from file.  */
            long tmp_long;
            gcov_type tmp_gcov;
      
            if (/* magic */
                (__read_long (&tmp_long, da_file, 4) || tmp_long != -123l)
                /* functions in object file.  */
                || (__read_long (&tmp_long, da_file, 4)
                    || tmp_long != object_functions)
                /* extension block, skipped */
                || (__read_long (&tmp_long, da_file, 4)
                    || fseek (da_file, tmp_long, SEEK_CUR)))
            {
            read_error:;
                ERROR("Error merging `%s'", filename);
                clearerr (da_file);
            }
            else
            {
                /* Merge execution counts for each function.  */
                long long *count_ptr = obj_ctrs[objno].values;
          
                for (fn = 0; fn < object_functions; fn++)
                {
                    char name[64];
                    int arc_count;
                    long checksum;
                    
                    tce_read_value(rpcs, TCE_ARC(progno, objno, fn, 0), "count", "%d", 
                                   &arc_count);
                    tce_read_value(rpcs, TCE_FUN(progno, objno, fn), "name", "%s", 
                                   name);
                    tce_read_value(rpcs, TCE_FUN(progno, objno, fn), "checksum", "%ld", 
                                   &checksum);
                    if (/* function name delim */
                        (__read_long (&tmp_long, da_file, 4)
                         || tmp_long != -1)
                        /* function name length */
                        || (__read_long (&tmp_long, da_file, 4)
                            || tmp_long != (long) strlen (name))
                        /* skip string */
                        || fseek (da_file, ((tmp_long + 1) + 3) & ~3, SEEK_CUR)
                        /* function name delim */
                        || (__read_long (&tmp_long, da_file, 4)
                            || tmp_long != -1))
                        goto read_error;

                    if (/* function checksum */
                        (__read_long (&tmp_long, da_file, 4)
                         || tmp_long != checksum)
                        /* arc count */
                        || (__read_long (&tmp_long, da_file, 4)
                            || tmp_long != arc_count))
                        goto read_error;
          
                    for (i = arc_count; i > 0; i--, count_ptr++)
                        if (__read_gcov_type (&tmp_gcov, da_file, 8))
                            goto read_error;
                        else
                            *count_ptr += tmp_gcov;
                }
            }
            fseek (da_file, 0, SEEK_SET);
        }
      
        /* Calculate the per-object statistics.  */
        for (i = 0; i < obj_ctrs[objno].num; i++)
        {
            object_sum += obj_ctrs[objno].values[i];

            if (obj_ctrs[objno].values[i] > object_max)
                object_max = obj_ctrs[objno].values[i];
        }
        merged_sum += object_sum;
        if (merged_max < object_max)
            merged_max = object_max;
        merged_arcs += obj_ctrs[objno].num;
      
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
            || __write_long (merging ? (long)obj_ctrs[objno].num : program_arcs, da_file, 4)
            /* sum of counters.  */
            || __write_gcov_type (merging ? object_sum : program_sum, da_file, 8)
            /* maximal counter.  */
            || __write_gcov_type (merging ? object_max : program_max, da_file, 8)

            /* per-object statistics.  */
            /* number of counters.  */
            || __write_long (obj_ctrs[objno].num, da_file, 4)
            /* sum of counters.  */
            || __write_gcov_type (object_sum, da_file, 8)
            /* maximal counter.  */
            || __write_gcov_type (object_max, da_file, 8))
        {
        write_error:;
            ERROR(" Error writing output file %s", filename);
            error = 1;
        }
        else
        {
            /* Write execution counts for each function.  */
            long long *count_ptr = obj_ctrs[objno].values;

            for (fn = 0; fn < object_functions; fn++)
            {
                char name[64];
                int arc_count;
                long checksum;
                
                tce_read_value(rpcs, TCE_ARC(progno, objno, fn, 0), "count", "%d", 
                               &arc_count);
                tce_read_value(rpcs, TCE_FUN(progno, objno, fn), "name", "%s", 
                               name);
                tce_read_value(rpcs, TCE_FUN(progno, objno, fn), "checksum", "%ld", 
                               &checksum);
                
                if (__write_gcov_string (name, strlen(name), da_file, -1)
                    || __write_long (checksum, da_file, 4)
                    || __write_long (arc_count, da_file, 4))
                    goto write_error;
          
                for (i = arc_count; i > 0; i--, count_ptr++)
                    if (__write_gcov_type (*count_ptr, da_file, 8))
                        goto write_error; /* RIP Edsger Dijkstra */
            }
        }

        if (fclose (da_file))
        {
            ERROR("Error closing output file %s.\n", filename);
            error = 1;
        }
        if (error || !merging)
        {
            free(obj_ctrs[objno].values);
            obj_ctrs[objno].values = NULL;
            obj_ctrs[objno].num = 0;
        }
    }

    /* Upate whole program statistics.  */
    for (objno = 0; objno < n_objs; objno++)
    {
        if (obj_ctrs[objno].values != NULL)
        {
            FILE *da_file;

            char filename[RCF_MAX_NAME + 1] = "";
      
            tce_read_value(rpcs, TCE_OBJ(progno, i), "filename", "%s", filename);
    
            da_file = fopen (filename, "r+b");
            if (!da_file)
            {
                ERROR("Cannot reopen %s", filename);
                continue;
            }
    
            if (fseek (da_file, 4 * 3, SEEK_SET)
                /* number of instrumented arcs.  */
                || __write_long (merged_arcs, da_file, 4)
                /* sum of counters.  */
                || __write_gcov_type (merged_sum, da_file, 8)
                /* maximal counter.  */
                || __write_gcov_type (merged_max, da_file, 8))
                ERROR("arc profiling: Error updating program header %s",
                      filename);
            if (fclose (da_file))
                ERROR("Error reclosing %s", filename);
        }
    }
    for (i = 0; i < n_objs; i++)
        free(obj_ctrs[i].values);
    free(obj_ctrs);
    return 0;
}
