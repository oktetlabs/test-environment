/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Generic mapping between names and integral values
 *
 * @defgroup te_tools_te_enum Mapping between names and values
 * @ingroup te_tools
 * @{
 *
 * Definition of the mapping functions.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
 * Parse the longest label from @p map at the start of @p str.
 *
 * If @p exact_match is @c TRUE, @p str must start
 * with one of the labels from @p map. If there are several
 * candidate labels, the longest one is chosen.
 *
 * If @p exact_match is @c FALSE, @p str must have
 * a non-empty common prefix with one of the labels
 * from @p map. If there are several such labels,
 * the one with the longest common prefix is chosen
 * (i.e. in this case the length of the label does not
 * matter as such).
 *
 * If there are multiple matches of the same length, the first one is
 * chosen.
 *
 * If a match is found, a corresponding value from @p map is
 * returned and if @p next is not @c NULL, it is set to point to
 * the rest of @p str.
 *
 * If no match is found, @p defval is returned and @p next would
 * contain unchanged @p str.
 *
 * For example:
 *
 * @code
 * static const te_enum_map map[] = {
 *     { "ERROR", LEVEL_ERROR },
 *     { "WARNING", LEVEL_WARNING },
 *     TE_ENUM_MAP_END
 * };
 * ...
 * val = te_enum_parse_longest_match(map, -1, FALSE, "ERR message", &next);
 * // val is LEVEL_ERROR, next is " message"
 * // "ERR" and "ERROR" have a 3-character common prefix
 *
 * val = te_enum_parse_longest_match(map, -1, TRUE, "ERR message", &next);
 * // exact match is required, but "ERR message" does not start
 * // with any of labels in map, val is -1, next is "ERR message"
 *
 * val = te_enum_parse_longest_match(map, -1, FALSE, "WARN", &next);
 * // val is LEVEL_WARNING, next is ""
 *
 * val = te_enum_parse_longest_match(map, -1, FALSE, "INFO", &next);
 * // even though match is not exact, there is no label in map
 * // that would have a non-empty common prefix with "INFO",
 * // so val is -1, next is "INFO"
 * @endcode
 *
 * This function may be used on non-zero terminated byte arrays,
 * if it can be ensured there are always valid characters after
 * the prefix:
 *
 * @code
 * char bytes[13] = "ERROR message"; // no terminating zero
 * te_enum_parse_longest_match(map, -1, FALSE, bytes, &next);
 * // ok, we know there are valid characters after the label
 *
 * char bytes[3] = "ERR"; // no terminating zero
 * te_enum_parse_longest_match(map, -1, FALSE, bytes, &next);
 * // not ok: the function will look past the last valid character
 * @endcode
 *
 * @param[in]  map           Mapping.
 * @param[in]  defval        Default return value.
 * @param[in]  exact_match   If @c TRUE, prefixes of map labels are matched.
 * @param[in]  str           Input string.
 * @param[out] next          If not @c NULL, the rest of @p str is
 *                           stored here.
 *
 * @return A corresponding enum value from @p map or @p defval.
 */
extern int te_enum_parse_longest_match(const te_enum_map map[], int defval,
                                       te_bool exact_match, const char *str,
                                       char **next);

/**
 * Fill in an enum mapping array based on the mapping function.
 *
 * The purpose of the function is to bridge te_enum API and pre-existing
 * value-to-string functions such as used in RPC libraries
 *
 * @param[out] map  An array of sufficient size (@p maxval - @p minval + 2)
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
 * Define a structure to map names to actions.
 *
 * The structure will have two fields:
 * - @a name
 * - @a action
 *
 * The structure is intended to be used with TE_ENUM_DISPATCH.
 *
 * An array of such structures should be terminated with TE_ENUM_MAP_END.
 *
 * @param ftype_  type of actions: a pointer to function type
 */
#define TE_ENUM_MAP_ACTION(ftype_) \
    struct {                       \
        const char *name;          \
        ftype_ action;             \
    }

/**
 * Execute an action associated with a given @p name_.
 *
 * The @p table_ should be an array of structures defined by
 * TE_ENUM_MAP_ACTION terminated with TE_ENUM_MAP_END.
 *
 * If @p name_ is not found in @p table_, @p unknown_ is called
 * which may be a function, a pointer to function or a macro name.
 *
 * Actions cannot be @c void functions.
 *
 * @code
 * typedef te_errno (*handler_fn)(int arg);
 *
 * static te_errno handler1(int arg)
 * {
 *     ...
 * }
 *
 * static te_errno handler1(int arg)
 * {
 *     ...
 * }
 * ...
 * static const TE_ENUM_ACTION(handler_fn) actions[] = {
 *     {.name= "handler1", .action = handler1},
 *     {.name= "handler2", .action = handler2},
 *     TE_ENUM_MAP_END
 * };
 * te_errno rc;
 * int arg;
 *
 * TE_ENUM_DISPATCH(actions, unknown_handler, rc, arg);
 * @encode
 *
 * @param[in]  table_    a table of name-to-action mappings
 * @param[in]  unknown_  a handler for unknown names
 * @param[out] retval_   lvalue to store the return value of an action
 * @param[in]  ...       parameters passed to actions
 */
#define TE_ENUM_DISPATCH(table_, unknown_, name_, retval_, ...) \
    do {                                                        \
        unsigned int i_;                                        \
                                                                \
        for (i_ = 0; (table_)[i_].name != NULL; i_++)           \
        {                                                       \
            if (strcmp((table_)[i_].name, (name_)) == 0)        \
            {                                                   \
                (retval_) = (table_)[i_].action(__VA_ARGS__);   \
                break;                                          \
            }                                                   \
        }                                                       \
                                                                \
        if ((table_)[i_].name == NULL)                          \
            (retval_) = unknown_(__VA_ARGS__);                  \
    } while (0)

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
 * A mapping between two bitmasks corresponding to each other via
 * named bits.
 *
 * This structure permits the mapping of one or more bits from one
 * side to one or more bits on the other side, with the constraint
 * that the bits on each side do not overlap. This constraint results
 * in the prohibition of mapping two or more set bits to the same
 * corresponding bits on the other side.
 *
 * An array of mappings should end with #TE_ENUM_BITMASK_CONV_END and
 * should not contain @c UINT64_MAX as a value of one of the fields.
 */
typedef struct te_enum_bitmask_conv {
    /** Left-hand side bits mask. */
    uint64_t bits_from;
    /** Right-hand side bits mask. */
    uint64_t bits_to;
} te_enum_bitmask_conv;

/** Terminating element of an enum bitmask conversion array. */
#define TE_ENUM_BITMASK_CONV_END { .bits_from = UINT64_MAX, \
                                   .bits_to = UINT64_MAX }

/**
 * Convert a @p bm into a correpsonding bitmask according to @p bm_conv.
 *
 * @param[in] bm_conv     Conversion table.
 * @param[in] bm          Bitmask to convert.
 * @param[in] reverse     If @c TRUE, the @a to_bits field of the conversion
 *                        is used as a key, i.e. a reverse conversion is
 *                        performed.
 * @param[out] result     Resulting bitmask. May be @c NULL to check that
 *                        there are no bits that can not be converted.
 *
 * @return Status code.
 * @retval TE_EINVAL      @p bm_conv contains zero values or overlapped bits.
 * @retval TE_ERANGE      Conversion was done, but some bits of @p bm can not
 *                        be converted using @p conv_map.
 */
extern te_errno te_enum_bitmask_convert(const te_enum_bitmask_conv bm_conv[],
                                        uint64_t bm, te_bool reverse,
                                        uint64_t *result);

/**
 * Fill in an enum translation array based on the translation function.
 *
 * The purpose of the function is to bridge te_enum API and pre-existing
 * value-to-value conversion functions such as used in RPC libraries
 *
 * @param[out] trn  An array of sufficient size (@p maxval - @p minval + 2)
 *                  to be filled.
 *                  The terminating TE_ENUM_TRN_END will be appended
 * @param minval    Minimal source enum value
 * @param maxval    Maximum source enum value
 * @param val2val   Conversion function
 */
extern void te_enum_trn_fill_by_conversion(te_enum_trn trn[],
                                           int minval, int maxval,
                                           int (*val2val)(int val));


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ndef __TE_ENUM_H__ */
/**@} <!-- END te_tools_te_enum --> */
