/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Testing Results Comparator
 *
 * Parser of TE log in XML format.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Log Parser"

#include "te_config.h"
#include "trc_config.h"

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
#include "gen_wilds.h"
#include <time.h>
#include <sys/time.h>

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

/* See description in log_parse.h */
te_errno
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
    int                      tin = -1;
    int                      test_id = -1;
    char                    *hash = NULL;
    te_bool                  name_found = FALSE;
    te_bool                  status_found = FALSE;
    te_test_status           status = TE_TEST_UNSPEC;
    char                    *s = NULL;
    trc_test                *test = NULL;
    trc_update_tests_group  *group;
    trc_update_test_entry   *test_p;

    while (ctx->rc == 0 && attrs[0] != NULL && attrs[1] != NULL)
    {
        /*
         * Session name is ignored because currently Tester ignores
         * name attribute of <session> tag in package.xml, instead
         * using name attribute of parent <run> tag. And it does
         * not match this name to TRC database, but only allows to
         * use it when specifying test path to be executed.
         * For consistency the same should be done here if session
         * name is printed to RAW log.
         */
        if (ctx->type != TRC_TEST_SESSION &&
            xmlStrcmp(attrs[0], CONST_CHAR2XML("name")) == 0)
        {
            name_found = TRUE;
            if (!trc_db_walker_step_test(ctx->db_walker,
                                         XML2CHAR(attrs[1]), TRUE))
            {
                ERROR("Unable to create a new test entry %s",
                      attrs[1]);
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
        else if (xmlStrcmp(attrs[0], CONST_CHAR2XML("test_id")) == 0)
        {
            if (sscanf(XML2CHAR(attrs[1]), "%d", &test_id) < 1)
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

        if (ctx->app_id == TRC_LOG_PARSE_APP_UPDATE)
        {
            trc_update_ctx *app_ctx = (trc_update_ctx *)ctx->app_ctx;

            if (!TAILQ_EMPTY(&app_ctx->test_names))
            {
                /* Determining whether a given test is in one
                 * of paths to tests to be updated */
                TAILQ_FOREACH(tqe_str, &app_ctx->test_names, links)
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
            }

            if ((tqe_str == NULL &&
                 !TAILQ_EMPTY(&app_ctx->test_names)) ||
                ((strcmp(test->name, "prologue") == 0 ||
                  strcmp(test->name, "epilogue") == 0 ||
                  strcmp(test->name, "loop_prologue") == 0 ||
                  strcmp(test->name, "loop_epilogue") == 0) &&
                 (app_ctx->flags & TRC_UPDATE_NO_PE)))
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
        entry->test_id = test_id;
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

        if (ctx->app_id == TRC_LOG_PARSE_APP_UPDATE)
        {
            trc_update_ctx *app_ctx = (trc_update_ctx *)ctx->app_ctx;

            if (app_ctx->flags & (TRC_UPDATE_FAKE_LOG |
                                  TRC_UPDATE_LOG_WILDS |
                                  TRC_UPDATE_PRINT_PATHS))
            {
                if (test->type == TRC_TEST_SCRIPT)
                {
                    TAILQ_FOREACH(group, &app_ctx->updated_tests,
                                  links)
                        if (strcmp(group->path, test->path) == 0)
                            break;

                    if (group == NULL)
                    {
                        group = TE_ALLOC(sizeof(*group));
                        group->rules = NULL;
                        group->path = strdup(test->path);
                        TAILQ_INIT(&group->tests);
                        TAILQ_INSERT_TAIL(&app_ctx->updated_tests,
                                          group, links);
                    }

                    if (!(app_ctx->flags & TRC_UPDATE_PRINT_PATHS))
                    {
                        TAILQ_FOREACH(test_p, &group->tests,
                                      links)
                            if (test_p->test == test)
                                break;

                        if (test_p == NULL)
                        {
                            test_p = TE_ALLOC(sizeof(*test_p));
                            test_p->test = test;

                            TAILQ_INSERT_TAIL(&group->tests,
                                              test_p, links);
                        }
                    }
                }
            }
        }
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

    if (exp_result == NULL ||
        exp_result == exp_defaults_get(TE_TEST_UNSPEC))
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
        case TRC_LOG_PARSE_ARTIFACT:
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
                if (ctx->flags & TRC_LOG_PARSE_IGNORE_LOG_TAGS)
                {
                    /* Ignore logs inside tests */
                    ctx->skip_state = ctx->state;
                    ctx->skip_depth = 1;
                    ctx->state = TRC_LOG_PARSE_SKIP;
                }
                else
                {
                    ctx->log_parent_state = TRC_LOG_PARSE_ROOT;
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
                if (ctx->flags & TRC_LOG_PARSE_IGNORE_LOG_TAGS)
                {
                    /* Ignore logs inside tests */
                    ctx->skip_state = ctx->state;
                    ctx->skip_depth = 1;
                    ctx->state = TRC_LOG_PARSE_SKIP;
                }
                else
                {
                    ctx->log_parent_state = TRC_LOG_PARSE_TEST;
                    ctx->state = TRC_LOG_PARSE_LOGS;
                }
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
            else if (strcmp(tag, "artifacts") == 0)
            {
                ctx->state = TRC_LOG_PARSE_ARTIFACTS;
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

        case TRC_LOG_PARSE_ARTIFACTS:
            if (strcmp(tag, "artifact") == 0)
            {
                ctx->state = TRC_LOG_PARSE_ARTIFACT;
                assert(ctx->str == NULL);
            }
            else
            {
                ERROR("Unexpected element '%s' in 'artifacts'", tag);
                ctx->rc = TE_EFMT;
            }
            break;

        case TRC_LOG_PARSE_VERDICT:
        case TRC_LOG_PARSE_ARTIFACT:
        case TRC_LOG_PARSE_OBJECTIVE:
            if (strcmp(tag, "br") == 0)
            {
                trc_log_parse_characters(user_data,
                                         CONST_CHAR2XML("\n"),
                                         strlen("\n"));
            }
            else
            {
                ERROR("Unexpected element '%s' in 'verdict', 'artifact' or "
                      "'objective'", tag);
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
                        if (xmlStrcmp(attrs[1], CONST_CHAR2XML("Tester")) != 0)
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
            ctx->state = ctx->log_parent_state;
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
                if (trc_db_walker_is_iter(ctx->db_walker))
                {
                    /* Step iteration back */
                    trc_db_walker_step_back(ctx->db_walker);
                }

                /* Step test entry back */
                trc_db_walker_step_back(ctx->db_walker);
            }
            ctx->state = TRC_LOG_PARSE_ROOT;

            break;

        case TRC_LOG_PARSE_META:
        {
            trc_report_test_iter_data     *iter_data;
            trc_report_test_iter_entry    *entry;
            func_args_match_ptr            func_ptr = NULL;
            trc_test                      *test;
            trc_test_iter                 *iter;

            uint32_t  step_iter_flags = STEP_ITER_CREATE_NFOUND;

            test = trc_db_walker_get_test(ctx->db_walker);
            entry = TAILQ_FIRST(&ctx->iter_data->runs);
            assert(entry != NULL);
            assert(TAILQ_NEXT(entry, links) == NULL);

            assert(strcmp(tag, "meta") == 0);
            ctx->state = TRC_LOG_PARSE_TEST;

            if (ctx->app_id == TRC_LOG_PARSE_APP_UPDATE)
            {
                trc_update_ctx *app_ctx = (trc_update_ctx *)ctx->app_ctx;

               /*
                * If we merge iterations from log
                * into TRC already containing
                * all currently possible
                * iterations (via previously
                * parsed fake log), we do not
                * try to add currently
                * non-existing iterations
                */

                if (app_ctx->flags & TRC_UPDATE_MERGE_LOG)
                    step_iter_flags &= ~STEP_ITER_CREATE_NFOUND;

                if ((app_ctx->flags & (TRC_UPDATE_FAKE_LOG |
                                       TRC_UPDATE_MERGE_LOG |
                                       TRC_UPDATE_LOG_WILDS)) &&
                    test->type == TRC_TEST_SCRIPT)
                {
                   /*
                    * We obtain full TRC without
                    * wildcards for every
                    * script in TRC update
                    * process
                    */
                    step_iter_flags |= STEP_ITER_NO_MATCH_WILD |
                                       STEP_ITER_NO_MATCH_OLD;
                }

                if (test->type == TRC_TEST_SCRIPT &&
                    (app_ctx->flags & TRC_UPDATE_FAKE_LOG) &&
                    !(app_ctx->flags & TRC_UPDATE_PRINT_PATHS))
                    func_ptr = app_ctx->func_args_match;
            }
            else
            {
                /*
                 * If this is not TRC Update tool, for unknown iterations
                 * added to DB default expected result should be
                 * TE_TEST_UNSPEC. This allows to detect such
                 * iterations as new (not known to existing TRC DB) later.
                 */
                step_iter_flags |= STEP_ITER_CREATE_UNSPEC;
            }

            if (!trc_db_walker_step_iter(ctx->db_walker, entry->args_n,
                                         entry->args,
                                         step_iter_flags,
                                         ctx->db_uid,
                                         func_ptr))
            {
                if (ctx->app_id == TRC_LOG_PARSE_APP_UPDATE &&
                    (((trc_update_ctx *)ctx->app_ctx)->flags &
                                                  TRC_UPDATE_MERGE_LOG))
                {
                    trc_report_free_test_iter_data(ctx->iter_data);
                    ctx->iter_data = NULL;
                }
                else
                {
                    ERROR("Unable to create a new iteration");
                    ctx->rc = TE_ENOMEM;
                }
                break;
            }

            iter = trc_db_walker_get_iter(ctx->db_walker);

            if (ctx->app_id == TRC_LOG_PARSE_APP_UPDATE)
            {
                ctx->rc = trc_update_process_iter(
                                        (trc_update_ctx *)ctx->app_ctx,
                                        ctx->db_walker, iter, entry);

                trc_report_free_test_iter_data(ctx->iter_data);
                ctx->iter_data = NULL;
            }
            else
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

            break;
        }

        case TRC_LOG_PARSE_OBJECTIVE:
            /* Handle multi-line objectives */
            if (strcmp(tag, "br") == 0)
                break;
            assert(strcmp(tag, "objective") == 0);
            if (ctx->str != NULL)
            {
                trc_test   *test = NULL;

                test = trc_db_walker_get_test(ctx->db_walker);
                assert(test != NULL);
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

        case TRC_LOG_PARSE_ARTIFACTS:
            assert(strcmp(tag, "artifacts") == 0);
            ctx->state = TRC_LOG_PARSE_META;
            break;

        case TRC_LOG_PARSE_VERDICT:
        case TRC_LOG_PARSE_ARTIFACT:
            if (strcmp(tag, "br") == 0)
                break;

            if (ctx->state == TRC_LOG_PARSE_VERDICT)
                assert(strcmp(tag, "verdict") == 0);
            else
                assert(strcmp(tag, "artifact") == 0);

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

                if (ctx->state == TRC_LOG_PARSE_VERDICT)
                {
                    TAILQ_INSERT_TAIL(
                      &TAILQ_FIRST(&ctx->iter_data->runs)->result.verdicts,
                      verdict, links);
                }
                else
                {
                    TAILQ_INSERT_TAIL(
                      &TAILQ_FIRST(&ctx->iter_data->runs)->result.artifacts,
                      verdict, links);
                }
            }

            if (ctx->state == TRC_LOG_PARSE_VERDICT)
                ctx->state = TRC_LOG_PARSE_VERDICTS;
            else
                ctx->state = TRC_LOG_PARSE_ARTIFACTS;

            break;

        case TRC_LOG_PARSE_TAGS:
            assert(strcmp(tag, "msg") == 0);
            if (trc_tags_json_to_list(ctx->tags, ctx->str) != 0)
                ERROR("TRC tags parse failure");
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

te_errno
trc_log_parse_process_log(trc_log_parse_ctx *ctx)
{
    te_errno rc;

    if (xmlSAXUserParseFile(&sax_handler, ctx, ctx->log) != 0)
    {
        ERROR("Cannot parse XML document with TE log '%s'", ctx->log);
        rc = TE_EFMT;
    }
    else if ((rc = ctx->rc) != 0)
    {
        ERROR("Processing of the XML document with TE log '%s' "
              "failed: %r", ctx->log, rc);
    }

    return rc;
}

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

    rc = trc_log_parse_process_log(&ctx);

#if 0
    else if ((rc = trc_report_collect_stats(gctx)) != 0)
    {
        ERROR("Collect of TRC report statistics failed: %r", rc);
    }
#endif
    free(ctx.stack_info);

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
    ctx.flags = 0;
    ctx.db = gctx->db;

    TAILQ_FOREACH(diff_set, &gctx->sets, links)
    {
        if (diff_set->log == NULL)
            continue;

        ctx.run_name = diff_set->name;
        ctx.log      = diff_set->log;
        ctx.db_uid   = diff_set->db_uid;
        ctx.tags     = &diff_set->tags;

        rc = trc_log_parse_process_log(&ctx);

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
