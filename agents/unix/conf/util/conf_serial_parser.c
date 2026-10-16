/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent serial console parser support.
 *
 * Implementation of unix TA serial console parse configuring support.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef TE_LGR_USER
#define TE_LGR_USER      "Unix Conf Serial"
#endif

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include "te_alloc.h"
#include "te_serial_parser.h"
#include "te_kernel_log.h"
#include "te_alloc.h"

#include "logger_api.h"
#include "te_raw_log.h"
#include "te_sockaddr.h"
#include "te_string.h"
#include "te_vector.h"
#include "unix_internal.h"
#include "rcf_pch_tree.h"

#include <semaphore.h>

/**< Head of the parsers list */
static SLIST_HEAD(, serial_parser_t) parsers_h;

/**
 * Check return code of the pthread_mutex_lock and
 * pthread_mutex_unlock calls
 *
 * @param _rc   Returned value of the calls
 */
#define TE_SERIAL_CHECK_LOCK(_rc) \
{ \
    if (_rc != 0) \
    { \
        ERROR("Couldn't (un)lock the mutex %d: %s", _rc, strerror(_rc));\
        return TE_OS_RC(TE_TA_UNIX, _rc); \
    } \
}

/**
 * Searching for the parser by name.
 *
 * @param name        Parser name.
 *
 * @return The parser unit or @c NULL.
 */
static serial_parser_t *
parser_get_by_name(const char *name)
{
    serial_parser_t *parser;

    if (name == NULL)
        return NULL;

    SLIST_FOREACH(parser, &parsers_h, ent_pars_l)
    {
        if (strcmp(parser->name, name) == 0)
            return parser;
    }
    return NULL;
}

/**
 * Searching for the event by name
 *
 * @param parser    Pointer to the parser
 * @param name      Event name
 *
 * @return The event unit or @c NULL
 */
static serial_event_t *
parser_get_event_by_name(serial_parser_t *parser, const char *name)
{
    serial_event_t *event;

    if (name == NULL || parser == NULL)
        return NULL;

    SLIST_FOREACH(event, &parser->events, ent_ev_l)
    {
        if (strcmp(event->name, name) == 0)
            return event;
    }
    return NULL;
}

/**
 * Searching for the pattern by name
 *
 * @param event     Pointer to the event
 * @param name      Pattern name
 *
 * @return The pattern unit or @c NULL
 */
static serial_pattern_t *
parser_get_pattern_by_name(serial_event_t *event, const char *name)
{
    serial_pattern_t *pat;

    if (name == NULL || event == NULL)
        return NULL;

    SLIST_FOREACH(pat, &event->patterns, ent_pat_l)
    {
        if (strcmp(pat->name, name) == 0)
            return pat;
    }
    return NULL;
}

/**
 * Release patterns in the event
 *
 * @param event     The event location
 */
static void
parser_clean_event_patterns(serial_event_t *event)
{
    serial_pattern_t *pat;

    /* Cleanup the list of patterns */
    while (!SLIST_EMPTY(&event->patterns))
    {
        pat = SLIST_FIRST(&event->patterns);
        SLIST_REMOVE_HEAD(&event->patterns, ent_pat_l);
        free(pat);
    }
}

/**
 * Release events in the parser
 *
 * @param event     The parser location
 */
static void
parser_clean_parser_events(serial_parser_t *parser)
{
    serial_event_t *event;

    /* Cleanup the list of events */
    while (!SLIST_EMPTY(&parser->events))
    {
        event = SLIST_FIRST(&parser->events);
        parser_clean_event_patterns(event);
        SLIST_REMOVE_HEAD(&parser->events, ent_ev_l);
        free(event);
    }
}

/**
 * Stop the parser thread if it is enabled
 *
 * @param parser    The parser location
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_stop_thread(serial_parser_t *parser)
{
    if (parser->enable == true)
    {
        TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));

        /* Stop the thread */
        pthread_cancel(parser->thread);
        TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));
        pthread_join(parser->thread, NULL);
        parser->enable = false;
    }
    return 0;
}

/**
 * Find the parser a request is addressed to.
 *
 * @param ctx       Request context
 *
 * @return The parser unit or @c NULL.
 */
static serial_parser_t *
parser_find(const ta_conf_ctx *ctx)
{
    return parser_get_by_name(ta_conf_ctx_inst(ctx, "parser"));
}

/**
 * Find the parser event a request is addressed to.
 *
 * @param ctx       Request context
 * @param parser    Location for the parser
 *
 * @return The event unit or @c NULL.
 */
static serial_event_t *
parser_event_find(const ta_conf_ctx *ctx, serial_parser_t **parser)
{
    *parser = parser_find(ctx);
    return parser_get_event_by_name(*parser, ta_conf_ctx_inst(ctx, "event"));
}

/**
 * Add the parser object
 *
 * @param ctx       Request context
 * @param cname     The serial console name
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_add(const ta_conf_ctx *ctx, const char *cname)
{
    const char      *pname = ta_conf_ctx_inst(ctx, "parser");
    serial_parser_t *parser;
    int              rc;

    if (strlen(pname) > TE_SERIAL_MAX_NAME)
        return TE_RC(TE_TA_UNIX, TE_ENAMETOOLONG);
    if (parser_get_by_name(pname) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    parser = TE_ALLOC(sizeof(serial_parser_t));
    parser->enable      = false;
    parser->rcf         = false;
    parser->port        = TE_SERIAL_PORT;
    parser->interval    = TE_SERIAL_INTERVAL;
    parser->logging     = true;
    parser->level       = map_name_to_level(TE_SERIAL_LLEVEL);

    te_strlcpy(parser->name, pname, sizeof(parser->name));
    te_strlcpy(parser->c_name, cname, sizeof(parser->c_name));
    te_strlcpy(parser->user, TE_SERIAL_USER, sizeof(parser->user));
    memset(parser->mode, 0, TE_SERIAL_MAX_NAME + 1);

    rc = pthread_mutex_init(&parser->mutex, NULL);
    if (rc != 0)
    {
        ERROR("Couldn't init mutex of the %s parser, error: %s",
              parser->name, strerror(rc));
        free(parser);
        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    SLIST_INIT(&parser->events);

    SLIST_INSERT_HEAD(&parsers_h, parser, ent_pars_l);

    return 0;
}

/**
 * Delete the parser object.
 *
 * @param ctx       Request context
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_del(const ta_conf_ctx *ctx)
{
    serial_parser_t *parser;

    parser = parser_find(ctx);
    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    parser_stop_thread(parser);
    parser_clean_parser_events(parser);

    SLIST_REMOVE(&parsers_h, parser, serial_parser_t, ent_pars_l);
    free(parser);

    return 0;
}

/**
 * Set a serial console name for the parser.
 *
 * @param ctx       Request context
 * @param cname     Serial console name
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_set(const ta_conf_ctx *ctx, const char *cname)
{
    serial_parser_t *parser;

    parser = parser_find(ctx);
    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_strlcpy(parser->c_name, cname, sizeof(parser->c_name));

    return 0;
}

/**
 * Get a serial console name of the parser.
 *
 * @param ctx       Request context
 * @param cname     Location for the serial console name
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_get(const ta_conf_ctx *ctx, te_string *cname)
{
    serial_parser_t *parser;

    parser = parser_find(ctx);
    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_append(cname, "%s", parser->c_name);

    return 0;
}

/**
 * Get instance list of the parsers.
 *
 * @param ctx       Request context (unused)
 * @param names     Vector of heap-allocated names to append to
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parsers_list(const ta_conf_ctx *ctx, te_vec *names)
{
    serial_parser_t  *parser;

    UNUSED(ctx);

    SLIST_FOREACH(parser, &parsers_h, ent_pars_l)
    {
        char *name = TE_STRDUP(parser->name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

/**
 * Parser thread entry point.
 *
 * @param arg       The parser location
 *
 * @return @c NULL
 */
static void *
parser_thread(void *arg)
{
    te_serial_parser_connect(arg);
    return NULL;
}

/**
 * Get the parser thread state.
 *
 * @param ctx       Request context
 * @param val       Location for the state
 *
 * @return Status code
 */
static te_errno
parser_enable_get(const ta_conf_ctx *ctx, int32_t *val)
{
    serial_parser_t *parser = parser_find(ctx);

    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = parser->enable;
    return 0;
}

/**
 * Start/stop the parser thread.
 *
 * @param ctx       Request context
 * @param val       Non-zero to start, 0 to stop
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_set_enable(const ta_conf_ctx *ctx, int32_t val)
{
    serial_parser_t *parser;
    bool en;
    int              rc;

    parser = parser_find(ctx);
    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    en = val == 0 ? false : true;

    if (en == parser->enable)
        return 0;
    if (en == true)
    {
        rc = pthread_create(&parser->thread, NULL, parser_thread, parser);
        if (rc != 0)
        {
            ERROR("Couldn't to start the parser thread %s, oid %s",
                  parser->name, ta_conf_ctx_oid(ctx));
            return TE_OS_RC(TE_TA_UNIX, rc);
        }
    }
    else
    {
        /* Stop the thread */
        TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
        rc = pthread_cancel(parser->thread);
        TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));
        pthread_join(parser->thread, NULL);
    }

    parser->enable = en;

    return 0;
}

/**
 * Get the conserver port of the parser.
 *
 * @param ctx       Request context
 * @param val       Location for the port
 *
 * @return Status code
 */
static te_errno
parser_port_get(const ta_conf_ctx *ctx, int32_t *val)
{
    serial_parser_t *parser = parser_find(ctx);

    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = parser->port;
    return 0;
}

/**
 * Set the conserver port of the parser.
 *
 * @param ctx       Request context
 * @param val       Port
 *
 * @return Status code
 */
static te_errno
parser_port_set(const ta_conf_ctx *ctx, int32_t val)
{
    serial_parser_t *parser = parser_find(ctx);

    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    parser->port = val;
    return 0;
}

/**
 * Get the conserver user of the parser.
 *
 * @param ctx       Request context
 * @param val       Location for the user name
 *
 * @return Status code
 */
static te_errno
parser_user_get(const ta_conf_ctx *ctx, te_string *val)
{
    serial_parser_t *parser = parser_find(ctx);

    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_append(val, "%s", parser->user);
    return 0;
}

/**
 * Set the conserver user of the parser.
 *
 * @param ctx       Request context
 * @param val       User name
 *
 * @return Status code
 */
static te_errno
parser_user_set(const ta_conf_ctx *ctx, const char *val)
{
    serial_parser_t *parser = parser_find(ctx);

    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(parser->user, TE_SERIAL_MAX_NAME, "%s", val);
    return 0;
}

/**
 * Get the console polling interval of the parser.
 *
 * @param ctx       Request context
 * @param val       Location for the interval, in milliseconds
 *
 * @return Status code
 */
static te_errno
parser_interval_get(const ta_conf_ctx *ctx, int32_t *val)
{
    serial_parser_t *parser = parser_find(ctx);

    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = parser->interval;
    return 0;
}

/**
 * Set the console polling interval of the parser.
 *
 * @param ctx       Request context
 * @param val       Interval, in milliseconds, -1 for the default
 *
 * @return Status code
 */
static te_errno
parser_interval_set(const ta_conf_ctx *ctx, int32_t val)
{
    serial_parser_t *parser = parser_find(ctx);

    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    parser->interval = val;
    if (parser->interval == -1)
        parser->interval = TE_SERIAL_INTERVAL;
    return 0;
}

/**
 * Get the reset node value: always 0.
 *
 * @param ctx       Request context
 * @param val       Location for the value
 *
 * @return Status code
 */
static te_errno
parser_reset_get(const ta_conf_ctx *ctx, int32_t *val)
{
    if (parser_find(ctx) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = 0;
    return 0;
}

/**
 * Reset status of each event of the parser.
 *
 * @param ctx       Request context
 * @param val       Non-zero to reset, 0 to do nothing
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_reset(const ta_conf_ctx *ctx, int32_t val)
{
    serial_parser_t  *parser;
    serial_event_t   *event;

    if (val == 0)
        return 0;

    parser = parser_find(ctx);
    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    SLIST_FOREACH(event, &parser->events, ent_ev_l)
        event->status = false;
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

/**
 * Get the console logging state of the parser.
 *
 * @param ctx       Request context
 * @param val       Location for the state
 *
 * @return Status code
 */
static te_errno
parser_logging_get(const ta_conf_ctx *ctx, int32_t *val)
{
    serial_parser_t *parser = parser_find(ctx);

    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    *val = parser->logging;
    return 0;
}

/**
 * Enable/disable the console logging of the parser.
 *
 * @param ctx       Request context
 * @param val       Non-zero to enable, 0 to disable
 *
 * @return Status code
 */
static te_errno
parser_logging_set(const ta_conf_ctx *ctx, int32_t val)
{
    serial_parser_t *parser = parser_find(ctx);

    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    parser->logging = val == 0 ? false : true;
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));
    return 0;
}

/**
 * Get the Logger level of the console messages.
 *
 * @param ctx       Request context
 * @param val       Location for the level name
 *
 * @return Status code
 */
static te_errno
parser_log_level_get(const ta_conf_ctx *ctx, te_string *val)
{
    serial_parser_t *parser = parser_find(ctx);
    const char      *log_level_str;

    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    log_level_str = te_log_level2str(parser->level);
    te_string_append(val, "%s",
                     log_level_str != NULL ? log_level_str : TE_LL_WARN_STR);
    return 0;
}

/**
 * Set the Logger level of the console messages.
 *
 * @param ctx       Request context
 * @param val       Level name
 *
 * @return Status code
 */
static te_errno
parser_log_level_set(const ta_conf_ctx *ctx, const char *val)
{
    serial_parser_t *parser = parser_find(ctx);

    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    parser->level = map_name_to_level(val);
    if (parser->level == 0)
        parser->level = TE_LL_WARN;
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));
    return 0;
}

/**
 * Get the Logger user of the console messages.
 *
 * @param ctx       Request context
 * @param val       Location for the user name
 *
 * @return Status code
 */
static te_errno
parser_log_user_get(const ta_conf_ctx *ctx, te_string *val)
{
    serial_parser_t *parser = parser_find(ctx);

    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_append(val, "%s", parser->log_user);
    return 0;
}

/**
 * Set the Logger user of the console messages.
 *
 * @param ctx       Request context
 * @param val       User name
 *
 * @return Status code
 */
static te_errno
parser_log_user_set(const ta_conf_ctx *ctx, const char *val)
{
    serial_parser_t *parser = parser_find(ctx);

    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    snprintf(parser->log_user, TE_SERIAL_MAX_NAME, "%s", val);
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));
    return 0;
}

/**
 * Get instance list of the events, that are located on the parser subtree
 *
 * @param ctx       Request context (the parser instance)
 * @param names     Vector of heap-allocated names to append to
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_list(const ta_conf_ctx *ctx, te_vec *names)
{
    serial_parser_t  *parser;
    serial_event_t   *event;

    parser = parser_find(ctx);
    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(event, &parser->events, ent_ev_l)
    {
        char *name = TE_STRDUP(event->name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

/**
 * Add event to the parser
 *
 * @param ctx       Request context
 * @param t_name    The Tester event name
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_add(const ta_conf_ctx *ctx, const char *t_name)
{
    const char      *ename = ta_conf_ctx_inst(ctx, "event");
    serial_parser_t *parser;
    serial_event_t  *event;

    parser = parser_find(ctx);
    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    if (strlen(ename) > TE_SERIAL_MAX_NAME)
        return TE_RC(TE_TA_UNIX, TE_ENAMETOOLONG);
    if (parser_get_event_by_name(parser, ename) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    event = TE_ALLOC_UNINITIALIZED(sizeof(serial_event_t));

    event->count                      = 0;
    event->status                     = false;

    te_strlcpy(event->name, ename, sizeof(event->name));
    te_strlcpy(event->t_name, t_name, sizeof(event->t_name));

    SLIST_INIT(&event->patterns);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    SLIST_INSERT_HEAD(&parser->events, event, ent_ev_l);
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

/**
 * Delete event from the parser
 *
 * @param ctx       Request context
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_del(const ta_conf_ctx *ctx)
{
    serial_parser_t     *parser;
    serial_event_t      *event;

    event = parser_event_find(ctx, &parser);
    if (event == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    parser_clean_event_patterns(event);

    SLIST_REMOVE(&parser->events, event, serial_event_t, ent_ev_l);
    free(event);
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

/**
 * Get value (tester event name) of the parser event
 *
 * @param ctx       Request context
 * @param value     Location for the value of the parser event
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_get(const ta_conf_ctx *ctx, te_string *value)
{
    serial_parser_t     *parser;
    serial_event_t      *event;

    event = parser_event_find(ctx, &parser);
    if (event == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_append(value, "%s", event->t_name);

    return 0;
}

/**
 * Set value (tester event name) of the parser event
 *
 * @param ctx       Request context
 * @param value     Value of the parser event
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_set(const ta_conf_ctx *ctx, const char *value)
{
    serial_parser_t     *parser;
    serial_event_t      *event;

    event = parser_event_find(ctx, &parser);
    if (event == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_strlcpy(event->t_name, value, sizeof(event->t_name));

    return 0;
}


/**
 * Get instance list of the patterns, that are located on the event subtree
 *
 * @param ctx       Request context (the event instance)
 * @param names     Vector of heap-allocated names to append to
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_pattern_list(const ta_conf_ctx *ctx, te_vec *names)
{
    serial_parser_t  *parser;
    serial_event_t   *event;
    serial_pattern_t *pat;

    event = parser_event_find(ctx, &parser);
    if (event == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    SLIST_FOREACH(pat, &event->patterns, ent_pat_l)
    {
        char *name = TE_STRDUP(pat->name);

        TE_VEC_APPEND(names, name);
    }

    return 0;
}

/**
 * Add pattern to the event
 *
 * @param ctx       Request context
 * @param pattern   A pattern expression string
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_pattern_add(const ta_conf_ctx *ctx, const char *pattern)
{
    const char          *name = ta_conf_ctx_inst(ctx, "pattern");
    serial_parser_t     *parser;
    serial_event_t      *event;
    serial_pattern_t    *pat;

    if (atoi(name) <= 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    event = parser_event_find(ctx, &parser);
    if (event == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    if (strlen(name) > TE_SERIAL_MAX_NAME)
        return TE_RC(TE_TA_UNIX, TE_ENAMETOOLONG);
    if (parser_get_pattern_by_name(event, name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    pat = TE_ALLOC_UNINITIALIZED(sizeof(serial_pattern_t));

    te_strlcpy(pat->name, name, sizeof(pat->name));
    te_strlcpy(pat->v, pattern, sizeof(pat->v));

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    SLIST_INSERT_HEAD(&event->patterns, pat, ent_pat_l);
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

/**
 * Find the event pattern a request is addressed to.
 *
 * @param ctx       Request context
 * @param parser    Location for the parser
 * @param event     Location for the event
 *
 * @return The pattern unit or @c NULL.
 */
static serial_pattern_t *
parser_pattern_find(const ta_conf_ctx *ctx, serial_parser_t **parser,
                    serial_event_t **event)
{
    *event = parser_event_find(ctx, parser);
    return parser_get_pattern_by_name(*event,
                                      ta_conf_ctx_inst(ctx, "pattern"));
}

/**
 * Delete a pattern from an event
 *
 * @param ctx       Request context
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_pattern_del(const ta_conf_ctx *ctx)
{
    serial_parser_t     *parser;
    serial_event_t      *event;
    serial_pattern_t    *pat;

    pat = parser_pattern_find(ctx, &parser, &event);
    if (pat == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    SLIST_REMOVE(&event->patterns, pat, serial_pattern_t, ent_pat_l);
    free(pat);
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

/**
 * Get value (pattern) of the pattern instance
 *
 * @param ctx       Request context
 * @param value     Location for the pattern
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_pattern_get(const ta_conf_ctx *ctx, te_string *value)
{
    serial_parser_t     *parser;
    serial_event_t      *event;
    serial_pattern_t    *pat;

    pat = parser_pattern_find(ctx, &parser, &event);
    if (pat == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    te_string_append(value, "%s", pat->v);

    return 0;
}

/**
 * Set value (pattern) of the pattern instance
 *
 * @param ctx       Request context
 * @param value     The pattern
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_pattern_set(const ta_conf_ctx *ctx, const char *value)
{
    serial_parser_t     *parser;
    serial_event_t      *event;
    serial_pattern_t    *pat;

    pat = parser_pattern_find(ctx, &parser, &event);
    if (pat == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    te_strlcpy(pat->v, value, sizeof(pat->v));
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

/**
 * Get the counter of the event occurrences
 *
 * @param ctx       Request context
 * @param val       Location for the counter
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_counter_get(const ta_conf_ctx *ctx, int32_t *val)
{
    serial_parser_t     *parser;
    serial_event_t      *event;

    event = parser_event_find(ctx, &parser);
    if (event == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    *val = event->count;
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

/**
 * Set the counter of the event occurrences
 *
 * @param ctx       Request context
 * @param val       Counter
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_counter_set(const ta_conf_ctx *ctx, int32_t val)
{
    serial_parser_t     *parser;
    serial_event_t      *event;

    event = parser_event_find(ctx, &parser);
    if (event == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    event->count = val;
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

/**
 * Get the event status
 *
 * @param ctx       Request context
 * @param val       Location for the status
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_status_get(const ta_conf_ctx *ctx, int32_t *val)
{
    serial_parser_t     *parser;
    serial_event_t      *event;

    event = parser_event_find(ctx, &parser);
    if (event == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    *val = event->status;
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

/**
 * Set the event status
 *
 * @param ctx       Request context
 * @param val       Non-zero if the event has been detected
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_status_set(const ta_conf_ctx *ctx, int32_t val)
{
    serial_parser_t     *parser;
    serial_event_t      *event;

    event = parser_event_find(ctx, &parser);
    if (event == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    event->status = val == 0 ? false : true;
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

static const ta_conf_node *const node_parser =
    TA_CONF_COLL_STR_RW("parser", parser_get, parser_set,
                        parser_add, parser_del, parsers_list,
        TA_CONF_RW_INT32("enable", parser_enable_get, parser_set_enable),
        TA_CONF_RW_INT32("port", parser_port_get, parser_port_set),
        TA_CONF_RW_STR("user", parser_user_get, parser_user_set),
        TA_CONF_RW_INT32("interval", parser_interval_get,
                         parser_interval_set),
        TA_CONF_RW_INT32("reset", parser_reset_get, parser_reset),
        TA_CONF_RW_INT32("logging", parser_logging_get, parser_logging_set,
            TA_CONF_RW_STR("level", parser_log_level_get,
                           parser_log_level_set),
            TA_CONF_RW_STR("user", parser_log_user_get,
                           parser_log_user_set)),
        TA_CONF_COLL_STR_RW("event", parser_event_get, parser_event_set,
                            parser_event_add, parser_event_del,
                            parser_event_list,
            TA_CONF_COLL_STR_RW("pattern", parser_pattern_get,
                                parser_pattern_set, parser_pattern_add,
                                parser_pattern_del, parser_pattern_list),
            TA_CONF_RW_INT32("counter", parser_event_counter_get,
                             parser_event_counter_set),
            TA_CONF_RW_INT32("status", parser_event_status_get,
                             parser_event_status_set)));

/* See description in conf_serial_parser.h */
te_errno
ta_unix_serial_parser_init(void)
{
    SLIST_INIT(&parsers_h);

    return ta_conf_register("/agent", node_parser);
}

/* See description in conf_serial_parser.h */
te_errno
ta_unix_serial_parser_cleanup(void)
{
    serial_parser_t     *parser;

    while(!SLIST_EMPTY(&parsers_h))
    {
        parser = SLIST_FIRST(&parsers_h);
        if (parser == NULL)
            break;

        parser_stop_thread(parser);

        parser_clean_parser_events(parser);
        SLIST_REMOVE_HEAD(&parsers_h, ent_pars_l);
        free(parser);
    }

    return 0;
}
