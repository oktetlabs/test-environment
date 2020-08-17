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

#include <poll.h>

#include "te_defs.h"
#include "logger_api.h"
#include "logger_stream.h"
#include "logger_stream_rules.h"
#include "te_alloc.h"

msg_queue  listener_queue;
te_bool    listeners_enabled = FALSE;

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

/** Process the log message with the test execution plan */
static te_errno
process_plan(log_msg_view *plan)
{
    size_t        i;
    te_errno      rc;
    te_string     plan_str = TE_STRING_INIT;
    refcnt_buffer plan_buf;

    rc = te_raw_log_expand(plan, &plan_str);
    if (rc != 0)
    {
        ERROR("Failed to expand the plan message: %r", rc);
        return TE_EFAIL;
    }

    refcnt_buffer_init(&plan_buf, plan_str.ptr, plan_str.len);
    for (i = 0; i < listeners_num; i++)
        listener_process_plan(&listeners[i], &plan_buf);
    refcnt_buffer_free(&plan_buf);

    return 0;
}

/** Events that can happen during log message queue processing */
typedef enum queue_event {
    QEVENT_NONE    = 0,      /**< Nothing happened */
    QEVENT_PLAN    = 1 << 0, /**< Execution plan has been posted */
    QEVENT_FINISH  = 1 << 1, /**< There will not be any more messages
                                  in the queue */
    QEVENT_FAILURE = 1 << 2, /**< A critical failure has occurred.
                                  All listeners should be freed. */
} queue_event;

/** Process the messages from Logger threads */
static queue_event
process_queue(void)
{
    int                 i;
    refcnt_buffer_list  messages;
    refcnt_buffer      *item;
    refcnt_buffer      *tmp;
    queue_event         evt = QEVENT_NONE;
    te_bool             queue_shutdown;
    te_bool             failure = FALSE;
    static te_bool      found_plan = FALSE;
    static const char   plan_entity[] = TE_LOG_CMSG_ENTITY_TESTER;
    static const char   plan_user[] = TE_LOG_EXEC_PLAN_USER;

    msg_queue_extract(&listener_queue, &messages, &queue_shutdown);
    if (queue_shutdown)
        evt |= QEVENT_FINISH;

    TAILQ_FOREACH_SAFE(item, &messages, links, tmp)
    {
        te_errno rc;
        log_msg_view view;

        if (!failure)
        {
            /* Parse the entry */
            rc = te_raw_log_parse(item->buf, item->len, &view);
            if (rc == 0)
            {
                if (!found_plan &&
                    view.entity_len == sizeof(plan_entity) - 1 &&
                    strncmp(view.entity, plan_entity, view.entity_len) == 0 &&
                    view.user_len == sizeof(plan_user) - 1 &&
                    strncmp(view.user, plan_user, view.user_len) == 0)
                {
                    found_plan = TRUE;
                    evt |= QEVENT_PLAN;
                    rc = process_plan(&view);
                    if (rc != 0)
                    {
                        evt |= QEVENT_FAILURE;
                        failure = TRUE;
                    }
                }
                /* Run through filters */
                if (!failure)
                    for (i = 0; i < LOG_MAX_FILTERS; i++)
                        streaming_filter_process(&streaming_filters[i], &view);
            }
        }

        /* Free the entry */
        refcnt_buffer_free(item);
        free(item);
    }

    return evt;
}

/* See description in logger_stream.h */
void *
listeners_thread(void *arg)
{
    size_t         i;
    int            ret;
    te_errno       rc;
    struct pollfd  fds[1];
    struct timeval now;
    struct timeval next;
    queue_event    events_happened = QEVENT_NONE;
    int            listeners_running;
    int            poll_timeout;

    UNUSED(arg);

    if (listeners_num == 0)
        return NULL;

    gettimeofday(&now, NULL);

    /* Initialize polling structure */
    fds[0].fd = listener_queue.eventfd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    /* Initialize listener timeouts */
    for (i = 0; i < listeners_num; i++)
        listeners[i].next_tv.tv_sec = 0;

    next.tv_sec = 0;
    do {
        gettimeofday(&now, NULL);
        if (next.tv_sec != 0 && timercmp(&next, &now, <))
            next = now;

        poll_timeout = next.tv_sec == 0 ? -1 :
                       (next.tv_sec - now.tv_sec) * 1000 +
                       (next.tv_usec - now.tv_usec) / 1000;

        ret = poll(fds, 1, poll_timeout);
        if (ret == -1)
        {
            ERROR("Poll error: %s", strerror(errno));
            break;
        }

        gettimeofday(&now, NULL);

        if (fds[0].revents & POLLIN)
            events_happened |= process_queue();

        if (events_happened & QEVENT_FAILURE)
        {
            for (i = 0; i < listeners_num; i++)
                listener_free(&listeners[i]);
            break;
        }

        listeners_running = 0;
        next.tv_sec = 0;
        for (i = 0; i < listeners_num; i++)
        {
            log_listener *listener = &listeners[i];

            /*
             * Finish if the queue was finished before listener could start
             * its operation
             */
            if ((events_happened & QEVENT_FINISH) &&
                listener->state == LISTENER_INIT)
                listener_free(listener);

            /* Skip listener if it has already finished its operation */
            if (listener->state == LISTENER_FINISHED)
                continue;
            listeners_running += 1;

            /*
             * Dump if:
             *   a) the virtual buffer is full,
             *   b) dump timeout has been reached,
             *   c) no new messages will appear
             *      (there's no point in buffering messages).
             */
            if (listener->state == LISTENER_GATHERING &&
                (listener->buffer.total_length >= listener->buffer_size ||
                 timercmp(&listener->next_tv, &now, <) ||
                 ((events_happened & QEVENT_FINISH) &&
                  listener->buffer.total_length > 0)))
            {
                rc = listener_dump(listener);
                if (rc != 0)
                    ERROR("Listener %s: failed to dump messages");
            }

            /* Finish once all messages have been sent */
            if (listener->state == LISTENER_GATHERING &&
                (events_happened & QEVENT_FINISH) &&
                listener->buffer.total_length == 0)
            {
                listener_finish(listener);
                listeners_running -= 1;
                continue;
            }

            if (next.tv_sec == 0 ||
                (listener->next_tv.tv_sec != 0 &&
                 timercmp(&listener->next_tv, &next, <)))
                next = listener->next_tv;
        }
    } while (listeners_running > 0);

    RING("Listener thread finished");
    return NULL;
}
