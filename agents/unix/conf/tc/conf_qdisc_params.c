/** @file
 * @brief Unix TA Traffic Control qdisc netem configuration support
 *
 * Implementation of get/set methods for qdisc netem node
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#include <linux/if_ether.h>

#include "te_defs.h"
#include "rcf_common.h"
#include "te_string.h"
#include "te_str.h"
#include "te_alloc.h"
#include "te_queue.h"
#include "logger_api.h"

#include "conf_qdisc_params.h"
#include "conf_tc_internal.h"
#include "conf_bpf_inner.h"
#include "conf_net_if_wrapper.h"

#include <netlink/route/qdisc/netem.h>
#include <netlink/route/qdisc/tbf.h>

typedef int (*netem_getter)(struct rtnl_qdisc *);
typedef void (*netem_setter)(struct rtnl_qdisc *, int);
typedef te_errno (*value_to_string_converter)(int, char *);
typedef te_errno (*string_to_value_converter)(const char *, int *);

typedef struct bpf_link_info
{
    LIST_ENTRY(bpf_link_info) next;
    char ifname[IFNAMSIZ];
    char prog[RCF_MAX_VAL];
} bpf_link_info;

LIST_HEAD(bpf_link_info_list, bpf_link_info);

typedef te_errno (*clsact_setter)(struct bpf_link_info_list *, const char *,
                                  const char *);
typedef void (*clsact_getter)(struct bpf_link_info_list *, const char *,
                              char *);

/*
 * Some old distros with kernel < 4.5 (e.g. Ubuntu 16.04) may have
 * TC_H_MIN_INGRESS and TC_H_MIN_EGRESS undefined. Handle this case and do
 * not fail the build.
 */
#if defined(TC_H_MIN_INGRESS)
typedef enum clsact_dir {
    CONF_CLSACT_INGRESS = TC_H_MIN_INGRESS,
    CONF_CLSACT_EGRESS = TC_H_MIN_EGRESS,
} clsact_dir;
#else

/* If TC_H_MIN_INGRESS is undefined, declare a dummy type to keep build pass. */
typedef enum clsact_dir {
    CONF_CLSACT_INGRESS,
    CONF_CLSACT_EGRESS,
} clsact_dir;

/*
 * If for some reason TC_H_CLSACT is defined, undefine it to prevent invalid
 * values usage in conf_qdisc_clsact_bpf_set().
 */
#ifdef TC_H_CLSACT
#undef TC_H_CLSACT
#endif /* TC_H_CLSACT */

#endif /* TC_H_MIN_INGRESS */

/* Kind of tc qdisc discipline */
typedef enum conf_qdisc_kind {
    CONF_QDISC_NETEM,
    CONF_QDISC_TBF,
    CONF_QDISC_CLSACT,
    CONF_QDISC_UNKNOWN,
} conf_qdisc_kind;

typedef struct tbf_params {
    LIST_ENTRY(tbf_params) next;

    char ifname[RCF_MAX_VAL];
    int bucket;
    int rate;
    int cell;
    int limit;
    int latency;
    int peakrate;
    int mtu;
} tbf_params;

static LIST_HEAD(, tbf_params) tbf_params_h = LIST_HEAD_INITIALIZER(tbf_params_h);

typedef int (*tbf_getter)(struct tbf_params *, struct rtnl_qdisc *);
typedef void (*tbf_setter)(struct tbf_params *, struct rtnl_qdisc *, int);

static struct tbf_params *
conf_qdisc_tbf_params_get(const char *ifname)
{
    struct tbf_params *p;

    LIST_FOREACH(p, &tbf_params_h, next)
    {
        if (strcmp(ifname, p->ifname) == 0)
            return p;
    }

    return NULL;
}

static te_errno
conf_qdisc_tbf_params_add(const char *ifname)
{
    struct tbf_params *params;

    params = TE_ALLOC(sizeof(*params));
    if (params == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    TE_STRLCPY(params->ifname, ifname, RCF_MAX_VAL);

    LIST_INSERT_HEAD(&tbf_params_h, params, next);

    return 0;
}

static conf_qdisc_kind
conf_qdisc_get_kind(struct rtnl_qdisc *qdisc)
{
    const char *kind;
    size_t i;
    static const char *qdisc_kind_names[] = {
        [CONF_QDISC_NETEM] = "netem",
        [CONF_QDISC_TBF] = "tbf",
        [CONF_QDISC_CLSACT] = "clsact",
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

static int
conf_qdisc_tbf_rate_get(struct tbf_params *params, struct rtnl_qdisc *qdisc)
{
    int rate = rtnl_qdisc_tbf_get_rate(qdisc);

    if (params->rate > 0 && rate != params->rate)
    {
        WARN("Returned value of TBF rate (%d) isn't equal to the one that was "
             "set (%d)", rate, params->rate);
        params->rate = rate;
    }
    return rate;
}

static void
conf_qdisc_tbf_rate_set(struct tbf_params *params, struct rtnl_qdisc *qdisc,
                        int val)
{
    rtnl_qdisc_tbf_set_rate(qdisc, val, params->bucket, params->cell);
    params->rate = val;
}

static int
conf_qdisc_tbf_bucket_get(struct tbf_params *params, struct rtnl_qdisc *qdisc)
{
    int bucket = rtnl_qdisc_tbf_get_rate_bucket(qdisc);

    if (params->bucket > 0 && bucket != params->bucket)
    {
        WARN("Returned value of TBF bucket (%d) isn't equal to the one that was "
             "set (%d)", bucket, params->bucket);
        params->bucket = bucket;
    }
    return bucket;
}

static void
conf_qdisc_tbf_bucket_set(struct tbf_params *params, struct rtnl_qdisc *qdisc,
                          int val)
{
    rtnl_qdisc_tbf_set_rate(qdisc, params->rate, val, params->cell);
    params->bucket = val;
}

static int
conf_qdisc_tbf_cell_get(struct tbf_params *params, struct rtnl_qdisc *qdisc)
{
    int cell = rtnl_qdisc_tbf_get_rate_cell(qdisc);

    if (params->cell > 0 && cell != params->cell)
    {
        WARN("Returned value of TBF cell (%d) isn't equal to the one that was "
             "set (%d)", cell, params->cell);
        params->cell = cell;
    }
    return cell;
}

static void
conf_qdisc_tbf_cell_set(struct tbf_params *params, struct rtnl_qdisc *qdisc,
                        int val)
{
    rtnl_qdisc_tbf_set_rate(qdisc, params->rate, params->bucket, val);
    params->cell = val;
}

static int
conf_qdisc_tbf_limit_get(struct tbf_params *params, struct rtnl_qdisc *qdisc)
{
    int limit = rtnl_qdisc_tbf_get_limit(qdisc);

    if (params->limit > 0 && limit != params->limit)
    {
        WARN("Returned value of TBF limit (%d) isn't equal to the one that was "
             "set (%d)", limit, params->limit);
        params->limit = limit;
    }
    return limit;
}

static void
conf_qdisc_tbf_limit_set(struct tbf_params *params, struct rtnl_qdisc *qdisc,
                         int val)
{
    rtnl_qdisc_tbf_set_limit(qdisc, val);
    params->limit = val;
}

static int
conf_qdisc_tbf_latency_get(struct tbf_params *params, struct rtnl_qdisc *qdisc)
{
    UNUSED(qdisc);
    return params->latency;
}

static void
conf_qdisc_tbf_latency_set(struct tbf_params *params, struct rtnl_qdisc *qdisc,
                           int val)
{
    rtnl_qdisc_tbf_set_limit_by_latency(qdisc, val);
    params->latency = val;
}

static int
conf_qdisc_tbf_peakrate_get(struct tbf_params *params, struct rtnl_qdisc *qdisc)
{
    int peakrate = rtnl_qdisc_tbf_get_peakrate(qdisc);

    if (params->peakrate > 0 && peakrate != params->peakrate)
    {
        WARN("Returned value of TBF peakrate (%d) isn't equal to the one that was "
             "set (%d)", peakrate, params->peakrate);
        params->peakrate = peakrate;
    }
    return peakrate;
}

static void
conf_qdisc_tbf_peakrate_set(struct tbf_params *params, struct rtnl_qdisc *qdisc,
                            int val)
{
    rtnl_qdisc_tbf_set_peakrate(qdisc, val, params->mtu, 8);
    params->peakrate = val;
}

static int
conf_qdisc_tbf_mtu_get(struct tbf_params *params, struct rtnl_qdisc *qdisc)
{
    int mtu = rtnl_qdisc_tbf_get_peakrate_bucket(qdisc);

    if (params->mtu > 0 && mtu != params->mtu)
    {
        WARN("Returned value of TBF mtu (%d) isn't equal to the one that was "
             "set (%d)", mtu, params->mtu);
        params->mtu = mtu;
    }
    return mtu;
}

static void
conf_qdisc_tbf_mtu_set(struct tbf_params *params, struct rtnl_qdisc *qdisc,
                       int val)
{
    rtnl_qdisc_tbf_set_peakrate(qdisc, params->peakrate, val, 8);
    params->mtu = val;
}

struct tbf_param {
    const char *name;
    tbf_getter get;
    tbf_setter set;
};

static struct tbf_param tbf_params_list[] = {
    {
        /* Rate bucket size */
        .name = "bucket",
        .get = conf_qdisc_tbf_bucket_get,
        .set = conf_qdisc_tbf_bucket_set
    },
    {
        /* Rate of tbf qdisc */
        .name = "rate",
        .get = conf_qdisc_tbf_rate_get,
        .set = conf_qdisc_tbf_rate_set
    },
    {
        /* Rate cell size */
        .name = "cell",
        .get = conf_qdisc_tbf_cell_get,
        .set = conf_qdisc_tbf_cell_set
    },
    {
        /* Limit of tbf qdisc */
        .name = "limit",
        .get = conf_qdisc_tbf_limit_get,
        .set = conf_qdisc_tbf_limit_set
    },
    {
        /* Limit of tbf qdisc by latency */
        .name = "latency",
        .get = conf_qdisc_tbf_latency_get,
        .set = conf_qdisc_tbf_latency_set
    },
    {
        /* Pealrate of tbf qdisc */
        .name = "peakrate",
        .get = conf_qdisc_tbf_peakrate_get,
        .set = conf_qdisc_tbf_peakrate_set
    },
    {
         /* Pealrate bucket size */
        .name = "mtu",
        .get = conf_qdisc_tbf_mtu_get,
        .set = conf_qdisc_tbf_mtu_set
    },
};

/**
 * Link BPF program @p prog_name to TC attach point on the
 * interface @p if_name, or unlink if @p prog_name is an empty
 * string.
 *
 * @param list          List of linked BPF TC programs.
 * @param if_name       The interface name.
 * @param prog_name     Oid of the BPF program, or empty string to unlink.
 * @param dir           Data path direction (ingress or egress).
 *
 * @return Status code.
 */
static te_errno
conf_qdisc_clsact_bpf_set(struct bpf_link_info_list *list,
                          const char *if_name,
                          const char *prog_name,
                          clsact_dir dir)
{
/*
 * Linux < 4.4 doesn't support tc BPF classifier (TCA_BPF_FLAG_ACT_DIRECT).
 * Linus < 4.5 doesn't support CLSACT qdisc (TC_H_CLSACT), but some old RHEL
 * kernels have it. So we need to check both defines to get a working function.
 */
#if defined(WITH_BPF) && defined(TCA_BPF_FLAG_ACT_DIRECT) && defined(TC_H_CLSACT)
    struct nl_msg *msg;
    struct tcmsg tchdr = {0};
    te_bool link = *prog_name != '\0';
    int err = 0;
    unsigned int protocol = 0;
    int cmd = 0;
    unsigned int flags = 0;
    bpf_link_info *item;
    te_bool iface_found = FALSE;

    /* Check that only one TC program is linked to the interface. */
    LIST_FOREACH(item, list, next)
    {
        if (strcmp(if_name, item->ifname) == 0)
        {
            if (link)
            {
                ERROR("Other BPF TC program \"%s\" is already linked "
                      "to the interface %s", item->prog, if_name);
                return TE_RC(TE_TA_UNIX, TE_EEXIST);
            }
            else
            {
                iface_found = TRUE;
            }
        }
    }

    /*
     * If there are no linked programs, we have nothing to do.
     * So just return success code.
     */
    if (!iface_found && !link)
        return 0;

    if (link)
    {
        protocol = htons(ETH_P_ALL);
        cmd = RTM_NEWTFILTER;
        flags = NLM_F_EXCL | NLM_F_CREATE;
    }
    else
    {
        cmd = RTM_DELTFILTER;
    }

    tchdr.tcm_family = AF_UNSPEC;
    tchdr.tcm_parent = TC_H_MAKE(TC_H_CLSACT, dir);
    tchdr.tcm_info = TC_H_MAKE(0, protocol);
    tchdr.tcm_ifindex = conf_net_if_wrapper_if_nametoindex(if_name);

    if ((msg = nlmsg_alloc_simple(cmd, flags)) == NULL)
    {
        ERROR("Failed to allocate netlink message");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    err = nlmsg_append(msg, &tchdr, sizeof(tchdr), NLMSG_ALIGNTO);
    if (err < 0)
    {
        ERROR("Failed to append tc header to netlink message: %s",
              nl_geterror(err));
        nlmsg_free(msg);
        return conf_tc_internal_nl_error2te_errno(err);
    }

    if (link)
    {
        int bpf_fd = -1;
        struct nlattr *opts;

        bpf_fd = conf_bpf_fd_by_prog_oid(prog_name);
        if (bpf_fd == -1)
        {
            ERROR("Failed to obtain BPF program descriptor");
            nlmsg_free(msg);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        err = nla_put_string(msg, TCA_KIND, "bpf");
        if (err < 0)
        {
            ERROR("Failed to add attribute \"bpf\" to netlink message: %s",
                  nl_geterror(err));
            nlmsg_free(msg);
            return conf_tc_internal_nl_error2te_errno(err);
        }

        if ((opts = nla_nest_start(msg, TCA_OPTIONS)) == NULL)
        {
            ERROR("Failed to add nester TCA_OPTIONS attribute to netlink "
                  "message");
            nlmsg_free(msg);
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }

        err = nla_put_u32(msg, TCA_BPF_FD, bpf_fd);
        if (err < 0)
        {
            ERROR("Failed to add attribute to netlink message: %s",
                  nl_geterror(err));
            nlmsg_free(msg);
            return conf_tc_internal_nl_error2te_errno(err);
        }

        err = nla_put_u32(msg, TCA_BPF_FLAGS, TCA_BPF_FLAG_ACT_DIRECT);
        if (err < 0)
        {
            ERROR("Failed to add attribute to netlink message: %s",
                  nl_geterror(err));
            nlmsg_free(msg);
            return conf_tc_internal_nl_error2te_errno(err);
        }

        nla_nest_end(msg, opts);
    }

    err = nl_send_sync(conf_tc_internal_get_sock(), msg);
    if (err != 0)
    {
        ERROR("Failed to send netlink message %s", nl_geterror(err));
        return conf_tc_internal_nl_error2te_errno(err);
    }

    if (link)
    {
        item = TE_ALLOC(sizeof(*item));
        if (item == NULL)
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);

        TE_STRLCPY(item->ifname, if_name, sizeof(item->ifname));
        TE_STRLCPY(item->prog, prog_name, sizeof(item->prog));

        LIST_INSERT_HEAD(list, item, next);
    }
    else
    {
        bpf_link_info *tmp;

        LIST_FOREACH_SAFE(item, list, next, tmp)
        {
            if (strcmp(if_name, item->ifname) == 0)
            {
                LIST_REMOVE(item, next);
                free(item);
            }
        }
    }

    return 0;
#else
    UNUSED(list);
    UNUSED(if_name);
    UNUSED(prog_name);
    UNUSED(dir);

    ERROR("BPF is not supported");
    return TE_RC(TE_TA_UNIX, TE_EINVAL);
#endif /* WITH_BPF && TCA_BPF_FLAG_ACT_DIRECT && TC_H_CLSACT */
}

/**
 * Get oid string of the BPF TC program linked to the
 * interface @p ifname.
 *
 * @param[in]  list     List of linked BPF TC ingress programs.
 * @param[in]  ifname   The interface name.
 * @param[out] val      Buffer to write the string to.
 */
static void
conf_qdisc_clsact_bpf_get(struct bpf_link_info_list *list, const char *ifname,
                          char *val)
{
    bpf_link_info *item;

    LIST_FOREACH(item, list, next)
    {
        if (strcmp(ifname, item->ifname) == 0)
        {
            TE_STRLCPY(val, item->prog, RCF_MAX_VAL);
            return;
        }
    }

    TE_STRLCPY(val, "", RCF_MAX_VAL);
}

static te_errno
conf_qdisc_clsact_bpf_ingress_set(struct bpf_link_info_list *list,
                                  const char *if_name,
                                  const char *prog_name)
{
    return conf_qdisc_clsact_bpf_set(list, if_name, prog_name,
                                     CONF_CLSACT_INGRESS);
}

static te_errno
conf_qdisc_clsact_bpf_egress_set(struct bpf_link_info_list *list,
                                 const char *if_name,
                                 const char *prog_name)
{
    return conf_qdisc_clsact_bpf_set(list, if_name, prog_name,
                                     CONF_CLSACT_EGRESS);
}

typedef struct clsact_param
{
    const char *name;
    struct bpf_link_info_list bpf_prog_list;
    clsact_setter set;
    clsact_getter get;
} clsact_param;

static clsact_param clsact_param_list[] = {
    {
        .name = "bpf_ingress",
        .bpf_prog_list = LIST_HEAD_INITIALIZER(.bpf_prog_list),
        .set = conf_qdisc_clsact_bpf_ingress_set,
        .get = conf_qdisc_clsact_bpf_get,
    },
    {
        .name = "bpf_egress",
        .bpf_prog_list = LIST_HEAD_INITIALIZER(.bpf_prog_list),
        .set = conf_qdisc_clsact_bpf_egress_set,
        .get = conf_qdisc_clsact_bpf_get,
    },
};

/* See the description in conf_qdisc_params.h */
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
    switch (qdisc_kind)
    {
        case CONF_QDISC_NETEM:
        {
            for (i = 0; i < TE_ARRAY_LEN(netem_params); i++)
            {
                if (strcmp(netem_params[i].name, param) != 0)
                    continue;
                return set_netem_value_with_qdisc(qdisc, netem_params + i, value);
            }
            break;
        }

        case CONF_QDISC_TBF:
        {
            int val;
            struct tbf_params *params;
            te_errno rc;

            if ((rc = default_str2val(value, &val)) != 0)
                return TE_RC(TE_TA_UNIX, rc);

            params = conf_qdisc_tbf_params_get(if_name);
            if (params == NULL)
                return TE_RC(TE_TA_UNIX, TE_ENOENT);

            for (i = 0; i < TE_ARRAY_LEN(tbf_params_list); i++)
            {
                if (strcmp(tbf_params_list[i].name, param) == 0)
                {
                    tbf_params_list[i].set(params, qdisc, val);
                    return 0;
                }
            }
            break;
        }

        case CONF_QDISC_CLSACT:
            for (i = 0; i < TE_ARRAY_LEN(clsact_param_list); i++)
            {
                if (strcmp(clsact_param_list[i].name, param) == 0)
                {
                    return clsact_param_list[i].set(
                                &clsact_param_list[i].bpf_prog_list,
                                if_name, value);
                }
            }
            break;

        default:
            break;
    }

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/* See the description in conf_qdisc_params.h */
te_errno
conf_qdisc_param_add(unsigned int gid, const char *oid,
                     const char *value, const char *if_name,
                     const char *tc, const char *qdisc_str,
                     const char *param)
{
    size_t i;
    struct rtnl_qdisc *qdisc = NULL;
    conf_qdisc_kind qdisc_kind;

    if ((qdisc = conf_tc_internal_get_qdisc(if_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    qdisc_kind = conf_qdisc_get_kind(qdisc);
    switch (qdisc_kind)
    {
        case CONF_QDISC_NETEM:
            for (i = 0; i < TE_ARRAY_LEN(netem_params); i++)
            {
                if (strcmp(netem_params[i].name, param) != 0)
                    continue;
                return set_netem_value_with_qdisc(qdisc, netem_params + i, value);
            }
            break;

        case CONF_QDISC_TBF:
            for (i = 0; i < TE_ARRAY_LEN(tbf_params_list); i++)
            {
                struct tbf_params *params;
                te_errno rc;

                if (strcmp(tbf_params_list[i].name, param) != 0)
                    continue;

                params = conf_qdisc_tbf_params_get(if_name);
                if (params == NULL)
                {
                    rc = conf_qdisc_tbf_params_add(if_name);
                    if (rc != 0)
                        return rc;
                }
                else if (strcmp(param, "limit") == 0)
                {
                    if (params->latency > 0)
                        return TE_RC(TE_TA_UNIX, TE_EEXIST);
                }
                else if (strcmp(param, "latency") == 0)
                {
                    if (params->limit > 0)
                        return TE_RC(TE_TA_UNIX, TE_EEXIST);
                }
                return conf_qdisc_param_set(gid, oid, value, if_name, tc,
                                            qdisc_str, param);
            }
            break;

        case CONF_QDISC_CLSACT:
            for (i = 0; i < TE_ARRAY_LEN(clsact_param_list); i++)
            {
                if (strcmp(clsact_param_list[i].name, param) == 0)
                {
                    return clsact_param_list[i].set(
                                &clsact_param_list[i].bpf_prog_list,
                                if_name, value);
                }
            }
            break;

        default:
            break;
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
    switch (qdisc_kind)
    {
        case CONF_QDISC_NETEM:
        {
            for (i = 0; i < TE_ARRAY_LEN(netem_params); i++)
            {
                if (strcmp(netem_params[i].name, param) != 0)
                    continue;
                return get_netem_value_with_qdisc(qdisc, netem_params + i, value);
            }
            break;
        }

        case CONF_QDISC_TBF:
        {
            int val = 0;
            struct tbf_params *params = conf_qdisc_tbf_params_get(if_name);

            if (params == NULL)
                return TE_RC(TE_TA_UNIX, TE_ENOENT);

            for (i = 0; i < TE_ARRAY_LEN(tbf_params_list); i++)
            {
                if (strcmp(tbf_params_list[i].name, param) == 0)
                {
                    val = tbf_params_list[i].get(params, qdisc);
                    snprintf(value, RCF_MAX_VAL, "%d", val);
                    return 0;
                }
            }
            break;
        }

        case CONF_QDISC_CLSACT:
            for (i = 0; i < TE_ARRAY_LEN(clsact_param_list); i++)
            {
                if (strcmp(clsact_param_list[i].name, param) == 0)
                {
                    clsact_param_list[i].get(&clsact_param_list[i].bpf_prog_list,
                                             if_name, value);
                    return 0;
                }
            }
            break;

        default:
            break;
    }

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/* See the description in conf_qdisc_params.h */
te_errno
conf_qdisc_param_del(unsigned int gid, const char *oid,
                     const char *if_name, const char *tc,
                     const char *qdisc, const char *param)
{
    size_t i;
    struct rtnl_qdisc *nl_qdisc = NULL;
    conf_qdisc_kind qdisc_kind;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(tc);
    UNUSED(qdisc);

    if ((nl_qdisc = conf_tc_internal_get_qdisc(if_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    qdisc_kind = conf_qdisc_get_kind(nl_qdisc);
    switch (qdisc_kind)
    {
        case CONF_QDISC_CLSACT:
            for (i = 0; i < TE_ARRAY_LEN(clsact_param_list); i++)
            {
                if (strcmp(clsact_param_list[i].name, param) == 0)
                {

                    return clsact_param_list[i].set(
                                &clsact_param_list[i].bpf_prog_list,
                                if_name, "");
                }
            }
            break;

        default:
            break;
    }

    return 0;
}

/* See the description in conf_qdisc_params.h */
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

    /*
     * Return empty list if qdisc is not available or if it is disabled.
     * Bug 11795: we should not list parameters for disabled qdisc as they may
     * appear after the nodes removal and lead to configuration backup issues.
     */
    if (qdisc == NULL || rtnl_tc_get_handle(TC_CAST(qdisc)) == 0)
    {
        *list = list_out.ptr;
        return 0;
    }

    qdisc_kind = conf_qdisc_get_kind(qdisc);
    switch (qdisc_kind)
    {
        case CONF_QDISC_NETEM:
            for (i = 0; i < TE_ARRAY_LEN(netem_params); i++)
            {
                int value = netem_params[i].get(qdisc);

                if (value == -NLE_NOATTR || value == 0)
                    continue;

                rc = te_string_append(&list_out, "%s ", netem_params[i].name);
                if (rc != 0)
                {
                    te_string_free(&list_out);
                    return rc;
                }
            }
            break;

        case CONF_QDISC_TBF:
            for (i = 0; i < TE_ARRAY_LEN(tbf_params_list); i++)
            {
                rc = te_string_append(&list_out, "%s ", tbf_params_list[i].name);
                if (rc != 0)
                {
                    te_string_free(&list_out);
                    return rc;
                }
            }
            break;

        case CONF_QDISC_CLSACT:
            for (i = 0; i < TE_ARRAY_LEN(clsact_param_list); i++)
            {
                rc = te_string_append(&list_out, "%s ", clsact_param_list[i].name);
                if (rc != 0)
                {
                    te_string_free(&list_out);
                    return rc;
                }
            }
            break;

        default:
            break;
    }

    *list = list_out.ptr;
    return 0;
}

/* See the description in conf_qdisc_params.h */
void
conf_qdisc_tbf_params_free(void)
{
    struct tbf_params *params;
    struct tbf_params *tmp;

    LIST_FOREACH_SAFE(params, &tbf_params_h, next, tmp)
    {
        LIST_REMOVE(params, next);
        free(params);
    }
}

/* See the description in conf_qdisc_params.h */
void
conf_qdisc_clsact_params_free(void)
{
    bpf_link_info  *item;
    bpf_link_info  *tmp;
    int             i;

    for (i = 0; i < TE_ARRAY_LEN(clsact_param_list); i++)
    {
        LIST_FOREACH_SAFE(item,
                          &clsact_param_list[i].bpf_prog_list,
                          next, tmp)
        {
            LIST_REMOVE(item, next);
            free(item);
        }
    }
}
