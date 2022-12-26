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

void
te_intset_generic_add_range(const te_intset_ops *ops, void *val,
                            int first, int last)
{
    int i;

    for (i = first; i <= last; i++)
        ops->set(i, val);
}

void
te_intset_generic_remove_range(const te_intset_ops *ops, void *val,
                               int first, int last)
{
    int i;

    for (i = first; i <= last; i++)
        ops->unset(i, val);
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

static void
unset_uint64_bit(int v, void *val)
{
    *(uint64_t *)val &= ~(1ull << v);
}

static te_bool
check_uint64_bit(int v, const void *val)
{
    return (*(const uint64_t *)val & (1ull << v)) != 0;
}

const te_intset_ops te_bits_intset = {
    .clear = clear_uint64,
    .set = set_uint64_bit,
    .unset = unset_uint64_bit,
    .check = check_uint64_bit,
};

static void
clear_charset(void *val)
{
    memset(val, 0, sizeof(te_charset));
}

static te_bool
check_charset_byte(int v, const void *val)
{
    const te_charset *cs = val;

    return (cs->items[v / TE_BITSIZE(uint64_t)] &
            (1ull << (v % TE_BITSIZE(uint64_t)))) != 0;
}

static void
set_charset_byte(int v, void *val)
{
    te_charset *cs = val;

    if (!check_charset_byte(v, val))
    {
        cs->items[v / TE_BITSIZE(uint64_t)] |=
            1ull << (v % TE_BITSIZE(uint64_t));
        cs->n_items++;
    }
}

static void
unset_charset_byte(int v, void *val)
{
    te_charset *cs = val;

    if (check_charset_byte(v, val))
    {
        cs->items[v / TE_BITSIZE(uint64_t)] &=
            ~(1ull << (v % TE_BITSIZE(uint64_t)));
        cs->n_items--;
    }
}

const te_intset_ops te_charset_intset = {
    .clear = clear_charset,
    .set = set_charset_byte,
    .unset = unset_charset_byte,
    .check = check_charset_byte,
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

static void
fd_set_unset(int v, void *val)
{
    FD_CLR(v, (fd_set *)val);
}

static te_bool
fd_set_check(int v, const void *val)
{
    return FD_ISSET(v, (const fd_set *)val);
}

const te_intset_ops te_fdset_intset = {
    .clear = fd_set_clear,
    .set = fd_set_set,
    .unset = fd_set_unset,
    .check = fd_set_check,
};

#if HAVE_SCHED_SETAFFINITY

static void
cpu_set_clear(void *val)
{
    CPU_ZERO((cpu_set_t *)val);
}

static void
cpu_set_set(int v, void *val)
{
    CPU_SET(v, (cpu_set_t *)val);
}

static void
cpu_set_unset(int v, void *val)
{
    CPU_CLR(v, (cpu_set_t *)val);
}

static te_bool
cpu_set_check(int v, const void *val)
{
    return CPU_ISSET(v, (const cpu_set_t *)val);
}

const te_intset_ops te_cpuset_intset = {
    .clear = cpu_set_clear,
    .set = cpu_set_set,
    .unset = cpu_set_unset,
    .check = cpu_set_check,
};

#endif /* HAVE_SCHED_SETAFFINITY */
