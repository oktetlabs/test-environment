/** @file
 * @brief TE project. Logger subsystem.
 *
 * This module provides data structures and logic for log message processing
 * in term of log streaming.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_LOGGER_STREAM_RULES_H__
#define __TE_LOGGER_STREAM_RULES_H__

#include <stdio.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "logger_bufs.h"
#include "log_msg_filter.h"
#include "logger_listener.h"

#ifdef _cplusplus
extern "C" {
#endif

#define LOG_MAX_FILTERS           20
#define LOG_MAX_FILTER_RULE_NAME  24
#define LOG_MAX_FILTER_RULES      10

/**
 * Handler function that transforms a log message into a message
 * that will be sent to the listeners.
 */
typedef te_errno (*streaming_handler)(const log_msg_view *view,
                                      refcnt_buffer *str);

/** Rule that describes how a message should be processed */
typedef struct streaming_rule {
    char              name[LOG_MAX_FILTER_RULE_NAME]; /**< Rule name */
    streaming_handler handler;                        /**< Handler function */
} streaming_rule;

/** Action that needs to be done for messages of a certain type */
typedef struct streaming_action {
    const streaming_rule *rule;                   /**< Streaming rule */
    int             listeners[LOG_MAX_LISTENERS]; /**< Listeners that need to
                                                       receive the result */
    size_t          listeners_num;                /**< Number of listeners */
} streaming_action;

/**
 * Add a listener to streaming action.
 *
 * @param action            streaming action
 * @param listener_id       id of listener to be added
 *
 * @returns Status code
 */
extern te_errno streaming_action_add_listener(streaming_action *action,
                                              int listener_id);

/** Message filter with a set of streaming actions */
typedef struct streaming_filter {
    log_msg_filter   filter;                        /**< Message filter */
    streaming_action actions[LOG_MAX_FILTER_RULES]; /**< List of actions */
    size_t           actions_num;                   /**< Number of actions */
} streaming_filter;

/**
 * Process a log message through a given filter.
 *
 * @param filter            streaming filter
 * @param view              log message
 *
 * @returns Status code
 */
extern te_errno streaming_filter_process(const streaming_filter *filter,
                                         const log_msg_view *view);

/**
 * Add an action to a streaming filter.
 *
 * @param filter            streaming filter
 * @param rule_name         streaming rule name
 * @param listener_id       listener's position in the listeners array
 */
extern te_errno streaming_filter_add_action(streaming_filter *filter,
                                            const char *rule_name,
                                            int listener_id);


/** Array of streamling filters */
extern streaming_filter streaming_filters[LOG_MAX_FILTERS];
extern size_t           streaming_filters_num;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_STREAM_RULES_H__ */
