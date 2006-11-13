/** @file
 * @brief Testing Results Comparator
 *
 * Parser of TE log in XML format.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Log Parser"

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include <libxml/tree.h>

#include "te_errno.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "te_test_result.h"
#include "te_trc.h"
#include "trc_tags.h"
#include "trc_report.h"


#define CONST_CHAR2XML  (const xmlChar *)
#define XML2CHAR(p)     ((char *)p)


/** State of the TE log parser from TRC point of view */
typedef enum trc_report_log_parse_state {
    TRC_REPORT_LOG_PARSE_INIT,      /**< Initial state */
    TRC_REPORT_LOG_PARSE_ROOT,      /**< Root state */
    TRC_REPORT_LOG_PARSE_TAGS,      /**< Inside log message with TRC
                                         tags list */
    TRC_REPORT_LOG_PARSE_TEST,      /**< Inside 'test', 'pkg' or
                                         'session' element */
    TRC_REPORT_LOG_PARSE_META,      /**< Inside 'meta' element */
    TRC_REPORT_LOG_PARSE_OBJECTIVE, /**< Inside 'objective' element */
    TRC_REPORT_LOG_PARSE_VERDICTS,  /**< Inside 'verdicts' element */
    TRC_REPORT_LOG_PARSE_VERDICT,   /**< Inside 'verdict' element */
    TRC_REPORT_LOG_PARSE_PARAMS,    /**< Inside 'params' element */
    TRC_REPORT_LOG_PARSE_LOGS,      /**< Inside 'logs' element */
    TRC_REPORT_LOG_PARSE_SKIP,      /**< Skip entire contents */
} trc_report_log_parse_state;

/** TRC report TE log parser context. */
typedef struct trc_report_log_parse_ctx {

    te_errno            rc;         /**< Status of processing */

    unsigned int        flags;      /**< Processing flags */
    te_trc_db          *db;         /**< TRC database handle */
    unsigned int        db_uid;     /**< TRC database user ID */
    const char         *log;        /**< Name of the file with log */
    tqh_strings        *tags;       /**< List of tags */

    te_trc_db_walker   *db_walker;  /**< TRC database walker */

    trc_report_log_parse_state  state;  /**< Log parse state */
    trc_test_type               type;   /**< Type of the test */

    char   *str;    /**< Accumulated string (used when single string is
                         reported by few trc_report_log_characters()
                         calls because of entities in it) */

    unsigned int                skip_depth; /**< Skip depth */
    trc_report_log_parse_state  skip_state; /**< State to return */

    trc_report_test_iter_data  *iter_data;  /**< Current test iteration
                                                 data */

    unsigned int    args_max;   /**< Maximum number of arguments
                                     the space is allocated for */
    unsigned int    args_n;     /**< Current number of arguments */
    char          **args_name;  /**< Names of arguments */
    char          **args_value; /**< Values of arguments */

    unsigned int    stack_size; /**< Size of the stack in elements */
    te_bool        *stack_info; /**< Stack */
    unsigned int    stack_pos;  /**< Current position in the stack */

} trc_report_log_parse_ctx;


/**
 * Push value into the stack.
 * 
 * @param ctx           TRC report log parser context
 * @param value         Value to put on the stack
 *
 * @return Status code.
 */
static te_errno
trc_report_log_parse_stack_push(trc_report_log_parse_ctx *ctx,
                                te_bool value)
{
    if (ctx->stack_pos == ctx->stack_size)
    {
        ctx->stack_info = realloc(ctx->stack_info, ++(ctx->stack_size));
        if (ctx->stack_info == NULL)
        {
            ERROR("%s(): realloc() failed", __FUNCTION__);
            return TE_ENOMEM;
        }
    }
    ctx->stack_info[ctx->stack_pos++] = value;
    return 0;
}

/**
 * Pop value from the stack.
 * 
 * @param ctx           TRC report log parser context
 *
 * @return Value from the stack.
 */
static te_bool
trc_report_log_parse_stack_pop(trc_report_log_parse_ctx *ctx)
{
    assert(ctx->stack_pos > 0);
    return ctx->stack_info[--(ctx->stack_pos)];
}

/**
 * Callback function that is called before parsing the document.
 *
 * @param user_data     Pointer to user-specific data (user context)
 */
static void
trc_report_log_start_document(void *user_data)
{
    trc_report_log_parse_ctx   *ctx = user_data;

    assert(ctx != NULL);
    if (ctx->rc != 0)
        return;

    assert(ctx->db_walker == NULL);
    ctx->db_walker = trc_db_new_walker(ctx->db);
    if (ctx->db_walker == NULL)
    {
        ctx->rc = TE_ENOMEM;
        return;
    }

    ctx->state = TRC_REPORT_LOG_PARSE_INIT;
}

/**
 * Callback function that is called when XML parser reaches the end 
 * of the document.
 *
 * @param user_data     Pointer to user-specific data (user context)
 */
static void
trc_report_log_end_document(void *user_data)
{
    trc_report_log_parse_ctx   *ctx = user_data;

    trc_db_free_walker(ctx->db_walker);
    ctx->db_walker = NULL;
}


/**
 * Convert string representation of test status to enumeration member.
 *
 * @param str           String representation of test status
 * @param status        Location for converted test status
 *
 * @return Status code.
 */
static te_errno
te_test_str2status(const char *str, te_test_status *status)
{
    if (strcmp(str, "PASSED") == 0)
        *status = TE_TEST_PASSED;
    else if (strcmp(str, "FAILED") == 0)
        *status = TE_TEST_FAILED;
    else if (strcmp(str, "SKIPPED") == 0)
        *status = TE_TEST_SKIPPED;
    else if (strcmp(str, "FAKED") == 0)
        *status = TE_TEST_FAKED;
    else if (strcmp(str, "EMPTY") == 0)
        *status = TE_TEST_EMPTY;
    else if (strcmp(str, "INCOMPLETE") == 0)
        *status = TE_TEST_INCOMPLETE;
    else if (strcmp(str, "KILLED") == 0)
        *status = TE_TEST_FAILED;
    else if (strcmp(str, "CORED") == 0)
        *status = TE_TEST_FAILED;
    else
    {
        ERROR("Invalid value '%s' of the test status", XML2CHAR(str));
        return TE_EFMT;
    }
    return 0;
}

/**
 * Process test script, package or session entry point.
 *
 * @param ctx           Parser context
 * @param attrs         An array of attribute name, attribute value pairs
 */
static void
trc_report_test_entry(trc_report_log_parse_ctx *ctx, const xmlChar **attrs)
{
    te_bool         name_found = FALSE;
    te_bool         status_found = FALSE;
    te_test_status  status;

    while (ctx->rc == 0 && attrs[0] != NULL && attrs[1] != NULL)
    {
        if (xmlStrcmp(attrs[0], CONST_CHAR2XML("name")) == 0)
        {
            name_found = TRUE;
            if (!trc_db_walker_step_test(ctx->db_walker,
                                         XML2CHAR(attrs[1]), TRUE))
            {
                ERROR("Unable to create a new test entry");
                ctx->rc = TE_ENOMEM;
            }
            else
            {
                trc_test *test = trc_db_walker_get_test(ctx->db_walker);

                if (test->type == TRC_TEST_UNKNOWN)
                {
                    test->type = ctx->type;
                    INFO("A new test: %s", attrs[1]);
                }
                else if (test->type != ctx->type)
                {
                    ERROR("Inconsistency in test type from the log and "
                          "TRC database");
                    ctx->rc = TE_EINVAL;
                }
                else
                {
                    INFO("Found test: %s", attrs[1]);
                }

                /* It is harless to set it in the case of failure */
                ctx->rc = trc_report_log_parse_stack_push(ctx, TRUE);
            }
        }
        else if (xmlStrcmp(attrs[0], CONST_CHAR2XML("result")) == 0)
        {
            status_found = TRUE;
            ctx->rc = te_test_str2status(XML2CHAR(attrs[1]), &status);
        }
        attrs += 2;
    }

    if (ctx->rc != 0)
    {
        /* We already have error */
    }
    else if (!name_found)
    {
        INFO("Name of the test/package/session not found - ignore");
        assert(ctx->iter_data == NULL);
        ctx->rc = trc_report_log_parse_stack_push(ctx, FALSE);
    }
    else if (!status_found)
    {
        ERROR("Status of the test/package/session not found");
        ctx->rc = TE_EFMT;
    }
    else
    {
        trc_report_test_iter_entry *entry;

        /* Pre-allocate entry for result */
        entry = TE_ALLOC(sizeof(*entry));
        if (entry == NULL)
        {
            ctx->rc = TE_ENOMEM;
            return;
        }
        te_test_result_init(&entry->result);
        entry->result.status = status;

        assert(ctx->iter_data == NULL);
        ctx->iter_data = TE_ALLOC(sizeof(*ctx->iter_data));
        if (ctx->iter_data == NULL)
        {
            free(entry);
            ctx->rc = TE_ENOMEM;
            return;
        }
        TAILQ_INIT(&ctx->iter_data->runs);
        TAILQ_INSERT_TAIL(&ctx->iter_data->runs, entry, links);
    }
}

/**
 * Process test parameter from the log.
 *
 * @param ctx           Parser context
 * @param attrs         An array of attribute name, attribute value pairs
 */
static void
trc_report_test_param(trc_report_log_parse_ctx *ctx, const xmlChar **attrs)
{
    assert(ctx->args_n <= ctx->args_max);
    if (ctx->args_n == ctx->args_max)
    {
        ctx->args_max++;
        ctx->args_name = realloc(ctx->args_name,
                             sizeof(*(ctx->args_name)) * ctx->args_max);
        ctx->args_value = realloc(ctx->args_value,
                             sizeof(*(ctx->args_value)) * ctx->args_max);
        if (ctx->args_name == NULL || ctx->args_value == NULL)
        {
            ctx->rc = TE_ENOMEM;
            return;
        }
        ctx->args_name[ctx->args_n] = ctx->args_value[ctx->args_n] = NULL;
    }

    while (attrs[0] != NULL && attrs[1] != NULL)
    {
        if (xmlStrcmp(attrs[0], CONST_CHAR2XML("name")) == 0)
        {
            free(ctx->args_name[ctx->args_n]);
            ctx->args_name[ctx->args_n] = strdup(XML2CHAR(attrs[1]));
            if (ctx->args_name[ctx->args_n] == NULL)
            {
                ERROR("strdup(%s) failed", attrs[1]);
                ctx->rc = TE_ENOMEM;
                return;
            }
        }
        else if (xmlStrcmp(attrs[0], CONST_CHAR2XML("value")) == 0)
        {
            free(ctx->args_value[ctx->args_n]);
            ctx->args_value[ctx->args_n] = strdup(XML2CHAR(attrs[1]));
            if (ctx->args_value[ctx->args_n] == NULL)
            {
                ERROR("strdup(%s) failed", XML2CHAR(attrs[1]));
                ctx->rc = TE_ENOMEM;
                return;
            }
        }
        attrs += 2;
    }

    if (ctx->args_name[ctx->args_n] == NULL ||
        ctx->args_value[ctx->args_n] == NULL)
    {
        ERROR("Invalid format of the test parameter specification");
        ctx->rc = TE_EFMT;
    }
    else
    {
        ctx->args_n++;
    }
}

/**
 * Updated iteration statistics by expected and obtained results.
 *
 * @param stats         Statistics to update
 * @param exp_result    Expected result
 * @param result        Obtained result
 *
 * @return Is obtained result expected?
 */
static te_bool
trc_report_test_iter_stats_update(trc_report_stats     *stats,
                                  const trc_exp_result *exp_result,
                                  const te_test_result *result)
{
    te_bool is_expected;

    if (result->status == TE_TEST_UNSPEC)
    {
        ERROR("Unexpected value of obtained result");
        return FALSE;
    }

    if (result->status == TE_TEST_FAKED ||
        result->status == TE_TEST_EMPTY)
    {
        return TRUE;
    }

    if (exp_result == NULL)
    {
        /* Expected result is unknown */
        is_expected = FALSE;

        switch (result->status)
        {
            case TE_TEST_SKIPPED:
                stats->new_not_run++;
                break;

            case TE_TEST_PASSED:
            case TE_TEST_FAILED:
                stats->new_run++;
                break;

            default:
                stats->aborted++;
                break;
        }
    }
    else
    {
        is_expected = (trc_is_result_expected(exp_result, result) != NULL);

        switch (result->status)
        {
            case TE_TEST_PASSED:
                if (is_expected)
                    stats->pass_exp++;
                else
                    stats->pass_une++;
                break;

            case TE_TEST_FAILED:
                if (is_expected)
                    stats->fail_exp++;
                else
                    stats->fail_une++;
                break;

            case TE_TEST_SKIPPED:
                if (is_expected)
                    stats->skip_exp++;
                else
                    stats->skip_une++;
                break;

            default:
                stats->aborted++;
                break;
        }
    }
    return is_expected;
}

/**
 * Merge more information about test iteration executions in
 * already known information.
 *
 * @param result        Already known information
 * @param more          A new information (owned by the routine)
 *
 * @note Statistics are not merged. It is assumed that it is either
 *       unnecessary or has already been done by the caller.
 */
static void
trc_report_merge_test_iter_data(trc_report_test_iter_data *result,
                                trc_report_test_iter_data *more)
{
    trc_report_test_iter_entry *entry;

    assert(result != NULL);
    assert(more != NULL);

    entry = TAILQ_FIRST(&more->runs);
    assert(entry != NULL);
    assert(TAILQ_NEXT(entry, links) == NULL);

    TAILQ_REMOVE(&more->runs, entry, links);
    TAILQ_INSERT_TAIL(&result->runs, entry, links);

    trc_report_free_test_iter_data(more);
}

/**
 * Callback function that is called when XML parser meets an opening tag.
 *
 * @param user_data     Pointer to user-specific data (user context)
 * @param name          The element name
 * @param attrs         An array of attribute name, attribute value pairs
 */
static void
trc_report_log_start_element(void *user_data,
                             const xmlChar *name, const xmlChar **attrs)
{
    trc_report_log_parse_ctx   *ctx = user_data;
    const char                 *tag = XML2CHAR(name);

    assert(ctx != NULL);
    ENTRY("state=%u rc=%r tag=%s", ctx->state, ctx->rc, tag);

    if (ctx->rc != 0)
        return;

    switch (ctx->state)
    {
        case TRC_REPORT_LOG_PARSE_SKIP:
            ctx->skip_depth++;
            break;

        case TRC_REPORT_LOG_PARSE_INIT:
            if (strcmp(tag, "proteos:log_report") != 0)
            {
                ERROR("Unexpected element '%s' at start of TE log XML",
                      tag);
                ctx->rc = TE_EFMT;
            }
            else
            {
                ctx->state = TRC_REPORT_LOG_PARSE_ROOT;
            }
            break;

        case TRC_REPORT_LOG_PARSE_ROOT:
            if (strcmp(tag, "logs") == 0)
            {
                /* TODO: Avoid search of tags inside tests */
                if (ctx->flags & TRC_REPORT_IGNORE_LOG_TAGS)
                {
                    /* Ignore logs inside tests */
                    ctx->skip_state = ctx->state;
                    ctx->skip_depth = 1;
                    ctx->state = TRC_REPORT_LOG_PARSE_SKIP;
                }
                else
                {
                    ctx->state = TRC_REPORT_LOG_PARSE_LOGS;
                }
            }
            else if (strcmp(tag, "test") == 0)
            {
                ctx->state = TRC_REPORT_LOG_PARSE_TEST;
                ctx->type = TRC_TEST_SCRIPT;
            }
            else if (strcmp(tag, "pkg") == 0)
            {
                ctx->state = TRC_REPORT_LOG_PARSE_TEST;
                ctx->type = TRC_TEST_PACKAGE;
            }
            else if (strcmp(tag, "session") == 0)
            {
                ctx->state = TRC_REPORT_LOG_PARSE_TEST;
                ctx->type = TRC_TEST_SESSION;
            }
            else
            {
                ERROR("Unexpected element '%s' in the root state", tag);
                ctx->rc = TE_EFMT;
            }
            if (ctx->state == TRC_REPORT_LOG_PARSE_TEST)
                trc_report_test_entry(ctx, attrs);
            break;

        case TRC_REPORT_LOG_PARSE_TEST:
            if (strcmp(tag, "meta") == 0)
            {
                if (ctx->iter_data == NULL)
                {
                    /* Ignore metadata of ignored tests */
                    ctx->skip_state = ctx->state;
                    ctx->skip_depth = 1;
                    ctx->state = TRC_REPORT_LOG_PARSE_SKIP;
                }
                else
                {
                    ctx->state = TRC_REPORT_LOG_PARSE_META;
                }
            }
            else if (strcmp(tag, "branch") == 0)
            {
                ctx->state = TRC_REPORT_LOG_PARSE_ROOT;
            }
            else if (strcmp(tag, "logs") == 0)
            {
                /* Ignore logs inside tests */
                ctx->skip_state = ctx->state;
                ctx->skip_depth = 1;
                ctx->state = TRC_REPORT_LOG_PARSE_SKIP;
            }
            else
            {
                ERROR("Unexpected element '%s' in the "
                      "test/package/session", tag);
                ctx->rc = TE_EFMT;
            }
            break;
        
        case TRC_REPORT_LOG_PARSE_META:
            if (strcmp(tag, "objective") == 0)
            {
                ctx->state = TRC_REPORT_LOG_PARSE_OBJECTIVE;
                assert(ctx->str == NULL);
            }
            else if (strcmp(tag, "verdicts") == 0)
            {
                ctx->state = TRC_REPORT_LOG_PARSE_VERDICTS;
            }
            else if (strcmp(tag, "params") == 0)
            {
                ctx->state = TRC_REPORT_LOG_PARSE_PARAMS;
            }
            else 
            {
                ctx->skip_state = ctx->state;
                ctx->skip_depth = 1;
                ctx->state = TRC_REPORT_LOG_PARSE_SKIP;
            }
            break;
            
        case TRC_REPORT_LOG_PARSE_VERDICTS:
            if (strcmp(tag, "verdict") == 0)
            {
                ctx->state = TRC_REPORT_LOG_PARSE_VERDICT;
                assert(ctx->str == NULL);
            }
            else
            {
                ERROR("Unexpected element '%s' in 'verdicts'", tag);
                ctx->rc = TE_EFMT;
            }
            break;

        case TRC_REPORT_LOG_PARSE_PARAMS:
            if (strcmp(tag, "param") == 0)
            {
                trc_report_test_param(ctx, attrs);
                /* FIXME */
                ctx->skip_state = ctx->state;
                ctx->skip_depth = 1;
                ctx->state = TRC_REPORT_LOG_PARSE_SKIP;
            }
            else
            {
                ERROR("Unexpected element '%s' in 'params'", tag);
                ctx->rc = TE_EFMT;
            }
            break;

        case TRC_REPORT_LOG_PARSE_LOGS:
            if (strcmp(tag, "msg") == 0)
            {
                te_bool entity_match = FALSE;
                te_bool user_match = FALSE;

                while ((!entity_match || !user_match) &&
                       (attrs[0] != NULL) && (attrs[1] != NULL))
                {
                    if (!entity_match &&
                        xmlStrcmp(attrs[0], CONST_CHAR2XML("entity")) == 0)
                    {
                        if (xmlStrcmp(attrs[1],
                                      CONST_CHAR2XML("Dispatcher")) != 0)
                            break;
                        entity_match = TRUE;
                    }
                    if (!user_match &&
                        xmlStrcmp(attrs[0], CONST_CHAR2XML("user")) == 0)
                    {
                        if (xmlStrcmp(attrs[1],
                                      CONST_CHAR2XML("TRC tags")) != 0)
                            break;
                        user_match = TRUE;
                    }
                    attrs += 2;
                }
                if (entity_match && user_match)
                {
                    ctx->state = TRC_REPORT_LOG_PARSE_TAGS;
                    assert(ctx->str == NULL);
                }
                else
                {
                    ctx->skip_state = ctx->state;
                    ctx->skip_depth = 1;
                    ctx->state = TRC_REPORT_LOG_PARSE_SKIP;
                }
            }
            else
            {
                ERROR("Unexpected element '%s' in 'logs'", tag);
                ctx->rc = TE_EFMT;
            }
            break;

        default:
            assert(FALSE);
            break;
    }
}

/**
 * Callback function that is called when XML parser meets the end of 
 * an element.
 *
 * @param user_data     Pointer to user-specific data (user context)
 * @param name          The element name
 */
static void
trc_report_log_end_element(void *user_data, const xmlChar *tag)
{
    trc_report_log_parse_ctx   *ctx = user_data;

    assert(ctx != NULL);
    ENTRY("state=%u rc=%r tag=%s", ctx->state, ctx->rc, tag);

    if (ctx->rc != 0)
        return;

    switch (ctx->state)
    {
        case TRC_REPORT_LOG_PARSE_SKIP:
            if (--(ctx->skip_depth) == 0)
                ctx->state = ctx->skip_state;
            break;

        case TRC_REPORT_LOG_PARSE_ROOT:
            if (strcmp(tag, "branch") == 0)
            {
                ctx->state = TRC_REPORT_LOG_PARSE_TEST;
            }
            else
            {
                assert(strcmp(tag, "proteos:log_report") == 0);
                ctx->state = TRC_REPORT_LOG_PARSE_INIT;
            }
            break;

        case TRC_REPORT_LOG_PARSE_LOGS:
            assert(strcmp(tag, "logs") == 0);
            ctx->state = TRC_REPORT_LOG_PARSE_ROOT;
            break;

        case TRC_REPORT_LOG_PARSE_TEST:
            if (ctx->iter_data != NULL)
            {
                trc_report_free_test_iter_data(ctx->iter_data);
                ctx->iter_data = NULL;
                ERROR("No meta data for the test entry!");
                ctx->rc = TE_EFMT;
            }
            if (trc_report_log_parse_stack_pop(ctx))
            {
                /* Step iteration back */
                trc_db_walker_step_back(ctx->db_walker);
                /* Step test entry back */
                trc_db_walker_step_back(ctx->db_walker);
            }
            ctx->state = TRC_REPORT_LOG_PARSE_ROOT;
            break;

        case TRC_REPORT_LOG_PARSE_META:
        {
            trc_report_test_iter_data  *iter_data;

            assert(strcmp(tag, "meta") == 0);
            ctx->state = TRC_REPORT_LOG_PARSE_TEST;

            if (!trc_db_walker_step_iter(ctx->db_walker, ctx->args_n,
                                         ctx->args_name, ctx->args_value,
                                         TRUE))
            {
                ERROR("Unable to create a new iteration");
                ctx->rc = TE_ENOMEM;
                break;
            }

            ctx->args_n = 0;
            iter_data = trc_db_walker_get_user_data(ctx->db_walker,
                                                    ctx->db_uid);
            if (iter_data == NULL)
            {
                /* Get expected result */
                ctx->iter_data->exp_result =
                    trc_db_walker_get_exp_result(ctx->db_walker,
                                                 ctx->tags);
                /* Update statistics */
                if (ctx->type == TRC_TEST_SCRIPT)
                    TAILQ_FIRST(&ctx->iter_data->runs)->is_exp = 
                        trc_report_test_iter_stats_update(
                            &ctx->iter_data->stats,
                            ctx->iter_data->exp_result,
                            &TAILQ_FIRST(&ctx->iter_data->runs)->result);
                /* Attach iteration data to TRC database */
                ctx->rc = trc_db_walker_set_user_data(ctx->db_walker,
                                                      ctx->db_uid,
                                                      ctx->iter_data);
                if (ctx->rc != 0)
                    break;
            }
            else
            {
                /* Update statistics */
                if (ctx->type == TRC_TEST_SCRIPT)
                    TAILQ_FIRST(&ctx->iter_data->runs)->is_exp = 
                        trc_report_test_iter_stats_update(
                            &iter_data->stats, iter_data->exp_result,
                            &TAILQ_FIRST(&ctx->iter_data->runs)->result);
                /* Merge a new entry */
                trc_report_merge_test_iter_data(iter_data,
                                                ctx->iter_data);
            }
            ctx->iter_data = NULL;
            break;
        }

        case TRC_REPORT_LOG_PARSE_OBJECTIVE:
            assert(strcmp(tag, "objective") == 0);
            if (ctx->str != NULL)
            {
                trc_test   *test = NULL;

                test = trc_db_walker_get_test(ctx->db_walker);
                assert(test != NULL);
                if (test->objective == NULL ||
                    strcmp(test->objective, ctx->str) != 0)
                {
                    test->obj_update = TRUE;
                }
                free(test->objective);
                test->objective = ctx->str;
                ctx->str = NULL;
            }
            ctx->state = TRC_REPORT_LOG_PARSE_META;
            break;

        case TRC_REPORT_LOG_PARSE_PARAMS:
            assert(strcmp(tag, "params") == 0);
            ctx->state = TRC_REPORT_LOG_PARSE_META;
            break;

        case TRC_REPORT_LOG_PARSE_VERDICTS:
            assert(strcmp(tag, "verdicts") == 0);
            ctx->state = TRC_REPORT_LOG_PARSE_META;
            break;

        case TRC_REPORT_LOG_PARSE_VERDICT:
            assert(strcmp(tag, "verdict") == 0);
            if (ctx->str != NULL)
            {
                te_test_verdict *verdict;

                verdict = TE_ALLOC(sizeof(*verdict));
                if (verdict == NULL)
                {
                    ERROR("Memory allocation failure");
                    free(ctx->str);
                    ctx->str = NULL;
                    ctx->rc = TE_ENOMEM;
                    return;
                }
                verdict->str = ctx->str;
                ctx->str = NULL;
                assert(ctx->iter_data != NULL);
                TAILQ_INSERT_TAIL(
                    &TAILQ_FIRST(&ctx->iter_data->runs)->result.verdicts,
                    verdict, links);
            }
            ctx->state = TRC_REPORT_LOG_PARSE_VERDICTS;
            break;

        case TRC_REPORT_LOG_PARSE_TAGS:
            assert(strcmp(tag, "msg") == 0);
            trc_tags_str_to_list(ctx->tags, ctx->str);
            free(ctx->str);
            ctx->str = NULL;
            ctx->state = TRC_REPORT_LOG_PARSE_LOGS;
            break;

        default:
            ERROR("%s(): Unexpected state %u", __FUNCTION__, ctx->state);
            assert(FALSE);
            break;
    }
}

/**
 * Callback function that is called when XML parser meets character data.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 * @param  ch         Pointer to the string
 * @param  len        Number of the characters in the string
 *
 * @return Nothing
 */
static void
trc_report_log_characters(void *user_data, const xmlChar *ch, int len)
{
    trc_report_log_parse_ctx   *ctx = user_data;
    size_t                      init_len;

    assert(ctx != NULL);

    if (ctx->rc != 0)
        return;

    /*
     * Don't want to update objective to empty string.
     * Empty verdict is meaningless.
     * Empty list of TRC tags is useless.
     */
    if (len == 0)
        return;

    switch (ctx->state)
    {
        case TRC_REPORT_LOG_PARSE_VERDICT:
        case TRC_REPORT_LOG_PARSE_OBJECTIVE:
        case TRC_REPORT_LOG_PARSE_TAGS:
            break;

        default:
            /* Just ignore */
            return;
    }

    init_len = (ctx->str != NULL) ? strlen(ctx->str) : 0;
    ctx->str = realloc(ctx->str, init_len + len + 1);
    if (ctx->str == NULL)
    {
        ERROR("Memory allocation failure");
        ctx->rc = TE_ENOMEM;
    }
    else
    {
        memcpy(ctx->str + init_len, ch, len);
        ctx->str[init_len + len] = '\0';
    }
}

/**
 * Function to report issues (warnings, errors) generated by XML
 * SAX parser.
 *
 * @param user_data     TRC report context
 * @param msg           Message
 */
static void
trc_report_log_problem(void *user_data, const char *msg, ...)
{
    va_list ap;

    UNUSED(user_data);
    
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
}

/* See the description in trc_report.h */
te_errno
trc_report_process_log(trc_report_ctx *gctx, const char *log)
{
    te_errno                    rc = 0;
    trc_report_log_parse_ctx    ctx;

    /**
     * The structure specifies all types callback routines that
     * should be called.
     */
    xmlSAXHandler sax_handler = {
        .internalSubset         = NULL,
        .isStandalone           = NULL,
        .hasInternalSubset      = NULL,
        .hasExternalSubset      = NULL,
        .resolveEntity          = NULL,
        .getEntity              = NULL,
        .entityDecl             = NULL,
        .notationDecl           = NULL,
        .attributeDecl          = NULL,
        .elementDecl            = NULL,
        .unparsedEntityDecl     = NULL,
        .setDocumentLocator     = NULL,
        .startDocument          = trc_report_log_start_document,
        .endDocument            = trc_report_log_end_document,
        .startElement           = trc_report_log_start_element,
        .endElement             = trc_report_log_end_element,
        .reference              = NULL,
        .characters             = trc_report_log_characters,
        .ignorableWhitespace    = NULL,
        .processingInstruction  = NULL,
        .comment                = NULL,
        .warning                = trc_report_log_problem,
        .error                  = trc_report_log_problem,
        .fatalError             = trc_report_log_problem,
        .getParameterEntity     = NULL,
        .cdataBlock             = NULL,
        .externalSubset         = NULL,
        .initialized            = 1,
        /* 
         * The following fields are extensions available only 
         * on version 2
         */
#if HAVE___STRUCT__XMLSAXHANDLER__PRIVATE
        ._private               = NULL,
#endif
#if HAVE___STRUCT__XMLSAXHANDLER_STARTELEMENTNS
        .startElementNs         = NULL,
#endif
#if HAVE___STRUCT__XMLSAXHANDLER_ENDELEMENTNS
        .endElementNs           = NULL,
#endif
#if HAVE___STRUCT__XMLSAXHANDLER_SERROR___
        .serror                 = NULL
#endif
    };

    memset(&ctx, 0, sizeof(ctx));
    ctx.flags = gctx->flags;
    ctx.db = gctx->db;
    ctx.db_uid = gctx->db_uid;
    ctx.log = (log != NULL) ? log : "-";
    ctx.tags = &gctx->tags;

    if (xmlSAXUserParseFile(&sax_handler, &ctx, ctx.log) != 0)
    {
        ERROR("Cannot parse XML document with TE log '%s'", ctx.log);
        rc = TE_EFMT;
    }
    else if ((rc = ctx.rc) != 0)
    {
        ERROR("Processing of the XML document with TE log '%s' "
              "failed: %r", ctx.log, rc);
    }
    else if ((rc = trc_report_collect_stats(gctx)) != 0)
    {
        ERROR("Collect of TRC report statistics failed: %r", rc);
    }

    free(ctx.stack_info);
    for (ctx.args_n = 0; ctx.args_n < ctx.args_max; ctx.args_n++)
    {
        free(ctx.args_name[ctx.args_n]);
        free(ctx.args_value[ctx.args_n]);
    }
    free(ctx.args_name);
    free(ctx.args_value);

    return rc;
}
