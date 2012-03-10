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
    TRC_LOG_PARSE_IGNORE_LOG_TAGS = (1 << 0),  /**< Ignore TRC tags
                                                    extracted
                                                    from the log */
    TRC_LOG_PARSE_FAKE_LOG        = (1 << 1),  /**< Parse log of fake
                                                    Tester run */
    TRC_LOG_PARSE_MERGE_LOG       = (1 << 2),  /**< Merge iterations from
                                                    log into TRC DB
                                                    performing TRC
                                                    update */
    TRC_LOG_PARSE_RULES_ALL       = (1 << 3),  /**< Generate updating rules
                                                    for all possible results
                                                    (not only those for
                                                    which there are new
                                                    results in logs) */
    TRC_LOG_PARSE_USE_RULE_IDS    = (1 << 4),  /**< Insert updating rule
                                                    ID in user_attr
                                                    attribute of test
                                                    iterations in
                                                    generated TRC
                                                    to simplify applying
                                                    of edited rules */
    TRC_LOG_PARSE_NO_GEN_WILDS    = (1 << 5),  /**< Do not replace test
                                                    iterations with
                                                    wildcards in
                                                    generated TRC */
    TRC_LOG_PARSE_LOG_WILDS       = (1 << 6),  /**< Generate wildcards for
                                                    results from logs, not
                                                    from TRC DB */
    TRC_LOG_PARSE_LOG_WILDS_NEXP  = (1 << 7),  /**< Generate wildcards for
                                                    unexpected results from
                                                    logs only */
    TRC_LOG_PARSE_COPY_OLD        = (1 << 8),  /**< Copy results from
                                                    current TRC DB in <new>
                                                    section of updating
                                                    rule */
    TRC_LOG_PARSE_COPY_CONFLS     = (1 << 9),  /**< Copy conflicting results
                                                    from logs in <news>
                                                    section of updating
                                                    rule */
    TRC_LOG_PARSE_COPY_OLD_FIRST  = (1 << 10), /**< If both COPY_CONFLS and
                                                    COPY_OLD are specified,
                                                    copy expected results
                                                    from current TRC DB in
                                                    <new> section of
                                                    updating rule; if there
                                                    is no such results -
                                                    copy conlficting
                                                    results from logs to
                                                    the same place */
    TRC_LOG_PARSE_CONFLS_ALL      = (1 << 11), /**< Treat all results from
                                                    logs as unexpected
                                                    ones */
    TRC_LOG_PARSE_COPY_BOTH       = (1 << 12), /**< If both COPY_CONFLS and
                                                    COPY_BOTH are specified,
                                                    copy both results from
                                                    existing TRC and
                                                    conflicting results from
                                                    logs in <new> section
                                                    of updating rule. If
                                                    COPY_OLD_FIRST
                                                    is specified too, copy
                                                    results from existing
                                                    TRC firstly, otherwise
                                                    firstly copy results
                                                    from logs */
    TRC_LOG_PARSE_TAGS_STR        = (1 << 13), /**< Do not change string
                                                    representation of
                                                    tags */
    TRC_LOG_PARSE_GEN_APPLY       = (1 << 14), /**< Apply updating rules
                                                    after generating
                                                    them */
    TRC_LOG_PARSE_RULES_CONFL     = (1 << 15), /**< If applying of a rule
                                                    leads to replacing
                                                    some alredy existing
                                                    expected results with
                                                    different ones,
                                                    do not replace them
                                                    but treat results
                                                    from '<new>' section
                                                    of rule as conflicting
                                                    results from logs */
};

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LOG_PARSE_H__ */
