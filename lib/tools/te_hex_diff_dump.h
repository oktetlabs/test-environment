/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2021-2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief API to log a diff between two binary blocks of memory
 *
 * @defgroup te_tools_te_hex_diff_dump HEX diff dump API
 * @ingroup te_tools
 * @{
 *
 * Functions to format a hex diff dump of two binary memory blocks and log it.
 */

#ifndef __TE_HEX_DIFF_DUMP_H__
#define __TE_HEX_DIFF_DUMP_H__

#include "te_defs.h"
#include "te_string.h"
#include "logger_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Log the difference between buffers with a given log level.
 *
 * @param _lvl      Log level for the diff.
 * @param _expected Buffer with expected data.
 * @param _explen   The length of the expected buffer.
 * @param _actual   Buffer with actual data.
 * @param _actlen   The length of the actual buffer.
 * @param _offset   The offset of the buffers
 *                  (only affects logging).
 *
 * @sa te_hex_diff_log
 */
#define LOG_HEX_DIFF_DUMP_AT(_lvl, _expected, _explen, \
                             _actual, _actlen, _offset)                 \
    TE_DO_IF_LOG_LEVEL((_lvl),                                          \
                    te_hex_diff_log((_expected), (_explen),             \
                                    (_actual), (_actlen),               \
                                    (_offset),                          \
                                    __FILE__, __LINE__, (_lvl),         \
                                    TE_LGR_ENTITY, TE_LGR_USER))

/**
 * Log the difference between buffers with a given log level.
 *
 * The buffers are assumed to be at offset 0 and be of the same
 * length.
 *
 * @param _lvl      Log level for the diff.
 * @param _expected Buffer with expected data.
 * @param _actual   Buffer with actual data.
 * @param _len      Length of buffers.
 *
 * @sa LOG_HEX_DIFF_DUMP_AT
 */
#define LOG_HEX_DIFF_DUMP(_lvl, _expected, _actual, _len) \
    LOG_HEX_DIFF_DUMP_AT((_lvl), (_expected), (_len), (_actual), (_len), 0)

/**
 * Format the difference between @p expected and @p actual
 * and put the dump into @p dest.
 *
 * @param[in]  expected   Buffer with the expected data.
 * @param[in]  exp_len    Length of the expected buffer.
 * @param[in]  actual     Buffer with the actual data.
 * @param[in]  actual_len Length of the actual buffer.
 * @param[in]  offset     The offset of the buffers
 *                        (only affects logging).
 * @param[out] dest       Destination string.
 */
void te_hex_diff_dump(const void *expected, size_t exp_len,
                      const void *actual, size_t actual_len,
                      size_t offset, te_string *dest);

/**
 * Log the difference between @p expected and @p actual with the specified
 * log @p level and @p user.
 *
 * The formatting is done by te_hex_diff_dump().
 *
 * Usually the function shall not be used directly, but through
 * the macro LOG_HEX_DIFF_DUMP_AT() or LOG_HEX_DIFF_DUMP().
 *
 * @param expected   Buffer with the expected data.
 * @param exp_len    Length of the expected buffer.
 * @param actual     Buffer with the actual data.
 * @param actual_len Length of the actual buffer.
 * @param offset     The offset of the buffers
 *                   (only affects logging).
 * @param level      Log level
 * @param entity     Log entity
 * @param user       Log user
 */
void te_hex_diff_log(const void *expected, size_t exp_len,
                     const void *actual, size_t actual_len,
                     size_t offset,
                     const char *file, int line, unsigned int level,
                     const char *entity, const char *user);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_HEX_DIFF_DUMP_H__ */
/**@} <!-- END te_tools_te_hex_diff_dump --> */
