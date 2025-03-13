/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023-2025 OKTET Labs Ltd. All rights reserved. */

/** @file
 * @brief API for random numbers
 *
 * Implementation of API for generating random numbers.
 */

#define TE_LGR_USER "TE RAND"

#include "te_config.h"

#include "te_rand.h"
#include "te_defs.h"
#include "logger_api.h"

/* Just to be on the safe side... */
#if UINTMAX_MAX > UINT64_MAX
#warning "uintmax_t is bigger than uint64_t: this may break PRNG routines"
#endif

/*
 * The functions below are adapted from
 * https://prng.di.unimi.it/xoshiro256plusplus.c and
 * https://prng.di.unimi.it/splitmix64.c
 *
 * No substantial changes have been made to the code,
 * only some renaming and reformatting.
 *
 * In particular, all magic numbers come from the original.
 *
 * See also http://vigna.di.unimi.it/ftp/papers/ScrambledLinear.pdf
 * for a theoretical discussion of the algorithm.
 */

/*
 * splitmix64:
 * Written in 2015 by Sebastiano Vigna (vigna@acm.org).
 *
 * xoshiro256pp:
 * Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org).
 *
 * To the extent possible under law, the author has dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

static uint64_t
rotl(uint64_t x, int k)
{
    return (x << k) | (x >> (64 - k));
}

static uint64_t
xoshiro256pp(uint64_t state[static 4])
{
    uint64_t result = rotl(state[0] + state[3], 23) + state[0];
    uint64_t t = state[1] << 17;

    state[2] ^= state[0];
    state[3] ^= state[1];
    state[1] ^= state[2];
    state[0] ^= state[3];

    state[2] ^= t;

    state[3] = rotl(state[3], 45);

    return result;
}

static uint64_t
splitmix64(uint64_t *seed)
{
    uint64_t z;

    *seed += 0x9e3779b97f4a7c15ull;
    z = *seed;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;

    return z ^ (z >> 31);
}

static uint64_t global_xoshiro_state[4];
static bool xoshiro_state_seeded;

/* See description in te_rand.h */
uintmax_t
te_rand_unsigned(uintmax_t min, uintmax_t max)
{
    uintmax_t r;

    if (!xoshiro_state_seeded)
    {
        /*
         * As stated by its designers, xoshiro algorithm
         * cannot be reliably seeded from a 32-bit seed,
         * so we need the second different PRNG to seed
         * 256 bit of state from the standard rand().
         */
        uint64_t seed = (uint64_t)rand();
        unsigned int i;

        for (i = 0; i < TE_ARRAY_LEN(global_xoshiro_state); i++)
            global_xoshiro_state[i] = splitmix64(&seed);

        xoshiro_state_seeded = true;
    }

    r = xoshiro256pp(global_xoshiro_state);

    /* Treat full range specially to avoid overflows */
    if (min == 0 && max == UINTMAX_MAX)
        return r;

    /*
     * It has always been recommended to use higher order bits
     * of a random value instead of lower order ones for
     * downscaling, because the former tend to be more
     * random than the latter. However, the algorithm we use
     * does generate pretty random lower bits.
     *
     * In addition, we cannot do the same thing as it's usually done
     * with @c rand(), namely `rand() * (max - min + 1) / RAND_MAX`,
     * because our generator is 64-bit, and so this would require
     * genuine 128-bit multiplication and division for this
     * to work, possible but cumbersome.
     *
     * So for the time being we just plainly take the value modulo
     * the range. As demonstrated by @c suites/selftest/rand.c,
     * the obtained values do pass the Kolmogorov-Smirnov test for
     * uniformity, so they are random enough.
     *
     * However, if the algorithm is ever to change, the decision
     * should be re-examined.
     */

    return r % (max - min + 1) + min;
}

/* See description in te_rand.h */
intmax_t
te_rand_signed(intmax_t min, intmax_t max)
{
    /*
     * If min is positive, all values fit safely in uintmax_t range.
     */
    if (min >= 0)
        return (intmax_t)te_rand_unsigned((uintmax_t)min, (uintmax_t)max);

    return (intmax_t)te_rand_unsigned(0, (uintmax_t)max +
                                      (uintmax_t)-min) + min;
}

/* See description in te_rand.h */
int
te_rand_range_exclude(int min, int max, int exclude)
{
    int value;

    if (min > max)
        TE_FATAL_ERROR("incorrect range limits");

    if (exclude < min || exclude > max)
        return (int)te_rand_signed(min, max);

    if (min == max)
        TE_FATAL_ERROR("no eligible values remain");

    /*
     * Here mapping of [min, max - 1] to [min, max] is used.
     * if x < exclude: x -> x
     * if x >= exclude: x -> x + 1
     *
     * For example, for min = 1, max = 5, exclude = 3:
     * 1 -> 1
     * 2 -> 2
     * 3 -> 4
     * 4 -> 5
     *
     * This way exclude value is excluded, and any other number
     * from [min, max] has the same chance of being chosen.
     */
    value = (int)te_rand_signed(min, max - 1);
    if (value >= exclude)
        value++;

    return value;
}
