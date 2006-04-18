/** @file
 * @brief Network statistics
 *
 * Unix TA network statistics support
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id: $
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

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
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


#define STATS_NET_DEV_PROC_LINE_LEN 1024

#define STATS_NET_DEV_LINES_TO_SKIP 2

static char *stats_net_dev_fmt =
    U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT
    U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT
    U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT;

#define STATS_NET_DEV_PARAM_COUNT   15

static te_errno
dev_stats_get(const char *devname, if_stats *stats)
{
    int   rc = 0;
#if __linux__
    char *buf = NULL;
    char *ptr = NULL;
    FILE *devf = NULL;
    int   line = 0;

    uint64_t in_overruns;
    uint64_t in_frame_losses;
    uint64_t in_compressed;
    uint64_t out_overruns;
    uint64_t out_carrier_losses;
    uint64_t out_compressed;
#endif

    memset(stats, 0, sizeof(*stats));

    VERB("dev_stats_get(devname=\"%s\") started", devname);

#if __linux__
    if ((devname == NULL) || (stats == NULL))
    {
        return TE_OS_RC(TE_TA_UNIX, EINVAL);
    }

    buf = (char *)malloc(STATS_NET_DEV_PROC_LINE_LEN);
    if (buf == NULL)
    {
        return TE_OS_RC(TE_TA_UNIX, ENOMEM);
    }

    VERB("Try to open /proc/net/dev file");

    if ((devf = fopen("/proc/net/dev", "r")) == NULL)
    {
        ERROR("Cannot open() /proc/net/dev");
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        goto cleanup;
    }

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
        VERB("Found line %d: >%s", line, buf);
        ptr += strlen(devname) + 1;
        if ((rc = sscanf(ptr, stats_net_dev_fmt,
                         &stats->in_octets,
                         &stats->in_ucast_pkts,
                         &stats->in_errors,
                         &stats->in_discards,
                         &in_overruns,
                         &in_frame_losses,
                         &in_compressed,
                         &stats->in_nucast_pkts,
                         &stats->out_octets,
                         &stats->out_ucast_pkts,
                         &stats->out_errors,
                         &stats->out_discards,
                         &out_overruns,
                         &out_carrier_losses,
                         &out_compressed)) != STATS_NET_DEV_PARAM_COUNT)
        {
            ERROR("Invalid /proc/net/dev file format, "
                  "only %d of %d counters are parsed",
                  rc, STATS_NET_DEV_PARAM_COUNT);
            return TE_OS_RC(TE_TA_UNIX, EINVAL);
        }
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




static char *stats_net_snmp_ipv4_fmt =
    "Ip:"
    I64_FMT I64_FMT U64_FMT U64_FMT U64_FMT U64_FMT
    U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT
    U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT;

#define STATS_SNMP_IPV4_PARAM_COUNT     19

static char *stats_net_snmp_icmp_fmt =
    "Icmp:"
    U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT
    U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT
    U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT
    U64_FMT U64_FMT U64_FMT U64_FMT U64_FMT
    U64_FMT U64_FMT U64_FMT;

#define STATS_SNMP_ICMP_PARAM_COUNT     26

#define MAX_PROC_NET_SNMP_SIZE  4096

static te_errno
net_stats_get(net_stats *stats)
{
#if __linux__
    int         rc = 0;
    char       *buf = NULL;
    char       *ptr = NULL;
    uint64_t    forwarding;
    uint64_t    default_ttl;
    int         fd = -1;
#endif

    memset(stats, 0, sizeof(*stats));

#if __linux__

    buf = (char *)malloc(MAX_PROC_NET_SNMP_SIZE);
    if (buf == NULL)
    {
        return TE_OS_RC(TE_TA_UNIX, ENOMEM);
    }

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

    free(buf);

cleanup:
#endif    

    return 0;
}

#define STATS_IFTABLE_COUNTER_GET(_counter_, _field_) \
static te_errno net_if_stats_##_counter_##_get(unsigned int gid_,       \
                                               const char  *oid_,       \
                                               char        *value_,     \
                                               const char  *dev_name_)  \
{                                                                       \
    int        rc = 0;                                                  \
    if_stats   stats;                                                   \
                                                                        \
    UNUSED(gid_);                                                       \
    UNUSED(oid_);                                                       \
                                                                        \
    memset(&stats, 0, sizeof(if_stats));                                \
                                                                        \
    if ((rc = dev_stats_get((dev_name_), &stats)) != 0)                 \
    {                                                                   \
        ERROR("Cannot get statistics for interface %s", (dev_name_));   \
    }                                                                   \
                                                                        \
    snprintf((value_), RCF_MAX_VAL, U64_FMT, stats. _field_);           \
                                                                        \
    VERB("dev_counter_get(dev_name=%s, counter=%s) returns %s",         \
         dev_name_, #_counter_, value_);                                \
                                                                        \
    return 0;                                                           \
}


STATS_IFTABLE_COUNTER_GET(in_octets, in_octets);
STATS_IFTABLE_COUNTER_GET(in_ucast_pkts, in_ucast_pkts);
STATS_IFTABLE_COUNTER_GET(in_nucast_pkts, in_nucast_pkts);
STATS_IFTABLE_COUNTER_GET(in_discards, in_discards);
STATS_IFTABLE_COUNTER_GET(in_errors, in_errors);
STATS_IFTABLE_COUNTER_GET(in_unknown_protos, in_unknown_protos);
STATS_IFTABLE_COUNTER_GET(out_octets, out_octets);
STATS_IFTABLE_COUNTER_GET(out_ucast_pkts, out_ucast_pkts);
STATS_IFTABLE_COUNTER_GET(out_nucast_pkts, out_nucast_pkts);
STATS_IFTABLE_COUNTER_GET(out_discards, out_discards);
STATS_IFTABLE_COUNTER_GET(out_errors, out_errors);

#undef STATS_IFTABLE_COUNTER_GET


#define STATS_NET_SNMP_IPV4_COUNTER_GET(_counter_, _field_)     \
static te_errno                                                 \
net_snmp_ipv4_stats_##_counter_##_get(unsigned int gid_,        \
                                      const char  *oid_,        \
                                      char        *value_)      \
{                                                               \
    int         rc = 0;                                         \
    net_stats   net_stats;                                      \
                                                                \
    UNUSED(gid_);                                               \
    UNUSED(oid_);                                               \
                                                                \
    memset(&net_stats, 0, sizeof(net_stats));                   \
                                                                \
    if ((rc = net_stats_get(&net_stats)) != 0)                  \
    {                                                           \
        ERROR("Cannot get network statistics for system");      \
    }                                                           \
                                                                \
    snprintf((value_), RCF_MAX_VAL, U64_FMT,                    \
             net_stats.ipv4._field_);                           \
                                                                \
    VERB("net_snmp_ipv4_counter_get(counter=%s) returns %s",    \
         #_counter_, value_);                                   \
                                                                \
    return 0;                                                   \
}

STATS_NET_SNMP_IPV4_COUNTER_GET(in_recvs, in_recvs);
STATS_NET_SNMP_IPV4_COUNTER_GET(in_hdr_errs, in_hdr_errs);
STATS_NET_SNMP_IPV4_COUNTER_GET(in_addr_errs, in_addr_errs);
STATS_NET_SNMP_IPV4_COUNTER_GET(forw_dgrams, forw_dgrams);
STATS_NET_SNMP_IPV4_COUNTER_GET(in_unknown_protos, in_unknown_protos);
STATS_NET_SNMP_IPV4_COUNTER_GET(in_discards, in_discards);
STATS_NET_SNMP_IPV4_COUNTER_GET(in_delivers, in_delivers);
STATS_NET_SNMP_IPV4_COUNTER_GET(out_requests, out_requests);
STATS_NET_SNMP_IPV4_COUNTER_GET(out_discards, out_discards);
STATS_NET_SNMP_IPV4_COUNTER_GET(out_no_routes, out_no_routes);
STATS_NET_SNMP_IPV4_COUNTER_GET(reasm_timeout, reasm_timeout);
STATS_NET_SNMP_IPV4_COUNTER_GET(reasm_reqds, reasm_reqds);
STATS_NET_SNMP_IPV4_COUNTER_GET(reasm_oks, reasm_oks);
STATS_NET_SNMP_IPV4_COUNTER_GET(reasm_fails, reasm_fails);
STATS_NET_SNMP_IPV4_COUNTER_GET(frag_oks, frag_oks);
STATS_NET_SNMP_IPV4_COUNTER_GET(frag_fails, frag_fails);
STATS_NET_SNMP_IPV4_COUNTER_GET(frag_creates, frag_creates);

#undef STATS_NET_SNMP_IPV4_COUNTER_GET


#define STATS_NET_SNMP_ICMP_COUNTER_GET(_counter_, _field_) \
static te_errno                                                 \
net_snmp_icmp_stats_ ## _counter_ ## _get(unsigned int gid_,    \
                                          const char  *oid_,    \
                                          char        *value_)  \
{                                                               \
    int         rc = 0;                                         \
    net_stats   net_stats;                                      \
                                                                \
    UNUSED(gid_);                                               \
    UNUSED(oid_);                                               \
                                                                \
    memset(&net_stats, 0, sizeof(net_stats));                   \
                                                                \
    if ((rc = net_stats_get(&net_stats)) != 0)                  \
    {                                                           \
        ERROR("Cannot get network statistics for system");      \
    }                                                           \
                                                                \
    snprintf((value_), RCF_MAX_VAL, U64_FMT,                    \
             net_stats.icmp._field_);                           \
                                                                \
    VERB("net_snmp_icmp_counter_get(counter=%s) returns %s",    \
         #_counter_, value_);                                   \
                                                                \
    return 0;                                                   \
}

STATS_NET_SNMP_ICMP_COUNTER_GET(in_msgs, in_msgs);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_errs, in_errs);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_dest_unreachs, in_dest_unreachs);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_time_excds, in_time_excds);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_parm_probs, in_parm_probs);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_src_quenchs, in_src_quenchs);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_redirects, in_redirects);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_echos, in_echos);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_echo_reps, in_echo_reps);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_timestamps, in_timestamps);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_timestamp_reps, in_timestamp_reps);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_addr_masks, in_addr_masks);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_addr_mask_reps, in_addr_mask_reps);

STATS_NET_SNMP_ICMP_COUNTER_GET(out_msgs, out_msgs);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_errs, out_errs);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_dest_unreachs, out_dest_unreachs);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_time_excds, out_time_excds);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_parm_probs, out_parm_probs);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_src_quenchs, out_src_quenchs);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_redirects, out_redirects);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_echos, out_echos);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_echo_reps, out_echo_reps);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_timestamps, out_timestamps);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_timestamp_reps, out_timestamp_reps);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_addr_masks, out_addr_masks);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_addr_mask_reps, out_addr_mask_reps);

#undef STATS_NET_SNMP_ICMP_COUNTER_GET


/* ifTable counters */

RCF_PCH_CFG_NODE_RO(node_stats_net_if_in_octets, "in_octets",
                    NULL, NULL, net_if_stats_in_octets_get);

#define STATS_NET_IF_ATTR(_name_, _next) \
    RCF_PCH_CFG_NODE_RO(node_stats_net_if_##_name_, #_name_, \
                        NULL, &node_stats_net_if_##_next,    \
                        net_if_stats_##_name_##_get);

STATS_NET_IF_ATTR(in_ucast_pkts, in_octets);
STATS_NET_IF_ATTR(in_nucast_pkts, in_ucast_pkts);
STATS_NET_IF_ATTR(in_discards, in_nucast_pkts);
STATS_NET_IF_ATTR(in_errors, in_discards);
STATS_NET_IF_ATTR(in_unknown_protos, in_errors);
STATS_NET_IF_ATTR(out_octets, in_unknown_protos);
STATS_NET_IF_ATTR(out_ucast_pkts, out_octets);
STATS_NET_IF_ATTR(out_nucast_pkts, out_ucast_pkts);
STATS_NET_IF_ATTR(out_discards, out_nucast_pkts);
STATS_NET_IF_ATTR(out_errors, out_discards);


/* /proc/net/snmp/ipv4 counters */

RCF_PCH_CFG_NODE_RO(node_stats_net_snmp_ipv4_in_recvs, "ipv4_in_recvs",
                    NULL, NULL, net_snmp_ipv4_stats_in_recvs_get);

#define STATS_NET_SNMP_IPV4_ATTR(_name_, _next) \
    RCF_PCH_CFG_NODE_RO(node_stats_net_snmp_ipv4_##_name_,          \
                        "ipv4_" #_name_,                            \
                        NULL, &node_stats_net_snmp_ipv4_##_next,    \
                        net_snmp_ipv4_stats_##_name_##_get);

STATS_NET_SNMP_IPV4_ATTR(in_hdr_errs, in_recvs);
STATS_NET_SNMP_IPV4_ATTR(in_addr_errs, in_hdr_errs);
STATS_NET_SNMP_IPV4_ATTR(forw_dgrams, in_addr_errs);
STATS_NET_SNMP_IPV4_ATTR(in_unknown_protos, forw_dgrams);
STATS_NET_SNMP_IPV4_ATTR(in_discards, in_unknown_protos);
STATS_NET_SNMP_IPV4_ATTR(in_delivers, in_discards);
STATS_NET_SNMP_IPV4_ATTR(out_requests, in_delivers);
STATS_NET_SNMP_IPV4_ATTR(out_discards, out_requests);
STATS_NET_SNMP_IPV4_ATTR(out_no_routes, out_discards);
STATS_NET_SNMP_IPV4_ATTR(reasm_timeout, out_no_routes);
STATS_NET_SNMP_IPV4_ATTR(reasm_reqds, reasm_timeout);
STATS_NET_SNMP_IPV4_ATTR(reasm_oks, reasm_reqds);
STATS_NET_SNMP_IPV4_ATTR(reasm_fails, reasm_oks);
STATS_NET_SNMP_IPV4_ATTR(frag_oks, reasm_fails);
STATS_NET_SNMP_IPV4_ATTR(frag_fails, frag_oks);
STATS_NET_SNMP_IPV4_ATTR(frag_creates, frag_fails);


/* /proc/net/snmp/icmp counters */

RCF_PCH_CFG_NODE_RO(node_stats_net_snmp_icmp_in_msgs, "icmp_in_msgs",
                    NULL, &node_stats_net_snmp_ipv4_frag_creates,
                    net_snmp_icmp_stats_in_msgs_get);

#define STATS_NET_SNMP_ICMP_ATTR(_name_, _next) \
    RCF_PCH_CFG_NODE_RO(node_stats_net_snmp_icmp_##_name_,          \
                        "icmp_" #_name_,                            \
                        NULL, &node_stats_net_snmp_icmp_##_next,    \
                        net_snmp_icmp_stats_##_name_##_get);

STATS_NET_SNMP_ICMP_ATTR(in_errs, in_msgs);
STATS_NET_SNMP_ICMP_ATTR(in_dest_unreachs, in_errs);
STATS_NET_SNMP_ICMP_ATTR(in_time_excds, in_dest_unreachs);
STATS_NET_SNMP_ICMP_ATTR(in_parm_probs, in_time_excds);
STATS_NET_SNMP_ICMP_ATTR(in_src_quenchs, in_parm_probs);
STATS_NET_SNMP_ICMP_ATTR(in_redirects, in_src_quenchs);
STATS_NET_SNMP_ICMP_ATTR(in_echos, in_redirects);
STATS_NET_SNMP_ICMP_ATTR(in_echo_reps, in_echos);
STATS_NET_SNMP_ICMP_ATTR(in_timestamps, in_echo_reps);
STATS_NET_SNMP_ICMP_ATTR(in_timestamp_reps, in_timestamps);
STATS_NET_SNMP_ICMP_ATTR(in_addr_masks, in_timestamp_reps);
STATS_NET_SNMP_ICMP_ATTR(in_addr_mask_reps, in_addr_masks);

STATS_NET_SNMP_ICMP_ATTR(out_msgs, in_addr_mask_reps);
STATS_NET_SNMP_ICMP_ATTR(out_errs, out_msgs);
STATS_NET_SNMP_ICMP_ATTR(out_dest_unreachs, out_errs);
STATS_NET_SNMP_ICMP_ATTR(out_time_excds, out_dest_unreachs);
STATS_NET_SNMP_ICMP_ATTR(out_parm_probs, out_time_excds);
STATS_NET_SNMP_ICMP_ATTR(out_src_quenchs, out_parm_probs);
STATS_NET_SNMP_ICMP_ATTR(out_redirects, out_src_quenchs);
STATS_NET_SNMP_ICMP_ATTR(out_echos, out_redirects);
STATS_NET_SNMP_ICMP_ATTR(out_echo_reps, out_echos);
STATS_NET_SNMP_ICMP_ATTR(out_timestamps, out_echo_reps);
STATS_NET_SNMP_ICMP_ATTR(out_timestamp_reps, out_timestamps);
STATS_NET_SNMP_ICMP_ATTR(out_addr_masks, out_timestamp_reps);
STATS_NET_SNMP_ICMP_ATTR(out_addr_mask_reps, out_addr_masks);


/*
 * Unix Test Agent network statistics configuration tree.
 */

RCF_PCH_CFG_NODE_NA(node_net_if_stats, "stats",
                    &node_stats_net_if_out_errors,
                    NULL);

RCF_PCH_CFG_NODE_NA(node_net_snmp_stats, "stats",
                    &node_stats_net_snmp_icmp_out_addr_mask_reps,
                    NULL);


/* See the description in conf_stats.h */
te_errno
ta_unix_conf_net_snmp_stats_init(void)
{
    return rcf_pch_add_node("/agent", &node_net_snmp_stats);
}

/* See the description in conf_stats.h */
te_errno
ta_unix_conf_net_if_stats_init(void)
{
    return rcf_pch_add_node("/agent/interface", &node_net_if_stats);
}
