/** @file
 * @brief TE project. Logger subsystem.
 *
 * This module provides data structures and logic for listener handling.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Viacheslav Galaktionov <Viacheslav.Galaktionov@oktetlabs.ru>
 */

#ifndef __TE_LOGGER_LISTENER_H__
#define __TE_LOGGER_LISTENER_H__

#include <stdio.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_queue.h"
#include "logger_bufs.h"

#ifdef _cplusplus
extern "C" {
#endif

#define LOG_MAX_LISTENERS      10
#define LOG_MAX_LISTENER_NAME  64
#define LOG_MAX_LISTENER_URL   256

/** Listener operation state */
typedef enum listener_state {
    LISTENER_INIT,      /**< Initial state, waiting for execution
                             plan to be posted */
    LISTENER_GATHERING, /**< Gathering messages to be sent */
    LISTENER_FINISHED   /**< Listener has finished its operation */
} listener_state;

/** Log message listener */
typedef struct log_listener {
    char            name[LOG_MAX_LISTENER_NAME]; /**< Name */
    char            url[LOG_MAX_LISTENER_URL];   /**< URL */
    listener_state  state;       /**< Current state */
    struct timeval  next_tv;     /**< Timestamp of the next dump */
    int             interval;    /**< Time interval between dumps, in seconds */
    FILE           *file;        /**< File to dump to */
    msg_buffer      buffer;      /**< Message buffer */
    size_t          buffer_size; /**< Virtual buffer size */
    size_t          buffers_num; /**< Number of virtual message buffers */
} log_listener;

/**
 * Send execution plan to the listener.
 *
 * @param listener          listener description
 * @param plan              execution plan
 *
 * @returns Status code
 */
extern te_errno listener_process_plan(log_listener *listener,
                                      const refcnt_buffer *plan);

/**
 * Add message to listener's buffer.
 *
 * @param listener          listener description
 * @param msg               message
 *
 * @returns Status code
 */
extern te_errno listener_add_msg(log_listener *listener,
                                 const refcnt_buffer *msg);

/**
 * Dump messages to listener.
 *
 * @param listener          listener description
 *
 * @returns Status code
 */
extern te_errno listener_dump(log_listener *listener);

/**
 * Finish listener's operation.
 *
 * @param listener          listener description
 *
 * @returns Status code
 */
extern te_errno listener_finish(log_listener *listener);

/**
 * Deinitialize listener and free its resources.
 *
 * @param listener          listener description
 */
extern void listener_free(log_listener *listener);


/** Array of listeners */
extern log_listener    listeners[LOG_MAX_LISTENERS];
extern size_t          listeners_num;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_LISTENER_H__ */
