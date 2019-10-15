/** @file
 * @brief Test Environment unit-conversion functions
 *
 * Unit-conversion functions
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */
#define TE_LGR_USER     "TE Units"

#include "te_config.h"

#include "te_units.h"
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include "logger_api.h"
#include "te_defs.h"

/* All units supported by module */
static const te_unit_prefix units[] = {
    TE_UNIT_PREFIX_NONE,
    TE_UNIT_PREFIX_KILO,
    TE_UNIT_PREFIX_MEGA,
    TE_UNIT_PREFIX_GIGA
};

/* Prefixes for units supported by module, must be in sync with units array */
static const char *prefixes[] = {
    [TE_UNIT_PREFIX_NONE] = "",
    [TE_UNIT_PREFIX_KILO] = "K",
    [TE_UNIT_PREFIX_MEGA] = "M",
    [TE_UNIT_PREFIX_GIGA] = "G"
};

/* Get supported unit count */
static size_t
te_unit_count(void)
{
    return TE_ARRAY_LEN(units);
}

/* Get unit prefix by index in range [0, te_unit_count() - 1] */
static te_unit_prefix
te_unit_prefix_get_by_index(size_t index)
{
    assert(index < te_unit_count());

    return units[index];
}

/* Pack decimal/binary plain value to te_unit */
static te_unit
pack(double value, double factor)
{
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
    assert(unit < TE_ARRAY_LEN(prefixes));

    return prefixes[unit];
}

static te_errno
te_unit_parse_unit(const char *str, double *value, const char **prefix)
{
    int saved_errno = errno;
    int new_errno = 0;
    double val;
    char *end;

    errno = 0;
    val = strtod(str, &end);
    new_errno = errno;
    errno = saved_errno;
    if (new_errno != 0)
        return te_rc_os2te(new_errno);
    else if (end == NULL || end == str)
        return TE_EINVAL;

    *value = val;
    *prefix = (const char *)end;

    return 0;
}

/* See description in te_units.h */
te_errno
te_unit_from_string(const char *str, te_unit *value)
{
    double          val;
    te_unit_prefix  prefix;
    const char *prefix_str;
    size_t  i;
    size_t  count;
    te_errno rc;

    rc = te_unit_parse_unit(str, &val, &prefix_str);
    if (rc != 0)
        return rc;

    count = te_unit_count();
    for (i = 0; i < count; ++i)
    {
        te_unit_prefix pfx = te_unit_prefix_get_by_index(i);

        if (strcmp(prefix_str, te_unit_prefix2str(pfx)) == 0)
        {
            prefix = pfx;
            break;
        }
    }

    if (i == count)
    {
        ERROR("Unknown unit prefix: %s", prefix_str);
        return TE_EINVAL;
    }

    value->value = val;
    value->unit = prefix;
    return 0;
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

/* See description in te_units.h */
te_errno
te_unit_list_value_from_string(const char *str, const te_unit_list *type,
                               double *value)
{
    double val;
    const char *prefix_str;
    size_t i;
    int power;
    te_errno rc;

    rc = te_unit_parse_unit(str, &val, &prefix_str);
    if (rc != 0)
        return rc;

    for (i = 0, power = type->start_pow; type->units[i] != NULL; i++, power++)
    {
        if (strcmp(prefix_str, type->units[i]) == 0)
        {
            *value = val * pow(type->scale, power);

            return 0;
        }
    }

    ERROR("Failed to parse unit prefix %s from string: %s", prefix_str, str);

    return TE_EINVAL;
}
