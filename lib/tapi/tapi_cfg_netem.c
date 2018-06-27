/** @file
 * @brief Network Emulator configuration
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#include "te_str.h"
#include "te_stdint.h"
#include "rcf_api.h"
#include "logger_api.h"
#include "conf_api.h"
#include "tapi_cfg.h"

#include "tapi_cfg_qdisc.h"
#include "tapi_cfg_netem.h"

#define VALUE_SIZE      (64)
#define NETEM_PARAM_FMT \
    "/agent:%s/interface:%s/tc:/qdisc:/param:%s"

static te_bool
is_exist(const char *oid)
{
    return cfg_find_str(oid, NULL) == 0;
}

static te_errno
add_param(const char *oid, const char *value)
{
    return cfg_add_instance_str(oid, NULL, CFG_VAL(STRING, value));
}

/* See description in the tapi_cfg_qdisc.h */
te_errno
tapi_cfg_netem_get_param(const char *ta, const char *if_name,
                         const char *param, char **value)
{
    cfg_val_type type = CVT_STRING;

    if (ta == NULL || if_name == NULL || param == NULL || value == NULL)
        return TE_EINVAL;

    return cfg_get_instance_fmt(&type, value, NETEM_PARAM_FMT,
                                ta, if_name, param);
}

/* See description in the tapi_cfg_qdisc.h */
te_errno
tapi_cfg_netem_set_param(const char *ta, const char *if_name,
                         const char *param, const char *value)
{
    te_errno rc;
    char oid[CFG_OID_MAX];

    if (ta == NULL || if_name == NULL || param == NULL || value == NULL)
        return TE_EINVAL;

    snprintf(oid, CFG_OID_MAX, NETEM_PARAM_FMT, ta, if_name, param);

    if (!is_exist(oid) && (rc = add_param(oid, value)) != 0)
        return rc;
    else if ((rc = cfg_set_instance_str(CFG_VAL(STRING, value), oid)) != 0)
        return rc;

    return 0;
}

static te_errno
from_integer(uint32_t integer, char *string_value)
{
    snprintf(string_value, VALUE_SIZE, "%u", integer);
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

static te_errno
from_percent(double value, char *string_value)
{
    if (value < 0.0 || value > 100.0)
        return TE_EINVAL;

    snprintf(string_value, VALUE_SIZE, "%.2f%%", value);
    return 0;
}

static te_errno
to_percent(const char *string_value, double *value)
{
    char *endp;
    double result;

    if (string_value == NULL || value == NULL)
        return TE_EINVAL;

    result = strtod(string_value, &endp);
    if (endp != NULL && *endp == '%')
    {
        *value = result;
        return 0;
    }

    return TE_EINVAL;
}

/* Template get function */
#define R(_name, _type, _str2val) \
    te_errno tapi_cfg_netem_get_ ##_name(const char *ta,                \
                                         const char *if_name,           \
                                         _type *value)                  \
    {                                                                   \
        te_errno rc;                                                    \
        char *value_str = NULL;                                         \
                                                                        \
        rc = tapi_cfg_netem_get_param(ta, if_name, #_name, &value_str); \
        if (rc != 0)                                                    \
            return rc;                                                  \
                                                                        \
        rc = _str2val(value_str, value);                                \
        free(value_str);                                                \
        return rc;                                                      \
    }

/* Template set function */
#define W(_name, _type, _val2str) \
    te_errno tapi_cfg_netem_set_ ##_name(const char *ta,                    \
                                         const char *if_name,               \
                                         _type value)                       \
    {                                                                       \
        te_errno rc;                                                        \
        char value_str[VALUE_SIZE];                                         \
                                                                            \
        if ((rc = _val2str(value, value_str)) != 0)                         \
            return rc;                                                      \
                                                                            \
        return tapi_cfg_netem_set_param(ta, if_name, #_name, value_str);    \
    }

/* Template get/set function */
#define RW(_name, _type, _val2str, _str2val) \
    R(_name, _type, _str2val) \
    W(_name, _type, _val2str)

RW(delay,                  uint32_t, from_integer, to_integer)
RW(jitter,                 uint32_t, from_integer, to_integer)
RW(delay_correlation,      double,   from_percent, to_percent)
RW(loss,                   double,   from_percent, to_percent)
RW(loss_correlation,       double,   from_percent, to_percent)
RW(duplicate,              double,   from_percent, to_percent)
RW(duplicate_correlation,  double,   from_percent, to_percent)
RW(limit,                  uint32_t, from_integer, to_integer)
RW(gap,                    uint32_t, from_integer, to_integer)
RW(reorder_probability,    double,   from_percent, to_percent)
RW(reorder_correlation,    double,   from_percent, to_percent)
RW(corruption_probability, double,   from_percent, to_percent)
RW(corruption_correlation, double,   from_percent, to_percent)
