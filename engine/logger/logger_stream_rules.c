/** @file
 * @brief TE project. Logger subsystem.
 *
 * This module provides data structures and logic for log message processing
 * in term of log streaming.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 *
 */

#define TE_LGR_USER "Log streaming rules"

#include <float.h>
#include <jansson.h>

#include "logger_stream_rules.h"
#include "logger_api.h"
#include "te_alloc.h"
#include "te_raw_log.h"
#include "te_queue.h"

streaming_filter streaming_filters[LOG_MAX_FILTERS];
size_t           streaming_filters_num;

/** The number of least recently executed tests whose times should be stored */
#define TEST_TIME_HISTORY_SIZE 20

/** Data structure that represents test execution times */
typedef struct test_run_time {
    TAILQ_ENTRY(test_run_time) links;

    int    test_id;
    double ts_start;
    double ts_end;
} test_run_time;

typedef TAILQ_HEAD(test_run_times, test_run_time) test_run_times;

static test_run_times test_times = TAILQ_HEAD_INITIALIZER(test_times);
static size_t         test_times_num = 0;

/** Register test start time */
static te_errno
test_times_add_start(int test_id, double ts)
{
    test_run_time *times = NULL;

    if (test_times_num >= TEST_TIME_HISTORY_SIZE)
    {
        times = TAILQ_LAST(&test_times, test_run_times);
        TAILQ_REMOVE(&test_times, times, links);
        TAILQ_INSERT_HEAD(&test_times, times, links);
    }
    else
    {
        times = TE_ALLOC(sizeof(*times));
        if (times == NULL)
            return TE_ENOMEM;
        TAILQ_INSERT_HEAD(&test_times, times, links);
    }

    times->test_id = test_id;
    times->ts_start = ts;
    times->ts_end = DBL_MAX;

    return 0;
}

/** Register test end time */
static te_errno
test_times_add_end(int test_id, double ts)
{
    test_run_time *first = NULL;

    first = TAILQ_FIRST(&test_times);

    if (first == NULL || first->test_id != test_id)
        return TE_ENOENT;

    first->ts_end = ts;

    return 0;
}

/** Find test that was running at the given time */
static test_run_time *
test_times_get_test(double ts)
{
    test_run_time *item;

    TAILQ_FOREACH(item, &test_times, links)
    {
        if (item->ts_start <= ts && ts <= item->ts_end)
            return item;
    }

    return NULL;
}

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
    int           ret;
    te_string     body = TE_STRING_INIT;
    int           test_id;
    const char   *node_type;
    te_bool       start;
    json_t       *json;
    json_t       *msg;
    json_t       *type;
    json_t       *ts;
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

    ts = json_real(view->ts_sec + view->ts_usec * 0.000001);
    if (ts == NULL)
    {
        ERROR("Failed to create JSON representation for Tester:Control "
              "message timestamp");
        json_decref(json);
        return TE_ENOMEM;
    }
    if (json_object_set_new(msg, "ts", ts) == -1)
    {
        ERROR("Failed to add \"ts\" property to a Tester:Control message");
        json_decref(ts);
        json_decref(json);
        return TE_EFAULT;
    }

    ret = json_unpack_ex(msg, &err, 0, "{s:i, s?s}",
                         "id", &test_id,
                         "node_type", &node_type);
    if (ret == -1)
    {
        ERROR("Failed to extract test ID and node type from JSON log message: "
              "%s (line %d, column %d)",
              err.text, err.line, err.column);
        json_decref(json);
        return TE_EINVAL;
    }


    start = strcmp(json_string_value(type), "test_start") == 0;
    if (start)
    {
        if (strcmp(node_type, "test") == 0)
        {
            rc = test_times_add_start(test_id, json_real_value(ts));
            if (rc != 0)
            {
                ERROR("Failed to record test start time: %r", rc);
                json_decref(json);
                return rc;
            }
        }
    }
    else
    {
        rc = test_times_add_end(test_id, json_real_value(ts));
        if (rc != 0 && rc != TE_ENOENT)
        {
            ERROR("Failed to record test end time: %r", rc);
            json_decref(json);
            return rc;
        }
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

/**
 * Convert the given artifact message to JSON.
 *
 * Detect the test ID of the artifact relying on the test history prepared
 * by handler_test_progress. This means that test IDs will be present in
 * event messages only if the 'test_progress' rule is used anywhere in the
 * configuration file.
 *
 * @param view              log message view
 * @param buf               where the result should be placed
 *
 * @returns Status code
 */
static te_errno
handler_artifact(const log_msg_view *view, refcnt_buffer *buf)
{
    te_errno       rc;
    char          *dump;
    double         ts;
    te_string      body = TE_STRING_INIT;
    uint32_t       test_id;
    test_run_time *time;
    json_t        *obj;

    rc = te_raw_log_expand(view, &body);
    if (rc != 0)
        return rc;

    ts = (double)(view->ts_sec + view->ts_usec * 0.000001);
    test_id = view->log_id;
    if (test_id == TE_LOG_ID_UNDEFINED)
    {
        RING("Artifact log ID was undefined, checking run history");
        time = test_times_get_test(ts);
        if (time == NULL)
        {
            ERROR("Failed to find test id for an artifact from %.*s",
                  view->user_len, view->user);
            test_id = -1;
        }
        else
        {
            test_id = time->test_id;
        }
    }

    /*
     * "*" instead of "?" would be better here, but it's not supported in
     * jansson-2.10, which is currently the newest version available on
     * CentOS/RHEL-7.x
     */
    obj = json_pack("{s:s, s:s#, s:i, s:f, s:s}",
                    "type", "artifact",
                    "entity", view->entity, (int)view->entity_len,
                    "test_id", test_id,
                    "ts", ts,
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

static const streaming_rule rules[] = {
    {"raw", handler_raw},
    {"test_progress", handler_test_progress},
    {"artifact", handler_artifact},
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
