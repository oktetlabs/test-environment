/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */

/** @file
 * @brief API for random numbers
 *
 * @defgroup te_tools_te_rand Random numbers generation
 * @ingroup te_tools
 * @{
 *
 * Definition of API for generating random numbers.
 */

#ifndef __TE_TOOLS_RAND_H__
#define __TE_TOOLS_RAND_H__

#include "te_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

/**
 * Generate a random unsigned number in a given range.
 *
 * @param min      Minimum value (inclusive).
 * @param max      Maximum value (inclusive).
 *
 * @return The next pseudo-random number between @p min and @p max.
 *
 * @alg The function uses xoshiro256++ algorithm that is
 *      able to generate the full range of 64-bit numbers,
 *      see http://vigna.di.unimi.it/ftp/papers/ScrambledLinear.pdf.
 *
 *      The PRNG state is seeded based on the standard @c rand(), so
 *      that applications that use @c srand() would get reproducible
 *      results with this function as well.
 *
 * @todo The current implementation is not well suited for multithreaded
 *       applications, just like the standard @c rand() used by
 *       rand_range(), however, the algorithm does support creation of
 *       robust parallel PRNGs, so it may be implemented, should it ever
 *       be needed.
 */
extern uintmax_t te_rand_unsigned(uintmax_t min, uintmax_t max);

/**
 * Generate a random signed number in a given range.
 *
 * See te_rand_unsigned() for details.
 *
 * @param min      Minimum value (inclusive).
 * @param max      Maximum value (inclusive).
 *
 * @return The next pseudo-random number between @p min and @p max.
 */
extern intmax_t te_rand_signed(intmax_t min, intmax_t max);

/**
 * Generate a random unsigned number in a given range
 * such that its value modulo @p div is @p rem.
 *
 * @param min      Minimum value (inclusive).
 * @param max      Maximum value (inclusive).
 * @param div      Divisor.
 * @param rem      Expected remainder.
 *
 * @return A random value such that it's equal to @p rem
 *         modulo @p div.
 */
static inline uintmax_t
te_rand_unsigned_div(uintmax_t min, uintmax_t max,
                     unsigned int div, unsigned int rem)
{
    return (te_rand_unsigned(min, max) / div) * div + rem;
}

/**
 * Generate a random signed number in a given range
 * such that its value modulo @p div is @p rem.
 *
 * If @p rem is zero, the value will always be in the
 * range between @p min and @p max. Otherwise, it may fall
 * out of this range if @p min or @p max themselves are not
 * equal to @p rem modulo @p div.
 *
 * The function takes into account that in C the remainder
 * is negative if the quotient is negative, so if the result
 * of the function is negative, it will actually be so that:
 * `te_rand_signed_div(min, max, div, mod) % div == -mod`.
 *
 * @param min      Minimum value (inclusive).
 * @param max      Maximum value (inclusive).
 * @param div      Divisor.
 * @param rem      Expected remainder.
 *
 * @return A random value such that it's equal to @p rem
 *         modulo @p div.
 */
static inline intmax_t
te_rand_signed_div(intmax_t min, intmax_t max,
                   unsigned int div, unsigned int mod)
{
    intmax_t v = (te_rand_signed(min, max) / div) * div;

    return v >= 0 ? v + mod : v - mod;
}

/**
 * Choose a random value from a range excluding some value inside
 * that range.
 *
 * @note This function will use TE_FATAL_ERROR() if it is impossible
 *       to choose any value.
 *
 * @param min       Range lower limit.
 * @param max       Range upper limit.
 * @param exclude   Value to exclude.
 *
 * @return Random number from the range [min, max].
 */
extern int te_rand_range_exclude(int min, int max, int exclude);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_RAND_H__ */
/**@} <!-- END te_tools_te_rand --> */
