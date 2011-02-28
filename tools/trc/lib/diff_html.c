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
"    .expected {font-weight: normal; color: blue}\n"
"    .expected {font-weight: normal; color: blue}\n"
"    .unexpected {font-weight: normal; color: red}\n"
"    .passed {font-weight: bold; color: black}\n"
"    .passed_new {font-weight: bold; color: green}\n"
"    .passed_old {font-weight: bold; color: red}\n"
"    .passed_ignored_new {font-weight: bold; color: blue}\n"
"    .passed_ignored_old {font-weight: bold; color: blue}\n"
"    .passed_une {font-weight: bold; color: black}\n"
"    .passed_une_new {font-weight: bold; color: red}\n"
"    .passed_une_old {font-weight: bold; color: green}\n"
"    .passed_une_ignored_new {font-weight: bold; color: blue}\n"
"    .passed_une_ignored_old {font-weight: bold; color: blue}\n"
"    .failed {font-weight: bold; color: black}\n"
"    .failed_new {font-weight: bold; color: green}\n"
"    .failed_old {font-weight: bold; color: red}\n"
"    .failed_ignored_new {font-weight: bold; color: blue}\n"
"    .failed_ignored_old {font-weight: bold; color: blue}\n"
"    .failed_une {font-weight: bold; color: black}\n"
"    .failed_une_new {font-weight: bold; color: red}\n"
"    .failed_une_old {font-weight: bold; color: green}\n"
"    .failed_une_ignored_new {font-weight: bold; color: blue}\n"
"    .failed_une_ignored_old {font-weight: bold; color: blue}\n"
"    .skipped {font-weight: normal; color: grey}\n"
"    .total {font-weight: bold; color: black}\n"
"    .total_new {font-weight: bold; color: black}\n"
"    .total_old {font-weight: bold; color: black}\n"
"    .matched {font-weight: bold; color: green}\n"
"    .unmatched {font-weight: bold; color: red}\n"
"    .ignored {font-weight: italic; color: blue}\n"
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
"  </style>\n"
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
"        innerHTML += '<div style=\"height:380px;overflow:auto;\">';\n"
"\n"
"        innerHTML += '<table border=1 cellpadding=4 cellspacing=3 ' + "
"'width=100% style=\"font-size:small;text-align:right;\">';\n"
"        innerHTML += '<tr><td><b>Count</b></td>' +\n"
"                     '<td><b>Test</b></td></tr>';\n"
"\n"
"        var test;\n"
"        var path = \"\";\n"
"        for (var test_no in test_list)\n"
"        {\n"
"            var test = test_list[test_no];\n"
"            if (path != test['path'])\n"
"            {\n"
"                innerHTML += '<tr><td colspan=2 "
"style=\"text-align:left;\"><b>' + test['path'] + '</b></td></tr>';\n"
"                path = test['path'];\n"
"            }\n"
"            innerHTML += '<tr><td style=\"text-align:right;\">' +\n"
"                         test['count'] + '</td>';\n"
"            innerHTML += '<td style=\"text-align:left;\">';\n"
"            if (test['hash'])\n"
"            {\n"
"                innerHTML += '<a href=\"#' + test['hash'] + '\">' +\n"
"                             test['name'] + '</a>';\n"
"            }\n"
"            else\n"
"            {\n"
"                innerHTML += test['name'];\n"
"            }\n"
"            innerHTML += '</td></tr>';\n"
"        }\n"
"        innerHTML += '</table></div>';\n"
"        return innerHTML;\n"
"    }\n"
"\n"
"    function centerTip(obj,pos)\n"
"    {\n"
"        var scrolled_x, scrolled_y;\n"
"        if (self.pageYOffset)\n"
"        {\n"
"            scrolled_x = self.pageXOffset;\n"
"            scrolled_y = self.pageYOffset;\n"
"        }\n"
"        else if (document.documentElement &&\n"
"                 document.documentElement.scrollTop)\n"
"        {\n"
"            scrolled_x = document.documentElement.scrollLeft;\n"
"            scrolled_y = document.documentElement.scrollTop;\n"
"        }\n"
"        else if (document.body)\n"
"        {\n"
"            scrolled_x = document.body.scrollLeft;\n"
"            scrolled_y = document.body.scrollTop;\n"
"        }\n"
"\n"
"        var center_x, center_y;\n"
"        if (self.innerHeight)\n"
"        {\n"
"            center_x = self.innerWidth;\n"
"            center_y = self.innerHeight;\n"
"        }\n"
"        else if (document.documentElement &&\n"
"                 document.documentElement.clientHeight)\n"
"        {\n"
"            center_x = document.documentElement.clientWidth;\n"
"            center_y = document.documentElement.clientHeight;\n"
"        }\n"
"        else if (document.body)\n"
"        {\n"
"            center_x = document.body.clientWidth;\n"
"            center_y = document.body.clientHeight;\n"
"        }\n"
"\n"
"        if (pos == 'right')\n"
"            var leftOffset = scrolled_x +\n"
"                             (center_x - obj.offsetWidth) - 20;\n"
"        else\n"
"            var leftOffset = scrolled_x +\n"
"                             (center_x - obj.offsetWidth) / 2;\n"
"\n"
"        var topOffset = scrolled_y + (center_y - obj.offsetHeight) / 2;\n"
"\n"
"        obj.style.top = topOffset + 'px';\n"
"        obj.style.left = leftOffset + 'px';\n"
"    }\n"
"\n"
"    function showList(test_list)\n"
"    {\n"
"        var obj = document.getElementById('StatsTip');\n"
"        if (typeof obj == 'undefined')\n"
"            alert('Failed to get object ' + 'StatsTip');\n"
"\n"
"        obj.innerHTML = fillTip(test_list);\n"
"\n"
"        //alert(obj.innerHTML);\n"
"        centerTip(obj);\n"
"        obj.style.visibility = \"visible\";\n"
"    }\n"
"\n"
"    function hideObject(obj_name)\n"
"    {\n"
"        var obj = document.getElementById(obj_name);\n"
"        if (typeof obj == 'undefined')\n"
"            alert('Failed to get object ' + obj_name);\n"
"\n"
"        obj.style.visibility = \"hidden\";\n"
"    }\n"
"\n"
"    function hidePopups()\n"
"    {\n"
"        hideObject('StatsTip');\n"
"    }\n"
"\n"
"    function doNothing(ev)\n"
"    {\n"
"        ev = ev ? ev : window.event;\n"
"        ev.cancelBubble = true;\n"
"        if (ev.stopPropagation)\n"
"        {\n"
"            ev.stopPropagation();\n"
"        }\n"
"    }\n"
"  </script>\n"
"</head>\n"
"<body lang=\"en-US\" dir=\"ltr\" onClick=\"hidePopups()\" "
"onScroll=\"centerTip(document.getElementById('StatsTip'));return false;\">"
"\n"
"    <font face=\"Verdana\">\n"
"    <div id=\"StatsTip\" onClick=\"doNothing(event)\">\n"
"      <span id=\"close\"><a href=\"javascript:hidePopups()\""
" style=\"text-decoration: none\"><strong>[x]</strong></a></span>\n"
"      <p>Tests Statistics</p><br/>\n"
"    </div>\n";

static const char * const trc_diff_html_doc_end =
"</font>\n"
"</body>\n"
"</html>\n";

static const char * const trc_diff_js_table_align = "    ";

static const char * const trc_diff_js_table_start =
"  <script type=\"text/javascript\">\n"
"    var diff_stats_map = {\n";

static const char * const trc_diff_js_table_row_start =
"\"%s\":{\n";

static const char * const trc_diff_js_table_row_end =
"},\n";

static const char * const trc_diff_js_table_entry_start =
"total:%d, tests:[\n";

static const char * const trc_diff_js_table_entry_end =
"]\n";

static const char * const trc_diff_js_table_entry_test =
"{name:\"%s\", path:\"%s\", count:%d, hash:\"%s\"},\n";

static const char * const trc_diff_js_table_end =
"  };\n"
"  </script>\n";

static const char * const trc_diff_js_stats_start =
"  <script type=\"text/javascript\">\n"
"  function calcSummary(set1,set2)\n"
"  {\n"
"      var stats = diff_stats_map[set1][set2];\n"
"      var status_type = new Array('passed','passed_une',\n"
"                                  'failed','failed_une');\n"
"\n"
"      for (var s1 in status_type)\n"
"      {\n"
"          var status1 = status_type[s1];\n"
"          for (var s2 in status_type)\n"
"          {\n"
"              var status2 = status_type[s2];\n"
"\n"
"              if (status1 == status2)\n"
"              {\n"
"                  stats['summary'][status1]['match']['total'] +=\n"
"                          stats[status1][status2]['match']['total'];\n"
"                  stats['summary'][status1]['match']['tests'] =\n"
"                    stats['summary'][status1]['match']['tests'].concat(\n"
"                      stats[status1][status2]['match']['tests']);\n"
"              }\n"
"              else\n"
"              {\n"
"                  stats['summary'][status1]['old']['total'] +=\n"
"                          stats[status1][status2]['unmatch']['total'];\n"
"\n"
"                  stats['summary'][status1]['old']['tests'] =\n"
"                      stats['summary'][status1]['old']['tests'].concat(\n"
"                          stats[status1][status2]['unmatch']['tests']);\n"
"\n"
"                  stats['summary'][status2]['new']['total'] +=\n"
"                          stats[status1][status2]['unmatch']['total'];\n"
"\n"
"                  stats['summary'][status2]['new']['tests'] =\n"
"                      stats['summary'][status2]['new']['tests'].concat(\n"
"                          stats[status1][status2]['unmatch']['tests']);\n"
"              }\n"
"          }\n"
"\n"
"          stats['summary'][status1]['skipped_old']['total'] +=\n"
"              stats[status1]['skipped']['ignore']['total'];\n"
"          stats['summary'][status1]['skipped_old']['tests'] =\n"
"              stats['summary'][status1]['skipped_old']['tests'].concat(\n"
"                  stats[status1]['skipped']['ignore']['tests']);\n"
" \n"
"          stats['summary'][status1]['skipped_new']['total'] +=\n"
"              stats['skipped'][status1]['ignore']['total'];\n"
"          stats['summary'][status1]['skipped_new']['tests'] =\n"
"              stats['summary'][status1]['skipped_new']['tests'].concat(\n"
"                  stats['skipped'][status1]['ignore']['tests']);\n"
" \n"
"          stats['summary']['total']['old']['total'] +=\n"
"              stats[status1]['skipped']['ignore']['total'];\n"
"          stats['summary']['total']['old']['tests'] =\n"
"              stats['summary']['total']['old']['tests'].concat(\n"
"                  stats[status1]['skipped']['ignore']['tests']);\n"
" \n"
"          stats['summary']['total']['new']['total'] +=\n"
"              stats['skipped'][status1]['ignore']['total'];\n"
"          stats['summary']['total']['new']['tests'] =\n"
"              stats['summary']['total']['new']['tests'].concat(\n"
"                  stats['skipped'][status1]['ignore']['tests']);\n"
" \n"
"          stats['summary']['total']['match']['total'] +=\n"
"              stats[status1][status1]['match']['total'];\n"
"          stats['summary']['total']['match']['tests'] =\n"
"              stats['summary']['total']['match']['tests'].concat(\n"
"                  stats[status1][status1]['match']['tests']);\n"
"          \n"
"      }\n"
"  }\n";

static const char * const trc_diff_js_stats_row =
"  calcSummary('%s','%s');";

static const char * const trc_diff_js_stats_end =
"  </script>\n";


static const char * const trc_diff_stats_table_start =
"<table border=1 cellpadding=4 cellspacing=3 "
"style=\"font-size:small;\" id=\"%s_%s\">\n"
"  <thead>\n"
"    <tr>\n"
"      <td align=center>-\\-</td><td><b>%s</b></td>\n"
"      <td align=center colspan=2><b>PASSED</b></td>\n"
"      <td align=center colspan=2><b>FAILED</b></td>\n"
"      <td rowspan=2><b>UNSTABLE</b></td>\n"
"      <td rowspan=2><b>skipped</b></td>\n"
"      <td rowspan=2><b>unspecified</b></td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td><b>%s</b></td><td align=center>-\\-</td>\n"
"      <td><b>Expected</b></td>\n"
"      <td><b>Unexpected</b></td>\n"
"      <td><b>Expected</b></td>\n"
"      <td><b>Unexpected</b></td>\n"
"    </tr>\n"
"  </thead>\n"
    "  <tbody align=center>\n";

static const char * const trc_diff_stats_table_double_row_start =
"    <tr>\n"
"      <td align=left rowspan=2>\n<b>%s</b>\n</td>\n"
"      <td align=left>\n<b>%s</b>\n</td>\n"
"      <td>\n";

static const char * const trc_diff_stats_table_single_row_start =
"    <tr>\n"
"      <td align=left><b>%s</b></td>\n"
"      <td>";

static const char * const trc_diff_stats_table_row_start =
"    <tr>\n"
"      <td align=center colspan=2><b>%s</b></td>\n"
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
"      <td align=left colspan=9><h3>Total run: "
"<font class=\"matched\">%u</font>+"
"<font class=\"unmatched\">%u</font>+"
"<font class=\"ignored\">%u</font>=%u</h3></td>"
"    </tr>\n"
"    <tr>\n"
"      <td align=left colspan=9>[<font class=\"matched\">X</font>+]"
                                "<font class=\"unmatched\">Y</font>+"
                                "<font class=\"ignored\">Z</font><br/>"
"X - result match, Y - result does not match (to be fixed), "
"Z - result does not match (ignored)</td>"
"    </tr>\n"
"  </tbody>\n"
"</table><br/>\n";

static const char * const trc_diff_stats_stats_href_start =
"<a href=\"javascript:showList(";

static const char * const trc_diff_stats_stats_href_end =
");\">";

static const char * const trc_diff_stats_stats_list =
"diff_stats_map['%s']['%s']['%s']['%s']['%s']['tests']";

static const char * const trc_diff_stats_stats_concat_start =
".concat(";

static const char * const trc_diff_stats_stats_concat_end =
")";

static const char * const trc_diff_stats_stats_href_close =
"</a>";

static const char * const trc_diff_stats_brief_counter =
"<font class=\"%s\">%u</font>";

static const char * const trc_diff_stats_brief_table_start =
"<table border=1 cellpadding=4 cellspacing=3 "
"style=\"font-size:small;\">\n"
"  <thead>\n"
"    <tr><td colspan=6 align=center>"
"<b>Brief Diff Statistics: %s (reference)</b></td></tr>\n"
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

static const char * const trc_diff_stats_brief_increment_table_start =
"<table border=1 cellpadding=4 cellspacing=3 "
"style=\"font-size:small;\">\n"
"  <thead>\n"
"    <tr><td colspan=6 align=center>"
"<b>Brief Incremental Diff Statistics</b></td></tr>\n"
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

static const char * const trc_diff_stats_brief_table_row_href_start =
"    <tr><td><a href=\"#%s_%s\"><b>%s</b></a></td>\n";

static const char * const trc_diff_stats_brief_table_row_end =
"    </tr>\n";

static const char * const trc_diff_stats_brief_table_end =
"    <tr>\n"
"      <td align=left colspan=6>[<font class=\"total\">X</font>+]"
                                "<font class=\"matched\">Y_new</font>-"
                                "<font class=\"unmatched\">Y_old</font>+"
                                "<font class=\"ignored\">Z</font>+"
                                "<font class=\"ignored\">Z_new</font>-"
                                "<font class=\"ignored\">Z_old</font><br/>"
"X - result match<br/>"
"Y_new - new result does not match<br/>"
"Y_old - old result does not match<br/> "
"Z - result does not match (ignored)</td>"
"    </tr>\n"
"  </tbody>\n"
"</table><br/>\n";

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
"</table><br/>\n";

static const char * const trc_diff_full_table_test_row_start =
"    <tr>\n"
"      <td><a name=\"%u\"/>%s<b>%s</b></td>\n"  /* Name */
"      <td>%s</td>\n";          /* Objective */

static const char * const trc_diff_brief_table_test_row_start =
"    <tr>\n"
"      <td><a href=\"#%u\">%s</a></td>\n";  /* Name */

static const char * const trc_diff_table_iter_row_start =
"    <tr>\n"
"      <td id=\"%s\" colspan=2><a name=\"%u\"/>";

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

static const char * const trc_diff_test_iter_hlink =
"<a name=\"%s\"> </a>";

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

char *
trc_diff_js_test_status2str(int status)
{
    switch (status)
    {
        case TRC_TEST_PASSED:
            return "passed";
        case TRC_TEST_PASSED_UNE:
            return "passed_une";
        case TRC_TEST_FAILED:
            return "failed";
        case TRC_TEST_FAILED_UNE:
            return "failed_une";
        case TRC_TEST_UNSTABLE:
            return "unstable";
        case TRC_TEST_SKIPPED:
            return "skipped";
        case TRC_TEST_UNSPECIFIED:
            return "unspecified";
        default:
            return NULL;
    }
}

char *
trc_diff_js_diff_status2str(int status)
{
    switch (status)
    {
        case TRC_DIFF_MATCH:
            return "match";
        case TRC_DIFF_NO_MATCH:
            return "unmatch";
        case TRC_DIFF_NO_MATCH_IGNORE:
            return "ignore";
        default:
            return NULL;
    }
}

char *
trc_diff_js_summary_status2str(int summary_status)
{
    switch (summary_status)
    {
        case TRC_DIFF_SUMMARY_PASSED:
            return "passed";
        case TRC_DIFF_SUMMARY_PASSED_UNE:
            return "passed_une";
        case TRC_DIFF_SUMMARY_FAILED:
            return "failed";
        case TRC_DIFF_SUMMARY_FAILED_UNE:
            return "failed_une";
        case TRC_DIFF_SUMMARY_TOTAL:
            return "total";
        default:
            return NULL;
    }
}

char *
trc_diff_js_summary_component2str(int summary_component)
{
    switch (summary_component)
    {
        case TRC_DIFF_SUMMARY_MATCH:
            return "match";
        case TRC_DIFF_SUMMARY_NEW:
            return "new";
        case TRC_DIFF_SUMMARY_OLD:
            return "old";
        case TRC_DIFF_SUMMARY_SKIPPED_MATCH:
            return "skipped_match";
        case TRC_DIFF_SUMMARY_SKIPPED_NEW:
            return "skipped_new";
        case TRC_DIFF_SUMMARY_SKIPPED_OLD:
            return "skipped_old";
        default:
            return NULL;
    }
}

static void
trc_diff_stats_js_table_align(FILE *f, unsigned int align)
{
    while (align-- != 0)
        fprintf(f, trc_diff_js_table_align);
}

void
trc_diff_stats_js_table_report(FILE *f, trc_diff_stats *stats,
                               const trc_diff_sets *sets)
{
    const trc_diff_set *set1;
    const trc_diff_set *set2;
    int                 status1;
    int                 status2;
    int                 diff;
    int                 summary_status;
    int                 summary_component;
    trc_diff_stats_counters *counters;

    fprintf(f, trc_diff_js_table_start);

    TAILQ_FOREACH(set1, sets, links)
    {
        trc_diff_stats_js_table_align(f, 1);
        fprintf(f, trc_diff_js_table_row_start, set1->name);

        TAILQ_FOREACH(set2, sets, links)
        {
            trc_diff_stats_js_table_align(f, 2);
            fprintf(f, trc_diff_js_table_row_start, set2->name);

            counters = &(*stats)[set1->id][set2->id];
            for (status1 = TRC_TEST_PASSED;
                 status1 < TRC_TEST_STATUS_MAX;
                 status1++)
            {
                trc_diff_stats_js_table_align(f, 3);
                fprintf(f, trc_diff_js_table_row_start,
                        trc_diff_js_test_status2str(status1));
                for (status2 = TRC_TEST_PASSED;
                     status2 < TRC_TEST_STATUS_MAX;
                     status2++)
                {
                    trc_diff_stats_js_table_align(f, 4);
                    fprintf(f, trc_diff_js_table_row_start,
                            trc_diff_js_test_status2str(status2));
                    for (diff = TRC_DIFF_MATCH;
                         diff < TRC_DIFF_STATUS_MAX;
                         diff++)
                    {
                        trc_diff_stats_counter_list_entry *p = NULL;
                        trc_diff_stats_counter *counter =
                            &(*counters)[status1][status2][diff];

                        trc_diff_stats_js_table_align(f, 5);
                        fprintf(f, trc_diff_js_table_row_start,
                                trc_diff_js_diff_status2str(diff));

                        trc_diff_stats_js_table_align(f, 6);
                        fprintf(f, trc_diff_js_table_entry_start,
                                counter->counter);

                        if (counter->counter > 0)
                        {
                            TAILQ_FOREACH(p, &counter->entries, links)
                            {
                                char *test_path = NULL;
                                if (p->test == NULL)
                                    continue;

                                test_path = strndup(p->test->path,
                                                    strlen(p->test->path) -
                                                    strlen(p->test->name));

                                trc_diff_stats_js_table_align(f, 7);
                                fprintf(f,
                                    trc_diff_js_table_entry_test,
                                        p->test->name, test_path,
                                        p->count, (p->hash != NULL) ?
                                                   p->hash : "");
                                free(test_path);
                            }
                        }
                        trc_diff_stats_js_table_align(f, 6);
                        fprintf(f, trc_diff_js_table_entry_end);
                        trc_diff_stats_js_table_align(f, 5);
                        fprintf(f, trc_diff_js_table_row_end);
                    }
                    trc_diff_stats_js_table_align(f, 4);
                    fprintf(f, trc_diff_js_table_row_end);
                }
                trc_diff_stats_js_table_align(f, 3);
                fprintf(f, trc_diff_js_table_row_end);
            }

            trc_diff_stats_js_table_align(f, 3);
            fprintf(f, trc_diff_js_table_row_start, "summary");
            for (summary_status = 0;
                 summary_status < TRC_DIFF_SUMMARY_STATUS_MAX;
                 summary_status++)
            {
                trc_diff_stats_js_table_align(f, 4);
                fprintf(f, trc_diff_js_table_row_start,
                        trc_diff_js_summary_status2str(summary_status));

                for (summary_component = 0;
                     summary_component < TRC_DIFF_SUMMARY_COMPONENT_MAX;
                     summary_component++)
                {
                    trc_diff_stats_js_table_align(f, 5);
                    fprintf(f, trc_diff_js_table_row_start,
                            trc_diff_js_summary_component2str(
                                summary_component));

                    trc_diff_stats_js_table_align(f, 6);
                    fprintf(f, trc_diff_js_table_entry_start, 0);

                    trc_diff_stats_js_table_align(f, 6);
                    fprintf(f, trc_diff_js_table_entry_end);

                    trc_diff_stats_js_table_align(f, 5);
                    fprintf(f, trc_diff_js_table_row_end);
                }

                trc_diff_stats_js_table_align(f, 4);
                fprintf(f, trc_diff_js_table_row_end);
            }

            trc_diff_stats_js_table_align(f, 3);
            fprintf(f, trc_diff_js_table_row_end);

            trc_diff_stats_js_table_align(f, 2);
            fprintf(f, trc_diff_js_table_row_end);
        }
        trc_diff_stats_js_table_align(f, 1);
        fprintf(f, trc_diff_js_table_row_end);
    }

    fprintf(f, trc_diff_js_table_end);

    fprintf(f, trc_diff_js_stats_start);
    TAILQ_FOREACH(set1, sets, links)
    {
        TAILQ_FOREACH(set2, sets, links)
        {
            fprintf(f, trc_diff_js_stats_row, set1->name, set2->name);
        }
    }
    fprintf(f, trc_diff_js_stats_end);
}

static void
trc_diff_stats_entry_to_html(FILE *f, trc_diff_stats_counter *counters,
                             char *set1_name, char *set2_name,
                             int status1, int status2)
{
    if ((counters[TRC_DIFF_MATCH].counter == 0)    &&
        (counters[TRC_DIFF_NO_MATCH].counter == 0) &&
        (counters[TRC_DIFF_NO_MATCH_IGNORE].counter == 0))
    {
        fprintf(f, "-");
        return;
    }

    if (counters[TRC_DIFF_MATCH].counter > 0)
    {
        fprintf(f, trc_diff_stats_stats_href_start);
        fprintf(f, trc_diff_stats_stats_list, set1_name, set2_name,
                trc_diff_js_test_status2str(status1),
                trc_diff_js_test_status2str(status2),
                trc_diff_js_diff_status2str(TRC_DIFF_MATCH));
        fprintf(f, trc_diff_stats_stats_href_end);

        fprintf(f, trc_diff_stats_table_entry_matched,
                counters[TRC_DIFF_MATCH].counter);

        fprintf(f, trc_diff_stats_stats_href_close);

        if (counters[TRC_DIFF_NO_MATCH].counter +
            counters[TRC_DIFF_NO_MATCH_IGNORE].counter > 0)
            fprintf(f, "+");
    }
    if (counters[TRC_DIFF_NO_MATCH].counter > 0)
    {
        fprintf(f, trc_diff_stats_stats_href_start);
        fprintf(f, trc_diff_stats_stats_list, set1_name, set2_name,
                trc_diff_js_test_status2str(status1),
                trc_diff_js_test_status2str(status2),
                trc_diff_js_diff_status2str(TRC_DIFF_NO_MATCH));
        fprintf(f, trc_diff_stats_stats_href_end);

        fprintf(f, trc_diff_stats_table_entry_unmatched,
                counters[TRC_DIFF_NO_MATCH].counter);

        fprintf(f, trc_diff_stats_stats_href_close);

        if (counters[TRC_DIFF_NO_MATCH_IGNORE].counter > 0)
            fprintf(f, "+");
    }
    if (counters[TRC_DIFF_NO_MATCH_IGNORE].counter > 0)
    {
        fprintf(f, trc_diff_stats_stats_href_start);
        fprintf(f, trc_diff_stats_stats_list, set1_name, set2_name,
                trc_diff_js_test_status2str(status1),
                trc_diff_js_test_status2str(status2),
                trc_diff_js_diff_status2str(TRC_DIFF_NO_MATCH_IGNORE));
        fprintf(f, trc_diff_stats_stats_href_end);

        fprintf(f, trc_diff_stats_table_entry_ignored,
                counters[TRC_DIFF_NO_MATCH_IGNORE].counter);

        fprintf(f, trc_diff_stats_stats_href_close);
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
    unsigned int             total_match            = 0;
    unsigned int             total_no_match         = 0;
    unsigned int             total_no_match_ignored = 0;
    unsigned int             total                  = 0;
    trc_test_status          status1;
    trc_test_status          status2;

    counters = &((*stats)[tags_x->id][tags_y->id]);

    for (status1 = TRC_TEST_PASSED;
         status1 < TRC_TEST_SKIPPED; status1++)
    {
        for (status2 = TRC_TEST_PASSED;
             status2 < TRC_TEST_SKIPPED; status2++)
        {
            total_match +=
                (*counters)[status1][status2][TRC_DIFF_MATCH].counter;
            total_no_match +=
                (*counters)[status1][status2][TRC_DIFF_NO_MATCH].counter;
            total_no_match_ignored +=
                (*counters)[status1][status2]
                           [TRC_DIFF_NO_MATCH_IGNORE].counter;
        }
    }

    total = total_match + total_no_match + total_no_match_ignored;

    fprintf(f, trc_diff_stats_table_start,
            tags_x->name, tags_y->name,
            tags_y->name, tags_x->name);
    for (status1 = TRC_TEST_PASSED;
         status1 < TRC_TEST_STATUS_MAX; status1++)
    {
        switch(status1)
        {
            case TRC_TEST_PASSED:
                fprintf(f, trc_diff_stats_table_double_row_start,
                        "PASSED", "Expected");
                break;
            case TRC_TEST_PASSED_UNE:
                fprintf(f, trc_diff_stats_table_single_row_start,
                        "Unexpected");
                break;
            case TRC_TEST_FAILED:
                fprintf(f, trc_diff_stats_table_double_row_start,
                        "FAILED", "Expected");
                break;
            case TRC_TEST_FAILED_UNE:
                fprintf(f, trc_diff_stats_table_single_row_start,
                        "Unexpected");
                break;
            case TRC_TEST_UNSTABLE:
                fprintf(f, trc_diff_stats_table_row_start,
                        "Unstable");
                break;
            case TRC_TEST_SKIPPED:
                fprintf(f, trc_diff_stats_table_row_start,
                        "skipped");
                break;
            case TRC_TEST_UNSPECIFIED:
                fprintf(f, trc_diff_stats_table_row_start,
                        "unspecified");
                break;
            default:
                break;
        }

        for (status2 = TRC_TEST_PASSED;
             status2 < TRC_TEST_STATUS_MAX;
             status2++)
        {
            trc_diff_stats_entry_to_html(f, (*counters)[status1][status2],
                                         tags_x->name, tags_y->name,
                                         status1, status2);
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

    unsigned int            total[TRC_TEST_STATUS_MAX];
    unsigned int            total_x[TRC_TEST_STATUS_MAX];
    unsigned int            total_y[TRC_TEST_STATUS_MAX];
    trc_test_status         status_x;
    trc_test_status         status_y;
    trc_diff_status         diff;
    te_bool                 empty;

    counters = &((*stats)[tags_x->id][tags_y->id]);

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
                        (*counters)[status_x][status_y][diff].counter;

                    total_y[status_y] +=
                        (*counters)[status_x][status_y][diff].counter;
                }
            }
            else
            {
                total[status_x] +=
                    (*counters)[status_x][status_x]
                               [TRC_DIFF_MATCH].counter +
                    (*counters)[status_x][status_x]
                               [TRC_DIFF_NO_MATCH].counter;
            }
        }
        total_x[TRC_TEST_SKIPPED] +=
            (*counters)[status_x][TRC_TEST_SKIPPED]
                       [TRC_DIFF_NO_MATCH_IGNORE].counter;

        total_y[TRC_TEST_SKIPPED] +=
            (*counters)[TRC_TEST_SKIPPED][status_x]
                       [TRC_DIFF_NO_MATCH_IGNORE].counter;
    }

    total[TRC_TEST_PASSED_UNE] +=
        (*counters)[TRC_TEST_PASSED][TRC_TEST_PASSED]
                   [TRC_DIFF_NO_MATCH].counter +
        (*counters)[TRC_TEST_PASSED_UNE][TRC_TEST_PASSED_UNE]
                   [TRC_DIFF_NO_MATCH].counter;
    total[TRC_TEST_FAILED_UNE] +=
        (*counters)[TRC_TEST_FAILED][TRC_TEST_FAILED]
                   [TRC_DIFF_NO_MATCH].counter +
        (*counters)[TRC_TEST_FAILED_UNE][TRC_TEST_FAILED_UNE]
                   [TRC_DIFF_NO_MATCH].counter;

#define WRITE_COUNTER(color_, counter_, prefix_, status_, component_)   \
    do                                                                  \
    {                                                                   \
        char *prefix = (prefix_);                                       \
        if (prefix != NULL)                                             \
        {                                                               \
            if ((counter_) > 0)                                         \
            {                                                           \
                fprintf(f, "%s", prefix);                               \
            }                                                           \
            else                                                        \
                break;                                                  \
        }                                                               \
                                                                        \
        empty = FALSE;                                                  \
                                                                        \
        fprintf(f, trc_diff_stats_stats_href_start);                    \
        fprintf(f, trc_diff_stats_stats_list,                           \
                tags_x->name, tags_y->name, "summary",                  \
                trc_diff_js_summary_status2str(status_),                \
                trc_diff_js_summary_component2str(component_));         \
        fprintf(f, trc_diff_stats_stats_href_end);                      \
                                                                        \
        fprintf(f, trc_diff_stats_brief_counter,                        \
                (color_), (counter_));                                  \
                                                                        \
        fprintf(f, trc_diff_stats_stats_href_close);                    \
    } while (0)

    fprintf(f, "<td style=\"text-align:right;\">");
    empty = TRUE;
    if (tags_x == tags_y)
    {
        WRITE_COUNTER("passed", total[TRC_TEST_PASSED], NULL,
                      TRC_DIFF_SUMMARY_PASSED, TRC_DIFF_SUMMARY_MATCH);
    }
    else
    {
        WRITE_COUNTER("passed_new", total_y[TRC_TEST_PASSED], " + ",
                      TRC_DIFF_SUMMARY_PASSED, TRC_DIFF_SUMMARY_NEW);
        WRITE_COUNTER("passed_old", total_x[TRC_TEST_PASSED], " - ",
                      TRC_DIFF_SUMMARY_PASSED, TRC_DIFF_SUMMARY_OLD);
        WRITE_COUNTER("passed_ignored_new",
                      (*counters)[TRC_TEST_SKIPPED][TRC_TEST_PASSED]
                                 [TRC_DIFF_NO_MATCH_IGNORE].counter, " + ",
                      TRC_DIFF_SUMMARY_PASSED,
                      TRC_DIFF_SUMMARY_SKIPPED_NEW);
        WRITE_COUNTER("passed_ignored_old",
                      (*counters)[TRC_TEST_PASSED][TRC_TEST_SKIPPED]
                                 [TRC_DIFF_NO_MATCH_IGNORE].counter, " - ",
                      TRC_DIFF_SUMMARY_PASSED,
                      TRC_DIFF_SUMMARY_SKIPPED_OLD);
    }
    if (empty)
        fprintf(f, "-");
    fprintf(f, "</td>");

    fprintf(f, "<td style=\"text-align:right;\">");
    empty = TRUE;
    if (tags_x == tags_y)
    {
        WRITE_COUNTER("passed_une", total[TRC_TEST_PASSED_UNE], NULL,
                      TRC_DIFF_SUMMARY_PASSED_UNE, TRC_DIFF_SUMMARY_MATCH);
    }
    else
    {
        WRITE_COUNTER("passed_une_new", total_y[TRC_TEST_PASSED_UNE], " + ",
                      TRC_DIFF_SUMMARY_PASSED_UNE, TRC_DIFF_SUMMARY_NEW);
        WRITE_COUNTER("passed_une_old", total_x[TRC_TEST_PASSED_UNE], " - ",
                      TRC_DIFF_SUMMARY_PASSED_UNE, TRC_DIFF_SUMMARY_OLD);
        WRITE_COUNTER("passed_une_ignored_new",
                      (*counters)[TRC_TEST_SKIPPED][TRC_TEST_PASSED_UNE]
                                 [TRC_DIFF_NO_MATCH_IGNORE].counter, " + ",
                      TRC_DIFF_SUMMARY_PASSED_UNE,
                      TRC_DIFF_SUMMARY_SKIPPED_NEW);
        WRITE_COUNTER("passed_une_ignored_old",
                      (*counters)[TRC_TEST_PASSED_UNE][TRC_TEST_SKIPPED]
                                 [TRC_DIFF_NO_MATCH_IGNORE].counter, " - ",
                      TRC_DIFF_SUMMARY_PASSED_UNE,
                      TRC_DIFF_SUMMARY_SKIPPED_OLD);
    }
    if (empty)
        fprintf(f, "-");
    fprintf(f, "</td>");

    fprintf(f, "<td style=\"text-align:right;\">");
    empty = TRUE;
    if (tags_x == tags_y)
    {
        WRITE_COUNTER("failed", total[TRC_TEST_FAILED], NULL,
                      TRC_DIFF_SUMMARY_FAILED, TRC_DIFF_SUMMARY_MATCH);
    }
    else
    {
        WRITE_COUNTER("failed_new", total_y[TRC_TEST_FAILED], " + ",
                      TRC_DIFF_SUMMARY_FAILED, TRC_DIFF_SUMMARY_NEW);
        WRITE_COUNTER("failed_old", total_x[TRC_TEST_FAILED], " - ",
                      TRC_DIFF_SUMMARY_FAILED, TRC_DIFF_SUMMARY_OLD);
        WRITE_COUNTER("failed_ignored_new",
                      (*counters)[TRC_TEST_SKIPPED][TRC_TEST_FAILED]
                                 [TRC_DIFF_NO_MATCH_IGNORE].counter, " + ",
                      TRC_DIFF_SUMMARY_FAILED,
                      TRC_DIFF_SUMMARY_SKIPPED_NEW);

        WRITE_COUNTER("failed_ignored_old",
                      (*counters)[TRC_TEST_FAILED][TRC_TEST_SKIPPED]
                                 [TRC_DIFF_NO_MATCH_IGNORE].counter, " - ",
                      TRC_DIFF_SUMMARY_FAILED,
                      TRC_DIFF_SUMMARY_SKIPPED_OLD);
    }
    if (empty)
        fprintf(f, "-");
    fprintf(f, "</td>");

    fprintf(f, "<td style=\"text-align:right;\">");
    empty = TRUE;
    if (tags_x == tags_y)
    {
        WRITE_COUNTER("failed_une", total[TRC_TEST_FAILED_UNE], NULL,
                      TRC_DIFF_SUMMARY_FAILED_UNE, TRC_DIFF_SUMMARY_MATCH);
    }
    else
    {
        WRITE_COUNTER("failed_une_new", total_y[TRC_TEST_FAILED_UNE], " + ",
                      TRC_DIFF_SUMMARY_FAILED_UNE, TRC_DIFF_SUMMARY_NEW);
        WRITE_COUNTER("failed_une_old", total_x[TRC_TEST_FAILED_UNE], " - ",
                      TRC_DIFF_SUMMARY_FAILED_UNE, TRC_DIFF_SUMMARY_OLD);
        WRITE_COUNTER("failed_une_ignored_new",
                      (*counters)[TRC_TEST_SKIPPED][TRC_TEST_FAILED_UNE]
                                 [TRC_DIFF_NO_MATCH_IGNORE].counter, " + ",
                      TRC_DIFF_SUMMARY_FAILED_UNE,
                      TRC_DIFF_SUMMARY_SKIPPED_NEW);
        WRITE_COUNTER("failed_une_ignored_old",
                      (*counters)[TRC_TEST_FAILED_UNE][TRC_TEST_SKIPPED]
                                 [TRC_DIFF_NO_MATCH_IGNORE].counter, " - ",
                      TRC_DIFF_SUMMARY_FAILED_UNE,
                      TRC_DIFF_SUMMARY_SKIPPED_OLD);
    }
    if (empty)
        fprintf(f, "-");
    fprintf(f, "</td>");

    fprintf(f, "<td style=\"text-align:right;\">");
    empty = TRUE;
    if (tags_x == tags_y)
    {
        WRITE_COUNTER("total",
                      total[TRC_TEST_PASSED] + total[TRC_TEST_PASSED_UNE] +
                      total[TRC_TEST_FAILED] + total[TRC_TEST_FAILED_UNE],
                      NULL, TRC_DIFF_SUMMARY_TOTAL, TRC_DIFF_SUMMARY_MATCH);

    }
    else
    {
        WRITE_COUNTER("total_new",
                      total_y[TRC_TEST_SKIPPED], " + ",
                      TRC_DIFF_SUMMARY_TOTAL, TRC_DIFF_SUMMARY_NEW);
        WRITE_COUNTER("total_old",
                      total_x[TRC_TEST_SKIPPED], " - ",
                      TRC_DIFF_SUMMARY_TOTAL, TRC_DIFF_SUMMARY_OLD);
    }
    if (empty)
        fprintf(f, "-");
    fprintf(f, "</td>");
#undef WRITE_COUNTER
}


void
trc_diff_stats_brief_report(FILE *f, const trc_diff_sets *sets,
                            trc_diff_stats *stats)
{
    trc_diff_set *ref_set = TAILQ_FIRST(sets);
    trc_diff_set *compare_set = NULL;
    trc_diff_set *prev_set = NULL;

    fprintf(f, trc_diff_stats_brief_table_start, ref_set->name);

    fprintf(f, trc_diff_stats_brief_table_row_start,
               ref_set->name);
    trc_diff_one_stats_brief_to_html(f, stats, ref_set, ref_set);
    fprintf(f, trc_diff_stats_brief_table_row_end);

    TAILQ_FOREACH(compare_set, sets, links)
    {
        if (compare_set == ref_set)
            continue;

        fprintf(f, trc_diff_stats_brief_table_row_href_start,
                ref_set->name, compare_set->name, compare_set->name);
        trc_diff_one_stats_brief_to_html(f, stats, ref_set, compare_set);
        fprintf(f, trc_diff_stats_brief_table_row_end);

        prev_set = compare_set;
    }

    fprintf(f, trc_diff_stats_brief_table_end);
}

void
trc_diff_stats_brief_report_incremental(FILE *f, const trc_diff_sets *sets,
                                        trc_diff_stats *stats)
{
    trc_diff_set *ref_set = NULL;
    trc_diff_set *compare_set = NULL;

    fprintf(f, trc_diff_stats_brief_table_start, ref_set->name);


    TAILQ_FOREACH(ref_set, sets, links)
    {
        fprintf(f, trc_diff_stats_brief_table_row_start,
                   ref_set->name);
        trc_diff_one_stats_brief_to_html(f, stats, ref_set, ref_set);
        fprintf(f, trc_diff_stats_brief_table_row_end);

        if ((compare_set = TAILQ_NEXT(ref_set, links)) == NULL)
            break;

        fprintf(f, trc_diff_stats_brief_table_row_start, "");
        trc_diff_one_stats_brief_to_html(f, stats, ref_set, compare_set);
        fprintf(f, trc_diff_stats_brief_table_row_end);
    }

    fprintf(f, trc_diff_stats_brief_table_end);
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
        rc = trc_exp_result_to_html(f, entry->results[set->id],
                                    flags, &set->tags);
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
        if ((entry->results[set->id] != NULL) &&
            (entry->results[set->id]->key != NULL))
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
        if ((entry->results[set->id] != NULL) &&
            (entry->results[set->id]->notes != NULL))
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
            char *hash = NULL;
            const trc_diff_set *set;

            TAILQ_FOREACH(set, sets, links)
            {
                hash = trc_diff_iter_hash_get(curr->ptr.iter, set->db_uid);
                if (hash != NULL)
                    break;
            }

            fprintf(f, trc_diff_table_iter_row_start,
                    (hash != NULL) ? hash : "", i);
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

    /* Fill tests lists in stats table */
    trc_diff_stats_js_table_report(f, &ctx->stats, &ctx->sets);
    //trc_diff_stats_table(f, &ctx->stats);

    /* Compared sets */
    trc_diff_tags_to_html(f, &ctx->sets);

    /* Output brief total statistics */
    trc_diff_stats_brief_report(f, &ctx->sets, &ctx->stats);

    /* Output brief total statistics */
    //trc_diff_stats_brief_to_html(f, &ctx->sets, &ctx->stats);

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
