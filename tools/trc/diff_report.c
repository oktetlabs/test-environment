/** @file
 * @brief Testing Results Comparator
 *
 * Generator of two set of tags comparison report in HTML format.
 *
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
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

/** Title of the report in HTML format */
extern char *trc_diff_title;
/** Template of keys to be excluded */
extern lh_string trc_diff_exclude_keys;


/**
 * Types of statistics collected per set X vs set Y.
 */
typedef enum trc_diff_stats_index {
    TRC_DIFF_STATS_PASSED = 0,
    TRC_DIFF_STATS_PASSED_DIFF,
    TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE,
    TRC_DIFF_STATS_FAILED,
    TRC_DIFF_STATS_FAILED_DIFF,
    TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE,
    TRC_DIFF_STATS_SKIPPED,
    TRC_DIFF_STATS_OTHER,

    TRC_DIFF_STATS_MAX
} trc_diff_stats_index;


/** Type of simple counter. */
typedef unsigned int trc_diff_stats_counter;

/**
 * Set X vs set Y statistics are two dimension array of simple
 * counters. Indices are the results of the corresponding set together
 * with equal/different knowledge, when main result is the same.
 */
typedef trc_diff_stats_counter trc_diff_stats_counters[TRC_DIFF_STATS_MAX]
                                                      [TRC_DIFF_STATS_MAX];

/**
 * TRC differencies statistics are two dimension array of statistics per
 * set X vs set Y statistics.
 * 
 * A half of this array is used in fact (the first index is always
 * greater than the second one).
 */
typedef trc_diff_stats_counters trc_diff_stats[TRC_DIFF_IDS]
                                              [TRC_DIFF_IDS - 1];


static FILE            *f;
static int              fd;
static trc_diff_stats   stats;


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
"      <TD ALIGN=LEFT COLSPAN=5>Total run: <FONT class=\"S\">%u</FONT>+"
                                          "<FONT class=\"U\">%u</FONT>+"
                                          "<FONT class=\"E\">%u</FONT>=%u"
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


static char test_name[1024];


static te_bool trc_diff_tests_has_diff(test_runs *tests,
                                       unsigned int flags);

static int trc_diff_tests_to_html(const test_runs *tests,
                                  unsigned int flags,
                                  unsigned int level);


static te_bool
trc_diff_exclude_by_key(const test_iter *iter)
{
    le_string      *p;
    trc_tags_entry *tags;
    te_bool         exclude = FALSE;

    for (p = trc_diff_exclude_keys.lh_first;
         p != NULL && !exclude;
         p = p->links.le_next)
    {
        for (tags = tags_diff.tqh_first;
             tags != NULL;
             tags = tags->links.tqe_next)
        {
            if (iter->diff_exp[tags->id].key != NULL &&
                strlen(iter->diff_exp[tags->id].key) > 0)
            {
                exclude = (strncmp(iter->diff_exp[tags->id].key,
                                   p->str, strlen(p->str)) == 0);
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


static char trc_args_buf[0x10000];

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
 * Map test result, match and exclude status to statistics index.
 *
 * @param result        Result
 * @param match         Do results match?
 * @param exclude       Does exclude of such differencies requested?
 *
 * @return Index of statistics counter.
 */
static trc_diff_stats_index
trc_diff_result_to_stats_index(trc_test_result result,
                               te_bool match, te_bool exclude)
{
    switch (result)
    {
        case TRC_TEST_PASSED:
            if (match)
                return TRC_DIFF_STATS_PASSED;
            else if (exclude)
                return TRC_DIFF_STATS_PASSED_DIFF_EXCLUDE;
            else
                return TRC_DIFF_STATS_PASSED_DIFF;

        case TRC_TEST_FAILED:
            if (match)
                return TRC_DIFF_STATS_FAILED;
            else if (exclude)
                return TRC_DIFF_STATS_FAILED_DIFF_EXCLUDE;
            else
                return TRC_DIFF_STATS_FAILED_DIFF;

        case TRC_TEST_SKIPPED:
            return TRC_DIFF_STATS_SKIPPED;

        default:
            return TRC_DIFF_STATS_OTHER;
    }
}

/**
 * Update total statistics for sets X and Y based on an iteration data.
 *
 * @param iter          Test iteration
 * @param tags_x        Set X of tags
 * @param tags_y        Set Y of tags
 */
static void
trc_diff_iter_stats(const test_iter      *iter,
                    const trc_tags_entry *tags_x,
                    const trc_tags_entry *tags_y)
{
    te_bool match;
    te_bool exclude;

    /* 
     * Do nothing if an index of the first set is greater or equal to the
     * index of the second set.
     */
    if (tags_x->id >= tags_y->id)
        return;

    /*
     * Exclude iterations of test packages
     */
    if (iter->tests.head.tqh_first != NULL)
        return;

    match = (iter->diff_exp[tags_x->id].value ==
             iter->diff_exp[tags_y->id].value) &&
            tq_strings_equal(&iter->diff_exp[tags_x->id].verdicts,
                             &iter->diff_exp[tags_y->id].verdicts);

    exclude = !match && trc_diff_exclude_by_key(iter);

    assert(tags_y->id > 0);

    stats[tags_x->id][tags_y->id - 1]
         [trc_diff_result_to_stats_index(iter->diff_exp[tags_x->id].value,
                                         match, exclude)]
         [trc_diff_result_to_stats_index(iter->diff_exp[tags_y->id].value,
                                         match, exclude)]++;
}
                    

/**
 * Do test iterations have different expected results?
 *
 * @param[in]  test         Test
 * @param[in]  flags        Processing flags
 * @param[out] all_equal    Do @e all iterations have output?
 */
static te_bool
trc_diff_iters_has_diff(test_run *test, unsigned int flags,
                        te_bool *all_equal)
{
    te_bool         has_diff;
    te_bool         iter_has_diff;
    trc_exp_result *iter_result;
    trc_tags_entry *tags_i;
    trc_tags_entry *tags_j;
    test_iter      *p;

    *all_equal = TRUE;
    for (has_diff = FALSE, p = test->iters.head.tqh_first;
         p != NULL;
         has_diff = has_diff || p->output, p = p->links.tqe_next)
    {
        iter_has_diff = FALSE;
        iter_result = NULL;

        for (tags_i = tags_diff.tqh_first;
             tags_i != NULL;
             tags_i = tags_i->links.tqe_next)
        {
            if (test->diff_exp[tags_i->id] == TRC_TEST_UNSET)
            {
                test->diff_exp[tags_i->id] = p->diff_exp[tags_i->id].value;
                test->diff_verdicts[tags_i->id] =
                    &p->diff_exp[tags_i->id].verdicts;
            }
            else if (test->diff_exp[tags_i->id] == TRC_TEST_MIXED)
            {
                /* Nothing to do */
            }
            else if (test->diff_exp[tags_i->id] !=
                     p->diff_exp[tags_i->id].value ||
                     !tq_strings_equal(test->diff_verdicts[tags_i->id],
                                       &p->diff_exp[tags_i->id].verdicts))
            {
                test->diff_exp[tags_i->id] = TRC_TEST_MIXED;
                test->diff_verdicts[tags_i->id] = NULL;
                *all_equal = FALSE;
            }

            if (iter_result == NULL)
            {
                iter_result = p->diff_exp + tags_i->id;
            }
            else if (iter_has_diff)
            {
                /* Nothing to do */
            }
            else if (iter_result->value != p->diff_exp[tags_i->id].value ||
                     !tq_strings_equal(&iter_result->verdicts,
                                       &p->diff_exp[tags_i->id].verdicts))
            {
                iter_has_diff = TRUE;
            }

            for (tags_j = tags_diff.tqh_first;
                 tags_j != NULL;
                 tags_j = tags_j->links.tqe_next)
            {
                if (!test->aux)
                    trc_diff_iter_stats(p, tags_i, tags_j);
            }
        }

        /* The routine should be called first to be called in any case */
        p->output = trc_diff_tests_has_diff(&p->tests, flags) ||
                    (iter_has_diff && !trc_diff_exclude_by_key(p));
        /*<
         * Iteration is output, if its tests have differencies or
         * expected results of the test iteration are different and it
         * shouldn't be excluded because of keys pattern.
         */
    }

    return has_diff;
}

/**
 * Do tests in the set have different expected results?
 *
 * @param tests         Set of tests
 * @param flags         Processing flags
 */
static te_bool
trc_diff_tests_has_diff(test_runs *tests, unsigned int flags)
{
    trc_tags_entry *entry;
    test_run       *p;
    te_bool         has_diff;
    te_bool         all_iters_equal;

    for (has_diff = FALSE, p = tests->head.tqh_first;
         p != NULL;
         has_diff = has_diff || p->diff_out, p = p->links.tqe_next)
    {
        /* Initialize expected result of the test as whole */
        for (entry = tags_diff.tqh_first;
             entry != NULL;
             entry = entry->links.tqe_next)
        {
            p->diff_exp[entry->id] = TRC_TEST_UNSET;
        }

        /* Output the test, if  iteration has differencies. */
        p->diff_out = trc_diff_iters_has_diff(p, flags,
                                              &all_iters_equal);

        /**
         * Output test iterations if and only if test should be output
         * itself and:
         *  - set of iterations is empty, or
         *  - all iterations are not equal, or
         *  - it is not leaf of the tests tree.
         */
        p->diff_out_iters = p->diff_out &&
            (p->iters.head.tqh_first == NULL ||
             !all_iters_equal ||
             p->iters.head.tqh_first->tests.head.tqh_first != NULL);
    }

    return has_diff;
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
 * Are expected results for two iterations equal?
 */
static te_bool
trc_diff_expectations_equal(const test_iter *p, const test_iter *q)
{
    trc_tags_entry *tags;

    for (tags = tags_diff.tqh_first;
         tags != NULL &&
         p->diff_exp[tags->id].value == q->diff_exp[tags->id].value &&
         tq_strings_equal(&p->diff_exp[tags->id].verdicts,
                          &q->diff_exp[tags->id].verdicts);
         tags = tags->links.tqe_next);

    return (tags == NULL);
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
                strcmp(tag->name, "result") != 0)
            {
                fprintf(f, " %s", tag->name);
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
int
trc_diff_report_to_html(trc_database *db, unsigned int flags,
                        const char *filename)
{
    int     rc;
    te_bool has_diff;

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
    fprintf(f, trc_diff_html_doc_start,
            (trc_diff_title != NULL) ? trc_diff_title :
                                       trc_diff_html_title_def,
            (trc_diff_title != NULL) ? trc_diff_title :
                                       trc_diff_html_title_def,
            db->version);

    /* Compared sets */
    trc_diff_tags_to_html(&tags_diff);

    /* Initialize statistics */
    memset(stats, 0, sizeof(stats));

    /* Preprocess tests tree and gather statistics */
    has_diff = trc_diff_tests_has_diff(&db->tests, flags);

    /* Output statistics */
    trc_diff_stats_to_html(flags);

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

    fclose(f);
    return 0;

cleanup:
    fclose(f);
    unlink(filename);
    return rc;
}
