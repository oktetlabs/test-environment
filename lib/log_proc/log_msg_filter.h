/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Log processing
 *
 * This module provides a view structure for log messages and
 * and a set of utility functions.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_LOG_MSG_FILTER_H__
#define __TE_LOG_MSG_FILTER_H__

#include <pcre.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_raw_log.h"
#include "log_msg_view.h"

#ifdef _cplusplus
extern "C" {
#endif

#define LOG_LEVEL_ALL 0xffff

/** Possible verdicts */
typedef enum log_filter_result {
    LOG_FILTER_PASS,   /**< Log message should be included */
    LOG_FILTER_FAIL,   /**< Log message should be rejected */
    LOG_FILTER_DEFAULT /**< Use some default mode for filtering */
} log_filter_result;

/** User filter */
typedef struct log_user_filter {
    SLIST_ENTRY(log_user_filter) links; /**< Pointer to the next rule */

    te_log_level  level; /**< Bitmask for allowed log levels */
    char         *name;  /**< User name or PCRE pattern */
    pcre         *regex; /**< Compiled PCRE or NULL */
} log_user_filter;

/** Entity filter */
typedef struct log_entity_filter {
    SLIST_ENTRY(log_entity_filter) links; /**< Pointer to the next rule */
    SLIST_HEAD(, log_user_filter)  users; /**< List of user filters */

    te_log_level           level; /**< Bitmask for allowed log levels */
    char                  *name;  /**< Entity name or PCRE pattern */
    pcre                  *regex; /**< Compiled PCRE or NULL */
} log_entity_filter;

/** Message filter */
typedef struct log_msg_filter {
    SLIST_HEAD(, log_entity_filter) entities; /**< List of entity filters */

    log_entity_filter def_entity; /**< Default entity filter */
} log_msg_filter;

/**
 * Initialize the given message filter.
 *
 * @param filter        message filter
 *
 * @returns Status code
 */
extern te_errno log_msg_filter_init(log_msg_filter *filter);

/**
 * Add a rule without any specific entity or user.
 *
 * Calls to this and other functions that add a filtering rule can act as
 * either an "include" or an "exclude" command. This differentiation is done
 * through the "include" parameter provided by these functions.
 *
 * An "include" command makes the filter include the message with the
 * properties described in the new rule, despite what the previous rules might
 * have said. "Exclude" commands behave similarly.
 *
 * @param filter        message filter
 * @param include       include or exclude
 * @param level_mask    log levels bitmask
 *
 * @returns Status code
 */
extern te_errno log_msg_filter_set_default(log_msg_filter *filter,
                                           te_bool include,
                                           te_log_level level_mask);

/**
 * Add an entity-specific rule.
 *
 * In case of a failure, the new rule may be applied only partially.
 *
 * @param filter        message filter
 * @param include       include or exclude
 * @param name          name or PCRE
 * @param regex         is name a PCRE?
 * @param level_mask    log levels bitmask
 *
 * @returns Status code
 */
extern te_errno log_msg_filter_add_entity(log_msg_filter *filter,
                                          te_bool include,
                                          const char *name, te_bool regex,
                                          te_log_level level_mask);

/**
 * Add a user-specific rule.
 *
 * In case of a failure, the new rule may be applied only partially.
 *
 * @param filter        message filter
 * @param include       include or exclude
 * @param entity        entity name or PCRE or NULL (apply for all entities)
 * @param entity_regex  is entity a PCRE?
 * @param user          user name or PCRE
 * @param user_regex    is user a PCRE?
 * @param level_mask    log levels bitmask
 *
 * @returns Status code
 */
extern te_errno log_msg_filter_add_user(log_msg_filter *filter, te_bool include,
                                        const char *entity, te_bool entity_regex,
                                        const char *user, te_bool user_regex,
                                        te_log_level level_mask);

/**
 * Check a log message against a message filter.
 *
 * @param filter        message filter
 * @param view          message view
 *
 * @returns Verdict
 */
extern log_filter_result log_msg_filter_check(const log_msg_filter *filter,
                                              const log_msg_view *view);

/**
 * Compare two filters and check if they're equal.
 *
 * @param a             message filter 1
 * @param b             message filter 2
 */
extern te_bool log_msg_filter_equal(const log_msg_filter *a,
                                    const log_msg_filter *b);

/**
 * Free the memory allocated by the message filter.
 *
 * @param filter        message filter
 */
extern void log_msg_filter_free(log_msg_filter *filter);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOG_MSG_FILTER_H__ */
