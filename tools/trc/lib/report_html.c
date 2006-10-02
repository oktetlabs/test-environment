/** @file
 * @brief Testing Results Comparator
 *
 * Generator of comparison report in HTML format.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "te_defs.h"
#include "logger_api.h"
#include "trc_db.h"
#include "trc_report.h"


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


static const char * const trc_html_doc_start =
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n"
"<html>\n"
"<head>\n"
"  <meta http-equiv=\"content-type\" content=\"text/html; "
"charset=utf-8\">\n"
"  <title>Testing Results Comparison Report</title>\n"
"  <style type=\"text/css\">\n"
"    .A {padding-left: 0.14in; padding-right: 0.14in}\n"
"    .B {padding-left: 0.24in; padding-right: 0.04in}\n"
"    .C {text-align: right; padding-left: 0.14in; padding-right: 0.14in}\n"
"    .D {text-align: right; padding-left: 0.24in; padding-right: 0.24in}\n"
"    .E {font-weight: bold; text-align: right; "
"padding-left: 0.14in; padding-right: 0.14in}\n"
"  </style>\n"
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

static const char * const trc_tests_stats_start =
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

static const char * const trc_test_exp_got_start =
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
"        <b>Got</b>\n"
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
"      <td>%s</td>\n"
"      <td>";

static const char * const trc_test_exp_got_row_mid =
" </td>\n<td>";

static const char * const trc_test_exp_got_row_end =
"</td>\n"
"      <td>%s</td>\n"
"      <td>%s %s</td>\n"
"    </tr>\n";


static int tests_to_html(FILE *f, te_bool stats, unsigned int flags,
                         const test_run *parent, test_runs *tests,
                         unsigned int level);


/**
 * Output grand total statistics to HTML.
 *
 * @param stats     Statistics to output
 *
 * @return Status code.
 */
static int
trc_report_stats_to_html(FILE *f, const trc_stats *stats)
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
 * Should test iteration be output in accordance with expected/got
 * result and current output flags.
 *
 * @param test      Test
 * @param iter      Test iteration
 * @param flag      Output flags
 */
static te_bool
test_iter_output(const test_run *test, test_iter *iter, unsigned int flags)
{
    if (!iter->processed || flags != iter->proc_flags)
    {
        iter->processed = TRUE;
        iter->proc_flags = flags;
        iter->output = 
           (/* NO_SCRIPTS is clear or it is NOT a script */
            (~flags & TRC_REPORT_NO_SCRIPTS) ||
             (test->type != TRC_TEST_SCRIPT)) &&
            (/* NO_UNSPEC is clear or got result is not UNSPEC */
             (~flags & TRC_REPORT_NO_UNSPEC) ||
             (iter->got_result != TE_TEST_UNSPEC)) &&
            (/* NO_SKIPPED is clear or got result is not SKIPPED */
             (~flags & TRC_REPORT_NO_SKIPPED) ||
             (iter->got_result != TE_TEST_SKIPPED)) &&
            (/*
              * NO_EXP_PASSED is clear or
              * got result is not PASSED as expected
              */
             (~flags & TRC_REPORT_NO_EXP_PASSED) ||
             (iter->got_result != TE_TEST_PASSED) ||
             (!iter->got_as_expect)) &&
            (/* NO_EXPECTED is clear or got result is equal to expected */
             (~flags & TRC_REPORT_NO_EXPECTED) ||
             (!iter->got_as_expect));
    }
    return iter->output;
}

/**
 * Output test iterations to HTML report.
 *
 * @param stats     Is it statistics or details mode?
 * @param flags     Output flags
 * @param test      Test
 * @param level     Level of the test in the suite
 *
 * @return Status code.
 */
static int
test_iters_to_html(te_bool stats, unsigned int flags,
                   test_run *test, unsigned int level)
{
    int             rc;
    test_iter      *p;
    char            level_str[64] = { 0, };
    char           *s;
    unsigned int    i;
    te_bool         name_anchor = TRUE;

    for (s = level_str, i = 0; i < level; ++i)
        s += sprintf(s, "*-");

    for (p = test->iters.head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if ((!stats) && /* It is NOT a statistics report */
            test_iter_output(test, p, flags))
        {
            fprintf(f, trc_test_exp_got_row_start,
                    level_str,
                    name_anchor ? "name=\"" : "",
                    name_anchor ? test->test_path : "",
                    name_anchor ? "\" " : "",
                    test->test_path ? : "ERROR",
                    test->name,
                    trc_test_args_to_string(&p->args));
            
            trc_exp_result_to_html(f, /* expected */);
            
            fputs(trc_test_exp_got_row_mid, f);
            
            trc_test_result_to_html(f, /* obtained */);
            
            fprintf(f, trc_test_exp_got_row_end,
                    PRINT_STR(p->exp_result.key),
                    PRINT_STR(p->exp_result.notes),
                    PRINT_STR(p->notes));

            name_anchor = FALSE;
        }
        rc = tests_to_html(f, stats, flags, test, &p->tests, level);
        if (rc != 0)
            return rc;
    }
    return 0;
}

/**
 * Generate a list (in a string separated by HTML new line) of unique
 * (as strings) keys for iterations to be output.
 *
 * @param test      Test
 * @param flags     Output flags
 *
 * @se Update 'output' field of each iteration to be used further.
 *
 * @return Pointer a generated string (static buffer).
 */
static const char *
test_iters_check_output_and_get_keys(test_run *test, unsigned int flags)
{
    static char buf[0x10000];

    test_iter  *p;
    test_iter  *q;
    char       *s = buf;


    buf[0] = '\0';
    for (p = test->iters.head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (test_iter_output(test, p, flags) &&
            (p->exp_result.key != NULL))
        {
            for (q = test->iters.head.tqh_first;
                 (q != p) &&
                 ((q->exp_result.key == NULL) || (!q->output) ||
                  (strcmp(p->exp_result.key, q->exp_result.key) != 0));
                 q = q->links.tqe_next);

            if (p == q)
                s += sprintf(s, "%s<BR/>", p->exp_result.key);
        }
    }

    return buf;
}

/**
 * Output tests to HTML report.
 *
 * @param f         File stream to write to
 * @param stats     Is it statistics or details mode?
 * @param flags     Output flags
 * @param parent    Parent test
 * @param tests     List of tests to output
 * @param level     Level of the test in the suite
 *
 * @return Status code.
 */
static int
tests_to_html(FILE *f, te_bool stats, unsigned int flags,
              const test_run *parent, test_runs *tests,
              unsigned int level)
{
    int         rc;
    test_run   *p;
    char        level_str[64] = { 0, };
    char       *s;
    unsigned int i;

    if (level == 0)
    {
        if (stats)
            WRITE_STR(trc_tests_stats_start);
        else
            WRITE_STR(trc_test_exp_got_start);
    }
    for (s = level_str, i = 0; i < level; ++i)
        s += sprintf(s, "*-");
    for (p = tests->head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        te_bool output = 
            (/* It is a script. Do output, if ... */
             /* NO_SCRIPTS is clear */
             (~flags & TRC_REPORT_NO_SCRIPTS) &&
             /* NO_UNSPEC is clear or tests with specified result */
             ((~flags & TRC_REPORT_NO_UNSPEC) ||
              (TRC_STATS_SPEC(&p->stats) != 0)) &&
             /* NO_SKIPPED is clear or tests are run or unspec */
             ((~flags & TRC_REPORT_NO_SKIPPED) ||
              (TRC_STATS_RUN(&p->stats) != 0) ||
              ((~flags & TRC_REPORT_NO_STATS_NOT_RUN) &&
               (TRC_STATS_NOT_RUN(&p->stats) != 
                (p->stats.skip_exp + p->stats.skip_une)))) &&
             /* NO_EXP_PASSED or not all tests are passed as expected */
             ((~flags & TRC_REPORT_NO_EXP_PASSED) ||
              (TRC_STATS_RUN(&p->stats) != p->stats.pass_exp) ||
              ((TRC_STATS_NOT_RUN(&p->stats) != 0) &&
               ((TRC_STATS_NOT_RUN(&p->stats) != p->stats.not_run) ||
                (~flags & TRC_REPORT_NO_STATS_NOT_RUN)))) &&
             /* NO_EXPECTED or unexpected results are got */
             ((~flags & TRC_REPORT_NO_EXPECTED) ||
              ((TRC_STATS_UNEXP(&p->stats) != 0) &&
               ((TRC_STATS_UNEXP(&p->stats) != p->stats.not_run) ||
                (~flags & TRC_REPORT_NO_STATS_NOT_RUN)))));

        if (stats &&
            (((p->type == TRC_TEST_PACKAGE) &&
              (flags & TRC_REPORT_NO_SCRIPTS)) || output))
        {
            te_bool     name_link;
            char       *test_path = NULL;
            const char *keys =
                test_iters_check_output_and_get_keys(p, flags);

            name_link = ((flags & TRC_REPORT_NO_SCRIPTS) ||
                         ((~flags & TRC_REPORT_NO_SCRIPTS) &&
                         (p->type == TRC_TEST_SCRIPT)));

            if (p->test_path == NULL)
            {
                size_t len;

                len = strlen(p->name) + 2 +
                      ((parent == NULL) ? strlen("OBJ") :
                                          strlen(parent->test_path));
                test_path = malloc(len);
                if (test_path == NULL)
                {
                    ERROR("malloc(%u) failed", (unsigned)len);
                    return ENOMEM;
                }
                sprintf(test_path, "%s-%s",
                        (parent) ? parent->test_path : "", p->name);
                p->test_path = test_path;
            }

            fprintf(f, trc_tests_stats_row,
                    level_str,
                    name_link ? "href" : "name",
                    name_link ? "#" : "",
                    p->test_path,
                    p->name,
                    test_path != NULL ? "<a name=\"OBJECTIVE" : "",
                    PRINT_STR(test_path),
                    test_path != NULL ? "\">": "",
                    PRINT_STR(p->objective),
                    test_path != NULL ? "</a>": "",
                    TRC_STATS_RUN(&p->stats),
                    p->stats.pass_exp, p->stats.fail_exp,
                    p->stats.pass_une, p->stats.fail_une,
                    p->stats.aborted + p->stats.new_run,
                    TRC_STATS_NOT_RUN(&p->stats),
                    p->stats.skip_exp, p->stats.skip_une,
                    keys, PRINT_STR(p->notes));
        }
        if ((p->type != TRC_TEST_SCRIPT) ||
            (~flags & TRC_REPORT_NO_SCRIPTS))
        {
            rc = test_iters_to_html(stats, flags, p, level + 1);
            if (rc != 0)
                goto cleanup;
        }
    }
    if (level == 0)
    {
        if (stats)
            WRITE_STR(trc_tests_stats_end);
        else
            WRITE_STR(trc_test_exp_got_end);
    }
    return 0;

cleanup:
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
                   FILE *header, unsigned int flags)
{
    te_errno    rc;

    f = fopen(filename, "w");
    if (f == NULL)
    {
        rc = te_rc_os2te(errno);
        ERROR("Failed to open file to write HTML report to: %r", rc);
        return rc;
    }

    /* HTML header */
    WRITE_STR(trc_html_doc_start);

    if (header != NULL)
    {
        rc = file_to_file(f, header);
        if (rc != 0)
        {
            ERROR("Failed to copy header to HTML report");
            goto cleanup;
        }
    }

    if (~flags & TRC_REPORT_NO_TOTAL_STATS)
    {
        /* Grand total */
        rc = trc_report_stats_to_html(f, &gctx->stats);
        if (rc != 0)
            goto cleanup;
    }

    /* Report for packages */
    if (~flags & TRC_REPORT_NO_PACKAGES_ONLY)
    {
        rc = tests_to_html(f, TRUE, flags | TRC_REPORT_NO_SCRIPTS,
                           NULL, &db->tests, 0);
        if (rc != 0)
            goto cleanup;
    }

    if (~flags & TRC_REPORT_NO_SCRIPTS)
    {
        /* Report with iterations of packages and w/o iterations of tests */
        rc = tests_to_html(f, TRUE, flags, NULL, &db->tests, 0);
        if (rc != 0)
            goto cleanup;
    }

    if ((~flags & TRC_REPORT_STATS_ONLY) &&
        (~flags & TRC_REPORT_NO_SCRIPTS))
    {
        /* Full report */
        rc = tests_to_html(f, FALSE, flags, NULL, &db->tests, 0);
        if (rc != 0)
            goto cleanup;
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
