/** @file
 * @brief Unix TA Traffic Control qdisc netem configuration support
 *
 * Implementation of get/set methods for qdisc netem node
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#include "te_defs.h"
#include "rcf_common.h"
#include "te_string.h"
#include "te_str.h"

#include "conf_netem.h"
#include "conf_tc_internal.h"

#include <netlink/route/qdisc/netem.h>

typedef int (*netem_getter)(struct rtnl_qdisc *);
typedef void (*netem_setter)(struct rtnl_qdisc *, int);
typedef te_errno (*value_to_string_converter)(int, char *);
typedef te_errno (*string_to_value_converter)(const char *, int *);

/* Kind of tc qdisc discipline */
typedef enum conf_qdisc_kind {
    CONF_QDISC_NETEM,
    CONF_QDISC_UNKNOWN,
} conf_qdisc_kind;

static conf_qdisc_kind
conf_qdisc_get_kind(struct rtnl_qdisc *qdisc)
{
    const char *kind;
    size_t i;
    static const char *qdisc_kind_names[] = {
        [CONF_QDISC_NETEM] = "netem",
        [CONF_QDISC_UNKNOWN] = NULL
    };

    kind = rtnl_tc_get_kind(TC_CAST(qdisc));
    if (kind == NULL)
        return CONF_QDISC_UNKNOWN;

    for (i = 0; i < TE_ARRAY_LEN(qdisc_kind_names); i++)
    {
        if (qdisc_kind_names[i] != NULL &&
            strcmp(qdisc_kind_names[i], kind) == 0)
        {
            return i;
        }
    }

    return CONF_QDISC_UNKNOWN;
}

static te_errno
default_val2str(int value, char *string)
{
    snprintf(string, RCF_MAX_VAL, "%d", value);
    return 0;
}

static te_errno
default_str2val(const char *string, int *value)
{
    te_errno rc;
    long int result;

    if ((rc = te_strtol(string, 10, &result)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if (result < INT_MIN || result > INT_MAX )
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *value = (int) result;
    return 0;
}

static te_errno
prob_val2str(int value, char *string)
{
    double prob = (uint32_t)(value) / (double)(NL_PROB_MAX);
    snprintf(string, RCF_MAX_VAL, "%.2f%%", 100.0 * prob);
    return 0;
}

static te_errno
prob_str2val(const char *string, int *value)
{
    long result = nl_prob2int(string);

    if (result < 0)
        return conf_tc_internal_nl_error2te_errno((int) result);

    *value = (int) result;
    return 0;
}

struct netem_param {
    const char *name;
    netem_getter get;
    netem_setter set;
    value_to_string_converter val2str;
    string_to_value_converter str2val;
};

static struct netem_param netem_params[] = {
    /* Packet Delay */
    {
        .name = "delay",
        .get = rtnl_netem_get_delay,
        .set = rtnl_netem_set_delay,
        .val2str = default_val2str,
        .str2val = default_str2val
    },
    {
        .name = "jitter",
        .get = rtnl_netem_get_jitter,
        .set = rtnl_netem_set_jitter,
        .val2str = default_val2str,
        .str2val = default_str2val
    },
    {
        .name = "delay_correlation",
        .get = rtnl_netem_get_delay_correlation,
        .set = rtnl_netem_set_delay_correlation,
        .val2str = prob_val2str,
        .str2val = prob_str2val
    },
    /* Packet Loss */
    {
        .name = "loss",
        .get = rtnl_netem_get_loss,
        .set = rtnl_netem_set_loss,
        .val2str = prob_val2str,
        .str2val = prob_str2val
    },
    {
        .name = "loss_correlation",
        .get = rtnl_netem_get_loss_correlation,
        .set = rtnl_netem_set_loss_correlation,
        .val2str = prob_val2str,
        .str2val = prob_str2val
    },
    /* Packet Duplication */
    {
        .name = "duplicate",
        .get = rtnl_netem_get_duplicate,
        .set = rtnl_netem_set_duplicate,
        .val2str = default_val2str,
        .str2val = default_str2val
    },
    {
        .name = "duplicate_correlation",
        .get = rtnl_netem_get_duplicate_correlation,
        .set = rtnl_netem_set_duplicate_correlation,
        .val2str = prob_val2str,
        .str2val = prob_str2val
    },
    /* Queue Limit */
    {
        .name = "limit",
        .get = rtnl_netem_get_limit,
        .set = rtnl_netem_set_limit,
        .val2str = default_val2str,
        .str2val = default_str2val
    },
    /* Packet Re-ordering */
    {
        .name = "gap",
        .get = rtnl_netem_get_gap,
        .set = rtnl_netem_set_gap,
        .val2str = default_val2str,
        .str2val = default_str2val
    },
    {
        .name = "reorder_probability",
        .get = rtnl_netem_get_reorder_probability,
        .set = rtnl_netem_set_reorder_probability,
        .val2str = prob_val2str,
        .str2val = prob_str2val
    },
    {
        .name = "reorder_correlation",
        .get = rtnl_netem_get_reorder_correlation,
        .set = rtnl_netem_set_reorder_correlation,
        .val2str = prob_val2str,
        .str2val = prob_str2val
    },
    /* Corruption */
    {
        .name = "corruption_probability",
        .get = rtnl_netem_get_corruption_probability,
        .set = rtnl_netem_set_corruption_probability,
        .val2str = prob_val2str,
        .str2val = prob_str2val
    },
    {
        .name = "corruption_correlation",
        .get = rtnl_netem_get_corruption_correlation,
        .set = rtnl_netem_set_corruption_correlation,
        .val2str = prob_val2str,
        .str2val = prob_str2val
    },
};

static te_errno
get_netem_value_with_qdisc(struct rtnl_qdisc *qdisc, struct netem_param *param,
                           char *value)
{
    int rc;

    if ((rc = param->val2str(param->get(qdisc), value)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    return 0;
}

static te_errno
set_netem_value_with_qdisc(struct rtnl_qdisc *qdisc, struct netem_param *param,
                           const char *value)
{
    te_errno rc;
    int val = 0;

    if ((rc = param->str2val(value, &val)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    param->set(qdisc, val);
    return 0;
}

/* See the description in conf_netem.h */
te_errno
conf_qdisc_param_set(unsigned int gid, const char *oid,
                     const char *value, const char *if_name,
                     const char *tc, const char *qdisc_str,
                     const char *param)
{
    size_t i;
    struct rtnl_qdisc *qdisc = NULL;
    conf_qdisc_kind qdisc_kind;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tc);
    UNUSED(qdisc_str);

    if ((qdisc = conf_tc_internal_get_qdisc(if_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    qdisc_kind = conf_qdisc_get_kind(qdisc);

    if (qdisc_kind == CONF_QDISC_NETEM)
    {
        for (i = 0; i < TE_ARRAY_LEN(netem_params); i++)
        {
            if (strcmp(netem_params[i].name, param) != 0)
                continue;
            return set_netem_value_with_qdisc(qdisc, netem_params + i, value);
        }
    }

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/* See the description in conf_netem.h */
te_errno
conf_qdisc_param_add(unsigned int gid, const char *oid,
                     const char *value, const char *if_name,
                     const char *tc, const char *qdisc_str,
                     const char *param)
{
    size_t i;
    struct rtnl_qdisc *qdisc = NULL;
    conf_qdisc_kind qdisc_kind;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tc);
    UNUSED(qdisc_str);

    if ((qdisc = conf_tc_internal_get_qdisc(if_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    qdisc_kind = conf_qdisc_get_kind(qdisc);
    if (qdisc_kind == CONF_QDISC_NETEM)
    {
        for (i = 0; i < TE_ARRAY_LEN(netem_params); i++)
        {
            if (strcmp(netem_params[i].name, param) != 0)
                continue;
            return set_netem_value_with_qdisc(qdisc, netem_params + i, value);
        }
    }

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

te_errno
conf_qdisc_param_get(unsigned int gid, const char *oid,
                     char *value, const char *if_name,
                     const char *tc, const char *qdisc_str,
                     const char *param)
{
    size_t i;
    struct rtnl_qdisc *qdisc = NULL;
    conf_qdisc_kind qdisc_kind;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tc);
    UNUSED(qdisc_str);

    if ((qdisc = conf_tc_internal_get_qdisc(if_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    qdisc_kind = conf_qdisc_get_kind(qdisc);

    if (qdisc_kind == CONF_QDISC_NETEM)
    {
        for (i = 0; i < TE_ARRAY_LEN(netem_params); i++)
        {
            if (strcmp(netem_params[i].name, param) != 0)
                continue;
            return get_netem_value_with_qdisc(qdisc, netem_params + i, value);
        }
    }

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/* See the description in conf_netem.h */
te_errno
conf_qdisc_param_del(unsigned int gid, const char *oid,
                     const char *value)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    return 0;
}

/* See the description in conf_netem.h */
te_errno
conf_qdisc_param_list(unsigned int gid, const char *oid,
                      const char *sub_id, char **list,
                      const char *if_name)
{
    te_errno rc;
    size_t i;
    te_string list_out = TE_STRING_INIT;
    struct rtnl_qdisc *qdisc = conf_tc_internal_get_qdisc(if_name);
    conf_qdisc_kind qdisc_kind;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    if ((rc = te_string_append(&list_out, " ")) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    if (qdisc == NULL)
    {
        *list = list_out.ptr;
        return 0;
    }

    qdisc_kind = conf_qdisc_get_kind(qdisc);

    if (qdisc_kind == CONF_QDISC_NETEM)
    {
        for (i = 0; i < TE_ARRAY_LEN(netem_params); i++)
        {
            int value = netem_params[i].get(qdisc);
            if (value == -NLE_NOATTR || value == 0)
                continue;

            te_string_append(&list_out, "%s ", netem_params[i].name);
        }
    }

    *list = list_out.ptr;
    return 0;
}
