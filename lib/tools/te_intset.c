/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Generic API to operate on integral sets.
 *
 * Copyright (C) 2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER  "Tools"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "te_str.h"
#include "te_string.h"
#include "te_intset.h"

te_errno
te_intset_generic_parse(const te_intset_ops *ops, int minval, int maxval,
                        const char *str, void *val)
{
    if (str == NULL || val == NULL)
        return TE_EFAULT;

    ops->clear(val);
    while (*str != '\0')
    {
        te_errno rc;
        char *end;
        int item;

        rc = te_strtoi_range_raw(str, minval, maxval,
                                 &end, 0, &item);
        if (rc != 0)
            return rc;
        if (*end == '-')
        {
            int item2;

            rc = te_strtoi_range_raw(end + 1, minval, maxval,
                                     &end, 0, &item2);
            if (rc != 0)
                return rc;

            if (item2 < item)
            {
                ERROR("%s(): empty range %d..%d", __func__, item, item2);
                return TE_EINVAL;
            }
            for (; item <= item2; item++)
                ops->set(item, val);
        }
        else
        {
            ops->set(item, val);
        }
        if (*end == ',')
            end++;
        str = end;
    }
    return 0;
}

static void
int_range2string(te_string *str, int minval, int maxval)
{
    if (str->len > 0)
        te_string_append(str, ",");

    if (minval == maxval)
        te_string_append(str, "%d", minval);
    else
        te_string_append(str, "%d-%d", minval, maxval);
}

char *
te_intset_generic2string(const te_intset_ops *ops, int minval, int maxval,
                         const void *val)
{
    te_string result = TE_STRING_INIT;
    int i;
    unsigned range;

    if (val == NULL)
        return NULL;

    /* Otherwise an empty set would yield a NULL */
    te_string_append(&result, "");

    assert(maxval != INT_MAX);
    for (i = minval, range = 0; i <= maxval; i++)
    {
        if (ops->check(i, val))
            range++;
        else if (range > 0)
        {
            int_range2string(&result, i - range, i - 1);
            range = 0;
        }
    }
    if (range > 0)
        int_range2string(&result, maxval - range + 1, maxval);

    return result.ptr;
}

te_bool
te_intset_generic_is_subset(const te_intset_ops *ops,
                            int minval, int maxval,
                            const void *subset, const void *superset)
{
    int i;

    for (i = minval; i <= maxval; i++)
    {
        if (ops->check(i, subset) &&
            !ops->check(i, superset))
        {
            return FALSE;
        }
    }

    return TRUE;
}

static void
clear_uint64(void *val)
{
    *(uint64_t *)val = 0;
}

static void
set_uint64_bit(int v, void *val)
{
    *(uint64_t *)val |= 1ull << v;
}

static te_bool
check_uint64_bit(int v, const void *val)
{
    return (*(const uint64_t *)val & (1ull << v)) != 0;
}

const te_intset_ops te_bits_intset = {
    .clear = clear_uint64,
    .set = set_uint64_bit,
    .check = check_uint64_bit,
};

static void
fd_set_clear(void *val)
{
    FD_ZERO((fd_set *)val);
}

static void
fd_set_set(int v, void *val)
{
    FD_SET(v, (fd_set *)val);
}

static te_bool
fd_set_check(int v, const void *val)
{
    return FD_ISSET(v, (const fd_set *)val);
}

const te_intset_ops te_fdset_intset = {
    .clear = fd_set_clear,
    .set = fd_set_set,
    .check = fd_set_check,
};
