/** @file
 * @brief Test Environment unit-conversion functions
 *
 * @defgroup te_tools_te_units Unit-conversion
 * @ingroup te_tools
 * @{
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
    TE_UNIT_PREFIX_NONE,    /**< no prefix: value as is */
    TE_UNIT_PREFIX_KILO,    /**< kilo */
    TE_UNIT_PREFIX_MEGA,    /**< mega */
    TE_UNIT_PREFIX_GIGA,    /**< giga */
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

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_UNITS_H__ */
/**@} <!-- END te_tools_te_units --> */
