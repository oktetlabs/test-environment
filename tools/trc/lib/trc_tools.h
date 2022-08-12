/** @file
 * @brief Testing Results Comparator: diff tool
 *
 * Definition of TRC diff tool types and related routines.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
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
