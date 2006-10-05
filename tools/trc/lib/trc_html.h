/** @file
 * @brief Testing Results Comparator
 *
 * Helper functions to prepare HTML reports.
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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

#ifndef __TE_TRC_HTML_H__
#define __TE_TRC_HTML_H__

#include <stdio.h>

#include "te_errno.h"
#include "te_trc.h"
#include "trc_db.h"

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
extern te_errno trc_test_result_to_html(FILE                 *f,
                                        const te_test_result *result);

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
                                       unsigned int          flags);

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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TRC_HTML_H__ */
