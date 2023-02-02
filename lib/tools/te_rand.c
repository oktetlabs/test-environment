/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */

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

/* See description in te_rand.h */
int
te_rand_range_exclude(int min, int max, int exclude)
{
    int value;

    if (min > max)
        TE_FATAL_ERROR("incorrect range limits");

    if (exclude < min || exclude > max)
        return rand_range(min, max);

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
    value = rand_range(min, max - 1);
    if (value >= exclude)
        value++;

    return value;
}
