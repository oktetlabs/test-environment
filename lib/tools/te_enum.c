/** @file
 * @brief Generic mapping between names and integral values
 *
 * Implementation of the mapping functions.
 *
 * Copyright (C) 2003-2022 OKTET Labs. All rights reserved.
 *
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 *
 */

#include "te_config.h"
#include <stdlib.h>
#include <string.h>

#include "te_defs.h"
#include "te_enum.h"

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
te_enum_translate(const te_enum_trn trn[], int value, te_bool reverse,
                  int unknown_val)
{
    for (; trn->from != INT_MIN; trn++)
    {
        if (value == (reverse ? trn->to : trn->from))
            return reverse ? trn->from : trn->to;
    }
    return unknown_val;
}

int
te_enum_translate_bitmap(const te_enum_trn trn[], int value, te_bool reverse)
{
    int result = 0;

    if (value == 0)
        return te_enum_translate(trn, 0, reverse, 0);

    for (; trn->from != INT_MIN; trn++)
    {
        int mask = reverse ? trn->to : trn->from;
        int newval = reverse ? trn->from : trn->to;

        if (mask == 0)
            continue;

        if ((value & mask) == mask)
        {
            value &= ~mask;
            result |= newval;
        }
    }

    return result;
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
