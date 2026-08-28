/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Network statistics
 *
 * Unix TA network statistics support
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Conf Net Stats"

#include "te_config.h"
#include "config.h"

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "te_alloc.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "rcf_pch_tree.h"
#include "logger_api.h"
#include "unix_internal.h"
#include "te_shell_cmd.h"

#ifndef IF_NAMESIZE
#define IF_NAMESIZE IFNAMSIZ
#endif

#ifdef U64_FMT
#undef U64_FMT
#endif

#ifdef _U64_FMT
#undef _U64_FMT
#endif

#ifdef I64_FMT
#undef I64_FMT
#endif

#ifdef _I64_FMT
#undef _I64_FMT
#endif

#define U64_FMT "%" TE_PRINTF_64 "u"
#define I64_FMT "%" TE_PRINTF_64 "u"

#define _U64_FMT " " U64_FMT
#define _I64_FMT " " I64_FMT

/*
 * Data model based on IF-MIB
 * https://www.rfc-editor.org/rfc/rfc2863.html
 */
typedef struct if_stats {
    uint64_t      in_octets;
    uint64_t      in_ucast_pkts;
    uint64_t      in_nucast_pkts;
    uint64_t      in_discards;
    uint64_t      in_errors;
    uint64_t      in_unknown_protos;
    uint64_t      out_octets;
    uint64_t      out_ucast_pkts;
    uint64_t      out_nucast_pkts;
    uint64_t      out_discards;
    uint64_t      out_errors;
} if_stats;

/* Interface stats counters from /proc/net/dev */
typedef struct linux_if_stats {
    uint64_t      rx_bytes;
    uint64_t      rx_packets;
    uint64_t      rx_errs;
    uint64_t      rx_drop;
    uint64_t      rx_fifo;
    uint64_t      rx_frame;
    uint64_t      rx_compressed;
    uint64_t      rx_multicast;
    uint64_t      tx_bytes;
    uint64_t      tx_packets;
    uint64_t      tx_errs;
    uint64_t      tx_drop;
    uint64_t      tx_fifo;
    uint64_t      tx_colls;
    uint64_t      tx_carrier;
    uint64_t      tx_compressed;
} linux_if_stats;

typedef struct net_stats_ipv4{
    uint64_t      in_recvs;
    uint64_t      in_hdr_errs;
    uint64_t      in_addr_errs;
    uint64_t      forw_dgrams;
    uint64_t      in_unknown_protos;
    uint64_t      in_discards;
    uint64_t      in_delivers;
    uint64_t      out_requests;
    uint64_t      out_discards;
    uint64_t      out_no_routes;
    uint64_t      reasm_timeout;
    uint64_t      reasm_reqds;
    uint64_t      reasm_oks;
    uint64_t      reasm_fails;
    uint64_t      frag_oks;
    uint64_t      frag_fails;
    uint64_t      frag_creates;
} net_stats_ipv4;

typedef struct net_stats_icmp {
    uint64_t      in_msgs;
    uint64_t      in_errs;
    uint64_t      in_dest_unreachs;
    uint64_t      in_time_excds;
    uint64_t      in_parm_probs;
    uint64_t      in_src_quenchs;
    uint64_t      in_redirects;
    uint64_t      in_echos;
    uint64_t      in_echo_reps;
    uint64_t      in_timestamps;
    uint64_t      in_timestamp_reps;
    uint64_t      in_addr_masks;
    uint64_t      in_addr_mask_reps;

    uint64_t      out_msgs;
    uint64_t      out_errs;
    uint64_t      out_dest_unreachs;
    uint64_t      out_time_excds;
    uint64_t      out_parm_probs;
    uint64_t      out_src_quenchs;
    uint64_t      out_redirects;
    uint64_t      out_echos;
    uint64_t      out_echo_reps;
    uint64_t      out_timestamps;
    uint64_t      out_timestamp_reps;
    uint64_t      out_addr_masks;
    uint64_t      out_addr_mask_reps;
} net_stats_icmp;

typedef struct net_stats {
    net_stats_ipv4  ipv4;
    net_stats_icmp  icmp;
} net_stats;


static te_errno
dev_stats_get(const char *devname, if_stats *stats)
{
    int   rc = 0;
#if __linux__
    char *buf = NULL;
    char *ptr = NULL;
    FILE *devf = NULL;
    int   line = 0;

    linux_if_stats linux_stats;
#endif

    memset(stats, 0, sizeof(*stats));

    VERB("dev_stats_get(devname=\"%s\") started", devname);

#if __linux__
    if ((devname == NULL) || (stats == NULL))
    {
        return TE_OS_RC(TE_TA_UNIX, EINVAL);
    }

#define STATS_NET_DEV_PROC_LINE_LEN 1024

    buf = TE_ALLOC(STATS_NET_DEV_PROC_LINE_LEN);

    VERB("Try to open /proc/net/dev file");

    if ((devf = fopen("/proc/net/dev", "r")) == NULL)
    {
        ERROR("Cannot open() /proc/net/dev");
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        goto cleanup;
    }

#define STATS_NET_DEV_LINES_TO_SKIP 2

    for (line = 0; line < STATS_NET_DEV_LINES_TO_SKIP; line++)
    {
        if (fgets(buf, STATS_NET_DEV_PROC_LINE_LEN, devf) == NULL)
        {
            ERROR("Invalid /proc/net/dev file format");
            rc = TE_OS_RC(TE_TA_UNIX, EINVAL);
            goto cleanup;
        }
    }

    ptr = NULL;

    for (;; line++)
    {
        if (fgets(buf, STATS_NET_DEV_PROC_LINE_LEN, devf) == NULL)
            break;

        VERB("/proc/net/dev: line %d: >%s", line, buf);

        if ((ptr = strstr(buf, devname)) != NULL)
        {
            if (*(ptr + strlen(devname)) == ':')
                break;
            else
                ptr = NULL;
        }
    }


    if (ptr != NULL)
    {
#define LINUX_IF_STATS_COUNT   16

        static const char *stats_net_dev_fmt =
            U64_FMT U64_FMT U64_FMT U64_FMT
            U64_FMT U64_FMT U64_FMT U64_FMT
            U64_FMT U64_FMT U64_FMT U64_FMT
            U64_FMT U64_FMT U64_FMT U64_FMT;

        VERB("Found line %d: >%s", line, buf);
        ptr += strlen(devname) + 1;
        if ((rc = sscanf(ptr, stats_net_dev_fmt,
                         &linux_stats.rx_bytes,
                         &linux_stats.rx_packets,
                         &linux_stats.rx_errs,
                         &linux_stats.rx_drop,
                         &linux_stats.rx_fifo,
                         &linux_stats.rx_frame,
                         &linux_stats.rx_compressed,
                         &linux_stats.rx_multicast,
                         &linux_stats.tx_bytes,
                         &linux_stats.tx_packets,
                         &linux_stats.tx_errs,
                         &linux_stats.tx_drop,
                         &linux_stats.tx_fifo,
                         &linux_stats.tx_colls,
                         &linux_stats.tx_carrier,
                         &linux_stats.tx_compressed)) != LINUX_IF_STATS_COUNT)
        {
            ERROR("Invalid /proc/net/dev file format, "
                  "only %d of %d counters are parsed",
                  rc, LINUX_IF_STATS_COUNT);
            return TE_OS_RC(TE_TA_UNIX, EINVAL);
        }

        stats->in_octets = linux_stats.rx_bytes;
        stats->in_ucast_pkts = linux_stats.rx_packets -
                               linux_stats.rx_multicast;
        stats->in_nucast_pkts = linux_stats.rx_multicast;
        stats->in_discards = linux_stats.rx_drop;
        stats->in_errors = linux_stats.rx_errs;

        stats->out_octets = linux_stats.tx_bytes;
        stats->out_ucast_pkts = linux_stats.tx_packets;
        stats->out_discards = linux_stats.tx_drop;
        stats->out_errors = linux_stats.tx_errs;

        /*
         * Due to differences between IF-MIB and /proc/net/dev fields,
         * these counters are not calculated.
         */
        stats->in_unknown_protos = 0;
        stats->out_nucast_pkts = 0;
#undef LINUX_IF_STATS_COUNT
    }
#endif

    rc = 0;


#if __linux__
cleanup:
    if (devf != NULL)
        fclose(devf);

    if (buf != NULL)
        free(buf);
#endif

    return rc;
}




#define MAX_PROC_NET_SNMP_SIZE  4096

static te_errno
net_stats_get(net_stats *stats)
{
#if __linux__

#define STATS_SNMP_IPV4_PARAM_COUNT     19

    static const char *stats_net_snmp_ipv4_fmt =
        "Ip:"
        I64_FMT I64_FMT U64_FMT U64_FMT U64_FMT U64_FMT
        U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT
        U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT;

#define STATS_SNMP_ICMP_PARAM_COUNT     26

    static const char *stats_net_snmp_icmp_fmt =
        "Icmp:"
        U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT
        U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT
        U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT
        U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT
        U64_FMT U64_FMT U64_FMT;

    int         rc = 0;
    char       *buf = NULL;
    char       *ptr = NULL;
    uint64_t    forwarding;
    uint64_t    default_ttl;
    int         fd = -1;
#endif

    memset(stats, 0, sizeof(*stats));

#if __linux__

    buf = TE_ALLOC(MAX_PROC_NET_SNMP_SIZE);

    VERB("Try to open /proc/net/snmp file");

    if ((fd = open("/proc/net/snmp", O_RDONLY)) < 0)
    {
        ERROR("Cannot open() /proc/net/snmp");
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        goto cleanup;
    }

    VERB("Try to read /proc/net/snmp file");

    if (read(fd, buf, MAX_PROC_NET_SNMP_SIZE) <= 0)
    {
        ERROR("Cannot read /proc/net/snmp file");
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        goto cleanup;
    }

    VERB("Close /proc/net/snmp file");

    close(fd);

    VERB("/proc/net/snmp file dump:\n%s", buf);

    ptr = buf;

#define STATS_GO_TO_NEXT_LINE \
    do                                                      \
    {                                                       \
        ptr = strchr(ptr, '\n');                            \
        if ((ptr == NULL) ||                                \
            (ptr + 1 - buf >= MAX_PROC_NET_SNMP_SIZE))      \
        {                                                   \
            ERROR("Invalid /proc/net/snmp file format");    \
            return TE_OS_RC(TE_TA_UNIX, EINVAL);            \
        }                                                   \
        ptr++;                                              \
    } while (0)

    /* Skip IPv4 header */
    STATS_GO_TO_NEXT_LINE;

    /* Read IPv4 counters */
    if ((rc = sscanf(ptr, stats_net_snmp_ipv4_fmt,
                     &forwarding,
                     &default_ttl,
                     &stats->ipv4.in_recvs,
                     &stats->ipv4.in_hdr_errs,
                     &stats->ipv4.in_addr_errs,
                     &stats->ipv4.forw_dgrams,
                     &stats->ipv4.in_unknown_protos,
                     &stats->ipv4.in_discards,
                     &stats->ipv4.in_delivers,
                     &stats->ipv4.out_requests,
                     &stats->ipv4.out_discards,
                     &stats->ipv4.out_no_routes,
                     &stats->ipv4.reasm_timeout,
                     &stats->ipv4.reasm_reqds,
                     &stats->ipv4.reasm_oks,
                     &stats->ipv4.reasm_fails,
                     &stats->ipv4.frag_oks,
                     &stats->ipv4.frag_fails,
                     &stats->ipv4.frag_creates)) !=
        STATS_SNMP_IPV4_PARAM_COUNT)
    {
        WARN("Invalid /proc/net/snmp file format, "
             "failed on IPv4 statistics, rc=%d, expected %d",
             rc, STATS_SNMP_IPV4_PARAM_COUNT);
        WARN("%s", ptr);
        return TE_OS_RC(TE_TA_UNIX, EINVAL);
    }

    /* Skip IPv4 counters */
    STATS_GO_TO_NEXT_LINE;
    /* Skip ICMP header */
    STATS_GO_TO_NEXT_LINE;

    /* Read ICMP counters */
    if ((rc = sscanf(ptr, stats_net_snmp_icmp_fmt,
                     &stats->icmp.in_msgs,
                     &stats->icmp.in_errs,
                     &stats->icmp.in_dest_unreachs,
                     &stats->icmp.in_time_excds,
                     &stats->icmp.in_parm_probs,
                     &stats->icmp.in_src_quenchs,
                     &stats->icmp.in_redirects,
                     &stats->icmp.in_echos,
                     &stats->icmp.in_echo_reps,
                     &stats->icmp.in_timestamps,
                     &stats->icmp.in_timestamp_reps,
                     &stats->icmp.in_addr_masks,
                     &stats->icmp.in_addr_mask_reps,
                     &stats->icmp.out_msgs,
                     &stats->icmp.out_errs,
                     &stats->icmp.out_dest_unreachs,
                     &stats->icmp.out_time_excds,
                     &stats->icmp.out_parm_probs,
                     &stats->icmp.out_src_quenchs,
                     &stats->icmp.out_redirects,
                     &stats->icmp.out_echos,
                     &stats->icmp.out_echo_reps,
                     &stats->icmp.out_timestamps,
                     &stats->icmp.out_timestamp_reps,
                     &stats->icmp.out_addr_masks,
                     &stats->icmp.out_addr_mask_reps)) !=
        STATS_SNMP_ICMP_PARAM_COUNT)
    {
        WARN("Invalid /proc/net/snmp file format, "
             "failed on ICMP statistics, rc=%d, expected %d",
             rc, STATS_SNMP_ICMP_PARAM_COUNT);
        return TE_OS_RC(TE_TA_UNIX, EINVAL);
    }

cleanup:

    free(buf);

#endif

    return 0;
}

#define STATS_IFTABLE_COUNTER_GET(_name_) \
static te_errno                                                       \
net_if_stats_##_name_##_get(const ta_conf_ctx *ctx, uint64_t *val)          \
{                                                                     \
    const char *dev_name_ = ta_conf_ctx_inst(ctx, "interface");       \
    int         rc = 0;                                               \
    if_stats    stats;                                                \
                                                                      \
    memset(&stats, 0, sizeof(if_stats));                              \
                                                                      \
    if ((rc = dev_stats_get((dev_name_), &stats)) != 0)               \
    {                                                                 \
        ERROR("Cannot get statistics for interface %s", (dev_name_)); \
    }                                                                 \
                                                                      \
    *(val) = stats. _name_;                                           \
                                                                      \
    VERB("dev_counter_get(dev_name=%s, counter=%s) returns %ju",      \
         dev_name_, #_name_, *(val));                                 \
                                                                      \
    return 0;                                                         \
}


/*
 * Single source list of ifTable counter names: expanded below to
 * generate the getters, and again further down to generate the
 * matching tree leaves.
 */
#define STATS_NET_IF_COUNTERS(X_) \
X_(out_errors)        \
X_(out_discards)      \
X_(out_nucast_pkts)   \
X_(out_ucast_pkts)    \
X_(out_octets)        \
X_(in_unknown_protos) \
X_(in_errors)         \
X_(in_discards)       \
X_(in_nucast_pkts)    \
X_(in_ucast_pkts)     \
X_(in_octets)


#define STATS_IFTABLE_COUNTER_GET_X_(n_) \
    STATS_IFTABLE_COUNTER_GET(n_)

STATS_NET_IF_COUNTERS(STATS_IFTABLE_COUNTER_GET_X_)

#undef STATS_IFTABLE_COUNTER_GET_X_
#undef STATS_IFTABLE_COUNTER_GET


#define STATS_NET_SNMP_IPV4_COUNTER_GET(_name_) \
static te_errno                                               \
net_snmp_ipv4_stats_##_name_##_get(const ta_conf_ctx *ctx,          \
                                    uint64_t    *val)         \
{                                                             \
    int         rc = 0;                                       \
    net_stats   net_stats;                                    \
                                                              \
    UNUSED(ctx);                                              \
                                                              \
    memset(&net_stats, 0, sizeof(net_stats));                 \
                                                              \
    if ((rc = net_stats_get(&net_stats)) != 0)                \
    {                                                         \
        ERROR("Cannot get network statistics for system");    \
    }                                                         \
                                                              \
    *(val) = net_stats.ipv4._name_;                           \
                                                              \
    VERB("net_snmp_ipv4_counter_get(counter=%s) returns %ju", \
         #_name_, *(val));                                    \
                                                              \
    return 0;                                                 \
}


/*
 * Single source list of /proc/net/snmp ipv4 counter names:
 * expanded below to generate the getters, and again further
 * down to generate the matching tree leaves.
 */
#define STATS_NET_SNMP_IPV4_COUNTERS(X_) \
X_(frag_creates)      \
X_(frag_fails)        \
X_(frag_oks)          \
X_(reasm_fails)       \
X_(reasm_oks)         \
X_(reasm_reqds)       \
X_(reasm_timeout)     \
X_(out_no_routes)     \
X_(out_discards)      \
X_(out_requests)      \
X_(in_delivers)       \
X_(in_discards)       \
X_(in_unknown_protos) \
X_(forw_dgrams)       \
X_(in_addr_errs)      \
X_(in_hdr_errs)       \
X_(in_recvs)


#define STATS_NET_SNMP_IPV4_COUNTER_GET_X_(n_) \
    STATS_NET_SNMP_IPV4_COUNTER_GET(n_)

STATS_NET_SNMP_IPV4_COUNTERS(STATS_NET_SNMP_IPV4_COUNTER_GET_X_)

#undef STATS_NET_SNMP_IPV4_COUNTER_GET_X_
#undef STATS_NET_SNMP_IPV4_COUNTER_GET


#define STATS_NET_SNMP_ICMP_COUNTER_GET(_name_) \
static te_errno                                               \
net_snmp_icmp_stats_##_name_##_get(const ta_conf_ctx *ctx,          \
                                    uint64_t    *val)         \
{                                                             \
    int         rc = 0;                                       \
    net_stats   net_stats;                                    \
                                                              \
    UNUSED(ctx);                                              \
                                                              \
    memset(&net_stats, 0, sizeof(net_stats));                 \
                                                              \
    if ((rc = net_stats_get(&net_stats)) != 0)                \
    {                                                         \
        ERROR("Cannot get network statistics for system");    \
    }                                                         \
                                                              \
    *(val) = net_stats.icmp._name_;                           \
                                                              \
    VERB("net_snmp_icmp_counter_get(counter=%s) returns %ju", \
         #_name_, *(val));                                    \
                                                              \
    return 0;                                                 \
}


/*
 * Single source list of /proc/net/snmp icmp counter names:
 * expanded below to generate the getters, and again further
 * down to generate the matching tree leaves.
 */
#define STATS_NET_SNMP_ICMP_COUNTERS(X_) \
X_(out_addr_mask_reps) \
X_(out_addr_masks)     \
X_(out_timestamp_reps) \
X_(out_timestamps)     \
X_(out_echo_reps)      \
X_(out_echos)          \
X_(out_redirects)      \
X_(out_src_quenchs)    \
X_(out_parm_probs)     \
X_(out_time_excds)     \
X_(out_dest_unreachs)  \
X_(out_errs)           \
X_(out_msgs)           \
X_(in_addr_mask_reps)  \
X_(in_addr_masks)      \
X_(in_timestamp_reps)  \
X_(in_timestamps)      \
X_(in_echo_reps)       \
X_(in_echos)           \
X_(in_redirects)       \
X_(in_src_quenchs)     \
X_(in_parm_probs)      \
X_(in_time_excds)      \
X_(in_dest_unreachs)   \
X_(in_errs)            \
X_(in_msgs)


#define STATS_NET_SNMP_ICMP_COUNTER_GET_X_(n_) \
    STATS_NET_SNMP_ICMP_COUNTER_GET(n_)

STATS_NET_SNMP_ICMP_COUNTERS(STATS_NET_SNMP_ICMP_COUNTER_GET_X_)

#undef STATS_NET_SNMP_ICMP_COUNTER_GET_X_
#undef STATS_NET_SNMP_ICMP_COUNTER_GET


/*
 * Unix Test Agent network statistics configuration tree.
 *
 * Children are listed in the order the legacy brother chains produced
 * them (ifTable: out-to-in descending; snmp: all ICMP counters
 * out-to-in descending, followed by all IPv4 counters out-to-in
 * descending).
 */

#define STATS_NET_IF_NODE_(n_) \
    TA_CONF_RO_UINT64(#n_, net_if_stats_##n_##_get),

static const ta_conf_node *const node_net_if_stats =
    TA_CONF_NA("stats", STATS_NET_IF_COUNTERS(STATS_NET_IF_NODE_) NULL);

#undef STATS_NET_IF_NODE_
#undef STATS_NET_IF_COUNTERS


#define STATS_NET_SNMP_ICMP_NODE_(n_) \
    TA_CONF_RO_UINT64("icmp_" #n_, net_snmp_icmp_stats_##n_##_get),

#define STATS_NET_SNMP_IPV4_NODE_(n_) \
    TA_CONF_RO_UINT64("ipv4_" #n_, net_snmp_ipv4_stats_##n_##_get),

static const ta_conf_node *const node_net_snmp_stats =
    TA_CONF_NA("stats",
        STATS_NET_SNMP_ICMP_COUNTERS(STATS_NET_SNMP_ICMP_NODE_)
        STATS_NET_SNMP_IPV4_COUNTERS(STATS_NET_SNMP_IPV4_NODE_) NULL);

#undef STATS_NET_SNMP_ICMP_NODE_
#undef STATS_NET_SNMP_IPV4_NODE_
#undef STATS_NET_SNMP_ICMP_COUNTERS
#undef STATS_NET_SNMP_IPV4_COUNTERS


/* See the description in conf_stats.h */
te_errno
ta_unix_conf_net_snmp_stats_init(void)
{
    int fd = open("/proc/net/snmp", O_RDONLY);

    if (fd < 0)
        return 0;
    close(fd);

    return ta_conf_register("/agent", node_net_snmp_stats);
}

/* See the description in conf_stats.h */
te_errno
ta_unix_conf_net_if_stats_init(void)
{
    int fd = open("/proc/net/dev", O_RDONLY);

    if (fd < 0)
        return 0;
    close(fd);

    return ta_conf_register("/agent/interface", node_net_if_stats);
}
