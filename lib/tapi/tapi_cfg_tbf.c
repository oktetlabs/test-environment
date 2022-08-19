/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief tc qdisc TBF configuration
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_str.h"
#include "te_stdint.h"
#include "rcf_api.h"

#include "tapi_cfg_qdisc.h"
#include "tapi_cfg_tbf.h"

static te_errno
from_integer(uint32_t integer, char *string_value)
{
    snprintf(string_value, RCF_MAX_VAL, "%u", integer);
    return 0;
}

static te_errno
to_integer(const char *string_value, uint32_t *integer)
{
    te_errno rc;
    unsigned long result;

    if ((rc = te_strtoul(string_value, 10, &result)) != 0)
        return rc;

    *integer = (uint32_t) result;
    return 0;
}

/* Template get function */
#define R(_name, _type, _str2val) \
    te_errno tapi_cfg_tbf_get_ ##_name(const char *ta,                  \
                                       const char *if_name,             \
                                       _type *value)                    \
    {                                                                   \
        te_errno rc;                                                    \
        char *value_str = NULL;                                         \
                                                                        \
        rc = tapi_cfg_qdisc_get_param(ta, if_name, #_name, &value_str); \
        if (rc != 0)                                                    \
            return rc;                                                  \
                                                                        \
        rc = _str2val(value_str, value);                                \
        free(value_str);                                                \
        return rc;                                                      \
    }

/* Template set function */
#define W(_name, _type, _val2str) \
    te_errno tapi_cfg_tbf_set_ ##_name(const char *ta,                      \
                                       const char *if_name,                 \
                                       _type value)                         \
    {                                                                       \
        te_errno rc;                                                        \
        char value_str[RCF_MAX_VAL];                                        \
                                                                            \
        if ((rc = _val2str(value, value_str)) != 0)                         \
            return rc;                                                      \
                                                                            \
        return tapi_cfg_qdisc_set_param(ta, if_name, #_name, value_str);    \
    }

/* Template get/set function */
#define RW(_name, _type, _val2str, _str2val) \
    R(_name, _type, _str2val) \
    W(_name, _type, _val2str)

RW(rate,    uint32_t, from_integer, to_integer)
RW(bucket,  uint32_t, from_integer, to_integer)
RW(cell,    uint32_t, from_integer, to_integer)
RW(limit,   uint32_t, from_integer, to_integer)
RW(latency, uint32_t, from_integer, to_integer)
RW(peakrate,uint32_t, from_integer, to_integer)
RW(mtu,     uint32_t, from_integer, to_integer)
