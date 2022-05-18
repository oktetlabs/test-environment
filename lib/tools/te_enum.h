/** @file
 * @brief Generic mapping between names and integral values
 *
 * @defgroup te_tools_te_enum Mapping between names and values
 * @ingroup te_tools
 * @{
 *
 * Definition of the mapping functions.
 *
 * Copyright (C) 2003-2022 OKTET Labs. All rights reserved.
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 *
 */

#ifndef __TE_ENUM_H__
#define __TE_ENUM_H__

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A mapping between names and integral values.
 *
 * An array of mappings should end with TE_ENUM_MAP_END
 */
typedef struct te_enum_map {
    /** Element name */
    const char *name;
    /** Corresponding value */
    int value;
} te_enum_map;

/** Terminating element of an enum mapping array */
#define TE_ENUM_MAP_END { .name = NULL }

/**
 * Convert a symbolic name into a value.
 *
 * If there are several mappings with the same name, the first
 * one is used.
 *
 * @param map   Mapping
 * @param name  Name
 * @param unknown_val Value to return if the name is not found
 *
 * @return the value corresponding to @p name or @p unknown_val
 */
extern int te_enum_map_from_str(const te_enum_map map[],
                                const char *name,
                                int unknown_val);

/**
 * Convert a @p value into a symbolic name.
 *
 * If @p value is not
 * found in @p map, the function aborts, since it's a clearly programmer's
 * error, not a runtime error.
 *
 * If there are several mappings with the same value, the first one is used.
 *
 * @param map   Mapping
 * @param value Value
 *
 * @return A string matching the @p value
 */
extern const char *te_enum_map_from_value(const te_enum_map map[], int value);

/**
 * Fill in an enum mapping array based on the mapping function.
 *
 * The purpose of the function is to bridge te_enum API and pre-existing
 * value-to-string functions such as used in RPC libraries
 *
 * @param map[out]  An array of sufficient size (@p maxval - @p minval + 2)
 *                  to be filled.
 *                  The terminating TE_ENUM_MAP_END will be appended
 * @param minval    Minimal enum value
 * @param maxval    Maximum enum value
 * @param val2str   Conversion function
 */
extern void te_enum_map_fill_by_conversion(te_enum_map map[],
                                           int minval, int maxval,
                                           const char *(*val2str)(int val));


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ndef __TE_ENUM_H__ */
/**@} <!-- END te_tools_te_enum --> */
