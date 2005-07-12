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

typedef struct bb_function_info 
{
    long checksum;
    int arc_count;
    const char *name;
    long long *counts;
    struct bb_function_info *next;
/* GCC 3.4+ specific fields */
    unsigned ident;
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
    unsigned stamp;
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
    long long *counter;
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

