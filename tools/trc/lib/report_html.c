/** @file
 * @brief Testing Results Comparator
 *
 * Generator of comparison report in HTML format.
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

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_defs.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "trc_db.h"
#include "trc_tags.h"
#include "trc_html.h"
#include "trc_report.h"
#include "re_subst.h"
#include "te_shell_cmd.h"

/** Define to 1 to use spoilers to show/hide test parameters */
#define TRC_USE_PARAMS_SPOILERS 0

#define WRITE_STR(str) \
    do {                                                \
        fflush(f);                                      \
        if (fwrite(str, strlen(str), 1, f) != 1)        \
        {                                               \
            rc = te_rc_os2te(errno) ? : TE_EIO;         \
            ERROR("Writing to the file failed");        \
            goto cleanup;                               \
        }                                               \
    } while (0)

#define PRINT_STR(str_)  (((str_) != NULL) ? (str_) : "")


static const char * const trc_html_title_def =
    "Testing Results Comparison Report";

static const char * const trc_html_doc_start =
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n"
"<html>\n"
"<head>\n"
"  <meta http-equiv=\"content-type\" content=\"text/html; "
"charset=utf-8\">\n"
"  <title>%s</title>\n"
"  <style type=\"text/css\">\n"
"    .A {padding-left: 0.14in; padding-right: 0.14in}\n"
"    .B {padding-left: 0.24in; padding-right: 0.04in}\n"
"    .C {text-align: right; padding-left: 0.14in; padding-right: 0.14in}\n"
"    .D {text-align: right; padding-left: 0.24in; padding-right: 0.24in}\n"
"    .E {font-weight: bold; text-align: right; "
"padding-left: 0.14in; padding-right: 0.14in}\n"
"  </style>\n"
#if TRC_USE_PARAMS_SPOILERS
"  <script type=\"text/javascript\">\n"
"    function showSpoiler(obj)\n"
"    {\n"
"      var button = obj.parentNode.getElementsByTagName(\"input\")[0];\n"
"      var inner = obj.parentNode.getElementsByTagName(\"div\")[0];\n"
"      if (inner.style.display == \"none\")\n"
"      {\n"
"        inner.style.display = \"\";\n"
"        button.value=\"Hide Parameters\";"
"      }\n"
"      else\n"
"      {\n"
"        inner.style.display = \"none\";\n"
"        button.value=\"Show Parameters\";"
"      }\n"
"    }\n"
"  </script>\n"
#endif
"</head>\n"
"<body lang=\"en-US\" dir=\"ltr\">\n";


static const char * const trc_html_doc_end =
"</body>\n"
"</html>\n";

static const char * const trc_stats_table =
"<table border=1 cellpadding=4 cellspacing=3>\n"
"  <tr>\n"
"    <td rowspan=7>\n"
"      <h2>Run</h2>\n"
"    </td>\n"
"    <td>\n"
"      <b>Total</b>\n"
"    </td>\n"
"    <td class=\"D\">\n"
"      %u\n"
"    </td>\n"
"  </tr>\n"
"  <tr>\n"
"    <td class=\"B\">\n"
"      Passed, as expected\n"
"    </td>\n"
"    <td class=\"D\">\n"
"      %u\n"
"    </td>\n"
"  </tr>\n"
"  <tr>\n"
"    <td class=\"B\">\n"
"      Failed, as expected\n"
"    </td>\n"
"    <td class=\"D\">\n"
"      %u\n"
"    </td>\n"
"  </tr>\n"
"  <tr>\n"
"    <td class=\"B\">\n"
"      Passed unexpectedly\n"
"    </td>\n"
"    <td class=\"D\">\n"
"      %u\n"
"    </td>\n"
"  </tr>\n"
"  <tr>\n"
"    <td class=\"B\">\n"
"      Failed unexpectedly\n"
"    </td>\n"
"    <td class=\"D\">\n"
"      %u\n"
"    </td>\n"
"  </tr>\n"
"  <tr>\n"
"    <td class=\"B\">\n"
"      Aborted (no useful feedback)\n"
"    </td>\n"
"    <td class=\"D\">\n"
"      %u\n"
"    </td>\n"
"  </tr>\n"
"  <tr>\n"
"    <td class=\"B\">\n"
"      New (expected result is not known)\n"
"    </td>\n"
"    <td class=\"D\">\n"
"      %u\n"
"    </td>\n"
"  </tr>\n"
"  <tr>\n"
"    <td rowspan=3>\n"
"      <h2>Not Run</h2>\n"
"    </td>\n"
"    <td>\n"
"      <b>Total</b>\n"
"    </td>\n"
"    <td class=\"D\">\n"
"      %u\n"
"    </td>\n"
"  </tr>\n"
"  <tr>\n"
"    <td class=\"B\">\n"
"      Skipped, as expected\n"
"    </td>\n"
"    <td class=\"D\">\n"
"      %u\n"
"    </td>\n"
"  </tr>\n"
"  <tr>\n"
"    <td class=\"B\">\n"
"      Skipped unexpectedly\n"
"    </td>\n"
"    <td class=\"D\">\n"
"      %u\n"
"    </td>\n"
"  </tr>\n"
"</TABLE>\n";

static const char * const trc_report_html_tests_stats_start =
"<table border=1 cellpadding=4 cellspacing=3>\n"
"  <thead>\n"
"    <tr>\n"
"      <td rowspan=2>\n"
"        <b>Name</b>\n"
"      </td>\n"
"      <td rowspan=2>\n"
"        <b>Objective</b>\n"
"      </td>\n"
"      <td colspan=6 align=center>\n"
"        <b>Run</b>\n"
"      </td>\n"
"      <td colspan=3 align=center>\n"
"        <b>Not Run</b>\n"
"      </td>\n"
"      <td rowspan=2>\n"
"        <b>Key</b>\n"
"      </td>\n"
"      <td rowspan=2>\n"
"        <b>Notes</b>\n"
"      </td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td>\n"
"        <b>Total</b>\n"
"      </td>\n"
"      <td>\n"
"        Passed expect\n"
"      </td>\n"
"      <td>\n"
"        Failed expect\n"
"      </td>\n"
"      <td>\n"
"        Passed unexp\n"
"      </td>\n"
"      <td>\n"
"        Failed unexp\n"
"      </td>\n"
"      <td>\n"
"        Aborted, New\n"
"      </td>\n"
"      <td>\n"
"        <b>Total</b>\n"
"      </td>\n"
"      <td>\n"
"        Skipped expect\n"
"      </td>\n"
"      <td>\n"
"        Skipped unexp\n"
"      </td>\n"
"    </tr>\n"
"  </thead>\n"
"  <tbody>\n";

static const char * const trc_tests_stats_end =
"  </tbody>\n"
"</table>\n";

static const char * const trc_tests_stats_row =
"    <tr>\n"
"      <td>\n"
"        %s<b><a %s=\"%s%s\">%s</a></b>\n"
"      </td>\n"
"      <td>\n"
"        %s%s%s%s%s\n"
"      </td>\n"
"      <td class=\"E\">\n"
"        %u\n"
"      </td>\n"
"      <td class=\"C\">\n"
"        %u\n"
"      </td>\n"
"      <td class=\"C\">\n"
"        %u\n"
"      </td>\n"
"      <td class=\"C\">\n"
"        %u\n"
"      </td>\n"
"      <td class=\"C\">\n"
"        %u\n"
"      </td>\n"
"      <td class=\"C\">\n"
"        %u\n"
"      </td>\n"
"      <td class=\"E\">\n"
"        %u\n"
"      </td>\n"
"      <td class=\"C\">\n"
"        %u\n"
"      </td>\n"
"      <td class=\"C\">\n"
"        %u\n"
"      </td>\n"
"      <td>%s</td>\n"
"      <td>%s</td>\n"
"    </tr>\n";

static const char * const trc_report_html_test_exp_got_start =
"<table border=1 cellpadding=4 cellspacing=3>\n"
"  <thead>\n"
"    <tr>\n"
"      <td>\n"
"        <b>Name</b>\n"
"      </td>\n"
"      <td>\n"
"        <b>Parameters</b>\n"
"      </td>\n"
"      <td>\n"
"        <b>Expected</b>\n"
"      </td>\n"
"      <td>\n"
"        <b>Obtained</b>\n"
"      </td>\n"
"      <td>"
"        <b>Key</b>\n"
"      </td>\n"
"      <td>"
"        <b>Notes</b>\n"
"      </td>\n"
"    </tr>\n"
"  </thead>\n"
"  <tbody>\n";

static const char * const trc_test_exp_got_end =
"  </tbody>\n"
"</table>\n";

static const char * const trc_test_exp_got_row_start =
"    <tr>\n"
"      <td>\n"
"        %s<b><a %s%s%shref=\"#OBJECTIVE%s\">%s</a></b>\n"
"      </td>\n"
"      <td valign=top>";

#if TRC_USE_PARAMS_SPOILERS
static const char * const trc_test_exp_got_row_params_start =
"<input type=\"button\" onclick=\"showSpoiler(this);\""
" value=\"Show Parameters\" />\n"
"          <div class=\"inner\" style=\"display:none;\">";

static const char * const trc_test_exp_got_row_params_end =
" </div>";
#endif

static const char * const trc_test_exp_got_row_mid =
" </td>\n<td>";

static const char * const trc_test_exp_got_row_end =
"</td>\n"
"      <td>%s %s</td>\n"
"    </tr>\n";


static TAILQ_HEAD(, trc_report_key_entry) keys;

static int file_to_file(FILE *dst, FILE *src);

static trc_report_key_entry *
trc_report_key_find(const char *key_name)
{
    trc_report_key_entry *key;
    for (key = TAILQ_FIRST(&keys);
         key != NULL;
         key = TAILQ_NEXT(key, links))
    {
        if (strcmp(key->name, key_name) == 0)
        {
            break;
        }
    }
    return key;
}

static trc_report_key_entry *
trc_report_key_add(const char *key_name,
                   const trc_report_test_iter_data *iter_data,
                   char *iter_name, char *iter_path)
{
    trc_report_key_entry        *key = trc_report_key_find(key_name);
    trc_report_key_iter_entry   *key_iter = NULL;

    /* Create new key entry, if key does not exist */
    if (key == NULL)
    {
        if ((key = calloc(1, sizeof(trc_report_key_entry))) == NULL)
            return NULL;

        key->name = strdup(key_name);
        TAILQ_INIT(&key->iters);
        TAILQ_INSERT_TAIL(&keys, key, links);
    }

    for (key_iter = TAILQ_FIRST(&key->iters);
         key_iter != NULL;
         key_iter = TAILQ_NEXT(key_iter, links))
    {
        if (strcmp(iter_path, key_iter->path) == 0)
        {
            /* Do not duplicate iterations, exit */
            return key;
        }
    }

    /* Create new key iteration entry, if not found */
    if ((key_iter = calloc(1, sizeof(trc_report_key_iter_entry))) == NULL)
    {
        ERROR("Failed to allocate structure to store iteration key");
        return NULL;
    }
    key_iter->iter = (trc_report_test_iter_data *)iter_data;
    key_iter->name = strdup(iter_name);
    key_iter->path = strdup(iter_path);
    TAILQ_INSERT_TAIL(&key->iters, key_iter, links);

    return key;
}

static int
trc_report_keys_add(const char *key_names,
                    const trc_report_test_iter_data *iter_data,
                    char *iter_name, char *iter_path)
{
    int     count = 0;
    char   *p = NULL;
    trc_report_test_iter_entry *iter = NULL;

    for (iter = TAILQ_FIRST(&iter_data->runs);
         iter != NULL;
         iter = TAILQ_NEXT(iter, links))
    {
        if ((iter->result.status != TE_TEST_PASSED) &&
            (iter->result.status != TE_TEST_SKIPPED))
            break;
    }

    if (iter == NULL)
        return 0;

    /* Iterate through keys list with ',' delimiter */
    while ((key_names != NULL) && (*key_names != '\0'))
    {
        if ((p = strchr(key_names, ',')) != NULL)
        {
            char *tmp_key_name = strndup(key_names, p - key_names);

            if (tmp_key_name == NULL)
                break;

            trc_report_key_add(tmp_key_name, iter_data,
                               iter_name, iter_path);
            free(tmp_key_name);
            key_names = p + 1;
            while (*key_names == ' ')
                key_names++;
        }
        else
        {
            trc_report_key_add(key_names, iter_data,
                               iter_name, iter_path);
            key_names = NULL;
        }
        count++;
    }

    return count;
}

char *
trc_report_key_test_path(FILE *f, char *test_path, char *key_names)
{
    char *path = te_sprintf("%s-%s", test_path, key_names);
    char *p;

    if (path == NULL)
        return NULL;

    for (p = path; *p != '\0'; p++)
    {
        if (*p == ' ')
            *p = '_';
        else if (*p == ',')
            *p = '-';
    }

    fprintf(f, "<a name=\"%s\"/>", path);

    return path;
}

static void
trc_report_init_keys()
{
    TAILQ_INIT(&keys);
}

#define TRC_REPORT_OL_KEY_PREFIX        "OL "

/**
 * Output key entry to HTML report.
 *
 * @param f             File stream to write to
 * @param flags         Current output flags
 * @param test_path     Test path
 * @param level_str     String to represent depth of the test in the
 *                      suite
 *
 * @return Status code.
 */
static te_errno
trc_report_keys_to_html(FILE *f, char *keytool_fn)
{
    int                     fd_in = -1;
    int                     fd_out = -1;
    FILE                   *f_in = NULL;
    FILE                   *f_out = NULL;
    pid_t                   pid;
    trc_report_key_entry   *key;

    if ((pid = te_shell_cmd_inline(keytool_fn, -1,
                                   &fd_in, &fd_out, NULL)) < 0)
    {
        return pid;
    }
    f_in = fdopen(fd_in, "w");
    f_out = fdopen(fd_out, "r");

    for (key = TAILQ_FIRST(&keys);
         key != NULL;
         key = TAILQ_NEXT(key, links))
    {
        if (strncmp(key->name, TRC_REPORT_OL_KEY_PREFIX,
                    strlen(TRC_REPORT_OL_KEY_PREFIX)) == 0)
        {
            trc_report_key_iter_entry *key_iter = NULL;

            fprintf(f_in, "%s:", key->name);

            for (key_iter = TAILQ_FIRST(&key->iters);
                 key_iter != NULL;
                 key_iter = TAILQ_NEXT(key_iter, links))
            {
                fprintf(f_in, "%s#%s,", key_iter->name, key_iter->path);
            }
            fprintf(f_in, "\n");
        }
    }

    fclose(f_in);
    close(fd_in);

    file_to_file(f, f_out);

    fclose(f_out);
    close(fd_out);

    return 0;
}


/**
 * Output grand total statistics to HTML.
 *
 * @param stats     Statistics to output
 *
 * @return Status code.
 */
static int
trc_report_stats_to_html(FILE *f, const trc_report_stats *stats)
{
    fprintf(f, trc_stats_table,
            TRC_STATS_RUN(stats),
            stats->pass_exp, stats->fail_exp,
            stats->pass_une, stats->fail_une,
            stats->aborted, stats->new_run,
            TRC_STATS_NOT_RUN(stats),
            stats->skip_exp, stats->skip_une);

    return 0;
}

/**
 * Should test iteration instance be output in accordance with
 * expected/obtained result and current output flags?
 *
 * @param test      Test
 * @param iter      Test iteration entry in TRC report
 * @param flag      Output flags
 */
static te_bool
trc_report_test_iter_entry_output(
    const trc_test                   *test,
    const trc_report_test_iter_entry *iter,
    unsigned int                      flags)
{
    te_test_status status =
        (iter == NULL) ? TE_TEST_UNSPEC : iter->result.status;

    return (/* NO_SCRIPTS is clear or it is NOT a script */
            (~flags & TRC_REPORT_NO_SCRIPTS) ||
             (test->type != TRC_TEST_SCRIPT)) &&
            (/* NO_UNSPEC is clear or obtained result is not UNSPEC */
             (~flags & TRC_REPORT_NO_UNSPEC) ||
             (status != TE_TEST_UNSPEC)) &&
            (/* NO_SKIPPED is clear or obtained result is not SKIPPED */
             (~flags & TRC_REPORT_NO_SKIPPED) ||
             (status != TE_TEST_SKIPPED)) &&
            (/*
              * NO_EXP_PASSED is clear or
              * obtained result is not PASSED as expected
              */
             (~flags & TRC_REPORT_NO_EXP_PASSED) ||
             (status != TE_TEST_PASSED) || (!iter->is_exp)) &&
            (/* 
              * NO_EXPECTED is clear or obtained result is equal
              * to expected
              */
             (~flags & TRC_REPORT_NO_EXPECTED) || (!iter->is_exp));
}

/**
 * Output test iteration expected/obtained results to HTML report.
 *
 * @param f             File stream to write to
 * @param ctx           TRC report context
 * @param walker        TRC database walker position
 * @param flags         Current output flags
 * @param anchor        Should HTML anchor be created?
 * @param test_path     Full test path for anchor
 * @param level_str     String to represent depth of the test in the
 *                      suite
 *
 * @return Status code.
 */
static te_errno
trc_report_exp_got_to_html(FILE                *f,
                           trc_report_ctx      *ctx,
                           te_trc_db_walker    *walker,
                           unsigned int         flags,
                           te_bool             *anchor,
                           const char          *test_path,
                           const char          *level_str)
{
    const trc_test                   *test;
    const trc_test_iter              *iter;
    trc_report_test_iter_data        *iter_data;
    const trc_report_test_iter_entry *iter_entry;
    te_errno                           rc = 0;

    assert(anchor != NULL);
    assert(test_path != NULL);

    iter = trc_db_walker_get_iter(walker);
    test = iter->parent;
    iter_data = trc_db_walker_get_user_data(walker, ctx->db_uid);
    iter_entry = (iter_data == NULL) ? NULL : TAILQ_FIRST(&iter_data->runs);

    do {
        if (trc_report_test_iter_entry_output(test, iter_entry, flags))
        {
            if (iter_data == NULL)
            {
                iter_data = TE_ALLOC(sizeof(*iter_data));
                if (iter_data == NULL)
                {
                    rc = TE_ENOMEM;
                    break;
                }
                TAILQ_INIT(&iter_data->runs);
                iter_data->exp_result =
                    trc_db_walker_get_exp_result(walker, &ctx->tags);

                rc = trc_db_walker_set_user_data(walker, ctx->db_uid,
                                                 iter_data);
                if (rc != 0)
                {
                    free(iter_data);
                    break;
                }
            }
            assert(iter_data != NULL);

            fprintf(f, trc_test_exp_got_row_start,
                    PRINT_STR(level_str),
                    anchor ? "name=\"" : "",
                    anchor ? test_path : "",
                    anchor ? "\" " : "",
                    test_path,
                    test->name);
            *anchor = FALSE;

#if TRC_USE_PARAMS_SPOILERS
            if (!TAILQ_EMPTY(&iter->args.head))
                WRITE_STR(trc_test_exp_got_row_params_start);
#endif

            rc = trc_test_iter_args_to_html(f, &iter->args, 0);
            if (rc != 0)
                break;

#if TRC_USE_PARAMS_SPOILERS
            if (!TAILQ_EMPTY(&iter->args.head))
                WRITE_STR(trc_test_exp_got_row_params_end);
#endif

            WRITE_STR(trc_test_exp_got_row_mid);

            rc = trc_exp_result_to_html(f, iter_data->exp_result, 0);
            if (rc != 0)
                break;

            WRITE_STR(trc_test_exp_got_row_mid);

            rc = te_test_result_to_html(f, (iter_entry == NULL) ? NULL :
                                               &iter_entry->result);
            if (rc != 0)
                break;

            WRITE_STR(trc_test_exp_got_row_mid);

            if (iter_data->exp_result != NULL &&
                iter_data->exp_result->key != NULL)
            {
                char *key_test_path =
                    trc_report_key_test_path(f, (char *)test_path,
                                             iter_data->exp_result->key);

                trc_re_key_substs(iter_data->exp_result->key, f);

                /*
                 * Iterations does not have unique names and paths yet,
                 * use test name and path instead of
                 */
                trc_report_keys_add(iter_data->exp_result->key, iter_data,
                                    test->name, key_test_path);

                free(key_test_path);
            }

            fprintf(f, trc_test_exp_got_row_end,
                    (iter_data->exp_result == NULL) ? "" :
                        PRINT_STR(iter_data->exp_result->notes),
                    PRINT_STR(iter->notes));
        }
    } while (iter_entry != NULL &&
             (iter_entry = TAILQ_NEXT(iter_entry, links)) != NULL);

cleanup:
    return rc;
}


/**
 * Should test entry be output in accordance with expected/obtained
 * result statistics and current output flags?
 *
 * @param stats     Statistics
 * @param flag      Output flags
 */
static te_bool
trc_report_test_output(const trc_report_stats *stats, unsigned int flags)
{
    return
        (/* It is a script. Do output, if ... */
         /* NO_SCRIPTS is clear */
         (~flags & TRC_REPORT_NO_SCRIPTS) &&
         /* NO_UNSPEC is clear or tests with specified result */
         ((~flags & TRC_REPORT_NO_UNSPEC) ||
          (TRC_STATS_SPEC(stats) != 0)) &&
         /* NO_SKIPPED is clear or tests are run or unspec */
         ((~flags & TRC_REPORT_NO_SKIPPED) ||
          (TRC_STATS_RUN(stats) != 0) ||
          ((~flags & TRC_REPORT_NO_STATS_NOT_RUN) &&
           (TRC_STATS_NOT_RUN(stats) != 
            (stats->skip_exp + stats->skip_une)))) &&
         /* NO_EXP_PASSED or not all tests are passed as expected */
         ((~flags & TRC_REPORT_NO_EXP_PASSED) ||
          (TRC_STATS_RUN(stats) != stats->pass_exp) ||
          ((TRC_STATS_NOT_RUN(stats) != 0) &&
           ((TRC_STATS_NOT_RUN(stats) != stats->not_run) ||
            (~flags & TRC_REPORT_NO_STATS_NOT_RUN)))) &&
         /* NO_EXPECTED or unexpected results are obtained */
         ((~flags & TRC_REPORT_NO_EXPECTED) ||
          ((TRC_STATS_UNEXP(stats) != 0) &&
           ((TRC_STATS_UNEXP(stats) != stats->not_run) ||
            (~flags & TRC_REPORT_NO_STATS_NOT_RUN)))));
}

/**
 * Output test entry to HTML report.
 *
 * @param f             File stream to write to
 * @param ctx           TRC report context
 * @param walker        TRC database walker position
 * @param flags         Current output flags
 * @param test_path     Test path
 * @param level_str     String to represent depth of the test in the
 *                      suite
 *
 * @return Status code.
 */
static te_errno
trc_report_test_stats_to_html(FILE             *f,
                              trc_report_ctx   *ctx,
                              te_trc_db_walker *walker,
                              unsigned int      flags,
                              const char       *test_path,
                              const char       *level_str)
{
    const trc_test             *test;
    const trc_report_test_data *test_data;
    const trc_report_stats     *stats;

    test = trc_db_walker_get_test(walker);
    test_data = trc_db_walker_get_user_data(walker, ctx->db_uid);
    assert(test_data != NULL);
    stats = &test_data->stats;

    if (((test->type == TRC_TEST_PACKAGE) &&
         (flags & TRC_REPORT_NO_SCRIPTS)) ||
        trc_report_test_output(stats, flags))
    {
        te_bool     name_link;
        const char *keys = NULL;

        /* test_iters_check_output_and_get_keys(test, flags); */

        name_link = ((flags & TRC_REPORT_NO_SCRIPTS) ||
                     ((~flags & TRC_REPORT_NO_SCRIPTS) &&
                     (test->type == TRC_TEST_SCRIPT)));

        fprintf(f, trc_tests_stats_row,
                PRINT_STR(level_str),
                name_link ? "href" : "name",
                name_link ? "#" : "",
                test_path,
                test->name,
                test_path != NULL ? "<a name=\"OBJECTIVE" : "",
                PRINT_STR(test_path),
                test_path != NULL ? "\">": "",
                PRINT_STR(test->objective),
                test_path != NULL ? "</a>": "",
                TRC_STATS_RUN(stats),
                stats->pass_exp, stats->fail_exp,
                stats->pass_une, stats->fail_une,
                stats->aborted + stats->new_run,
                TRC_STATS_NOT_RUN(stats),
                stats->skip_exp, stats->skip_une,
                PRINT_STR(keys), PRINT_STR(test->notes));
    }

    return 0;
}

/**
 * Generate one table in HTML report.
 *
 * @param f         File stream to write to
 * @param ctx       TRC report context
 * @param stats     Is it statistics or details mode?
 * @param flags     Output flags
 *
 * @return Status code.
 */
static te_errno
trc_report_html_table(FILE *f, trc_report_ctx *ctx, 
                      te_bool is_stats, unsigned int flags)
{
    te_errno                rc = 0;
    te_trc_db_walker       *walker;
    trc_db_walker_motion    mv;
    unsigned int            level = 0;
    te_bool                 anchor = FALSE; /* FIXME */
    const char             *last_test_name = NULL;
    te_string               test_path = TE_STRING_INIT;
    te_string               level_str = TE_STRING_INIT;

    walker = trc_db_new_walker(ctx->db);
    if (walker == NULL)
        return TE_ENOMEM;

    if (is_stats)
        WRITE_STR(trc_report_html_tests_stats_start);
    else
        WRITE_STR(trc_report_html_test_exp_got_start);

    while ((rc == 0) &&
           ((mv = trc_db_walker_move(walker)) != TRC_DB_WALKER_ROOT))
    {
        switch (mv)
        {
            case TRC_DB_WALKER_SON:
                level++;
                if ((level & 1) == 1)
                {
                    /* Test entry */
                    if (level > 1)
                    {
                        rc = te_string_append(&level_str, "*-");
                        if (rc != 0)
                            break;
                    }
                }
                /*@fallthrou@*/

            case TRC_DB_WALKER_BROTHER:
                if ((level & 1) == 1)
                {
                    /* Test entry */
                    if (mv != TRC_DB_WALKER_SON)
                    {
                        te_string_cut(&test_path,
                                      strlen(last_test_name) + 1);
                    }

                    last_test_name = trc_db_walker_get_test(walker)->name;

                    rc = te_string_append(&test_path, "-%s",
                                          last_test_name);
                    if (rc != 0)
                        break;
                    if (is_stats)
                        rc = trc_report_test_stats_to_html(f, ctx, walker,
                                                           flags,
                                                           test_path.ptr,
                                                           level_str.ptr);
                }
                else
                {
                    if (!is_stats)
                        rc = trc_report_exp_got_to_html(f, ctx, walker,
                                                        flags, &anchor,
                                                        test_path.ptr,
                                                        level_str.ptr);
                }
                break;

            case TRC_DB_WALKER_FATHER:
                level--;
                if ((level & 1) == 0)
                {
                    /* Back from the test to parent iteration */
                    te_string_cut(&level_str, strlen("*-"));
                    te_string_cut(&test_path,
                                  strlen(last_test_name) + 1);
                    last_test_name = trc_db_walker_get_test(walker)->name;
                }
                break;

            default:
                assert(FALSE);
                break;
        }
    }
    if (is_stats)
        WRITE_STR(trc_tests_stats_end);
    else
        WRITE_STR(trc_test_exp_got_end);

cleanup:
    trc_db_free_walker(walker);
    te_string_free(&test_path);
    te_string_free(&level_str);
    return rc;
}

/**
 * Copy all content of one file to another.
 *
 * @param dst       Destination file
 * @param src       Source file
 *
 * @return Status code.
 */
static int
file_to_file(FILE *dst, FILE *src)
{
    char    buf[4096];
    size_t  r;

    rewind(src);
    do {
        r = fread(buf, 1, sizeof(buf), src);
        if (r > 0)
           if (fwrite(buf, r, 1, dst) != 1)
               return errno;
    } while (r == sizeof(buf));

    return 0;
}


/** See the description in trc_report.h */
te_errno
trc_report_to_html(trc_report_ctx *gctx, const char *filename,
                   const char *title, FILE *header,
                   unsigned int flags)
{
    FILE       *f;
    te_errno    rc;
    tqe_string *tag;

    f = fopen(filename, "w");
    if (f == NULL)
    {
        rc = te_rc_os2te(errno);
        ERROR("Failed to open file to write HTML report to: %r", rc);
        return rc;
    }

    /* HTML header */
    fprintf(f, trc_html_doc_start,
            (title != NULL) ? title : trc_html_title_def);
    if (title != NULL)
        fprintf(f, "<h1 align=center>%s</h1>\n", title);
    if (gctx->db->version != NULL)
        fprintf(f, "<h2 align=center>%s</h2>\n", gctx->db->version);

    /* TRC tags */
    WRITE_STR("<b>Tags:</b>");
    TAILQ_FOREACH(tag, &gctx->tags, links)
    {
        fprintf(f, "  %s", tag->v);
    }
    WRITE_STR("<p/>");

    /* Header provided by user */
    if (header != NULL)
    {
        rc = file_to_file(f, header);
        if (rc != 0)
        {
            ERROR("Failed to copy header to HTML report");
            goto cleanup;
        }
    }

    if (~flags & TRC_REPORT_NO_KEYS)
    {
        trc_report_init_keys();
    }

    if (~flags & TRC_REPORT_NO_TOTAL_STATS)
    {
        /* Grand total */
        rc = trc_report_stats_to_html(f, &gctx->stats);
        if (rc != 0)
            goto cleanup;
    }

    if (~flags & TRC_REPORT_NO_PACKAGES_ONLY)
    {
        /* Report for packages */
        rc = trc_report_html_table(f, gctx, TRUE,
                                   flags | TRC_REPORT_NO_SCRIPTS);
        if (rc != 0)
            goto cleanup;
    }

    if (~flags & TRC_REPORT_NO_SCRIPTS)
    {
        /* Report with iterations of packages and w/o iterations of tests */
        rc = trc_report_html_table(f, gctx, TRUE, flags);
        if (rc != 0)
            goto cleanup;
    }

    if ((~flags & TRC_REPORT_STATS_ONLY) &&
        (~flags & TRC_REPORT_NO_SCRIPTS))
    {
        /* Full report */
        rc = trc_report_html_table(f, gctx, FALSE, flags);
        if (rc != 0)
            goto cleanup;
    }

    if (~flags & TRC_REPORT_NO_KEYS)
    {
        trc_report_keys_to_html(f, "te-trc-key");
    }

    /* HTML footer */
    WRITE_STR(trc_html_doc_end);

    fclose(f);
    return 0;

cleanup:
    fclose(f);
    unlink(filename);
    return rc;
}
