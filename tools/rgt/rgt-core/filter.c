/** @file
 * @brief Test Environment: Interface for filtering of log messages.
 *
 * The module is responsible for making descisions about message filtering.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#include <libxml/parser.h>

#include "rgt_common.h"
#include "logger_defs.h"
#include "log_filters_xml.h"
#include "filter.h"

/**
 * Get control message flags.
 *
 * @param user        Log user.
 * @param level       Log level.
 * @param flags       Where to set control message flags.
 */
static void
get_control_msg_flags(const char *user, te_log_level level,
                      uint32_t *flags)
{
    if (rgt_ctx.proc_cntrl_msg)
    {
        if (level & TE_LL_CONTROL)
        {
            if (strcmp(user, TE_LOG_VERDICT_USER) == 0)
                *flags |= RGT_MSG_FLG_VERDICT;
            if (strcmp(user, TE_LOG_ARTIFACT_USER) == 0)
                *flags |= RGT_MSG_FLG_ARTIFACT;
        }
        else if (strcmp(user, TE_LOG_CMSG_USER) == 0)
        {
            /*
             * This is kept for backward compatibility with
             * previously generated night testing raw logs.
             */
            *flags |= RGT_MSG_FLG_VERDICT;
        }
    }
}

static log_branch_filter branch_filter;
static log_duration_filter duration_filter;
static log_msg_filter msg_filter;

static te_bool     initialized = FALSE;

/* See the description in filter.h */
int
rgt_filter_init(const char *fltr_fname)
{
    xmlDocPtr     doc;
    xmlNodePtr    cur;

    if (initialized)
    {
        TRACE("rgt_filter library has already been initialized");
        return -1;
    }

    log_branch_filter_init(&branch_filter);
    log_duration_filter_init(&duration_filter);
    log_msg_filter_init(&msg_filter);

    if (fltr_fname == NULL)
    {
        initialized = TRUE;
        return 0;
    }

    doc = xmlParseFile(fltr_fname);
    if (doc == NULL)
    {
        return TE_EINVAL;
    }

    cur = xmlDocGetRootElement(doc);
    if (cur == NULL)
        /* Empty file, nothing to do */
        return 0;

    if (xmlStrcmp(cur->name, (const xmlChar *)"filters") != 0)
    {
        return TE_EINVAL;
    }

    cur = cur->xmlChildrenNode;
    while (cur != NULL)
    {
        if (xmlStrcmp(cur->name, (const xmlChar *)"entity-filter") == 0)
        {
            log_branch_filter_load_xml(&branch_filter, cur);
        }
        if (xmlStrcmp(cur->name, (const xmlChar *)"duration-filter") == 0)
        {
            log_duration_filter_load_xml(&duration_filter, cur);
        }
        if (xmlStrcmp(cur->name, (const xmlChar *)"entity-filter") == 0)
        {
            log_msg_filter_load_xml(&msg_filter, cur);
        }
        cur = cur->next;
    }

    initialized = TRUE;
    return 0;
}

/**
 * Destroys filter module.
 *
 * @return  Nothing.
 */
void
rgt_filter_destroy()
{
    if (!initialized)
    {
        TRACE("rgt_filter library has not been initialized");
        return;
    }

    log_branch_filter_free(&branch_filter);
    log_duration_filter_free(&duration_filter);
    log_msg_filter_free(&msg_filter);
}

/**
 * Validates if log message with a particular tuple (level, entity name,
 * user name and timestamp) passes through user defined filter.
 * The function updates message flags.
 *
 * @param entity        Entity name
 * @param user          User name
 * @param level         Log level
 * @param timestamp     Timestamp
 * @param flags         Log message flags (OUT)
 *
 * @return Returns filtering mode for the tuple.
 *         It never returns NFMODE_DEFAULT value.
 *
 * @retval NFMODE_INCLUDE   the tuple is passed through the filter.
 * @retval NFMODE_EXCLUDE   the tuple is rejected by the filter.
 */
enum node_fltr_mode
rgt_filter_check_message(const char *entity, const char *user,
                         te_log_level level,
                         const uint32_t *timestamp, uint32_t *flags)
{
    log_msg_view message;

    memset(&message, 0, sizeof(message));

    message.entity = entity;
    message.entity_len = strlen(entity);

    message.user = user;
    message.user_len = strlen(user);

    message.level = level;

    message.ts_sec = timestamp[0];
    message.ts_usec = timestamp[1];

    log_msg_filter_check(&msg_filter, &message);

    get_control_msg_flags(user, level, flags);

    if (log_msg_filter_check(&msg_filter, &message) == LOG_FILTER_PASS)
        *flags |= RGT_MSG_FLG_NORMAL;

    /*
     * Include ordinary log messages if they pass through the filter
     * and any control messages even if they do not pass through the filter.
     */
    return (*flags & (RGT_MSG_FLG_VERDICT | RGT_MSG_FLG_ARTIFACT |
                      RGT_MSG_FLG_NORMAL)) ?
                NFMODE_INCLUDE : NFMODE_EXCLUDE;
}

/**
 * Verifies if the whole branch of execution flow should be excluded or
 * included from the log report.
 *
 * @param   path  Path (name) of the branch to be checked.
 *                Path is formed from names of packages and/or test
 *                of the execution flow separated by '/'. For example
 *                path "/a/b/c/d" means that execution flow is
 *                pkg "a" -> pkg "b" -> pkg "c" -> [test | pkg] "d"
 *
 * @return  Returns filtering mode for the branch.
 */
enum node_fltr_mode
rgt_filter_check_branch(const char *path)
{
    log_filter_result res;

    res = log_branch_filter_check(&branch_filter, path);

    if (res == LOG_FILTER_PASS)
        return NFMODE_INCLUDE;
    else if (res == LOG_FILTER_FAIL)
        return NFMODE_EXCLUDE;
    else
        return NFMODE_DEFAULT;
}

/**
 * Validates if the particular node (TEST, SESSION or PACKAGE) passes
 * through duration filter.
 *
 * @param node_type  Typo of the node ("TEST", "SESSION" or "PACKAGE")
 * @param start_ts   Start timestamp
 * @param end_ts     End timestamp
 *
 * @return Returns filtering mode for the node.
 *
 * @retval NFMODE_INCLUDE   the node is passed through the filter.
 * @retval NFMODE_EXCLUDE   the node is rejected by the filter.
 */
enum node_fltr_mode
rgt_filter_check_duration(const char *node_type,
                          uint32_t *start_ts, uint32_t *end_ts)
{
    log_filter_result res;
    uint32_t          duration[2];

    TIMESTAMP_SUB(duration, end_ts, start_ts);

    res = log_duration_filter_check(&duration_filter, node_type, duration[0]);

    if (res == LOG_FILTER_FAIL)
        return NFMODE_EXCLUDE;
    else
        return NFMODE_INCLUDE;
}
