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

enum {
    TRC_HTML_OUT_STATS         = 0x01,
    TRC_HTML_OUT_PACKAGES_ONLY = 0x02,
};

static FILE   *f;
static int     fd;


static const char * const trc_html_doc_start =
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n"
"<HTML>\n"
"<HEAD>\n"
"  <META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; "
"charset=utf-8\">\n"
"  <TITLE>TE Testing Results Comparison Report</TITLE>\n"
"</HEAD>\n"
"<BODY LANG=\"en-US\" DIR=\"LTR\">\n"
"<H1 ALIGN=CENTER>Testing Results Comparison Report</H1>\n";

static const char * const trc_html_doc_end =
"</BODY>\n"
"</HTML>\n";

static const char * const trc_stats_table =
"<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=3>\n"
"  <TR>\n"
"    <TD ROWSPAN=7>\n"
"      <P><FONT SIZE=5><B>Run</B></FONT></P>\n"
"    </TD>\n"
"    <TD>\n"
"      <P><B>Total</B></P>\n"
"    </TD>\n"
"    <TD>\n"
"      <P ALIGN=RIGHT STYLE=\"margin-left: 0.2in; margin-right: 0.2in\">\n"
"      %u\n"
"      </P>\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD>\n"
"      <P STYLE=\"margin-left: 0.2in; margin-right: 0in\">\n"
"        Passed, as expected\n"
"      </P>\n"
"    </TD>\n"
"    <TD>\n"
"      <P ALIGN=RIGHT STYLE=\"margin-left: 0.2in; margin-right: 0.2in\">\n"
"      %u\n"
"      </P>\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD>\n"
"      <P STYLE=\"margin-left: 0.2in; margin-right: 0in\">\n"
"        Failed, as expected\n"
"      </P>\n"
"    </TD>\n"
"    <TD>\n"
"      <P ALIGN=RIGHT STYLE=\"margin-left: 0.2in; margin-right: 0.2in\">\n"
"      %u\n"
"      </P>\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD>\n"
"      <P STYLE=\"margin-left: 0.2in; margin-right: 0in\">\n"
"        Passed unexpectedly\n"
"      </P>\n"
"    </TD>\n"
"    <TD>\n"
"      <P ALIGN=RIGHT STYLE=\"margin-left: 0.2in; margin-right: 0.2in\">\n"
"      %u\n"
"      </P>\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD>\n"
"      <P STYLE=\"margin-left: 0.2in; margin-right: 0in\">\n"
"        Failed unexpectedly\n"
"      </P>\n"
"    </TD>\n"
"    <TD>\n"
"      <P ALIGN=RIGHT STYLE=\"margin-left: 0.2in; margin-right: 0.2in\">\n"
"      %u\n"
"      </P>\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD>\n"
"      <P STYLE=\"margin-left: 0.2in; margin-right: 0in\">\n"
"        Aborted (no useful feedback)\n"
"      </P>\n"
"    </TD>\n"
"    <TD>\n"
"      <P ALIGN=RIGHT STYLE=\"margin-left: 0.2in; margin-right: 0.2in\">\n"
"      %u\n"
"      </P>\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD>\n"
"      <P STYLE=\"margin-left: 0.2in; margin-right: 0in\">\n"
"        New (expected result is not known)\n"
"      </P>\n"
"    </TD>\n"
"    <TD>\n"
"      <P ALIGN=RIGHT STYLE=\"margin-left: 0.2in; margin-right: 0.2in\">\n"
"      %u\n"
"      </P>\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD ROWSPAN=3>\n"
"      <P><FONT SIZE=5><B>Not Run</B></FONT></P>\n"
"    </TD>\n"
"    <TD>\n"
"      <P><B>Total</B></P>\n"
"    </TD>\n"
"    <TD>\n"
"      <P ALIGN=RIGHT STYLE=\"margin-left: 0.2in; margin-right: 0.2in\">\n"
"      %u\n"
"      </P>\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD>\n"
"      <P STYLE=\"margin-left: 0.2in; margin-right: 0in\">\n"
"        Skipped, as expected\n"
"      </P>\n"
"    </TD>\n"
"    <TD>\n"
"      <P ALIGN=RIGHT STYLE=\"margin-left: 0.2in; margin-right: 0.2in\">\n"
"      %u\n"
"      </P>\n"
"    </TD>\n"
"  </TR>\n"
"  <TR>\n"
"    <TD>\n"
"      <P STYLE=\"margin-left: 0.2in; margin-right: 0in\">\n"
"        Skipped unexpectedly\n"
"      </P>\n"
"    </TD>\n"
"    <TD>\n"
"      <P ALIGN=RIGHT STYLE=\"margin-left: 0.2in; margin-right: 0.2in\">\n"
"      %u\n"
"      </P>\n"
"    </TD>\n"
"  </TR>\n"
"</TABLE>\n";

static const char * const trc_tests_stats_start =
"<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=3>\n"
"  <THEAD>\n"
"    <TR>\n"
"      <TD ROWSPAN=2>\n"
"        <P><B>Name</B></P>\n"
"      </TD>\n"
"      <TD ROWSPAN=2>\n"
"        <P><B>Objective</B></P>\n"
"      </TD>\n"
"      <TD COLSPAN=6>\n"
"        <P ALIGN=CENTER><B>Run</B></P>\n"
"      </TD>\n"
"      <TD COLSPAN=3>\n"
"        <P ALIGN=CENTER><B>Not Run</B></P>\n"
"      </TD>\n"
"      <TD ROWSPAN=2>\n"
"        <P><B>Notes</B></P>\n"
"      </TD>\n"
"    </TR>\n"
"    <TR>\n"
"      <TD>\n"
"        <P><B>Total</B></P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P>Passed expect</P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P>Failed expect</P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P>Passed unexp</P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P>Failed unexp</P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P>Aborted, New</P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P><B>Total</B></P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P>Skipped expect</P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P>Skipped unexp</P>\n"
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
"        <P>%s<B><A %s=\"%s%s\">%s</A></B></P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P>%s%s%s%s%s</P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P ALIGN=RIGHT STYLE=\"margin-left: 0.1in; "
"margin-right: 0.1in\">\n"
"          <B>%u</B>\n"
"        </P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P ALIGN=RIGHT STYLE=\"margin-left: 0.1in; "
"margin-right: 0.1in\">\n"
"          %u\n"
"        </P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P ALIGN=RIGHT STYLE=\"margin-left: 0.1in; "
"margin-right: 0.1in\">\n"
"          %u\n"
"        </P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P ALIGN=RIGHT STYLE=\"margin-left: 0.1in; "
"margin-right: 0.1in\">\n"
"          %u\n"
"        </P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P ALIGN=RIGHT STYLE=\"margin-left: 0.1in; "
"margin-right: 0.1in\">\n"
"          %u\n"
"        </P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P ALIGN=RIGHT STYLE=\"margin-left: 0.1in; "
"margin-right: 0.1in\">\n"
"          %u\n"
"        </P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P ALIGN=RIGHT STYLE=\"margin-left: 0.1in; "
"margin-right: 0.1in\">\n"
"          <B>%u</B>\n"
"        </P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P ALIGN=RIGHT STYLE=\"margin-left: 0.1in; "
"margin-right: 0.1in\">\n"
"          %u\n"
"        </P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P ALIGN=RIGHT STYLE=\"margin-left: 0.1in; "
"margin-right: 0.1in\">\n"
"          %u\n"
"        </P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P>%s</P>\n"
"      </TD>\n"
"    </TR>\n";

static const char * const trc_test_exp_got_start =
"<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=3>\n"
"  <THEAD>\n"
"    <TR>\n"
"      <TD>\n"
"        <P><B>Name</B></P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P><B>Parameters</B></P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P><B>Expected</B></P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P><B>Got</B></P>\n"
"      </TD>\n"
"      <TD>"
"        <P><B>Notes</B></P>\n"
"      </TD>\n"
"    </TR>\n"
"  </THEAD>\n"
"  <TBODY>\n";

static const char * const trc_test_exp_got_end =
"  </TBODY>\n"
"</TABLE>\n";

static const char * const trc_test_exp_got_row =
"    <TR>\n"
"      <TD>\n"
"        <P>%s<B><A %s%s%shref=\"#%s\">%s</A></B></P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P>%s</P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P>%s</P>\n"
"      </TD>\n"
"      <TD>\n"
"        <P>%s</P>\n"
"      </TD>\n"
"      <TD>"
"        <P>%s</P>\n"
"      </TD>\n"
"    </TR>\n";


static int
stats_to_html(const trc_stats *stats)
{
    fprintf(f, trc_stats_table,
            TRC_STATS_RUN(stats),
            stats->pass_exp, stats->fail_exp,
            stats->pass_une, stats->fail_une,
            stats->aborted, stats->new_run,
            TRC_STATS_NOT_RUN(stats), stats->skip_exp, stats->skip_une);

    return 0;
}


static int tests_to_html(const test_run *parent, const test_runs *tests,
                         unsigned int level, unsigned int flags);

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

static char trc_args_buf[0x10000];

static const char *
trc_test_args_to_string(const test_args *args)
{
    test_arg *p;
    char     *s = trc_args_buf;

    for (p = args->head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        s += sprintf(s, "%s=%s\n", p->name, p->value);
    }
    return trc_args_buf;
}


static int
iters_to_html(const test_run *test, const test_iters *iters,
              unsigned int level, unsigned int flags)
{
    int         rc;
    test_iter  *p;
    char        level_str[64] = { 0, };
    char       *s;
    unsigned int i;

    for (s = level_str, i = 0; i < level; ++i)
        s += sprintf(s, "*-");

    for (p = iters->head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if ((~flags & TRC_HTML_OUT_STATS) &&
            ((test->type == TRC_TEST_PACKAGE) ||
             (~flags & TRC_HTML_OUT_PACKAGES_ONLY)))
        {
            te_bool name_anchor = (p == iters->head.tqh_first);

            fprintf(f, trc_test_exp_got_row,
                    level_str,
                    name_anchor ? "name=\"" : "",
                    name_anchor ? test->name : "",
                    name_anchor ? "\" " : "",
                    test->obj_link ? : "ERROR",
                    test->name,
                    trc_test_args_to_string(&p->args),
                    trc_test_result_to_string(p->exp_result),
                    trc_test_result_to_string(p->got_result),
                    p->notes ? : "");
        }
        rc = tests_to_html(test, &p->tests, level, flags);
        if (rc != 0)
            return rc;
    }
    return 0;
}

static int
tests_to_html(const test_run *parent, const test_runs *tests,
              unsigned int level, unsigned int flags)
{
    int         rc;
    test_run   *p;
    char        level_str[64] = { 0, };
    char       *s;
    unsigned int i;

    if (level == 0)
    {
        if (flags & TRC_HTML_OUT_STATS)
            WRITE_STR(trc_tests_stats_start);
        else
            WRITE_STR(trc_test_exp_got_start);
    }
    for (s = level_str, i = 0; i < level; ++i)
        s += sprintf(s, "*-");
    for (p = tests->head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if ((flags & TRC_HTML_OUT_STATS) &&
            ((p->type == TRC_TEST_PACKAGE) ||
             (~flags & TRC_HTML_OUT_PACKAGES_ONLY)))
        {
            te_bool name_link;
            char *obj_link = NULL;

            name_link = ((flags & TRC_HTML_OUT_PACKAGES_ONLY) ||
                         ((~flags & TRC_HTML_OUT_PACKAGES_ONLY) &&
                         (p->type == TRC_TEST_SCRIPT)));

            if (p->obj_link == NULL)
            {
                size_t len;

                len = strlen(p->name) + 2 +
                      ((parent == NULL) ? strlen("OBJ") :
                                          strlen(parent->obj_link));
                obj_link = malloc(len);
                if (obj_link == NULL)
                {
                    ERROR("malloc(%u) failed", (unsigned)len);
                    return ENOMEM;
                }
                sprintf(obj_link, "%s-%s",
                        (parent) ? parent->obj_link : "OBJ", p->name);
                p->obj_link = obj_link;
            }

            fprintf(f, trc_tests_stats_row,
                    level_str,
                    name_link ? "href" : "name",
                    name_link ? "#" : "",
                    p->name,
                    p->name,
                    obj_link ? "<A name=\"" : "",
                    obj_link ? : "",
                    obj_link ? "\">": "",
                    p->objective ? : "",
                    obj_link ? "</A>": "",
                    TRC_STATS_RUN(&p->stats),
                    p->stats.pass_exp, p->stats.fail_exp,
                    p->stats.pass_une, p->stats.fail_une,
                    p->stats.aborted + p->stats.new_run,
                    TRC_STATS_NOT_RUN(&p->stats),
                    p->stats.skip_exp, p->stats.skip_une,
                    p->notes ? : "");
        }
        if ((p->type != TRC_TEST_SCRIPT) ||
            (~flags & TRC_HTML_OUT_PACKAGES_ONLY))
        {
            rc = iters_to_html(p, &p->iters, level + 1, flags);
            if (rc != 0)
                goto cleanup;
        }
    }
    if (level == 0)
    {
        if (flags & TRC_HTML_OUT_STATS)
            WRITE_STR(trc_tests_stats_end);
        else
            WRITE_STR(trc_test_exp_got_end);
    }
    return 0;

cleanup:
    return rc;
}

/** See descriptino in trc_db.h */
int
trc_report_to_html(const char *filename, trc_database *db,
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

    if (flags & TRC_OUT_TOTAL_STATS)
    {
        /* Grand total */
        rc = stats_to_html(&db->stats);
        if (rc != 0)
            goto cleanup;
    }

    if (flags & TRC_OUT_PACKAGES_ONLY_STATS)
    {
        /* Report for packages */
        rc = tests_to_html(NULL, &db->tests, 0,
                 TRC_HTML_OUT_PACKAGES_ONLY | TRC_HTML_OUT_STATS);
        if (rc != 0)
            goto cleanup;
    }

    if (flags & TRC_OUT_FULL_STATS)
    {
        /* Report with iterations of packages and w/o iterations of tests */
        rc = tests_to_html(NULL, &db->tests, 0, TRC_HTML_OUT_STATS);
        if (rc != 0)
            goto cleanup;
    }

    if (flags & TRC_OUT_FULL)
    {
        /* Full report */
        rc = tests_to_html(NULL, &db->tests, 0, 0);
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
