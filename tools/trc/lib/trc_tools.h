/** @file
 * @brief Testing Results Comparator: diff tool
 *
 * Definition of TRC diff tool types and related routines.
 *
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id: $
 */

#ifndef __TE_TRC_TOOLS_H__
#define __TE_TRC_TOOLS_H__

#include "te_defs.h"
#include "te_queue.h"
#include "tq_string.h"
#include "te_errno.h"
#include "te_trc.h"
#include "trc_db.h"
#include "trc_report.h"

#ifdef __cplusplus
extern "C" {
#endif

extern te_errno trc_tools_filter_db(te_trc_db *db,
                                    unsigned int *db_uids,
                                    int db_uids_size,
                                    tqh_strings *tests_include,
                                    tqh_strings *tests_exclude);

extern te_errno trc_tools_cut_db(te_trc_db *db, unsigned int db_uid,
                                 const char *path_pattern, te_bool inverse);

extern te_errno trc_tools_merge_db(te_trc_db *db, int dst_uid,
                                   int src_uid1, int src_uid2);

extern te_errno trc_report_merge(trc_report_ctx *ctx, const char *filename);

/**
 * Copy all content of one file to another.
 *
 * @param dst   Destination file
 * @param src   Source file
 *
 * @return      Status code.
 */
extern int trc_tools_file_to_file(FILE *dst, FILE *src);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_TOOLS_H__ */
