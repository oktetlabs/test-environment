/** @file
 * @brief TE project. Logger subsystem.
 *
 * This module provides data structures and logic for listener handling.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Viacheslav Galaktionov <Viacheslav.Galaktionov@oktetlabs.ru>
 */

#define TE_LGR_USER "Log streaming"

#include <stddef.h>
#include <sys/time.h>

#include "te_defs.h"
#include "te_str.h"
#include "logger_api.h"
#include "logger_listener.h"

log_listener_conf listener_confs[LOG_MAX_LISTENERS];
size_t            listener_confs_num;
log_listener      listeners[LOG_MAX_LISTENERS];
size_t            listeners_num;

static te_errno
listener_prepare_request(log_listener *listener, const char *url_suffix,
                         void *data, long data_len)
{
    char *url;

    url = te_string_fmt("%s%s", listener->url, url_suffix);
    if (url == NULL)
        return TE_ENOMEM;

#define SET_CURL_OPT(OPT, ARG) \
    do {                                                              \
        CURLcode ret;                                                 \
                                                                      \
        ret = curl_easy_setopt(listener->curl_handle, OPT, ARG);      \
        if (ret != CURLE_OK)                                          \
        {                                                             \
            ERROR("Failed to set CURL option %s for listener %s: %s", \
                  #OPT, listener->name, curl_easy_strerror(ret));     \
            free(url);                                                \
            return TE_EFAULT;                                         \
        }                                                             \
    } while (0)

    SET_CURL_OPT(CURLOPT_HTTPHEADER, listener->headers);
    SET_CURL_OPT(CURLOPT_POSTFIELDSIZE, data_len);
    SET_CURL_OPT(CURLOPT_POSTFIELDS, data);
    SET_CURL_OPT(CURLOPT_POST, 1L);
    SET_CURL_OPT(CURLOPT_URL, url);
#undef SET_CURL_OPT

    free(url);
    return 0;
}

/* See description in logger_listener.h */
te_errno
listener_conf_add(const char *confstr)
{
    size_t             i;
    size_t             ret;
    log_listener_conf *conf;

    if (listener_confs_num >= LOG_MAX_LISTENERS)
        return TE_ENOMEM;

    for (i = 0; i < listener_confs_num; i++)
    {
        if (strcmp(listener_confs[i].name, confstr) == 0)
            return TE_EEXIST;
    }

    conf = &listener_confs[listener_confs_num];
    ret = te_strlcpy(conf->name, confstr, LOG_MAX_LISTENER_NAME);
    if (ret == LOG_MAX_LISTENER_NAME)
        return TE_EINVAL;

    listener_confs_num++;
    return 0;
}

/* See description in logger_listener.h */
log_listener_conf *
listener_conf_get(const char *name)
{
    size_t i;

    for (i = 0; i < listener_confs_num; i++)
    {
        if (strcmp(listener_confs[i].name, name) == 0)
            return &listener_confs[i];
    }

    return NULL;
}

/* See description in logger_listener.h */
te_errno
listener_process_plan(log_listener *listener, const refcnt_buffer *plan)
{
    te_errno rc;

    refcnt_buffer_copy(&listener->plan, plan);

    rc = listener_prepare_request(listener, "init", plan->buf, plan->len);
    if (rc != 0)
    {
        ERROR("Listener %s: Failed to prepare /init request: %r",
              listener->name, rc);
        listener_free(listener);
        refcnt_buffer_free(&listener->plan);
        return rc;
    }

    listener->state = LISTENER_INIT_WAITING;
    return 0;
}

/* See description in logger_listener.h */
te_errno
listener_add_msg(log_listener *listener, const refcnt_buffer *msg)
{
    te_errno rc;

    rc = msg_buffer_add(&listener->buffer, msg);
    if (rc != 0)
        return rc;

    while (listener->buffer.n_items > 1 &&
           listener->buffer.total_length >
             listener->buffer_size * listener->buffers_num)
        msg_buffer_remove_first(&listener->buffer);

    return 0;
}

/* See description in logger_listener.h */
te_errno
listener_dump(log_listener *listener)
{
    te_errno       rc;
    refcnt_buffer *item;

    /* Fill the out buffer. TODO: get rid of this bufferring? */
    te_string_reset(&listener->buffer_out);
    te_string_append(&listener->buffer_out, "[");
    TAILQ_FOREACH(item, &listener->buffer.items, links)
    {
        te_string_append(&listener->buffer_out, "%.*s", item->len, item->buf);
        listener->last_message_id += 1;
        if (listener->buffer_out.len > listener->buffer_size)
            break;
        if (TAILQ_NEXT(item, links) != NULL)
            te_string_append(&listener->buffer_out, ",");
    }
    te_string_append(&listener->buffer_out, "]");

    /* Prepare HTTP request */
    rc = listener_prepare_request(listener, "feed", listener->buffer_out.ptr,
                                  (long)listener->buffer_out.len);
    if (rc != 0)
    {
        ERROR("Listener %s: Failed to prepare /init request: %r",
              listener->name, rc);
        listener_free(listener);
        return rc;
    }

    listener->state = LISTENER_TRANSFERRING;
    listener->next_tv.tv_sec = 0;
    return 0;
}

/* See description in logger_listener.h */
te_errno
listener_finish_request(log_listener *listener)
{
    long           response_code;
    refcnt_buffer *item;
    refcnt_buffer *tmp;

    if (listener->state != LISTENER_INIT_WAITING &&
        listener->state != LISTENER_TRANSFERRING &&
        listener->state != LISTENER_FINISHING)
    {
        ERROR("Request finished while in an unexpected state");
        listener_free(listener);
        return TE_EINVAL;
    }
    if (curl_easy_getinfo(listener->curl_handle, CURLINFO_RESPONSE_CODE,
                          &response_code) != CURLE_OK)
    {
        ERROR("Listener %s: failed to extract request response code");
        listener_free(listener);
        return TE_EINVAL;
    }
    switch (listener->state)
    {
        case LISTENER_INIT_WAITING:
            /* Check status */
            if (response_code != 200)
            {
                ERROR("Listener %s: /init returned %d", response_code);
                listener_free(listener);
                return TE_EINVAL;
            }
            else
            {
                refcnt_buffer_free(&listener->plan);
                RING("Listener %s: session initialized", listener->name);
                listener->state = LISTENER_GATHERING;
            }
            break;
        case LISTENER_TRANSFERRING:
            /* Check response code */
            if (response_code != 200)
            {
                ERROR("Listener %s: /feed returned %d", response_code);
                listener_free(listener);
                return TE_EINVAL;
            }
            /* Remove the messages that have been transferred */
            TAILQ_FOREACH_SAFE(item, &listener->buffer.items, links, tmp)
            {
                if (listener->buffer.first_id > listener->last_message_id)
                    break;
                msg_buffer_remove_first(&listener->buffer);
            }
            gettimeofday(&listener->next_tv, NULL);
            /* Don't delay next dump if there are already enough messages for it */
            if (listener->buffer.total_length < listener->buffer_size)
                listener->next_tv.tv_sec += listener->interval;
            listener->state = LISTENER_GATHERING;
            break;
        case LISTENER_FINISHING:
            listener_free(listener);
            break;
        default:;
    }

    return 0;
}

/* See description in logger_listener.h */
te_errno
listener_finish(log_listener *listener)
{
    te_errno rc;

    if (listener->state == LISTENER_INIT)
    {
        listener->state = LISTENER_FINISHED;
        return 0;
    }

    RING("Listener %s: finishing", listener->name);
    rc = listener_prepare_request(listener, "finish", NULL, 0);
    if (rc != 0)
    {
        ERROR("Listener %s: Failed to prepare /init request: %r",
              listener->name, rc);
        listener_free(listener);
        return rc;
    }

    listener->state = LISTENER_FINISHING;
    listener->next_tv.tv_sec = 0;
    return 0;
}

/* See description in logger_listener.h */
void
listener_free(log_listener *listener)
{
    curl_easy_cleanup(listener->curl_handle);
    listener->curl_handle = NULL;
    curl_slist_free_all(listener->headers);
    listener->headers = NULL;
    te_string_free(&listener->buffer_out);
    /* Free state-specific data */
    switch (listener->state)
    {
        case LISTENER_INIT_WAITING:
            refcnt_buffer_free(&listener->plan);
            break;
        default:;
    }
    msg_buffer_free(&listener->buffer);
    listener->state = LISTENER_FINISHED;
}
