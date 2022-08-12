/** @file
 * @brief Generic mapping between names and integral values
 *
 * @defgroup te_tools_te_enum Mapping between names and values
 * @ingroup te_tools
 * @{
 *
 * Definition of the mapping functions.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 */

#ifndef __TE_ENUM_H__
#define __TE_ENUM_H__

#include "te_defs.h"
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
 * If there are several mappings with the same value, the first one is used.
 *
 * @param map     Mapping
 * @param value   Value
 * @param unknown Label to return if @p value is not found
 *
 * @return A string matching the @p value or @p unknown
 */
extern const char *te_enum_map_from_any_value(const te_enum_map map[],
                                              int value,
                                              const char *unknown);

/**
 * Same as te_enum_map_from_any_value() but aborts the program if
 * the value is not found.
 *
 * This is to be used where a set of values is known to be closed,
 * so any value not in that set results from the programmer's error.
 *
 * @param map     Mapping
 * @param value   Value
 *
 * @return A string matching the @p value
 */
static inline const char *
te_enum_map_from_value(const te_enum_map map[], int value)
{
    const char *label = te_enum_map_from_any_value(map, value, NULL);

    if (label == NULL)
        abort();

    return label;
}

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


/**
 * A translation between two sets of integral values
 *
 * An array of translations should end with TE_ENUM_TRN_END
 */
typedef struct te_enum_trn {
    /** Left-hand side value */
    int from;
    /** Right-hand side value */
    int to;
} te_enum_trn;

/** Terminating element of an enum translation array */
#define TE_ENUM_TRN_END { .from = INT_MIN, .to = INT_MIN }


/**
 * Translate a @p value into a correpsonding value according to @p trn.
 *
 * If there are several translations with the same value,
 * the first one is used.
 *
 * @param trn         Translation table
 * @param value       Value
 * @param reverse     If `TRUE`, the @a to field of the translation is
 *                    used as a key, i.e. a reverse translation is
 *                    performed.
 * @param unknown_val Value to return if @p value is not found
 *
 * @return A value correspodning to the @p value or @p unknown_val
 */

extern int te_enum_translate(const te_enum_trn trn[],
                             int value, te_bool reverse,
                             int unknown_val);

/**
 * Fill in an enum translation array based on the translation function.
 *
 * The purpose of the function is to bridge te_enum API and pre-existing
 * value-to-value conversion functions such as used in RPC libraries
 *
 * @param trn[out]  An array of sufficient size (@p maxval - @p minval + 2)
 *                  to be filled.
 *                  The terminating TE_ENUM_TRN_END will be appended
 * @param minval    Minimal source enum value
 * @param maxval    Maximum source enum value
 * @param val2str   Conversion function
 */
extern void te_enum_trn_fill_by_conversion(te_enum_trn trn[],
                                           int minval, int maxval,
                                           int (*val2val)(int val));


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ndef __TE_ENUM_H__ */
/**@} <!-- END te_tools_te_enum --> */
