/** @file
 * @brief TE project. Logger subsystem.
 *
 * This module provides different buffers for log messages.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Viacheslav Galaktionov <Viacheslav.Galaktionov@oktetlabs.ru>
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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_BUFS_H__ */
