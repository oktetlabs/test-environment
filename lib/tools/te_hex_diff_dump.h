/** @file
 * @brief API to log a diff between two binary blocks of memory
 *
 * @defgroup te_tools_te_hex_diff_dump HEX diff dump API
 * @ingroup te_tools
 * @{
 *
 * Functions to create a hex diff dump of two binary memory blocks and log it.
 *
 * Copyright (C) 2021-2022 OKTET Labs. All rights reserved.
 *
 */

#ifndef __TE_HEX_DIFF_DUMP_H__
#define __TE_HEX_DIFF_DUMP_H__

#include "te_defs.h"
#include "te_stdint.h"
#include "logger_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Wrapper over @ref te_hex_diff_dump to print diff via Logger.
 *
 * @param _lvl     Log level for the diff.
 * @param _exp_buf Buffer with expected data.
 * @param _rx_buf  Buffer with actual data.
 * @param _len     Length of buffers.
 */
#define LOG_HEX_DIFF_DUMP(_lvl, _exp_buf, _rx_buf, _len)            \
    do {                                                            \
        int log_func(const char *s) {LOG_MSG(_lvl, s); return 0;}   \
        te_hex_diff_dump(_exp_buf, _rx_buf, _len, log_func);        \
    } while (0);

/**
 * Compare two memory chunks of size @b size and log the diff via @b log_func.
 *
 * @param ex_pkt   Buffer with expected data.
 * @param rx_pkt   Buffer with actual data.
 * @param size     Size of buffers.
 * @param log_func Function to log the diff output, which argument is a text
 *                 string without LF symbol.
 */
void te_hex_diff_dump(const void *ex_pkt, const void *rx_pkt, size_t size,
                      int (*log_func)(const char *s));

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __TE_HEX_DIFF_DUMP_H__ */
/**@} <!-- END te_tools_te_hex_diff_dump --> */
