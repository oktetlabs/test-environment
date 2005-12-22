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
#include "trc_log.h"
#include "trc_db.h"


#define WRITE_STR(str) \
    do {                                                \
        fflush(f);                                      \
        if (fwrite(str, strlen(str), 1, f) != 1)        \
        {                                               \
            rc = errno ? : EIO;                         \
            ERROR("Writing to the file failed");        \
            goto cleanup;                               \
        }                                               \
    } while (0)

#define PRINT_STR(str_)  (((str_) != NULL) ? (str_) : "")


static FILE   *f;
static int     fd;


static const char * const trc_html_doc_start =
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n"
"<HTML>\n"
"<HEAD>\n"
"  <META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; "
"charset=utf-8\">\n"
"  <TITLE>Testing Results Comparison Report</TITLE>\n"
"  <style type=\"text/css\">\n"
"    .A {padding-left: 0.14in; padding-right: 0.14in}\n"
"    .B {padding-left: 0.24in; padding-right: 0.04in}\n"
"    .C {text-align: right; padding-left: 0.14in; padding-right: 0.14in}\n"
"    .D {text-align: right; padding-left: 0.24in; padding-right: 0.24in}\n"
"    .E {font-weight: bold; text-align: right; "
"padding-left: 0.14in; padding-right: 0.14in}\n"
"  </style>\n"
"</HEAD>\n"
"<BODY LANG=\"en-US\" DIR=\"LTR\">\n";

static const char * const trc_html_doc_end =
"</BODY>\n"
"</HTML>\n";

static const char * const trc_stats_table =
"<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=3>\n"
"  <TR>\n"
"    <TD ROWSPAN=7>\n"
"      <H2>Run</H2>\n"
"    </TD>\n"
"    <TD>\n"
"      <B>Total</B>\n"
"    </TD>\n"
"    <TD class=\"D\">\n"
"      %u\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD class=\"B\">\n"
"      Passed, as expected\n"
"    </TD>\n"
"    <TD class=\"D\">\n"
"      %u\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD class=\"B\">\n"
"      Failed, as expected\n"
"    </TD>\n"
"    <TD class=\"D\">\n"
"      %u\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD class=\"B\">\n"
"      Passed unexpectedly\n"
"    </TD>\n"
"    <TD class=\"D\">\n"
"      %u\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD class=\"B\">\n"
"      Failed unexpectedly\n"
"    </TD>\n"
"    <TD class=\"D\">\n"
"      %u\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD class=\"B\">\n"
"      Aborted (no useful feedback)\n"
"    </TD>\n"
"    <TD class=\"D\">\n"
"      %u\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD class=\"B\">\n"
"      New (expected result is not known)\n"
"    </TD>\n"
"    <TD class=\"D\">\n"
"      %u\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD ROWSPAN=3>\n"
"      <H2>Not Run</H2>\n"
"    </TD>\n"
"    <TD>\n"
"      <B>Total</B>\n"
"    </TD>\n"
"    <TD class=\"D\">\n"
"      %u\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD class=\"B\">\n"
"      Skipped, as expected\n"
"    </TD>\n"
"    <TD class=\"D\">\n"
"      %u\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD class=\"B\">\n"
"      Skipped unexpectedly\n"
"    </TD>\n"
"    <TD class=\"D\">\n"
"      %u\n"
"    </TD>\n"
"  </TR>\n"
"</TABLE>\n";

static const char * const trc_tests_stats_start =
"<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=3>\n"
"  <THEAD>\n"
"    <TR>\n"
"      <TD ROWSPAN=2>\n"
"        <B>Name</B>\n"
"      </TD>\n"
"      <TD ROWSPAN=2>\n"
"        <B>Objective</B>\n"
"      </TD>\n"
"      <TD COLSPAN=6 ALIGN=CENTER>\n"
"        <B>Run</B>\n"
"      </TD>\n"
"      <TD COLSPAN=3 ALIGN=CENTER>\n"
"        <B>Not Run</B>\n"
"      </TD>\n"
"      <TD ROWSPAN=2>\n"
"        <B>Key</B>\n"
"      </TD>\n"
"      <TD ROWSPAN=2>\n"
"        <B>Notes</B>\n"
"      </TD>\n"
"    </TR>\n"
"    <TR>\n"
"      <TD>\n"
"        <B>Total</B>\n"
"      </TD>\n"
"      <TD>\n"
"        Passed expect\n"
"      </TD>\n"
"      <TD>\n"
"        Failed expect\n"
"      </TD>\n"
"      <TD>\n"
"        Passed unexp\n"
"      </TD>\n"
"      <TD>\n"
"        Failed unexp\n"
"      </TD>\n"
"      <TD>\n"
"        Aborted, New\n"
"      </TD>\n"
"      <TD>\n"
"        <B>Total</B>\n"
"      </TD>\n"
"      <TD>\n"
"        Skipped expect\n"
"      </TD>\n"
"      <TD>\n"
"        Skipped unexp\n"
"      </TD>\n"
"    </TR>\n"
"  </THEAD>\n"
"  <TBODY>\n";

static const char * const trc_tests_stats_end =
"  </TBODY>\n"
"</TABLE>\n";

static const char * const trc_tests_stats_row =
"    <TR>\n"
"      <TD>\n"
"        %s<B><A %s=\"%s%s\">%s</A></B>\n"
"      </TD>\n"
"      <TD>\n"
"        %s%s%s%s%s\n"
"      </TD>\n"
"      <TD class=\"E\">\n"
"        %u\n"
"      </TD>\n"
"      <TD class=\"C\">\n"
"        %u\n"
"      </TD>\n"
"      <TD class=\"C\">\n"
"        %u\n"
"      </TD>\n"
"      <TD class=\"C\">\n"
"        %u\n"
"      </TD>\n"
"      <TD class=\"C\">\n"
"        %u\n"
"      </TD>\n"
"      <TD class=\"C\">\n"
"        %u\n"
"      </TD>\n"
"      <TD class=\"E\">\n"
"        %u\n"
"      </TD>\n"
"      <TD class=\"C\">\n"
"        %u\n"
"      </TD>\n"
"      <TD class=\"C\">\n"
"        %u\n"
"      </TD>\n"
"      <TD>%s</TD>\n"
"      <TD>%s</TD>\n"
"    </TR>\n";

static const char * const trc_test_exp_got_start =
"<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=3>\n"
"  <THEAD>\n"
"    <TR>\n"
"      <TD>\n"
"        <B>Name</B>\n"
"      </TD>\n"
"      <TD>\n"
"        <B>Parameters</B>\n"
"      </TD>\n"
"      <TD>\n"
"        <B>Expected</B>\n"
"      </TD>\n"
"      <TD>\n"
"        <B>Got</B>\n"
"      </TD>\n"
"      <TD>"
"        <B>Key</B>\n"
"      </TD>\n"
"      <TD>"
"        <B>Notes</B>\n"
"      </TD>\n"
"    </TR>\n"
"  </THEAD>\n"
"  <TBODY>\n";

static const char * const trc_test_exp_got_end =
"  </TBODY>\n"
"</TABLE>\n";

static const char * const trc_test_exp_got_row_start =
"    <TR>\n"
"      <TD>\n"
"        %s<B><A %s%s%shref=\"#OBJECTIVE%s\">%s</A></B>\n"
"      </TD>\n"
"      <TD>%s</TD>\n"
"      <TD>";

static const char * const trc_test_exp_got_row_mid =
" </TD>\n<TD>";

static const char * const trc_test_exp_got_row_end =
"</TD>\n"
"      <TD>%s</TD>\n"
"      <TD>%s %s</TD>\n"
"    </TR>\n";


static int tests_to_html(te_bool stats, unsigned int flags,
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
stats_to_html(const trc_stats *stats)
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
 * Map test result to string.
 *
 * @param result    Result to be mapped
 *
 * @return Test result as string.
 */
const char *
trc_test_result_to_string(trc_test_result result)
{
    switch (result)
    {
        case TRC_TEST_PASSED:
            return "passed";
        case TRC_TEST_FAILED:
            return "failed";
        case TRC_TEST_CORED:
            return "CORED";
        case TRC_TEST_KILLED:
            return "KILLED";
        case TRC_TEST_FAKED:
            return "faked";
        case TRC_TEST_SKIPPED:
            return "skipped";
        case TRC_TEST_UNSPEC:
            return "UNSPEC";
        default:
            return "OOps";
    }
}

/**
 * Generate a string with test arguments separated by new line.
 *
 * @param args      List of arguments
 *
 * @return Pointer to the generated string (static buffer).
 */
static const char *
trc_test_args_to_string(const test_args *args)
{
    static char trc_args_buf[0x10000];

    test_arg *p;
    char     *s = trc_args_buf;

    for (p = args->head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        s += sprintf(s, "%s=%s<BR/>", p->name, p->value);
    }
    return trc_args_buf;
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
            (~flags & TRC_OUT_NO_SCRIPTS) ||
             (test->type != TRC_TEST_SCRIPT)) &&
            (/* NO_UNSPEC is clear or got result is not UNSPEC */
             (~flags & TRC_OUT_NO_UNSPEC) ||
             (iter->got_result != TRC_TEST_UNSPEC)) &&
            (/* NO_SKIPPED is clear or got result is not SKIPPED */
             (~flags & TRC_OUT_NO_SKIPPED) ||
             (iter->got_result != TRC_TEST_SKIPPED)) &&
            (/*
              * NO_EXP_PASSED is clear or
              * got result is not PASSED as expected
              */
             (~flags & TRC_OUT_NO_EXP_PASSED) ||
             (iter->got_result != TRC_TEST_PASSED) ||
             (!iter->got_as_expect)) &&
            (/* NO_EXPECTED is clear or got result is equal to expected */
             (~flags & TRC_OUT_NO_EXPECTED) ||
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
    tqe_string     *v;

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
            
            fputs(trc_test_result_to_string(p->exp_result.value), f);
            if (p->exp_result.verdicts.tqh_first != NULL)
                fputs("<BR/><BR/>", f);
            for (v = p->exp_result.verdicts.tqh_first;
                 v != NULL;
                 v = v->links.tqe_next)
            {
                fputc(' ', f);
                fputs(v->str, f);
            }
            
            fputs(trc_test_exp_got_row_mid, f);
            
            fputs(trc_test_result_to_string(p->got_result), f);
            if (p->got_verdicts.tqh_first != NULL)
                fputs("<BR/><BR/>", f);
            for (v = p->got_verdicts.tqh_first;
                 v != NULL;
                 v = v->links.tqe_next)
            {
                fputc(' ', f);
                fputs(v->str, f);
            }
            
            fprintf(f, trc_test_exp_got_row_end,
                    PRINT_STR(p->exp_result.key),
                    PRINT_STR(p->exp_result.notes),
                    PRINT_STR(p->notes));

            name_anchor = FALSE;
        }
        rc = tests_to_html(stats, flags, test, &p->tests, level);
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
 * @param stats     Is it statistics or details mode?
 * @param flags     Output flags
 * @param parent    Parent test
 * @param tests     List of tests to output
 * @param level     Level of the test in the suite
 *
 * @return Status code.
 */
static int
tests_to_html(te_bool stats, unsigned int flags,
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
             (~flags & TRC_OUT_NO_SCRIPTS) &&
             /* NO_UNSPEC is clear or tests with specified result */
             ((~flags & TRC_OUT_NO_UNSPEC) ||
              (TRC_STATS_SPEC(&p->stats) != 0)) &&
             /* NO_SKIPPED is clear or tests are run or unspec */
             ((~flags & TRC_OUT_NO_SKIPPED) ||
              (TRC_STATS_RUN(&p->stats) != 0) ||
              (TRC_STATS_NOT_RUN(&p->stats) != 
               (p->stats.skip_exp + p->stats.skip_une))) &&
             /* NO_EXP_PASSED or not all tests are passed as expected */
             ((~flags & TRC_OUT_NO_EXP_PASSED) ||
              (TRC_STATS_RUN(&p->stats) != p->stats.pass_exp) ||
              (TRC_STATS_NOT_RUN(&p->stats) != 0)) &&
             /* NO_EXPECTED or unexpected results are got */
             ((~flags & TRC_OUT_NO_EXPECTED) ||
              (TRC_STATS_UNEXP(&p->stats) != 0)));

        if (stats &&
            (((p->type == TRC_TEST_PACKAGE) &&
              (flags & TRC_OUT_NO_SCRIPTS)) || output))
        {
            te_bool     name_link;
            char       *test_path = NULL;
            const char *keys =
                test_iters_check_output_and_get_keys(p, flags);

            name_link = ((flags & TRC_OUT_NO_SCRIPTS) ||
                         ((~flags & TRC_OUT_NO_SCRIPTS) &&
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
                    test_path != NULL ? "<A name=\"OBJECTIVE" : "",
                    PRINT_STR(test_path),
                    test_path != NULL ? "\">": "",
                    PRINT_STR(p->objective),
                    test_path != NULL ? "</A>": "",
                    TRC_STATS_RUN(&p->stats),
                    p->stats.pass_exp, p->stats.fail_exp,
                    p->stats.pass_une, p->stats.fail_une,
                    p->stats.aborted + p->stats.new_run,
                    TRC_STATS_NOT_RUN(&p->stats),
                    p->stats.skip_exp, p->stats.skip_une,
                    keys, PRINT_STR(p->notes));
        }
        if ((p->type != TRC_TEST_SCRIPT) ||
            (~flags & TRC_OUT_NO_SCRIPTS))
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


/** See descriptino in trc_db.h */
int
trc_report_to_html(const char *filename, FILE *header, trc_database *db,
                   unsigned int flags)
{
    int             rc;

    f = fopen(filename, "w");
    if (f == NULL)
    {
        ERROR("Failed to open file to write HTML report to");
        return errno;
    }
    fd = fileno(f);
    if (fd < 0)
    {
        rc = errno;
        ERROR("Failed to get descriptor of output file");
        goto cleanup;
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

    if (~flags & TRC_OUT_NO_TOTAL_STATS)
    {
        /* Grand total */
        rc = stats_to_html(&db->stats);
        if (rc != 0)
            goto cleanup;
    }

    /* Report for packages */
    if (~flags & TRC_OUT_NO_PACKAGES_ONLY)
    {
        rc = tests_to_html(TRUE, flags | TRC_OUT_NO_SCRIPTS,
                           NULL, &db->tests, 0);
        if (rc != 0)
            goto cleanup;
    }

    if (~flags & TRC_OUT_NO_SCRIPTS)
    {
        /* Report with iterations of packages and w/o iterations of tests */
        rc = tests_to_html(TRUE, flags, NULL, &db->tests, 0);
        if (rc != 0)
            goto cleanup;
    }

    if ((~flags & TRC_OUT_STATS_ONLY) &&
        (~flags & TRC_OUT_NO_SCRIPTS))
    {
        /* Full report */
        rc = tests_to_html(FALSE, flags, NULL, &db->tests, 0);
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
