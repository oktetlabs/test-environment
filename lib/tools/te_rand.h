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
