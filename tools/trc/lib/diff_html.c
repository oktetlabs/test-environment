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
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "te_defs.h"
#include "logger_api.h"

#include "trc_tags.h"
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
"      <td colspan=5 align=center>\n"
"        <b>%s</b>\n"
"      </td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td align=center><b>%s</b></td>\n"
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
"      <td><font class=\"U\">%u</font></td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td align=left><b>%s</b></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"S\">%u</font>+<font class=\"U\">%u</font>+"
          "<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font></td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td align=left><b>%s</b></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"S\">%u</font>+<font class=\"U\">%u</font>+"
          "<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font></td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td align=left><b>%s</b></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font>+<font class=\"E\">%u</font></td>\n"
"      <td><font class=\"S\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font></td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td align=left><b>%s</b></td>\n"
"      <td><font class=\"U\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font></td>\n"
"      <td><font class=\"U\">%u</font></td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td align=left colspan=6><h3>Total run: <font class=\"S\">%u</font>+"
          "<font class=\"U\">%u</font>+<font class=\"E\">%u</font>=%u"
          "</h3></td>"
"    </tr>\n"
"    <tr>\n"
"      <td align=left colspan=6>[<font class=\"S\">X</font>+]"
                                "<font class=\"U\">Y</font>+"
                                "<font class=\"E\">Z</font><br/>"
"X - result match, Y - result does not match (to be fixed), "
"Z - result does not match (ignored)</td>"
"    </tr>\n"
"  </tbody>\n"
"</table>\n";


static const char * const trc_diff_key_table_heading =
"<table border=1 cellpadding=4 cellspacing=3>\n"
"  <thead>\n"
"    <tr>\n"
"      <td>\n"
"        <b>%s Key</b>\n"
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
 * @param sets          List of tags
 */
static void
trc_diff_tags_to_html(FILE *f, const trc_diff_sets *sets)
{
    const trc_diff_set *p;
    const tqe_string   *tag;

    TAILQ_FOREACH(p, sets, links)
    {
        fprintf(f, "<b>%s: </b>", p->name);
        TAILQ_FOREACH(tag, &p->tags, links)
        {
            if (TAILQ_NEXT(tag, links) != NULL ||
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
 */
static void
trc_diff_one_stats_to_html(FILE               *f,
                           trc_diff_stats     *stats,
                           const trc_diff_set *tags_x,
                           const trc_diff_set *tags_y)
{
    trc_diff_stats_counters    *counters;
    trc_diff_stats_counter      total_match;
    trc_diff_stats_counter      total_no_match;
    trc_diff_stats_counter      total_no_match_ignored;
    trc_diff_stats_counter      total;

    counters = &((*stats)[tags_x->id][tags_y->id - 1]);

    total_match =
        (*counters)[TRC_TEST_PASSED][TRC_TEST_PASSED][TRC_DIFF_MATCH] +
        (*counters)[TRC_TEST_FAILED][TRC_TEST_FAILED][TRC_DIFF_MATCH] +
        (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_UNSTABLE][TRC_DIFF_MATCH];
    total_no_match =
        (*counters)[TRC_TEST_PASSED][TRC_TEST_PASSED][TRC_DIFF_NO_MATCH] +
        (*counters)[TRC_TEST_PASSED][TRC_TEST_FAILED][TRC_DIFF_NO_MATCH] +
        (*counters)[TRC_TEST_PASSED][TRC_TEST_UNSTABLE][TRC_DIFF_NO_MATCH] +
        (*counters)[TRC_TEST_FAILED][TRC_TEST_PASSED][TRC_DIFF_NO_MATCH] +
        (*counters)[TRC_TEST_FAILED][TRC_TEST_FAILED][TRC_DIFF_NO_MATCH] +
        (*counters)[TRC_TEST_FAILED][TRC_TEST_UNSTABLE][TRC_DIFF_NO_MATCH] +
        (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_PASSED][TRC_DIFF_NO_MATCH] +
        (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_FAILED][TRC_DIFF_NO_MATCH] +
        (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_UNSTABLE]
                   [TRC_DIFF_NO_MATCH];
    total_no_match_ignored =
        (*counters)[TRC_TEST_PASSED][TRC_TEST_PASSED]
                   [TRC_DIFF_NO_MATCH_IGNORE] +
        (*counters)[TRC_TEST_PASSED][TRC_TEST_FAILED]
                   [TRC_DIFF_NO_MATCH_IGNORE] +
        (*counters)[TRC_TEST_PASSED][TRC_TEST_UNSTABLE]
                   [TRC_DIFF_NO_MATCH_IGNORE] +
        (*counters)[TRC_TEST_FAILED][TRC_TEST_PASSED]
                   [TRC_DIFF_NO_MATCH_IGNORE] +
        (*counters)[TRC_TEST_FAILED][TRC_TEST_FAILED]
                   [TRC_DIFF_NO_MATCH_IGNORE] +
        (*counters)[TRC_TEST_FAILED][TRC_TEST_UNSTABLE]
                   [TRC_DIFF_NO_MATCH_IGNORE] +
        (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_PASSED]
                   [TRC_DIFF_NO_MATCH_IGNORE] +
        (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_FAILED]
                   [TRC_DIFF_NO_MATCH_IGNORE] +
        (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_UNSTABLE]
                   [TRC_DIFF_NO_MATCH_IGNORE];
    total = total_match + total_no_match + total_no_match_ignored;

    fprintf(f, trc_diff_stats_table,
            tags_x->name, tags_y->name,
            "PASSED", "FAILED", "unstable", "SKIPPED", "unspecified",
            "PASSED",
            (*counters)[TRC_TEST_PASSED][TRC_TEST_PASSED]
                       [TRC_DIFF_MATCH],
            (*counters)[TRC_TEST_PASSED][TRC_TEST_PASSED]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_PASSED][TRC_TEST_PASSED]
                       [TRC_DIFF_NO_MATCH_IGNORE],
            (*counters)[TRC_TEST_PASSED][TRC_TEST_FAILED]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_PASSED][TRC_TEST_FAILED]
                       [TRC_DIFF_NO_MATCH_IGNORE],
            (*counters)[TRC_TEST_PASSED][TRC_TEST_UNSTABLE]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_PASSED][TRC_TEST_UNSTABLE]
                       [TRC_DIFF_NO_MATCH_IGNORE],
            (*counters)[TRC_TEST_PASSED][TRC_TEST_SKIPPED]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_PASSED][TRC_TEST_SKIPPED]
                       [TRC_DIFF_NO_MATCH_IGNORE],
            (*counters)[TRC_TEST_PASSED][TRC_TEST_UNSPECIFIED]
                       [TRC_DIFF_NO_MATCH],
            "FAILED",
            (*counters)[TRC_TEST_FAILED][TRC_TEST_PASSED]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_FAILED][TRC_TEST_PASSED]
                       [TRC_DIFF_NO_MATCH_IGNORE],
            (*counters)[TRC_TEST_FAILED][TRC_TEST_FAILED]
                       [TRC_DIFF_MATCH],
            (*counters)[TRC_TEST_FAILED][TRC_TEST_FAILED]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_FAILED][TRC_TEST_FAILED]
                       [TRC_DIFF_NO_MATCH_IGNORE],
            (*counters)[TRC_TEST_FAILED][TRC_TEST_UNSTABLE]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_FAILED][TRC_TEST_UNSTABLE]
                       [TRC_DIFF_NO_MATCH_IGNORE],
            (*counters)[TRC_TEST_FAILED][TRC_TEST_SKIPPED]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_FAILED][TRC_TEST_SKIPPED]
                       [TRC_DIFF_NO_MATCH_IGNORE],
            (*counters)[TRC_TEST_FAILED][TRC_TEST_UNSPECIFIED]
                       [TRC_DIFF_NO_MATCH],
            "unstable",
            (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_PASSED]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_PASSED]
                       [TRC_DIFF_NO_MATCH_IGNORE],
            (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_FAILED]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_FAILED]
                       [TRC_DIFF_NO_MATCH_IGNORE],
            (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_UNSTABLE]
                       [TRC_DIFF_MATCH],
            (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_UNSTABLE]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_UNSTABLE]
                       [TRC_DIFF_NO_MATCH_IGNORE],
            (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_SKIPPED]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_SKIPPED]
                       [TRC_DIFF_NO_MATCH_IGNORE],
            (*counters)[TRC_TEST_UNSTABLE][TRC_TEST_UNSPECIFIED]
                       [TRC_DIFF_NO_MATCH],
            "SKIPPED",
            (*counters)[TRC_TEST_SKIPPED][TRC_TEST_PASSED]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_SKIPPED][TRC_TEST_PASSED]
                       [TRC_DIFF_NO_MATCH_IGNORE],
            (*counters)[TRC_TEST_SKIPPED][TRC_TEST_FAILED]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_SKIPPED][TRC_TEST_FAILED]
                       [TRC_DIFF_NO_MATCH_IGNORE],
            (*counters)[TRC_TEST_SKIPPED][TRC_TEST_UNSTABLE]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_SKIPPED][TRC_TEST_UNSTABLE]
                       [TRC_DIFF_NO_MATCH_IGNORE],
            (*counters)[TRC_TEST_SKIPPED][TRC_TEST_SKIPPED]
                       [TRC_DIFF_MATCH],
            (*counters)[TRC_TEST_SKIPPED][TRC_TEST_UNSPECIFIED]
                       [TRC_DIFF_NO_MATCH],
            "unspecified",
            (*counters)[TRC_TEST_UNSPECIFIED][TRC_TEST_PASSED]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_UNSPECIFIED][TRC_TEST_FAILED]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_UNSPECIFIED][TRC_TEST_UNSTABLE]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_UNSPECIFIED][TRC_TEST_SKIPPED]
                       [TRC_DIFF_NO_MATCH],
            (*counters)[TRC_TEST_UNSPECIFIED][TRC_TEST_UNSPECIFIED]
                       [TRC_DIFF_MATCH],
            total_match, total_no_match, total_no_match_ignored, total);
}

/**
 * Output all statistics to HTML report
 *
 * @param f             File stream to write
 * @param sets          Compared sets
 * @param stats         Calculated statistics
 * @param flags         Processing flags
 */
static void
trc_diff_stats_to_html(FILE *f, const trc_diff_sets *sets,
                       trc_diff_stats *stats)
{
    const trc_diff_set *tags_i;
    const trc_diff_set *tags_j;

    TAILQ_FOREACH(tags_i, sets, links)
    {
        TAILQ_FOREACH(tags_j, sets, links)
        {
            if (tags_i->id < tags_j->id)
                trc_diff_one_stats_to_html(f, stats, tags_i, tags_j);
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
 * Output per-key statistics for all sets.
 *
 * @param f             File stream to write
 * @param sets          Compared sets
 *
 * @return Status code.
 */
static te_errno
trc_diff_keys_stats_to_html(FILE *f, trc_diff_sets *sets)
{
    te_errno                    rc = 0;
    trc_diff_set               *p;
    const trc_diff_key_stats   *q;

    TAILQ_FOREACH(p, sets, links)
    {
        if (p->show_keys &&
            p->keys_stats.cqh_first != (void *)&(p->keys_stats))
        {
            trc_diff_keys_sort(&p->keys_stats);

            fprintf(f, trc_diff_key_table_heading, p->name);
            for (q = p->keys_stats.cqh_first;
                 q != (void *)&(p->keys_stats);
                 q = q->links.cqe_next)
            {
                fprintf(f, trc_diff_key_table_row, q->key, q->count);
            }
            WRITE_STR(trc_diff_table_end);
        }
    }

cleanup:
    return rc;
}


/**
 * Output expected results to HTML file.
 *
 * @param f             File stream to write
 * @param sets          Compared sets
 * @param entry         TRC diff result entry
 * @param flags         Processing flags
 *
 * @return Status code.
 */
static te_errno
trc_diff_exp_results_to_html(FILE                 *f,
                             const trc_diff_sets  *sets,
                             const trc_diff_entry *entry,
                             unsigned int          flags)
{
    te_errno            rc = 0;
    const trc_diff_set *set;

    for (set = TAILQ_FIRST(sets);
         set != NULL && rc == 0;
         set = TAILQ_NEXT(set, links))
    {
        WRITE_STR(trc_diff_table_row_col_start);
        rc = trc_exp_result_to_html(f, entry->results[set->id], flags);
        WRITE_STR(trc_diff_table_row_col_end);
    }

cleanup:
    return rc;
}

/**
 * Output test iteration keys to HTML.
 *
 * @param f             File stream to write
 * @param sets          Compared sets
 * @param entry         TRC diff result entry
 *
 * @return Status code.
 */
static te_errno
trc_diff_test_iter_keys_to_html(FILE                 *f,
                                const trc_diff_sets  *sets,
                                const trc_diff_entry *entry)
{
    const trc_diff_set *set;

    TAILQ_FOREACH(set, sets, links)
    {
        if (entry->results[set->id]->key != NULL)
            fprintf(f, "<em>%s</em> - %s<br/>",
                    set->name, entry->results[set->id]->key);
    }

    return 0;
}

/**
 * Output test iteration notes to HTML.
 *
 * @param f             File stream to write
 * @param sets          Compared sets
 * @param entry         TRC diff result entry
 *
 * @return Status code.
 */
static te_errno
trc_diff_test_iter_notes_to_html(FILE                 *f,
                                 const trc_diff_sets  *sets,
                                 const trc_diff_entry *entry)
{
    const trc_diff_set *set;

    TAILQ_FOREACH(set, sets, links)
    {
        if (entry->results[set->id]->notes != NULL)
            fprintf(f, "<em>%s</em> - %s<br/>",
                    set->name, entry->results[set->id]->notes);
    }

    return 0;
}

/**
 * Output collected keys for the test by its iterations to HTML report.
 *
 * @param f             File stream to write to
 * @param sets          Compared sets
 * @param entry         TRC diff result entry
 *
 * @return Status code.
 */
static te_errno
trc_diff_test_keys_to_html(FILE *f, const trc_diff_sets *sets,
                           const trc_diff_entry *entry)
{
    const trc_diff_set *set;
    const tqe_string   *str;

    TAILQ_FOREACH(set, sets, links)
    {
        if (!TAILQ_EMPTY(entry->keys + set->id))
            fprintf(f, "<em>%s</em> - ", set->name);
        TAILQ_FOREACH(str, entry->keys + set->id, links)
        {
            fprintf(f, "%s%s",
                    (str == TAILQ_FIRST(entry->keys + set->id)) ? "" : ", ",
                    str->v);
        }
    }
    return 0;
}

/**
 * Find an iteration of this test before this iteration with the same
 * expected result and keys.
 *
 * @param sets          Compared sets
 * @param entry         TRC diff entry for current iteration
 */
static const trc_diff_entry *
trc_diff_html_brief_find_dup_iter(const trc_diff_sets  *sets,
                                  const trc_diff_entry *entry)
{
    const trc_diff_entry   *p = entry;
    const trc_diff_set     *set;

    assert(entry->is_iter);
    while (((p = TAILQ_PREV(p, trc_diff_result, links)) != NULL) &&
           (p->level == entry->level))
    {
        for (set = TAILQ_FIRST(sets);
             set != NULL &&
             trc_diff_is_exp_result_equal(entry->results[set->id],
                                          p->results[set->id]);
             set = TAILQ_NEXT(set, links));

        if (set == NULL)
            return p;
    }
    return NULL;
}

/**
 * Output differences into a file @a f (global variable).
 *
 * @param result        List with results
 * @param sets         Compared sets
 * @param flags         Processing flags
 *
 * @return Status code.
 */
static te_errno
trc_diff_result_to_html(const trc_diff_result *result,
                        const trc_diff_sets   *sets,
                        unsigned int           flags,
                        FILE                  *f)
{
    te_errno                rc = 0;
    const trc_diff_set     *tags;
    const trc_diff_entry   *curr;
    const trc_diff_entry   *next;
    unsigned int            i, j;
    te_string               test_name = { NULL, 0, 0 };


    /*
     * Do nothing if no differences
     */
    if (TAILQ_EMPTY(result))
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
    TAILQ_FOREACH(tags, sets, links)
    {
        fprintf(f, trc_diff_table_heading_entry, tags->name);
    }
    WRITE_STR(trc_diff_table_heading_end);

    /*
     * Table content
     */
    for (i = 0, curr = TAILQ_FIRST(result); curr != NULL; ++i, curr = next)
    {
        next = TAILQ_NEXT(curr, links);

        if (flags & TRC_DIFF_BRIEF)
        {
            if (!curr->is_iter)
            {
                rc = te_string_append(&test_name, "%s%s",
                                      curr->level == 0 ? "" : "/",
                                      curr->ptr.test->name);
                if (rc != 0)
                    goto cleanup;
            }

            /*
             * Only leaves are output in brief mode
             */
            if ((next != NULL) && (next->level > curr->level))
            {
                assert((next->level - curr->level == 1) ||
                       (!curr->is_iter &&
                        (next->level - curr->level == 2)));
                /*
                 * It is OK to skip cutting of accumulated name,
                 * since we go in the deep.
                 */
                continue;
            }
            /*
             * Don't want to see iterations with equal verdicts and
             * keys in brief mode.
             */
            if (curr->is_iter &&
                trc_diff_html_brief_find_dup_iter(sets, curr) != NULL)
            {
                goto cut;
            }

            /*
             * In brief output for tests and iterations the first
             * column is the same - long test name.
             */
            fprintf(f, trc_diff_brief_table_test_row_start,
                    i, test_name.ptr);
        }
        else if (curr->is_iter)
        {
            fprintf(f, trc_diff_table_iter_row_start, i);
            trc_test_iter_args_to_html(f, &curr->ptr.iter->args,
                                       flags);
            WRITE_STR(trc_diff_table_row_col_end);
        }
        else
        {
            rc = te_string_append(&test_name,
                                  (curr->level > 0) ? "*-" : "");
            if (rc != 0)
                goto cleanup;

            fprintf(f, trc_diff_full_table_test_row_start,
                    i, test_name.ptr, curr->ptr.test->name,
                    PRINT_STR(curr->ptr.test->objective));
        }

        rc = trc_diff_exp_results_to_html(f, sets, curr, flags);
        if (rc != 0)
            goto cleanup;

        if (curr->is_iter)
        {
            WRITE_STR(trc_diff_table_row_col_start);
            rc = trc_diff_test_iter_keys_to_html(f, sets, curr);
            if (rc != 0)
                goto cleanup;
            WRITE_STR(trc_diff_table_row_col_end);

            WRITE_STR(trc_diff_table_row_col_start);
            fprintf(f, "%s<br/>", PRINT_STR(curr->ptr.iter->notes));
            rc = trc_diff_test_iter_notes_to_html(f, sets, curr);
            if (rc != 0)
                goto cleanup;
            WRITE_STR(trc_diff_table_row_col_end);
        }
        else
        {
            WRITE_STR(trc_diff_table_row_col_start);
            trc_diff_test_keys_to_html(f, sets, curr);
            WRITE_STR(trc_diff_table_row_col_end);

            fprintf(f, trc_diff_table_row_end,
                    PRINT_STR(curr->ptr.test->notes));
        }

cut:
        /* If level of the next entry is less */
        if (next != NULL && 
            ((next->level < curr->level) ||
             (next->level == curr->level && (curr->level & 1) == 0)))
        {
            if (flags & TRC_DIFF_BRIEF)
            {
                char *slash;

                j = ((curr->level - next->level) >> 1);
                do {
                    slash = strrchr(test_name.ptr, '/');
                    if (slash == NULL)
                        slash = test_name.ptr;
                    test_name.ptr[slash - test_name.ptr] = '\0'; 
                } while (j-- > 0);
                test_name.len = slash - test_name.ptr;
            }
            else
            {
                test_name.len -=
                    ((curr->level - next->level) >> 1 << 1) + 2;
                test_name.ptr[test_name.len] = '\0';
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

    /* Output grand total statistics */
    trc_diff_stats_to_html(f, &ctx->sets, &ctx->stats);

    /* Output per-key summary */
    trc_diff_keys_stats_to_html(f, &ctx->sets);
    
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
