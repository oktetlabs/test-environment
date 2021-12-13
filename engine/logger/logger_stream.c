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
#include <limits.h>
#include <jansson.h>

#include "te_defs.h"
#include "te_str.h"
#include "log_bufs.h"
#include "logger_api.h"
#include "logger_internal.h"
#include "logger_stream.h"
#include "logger_stream_rules.h"
#include "te_alloc.h"

msg_queue  listener_queue;
te_bool    listeners_enabled = FALSE;
char      *metafile_path = NULL;

static json_t *trc_tags = NULL;

pid_t  tester_pid = -1;
double start_ts = 0.0;

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

/** Log entity filter */
static void
entity_dump(log_entity_filter *entity, te_log_buf *buffer)
{
    log_user_filter *user;

    te_log_buf_append(buffer, "    entity %s, level %d\n",
                      entity->name, entity->level);

    SLIST_FOREACH(user, &entity->users, links)
    {
        te_log_buf_append(buffer, "      user %s, level %d\n",
                          user->name, user->level);
    }
}

/** Log streaming filter */
static void
filter_dump(streaming_filter *filter, te_log_buf *buffer)
{
    log_msg_filter    *flt = &filter->filter;
    log_entity_filter *entity;
    size_t             i;
    size_t             j;

    te_log_buf_append(buffer, "  filter:\n");
    SLIST_FOREACH(entity, &flt->entities, links)
        entity_dump(entity, buffer);
    entity_dump(&flt->def_entity, buffer);

    for (i = 0; i < filter->actions_num; i++)
    {
        streaming_action *action = &filter->actions[i];
        te_log_buf_append(buffer, "  rule %s:\n", action->rule->name);
        for (j = 0; j < action->listeners_num; j++)
            te_log_buf_append(buffer, "    listener %s\n",
                              listeners[action->listeners[j]].name);
    }
}

/* See description in logger_stream.h */
void
listeners_conf_dump(void)
{
    size_t      i;
    te_log_buf *buffer;

    buffer = te_log_buf_alloc();

    te_log_buf_append(buffer, "Listeners:\n");
    for (i = 0; i < listeners_num; i++)
    {
        log_listener *listener = &listeners[i];
        te_log_buf_append(buffer, "  name: %s\n", listener->name);
        te_log_buf_append(buffer, "  url: %s\n", listener->url);
        te_log_buf_append(buffer, "  interval: %d\n", listener->interval);
        te_log_buf_append(buffer, "  buffer_size: %lu\n", listener->buffer_size);
        te_log_buf_append(buffer, "  buffers_num: %lu\n", listener->buffers_num);
        te_log_buf_append(buffer, "\n");
    }

    te_log_buf_append(buffer, "Filters:\n");
    for (i = 0; i < streaming_filters_num; i++)
    {
        filter_dump(&streaming_filters[i], buffer);
        te_log_buf_append(buffer, "\n");
    }

    RING("Current listeners configuration:\n%s", te_log_buf_get(buffer));
    te_log_buf_free(buffer);
}

/** Process the log message with Tester process info */
static te_errno
process_tester_proc_info(const log_msg_view *msg)
{
    int           ret;
    te_errno      rc;
    te_string     body = TE_STRING_INIT;
    json_t       *json;
    json_t       *ignored;
    json_error_t  err;

    start_ts = msg->ts_sec + msg->ts_usec / 1000000;

    rc = te_raw_log_expand(msg, &body);
    if (rc != 0)
    {
        ERROR("Failed to expand Tester process info message: %r", rc);
        return rc;
    }

    json = json_loads(body.ptr, 0, &err);
    te_string_free(&body);
    if (json == NULL)
    {
        ERROR("Error parsing Tester process info: %s (line %d, column %d)",
              err.text, err.line, err.column);
        return TE_EFAIL;
    }

    ret = json_unpack_ex(json, &err, JSON_STRICT, "{s:o, s:o, s:i}",
                         "type", &ignored,
                         "version", &ignored,
                         "pid", &tester_pid);
    json_decref(json);
    if (ret == -1)
    {
        ERROR("Error extracting Tester PID: %s (line %d, column %d)",
              err.text, err.line, err.column);
        return TE_EFAIL;
    }

    return 0;
}

/** Process the log message with the test execution plan */
static te_errno
process_trc_tags(const log_msg_view *msg)
{
    te_errno     rc;
    te_string    str = TE_STRING_INIT;
    json_error_t err;

    rc = te_raw_log_expand(msg, &str);
    if (rc != 0)
    {
        ERROR("Failed to expand the TRC tags message: %r", rc);
        return rc;
    }

    trc_tags = json_loads(str.ptr, 0, &err);
    te_string_free(&str);
    if (trc_tags == NULL)
    {
        ERROR("Error parsing TRC tags: %s (line %d, column %d)",
              err.text, err.line, err.column);
        return TE_EFAIL;
    }

    return 0;
}

/**
 * Process the log message with the test execution plan.
 *
 * It is assumed that this message is the last piece of data needed for
 * live resuts, so this is where the connections to listeners will be
 * initialized.
 *
 * @param plan          log message with execution plan
 *
 * @returns Status code
 */
static te_errno
process_plan(const log_msg_view *plan)
{
    size_t         i;
    te_errno       rc;
    te_string      plan_str = TE_STRING_INIT;
    json_t        *metadata;
    json_t        *meta = NULL;
    json_t        *plan_obj;
    json_error_t   err;

    rc = te_raw_log_expand(plan, &plan_str);
    if (rc != 0)
    {
        ERROR("Failed to expand the plan message: %r", rc);
        return TE_EFAIL;
    }

    plan_obj = json_loads(plan_str.ptr, 0, &err);
    te_string_free(&plan_str);
    if (plan_obj == NULL)
    {
        ERROR("Error parsing execution plan: %s (line %d, column %d)",
              err.text, err.line, err.column);
        return TE_EFAIL;
    }

    if (metafile_path != NULL)
    {
        meta = json_load_file(metafile_path, 0, &err);
        free(metafile_path);
        metafile_path = NULL;
        if (meta == NULL)
        {
            ERROR("Error parsing JSON metadata: %s (line %d, column %d)",
                  err.text, err.line, err.column);
            return TE_EFAIL;
        }
        if (!json_is_object(meta))
        {
            ERROR("JSON metadata must be an object");
            json_decref(meta);
            json_decref(plan_obj);
            return TE_EINVAL;
        }
    }

    /*
     * "*" instead of "?" would be better here, but it's not supported in
     * jansson-2.10, which is currently the newest version available on
     * CentOS/RHEL-7.x
     */
    metadata = json_pack("{s:f, s:o?, s:o?, s:o}",
                         "ts", start_ts,
                         "meta_data", meta,
                         "tags", trc_tags,
                         "plan", plan_obj);
    if (metadata == NULL)
    {
        ERROR("Failed to pack live results init message");
        json_decref(plan_obj);
        return TE_EFAIL;
    }
    trc_tags = NULL;

    for (i = 0; i < listeners_num; i++)
        listener_init(&listeners[i], metadata);

    json_decref(metadata);
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

/**
 * Special message description.
 *
 * Special messages are recognized based on their Entity and User fields.
 *
 * These messages are special in the following ways:
 * 1. They provide some information that is important for log streaming and
 *    need to be handled in a special way.
 * 2. They are unique: it is assumed that each special message appears only
 *    once in the log. If another message with the same Entity and User values
 *    is found, it is ignored silently (for the purposes of optimization).
 */
typedef struct special_message {
    te_bool             found;
    const queue_event   event;
    te_errno          (*handler)(const log_msg_view *msg);
    const char         *entity;
    const size_t        entity_len;
    const char         *user;
    const size_t        user_len;
} special_message;

/** Check if a message is "special" and process it accordingly */
queue_event
process_special_messages(const log_msg_view *msg)
{
#define STRING_WITH_LEN(str) (str), (sizeof(str) - 1)
    static special_message special[] = {
        /* Execution plan */
        { FALSE, QEVENT_PLAN, process_plan,
          STRING_WITH_LEN(TE_LOG_CMSG_ENTITY_TESTER),
          STRING_WITH_LEN(TE_LOG_EXEC_PLAN_USER) },
        /* TRC tags */
        { FALSE, QEVENT_NONE, process_trc_tags,
          STRING_WITH_LEN(TE_LOG_CMSG_ENTITY_TESTER),
          STRING_WITH_LEN(TE_LOG_TRC_TAGS_USER) },
        /* Tester PID */
        { FALSE, QEVENT_NONE, process_tester_proc_info,
          STRING_WITH_LEN(TE_LOG_CMSG_ENTITY_TESTER),
          STRING_WITH_LEN(TE_LOG_PROC_INFO_USER) },
    };
#undef STRING_WITH_LEN
    static const size_t special_num = TE_ARRAY_LEN(special);

    size_t      i;
    te_errno    rc;
    queue_event evt = QEVENT_NONE;

    for (i = 0; i < special_num; i++)
    {
        if (!special[i].found &&
            msg->entity_len == special[i].entity_len &&
            msg->user_len == special[i].user_len &&
            strncmp(msg->entity, special[i].entity, msg->entity_len) == 0 &&
            strncmp(msg->user, special[i].user, msg->user_len) == 0)
        {
            special[i].found = TRUE;
            evt = special[i].event;
            rc = special[i].handler(msg);
            if (rc != 0)
                evt |= QEVENT_FAILURE;
            break;
        }
    }

    return evt;
}

/** Process the messages from Logger threads */
static queue_event
process_queue(void)
{

    size_t              i;
    refcnt_buffer_list  messages;
    refcnt_buffer      *item;
    refcnt_buffer      *tmp;
    queue_event         evt = QEVENT_NONE;
    te_bool             queue_shutdown;
    te_bool             failure = FALSE;

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
                /* Check if the message is special */
                evt |= process_special_messages(&view);
                if (evt & QEVENT_FAILURE)
                    failure = TRUE;
                /* Run through filters */
                if (!failure)
                    for (i = 0; i < streaming_filters_num; i++)
                        streaming_filter_process(&streaming_filters[i], &view);
            }
        }

        /* Free the entry */
        refcnt_buffer_free(item);
        free(item);
    }

    return evt;
}

/** CURL socket callback */
static int
socket_cb(CURL *handle, curl_socket_t sock, int what, void *userp,
          void *socketp)
{
    size_t         i;
    struct pollfd *fds = userp;

    UNUSED(handle);
    UNUSED(socketp);
    if (what == CURL_POLL_REMOVE)
    {
        /* Remove this socket from poll fds */
        for (i = 1; i < listeners_num + 1; i++)
        {
            if (fds[i].fd == sock)
            {
                fds[i].fd = -1;
                return 0;
            }
        }
        /* Unreachable */
        ERROR("CURL requested removal of a nonexistent socket");
        assert(0);
    }
    else
    {
        /* Find this socket in the array */
        for (i = 1; i < listeners_num + 1; i++)
        {
            if (fds[i].fd == sock)
                break;
        }
        /* Find a spot in the fd array for this new socket */
        if (i == listeners_num + 1)
        {
            for (i = 1; i < listeners_num + 1; i++)
            {
                if (fds[i].fd == -1)
                    break;
            }
        }
        if (i < listeners_num + 1)
        {
            fds[i].fd = sock;
            if (what == CURL_POLL_IN)
                fds[i].events = POLLIN;
            else if (what == CURL_POLL_OUT)
                fds[i].events = POLLOUT;
            else
                fds[i].events = POLLIN | POLLOUT;
        }
        else
        {
            ERROR("curl requested addition of too many sockets");
            assert(0);
        }
    }
    return 0;
}

/** CURL timer callback */
static int
timer_cb(CURLM *handle, long timeout_ms, void *userp)
{
    struct timeval *timeout = userp;

    UNUSED(handle);
    if (timeout_ms == -1)
    {
        timeout->tv_sec = 0;
    }
    else
    {
        gettimeofday(timeout, NULL);
        timeout->tv_sec += timeout_ms / 1000;
        timeout->tv_usec += (timeout_ms % 1000) * 1000;
        if (timeout->tv_usec >= 1000000)
        {
            timeout->tv_sec += 1;
            timeout->tv_usec -= 1000000;
        }
    }

    return 0;
}

/* See description in logger_stream.h */
void *
listeners_thread(void *arg)
{
    size_t          i;
    int             ret;
    te_errno        rc;
    te_bool         added_handles;
    struct pollfd   fds[1 + LOG_MAX_LISTENERS];
    struct timeval  now;
    struct timeval  next;
    struct timeval  curl_timeout;
    CURLM          *curl_mhandle;
    int             curl_running;
    CURLMsg        *curl_msg;
    int             in_queue;
    queue_event     events_happened = QEVENT_NONE;
    int             listeners_running;
    int             poll_timeout;

    UNUSED(arg);

    if (listeners_num == 0)
        return NULL;

    curl_mhandle = curl_multi_init();
    if (curl_mhandle == NULL)
    {
        ERROR("curl_multi_init failed");
        return NULL;
    }
    curl_timeout.tv_sec = 0;
#define SET_CURLM_OPT(OPT, ARG) \
    do {                                                 \
        CURLMcode ret;                                   \
                                                         \
        ret = curl_multi_setopt(curl_mhandle, OPT, ARG); \
        if (ret != CURLM_OK)                             \
        {                                                \
            ERROR("Failed to set CURLM option %s: %s",   \
                  #OPT, curl_multi_strerror(ret));       \
            return NULL;                                 \
        }                                                \
    } while (0)

    SET_CURLM_OPT(CURLMOPT_SOCKETDATA, fds);
    SET_CURLM_OPT(CURLMOPT_SOCKETFUNCTION, socket_cb);
    SET_CURLM_OPT(CURLMOPT_TIMERDATA, &curl_timeout);
    SET_CURLM_OPT(CURLMOPT_TIMERFUNCTION, timer_cb);
#undef SET_CURLM_OPT

    gettimeofday(&now, NULL);

    /* Initialize polling structure */
    fds[0].fd = listener_queue.eventfd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    /* Initialize listener timeouts */
    for (i = 0; i < listeners_num; i++)
        listeners[i].next_tv.tv_sec = 0;

    for (i = 1; i < listeners_num + 1; i++)
    {
        fds[i].fd = -1;
        fds[i].events = 0;
        fds[i].revents = 0;
    }

    next.tv_sec = 0;
    curl_multi_socket_action(curl_mhandle, CURL_SOCKET_TIMEOUT, 0, &curl_running);
    do {
        added_handles = FALSE;

        gettimeofday(&now, NULL);
        if (next.tv_sec != 0 && timercmp(&next, &now, <))
            next = now;
        if (curl_timeout.tv_sec != 0 && timercmp(&next, &curl_timeout, <))
            next = curl_timeout;

        poll_timeout = next.tv_sec == 0 ? -1 :
                       (next.tv_sec - now.tv_sec) * 1000 +
                       (next.tv_usec - now.tv_usec) / 1000;

        ret = poll(fds, listeners_num + 1, poll_timeout);
        if (ret == -1)
        {
            ERROR("Poll error: %s", strerror(errno));
            break;
        }

        gettimeofday(&now, NULL);

        if (fds[0].revents & POLLIN)
        {
            queue_event evt;

            evt = process_queue();
            events_happened |= evt;
            if (evt & QEVENT_PLAN)
            {
                for (i = 0; i < listeners_num; i++)
                    curl_multi_add_handle(curl_mhandle, listeners[i].curl_handle);
                curl_multi_socket_action(curl_mhandle,
                                         CURL_SOCKET_TIMEOUT, 0, &curl_running);
            }
        }

        if (events_happened & QEVENT_FAILURE)
        {
            for (i = 0; i < listeners_num; i++)
                listener_free(&listeners[i]);
            break;
        }

        if (curl_timeout.tv_sec != 0 && timercmp(&curl_timeout, &now, <))
        {
            curl_timeout.tv_sec = 0;
            curl_multi_socket_action(curl_mhandle, CURL_SOCKET_TIMEOUT, 0, &curl_running);
        }

        /* Handle socket events and timeouts */
        for (i = 1; i < listeners_num + 1; i++)
        {
            if (fds[i].revents != 0)
            {
                int ev_bitmask = 0;

                if (fds[i].revents & POLLIN)
                    ev_bitmask |= CURL_CSELECT_IN;
                if (fds[i].revents & POLLOUT)
                    ev_bitmask |= CURL_CSELECT_OUT;
                if (fds[i].revents & POLLERR)
                    ev_bitmask |= CURL_CSELECT_ERR;
                fds[i].revents = 0;
                curl_multi_socket_action(curl_mhandle, fds[i].fd, ev_bitmask, &curl_running);
            }
        }

        /* Process finished curl transfers */
        do {
            curl_msg = curl_multi_info_read(curl_mhandle, &in_queue);
            if (curl_msg && curl_msg->msg == CURLMSG_DONE)
            {
                log_listener *listener;

                /* Remove the handle */
                curl_multi_remove_handle(curl_mhandle, curl_msg->easy_handle);
                /* Fetch the listener */
                curl_easy_getinfo(curl_msg->easy_handle, CURLINFO_PRIVATE,
                                  (char **)&listener);
                /* Handle request result */
                listener_finish_request(listener, curl_msg->data.result);
            }
        } while (in_queue > 0);

        /* Listener logic */
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

            /* Let Logger finish if the listener is unavailable */
            if (listener->need_retry && (events_happened & QEVENT_FINISH))
            {
                listener_free(listener);
                continue;
            }
            listeners_running += 1;

            if (listener->need_retry &&
                timercmp(&listener->next_tv, &now, <))
            {
                listener->need_retry = FALSE;
                curl_multi_add_handle(curl_mhandle, listener->curl_handle);
                added_handles = TRUE;
                continue;
            }

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
                curl_multi_add_handle(curl_mhandle, listener->curl_handle);
                added_handles = TRUE;
            }

            /* Finish once all messages have been sent */
            if (listener->state == LISTENER_GATHERING &&
                (events_happened & QEVENT_FINISH) &&
                listener->buffer.total_length == 0)
            {
                listener_finish(listener);
                curl_multi_add_handle(curl_mhandle, listener->curl_handle);
                added_handles = TRUE;
            }

            if (next.tv_sec == 0 ||
                (listener->next_tv.tv_sec != 0 &&
                 timercmp(&listener->next_tv, &next, <)))
                next = listener->next_tv;
        }
        if (added_handles)
            curl_multi_socket_action(curl_mhandle, CURL_SOCKET_TIMEOUT, 0, &curl_running);
    } while (listeners_running > 0);

    /* Free listener config data in case the execution plan has not been received */
    free(metafile_path);
    json_decref(trc_tags);

    curl_multi_cleanup(curl_mhandle);
    RING("Listener thread finished");
    return NULL;
}
