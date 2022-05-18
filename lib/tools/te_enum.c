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
te_enum_map_from_value(const te_enum_map map[], int value)
{
    for (; map->name != NULL; map++)
    {
        if (map->value == value)
            return map->name;
    }
    abort();
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
