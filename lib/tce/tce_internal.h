/** @file
 * @brief Tester Subsystem
 *
 * TCE data collector internal interfaces
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

#ifndef  __TE_LIB_TCE_INTERNAL_H
#define  __TE_LIB_TCE_INTERNAL_H 1

/** The following macros are grabbed from gcc-3.4.4 sources,
 *   namely 'gcc/gcov-io.h' and 'gcc/libgcov.c'.
 *   They relate to the structure of the GCOV data file.
 *   They should never be touched; if some future GCC
 *   versions provide different values for them, 
 *   new macros should be created.
 */
#define GCOV_COUNTER_GROUPS 5 
#define GCOV_DATA_MAGIC (0x67636461U) /* "gcda" */
#define GCOV_TAG_FUNCTION    (0x01000000U)
#define GCOV_TAG_FUNCTION_LENGTH (2)
#define GCOV_TAG_COUNTER_BASE    ((unsigned)0x01a10000)
#define GCOV_TAG_COUNTER_LENGTH(NUM) ((NUM) * 2)
#define GCOV_TAG_FOR_COUNTER(COUNT)             \
    (GCOV_TAG_COUNTER_BASE + ((unsigned)(COUNT) << 17))
#define GCOV_TAG_OBJECT_SUMMARY  ((unsigned)0xa1000000)
#define GCOV_TAG_PROGRAM_SUMMARY ((unsigned)0xa3000000)
#define GCOV_TAG_SUMMARY_LENGTH  \
    (1 + (2 + 3 * 2))


/** Merge modes corresponding to different merge
 *   functions in gcc 3.4+ gcov-related code.
 *   See 'gcc/libgcov.c'(__gcov_merge_add,
 *   __gcov_merge_single, __gcov_merge_delta)
 */
enum tce_merge_mode { 
    TCE_MERGE_UNDEFINED, 
    TCE_MERGE_ADD,     /**< == __gcov_merge_add */
    TCE_MERGE_SINGLE,  /**< == __gcov_merge_single */
    TCE_MERGE_DELTA,   /**< == __gcov_merge_delta */
    TCE_MERGE_MAX
};

/** A record for a counter group
    (only relevant for gcc3.4+; for earlier
    versions there's always a single group
    with mode == TCE_MERGE_ADD
 */
typedef struct tce_counter_group {
    unsigned             number;
    enum tce_merge_mode  mode;
} tce_counter_group;

/** A record for a function coverage data */
typedef struct tce_function_info {
    long                      checksum;
    long                      arc_count;
    const char               *name;
    long long                *counts;
    tce_counter_group         groups[GCOV_COUNTER_GROUPS];
    struct tce_function_info *next;
} tce_function_info;

/** A record for an object file coverage data */
typedef struct tce_object_info {
    int                       peer_id;    /**< the data are for that peer */
    const char               *filename;   /**< data filename */
    struct tce_object_info   *next;
    struct tce_function_info *function_infos;
/* The following fields directly map to gcc/gcov internal data.
   See 'bbhook.c'.
 */
    long long                 object_max;
    long long                 object_sum;
    long                      object_functions;
    long long                 program_sum;
    long long                 program_max;
    long                      program_arcs;
    long                      ncounts;
/* GCC-3.4+ specific fields: */
    unsigned                  gcov_version;
    unsigned                  checksum;
    unsigned                  program_checksum;
    unsigned                  ctr_mask;
    unsigned                  stamp;
    unsigned                  program_ncounts;
    long long                 program_sum_max;
    long long                 object_sum_max;
    unsigned                  program_runs;
    unsigned                  object_runs;
} tce_object_info;

typedef struct tce_channel_data tce_channel_data;
typedef void (*tce_state_function)(tce_channel_data *self);

/** TCE collector connection state */
struct tce_channel_data {
    int                    fd;            /**< connection handle */
    tce_state_function     state;         /**< current state */
    char                   buffer[256];   /**< current input line */
    char                  *bufptr;        /**< the pointer beyond the
                                           last byte read in buffer */
    int                    remaining;     /**< bytes remaining in buffer */
    int                    peer_id;       /**< current peer ID */
    tce_object_info       *object;        /**< current object file record */
    tce_function_info     *function;      /**< current function record */
    long long             *counter;       /**< current counter */
    int                    the_group;     /**< current counter group */
    int                    counter_guard; /**< remaining counters */
    tce_channel_data      *next;          /**< chain pointer */
};

/** Reports a TCE error (printf-like interface */
extern void tce_report_error(const char *fmt, ...);

#define tce_report_notice tce_report_error
#define DEBUGGING(x) 

/** Find a record for an object file 'filename', peer 'peer_id'.
 *   If there is no such record, it is created 
 */
extern tce_object_info   *tce_get_object_info(int peer_id, 
                                              const char *filename);

/** Find a record for a function 'name' in a object file denoted by 'oi'.
 *  If there is no such record, it is created and 'arc_count' and
 *  'checksum' fields are set. 
 *   If the record is found, 'arc_count' and 'checksum' must match those
 *   of the record.
 */
extern tce_function_info *tce_get_function_info(tce_object_info *oi,
                                                const char *name, 
                                                long arc_count, 
                                                long checksum);

/**  Sets a name of a file containg kernel symbol table 
 *   a la /proc/kallsyms 
 */
extern void tce_set_ksymtable(char *table);

/** Causes the coverage data from the kernel to be obtained */
extern void tce_obtain_kernel_coverage(void);

#endif  /* __TE_LIB_TCE_INTERNAL_H */
