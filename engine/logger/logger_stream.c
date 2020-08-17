/** @file
 * @brief TE project. Logger subsystem.
 *
 * This module provides implementation for streaming logic.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Viacheslav Galaktionov <Viacheslav.Galaktionov@oktetlabs.ru>
 */

#define TE_LGR_USER "Log streaming"

#include "te_defs.h"
#include "logger_api.h"
#include "logger_stream.h"
#include "te_alloc.h"

/* See description in logger_stream.h */
te_errno
msg_queue_init(msg_queue *queue)
{
    memset(queue, 0, sizeof(*queue));
    TAILQ_INIT(&queue->items);
    queue->shutdown = FALSE;
    pthread_mutex_init(&queue->mutex, NULL);
    queue->eventfd = eventfd(0, 0);
    return 0;
}

/* See description in logger_stream.h */
te_errno
msg_queue_post(msg_queue *queue, const char *buf, size_t len)
{
    uint64_t      inc;
    te_errno      rc;
    refcnt_buffer *item;

    item = TE_ALLOC(sizeof(*item));
    if (item == NULL)
        return TE_ENOMEM;

    rc = refcnt_buffer_init_copy(item, buf, len);
    if (rc != 0)
        return rc;

    pthread_mutex_lock(&queue->mutex);
    if (queue->shutdown)
    {
        pthread_mutex_unlock(&queue->mutex);
        refcnt_buffer_free(item);
        free(item);
        return TE_EFAIL;
    }
    TAILQ_INSERT_TAIL(&queue->items, item, links);
    pthread_mutex_unlock(&queue->mutex);

    inc = 1;
    write(queue->eventfd, &inc, sizeof(inc));
    return 0;
}

/* See description in logger_stream.h */
void
msg_queue_extract(msg_queue *queue, refcnt_buffer_list *list,
                  te_bool *shutdown)
{
    uint64_t cnt;

    pthread_mutex_lock(&queue->mutex);
    read(queue->eventfd, &cnt, sizeof(cnt));
    if (list != NULL)
    {
        *list = queue->items;
        TAILQ_INIT(&queue->items);
    }
    if (shutdown != NULL)
        *shutdown = queue->shutdown;
    pthread_mutex_unlock(&queue->mutex);
}

/* See description in logger_stream.h */
void
msg_queue_shutdown(msg_queue *queue)
{
    uint64_t inc;

    pthread_mutex_lock(&queue->mutex);
    queue->shutdown = TRUE;
    pthread_mutex_unlock(&queue->mutex);

    inc = 1;
    write(queue->eventfd, &inc, sizeof(inc));
}

/* See description in logger_stream.h */
te_errno
msg_queue_fini(msg_queue *queue)
{
    refcnt_buffer *item;
    refcnt_buffer *tmp;

    if (!TAILQ_EMPTY(&queue->items))
    {
        WARN("Not all messages in listener queue have been processed");
        TAILQ_FOREACH_SAFE(item, &queue->items, links, tmp)
        {
            refcnt_buffer_free(item);
            free(item);
        }
    }

    pthread_mutex_destroy(&queue->mutex);
    close(queue->eventfd);
    return 0;
}
