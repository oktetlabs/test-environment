/** @file
 * @brief Testing Results Comparator
 *
 * Generator of two set of tags comparison report in HTML format.
 *
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 * 
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
 *  
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
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

#include "trc_diff.h"


/** Generate brief version of the diff report */
#define TRC_DIFF_BRIEF      0x01

#define WRITE_STR(str) \
    do {                                                \
        fflush(f);                                      \
        if (fwrite(str, strlen(str), 1, f) != 1)        \
        {                                               \
            rc = errno ? : TE_EIO;                      \
            ERROR("Writing to the file failed");        \
            goto cleanup;                               \
        }                                               \
    } while (0)

#define PRINT_STR(str_)  (((str_) != NULL) ? (str_) : "")


static const char * const trc_diff_html_title_def =
    "Testing Results Expectations Differences Report";

static const char * const trc_diff_html_doc_start =
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n"
"<HTML>\n"
"<HEAD>\n"
"  <META HTTP-EQUIV=\"CONTENT-TYPE\" CONTENT=\"text/html; "
"charset=utf-8\">\n"
"  <TITLE>%s</TITLE>\n"
"  <style type=\"text/css\">\n"
"    .S {font-weight: bold; color: green; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .U {font-weight: bold; color: red; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .E {font-weight: italic; color: blue; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"  </style>\n"
"</HEAD>\n"
"<BODY LANG=\"en-US\" DIR=\"LTR\">\n"
"<H1 ALIGN=CENTER>%s</H1>\n"
"<H2 ALIGN=CENTER>%s</H2>\n";

static const char * const trc_diff_html_doc_end =
"</BODY>\n"
"</HTML>\n";


static const char * const trc_diff_stats_table =
"<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=3>\n"
"  <THEAD>\n"
"    <TR>\n"
"      <TD ROWSPAN=2>\n"
"        <B>%s</B>\n"
"      </TD>\n"
"      <TD COLSPAN=4 ALIGN=CENTER>\n"
"        <B>%s</B>\n"
"      </TD>\n"
"    </TR>\n"
"    <TR>\n"
"      <TD ALIGN=CENTER><B>%s</B></TD>\n"
"      <TD ALIGN=CENTER><B>%s</B></TD>\n"
"      <TD ALIGN=CENTER><B>%s</B></TD>\n"
"      <TD ALIGN=CENTER><B>%s</B></TD>\n"
"    </TR>\n"
"  </THEAD>\n"
"  <TBODY ALIGN=RIGHT>\n"
"    <TR>\n"
"      <TD ALIGN=LEFT><B>%s</B></TD>\n"
"      <TD><FONT class=\"S\">%u</FONT>+<FONT class=\"U\">%u</FONT>+"
          "<FONT class=\"E\">%u</FONT></TD>\n"
"      <TD><FONT class=\"U\">%u</FONT>+<FONT class=\"E\">%u</FONT></TD>\n"
"      <TD><FONT class=\"U\">%u</FONT>+<FONT class=\"E\">%u</FONT></TD>\n"
"      <TD><FONT class=\"U\">%u</FONT>+<FONT class=\"E\">%u</FONT></TD>\n"
"    </TR>\n"
"    <TR>\n"
"      <TD ALIGN=LEFT><B>%s</B></TD>\n"
"      <TD><FONT class=\"U\">%u</FONT>+<FONT class=\"E\">%u</FONT></TD>\n"
"      <TD><FONT class=\"S\">%u</FONT>+<FONT class=\"U\">%u</FONT>+"
          "<FONT class=\"E\">%u</FONT></TD>\n"
"      <TD><FONT class=\"U\">%u</FONT>+<FONT class=\"E\">%u</FONT></TD>\n"
"      <TD><FONT class=\"U\">%u</FONT>+<FONT class=\"E\">%u</FONT></TD>\n"
"    </TR>\n"
"    <TR>\n"
"      <TD ALIGN=LEFT><B>%s</B></TD>\n"
"      <TD><FONT class=\"U\">%u</FONT>+<FONT class=\"E\">%u</FONT></TD>\n"
"      <TD><FONT class=\"U\">%u</FONT>+<FONT class=\"E\">%u</FONT></TD>\n"
"      <TD><FONT class=\"S\">%u</FONT></TD>\n"
"      <TD><FONT class=\"U\">%u</FONT></TD>\n"
"    </TR>\n"
"    <TR>\n"
"      <TD ALIGN=LEFT><B>%s</B></TD>\n"
"      <TD><FONT class=\"U\">%u</FONT>+<FONT class=\"E\">%u</FONT></TD>\n"
"      <TD><FONT class=\"U\">%u</FONT>+<FONT class=\"E\">%u</FONT></TD>\n"
"      <TD><FONT class=\"U\">%u</FONT></TD>\n"
"      <TD><FONT class=\"U\">%u</FONT></TD>\n"
"    </TR>\n"
"    <TR>\n"
"      <TD ALIGN=LEFT COLSPAN=5><H3>Total run: <FONT class=\"S\">%u</FONT>+"
          "<FONT class=\"U\">%u</FONT>+<FONT class=\"E\">%u</FONT>=%u"
          "</H3></TD>"
"    </TR>\n"
"    <TR>\n"
"      <TD ALIGN=LEFT COLSPAN=5>[<FONT class=\"S\">X</FONT>+]"
                                "<FONT class=\"U\">Y</FONT>+"
                                "<FONT class=\"E\">Z</FONT><BR/>"
"X - result match, Y - result does not match (to be fixed), "
"Z - result does not match (excluded)</TD>"
"    </TR>\n"
"  </TBODY>\n"
"</TABLE>\n";


static const char * const trc_diff_key_table_heading =
"<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=3>\n"
"  <THEAD>\n"
"    <TR>\n"
"      <TD>\n"
"        <B>Key</B>\n"
"      </TD>\n"
"      <TD>\n"
"        <B>Number of caused differences</B>\n"
"      </TD>\n"
"    </TR>\n"
"  </THEAD>\n"
"  <TBODY>\n";

static const char * const trc_diff_key_table_row =
"    <TR>\n"
"      <TD>%s</TD>\n"
"      <TD ALIGN=RIGHT>%u</TD>\n"
"    </TR>\n";


static const char * const trc_diff_full_table_heading_start =
"<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=3>\n"
"  <THEAD>\n"
"    <TR>\n"
"      <TD>\n"
"        <B>Name</B>\n"
"      </TD>\n"
"      <TD>\n"
"        <B>Objective</B>\n"
"      </TD>\n";

static const char * const trc_diff_brief_table_heading_start =
"<TABLE BORDER=1 CELLPADDING=4 CELLSPACING=3>\n"
"  <THEAD>\n"
"    <TR>\n"
"      <TD>\n"
"        <B>Name</B>\n"
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
"        <B>Key</B>\n"
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

static const char * const trc_diff_full_table_test_row_start =
"    <TR>\n"
"      <TD><A name=\"%s=0\"/>%s<B>%s</B></TD>\n"  /* Name */
"      <TD>%s</TD>\n";          /* Objective */

static const char * const trc_diff_brief_table_test_row_start =
"    <TR>\n"
"      <TD><A href=\"#%s=%u\">%s</A></TD>\n";  /* Name */

static const char * const trc_diff_table_iter_row_start =
"    <TR>\n"
"      <TD COLSPAN=2><A name=\"%s=%u\"/>%s</TD>\n"; /* Parameters */

static const char * const trc_diff_table_row_end =
"      <TD>%s</TD>\n"           /* Key */
"      <TD>%s</TD>\n"           /* Notes */
"    </TR>\n";

static const char * const trc_diff_table_row_col_start =
"      <TD>";

static const char * const trc_diff_table_row_col_end =
"</TD>\n";


static FILE *f;
static int   fd;

static char test_name[1024];
static char trc_args_buf[0x10000];


static te_bool trc_diff_tests_has_diff(test_runs *tests,
                                       unsigned int flags);

static int trc_diff_tests_to_html(const test_runs *tests,
                                  unsigned int flags,
                                  unsigned int level);


/** Sort list of keys by 'count' in decreasing order. */
static void
trc_diff_keys_sort(void)
{
    trc_diff_key_stats *curr, *prev, *p;

    assert(keys_stats.cqh_first != (void *)&keys_stats);
    for (prev = keys_stats.cqh_first;
         (curr = prev->links.cqe_next) != (void *)&keys_stats;
         prev = curr)
    {
        if (prev->count < curr->count)
        {
            CIRCLEQ_REMOVE(&keys_stats, curr, links);
            for (p = keys_stats.cqh_first;
                 p != prev && p->count >= curr->count;
                 p = p->links.cqe_next);

            /* count have to fire */
            assert(p->count < curr->count);
            CIRCLEQ_INSERT_BEFORE(&keys_stats, p, curr, links);
        }
    }
}

static int
trc_diff_key_to_html(void)
{
    int                 rc = 0;
    trc_diff_key_stats *p;

    if (keys_stats.cqh_first == (void *)&keys_stats)
        return 0;

    trc_diff_keys_sort();

    WRITE_STR(trc_diff_key_table_heading);
    for (p = keys_stats.cqh_first;
         p != (void *)&keys_stats;
         p = p->links.cqe_next)
    {
        if (p->tags->show_keys)
        {
            fprintf(f, trc_diff_key_table_row,
                    p->key, p->count);
        }
    }
    WRITE_STR(trc_diff_table_end);

cleanup:
    return rc;
}


static te_bool
trc_diff_exclude_by_key(const test_iter *iter)
{
    tqe_string     *p;
    trc_tags_entry *tags;
    te_bool         exclude = FALSE;

    for (p = trc_diff_exclude_keys.tqh_first;
         p != NULL && !exclude;
         p = p->links.tqe_next)
    {
        for (tags = tags_diff.tqh_first;
             tags != NULL;
             tags = tags->links.tqe_next)
        {
            if (iter->diff_exp[tags->id].key != NULL &&
                strlen(iter->diff_exp[tags->id].key) > 0)
            {
                exclude = (strncmp(iter->diff_exp[tags->id].key,
                                   p->v, strlen(p->v)) == 0);
                if (!exclude)
                    break;
            }
        }
    }
    return exclude;
}


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



/**
 * Convert test arguments to string for HTML report.
 *
 * @param args          Arguments
 *
 * @result Pointer to static buffer with arguments in string
 *         representation.
 */
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

/**
 * String representation (with HTML markers) of test iteration keys
 * for all tags.
 *
 * @param iter          Test iteration
 *
 * @return Pointer to a static buffer with HTML text string.
 */
static const char *
trc_diff_test_iter_keys(const test_iter *iter)
{
    static char buf[0x1000];

    char           *s = buf;
    trc_tags_entry *tags;

    buf[0] = '\0';
    for (tags = tags_diff.tqh_first;
         tags != NULL;
         tags = tags->links.tqe_next)
    {
        if (iter->diff_exp[tags->id].key != NULL &&
            strlen(iter->diff_exp[tags->id].key) > 0)
        {
            s += sprintf(s, "<EM>%s</EM> - %s<BR/>",
                         tags->name, iter->diff_exp[tags->id].key);
        }
    }

    return buf;
}

/**
 * String representation (with HTML markers) of test iteration notes
 * for all tags.
 *
 * @param iter          Test iteration
 *
 * @return Pointer to a static buffer with HTML text string.
 */
static const char *
trc_diff_test_iter_notes(const test_iter *iter)
{
    static char buf[0x1000];

    char           *s = buf;
    trc_tags_entry *tags;

    buf[0] = '\0';
    if (iter->notes != NULL && strlen(iter->notes) > 0)
    {
        s += sprintf(s, "%s<BR/>", iter->notes);
    }
    for (tags = tags_diff.tqh_first;
         tags != NULL;
         tags = tags->links.tqe_next)
    {
        if (iter->diff_exp[tags->id].notes != NULL &&
            strlen(iter->diff_exp[tags->id].notes) > 0)
        {
            s += sprintf(s, "<EM>%s</EM> - %s<BR/>",
                         tags->name, iter->diff_exp[tags->id].notes);
        }
    }

    return buf;
}

/**
 * Output test iterations differencies into a file @a f (global
 * variable).
 *
 * @param iters         Test iterations
 * @param flags         Processing flags
 * @param level         Level of this test in whole test suite
 */
static int
trc_diff_iters_to_html(const test_iters *iters, unsigned int flags,
                       unsigned int level)
{
    int             rc;
    unsigned int    i;
    trc_tags_entry *entry;
    test_iter      *p;
    test_iter      *q;
    te_bool         one_iter = (iters->head.tqh_first != NULL) &&
                               (&iters->head.tqh_first->links.tqe_next ==
                                iters->head.tqh_last);

    for (p = iters->head.tqh_first, i = 1;
         p != NULL;
         p = p->links.tqe_next, ++i)
    {
        if (p->output)
        {
            /*
             * Don't want to see iteration parameters, if iteration is
             * only one.
             */
            if (!one_iter &&
                ((~flags & TRC_DIFF_BRIEF) ||
                 (p->tests.head.tqh_first == NULL)))
            {
                p->diff_keys = strdup(trc_diff_test_iter_keys(p));
                assert(p->diff_keys != NULL);

                if (flags & TRC_DIFF_BRIEF)
                {
                    /* 
                     * We are in the brief mode, therefore, it is
                     * a test script (not session or package) iteration.
                     * We don't want to see iterations with equal
                     * expectations and keys.
                     */
                    for (q = iters->head.tqh_first;
                         (q != p) &&
                         (!q->output ||
                          strcmp(p->diff_keys, q->diff_keys) != 0 ||
                          !trc_diff_expectations_equal(p, q));
                         q = q->links.tqe_next);

                    if (p != q)
                        continue;

                    fprintf(f, trc_diff_brief_table_test_row_start,
                            test_name, i, test_name);
                }
                else
                {
                    fprintf(f, trc_diff_table_iter_row_start,
                            test_name, i,
                            trc_test_args_to_string(&p->args));
                }
                for (entry = tags_diff.tqh_first;
                     entry != NULL;
                     entry = entry->links.tqe_next)
                {
                    fputs(trc_diff_table_row_col_start, f);
                    fputs(trc_test_result_to_string(
                              p->diff_exp[entry->id].value), f);
                    trc_verdicts_to_html(f,
                        &p->diff_exp[entry->id].verdicts);
                    fputs(trc_diff_table_row_col_end, f);
                }
                fprintf(f, trc_diff_table_row_end,
                        p->diff_keys,
                        trc_diff_test_iter_notes(p));
            }

            rc = trc_diff_tests_to_html(&p->tests, flags, level + 1);
            if (rc != 0)
                return rc;
        }
    }

    return 0;
}

static const char *
trc_diff_test_iters_get_keys(test_run *test, unsigned int id)
{
    static char buf[0x10000];

    test_iter  *p;
    test_iter  *q;
    char       *s = buf;


    buf[0] = '\0';
    for (p = test->iters.head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (p->output && (p->diff_exp[id].key != NULL) &&
            (strlen(p->diff_exp[id].key) > 0))
        {
            for (q = test->iters.head.tqh_first;
                 (q != p) &&
                 ((q->diff_exp[id].key == NULL) || (!q->output) ||
                  (strcmp(p->diff_exp[id].key, q->diff_exp[id].key) != 0));
                 q = q->links.tqe_next);

            if (p == q)
                s += sprintf(s, "%s%s", (s == buf) ? "" : ", ",
                             p->diff_exp[id].key);
        }
    }

    return buf;
}

/**
 * Output tests differencies into a file @a f (global variable).
 *
 * @param tests         Tests
 * @param flags         Processing flags
 * @param level         Level of this test in whole test suite
 */
static int
trc_diff_tests_to_html(const test_runs *tests, unsigned int flags,
                       unsigned int level)
{
    static char buf[0x10000];

    int             rc = 0;
    test_run       *p;
    trc_tags_entry *entry;
    char            level_str[64] = { 0, };
    char           *s;
    unsigned int    i;
    size_t          parent_len;
    const char     *keys;


    if (level == 0)
    {
        test_name[0] = '\0';

        if (flags & TRC_DIFF_BRIEF)
        {
            WRITE_STR(trc_diff_brief_table_heading_start);
        }
        else
        {
            WRITE_STR(trc_diff_full_table_heading_start);
        }

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
    else
    {
        strcat(test_name, "/");
    }

    parent_len = strlen(test_name);

    if (~flags & TRC_DIFF_BRIEF)
        for (s = level_str, i = 0; i < level; ++i)
            s += sprintf(s, "*-");

    for (p = tests->head.tqh_first; p != NULL; p = p->links.tqe_next)
    {
        test_name[parent_len] = '\0';
        strcat(test_name, p->name);

        if (p->diff_out &&
            ((~flags & TRC_DIFF_BRIEF) ||
             ((p->type == TRC_TEST_SCRIPT) && (!p->diff_out_iters))))
        {
            if (flags & TRC_DIFF_BRIEF)
            {
                fprintf(f, trc_diff_brief_table_test_row_start,
                        test_name, 0, test_name);
            }
            else
            {
                fprintf(f, trc_diff_full_table_test_row_start,
                        test_name, level_str, p->name,
                        PRINT_STR(p->objective));
            }

            buf[0] = '\0';
            s = buf;
            for (entry = tags_diff.tqh_first;
                 entry != NULL;
                 entry = entry->links.tqe_next)
            {
                fputs(trc_diff_table_row_col_start, f);
                fputs(trc_test_result_to_string(
                          p->diff_exp[entry->id]), f);
                trc_verdicts_to_html(f, p->diff_verdicts[entry->id]);
                fputs(trc_diff_table_row_col_end, f);

                keys = trc_diff_test_iters_get_keys(p, entry->id);
                if (strlen(keys) > 0)
                {
                    s += sprintf(s, "<EM>%s</EM> - %s<BR/>",
                                 entry->name, keys);
                }
            }
            fprintf(f, trc_diff_table_row_end, buf, PRINT_STR(p->notes));
        }
        if (p->diff_out_iters)
        {
            rc = trc_diff_iters_to_html(&p->iters, flags, level);
            if (rc != 0)
                return rc;
        }
    }

    if (level == 0)
    {
        WRITE_STR(trc_diff_table_end);

        test_name[parent_len] = '\0';
    }
    else
    {
        test_name[parent_len - 1] = '\0';
    }

cleanup:
    return rc;
}


/**
 * Output set of tags used for comparison to HTML report.
 *
 * @param tags_list     List of tags
 */
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
                strcmp(tag->v, "result") != 0)
            {
                fprintf(f, " %s", tag->v);
            }
        }
        fprintf(f, "<BR/><BR/>");
    }
}


/**
 * Output statistics for one comparison to HTML report.
 *
 * @param tags_x        The first set of tags
 * @param tags_y        The second set of tags
 * @param flags         Processing flags
 */
static void
trc_diff_one_stats_to_html(const trc_tags_entry *tags_x,
                           const trc_tags_entry *tags_y,
                           unsigned int flags)
{
    trc_diff_stats_counters *counters = &stats[tags_x->id][tags_y->id - 1];
    trc_diff_stats_counter   total_match;
    trc_diff_stats_counter   total_no_match;
    trc_diff_stats_counter   total_excluded;
    trc_diff_stats_counter   total;

    UNUSED(flags);

    total_match =
        (*counters)[TRC_DIFF_STATS_PASSED][TRC_DIFF_STATS_PASSED] +
        (*counters)[TRC_DIFF_STATS_FAILED][TRC_DIFF_STATS_FAILED];
    total_no_match =
        (*counters)[TRC_DIFF_STATS_PASSED_DIFF]
                   [TRC_DIFF_STATS_PASSED_DIFF] +
        (*counters)[TRC_DIFF_STATS_PASSED_DIFF]
                   [TRC_DIFF_STATS_FAILED_DIFF] +
        (*counters)[TRC_DIFF_STATS_FAILED_DIFF]
                   [TRC_DIFF_STATS_PASSED_DIFF] +
        (*counters)[TRC_DIFF_STATS_FAILED_DIFF]
                   [TRC_DIFF_STATS_FAILED_DIFF];
    total_excluded =
        (*counters)[TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE]
                   [TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE] +
        (*counters)[TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE]
                   [TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE] +
        (*counters)[TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE]
                   [TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE] +
        (*counters)[TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE]
                   [TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE];
    total = total_match + total_no_match + total_excluded;

    fprintf(f, trc_diff_stats_table,
            tags_x->name, tags_y->name,
            "PASSED", "FAILED", "SKIPPED", "other",
            "PASSED",
            (*counters)[TRC_DIFF_STATS_PASSED][TRC_DIFF_STATS_PASSED],
            (*counters)[TRC_DIFF_STATS_PASSED_DIFF]
                       [TRC_DIFF_STATS_PASSED_DIFF],
            (*counters)[TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE]
                       [TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE],
            (*counters)[TRC_DIFF_STATS_PASSED_DIFF]
                       [TRC_DIFF_STATS_FAILED_DIFF],
            (*counters)[TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE]
                       [TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE],
            (*counters)[TRC_DIFF_STATS_PASSED_DIFF]
                       [TRC_DIFF_STATS_SKIPPED],
            (*counters)[TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE]
                       [TRC_DIFF_STATS_SKIPPED],
            (*counters)[TRC_DIFF_STATS_PASSED_DIFF]
                       [TRC_DIFF_STATS_OTHER],
            (*counters)[TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE]
                       [TRC_DIFF_STATS_OTHER],
            "FAILED",
            (*counters)[TRC_DIFF_STATS_FAILED_DIFF]
                       [TRC_DIFF_STATS_PASSED_DIFF],
            (*counters)[TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE]
                       [TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE],
            (*counters)[TRC_DIFF_STATS_FAILED][TRC_DIFF_STATS_FAILED],
            (*counters)[TRC_DIFF_STATS_FAILED_DIFF]
                       [TRC_DIFF_STATS_FAILED_DIFF],
            (*counters)[TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE]
                       [TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE],
            (*counters)[TRC_DIFF_STATS_FAILED_DIFF]
                       [TRC_DIFF_STATS_SKIPPED],
            (*counters)[TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE]
                       [TRC_DIFF_STATS_SKIPPED],
            (*counters)[TRC_DIFF_STATS_FAILED_DIFF]
                       [TRC_DIFF_STATS_OTHER],
            (*counters)[TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE]
                       [TRC_DIFF_STATS_OTHER],
            "SKIPPED",
            (*counters)[TRC_DIFF_STATS_SKIPPED]
                       [TRC_DIFF_STATS_PASSED_DIFF],
            (*counters)[TRC_DIFF_STATS_SKIPPED]
                       [TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE],
            (*counters)[TRC_DIFF_STATS_SKIPPED]
                       [TRC_DIFF_STATS_FAILED_DIFF],
            (*counters)[TRC_DIFF_STATS_SKIPPED]
                       [TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE],
            (*counters)[TRC_DIFF_STATS_SKIPPED][TRC_DIFF_STATS_SKIPPED],
            (*counters)[TRC_DIFF_STATS_SKIPPED][TRC_DIFF_STATS_OTHER],
            "other",
            (*counters)[TRC_DIFF_STATS_OTHER]
                       [TRC_DIFF_STATS_PASSED_DIFF],
            (*counters)[TRC_DIFF_STATS_OTHER]
                       [TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE],
            (*counters)[TRC_DIFF_STATS_OTHER]
                       [TRC_DIFF_STATS_FAILED_DIFF],
            (*counters)[TRC_DIFF_STATS_OTHER]
                       [TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE],
            (*counters)[TRC_DIFF_STATS_OTHER][TRC_DIFF_STATS_SKIPPED],
            (*counters)[TRC_DIFF_STATS_OTHER][TRC_DIFF_STATS_OTHER],
            total_match, total_no_match, total_excluded, total);
}

/**
 * Output all statistics to HTML report
 *
 * @param flags         Processing flags
 */
static void
trc_diff_stats_to_html(unsigned int flags)
{
    const trc_tags_entry *tags_i;
    const trc_tags_entry *tags_j;

    for (tags_i = tags_diff.tqh_first;
         tags_i != NULL;
         tags_i = tags_i->links.tqe_next)
    {
        for (tags_j = tags_diff.tqh_first;
             tags_j != NULL;
             tags_j = tags_j->links.tqe_next)
        {
            if (tags_i->id < tags_j->id)
                trc_diff_one_stats_to_html(tags_i, tags_j, flags);
        }
    }
}
    
/** See descriptino in trc_db.h */
te_errno
trc_diff_report_to_html(trc_diff_ctx *ctx, const char *filename,
                        const char *title)
{
    int     rc;
    te_bool has_diff;

    if (filename == NULL)
    {
        f = stdout;
        fd = STDOUT_FILENO;
    }
    else
    {
        f = fopen(filename, "w");
        if (f == NULL)
        {
            ERROR("Failed to open file to write HTML report to");
            return te_rc_os2te(errno);
        }
        fd = fileno(f);
        if (fd < 0)
        {
            rc = errno;
            ERROR("Failed to get descriptor of output file");
            goto cleanup;
        }
    }

    /* HTML header */
    fprintf(f, trc_diff_html_doc_start,
            (title != NULL) ? title : trc_diff_html_title_def,
            (title != NULL) ? title : trc_diff_html_title_def,
            db->version);

    /* Compared sets */
    trc_diff_tags_to_html(&tags_diff);

    /* Output statistics */
    trc_diff_stats_to_html(flags);

    /* Output per-key summary */
    trc_diff_key_to_html();

    /* Initial test name is empty */
    test_name[0] = '\0';
    
    /* Report */
    if (has_diff &&
        ((rc = trc_diff_tests_to_html(&db->tests,
                                      flags | TRC_DIFF_BRIEF, 0)) != 0 ||
         (rc = trc_diff_tests_to_html(&db->tests, flags, 0)) != 0))
    {
        goto cleanup;
    }

    /* HTML footer */
    WRITE_STR(trc_diff_html_doc_end);

    if (filename != NULL)
        fclose(f);

    return 0;

cleanup:
    if (filename != NULL)
    {
        fclose(f);
        unlink(filename);
    }
    return rc;
}
