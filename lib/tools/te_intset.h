/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Generic API to operate on integral sets.
 *
 * @defgroup te_tools_te_intset Integral sets
 * @ingroup te_tools
 * @{
 *
 * Generic functions to operate on integral sets.
 *
 * @note The functions in this module use dynamic dispatching,
 * so they may not be suitable for use in really performance-critical
 * cases.
 */

#ifndef __TE_INTSET_H__
#define __TE_INTSET_H__

#include "te_defs.h"
#include "te_errno.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A description of a specific intset type.
 */
typedef struct te_intset_ops {
    /** Method to remove all elements from the set @p val. */
    void (*clear)(void *val);
    /** Method to add an integer @p v to the set @p val. */
    void (*set)(int v, void *val);
    /** Method to check whether @p v is in @p val. */
    te_bool (*check)(int v, const void *val);
} te_intset_ops;

/**
 * Convert a string to an integral set.
 *
 * The string @p str is a comma-separated list of single numbers and
 * ranges, e.g. @c 1,2-10,100.
 *
 * The set @p val will be cleared before parsing.
 *
 * @param[in]  ops    intset type description
 * @param[in]  minval minimum possible value in a set
 * @param[in]  maxval maximum possible value in a set
 * @param[in]  str    string to parse
 * @param[out] val    target value
 *
 * @return status code
 */
extern te_errno te_intset_generic_parse(const te_intset_ops *ops,
                                        int minval, int maxval,
                                        const char *str, void *val);

/**
 * Convert an integral set to a string.
 *
 * The resulting string may be passed to te_intset_generic_parse()
 * yielding the original set. Sequences of consecutive numbers are
 * represented as ranges @c N-M.
 *
 * @param ops    intset type description
 * @param minval minimum possible value in a set
 * @param maxval maximum possible value in a set
 * @param val    intset
 *
 * @return a string representation or @c NULL in case of an error
 *         (it should be free()'d)
 *
 * @note @p maxval should be less than @c INT_MAX and in general
 * the function is not very efficient if @p maxval is too high.
 */
extern char *te_intset_generic2string(const te_intset_ops *ops,
                                      int minval, int maxval,
                                      const void *val);

/**
 * Tests whether @p subset is really a subset of @p superset.
 *
 * @param ops      intset type description
 * @param minval   minimum possible value in a set
 * @param maxval   maximum possible value in a set
 * @param subset   a subset to test
 * @param superset its superset
 *
 * @return @c TRUE iff @p subset is a subset of @p superset
 */
extern te_bool te_intset_generic_is_subset(const te_intset_ops *ops,
                                           int minval, int maxval,
                                           const void *subset,
                                           const void *superset);

/** A description of 64-bit integer as a bit set */
extern const te_intset_ops te_bits_intset;

/**
 * Convert a string to a 64-bit integer treated as a bit set.
 *
 * The string @p str is a comma-separated list of single numbers and
 * ranges, e.g. @c 1,2-10,63.
 *
 * @param[in]  str    input string
 * @param[out] val    resulting value
 *
 * @return status code
 *
 * @sa te_intset_generic_parse
 */
static inline te_errno
te_bits_parse(const char *str, uint64_t *val)
{
    return te_intset_generic_parse(&te_bits_intset,
                                   0, sizeof(uint64_t) * CHAR_BIT - 1,
                                   str, val);
}

/**
 * Convert a 64-bit integer treated as a bit set to a string.
 *
 * Sequences of consecutive bits are represented as ranges @c N-M.
 *
 * @param val input integer
 *
 * @return a string representation or @c NULL in case of an error
 *         (it should be free()'d)
 *
 * @sa te_intset_generic2string
 */
static inline char *
te_bits2string(uint64_t val)
{
    return te_intset_generic2string(&te_bits_intset,
                                    0, sizeof(uint64_t) * CHAR_BIT - 1,
                                    &val);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_INTSET_H__ */
/**@} <!-- END te_tools_te_intset --> */
