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

#define _GNU_SOURCE

#include "te_config.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "te_defs.h"
#include "logger_api.h"

#include "trc_diff.h"
#include "trc_html.h"


/** Generate brief version of the diff report */
#define TRC_DIFF_BRIEF      0x01

#define WRITE_STR(str) \
    do {                                                    \
        if (fputs(str, f) == EOF)                           \
        {                                                   \
            rc = te_rc_os2te(errno);                        \
            assert(rc != 0);                                \
            ERROR("Writing to the file failed: %r", rc);    \
            goto cleanup;                                   \
        }                                                   \
    } while (0)

#define PRINT_STR(str_)  (((str_) != NULL) ? (str_) : "")


/** Initial length of the dynamically allocated string */
#define TE_STRING_INIT_LEN  1

/** Additional length when dynamically allocated string is reallocated */
#define TE_STRING_EXTRA_LEN 0

/**
 * Dynamically allocated string.
 */
typedef struct te_string {
    char   *ptr;    /**< Pointer to the buffer */
    size_t  size;   /**< Size of the buffer */
    size_t  len;    /**< Length of the string */
} te_string;

/**
 * Append to the string results of the sprintf(fmt, ...);
 *
 * @param str           Dynamic string
 * @param fmt           Format string
 * @param ...           Format string arguments
 *
 * @return Status code.
 */
extern te_errno te_string_append(te_string *str, const char *fmt, ...);
                                 
te_errno
te_string_append(te_string *str, const char *fmt, ...)
{
    char       *s;
    size_t      rest;
    int         printed;
    va_list     ap;
    te_bool     again;

    if (str->ptr == NULL)
    {
        str->ptr = malloc(TE_STRING_INIT_LEN);
        if (str->ptr == NULL)
        {
            ERROR("%s(): Memory allocation failure", __FUNCTION__);
            return TE_ENOMEM;
        }
        str->size = TE_STRING_INIT_LEN;
        str->len = 0;
    }

    assert(str->size > str->len);
    rest = str->size - str->len;
    do  {
        s = str->ptr + str->len;

        va_start(ap, fmt);
        printed = vsnprintf(s, rest, fmt, ap);
        assert(printed > 0);
        va_end(ap);

        if ((size_t)printed >= rest)
        {
            /* We are going to extend buffer */
            rest = printed + 1 /* '\0' */ + TE_STRING_EXTRA_LEN;
            str->size = str->len + rest;
            str->ptr = realloc(str->ptr, str->size);
            if (str->ptr == NULL)
            {
                ERROR("%s(): Memory allocation failure", __FUNCTION__);
                return TE_ENOMEM;
            }
            /* Print again */
            again = TRUE;
        }
        else
        {
            str->len += printed;
            again = FALSE;
        }
    } while (again);

    return 0;
}


static const char * const trc_diff_html_title_def =
    "Testing Results Expectations Differences Report";

static const char * const trc_diff_html_doc_start =
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n"
"<html>\n"
"<head>\n"
"  <meta http-equiv=\"content-type\" content=\"text/html; "
"charset=utf-8\">\n"
"  <title>%s</title>\n"
"  <style type=\"text/css\">\n"
"    .S {font-weight: bold; color: green; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .U {font-weight: bold; color: red; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .E {font-weight: italic; color: blue; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"  </style>\n"
"</head>\n"
"<body lang=\"en-US\" dir=\"LTR\">\n"
"<h1 align=center>%s</h1>\n"
"<h2 align=center>%s</h2>\n";

static const char * const trc_diff_html_doc_end =
"</body>\n"
"</html>\n";


static const char * const trc_diff_stats_table =
"<table border=1 cellpadding=4 cellspacing=3>\n"
"  <thead>\n"
"    <tr>\n"
"      <td rowspan=2>\n"
"        <b>%s</b>\n"
"      </td>\n"
"      <td colspan=4 align=center>\n"
"        <b>%s</b>\n"
"      </td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td align=center><b>%s</b></td>\n"
"      <td align=center><b>%s</b></td>\n"
"      <td align=center><b>%s</b></td>\n"
"      <td align=center><b>%s</b></td>\n"
"    </tr>\n"
"  </thead>\n"
"  <tbody align=right>\n"
"    <tr>\n"
"      <td align=left><b>%s</b></td>\n"
"      <td><font class=\"S\">%u</font>+<font class=\"U\">%u</font>+"
          "<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td align=left><b>%s</b></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"S\">%u</font>+<font class=\"U\">%u</font>+"
          "<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td align=left><b>%s</b></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"S\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font></td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td align=left><b>%s</b></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font></td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td align=left colspan=5><h3>Total run: <font class=\"S\">%u</font>+"
          "<font class=\"U\">%u</font>+<font class=\"E\">%u</font>=%u"
          "</h3></td>"
"    </tr>\n"
"    <tr>\n"
"      <td align=left colspan=5>[<font class=\"S\">X</font>+]"
                                "<font class=\"U\">Y</font>+"
                                "<font class=\"E\">Z</font><br/>"
"X - result match, Y - result does not match (to be fixed), "
"Z - result does not match (excluded)</td>"
"    </tr>\n"
"  </tbody>\n"
"</table>\n";


static const char * const trc_diff_key_table_heading =
"<table border=1 cellpadding=4 cellspacing=3>\n"
"  <thead>\n"
"    <tr>\n"
"      <td>\n"
"        <b>Key</b>\n"
"      </td>\n"
"      <td>\n"
"        <b>Number of caused differences</b>\n"
"      </td>\n"
"    </tr>\n"
"  </thead>\n"
"  <tbody>\n";

static const char * const trc_diff_key_table_row =
"    <tr>\n"
"      <td>%s</td>\n"
"      <td align=right>%u</td>\n"
"    </tr>\n";


static const char * const trc_diff_full_table_heading_start =
"<table border=1 cellpadding=4 cellspacing=3>\n"
"  <thead>\n"
"    <tr>\n"
"      <td>\n"
"        <b>Name</b>\n"
"      </td>\n"
"      <td>\n"
"        <b>Objective</b>\n"
"      </td>\n";

static const char * const trc_diff_brief_table_heading_start =
"<table border=1 cellpadding=4 cellspacing=3>\n"
"  <thead>\n"
"    <tr>\n"
"      <td>\n"
"        <b>Name</b>\n"
"      </td>\n";

static const char * const trc_diff_table_heading_entry =
"      <td>"
"        <b>%s</b>\n"
"      </td>\n";

static const char * const trc_diff_table_heading_end =
"      <td>"
"        <b>Key</b>\n"
"      </td>\n"
"      <td>"
"        <b>Notes</b>\n"
"      </td>\n"
"    </tr>\n"
"  </thead>\n"
"  <tbody>\n";

static const char * const trc_diff_table_end =
"  </tbody>\n"
"</table>\n";

static const char * const trc_diff_full_table_test_row_start =
"    <tr>\n"
"      <td><a name=\"%u\"/>%s<b>%s</b></td>\n"  /* Name */
"      <td>%s</td>\n";          /* Objective */

static const char * const trc_diff_brief_table_test_row_start =
"    <tr>\n"
"      <td><a href=\"#%u\">%s</a></td>\n";  /* Name */

static const char * const trc_diff_table_iter_row_start =
"    <tr>\n"
"      <td colspan=2><a name=\"%u\"/>";

static const char * const trc_diff_table_row_end =
"      <td>%s</td>\n"           /* Key */
"      <td>%s</td>\n"           /* Notes */
"    </tr>\n";

static const char * const trc_diff_table_row_col_start =
"      <td>";

static const char * const trc_diff_table_row_col_end =
"</td>\n";


/**
 * Output set of tags used for comparison to HTML report.
 *
 * @param f             File stream to write
 * @param tags_list     List of tags
 */
static void
trc_diff_tags_to_html(FILE *f, const trc_diff_tags_list *tags_list)
{
    const trc_diff_tags_entry  *p;
    const tqe_string           *tag;

    for (p = tags_list->tqh_first; p != NULL; p = p->links.tqe_next)
    {
        if (p->name != NULL)
            fprintf(f, "<b>%s: </b>", p->name);
        else
            fprintf(f, "<b>Set %d: </b>", p->id);

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
        fprintf(f, "<br/><br/>");
    }
}


/**
 * Output statistics for one comparison to HTML report.
 *
 * @param f             File stream to write
 * @param stats         Calculated statistics
 * @param tags_x        The first set of tags
 * @param tags_y        The second set of tags
 * @param flags         Processing flags
 */
static void
trc_diff_one_stats_to_html(FILE                      *f,
                           const trc_diff_stats      *stats,
                           const trc_diff_tags_entry *tags_x,
                           const trc_diff_tags_entry *tags_y,
                           unsigned int               flags)
{
    const trc_diff_stats_counters  *counters;
    trc_diff_stats_counter          total_match;
    trc_diff_stats_counter          total_no_match;
    trc_diff_stats_counter          total_excluded;
    trc_diff_stats_counter          total;

    UNUSED(flags);

    counters = &stats[tags_x->id][tags_y->id - 1];

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
 * @param f             File stream to write
 * @param diffs         Compared sets
 * @param stats         Calculated statistics
 * @param flags         Processing flags
 */
static void
trc_diff_stats_to_html(FILE *f, const trc_diff_tags_list *diffs,
                       trc_diff_stats *stats, unsigned int flags)
{
    const trc_diff_tags_entry *tags_i;
    const trc_diff_tags_entry *tags_j;

    for (tags_i = diffs->tqh_first;
         tags_i != NULL;
         tags_i = tags_i->links.tqe_next)
    {
        for (tags_j = diffs->tqh_first;
             tags_j != NULL;
             tags_j = tags_j->links.tqe_next)
        {
            if (tags_i->id < tags_j->id)
                trc_diff_one_stats_to_html(f, stats, tags_i, tags_j, flags);
        }
    }
}


/**
 * Sort list of keys by 'count' in decreasing order.
 *
 * @param keys_stats    Per-key statistics
 */
static void
trc_diff_keys_sort(trc_diff_keys_stats *keys_stats)
{
    trc_diff_key_stats *curr, *prev, *p;

    assert(keys_stats->cqh_first != (void *)keys_stats);
    for (prev = keys_stats->cqh_first;
         (curr = prev->links.cqe_next) != (void *)keys_stats;
         prev = curr)
    {
        if (prev->count < curr->count)
        {
            CIRCLEQ_REMOVE(keys_stats, curr, links);
            for (p = keys_stats->cqh_first;
                 p != prev && p->count >= curr->count;
                 p = p->links.cqe_next);

            /* count have to fire */
            assert(p->count < curr->count);
            CIRCLEQ_INSERT_BEFORE(keys_stats, p, curr, links);
        }
    }
}

/**
 * Output per-key statistics to HTML.
 *
 * @param f             File stream to write
 * @param keys_stats    Per-key statistics
 *
 * @return Status code.
 */
static te_errno
trc_diff_key_to_html(FILE *f, trc_diff_keys_stats *keys_stats)
{
    int                 rc = 0;
    trc_diff_key_stats *p;

    if (keys_stats->cqh_first == (void *)keys_stats)
        return 0;

    trc_diff_keys_sort(keys_stats);

    WRITE_STR(trc_diff_key_table_heading);
    for (p = keys_stats->cqh_first;
         p != (void *)keys_stats;
         p = p->links.cqe_next)
    {
#if 0
        if (p->tags->show_keys)
#endif
        {
            fprintf(f, trc_diff_key_table_row, p->key, p->count);
        }
    }
    WRITE_STR(trc_diff_table_end);

cleanup:
    return rc;
}

#if 0

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
            s += sprintf(s, "<em>%s</em> - %s<br/>",
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
    char           *s = buf;
    trc_tags_entry *tags;

    buf[0] = '\0';
    if (iter->notes != NULL && strlen(iter->notes) > 0)
    {
        s += sprintf(s, "%s<br/>", iter->notes);
    }
    for (tags = tags_diff.tqh_first;
         tags != NULL;
         tags = tags->links.tqe_next)
    {
        if (iter->diff_exp[tags->id].notes != NULL &&
            strlen(iter->diff_exp[tags->id].notes) > 0)
        {
            s += sprintf(s, "<em>%s</em> - %s<br/>",
                         tags->name, iter->diff_exp[tags->id].notes);
        }
    }

    return buf;
}

static const char *
trc_diff_test_iters_get_keys(test_run *test, unsigned int id)
{
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
#endif


/**
 * Output expected results to HTML file.
 *
 * @param f             File stream to write
 * @param diffs         Compared sets
 * @param flags         Processing flags
 *
 * @return Status code.
 */
static te_errno
trc_diff_exp_results_to_html(FILE                      *f,
                             const trc_diff_tags_list  *diffs,
                             const trc_diff_entry      *entry,
                             unsigned int               flags)
{
    te_errno                    rc = 0;
    const trc_diff_tags_entry  *tags;

    for (tags = diffs->tqh_first;
         tags != NULL && rc == 0;
         tags = tags->links.tqe_next)
    {
        WRITE_STR(trc_diff_table_row_col_start);
        rc = trc_exp_result_to_html(f, entry->results[tags->id], flags);
        WRITE_STR(trc_diff_table_row_col_end);
    }

cleanup:
    return rc;
}

/**
 * Output differences into a file @a f (global variable).
 *
 * @param result        List with results
 * @param diffs         Compared sets
 * @param flags         Processing flags
 *
 * @return Status code.
 */
static te_errno
trc_diff_result_to_html(const trc_diff_result    *result,
                        const trc_diff_tags_list *diffs,
                        unsigned int              flags,
                        FILE                     *f)
{
    te_errno                    rc = 0;
    const trc_diff_tags_entry  *tags;
    const trc_diff_entry       *curr;
    const trc_diff_entry       *next;
    unsigned int                i, j;
    te_string                   str_buf = { NULL, 0, 0 };


    /*
     * Do nothing if no differences
     */
    if (result->tqh_first == NULL)
        return 0;

    /*
     * Table header
     */
    if (flags & TRC_DIFF_BRIEF)
    {
        WRITE_STR(trc_diff_brief_table_heading_start);
    }
    else
    {
        WRITE_STR(trc_diff_full_table_heading_start);
    }
    for (tags = diffs->tqh_first;
         tags != NULL;
         tags = tags->links.tqe_next)
    {
        if ((tags->name == NULL) &&
            (asprintf(&tags->name, "Set %u", tags->id) == -1))
            return TE_ENOMEM;
        fprintf(f, trc_diff_table_heading_entry, tags->name);
    }
    WRITE_STR(trc_diff_table_heading_end);

    /*
     * Table content
     */
    for (i = 0, curr = result->tqh_first; curr != NULL; ++i, curr = next)
    {
        next = curr->links.tqe_next;

        if (!curr->is_iter)
        {
            if (flags & TRC_DIFF_BRIEF)
            {
                rc = te_string_append(&str_buf, "%s%s",
                                      curr->level == 0 ? "" : "/",
                                      curr->ptr.test->name);
                if (rc != 0)
                    return rc;

                /* 
                 * Test is not output in brief mode, if its iterations
                 * follow.
                 */
                if ((next != NULL) && (next->is_iter))
                {
                    assert(curr->level == next->level);
                    continue;
                }

                fprintf(f, trc_diff_brief_table_test_row_start,
                        i, str_buf.ptr);

                rc = trc_diff_exp_results_to_html(f, diffs, curr, flags);
                if (rc != 0)
                    goto cleanup;
            }
            else
            {
                rc = te_string_append(&str_buf, "*-");
                if (rc != 0)
                    return rc;

                fprintf(f, trc_diff_full_table_test_row_start,
                        i, str_buf.ptr, curr->ptr.test->name,
                        PRINT_STR(curr->ptr.test->objective));

                rc = trc_diff_exp_results_to_html(f, diffs, curr, flags);
                if (rc != 0)
                    goto cleanup;
            }

#if 0
            buf[0] = '\0';
            s = buf;
            for (entry = tags_diff.tqh_first;
                 entry != NULL;
                 entry = entry->links.tqe_next)
            {
                keys = trc_diff_test_iters_get_keys(p, entry->id);
                if (strlen(keys) > 0)
                {
                    s += sprintf(s, "<em>%s</em> - %s<br/>",
                                 entry->name, keys);
                }
            }
            fprintf(f, trc_diff_table_row_end, buf, PRINT_STR(p->notes));
#endif
        }
        else
        {
            if (flags & TRC_DIFF_BRIEF)
            {
                fprintf(f, trc_diff_brief_table_test_row_start,
                        i, str_buf.ptr);

                rc = trc_diff_exp_results_to_html(f, diffs, curr, flags);
                if (rc != 0)
                    goto cleanup;
            }
            else
            {
                fprintf(f, trc_diff_table_iter_row_start, i);
                trc_test_iter_args_to_html(f, &curr->ptr.iter->args,
                                           flags);
                WRITE_STR(trc_diff_table_row_col_end);

                rc = trc_diff_exp_results_to_html(f, diffs, curr, flags);
                if (rc != 0)
                    goto cleanup;
            }
        }

        /* If level of the next entry is less */
        if (next != NULL && next->level < curr->level)
        {
            if (flags & TRC_DIFF_BRIEF)
            {
                for (j = curr->level - next->level; j-- > 0; )
                {
                    str_buf.ptr[strrchr(str_buf.ptr, '/') -
                                str_buf.ptr] = '\0';
                }
            }
            else
            {
                str_buf.ptr[str_buf.len -
                            (curr->level - next->level) * 2] = '\0';
            }
        }
    }

    /*
     * Table end
     */
    WRITE_STR(trc_diff_table_end);

cleanup:
    return rc;
}


/** See descriptino in trc_db.h */
te_errno
trc_diff_report_to_html(trc_diff_ctx *ctx, const char *filename,
                        const char *title)
{
    FILE       *f;
    te_errno    rc;

    if (filename == NULL)
    {
        f = stdout;
    }
    else
    {
        f = fopen(filename, "w");
        if (f == NULL)
        {
            ERROR("Failed to open file to write HTML report to");
            return te_rc_os2te(errno);
        }
    }

    /* HTML header */
    fprintf(f, trc_diff_html_doc_start,
            (title != NULL) ? title : trc_diff_html_title_def,
            (title != NULL) ? title : trc_diff_html_title_def,
            ctx->db->version);

    /* Compared sets */
    trc_diff_tags_to_html(f, &ctx->sets);

    /* Output statistics */
    trc_diff_stats_to_html(f, &ctx->sets, &ctx->stats, ctx->flags);

    /* Output per-key summary */
    trc_diff_key_to_html(f, &ctx->keys_stats);
    
    /* Report */
    if ((rc = trc_diff_result_to_html(&ctx->result, &ctx->sets,
                                      ctx->flags | TRC_DIFF_BRIEF,
                                      f)) != 0 ||
        (rc = trc_diff_result_to_html(&ctx->result, &ctx->sets,
                                      ctx->flags, f)) != 0)
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
