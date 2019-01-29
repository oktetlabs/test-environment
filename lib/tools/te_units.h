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
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
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

#include "te_errno.h"

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

/**
 * Read value-unit from the string and store it to @p unit
 *
 * @param str               String representation of value-unit
 * @param value             Value-unit to store result in
 *
 * @return Status code.
 */
extern te_errno te_unit_from_string(const char *str, te_unit *value);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_UNITS_H__ */
/**@} <!-- END te_tools_te_units --> */
