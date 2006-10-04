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
    tqh_strings         tags;       /**< List of tags */

    te_trc_db_walker   *db_walker;  /**< TRC database walker */

    trc_report_log_parse_state  state;  /**< Log parse state */

    unsigned int                skip_depth; /**< Skip depth */
    trc_report_log_parse_state  skip_state; /**< State to return */

    trc_report_test_iter_data  *iter_data;  /**< Current test iteration
                                                 data */

    unsigned int        args_max;   /**< Maximum number of arguments
                                         the space is allocated for */
    unsigned int        args_n;     /**< Current number of arguments */
    char              **args_name;   /**< Names of arguments */
    char              **args_value;  /**< Values of arguments */

} trc_report_log_parse_ctx;


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

    tq_strings_free(&ctx->tags, free);
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
    trc_report_test_iter_entry *entry;

    te_bool name_found = FALSE;
    te_bool status_found = FALSE;

    /* Pre-allocate entry for result */
    entry = TE_ALLOC(sizeof(*entry));
    if (entry == NULL)
    {
        ctx->rc = TE_ENOMEM;
        return;
    }
    te_test_result_init(&entry->result);

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

    while (ctx->rc == 0 && attrs[0] != NULL && attrs[1] != NULL)
    {
        if (strcmp(attrs[0], "name") == 0)
        {
            name_found = TRUE;
            if (!trc_db_walker_step_test(ctx->db_walker, attrs[1], TRUE))
            {
                ERROR("Unable to create a new test entry");
                ctx->rc = TE_ENOMEM;
            }
        }
        else if (strcmp(attrs[0], "result") == 0)
        {
            status_found = TRUE;
            ctx->rc = te_test_str2status(attrs[1],
                          &ctx->iter_data->runs.tqh_first->result.status);
        }
        attrs += 2;
    }

    if (ctx->rc != 0)
    {
        /* We already have error */
    }
    else if (!name_found)
    {
        ERROR("Name of the test/package/session not found");
        ctx->rc = TE_EFMT;
    }
    else if (!status_found)
    {
        ERROR("Status of the test/package/session not found");
        ctx->rc = TE_EFMT;
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
        ctx->args_name = realloc(ctx->args_name, ctx->args_max);
        ctx->args_value = realloc(ctx->args_value, ctx->args_max);
        if (ctx->args_name == NULL || ctx->args_value == NULL)
        {
            ctx->rc = TE_ENOMEM;
            return;
        }
        ctx->args_name[ctx->args_n] = ctx->args_value[ctx->args_n] = NULL;
    }

    while (attrs[0] != NULL && attrs[1] != NULL)
    {
        if (strcmp(attrs[0], "name") == 0)
        {
            assert(ctx->args_name[ctx->args_n] == NULL);
            ctx->args_name[ctx->args_n] = strdup(attrs[1]);
            if (ctx->args_name[ctx->args_n] == NULL)
            {
                ERROR("strdup(%s) failed", attrs[1]);
                ctx->rc = TE_ENOMEM;
                return;
            }
        }
        else if (strcmp(attrs[0], "value") == 0)
        {
            assert(ctx->args_value[ctx->args_n] == NULL);
            ctx->args_value[ctx->args_n] = strdup(attrs[1]);
            if (ctx->args_value[ctx->args_n] == NULL)
            {
                ERROR("strdup(%s) failed", attrs[1]);
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
        is_expected = trc_is_result_expected(exp_result, result);

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

    entry = more->runs.tqh_first;
    assert(entry != NULL);
    assert(entry->links.tqe_next == NULL);

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
                             const xmlChar *tag, const xmlChar **attrs)
{
    trc_report_log_parse_ctx   *ctx = user_data;

    assert(ctx != NULL);
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
                    ctx->state = TRC_REPORT_LOG_PARSE_SKIP;
                    ctx->skip_state = ctx->state;
                    ctx->skip_depth = 1;
                }
                else
                {
                    ctx->state = TRC_REPORT_LOG_PARSE_LOGS;
                }
            }
            else if (strcmp(tag, "test") == 0 ||
                     strcmp(tag, "pkg") == 0 ||
                     strcmp(tag, "session") == 0)
            {
                trc_report_test_entry(ctx, attrs);
                /* FIXME: Set type for a new entries */
                ctx->state = TRC_REPORT_LOG_PARSE_TEST;
            }
            else
            {
                ERROR("Unexpected element '%s' in the root state", tag);
                ctx->rc = TE_EFMT;
            }
            break;

        case TRC_REPORT_LOG_PARSE_TEST:
            if (strcmp(tag, "meta") == 0)
            {
                ctx->state = TRC_REPORT_LOG_PARSE_META;
            }
            else if (strcmp(tag, "branch") == 0)
            {
                ctx->state = TRC_REPORT_LOG_PARSE_ROOT;
            }
            else if (strcmp(tag, "logs") == 0)
            {
                /* Ignore logs inside tests */
                ctx->state = TRC_REPORT_LOG_PARSE_SKIP;
                ctx->skip_state = ctx->state;
                ctx->skip_depth = 1;
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
                if (ctx->flags & TRC_REPORT_UPDATE_DB)
                {
                    ctx->state = TRC_REPORT_LOG_PARSE_OBJECTIVE;
                }
                else
                {
                    ctx->state = TRC_REPORT_LOG_PARSE_SKIP;
                    ctx->skip_state = ctx->state;
                    ctx->skip_depth = 1;
                }
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
                ctx->state = TRC_REPORT_LOG_PARSE_SKIP;
                ctx->skip_state = ctx->state;
                ctx->skip_depth = 1;
            }
            break;
            
        case TRC_REPORT_LOG_PARSE_VERDICTS:
            if (strcmp(tag, "verdict") == 0)
            {
                ctx->state = TRC_REPORT_LOG_PARSE_VERDICT;
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
                }
                else
                {
                    ctx->state = TRC_REPORT_LOG_PARSE_SKIP;
                    ctx->skip_state = ctx->state;
                    ctx->skip_depth = 1;
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
    if (ctx->rc != 0)
        return;

    switch (ctx->state)
    {
        case TRC_REPORT_LOG_PARSE_SKIP:
            if (--(ctx->skip_depth) == 0)
                ctx->state = ctx->skip_state;
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
            /* Step iteration back */
            trc_db_walker_step_back(ctx->db_walker);
            /* Step test entry back */
            trc_db_walker_step_back(ctx->db_walker);
            break;

        case TRC_REPORT_LOG_PARSE_META:
        {
            trc_report_test_iter_data  *iter_data;

            assert(strcmp(tag, "meta") == 0);
            ctx->state = TRC_REPORT_LOG_PARSE_TEST;
            if (!trc_db_walker_step_iter(ctx->db_walker, ctx->args_n,
                                         (const char **)ctx->args_name,
                                         (const char **)ctx->args_value,
                                         TRUE))
            {
                ERROR("Unable to create a new iteration");
                ctx->rc = TE_ENOMEM;
                break;
            }
            ctx->args_n = 0;
            iter_data = trc_db_get_user_data(ctx->db_walker, ctx->db_uid);
            if (iter_data == NULL)
            {
                /* Get expected result */
                ctx->iter_data->exp_result =
                    trc_db_walker_get_exp_result(ctx->db_walker,
                                                 &ctx->tags);
                /* Update statistics */
                trc_report_test_iter_stats_update(&ctx->iter_data->stats,
                    ctx->iter_data->exp_result,
                    &ctx->iter_data->runs.tqh_first->result);
                /* Attach iteration data to TRC database */
                ctx->rc = trc_db_set_user_data(ctx->db_walker, ctx->db_uid,
                                               ctx->iter_data);
                if (ctx->rc != 0)
                    break;
            }
            else
            {
                /* Update statistics */
                trc_report_test_iter_stats_update(&iter_data->stats,
                    iter_data->exp_result,
                    &ctx->iter_data->runs.tqh_first->result);
                /* Merge a new entry */
                trc_report_merge_test_iter_data(iter_data,
                                                ctx->iter_data);
            }
            ctx->iter_data = NULL;
            break;
        }

        case TRC_REPORT_LOG_PARSE_OBJECTIVE:
            assert(strcmp(tag, "objective") == 0);
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
            ctx->state = TRC_REPORT_LOG_PARSE_VERDICTS;
            break;

        case TRC_REPORT_LOG_PARSE_TAGS:
            assert(strcmp(tag, "msg") == 0);
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
    char                      **location;
    char                       *tags_str = NULL;

    trc_test   *test = NULL;
    char       *save = NULL;

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
        case TRC_REPORT_LOG_PARSE_OBJECTIVE:
            test = trc_db_walker_get_test(ctx->db_walker);
            assert(test != NULL);
            save = test->objective;
            test->objective = NULL;
            location = &test->objective;
            break;

        case TRC_REPORT_LOG_PARSE_VERDICT:
        {
            te_test_verdict *verdict = TE_ALLOC(sizeof(*verdict));

            if (verdict == NULL)
            {
                ERROR("Memory allocation failure");
                ctx->rc = TE_ENOMEM;
                return;
            }
            assert(ctx->iter_data != NULL);
            TAILQ_INSERT_TAIL(
                &ctx->iter_data->runs.tqh_first->result.verdicts,
                verdict, links);
            location = &verdict->str;
            break;
        }

        case TRC_REPORT_LOG_PARSE_TAGS:
            location = &tags_str;
            break;

        default:
            /* Just ignore */
            return;
    }

    assert(*location == NULL);
    *location = malloc(len + 1);
    if (*location == NULL)
    {
        ERROR("Memory allocation failure");
        ctx->rc = TE_ENOMEM;
        if (save != NULL)
        {
            assert(test != NULL);
            test->objective = save;
        }
    }
    else
    {
        memcpy(*location, ch, len);
        (*location)[len] = '\0';
        if (ctx->state == TRC_REPORT_LOG_PARSE_TAGS)
        {
            trc_tags_str_to_list(&ctx->tags, tags_str);
        }
        else if (save != NULL && strcpy(save, *location) != 0)
        {
            /* TODO: Set flag to update test objective in DB */
        }
        free(save);
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
    ctx.log = log;
    TAILQ_INIT(&ctx.tags);

    if (xmlSAXUserParseFile(&sax_handler, &ctx, ctx.log) != 0)
    {
        ERROR("Cannot parse XML document with TE log");
        rc = TE_EFMT;
    }
    else if ((rc = ctx.rc) != 0)
    {
        ERROR("Processing of the XML document with TE log '%s' "
              "failed: %r", ctx.log, rc);
    }

    return rc;
}
