/** @file
 * @brief Test Environment unit-conversion functions
 *
 * Unit-conversion functions
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */
#include "te_units.h"
#ifdef HAVE_MATH_H
#include <math.h>
#endif
#include "te_defs.h"

/* Pack decimal/binary plain value to te_unit */
static te_unit
pack(double value, double factor)
{
    static const te_unit_prefix units[] = {
        TE_UNIT_PREFIX_NONE,
        TE_UNIT_PREFIX_KILO,
        TE_UNIT_PREFIX_MEGA,
        TE_UNIT_PREFIX_GIGA
    };
    size_t i = 0;

    while (fabs(value) >= factor && i < (TE_ARRAY_LEN(units) - 1))
    {
        value /= factor;
        i++;
    }

    return (te_unit){ .value = value, .unit = units[i] };
}

/* Unpack te_unit to decimal/binary plain value */
static double
unpack(te_unit value, double factor)
{
    static const double powers[] = {
        [TE_UNIT_PREFIX_NONE] = 0,
        [TE_UNIT_PREFIX_KILO] = 1,
        [TE_UNIT_PREFIX_MEGA] = 2,
        [TE_UNIT_PREFIX_GIGA] = 3
    };

    return value.value * pow(factor, powers[value.unit]);
}

/* See description in te_units.h */
const char *
te_unit_prefix2str(te_unit_prefix unit)
{
    static const char *prefixes[] = {
        [TE_UNIT_PREFIX_NONE] = "",
        [TE_UNIT_PREFIX_KILO] = "K",
        [TE_UNIT_PREFIX_MEGA] = "M",
        [TE_UNIT_PREFIX_GIGA] = "G"
    };

    assert(unit < TE_ARRAY_LEN(prefixes));

    return prefixes[unit];
}

/* See description in te_units.h */
te_unit
te_unit_pack(double value)
{
    return pack(value, 1000.);
}

/* See description in te_units.h */
double
te_unit_unpack(te_unit value)
{
    return unpack(value, 1000.);
}

/* See description in te_units.h */
te_unit
te_unit_bin_pack(double value)
{
    return pack(value, 1024.);
}

/* See description in te_units.h */
double
te_unit_bin_unpack(te_unit value)
{
    return unpack(value, 1024.);
}
