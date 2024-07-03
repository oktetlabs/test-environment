/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Generic mapping between names and integral values
 *
 * Implementation of the mapping functions.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"
#include <stdlib.h>
#include <string.h>

#include "te_defs.h"
#include "te_enum.h"
#include "te_str.h"

int
te_enum_map_from_str(const te_enum_map map[], const char *name,
                     int unknown_val)
{
    for (; map->name != NULL; map++)
    {
        if (strcmp(map->name, name) == 0)
            return map->value;
    }
    return unknown_val;
}

const char *
te_enum_map_from_any_value(const te_enum_map map[], int value,
                           const char *unknown)
{
    for (; map->name != NULL; map++)
    {
        if (map->value == value)
            return map->name;
    }
    return unknown;
}

int
te_enum_parse_longest_match(const te_enum_map map[], int defval,
                            bool exact_match, const char *str, char **next)
{
    int result = defval;
    size_t max_len = 0;

    if (str == NULL)
    {
        *next = NULL;
        return defval;
    }

    for (; map->name != NULL; map++)
    {
        size_t cur_len;

        if (!exact_match)
            cur_len = te_str_common_prefix(map->name, str);
        else
        {
            cur_len = strlen(map->name);
            if (strncmp(str, map->name, cur_len) != 0)
                cur_len = 0;
        }

        if (cur_len > max_len)
        {
            result = map->value;
            max_len = cur_len;
        }
    }

    /*
     * Here is the same problem of const/non-const output
     * pointers as with standard strtol() and similiar functions.
     */
    if (next != NULL)
        *next = TE_CONST_PTR_CAST(char, str + max_len);

    return result;
}

void
te_enum_map_fill_by_conversion(te_enum_map map[],
                               int minval, int maxval,
                               const char *(*val2str)(int val))
{
    int value;

    for (value = minval; value <= maxval; value++, map++)
    {
        map->value = value;
        map->name = val2str(value);
    }
    map->name = NULL;
}

int
te_enum_translate(const te_enum_trn trn[], int value, bool reverse,
                  int unknown_val)
{
    for (; trn->from != INT_MIN; trn++)
    {
        if (value == (reverse ? trn->to : trn->from))
            return reverse ? trn->from : trn->to;
    }
    return unknown_val;
}

te_errno
te_enum_bitmask_convert(const te_enum_bitmask_conv conv[],
                        uint64_t bm, bool reverse,
                        uint64_t *result)
{
    uint64_t bitmask = bm;
    uint64_t result_bm = 0;
    uint64_t check_mask;
    uint64_t from_converted = 0;
    uint64_t to_converted = 0;

    for (; conv->bits_from != UINT64_MAX; conv++)
    {
        if (conv->bits_from == 0 || conv->bits_to == 0 ||
            (from_converted & conv->bits_from) != 0 ||
            (to_converted & conv->bits_to) != 0)
        {
            return TE_EINVAL;
        }

        check_mask = reverse ? conv->bits_to : conv->bits_from;
        if ((bitmask & check_mask) == check_mask)
        {
            bitmask &= ~check_mask;
            result_bm |= reverse ? conv->bits_from : conv->bits_to;
        }

        from_converted |= conv->bits_from;
        to_converted |= conv->bits_to;
    }

    if (bitmask != 0)
        return TE_ERANGE;

    if (result != NULL)
        *result = result_bm;

    return 0;
}

void
te_enum_trn_fill_by_conversion(te_enum_trn trn[],
                               int minval, int maxval,
                               int (*val2val)(int val))
{
    int value;

    for (value = minval; value <= maxval; value++, trn++)
    {
        trn->from = value;
        trn->to = val2val(value);
    }
    trn->from = trn->to = INT_MIN;
}
