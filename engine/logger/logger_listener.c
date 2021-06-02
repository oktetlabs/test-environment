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
#include <signal.h>
#include <sys/time.h>

#include "te_defs.h"
#include "te_str.h"
#include "logger_api.h"
#include "logger_internal.h"
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

    te_dbuf_reset(&listener->buffer_in);
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
    size_t             len;
    char              *delim;
    char              *runid;
    log_listener_conf *conf;

    if (listener_confs_num >= LOG_MAX_LISTENERS)
        return TE_ENOMEM;

    delim = strchr(confstr, ':');
    len = delim == NULL ? strlen(confstr) : (delim - confstr);
    if (len >= LOG_MAX_LISTENER_NAME)
        return TE_EINVAL;

    for (i = 0; i < listener_confs_num; i++)
    {
        if (strncmp(listener_confs[i].name, confstr, len) == 0)
            return TE_EEXIST;
    }

    conf = &listener_confs[listener_confs_num];
    strncpy(conf->name, confstr, len);

    if (delim != NULL)
    {
        runid = delim + 1;
        if (*runid == '\0')
            return TE_EINVAL;
        ret = te_strlcpy(conf->runid, runid, LOG_MAX_LISTENER_RUNID);
        if (ret == LOG_MAX_LISTENER_RUNID)
            return TE_EINVAL;
    }

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
listener_init(log_listener *listener, json_t *data)
{
    te_errno  rc;
    json_t   *runid;
    char     *json;

    data = json_copy(data);
    if (data == NULL)
    {
        ERROR("Failed to copy init message");
        return TE_EFAIL;
    }

    if (listener->runid[0] != '\0')
    {
        runid = json_string(listener->runid);
        if (runid == NULL)
        {
            ERROR("Failed to prepare a JSON string for run ID");
            json_decref(data);
            return TE_EFAIL;
        }

        if (json_object_set_new(data, "runid", runid) == -1)
        {
            ERROR("Failed to prepare a JSON string for run ID");
            json_decref(data);
            json_decref(runid);
            return TE_EFAIL;
        }
    }

    json = json_dumps(data, JSON_COMPACT);
    if (json == NULL)
    {
        ERROR("Failed to dump session initialization message");
        return TE_ENOMEM;
    }
    te_string_append(&listener->buffer_out, "%s", json);
    free(json);
    json_decref(data);

    rc = listener_prepare_request(listener, "init", listener->buffer_out.ptr,
                                  listener->buffer_out.len);
    if (rc != 0)
    {
        ERROR("Listener %s: Failed to prepare /init request: %r",
              listener->name, rc);
        listener_free(listener);
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
    char          *url_suffix;

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
    url_suffix = te_string_fmt("feed?run=%s", listener->runid);
    rc = listener_prepare_request(listener, url_suffix, listener->buffer_out.ptr,
                                  (long)listener->buffer_out.len);
    free(url_suffix);
    if (rc != 0)
    {
        ERROR("Listener %s: Failed to prepare /feed request: %r",
              listener->name, rc);
        listener_free(listener);
        return rc;
    }

    listener->state = LISTENER_TRANSFERRING;
    listener->next_tv.tv_sec = 0;
    return 0;
}

/**
 * Process listener response after a /feed request.
 *
 * The listener is expected to return a JSON object with the following fields:
 * 1. "stop": (optional) stop the test execution.
 *            If the listener's "allow_stop" value is true and the Tester's
 *            PID is known, Tester will be sent a SIGINT. Otherwise, a
 *            corresponding error message will be logged and the execution will
 *            proceed as usual.
 */
static void
check_dump_response_body(log_listener *listener)
{
    int           ret;
    int           stop = 0;
    json_t       *body;
    json_error_t  err;

    body = json_loadb((const char *)listener->buffer_in.ptr,
                      listener->buffer_in.len, 0, &err);
    if (body == NULL)
    {
        ERROR("Error parsing HTTP body: %s (line %d, column %d)",
              err.text, err.line, err.column);
        return;
    }

    ret = json_unpack_ex(body, &err, JSON_STRICT, "{s?b}",
                         "stop", &stop);
    if (ret == -1)
    {
        ERROR("Error unpacking JSON log message: %s (line %d, column %d)",
                  err.text, err.line, err.column);
        json_decref(body);
        return;
    }

    if (stop)
    {
        if (!listener->allow_stop)
        {
            ERROR("Listener %s is not allowed to stop test execution",
                  listener->name);
        }
        else if (tester_pid <= 0)
        {
            ERROR("Failed to kill Tester: PID is unknown");
        }
        else
        {
            ret = kill(tester_pid, SIGINT);
            if (ret == -1)
                ERROR("Failed to kill Tester: %s", strerror(errno));
        }
    }

    json_decref(body);
}

/** Process listener's response to an init request */
static te_errno
listener_finish_request_init(log_listener *listener, long response_code)
{
    te_errno      rc;
    int           ret;
    json_t       *response;
    json_t       *runid;
    json_error_t  err;

    /* Check status */
    if (response_code != 200)
    {
        ERROR("Listener %s: /init returned %d", listener->name,
              response_code);
        listener_free(listener);
        return TE_EINVAL;
    }

    /* Store the run ID */
    response = json_loadb((const char *)listener->buffer_in.ptr,
                          listener->buffer_in.len, 0, &err);
    if (response == NULL)
    {
        ERROR("Listener returned malformed init JSON: "
              "%s (line %d, column %d)",
              err.text, err.line, err.column);
        listener_free(listener);
        return TE_EINVAL;
    }

    ret = json_unpack_ex(response, &err, 0, "{s:o}",
                         "runid", &runid);
    if (ret == -1)
    {
        ERROR("Failed to unpack listener init JSON: "
              "%s (line %d, column %d)",
              err.text, err.line, err.column);
        listener_free(listener);
        return TE_EINVAL;
    }

    switch (json_typeof(runid))
    {
        case JSON_INTEGER:
            rc = te_snprintf(listener->runid, sizeof(listener->runid),
                             "%lld", json_integer_value(runid));
            if (rc != 0)
            {
                ERROR("Failed to convert listener run ID to string: %r", rc);
                listener_free(listener);
                return rc;
            }
            break;
        case JSON_STRING:
            rc = te_strlcpy(listener->runid, json_string_value(runid),
                            sizeof(listener->runid));
            if (rc != 0)
            {
                ERROR("Failed to copy listener run ID: %r", rc);
                listener_free(listener);
                return rc;
            }
            break;
        default:
            ERROR("Failed to save listener run ID");
            listener_free(listener);
            return TE_EINVAL;
    }
    json_decref(response);

    RING("Listener %s: session initialized", listener->name);
    listener->state = LISTENER_GATHERING;
    return 0;
}

/* See description in logger_listener.h */
te_errno
listener_finish_request(log_listener *listener, CURLcode result)
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
    if (result != CURLE_OK)
    {
        ERROR("Listener %s: request failed: %s", listener->name,
              curl_easy_strerror(result));
        /*
         * Technically, this causes an infinite loop of retries if the
         * listener fails permanently.
         * However, these retries will happen on each heartbeat, so they
         * should not occupy our computational resources. They will also
         * not get in the way of Logger exiting once it's told to shutdown.
         */
        listener->need_retry = TRUE;
        gettimeofday(&listener->next_tv, NULL);
        listener->next_tv.tv_sec += listener->interval;
        return 0;
    }
    if (curl_easy_getinfo(listener->curl_handle, CURLINFO_RESPONSE_CODE,
                          &response_code) != CURLE_OK)
    {
        ERROR("Listener %s: failed to extract request response code",
              listener->name);
        listener_free(listener);
        return TE_EINVAL;
    }
    INFO("HTTP response: %d\n\n%.*s", response_code, listener->buffer_in.len,
         listener->buffer_in.ptr);
    switch (listener->state)
    {
        case LISTENER_INIT_WAITING:
            return listener_finish_request_init(listener, response_code);
        case LISTENER_TRANSFERRING:
            /* Check response code */
            if (response_code != 200)
            {
                ERROR("Listener %s: /feed returned %d", listener->name,
                      response_code);
                listener_free(listener);
                return TE_EINVAL;
            }
            check_dump_response_body(listener);
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
    te_errno        rc;
    double          ts;
    struct timeval  tv;
    json_t         *json;
    char           *url_suffix;
    char           *data;

    if (listener->state == LISTENER_INIT)
    {
        listener->state = LISTENER_FINISHED;
        return 0;
    }

    gettimeofday(&tv, NULL);
    ts = tv.tv_sec + tv.tv_usec / 1000000;
    json = json_pack("{s:f}",
                     "ts", ts);
    if (json == NULL)
    {
        ERROR("Failed to pack finish data into a JSON object");
        listener_free(listener);
        return TE_ENOMEM;
    }
    data = json_dumps(json, JSON_COMPACT);
    json_decref(json);
    if (data == NULL)
    {
        ERROR("Failed to encode finish data");
        listener_free(listener);
        return TE_ENOMEM;
    }
    te_string_reset(&listener->buffer_out);
    te_string_append(&listener->buffer_out, data);
    free(data);

    RING("Listener %s: finishing", listener->name);
    url_suffix = te_string_fmt("finish?run=%s", listener->runid);
    rc = listener_prepare_request(listener, url_suffix,
                                  listener->buffer_out.ptr,
                                  listener->buffer_out.len);
    free(url_suffix);
    if (rc != 0)
    {
        ERROR("Listener %s: Failed to prepare /finish request: %r",
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
    te_dbuf_free(&listener->buffer_in);
    te_string_free(&listener->buffer_out);
    msg_buffer_free(&listener->buffer);
    listener->state = LISTENER_FINISHED;
}
