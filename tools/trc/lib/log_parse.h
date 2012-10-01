/** @file
 * @brief Testing Results Comparator: tools
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
    TRC_LOG_PARSE_IGNORE_LOG_TAGS
                               = (1LLU << 0),  /**< Ignore TRC tags
                                                    extracted
                                                    from the log */
    TRC_LOG_PARSE_FAKE_LOG     = (1LLU << 1),  /**< Parse log of fake
                                                    Tester run */
    TRC_LOG_PARSE_MERGE_LOG    = (1LLU << 2),  /**< Merge iterations from
                                                    log into TRC DB
                                                    performing TRC
                                                    update */
    TRC_LOG_PARSE_RULES_ALL    = (1LLU << 3),  /**< Generate updating rules
                                                    for all possible results
                                                    (not only those for
                                                    which there are new
                                                    results in logs) */
    TRC_LOG_PARSE_USE_RULE_IDS = (1LLU << 4),  /**< Insert updating rule
                                                    ID in user_attr
                                                    attribute of test
                                                    iterations in
                                                    generated TRC
                                                    to simplify applying
                                                    of edited rules */
    TRC_LOG_PARSE_NO_GEN_WILDS = (1LLU << 5),  /**< Do not replace test
                                                    iterations with
                                                    wildcards in
                                                    generated TRC */
    TRC_LOG_PARSE_LOG_WILDS    = (1LLU << 6),  /**< Generate wildcards for
                                                    results from logs, not
                                                    from TRC DB */
    TRC_LOG_PARSE_LOG_WILDS_UNEXP
                               = (1LLU << 7),  /**< Generate wildcards for
                                                    unexpected results from
                                                    logs only */
    TRC_LOG_PARSE_COPY_OLD     = (1LLU << 8),  /**< Copy results from
                                                    current TRC DB in <new>
                                                    section of updating
                                                    rule */
    TRC_LOG_PARSE_COPY_CONFLS  = (1LLU << 9),  /**< Copy conflicting results
                                                    from logs in <news>
                                                    section of updating
                                                    rule */
    TRC_LOG_PARSE_COPY_OLD_FIRST
                               = (1LLU << 10), /**< If both COPY_CONFLS and
                                                    COPY_OLD are specified,
                                                    copy expected results
                                                    from current TRC DB in
                                                    <new> section of
                                                    updating rule; if there
                                                    is no such results -
                                                    copy conlficting
                                                    results from logs to
                                                    the same place */
    TRC_LOG_PARSE_CONFLS_ALL   = (1LLU << 11), /**< Treat all results from
                                                    logs as unexpected
                                                    ones */
    TRC_LOG_PARSE_COPY_BOTH    = (1LLU << 12), /**< If both COPY_CONFLS and
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
    TRC_LOG_PARSE_TAGS_STR     = (1LLU << 13), /**< Do not change string
                                                    representation of
                                                    tags */
    TRC_LOG_PARSE_GEN_APPLY    = (1LLU << 14), /**< Apply updating rules
                                                    after generating
                                                    them */
    TRC_LOG_PARSE_RULES_CONFL  = (1LLU << 15), /**< If applying of a rule
                                                    leads to replacing
                                                    some alredy existing
                                                    expected results with
                                                    different ones,
                                                    do not replace them
                                                    but treat results
                                                    from '<new>' section
                                                    of rule as conflicting
                                                    results from logs */
    TRC_LOG_PARSE_RRESULTS    = (1LLU << 16),  /**< Generate updating
                                                    rules of type @c
                                                    TRC_UPDATE_RRESULTS */
    TRC_LOG_PARSE_RRESULT     = (1LLU << 17),  /**< Generate updating
                                                    rules of type @c
                                                    TRC_UPDATE_RRESULT */
    TRC_LOG_PARSE_RRENTRY     = (1LLU << 18),  /**< Generate updating
                                                    rules of type @c
                                                    TRC_UPDATE_RRENTRY */
    TRC_LOG_PARSE_RVERDICT    = (1LLU << 19),  /**< Generate updating
                                                    rules of type @c
                                                    TRC_UPDATE_RVERDICT */
    TRC_LOG_PARSE_PATHS       = (1LLU << 20),  /**< Print paths of all
                                                    scripts encountered in
                                                    parsed logs */
    TRC_LOG_PARSE_NO_PE       = (1LLU << 21),  /**< Do not take into
                                                    consideration prologues
                                                    and epilogues */
    TRC_LOG_PARSE_RULE_UPD_ONLY
                              = (1LLU << 22),  /**< Save only tests for
                                                    which iterations at
                                                    least one rule was
                                                    applied */
    TRC_LOG_PARSE_SKIPPED     = (1LLU << 23),  /**< Show skipped unexpected
                                                    results */
    TRC_LOG_PARSE_NO_SKIP_ONLY
                              = (1LLU << 24),  /**< Do not create rules with
                                                    <conflicts/> containing
                                                    skipped only results */
    TRC_LOG_PARSE_NO_EXP_ONLY = (1LLU << 25),  /**< Do not create rules with
                                                    <conflicts/> containing
                                                    expected only results
                                                    if CONFLS_ALL is turned
                                                    on */
    TRC_LOG_PARSE_SELF_CONFL  = (1LLU << 26),  /**< Get conflicting results
                                                    from expected results of
                                                    an iteration found with
                                                    help of matching
                                                    function */
    TRC_LOG_PARSE_GEN_TAGS    = (1LLU << 27),  /**< Generate tags for
                                                    logs */
    TRC_LOG_PARSE_EXT_WILDS   = (1LLU << 28),  /**< Specify a value for
                                                    each argument in
                                                    wildcard where it is
                                                    possible for a given
                                                    wildcard */
    TRC_LOG_PARSE_SIMPL_TAGS  = (1LLU << 29),  /**< Simplify tag
                                                    expressions in lists
                                                    of unexpected results
                                                    from logs */
    TRC_LOG_PARSE_DIFF        = (1LLU << 30),  /**< Show results from the
                                                    second group of logs
                                                    which were not
                                                    presented in the first
                                                    group of logs */
    TRC_LOG_PARSE_DIFF_NO_TAGS
                              = (1LLU << 31),  /**< Show results from the
                                                    second group of logs
                                                    which were not
                                                    presented in the first
                                                    group of logs -
                                                    not taking into account
                                                    tag expressions */
    TRC_LOG_PARSE_INTERSEC_WILDS
                              = (1LLU << 32),  /**< It's allowed for
                                                    iteration to have more
                                                    than one wildcard
                                                    describing it */
    TRC_LOG_PARSE_NO_GEN_FSS  = (1LLU << 33),  /**< Do not try to
                                                    find out subsets
                                                    corresponding to
                                                    every possible
                                                    iteration record, do
                                                    not use algorithms
                                                    based on it */
    TRC_LOG_PARSE_FSS_UNLIM   = (1LLU << 34),  /**< Do not resrict amount
                                                    of time used to
                                                    find out subsets for
                                                    every possible
                                                    iteration record */
    TRC_LOG_PARSE_NO_R_FAIL   = (1LLU << 35),  /**< Do not consider
                                                    results of kind
                                                    "FAILED without
                                                     verdicts" */
    TRC_LOG_PARSE_NO_INCOMPL  = (1LLU << 36),  /**< Do not consider
                                                    INCOMPLETE
                                                    results */
    TRC_LOG_PARSE_NO_INT_ERR  = (1LLU << 37),  /**< Do not consider
                                                    results with
                                                    internal error */
    TRC_LOG_PARSE_MATCH_LOGS  = (1LLU << 38),  /**< Use user
                                                    matching
                                                    function to filter
                                                    iterations from
                                                    testing logs */
    TRC_LOG_PARSE_FILT_LOG    = (1LLU << 39),  /**< Log to be used
                                                    for filtering out
                                                    iterations not
                                                    appearing in it */
};

/** All rule type flags */
#define TRC_LOG_PARSE_RTYPES \
    (TRC_LOG_PARSE_RRESULTS | TRC_LOG_PARSE_RRESULT | \
     TRC_LOG_PARSE_RRENTRY | TRC_LOG_PARSE_RVERDICT)

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LOG_PARSE_H__ */
