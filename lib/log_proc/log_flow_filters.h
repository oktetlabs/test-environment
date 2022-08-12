/** @file
 * @brief Log processing
 *
 * This module provides some filters that do not work with log messages
 * directly, but instead deal with flow nodes.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_LOG_FLOW_FILTERS_H__
#define __TE_LOG_FLOW_FILTERS_H__

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "te_raw_log.h"
#include "log_msg_filter.h"

#ifdef _cplusplus
extern "C" {
#endif

/**
 * Branch filtering relies on a list of (path, result) pairs.
 * If the path is not mentioned in the list, "default" will be returned.
 */
typedef struct log_branch_filter_rule {
    SLIST_ENTRY(log_branch_filter_rule) links;

    char              *path;
    log_filter_result  result;
} log_branch_filter_rule;

typedef struct log_branch_filter {
    SLIST_HEAD(, log_branch_filter_rule) list;
} log_branch_filter;

/**
 * Initialite a branch filter.
 *
 * @param filter    branch filter
 */
extern void log_branch_filter_init(log_branch_filter *filter);

/**
 * Add a branch rule.
 *
 * @param filter    branch filter
 * @param path      branch path
 * @param include   whether to include or exclude the branch
 *
 * @return  Status code
 */
extern te_errno log_branch_filter_add(log_branch_filter *filter,
                                      const char *path,
                                      te_bool include);

/**
 * Check a path.
 *
 * @param filter    branch filter
 * @param path      branch path
 *
 * @returns Filtering result
 */
extern log_filter_result log_branch_filter_check(log_branch_filter *filter,
                                                 const char *path);

/**
 * Free the memory allocated for the filter.
 *
 * @param filter    branch filter
 */
extern void log_branch_filter_free(log_branch_filter *filter);

/**
 * Filtering by duration is represented with a set of rules for each kind of
 * node (package, session, test).
 *
 * These rules have a form of (interval, result) pairs.
 */
typedef struct log_duration_filter_rule {
    SLIST_ENTRY(log_duration_filter_rule) links;

    uint32_t          min;
    uint32_t          max;
    log_filter_result result;
} log_duration_filter_rule;

typedef struct log_duration_filter_rules {
    SLIST_HEAD(, log_duration_filter_rule) list;
} log_duration_filter_rules;

typedef struct log_duration_filter {
    log_duration_filter_rules package;
    log_duration_filter_rules session;
    log_duration_filter_rules test;
} log_duration_filter;

/**
 * Initialize a duration filter.
 *
 * @param filter    duration filter
 *
 * @returns Status code
 */
extern te_errno log_duration_filter_init(log_duration_filter *filter);

/**
 * Add a duration rule.
 *
 * @param filter    duration filter
 * @param type      flow node type (e.g. "PACKAGE" or "SESSION")
 * @param min       start of interval
 * @param max       end of interval
 * @param include   should this interval be included
 *
 * @returns  Status code
 */
extern te_errno log_duration_filter_add(log_duration_filter *filter,
                                        const char *type,
                                        uint32_t min, uint32_t max,
                                        te_bool include);

/**
 * Check a duration.
 *
 * @param filter    duration filter
 * @param type      flow node type (e.g. "PACKAGE" or "SESSION")
 * @param duration  duration
 *
 * @returns Filtering result
 */
extern log_filter_result log_duration_filter_check(log_duration_filter *filter,
                                                   const char *type,
                                                   uint32_t duration);

/**
 * Free the memory allocated for the filter.
 *
 * @param filter    duration filter
 */
extern void log_duration_filter_free(log_duration_filter *filter);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOG_FLOW_FILTERS_H__ */
