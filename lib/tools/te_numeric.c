/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Numeric operations.
 *
 * Implementation of numeric functions.
 */

#define TE_LGR_USER     "TE Numeric"

#include "te_config.h"
#include <math.h>

#include "te_numeric.h"
#include "logger_api.h"

static te_errno
generic_double2int(long double val, long double min, long double max,
                   te_bool is_unsigned, void *result)
{
    switch (fpclassify(val))
    {
        case FP_NAN:
            ERROR("Not a number");
            return TE_EINVAL;

        case FP_INFINITE:
            ERROR("Infinite value");
            return TE_EINVAL;

        case FP_SUBNORMAL:
            ERROR("%Lg is denormalized", val);
            return TE_EDOM;

        case FP_ZERO:
            if (is_unsigned)
                *(uintmax_t *)result = 0u;
            else
                *(intmax_t *)result = 0;
            break;

        case FP_NORMAL:
            if (isgreater(val, max))
            {
                ERROR("%Lg is greater than %Lg", val, max);
                return TE_ERANGE;
            }
            if (isless(val, min))
            {
                ERROR("%Lg is less than %Lg", val, min);
                return TE_ERANGE;
            }
            if (isless(fabsl(val), 1.0L))
            {
                ERROR("%Lg is non-zero and less than 1.0", val);
                return TE_EDOM;
            }
            if (is_unsigned)
                *(uintmax_t *)result = (uintmax_t)val;
            else
                *(intmax_t *)result = (intmax_t)val;
            break;

        default:
            assert(0);
    }
    return 0;
}

te_errno
te_double2int_safe(long double val, intmax_t lim, intmax_t *result)
{
    return generic_double2int(val, (long double)(-lim - 1), (long double)lim,
                              FALSE, result);
}

te_errno
te_double2uint_safe(long double val, uintmax_t max, uintmax_t *result)
{
    return generic_double2int(val, 0.0L, (long double)max, FALSE, result);
}
