/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TE project. Logger subsystem.
 *
 * This module provides different buffers for log messages.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_LOGGER_BUFS_H__
#define __TE_LOGGER_BUFS_H__

#include "te_errno.h"
#include "te_queue.h"

#ifdef _cplusplus
extern "C" {
#endif

/** A memory buffer that keeps track of references to its contents */
typedef struct refcnt_buffer {
    TAILQ_ENTRY(refcnt_buffer) links; /**< Pointers to other buffers */

    void   *buf;       /**< Pointer to data */
    size_t  len;       /**< Length of data */
    int    *refcount;  /**< Reference counter */
} refcnt_buffer;

/** List of reference-counted buffers */
typedef TAILQ_HEAD(, refcnt_buffer) refcnt_buffer_list;

/**
 * Initialize a reference-counting buffer using the given data.
 *
 * Ownership over the data is transferred to the buffer.
 *
 * @param rbuf          refcount buffer
 * @param data          buffer contents
 * @param len           buffer size
 *
 * @returns Status code
 */
extern te_errno refcnt_buffer_init(refcnt_buffer *rbuf,
                                   void *data,
                                   size_t len);

/**
 * Initialize a reference-counting buffer using the given data.
 *
 * User-supplied data is copied into the buffer.
 *
 * @param rbuf          refcount buffer
 * @param data          buffer contents
 * @param len           buffer size
 *
 * @returns Status code
 */
extern te_errno refcnt_buffer_init_copy(refcnt_buffer *rbuf,
                                        const void *data,
                                        size_t len);

/**
 * Copy a reference-counting buffer.
 *
 * @param dest          target buffer
 * @param src           source buffer
 */
extern void refcnt_buffer_copy(refcnt_buffer *dest,
                               const refcnt_buffer *src);

/**
 * Deinitialize a reference-counting buffer.
 *
 * @param rbuf          refcount buffer
 */
extern void refcnt_buffer_free(refcnt_buffer *rbuf);

/** Buffer structure for log messages */
typedef struct msg_buffer {
    TAILQ_HEAD(, refcnt_buffer) items; /**< List of messages */

    size_t n_items;      /**< Number of messages in the buffer */
    size_t total_length; /**< Total length of all messages, in bytes */
} msg_buffer;

/**
 * Initialize a message buffer.
 *
 * @param buf           message buffer
 */
extern void msg_buffer_init(msg_buffer *buf);

/**
 * Add a message to a message buffer.
 *
 * @param buf           message buffer
 * @param msg           message
 *
 * @param Status code
 */
extern te_errno msg_buffer_add(msg_buffer *buf, const refcnt_buffer *msg);

/**
 * Remove the first message in the buffer.
 *
 * @param buf           message buffer
 */
extern void msg_buffer_remove_first(msg_buffer *buf);

/**
 * Deinitialize the buffer.
 *
 * All messages in the buffer will be freed.
 *
 * @param buf           message buffer
 */
extern void msg_buffer_free(msg_buffer *buf);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_BUFS_H__ */
