/** @file
 * @brief Unix Test Agent serial console parser support.
 *
 * Implementation of unix TA serial console parse configuring support.
 *
 * Copyright (C) 2004-2012 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef TE_LGR_USER
#define TE_LGR_USER      "Unix Conf Serial"
#endif

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "conf_serial_parser.h"

#include "logger_api.h"
#include "te_raw_log.h"
#include "te_sockaddr.h"
#include "unix_internal.h"

#include <semaphore.h>

/**< Head of the parsers list */
SLIST_HEAD(, serial_parser_t) parsers_h;

/**
 * Map Logger level name to the value
 * 
 * @param name  Name of the level
 * 
 * @return Level number
 */
static te_log_level
map_name_to_level(const char *name)
{
    static const struct {
        char           *name;
        te_log_level    level;
    } levels[] = {{"ERROR", TE_LL_ERROR},
                  {"WARN",  TE_LL_WARN},
                  {"RING",  TE_LL_RING},
                  {"INFO",  TE_LL_INFO},
                  {"VERB",  TE_LL_VERB},
                  {"PACKET", TE_LL_PACKET}};
    unsigned i;

    for (i = 0; i < sizeof(levels) / sizeof(*levels); i++)
    {
        if (!strcmp(levels[i].name, name))
            return levels[i].level;
    }
    return 0;
}

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
    if (parser->enable == TRUE)
    {
        TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));

        /* Stop the thread */
        pthread_cancel(parser->thread);
        TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));
        parser->enable = FALSE;
    }
    return 0;
}

/**
 * Add the parser object
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param cname     The serial console name
 * @param pname     The parser name
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_add(unsigned int gid, const char *oid, char *cname,
           const char *pname)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t *parser;
    int              rc;

    TE_SERIAL_MALLOC(parser, sizeof(serial_parser_t));
    parser->enable      = FALSE;
    parser->port        = TE_SERIAL_PORT;
    parser->interval    = TE_SERIAL_INTERVAL;
    parser->logging     = TRUE;
    parser->level       = map_name_to_level(TE_SERIAL_LLEVEL);

    parser->name[TE_SERIAL_MAX_NAME]    = '\0';
    parser->c_name[TE_SERIAL_MAX_NAME]  = '\0';
    strncpy(parser->name, pname, TE_SERIAL_MAX_NAME);
    strncpy(parser->c_name, cname, TE_SERIAL_MAX_NAME);
    strncpy(parser->user, TE_SERIAL_USER, TE_SERIAL_MAX_NAME);
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
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier (unused).
 * @param pname        The parser name.
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_del(unsigned int gid, const char *oid, const char *pname)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t *parser;

    parser = parser_get_by_name(pname);
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
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier (unused).
 * @param cname        Serial console name.
 * @param pname        The parser name.
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_set(unsigned int gid, const char *oid, char *cname,
           const char *pname)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t *parser;

    parser = parser_get_by_name(pname);
    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strncpy(parser->c_name, cname, TE_SERIAL_MAX_NAME - 1);

    return 0;
}

/**
 * Get a serial console name of the parser.
 *
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier (unused).
 * @param cname        Serial console name.
 * @param pname        The parser name.
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_get(unsigned int gid, const char *oid, char *cname,
           const char *pname)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t *parser;

    parser = parser_get_by_name(pname);
    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strncpy(cname, parser->c_name, TE_SERIAL_MAX_NAME - 1);

    return 0;
}

/**
 * Get instance list of the parsers.
 *
 * @param gid       Group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param list      Location for the list pointer.
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parsers_list(unsigned int gid, const char *oid, char **list)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t  *parser;
    size_t            list_size = 0;
    size_t            list_len  = 0;

    list_size = PARSER_LIST_SIZE;
    TE_SERIAL_MALLOC(*list, list_size);
    memset(*list, 0, list_size);

    list_len = 0;
    SLIST_FOREACH(parser, &parsers_h, ent_pars_l)
    {
        if (list_len + strlen(parser->name) + 1 >= list_size)
        {
            list_size *= 2;
            if ((*list = realloc(*list, list_size)) == NULL)
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }

        list_len += sprintf(*list + list_len, "%s ", parser->name);
    }

    return 0;
}

/**
 * Start/stop the parser thread.
 *
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier (unused).
 * @param value        New value.
 * @param pname        The parser name.
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_set_enable(unsigned int gid, const char *oid, char *value,
                  const char *pname)
{
    UNUSED(gid);

    serial_parser_t *parser;
    te_bool          en;
    int              rc;

    parser = parser_get_by_name(pname);
    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    en = atoi(value) == 0 ? FALSE : TRUE;

    if (en == parser->enable)
        return 0;
    if (en == TRUE)
    {
        rc = pthread_create(&parser->thread, NULL,
                            (void *)&te_serial_parser, parser);
        if (rc != 0)
        {
            ERROR("Couldn't to start the parser thread %s, oid %s",
                  pname, oid);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
    }
    else
    {
        /* Stop the thread */
        TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
        rc = pthread_cancel(parser->thread);
        TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));
    }

    parser->enable = en;

    return 0;
}

/**
 * Common function to set variable values.
 *
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier (unused).
 * @param value        New value.
 * @param pname        The parser name.
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_common_set(unsigned int gid, const char *oid, char *value,
                  const char *pname)
{
    UNUSED(gid);

    serial_parser_t *parser;

    parser = parser_get_by_name(pname);
    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    if (value == NULL)
    {
        ERROR("A buffer to set a variable value is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (strstr(oid, "/port:") != NULL)
        parser->port= atoi(value);
    else if (strstr(oid, "/user:") != NULL)
        snprintf(parser->user, TE_SERIAL_MAX_NAME, "%s", value);
    else if (strstr(oid, "/mode:") != NULL)
        snprintf(parser->mode, TE_SERIAL_MAX_NAME, "%s", value);
    else if (strstr(oid, "/interval:") != NULL)
    {
        parser->interval = atoi(value);
        if (parser->interval == -1)
            parser->interval = TE_SERIAL_INTERVAL;
    }
    else if (strstr(oid, "/logging:") != NULL)
    {
        TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
        if (strstr(oid, "/level:") != NULL)
        {
            parser->level = map_name_to_level(value);
            if (parser->level == 0)
                parser->level = TE_LL_WARN;
        }
        else
            parser->logging = atoi(value) == 0 ? FALSE : TRUE;
        TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));
    }
    else
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return 0;
}

/**
 * Common function to get variable values.
 *
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier (unused).
 * @param value        New value (OUT).
 * @param pname        The parser name.
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_common_get(unsigned int gid, const char *oid, char *value,
                  const char *pname)
{
    UNUSED(gid);

    serial_parser_t *parser;

    parser = parser_get_by_name(pname);
    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    if (value == NULL)
    {
       ERROR("A buffer to get a variable value is not provided");
       return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (strstr(oid, "/enable:") != NULL)
        snprintf(value, RCF_MAX_VAL, "%d", parser->enable);
    else if (strstr(oid, "/port:") != NULL)
        snprintf(value, RCF_MAX_VAL, "%d", parser->port);
    else if (strstr(oid, "/user:") != NULL)
        snprintf(value, RCF_MAX_VAL, "%s", parser->user);
    else if (strstr(oid, "/interval:") != NULL)
        snprintf(value, RCF_MAX_VAL, "%d", parser->interval);
    else if (strstr(oid, "/reset:") != NULL)
        snprintf(value, RCF_MAX_VAL, "%d", 0);
    else if (strstr(oid, "/mode:") != NULL)
        snprintf(value, RCF_MAX_VAL, "%s", parser->mode);
    else if (strstr(oid, "/logging:") != NULL)
    {
        if (strstr(oid, "/level:") != NULL)
            snprintf(value, RCF_MAX_VAL, "%d", parser->level);
        else
            snprintf(value, RCF_MAX_VAL, "%d", parser->logging);
    }
    else
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return 0;
}

/**
 * Reset status of each event of the parser.
 *
 * @param gid          Group identifier (unused).
 * @param oid          Full object instance identifier (unused).
 * @param value        New value.
 * @param pname        The parser name.
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_reset(unsigned int gid, const char *oid, char *value,
             const char *pname)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t  *parser;
    serial_event_t   *event;
    serial_pattern_t *pat;

    if (atoi(value) == FALSE)
        return 0;

    parser = parser_get_by_name(pname);
    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    SLIST_FOREACH(event, &parser->events, ent_ev_l)
    {
        SLIST_FOREACH(pat, &event->patterns, ent_pat_l)
        {
            event->status = FALSE;
        }
    }
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

/**
 * Get instance list of the events, that are located on the parser subtree
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param list      Location for the list pointer
 * @param pname     The parser name
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_list(unsigned int gid, const char *oid, char **list,
                  const char *pname)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t  *parser;
    serial_event_t   *event;
    size_t            list_size = 0;
    size_t            list_len  = 0;

    parser = parser_get_by_name(pname);
    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    list_size = PARSER_LIST_SIZE;
    TE_SERIAL_MALLOC(*list, list_size);
    memset(*list, 0, list_size);

    list_len = 0;

    SLIST_FOREACH(event, &parser->events, ent_ev_l)
    {
        if (list_len + strlen(event->name) + 1 >= list_size)
        {
            list_size *= 2;
            if ((*list = realloc(*list, list_size)) == NULL)
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }

        list_len += sprintf(*list + list_len, "%s ", event->name);
    }

    return 0;
}

/**
 * Add event to the parser
 * 
 * @param gid       Group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param t_name    The Tester event name
 * @param pname     The parser name
 * @param ename     Parser event name
 * 
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_add(unsigned int gid, const char *oid, char *t_name,
                 const char *pname, const char *ename)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t *parser;
    serial_event_t  *event;

    parser = parser_get_by_name(pname);
    if (parser == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_MALLOC(event, sizeof(serial_event_t));

    event->count                      = 0;
    event->status                     = FALSE;
    event->name[TE_SERIAL_MAX_NAME]   = '\0';
    event->t_name[TE_SERIAL_MAX_NAME] = '\0';

    strncpy(event->name, ename, TE_SERIAL_MAX_NAME);
    strncpy(event->t_name, t_name, TE_SERIAL_MAX_NAME);

    SLIST_INIT(&event->patterns);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    SLIST_INSERT_HEAD(&parser->events, event, ent_ev_l);
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

/**
 * Delete event from the parser
 * 
 * @param gid       Group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param ename     Parser event name
 * @param pname     The parser name
 * 
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_del(unsigned int gid, const char *oid, const char *pname,
                 const char *ename)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t     *parser;
    serial_event_t      *event;

    parser = parser_get_by_name(pname);
    event = parser_get_event_by_name(parser, ename);
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
 * @param gid       Group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param value     Value of the parser event (OUT)
 * @param pname     The parser name
 * @param ename     Parser event name
 * 
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_get(unsigned int gid, const char *oid, char *value,
                 const char *pname, const char *ename)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t     *parser;
    serial_event_t      *event;

    parser = parser_get_by_name(pname);
    event = parser_get_event_by_name(parser, ename);
    if (event == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strncpy(value, event->t_name, RCF_MAX_VAL);

    return 0;
}

/**
 * Set value (tester event name) of the parser event
 * 
 * @param gid       Group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param value     Value of the parser event 
 * @param pname     The parser name
 * @param ename     Parser event name
 * 
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_set(unsigned int gid, const char *oid, char *value,
                 const char *pname, const char *ename)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t     *parser;
    serial_event_t      *event;

    parser = parser_get_by_name(pname);
    event = parser_get_event_by_name(parser, ename);
    if (event == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strncpy(event->t_name, value, TE_SERIAL_MAX_NAME);

    return 0;
}


/**
 * Get instance list of the patterns, that are located on the event subtree
 *
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param list      Location for the list pointer
 * @param pname     The parser name
 * @param ename     The event name
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_pattern_list(unsigned int gid, const char *oid, char **list,
                    const char *pname, const char *ename)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t  *parser;
    serial_event_t   *event;
    serial_pattern_t *pat;
    size_t            list_size = 0;
    size_t            list_len  = 0;

    parser = parser_get_by_name(pname);
    event = parser_get_event_by_name(parser, ename);
    if (event == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    list_size = PARSER_LIST_SIZE;
    TE_SERIAL_MALLOC(*list, list_size);
    memset(*list, 0, list_size);

    list_len = 0;

    SLIST_FOREACH(pat, &event->patterns, ent_pat_l)
    {
        if (list_len + strlen(pat->name) + 1 >= list_size)
        {
            list_size *= 2;
            if ((*list = realloc(*list, list_size)) == NULL)
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }

        list_len += sprintf(*list + list_len, "%s ", pat->name);
    }

    return 0;
}

/**
 * Add pattern to the event
 * 
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param pattern   A pattern expression string
 * @param pname     The parser name
 * @param ename     Parser event name
 * @param name      Pattern name
 * 
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_pattern_add(unsigned int gid, const char *oid, const char *pattern,
                 const char *pname, const char *ename, const char *name)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t     *parser;
    serial_event_t      *event;
    serial_pattern_t    *pat;

    if (atoi(name) <= 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    parser = parser_get_by_name(pname);
    event = parser_get_event_by_name(parser, ename);
    if (event == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_MALLOC(pat, sizeof(serial_pattern_t));

    pat->name[TE_SERIAL_MAX_NAME]   = '\0';
    pat->v[TE_SERIAL_MAX_PATT]      = '\0';

    strncpy(pat->name, name, TE_SERIAL_MAX_NAME);
    strncpy(pat->v, pattern, TE_SERIAL_MAX_PATT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    SLIST_INSERT_HEAD(&event->patterns, pat, ent_pat_l);
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

/**
 * Delete a pattern from an event
 * 
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param pname     The parser name
 * @param ename     The parser event name
 * @param name      The pattern name
 * 
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_pattern_del(unsigned int gid, const char *oid, const char *pname,
                   const char *ename, const char *name)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t     *parser;
    serial_event_t      *event;
    serial_pattern_t    *pat;

    parser = parser_get_by_name(pname);
    event  = parser_get_event_by_name(parser, ename);
    pat    = parser_get_pattern_by_name(event, name);
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
 * @param gid       Group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param value     Value of the pattern (OUT)
 * @param pname     The parser name
 * @param ename     The parser event name
 * @param name      The pattern name
 * 
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_pattern_get(unsigned int gid, const char *oid, char *value,
                   const char *pname, const char *ename, const char *name)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t     *parser;
    serial_event_t      *event;
    serial_pattern_t    *pat;

    parser = parser_get_by_name(pname);
    event  = parser_get_event_by_name(parser, ename);
    pat    = parser_get_pattern_by_name(event, name);
    if (pat == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strncpy(value, pat->v, RCF_MAX_VAL);

    return 0;
}

/**
 * Set value (pattern) of the pattern instance
 * 
 * @param gid       Group identifier (unused).
 * @param oid       Full object instance identifier (unused).
 * @param value     Value of the pattern (OUT)
 * @param pname     The parser name
 * @param ename     The parser event name
 * @param name      The pattern name
 * 
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_pattern_set(unsigned int gid, const char *oid, char *value,
                   const char *pname, const char *ename, const char *name)
{
    UNUSED(gid);
    UNUSED(oid);

    serial_parser_t     *parser;
    serial_event_t      *event;
    serial_pattern_t    *pat;

    parser = parser_get_by_name(pname);
    event  = parser_get_event_by_name(parser, ename);
    pat    = parser_get_pattern_by_name(event, name);
    if (pat == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    strncpy(pat->v, value, TE_SERIAL_MAX_PATT);
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

/**
 * Get value of the event instance variable
 * 
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param value     Value of the pattern (OUT)
 * @param pname     The parser name
 * @param ename     The parser event name
 * @param name      The instance name
 * 
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_common_get(unsigned int gid, const char *oid, char *value,
                        const char *pname, const char *ename,
                        const char *name)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(name);

    serial_parser_t     *parser;
    serial_event_t      *event;

    parser = parser_get_by_name(pname);
    event  = parser_get_event_by_name(parser, ename);
    if (event == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    if (strstr(oid, "/status:") != NULL)
        snprintf(value, RCF_MAX_VAL, "%d", event->status);
    else if (strstr(oid, "/counter:") != NULL)
        snprintf(value, RCF_MAX_VAL, "%d", event->count);
    else
    {
        TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

/**
 * Set value of the event instance variable
 * 
 * @param gid       Group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param value     Value of the pattern
 * @param pname     The parser name
 * @param ename     The parser event name
 * @param name      The instance name
 * 
 * @return Status code
 * @retval 0        Success
 */
static te_errno
parser_event_common_set(unsigned int gid, const char *oid, char *value,
                        const char *pname, const char *ename,
                        const char *name)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(name);

    serial_parser_t     *parser;
    serial_event_t      *event;

    parser = parser_get_by_name(pname);
    event  = parser_get_event_by_name(parser, ename);
    if (event == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TE_SERIAL_CHECK_LOCK(pthread_mutex_lock(&parser->mutex));
    if (strstr(oid, "/status:") != NULL)
        event->status = atoi(value) == 0 ? FALSE : TRUE;
    else if (strstr(oid, "/counter:") != NULL)
        event->count = atoi(value);
    else
    {
        TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    TE_SERIAL_CHECK_LOCK(pthread_mutex_unlock(&parser->mutex));

    return 0;
}

RCF_PCH_CFG_NODE_RW(serial_event_status, "status", NULL, NULL,
                    parser_event_common_get, parser_event_common_set);

RCF_PCH_CFG_NODE_RW(serial_event_counter, "counter", NULL,
                    &serial_event_status, parser_event_common_get,
                    parser_event_common_set);

static rcf_pch_cfg_object serial_pattern =
    { "pattern", 0, NULL, &serial_event_counter,
     (rcf_ch_cfg_get)parser_pattern_get, (rcf_ch_cfg_set)parser_pattern_set,
     (rcf_ch_cfg_add)parser_pattern_add, (rcf_ch_cfg_del)parser_pattern_del,
     (rcf_ch_cfg_list)parser_pattern_list, NULL, NULL };

static rcf_pch_cfg_object serial_event =
    { "event", 0, &serial_pattern, NULL,
     (rcf_ch_cfg_get)parser_event_get, (rcf_ch_cfg_set)parser_event_set,
     (rcf_ch_cfg_add)parser_event_add, (rcf_ch_cfg_del)parser_event_del,
     (rcf_ch_cfg_list)parser_event_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(serial_log_level, "level", NULL, NULL,
                    parser_common_get, parser_common_set);

RCF_PCH_CFG_NODE_RW(serial_logging, "logging", &serial_log_level, 
                    &serial_event, parser_common_get, parser_common_set);

RCF_PCH_CFG_NODE_RW(serial_reset, "reset", NULL, &serial_logging,
                    parser_common_get, parser_reset);

RCF_PCH_CFG_NODE_RW(serial_interval, "interval", NULL, &serial_reset,
                    parser_common_get, parser_common_set);

RCF_PCH_CFG_NODE_RW(serial_user, "user", NULL, &serial_interval,
                    parser_common_get, parser_common_set);

RCF_PCH_CFG_NODE_RW(serial_port, "port", NULL, &serial_user,
                    parser_common_get, parser_common_set);

RCF_PCH_CFG_NODE_RW(serial_enable, "enable", NULL, &serial_port,
                    parser_common_get, parser_set_enable);

static rcf_pch_cfg_object node_parser_inst =
    { "parser", 0, &serial_enable, NULL,
      (rcf_ch_cfg_get)parser_get, (rcf_ch_cfg_set)parser_set,
      (rcf_ch_cfg_add)parser_add, (rcf_ch_cfg_del)parser_del,
      (rcf_ch_cfg_list)parsers_list, NULL, NULL };

/* See description in conf_serial_parser.h */
te_errno
ta_unix_serial_parser_init(void)
{
    SLIST_INIT(&parsers_h);

    return rcf_pch_add_node("/agent", &node_parser_inst);
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

/* See description in conf_serial_parser.h */
int
serial_console_log(void *ready, int argc, char *argv[])
{
    serial_parser_t parser;
    int             rc;

    if (argc < 4)
    {
        ERROR("Too few parameters to serial_console_log");
        sem_post(ready);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    strncpy(parser.user, argv[0], TE_SERIAL_MAX_NAME);
    parser.level = map_name_to_level(argv[1]);
    if (parser.level == 0)
    {
        ERROR("Error level %s is unknown", argv[1]);
        sem_post(ready);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    parser.interval = strtol(argv[2], NULL, 10);
    if (parser.interval <= 0)
    {
        ERROR("Invalid interval value: %s", argv[2]);
        sem_post(ready);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    strncpy(parser.c_name, argv[3], TE_SERIAL_MAX_NAME);
    if (argc == 5)
        strncpy(parser.mode, argv[4], TE_SERIAL_MAX_NAME);
    sem_post(ready);

    parser.logging = TRUE;
    parser.port = -1;
    rc = pthread_mutex_init(&parser.mutex, NULL);
    if (rc != 0)
    {
        ERROR("Couldn't init mutex of the %s parser, error: %s",
              parser.name, strerror(rc));
        return TE_OS_RC(TE_TA_UNIX, rc);
    }

    te_serial_parser(&parser);

    return 0;
}
