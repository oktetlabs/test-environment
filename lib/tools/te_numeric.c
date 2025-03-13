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
#include "te_rand.h"
#include "logger_api.h"

const te_enum_map te_scalar_type_names[] = {
#define MEMBER(scalar_type_, c_type_, ...) \
    {.name = #c_type_, .value = (scalar_type_)},
    TE_SCALAR_TYPE_LIST(MEMBER)
#undef MEMBER
    TE_ENUM_MAP_END
};

typedef struct te_scalar_type_props {
    size_t size;
    intmax_t min_val;
    uintmax_t max_val;
} te_scalar_type_props;

static const te_scalar_type_props te_scalar_types_props[] = {
#define MEMBER(scalar_type_, c_type_, min_, max_) \
    [scalar_type_] = (te_scalar_type_props) {       \
        .size = sizeof(c_type_),                    \
        .min_val = (min_),                          \
        .max_val = (max_),                          \
    },
    TE_SCALAR_TYPE_LIST(MEMBER)
#undef MEMBER
};

static te_errno
generic_double2int(long double val, long double min, long double max,
                   bool is_unsigned, void *result)
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
                              false, result);
}

te_errno
te_double2uint_safe(long double val, uintmax_t max, uintmax_t *result)
{
    return generic_double2int(val, 0.0L, (long double)max, false, result);
}

uintmax_t
te_scalar_type_max(te_scalar_type type)
{
    return te_scalar_types_props[type].max_val;
}

intmax_t
te_scalar_type_min(te_scalar_type type)
{
    return te_scalar_types_props[type].min_val;
}

size_t
te_scalar_type_sizeof(te_scalar_type type)
{
    return te_scalar_types_props[type].size;
}

bool
te_scalar_type_is_signed(te_scalar_type type)
{
    return te_scalar_type_min(type) != 0;
}

te_errno
te_scalar_type_fit_size(bool is_signed, size_t size, te_scalar_type *type)
{
    size_t i;

    for (i = 0; i < TE_ARRAY_LEN(te_scalar_types_props); i++)
    {
        if ((te_scalar_type_sizeof(i) == size) &&
            (te_scalar_type_is_signed(i) == is_signed))
        {
            *type = (te_scalar_type)i;
            return 0;
        }
    }
    return TE_ERANGE;
}

static te_errno
te_scalar_val_fit_type_limits(intmax_t val, te_scalar_type src_type,
                              te_scalar_type dst_type)
{
    if (!te_scalar_type_is_signed(src_type) || val >= 0)
        return (uintmax_t)val > te_scalar_type_max(dst_type) ? TE_EOVERFLOW : 0;

    /* val < 0 here and src_type is signed */
    if (!te_scalar_type_is_signed(dst_type))
        return TE_EOVERFLOW;

    /* val < 0 here and dst_type is signed */
    return val < te_scalar_type_min(dst_type) ? TE_EOVERFLOW : 0;
}

te_errno
te_scalar_dynamic_cast(te_scalar_type src_type, const void *src,
                       te_scalar_type dst_type, void *dst)
{
    intmax_t src_val = 0;
    te_errno rc;

#define CASE(scalar_type_, src_type_, ...) \
    case (scalar_type_):                        \
    {                                           \
        src_val = (intmax_t)*(src_type_ *)src;  \
        break;                                  \
    }
    switch (src_type)
    {
        TE_SCALAR_TYPE_LIST(CASE);
    }
#undef CASE

    rc = te_scalar_val_fit_type_limits(src_val, src_type, dst_type);

    if (dst == NULL)
        return rc;

#define CASE(scalar_type_, dst_type_, ...) \
    case (scalar_type_):                        \
    {                                           \
        *(dst_type_ *)dst = (dst_type_)src_val; \
        break;                                  \
    }
    switch (dst_type)
    {
        TE_SCALAR_TYPE_LIST(CASE);
    }
#undef CASE

    return rc;
}

void
te_scalar_random(te_scalar_type type, void *dst)
{
    if (te_scalar_type_is_signed(type))
    {
        intmax_t val = te_rand_signed(te_scalar_type_min(type),
                                      (intmax_t)te_scalar_type_max(type));

        te_scalar_dynamic_cast(TE_SCALAR_TYPE_INTMAX_T, &val, type, dst);
    }
    else
    {
        uintmax_t val = te_rand_signed((uintmax_t)te_scalar_type_min(type),
                                       te_scalar_type_max(type));

        te_scalar_dynamic_cast(TE_SCALAR_TYPE_UINTMAX_T, &val, type, dst);
    }
}
