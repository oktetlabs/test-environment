/** @file
 * @brief Test Environment unit-conversion functions
 *
 * Unit-conversion functions
 *
 *
 * Copyright (C) 2017 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#ifndef __TE_UNITS_H__
#define __TE_UNITS_H__

#include "te_config.h"
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Decimal unit-convertion functions
 * (International System of Units (SI))
 */
#define TE_UNITS_DEC_G2U(_val) ((_val) * 1000000000.)
#define TE_UNITS_DEC_M2U(_val) ((_val) * 1000000.)
#define TE_UNITS_DEC_K2U(_val) ((_val) * 1000.)

#define TE_UNITS_DEC_U2G(_val) ((_val) / 1000000000.)
#define TE_UNITS_DEC_U2M(_val) ((_val) / 1000000.)
#define TE_UNITS_DEC_U2K(_val) ((_val) / 1000.)

/*
 * Binary unit-convertion functions
 * (International Electrotechnical Commission (IEC))
 */
#define TE_UNITS_BIN_G2U(_val) ((_val) * 1024. * 1024. * 1024.)
#define TE_UNITS_BIN_M2U(_val) ((_val) * 1024. * 1024.)
#define TE_UNITS_BIN_K2U(_val) ((_val) * 1024.)

#define TE_UNITS_BIN_U2G(_val) ((_val) / (1024. * 1024. * 1024.))
#define TE_UNITS_BIN_U2M(_val) ((_val) / (1024. * 1024.))
#define TE_UNITS_BIN_U2K(_val) ((_val) / 1024.)


/** List of supported unit prefixes */
typedef enum te_unit_prefix {
    TE_UNIT_PREFIX_NONE,
    TE_UNIT_PREFIX_KILO,
    TE_UNIT_PREFIX_MEGA,
    TE_UNIT_PREFIX_GIGA,
} te_unit_prefix;

/** Value-unit pair */
typedef struct te_unit {
    double value;           /**< Value */
    te_unit_prefix unit;    /**< Unit prefix */
} te_unit;


/**
 * Convert unit prefix to string.
 *
 * @param unit      Unit prefix.
 *
 * @return String representation of unit prefix, or @c NULL if prefix is unknown.
 */
extern const char *te_unit_prefix2str(te_unit_prefix unit);

/**
 * Convert plain value to value-unit.
 *
 * @param value             Value to convert.
 *
 * @return Converted value with unit prefix.
 */
extern te_unit te_unit_pack(double value);

/**
 * Convert value-unit to plain value.
 *
 * @param value             Value to convert.
 *
 * @return Plain value.
 */
extern double te_unit_unpack(te_unit value);

/**
 * Convert binary plain value to value-unit.
 *
 * @param value             Value to convert.
 *
 * @return Converted value with unit prefix.
 */
extern te_unit te_unit_bin_pack(double value);

/**
 * Convert value-unit to binary plain value.
 *
 * @param value             Value to convert.
 *
 * @return Plain value.
 */
extern double te_unit_bin_unpack(te_unit value);


/**
 * Convert bytes to kilobytes.
 *
 * @param bytes     Number of bytes.
 *
 * @return Floating point value of kilobytes.
 *
 * @sa te_b2kb, te_kb2b
 */
static inline double
te_kb(uint64_t bytes)
{
    return (double)bytes / 1024;
}

/**
 * Convert bytes to megabytes.
 *
 * @param bytes     Number of bytes.
 *
 * @return Floating point value of megabytes.
 *
 * @sa te_b2mb, te_mb2b
 */
static inline double
te_mb(uint64_t bytes)
{
    return te_kb(bytes) / 1024;
}

/**
 * Convert bytes to gigabytes.
 *
 * @param bytes     Number of bytes.
 *
 * @return Floating point value of gigabytes.
 *
 * @sa te_b2gb, te_gb2b
 */
static inline double
te_gb(uint64_t bytes)
{
    return te_mb(bytes) / 1024;
}

/**
 * Convert bytes to kilobytes.
 *
 * @param bytes     Number of bytes.
 *
 * @return Number of kilobytes.
 *
 * @sa te_kb, te_kb2b
 */
static inline uint64_t
te_b2kb(uint64_t bytes)
{
    return te_kb(bytes);
}

/**
 * Convert bytes to megabytes.
 *
 * @param bytes     Number of bytes.
 *
 * @return Number of megabytes.
 *
 * @sa te_mb, te_mb2b
 */
static inline uint64_t
te_b2mb(uint64_t bytes)
{
    return te_mb(bytes);
}

/**
 * Convert bytes to gigabytes.
 *
 * @param bytes     Number of bytes.
 *
 * @return Number of gigabytes.
 *
 * @sa te_gb, te_gb2b
 */
static inline uint64_t
te_b2gb(uint64_t bytes)
{
    return te_gb(bytes);
}

/**
 * Convert kilobytes to bytes.
 *
 * @param kilobytes     Number of kilobytes.
 *
 * @return Number of bytes.
 *
 * @sa te_kb, te_b2kb
 */
static inline uint64_t
te_kb2b(uint64_t kilobytes)
{
    return kilobytes * 1024;
}

/**
 * Convert megabytes to bytes.
 *
 * @param megabytes     Number of megabytes.
 *
 * @return Number of bytes.
 *
 * @sa te_mb, te_b2mb, te_mb2kb
 */
static inline uint64_t
te_mb2b(uint64_t megabytes)
{
    return te_kb2b(megabytes) * 1024;
}

/**
 * Convert gigabytes to bytes.
 *
 * @param gigabytes     Number of gigabytes.
 *
 * @return Number of bytes.
 *
 * @sa te_gb, te_b2gb
 */
static inline uint64_t
te_gb2b(uint64_t gigabytes)
{
    return te_mb2b(gigabytes) * 1024;
}

/**
 * Convert megabytes to kilobytes.
 *
 * @param megabytes     Number of megabytes.
 *
 * @return Number of kilobytes.
 *
 * @sa te_mb2b
 */
static inline uint64_t
te_mb2kb(uint64_t megabytes)
{
    return te_kb2b(megabytes);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_UNITS_H__ */
