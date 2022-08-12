/** @file
 * @brief Log processing
 *
 * This module provides a view structure for log messages and
 * and a set of utility functions.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 *
 */

#ifndef __TE_LOG_MSG_VIEW_H__
#define __TE_LOG_MSG_VIEW_H__

#include "te_defs.h"
#include "te_raw_log.h"
#include "te_string.h"
#include "te_errno.h"

#ifdef _cplusplus
extern "C" {
#endif

/** View structure for log messages. */
typedef struct log_msg_view {
    size_t          length;
    const void     *start;
    te_log_version  version;
    te_log_ts_sec   ts_sec;
    te_log_ts_usec  ts_usec;
    te_log_level    level;
    te_log_id       log_id;
    te_log_nfl      entity_len;
    const char     *entity;
    te_log_nfl      user_len;
    const char     *user;
    te_log_nfl      fmt_len;
    const char     *fmt;
    const void     *args;
} log_msg_view;

/**
 * Parse a raw log message.
 * After a successful call, every pointer in the view structure
 * will point to an address within the [buf; buf + len] region.
 * If an error occurs, the view structure may be filled only partially.
 *
 * @param buf       message buffer
 * @param buf_len   length of message buffer
 * @param view      view structure to be filled
 */
extern te_errno te_raw_log_parse(const void *buf, size_t buf_len,
                                 log_msg_view *view);

/**
 * Expand the message's format string.
 *
 * The result will be appended to the given te_string.
 *
 * @param view      message view
 * @param target    where the result should be placed
 *
 * @returns Status code
 */
extern te_errno te_raw_log_expand(const log_msg_view *view,
                                  te_string *target);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOG_MSG_VIEW_H__ */
