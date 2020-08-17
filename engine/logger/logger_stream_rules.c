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

#include "logger_stream_rules.h"
#include "logger_api.h"

streaming_filter streaming_filters[LOG_MAX_FILTERS];
size_t           streaming_filters_num;

static const streaming_rule rules[] = {};
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
