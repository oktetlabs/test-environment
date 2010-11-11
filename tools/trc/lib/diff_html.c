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
#include "re_subst.h"
#include "trc_report.h"


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
"<!DOCTYPE HTML PUBLIC \"-      //W3C//DTD HTML 4.0 Transitional//EN\">\n"
"<html>\n"
"<head>\n"
"  <meta http-equiv=\"content-type\" content=\"text/html; "
"charset=utf-8\">\n"
"  <title>%s</title>\n"
"  <style type=\"text/css\">\n"
"    .expected {font-weight: normal; color: blue; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .expected {font-weight: normal; color: blue; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .unexpected {font-weight: normal; color: red; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .passed {font-weight: bold; color: black; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .passed_new {font-weight: bold; color: blue; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .passed_old {font-weight: bold; color: red; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .passed_une {font-weight: bold; color: black; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .passed_une_new {font-weight: bold; color: blue; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .passed_une_old {font-weight: bold; color: red; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .failed {font-weight: bold; color: black; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .failed_new {font-weight: bold; color: red; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .failed_old {font-weight: bold; color: blue; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .failed_une {font-weight: bold; color: black; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .failed_une_new {font-weight: bold; color: red; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .failed_une_old {font-weight: bold; color: blue; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .skipped {font-weight: normal; color: grey; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .total {font-weight: bold; color: black; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .total_new {font-weight: bold; color: black; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .total_old {font-weight: bold; color: black; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .matched {font-weight: bold; color: green; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .unmatched {font-weight: bold; color: red; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
"    .ignored {font-weight: italic; color: blue; "
        "padding-left: 0.08in; padding-right: 0.08in}\n"
#if TRC_USE_STATS_POPUP
"    #StatsTip {\n"
"        position: absolute;\n"
"        visibility: hidden;\n"
"        font-size: small;\n"
"        overflow: auto;\n"
"        width: 600px;\n"
"        height: 400px;\n"
"        left: 200px;\n"
"        top: 100px;\n"
"        background-color: #c0c0c0;\n"
"        border: 1px solid #000000;\n"
"        padding: 10px;\n"
"    }\n"
"    #close {\n"
"        float: right;\n"
"    }\n"
#endif
"  </style>\n"
#if TRC_USE_STATS_POPUP
"  <script type=\"text/javascript\">\n"
"    function fillTip(test_list)\n"
"    {\n"
"        if (test_list === null)\n"
"        {\n"
"            alert('Failed to get subtests of ' + name);\n"
"            return;\n"
"        }\n"
"\n"
"        var innerHTML = '';\n"
"        innerHTML += '<span id=\"close\"> ' +\n"
"                     '<a href=\"javascript:hidePopups()\"' +\n"
"                     'style=\"text-decoration: none\">' +\n"
"                     '<strong>[x]</strong></a></span>';\n"
"        innerHTML += '<div align=\"center\"><b>Package: ' + \n"
"                     package['name'] + '</b></div>';\n"
"        innerHTML += '<div style=\"height:380px;overflow:auto;\">';"
"\n"
"        var test;\n"
"        var test_prev;\n"
"        for (var test_no in test_list)\n"
"        {\n"
"            var test = test_list[test_no];\n"
"            if ((!test_prev) or (test['path'] != test_prev['path']))\n"
"            {\n"
"                innerHTML += test['path'] + '<br/>';\n"
"            }\n"
"            innerHTML += '&nbsp;&nbsp;&nbsp;&nbsp;' + test['name'] +\n"
"                         '&nbsp;' + test['count'] + '<br/>';\n"
"        }\n"
"        innerHTML += '</div>';"
"\n"
"        return innerHTML;\n"
"    }\n"
"  </script>\n"
#endif
"</head>\n"
"<body lang=\"en-US\" dir=\"LTR\">\n";

static const char * const trc_diff_html_doc_end =
"</body>\n"
"</html>\n";

static const char * const trc_diff_stats_table_start =
"<table border=1 cellpadding=4 cellspacing=3 "
"style=\"font-size:small;\">\n"
"  <thead>\n"
"    <tr>\n"
"      <td rowspan=3>\n"
"        <b>%s</b>\n"
"      </td>\n"
"      <td colspan=7 align=center>\n"
"        <b>%s</b>\n"
"      </td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td align=center colspan=2><b>PASSED</b></td>\n"
"      <td align=center colspan=2><b>FAILED</b></td>\n"
"      <td rowspan=2><b>UNSTABLE</b></td>\n"
"      <td rowspan=2><b>skipped</b></td>\n"
"      <td rowspan=2><b>unspecified</b></td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td><b>Expected</b></td>\n"
"      <td><b>Unexpected</b></td>\n"
"      <td><b>Expected</b></td>\n"
"      <td><b>Unexpected</b></td>\n"
"    </tr>\n"
"  </thead>\n"
    "  <tbody align=center>\n";

static const char * const trc_diff_stats_table_row_start =
"    <tr>\n"
"      <td align=left><b>%s</b></td>\n"
"      <td>";

static const char * const trc_diff_stats_table_entry_matched =
"<font class=\"matched\">%u</font>";

static const char * const trc_diff_stats_table_entry_unmatched =
"<font class=\"unmatched\">%u</font>";

static const char * const trc_diff_stats_table_entry_ignored =
"<font class=\"ignored\">%u</font>";

static const char * const trc_diff_stats_table_row_mid =
"      </td>\n"
"      <td>\n";

static const char * const trc_diff_stats_table_row_end =
"      </td>\n"
"    </tr>\n";

static const char * const trc_diff_stats_table_end = 
"    <tr>\n"
"      <td align=left colspan=8><h3>Total run: "
"<font class=\"matched\">%u</font>+"
"<font class=\"unmatched\">%u</font>+"
"<font class=\"ignored\">%u</font>=%u</h3></td>"
"    </tr>\n"
"    <tr>\n"
"      <td align=left colspan=8>[<font class=\"matched\">X</font>+]"
                                "<font class=\"unmatched\">Y</font>+"
                                "<font class=\"ignored\">Z</font><br/>"
"X - result match, Y - result does not match (to be fixed), "
"Z - result does not match (ignored)</td>"
"    </tr>\n"
"  </tbody>\n"
"</table>\n";

static const char * const trc_diff_stats_brief_counter =
"<font class=\"%s\">%u</font>";

static const char * const trc_diff_stats_brief_table_start = 
"<table border=1 cellpadding=4 cellspacing=3 "
"style=\"font-size:small;\">\n"
"  <thead>\n"
"    <tr><td colspan=6 align=center><b>Brief Diff Stats: %s</b></td></tr>\n"
"    <tr><td rowspan=2><b>Diff with:</b></td>\n"
"        <td align=center colspan=2><b>PASSED</b></td>\n"
"        <td align=center colspan=2><b>FAILED</b></td>\n"
"        <td align=center rowspan=2><b>TOTAL</b></td>\n"
"    </tr>\n"
"    <tr>\n"
"        <td align=center><b>Expected</b></td>\n"
"        <td align=center><b>Unexpected</b></td>\n"
"        <td align=center><b>Expected</b></td>\n"
"        <td align=center><b>Unexpected</b></td>\n"
"    </tr>\n"
"  </thead>\n"
"  <tbody align=center>\n";

static const char * const trc_diff_stats_brief_table_row_start =
"    <tr><td><b>%s</b></td>\n";

static const char * const trc_diff_stats_brief_table_row_end =
"    </tr>\n";

static const char * const trc_diff_stats_brief_table_end =
"  </tbody>\n"
"</table>\n";

static const char * const trc_diff_key_table_heading =
"<table border=1 cellpadding=4 cellspacing=3 style=\"font-size:small;\">\n"
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

static const char * const trc_diff_key_table_row_start =
"    <tr>\n"
"      <td>";

static const char * const trc_diff_key_table_row_end =
"</td>\n"
"      <td align=right>%u</td>\n"
"    </tr>\n";


static const char * const trc_diff_full_table_heading_start =
"<table border=1 cellpadding=4 cellspacing=3 style=\"font-size:small;\">\n"
"  <thead>\n"
"    <tr>\n"
"      <td>\n"
"        <b>Name</b>\n"
"      </td>\n"
"      <td>\n"
"        <b>Objective</b>\n"
"      </td>\n";

static const char * const trc_diff_brief_table_heading_start =
"<table border=1 cellpadding=4 cellspacing=3 style=\"font-size:small;\">\n"
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

static const char * const trc_diff_test_result_start =
"<font class=\"%s\">";

static const char * const trc_diff_test_result_end =
"</font>";

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
        fprintf(f, "<br/>");
    }
    fprintf(f, "<br/>");
}

static void
trc_diff_stats_entry_to_html(FILE *f, trc_diff_stats_counter *counters)
{
    if ((counters[TRC_DIFF_MATCH] == 0)    &&
        (counters[TRC_DIFF_NO_MATCH] == 0) &&
        (counters[TRC_DIFF_NO_MATCH_IGNORE] == 0))
    {
        fprintf(f, "0");
        return;
    }

    if (counters[TRC_DIFF_MATCH] > 0)
    {
        fprintf(f, trc_diff_stats_table_entry_matched,
                counters[TRC_DIFF_MATCH]);
        if (counters[TRC_DIFF_NO_MATCH] +
            counters[TRC_DIFF_NO_MATCH_IGNORE] > 0)
            fprintf(f, " + ");
    }
    if (counters[TRC_DIFF_NO_MATCH] > 0)
    {
        fprintf(f, trc_diff_stats_table_entry_unmatched,
                counters[TRC_DIFF_NO_MATCH]);
        if (counters[TRC_DIFF_NO_MATCH_IGNORE] > 0)
            fprintf(f, " + ");
    }
    if (counters[TRC_DIFF_NO_MATCH_IGNORE] > 0)
    {
        fprintf(f, trc_diff_stats_table_entry_ignored,
                counters[TRC_DIFF_NO_MATCH_IGNORE]);
    }
}

static const char *
trc_diff_test_status_to_str(trc_test_status status)
{
    switch (status)
    {
        case TRC_TEST_PASSED:
            return "passed";
        case TRC_TEST_PASSED_UNE:
            return "PASSED unexpectedly";
        case TRC_TEST_FAILED:
            return "failed";
        case TRC_TEST_FAILED_UNE:
            return "FAILED unexpectedly";
        case TRC_TEST_UNSTABLE:
            return "UNSTABLE";
        case TRC_TEST_SKIPPED:
            return "skipped";
        case TRC_TEST_UNSPECIFIED:
            return "unspecified";
        default:
            break;
    }

    return "unknown";
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
    trc_diff_stats_counters *counters;
    trc_diff_stats_counter   total_match            = 0;
    trc_diff_stats_counter   total_no_match         = 0;
    trc_diff_stats_counter   total_no_match_ignored = 0;
    trc_diff_stats_counter   total                  = 0;
    trc_test_status          status1;
    trc_test_status          status2;

    counters = &((*stats)[tags_x->id][tags_y->id - 1]);

    for (status1 = TRC_TEST_PASSED;
         status1 < TRC_TEST_SKIPPED; status1++)
    {
        for (status2 = TRC_TEST_PASSED;
             status2 < TRC_TEST_SKIPPED; status2++)
        {
            total_match += (*counters)[status1][status2][TRC_DIFF_MATCH];
            total_no_match +=
                (*counters)[status1][status2][TRC_DIFF_NO_MATCH];
            total_no_match_ignored +=
                (*counters)[status1][status2][TRC_DIFF_NO_MATCH_IGNORE];
        }
    }

    total = total_match + total_no_match + total_no_match_ignored;

    fprintf(f, trc_diff_stats_table_start, tags_x->name, tags_y->name);
    for (status1 = TRC_TEST_PASSED;
         status1 < TRC_TEST_STATUS_MAX; status1++)
    {
        fprintf(f, trc_diff_stats_table_row_start,
                trc_diff_test_status_to_str(status1));
        for (status2 = TRC_TEST_PASSED;
             status2 < TRC_TEST_STATUS_MAX;
             status2++)
        {
            trc_diff_stats_entry_to_html(f, (*counters)[status1][status2]);
            if (status2 < TRC_TEST_UNSPECIFIED)
                fprintf(f, trc_diff_stats_table_row_mid);
        }
        fprintf(f, trc_diff_stats_table_row_end);
    }

    fprintf(f, trc_diff_stats_table_end,
            total_match, total_no_match, total_no_match_ignored, total);
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
trc_diff_one_stats_brief_to_html(FILE               *f,
                                 trc_diff_stats     *stats,
                                 const trc_diff_set *tags_x,
                                 const trc_diff_set *tags_y)
{
    trc_diff_stats_counters *counters;

    trc_diff_stats_counter  total[TRC_TEST_STATUS_MAX];
    trc_diff_stats_counter  total_x[TRC_TEST_STATUS_MAX];
    trc_diff_stats_counter  total_y[TRC_TEST_STATUS_MAX];
    trc_test_status         status_x;
    trc_test_status         status_y;
    trc_diff_status         diff;

    counters = &((*stats)[tags_x->id][tags_y->id - 1]);

    memset(total, 0, sizeof(total_x));
    memset(total_x, 0, sizeof(total_x));
    memset(total_y, 0, sizeof(total_y));

    for (status_x = TRC_TEST_PASSED;
         status_x < TRC_TEST_SKIPPED; status_x++)
    {
        for (status_y = TRC_TEST_PASSED;
             status_y < TRC_TEST_SKIPPED; status_y++)
        {
            if (status_x != status_y)
            {
                for (diff = TRC_DIFF_MATCH;
                     diff < TRC_DIFF_STATUS_MAX; diff++)
                {
                    total_x[status_x] +=
                        (*counters)[status_x][status_y][diff];
                    total_y[status_y] +=
                        (*counters)[status_x][status_y][diff];
                }
            }
            else
            {
                total[status_x] +=
                    (*counters)[status_y][status_x][TRC_DIFF_MATCH];
            }
        }
    }
    total[TRC_TEST_PASSED_UNE] +=
        (*counters)[TRC_TEST_PASSED][TRC_TEST_PASSED]
            [TRC_DIFF_NO_MATCH] +
        (*counters)[TRC_TEST_PASSED_UNE][TRC_TEST_PASSED_UNE]
            [TRC_DIFF_NO_MATCH];
    total[TRC_TEST_FAILED_UNE] +=
        (*counters)[TRC_TEST_FAILED][TRC_TEST_FAILED]
            [TRC_DIFF_NO_MATCH] +
        (*counters)[TRC_TEST_FAILED_UNE][TRC_TEST_FAILED_UNE]
            [TRC_DIFF_NO_MATCH];

    for (status_x = TRC_TEST_PASSED;
         status_x < TRC_TEST_SKIPPED; status_x++)
    {
        total_x[status_x] +=
            (*counters)[TRC_TEST_SKIPPED][status_x]
                [TRC_DIFF_NO_MATCH_IGNORE];
        total_y[status_x] +=
            (*counters)[status_x][TRC_TEST_SKIPPED]
                [TRC_DIFF_NO_MATCH_IGNORE];
    }

#define WRITE_COUNTER(color_, counter_, prefix_)        \
    do                                                  \
    {                                                   \
        char *prefix = (prefix_);                       \
        if (prefix != NULL)                             \
        {                                               \
            if ((counter_) > 0)                         \
            {                                           \
                fprintf(f, " %s ", prefix);             \
            }                                           \
            else                                        \
                break;                                  \
        }                                               \
                                                        \
        fprintf(f, trc_diff_stats_brief_counter,        \
                (color_), (counter_));                  \
    } while (0)

    fprintf(f, "<td>");
    WRITE_COUNTER("passed", total[TRC_TEST_PASSED], NULL);
    WRITE_COUNTER("passed_new", total_y[TRC_TEST_PASSED], "+");
    WRITE_COUNTER("passed_old", total_x[TRC_TEST_PASSED], "-");
    fprintf(f, "</td>");

    fprintf(f, "<td>");
    WRITE_COUNTER("passed_une", total[TRC_TEST_PASSED_UNE], NULL);
    WRITE_COUNTER("passed_une_new", total_y[TRC_TEST_PASSED_UNE], "+");
    WRITE_COUNTER("passed_une_old", total_x[TRC_TEST_PASSED_UNE], "-");
    fprintf(f, "</td>");

    fprintf(f, "<td>");
    WRITE_COUNTER("failed", total[TRC_TEST_FAILED], NULL);
    WRITE_COUNTER("failed_new", total_y[TRC_TEST_FAILED], "+");
    WRITE_COUNTER("failed_old", total_x[TRC_TEST_FAILED], "-");
    fprintf(f, "</td>");

    fprintf(f, "<td>");
    WRITE_COUNTER("failed_une", total[TRC_TEST_FAILED_UNE], NULL);
    WRITE_COUNTER("failed_une_new", total_y[TRC_TEST_FAILED_UNE], "+");
    WRITE_COUNTER("failed_une_old", total_x[TRC_TEST_FAILED_UNE], "-");
    fprintf(f, "</td>");

    fprintf(f, "<td>");
    WRITE_COUNTER("total",
                  total[TRC_TEST_PASSED] + total[TRC_TEST_PASSED_UNE] +
                  total[TRC_TEST_FAILED] + total[TRC_TEST_FAILED_UNE],
                  NULL);
    WRITE_COUNTER("total_new",
                  total_y[TRC_TEST_PASSED] + total_y[TRC_TEST_PASSED_UNE] +
                  total_y[TRC_TEST_FAILED] + total_y[TRC_TEST_FAILED_UNE] +
                  total_y[TRC_TEST_SKIPPED], "+");
    WRITE_COUNTER("total_old",
                  total_y[TRC_TEST_PASSED] + total_y[TRC_TEST_PASSED_UNE] +
                  total_y[TRC_TEST_FAILED] + total_y[TRC_TEST_FAILED_UNE] +
                  total_y[TRC_TEST_SKIPPED], "-");
    fprintf(f, "</td>");
#undef WRITE_COUNTER
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
trc_diff_stats_brief_to_html(FILE *f, const trc_diff_sets *sets,
                             trc_diff_stats *stats)
{
    const trc_diff_set *tags_i;
    const trc_diff_set *tags_j;

    TAILQ_FOREACH(tags_i, sets, links)
    {
        if (TAILQ_NEXT(tags_i, links) == NULL)
            break;

        fprintf(f, trc_diff_stats_brief_table_start, tags_i->name);
        for (tags_j = TAILQ_NEXT(tags_i, links);
             tags_j != NULL;
             tags_j = TAILQ_NEXT(tags_j, links))
        {
                fprintf(f, trc_diff_stats_brief_table_row_start,
                        tags_j->name);
                trc_diff_one_stats_brief_to_html(f, stats, tags_i, tags_j);
                fprintf(f, trc_diff_stats_brief_table_row_end);
        }
        fprintf(f, trc_diff_stats_brief_table_end);
    }
}

/**
 * Output brief statistics to HTML report
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
                WRITE_STR(trc_diff_key_table_row_start);
                trc_re_key_substs(TRC_RE_KEY_URL, q->key, f);
                fprintf(f, trc_diff_key_table_row_end, q->count);
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
    const trc_diff_set         *set;
    trc_report_test_iter_data  *iter_data;
    trc_report_test_iter_entry *iter_entry;
    te_errno                    rc = 0;

    for (set = TAILQ_FIRST(sets);
         set != NULL && rc == 0;
         set = TAILQ_NEXT(set, links))
    {
        WRITE_STR(trc_diff_table_row_col_start);
        rc = trc_exp_result_to_html(f, entry->results[set->id], flags);
        if (set->log)
        {
            if (entry->is_iter)
            {
                iter_data = trc_db_iter_get_user_data(entry->ptr.iter,
                                                      set->db_uid);
                if (iter_data != NULL)
                {
                    TAILQ_FOREACH(iter_entry, &iter_data->runs, links)
                    {
                        WRITE_STR("<HR/>");
                        fprintf(f, trc_diff_test_result_start,
                                (iter_entry->is_exp) ?
                                "matched" : "unmatched");
                        rc = te_test_result_to_html(f, &iter_entry->result);
                        fprintf(f, trc_diff_test_result_end);
                    }

                    if (TAILQ_EMPTY(&iter_data->runs))
                    {
                        WRITE_STR(te_test_status_to_str(TE_TEST_SKIPPED));
                    }
                }
                else
                {
                    WRITE_STR("<HR/>");
                    fprintf(f, trc_diff_test_result_start, "skipped");
                    WRITE_STR(te_test_status_to_str(TE_TEST_SKIPPED));
                    fprintf(f, trc_diff_test_result_end);
                }
            }
        }
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
        {
            fprintf(f, "<em>%s</em> - ", set->name);
            trc_re_key_substs(TRC_RE_KEY_URL,
                              entry->results[set->id]->key, f);
            fputs("<br/>", f);
        }
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
            if (str != TAILQ_FIRST(entry->keys + set->id))
                fputs(", ", f);
            trc_re_key_substs(TRC_RE_KEY_TAGS, str->v, f);
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
    te_string               test_name = TE_STRING_INIT;

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
            if (curr->level > 0)
            {
                rc = te_string_append(&test_name, "*-");
                if (rc != 0)
                    goto cleanup;
            }

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
                    te_string_cut(&test_name, strlen(slash));
                } while (j-- > 0);
            }
            else
            {
                te_string_cut(&test_name,
                              ((curr->level - next->level) >> 1 << 1) + 2);
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
            (title != NULL) ? title : trc_diff_html_title_def);
    fprintf(f, "<h1 align=center>%s</h1>\n",
            (title != NULL) ? title : trc_diff_html_title_def);
    if (ctx->db->version != NULL)
        fprintf(f, "<h2 align=center>%s</h2>\n", ctx->db->version);

    /* Compared sets */
    trc_diff_tags_to_html(f, &ctx->sets);

    /* Output brief total statistics */
    trc_diff_stats_brief_to_html(f, &ctx->sets, &ctx->stats);

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
