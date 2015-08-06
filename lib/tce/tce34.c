/** @file
 * Retrieve TCE data for gcc >= 3.4
 * 
 *
 * Copyright (C) 2007 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 * Copyright (C) 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999,
 * 2000, 2001, 2002, 2003  Free Software Foundation, Inc.
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

/* This file is based on a libgcov.c file that is part of the GCC 3.4 */

#include "te_config.h"
#include <inttypes.h>

#include "tapi_rpc_unistd.h"

#include "tce_internal.h"

/* About the target */

typedef uint32_t gcov_unsigned_t;
typedef uint32_t gcov_position_t;
typedef int64_t gcov_type;

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
#define GCOV_COUNTER_V_INTERVAL 1  /* Histogram of value inside an interval.  */
#define GCOV_COUNTER_V_POW2 2  /* Histogram of exact power2 logarithm
                      of a value.  */
#define GCOV_COUNTER_V_SINGLE   3  /* The most common value of expression.  */
#define GCOV_COUNTER_V_DELTA    4  /* The most common difference between
                      consecutive values of expression.  */
#define GCOV_LAST_VALUE_COUNTER 4  /* The last of counters used for value
                      profiling.  */
#define GCOV_COUNTERS       5

/* Number of counters used for value profiling.  */
#define GCOV_N_VALUE_COUNTERS \
  (GCOV_LAST_VALUE_COUNTER - GCOV_FIRST_VALUE_COUNTER + 1)
  
  /* A list of human readable names of the counters */
#define GCOV_COUNTER_NAMES  {"arcs", "interval", "pow2", "single", "delta"}
  
  /* Names of merge functions for counters.  */
#define GCOV_MERGE_FUNCTIONS    {"__gcov_merge_add",    \
                 "__gcov_merge_add",    \
                 "__gcov_merge_add",    \
                 "__gcov_merge_single", \
                 "__gcov_merge_delta"}
  
/* Convert a counter index to a tag.  */
#define GCOV_TAG_FOR_COUNTER(COUNT)             \
    (GCOV_TAG_COUNTER_BASE + ((gcov_unsigned_t)(COUNT) << 17))
/* Convert a tag to a counter.  */
#define GCOV_COUNTER_FOR_TAG(TAG)                   \
    ((unsigned)(((TAG) - GCOV_TAG_COUNTER_BASE) >> 17))
/* Check whether a tag is a counter tag.  */
#define GCOV_TAG_IS_COUNTER(TAG)                \
    (!((TAG) & 0xFFFF) && GCOV_COUNTER_FOR_TAG (TAG) < GCOV_COUNTERS)

/* The tag level mask has 1's in the position of the inner levels, &
   the lsb of the current level, and zero on the current and outer
   levels.  */
#define GCOV_TAG_MASK(TAG) (((TAG) - 1) ^ (TAG))

/* Return nonzero if SUB is an immediate subtag of TAG.  */
#define GCOV_TAG_IS_SUBTAG(TAG,SUB)             \
    (GCOV_TAG_MASK (TAG) >> 8 == GCOV_TAG_MASK (SUB)    \
     && !(((SUB) ^ (TAG)) & ~GCOV_TAG_MASK(TAG)))

/* Return nonzero if SUB is at a sublevel to TAG.  */
#define GCOV_TAG_IS_SUBLEVEL(TAG,SUB)               \
        (GCOV_TAG_MASK (TAG) > GCOV_TAG_MASK (SUB))

/* Basic block flags.  */
#define GCOV_BLOCK_UNEXPECTED   (1 << 1)

/* Arc flags.  */
#define GCOV_ARC_ON_TREE    (1 << 0)
#define GCOV_ARC_FAKE       (1 << 1)
#define GCOV_ARC_FALLTHROUGH    (1 << 2)

/* Structured records.  */

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

/* The merge function that just sums the counters.  */
static void __gcov_merge_add (gcov_type *, unsigned);

/* The merge function to choose the most common value.  */
static void __gcov_merge_single (gcov_type *, unsigned);

/* The merge function to choose the most common difference between
   consecutive values.  */

static void __gcov_merge_delta (gcov_type *, unsigned);

/* Optimum number of gcov_unsigned_t's read from or written to disk.  */
#define GCOV_BLOCK_SIZE (1 << 10)

struct gcov_var
{
  FILE *file;
  gcov_position_t start;    /* Position of first byte of block */
  unsigned offset;      /* Read/write position within the block.  */
  unsigned length;      /* Read limit in the block.  */
  unsigned overread;        /* Number of words overread.  */
  int error;            /* < 0 overflow, > 0 disk error.  */
  int mode;                 /* < 0 writing, > 0 reading */
  /* Holds one block plus 4 bytes, thus all coverage reads & writes
     fit within this buffer and we always can transfer GCOV_BLOCK_SIZE
     to and from the disk. libgcov never backtracks and only writes 4
     or 8 byte objects.  */
  gcov_unsigned_t buffer[GCOV_BLOCK_SIZE + 1];
} gcov_var;

/* Functions for reading and writing gcov files. In libgcov you can
   open the file for reading then writing. Elsewhere you can open the
   file either for reading or for writing. When reading a file you may
   use the gcov_read_* functions, gcov_sync, gcov_position, &
   gcov_error. When writing a file you may use the gcov_write
   functions, gcov_seek & gcov_error. When a file is to be rewritten
   you use the functions for reading, then gcov_rewrite then the
   functions for writing.  Your file may become corrupted if you break
   these invariants.  */
static int gcov_open (const char *name);
static int gcov_close (void);

/* Available everywhere.  */
static gcov_position_t gcov_position (void);
static int gcov_is_error (void);
static int gcov_is_eof (void);

static gcov_unsigned_t gcov_read_unsigned (void);
static gcov_type gcov_read_counter (void);
static void gcov_read_summary (struct gcov_summary *);

/* Available only in libgcov */
static void gcov_write_counter (gcov_type);
static void gcov_write_tag_length (gcov_unsigned_t, gcov_unsigned_t);
static void gcov_write_summary (gcov_unsigned_t /*tag*/,
                      const struct gcov_summary *);
static void gcov_truncate (void);
static void gcov_rewrite (void);
static void gcov_seek (gcov_position_t /*position*/);

/* Available outside gcov */
static void gcov_write_unsigned (gcov_unsigned_t);

/* Make sure the library is used correctly.  */
#if ENABLE_CHECKING
#define GCOV_CHECK(expr) ((expr) ? (void)0 : (void)abort ())
#else
#define GCOV_CHECK(expr)
#endif
#define GCOV_CHECK_READING() GCOV_CHECK(gcov_var.mode > 0)
#define GCOV_CHECK_WRITING() GCOV_CHECK(gcov_var.mode < 0)

/* Save the current position in the gcov file.  */

static inline gcov_position_t
gcov_position (void)
{
  GCOV_CHECK_READING ();
  return gcov_var.start + gcov_var.offset;
}

/* Return nonzero if we read to end of file.  */

static inline int
gcov_is_eof (void)
{
  return !gcov_var.overread;
}

/* Return nonzero if the error flag is set.  */

static inline int
gcov_is_error (void)
{
  return gcov_var.file ? gcov_var.error : 1;
}

/* Move to beginning of file and initialize for writing.  */

static inline void
gcov_rewrite (void)
{
  GCOV_CHECK_READING ();
  gcov_var.mode = -1;
  gcov_var.start = 0;
  gcov_var.offset = 0;
  fseek (gcov_var.file, 0L, SEEK_SET);
}

static inline void
gcov_truncate (void)
{
  ftruncate (fileno (gcov_var.file), 0L);
}

static void gcov_write_block (unsigned);
static gcov_unsigned_t *gcov_write_words (unsigned);
static const gcov_unsigned_t *gcov_read_words (unsigned);

static inline gcov_unsigned_t from_file (gcov_unsigned_t value)
{
    return value;
}

/* Open a gcov file. NAME is the name of the file to open and MODE
   indicates whether a new file should be created, or an existing file
   opened for modification. If MODE is >= 0 an existing file will be
   opened, if possible, and if MODE is <= 0, a new file will be
   created. Use MODE=0 to attempt to reopen an existing file and then
   fall back on creating a new one.  Return zero on failure, >0 on
   opening an existing file and <0 on creating a new one.  */

static int
gcov_open (const char *name)
{
    if (gcov_var.file)
        abort ();
    gcov_var.start = 0;
    gcov_var.offset = gcov_var.length = 0;
    gcov_var.overread = -1u;
    gcov_var.error = 0;

    gcov_var.file = fopen (name, "r+b");
    if (gcov_var.file)
        gcov_var.mode = 1;
    else 
    {
        gcov_var.file = fopen (name, "w+b");
        if (gcov_var.file)
            gcov_var.mode = 1;
    }
    if (!gcov_var.file)
        return 0;
    
    setbuf (gcov_var.file, (char *)0);
    
    return 1;
}

/* Close the current gcov file. Flushes data to disk. Returns nonzero
   on failure or error flag set.  */

static int
gcov_close (void)
{
  if (gcov_var.file)
  {
      if (gcov_var.offset && gcov_var.mode < 0)
          gcov_write_block (gcov_var.offset);
      fclose (gcov_var.file);
      gcov_var.file = 0;
      gcov_var.length = 0;
  }
  gcov_var.mode = 0;
  return gcov_var.error;
}

/* Write out the current block, if needs be.  */

static void
gcov_write_block (unsigned size)
{
  if (fwrite (gcov_var.buffer, size << 2, 1, gcov_var.file) != 1)
    gcov_var.error = 1;
  gcov_var.start += size;
  gcov_var.offset -= size;
}

/* Allocate space to write BYTES bytes to the gcov file. Return a
   pointer to those bytes, or NULL on failure.  */

static gcov_unsigned_t *
gcov_write_words (unsigned words)
{
  gcov_unsigned_t *result;

  GCOV_CHECK_WRITING ();
  if (gcov_var.offset >= GCOV_BLOCK_SIZE)
  {
      gcov_write_block (GCOV_BLOCK_SIZE);
      if (gcov_var.offset)
      {
          GCOV_CHECK (gcov_var.offset == 1);
          memcpy (gcov_var.buffer, gcov_var.buffer + GCOV_BLOCK_SIZE, 4);
      }
  }
  result = &gcov_var.buffer[gcov_var.offset];
  gcov_var.offset += words;
  
  return result;
}

/* Write unsigned VALUE to coverage file.  Sets error flag
   appropriately.  */

static void
gcov_write_unsigned (gcov_unsigned_t value)
{
  gcov_unsigned_t *buffer = gcov_write_words (1);

  buffer[0] = value;
}

/* Write counter VALUE to coverage file.  Sets error flag
   appropriately.  */

static void
gcov_write_counter (gcov_type value)
{
  gcov_unsigned_t *buffer = gcov_write_words (2);

  buffer[0] = (gcov_unsigned_t) value;
  if (sizeof (value) > sizeof (gcov_unsigned_t))
    buffer[1] = (gcov_unsigned_t) (value >> 32);
  else
    buffer[1] = 0;
  
  if (value < 0)
    gcov_var.error = -1;
}

/* Write a tag TAG and length LENGTH.  */

static void
gcov_write_tag_length (gcov_unsigned_t tag, gcov_unsigned_t length)
{
  gcov_unsigned_t *buffer = gcov_write_words (2);

  buffer[0] = tag;
  buffer[1] = length;
}

/* Write a summary structure to the gcov file.  Return nonzero on
   overflow.  */

static void
gcov_write_summary (gcov_unsigned_t tag, const struct gcov_summary *summary)
{
  unsigned ix;
  const struct gcov_ctr_summary *csum;

  gcov_write_tag_length (tag, GCOV_TAG_SUMMARY_LENGTH);
  gcov_write_unsigned (summary->checksum);
  for (csum = summary->ctrs, ix = GCOV_COUNTERS_SUMMABLE; ix--; csum++)
    {
      gcov_write_unsigned (csum->num);
      gcov_write_unsigned (csum->runs);
      gcov_write_counter (csum->sum_all);
      gcov_write_counter (csum->run_max);
      gcov_write_counter (csum->sum_max);
    }
}

/* Return a pointer to read BYTES bytes from the gcov file. Returns
   NULL on failure (read past EOF).  */

static const gcov_unsigned_t *
gcov_read_words (unsigned words)
{
  const gcov_unsigned_t *result;
  unsigned excess = gcov_var.length - gcov_var.offset;
  
  GCOV_CHECK_READING ();
  if (excess < words)
  {
      gcov_var.start += gcov_var.offset;
      if (excess)
      {
          GCOV_CHECK (excess == 1);
          memcpy (gcov_var.buffer, gcov_var.buffer + gcov_var.offset, 4);
      }

      gcov_var.offset = 0;
      gcov_var.length = excess;
      GCOV_CHECK (!gcov_var.length || gcov_var.length == 1);
      excess = GCOV_BLOCK_SIZE;
      excess = fread (gcov_var.buffer + gcov_var.length,
              1, excess << 2, gcov_var.file) >> 2;
      gcov_var.length += excess;
      if (gcov_var.length < words)
      {
          gcov_var.overread += words - gcov_var.length;
          gcov_var.length = 0;
          return 0;
      }
  }
  result = &gcov_var.buffer[gcov_var.offset];
  gcov_var.offset += words;
  return result;
}

/* Read unsigned value from a coverage file. Sets error flag on file
   error, overflow flag on overflow */

static gcov_unsigned_t
gcov_read_unsigned (void)
{
  gcov_unsigned_t value;
  const gcov_unsigned_t *buffer = gcov_read_words (1);

  if (!buffer)
    return 0;
  value = from_file (buffer[0]);
  return value;
}

/* Read counter value from a coverage file. Sets error flag on file
   error, overflow flag on overflow */

static gcov_type
gcov_read_counter (void)
{
  gcov_type value;
  const gcov_unsigned_t *buffer = gcov_read_words (2);

  if (!buffer)
    return 0;
  value = from_file (buffer[0]);
  if (sizeof (value) > sizeof (gcov_unsigned_t))
    value |= ((gcov_type) from_file (buffer[1])) << 32;
  else if (buffer[1])
    gcov_var.error = -1;
  
  if (value < 0)
    gcov_var.error = -1;
  return value;
}

/* Read string from coverage file. Returns a pointer to a static
   buffer, or NULL on empty string. You must copy the string before
   calling another gcov function.  */

void
gcov_read_summary (struct gcov_summary *summary)
{
  unsigned ix;
  struct gcov_ctr_summary *csum;
  
  summary->checksum = gcov_read_unsigned ();
  for (csum = summary->ctrs, ix = GCOV_COUNTERS_SUMMABLE; ix--; csum++)
    {
      csum->num = gcov_read_unsigned ();
      csum->runs = gcov_read_unsigned ();
      csum->sum_all = gcov_read_counter ();
      csum->run_max = gcov_read_counter ();
      csum->sum_max = gcov_read_counter ();
    }
}

/* Move to the a set position in a gcov file.  BASE is zero to move to
   the end, and nonzero to move to that position.  */

static void
gcov_seek (gcov_position_t base)
{
  GCOV_CHECK_WRITING ();
  if (gcov_var.offset)
    gcov_write_block (gcov_var.offset);
  fseek (gcov_var.file, base << 2, base ? SEEK_SET : SEEK_END);
  gcov_var.start = ftell (gcov_var.file) >> 2;
}

te_errno
tce_save_data_gcc34(rcf_rpc_server *rpcs, int progno, unsigned version)
{
    int n_objs;
    int objno = 0;
    struct gcov_summary this_program;
    struct gcov_summary all;
    struct gcov_ctr_summary *cs_ptr;
    unsigned t_ix;
    unsigned ctr_base;
    unsigned ci;
    gcov_unsigned_t gcov_crc32;
    gcov_unsigned_t c_num;
    tce_counter *obj_ctrs;
    static void (*merge_functions[])(gcov_type *, unsigned) = 
        {__gcov_merge_add, __gcov_merge_single, __gcov_merge_delta};

    memset (&all, 0, sizeof (all));
    /* Find the totals for this execution.  */
    memset (&this_program, 0, sizeof (this_program));

    tce_read_value(rpcs, TCE_GLOBAL(progno), "n_objects", "%u", &n_objs);
    obj_ctrs = calloc(n_objs * GCOV_COUNTERS, sizeof(*obj_ctrs));

    for (objno = 0, ctr_base = 0; 
         objno < n_objs; 
         objno++, ctr_base += GCOV_COUNTERS)
    {
        unsigned ctr_mask;
        
        tce_read_value(rpcs, TCE_OBJ(progno, objno), "ctr_mask", "%x", &ctr_mask);
        for (t_ix = 0, ci = 0; t_ix < GCOV_COUNTERS_SUMMABLE; t_ix++)
        {
            if (!((1 << t_ix) & ctr_mask))
                continue;

            tce_read_counters(rpcs, progno, objno, ci, 
                              &obj_ctrs[ctr_base + t_ix]);
        
            cs_ptr = &this_program.ctrs[t_ix];
            cs_ptr->num += obj_ctrs[ctr_base + t_ix].num;
            for (c_num = 0; c_num < obj_ctrs[ctr_base + t_ix].num; c_num++)
            {
                cs_ptr->sum_all += obj_ctrs[ctr_base + t_ix].values[c_num];
                if (cs_ptr->run_max < obj_ctrs[ctr_base + t_ix].values[c_num])
                    cs_ptr->run_max = obj_ctrs[ctr_base + t_ix].values[c_num];
            }
            ci++;
        }
    }

    tce_read_value(rpcs, TCE_GLOBAL(progno), "crc", "%u", &gcov_crc32);

    /* Now merge each file.  */
    for (objno = 0, ctr_base = 0; objno < n_objs; objno++, ctr_base += GCOV_COUNTERS)
    {
        struct gcov_summary this_object;
        struct gcov_summary object, program;
        gcov_type *values[GCOV_COUNTERS];
        unsigned c_ix, f_ix, n_counts;
        struct gcov_ctr_summary *cs_obj, *cs_tobj, *cs_prg, *cs_tprg, *cs_all;
        int error = 0;
        gcov_unsigned_t tag, length;
        gcov_position_t summary_pos = 0;
        unsigned stamp;
        char filename[RCF_MAX_PATH + 1];
        unsigned n_functions;
        unsigned ctr_mask;

        tce_read_value(rpcs, TCE_OBJ(progno, objno), "ctr_mask", "%x", &ctr_mask);
        tce_read_value(rpcs, TCE_OBJ(progno, objno), "stamp", "%x", &stamp);
        tce_read_value(rpcs, TCE_OBJ(progno, objno), "filename", "%s", filename);
        tce_read_value(rpcs, TCE_OBJ(progno, objno), "n_functions", "%u", &n_functions);

        memset (&this_object, 0, sizeof (this_object));
        memset (&object, 0, sizeof (object));
      
        /* Totals for this object file.  */
        for (t_ix = 0; t_ix < GCOV_COUNTERS_SUMMABLE; t_ix++)
        {
            if (!((1 << t_ix) & ctr_mask))
                continue;

            cs_ptr = &this_object.ctrs[t_ix];
            cs_ptr->num += obj_ctrs[ctr_base + t_ix].num;
            for (c_num = 0; c_num < obj_ctrs[ctr_base + t_ix].num; c_num++)
            {
                cs_ptr->sum_all += obj_ctrs[ctr_base + t_ix].values[c_num];
                if (cs_ptr->run_max < obj_ctrs[ctr_base + t_ix].values[c_num])
                    cs_ptr->run_max = obj_ctrs[ctr_base + t_ix].values[c_num];
            }

        }

        c_ix = 0;
        for (t_ix = 0; t_ix < GCOV_COUNTERS; t_ix++)
        {
            if ((1 << t_ix) & ctr_mask)
            {
                if (t_ix >= GCOV_COUNTERS_SUMMABLE)
                {
                    tce_read_counters(rpcs, progno, objno, c_ix, 
                                   &obj_ctrs[ctr_base + t_ix]);
                }
                
                values[c_ix] = obj_ctrs[ctr_base + t_ix].values;
                c_ix++;
            }
        }

        if (!gcov_open (filename))
        {
            ERROR("cannot open %s", filename);
            continue;
        }

        tag = gcov_read_unsigned ();
        if (tag)
        {
            /* Merge data from file.  */
            if (tag != GCOV_DATA_MAGIC)
            {
                ERROR("Not a gcov data file: %s", filename);
            read_fatal:;
                gcov_close ();
                continue;
            }
            length = gcov_read_unsigned ();

            length = gcov_read_unsigned ();
            if (length != stamp)
            {
                /* Read from a different compilation. Overwrite the
                   file.  */
                gcov_truncate ();
                goto rewrite;
            }
      
            /* Merge execution counts for each function.  */
            for (f_ix = 0; f_ix < n_functions; f_ix++)
            {
                unsigned ident;
                unsigned checksum;
                
                tce_read_value(rpcs, TCE_FUN(progno, objno, f_ix), "ident", "%x", &ident);
                tce_read_value(rpcs, TCE_FUN(progno, objno, f_ix), "checksum", "%x", &checksum);

                tag = gcov_read_unsigned ();
                length = gcov_read_unsigned ();

                /* Check function.  */
                if (tag != GCOV_TAG_FUNCTION
                    || length != GCOV_TAG_FUNCTION_LENGTH
                    || gcov_read_unsigned () != ident
                    || gcov_read_unsigned () != checksum)
                {
                read_mismatch:;
                    ERROR("profiling:%s:Merge mismatch for %s\n",
                          filename,
                          f_ix + 1 ? "function" : "summaries");
                    goto read_fatal;
                }

                c_ix = 0;
                for (t_ix = 0; t_ix < GCOV_COUNTERS; t_ix++)
                {
                    int merger;
                    
                    if (!((1 << t_ix) & ctr_mask))
                        continue;
          
                    tce_read_value(rpcs, TCE_ARC(progno, objno, f_ix, c_ix), "count", "%u", &n_counts);
                    tce_read_value(rpcs, TCE_CTR(progno, objno, c_ix), "merger", "%d", &merger);

                    if (merger >= 0)
                    {
                        tag = gcov_read_unsigned ();
                        length = gcov_read_unsigned ();
                        if (tag != GCOV_TAG_FOR_COUNTER (t_ix)
                            || length != GCOV_TAG_COUNTER_LENGTH (n_counts))
                            goto read_mismatch;
                        (merge_functions[merger])(values[c_ix], n_counts);
                        values[c_ix] += n_counts;
                        c_ix++;
                    }
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
            ERROR(error < 0 ? "profiling:%s:Overflow merging\n"
                  : "profiling:%s:Error merging\n", filename);
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

            if ((1 << t_ix) & ctr_mask)
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
            else if (!all.checksum && memcmp (cs_all, cs_prg, sizeof (*cs_all)))
            {
                ERROR("profiling:%s:Invocation mismatch - some data files may have been removed",
                      filename);
                all.checksum = ~0u;
            }
        }
      
        c_ix = 0;
        for (t_ix = 0; t_ix < GCOV_COUNTERS; t_ix++)
        {
            if ((1 << t_ix) & ctr_mask)
            {
                values[c_ix] = obj_ctrs[ctr_base + t_ix].values;
                c_ix++;
            }
        }

        /* Write out the data.  */
        gcov_write_tag_length (GCOV_DATA_MAGIC, version);
        gcov_write_unsigned (stamp);
      
        /* Write execution counts for each function.  */
        for (f_ix = 0; f_ix < n_functions; f_ix++)
        {
            unsigned ident;
            unsigned checksum;
            
            tce_read_value(rpcs, TCE_FUN(progno, objno, f_ix), "ident", "%x", &ident);
            tce_read_value(rpcs, TCE_FUN(progno, objno, f_ix), "checksum", "%x", &checksum);

            /* Announce function.  */
            gcov_write_tag_length (GCOV_TAG_FUNCTION, GCOV_TAG_FUNCTION_LENGTH);
            gcov_write_unsigned (ident);
            gcov_write_unsigned (checksum);

            c_ix = 0;
            for (t_ix = 0; t_ix < GCOV_COUNTERS; t_ix++)
            {
                gcov_type *c_ptr;
                unsigned n_counts;

                if (!((1 << t_ix) & ctr_mask))
                    continue;

                tce_read_value(rpcs, TCE_ARC(progno, objno, f_ix, c_ix), 
                               "count", "%u", &n_counts);
            
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
        {
            ERROR(error  < 0 ? "profiling:%s:Overflow writing\n" :
                  "profiling:%s:Error writing\n",
                  filename);
        }
    }
    return 0;
}

/* The profile merging function that just adds the counters.  It is given
   an array COUNTERS of N_COUNTERS old counters and it reads the same number
   of counters from the gcov file.  */
void
__gcov_merge_add (gcov_type *counters, unsigned n_counters)
{
    for (; n_counters; counters++, n_counters--)
        *counters += gcov_read_counter ();
}

/* The profile merging function for choosing the most common value.
   It is given an array COUNTERS of N_COUNTERS old counters and it
   reads the same number of counters from the gcov file.  The counters
   are split into 3-tuples where the members of the tuple have
   meanings:
   
   -- the stored candidate on the most common value of the measured entity
   -- counter
   -- total number of evaluations of the value  */
void
__gcov_merge_single (gcov_type *counters, unsigned n_counters)
{
    unsigned i, n_measures;
    gcov_type value, counter, all;

    GCOV_CHECK (!(n_counters % 3));
    n_measures = n_counters / 3;
    for (i = 0; i < n_measures; i++, counters += 3)
    {
        value = gcov_read_counter ();
        counter = gcov_read_counter ();
        all = gcov_read_counter ();

        if (counters[0] == value)
            counters[1] += counter;
        else if (counter > counters[1])
        {
            counters[0] = value;
            counters[1] = counter - counters[1];
        }
        else
            counters[1] -= counter;
        counters[2] += all;
    }
}

/* The profile merging function for choosing the most common
   difference between two consecutive evaluations of the value.  It is
   given an array COUNTERS of N_COUNTERS old counters and it reads the
   same number of counters from the gcov file.  The counters are split
   into 4-tuples where the members of the tuple have meanings:
   
   -- the last value of the measured entity
   -- the stored candidate on the most common difference
   -- counter
   -- total number of evaluations of the value  */
void
__gcov_merge_delta (gcov_type *counters, unsigned n_counters)
{
    unsigned i, n_measures;
    gcov_type value, counter, all;

    GCOV_CHECK (!(n_counters % 4));
    n_measures = n_counters / 4;
    for (i = 0; i < n_measures; i++, counters += 4)
    {
        gcov_read_counter ();
        value = gcov_read_counter ();
        counter = gcov_read_counter ();
        all = gcov_read_counter ();

        if (counters[1] == value)
            counters[2] += counter;
        else if (counter > counters[2])
        {
            counters[1] = value;
            counters[2] = counter - counters[2];
        }
        else
            counters[2] -= counter;
        counters[3] += all;
    }
}
