/** @file
 * @brief Testing Results Comparator
 *
 * Generator of comparison report in HTML format.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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
#include <ctype.h>
#endif

#include "te_defs.h"
#include "te_alloc.h"
#include "logger_api.h"
#include "trc_db.h"
#include "trc_tags.h"
#include "trc_html.h"
#include "trc_report.h"
#include "re_subst.h"
#include "te_shell_cmd.h"

/** Define to 1 to use spoilers to show/hide test parameters */
#ifdef WITH_SPOILERS
#define TRC_USE_PARAMS_SPOILERS 1
#else
#define TRC_USE_PARAMS_SPOILERS 0
#endif

/** Define to 1 to enable statistics popups */
#ifdef WITH_POPUPS
#define TRC_USE_STATS_POPUP 1
#else
#define TRC_USE_STATS_POPUP 0
#endif

/** Define to 1 to enable hidden columns in statistics tables */
#ifdef WITH_HIDDEN_STATS
#define TRC_USE_HIDDEN_STATS 1
#else
#define TRC_USE_HIDDEN_STATS 0
#endif

/** Define to 1 to enable hidden columns in statistics tables */
#ifdef WITH_LOG_URLS
#define TRC_USE_LOG_URLS 1
#else
#define TRC_USE_LOG_URLS 0
#endif

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

#define PRINT_STR(str_) (((str_) != NULL) ? (str_) : "")

#define PRINT_STR1(expr_, str1_) \
    ((expr_) ? (str1_) : "")

#define PRINT_STR2(expr_, str1_, str2_) \
    PRINT_STR1(expr_, str1_), \
    PRINT_STR1(expr_, str2_)

#define PRINT_STR3(expr_, str1_, str2_, str3_) \
    PRINT_STR2(expr_, str1_, str2_), \
    PRINT_STR1(expr_, str3_)

#define PRINT_STR4(expr_, str1_, str2_, str3_, str4_) \
    PRINT_STR2(expr_, str1_, str2_), \
    PRINT_STR2(expr_, str3_, str4_)

#define PRINT_STR5(expr_, str1_, str2_, str3_, str4_, str5_) \
    PRINT_STR4(expr_, str1_, str2_, str3_, str4_), \
    PRINT_STR1(expr_, str5_)

#if TRC_USE_STATS_POPUP

/** Maximum number of nested test packages */
#define TRC_DB_NEST_LEVEL_MAX       8

#define TRC_STATS_SHOW_HREF_START \
    "        <a href=\"javascript:showStats('StatsTip','"

#define TRC_STATS_SHOW_HREF_CLOSE "')\">"

#define TRC_STATS_SHOW_HREF_CLOSE_PARAM_PREFIX "', '"

#define TRC_STATS_SHOW_HREF_END "</a>"
#endif

static const char * const trc_html_title_def =
    "Testing Results Comparison Report";

static const char * const trc_html_doc_start =
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n"
"<html>\n"
"<head>\n"
"  <meta http-equiv=\"content-type\" content=\"text/html; "
"charset=utf-8\">\n"
"  <title>%s</title>\n"
"  <style type=\"text/css\">\n"
"    .A {padding-left: 0.14in; padding-right: 0.14in}\n"
"    .B {padding-left: 0.24in; padding-right: 0.04in}\n"
"    .C {text-align: right; padding-left: 0.14in; padding-right: 0.14in}\n"
"    .D {text-align: right; padding-left: 0.24in; padding-right: 0.24in}\n"
"    .E {font-weight: bold; text-align: right; "
"padding-left: 0.14in; padding-right: 0.14in}\n"
"    .test_stats_name { font-weight: bold;}\n"
"    .test_stats_objective { }\n"
"    .test_stats_run_total { font-weight: bold; text-align: right; "
"padding-left: 0.14in; padding-right: 0.14in}\n"
"    .test_stats_passed_exp { text-align: right; padding-left: 0.14in; "
"padding-right: 0.14in}\n"
"    .test_stats_failed_exp { text-align: right; padding-left: 0.14in; "
"padding-right: 0.14in}\n"
"    .test_stats_passed_unexp { text-align: right; padding-left: 0.14in; "
"padding-right: 0.14in}\n"
"    .test_stats_failed_unexp { text-align: right; padding-left: 0.14in; "
"padding-right: 0.14in}\n"
"    .test_stats_aborted_new { text-align: right; padding-left: 0.14in; "
"padding-right: 0.14in}\n"
"    .test_stats_not_run_total { font-weight: bold; text-align: right; "
"padding-left: 0.14in; padding-right: 0.14in}\n"
"    .test_stats_skipped_exp { text-align: right; padding-left: 0.14in; "
"padding-right: 0.14in}\n"
"    .test_stats_skipped_unexp { text-align: right; padding-left: 0.14in; "
"padding-right: 0.14in}\n"
"    .test_stats_keys { }\n"
"    .test_stats_names { }\n"
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
#if TRC_USE_LOG_URLS
"    #TestLog {\n"
"        position: absolute;\n"
"        visibility: hidden;\n"
"        font-size: small;\n"
"        overflow: auto;\n"
"        width: 75%%;\n"
"        height: 80%%;\n"
"        left: 120px;\n"
"        top: 100px;\n"
"        background-color: #c0c0c0;\n"
"        border: 1px solid #000000;\n"
"        padding: 10px;\n"
"    }\n"
#endif
"    #close {\n"
"        float: right;\n"
"    }\n"
#endif
"  </style>\n"
#if TRC_USE_PARAMS_SPOILERS
"  <script type=\"text/javascript\">\n"
"    function showSpoiler(obj)\n"
"    {\n"
"      var button = obj.parentNode.getElementsByTagName(\"input\")[0];\n"
"      var inner = obj.parentNode.getElementsByTagName(\"div\")[0];\n"
"      if (inner.style.display == \"none\")\n"
"      {\n"
"        inner.style.display = \"\";\n"
"        button.value=\"Hide Parameters\";"
"      }\n"
"      else\n"
"      {\n"
"        inner.style.display = \"none\";\n"
"        button.value=\"Show Parameters\";"
"      }\n"
"    }\n"
"  </script>\n"
#endif
#if TRC_USE_STATS_POPUP
"  <script type=\"text/javascript\">\n"
#if TRC_USE_HIDDEN_STATS
"    function statsHideColumn(column)\n"
"    {\n"
"        var cells = document.getElementsByName('test_stats_' + column);\n"
"        for (var cell_no in cells)\n"
"        {\n"
"            cells[cell_no].style.visibility = 'none';\n"
"        }\n"
"    }\n"
#endif
"    function fillStats(name)\n"
"    {\n"
"        var package = test_stats[name];\n"
"        if (package == null)\n"
"        {\n"
"            alert('Failed to get element ' + name);\n"
"            return;\n"
"        }\n"
"\n"
"        var test_list = package['subtests'];\n"
"        if (test_list == null)\n"
"        {\n"
"            alert('Failed to get subtests of ' + name);\n"
"            return;\n"
"        }\n"
"\n"
"        var innerHTML = '';\n"
"        innerHTML += '<span id=\"close\"> ' +\n"
"                     '<a href=\"javascript:hidePopups()\"' +"
"\n"
"                     'style=\"text-decoration: none\">' +\n"
"                     '<strong>[x]</strong></a></span>';\n"
"        innerHTML += '<div align=\"center\"><b>Package: ' + "
"package['name'] + '</b></div>';\n"
"        innerHTML += '<div style=\"height:380px;overflow:auto;\">';\n"
"        innerHTML += '<table border=1 cellpadding=4 cellspacing=3 ' +\n"
"                     'width=100%% style=\"font-size:small;"
"text-align:right;\">';\n"
"        innerHTML += '<tr><td><b>Test</b></td>' +\n"
"                     '<td><b>Total</b></td>' +\n"
"                     '<td><b>Passed<br/>expect</b></td>' +\n"
"                     '<td><b>Failed<br/>expect</b></td>' +\n"
"                     '<td><b>Passed<br/>unexpect</b></td>' +\n"
"                     '<td><b>Failed<br/>unexpect</b></td>' +\n"
"                     '<td><b>Not run</b></td></tr>';\n"
"        var test_no;\n"
"        for (test_no in test_list)\n"
"        {\n"
"            var test = test_stats[test_list[test_no]];\n"
"\n"
"            innerHTML += '<tr>';\n"
"            innerHTML += '<td><a href=\"#' + test['path'] + '\">' + \n"
"                         test['name'] + '</a></td>' +\n"
"                         '<td>' + test['total'] + '</td>' +\n"
"                         '<td>' + test['passed_exp'] + '</td>' +\n"
"                         '<td>' + test['failed_exp'] + '</td>' +\n"
"                         '<td>' + test['passed_unexp'] + '</td>' +\n"
"                         '<td>' + test['failed_unexp'] + '</td>' +\n"
"                         '<td>' + test['not_run'] + '</td>';\n"
"            innerHTML += '</tr>';\n"
"        }\n"
"        innerHTML += '</table>';\n"
"        innerHTML += '</div>';\n"
"\n"
"        return innerHTML;\n"
"    }\n"
"\n"
"    function filterStats(name,field)\n"
"    {\n"
"        var package = test_stats[name];\n"
"        if (package === null)\n"
"        {\n"
"            alert('Failed to get element ' + name);\n"
"            return;\n"
"        }\n"
"\n"
"        var test_list = package['subtests'];\n"
"        if (test_list === null)\n"
"        {\n"
"            alert('Failed to get subtests of ' + name);\n"
"            return;\n"
"        }\n"
"\n"
"        var innerHTML = '';\n"
"        innerHTML += '<span id=\"close\"> ' +\n"
"                     '<a href=\"javascript:hidePopups()\"' +"
"\n"
"                     'style=\"text-decoration: none\">' +\n"
"                     '<strong>[x]</strong></a></span>';\n"
"        innerHTML += '<div align=\"center\"><b>Package: ' + "
"package['name'] + '</b></div>';\n"
"        innerHTML += '<div style=\"height:380px;overflow:auto;\">';\n"
"        innerHTML += '<table border=1 cellpadding=4 cellspacing=3 ' +\n"
"                     'width=100%% style=\"font-size:small;"
"text-align:right;\">';\n"
"        innerHTML += '<tr><td><b>Test</b></td>' +\n"
"                     '<td><b>' + field + '</b></td></tr>';\n"
"        var test_no;\n"
"        for (test_no in test_list)\n"
"        {\n"
"            var test = test_stats[test_list[test_no]];\n"
"            if (test[field] <= 0)\n"
"                continue;\n"
"\n"
"            innerHTML += '<tr>';\n"
"            innerHTML += '<td><a href=\"#' + test['path'] + '_' + \n"
"                         field + '\">' + test['name'] + '</a></td>' +\n"
"                         '<td>' + test[field] + '</td>';\n"
"            innerHTML += '</tr>';\n"
"        }\n"
"        innerHTML += '</table>';\n"
"        innerHTML += '</div>';\n"
"\n"
"        return innerHTML;\n"
"    }\n"
"\n"
"    function centerStats(obj,pos)\n"
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
"            var leftOffset = scrolled_x + "
"(center_x - obj.offsetWidth) - 20;\n"
"        else\n"
"            var leftOffset = scrolled_x + "
"(center_x - obj.offsetWidth) / 2;\n"
"\n"
"        var topOffset = scrolled_y + (center_y - obj.offsetHeight) / 2;\n"
"\n"
"        obj.style.top = topOffset + 'px';\n"
"        obj.style.left = leftOffset + 'px';\n"
"    }\n"
"\n"
"    function showStats(obj_name,name,field)\n"
"    {\n"
"        var obj = document.getElementById(obj_name);\n"
"        if (typeof obj == 'undefined')\n"
"            alert('Failed to get object ' + obj_name);\n"
"\n"
"        if (field)\n"
"        {\n"
"            obj.innerHTML = filterStats(name,field);\n"
"        }\n"
"        else\n"
"        {\n"
"            obj.innerHTML = fillStats(name);\n"
"        }\n"
"        //alert(obj.innerHTML);\n"
"        centerStats(obj);\n"
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
#if TRC_USE_LOG_URLS
"        hideObject('TestLog');\n"
#endif
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
"\n"
#if TRC_USE_LOG_URLS
"    function showLog(name, url, event)\n"
"    {\n"
"        hidePopups();\n"
"        //loadLogStart(url, 'TestLog');\n"
"        var obj = document.getElementById('TestLog');\n"
"        var innerHTML = '';\n"
"        innerHTML += '<span id=\"close\"> ' +\n"
"                     '<a href=\"javascript:hidePopups();\"' +\n"
"                     'style=\"text-decoration: none\">' +\n"
"                     '<strong>[x]</strong></a></span>';\n"
"        innerHTML += '<div align=\"center\"><b>Test: ' + name + "
"'</b></div>';\n"
"        innerHTML += '<iframe name=\"Log\" src=\"' + url + '\" ' +\n"
"                     'style=\"width:100%%; height:96%%\"/>';\n"
"        obj.innerHTML = innerHTML;\n"
"        centerStats(obj, 'right');\n"
"        obj.style.visibility = \"visible\";\n"
"        if (event)\n"
"            doNothing(event);\n"
"        return false;\n"
"    }\n"
#endif
"  </script>\n"
#endif
"</head>\n"
"<body lang=\"en-US\" dir=\"ltr\" "
#if TRC_USE_STATS_POPUP
"onClick=\"hidePopups()\" "
"onScroll=\"centerStats(document.getElementById('StatsTip'));"
#if TRC_USE_LOG_URLS
"centerStats(document.getElementById('TestLog'), 'right');"
#endif
"return false;\">\n"
"    <div id=\"StatsTip\" onClick=\"doNothing(event)\">\n"
"      <span id=\"close\"><a href=\"javascript:hidePopups()\" "
"style=\"text-decoration: none\"><strong>[x]</strong></a></span>\n"
"      <p>Tests Statistics</p><br/>\n"
"    </div>\n"
#if TRC_USE_LOG_URLS
"    <div id=\"TestLog\" onClick=\"doNothing(event);\" "
"onScroll=\"doNothing(event);\">\n"
"      <span id=\"close\"><a href=\"javascript:hidePopups()\" "
"style=\"text-decoration: none\"><strong>[x]</strong></a></span>\n"
"      <p>Tests Log</p><br/>\n"
"    </div>\n"
#endif
#else
">\n"
#endif
"\n";

static const char * const trc_html_doc_end =
"</body>\n"
"</html>\n";

static const char * const trc_stats_table =
"<table border=1 cellpadding=4 cellspacing=3 style=\"font-size:small;\">\n"
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
"</table>\n";

static const char * const trc_report_html_tests_stats_start =
"<table border=1 cellpadding=4 cellspacing=3 style=\"font-size:small;\">\n"
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
"      <td class=\"test_stats_total\" name=\"test_stats_total\">\n"
"        <b>Total</b>\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('total')\">x</a>\n"
#endif
"      </td>\n"
"      <td class=\"test_stats_passed_exp\" "
"name=\"test_stats_passed_exp\">\n"
"        Passed expect\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('passed_exp')\">x</a>\n"
#endif
"      </td>\n"
"      <td class=\"test_stats_failed_exp\" "
"name=\"test_stats_failed_exp\">\n"
"        Failed expect\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('failed_exp')\">x</a>\n"
#endif
"      </td>\n"
"      <td class=\"test_stats_passed_unexp\" "
"name=\"test_stats_passed_unexp\">\n"
"        Passed unexp\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('passed_unexp')\">x</a>\n"
#endif
"      </td>\n"
"      <td class=\"test_stats_failed_unexp\" "
"name=\"test_stats_failed_unexp\">\n"
"        Failed unexp\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('failed_unexp')\">x</a>\n"
#endif
"      </td>\n"
"      <td class=\"test_stats_aborted_new\" "
"name=\"test_stats_aborted_new\">\n"
"        Aborted, New\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('aborted_new')\">x</a>\n"
#endif
"      </td>\n"
"      <td class=\"test_stats_not_run\" "
"name=\"test_stats_not_run\">\n"
"        <b>Total</b>\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('not_run')\">x</a>\n"
#endif
"      </td>\n"
"      <td class=\"test_stats_skipped_exp\" "
"name=\"test_stats_skipped_exp\">\n"
"        Skipped expect\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('skipped_exp')\">x</a>\n"
#endif
"      </td>\n"
"      <td class=\"test_stats_skipped_unexp\" "
"name=\"test_stats_skipped_unexp\">\n"
"        Skipped unexp\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('skipped_unexp')\">x</a>\n"
#endif
"      </td>\n"
"    </tr>\n"
"  </thead>\n"
"  <tbody>\n";

static const char * const trc_tests_stats_end =
"  </tbody>\n"
"</table>\n";

#if TRC_USE_STATS_POPUP
#define TRC_TESTS_STATS_FIELD_FMT   "        %s%s%s%s%s%u%s\n"
#else
#define TRC_TESTS_STATS_FIELD_FMT   "        %u\n"
#endif

static const char * const trc_tests_stats_row =
"    <tr>\n"
"      <td class=\"test_stats_name\" name=\"test_stats_name\">\n"
"        %s<b><a %s=\"%s%s\">%s</a></b>\n"
#if TRC_USE_STATS_POPUP
"        %s%s%s\n"
#endif
"      </td>\n"
"      <td class=\"test_stats_objective\" "
"name=\"test_stats_objective\">\n"
"        %s%s%s%s%s\n"
"      </td>\n"
"      <td class=\"test_stats_total\" "
"name=\"test_stats_total\">\n"
       TRC_TESTS_STATS_FIELD_FMT
"      </td>\n"
"      <td class=\"test_stats_passed_exp\" "
"name=\"test_stats_passed_exp\">\n"
       TRC_TESTS_STATS_FIELD_FMT
"      </td>\n"
"      <td class=\"test_stats_failed_exp\" "
"name=\"test_stats_failed_exp\">\n"
       TRC_TESTS_STATS_FIELD_FMT
"      </td>\n"
"      <td class=\"test_stats_passed_unexp\" "
"name=\"test_stats_passed_unexp\">\n"
       TRC_TESTS_STATS_FIELD_FMT
"      </td>\n"
"      <td class=\"test_stats_failed_unexp\" "
"name=\"test_stats_failed_unexp\">\n"
       TRC_TESTS_STATS_FIELD_FMT
"      </td>\n"
"      <td class=\"test_stats_aborted_new\" "
"name=\"test_stats_aborted_new\">\n"
       TRC_TESTS_STATS_FIELD_FMT
"      </td>\n"
"      <td class=\"test_stats_not_run\" "
"name=\"test_stats_not_run\">\n"
       TRC_TESTS_STATS_FIELD_FMT
"      </td>\n"
"      <td class=\"test_stats_skip_exp\" "
"name=\"test_stats_skip_exp\">\n"
"        %u\n"
"      </td>\n"
"      <td class=\"test_stats_skip_unexp\" "
"name=\"test_stats_skip_unexp\">\n"
"        %u\n"
"      </td >\n"
"      <td class=\"test_stats_keys\" name=\"test_stats_keys\">%s</td>\n"
"      <td class=\"test_stats_notes\" name=\"test_stats_notes\">%s</td>\n"
"    </tr>\n";

static const char * const trc_report_html_test_exp_got_start =
"<table border=1 cellpadding=4 cellspacing=3 style=\"font-size:small;\">\n"
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
"        <b>Obtained</b>\n"
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

#if TRC_USE_LOG_URLS
static const char * const trc_test_log_url =
"<a href=\"node_%d.html\" "
#if TRC_USE_STATS_POPUP
"onClick=\"showLog('%s', 'node_%d.html', event); return false;\""
#endif
">[log]</a>";
#endif

static const char * const trc_test_exp_got_row_tin_ref =
"        <a name=\"tin_%d\"> </a>\n";

static const char * const trc_test_exp_got_row_start =
"    <tr>\n"
"      <td valign=top>\n"
"        %s%s<b><a %s%s%shref=\"#OBJECTIVE%s\">%s</a></b>\n"
#if TRC_USE_LOG_URLS
"        %s\n"
#endif
"      </td>\n"
"      <td valign=top>";

#if TRC_USE_PARAMS_SPOILERS
static const char * const trc_test_exp_got_row_params_start =
"<input type=\"button\" onclick=\"showSpoiler(this);\""
" value=\"Show Parameters\" />\n"
"          <div class=\"inner\" style=\"display:none;\">";

static const char * const trc_test_exp_got_row_params_end =
" </div>";
#endif

static const char * const trc_test_exp_got_row_result_anchor =
"  <a name=\"%s_%s\"> </a>";

static const char * const trc_test_exp_got_row_mid =
" </td>\n<td valign=top>";

static const char * const trc_test_exp_got_row_end =
"</td>\n"
"      <td valign=top>%s %s</td>\n"
"    </tr>\n";

#if TRC_USE_STATS_POPUP
static const char * const trc_report_javascript_table_start =
"  <script type=\"text/javascript\">\n"
"    var test_stats = new Array();\n"
"\n"
"    function updateStats(test_name, stats)\n"
"    {\n"
"        if (test_stats[test_name])\n"
"        {\n"
"            var test = test_stats[test_name];\n"
"            test['total'] += stats['total'];\n"
"            test['passed_exp'] += stats['passed_exp'];\n"
"            test['failed_exp'] += stats['failed_exp'];\n"
"            test['passed_unexp'] += stats['passed_unexp'];\n"
"            test['failed_unexp'] += stats['failed_unexp'];\n"
"            test['aborted_new'] += stats['aborted_new'];\n"
"            test['not_run'] += stats['not_run'];\n"
"        }\n"
"        else\n"
"        {\n"
"            test_stats[test_name] = stats;\n"
"        }\n"
"    }\n"
"\n";

static const char * const trc_report_javascript_table_end =
"  </script>\n";

static const char * const trc_report_javascript_table_row =
"    updateStats('%s', {name:'%s', path:'%s', total:%d, passed_exp:%d, "
"failed_exp:%d, passed_unexp:%d, failed_unexp:%d, aborted_new:%d, "
"not_run:%d, skipped_exp:%d, skipped_unexp:%d});\n";

static const char * const trc_report_javascript_table_subtests =
"    test_stats['%s'].subtests = [%s];\n";
#endif

#define TRC_REPORT_KEY_UNSPEC   "unspecified"

typedef TAILQ_HEAD(, trc_report_key_entry) trc_keys;

static int file_to_file(FILE *dst, FILE *src);

static te_bool
trc_report_test_iter_entry_output(
    const trc_test                   *test,
    const trc_report_test_iter_entry *iter,
    unsigned int                      flags);

static te_bool
trc_report_test_output(const trc_report_stats *stats,
                       unsigned int flags);

trc_report_key_entry *
trc_report_key_find(trc_keys *keys, const char *key_name)
{
    trc_report_key_entry *key;
    for (key = TAILQ_FIRST(keys);
         key != NULL;
         key = TAILQ_NEXT(key, links))
    {
        if (strcmp(key->name, key_name) == 0)
        {
            break;
        }
    }
    return key;
}

trc_report_key_entry *
trc_report_key_add(trc_keys *keys,
                   const char *key_name,
                   const trc_report_test_iter_entry *iter_entry,
                   const char *test_name, const char *test_path)
{
    trc_report_key_entry        *key = trc_report_key_find(keys, key_name);
    trc_report_key_test_entry   *key_test = NULL;
    trc_report_key_iter_entry   *key_iter = NULL;

    /* Create new key entry, if key does not exist */
    if (key == NULL)
    {
        if ((key = calloc(1, sizeof(trc_report_key_entry))) == NULL)
            return NULL;

        key->name = strdup(key_name);
        TAILQ_INIT(&key->tests);
        TAILQ_INSERT_TAIL(keys, key, links);
    }

    if ((key_iter = calloc(1, sizeof(trc_report_key_iter_entry))) == NULL)
        return NULL;
    key_iter->iter = iter_entry;

    TAILQ_FOREACH(key_test, &key->tests, links)
    {
        if (strcmp(test_path, key_test->path) == 0)
        {
            /* Do not duplicate iterations, exit */
            TAILQ_INSERT_TAIL(&key_test->iters, key_iter, links);
            key_test->count++;
            key->count++;
            return key;
        }
    }

    /* Create new key iteration entry, if not found */
    if ((key_test = calloc(1, sizeof(trc_report_key_test_entry))) == NULL)
    {
        ERROR("Failed to allocate structure to store iteration key");
        return NULL;
    }
    key_test->name = strdup(test_name);
    key_test->path = strdup(test_path);

    TAILQ_INIT(&key_test->iters);
    TAILQ_INSERT_TAIL(&key_test->iters, key_iter, links);
    key_test->count++;

    TAILQ_INSERT_TAIL(&key->tests, key_test, links);
    key->count++;

    return key;
}

int
trc_report_keys_add(trc_keys *keys,
                    const char *key_names,
                    const trc_report_test_iter_entry *iter_entry,
                    const char *test_name, const char *test_path)
{
    int     count = 0;
    char   *p = NULL;

    if ((key_names == NULL) || (*key_names == '\0'))
    {
        trc_report_key_add(keys, TRC_REPORT_KEY_UNSPEC, iter_entry,
                           test_name, test_path);
        return ++count;
    }

    /* Iterate through keys list with ',' delimiter */
    while ((key_names != NULL) && (*key_names != '\0'))
    {
        if ((p = strchr(key_names, ',')) != NULL)
        {
            char *tmp_key_name = strndup(key_names, p - key_names);

            if (tmp_key_name == NULL)
                break;

            trc_report_key_add(keys, tmp_key_name, iter_entry,
                               test_name, test_path);
            free(tmp_key_name);
            key_names = p + 1;
            while (*key_names == ' ')
                key_names++;
        }
        else
        {
            trc_report_key_add(keys, key_names, iter_entry,
                               test_name, test_path);
            key_names = NULL;
        }
        count++;
    }

    return count;
}

char *
trc_report_key_test_path(const char *test_path,
                         const char *key_names)
{
    char *path = te_sprintf("%s-%s", test_path, (key_names != NULL) ?
                            key_names : TRC_REPORT_KEY_UNSPEC);
    char *p;

    if (path == NULL)
        return NULL;

    for (p = path; *p != '\0'; p++)
    {
        if (*p == ' ')
            *p = '_';
        else if (*p == ',')
            *p = '-';
    }

    return path;
}

void
trc_keys_init(trc_keys *keys)
{
    TAILQ_INIT(keys);
}

void
trc_keys_free(trc_keys *keys)
{
#if 1
    trc_report_key_entry *key;
    trc_report_key_test_entry *key_test;
    trc_report_key_iter_entry *key_iter;

    while ((key = TAILQ_FIRST(keys)) != NULL)
    {
        while ((key_test = TAILQ_FIRST(&key->tests)) != NULL)
        {
            while ((key_iter = TAILQ_FIRST(&key_test->iters)) != NULL)
            {
                TAILQ_REMOVE(&key_test->iters, key_iter, links);
                free(key_iter);
            }
            TAILQ_REMOVE(&key->tests, key_test, links);
            free(key_test);
        }
        TAILQ_REMOVE(keys, key, links);
        free(key);
    }
#endif
}

#define TRC_REPORT_KEYS_FILENAME_SUFFIX ".keys"
#define TRC_REPORT_KEYS_FILENAME_EXTENSION ".html"

/**
 * Generate test iteration expected/obtained results to HTML report.
 *
 * @param ctx           TRC report context
 * @param walker        TRC database walker position
 * @param flags         Current output flags
 * @param test_path     Full test path for anchor
 *
 * @return Status code.
 */
char *trc_keys_filename(const char *filename)
{
    char *keys_filename;
    char *suffix = strstr(filename, TRC_REPORT_KEYS_FILENAME_EXTENSION);
    char *prefix = strndup(filename, (suffix == NULL) ? strlen(filename) :
                           (unsigned)(suffix - filename));
    if (prefix == NULL)
        return NULL;

    keys_filename = te_sprintf("%s%s%s", prefix,
                               TRC_REPORT_KEYS_FILENAME_SUFFIX,
                               TRC_REPORT_KEYS_FILENAME_EXTENSION);

    free(prefix);

    return keys_filename;
}

/**
 * Output test iteration expected/obtained results to HTML report.
 *
 * @param ctx           TRC report context
 * @param walker        TRC database walker position
 * @param flags         Current output flags
 * @param test_path     Full test path for anchor
 *
 * @return              Status code.
 */
te_errno
trc_keys_iter_add(trc_keys *keys,
                  trc_report_ctx      *ctx,
                  te_trc_db_walker    *walker,
                  unsigned int         flags,
                  const char          *test_path)
{
    const trc_test                   *test;
    const trc_test_iter              *iter;
    trc_report_test_iter_data        *iter_data;
    const trc_report_test_iter_entry *iter_entry;
    te_errno                          rc = 0;

    assert(test_path != NULL);

    iter = trc_db_walker_get_iter(walker);
    test = iter->parent;
    iter_data = trc_db_walker_get_user_data(walker, ctx->db_uid);
    iter_entry = (iter_data == NULL) ? NULL : TAILQ_FIRST(&iter_data->runs);

    do {
        if ((iter_data == NULL) || (iter_entry == NULL))
            return 0;

        if (trc_report_test_iter_entry_output(test, iter_entry, flags))
        {
            if ((test->type == TRC_TEST_SCRIPT) &&
                (iter_data->exp_result != NULL))
            {
                char *key_test_path =
                    trc_report_key_test_path(test_path,
                        iter_data->exp_result->key);

                if (~flags & TRC_REPORT_KEYS_SANITY)
                {
                    if ((iter_entry->result.status == TE_TEST_FAILED) ||
                         ((iter_entry->result.status == TE_TEST_PASSED) &&
                          (TAILQ_FIRST(&iter_entry->result.verdicts) !=
                          NULL)))
                    {
                        /*
                         * Iterations does not have unique names and
                         * paths yet, use test name and path instead of
                         */
                        trc_report_keys_add(keys,
                                            iter_data->exp_result->key,
                                            iter_entry, test->name,
                                            key_test_path);
                    }
                }
                else
                {
                    if ((iter_data->exp_result->key != NULL) &&
                        ((!iter_entry->is_exp) ||
                         (TAILQ_FIRST(&iter_entry->result.verdicts) ==
                          NULL)))
                    {
                        trc_report_keys_add(keys,
                                            iter_data->exp_result->key,
                                            iter_entry, test->name,
                                            key_test_path);
                    }
                }

                free(key_test_path);
            }
        }
    } while (iter_entry != NULL &&
             (iter_entry = TAILQ_NEXT(iter_entry, links)) != NULL);

    return rc;
}

/**
 * Aggregate all test keys into one table.
 *
 * @param ctx   TRC report context
 * @param flags Output flags
 *
 * @return      Status code.
 */
static te_errno
trc_report_keys_collect(trc_keys *keys,
                        trc_report_ctx *ctx,
                        unsigned int flags)
{
    te_errno                rc = 0;
    te_trc_db_walker       *walker;
    trc_db_walker_motion    mv;
    unsigned int            level = 0;
    const char             *last_test_name = NULL;
    te_string               test_path = TE_STRING_INIT;

    walker = trc_db_new_walker(ctx->db);
    if (walker == NULL)
        return TE_ENOMEM;

    while ((rc == 0) &&
           ((mv = trc_db_walker_move(walker)) != TRC_DB_WALKER_ROOT))
    {
        const trc_report_test_data *test_data =
            trc_db_walker_get_user_data(walker, ctx->db_uid);
        const trc_report_stats *stats =
            test_data ? &test_data->stats : NULL;
        const trc_test *test = trc_db_walker_get_test(walker);

        switch (mv)
        {
            case TRC_DB_WALKER_SON:
                level++;
                /*@fallthrough@*/

            case TRC_DB_WALKER_BROTHER:
                if ((level & 1) == 1)
                {
                    /* Test entry */
                    if (mv != TRC_DB_WALKER_SON)
                    {
                        te_string_cut(&test_path,
                                      strlen(last_test_name) + 1);
                    }

                    last_test_name = trc_db_walker_get_test(walker)->name;

                    rc = te_string_append(&test_path, "-%s",
                                          last_test_name);
                    if (rc != 0)
                        break;

                    if (!trc_report_test_output(stats, flags))
                        break;
                }
                else
                {
                    if (test->type == TRC_TEST_SCRIPT)
                        trc_keys_iter_add(keys, ctx, walker, flags,
                                          test_path.ptr);
                }
                break;

            case TRC_DB_WALKER_FATHER:
                level--;
                if ((level & 1) == 0)
                {
                    /* Back from the test to parent iteration */
                    te_string_cut(&test_path,
                                  strlen(last_test_name) + 1);

                    last_test_name = trc_db_walker_get_test(walker)->name;
                }
                break;

            default:
                assert(FALSE);
                break;
        }
    }

    trc_db_free_walker(walker);
    te_string_free(&test_path);
    return rc;
}

#define TRC_REPORT_OL_KEY_PREFIX        "OL "
#define TRC_KEYTOOL_CMD_BUF_SIZE        1024

/**
 * Output key entry to HTML report.
 *
 * @param f             File stream to write to
 * @param keytool_fn    Key table generation tool
 * @param keys          List of keys to report
 * @param keys_only     Suppress URLs to test iterations
 *
 * @return              Status code.
 */
te_errno
trc_report_keys_to_html(FILE           *f,
                        trc_report_ctx *ctx,
                        char           *keytool_fn,
                        trc_keys       *keys,
                        te_bool         keys_only,
                        char           *title)
{
    int                   fd_in       = -1;
    int                   fd_out      = -1;
    FILE                 *f_in        = NULL;
    FILE                 *f_out       = NULL;
    pid_t                 pid;
    trc_report_key_entry *key;
    tqe_string           *tag;
    char                  cmd_buf[TRC_KEYTOOL_CMD_BUF_SIZE];
    int                   cmd_buf_len = 0;


    cmd_buf_len = snprintf(cmd_buf, TRC_KEYTOOL_CMD_BUF_SIZE,
                           "%s", keytool_fn);
    if (title != NULL)
    {
        cmd_buf_len += snprintf(cmd_buf + cmd_buf_len,
                                TRC_KEYTOOL_CMD_BUF_SIZE - cmd_buf_len,
                                " --title=\"%s\"", title);
    }

    TAILQ_FOREACH(tag, &ctx->tags, links)
    {
        char *keyw =
            trc_re_key_substs_buf(TRC_RE_KEY_TAGS, tag->v);

        if (keyw == NULL)
            return ENOMEM;

        cmd_buf_len += snprintf(cmd_buf + cmd_buf_len,
                                TRC_KEYTOOL_CMD_BUF_SIZE - cmd_buf_len,
                                " --keyword=%s", keyw);
        free(keyw);
    }

    VERB("Run: %s", cmd_buf);
    if ((pid = te_shell_cmd_inline(cmd_buf, -1,
                                   &fd_in, &fd_out, NULL)) < 0)
    {
        return pid;
    }

    f_in = fdopen(fd_in, "w");
    f_out = fdopen(fd_out, "r");

    for (key = TAILQ_FIRST(keys);
         key != NULL;
         key = TAILQ_NEXT(key, links))
    {
         trc_report_key_test_entry *key_test = NULL;

         char *script =
            trc_re_key_substs_buf(TRC_RE_KEY_SCRIPT, key->name);
         fprintf(f_in, "%s:", script);
         free(script);

         TAILQ_FOREACH(key_test, &key->tests, links)
         {
             trc_report_key_iter_entry *key_iter = NULL;

             if (key_test != TAILQ_FIRST(&key->tests))
             {
                 fprintf(f_in, ",");
             }

             fprintf(f_in, "%s", key_test->name);
             if (!keys_only)
             {
                 fprintf(f_in, "#%s", key_test->path);
                 TAILQ_FOREACH(key_iter, &key_test->iters, links)
                 {
                     fprintf(f_in, "|%d", key_iter->iter->tin);
                 }
             }
             else
             {
                 fprintf(f_in, "&nbsp;[%d]", key_test->count);
             }
         }
         fprintf(f_in, "\n");
    }

    fclose(f_in);
    close(fd_in);

    file_to_file(f, f_out);

    fclose(f_out);
    close(fd_out);

    return 0;
}


/**
 * Output grand total statistics to HTML.
 *
 * @param stats Statistics to output
 *
 * @return      Status code.
 */
static int
trc_report_stats_to_html(FILE *f, const trc_report_stats *stats)
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
 * Should test iteration instance be output in accordance with
 * expected/obtained result and current output flags?
 *
 * @param test  Test
 * @param iter  Test iteration entry in TRC report
 * @param flag  Output flags
 */
static te_bool
trc_report_test_iter_entry_output(
    const trc_test                   *test,
    const trc_report_test_iter_entry *iter,
    unsigned int                      flags)
{
    te_test_status status =
        (iter == NULL) ? TE_TEST_UNSPEC : iter->result.status;

    return (/* NO_SCRIPTS is clear or it is NOT a script */
            (~flags & TRC_REPORT_NO_SCRIPTS) ||
             (test->type != TRC_TEST_SCRIPT)) &&
            (/* NO_UNSPEC is clear or obtained result is not UNSPEC */
             (~flags & TRC_REPORT_NO_UNSPEC) ||
             (status != TE_TEST_UNSPEC)) &&
            (/* NO_SKIPPED is clear or obtained result is not SKIPPED */
             (~flags & TRC_REPORT_NO_SKIPPED) ||
             (status != TE_TEST_SKIPPED)) &&
            (/*
              * NO_EXP_PASSED is clear or
              * obtained result is not PASSED as expected
              */
             (~flags & TRC_REPORT_NO_EXP_PASSED) ||
             (status != TE_TEST_PASSED) || (!iter->is_exp)) &&
            (/* 
              * NO_EXPECTED is clear or obtained result is equal
              * to expected
              */
             (~flags & TRC_REPORT_NO_EXPECTED) || (!iter->is_exp));
}

/**
 * Modify keys string to alter substitution rule to generate
 * reference to link
 *
 * @param keys  Keys string to modify
 *
 * @return      Modified keys string.
 */
static inline char *
trc_link_keys(const char *keys)
{
    char *p;
    char *link_keys = strdup((keys) ? keys : TRC_REPORT_KEY_UNSPEC);

    if (link_keys == NULL)
        return NULL;

    for (p = link_keys; *p != '\0'; p++)
        *p = tolower(*p);

    return link_keys;
}

#if TRC_USE_STATS_POPUP
/**
 * Convert iteration result to string.
 *
 * @param iter  Iteration entry with test result
 *
 * @return      Status code.
 */
static inline char *
trc_report_result_to_string(const trc_report_test_iter_entry *iter)
{
    if (iter == NULL)
        return NULL;

    switch (iter->result.status)
    {
        case TE_TEST_PASSED:
            return (iter->is_exp) ? "passed_exp" : "passed_unexp";

        case TE_TEST_FAILED:
            return (iter->is_exp) ? "failed_exp" : "failed_unexp";

        case TE_TEST_INCOMPLETE:
        case TE_TEST_UNSPEC:
            return "aborted_new";

        default:
            return "not_run";
    }

    return NULL;
}

/**
 * Output anchor to test iteration result to HTML report.
 *
 * @param f             File stream to write to
 * @param test_path     Full test path
 * @param iter          Iteration entry with test result
 *
 * @return              Status code.
 */
static inline int
trc_report_result_anchor(FILE *f, const char *test_path,
                         const trc_report_test_iter_entry *iter)
{
    fprintf(f, trc_test_exp_got_row_result_anchor, test_path,
            PRINT_STR(trc_report_result_to_string(iter)));

    return 0;
}
#endif

/**
 * Output test iteration expected/obtained results to HTML report.
 *
 * @param f             File stream to write to
 * @param ctx           TRC report context
 * @param walker        TRC database walker position
 * @param flags         Current output flags
 * @param anchor        Should HTML anchor be created?
 * @param test_path     Full test path for anchor
 * @param level_str     String to represent depth of the test in the
 *                      suite
 *
 * @return              Status code.
 */
te_errno
trc_report_exp_got_to_html(FILE             *f,
                           trc_report_ctx   *ctx,
                           te_trc_db_walker *walker,
                           unsigned int      flags,
                           te_bool          *anchor,
                           const char       *test_path,
                           const char       *level_str)
{
    const trc_test                   *test;
    const trc_test_iter              *iter;
    trc_report_test_iter_data        *iter_data;
    const trc_report_test_iter_entry *iter_entry;
    te_errno                          rc = 0;
    char                              tin_ref[128];

    assert(anchor != NULL);
    assert(test_path != NULL);

    iter = trc_db_walker_get_iter(walker);
    test = iter->parent;
    iter_data = trc_db_walker_get_user_data(walker, ctx->db_uid);
    iter_entry = (iter_data == NULL) ? NULL : TAILQ_FIRST(&iter_data->runs);

    do {
        if (trc_report_test_iter_entry_output(test, iter_entry, flags))
        {
#if TRC_USE_LOG_URLS
            char *test_url = NULL;
            if ((iter_entry != NULL) && (iter_entry->tin >= 0))
            {
                test_url = te_sprintf(trc_test_log_url,
#if TRC_USE_STATS_POPUP
                                      iter_entry->tin,
                                      test->name, iter_entry->tin);
#else
                                      iter_entry->tin);
#endif
            }
#endif

            if (iter_data == NULL)
            {
                iter_data = TE_ALLOC(sizeof(*iter_data));
                if (iter_data == NULL)
                {
                    rc = TE_ENOMEM;
                    break;
                }
                TAILQ_INIT(&iter_data->runs);
                iter_data->exp_result =
                    trc_db_walker_get_exp_result(walker, &ctx->tags);

                rc = trc_db_walker_set_user_data(walker, ctx->db_uid,
                                                 iter_data);
                if (rc != 0)
                {
                    free(iter_data);
                    break;
                }
            }
            assert(iter_data != NULL);

            tin_ref[0] = '\0';
            if ((~flags & TRC_REPORT_NO_KEYS)   &&
                (test->type == TRC_TEST_SCRIPT) &&
                (iter_entry != NULL) &&
                (iter_entry->tin >= 0))
            {
                sprintf(tin_ref, trc_test_exp_got_row_tin_ref,
                        iter_entry->tin);
            }

            fprintf(f, trc_test_exp_got_row_start, tin_ref,
                    PRINT_STR(level_str),
                    PRINT_STR3(anchor, "name=\"", test_path, "\" "),
#if TRC_USE_LOG_URLS
                    test_path, test->name, PRINT_STR(test_url));
#else
                    test_path, test->name);
#endif
            *anchor = FALSE;

#if TRC_USE_LOG_URLS
            if (test_url != NULL)
            {
                free(test_url);
            }
#endif

#if TRC_USE_PARAMS_SPOILERS
            if (!TAILQ_EMPTY(&iter->args.head))
                WRITE_STR(trc_test_exp_got_row_params_start);
#endif

            rc = trc_test_iter_args_to_html(f, &iter->args, 0);
            if (rc != 0)
                break;

#if TRC_USE_PARAMS_SPOILERS
            if (!TAILQ_EMPTY(&iter->args.head))
                WRITE_STR(trc_test_exp_got_row_params_end);
#endif

            WRITE_STR(trc_test_exp_got_row_mid);

            rc = trc_exp_result_to_html(f, iter_data->exp_result, 0);
            if (rc != 0)
                break;

            WRITE_STR(trc_test_exp_got_row_mid);

#if TRC_USE_STATS_POPUP
            rc = trc_report_result_anchor(f, test_path, iter_entry);
            if (rc != 0)
                break;
#endif

            rc = te_test_result_to_html(f, (iter_entry == NULL) ? NULL :
                                               &iter_entry->result);
            if (rc != 0)
                break;

            WRITE_STR(trc_test_exp_got_row_mid);

            if ((test->type == TRC_TEST_SCRIPT) &&
                (iter_data->exp_result != NULL))
            {
                if (iter_data->exp_result->key != NULL)
                {
                    char *iter_key =
                        trc_re_key_substs_buf(TRC_RE_KEY_URL,
                                              iter_data->exp_result->key);
                    if (iter_key != NULL)
                        fprintf(f, "%s", iter_key);
                    free(iter_key);
                }

                if ((iter_entry != NULL) &&
                    ((iter_entry->result.status == TE_TEST_FAILED) ||
                     ((iter_entry->result.status == TE_TEST_PASSED) &&
                      (TAILQ_FIRST(&iter_entry->result.verdicts) !=
                      NULL))))
                {
                    char *key_link = NULL;
                    char *key_test_path =
                        trc_report_key_test_path(test_path,
                            iter_data->exp_result->key);

                    fprintf(f, "<a name=\"%s\"/>", key_test_path);
                    /*
                     * Iterations does not have unique names and paths yet,
                     * use test name and path instead of
                     */
                    free(key_test_path);

                    /* Add also link to keys table */
                    key_link =
                        trc_re_key_substs_buf(TRC_RE_KEY_TABLE_HREF,
                                              (iter_data->exp_result->key !=
                                               NULL) ?
                                              iter_data->exp_result->key :
                                              TRC_REPORT_KEY_UNSPEC);
                    if (key_link != NULL)
                        fprintf(f, "%s", key_link);
                    free(key_link);
                }
            }

            fprintf(f, trc_test_exp_got_row_end,
                    (iter_data->exp_result == NULL) ? "" :
                        PRINT_STR(iter_data->exp_result->notes),
                    PRINT_STR(iter->notes));
        }
    } while (iter_entry != NULL &&
             (iter_entry = TAILQ_NEXT(iter_entry, links)) != NULL);

cleanup:
    return rc;
}


/**
 * Should test entry be output in accordance with expected/obtained
 * result statistics and current output flags?
 *
 * @param stats Statistics
 * @param flag  Output flags
 */
static te_bool
trc_report_test_output(const trc_report_stats *stats, unsigned int flags)
{
    return
        (/* It is a script. Do output, if ... */
         /* NO_SCRIPTS is clear */
         (~flags & TRC_REPORT_NO_SCRIPTS) &&
         /* NO_UNSPEC is clear or tests with specified result */
         ((~flags & TRC_REPORT_NO_UNSPEC) ||
          (TRC_STATS_SPEC(stats) != 0)) &&
         /* NO_SKIPPED is clear or tests are run or unspec */
         ((~flags & TRC_REPORT_NO_SKIPPED) ||
          (TRC_STATS_RUN(stats) != 0) ||
          ((~flags & TRC_REPORT_NO_STATS_NOT_RUN) &&
           (TRC_STATS_NOT_RUN(stats) != 
            (stats->skip_exp + stats->skip_une)))) &&
         /* NO_EXP_PASSED or not all tests are passed as expected */
         ((~flags & TRC_REPORT_NO_EXP_PASSED) ||
          (TRC_STATS_RUN(stats) != stats->pass_exp) ||
          ((TRC_STATS_NOT_RUN(stats) != 0) &&
           ((TRC_STATS_NOT_RUN(stats) != stats->not_run) ||
            (~flags & TRC_REPORT_NO_STATS_NOT_RUN)))) &&
         /* NO_EXPECTED or unexpected results are obtained */
         ((~flags & TRC_REPORT_NO_EXPECTED) ||
          ((TRC_STATS_UNEXP(stats) != 0) &&
           ((TRC_STATS_UNEXP(stats) != stats->not_run) ||
            (~flags & TRC_REPORT_NO_STATS_NOT_RUN)))));
}


/**
 * Output test entry to HTML report.
 *
 * @param f             File stream to write to
 * @param ctx           TRC report context
 * @param walker        TRC database walker position
 * @param flags         Current output flags
 * @param test_path     Test path
 * @param level_str     String to represent depth of the test in the
 *                      suite
 *
 * @return              Status code.
 */
static te_errno
trc_report_test_stats_to_html(FILE             *f,
                              trc_report_ctx   *ctx,
                              te_trc_db_walker *walker,
                              unsigned int      flags,
                              const char       *test_path,
                              const char       *level_str)
{
    const trc_test             *test;
    const trc_report_test_data *test_data;
    const trc_report_stats     *stats;

    test = trc_db_walker_get_test(walker);
    test_data = trc_db_walker_get_user_data(walker, ctx->db_uid);
    assert(test_data != NULL);
    stats = &test_data->stats;

    if (((test->type == TRC_TEST_PACKAGE) &&
         (flags & TRC_REPORT_NO_SCRIPTS)) ||
        trc_report_test_output(stats, flags))
    {
        te_bool     name_link;
        const char *keys = NULL;

        /* test_iters_check_output_and_get_keys(test, flags); */

        name_link = ((flags & TRC_REPORT_NO_SCRIPTS) ||
                     ((~flags & TRC_REPORT_NO_SCRIPTS) &&
                     (test->type == TRC_TEST_SCRIPT)));

#if TRC_USE_STATS_POPUP
#define TRC_STATS_FIELD_POPUP(field_,value_,expr_) \
    PRINT_STR5((test->type == TRC_TEST_PACKAGE) && \
               ((value_) > 0) && (expr_), \
               TRC_STATS_SHOW_HREF_START, \
               test_path, \
               TRC_STATS_SHOW_HREF_CLOSE_PARAM_PREFIX, \
               (field_), \
               TRC_STATS_SHOW_HREF_CLOSE), \
    (value_), \
    PRINT_STR1((test->type == TRC_TEST_PACKAGE) && \
               ((value_) > 0) && (expr_), \
               TRC_STATS_SHOW_HREF_END)
#else
#define TRC_STATS_FIELD_POPUP(field_,value_,expr_) (value_)
#endif

        fprintf(f, trc_tests_stats_row,
                PRINT_STR(level_str),
                name_link ? "href" : "name",
                name_link ? "#" : "",
                test_path,
                test->name,

#if TRC_USE_STATS_POPUP
                PRINT_STR3((test->type == TRC_TEST_PACKAGE) &&
                           (flags & TRC_REPORT_NO_SCRIPTS) &&
                           (TRC_STATS_RUN(stats) > 0),
                           TRC_STATS_SHOW_HREF_START,
                           test_path,
                           TRC_STATS_SHOW_HREF_CLOSE
                           "[...]"
                           TRC_STATS_SHOW_HREF_END),
#endif
                test_path != NULL ? "<a name=\"OBJECTIVE" : "",
                PRINT_STR(test_path),
                test_path != NULL ? "\">": "",
                PRINT_STR(test->objective),
                test_path != NULL ? "</a>": "",
                TRC_STATS_FIELD_POPUP("total", TRC_STATS_RUN(stats), TRUE),
                TRC_STATS_FIELD_POPUP("passed_exp", stats->pass_exp, TRUE),
                TRC_STATS_FIELD_POPUP("failed_exp", stats->fail_exp, TRUE),
                TRC_STATS_FIELD_POPUP("passed_unexp",
                                      stats->pass_une, TRUE),
                TRC_STATS_FIELD_POPUP("failed_unexp",
                                      stats->fail_une, TRUE),
                TRC_STATS_FIELD_POPUP("aborted_new",
                                      stats->aborted +
                                      stats->new_run, TRUE),
                TRC_STATS_FIELD_POPUP("not_run", TRC_STATS_NOT_RUN(stats),
                                      TRC_STATS_RUN(stats) > 0),
                stats->skip_exp, stats->skip_une,
                PRINT_STR(keys), PRINT_STR(test->notes));
#undef TRC_STATS_FIELD_POPUP
    }

    return 0;
}

#if TRC_USE_STATS_POPUP
/**
 * Generate javascript tests table in HTML report.
 *
 * @param f             File stream to write to
 * @param test_name     Test name
 * @param test_path     Full test path
 * @param stats         Test stats
 *
 * @return              Status code.
 */
te_errno
trc_report_javascript_table_entry(FILE                   *f,
                                  const char             *test_name,
                                  const char             *test_path,
                                  const trc_report_stats *stats)
{
    te_errno rc = 0;

    if ((test_name == NULL) || (test_path == NULL) || (stats == NULL))
        return 0;

    fprintf(f, trc_report_javascript_table_row,
            test_path, test_name, test_path,
            TRC_STATS_RUN(stats),
            stats->pass_exp, stats->fail_exp,
            stats->pass_une, stats->fail_une,
            stats->aborted + stats->new_run,
            TRC_STATS_NOT_RUN(stats),
            stats->skip_exp, stats->skip_une);

    return rc;
}

/**
 * Fill javascript subtests.
 *
 * @param f             File stream to write to
 * @param test_name     Test name
 * @param test_path     Full test path
 * @param stats         Test stats
 *
 * @return              Status code.
 */
te_errno
trc_report_javascript_package_subtests(FILE       *f,
                                       const char *test_name,
                                       const char *subtests)
{
    te_errno rc = 0;

    if ((test_name == NULL) || (subtests == NULL))
        return 0;

    fprintf(f, trc_report_javascript_table_subtests,
            test_name, PRINT_STR(subtests));

    return rc;
}

/**
 * Generate javascript test statistics tree in HTML report.
 *
 * @param f     File stream to write to
 * @param ctx   TRC report context
 * @param flags Output flags
 *
 * @return      Status code.
 */
static te_errno
trc_report_javascript_table(FILE         *f, trc_report_ctx *ctx, 
                            unsigned int  flags)
{
    te_errno              rc = 0;
    te_trc_db_walker     *walker;
    trc_db_walker_motion  mv;
    unsigned int          level = 0;
    const char           *last_test_name = NULL;
    te_string             test_path = TE_STRING_INIT;
    te_string             init_string = TE_STRING_INIT;
    te_string             subtests[TRC_DB_NEST_LEVEL_MAX];

    walker = trc_db_new_walker(ctx->db);
    if (walker == NULL)
        return TE_ENOMEM;

    WRITE_STR(trc_report_javascript_table_start);

    while ((rc == 0) &&
           ((mv = trc_db_walker_move(walker)) != TRC_DB_WALKER_ROOT))
    {
        const trc_test *test = trc_db_walker_get_test(walker);
        const trc_report_test_data *test_data =
            trc_db_walker_get_user_data(walker, ctx->db_uid);
        const trc_report_stats *stats =
            test_data ? &test_data->stats : NULL;

        switch (mv)
        {
            case TRC_DB_WALKER_SON:
                subtests[level++] = init_string;
                /*@fallthrough@*/

            case TRC_DB_WALKER_BROTHER:
                if ((level & 1) == 1)
                {
                    /* Test entry */
                    if (mv != TRC_DB_WALKER_SON)
                    {
                        te_string_cut(&test_path,
                                      strlen(last_test_name) + 1);
                    }

                    last_test_name = trc_db_walker_get_test(walker)->name;

                    rc = te_string_append(&test_path, "-%s",
                                          last_test_name);
                    if (rc != 0)
                        break;

                    if (!trc_report_test_output(stats, flags))
                        break;

                    rc = te_string_append(&subtests[level - 1], "'%s',",
                                          test_path);
                    if (rc != 0)
                        break;

                    rc = trc_report_javascript_table_entry(f, test->name,
                                                           test_path.ptr,
                                                           stats);
                }
                break;

            case TRC_DB_WALKER_FATHER:
                level--;
                if ((level & 1) == 0)
                {
                    /* Back from the test to parent iteration */
                    te_string_cut(&test_path,
                                  strlen(last_test_name) + 1);

                    last_test_name = trc_db_walker_get_test(walker)->name;

                    rc = trc_report_javascript_package_subtests(f,
                                     test_path.ptr, subtests[level].ptr);
    
                    te_string_free(&subtests[level]);
                }
                break;

            default:
                assert(FALSE);
                break;
        }
    }
    WRITE_STR(trc_report_javascript_table_end);

cleanup:
    trc_db_free_walker(walker);
    te_string_free(&test_path);
    return rc;
}
#endif


/**
 * Generate one table in HTML report.
 *
 * @param f     File stream to write to
 * @param ctx   TRC report context
 * @param stats Is it statistics or details mode?
 * @param flags Output flags
 *
 * @return      Status code.
 */
static te_errno
trc_report_html_table(FILE    *f, trc_report_ctx *ctx, 
                      te_bool  is_stats, unsigned int flags)
{
    te_errno              rc = 0;
    te_trc_db_walker     *walker;
    trc_db_walker_motion  mv;
    unsigned int          level = 0;
    te_bool               anchor = FALSE; /* FIXME */
    const char           *last_test_name = NULL;
    te_string             test_path = TE_STRING_INIT;
    te_string             level_str = TE_STRING_INIT;

    walker = trc_db_new_walker(ctx->db);
    if (walker == NULL)
        return TE_ENOMEM;

    if (is_stats)
        WRITE_STR(trc_report_html_tests_stats_start);
    else
        WRITE_STR(trc_report_html_test_exp_got_start);

    while ((rc == 0) &&
           ((mv = trc_db_walker_move(walker)) != TRC_DB_WALKER_ROOT))
    {
        switch (mv)
        {
            case TRC_DB_WALKER_SON:
                level++;
                if ((level & 1) == 1)
                {
                    /* Test entry */
                    if (level > 1)
                    {
                        rc = te_string_append(&level_str, "*-");
                        if (rc != 0)
                            break;
                    }
                }
                /*@fallthrou@*/

            case TRC_DB_WALKER_BROTHER:
                if ((level & 1) == 1)
                {
                    /* Test entry */
                    if (mv != TRC_DB_WALKER_SON)
                    {
                        te_string_cut(&test_path,
                                      strlen(last_test_name) + 1);
                    }

                    last_test_name = trc_db_walker_get_test(walker)->name;

                    rc = te_string_append(&test_path, "-%s",
                                          last_test_name);
                    if (rc != 0)
                        break;
                    if (is_stats)
                        rc = trc_report_test_stats_to_html(f, ctx, walker,
                                                           flags,
                                                           test_path.ptr,
                                                           level_str.ptr);
                }
                else
                {
                    if (!is_stats)
                        rc = trc_report_exp_got_to_html(f, ctx, walker,
                                                        flags, &anchor,
                                                        test_path.ptr,
                                                        level_str.ptr);
                }
                break;

            case TRC_DB_WALKER_FATHER:
                level--;
                if ((level & 1) == 0)
                {
                    /* Back from the test to parent iteration */
                    te_string_cut(&level_str, strlen("*-"));
                    te_string_cut(&test_path,
                                  strlen(last_test_name) + 1);
                    last_test_name = trc_db_walker_get_test(walker)->name;
                }
                break;

            default:
                assert(FALSE);
                break;
        }
    }
    if (is_stats)
        WRITE_STR(trc_tests_stats_end);
    else
        WRITE_STR(trc_test_exp_got_end);

cleanup:
    trc_db_free_walker(walker);
    te_string_free(&test_path);
    te_string_free(&level_str);
    return rc;
}

/**
 * Copy all content of one file to another.
 *
 * @param dst   Destination file
 * @param src   Source file
 *
 * @return      Status code.
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
                   const char *title, FILE *header,
                   unsigned int flags)
{
    FILE       *f;
    te_errno    rc;
    tqe_string *tag;
    te_string   title_string = TE_STRING_INIT;

    f = fopen(filename, "w");
    if (f == NULL)
    {
        rc = te_rc_os2te(errno);
        ERROR("Failed to open file to write HTML report to: %r", rc);
        return rc;
    }

    /* HTML header */
    if (title == NULL)
    {
        rc = te_string_append(&title_string, "%s",
                              trc_html_title_def);
        TAILQ_FOREACH(tag, &gctx->tags, links)
        {
            te_string_append(&title_string, "%s %s",
                             (tag == TAILQ_FIRST(&gctx->tags)) ?
                             ":" : ",", tag->v);
        }
    }
    fprintf(f, trc_html_doc_start,
            (title != NULL) ? title : title_string.ptr);
    if (title != NULL)
        fprintf(f, "<h1 align=center>%s</h1>\n", title);
    if (gctx->db->version != NULL)
        fprintf(f, "<h2 align=center>%s</h2>\n", gctx->db->version);

    if (~flags & TRC_REPORT_KEYS_ONLY)
    {
        WRITE_STR("<a name=\"start\"> </a>\n");

        /* TRC tags */
        WRITE_STR("<b>Tags:</b>");
        TAILQ_FOREACH(tag, &gctx->tags, links)
        {
            fprintf(f, "  %s", tag->v);
        }
        WRITE_STR("<p/>");

        if (~flags & TRC_REPORT_NO_KEYS)
        {
            WRITE_STR("<a href=\"#keys_table\"><b>Bugs</b></a>\n");
        }

        /* Header provided by user */
        if (header != NULL)
        {
            rc = file_to_file(f, header);
            if (rc != 0)
            {
                ERROR("Failed to copy header to HTML report");
                goto cleanup;
            }
        }

#if TRC_USE_STATS_POPUP
        if (~flags & TRC_REPORT_NO_SCRIPTS)
        {
            /* Build javascript tree of tests */
            rc = trc_report_javascript_table(f, gctx, flags);
            if (rc != 0)
                goto cleanup;
        }
#endif

        if (~flags & TRC_REPORT_NO_TOTAL_STATS)
        {
            /* Grand total */
            rc = trc_report_stats_to_html(f, &gctx->stats);
            if (rc != 0)
                goto cleanup;
        }

        if (~flags & TRC_REPORT_NO_PACKAGES_ONLY)
        {
            /* Report for packages */
            rc = trc_report_html_table(f, gctx, TRUE,
                                       flags | TRC_REPORT_NO_SCRIPTS);
            if (rc != 0)
                goto cleanup;
        }

        if (~flags & TRC_REPORT_NO_SCRIPTS)
        {
            /*
             * Report with iterations of packages and
             * w/o iterations of tests
             */
            rc = trc_report_html_table(f, gctx, TRUE, flags);
            if (rc != 0)
                goto cleanup;
        }

        if ((~flags & TRC_REPORT_STATS_ONLY) &&
            (~flags & TRC_REPORT_NO_SCRIPTS))
        {
            /* Full report */
            rc = trc_report_html_table(f, gctx, FALSE, flags);
            if (rc != 0)
                goto cleanup;
        }
    }

    if (~flags & TRC_REPORT_NO_KEYS)
    {
        trc_keys *keys = calloc(1, sizeof(trc_keys));
        if (keys == NULL)
        {
            rc = te_rc_os2te(errno);
            goto cleanup;
        }
        trc_keys_init(keys);

        rc = trc_report_keys_collect(keys, gctx,
                                     flags & ~TRC_REPORT_KEYS_SANITY);
        if (rc != 0)
            goto cleanup;

        WRITE_STR("<a name=\"keys_table\"> </a>\n");
        trc_report_keys_to_html(f, gctx, "te-trc-key", keys,
                                !(~flags & TRC_REPORT_KEYS_ONLY),
                                "Table of bugs that "
                                "lead to known test failures.");
        trc_keys_free(keys);

        if (flags & TRC_REPORT_KEYS_SANITY)
        {
            trc_keys *sanity_keys = calloc(1, sizeof(trc_keys));
            if (sanity_keys == NULL)
            {
                rc = te_rc_os2te(errno);
                goto cleanup;
            }
            trc_keys_init(sanity_keys);

            rc = trc_report_keys_collect(sanity_keys, gctx, flags);
            if (rc != 0)
                goto cleanup;

            WRITE_STR("<a name=\"sanity_table\"> </a>\n");
            trc_report_keys_to_html(f, gctx, "te-trc-key", sanity_keys,
                                    !(~flags & TRC_REPORT_KEYS_ONLY),
                                    "Table of bugs (prehaps fixed) that "
                                    "do not lead to test failures.");
            trc_keys_free(sanity_keys);
        }
    }

    WRITE_STR("<a href=\"#start\"><b>Up</b></a>\n");

    /* HTML footer */
    WRITE_STR(trc_html_doc_end);

    fclose(f);

    return 0;

cleanup:
    fclose(f);
    unlink(filename);
    return rc;
}
