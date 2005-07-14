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

#define GCOV_COUNTER_GROUPS 5 /* this value is defined by gcc */
#define GCOV_DATA_MAGIC (0x67636461U) /* "gcda" */
#define GCOV_TAG_FUNCTION    (0x01000000U)
#define GCOV_TAG_FUNCTION_LENGTH (2)
#define GCOV_TAG_COUNTER_BASE 	 ((unsigned)0x01a10000)
#define GCOV_TAG_COUNTER_LENGTH(NUM) ((NUM) * 2)
#define GCOV_TAG_FOR_COUNTER(COUNT)				\
	(GCOV_TAG_COUNTER_BASE + ((unsigned)(COUNT) << 17))
#define GCOV_TAG_OBJECT_SUMMARY  ((unsigned)0xa1000000)
#define GCOV_TAG_PROGRAM_SUMMARY ((unsigned)0xa3000000)
#define GCOV_TAG_SUMMARY_LENGTH  \
	(1 + (2 + 3 * 2))


enum gcov_merge_mode { GCOV_MERGE_UNDEFINED, 
                       GCOV_MERGE_ADD, GCOV_MERGE_SINGLE, GCOV_MERGE_DELTA };

typedef struct bb_counter_group
{
    unsigned number;
    enum gcov_merge_mode mode;
} bb_counter_group;

typedef struct bb_function_info 
{
    long checksum;
    long arc_count;
    const char *name;
    long long *counts;
    bb_counter_group groups[GCOV_COUNTER_GROUPS];
    struct bb_function_info *next;
} bb_function_info;

typedef struct bb_object_info
{
    int peer_id;
    const char *filename;
    long long object_max;
    long long object_sum;
    long object_functions;
    long long program_sum;
    long long program_max;
    long program_arcs;
    long ncounts;
    struct bb_object_info *next;
    struct bb_function_info *function_infos;
/* GCC-3.4+ specific fields: */
    unsigned gcov_version;
    unsigned checksum;
    unsigned program_checksum;
    unsigned ctr_mask;
    unsigned stamp;
    unsigned program_ncounts;
    long long program_sum_max;
    long long object_sum_max;
    unsigned program_runs;
    unsigned object_runs;
} bb_object_info;


typedef struct channel_data channel_data;
struct channel_data
{
    int fd;
    void (*state)(channel_data *me);
    char buffer[256];
    char *bufptr;
    int remaining;
    int peer_id;
    bb_object_info *object;
    bb_function_info *function;
    long long *counter;
    int the_group;
    int counter_guard;
    channel_data *next;
};

extern void tce_report_error(const char *fmt, ...);

#define tce_report_notice tce_report_error
#define DEBUGGING(x)

extern bb_object_info   *get_object_info(int peer_id, 
                                         const char *filename);
extern bb_function_info *get_function_info(bb_object_info *oi,
                                           const char *name, 
                                           long arc_count, long checksum);

extern void tce_set_ksymtable(char *table);
extern void get_kernel_coverage(void);

