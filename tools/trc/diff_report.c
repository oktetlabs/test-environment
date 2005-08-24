/** @file
 * @brief Testing Results Comparator
 *
 * Generator of two set of tags comparison report in HTML format.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
#include "trc_tag.h"
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


static const char * const trc_diff_html_doc_start =
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n"
"<HTML>\n"
"<HEAD>\n"
"  <META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; "
"charset=utf-8\">\n"
"  <TITLE>Testing Results Expectations Differences Report</TITLE>\n"
"</HEAD>\n"
"<BODY LANG=\"en-US\" DIR=\"LTR\">\n"
"<H1 ALIGN=CENTER>Testing Results Expectations Differences Report</H1>\n"
"<H2 ALIGN=CENTER>%s</H2>\n";

static const char * const trc_diff_html_doc_end =
"</BODY>\n"
"</HTML>\n";

static const char * const trc_diff_table_heading_start =
"<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=3>\n"
"  <THEAD>\n"
"    <TR>\n"
"      <TD>\n"
"        <B>Name</B>\n"
"      </TD>\n"
"      <TD>\n"
"        <B>Objective</B>\n"
"      </TD>\n";

static const char * const trc_diff_table_heading_named_entry =
"      <TD>"
"        <B>%s</B>\n"
"      </TD>\n";

static const char * const trc_diff_table_heading_unnamed_entry =
"      <TD>"
"        <B>Set %d</B>\n"
"      </TD>\n";

static const char * const trc_diff_table_heading_end =
"      <TD>"
"        <B>BugID</B>\n"
"      </TD>\n"
"      <TD>"
"        <B>Notes</B>\n"
"      </TD>\n"
"    </TR>\n"
"  </THEAD>\n"
"  <TBODY>\n";

static const char * const trc_diff_table_end =
"  </TBODY>\n"
"</TABLE>\n";

static const char * const trc_diff_table_test_row_start =
"    <TR>\n"
"      <TD>%s<B>%s</B></TD>\n"  /* Name */
"      <TD>%s</TD>\n";          /* Objective */

static const char * const trc_diff_table_iter_row_start =
"    <TR>\n"
"      <TD COLSPAN=2>%s</TD>\n"; /* Parameters */

static const char * const trc_diff_table_row_end =
"      <TD>%s</TD>\n"           /* BugID */
"      <TD>%s</TD>\n"           /* Notes */
"    </TR>\n";

static const char * const trc_diff_table_row_col =
"      <TD>%s</TD>\n";


static te_bool trc_diff_tests_has_diff(test_runs *tests);

static int trc_diff_tests_to_html(const test_runs *tests,
                                  unsigned int level);


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
        case TRC_TEST_MIXED:
            return "(see iters)";
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
        s += sprintf(s, "%s=%s<BR/>", p->name, p->value);
    }
    return trc_args_buf;
}


static te_bool
trc_diff_iters_has_diff(test_iters *iters, te_bool *all_out,
                        trc_test_result *diff_exp)
{
    te_bool         has_diff;
    te_bool         iter_has_diff;
    trc_test_result iter_result;
    te_bool         has_no_out;
    trc_tags_entry *entry;
    test_iter      *p;

    for (has_diff = FALSE, has_no_out = FALSE, p = iters->head.tqh_first;
         p != NULL;
         p = p->links.tqe_next)
    {
        iter_has_diff = FALSE;
        iter_result = TRC_TEST_UNSET;

        for (entry = tags_diff.tqh_first;
             entry != NULL;
             entry = entry->links.tqe_next)
        {
            if (diff_exp[entry->id] == TRC_TEST_UNSET)
                diff_exp[entry->id] = p->diff_exp[entry->id];
            else if (diff_exp[entry->id] != p->diff_exp[entry->id])
                diff_exp[entry->id] = TRC_TEST_MIXED;

            if (iter_result == TRC_TEST_UNSET)
                iter_result = p->diff_exp[entry->id];
            else if (iter_result != p->diff_exp[entry->id])
                iter_has_diff = TRUE;
        }

        /* The routine should be called first to be called in any case */
        p->diff_out = trc_diff_tests_has_diff(&p->tests) || iter_has_diff;

        if (!p->diff_out)
            has_no_out = TRUE;

        has_diff = has_diff || p->diff_out;
    }

    *all_out = has_diff && !has_no_out;

    return has_diff;
}

static te_bool
trc_diff_tests_has_diff(test_runs *tests)
{
    trc_tags_entry *entry;
    test_run       *p;
    te_bool         has_diff;
    te_bool         all_iters_out;

    for (has_diff = FALSE, p = tests->head.tqh_first;
         p != NULL;
         p = p->links.tqe_next)
    {
        for (entry = tags_diff.tqh_first;
             entry != NULL;
             entry = entry->links.tqe_next)
        {
            p->diff_exp[entry->id] = TRC_TEST_UNSET;
        }
        
        p->diff_out = trc_diff_iters_has_diff(&p->iters, &all_iters_out,
                                              p->diff_exp);

        p->diff_out_iters = p->diff_out &&
            (p->iters.head.tqh_first == NULL ||
             !all_iters_out ||
             p->iters.head.tqh_first->tests.head.tqh_first != NULL);

        has_diff = has_diff || p->diff_out;
    }

    return has_diff;
}


static int
trc_diff_iters_to_html(const test_iters *iters, unsigned int level)
{
    int             rc;
    trc_tags_entry *entry;
    test_iter      *p;
    te_bool         one_iter = (iters->head.tqh_first != NULL) &&
                               (&iters->head.tqh_first->links.tqe_next ==
                                iters->head.tqh_last);

    for (p = iters->head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (p->diff_out)
        {
            if (!one_iter)
            {
                fprintf(f, trc_diff_table_iter_row_start,
                        trc_test_args_to_string(&p->args));
                for (entry = tags_diff.tqh_first;
                     entry != NULL;
                     entry = entry->links.tqe_next)
                {
                    fprintf(f, trc_diff_table_row_col,
                            trc_test_result_to_string(
                                p->diff_exp[entry->id]));
                }
                fprintf(f, trc_diff_table_row_end,
                        PRINT_STR(p->bug), PRINT_STR(p->notes));
            }

            rc = trc_diff_tests_to_html(&p->tests, level + 1);
            if (rc != 0)
                return rc;
        }
    }

    return 0;
}

static int
trc_diff_tests_to_html(const test_runs *tests, unsigned int level)
{
    int             rc = 0;
    test_run       *p;
    trc_tags_entry *entry;
    char            level_str[64] = { 0, };
    char           *s;
    unsigned int    i;

    if (level == 0)
    {
        WRITE_STR(trc_diff_table_heading_start);
        for (entry = tags_diff.tqh_first;
             entry != NULL;
             entry = entry->links.tqe_next)
        {
            if (entry->name != NULL)
                fprintf(f, trc_diff_table_heading_named_entry, entry->name);
            else
                fprintf(f, trc_diff_table_heading_unnamed_entry, entry->id);
        }
        WRITE_STR(trc_diff_table_heading_end);
    }

    for (s = level_str, i = 0; i < level; ++i)
        s += sprintf(s, "*-");

    for (p = tests->head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (p->diff_out)
        {
            fprintf(f, trc_diff_table_test_row_start,
                    level_str, p->name, PRINT_STR(p->objective));
            for (entry = tags_diff.tqh_first;
                 entry != NULL;
                 entry = entry->links.tqe_next)
            {
                fprintf(f, trc_diff_table_row_col,
                        trc_test_result_to_string(p->diff_exp[entry->id]));
            }
            fprintf(f, trc_diff_table_row_end,
                    PRINT_STR(p->bug), PRINT_STR(p->notes));
        }
        if (p->diff_out_iters)
        {
            rc = trc_diff_iters_to_html(&p->iters, level);
            if (rc != 0)
                return rc;
        }
    }

    if (level == 0)
        WRITE_STR(trc_diff_table_end);

cleanup:
    return rc;
}


void
trc_diff_tags_to_html(const trc_tags_list *tags_list)
{
    const trc_tags_entry   *p;
    const trc_tag          *tag;

    for (p = tags_list->tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (p->name != NULL)
            fprintf(f, "<B>%s: </B>", p->name);
        else
            fprintf(f, "<B>Set %d: </B>", p->id);

        for (tag = p->tags.tqh_first;
             tag != NULL;
             tag = tag->links.tqe_next)
        {
            if (tag->links.tqe_next != NULL ||
                strcmp(tag->name, "result") != 0)
            {
                fprintf(f, " %s", tag->name);
            }
        }
        fprintf(f, "<BR/><BR/>");
    }
}

    
/** See descriptino in trc_db.h */
int
trc_diff_report_to_html(const char *filename, trc_database *db)
{
    int rc;

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
    fprintf(f, trc_diff_html_doc_start, db->version);

    /* Compared sets */
    trc_diff_tags_to_html(&tags_diff);
    
    /* Report */
    if (trc_diff_tests_has_diff(&db->tests) &&
        (rc = trc_diff_tests_to_html(&db->tests, 0)) != 0)
    {
        goto cleanup;
    }

    /* HTML footer */
    WRITE_STR(trc_diff_html_doc_end);

    fclose(f);
    return 0;

cleanup:
    fclose(f);
    unlink(filename);
    return rc;
}
