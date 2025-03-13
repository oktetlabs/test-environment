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
 * basically it uses Kolmogorov-Smirnov test to verify that
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

typedef struct prng_params {
    intmax_t min;
    intmax_t max;
    size_t bitstart;
    size_t bitlen;
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

static void
generate_random(const prng_params *params, unsigned int n, intmax_t seq[n])
{
    /*
     * This is a critical value for Kolmogorov distribution
     * assuming n > 40 for a significance level 99.5%,
     * see
     * e.g. https://www.york.ac.uk/depts/maths/tables/kolmogorovsmirnov.pdf
     */
    static const long double k995 = 1.63;

    unsigned int i;
    unsigned int span;
    long double maxd = 0.0L;

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

    /*
     * The following is doing one-sample Kolmogorov-Smirnov test
     * to ensure the drawn samples agree with the uniform distribution.
     *
     * See https://en.wikipedia.org/wiki/Kolmogorov%E2%80%93Smirnov_test
     * for an introduction to the topic.
     */
    qsort(seq, n, sizeof(*seq), compare_intmax);

    for (i = 0; i < n; i += span)
    {
        /*
         * All calculations must be with long doubles, because
         * plain double cannot represent the full range of int64_t
         * precisely enough.
         */
        long double ecdf;
        long double ucdf;
        long double d;

        for (span = 1; i + span < n; span++)
        {
            if (seq[i + span] != seq[i])
                break;
        }

        ecdf = (long double)(i + span) / (long double)n;

        if (params->bitlen == 0 || params->bitlen == TE_BITSIZE(intmax_t))
        {
            ucdf = ((long double)seq[i] - (long double)params->min + 1.0L) /
                ((long double)params->max - (long double)params->min + 1.0L);
        }
        else
        {
            ucdf = ((long double)seq[i] + 1.0L) /
                (long double)(1ull << params->bitlen);
        }

        d = fabsl(ecdf - ucdf);
        if (d > maxd)
            maxd = d;
    }

    if (sqrtl((long double)n) * maxd > k995)
    {
        TEST_VERDICT("The sample is not uniform: %Lf > %Lf",
                     sqrtl((long double)n) * maxd, k995);
    }
    RING("Max difference between ECDF and uniform CDF = %Lf (< %Lf)",
         maxd, k995 / sqrtl((long double)n));
}

static void
test_random(unsigned int n)
{
    static const prng_params variants[] = {
        {
            .min = INTMAX_MIN, .max = INTMAX_MAX
        },
        {
            .min = INTMAX_MIN, .max = INTMAX_MAX,
            .bitstart = 0, .bitlen = 1
        },
        {
            .min = INTMAX_MIN, .max = INTMAX_MAX,
            .bitstart = 0, .bitlen = 8
        },
        {
            .min = INTMAX_MIN, .max = INTMAX_MAX,
            .bitstart = TE_BITSIZE(intmax_t) - 8, .bitlen = 8
        },
        {
            .min = INTMAX_MIN, .max = INTMAX_MAX,
            .bitstart = TE_BITSIZE(intmax_t) - 1, .bitlen = 1
        },
        {
            .min = 0, .max = INTMAX_MAX
        },
        {
            .min = 0, .max = 1
        },
        {
            .min = -1, .max = 1
        },
        {
            .min = 0, .max = UINT8_MAX
        },
        {
            .min = 0, .max = UINT8_MAX,
            .bitstart = 0, .bitlen = 1
        },
        {
            .min = -1, .max = INTMAX_MAX
        },
        {
            .min = INTMAX_MIN, .max = 0
        },
        {
            .min = INTMAX_MIN, .max = INTMAX_MIN + 1
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
    intmax_t i8_sum = 0;
    uintmax_t u8_sum = 0;
    uintmax_t b_sum = 0;

    for (i = 0; i < n_numbers; i++)
    {
        int8_t i8;
        uint8_t u8;
        bool b;

        te_scalar_random(TE_SCALAR_TYPE_INT8_T, &i8);
        te_scalar_random(TE_SCALAR_TYPE_UINT8_T, &u8);
        te_scalar_random(TE_SCALAR_TYPE_BOOL, &b);

        i8_sum += i8;
        u8_sum += u8;
        b_sum += b;
    }

    /* The thresholds below are somewhat arbitrary. */
    if (fabs((double)i8_sum / (double)n_numbers) > 1.0)
        TEST_VERDICT("Generator of signed bytes is skewed");

    if (fabs((double)u8_sum / (double)n_numbers -
             (double)UINT8_MAX / 2.0) > 1.0)
    {
        TEST_VERDICT("Generator of signed bytes is skewed");
    }

    if (fabs((double)b_sum / (double)n_numbers - 0.5) > 0.01)
        TEST_VERDICT("Generator of booleans is skewed");
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
