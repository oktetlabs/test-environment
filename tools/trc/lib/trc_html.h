/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Testing Results Comparator
 *
 * Helper functions to prepare HTML reports.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TRC_HTML_H__
#define __TE_TRC_HTML_H__

#include <stdio.h>

#include "te_errno.h"
#include "te_trc.h"
#include "trc_db.h"
#include "tq_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Output TE test result to HTML file.
 *
 * @param f             File stream to write
 * @param result        Result to output
 *
 * @return Status code.
 */
extern te_errno te_test_result_to_html(FILE                 *f,
                                       const te_test_result *result);

/**
 * Output TRC test expected result entry to HTML file.
 *
 * @param f             File stream to write
 * @param result        Expected result entry to output
 *
 * @return Status code.
 */
extern te_errno trc_test_result_to_html(FILE                       *f,
                                        const trc_exp_result_entry *result);

/**
 * Output expected result to HTML file.
 *
 * @param f             File stream to write
 * @param result        Expected result to output
 * @param flags         Processing flags
 *
 * @return Status code.
 */
extern te_errno trc_exp_result_to_html(FILE                 *f,
                                       const trc_exp_result *result,
                                       unsigned int          flags,
                                       tqh_strings          *tags);

/**
 * Output test iteration arguments to HTML file.
 *
 * @param f             File stream to write
 * @param args          Arguments
 * @param flags         Processing flags
 *
 * @return Status code.
 */
extern te_errno trc_test_iter_args_to_html(FILE                     *f,
                                           const trc_test_iter_args *args,
                                           unsigned int              flags);

/**
 * Output test iteration arguments obtained from log to HTML file.
 *
 * @param f             File stream to write
 * @param args          Arguments
 * @param n_args        Number of arguments
 * @param max_len       Maximum length of string (to split too long lines)
 * @param flags         Processing flags
 *
 * @return Status code.
 */
extern te_errno trc_report_iter_args_to_html(FILE                  *f,
                                             const
                                               trc_report_argument *args,
                                             unsigned int           args_n,
                                             unsigned int           max_len,
                                             unsigned int           flags);

/**
 * Include code from a file to the generated HTML.
 *
 * @param f       File in which to append included code.
 * @param src     From were to include the code.
 *
 * @return Status code.
 */
extern te_errno trc_include_external_html(FILE *f, const char *src);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_HTML_H__ */
