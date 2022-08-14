/** @file
 * @brief TE project. Logger subsystem.
 *
 * This module provides a public interface to streaming log messages.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_LOGGER_STREAM_H__
#define __TE_LOGGER_STREAM_H__

#include <pthread.h>
#include <sys/eventfd.h>

#include "te_defs.h"
#include "te_errno.h"
#include "logger_bufs.h"

#ifdef _cplusplus
extern "C" {
#endif

/**
 * Thread safe message queue for communication between
 * Logger threads and listener server thread.
 */
typedef struct msg_queue {
    refcnt_buffer_list items; /**< Messages */

    te_bool         shutdown; /**< Whether the queue is being shutdown */
    pthread_mutex_t mutex;    /**< Mutex for consumer-producer synchronization */
    int             eventfd;  /**< File descriptor for consumer to poll on */
} msg_queue;

/**
 * Initialize a message queue.
 *
 * @param queue         Message queue
 *
 * @returns Status code
 */
extern te_errno msg_queue_init(msg_queue *queue);

/**
 * Post a message on the queue.
 *
 * A copy of the message will be made, so the caller is free
 * to reuse the buffer for other messages.
 *
 * @param queue         Message queue
 * @param buf           Message content
 * @param len           Message length
 *
 * @returns Status code
 */
extern te_errno msg_queue_post(msg_queue *queue, const char *buf, size_t len);

/**
 * Extract messages from the message queue.
 *
 * After this function is called, the queue's contents will be moved into the
 * user-supplied message list.
 *
 * @param queue         Message queue
 * @param list          Message list to be initialized with queue contents
 * @param shutdown      Whether the queue is currently being shutdown
 */
extern void msg_queue_extract(msg_queue *queue, refcnt_buffer_list *list,
                              te_bool *shutdown);

/** Notify the consumer that there will not be any new messages */
extern void msg_queue_shutdown(msg_queue *queue);

/**
 * Deinitialize a message queue.
 *
 * @param queue         Message queue
 *
 * @returns Status code
 */
extern te_errno msg_queue_fini(msg_queue *queue);

/** Log current listener configuration */
extern void listeners_conf_dump(void);

/** Main routine for the listener server thread */
extern void *listeners_thread(void *);

/** Message queue instance to be used by Logger threads */
extern msg_queue       listener_queue;
/** Whether there are active listeners */
extern te_bool         listeners_enabled;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_LOGGER_STREAM_H__ */
