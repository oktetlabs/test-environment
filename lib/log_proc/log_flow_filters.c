/** @file
 * @brief Log processing
 *
 * This module provides some filters that do not work with log messages
 * directly, but instead deal with flow nodes.
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 *
 * @author Viacheslav Galaktionov <Viacheslav.Galaktionov@oktetlabs.ru>
 * @author Oleg N. Kravtsov  <Oleg.Kravtsov@oktetlabs.ru>
 */

#define TE_LGR_USER "Log processing"

#include "te_config.h"
#include "logger_api.h"
#include "log_flow_filters.h"
#include "te_alloc.h"

/* Possible flow node types */
typedef enum node_type {
    NT_ALL,
    NT_UNKNOWN,
    NT_PACKAGE,
    NT_SESSION,
    NT_TEST
} node_type_t;

/**
 * Parse node type.
 *
 * @param name      node type name
 *
 * @returns Node type
 */
static node_type_t
get_node_type(const char *name)
{
    if (name == NULL)
        return NT_ALL;

    if (strcmp(name, "pkg") == 0)
        return NT_PACKAGE;
    if (strcmp(name, "PACKAGE") == 0)
        return NT_PACKAGE;

    if (strcmp(name, "session") == 0)
        return NT_SESSION;
    if (strcmp(name, "SESSION") == 0)
        return NT_SESSION;

    if (strcmp(name, "test") == 0)
        return NT_TEST;
    if (strcmp(name, "TEST") == 0)
        return NT_TEST;

    return NT_UNKNOWN;
}

/* See description in flow_filter.h */
void
log_branch_filter_init(log_branch_filter *filter)
{
    SLIST_INIT(&filter->list);
}

/* See description in flow_filter.h */
te_errno
log_branch_filter_add(log_branch_filter *filter,
                      const char *path, te_bool include)
{
    log_branch_filter_rule  *rule;
    log_branch_filter_rule **prev;

    if (path == NULL)
        return TE_EINVAL;

    SLIST_FOREACH_PREVPTR(rule, prev, &filter->list, links)
    {
        if (strcmp(rule->path, path) == 0)
        {
            if (include == (rule->result == LOG_FILTER_FAIL))
                return 0;
            else
                return TE_EINVAL;
        }
    }

    rule = TE_ALLOC(sizeof(*rule));
    if (rule == NULL)
        return TE_ENOMEM;

    rule->path = strdup(path);
    if (rule->path == NULL)
    {
        free(rule);
        return TE_ENOMEM;
    }

    rule->result = include ? LOG_FILTER_PASS : LOG_FILTER_FAIL;

    SLIST_INSERT_AFTER(rule, *prev, links);

    return 0;
}

/* See description in flow_filter.h */
log_filter_result
log_branch_filter_check(log_branch_filter *filter, const char *path)
{
    log_branch_filter_rule  *rule;

    SLIST_FOREACH(rule, &filter->list, links)
    {
        if (strcmp(rule->path, path) == 0)
            return rule->result;
    }

    return LOG_FILTER_DEFAULT;
}

/* See description in flow_filter.h */
void
log_branch_filter_free(log_branch_filter *filter)
{
    log_branch_filter_rule *rule;
    log_branch_filter_rule *tmp;

    SLIST_FOREACH_SAFE(rule, &filter->list, links, tmp)
    {
        free(rule->path);
        free(rule);
    }
}

/**
 * Initialize a rule list for duration filter.
 *
 * @param rules         list of rules
 *
 * @returns Status code
 */
static te_errno
log_duration_filter_rules_init(log_duration_filter_rules *rules)
{
    log_duration_filter_rule *rule;

    SLIST_INIT(&rules->list);

    rule = TE_ALLOC(sizeof(*rule));
    if (rule == NULL)
        return TE_ENOMEM;

    rule->min = 0;
    rule->max = UINT32_MAX;
    rule->result = LOG_FILTER_PASS;

    SLIST_INSERT_HEAD(&rules->list, rule, links);

    return 0;
}

/**
 * Add a rule to the duration rules list.
 *
 * @param rules         list of rules
 * @param min           start of interval
 * @param max           end of interval
 * @param include       include or exclude
 */
static te_errno
log_duration_filter_rules_add(log_duration_filter_rules *rules,
                              uint32_t min, uint32_t max,
                              te_bool include)
{
    log_duration_filter_rule  *rule;
    log_duration_filter_rule  *tmp1, *tmp2;
    log_duration_filter_rule **prev;

    SLIST_FOREACH_PREVPTR(rule, prev, &rules->list, links)
    {
        if (rule->min > min)
            break;

        if (rule->min <= min && rule->max > min)
        {
            if (rule->min == min && rule->max <= max)
            {
                rule->result = include ? LOG_FILTER_PASS : LOG_FILTER_FAIL;
                min = rule->max;
                continue;
            }

            /* We've found a starting place for our new record */
            if (rule->max >= max)
            {
                /*
                 * we're inside a single interval
                 * |-------------|
                 *  |          |
                 * min ------ max
                 */

                tmp1 = TE_ALLOC(sizeof(*tmp1));
                if (tmp1 == NULL)
                    return TE_ENOMEM;
                tmp1->min = min;
                tmp1->max = max;
                tmp1->result = include ? LOG_FILTER_PASS : LOG_FILTER_FAIL;

                if (rule->min == min)
                {
                    rule->min = max + 1;
                    /* Insert before rule */
                    if (*prev == rule)
                        SLIST_INSERT_HEAD(&rules->list, tmp1, links);
                    else
                        SLIST_INSERT_AFTER(*prev, tmp1, links);
                }
                else
                {
                    SLIST_INSERT_AFTER(rule, tmp1, links);
                    if (rule->max > max)
                    {
                        tmp2 = TE_ALLOC(sizeof(*tmp2));
                        if (tmp2 == NULL)
                            return TE_ENOMEM;
                        tmp2->min = max + 1;
                        tmp2->max = rule->max;
                        tmp2->result = rule->result;
                        SLIST_INSERT_AFTER(tmp1, tmp2, links);
                    }
                    rule->max = min - 1;
                }
                break;
            }
            else
            {
                /*
                 * Upper bound is out of single interval
                 * |-----|--------|
                 *   |
                 *  min ------->
                 */
                if (rule->min < min)
                {
                    /*
                     * We should split this rule into two
                     * |-----|--------|
                     *    |
                     *   min in the middle.
                     */
                    tmp1 = TE_ALLOC(sizeof(*tmp1));
                    if (tmp1 == NULL)
                        return TE_ENOMEM;
                    tmp1->min = min;
                    tmp1->max = rule->max;
                    tmp1->result = include ? LOG_FILTER_PASS : LOG_FILTER_FAIL;

                    SLIST_INSERT_AFTER(rule, tmp1, links);

                    rule->max = min - 1;
                    min = tmp1->max;
                }
                else
                {
                    assert(0);
                }
            }

            break;
        }
    }

    return 0;
}

/**
 * Check duration against a list of rules.
 *
 * @param rules         list of rules
 * @param duration      duration
 *
 * @returns Filtering result
 */
static log_filter_result
log_duration_filter_rules_check(log_duration_filter_rules *rules,
                                uint32_t duration)
{
    log_duration_filter_rule *rule;

    SLIST_FOREACH(rule, &rules->list, links)
    {
        if (rule->min <= duration && duration <= rule->max)
            return rule->result;
    }

    return LOG_FILTER_PASS;
}

/**
 * Free the memory allocated by the list of rules.
 *
 * @param rules         list of rules
 */
static void
log_duration_filter_rules_free(log_duration_filter_rules *rules)
{
    log_duration_filter_rule *rule;
    log_duration_filter_rule *tmp;

    SLIST_FOREACH_SAFE(rule, &rules->list, links, tmp)
        free(rule);
}

/* See description in flow_filter.h */
te_errno
log_duration_filter_init(log_duration_filter *filter)
{
    te_errno rc;

    memset(filter, 0, sizeof(*filter));

    rc = log_duration_filter_rules_init(&filter->package);

    if (rc == 0)
        rc = log_duration_filter_rules_init(&filter->session);

    if (rc == 0)
        rc = log_duration_filter_rules_init(&filter->test);

    return rc;
}

/* See description in flow_filter.h */
te_errno
log_duration_filter_add(log_duration_filter *filter, const char *type,
                        uint32_t min, uint32_t max, te_bool include)
{
    te_errno     rc;
    node_type_t  node_type;

    rc = 0;
    node_type = get_node_type(type);

    if (rc == 0 && (node_type == NT_ALL || node_type == NT_PACKAGE))
        rc = log_duration_filter_rules_add(&filter->package, min, max, include);

    if (rc == 0 && (node_type == NT_ALL || node_type == NT_SESSION))
        rc = log_duration_filter_rules_add(&filter->session, min, max, include);

    if (rc == 0 && (node_type == NT_ALL || node_type == NT_TEST))
        rc = log_duration_filter_rules_add(&filter->test, min, max, include);

    return rc;
}

/* See description in flow_filter.h */
log_filter_result
log_duration_filter_check(log_duration_filter *filter, const char *type,
                          uint32_t duration)
{
    node_type_t node_type;

    node_type = get_node_type(type);
    switch (node_type)
    {
        case NT_PACKAGE:
            return log_duration_filter_rules_check(&filter->package, duration);
        case NT_SESSION:
            return log_duration_filter_rules_check(&filter->session, duration);
        case NT_TEST:
            return log_duration_filter_rules_check(&filter->test, duration);
        default:
            ERROR("Invalid node type in %s: %s", __FUNCTION__, type);
            return LOG_FILTER_DEFAULT;
    }
}

/* See description in flow_filter.h */
void
log_duration_filter_free(log_duration_filter *filter)
{
    log_duration_filter_rules_free(&filter->package);
    log_duration_filter_rules_free(&filter->session);
    log_duration_filter_rules_free(&filter->test);
}
