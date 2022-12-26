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

#include "te_config.h"

#include "te_defs.h"
#include "te_errno.h"
#include <inttypes.h>
#if HAVE_SCHED_H
#include <sched.h>
#endif

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
    /** Method to remove an integer @p v from the set @p val. */
    void (*unset)(int v, void *val);
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

/**
 * Add a contiguous range of integers to the set @p val.
 *
 * @param ops     intset type description
 * @param val     intset
 * @param first   start of the range
 * @param last    end of the range
 */
extern void te_intset_generic_add_range(const te_intset_ops *ops,
                                        void *val, int first, int last);

/**
 * Remove a contiguous range of integers from the set @p val.
 *
 * @param ops     intset type description
 * @param val     intset
 * @param first   start of the range
 * @param last    end of the range
 */
extern void te_intset_generic_remove_range(const te_intset_ops *ops,
                                           void *val, int first, int last);

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
                                   0, TE_BITSIZE(uint64_t) - 1,
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
                                    0, TE_BITSIZE(uint64_t) - 1,
                                    &val);
}

/**
 * Character set or class.
 */
typedef struct te_charset {
    /** Bitmask of characters in the set */
    uint64_t items[(UINT8_MAX + 1) / TE_BITSIZE(uint64_t)];
    /** Number of character in the set */
    unsigned int n_items;
} te_charset;

/** Static initialiser for te_charset. */
#define TE_CHARSET_INIT {{0, 0, 0, 0}, 0}

/** Description of a te_charset as an intset. */
extern const te_intset_ops te_charset_intset;

/** Clear the character set @p cset. */
static inline void
te_charset_clear(te_charset *cset)
{
    te_charset_intset.clear(cset);
}

/**
 * Add a contiguous range of characters to a charset.
 *
 * @param cset      character set
 * @param minbyte   starting character
 * @param maxbyte   end character
 */
static inline void
te_charset_add_range(te_charset *cset, uint8_t minbyte, uint8_t maxbyte)
{
    te_intset_generic_add_range(&te_charset_intset, cset, minbyte, maxbyte);
}

/**
 * Remove a contiguous range of characters from a charset.
 *
 * @param cset      character set
 * @param minbyte   starting character
 * @param maxbyte   end character
 */
static inline void
te_charset_remove_range(te_charset *cset, uint8_t minbyte, uint8_t maxbyte)
{
    te_intset_generic_remove_range(&te_charset_intset, cset, minbyte, maxbyte);
}

/**
 * Check whether a @p byte is in a charset @p cset.
 *
 * @param cset  character set
 * @param byte  a byte to check
 *
 * @return @c TRUE iff @p byte is in @p cset
 */
static inline te_bool
te_charset_check(const te_charset *cset, uint8_t byte)
{
    return te_charset_intset.check(byte, cset);
}

/** Description of POSIX fd_set as an integral set. */
extern const te_intset_ops te_fdset_intset;

/**
 * Convert a string to a fd_set.
 *
 * The string @p str is a comma-separated list of single numbers and
 * ranges, e.g. @c 1,2-10,511.
 *
 * @param[in]  str    string to parse
 * @param[out] fdset  resulting FD set
 *
 * @return status code
 *
 * @sa te_intset_generic_parse
 */
static inline te_errno
te_fdset_parse(const char *str, fd_set *fdset)
{
    return te_intset_generic_parse(&te_fdset_intset, 0, FD_SETSIZE,
                                   str, fdset);
}

/**
 * Convert an FD set to a string.
 *
 * Sequences of consecutive FD numbers are represented as ranges @c N-M.
 *
 * @param nfds   highest FD number in @p fdset plus 1
 *               (the same as with POSIX select())
 * @param fdset  FD set
 *
 * @return a string representation or @c NULL in case of an error.
 *         (it should be free()'d)
 *
 * @sa te_intset_generic2string
 */
static inline char *
te_fdset2string(int nfds, const fd_set *fdset)
{
    return te_intset_generic2string(&te_fdset_intset, 0, nfds - 1, fdset);
}

/**
 * Check whether a FD set @p sub is a subset of @p super.
 *
 * @param nfds   highest FD number in @p fdset plus 1
 *               (the same as with POSIX select())
 * @param sub    FD set to test
 * @param super  FD superset
 *
 * @return @c TRUE iff @p sub is a subset of @p super
 */
static inline te_bool
te_fdset_is_subset(int nfds, const fd_set *sub, const fd_set *super)
{
    return te_intset_generic_is_subset(&te_fdset_intset, 0, nfds - 1,
                                       sub, super);
}

#if HAVE_SCHED_SETAFFINITY

/** Description of Linux cpu sets as an integral set */
extern const te_intset_ops te_cpuset_intset;

/**
 * Convert a string to a cpu_set_t.
 *
 * The string @p str is a comma-separated list of single numbers and
 * ranges, e.g. @c 0,2-10,512.
 *
 * @param[in]  str    string to parse
 * @param[out] cpuset resulting CPU set
 *
 * @return status code
 *
 * @sa te_intset_generic_parse
 */
static inline te_errno
te_cpuset_parse(const char *str, cpu_set_t *cpuset)
{
    return te_intset_generic_parse(&te_cpuset_intset, 0, CPU_SETSIZE - 1,
                                   str, cpuset);
}

/**
 * Convert a CPU set to a string.
 *
 * Sequences of consecutive CPU ids are represented as ranges @c N-M.
 *
 * @param  cpuset  CPU set
 *
 * @return a string representation or @c NULL in case of an error
 *         (it should be free()'d)
 *
 * @note Currently this only support fixed-size cpu_set_t's, not
 *       those returned by CPU_ALLOC.
 *
 * @sa te_intset_generic2string
 */
static inline char *
te_cpuset2string(const cpu_set_t *cpuset)
{
    return te_intset_generic2string(&te_cpuset_intset, 0,
                                    CPU_SETSIZE - 1, cpuset);
}

/**
 * Check whether a CPI set @p sub is a subset of @p super.
 *
 * @param sub    CPU set to test
 * @param super  CPU superset
 *
 * @return @c TRUE iff @p sub is a subset of @p super
 */
static inline te_bool
te_cpuset_is_subset(const cpu_set_t *sub, const cpu_set_t *super)
{
    return te_intset_generic_is_subset(&te_cpuset_intset, 0, CPU_SETSIZE - 1,
                                       sub, super);
}

#endif /* HAVE_SCHED_SETAFFINITY */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_INTSET_H__ */
/**@} <!-- END te_tools_te_intset --> */
