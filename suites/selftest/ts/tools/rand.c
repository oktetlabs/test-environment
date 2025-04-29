/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for pseudo random number generator.
 *
 * Testing the correctness of pseudo random number generator.
 */

/** @page tools_rand PRNG test
 *
 * @objective Testing the sanity of PRNG routines.
 *
 * The test is not a replacement for real PRNG burnout tests:
 * basically it uses Pearson's chi square test to verify that
 * generated random values are uniformly distributed.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/rand"

#include "te_config.h"

#include <math.h>
#include "tapi_test.h"
#include "te_rand.h"
#include "te_numeric.h"

/*
 * The following critical values for chi square test with
 * significance level of 0.9999 were precalculated using
 * SciPy's stats module. The part of the name after the
 * `chi` prefix denotes the degrees of freedom.
 */
static long double chi1 = 15.136705226623606L;
static long double chi2 = 18.420680743952584L;
static long double chi255 = 347.6542127045896L;
static long double chi2_63 = 9.22337205282783E+18;
static long double chi2_64 = 1.844674409629886E+19;


typedef struct prng_params {
    intmax_t min;
    intmax_t max;
    size_t bitstart;
    size_t bitlen;
    long double chi;
} prng_params;

static int
compare_intmax(const void *v1, const void *v2)
{
    intmax_t i1 = *(intmax_t *)v1;
    intmax_t i2 = *(intmax_t *)v2;

    if (i1 == i2)
        return 0;

    return i1 > i2 ? 1 : -1;
}

/*
 * The following is doing Pearson's chi-square goodness-of-fit test
 * for the uniform distribution.
 *
 * See https://en.wikipedia.org/wiki/Pearson%27s_chi-squared_test
 * for an introduction to the topic.
 */
static void
validate_random(const prng_params *params, unsigned int n, intmax_t seq[n])
{
    long double n_bins;
    long double sum = 0.0L;
    unsigned int span = 1;
    unsigned int i;

    if (params->bitlen == 0 || params->bitlen == TE_BITSIZE(intmax_t))
        n_bins = (long double)params->max - (long double)params->min + 1.0L;
    else
        n_bins = (long double)(1ull << params->bitlen);

    qsort(seq, n, sizeof(*seq), compare_intmax);

    for (i = 0; i < n; i += span)
    {
        long double expected;
        for (span = 1; i + span < n && seq[i + span] == seq[i]; span++)
            ;
        expected = (long double)n / n_bins;
        sum += (((long double)span - expected) *
                ((long double)span - expected)) / expected;
    }

    if (sum > params->chi)
    {
        TEST_VERDICT("The sampling of %zu..%zu bits of %jd..%jd "
                     "appears non-uniform: %Lf > %Lf",
                     params->bitstart,
                     params->bitstart +
                     (params->bitlen > 0 ? params->bitlen - 1 :
                      TE_BITSIZE(intmax_t) - 1),
                     params->min, params->max,
                     sum, params->chi);
    }
}

static void
generate_random(const prng_params *params, unsigned int n, intmax_t seq[n])
{
    unsigned int i;

    /*
     * Believe it or not, but it may actually happen if a test
     * is run under valgrind.
     */
    if ((params->bitlen == 0 || params->bitlen == TE_BITSIZE(intmax_t)) &&
        (long double)params->max - (long double)params->min < 1.0L)
    {
        WARN("Cannot represent the difference between %jd and %jd",
             params->min, params->max);
        return;
    }

    for (i = 0; i < n; i++)
    {
        intmax_t r = te_rand_signed(params->min, params->max);

        if (r < params->min)
            TEST_VERDICT("Generated value is less than %jd", params->min);
        if (r > params->max)
            TEST_VERDICT("Generated value is greater than %jd", params->max);

        if (params->bitlen != 0 && params->bitlen < TE_BITSIZE(intmax_t))
            r = TE_EXTRACT_BITS(r, params->bitstart, params->bitlen);

        seq[i] = r;
    }

    validate_random(params, n, seq);
}

static void
test_random(unsigned int n)
{
    const prng_params variants[] = {
        {
            .min = INTMAX_MIN, .max = INTMAX_MAX,
            .chi = chi2_64,
        },
        {
            .min = INTMAX_MIN, .max = INTMAX_MAX,
            .bitstart = 0, .bitlen = 1,
            .chi = chi1,
        },
        {
            .min = INTMAX_MIN, .max = INTMAX_MAX,
            .bitstart = 0, .bitlen = 8,
            .chi = chi255,
        },
        {
            .min = INTMAX_MIN, .max = INTMAX_MAX,
            .bitstart = TE_BITSIZE(intmax_t) - 8, .bitlen = 8,
            .chi = chi255,

        },
        {
            .min = INTMAX_MIN, .max = INTMAX_MAX,
            .bitstart = TE_BITSIZE(intmax_t) - 1, .bitlen = 1,
            .chi = chi1,
        },
        {
            .min = 0, .max = INTMAX_MAX,
            .chi = chi2_63,
        },
        {
            .min = 0, .max = 1,
            .chi = chi1,
        },
        {
            .min = -1, .max = 1,
            .chi = chi2,
        },
        {
            .min = 0, .max = UINT8_MAX,
            .chi = chi255,
        },
        {
            .min = 0, .max = UINT8_MAX,
            .bitstart = 0, .bitlen = 1,
            .chi = chi1,
        },
        {
            .min = -1, .max = INTMAX_MAX,
            .chi = chi2_63,
        },
        {
            .min = INTMAX_MIN, .max = 0,
            .chi = chi2_63,
        },
        {
            .min = INTMAX_MIN, .max = INTMAX_MIN + 1,
            .chi = chi1,
        },
    };
    unsigned int i;
    intmax_t *seq = TE_ALLOC(n * sizeof(*seq));

    for (i = 0; i < TE_ARRAY_LEN(variants); i++)
    {
        TEST_SUBSTEP("Generating %jd..%jd, testing bits %zu..%zu",
                     variants[i].min, variants[i].max,
                     variants[i].bitstart,
                     variants[i].bitstart +
                     (variants[i].bitlen == 0 ? TE_BITSIZE(intmax_t) :
                      variants[i].bitlen) - 1);
        generate_random(&variants[i], n, seq);
    }

    free(seq);
}

static void
test_scalar_random(unsigned int n_numbers)
{
    unsigned int i;
    intmax_t signed_bytes[n_numbers];
    intmax_t unsigned_bytes[n_numbers];
    intmax_t bools[n_numbers];

    for (i = 0; i < n_numbers; i++)
    {
        int8_t i8;
        uint8_t u8;
        bool b;

        te_scalar_random(TE_SCALAR_TYPE_INT8_T, &i8);
        te_scalar_random(TE_SCALAR_TYPE_UINT8_T, &u8);
        te_scalar_random(TE_SCALAR_TYPE_BOOL, &b);

        signed_bytes[i] = i8;
        unsigned_bytes[i] = u8;
        bools[i] = b;
    }

    validate_random(&(const prng_params){.min = INT8_MIN, .max = INT8_MAX,
                                         .chi = chi255},
                    n_numbers, signed_bytes);
    validate_random(&(const prng_params){.min = 0, .max = UINT8_MAX,
                                         .chi = chi255},
                    n_numbers, unsigned_bytes);
    validate_random(&(const prng_params){.min = 0, .max = 1,
                                         .chi = chi1},
                    n_numbers, bools);
}

static void
test_random_div(unsigned int n_numbers)
{
    unsigned int i;

    for (i = 0; i < n_numbers; i++)
    {
        unsigned j;

        for (j = 1; j < 7; j++)
        {
            unsigned k;

            for (k = 0; k < j; k++)
            {
                uintmax_t uv = te_rand_unsigned_div(0, UINTMAX_MAX, j, k);
                intmax_t v = te_rand_signed_div(-(int)j - (int)k,
                                                (int)j + (int)k, j, k);

                if (uv % j != k)
                    TEST_VERDICT("Invalid unsigned value");
                if ((v < -(int)j - (int)k) || (v > (int)j + (int)k))
                    TEST_VERDICT("Signed value not in range: %jd", v);
                if (v % j != (v > 0 ? (intmax_t)k : -(intmax_t)k))
                    TEST_VERDICT("Invalid signed value");
            }
        }
    }
}

int
main(int argc, char **argv)
{
    unsigned int n_numbers;

    TEST_START;
    TEST_GET_UINT_PARAM(n_numbers);

    RING("The first random number "
         "(must be the same if te_rand_seed is the same): %ju",
         te_rand_unsigned(0, UINTMAX_MAX));

    TEST_STEP("Checking random number generator");
    test_random(n_numbers);

    TEST_STEP("Checking random scalar values");
    test_scalar_random(n_numbers);

    TEST_STEP("Checking random number generator with a given remainder");
    test_random_div(n_numbers);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
