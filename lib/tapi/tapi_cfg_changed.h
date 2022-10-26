/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test API to track data changes
 *
 * The purpose of the API is to delegate restoring
 * changed data to a prologue/epilogue instead of doing
 * it on the spot in the test.
 *
 * The prototypical usecase for that is as follows:
 * - suppose there's a possibly large NV storage
 * - and there are read-only and read-write tests
 * - naturally, read-only tests would expect some known contents of a
 *   storage, while write tests spoil it
 *
 * It is infeasible to re-initialize the whole device before each session of
 * read-only tests, let alone before each individual test.
 * It is also not very robust to restore the changed data blocks after
 * each write test.
 *
 * The current API would solve this in the following manner:
 * - in a write test after doing a write operation, the changed data region
 *   is recorded:
 * @code{.c}
 * CHECK_RC(rpc_pwrite(rpc, fd, buf, size, offset));
 * tapi_cfg_changed_add_region(rpc->ta, offset, size);
 * @endcode
 *
 * Then in a session prologue or epilogue:
 * @code{.c}
 * static te_errno
 * restore_cb(const char *tag, size_t start, size_t len, void *rpcs)
 * {
 *    CHECK_RC(rpc_write(rpc, known_data, len, start));
 *    return 0;
 * }
 *...
 * CHECK_RC(tapi_cfg_changed_process_regions(rpc->ta, restore_cb, rpcs))
 * @endcode
 */

#ifndef __TE_TAPI_CFG_CHANGED_H__
#define __TE_TAPI_CFG_CHANGED_H__

#include "te_config.h"

#include "te_defs.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_changed Data change tracking
 * @ingroup tapi_conf
 * @{
 */

/**
 * Add a changed region starting at @p start of the length @p len.
 * If there's already a marked region at this point,
 * its length is extended (but never shrunk). No other checks are done,
 * so this function may cause overlapping regions to appear.
 *
 * @param tag    Change scope identifier (i.e. a block device name)
 * @param start  Start of a region
 * @param len    Length of a region
 *
 * @return Status code
 *
 * @note The semantics of region is completely test-specific, they may
 *       be blocks, pages byte ranges or anything else.
 */
extern te_errno tapi_cfg_changed_add_region(const char *tag,
                                            size_t start, size_t len);

/**
 * Like tapi_cfg_changed_add_region(), but properly check for overlapping
 * regions and modify them accordingly.
 *
 * This function is more robust than tapi_cfg_changed_add_region() but
 * is significantly slower because a full list of regions under a given @p
 * tag should be retrieved. Basically it performs a union of already marked
 * regions and an interval @p start .. @p start + @p len.
 *
 * @param tag    Change scope identifier
 * @param start  Start of a region
 * @param len    Length of a region
 *
 * @return Status code

 */
extern te_errno tapi_cfg_changed_add_region_overlap(const char *tag,
                                                    size_t start, size_t len);

/**
 * Remove a changed region of any length that starts at @p start.
 *
 * If there is no region at @p start, the function does nothing.
 * The region must start exactly at @p start, no checks for overlapping
 * regions are performed.
 *
 * @param tag    Change scope identifier
 * @param start  Start of a region
 *
 * @return Status code
 */
extern te_errno tapi_cfg_changed_remove_region(const char *tag, size_t start);

/**
 * Exclude @p start .. @p start + @p length from a list of changed regions.
 *
 * Unlike tapi_cfg_changed_remove_region(), the function performs proper set
 * difference. In particular that may mean that a single region may be split
 * in two.
 *
 * @param tag    Change scope identifier
 * @param start  Start of a region
 * @param len    Length of a region
 *
 * @return Status code
 */
extern te_errno tapi_cfg_changed_remove_region_overlap(const char *tag,
                                                       size_t start,
                                                       size_t len);

/**
 * Type for tapi_cfg_changed_process_regions() callback functions.
 *
 * @param tag    Change scope identifier
 * @param start  Start of a region
 * @param len    Length of a region
 * @param ctx    User data
 *
 * @return Status code
 * @retval 0         tapi_cfg_changed_process_regions() would remove the
 *                   processed region
 * @retval TE_EGAIN  the region would be retained
 */
typedef te_errno tapi_cfg_changed_callback(const char *tag,
                                           size_t start, size_t len, void *ctx);


/**
 * Process all defined regions for a given @p tag calling @p cb on each of
 * them.
 *
 * The regions are processed in the increased order of their start points.
 * The function fixes possible unsigned overflows (e.g. if a length is
 * @c SIZE_MAX) and adjust the lengths to removed overlaps, so the callback
 * may see a different length of a region than stored in the Configurator
 * tree.
 *
 * If the callback returns 0, the current region is removed.
 * If the callback return TE_EAGAIN, the region is retained.
 * All other return values cause processing to stop immediately.
 *
 * @param tag     Change scope identifier
 * @param cb      Callback
 * @param ctx     User data
 *
 * @return Status code
 */
extern te_errno tapi_cfg_changed_process_regions(const char *tag,
                                                 tapi_cfg_changed_callback *cb,
                                                 void *ctx);


/**
 * Remove all changed regions belonging to a given @p tag.
 *
 * @param tag    Change scope identifier
 *
 * @return Status code
 */
extern te_errno tapi_cfg_changed_clear_tag(const char *tag);

/**@} <!-- END tapi_conf_changed --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_CHANGED_H__ */
