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
#include "te_string.h"
#include "trc_tags.h"
#include "log_parse.h"
#include "trc_report.h"
#include "trc_update.h"
#include "trc_diff.h"
#include "trc_db.h"

#define CONST_CHAR2XML  (const xmlChar *)
#define XML2CHAR(p)     ((char *)p)


/** State of the TE log parser from TRC point of view */
typedef enum trc_log_parse_state {
    TRC_LOG_PARSE_INIT,      /**< Initial state */
    TRC_LOG_PARSE_ROOT,      /**< Root state */
    TRC_LOG_PARSE_TAGS,      /**< Inside log message with TRC
                                         tags list */
    TRC_LOG_PARSE_TEST,      /**< Inside 'test', 'pkg' or
                                         'session' element */
    TRC_LOG_PARSE_META,      /**< Inside 'meta' element */
    TRC_LOG_PARSE_OBJECTIVE, /**< Inside 'objective' element */
    TRC_LOG_PARSE_VERDICTS,  /**< Inside 'verdicts' element */
    TRC_LOG_PARSE_VERDICT,   /**< Inside 'verdict' element */
    TRC_LOG_PARSE_PARAMS,    /**< Inside 'params' element */
    TRC_LOG_PARSE_LOGS,      /**< Inside 'logs' element */
    TRC_LOG_PARSE_SKIP,      /**< Skip entire contents */
} trc_log_parse_state;

/** TRC report TE log parser context. */
typedef struct trc_log_parse_ctx {

    te_errno            rc;         /**< Status of processing */

    unsigned int        flags;      /**< Processing flags */
    te_trc_db          *db;         /**< TRC database handle */
    unsigned int        db_uid;     /**< TRC database user ID */
    const char         *log;        /**< Name of the file with log */
    tqh_strings        *tags;       /**< List of tags */
    te_trc_db_walker   *db_walker;  /**< TRC database walker */
    char               *run_name;   /**< Name of the tests run the
                                         current log belongs to */

    trc_log_parse_state         state;  /**< Log parse state */
    trc_test_type               type;   /**< Type of the test */

    char   *str;    /**< Accumulated string (used when single string is
                         reported by few trc_log_parse_characters()
                         calls because of entities in it) */

    unsigned int                skip_depth; /**< Skip depth */
    trc_log_parse_state         skip_state; /**< State to return */

    trc_report_test_iter_data  *iter_data;  /**< Current test iteration
                                                 data */
    logic_expr                 *merge_expr; /**< Tag expression with
                                                 which new results
                                                 should be merged into
                                                 existing database */

    func_args_match_ptr    func_args_match; /**< Function to match
                                                 iterations in TRC with
                                                 iterations from logs */

    trc_update_test_entries *updated_tests; /**< List of tests to be
                                                 updated */

    tqh_strings        *test_paths; /**< Paths of tests to be updated */

    unsigned int        stack_size; /**< Size of the stack in elements */
    te_bool            *stack_info; /**< Stack */
    unsigned int        stack_pos;  /**< Current position in the stack */


} trc_log_parse_ctx;


/**
 * Push value into the stack.
 * 
 * @param ctx           TRC report log parser context
 * @param value         Value to put on the stack
 *
 * @return Status code.
 */
static te_errno
trc_log_parse_stack_push(trc_log_parse_ctx *ctx,
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
trc_log_parse_stack_pop(trc_log_parse_ctx *ctx)
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
trc_log_parse_start_document(void *user_data)
{
    trc_log_parse_ctx   *ctx = user_data;

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

    ctx->state = TRC_LOG_PARSE_INIT;
}

/**
 * Callback function that is called when XML parser reaches the end 
 * of the document.
 *
 * @param user_data     Pointer to user-specific data (user context)
 */
static void
trc_log_parse_end_document(void *user_data)
{
    trc_log_parse_ctx   *ctx = user_data;

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
#if 0
static trc_report_test_iter_data *
trc_report_get_iter_data(const te_trc_db_walker *walker,
                         unsigned int uid,
                         const char *run_name)
{
    trc_report_test_iter_data *iter_data;

    for (iter_data = trc_db_walker_get_user_data(walker, uid);
         iter_data != NULL;
         iter_data = SLIST_NEXT(iter_data, links))
    {
        if ((run_name == NULL) && (iter_data->run_name == NULL))
            break;

        if ((run_name != NULL) && (iter_data->run_name != NULL) &&
            (strcmp(iter_data->run_name, run_name) == 0))
            break;
    }
    return iter_data;
}

static te_errno
trc_report_add_iter_data(const te_trc_db_walker *walker,
                         unsigned int uid,
                         trc_report_test_iter_data *iter_data)
{
    te_errno rc;
    trc_report_test_iter_data *prev = NULL;
    trc_report_test_iter_data *curr = NULL;

    curr = trc_db_walker_get_user_data(walker, uid);
    while (curr != NULL)
    {
        if ((curr->run_name == NULL) && (iter_data->run_name == NULL))
            break;

        if ((curr->run_name != NULL) && (iter_data->run_name != NULL) &&
            (strcmp(curr->run_name, iter_data->run_name) == 0))
            break;

        prev = curr;
        curr = SLIST_NEXT(curr, links);
    }

    if (curr != NULL)
    {
        SLIST_NEXT(iter_data, links) = SLIST_NEXT(iter_data, links);
        free(curr);
    }
    else
        SLIST_NEXT(iter_data, links) = NULL;

    if (prev != NULL)
        SLIST_NEXT(prev, links) = iter_data;
    else
    {
        rc = trc_db_walker_set_user_data(walker, uid, iter_data);
    }

    return 0;
}
#endif

/**
 * Process test script, package or session entry point.
 *
 * @param ctx           Parser context
 * @param attrs         An array of attribute name, attribute value pairs
 */
static void
trc_log_parse_test_entry(trc_log_parse_ctx *ctx, const xmlChar **attrs)
{
    int             tin = -1;
    char           *hash = NULL;
    te_bool         name_found = FALSE;
    te_bool         status_found = FALSE;
    te_test_status  status = TE_TEST_UNSPEC;
    char           *s = NULL;
    trc_test       *test = NULL;

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
                test = trc_db_walker_get_test(ctx->db_walker);

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
                ctx->rc = trc_log_parse_stack_push(ctx, TRUE);
            }
        }
        else if (xmlStrcmp(attrs[0], CONST_CHAR2XML("result")) == 0)
        {
            status_found = TRUE;
            ctx->rc = te_test_str2status(XML2CHAR(attrs[1]), &status);
        }
        else if (xmlStrcmp(attrs[0], CONST_CHAR2XML("tin")) == 0)
        {
            if (sscanf(XML2CHAR(attrs[1]), "%d", &tin) < 1)
            {
                ctx->rc = TE_EFMT;
            }
        }
        else if (xmlStrcmp(attrs[0], CONST_CHAR2XML("hash")) == 0)
        {
            if ((hash = strdup(XML2CHAR(attrs[1]))) == NULL)
            {
                ctx->rc = TE_ENOMEM;
            }
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
        ctx->rc = trc_log_parse_stack_push(ctx, FALSE);
    }
    else if (!status_found)
    {
        ERROR("Status of the test/package/session not found");
        ctx->rc = TE_EFMT;
    }
    else
    {
        trc_report_test_iter_entry    *entry;
        tqe_string                    *tqe_str = NULL;

        assert(test != NULL);

        if (ctx->test_paths != NULL && !TAILQ_EMPTY(ctx->test_paths))
        {
            /* Determining whether a given test is in one
             * of paths to tests to be updated */
            TAILQ_FOREACH(tqe_str, ctx->test_paths, links)
            {
                s = strstr(test->path, tqe_str->v);
                if (s != NULL && s - test->path <= 1)
                    break;
                s = strstr(tqe_str->v, test->path + 1);
                if (s != NULL &&
                    (s == tqe_str->v ||
                     (s == tqe_str->v + 1 &&
                      tqe_str->v[0] == '/')) &&
                    *(s + strlen(test->path) - 1) == '/')
                    break;
            }

            if (tqe_str == NULL)
            {
                /* If test is not in one of paths of
                 * tests to be updated - skip it */
                ctx->skip_state = TRC_LOG_PARSE_ROOT;
                ctx->skip_depth = 1;
                ctx->state = TRC_LOG_PARSE_SKIP;
                trc_db_walker_step_back(ctx->db_walker);
                trc_log_parse_stack_pop(ctx);
                return;
            }
        }

        /* Pre-allocate entry for result */
        entry = TE_ALLOC(sizeof(*entry));
        if (entry == NULL)
        {
            ctx->rc = TE_ENOMEM;
            return;
        }
        te_test_result_init(&entry->result);
        entry->result.status = status;
        entry->tin = tin;
        entry->hash = hash;
        entry->args = NULL;
        entry->args_max = 0;
        entry->args_n = 0;

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
trc_log_parse_test_param(trc_log_parse_ctx *ctx, const xmlChar **attrs)
{
    trc_report_test_iter_entry    *entry;
    unsigned int                   i = 0;
    void                          *p = NULL;

    entry = TAILQ_FIRST(&ctx->iter_data->runs);
    assert(entry != NULL);
    assert(TAILQ_NEXT(entry, links) == NULL);

    assert(entry->args_n <= entry->args_max);

    if (entry->args_max == 0)
    {
        entry->args_max = 20;
        entry->args = calloc(entry->args_max,
                             sizeof(*(entry->args)));
        if (entry->args == NULL)
        {
            ctx->rc = TE_ENOMEM;
            return;
        }
    }

    if (entry->args_n == entry->args_max)
    {
        entry->args_max += 5;
        p = realloc(entry->args,
                    sizeof(*(entry->args)) * entry->args_max);
        if (p == NULL)
        {
            ctx->rc = TE_ENOMEM;
            return;
        }
        entry->args = p;

        for (i = entry->args_n; i < entry->args_max; i++)
        {
            entry->args[i].name = NULL;
            entry->args[i].value = NULL;
            entry->args[i].variable = FALSE;
        }
    }

    while (attrs[0] != NULL && attrs[1] != NULL)
    {
        if (xmlStrcmp(attrs[0], CONST_CHAR2XML("name")) == 0)
        {
            free(entry->args[entry->args_n].name);
            entry->args[entry->args_n].name = strdup(XML2CHAR(attrs[1]));
            if (entry->args[entry->args_n].name == NULL)
            {
                ERROR("strdup(%s) failed", attrs[1]);
                ctx->rc = TE_ENOMEM;
                return;
            }
        }
        else if (xmlStrcmp(attrs[0], CONST_CHAR2XML("value")) == 0)
        {
            free(entry->args[entry->args_n].value);
            entry->args[entry->args_n].value = strdup(XML2CHAR(attrs[1]));
            if (entry->args[entry->args_n].value == NULL)
            {
                ERROR("strdup(%s) failed", XML2CHAR(attrs[1]));
                ctx->rc = TE_ENOMEM;
                return;
            }
        }
        attrs += 2;
    }

    if (entry->args[entry->args_n].name == NULL ||
        entry->args[entry->args_n].value == NULL)
    {
        ERROR("Invalid format of the test parameter specification");
        ctx->rc = TE_EFMT;
    }
    else
    {
        entry->args_n++;
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
 * Callback function that is called when XML parser meets character data.
 *
 * @param  user_data  Pointer to user-specific data (user context)
 * @param  ch         Pointer to the string
 * @param  len        Number of the characters in the string
 *
 * @return Nothing
 */
static void
trc_log_parse_characters(void *user_data, const xmlChar *ch, int len)
{
    trc_log_parse_ctx   *ctx = user_data;
    size_t               init_len;

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
        case TRC_LOG_PARSE_VERDICT:
        case TRC_LOG_PARSE_OBJECTIVE:
        case TRC_LOG_PARSE_TAGS:
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
 * Callback function that is called when XML parser meets an opening tag.
 *
 * @param user_data     Pointer to user-specific data (user context)
 * @param name          The element name
 * @param attrs         An array of attribute name, attribute value pairs
 */
static void
trc_log_parse_start_element(void *user_data,
                             const xmlChar *name, const xmlChar **attrs)
{
    trc_log_parse_ctx   *ctx = user_data;
    const char          *tag = XML2CHAR(name);

    assert(ctx != NULL);
    ENTRY("state=%u rc=%r tag=%s", ctx->state, ctx->rc, tag);

    if (ctx->rc != 0)
        return;

    switch (ctx->state)
    {
        case TRC_LOG_PARSE_SKIP:
            ctx->skip_depth++;
            break;

        case TRC_LOG_PARSE_INIT:
            if (strcmp(tag, "proteos:log_report") != 0)
            {
                ERROR("Unexpected element '%s' at start of TE log XML",
                      tag);
                ctx->rc = TE_EFMT;
            }
            else
            {
                ctx->state = TRC_LOG_PARSE_ROOT;
            }
            break;

        case TRC_LOG_PARSE_ROOT:
            if (strcmp(tag, "logs") == 0)
            {
                /* TODO: Avoid search of tags inside tests */
                if (ctx->flags & TRC_LOG_PARSE_IGNORE_LOG_TAGS)
                {
                    /* Ignore logs inside tests */
                    ctx->skip_state = ctx->state;
                    ctx->skip_depth = 1;
                    ctx->state = TRC_LOG_PARSE_SKIP;
                }
                else
                {
                    ctx->state = TRC_LOG_PARSE_LOGS;
                }
            }
            else if (strcmp(tag, "test") == 0)
            {
                ctx->state = TRC_LOG_PARSE_TEST;
                ctx->type = TRC_TEST_SCRIPT;
            }
            else if (strcmp(tag, "pkg") == 0)
            {
                ctx->state = TRC_LOG_PARSE_TEST;
                ctx->type = TRC_TEST_PACKAGE;
            }
            else if (strcmp(tag, "session") == 0)
            {
                ctx->state = TRC_LOG_PARSE_TEST;
                ctx->type = TRC_TEST_SESSION;
            }
            else
            {
                ERROR("Unexpected element '%s' in the root state", tag);
                ctx->rc = TE_EFMT;
            }
            if (ctx->state == TRC_LOG_PARSE_TEST)
                trc_log_parse_test_entry(ctx, attrs);
            break;

        case TRC_LOG_PARSE_TEST:
            if (strcmp(tag, "meta") == 0)
            {
                if (ctx->iter_data == NULL)
                {
                    /* Ignore metadata of ignored tests */
                    ctx->skip_state = ctx->state;
                    ctx->skip_depth = 1;
                    ctx->state = TRC_LOG_PARSE_SKIP;
                }
                else
                {
                    ctx->state = TRC_LOG_PARSE_META;
                }
            }
            else if (strcmp(tag, "branch") == 0)
            {
                ctx->state = TRC_LOG_PARSE_ROOT;
            }
            else if (strcmp(tag, "logs") == 0)
            {
                /* Ignore logs inside tests */
                ctx->skip_state = ctx->state;
                ctx->skip_depth = 1;
                ctx->state = TRC_LOG_PARSE_SKIP;
            }
            else
            {
                ERROR("Unexpected element '%s' in the "
                      "test/package/session", tag);
                ctx->rc = TE_EFMT;
            }
            break;

        case TRC_LOG_PARSE_META:
            if (strcmp(tag, "objective") == 0)
            {
                ctx->state = TRC_LOG_PARSE_OBJECTIVE;
                assert(ctx->str == NULL);
            }
            else if (strcmp(tag, "verdicts") == 0)
            {
                ctx->state = TRC_LOG_PARSE_VERDICTS;
            }
            else if (strcmp(tag, "params") == 0)
            {
                ctx->state = TRC_LOG_PARSE_PARAMS;
            }
            else 
            {
                ctx->skip_state = ctx->state;
                ctx->skip_depth = 1;
                ctx->state = TRC_LOG_PARSE_SKIP;
            }
            break;

        case TRC_LOG_PARSE_VERDICTS:
            if (strcmp(tag, "verdict") == 0)
            {
                ctx->state = TRC_LOG_PARSE_VERDICT;
                assert(ctx->str == NULL);
            }
            else
            {
                ERROR("Unexpected element '%s' in 'verdicts'", tag);
                ctx->rc = TE_EFMT;
            }
            break;

        case TRC_LOG_PARSE_VERDICT:
            if (strcmp(tag, "br") == 0)
            {
                trc_log_parse_characters(user_data,
                                         CONST_CHAR2XML("\n"),
                                         strlen("\n"));
            }
            else
            {
                ERROR("Unexpected element '%s' in 'verdict'", tag);
                ctx->rc = TE_EFMT;
            }
            break;

        case TRC_LOG_PARSE_PARAMS:
            if (strcmp(tag, "param") == 0)
            {
                trc_log_parse_test_param(ctx, attrs);
                /* FIXME */
                ctx->skip_state = ctx->state;
                ctx->skip_depth = 1;
                ctx->state = TRC_LOG_PARSE_SKIP;
            }
            else
            {
                ERROR("Unexpected element '%s' in 'params'", tag);
                ctx->rc = TE_EFMT;
            }
            break;

        case TRC_LOG_PARSE_LOGS:
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
                    ctx->state = TRC_LOG_PARSE_TAGS;
                    assert(ctx->str == NULL);
                }
                else
                {
                    ctx->skip_state = ctx->state;
                    ctx->skip_depth = 1;
                    ctx->state = TRC_LOG_PARSE_SKIP;
                }
            }
            else
            {
                ERROR("Unexpected element '%s' in 'logs'", tag);
                ctx->rc = TE_EFMT;
            }
            break;

        default:
            ERROR("Unexpected state %u at start of a new element '%s'",
                  ctx->state, tag);
            assert(FALSE);
            break;
    }
}

/**
 * Generate TRC Update user data.
 *
 * @param data      Pointer to data used for generationg
 *                  (boolean value determining whether to
 *                  save element to which generated user data
 *                  is attached in this case)
 * @param is_iter   Whether data is generated for iteration or test
 *
 * @return Generated data pointer
 */
static void *
trc_update_gen_user_data(void *data, te_bool is_iter)
{
    te_bool     to_save = *((te_bool *)data);

    if (is_iter)
    {
        trc_update_test_iter_data *iter_d = TE_ALLOC(sizeof(*iter_d));
        SLIST_INIT(&iter_d->new_results);

        if (to_save)
            iter_d->to_save = TRUE;
        else
            iter_d->to_save = FALSE;

        return iter_d;
    }
    else
    {
        trc_update_test_data *test_d = TE_ALLOC(sizeof(*test_d));
    
        if (to_save)
            test_d->to_save = TRUE;
        else
            test_d->to_save = FALSE;

        return test_d;
    }
}

/**
 * Generate and attach TRC Update data to a given set of tests.
 *
 * @param tests         Set of tests in TRC DB
 * @param updated_tests Tests to be updated
 * @param user_id       TRC DB user ID
 *
 * @return Status code
 */
static te_errno trc_update_fill_tests_user_data(trc_tests *tests,
                                                trc_update_test_entries
                                                        *updated_tests,
                                                unsigned int user_id);

/**
 * Generate and attach TRC Update data to a given set of iterations.
 *
 * @param iters         Set of iterations in TRC DB
 * @param updated_tests Tests to be updated
 * @param user_id       TRC DB user ID
 *
 * @return Status code
 */
static int
trc_update_fill_iters_user_data(trc_test_iters *iters,
                                trc_update_test_entries *updated_tests,
                                unsigned int user_id)
{
    trc_update_test_iter_data   *user_data;
    trc_test_iter               *iter;
    te_bool                      to_save = TRUE;
    trc_report_argument         *args;
    unsigned int                 args_n;
    unsigned int                 args_max;
    trc_test_iter_arg           *arg;
    int                          tests_count = 0;

    TAILQ_FOREACH(iter, &iters->head, links)
    {
        user_data = trc_update_gen_user_data(&to_save, TRUE);
        trc_db_iter_set_user_data(iter, user_id, user_data);
        if (!TAILQ_EMPTY(&iter->args.head))
        {
            args_max = 20;
            args = TE_ALLOC(args_max * sizeof(trc_report_argument));
            args_n = 0;

            TAILQ_FOREACH(arg, &iter->args.head, links)
            {
                args[args_n].name = strdup(arg->name);
                args[args_n].value = strdup(arg->value);
                args[args_n].variable = FALSE;
                args_n++;

                if (args_n == args_max)
                {
                    args_max *= 2;
                    args = realloc(args, args_max *
                                          sizeof(trc_report_argument));
                }
            }
            
            user_data->args = args;
            user_data->args_n = args_n;
            user_data->args_max = args_max;
        }

        tests_count += trc_update_fill_tests_user_data(&iter->tests,
                                                       updated_tests,
                                                       user_id);
    }

    return tests_count;
}

/** See description above */
static int
trc_update_fill_tests_user_data(trc_tests *tests,
                                trc_update_test_entries *updated_tests,
                                unsigned int user_id)
{
    trc_update_test_data    *user_data;
    trc_test                *test;
    te_bool                  to_save = TRUE;
    int                      tests_count = 0;
    trc_update_test_entry   *test_entry;

    TAILQ_FOREACH(test, &tests->head, links)
    {
        user_data = trc_update_gen_user_data(&to_save, FALSE);
        trc_db_test_set_user_data(test, user_id, user_data);
        if (trc_update_fill_iters_user_data(&test->iters,
                                            updated_tests,
                                            user_id) == 0)
        {
            test_entry = TE_ALLOC(sizeof(*test_entry));
            test_entry->test = test;
            test_entry->rules = TE_ALLOC(sizeof(trc_update_rules));
            TAILQ_INIT(test_entry->rules);
            TAILQ_INSERT_HEAD(updated_tests, test_entry, links);
        }
        tests_count++;
    }

    return tests_count;
}

/**
 * Generate and attach TRC Update data for all required
 * entries of TRC DB (tests and iterations to be updated).
 *
 * @param db            TRC DB
 * @param updated_tests Tests to be updated (list of all
 *                      tests not including other tests
 *                      will be inserted into this list
 *                      as a result of this function call)
 * @param user_id       TRC DB user ID
 *
 * @return Status code
 */
static te_errno
trc_update_fill_db_user_data(te_trc_db *db,
                             trc_update_test_entries *updated_tests,
                             unsigned int user_id)
{
    return trc_update_fill_tests_user_data(&db->tests, updated_tests,
                                           user_id);
}

/**
 * Get XML representation of TRC updating rule.
 *
 * @param rule      TRC updating rule
 * @param node      XML node where XML representation
 *                  should be placed
 *
 * @return Status code
 */
static te_errno
trc_update_rule_to_xml(trc_update_rule *rule, xmlNodePtr node)
{
#define ID_LEN 20
    char            id_val[ID_LEN];
    xmlNodePtr      rule_node;
    xmlNodePtr      defaults;
    xmlNodePtr      old_res;
    xmlNodePtr      new_res;
    xmlNodePtr      confl_res;

    rule_node = xmlNewChild(node, NULL, BAD_CAST "rule", NULL);
    if (rule->rule_id > 0)
    {
        snprintf(id_val, ID_LEN, "%d", rule->rule_id);
        xmlNewProp(rule_node, BAD_CAST "id", BAD_CAST id_val);
    }

    defaults = xmlNewChild(rule_node, NULL,
                           BAD_CAST "defaults", NULL);
    if (defaults == NULL)
    {
        ERROR("%s(): failed to create <defaults> node", __FUNCTION__);
        return TE_ENOMEM;
    }

    trc_exp_result_to_xml(rule->def_res, defaults,
                          TRUE);

    old_res = xmlNewChild(rule_node, NULL,
                       BAD_CAST "old", NULL);
    if (old_res == NULL)
    {
        ERROR("%s(): failed to create <old> node", __FUNCTION__);
        return TE_ENOMEM;
    }

    if (rule->old_res != NULL)
        trc_exp_results_to_xml(rule->old_res, old_res);

    confl_res = xmlNewChild(rule_node, NULL,
                            BAD_CAST "conflicts",
                            NULL);
    if (old_res == NULL)
    {
        ERROR("%s(): failed to create <conflicts> node", __FUNCTION__);
        return TE_ENOMEM;
    }

    if (rule->confl_res != NULL)
        trc_exp_results_to_xml(rule->confl_res, confl_res);

    if (rule->new_res != NULL && !SLIST_EMPTY(rule->new_res))
    {
        new_res = xmlNewChild(rule_node, NULL,
                           BAD_CAST "new", NULL);
        if (new_res == NULL)
        {
            ERROR("%s(): failed to create <new> node", __FUNCTION__);
            return TE_ENOMEM;
        }
        trc_exp_results_to_xml(rule->new_res, new_res);
    }

    return 0;
}

/**
 * Save updating rules to file.
 *
 * @param tests    List of tests to be updated
 * @param filename Path to file where rules should be saved
 * @param cmd      Command used to run TRC Update tool
 */
static te_errno
save_test_rules_to_file(trc_update_test_entries *tests,
                        const char *filename,
                        char *cmd)
{
    FILE                    *f = NULL;
    trc_update_test_entry   *test;
    trc_update_rule         *rule;
    xmlDocPtr                xml_doc;
    xmlNodePtr               root;
    xmlNodePtr               test_node;
    xmlNodePtr               comment_node;
    xmlNodePtr               command_node;
    xmlChar                 *xml_cmd = NULL;

    char                    *comment_text =
        "\n"
        "This file contains rules for updating TRC of a given test(s).\n"
        "For each specified test a rule is generated for any unique\n"
        "combination of old results specified in TRC for some iteration\n"
        "before updating and non-expected results extracted from logs for\n"
        "the same iteration.\n"
        "\n"
        "Every updating rule (tag <rule>) can contain following sections:\n"
        "1. Default results for iteration (tag <defaults>);\n"
        "2. Results currently specified in TRC (tag <old>);\n"
        "3. Non-expected results extracted from logs (tag <conflicts>);\n"
        "4. Results to be placed in TRC during its update for all\n"
        "   iterations this update rule is related to (tag <new>).\n"
        "5. Wildcards describing to which iterations this rule can\n"
        "   be applied (currently they aren't generated, can be\n"
        "   used manually - tag <args>, more than one tag can\n"
        "   be used).\n"
        "\n"
        "Any section can be omitted. If after generating rules you\n"
        "see rule with all sections omitted it means just that there\n"
        "were some iterations not presented in current TRC (so no\n"
        "<defaults> and <old>) and also having no results from logs (for\n"
        "example due to the fact that these iterations are specified\n"
        "in package.xml but not appeared in logs specified as source\n"
        "for updating TRC) - so no <conflicts> too.\n"
        "\n"
        "<new> section is always omitted after automatic generation\n"
        "of rules. Without this section, rule has no effect. If this\n"
        "section is presented but void, applying of rule will delete\n"
        "results from TRC for all iterations the rule is related to.\n"
        "\n"
        "To use these rules, you should write your edition of results\n"
        "in <new> section of them, and then run trc_update.pl with\n" 
        "\"rules\" parameter set to path of this file. See help of\n"
        "trc_update.pl for more info.\n";

    xml_doc = xmlNewDoc(BAD_CAST "1.0");
    if (xml_doc == NULL)
    {
        ERROR("%s(): xmlNewDoc() failed", __FUNCTION__);
        return TE_ENOMEM;
    }

    root = xmlNewNode(NULL, BAD_CAST "update_rules");
    if (root == NULL)
    {
        ERROR("%s(): xmlNewNode() failed", __FUNCTION__);
        return TE_ENOMEM;
    }

    xmlDocSetRootElement(xml_doc, root);

    comment_node = xmlNewComment(BAD_CAST comment_text);
    if (comment_node == NULL)
    {
        ERROR("%s(): xmlNewComment() failed", __FUNCTION__);
        return TE_ENOMEM;
    }

    if (xmlAddChild(root, comment_node) == NULL)
    {
        ERROR("%s(): xmlAddChild() failed", __FUNCTION__);
        return TE_ENOMEM;
    }

    xml_cmd = xmlEncodeEntitiesReentrant(xml_doc,
                                         BAD_CAST cmd);
    if (xml_cmd == NULL)
    {
        ERROR("xmlEncodeEntitiesReentrant() failed\n");
        return TE_ENOMEM;
    }

    command_node = xmlNewChild(root, NULL, BAD_CAST "command",
                               xml_cmd);
    if (command_node == NULL)
    {
        ERROR("%s(): xmlNewChild() failed", __FUNCTION__);
        return TE_ENOMEM;
    }

    TAILQ_FOREACH(test, tests, links)
    {
        test_node = xmlNewChild(root, NULL, BAD_CAST "test",
                                NULL);
        if (test_node == NULL)
        {
            ERROR("%s(): xmlNewChild() failed", __FUNCTION__);
            return TE_ENOMEM;
        }

        if (xmlNewProp(test_node, BAD_CAST "path",
                       BAD_CAST test->test->path) == NULL)
        {
            ERROR("%s(): xmlNewProp() failed", __FUNCTION__);
            return TE_ENOMEM;
        }

        TAILQ_FOREACH(rule, test->rules, links)
            trc_update_rule_to_xml(rule, test_node);
    }

    f = fopen(filename, "w");
    if (xmlDocFormatDump(f, xml_doc, 1) == -1)
    {
        ERROR("%s(): xmlDocFormatDump() failed", __FUNCTION__);
        return TE_ENOMEM;
    }
    fclose(f);
    xmlFreeDoc(xml_doc);

    return 0;
}

/**
 * Compare expected results of iterations (used for ordering).
 *
 * @param p         First expected result
 * @param q         Second expected result
 * @param tags_cmp  Whether to compare string representation of
 *                  tag expressions or not 
 *
 * @return -1, 0 or 1 as a result of comparison
 */
static int
trc_exp_result_cmp_gen(trc_exp_result *p, trc_exp_result *q,
                       te_bool tags_cmp)
{
    int                      rc;
    trc_exp_result_entry    *p_r;
    trc_exp_result_entry    *q_r;
    te_bool                  p_tags_str_void;
    te_bool                  q_tags_str_void;

    if (p == NULL && q == NULL)
        return 0;
    else if (p == NULL)
        return -1;
    else if (q == NULL)
        return 1;
   
    if (!tags_cmp)
        rc = 0;
    else
    {
        p_tags_str_void = (p->tags_str == NULL ||
                           strlen(p->tags_str) == 0);
        q_tags_str_void = (q->tags_str == NULL ||
                           strlen(q->tags_str) == 0);

        if (p_tags_str_void && q_tags_str_void)
            rc = 0;
        else if (p_tags_str_void)
            rc = -1;
        else if (q_tags_str_void)
            rc = 1;
        else
            rc = strcmp(p->tags_str, q->tags_str);
    }

    if (rc != 0)
        return rc;
    else
    {
        p_r = TAILQ_FIRST(&p->results);
        q_r = TAILQ_FIRST(&q->results);

        while (p_r != NULL && q_r != NULL)
        {
            rc = te_test_result_cmp(&p_r->result,
                                    &q_r->result);
            if (rc != 0)
                return rc;

            p_r = TAILQ_NEXT(p_r, links);
            q_r = TAILQ_NEXT(q_r, links);
        }

        if (p_r == NULL && q_r == NULL)
            return 0;
        else if (p_r != NULL)
            return 1;
        else
            return -1;
    }
}

/**
 * Compare expected results of iterations (used for ordering).
 *
 * @param p         First expected result
 * @param q         Second expected result
 *
 * @return -1, 0 or 1 as a result of comparison
 */
static int
trc_exp_result_cmp(trc_exp_result *p, trc_exp_result *q)
{
    return trc_exp_result_cmp_gen(p, q, TRUE);
}

/**
 * Compare expected results of iterations (used for ordering),
 * do not consider tag expressions in comparison.
 *
 * @param p         First expected result
 * @param q         Second expected result
 *
 * @return -1, 0 or 1 as a result of comparison
 */
static int
trc_exp_result_cmp_no_tags(trc_exp_result *p, trc_exp_result *q)
{
    return trc_exp_result_cmp_gen(p, q, FALSE);
}

/**
 * Compare lists of expected results (used for ordering).
 *
 * @param p         First expected results list.
 * @param q         Second expected results list.
 *
 * @return -1, 0 or 1 as a result of comparison
 */
static int
trc_exp_results_cmp(trc_exp_results *p, trc_exp_results *q)
{
    int                  rc;
    trc_exp_result      *p_r;
    trc_exp_result      *q_r;
    te_bool              p_is_void = (p == NULL || SLIST_EMPTY(p));
    te_bool              q_is_void = (q == NULL || SLIST_EMPTY(q));

    if (p_is_void && q_is_void)
        return 0;
    else if (!p_is_void && q_is_void)
        return 1;
    else if (p_is_void && !q_is_void)
        return -1;

    p_r = SLIST_FIRST(p);
    q_r = SLIST_FIRST(q);

    while (p_r != NULL && q_r != NULL)
    {
        rc = trc_exp_result_cmp(p_r, q_r);
        if (rc != 0)
            return rc;

        p_r = SLIST_NEXT(p_r, links);
        q_r = SLIST_NEXT(q_r, links);
    }

    if (p_r == NULL && q_r == NULL)
        return 0;
    else if (p_r != NULL)
        return 1;
    else
        return -1;
}

/**
 * Match result from XML logs to results currently presented
 * in TRC, add it to list of new results for a given iteration if
 * it is not expected and not encountered yet.
 *
 * @param ctx   TRC Log Parse context
 *
 * @return Status code
 */
static te_errno
trc_update_merge_result(trc_log_parse_ctx *ctx)
{
    int                         rc = 0;
    trc_update_test_iter_data  *upd_iter_data;
    trc_exp_result             *prev = NULL;
    trc_exp_result             *p;
    trc_exp_result_entry       *q;
    trc_exp_result             *merge_result;
    trc_exp_results            *new_results = NULL;
    te_test_result             *te_result;
    trc_exp_result_entry       *result_entry;

    p = (trc_exp_result *) trc_db_walker_get_exp_result(ctx->db_walker,
                                                        ctx->tags);
    if (p != NULL &&
        !(ctx->flags & TRC_LOG_PARSE_CONFLS_ALL) &&
        trc_is_result_expected(
            p,
            &TAILQ_FIRST(&ctx->iter_data->runs)->result))
        return 0;

    upd_iter_data = trc_db_walker_get_user_data(ctx->db_walker,
                                                ctx->db_uid);
    
    te_result = &TAILQ_FIRST(&ctx->iter_data->runs)->result;
    merge_result = TE_ALLOC(sizeof(*merge_result));
    TAILQ_INIT(&merge_result->results);

    result_entry = TE_ALLOC(sizeof(*result_entry));
    result_entry->key = NULL;
    result_entry->notes = NULL;
    te_test_result_cpy(&result_entry->result, te_result);
    TAILQ_INSERT_HEAD(&merge_result->results,
                      result_entry,
                      links);

    merge_result->tags_expr = logic_expr_dup(ctx->merge_expr);
    merge_result->tags_str = logic_expr_to_str(ctx->merge_expr);
    merge_result->key = NULL;
    merge_result->notes = NULL;

    if (ctx->flags & TRC_LOG_PARSE_LOG_WILDS)
        new_results = &trc_db_walker_get_iter(ctx->db_walker)->exp_results;
    else
        new_results = &upd_iter_data->new_results;

    if (SLIST_EMPTY(new_results))
    {
        SLIST_INSERT_HEAD(new_results,
                          merge_result, links);
    }
    else
    {
        SLIST_FOREACH(p, new_results, links)
        {
            if ((rc = strcmp(p->tags_str, merge_result->tags_str)) <= 0)
                break;

            prev = p;
        }

        if (p != NULL && rc == 0)
        {
            TAILQ_FOREACH(q, &p->results, links)
                if (te_test_result_cmp(
                        &q->result,
                        &TAILQ_FIRST(&merge_result->results)->result) == 0)
                    break;
                
            if (q == NULL)
            {
                TAILQ_REMOVE(&merge_result->results, result_entry,
                             links);
                TAILQ_INSERT_TAIL(&p->results, result_entry, links);
            }

            trc_exp_result_free(merge_result);
        }
        else
        {
            if (prev != NULL)
                SLIST_INSERT_AFTER(prev, merge_result, links);
            else
                SLIST_INSERT_HEAD(new_results,
                                  merge_result, links);
        }
    }

    return 0;
}

/**
 * Merge the same results having different tags into
 * single records.
 *
 * @param db_uid    TRC DB User ID
 * @param tests     Tests to be updated
 * @param flags     Flags
 *
 * @return Status code
 */
static te_errno
trc_update_simplify_results(unsigned int db_uid,
                            trc_update_test_entries *tests,
                            int flags)

{
    trc_update_test_entry       *test_entry;
    trc_test_iter               *iter;
    trc_update_test_iter_data   *iter_data;
    trc_exp_results             *new_results = NULL;
    trc_exp_result              *p;
    trc_exp_result              *q;
    trc_exp_result              *tvar;
    logic_expr                  *tags_expr;

    UNUSED(flags);

    TAILQ_FOREACH(test_entry, tests, links)
    {
        TAILQ_FOREACH(iter, &test_entry->test->iters.head, links)
        {
            iter_data = trc_db_iter_get_user_data(iter, db_uid);
            
            if (iter_data == NULL || iter_data->to_save == FALSE)
                continue;

            if (flags & TRC_LOG_PARSE_LOG_WILDS)
                new_results = &iter->exp_results;
            else
                new_results = &iter_data->new_results;

            SLIST_FOREACH(p, new_results, links)
            {
                for (q = SLIST_NEXT(p, links);
                     q != NULL && (tvar = SLIST_NEXT(q, links), 1);
                     q = tvar)
                    
                {
                    if (trc_exp_result_cmp_no_tags(p, q) == 0)
                    {
                        tags_expr = TE_ALLOC(sizeof(*tags_expr));
                        tags_expr->type = LOGIC_EXPR_OR;
                        tags_expr->u.binary.rhv = p->tags_expr;
                        tags_expr->u.binary.lhv = q->tags_expr;
                        
                        p->tags_expr = tags_expr;
                        q->tags_expr = NULL;
                        free(p->tags_str);
                        p->tags_str = logic_expr_to_str(p->tags_expr);
                        SLIST_REMOVE(new_results, q, trc_exp_result,
                                     links);
                        trc_exp_result_free(q);
                    }
                }
            }
        }
    }

    return 0;
}

/**
 * Apply updating rules.
 *
 * @param db_uid    TRC DB User ID
 * @param tests     Tests to be updated
 * @param flags     Flags
 *
 * @return Status code
 */
static te_errno
trc_update_apply_rules(unsigned int db_uid,
                       trc_update_test_entries *tests,
                       int flags)
{
    trc_update_test_entry       *test_entry;
    trc_test_iter               *iter;
    trc_update_test_iter_data   *iter_data;
    trc_exp_results             *results_dup;
    char                        *value;
    int                          rule_id;
    trc_update_rule             *rule;
    trc_update_wilds_list_entry *wild;
    tqe_string                  *expr;

    UNUSED(flags);
    
    TAILQ_FOREACH(test_entry, tests, links)
    {
        TAILQ_FOREACH(iter, &test_entry->test->iters.head, links)
        {
            iter_data = trc_db_iter_get_user_data(iter, db_uid);
            
            if (iter_data == NULL || iter_data->to_save == FALSE)
                continue;

            if (iter_data->rule == NULL)
            {
                rule_id = 0;
                value = XML2CHAR(xmlGetProp(
                                    iter->node,
                                    CONST_CHAR2XML("user_attr")));
                if (value != NULL &&
                    strncmp(value, "rule_", 5) == 0)
                    rule_id = atoi(value + 5);

                TAILQ_FOREACH(rule, test_entry->rules, links)
                {
                    if (rule->rule_id != rule_id && rule->rule_id != 0 &&
                        rule_id != 0)
                        continue;

                    if (rule->apply &&
                        ((rule->rule_id != 0 && rule_id != 0) ||
                         ((rule->def_res == NULL ||
                           trc_exp_result_cmp((struct trc_exp_result *)
                                              iter->exp_default,
                                              rule->def_res) == 0) &&
                          (rule->old_res == NULL ||
                           trc_exp_results_cmp((trc_exp_results *)
                                               &iter->exp_results,
                                               rule->old_res) == 0) &&
                          (rule->confl_res == NULL ||
                           trc_exp_results_cmp(&iter_data->new_results,
                                               rule->confl_res) == 0))))
                    {
                        if (rule->wilds != NULL &&
                            !SLIST_EMPTY(rule->wilds))
                        {
                            SLIST_FOREACH(wild, rule->wilds, links)
                            {
                                assert(wild->args != NULL);
                                assert(iter_data->args != NULL);
                                if (test_iter_args_match(
                                                    wild->args,
                                                    iter_data->args_n,
                                                    iter_data->args,
                                                    wild->is_strict) !=
                                                        ITER_NO_MATCH)
                                {
                                    iter_data->rule = rule;
                                    iter_data->rule_id = rule->rule_id;
                                    break;
                                }
                            }
                        }
                        else if (rule->match_exprs != NULL &&
                                 !TAILQ_EMPTY(rule->match_exprs))
                        {
                            TAILQ_FOREACH(expr, rule->match_exprs, links)
                            {
                                /*
                                 * Sorry, not implemented yet.
                                 */
                            }
                        }
                        else
                        {
                            iter_data->rule = rule;
                            iter_data->rule_id = rule->rule_id;
                            break;
                        }
                    }

                    if (iter_data->rule != NULL)
                        break;
                }
            }

            if (iter_data->rule == NULL || !iter_data->rule->apply)
                continue;

            trc_exp_results_free(&iter->exp_results);
            results_dup = trc_exp_results_dup(
                            iter_data->rule->new_res);
            if (results_dup != NULL)
            {
                memcpy(&iter->exp_results, results_dup,
                       sizeof(*results_dup));
            }

            free(results_dup);
        }
    }

    return 0;
}

/**
 * Get updating rule from XML.
 *
 * @param rule_node     XML node with updating rule
 * @param rule          Where to save updating rule
 *
 * @return Status code
 */
static te_errno
trc_update_load_rule(xmlNodePtr rule_node, trc_update_rule *rule)
{
#define GET_RULE_RESULT(result_) \
    do {                                                            \
        if (rule->result_ != NULL)                                  \
        {                                                           \
            ERROR("Duplicated %s node in TRC updating rules "       \
                  "XML file", #result_);                            \
            return TE_EFMT;                                         \
        }                                                           \
        else                                                        \
        {                                                           \
            rule->result_ = TE_ALLOC(sizeof(trc_exp_results));      \
            SLIST_INIT(rule->result_);                              \
        }                                                           \
        get_expected_results(&first_child_node, rule->result_);   \
    } while (0)

    xmlNodePtr              rule_section_node;
    xmlNodePtr              first_child_node;
    char                   *value;
    trc_exp_result_entry   *entry;
    tqe_string             *tqe_str;

    trc_update_wilds_list_entry *wilds_entry;

    rule_section_node = xmlFirstElementChild(rule_node);

    value = XML2CHAR(xmlGetProp(rule_node,
                                CONST_CHAR2XML("id")));
    if (value != NULL)
        rule->rule_id = atoi(value);

    while (rule_section_node != NULL)
    {
        first_child_node = xmlFirstElementChild(rule_section_node);

        if (xmlStrcmp(rule_section_node->name,
                      CONST_CHAR2XML("args")) == 0)
        {
            if (rule->wilds == NULL)
            {
                rule->wilds = TE_ALLOC(sizeof(*(rule->wilds)));
                SLIST_INIT(rule->wilds);
            }

            wilds_entry = TE_ALLOC(sizeof(*wilds_entry));
            wilds_entry->args = TE_ALLOC(sizeof(*(wilds_entry->args)));
            TAILQ_INIT(&wilds_entry->args->head);
            get_test_args(&first_child_node, wilds_entry->args);

            value = XML2CHAR(xmlGetProp(rule_section_node,
                                        CONST_CHAR2XML("type")));

            if (value != NULL && strcmp(value, "strict") == 0)
                wilds_entry->is_strict = TRUE;
            else
                wilds_entry->is_strict = FALSE;

            SLIST_INSERT_HEAD(rule->wilds, wilds_entry, links);
        }
        else if (xmlStrcmp(rule_section_node->name,
                      CONST_CHAR2XML("match_expr")) == 0)
        {
            if (rule->match_exprs == NULL)
            {
                rule->match_exprs = TE_ALLOC(sizeof(*(rule->match_exprs)));
                TAILQ_INIT(rule->match_exprs);
            }

            tqe_str = TE_ALLOC(sizeof(*tqe_str));
            trc_db_get_text_content(rule_section_node,
                                    &tqe_str->v);
            TAILQ_INSERT_TAIL(rule->match_exprs, tqe_str, links);
        }
        else if (xmlStrcmp(rule_section_node->name,
                      CONST_CHAR2XML("defaults")) == 0)
        {
            if (rule->def_res != NULL)
            {
                ERROR("Duplicated defaults node in TRC updating "
                      "rules XML file");
                return TE_EFMT;
            }
            else
            {
                value = XML2CHAR(xmlGetProp(rule_section_node,
                                            CONST_CHAR2XML("value")));
                rule->def_res = TE_ALLOC(sizeof(trc_exp_result));
                TAILQ_INIT(&rule->def_res->results);
                if (value == NULL)
                     get_expected_result(rule_section_node,
                                         rule->def_res, TRUE);
                else
                {
                    entry = TE_ALLOC(sizeof(*entry));
                    if (entry == NULL)
                        return TE_ENOMEM;
                    te_test_result_init(&entry->result);
                    TAILQ_INSERT_TAIL(&rule->def_res->results,
                                      entry, links);
                    te_test_str2status(value, &entry->result.status);
                    TAILQ_INIT(&entry->result.verdicts);
                }
            }
        }
        else if (xmlStrcmp(rule_section_node->name,
                           CONST_CHAR2XML("old")) == 0)
            GET_RULE_RESULT(old_res);
        else if (xmlStrcmp(rule_section_node->name,
                      CONST_CHAR2XML("conflicts")) == 0)
            GET_RULE_RESULT(confl_res);
        else if (xmlStrcmp(rule_section_node->name,
                      CONST_CHAR2XML("new")) == 0)
        {
            GET_RULE_RESULT(new_res);
            rule->apply = TRUE;
        }
        else
        {
            ERROR("Unexpected %s node in <rule> node of "
                  "TRC updating rules XML file",
                  XML2CHAR(rule_section_node->name));
            return TE_EFMT;
        }

        rule_section_node = xmlNextElementSibling(rule_section_node);
    }

    return 0;
}

/**
 * Load updating rules from file.
 *
 * @param filename  Path to file
 * @param tests     List of tests to be updated
 * @param flags     Flags
 *
 * @return Status code
 */
static te_errno
trc_update_load_rules(char *filename, trc_update_test_entries *tests,
                      int flags)
{
    xmlParserCtxtPtr        parser;
    xmlDocPtr               xml_doc;
    xmlNodePtr              root;
    xmlNodePtr              test_node;
    xmlNodePtr              rule_node;
    char                   *test_name;
    trc_update_test_entry  *test_entry;
    trc_update_rule        *rule;

#if HAVE_XMLERROR
    xmlError           *err;
#endif
    te_errno            rc;

    UNUSED(flags);

    parser = xmlNewParserCtxt();
    if (parser == NULL)
    {
        ERROR("xmlNewParserCtxt() failed");
        return TE_ENOMEM;
    }
    
    xml_doc = xmlCtxtReadFile(parser, filename, NULL,
                              XML_PARSE_NONET | XML_PARSE_NOBLANKS);

    if (xml_doc == NULL)
    {
#if HAVE_XMLERROR
        err = xmlCtxtGetLastError(parser);
        ERROR("Error occured during parsing configuration file:\n"
              "    %s:%d\n    %s", filename,
              err->line, err->message);
#else
        ERROR("Error occured during parsing configuration file:\n"
              "%s", filename);
#endif
        xmlFreeParserCtxt(parser);
        return TE_RC(TE_TRC, TE_EFMT);
    }

    root = xmlDocGetRootElement(xml_doc);
    if (root == NULL)
        return 0;

    if (xmlStrcmp(root->name, CONST_CHAR2XML("update_rules")) != 0)
    {
        ERROR("Unexpected root element of the TRC updating rules "
              "XML file");
        rc = TE_RC(TE_TRC, TE_EFMT);
        goto exit;
    }

    test_node = xmlFirstElementChild(root);
    if (xmlStrcmp(test_node->name, CONST_CHAR2XML("command")) == 0)
        test_node = xmlNextElementSibling(test_node);
    
    while (test_node != NULL)
    {
        if (xmlStrcmp(test_node->name, CONST_CHAR2XML("test")) != 0)
        {
            if (xmlStrcmp(test_node->name, CONST_CHAR2XML("rule")) == 0)
            {
                /*
                 * Rules not belonging to any test explicitly are
                 * added to set of rules of the first test in list
                 */
                test_entry = TAILQ_FIRST(tests);
                if (test_entry != NULL)
                {
                    rule = TE_ALLOC(sizeof(*rule));
                    
                    if ((rc = trc_update_load_rule(test_node, rule)) != 0)
                    {
                        ERROR("Loading rule from file failed");
                        rc = TE_RC(TE_TRC, TE_EFMT);
                        goto exit;
                    }

                    TAILQ_INSERT_TAIL(test_entry->rules, rule, links);
                }
            }
            else
            {
                ERROR("Unexpected %s element of the TRC updating rules "
                     "XML file", test_node->name);
                rc = TE_RC(TE_TRC, TE_EFMT);
                goto exit;
            }
        }

        test_name = XML2CHAR(xmlGetProp(test_node, BAD_CAST "path"));
        
        TAILQ_FOREACH(test_entry, tests, links)
            if (strcmp(test_name, test_entry->test->path) == 0)
                break;
        
        if (test_entry != NULL)
        {
            rule_node = xmlFirstElementChild(test_node);    

            if (test_entry->rules == NULL)
            {
                test_entry->rules = TE_ALLOC(sizeof(*(test_entry->rules)));
                TAILQ_INIT(test_entry->rules);
            }

            while (rule_node != NULL)
            {
                if (xmlStrcmp(rule_node->name,
                              CONST_CHAR2XML("rule")) != 0)
                {
                     ERROR("Unexpected %s element of the TRC updating "
                           "rules XML file", rule_node->name);
                     rc = TE_RC(TE_TRC, TE_EFMT);
                     goto exit;
                }

                rule = TE_ALLOC(sizeof(*rule));
                
                if ((rc = trc_update_load_rule(rule_node, rule)) != 0)
                {
                    ERROR("Loading rule from file for test %s failed",
                          test_entry->test->path);
                    rc = TE_RC(TE_TRC, TE_EFMT);
                    goto exit;
                }

                TAILQ_INSERT_TAIL(test_entry->rules, rule, links);

                rule_node = xmlNextElementSibling(rule_node);
            }
        }

        test_node = xmlNextElementSibling(test_node);
    }

exit:
    xmlFreeParserCtxt(parser);
    xmlFreeDoc(xml_doc);
    
    return rc;
}

/**
 * Delete all updating rules.
 *
 * @param db_uid    TRC DB User ID
 * @param tests     Tests to be updated
 */
static void
trc_update_clear_rules(unsigned int db_uid,
                       trc_update_test_entries *tests)
{
    trc_update_test_entry       *test = NULL;
    trc_test_iter               *iter = NULL;
    trc_update_test_iter_data   *iter_data = NULL;

    if (tests == NULL)
        return;

    TAILQ_FOREACH(test, tests, links)
    {
        trc_update_rules_free(test->rules);

        TAILQ_FOREACH(iter, &test->test->iters.head, links)
        {
            iter_data = trc_db_iter_get_user_data(iter, db_uid);

            if (iter_data == NULL)
                continue;

            iter_data->rule = NULL;
        }
    }
}

/**
 * Generate TRC updating rules.
 *
 * @param db_uid    TRC DB User ID
 * @param tests     Tests to be updated
 * @param flags     Flags
 *
 * @return Status code
 */
static te_errno
trc_update_gen_rules(unsigned int db_uid,
                     trc_update_test_entries *tests,
                     uint32_t flags)
{
    trc_update_test_entry       *test = NULL;
    trc_test_iter               *iter1 = NULL;
    trc_test_iter               *iter2 = NULL;
    trc_update_test_iter_data   *iter_data1 = NULL;
    trc_update_test_iter_data   *iter_data2 = NULL;
    trc_update_rule             *rule;
    int                          cur_rule_id = 0;

    TAILQ_FOREACH(test, tests, links)
    {
        test->rules = TE_ALLOC(sizeof(trc_update_rules));
        TAILQ_INIT(test->rules);

        TAILQ_FOREACH(iter1, &test->test->iters.head, links)
        {
            iter_data1 = trc_db_iter_get_user_data(iter1, db_uid);

            if (iter_data1 == NULL || iter_data1->to_save == FALSE)
                continue;

            if (iter_data1->rule == NULL &&
                (!SLIST_EMPTY(&iter_data1->new_results) ||
                 flags & TRC_LOG_PARSE_RULES_ALL))
            {
                rule = TE_ALLOC(sizeof(*rule));
                rule->def_res = trc_exp_result_dup((struct trc_exp_result *)
                                                   iter1->exp_default);
                rule->old_res = trc_exp_results_dup(&iter1->exp_results);
                rule->confl_res = trc_exp_results_dup(
                                            &iter_data1->new_results);
                if (flags & TRC_LOG_PARSE_COPY_OLD)
                    rule->new_res =
                        trc_exp_results_dup(&iter1->exp_results);
                else if (flags & TRC_LOG_PARSE_COPY_CONFLS)
                    rule->new_res = trc_exp_results_dup(rule->confl_res);
                else
                    rule->new_res = NULL;

                if (flags & TRC_LOG_PARSE_USE_RULE_IDS)
                    cur_rule_id++;
                rule->rule_id = cur_rule_id;

                TAILQ_INSERT_TAIL(test->rules, rule, links);

                iter_data1->rule = rule;
                iter_data1->rule_id = rule->rule_id;

                for (iter2 = TAILQ_NEXT(iter1, links); iter2 != NULL;
                     iter2 = TAILQ_NEXT(iter2, links))
                {
                    iter_data2 = trc_db_iter_get_user_data(
                                    iter2, db_uid);

                    if (iter_data2 == NULL || iter_data2->rule != NULL ||
                        iter_data2->to_save == FALSE)
                        continue;

                    if (trc_exp_result_cmp(
                                (struct trc_exp_result *)
                                            iter1->exp_default,
                                (struct trc_exp_result *)
                                            iter2->exp_default) == 0 &&
                        trc_exp_results_cmp(
                                &iter_data1->new_results,
                                &iter_data2->new_results) == 0 &&
                        trc_exp_results_cmp(&iter1->exp_results,
                                            &iter2->exp_results) == 0)
                    {
                        iter_data2->rule = rule;
                        iter_data2->rule_id = rule->rule_id;
                    }
                }
            }
        }
    }

    return 0;
}

/**
 * Group test iterations by results.
 *
 * @param db_uid    TRC DB User ID
 * @param test      TRC test
 *
 * @return Count of groups
 */
static int
trc_update_group_test_iters(unsigned int db_uid,
                            trc_test *test)
{
    trc_test_iter               *iter1 = NULL;
    trc_test_iter               *iter2 = NULL;
    trc_update_test_iter_data   *iter_data1 = NULL;
    trc_update_test_iter_data   *iter_data2 = NULL;
    int                          max_id = 0;

    TAILQ_FOREACH(iter1, &test->iters.head, links)
    {
        iter_data1 = trc_db_iter_get_user_data(iter1, db_uid);

        if (iter_data1 == NULL || iter_data1->to_save == FALSE ||
            iter_data1->results_id != 0)
            continue;

        iter_data1->results_id = ++max_id;

        for (iter2 = TAILQ_NEXT(iter1, links); iter2 != NULL;
             iter2 = TAILQ_NEXT(iter2, links))
        {
            iter_data2 = trc_db_iter_get_user_data(
                            iter2, db_uid);

            if (iter_data2 == NULL || iter_data2->results_id != 0 ||
                iter_data2->to_save == FALSE)
                continue;

            if (trc_exp_results_cmp(&iter1->exp_results,
                                    &iter2->exp_results) == 0 &&
                trc_exp_result_cmp((struct trc_exp_result *)
                                            iter1->exp_default,
                                   (struct trc_exp_result *)
                                            iter2->exp_default) == 0)
                iter_data2->results_id = iter_data1->results_id;
        }
    }

    return max_id;
}

/**
 * Get all possible combinations of arguments (not values
 * of arguments) for iterations with a given kind of result.
 *
 * @param db_uid        TRC DB User ID
 * @param test          Test
 * @param results_id    Number of a specific kind of results
 * @param args_groups   Array of arguments groups to be filled
 *
 * @return Status code
 */
static te_errno
trc_update_get_iters_args_combs(unsigned int db_uid,
                                trc_test *test,
                                int results_id,
                                trc_update_args_groups *args_groups)
{
    trc_test_iter               *iter = NULL;
    trc_update_test_iter_data   *iter_data = NULL;
    trc_update_args_group       *args_group = NULL;
    
    TAILQ_FOREACH(iter, &test->iters.head, links)
    {
        iter_data = trc_db_iter_get_user_data(iter, db_uid);

        if (iter_data == NULL)
            continue;

        if (iter_data->results_id == results_id)
        {
            SLIST_FOREACH(args_group, args_groups, links)
            {
                if (test_iter_args_match(args_group->args,
                                         iter_data->args_n,
                                         iter_data->args,
                                         TRUE) != ITER_NO_MATCH)
                    break;
            }

            if (args_group != NULL)
                continue;

            args_group = TE_ALLOC(sizeof(*args_group));
            args_group->args = trc_update_args_wild_dup(&iter->args);

            SLIST_INSERT_HEAD(args_groups, args_group, links);
        }
    }

    return 0;
}

/**
 * Get next possible combination of arguments from a given
 * group of arguments. For example, if we have group of
 * arguments (a, b, c), this function helps us to enumerate
 * all possible combinations: (), (a), (b), (c), (a, b),
 * (a, c), (b, c), (a, b, c).
 *
 * @param args_comb         Current combination of arguments
 * @param args_count        Count of arguments in a group
 * @param cur_comb_count    Count of arguments in current
 *                          combination
 *
 * @return 0 if next combination was generated, -1 if current
 *         combination is the last one.
 */
int
get_next_args_comb(te_bool *args_comb,
                   int args_count,
                   int *cur_comb_count)
{
    int j;
    int k;

    if (*cur_comb_count == args_count)
        return -1;
    else if (*cur_comb_count == 0)
    {
        args_comb[0] = TRUE;
        *cur_comb_count = 1;
    }
    else
    {
        for (j = args_count - 1; j >= args_count - *cur_comb_count; j--)
            if (args_comb[j] == FALSE)
                break;

        if (j < args_count - *cur_comb_count)
        {
            (*cur_comb_count)++;
            for (j = 0; j < args_count; j++)
                if (j < *cur_comb_count)
                    args_comb[j] = TRUE;
                else
                    args_comb[j] = FALSE;
        }
        else
        {
            k = args_count - j;

            while (args_comb[j] == FALSE)
                j--;

            args_comb[j] = FALSE;

            while (k > 0)
            {
                args_comb[++j] = TRUE;
                k--;
            }

            for (j++; j < args_count; j++)
                args_comb[j] = FALSE;
        }
    }

    return 0;
}

/**
 * Generate wildcard having boolean array determining
 * which arguments should be included in wildcard with
 * arguments, and iteration arguments.
 *
 * @param args_comb     Arguments combination
 * @param args_count    Count of arguments
 * @param iter_args     Iteration arguments
 * @param wildcard      Where to place generated wildcard
 *
 * @return Status code
 */
te_errno
args_comb_to_wildcard(te_bool *args_comb,
                      int args_count,
                      trc_report_argument *iter_args,
                      trc_update_args_group *wildcard)
{
    int                  i = 0;
    trc_test_iter_arg   *arg;    

    for (i = 0; i < args_count; i++)
    {
        arg = TE_ALLOC(sizeof(*arg));
        arg->name = strdup(iter_args[i].name);

        if (args_comb[i])
            arg->value = strdup(iter_args[i].value);
        else
        {
            arg->value = TE_ALLOC(1);
            arg->value[0] = '\0';
        }

        TAILQ_INSERT_TAIL(&wildcard->args->head, arg, links);
    }

    return 0;
}

/**
 * Set in_wildcard member of TRC Update iteration data
 * structure attached to iteration in TRC DB for each
 * iteration matching to any wildcard from a given group.
 *
 * @param db_uid            TRC DB User ID
 * @param test              Test
 * @param args_comb         Combination of arguments (can be
 *                          treated as wildcard with all arguments
 *                          without values)
 * @param cur_comb_wilds    Wildcards with a given combination
 *                          of arguments
 *
 * @return Status code
 */
te_errno
mark_iters_in_wildcards(unsigned int db_uid,
                        trc_test *test,
                        trc_update_args_group  *args_group,
                        trc_update_args_groups *cur_comb_wilds)
{
    trc_test_iter               *iter;
    trc_update_test_iter_data   *iter_data;
    trc_update_args_group       *p_group;

    TAILQ_FOREACH(iter, &test->iters.head, links)
    {
        iter_data = trc_db_iter_get_user_data(iter, db_uid);
        if (iter_data == NULL)
            continue;

        if (test_iter_args_match(args_group->args, iter_data->args_n,
                                 iter_data->args, TRUE) != ITER_NO_MATCH)
        {
            SLIST_FOREACH(p_group, cur_comb_wilds, links)
                if (test_iter_args_match(p_group->args,
                                         iter_data->args_n,
                                         iter_data->args,
                                         TRUE) != ITER_NO_MATCH)
                    break;

            if (p_group != NULL)
                iter_data->in_wildcard = TRUE;
        }
    }

    return 0;
}

/**
 * Generate all wildcards for a given group of arguments and
 * a given kind of results. We need considering several group
 * of arguments because some tests have different set of arguments
 * defined for different runs.
 *
 * @param db_uid        TRC DB User ID
 * @param test          Test
 * @param results_id    Number of specific kind of results to
 *                      which wildcards should be generated
 * @param args_group    Group of arguments
 * @param wildcards     Where to place generated wildcards
 *
 * @return Status code
 */
te_errno
trc_update_gen_args_group_wilds(unsigned int db_uid,
                                trc_test *test,
                                int results_id,
                                trc_update_args_group *args_group,
                                trc_update_args_groups *wildcards)
{
    te_bool            *args_in_wild;
    int                 args_count = 0;
    int                 i;
    int                 args_wild_count = 0;

    trc_update_args_groups       cur_comb_wilds;
    trc_update_args_group       *p_group;
    trc_update_args_group       *new_group;
    trc_test_iter_arg           *arg;
    trc_test_iter               *iter1;
    trc_update_test_iter_data   *iter_data1;
    trc_test_iter               *iter2;
    trc_update_test_iter_data   *iter_data2;

    memset(&cur_comb_wilds, 0, sizeof(cur_comb_wilds));
    SLIST_INIT(&cur_comb_wilds);

    TAILQ_FOREACH(arg, &args_group->args->head, links)
        args_count++;

    args_in_wild = TE_ALLOC(sizeof(te_bool) * args_count);
    for (i = 0; i < args_count; i++)
        args_in_wild[i] = FALSE;

    do {
        TAILQ_FOREACH(iter1, &test->iters.head, links)
        {
            iter_data1 = trc_db_iter_get_user_data(iter1, db_uid);
            if (iter_data1 == NULL || iter_data1->in_wildcard)
                continue;

            if (test_iter_args_match(args_group->args, iter_data1->args_n,
                                     iter_data1->args, TRUE) !=
                                                        ITER_NO_MATCH)
            {
                SLIST_FOREACH(p_group, &cur_comb_wilds, links)
                    if (test_iter_args_match(p_group->args,
                                             iter_data1->args_n,
                                             iter_data1->args,
                                             TRUE) != ITER_NO_MATCH)
                        break;

                if (p_group != NULL)
                    continue;
                else if (iter_data1->results_id == results_id)
                {
                    new_group = TE_ALLOC(sizeof(*new_group));
                    new_group->args = TE_ALLOC(sizeof(trc_test_iter_args));
                    TAILQ_INIT(&new_group->args->head);

                    args_comb_to_wildcard(args_in_wild, args_count,
                                          iter_data1->args,
                                          new_group);

                    TAILQ_FOREACH(iter2, &test->iters.head, links)
                    {
                        iter_data2 =
                            trc_db_iter_get_user_data(iter2, db_uid);
                        if (iter_data2 == NULL)
                            continue;

                        if (test_iter_args_match(new_group->args,
                                                 iter_data2->args_n,
                                                 iter_data2->args,
                                                 TRUE) != ITER_NO_MATCH &&
                            (iter_data2->in_wildcard ||
                             iter_data2->results_id != results_id))
                        {
                            break;
                        }
                    }

                    if (iter2 != NULL)
                    {
                        trc_free_test_iter_args(new_group->args);
                        free(new_group);
                        continue;
                    }

                    TAILQ_FOREACH(iter2, &test->iters.head, links)
                    {
                        iter_data2 =
                            trc_db_iter_get_user_data(iter2, db_uid);
                        if (iter_data2 == NULL)
                            continue;

                        if (test_iter_args_match(new_group->args,
                                                 iter_data2->args_n,
                                                 iter_data2->args,
                                                 TRUE) != ITER_NO_MATCH) 
                        {
                            iter_data2->in_wildcard = TRUE;
                        }
                    }

                    new_group->exp_results =
                        trc_exp_results_dup((struct trc_exp_results *)
                                            &iter1->exp_results);
                    new_group->exp_default =
                        trc_exp_result_dup((struct trc_exp_result *)
                                           iter1->exp_default);

                    SLIST_INSERT_HEAD(&cur_comb_wilds, new_group,
                                      links);
                }
            }
        }

        if (!SLIST_EMPTY(&cur_comb_wilds))
        {
            mark_iters_in_wildcards(db_uid, test, args_group,
                                    &cur_comb_wilds);

            while ((p_group =
                        SLIST_FIRST(&cur_comb_wilds)) != NULL)
            {
                SLIST_REMOVE_HEAD(&cur_comb_wilds, links);
                SLIST_INSERT_HEAD(wildcards, p_group, links);
            }
        }
    } while (get_next_args_comb(args_in_wild, args_count,
                                &args_wild_count) == 0);

    return 0;
}

/**
 * Generate wildcards for a given test.
 *
 * @param db_uid        TRC DB User ID
 * @param test          Test
 *
 * @return Status code
 */
te_errno
trc_update_generate_test_wilds(unsigned int db_uid,
                               trc_test *test)
{
    int ids_count;
    int res_id;

    trc_update_args_groups       args_groups;
    trc_update_args_group       *args_group;
    trc_update_args_groups       wildcards;
    trc_test_iter               *iter;
    trc_update_test_iter_data   *iter_data;
    trc_exp_results             *dup_results;
    trc_test_iter_args          *dup_args;
    trc_test_iter_arg           *arg;

    ids_count = trc_update_group_test_iters(db_uid, test);

    memset(&wildcards, 0, sizeof(wildcards));
    SLIST_INIT(&wildcards);

    for (res_id = 1; res_id <= ids_count; res_id++)
    {
        memset(&args_groups, 0, sizeof(args_groups));
        SLIST_INIT(&args_groups);

        trc_update_get_iters_args_combs(db_uid, test, res_id,
                                        &args_groups);
        SLIST_FOREACH(args_group, &args_groups, links)
            trc_update_gen_args_group_wilds(db_uid, test, res_id,
                                            args_group, &wildcards);

        trc_update_args_groups_free(&args_groups);
    }

    /* Delete original iterations - they will be replaced by wildcards */
    iter = TAILQ_FIRST(&test->iters.head);

    if (iter != NULL)
    {
        do {
            TAILQ_REMOVE(&test->iters.head, iter, links);
            iter_data = trc_db_iter_get_user_data(iter, db_uid);
            trc_update_free_test_iter_data(iter_data);
            trc_free_test_iter(iter);
            free(iter);
        } while ((iter = TAILQ_FIRST(&test->iters.head)) != NULL);
    }

    /* Insert generated wildcards in TRC DB */
    SLIST_FOREACH(args_group, &wildcards, links)
    {
        iter = TE_ALLOC(sizeof(*iter));
        iter->exp_default = trc_exp_result_dup(
                                    args_group->exp_default);
        dup_results = trc_exp_results_dup(
                                    args_group->exp_results);
        memcpy(&iter->exp_results, dup_results, sizeof(*dup_results));
        free(dup_results);
        TAILQ_INIT(&iter->args.head);
        dup_args = trc_test_iter_args_dup(args_group->args);
        while ((arg = TAILQ_FIRST(&dup_args->head)) != NULL)
        {
            TAILQ_REMOVE(&dup_args->head, arg, links);
            TAILQ_INSERT_TAIL(&iter->args.head, arg, links);
        }

        free(dup_args);
        iter->parent = test;

        iter_data = TE_ALLOC(sizeof(*iter_data));
        iter_data->to_save = TRUE;
        trc_db_iter_set_user_data(iter, db_uid, iter_data);

        TAILQ_INSERT_TAIL(&test->iters.head, iter, links);
    }

    trc_update_args_groups_free(&wildcards);
    return 0;
}

/**
 * Generate wildcards for all tests to be updated.
 *
 * @param db_uid        TRC DB User ID
 * @param tests         Tests to be updated
 *
 * @return Status code
 */
te_errno
trc_update_generate_wilds(unsigned int db_uid,
                          trc_update_test_entries *tests)
{
    trc_update_test_entry   *test_entry;

    TAILQ_FOREACH(test_entry, tests, links)
        trc_update_generate_test_wilds(db_uid, test_entry->test);

    return 0;
}

/**
 * Callback function that is called when XML parser meets the end of 
 * an element.
 *
 * @param user_data     Pointer to user-specific data (user context)
 * @param name          The element name
 */
static void
trc_log_parse_end_element(void *user_data, const xmlChar *name)
{
    trc_log_parse_ctx   *ctx = user_data;
    const char          *tag = XML2CHAR(name);

    assert(ctx != NULL);
    ENTRY("state=%u rc=%r tag=%s", ctx->state, ctx->rc, tag);

    if (ctx->rc != 0)
        return;

    switch (ctx->state)
    {
        case TRC_LOG_PARSE_SKIP:
            if (--(ctx->skip_depth) == 0)
                ctx->state = ctx->skip_state;
            break;

        case TRC_LOG_PARSE_ROOT:
            if (strcmp(tag, "branch") == 0)
            {
                ctx->state = TRC_LOG_PARSE_TEST;
            }
            else
            {
                assert(strcmp(tag, "proteos:log_report") == 0);
                ctx->state = TRC_LOG_PARSE_INIT;
            }
            break;

        case TRC_LOG_PARSE_LOGS:
            assert(strcmp(tag, "logs") == 0);
            ctx->state = TRC_LOG_PARSE_ROOT;
            break;

        case TRC_LOG_PARSE_TEST:
            if (ctx->iter_data != NULL)
            {
                trc_report_free_test_iter_data(ctx->iter_data);
                ctx->iter_data = NULL;
                ERROR("No meta data for the test entry!");
                ctx->rc = TE_EFMT;
            }

            if (trc_log_parse_stack_pop(ctx))
            {
                /* Step iteration back */
                trc_db_walker_step_back(ctx->db_walker);
                /* Step test entry back */
                trc_db_walker_step_back(ctx->db_walker);
            }
            ctx->state = TRC_LOG_PARSE_ROOT;

            break;

        case TRC_LOG_PARSE_META:
        {
            trc_report_test_iter_data     *iter_data;
            trc_report_test_iter_entry    *entry;
            trc_update_test_iter_data     *upd_iter_data;
            trc_update_test_iter_data     *upd_test_data;
            te_bool                        to_save;
            func_args_match_ptr            func_ptr = NULL;

            entry = TAILQ_FIRST(&ctx->iter_data->runs);
            assert(entry != NULL);
            assert(TAILQ_NEXT(entry, links) == NULL);

            assert(strcmp(tag, "meta") == 0);
            ctx->state = TRC_LOG_PARSE_TEST;

            if (!TAILQ_EMPTY(&trc_db_walker_get_test(ctx->db_walker)->
                                iters.head) &&
                TAILQ_EMPTY(
                    &TAILQ_FIRST(&trc_db_walker_get_test(ctx->db_walker)->
                                        iters.head)->tests.head))
            {
                /*
                 * We use this function only for "non-intermediate"
                 * iterations (i.e. not containing other tests)
                 */
                func_ptr = ctx->func_args_match;
            }

            if (!trc_db_walker_step_iter(ctx->db_walker, entry->args_n,
                                         entry->args,
                                         /* 
                                          * If we merge iterations from log
                                          * into TRC already containing
                                          * all currently possible
                                          * iterations (via previously
                                          * parsed fake log), we do not
                                          * try to add currently
                                          * non-existing iterations
                                          */
                                         (ctx->flags &
                                          TRC_LOG_PARSE_MERGE_LOG) ? FALSE :
                                                                    TRUE,
                                         /*
                                          * We obtain full TRC without
                                          * wildcards in TRC update
                                          * process
                                          */
                                         (ctx->flags &
                                            (TRC_LOG_PARSE_FAKE_LOG |
                                             TRC_LOG_PARSE_MERGE_LOG |
                                             TRC_LOG_PARSE_LOG_WILDS)) ?
                                                TRUE : FALSE,
                                         /* 
                                          * Currently not used - 
                                          * split results with complex
                                          * tag expressions into
                                          * results with simple ones
                                          * obtaining "raw" TRC for
                                          * TRC update parsing
                                          * fake log
                                          */
                                         FALSE,
                                         ctx->db_uid,
                                         func_ptr))
            {
                if (!(ctx->flags & TRC_LOG_PARSE_MERGE_LOG))
                {
                    ERROR("Unable to create a new iteration");
                    ctx->rc = TE_ENOMEM;
                }
                else
                {
                    trc_report_free_test_iter_data(ctx->iter_data);
                    ctx->iter_data = NULL;
                }
                break;
            }

            if (!(ctx->flags & (TRC_LOG_PARSE_FAKE_LOG |
                                TRC_LOG_PARSE_MERGE_LOG |
                                TRC_LOG_PARSE_LOG_WILDS)))
            {
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
                                &TAILQ_FIRST(&ctx->iter_data->runs)->
                                                                result);
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
                                &TAILQ_FIRST(&ctx->iter_data->runs)->
                                                                  result);
                    /* Merge a new entry */
                    trc_report_merge_test_iter_data(iter_data,
                                                    ctx->iter_data);
                }
                ctx->iter_data = NULL;
            }
            else
            {
                upd_iter_data = trc_db_walker_get_user_data(ctx->db_walker,
                                                            ctx->db_uid);

                upd_test_data = trc_db_walker_get_parent_user_data(
                                      ctx->db_walker, ctx->db_uid);
     
                if (upd_iter_data == NULL)
                {
                    to_save = TRUE;

                    /* Attach iteration data to TRC database */
                    if (upd_test_data != NULL)
                        ctx->rc = trc_db_walker_set_user_data(
                                        ctx->db_walker,
                                        ctx->db_uid,
                                        trc_update_gen_user_data(&to_save,
                                                                 TRUE));
                    else
                        ctx->rc = trc_db_walker_set_prop_ud(ctx->db_walker,
                                            ctx->db_uid, &to_save,
                                            &trc_update_gen_user_data);

                    if (ctx->rc != 0)
                        break;

                    upd_iter_data = trc_db_walker_get_user_data(
                                        ctx->db_walker,
                                        ctx->db_uid);

                    assert(upd_iter_data != NULL);

                    /*
                     * Now we assume that arguments and
                     * test results are specified
                     * only in leaves of TRC tree
                     */
                    if (entry->args_n > 0 && upd_test_data == NULL)
                    {
                        trc_update_test_entry   *test_p;
                        trc_test                *test;
                        
                        test = trc_db_walker_get_test(ctx->db_walker);

                        TAILQ_FOREACH(test_p, ctx->updated_tests,
                                      links)
                            if (test_p->test == test)
                                break;

                        assert(test_p == NULL);
                        test_p = TE_ALLOC(sizeof(*test_p));
                        test_p->test = test;

                        TAILQ_INSERT_TAIL(ctx->updated_tests,
                                          test_p, links);
                    }

                    /*
                     * Arguments here are sorted - qsort() was
                     * called inside trc_db_walker_step_iter()
                     */
                    upd_iter_data->args = entry->args;
                    upd_iter_data->args_n = entry->args_n;
                    upd_iter_data->args_max = entry->args_max;

                    entry->args = NULL;
                    entry->args_n = 0;
                    entry->args_max = 0;
                }
                else
                {
                    /* Duplicate iterations in package.xml?! */
                    assert(!(ctx->flags & TRC_LOG_PARSE_FAKE_LOG));
                }

                if (ctx->flags & (TRC_LOG_PARSE_MERGE_LOG |
                                  TRC_LOG_PARSE_LOG_WILDS))
                    trc_update_merge_result(ctx);

                trc_report_free_test_iter_data(ctx->iter_data);
                ctx->iter_data = NULL;
            }

            break;
        }

        case TRC_LOG_PARSE_OBJECTIVE:
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
            ctx->state = TRC_LOG_PARSE_META;
            break;

        case TRC_LOG_PARSE_PARAMS:
            assert(strcmp(tag, "params") == 0);
            ctx->state = TRC_LOG_PARSE_META;
            break;

        case TRC_LOG_PARSE_VERDICTS:
            assert(strcmp(tag, "verdicts") == 0);
            ctx->state = TRC_LOG_PARSE_META;
            break;

        case TRC_LOG_PARSE_VERDICT:
            if (strcmp(tag, "br") == 0)
                break;
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
            ctx->state = TRC_LOG_PARSE_VERDICTS;
            break;

        case TRC_LOG_PARSE_TAGS:
            assert(strcmp(tag, "msg") == 0);
            trc_tags_str_to_list(ctx->tags, ctx->str);
            free(ctx->str);
            ctx->str = NULL;
            ctx->state = TRC_LOG_PARSE_LOGS;
            break;

        default:
            ERROR("%s(): Unexpected state %u", __FUNCTION__, ctx->state);
            assert(FALSE);
            break;
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
trc_log_parse_problem(void *user_data, const char *msg, ...)
{
    va_list ap;

    UNUSED(user_data);
    
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
}

/**
 * The structure specifies all types callback routines that
 * should be called.
 */
static xmlSAXHandler sax_handler = {
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
    .startDocument          = trc_log_parse_start_document,
    .endDocument            = trc_log_parse_end_document,
    .startElement           = trc_log_parse_start_element,
    .endElement             = trc_log_parse_end_element,
    .reference              = NULL,
    .characters             = trc_log_parse_characters,
    .ignorableWhitespace    = NULL,
    .processingInstruction  = NULL,
    .comment                = NULL,
    .warning                = trc_log_parse_problem,
    .error                  = trc_log_parse_problem,
    .fatalError             = trc_log_parse_problem,
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

/* See the description in trc_report.h */
te_errno
trc_report_process_log(trc_report_ctx *gctx, const char *log)
{
    te_errno             rc = 0;
    trc_log_parse_ctx    ctx;

    memset(&ctx, 0, sizeof(ctx));
    ctx.flags = gctx->parsing_flags;
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
#if 0
    else if ((rc = trc_report_collect_stats(gctx)) != 0)
    {
        ERROR("Collect of TRC report statistics failed: %r", rc);
    }
#endif
    free(ctx.stack_info);

    return rc;
}

/**
 * Initialize TRC Log Parse context for TRC Update
 * Tool.
 *
 * @param ctx       TRC Log Parse context to be initialized
 * @param gctx      Context of TRC Update Tool
 *
 * @return Status code
 */
static te_errno
trc_update_init_parse_ctx(trc_log_parse_ctx *ctx,
                          trc_update_ctx *gctx)
{
    trc_update_test_entries     *updated_tests;

    updated_tests = ctx->updated_tests;
    memset(ctx, 0, sizeof(*ctx));
    ctx->updated_tests = updated_tests;

    ctx->db = gctx->db;
    ctx->db_uid = gctx->db_uid;

    ctx->test_paths = &gctx->test_names;
    ctx->tags = TE_ALLOC(sizeof(*(ctx->tags)));
    TAILQ_INIT(ctx->tags);

    ctx->func_args_match = gctx->func_args_match;

    return 0;
}

/* See the description in trc_update.h */
te_errno
trc_update_process_logs(trc_update_ctx *gctx)
{
    te_errno                    rc = 0;
    trc_log_parse_ctx           ctx;
    trc_update_tag_logs        *tl = NULL;
    tqe_string                 *tqe_str = NULL;

    trc_update_init_parse_ctx(&ctx, gctx);
    ctx.log = gctx->fake_log;
    ctx.flags = TRC_LOG_PARSE_FAKE_LOG;
    tl = TAILQ_FIRST(&gctx->tags_logs);
    if (tl != NULL)
        tqe_str = TAILQ_FIRST(&tl->logs);

    if (gctx->flags & TRC_LOG_PARSE_LOG_WILDS)
    {
        if (ctx.log == NULL && tqe_str != NULL)
        {
            ctx.merge_expr = tl->tags_expr;
            ctx.log = tqe_str->v;
            tqe_str = TAILQ_NEXT(tqe_str, links);
            ctx.flags = gctx->flags;
        }
    }

    ctx.updated_tests = TE_ALLOC(sizeof(trc_update_test_entries));
    TAILQ_INIT(ctx.updated_tests);

    do {
        if (ctx.log == NULL)
            break;

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

        free(ctx.stack_info);
    
        if (tl == NULL)
            break;

        free(ctx.tags);
        trc_update_init_parse_ctx(&ctx, gctx);
        ctx.func_args_match = NULL;

        if (tqe_str != NULL)
        {
            ctx.merge_expr = tl->tags_expr;
            ctx.log = tqe_str->v;
            tqe_str = TAILQ_NEXT(tqe_str, links);
        }
        else
        {
            tl = TAILQ_NEXT(tl, links);

            if (tl == NULL)
                break;
            else
                tqe_str = TAILQ_FIRST(&tl->logs);

            ctx.log = tqe_str->v;
            ctx.merge_expr = tl->tags_expr;
            tqe_str = TAILQ_NEXT(tqe_str, links);
        }

        ctx.flags = gctx->flags;
        if (!(gctx->flags & TRC_LOG_PARSE_LOG_WILDS))
            ctx.flags |= TRC_LOG_PARSE_MERGE_LOG;

    } while (1);

    if (gctx->fake_log == NULL &&
        !(gctx->flags & TRC_LOG_PARSE_LOG_WILDS))
        trc_update_fill_db_user_data(gctx->db, ctx.updated_tests,
                                     gctx->db_uid);

    trc_update_simplify_results(gctx->db_uid, ctx.updated_tests,
                                gctx->flags);

    if (gctx->rules_load_from != NULL)
    {
        trc_update_clear_rules(ctx.db_uid, ctx.updated_tests);

        trc_update_load_rules(gctx->rules_load_from,
                              ctx.updated_tests,
                              gctx->flags);
        
        trc_update_apply_rules(gctx->db_uid, ctx.updated_tests,
                               gctx->flags);
    }

    if (gctx->rules_save_to != NULL)
    {
        if (gctx->rules_load_from == NULL)
            trc_update_gen_rules(ctx.db_uid, ctx.updated_tests,
                                 gctx->flags);

        save_test_rules_to_file(ctx.updated_tests, gctx->rules_save_to,
                                gctx->cmd);
    }

    if ((!(gctx->rules_load_from == NULL &&
           gctx->rules_save_to != NULL)) &&
        !(gctx->flags & TRC_LOG_PARSE_NO_GEN_WILDS))
        trc_update_generate_wilds(gctx->db_uid,
                                  ctx.updated_tests);

    trc_update_clear_rules(ctx.db_uid, ctx.updated_tests);
    trc_update_test_entries_free(ctx.updated_tests);
    return rc;
}

/* See the description in trc_report.h */
te_errno
trc_diff_process_logs(trc_diff_ctx *gctx)
{
    te_errno                  rc = 0;
    trc_log_parse_ctx         ctx;
    trc_diff_set             *diff_set;

    memset(&ctx, 0, sizeof(ctx));
    ctx.flags = gctx->flags;
    ctx.db = gctx->db;

    TAILQ_FOREACH(diff_set, &gctx->sets, links)
    {
        if (diff_set->log == NULL)
            continue;

        ctx.run_name = diff_set->name;
        ctx.log      = diff_set->log;
        ctx.db_uid   = diff_set->db_uid;
        ctx.tags     = &diff_set->tags;

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
#if 0
        else if ((rc = trc_report_collect_stats(gctx)) != 0)
        {
            ERROR("Collect of TRC report statistics failed: %r", rc);
        }

        free(ctx.stack_info);
        for (ctx.args_n = 0; ctx.args_n < ctx.args_max; ctx.args_n++)
        {
            free(ctx.args[ctx.args_n].name);
            free(ctx.args[ctx.args_n].value);
        }
        free(ctx.args);
#endif
    }

    return rc;
}
