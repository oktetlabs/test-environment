/** @file
 * @brief Testing Results Comparator: report tool
 *
 * Definition of XML log parsing common types and routines.
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
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LOG_PARSE_H__
#define __TE_LOG_PARSE_H_

#ifdef __cplusplus
extern "C" {
#endif

/** TRC XML log parsing options */
enum trc_log_parse_flags {
    TRC_LOG_PARSE_IGNORE_LOG_TAGS  = 0x1,   /**< Ignore TRC tags extracted
                                                 from the log */
    TRC_LOG_PARSE_FAKE_LOG         = 0x2,   /**< Parse log of fake Tester
                                                 run */
    TRC_LOG_PARSE_MERGE_LOG        = 0x4,   /**< Merge iterations from log
                                                 into TRC DB performing
                                                 TRC update */
    TRC_LOG_PARSE_UNSTABLE_NEW     = 0x8,   /**< If the same iteration
                                                 produces different results
                                                 for a given set of tags in
                                                 logs to be merged,
                                                 merge these results into
                                                 "unstable" one */
    TRC_LOG_PARSE_UNSTABLE_OLD     = 0x10,  /**< Do not replace results
                                                 previously stored in
                                                 TRC with new ones,
                                                 but merge them into
                                                 "unstable" results" */
    TRC_LOG_PARSE_RULES_ALL        = 0x20,  /**< Generate updating rules
                                                 for all possible results
                                                 (not only those for
                                                  which there are new
                                                  results in logs) */
    TRC_LOG_PARSE_USE_RULE_IDS     = 0x40,  /**< Insert updating rule ID in
                                                 user_attr attribute of test
                                                 iterations in generated TRC
                                                 to simplify applying of
                                                 edited rules */
    TRC_LOG_PARSE_NO_GEN_WILDS     = 0x80,  /**< Do not replace test
                                                 iterations with wildcards
                                                 in generated TRC */
    TRC_LOG_PARSE_LOG_WILDS        = 0x100, /**< Generate wildcards for
                                                 results from logs, not
                                                 from TRC DB */
};

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LOG_PARSE_H__ */
