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
#include "te_string.h"
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

/** Define to 1 to enable links to test logs in detailed tables */
#ifdef WITH_LOG_URLS
#define TRC_USE_LOG_URLS 1
#else
#define TRC_USE_LOG_URLS 0
#endif

#define WRITE_FILE(format...) \
    do {                                                    \
        if (fprintf(f, format) < 0)                         \
        {                                                   \
            rc = te_rc_os2te(errno);                        \
            assert(rc != 0);                                \
            ERROR("Writing to the file failed: %r", rc);    \
            goto cleanup;                                   \
        }                                                   \
    } while (0)

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
    ((expr_) ? PRINT_STR(str1_) : "")

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

#define TRC_STATS_SHOW_HREF_CLOSE_PARAM_PREFIX "','"

#define TRC_STATS_SHOW_HREF_END "</a>"

#define TRC_TEST_STATS_SHOW_HREF_START \
    "        <a href=\"javascript:showResultWilds('StatsTip','"
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
"    .test_stats_zero { text-align: right; padding-left: 0.14in; "
"padding-right: 0.14in}\n"
"    .test_stats_passed_unexp { text-align: right; padding-left: 0.14in; "
"padding-right: 0.14in; background-color:#ffdfdf;}\n"
"    .test_stats_failed_unexp { text-align: right; padding-left: 0.14in; "
"padding-right: 0.14in; background-color:#ffdfdf;}\n"
"    .unexp_no_highlight { text-align: right; padding-left: 0.14in; "
"padding-right: 0.14in}\n"
"    .test_stats_aborted_new { text-align: right; padding-left: 0.14in; "
"padding-right: 0.14in; background-color:#ffdfdf;}\n"
"    .test_stats_not_run { font-weight: bold; text-align: right; "
"padding-left: 0.14in; padding-right: 0.14in}\n"
"    .test_stats_skipped_exp { text-align: right; padding-left: 0.14in; "
"padding-right: 0.14in}\n"
"    .test_stats_skipped_unexp { text-align: right; padding-left: 0.14in; "
"padding-right: 0.14in}\n"
"    .test_stats_keys { }\n"
"    .test_stats_names { }\n"
"    wbr { display: inline-block; }\n"
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
"        background-color: #ffffe0;\n"
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
"        background-color: #ffffe0;\n"
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
"    var  voidResWarn = '<div><br/><br/><strong>NOTE: </strong>If ' +\n"
"                       'there is no link ' + \n"
"                       'displaying wildcards for a ' +\n"
"                       'specific kind of results of a ' +\n"
"                       'given test, it means that ' +\n"
"                       'all iterations of the test have ' +\n"
"                       'the same kind of result.</div>';\n"
"    var  voidWildWarn = '<div><br/><br/><strong>NOTE: </strong>If ' +\n"
"                        'there is no wildcards for a ' + \n"
"                        'specific kind of result of a ' +\n"
"                        'given test, it means that ' +\n"
"                        'all iterations of the test have ' +\n"
"                        'this kind of result. Wildcards ' +\n"
"                        'can be used for updating TRC XML only if ' +\n"
"                        'this is a full TRC report with information ' +\n"
"                        'about all possible iterations of a given ' +\n"
"                        'test.</div>';\n"
"\n"
"    function getChildrenByName(elem, name)\n"
"    {\n"
"        var chlds = elem.childNodes;\n"
"        var i;\n"
"        var res = new Array();\n"
"        for (i = 0; i < chlds.length; i++)\n"
"            if (chlds[i].name == name)\n"
"                res[res.length] = chlds[i];\n"
"\n"
"        return res;\n"
"    }\n"
"\n"
"    function getInnerText(elem)\n"
"    {\n"
"        if (elem.innerText)\n"
"            return elem.innerText;\n"
"        else\n"
"            return elem.textContent;\n"
"    }\n"
"\n"
"    function getTestIters(name)\n"
"    {\n"
"        var res = new Object();\n"
"        var elms = document.getElementsByName(name);\n"
"        var param_elms;\n"
"        var test_row;\n"
"        var params_td;\n"
"        var results_td;\n"
"        var results_list;\n"
"        var i;\n"
"        var j;\n"
"        var k;\n"
"        var params = new Array();\n"
"        var par_vals;\n"
"        var iter_params;\n"
"        var iters = new Array();\n"
"        var inner_text;\n"
"\n"
"        for (i = 0; i < elms.length; i++)\n"
"        {\n"
"            iters[i] = new Object();\n"
"            test_row = elms[i].parentNode.parentNode.parentNode;\n"
"            params_td = test_row.getElementsByTagName('td')[1];\n"
"            results_td = test_row.getElementsByTagName('td')[3];\n"
"            param_elms = getChildrenByName(params_td, 'param');\n"
"            par_vals = new Array();\n"
"            iter_params = new Array();\n"
"\n"
"            for (j = 0; j < param_elms.length; j++)\n"
"            {\n"
"                inner_text = getInnerText(param_elms[j]);\n"
"\n"
"                if (params.indexOf(inner_text) == -1)\n"
"                    params[params.length] = inner_text;\n"
"\n"
"                iter_params[j] = inner_text;\n"
"            }\n"
"\n"
"            for (j = 0; j < params.length; j++)\n"
"            {\n"
"                if (iter_params.indexOf(params[j]) == -1)\n"
"                    par_vals[j] = null;\n"
"                else\n"
"                {\n"
"                    inner_text = getInnerText(getChildrenByName(\n"
"                                              params_td,\n"
"                                              params[j] +\n"
"                                              '_val')[0]);\n"
"\n"
"                    par_vals[j] = inner_text;\n"
"                }\n"
"            }\n"
"\n"
"            iters[i].par_vals = par_vals;\n"
"            iters[i].result_text = getInnerText(results_td).\n"
"                                       replace(/^\\s+|\\s+$/g, '');\n"
"            if (results_td.className.search('unexp') != -1)\n"
"                iters[i].expected = false;\n"
"            else\n"
"                iters[i].expected = true;\n"
"\n"
"            results_list = results_td.getElementsByTagName('span');\n"
"            iters[i].result = getInnerText(results_list[1]).\n"
"                                replace(/^\\s+|\\s+$/g, '');\n"
"            iters[i].verdicts = new Array();\n"
"\n"
"            for (j = 2; j < results_list.length; j++)\n"
"            {\n"
"                inner_text = getInnerText(results_list[j]);\n"
"                k = inner_text.lastIndexOf(';');\n"
"                if (k != -1)\n"
"                    iters[i].verdicts[j - 2] =\n"
"                        inner_text.substring(0, k - 1).\n"
"                        replace(/^\\s+|\\s+$/g, '');\n"
"                else\n"
"                    iters[i].verdicts[j - 2] = inner_text.\n"
"                        replace(/^\\s+|\\s+$/g, '');\n"
"            }\n"
"        }\n"
"\n"
"        res.iters = iters;\n"
"        res.par_names = params;\n"
"\n"
"        return res;\n"
"    }\n"
"\n"
"    function getMatchItersNumber(name, comp_func)\n"
"    {\n"
"        var tst_results = getTestIters(name);\n"
"        var tst_its = tst_results.iters;\n"
"        var match_itr_count = 0;\n"
"        var k;\n"
"\n"
"        for (k = 0; k < tst_its.length; k++)\n"
"        {\n"
"            if (comp_func(tst_its[k]))\n"
"                match_itr_count++;\n"
"        }\n"
"\n"
"        return match_itr_count;\n"
"    }\n"
"\n"
"    /**\n"
"     * Function determines wildcards describing those and only those\n"
"     * iterations of test on which comp_func function returns true\n"
"     *\n"
"     * @param tst_results      Array describing test iterations (output \n"
"     *                         of getTestIters())\n"
"     * @param comp_func        Function distinguishing iterations of \n"
"     *                         interest\n"
"     * @return Array of wildcards (sets of parameters with values)\n"
"     */\n"
"    function getResultWilds(tst_results, comp_func)\n"
"    {\n"
"        var arr = new Array(tst_results.par_names.length);\n"
"        var itr_prms;\n"
"        var tst_its = tst_results.iters;\n"
"        var itr_bad;\n"
"        var itr_sel;\n"
"        var tst_itr_sel = new Array(tst_results.iters.length);\n"
"        var n_tst_sel = 0;\n"
"        var itr_res;\n"
"        var match_itr_count = 0;\n"
"        var i;\n"
"        var j;\n"
"        var k;\n"
"        var l;\n"
"        var m;\n"
"        var res = new Array();\n"
"\n"
"        if (tst_its.length == 0)\n"
"            return res;\n"
"\n"
"        for (k = 0; k < tst_its.length; k++)\n"
"        {\n"
"            if (comp_func(tst_its[k]))\n"
"                match_itr_count++;\n"
"        }\n"
"\n"
"        for (i = 0; i <= arr.length; i++)\n"
"        {\n"
"            for (j = 0; j < i; j++)\n"
"                arr[j] = j;\n"
"\n"
"            /* In this cycle all possible combinations of\n"
"             * i parameters are generated and checked \n"
"             * until needed wildcards will be found */\n"
"            while (true)\n"
"            {\n"
"                itr_prms = new Array();\n"
"                itr_bad = new Array();\n"
"                itr_sel = new Array();\n"
"\n"
"                /* Determine all correct wildcards can be\n"
"                 * created for a given set of parameters */\n"
"                for (k = 0; k < tst_its.length; k++)\n"
"                {\n"
"                    for (l = 0; l < itr_prms.length; l++)\n"
"                    {\n"
"                        for (j = 0; j < i; j++)\n"
"                            if (tst_its[k].par_vals[arr[j]] !==\n"
"                                itr_prms[l][j] && \n"
"                                !(tst_its[k].par_vals.length <=\n"
"                                  arr[j] &&\n"
"                                  itr_prms[l][j] === null))\n"
"                                break;\n"
"\n"
"                        if (j == i)\n"
"                            break;\n"
"                    }\n"
"\n"
"                    if (l < itr_prms.length)\n"
"                    {\n"
"                        /* Wildcard must select only iterations\n"
"                         * of interest */\n"
"                        if (!comp_func(tst_its[k]))\n"
"                            itr_bad[l] = true;\n"
"\n"
"                        /* Wildcards must not intersect */\n"
"                        if (tst_itr_sel[k])\n"
"                            itr_sel[l] = true;\n"
"                    }\n"
"\n"
"                    if (l == itr_prms.length)\n"
"                    {\n"
"                        itr_prms[l] = new Array();\n"
"\n"
"                        if (!comp_func(tst_its[k]))\n"
"                            itr_bad[l] = true;\n"
"                        else\n"
"                            itr_bad[l] = false;\n"
"\n"
"                        if (tst_itr_sel[k])\n"
"                            itr_sel[l] = true;\n"
"                        else\n"
"                            itr_sel[l] = false;\n"
"\n"
"                        for (j = 0; j < i; j++)\n"
"                        {\n"
"                            if (tst_its[k].par_vals.length > arr[j])\n"
"                                    itr_prms[l][j] = tst_its[k].\n"
"                                        par_vals[arr[j]];\n"
"                            else\n"
"                                itr_prms[l][j] = null;\n"
"                        }\n"
"                    }\n"
"                }\n"
"                \n"
"                itr_res = new Array();\n"
"                for (j = 0; j < itr_prms.length; j++)\n"
"                {\n"
"                    if (!itr_bad[j] && !itr_sel[j])\n"
"                    {\n"
"                        itr_res[itr_res.length] = itr_prms[j];\n"
"                        itr_res[itr_res.length - 1].match_count = 0;\n"
"                    }\n"
"                }\n"
"\n"
"                /* Update information about iterations\n"
"                 * already selected by wildcards */\n"
"                if (itr_res.length > 0)\n"
"                {\n"
"                    res[res.length] = new Object();\n"
"                    res[res.length - 1].par_idx = arr.slice();\n"
"                    res[res.length - 1].par_idx.length = i;\n"
"                    res[res.length - 1].itr_prms = itr_res;\n"
"\n"
"                    for (k = 0; k < tst_its.length; k++)\n"
"                    {\n"
"                        if (tst_itr_sel[k])\n"
"                            continue;\n"
"\n"
"                        for (l = 0; l < itr_res.length; l++)\n"
"                        {\n"
"                            for (j = 0; j < i; j++)\n"
"                                if (tst_its[k].par_vals[arr[j]] !==\n"
"                                    itr_res[l][j] && \n"
"                                    !(tst_its[k].par_vals.length <=\n"
"                                      arr[j] &&\n"
"                                      itr_res[l][j] === null))\n"
"                                    break;\n"
"\n"
"                            if (j == i)\n"
"                            {\n"
"                                tst_itr_sel[k] = true;\n"
"                                n_tst_sel++;\n"
"                                itr_res[l].match_count++;\n"
"                                break;\n"
"                            }\n"
"                        }\n"
"                    }\n"
"\n"
"                    if (n_tst_sel == match_itr_count)\n"
"                        break;\n"
"                }\n"
"\n"
"                /* Generating next combination of parameters */\n"
"                for (j = i - 1; j >= 0; j--)\n"
"                    if (arr[j] < arr.length - (i - j))\n"
"                        break;\n"
"\n"
"                if (j < 0)\n"
"                    break;\n"
"\n"
"                arr[j]++;\n"
"                for (m = j + 1; m < i; m++)\n"
"                    arr[m] = arr[m - 1] + 1;\n"
"            }\n"
"\n"
"            if (n_tst_sel == match_itr_count)\n"
"                break;\n"
"        }\n"
"\n"
"        return res;\n"
"    }\n"
"\n"
"    function isExpPassed(x)\n"
"    {\n"
"        if (x.result == 'PASSED' && x.expected)\n"
"            return true;\n"
"        else\n"
"            return false;\n"
"    }\n"
"\n"
"    function isExpFailed(x)\n"
"    {\n"
"        if (x.result == 'FAILED' && x.expected)\n"
"            return true;\n"
"        else\n"
"            return false;\n"
"    }\n"
"\n"
"    function isUnExpPassed(x)\n"
"    {\n"
"        if (x.result == 'PASSED' && !x.expected)\n"
"            return true;\n"
"        else\n"
"            return false;\n"
"    }\n"
"\n"
"    function isUnExpFailed(x)\n"
"    {\n"
"        if (x.result == 'FAILED' && !x.expected)\n"
"            return true;\n"
"        else\n"
"            return false;\n"
"    }\n"
"\n"
"    function hasVerdict(v)\n"
"    {\n"
"        function result(x)\n"
"        {\n"
"            var i;\n"
"            for (i = 0; i < x.verdicts.length; i++)\n"
"            {\n"
"                if (x.verdicts[i] == v.replace(/^\\s+|\\s+$/g, ''))\n"
"                    return true;\n"
"            }\n"
"\n"
"            return false;\n"
"        }\n"
"\n"
"        return result;\n"
"    }\n"
"\n"
"    function hasResult(r)\n"
"    {\n"
"        function result(x)\n"
"        {\n"
"            if (x.result == r.replace(/^\\s+|\\s+$/g, ''))\n"
"                return true;\n"
"\n"
"            return false;\n"
"        }\n"
"\n"
"        return result;\n"
"    }\n"
"\n"
"    function hasResultText(rt)\n"
"    {\n"
"        function result(x)\n"
"        {\n"
"            if (x.result_text == rt.replace(/^\\s+|\\s+$/g, ''))\n"
"                return true;\n"
"\n"
"            return false;\n"
"        }\n"
"\n"
"        return result;\n"
"    }\n"
"\n"
"    function getFieldDescr(field)\n"
"    {\n"
"        switch (field)\n"
"        {\n"
"            case 'passed_exp':\n"
"                return 'expectedly passed iterations';\n"
"            case 'passed_unexp':\n"
"                return 'unexpectedly passed iterations';\n"
"            case 'failed_exp':\n"
"                return 'expectedly failed iterations';\n"
"            case 'failed_unexp':\n"
"                return 'unexpectedly failed iterations';\n"
"            default:\n"
"                return 'unknown';\n"
"        }\n"
"    }\n"
"\n"
"    function getFieldName(field)\n"
"    {\n"
"        switch (field)\n"
"        {\n"
"            case 'passed_exp':\n"
"                return 'Passed expectedly';\n"
"            case 'passed_unexp':\n"
"                return 'Passed unexpectedly';\n"
"            case 'failed_exp':\n"
"                return 'Failed expectedly';\n"
"            case 'failed_unexp':\n"
"                return 'Failed unexpectedly';\n"
"            case 'total':\n"
"                return 'Total';\n"
"            case 'aborted_new':\n"
"                return 'Aborted, New';\n"
"            case 'not_run':\n"
"                return 'Not Run';\n"
"            case 'skipped_exp':\n"
"                return 'Skipped expectedly';\n"
"            case 'skipped_unexp':\n"
"                return 'Skipped unexpectedly';\n"
"            default:\n"
"                return 'unknown';\n"
"        }\n"
"    }\n"
"\n"
"    function testFieldHTML(test, field)\n"
"    {\n"
"        var innerHTML = '';\n"
"\n"
"        if (test['type'] == 'script' &&\n"
"            (field == 'passed_exp' || field == 'passed_unexp' ||\n"
"             field == 'failed_exp' || field == 'failed_unexp') &&\n"
"             test[field] != 0 && test[field] != test['passed_exp'] +\n"
"             test['passed_unexp'] + test['failed_exp'] +\n"
"             test['failed_unexp'] &&\n"
"             resultWildsNumber(test['path'], field) > 0)\n"
"            innerHTML += '<td><a href=\"javascript:showResultWilds' +\n"
"                         '(\\'StatsTip\\',\\'' +\n"
"                         test['path'] + '\\',\\'' + field + '\\')\">' +\n"
"                         test[field] + '</a></td>';\n"
"        else\n"
"            innerHTML += '<td>' + test[field] + '</td>';\n"
"\n"
"        return innerHTML;\n"
"    }\n"
"\n"
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
"                     '<a href=\"javascript:hidePopups()\"' +\n"
"                     'style=\"text-decoration: none\">' +\n"
"                     '<strong>[x]</strong></a></span>';\n"
"        innerHTML += '<div align=\"center\"><b>Package: ' +\n"
"                     package['name'] + '</b></div>';\n"
"        innerHTML += '<div style=\"height:380px;overflow:auto;\">';\n"
"        innerHTML += '<table border=1 cellpadding=4 cellspacing=3 ' +\n"
"                     'width=100%% style=\"font-size:small;' +\n"
"                     'text-align:right;\">';\n"
"        innerHTML += '<tr><td><b>Test</b></td>' +\n"
"                     '<td><b>Total</b></td>' +\n"
"                     '<td><b>Passed<br/>expect</b></td>' +\n"
"                     '<td><b>Failed<br/>expect</b></td>' +\n"
"                     '<td><b>Passed<br/>unexpect</b></td>' +\n"
"                     '<td><b>Failed<br/>unexpect</b></td>' +\n"
"                     '<td><b>Not run</b></td></tr>';\n"
"        var test_no;\n"
"        var has_scripts = false;\n"
"\n"
"        for (test_no in test_list)\n"
"        {\n"
"            var test = test_stats[test_list[test_no]];\n"
"            var links_num = document.getElementsByName(test['path']).\n"
"                               length\n"
"\n"
"            if (test['type'] == 'script')\n"
"                has_scripts = true;\n"
"            innerHTML += '<tr>';\n"
"            innerHTML += '<td>';\n"
"\n"
"            if (links_num > 0)\n"
"                innerHTML += '<a href=\"#' +\n"
"                             test['path'].replace('\\/', '%%2F') +\n"
"                             '\">';\n"
"            innerHTML += test['name'];\n"
"            if (links_num > 0)\n"
"                innerHTML += '</a>';\n"
"\n"
"            innerHTML += '</td>' +\n"
"                         '<td>' + test['total'] + '</td>' +\n"
"                         testFieldHTML(test, 'passed_exp') +\n"
"                         testFieldHTML(test, 'failed_exp') +\n"
"                         testFieldHTML(test, 'passed_unexp') +\n"
"                         testFieldHTML(test, 'failed_unexp') +\n"
"                         '<td>' + test['not_run'] + '</td>';\n"
"            innerHTML += '</tr>';\n"
"        }\n"
"        innerHTML += '</table>';\n"
"        if (has_scripts)\n"
"            innerHTML += voidResWarn;\n"
"        innerHTML += '</div>';\n"
"\n"
"        return innerHTML;\n"
"    }\n"
"\n"
"    function filterStats(name,field)\n"
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
"                     '<a href=\"javascript:hidePopups()\"' +\n"
"                     'style=\"text-decoration: none\">' +\n"
"                     '<strong>[x]</strong></a></span>';\n"
"        innerHTML += '<div align=\"center\"><b>Package: ' +\n"
"                     package['name'] + '</b></div>';\n"
"        innerHTML += '<div style=\"height:380px;overflow:auto;\">';\n"
"        innerHTML += '<table border=1 cellpadding=4 cellspacing=3 ' +\n"
"                     'width=100%% style=\"font-size:small;' +\n"
"                     'text-align:right;\">';\n"
"        innerHTML += '<tr><td><b>Test</b></td>' +\n"
"                     '<td><b>' + getFieldName(field) + \n"
"                     '</b></td></tr>';\n"
"        var test_no;\n"
"        var has_scripts = false;\n"
"        var link_text;\n"
"        var field_links_num;\n"
"        var links_num;\n"
"\n"
"        for (test_no in test_list)\n"
"        {\n"
"            var test = test_stats[test_list[test_no]];\n"
"            if (test['type'] == 'script')\n"
"               has_scripts = true;\n"
"            if (test[field] <= 0)\n"
"                continue;\n"
"\n"
"            link_text = test['path'];\n"
"            links_num = document.getElementsByName(link_text).\n"
"                            length\n"
"            if (field != 'total' && test['type'] == 'script')\n"
"                link_text += '_' + field;\n"
"            field_links_num = document.getElementsByName(link_text).\n"
"                                  length\n"
"            innerHTML += '<tr>';\n"
"            innerHTML += '<td>';\n"
"            if (field_links_num > 0)\n"
"                innerHTML += '<a href=\"#' +\n"
"                             link_text.replace('\\/', '%%2F')+ '\">'\n"
"            /*\n"
"            else if (links_num > 0)\n"
"                innerHTML += '<a href=\"#' +\n"
"                             test['path'].replace('\\/', '%%2F')+ '\">'\n"
"            */\n"
"            innerHTML += test['name'];\n"
"            /*\n"
"            if (links_num > 0)\n"
"            */\n"
"            if (field_links_num > 0)\n"
"                innerHTML += '</a>';\n"
"            innerHTML += '</td>';\n"
"            innerHTML += testFieldHTML(test, field);\n"
"            innerHTML += '</tr>';\n"
"        }\n"
"\n"
"        innerHTML += '</table>';\n"
"        if ((field == 'passed_exp' || field == 'passed_unexp' || \n"
"             field == 'failed_exp' || field == 'failed_unexp') &&\n"
"            has_scripts)\n"
"           innerHTML += voidResWarn;\n"
"        innerHTML += '</div>';\n"
"\n"
"        return innerHTML;\n"
"    }\n"
"\n"
"    function retPrevTip(obj_name)\n"
"    {\n"
"        document.getElementById(obj_name).\n"
"            innerHTML=document.prev_tip;\n"
"    }\n"
"\n"
"    function frameHead(obj_name, name)\n"
"    {\n"
"        var innerHTML = '';\n"
"\n"
"        if (document.prev_tip != '')\n"
"            innerHTML += '<span id=\"prev\"> ' +\n"
"                         '<a href=\"javascript:' +\n"
"                         'retPrevTip(\\'' + obj_name + '\\')\"' +\n"
"                         'style=\"text-decoration: none\">' +\n"
"                         '<strong>[<<<]</strong></a></span>';\n"
"        innerHTML += '<span id=\"close\"> ' +\n"
"                     '<a href=\"javascript:hidePopups()\"' +\n"
"                     'style=\"text-decoration: none\">' +\n"
"                     '<strong>[x]</strong></a></span>';\n"
"        innerHTML += '<div align=\"center\"><b>Test: ' + name +\n"
"                     '</b></div>';\n"
"        innerHTML += '<div style=\"height:380px;overflow:auto;\">';\n"
"        innerHTML += '<table border=1 cellpadding=4 cellspacing=3 ' +\n"
"                     'width=100%% style=\"font-size:small;' +\n"
"                     'text-align:left;\">';\n"
"\n"
"        return innerHTML;\n"
"    }\n"
"\n"
"    function frameResults(results, res)\n"
"    {\n"
"        var innerHTML = '';\n"
"        var i;\n"
"        var j;\n"
"        var k;\n"
"        var l;\n"
"        var count = 0;\n"
"\n"
"        if (res !== null)\n"
"        {\n"
"            for (l = 0; l < res.length; l++)\n"
"            {\n"
"                for (i = 0; i < res[l].itr_prms.length; i++)\n"
"                {\n"
"                    count += res[l].itr_prms[i].match_count;\n"
"                    if (res[l].par_idx.length == 0)\n"
"                        continue;\n"
"\n"
"                    innerHTML += '<tr><td>';\n"
"                    \n"
"                    for (j = 0; j < res[l].par_idx.length; j++)\n"
"                    {\n"
"                        k = res[l].par_idx[j];\n"
"                        innerHTML += results.par_names[k] + '=' +\n"
"                                     res[l].itr_prms[i][j];\n"
"                        if (j < res[l].par_idx.length - 1)\n"
"                            innerHTML += '<br/>';\n"
"                    }\n"
"\n"
"                    innerHTML += '</td><td>';\n"
"                    innerHTML += res[l].itr_prms[i].match_count.\n"
"                                   toString();\n"
"                    innerHTML += '</td></tr>';\n"
"                }\n"
"            }\n"
"        }\n"
"\n"
"        innerHTML += '<tr><td><strong>Total count:</strong></td><td>' +\n"
"                     '<strong>' + count.toString() + '</strong>' +\n"
"                     '</td></tr>';\n"
"        return innerHTML;\n"
"    }\n"
"\n"
"    function resultWildsNumber(name, field)\n"
"    {\n"
"        var results;\n"
"        var res;\n"
"\n"
"        results = getTestIters(name);\n"
"\n"
"        switch(field)\n"
"        {\n"
"            case 'passed_exp':\n"
"                res = getResultWilds(results, isExpPassed);\n"
"                break;\n"
"            case 'failed_exp':\n"
"                res = getResultWilds(results, isExpFailed);\n"
"                break;\n"
"            case 'passed_unexp':\n"
"                res = getResultWilds(results, isUnExpPassed);\n"
"                break;\n"
"            case 'failed_unexp':\n"
"                res = getResultWilds(results, isUnExpFailed);\n"
"                break;\n"
"            default:\n"
"                return 0;\n"
"        }\n"
"\n"
"        return res.length == 1 ? (res[0].par_idx.length > 0 ? 1 : 0) :\n"
"                                  res.length;\n"
"    }\n"
"\n"
"    function resultWildsHTML(obj_name, name, field)\n"
"    {\n"
"        var results;\n"
"        var res;\n"
"        var innerHTML = '';\n"
"\n"
"        innerHTML += frameHead(obj_name, name);\n"
"        innerHTML += '<tr><td><b>Wildcards for ' +\n"
"                     getFieldDescr(field) + '</b></td>' +\n"
"                     '<td><strong>Number of iterations</strong>' +\n"
"                     '</td></tr>';\n"
"\n"
"        results = getTestIters(name);\n"
"\n"
"        switch(field)\n"
"        {\n"
"            case 'passed_exp':\n"
"                res = getResultWilds(results, isExpPassed);\n"
"                break;\n"
"            case 'failed_exp':\n"
"                res = getResultWilds(results, isExpFailed);\n"
"                break;\n"
"            case 'passed_unexp':\n"
"                res = getResultWilds(results, isUnExpPassed);\n"
"                break;\n"
"            case 'failed_unexp':\n"
"                res = getResultWilds(results, isUnExpFailed);\n"
"                break;\n"
"            default:\n"
"                res = null;\n"
"        }\n"
"\n"
"        innerHTML += frameResults(results, res);\n"
"        innerHTML += '</table>';\n"
"        innerHTML += voidWildWarn;\n"
"        innerHTML += '</div>';\n"
"\n"
"        return innerHTML;\n"
"    }\n"
"\n"
"    function verdictWildsHTML(obj_name, name, verdict)\n"
"    {\n"
"        var results;\n"
"        var res;\n"
"        var innerHTML = '';\n"
"\n"
"        innerHTML += frameHead(obj_name, name);\n"
"        innerHTML += '<tr><td><b>Wildcards for verdict:<br/>' +\n"
"                     verdict + '</b></td>' +\n"
"                     '<td><strong>Number of iterations</strong>' +\n"
"                     '</td></tr>';\n"
"\n"
"        results = getTestIters(name);\n"
"        res = getResultWilds(results, hasVerdict(verdict));\n"
"\n"
"        innerHTML += frameResults(results, res);\n"
"        innerHTML += '</table>';\n"
"        innerHTML += voidWildWarn;\n"
"        innerHTML += '</div>';\n"
"\n"
"        return innerHTML;\n"
"    }\n"
"\n"
"    function iterResultWildsHTML(obj_name, name, result)\n"
"    {\n"
"        var results;\n"
"        var res;\n"
"        var innerHTML = '';\n"
"\n"
"        innerHTML += frameHead(obj_name, name);\n"
"        innerHTML += '<tr><td><b>Wildcards for ' +\n"
"                     result + ' result</b></td>' +\n"
"                     '<td><strong>Number of iterations</strong>' +\n"
"                     '</td></tr>';\n"
"\n"
"        results = getTestIters(name);\n"
"        res = getResultWilds(results, hasResult(result));\n"
"\n"
"        innerHTML += frameResults(results, res);\n"
"        innerHTML += '</table>';\n"
"        innerHTML += voidWildWarn;\n"
"        innerHTML += '</div>';\n"
"\n"
"        return innerHTML;\n"
"    }\n"
"\n"
"    function iterResultTextWildsHTML(obj_name, name, element)\n"
"    {\n"
"        var results;\n"
"        var res;\n"
"        var innerHTML = '';\n"
"\n"
"        innerHTML += frameHead(obj_name, name);\n"
"        innerHTML += '<tr><td><strong>Wildcards for result:<br/>' +\n"
"                     element.innerHTML.replace(/\\[\\*\\]/, '') +\n"
"                     '</strong></td>' +\n"
"                     '<td><strong>Number of iterations</strong>' +\n"
"                     '</td></tr>';\n"
"\n"
"        results = getTestIters(name);\n"
"        res = getResultWilds(results, hasResultText(\n"
"                                            getInnerText(element)));\n"
"\n"
"        innerHTML += frameResults(results, res);\n"
"        innerHTML += '</table>';\n"
"        innerHTML += voidWildWarn;\n"
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
"            var leftOffset = scrolled_x +\n"
"                             (center_x - obj.offsetWidth) - 20;\n"
"        else\n"
"            var leftOffset = scrolled_x +\n"
"                             (center_x - obj.offsetWidth) / 2;\n"
"\n"
"        var topOffset = scrolled_y +\n"
"                        (center_y - obj.offsetHeight) / 2;\n"
"\n"
"        obj.style.top = topOffset + 'px';\n"
"        obj.style.left = leftOffset + 'px';\n"
"    }\n"
"\n"
"    function showStats(obj_name,name,field)\n"
"    {\n"
"        var obj = document.getElementById(obj_name);\n"
"        var innerHTML;\n"
"        document.prev_tip = '';\n"
"\n"
"        if (typeof obj == 'undefined')\n"
"            alert('Failed to get object ' + obj_name);\n"
"\n"
"        if (field)\n"
"        {\n"
"            innerHTML = filterStats(name,field);\n"
"        }\n"
"        else\n"
"        {\n"
"            innerHTML = fillStats(name);\n"
"        }\n"
"        obj.innerHTML = innerHTML;\n"
"        centerStats(obj);\n"
"        obj.style.visibility = \"visible\";\n"
"    }\n"
"\n"
"    function showWilds(func, obj_name, name, arg)\n"
"    {\n"
"        var obj = document.getElementById(obj_name);\n"
"        if (typeof obj == 'undefined')\n"
"            alert('Failed to get object ' + obj_name);\n"
"\n"
"        if (obj_name == 'StatsTip' &&\n"
"            obj.style.visibility == \"visible\")\n"
"            document.prev_tip = obj.innerHTML;\n"
"        else\n"
"            document.prev_tip = '';\n"
"\n"
"        obj.innerHTML = func(obj_name, name, arg);\n"
"\n"
"        centerStats(obj);\n"
"        obj.style.visibility = \"visible\";\n"
"    }\n"
"\n"
"    function showResultWilds(obj_name, name, field)\n"
"    {\n"
"        showWilds(resultWildsHTML, obj_name, name, field);\n"
"    }\n"
"\n"
"    function showVerdictWilds(obj_name, name, verdict)\n"
"    {\n"
"        showWilds(verdictWildsHTML, obj_name, name, verdict);\n"
"    }\n"
"\n"
"    function showIterResultWilds(obj_name, name, result)\n"
"    {\n"
"        showWilds(iterResultWildsHTML, obj_name, name, result);\n"
"    }\n"
"\n"
"    function showIterResultTextWilds(obj_name, name, text)\n"
"    {\n"
"        showWilds(iterResultTextWildsHTML, obj_name, name, text);\n"
"    }\n"
"\n"
"    function hideObject(obj_name)\n"
"    {\n"
"        var obj = document.getElementById(obj_name);\n"
"        if (typeof obj == 'undefined')\n"
"            alert('Failed to get object ' + obj_name);\n"
"\n"
"        obj.style.visibility = \"hidden\";\n"
"        document.prev_tip = '';\n"
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
"    <font face=\"Verdana\">\n"
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
"</font>\n"
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
"      <td name=\"test_stats_total\">\n"
"        <b>Total</b>\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('total')\">x</a>\n"
#endif
"      </td>\n"
"      <td name=\"test_stats_passed_exp\">\n"
"        Passed expect\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('passed_exp')\">x</a>\n"
#endif
"      </td>\n"
"      <td name=\"test_stats_failed_exp\">\n"
"        Failed expect\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('failed_exp')\">x</a>\n"
#endif
"      </td>\n"
"      <td name=\"test_stats_passed_unexp\">\n"
"        Passed unexp\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('passed_unexp')\">x</a>\n"
#endif
"      </td>\n"
"      <td name=\"test_stats_failed_unexp\">\n"
"        Failed unexp\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('failed_unexp')\">x</a>\n"
#endif
"      </td>\n"
"      <td name=\"test_stats_aborted_new\">\n"
"        Aborted, New\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('aborted_new')\">x</a>\n"
#endif
"      </td>\n"
"      <td name=\"test_stats_not_run\">\n"
"        <b>Total</b>\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('not_run')\">x</a>\n"
#endif
"      </td>\n"
"      <td name=\"test_stats_skipped_exp\">\n"
"        Skipped expect\n"
#if TRC_USE_HIDDEN_STATS
"        <a href=\"javascript:statsHideColumn('skipped_exp')\">x</a>\n"
#endif
"      </td>\n"
"      <td name=\"test_stats_skipped_unexp\">\n"
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
"      <td class=\"test_stats_%s\" "
"name=\"test_stats_total\">\n"
       TRC_TESTS_STATS_FIELD_FMT
"      </td>\n"
"      <td class=\"test_stats_%s\" "
"name=\"test_stats_passed_exp\">\n"
       TRC_TESTS_STATS_FIELD_FMT
"      </td>\n"
"      <td class=\"test_stats_%s\" "
"name=\"test_stats_failed_exp\">\n"
       TRC_TESTS_STATS_FIELD_FMT
"      </td>\n"
"      <td class=\"test_stats_%s\" "
"name=\"test_stats_passed_unexp\">\n"
       TRC_TESTS_STATS_FIELD_FMT
"      </td>\n"
"      <td class=\"test_stats_%s\" "
"name=\"test_stats_failed_unexp\">\n"
       TRC_TESTS_STATS_FIELD_FMT
"      </td>\n"
"      <td class=\"test_stats_%s\" "
"name=\"test_stats_aborted_new\">\n"
       TRC_TESTS_STATS_FIELD_FMT
"      </td>\n"
"      <td class=\"test_stats_%s\" "
"name=\"test_stats_not_run\">\n"
       TRC_TESTS_STATS_FIELD_FMT
"      </td>\n"
"      <td class=\"test_stats_skipped_exp\" "
"name=\"test_stats_skip_exp\">\n"
"        %u\n"
"      </td>\n"
"      <td class=\"test_stats_skipped_unexp\" "
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
"<a href=\"%s/node_%d.html\" "
#if TRC_USE_STATS_POPUP
"onClick=\"showLog('%s', '%s/node_%d.html', event); return false;\""
#endif
">[log]</a>";
#endif

static const char * const trc_test_exp_got_row_tin_ref =
"        <a name=\"tin_%d\"> </a>\n";

static const char * const trc_test_exp_got_row_start =
"    <tr>\n"
"      <td valign=top>\n"
"        %s<b><a %s%s%s href=\"#OBJECTIVE%s\">%s</a></b>\n"
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
" </td>\n<td valign=top %s>\n";

static const char * const trc_test_exp_got_row_mid_pass_unexp =
" </td>\n<td %s valign=top %s>\n";

static const char * const trc_test_exp_got_row_mid_fail_unexp =
" </td>\n<td %s valign=top %s>\n";

static const char * const trc_test_exp_got_row_end =
"</td>\n"
"      <td valign=top>%s%s%s%s%s</td>\n"
"    </tr>\n";

static const char * const trc_test_params_hash = "<br/>Hash: %s<br/>";

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
"            test['type'] += stats['type'];\n"
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
"    updateStats('%s', {name:'%s', type:'%s', path:'%s', total:%d, "
"passed_exp:%d, failed_exp:%d, passed_unexp:%d, failed_unexp:%d, "
"aborted_new:%d, not_run:%d, skipped_exp:%d, skipped_unexp:%d});\n";

static const char * const trc_report_javascript_table_subtests =
"    test_stats['%s'].subtests = [%s];\n";
#endif

#define TRC_REPORT_KEY_UNSPEC   "unspecified"

typedef TAILQ_HEAD(, trc_report_key_entry) trc_keys;

typedef enum {
    TRC_REPORT_KEYS_TYPE_FAILURES,
    TRC_REPORT_KEYS_TYPE_SANITY,
    TRC_REPORT_KEYS_TYPE_EXPECTED,
    TRC_REPORT_KEYS_TYPE_UNEXPECTED,
    TRC_REPORT_KEYS_TYPE_MAX,
    TRC_REPORT_KEYS_TYPE_NONE,
} trc_report_keys_type;

static char *trc_report_keys_table_heading[] = {
    "Bugs that lead to known test failures.",
    "Bugs (prehaps fixed) that do not lead to expected test failures.",
    "Bugs specified for expected test behaviour.",
    "Bugs that do not lead to expected test behaviour.",
};

static int file_to_file(FILE *dst, FILE *src);

static te_bool
trc_report_test_iter_entry_output(
    const trc_test                   *test,
    const trc_report_test_iter_entry *iter,
    unsigned int                      flags);

static te_bool
trc_report_test_output(const trc_report_stats *stats,
                       unsigned int flags);

/**
 * Escape slashes in test path to use it in URL.
 *
 * @param path  Test path
 *
 * @return Escaped test path or NULL.
 */
static char *
escape_test_path(const char *path)
{
    size_t i;
    size_t l;
    char *p;

    if (path == NULL)
        return NULL;

    l = strlen(path);

    for (i = 0; i < strlen(path); i++)
        if (path[i] == '/')
            l += 2;

    p = calloc(l + 1, sizeof(char));

    if (p == NULL)
    {
        ERROR("%s(): cannot allocate memory for string",
              __FUNCTION__);
        return NULL;
    }
    
    l = 0;
    for (i = 0; i < strlen(path); i++)
    {
        if (path[i] != '/')
            p[l++] = path[i];
        else
        {
            p[l++] = '%';
            p[l++] = '2';
            p[l++] = 'F';
        }
    }

    return p;
}

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
                   const char *test_name, const char *key_test_path,
                   const char *test_path)
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
        if (strcmp(key_test_path, key_test->key_path) == 0)
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
    key_test->key_path = strdup(key_test_path);

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
                    const char *test_name, const char *key_test_path,
                    const char *test_path)
{
    int     count = 0;
    char   *p = NULL;

    if ((key_names == NULL) || (*key_names == '\0'))
    {
        trc_report_key_add(keys, TRC_REPORT_KEY_UNSPEC, iter_entry,
                           test_name, key_test_path, test_path);
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
                               test_name, key_test_path, test_path);
            free(tmp_key_name);
            key_names = p + 1;
            while (*key_names == ' ')
                key_names++;
        }
        else
        {
            trc_report_key_add(keys, key_names, iter_entry,
                               test_name, key_test_path, test_path);
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

trc_keys *
trc_keys_alloc()
{
    trc_keys *keys = calloc(1, sizeof(trc_keys));

    if (keys != NULL)
        TAILQ_INIT(keys);

    return keys;
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
            free(key_test->name);
            free(key_test->path);
            free(key_test->key_path);
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
                te_bool key_add = FALSE;
                te_bool is_exp = (iter_entry->is_exp);
                te_bool has_key =
                    (iter_data->exp_result->key != NULL);
                te_bool has_verdict =
                    (TAILQ_FIRST(&iter_entry->result.verdicts) != NULL);

                char *key_test_path =
                    trc_report_key_test_path(test_path,
                        iter_data->exp_result->key);

                if ((flags & TRC_REPORT_KEYS_SANITY) != 0)
                {
                    key_add = has_key && ((!is_exp) || has_verdict);
                }
                else if ((flags & TRC_REPORT_KEYS_EXPECTED) != 0)
                {
                    key_add = is_exp && (has_key || has_verdict);
                }
                else if ((flags & TRC_REPORT_KEYS_UNEXPECTED) != 0)
                {
                    key_add = (!is_exp);
                }
                else
                {
                    key_add =
                        (iter_entry->result.status == TE_TEST_FAILED) ||
                        ((iter_entry->result.status == TE_TEST_PASSED) &&
                         (TAILQ_FIRST(&iter_entry->result.verdicts) !=
                          NULL));
                }

                if (key_add)
                {
                    trc_report_keys_add(keys,
                                        iter_data->exp_result->key,
                                        iter_entry, test->name,
                                        key_test_path,
                                        test_path);
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

                    rc = te_string_append(&test_path, "/%s",
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
    if ((f_in == NULL) || (f_out == NULL))
    {
        ERROR("Failed to fdopen in/out pipes for te-trc-key tool");
        close(fd_in);
        close(fd_out);
        return te_rc_os2te(errno);
    }

#define FWRITE_FMT(args...) \
    do {                        \
        fprintf(f_in, args);    \
        VERB(args);             \
    } while (0)

    for (key = TAILQ_FIRST(keys);
         key != NULL;
         key = TAILQ_NEXT(key, links))
    {
         trc_report_key_test_entry *key_test = NULL;

         char *script =
            trc_re_key_substs_buf(TRC_RE_KEY_SCRIPT, key->name);
         FWRITE_FMT("%s:", script);
         free(script);

         TAILQ_FOREACH(key_test, &key->tests, links)
         {
             trc_report_key_iter_entry *key_iter = NULL;

             if (key_test != TAILQ_FIRST(&key->tests))
             {
                 FWRITE_FMT(",");
             }

             FWRITE_FMT("%s", key_test->name);
             if (!keys_only)
             {
                 FWRITE_FMT("#%s", key_test->key_path);
                 TAILQ_FOREACH(key_iter, &key_test->iters, links)
                 {
                     FWRITE_FMT("|%d", key_iter->iter->tin);
                 }
             }
             else
             {
                 FWRITE_FMT("&nbsp;[%d]", key_test->count);
             }
         }
         FWRITE_FMT("\n");
    }

#undef FWRITE_FMT

    fclose(f_in);
    close(fd_in);

    file_to_file(f, f_out);

    fclose(f_out);
    close(fd_out);

    fprintf(f, "<br/>");

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
    te_bool        is_exp = (iter == NULL) ? FALSE : iter->is_exp;
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
             (status != TE_TEST_PASSED) || (!is_exp)) &&
            (/* 
              * NO_EXPECTED is clear or obtained result is equal
              * to expected
              */
             (~flags & TRC_REPORT_NO_EXPECTED) || (!is_exp));
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
        return "not_run";

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
 * @param new_result    Whether result is new
 *
 * @return              Status code.
 */
static inline int
trc_report_result_anchor(FILE *f, const char *test_path,
                         const trc_report_test_iter_entry *iter,
                         te_bool new_result)
{
    fprintf(f, trc_test_exp_got_row_result_anchor, test_path,
            PRINT_STR(new_result ? "aborted_new" :
                        trc_report_result_to_string(iter)));

    return 0;
}
#endif

/**
 * Output TRC test result entry to HTML file.
 *
 * @param f             File stream to write
 * @param result        Expected result entry to output
 * @param test_type     Test type
 * @param test_path     Test path
 * @param stats         Test statistics
 * @param tin_id        Test iteration ID
 * @param flags         Current output flags
 *
 * @return Status code.
 */
static te_errno
trc_report_test_result_to_html(FILE *f, const te_test_result *result,
                               trc_test_type test_type,
                               const char *test_path,
                               const trc_report_stats *stats,
                               char *tin_id, unsigned int flags)
{
    te_errno                rc = 0;
    const te_test_verdict  *v;
    int                     v_id = -1;
    te_bool                 obtained_link;
    te_bool                 result_link;

    UNUSED(stats);
    UNUSED(test_path);
    UNUSED(flags);

    assert(f != NULL);

    result_link = (test_type == TRC_TEST_SCRIPT &&
                    (result == NULL || result->status == TE_TEST_PASSED ||
                     result->status == TE_TEST_FAILED));
#if 0
    result_link = (result_link &&
                   get_stats_by_status(stats, result->status) !=
                   TRC_STATS_RUN(stats));
#endif
    obtained_link = result_link && (tin_id[0] != '\0');

    WRITE_FILE("<span>");
    WRITE_FILE("Obtained result:");
#if TRC_USE_STATS_POPUP
    if (obtained_link)
    {
        WRITE_FILE("<a href=\"javascript:showIterResultTextWilds("
                   "'StatsTip', '%s', document.getElementById('%s'))\">",
                   test_path, tin_id);
        WRITE_FILE("[*]</a>\n");
    }
#endif
    WRITE_FILE("</span><br/>\n");

    WRITE_FILE("<span>");
    if (result == NULL)
    {
        WRITE_FILE(te_test_status_to_str(TE_TEST_UNSPEC));
        WRITE_FILE("</span>\n");
        return 0;
    }
    else
    {
        WRITE_FILE(te_test_status_to_str(result->status));
        WRITE_FILE("</span>\n");
    }

#if TRC_USE_STATS_POPUP
    if ((flags & TRC_REPORT_WILD_VERBOSE) && result_link)
    {
        WRITE_FILE("<a href=\"javascript:showIterResultWilds("
                   "'StatsTip', '%s', '%s')\">", test_path,
                   te_test_status_to_str(result->status));
        WRITE_FILE("[*]</a>\n");
    }
#endif

    if (TAILQ_EMPTY(&result->verdicts))
        return 0;

    WRITE_FILE("<br/><br/>\n");
    TAILQ_FOREACH(v, &result->verdicts, links)
    {
        v_id++;
        WRITE_FILE("<span id=\"%s_%d\">", tin_id, v_id);
        WRITE_FILE(v->str);
        WRITE_FILE("</span>\n");
#if TRC_USE_STATS_POPUP
        if ((flags & TRC_REPORT_WILD_VERBOSE) && obtained_link)
        {
            WRITE_FILE("<a href=\"javascript:showVerdictWilds("
                       "'StatsTip', '%s', getInnerText(document."
                       "getElementById('%s_%d')))\">", test_path,
                       tin_id, v_id);
            WRITE_FILE("[*]</a>");
        }
#endif
        WRITE_FILE("<br/>\n");
    }

cleanup:
    return rc;
}

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
    const trc_report_test_data       *test_data;
    const trc_report_stats           *stats;
    trc_report_test_iter_data        *iter_data;
    const trc_report_test_iter_entry *iter_entry;
    te_errno                          rc = 0;
    char                              tin_ref[128];
    char                              id_tin_id[128];
    char                              tin_id[128];
    char                             *escaped_path = NULL;

    assert(anchor != NULL);
    assert(test_path != NULL);

    iter = trc_db_walker_get_iter(walker);
    test = iter->parent;
    assert(test != NULL);
    test_data = trc_db_test_get_user_data(test, ctx->db_uid);
    stats = (test_data != NULL) ? &test_data->stats : NULL;
    iter_data = trc_db_walker_get_user_data(walker, ctx->db_uid);
    iter_entry = (iter_data == NULL) ? NULL : TAILQ_FIRST(&iter_data->runs);

    do {
        if (trc_report_test_iter_entry_output(test, iter_entry, flags))
        {
#if TRC_USE_LOG_URLS
            char *test_url = NULL;
            if ((iter_entry != NULL) && (iter_entry->tin >= 0) &&
                (ctx->html_logs_path != NULL))
            {
                test_url = te_sprintf(trc_test_log_url,
#if TRC_USE_STATS_POPUP
                                      ctx->html_logs_path,
                                      iter_entry->tin,
                                      test->name,
                                      ctx->html_logs_path,
                                      iter_entry->tin);
#else
                                      ctx->html_logs_path,
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
            id_tin_id[0] = '\0';
            tin_id[0] = '\0';
            if ((test->type == TRC_TEST_SCRIPT) &&
                (iter_entry != NULL) &&
                (iter_entry->tin >= 0))
            {
                if (~flags & TRC_REPORT_NO_KEYS)
                    sprintf(tin_ref, trc_test_exp_got_row_tin_ref,
                            iter_entry->tin);
                sprintf(tin_id, "tin_%d",
                        iter_entry->tin);
                sprintf(id_tin_id, "id=\"%s\"",
                        tin_id);
            }

            escaped_path = escape_test_path(test_path);
            if (escaped_path == NULL)
            {
                rc = TE_ENOMEM;
                break;
            }

            fprintf(f, trc_test_exp_got_row_start,
                    PRINT_STR(level_str),
                    PRINT_STR3(anchor, "name=\"", test_path, "\" "),
#if TRC_USE_LOG_URLS
                    escaped_path, test->name, PRINT_STR(test_url));
#else
                    escaped_path, test->name);
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

            if (iter_entry != NULL)
            {
                rc = trc_report_iter_args_to_html(f, iter_entry->args,
                                                  iter_entry->args_n, 40,
                                                  0);
                if (rc != 0)
                    break;
            }

#if TRC_USE_PARAMS_SPOILERS
            if (!TAILQ_EMPTY(&iter->args.head))
                WRITE_STR(trc_test_exp_got_row_params_end);
#endif

            if ((iter_entry != NULL) && (iter_entry->hash != NULL))
                fprintf(f, trc_test_params_hash, iter_entry->hash);

            WRITE_FILE(trc_test_exp_got_row_mid, "");

            rc = trc_exp_result_to_html(f, iter_data->exp_result,
                                        0, &ctx->tags);
            if (rc != 0)
                break;

            if (iter_entry == NULL || iter_entry->is_exp)
                WRITE_FILE(trc_test_exp_got_row_mid, id_tin_id);
            else if (iter_entry->result.status == TE_TEST_PASSED)
                WRITE_FILE(trc_test_exp_got_row_mid_pass_unexp,
                           !(flags & TRC_REPORT_NO_EXPECTED) ?
                           "class=\"test_stats_passed_unexp\"" :
                           "class=\"unexp_no_highligth\"",
                           id_tin_id);
            else
                WRITE_FILE(trc_test_exp_got_row_mid_fail_unexp,
                           !(flags & TRC_REPORT_NO_EXPECTED) ?
                           "class=\"test_stats_failed_unexp\"" :
                           "class=\"unexp_no_highlight\"",
                           id_tin_id);

#if TRC_USE_STATS_POPUP
            rc = trc_report_result_anchor(f, test_path, iter_entry,
                                          (iter_data->exp_result == NULL ||
                                           TAILQ_FIRST(&iter_data->
                                                         exp_result->
                                                          results) ==
                                                            NULL));
            if (rc != 0)
                break;
#endif

            free(escaped_path);
            rc = trc_report_test_result_to_html(f, (iter_entry == NULL) ?
                                                NULL : &iter_entry->result,
                                                test->type,
                                                test_path, stats, tin_id,
                                                flags);
            if (rc != 0)
                break;

            WRITE_FILE(trc_test_exp_got_row_mid, "");

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
                    if (iter_data->exp_result->key != NULL)
                    {
                        key_link =
                            trc_re_key_substs_buf(TRC_RE_KEY_TABLE_HREF,
                                iter_data->exp_result->key);
                    }
                    else if (((iter_entry->result.status ==
                               TE_TEST_PASSED) &&
                              (~flags &
                               TRC_REPORT_KEYS_SKIP_PASSED_UNSPEC)) ||
                             ((iter_entry->result.status ==
                               TE_TEST_FAILED) &&
                              (~flags &
                               TRC_REPORT_KEYS_SKIP_FAILED_UNSPEC)))
                    {
                        key_link =
                            trc_re_key_substs_buf(TRC_RE_KEY_TABLE_HREF,
                                                  TRC_REPORT_KEY_UNSPEC);

                    }
                    if (key_link != NULL)
                        fprintf(f, "%s", key_link);

                    free(key_link);
                }
            }

            fprintf(f, trc_test_exp_got_row_end,
                    PRINT_STR(test->notes),
                    PRINT_STR1((test->notes != NULL), "<br/>"),
                    PRINT_STR2((iter_data->exp_result != NULL),
                               iter_data->exp_result->notes, "<br/>"),
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
    char                       *escaped_path;
    te_bool                     package_output;

    escaped_path = escape_test_path(test_path);
    if (escaped_path == NULL && test_path != NULL)
    {
        ERROR("%s(): cannot allocate memory for string",
              __FUNCTION__);
        return TE_ENOMEM;
    }

    test = trc_db_walker_get_test(walker);
    test_data = trc_db_walker_get_user_data(walker, ctx->db_uid);
    assert(test_data != NULL);
    stats = &test_data->stats;
    package_output = trc_report_test_output(stats,
                        (flags & ~TRC_REPORT_NO_SCRIPTS));

    if (((test->type == TRC_TEST_PACKAGE) &&
         (flags & TRC_REPORT_NO_SCRIPTS)) ||
        trc_report_test_output(stats, flags))
    {
        te_bool     name_link;
        const char *keys = NULL;

        if (TRC_STATS_RUN(stats) == 0)
            return 0;

        /* test_iters_check_output_and_get_keys(test, flags); */

        name_link = (((flags & TRC_REPORT_NO_SCRIPTS) && package_output) ||
                     ((~flags & TRC_REPORT_NO_SCRIPTS) &&
                     (test->type == TRC_TEST_SCRIPT)));

#if TRC_USE_STATS_POPUP
#define TRC_TEST_COND(field_) \
    ((test->type == TRC_TEST_PACKAGE && package_output) ||  \
    (test->type == TRC_TEST_SCRIPT &&                       \
     (strcmp(field_, "passed_exp") == 0 ||                  \
      strcmp(field_, "failed_exp") == 0 ||                  \
      strcmp(field_, "passed_unexp") == 0 ||                \
      strcmp(field_, "failed_unexp") == 0))) 

#define TRC_STATS_FIELD_POPUP(field_, value_, expr_) \
    ((value_) > 0) ? field_ : "zero",                   \
    PRINT_STR5(TRC_TEST_COND(field_) &&                 \
               ((value_) > 0) && (expr_),               \
               (test->type == TRC_TEST_PACKAGE) ?       \
                    TRC_STATS_SHOW_HREF_START :         \
                    TRC_TEST_STATS_SHOW_HREF_START,     \
               test_path,                               \
               TRC_STATS_SHOW_HREF_CLOSE_PARAM_PREFIX,  \
               (field_),                                \
               TRC_STATS_SHOW_HREF_CLOSE),              \
    (value_),                                           \
    PRINT_STR1(TRC_TEST_COND(field_) &&                 \
               ((value_) > 0) && (expr_),               \
               TRC_STATS_SHOW_HREF_END)
#else
#define TRC_STATS_FIELD_POPUP(field_,value_,expr_) (field_), (value_)
#endif

#define ZERO_PASS_EXP \
    (stats->pass_exp == 0 || \
     (flags & TRC_REPORT_NO_EXP_PASSED) || \
     (flags & TRC_REPORT_NO_EXPECTED))

#define ZERO_FAIL_EXP \
    (stats->fail_exp == 0 || \
     (flags & TRC_REPORT_NO_EXPECTED))

#define FAIL_PASS_COUNT \
     (ZERO_PASS_EXP ? 0 : stats->pass_exp) + \
     (ZERO_FAIL_EXP ? 0 : stats->fail_exp) + \
     stats->pass_une + stats->fail_une
    

        fprintf(f, trc_tests_stats_row,
                PRINT_STR(level_str),
                name_link ? "href" : "name",
                name_link ? "#" : "",
                name_link ? escaped_path : test_path,
                test->name,

#if TRC_USE_STATS_POPUP
                PRINT_STR3((test->type == TRC_TEST_PACKAGE) &&
                           (flags & TRC_REPORT_NO_SCRIPTS) &&
                           (TRC_STATS_RUN(stats) > 0) && package_output,
                           TRC_STATS_SHOW_HREF_START,
                           escaped_path,
                           TRC_STATS_SHOW_HREF_CLOSE
                           "[...]"
                           TRC_STATS_SHOW_HREF_END),
#endif
                test_path != NULL ? "<a name=\"OBJECTIVE" : "",
                PRINT_STR(test_path),
                test_path != NULL ? "\">": "",
                PRINT_STR(test->objective),
                test_path != NULL ? "</a>": "",
                TRC_STATS_FIELD_POPUP("total", TRC_STATS_RUN(stats),
                                      package_output),
                TRC_STATS_FIELD_POPUP("passed_exp", stats->pass_exp,
                                      !(test->type == TRC_TEST_SCRIPT &&
                                        stats->pass_exp ==
                                            FAIL_PASS_COUNT) &&
                                      !ZERO_PASS_EXP),
                TRC_STATS_FIELD_POPUP("failed_exp", stats->fail_exp, 
                                      !(test->type == TRC_TEST_SCRIPT &&
                                        stats->fail_exp ==
                                            FAIL_PASS_COUNT) &&
                                      !ZERO_FAIL_EXP),
                TRC_STATS_FIELD_POPUP("passed_unexp",
                                      stats->pass_une,
                                      !(test->type == TRC_TEST_SCRIPT &&
                                        stats->pass_une ==
                                            FAIL_PASS_COUNT)),
                TRC_STATS_FIELD_POPUP("failed_unexp",
                                      stats->fail_une,
                                      !(test->type == TRC_TEST_SCRIPT &&
                                        stats->fail_une ==
                                            FAIL_PASS_COUNT)),
                TRC_STATS_FIELD_POPUP("aborted_new",
                                      stats->aborted +
                                      stats->new_run, TRUE),
                TRC_STATS_FIELD_POPUP("not_run", TRC_STATS_NOT_RUN(stats),
                                      TRC_STATS_RUN(stats) > 0),
                stats->skip_exp, stats->skip_une,
                PRINT_STR(keys), PRINT_STR(test->notes));
#undef TRC_STATS_FIELD_POPUP
#undef TRC_TEST_COND
#undef ZERO_PASS_EXP
#undef ZERO_FAIL_EXP
#undef FAIL_PASS_COUNT
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
                                  const char             *test_type,
                                  const char             *test_path,
                                  const trc_report_stats *stats)
{
    te_errno rc = 0;

    if ((test_name == NULL) || (test_path == NULL) || (stats == NULL))
        return 0;

    fprintf(f, trc_report_javascript_table_row,
            test_path, test_name, test_type, test_path,
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
        char *test_type = (test->type == TRC_TEST_SCRIPT) ?
                          "script" : (test->type == TRC_TEST_PACKAGE) ?
                          "package" : (test->type == TRC_TEST_SESSION) ?
                          "session" : "unknown";
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

                    rc = te_string_append(&test_path, "/%s",
                                          last_test_name);
                    if (rc != 0)
                        break;

                    if (!trc_report_test_output(stats, flags))
                        break;

                    rc = te_string_append(&subtests[level - 1], "'%s',",
                                          test_path.ptr);
                    if (rc != 0)
                        break;

                    rc = trc_report_javascript_table_entry(f, test->name,
                                                           test_type,
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
                        rc = te_string_append(&level_str, "*/");
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

                    rc = te_string_append(&test_path, "/%s",
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
                    te_string_cut(&level_str, strlen("*/"));
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

static unsigned int
trc_report_key_type_to_flags(int keys_type)
{
    switch (keys_type)
    {
        case TRC_REPORT_KEYS_TYPE_FAILURES:
            return TRC_REPORT_KEYS_FAILURES;
        case TRC_REPORT_KEYS_TYPE_SANITY:
            return TRC_REPORT_KEYS_SANITY;
        case TRC_REPORT_KEYS_TYPE_EXPECTED:
            return TRC_REPORT_KEYS_EXPECTED;
        case TRC_REPORT_KEYS_TYPE_UNEXPECTED:
            return TRC_REPORT_KEYS_UNEXPECTED;
        case TRC_REPORT_KEYS_TYPE_NONE:
            return TRC_REPORT_NO_KEYS;
        default:
            return 0;
    }
}

/** See the description in trc_report.h */
te_errno
trc_report_to_html(trc_report_ctx *gctx, const char *filename,
                   const char *title, FILE *header,
                   unsigned int flags)
{
    FILE       *f;
    te_errno    rc = 0;
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

    /* XXX */
    if (~flags & TRC_REPORT_NO_KEYS)
    {
        int keys_type;

        WRITE_STR("<a name=\"keys_table\"> </a>\n");

        for (keys_type = TRC_REPORT_KEYS_TYPE_FAILURES;
             keys_type < TRC_REPORT_KEYS_TYPE_MAX; keys_type++)
        {
            trc_keys *keys = NULL;

            if ((~flags & trc_report_key_type_to_flags(keys_type)) != 0)
                continue;

            if ((keys = trc_keys_alloc()) == NULL)
            {
                rc = te_rc_os2te(errno);
                goto cleanup;
            }

            rc = trc_report_keys_collect(keys, gctx,
                     (flags & (~TRC_REPORT_KEYS_MASK)) |
                     trc_report_key_type_to_flags(keys_type));
            if (rc != 0)
                goto cleanup;

            trc_report_keys_to_html(f, gctx, "te-trc-key", keys,
                !(~flags & TRC_REPORT_KEYS_ONLY),
                trc_report_keys_table_heading[keys_type]);
        }
    }

    WRITE_STR("<a href=\"#start\"><b>Up</b></a>\n");

    if (gctx->show_cmd_file != NULL)
    {
        FILE *f_args;

        if ((f_args = fopen(gctx->show_cmd_file, "r")) == 0)
        {
            ERROR("Failed to open file '%s'", gctx->show_cmd_file);
            goto cleanup;
        }

        fprintf(f, "<br/><br/>Report is generated by command:<br/>"
                "<p><code>\n");

        file_to_file(f, f_args);

        fclose(f_args);

        fprintf(f, "\n</code></p></span>\n");
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

/** See the description in trc_report.h */
te_errno
trc_report_to_perl(trc_report_ctx *gctx, const char *filename)
{
    FILE                  *f;
    te_errno               rc = 0;
    trc_keys              *keys = NULL;
    trc_report_key_entry  *key;
    tqe_string            *tag;
    tqh_strings            unexp_tests;
    tqe_string            *tqe_str;

    f = fopen(filename, "w");
    if (f == NULL)
    {
        rc = te_rc_os2te(errno);
        ERROR("Failed to open file to write Perl report to: %r", rc);
        return rc;
    }

    fprintf(f, "@tags = (\n");
    TAILQ_FOREACH(tag, &gctx->tags, links)
        fprintf(f, "  '%s',\n", tag->v);
    fprintf(f, ");\n");

    if ((keys = trc_keys_alloc()) == NULL)
    {
        rc = te_rc_os2te(errno);
        goto cleanup;
    }

    rc = trc_report_keys_collect(keys, gctx,
                                 TRC_REPORT_KEYS_EXPECTED);
    if (rc != 0)
        goto cleanup;

    fprintf(f, "\n%%keys = (\n");
    for (key = TAILQ_FIRST(keys);
         key != NULL;
         key = TAILQ_NEXT(key, links))
    {
        trc_report_key_test_entry *key_test = NULL;

        if (strcmp(key->name, TRC_REPORT_KEY_UNSPEC) == 0)
            continue;

        fprintf(f, "  '%s' => [\n", key->name);

        TAILQ_FOREACH(key_test, &key->tests, links)
        {
            fprintf(f, "    '%s',\n", key_test->path);
        }

        fprintf(f, "  ],\n", key->name);
    }
    fprintf(f, ");\n");

    trc_keys_free(keys);
    if ((keys = trc_keys_alloc()) == NULL)
    {
        rc = te_rc_os2te(errno);
        goto cleanup;
    }

    rc = trc_report_keys_collect(keys, gctx,
                                 TRC_REPORT_KEYS_UNEXPECTED);
    if (rc != 0)
        goto cleanup;

    TAILQ_INIT(&unexp_tests);
    fprintf(f, "\n@unexp_results_tests = (\n");
    for (key = TAILQ_FIRST(keys);
         key != NULL;
         key = TAILQ_NEXT(key, links))
    {
        trc_report_key_test_entry *key_test = NULL;

        TAILQ_FOREACH(key_test, &key->tests, links)
        {
            int found = 0;

            TAILQ_FOREACH(tqe_str, &unexp_tests, links)
            {
              if (strcmp(tqe_str->v, key_test->path) == 0)
              {
                  found = 1;
                  break;
              }
            }

            if (found == 0)
            {
                fprintf(f, "  '%s',\n", key_test->path);
                tqe_str = calloc(1, sizeof(*tqe_str));
                tqe_str->v = strdup(key_test->path);
                TAILQ_INSERT_TAIL(&unexp_tests, tqe_str, links);
            }
        }
    }
    fprintf(f, ");\n");
    tq_strings_free(&unexp_tests, free);

cleanup:
    trc_keys_free(keys);
    fclose(f);
    if (rc != 0)
        unlink(filename);
    return rc;
}

