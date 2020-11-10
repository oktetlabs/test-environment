/** @file
 * @brief TE project. Logger subsystem.
 *
 * This module provides data structures and logic for log message processing
 * in term of log streaming.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Viacheslav Galaktionov <Viacheslav.Galaktionov@oktetlabs.ru>
 */

#define TE_LGR_USER "Log streaming rules"

#include <jansson.h>

#include "logger_stream_rules.h"
#include "logger_api.h"

streaming_filter streaming_filters[LOG_MAX_FILTERS];
size_t           streaming_filters_num;

/*************************************************************************/
/*       Streaming handlers                                              */
/*************************************************************************/

/**
 * Convert a message into JSON.
 *
 * The resulting JSON object will contain the following fields:
 *   a) "entity": entity that sent the message
 *   b) "user":   user that sent the message
 *   c) "ts":     timestamp of the message
 *   d) "body":   expanded format string
 *
 * @param view              log message view
 * @param buf               where the result should be placed
 *
 * @returns Status code
 */
static te_errno
handler_raw(const log_msg_view *view, refcnt_buffer *buf)
{
    te_errno   rc;
    char      *dump;
    te_string  body = TE_STRING_INIT;
    json_t    *obj;

    rc = te_raw_log_expand(view, &body);
    if (rc != 0)
        return rc;

    obj = json_pack("{s:s, s:s#, s:s#, s:f, s:s}",
                    "type", "log",
                    "entity", view->entity, (int)view->entity_len,
                    "user", view->user, (int)view->user_len,
                    "ts", (double)(view->ts_sec + view->ts_usec * 0.000001),
                    "body", body.ptr);
    te_string_free(&body);
    if (obj == NULL)
        return TE_EFAULT;

    dump = json_dumps(obj, JSON_COMPACT);
    json_decref(obj);
    if (dump == NULL)
        return TE_EFAULT;

    return refcnt_buffer_init(buf, dump, strlen(dump));
}

/**
 * Extract test progress information from the message.
 *
 * The resulting JSON object will have the same structure as
 * Tester Control messages, but slightly simplified.
 *
 * @param view              log message view
 * @param buf               where the result should be placed
 *
 * @returns Status code
 */
static te_errno
handler_test_progress(const log_msg_view *view, refcnt_buffer *str)
{
    te_errno      rc;
    te_string     body = TE_STRING_INIT;
    json_t       *json;
    json_t       *msg;
    json_t       *type;
    json_error_t  err;
    char         *dump;

    rc = te_raw_log_expand(view, &body);
    if (rc != 0)
        return rc;

    json = json_loads(body.ptr, JSON_REJECT_DUPLICATES, &err);
    te_string_free(&body);
    if (json == NULL)
    {
        ERROR("Failed to unpack JSON log message: %s (line %d, column %d)",
                  err.text, err.line, err.column);
        return TE_EINVAL;
    }

    msg = json_object_get(json, "msg");
    if (msg == NULL)
    {
        ERROR("Tester:Control message does not have a \"msg\" property");
        json_decref(json);
        return TE_EINVAL;
    }
    type = json_object_get(json, "type");
    if (type == NULL)
    {
        ERROR("Tester:Control message does not have a \"type\" property");
        json_decref(json);
        return TE_EINVAL;
    }
    if (json_object_set(msg, "type", type) == -1)
    {
        ERROR("Failed to add \"type\" property to a Tester:Control message");
        json_decref(json);
        return TE_EFAULT;
    }
    dump = json_dumps(msg, JSON_COMPACT);
    json_decref(json);
    if (dump == NULL)
    {
        ERROR("Failed to dump JSON");
        return TE_EFAULT;
    }

    return refcnt_buffer_init(str, dump, strlen(dump));
}

static const streaming_rule rules[] = {
    {"raw", handler_raw},
    {"test_progress", handler_test_progress},
};
static const size_t         rules_num = sizeof(rules) / sizeof(rules[0]);

/** Get handler function by name */
static const streaming_rule *
get_handler(const char *name)
{
    size_t i;
    for (i = 0; i < rules_num; i++)
    {
        if (strcmp(rules[i].name, name) == 0)
            return &rules[i];
    }
    return NULL;
}

/*************************************************************************/
/*       Streaming filter implementation                                 */
/*************************************************************************/

/**
 * Process message according to the specified rule and push the result
 * into the listener buffers.
 *
 * @param action        streaming action
 * @param msg           log message
 *
 * @returns Status code
 */
static te_errno
action_process(const streaming_action *action, const log_msg_view *msg)
{
    size_t        i;
    te_errno      rc;
    refcnt_buffer res;

    rc = action->rule->handler(msg, &res);
    if (rc != 0)
        return rc;

    for (i = 0; i < action->listeners_num; i++)
    {
        rc = listener_add_msg(&listeners[action->listeners[i]], &res);
        if (rc != 0)
        {
            ERROR("Failed to add message to listener %s: %r",
                  listeners[i], rc);
        }
    }

    refcnt_buffer_free(&res);
    return 0;
}

/* See description in logger_stream_rules.h */
te_errno
streaming_action_add_listener(streaming_action *action, int listener_id)
{
    size_t i;

    if (action->listeners_num >= LOG_MAX_FILTER_RULES)
    {
        ERROR("Reached listener limit in a rule");
        return TE_ETOOMANY;
    }
    for (i = 0; i < action->listeners_num; i++)
    {
        if (action->listeners[i] == listener_id)
            return TE_EEXIST;
    }
    action->listeners[action->listeners_num] = listener_id;
    action->listeners_num += 1;
    return 0;
}

/* See description in logger_stream_rules.h */
te_errno
streaming_filter_process(const streaming_filter *filter,
                         const log_msg_view *msg)
{
    size_t   i;
    te_errno rc;

    if (log_msg_filter_check(&filter->filter, msg) == LOG_FILTER_PASS)
    {
        for (i = 0; i < filter->actions_num; i++)
        {
            rc = action_process(&filter->actions[i], msg);
            if (rc != 0)
            {
                ERROR("Failed to process message in rule %s: %r",
                      filter->actions[i].rule->name, rc);
            }
        }
    }

    return 0;
}

/* See description in logger_stream_rules.h */
te_errno
streaming_filter_add_action(streaming_filter *filter,
                            const char *rule_name,
                            int listener_id)
{
    size_t                i;
    te_errno              rc;
    const streaming_rule *rule;

    if (rule_name == NULL)
        rule_name = "raw";

    rule = get_handler(rule_name);
    if (rule == NULL)
    {
        ERROR("Failed to get handler \"%s\"", rule_name);
        return TE_EINVAL;
    }

    for (i = 0; i < filter->actions_num; i++)
        if (filter->actions[i].rule == rule)
            break;

    if (i == filter->actions_num)
    {
        if (filter->actions_num >= LOG_MAX_FILTER_RULES)
        {
            ERROR("Reached the rule limit");
            return TE_ETOOMANY;
        }
        filter->actions[filter->actions_num].rule = rule;
        filter->actions_num += 1;
    }

    rc = streaming_action_add_listener(&filter->actions[i], listener_id);
    if (rc == TE_EEXIST)
    {
        WARN("Attempted to add listener to the same rule");
        return 0;
    }
    return rc;
}
