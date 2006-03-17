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

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "logger_api.h"
#include "unix_internal.h"

#ifndef IF_NAMESIZE
#define IF_NAMESIZE IFNAMSIZ
#endif


typedef struct if_stats {
    uint64      in_octets;
    uint64      in_ucast_pkts;
    uint64      in_nucast_pkts;
    uint64      in_discards;
    uint64      in_errors;
    uint64      in_unknown_protos;
    uint64      out_octets;
    uint64      out_ucast_pkts;
    uint64      out_nucast_pkts;
    uint64      out_discards;
    uint64      out_errors;
} if_stats;


typedef struct net_stats_ipv4{
    uint64      in_recvs;
    uint64      in_hdr_errs;
    uint64      in_addr_errs;
    uint64      forw_dgrams;
    uint64      in_unknown_protos;
    uint64      in_discards;
    uint64      in_delivers;
    uint64      out_requests;
    uint64      out_discards;
    uint64      out_no_routes;
    uint64      reasm_timeout;
    uint64      reasm_reqds;
    uint64      reasm_oks;
    uint64      reasm_fails;
    uint64      frag_oks;
    uint64      frag_fails;
    uint64      frag_creates;
} net_stats_ipv4;

typedef struct net_stats_icmp {
    uint64      in_msgs;
    uint64      in_errs;
    uint64      in_dest_unreachs;
    uint64      in_time_excds;
    uint64      in_parm_probs;
    uint64      in_src_quenchs;
    uint64      in_redirects;
    uint64      in_echos;
    uint64      in_echo_reps;
    uint64      in_timestamps;
    uint64      in_timestamp_reps;
    uint64      in_addr_masks;
    uint64      in_addr_mask_reps;

    uint64      out_msgs;
    uint64      out_errs;
    uint64      out_dest_unreachs;
    uint64      out_time_excds;
    uint64      out_parm_probs;
    uint64      out_src_quenchs;
    uint64      out_redirects;
    uint64      out_echos;
    uint64      out_echo_reps;
    uint64      out_timestamps;
    uint64      out_timestamp_reps;
    uint64      out_addr_masks;
    uint64      out_addr_mask_reps;
} net_stats_icmp;

typedef struct {
    net_stats_ipv4  ipv4;
    net_stats_icmp  icmp;
} net_stats;


static char *if_stats_rx_pkts_fmt =
    "RX packets:%llu errors:%lu dropped:%lu overruns:%lu frame:%lu";

static char *if_stats_tx_pkts_fmt =
    "TX packets:%llu errors:%lu dropped:%lu overruns:%lu frame:%lu";

static char *if_stats_rx_octets_fmt =
    "RX bytes:%llu";

static char *if_stats_tx_octets_fmt =
    "TX bytes:%llu";

#define MAX_IFCONFIG_OUTPUT_LEN             1024
#define MAX_IFCONFIG_CMD_LEN                48
#define IFCONFIG_OUTPUT_RX_STAT_LINE        4
#define IFCONFIG_OUTPUT_TX_STAT_LINE        5
#define IFCONFIG_OUTPUT_RX_TX_BYTES_LINE    7
#define MAX_IFCONFIG_OUTPUT_LINE            9

#define IFCONFIG_OUTPUT_RX_STAT_LINE_PARAMS     6
#define IFCONFIG_OUTPUT_TX_STAT_LINE_PARAMS     6

static te_errno
if_stats_get(const char *ifname, if_stats *stats)
{
    int   rc = 0;
    char *buf = NULL;
    char *ptr = NULL;
    int   pid;
    int   out_fd;
    char  cmd[MAX_IFCONFIG_CMD_LEN];
    FILE *ifcfg_output;

    uint64 in_overruns;
    uint64 in_frame_losses;
    uint64 out_overruns;
    uint64 out_carrier_losses;

    memset(stats, 0, sizeof(if_stats);

#if __linux__
    if ((ifname == NULL) || (stats == NULL))
    {
        return TE_OS_RC(TE_TA_UNIX, EINVAL);
    }
    
    buf = (char *)malloc(MAX_IFCONFIG_OUTPUT_LEN);
    if (buf == NULL)
    {
        rc = TE_OS_RC(TE_TA_UNIX, ENOMEM);
        goto cleanup;
    }

    snprintf(cmd, MAX_IFCONFIG_CMD_LEN, "ifconfig %s", ifname);
    
    pid = te_shell_cmd(cmd, -1, NULL, &out_fd);
    if (pid < 0)
    {
        ERROR("Cannot execute ifconfig utility");
        rc = TE_OS_RC(TE_TA_UNIX, pid);
        goto cleanup;
    }

    ifcfg_output = fdopen(out_fd, "r");
    if (ifcfg_output == NULL)
    {
        ERROR("Cannot do fdopen() on ifconfig output file descriptor");
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        goto cleanup;
    }
        

    for (line = 0; line < MAX_IFCONFIG_OUTPUT_LINE; line++)
    {
        if (fgets(buf, MAX_IFCONFIG_OUTPUT_LEN, ifcfg_output) == NULL)
        {
            ERROR("Invalid ifconfig output format");
            rc = TE_OS_RC(TE_TA_UNIX, EINVAL);
            goto cleanup;
        }
        
        switch (line)
        {
            case IFCONFIG_OUTPUT_RX_STAT_LINE:
                if (sscanf(buf, if_stats_rx_pkts_fmt,
                           &stats->in_ucast_pkts,
                           &stats->in_errors,
                           &stats->in_discards,
                           &in_overruns,
                           &in_frame_losses) !=
                    IFCONFIG_OUTPUT_RX_STAT_LINE_PARAMS)
                {
                    ERROR("Invalid format of the Rx stats line");
                    rc = TE_OS_RC(TE_TA_UNIX, EINVAL);
                    goto cleanup;
                }
                break;

            case IFCONFIG_OUTPUT_TX_STAT_LINE:
                if (sscanf(buf, if_stats_tx_pkts_fmt,
                           &stats->out_ucast_pkts,
                           &stats->out_errors,
                           &stats->out_discards,
                           &out_overruns,
                           &out_carrier_losses) !=
                    IFCONFIG_OUTPUT_TX_STAT_LINE_PARAMS)
                {
                    ERROR("Invalid format of the Tx stats line");
                    rc = TE_OS_RC(TE_TA_UNIX, EINVAL);
                    goto cleanup;
                }
                break;

            case IFCONFIG_OUTPUT_RX_TX_BYTES_LINE:
                if ((ptr = strstr(buf, "RX bytes")) == NULL)
                {
                    ERROR("Invalid format of the Rx/Tx bytes stats line");
                    rc = TE_OS_RC(TE_TA_UNIX, EINVAL);
                    goto cleanup;
                }
                if (sscanf(ptr, if_stats_rx_octets_fmt,
                           &stats->in_octets) != 1)
                {
                    ERROR("Invalid format of the Rx bytes stats line");
                    rc = TE_OS_RC(TE_TA_UNIX, EINVAL);
                    goto cleanup;
                }

                if ((ptr = strstr(buf, "TX bytes")) == NULL)
                {
                    ERROR("Invalid format of the Rx/Tx bytes stats line");
                    rc = TE_OS_RC(TE_TA_UNIX, EINVAL);
                    goto cleanup;
                }
                if (sscanf(ptr, if_stats_tx_octets_fmt,
                           &stats->out_octets) != 1)
                {
                    ERROR("Invalid format of the Tx bytes stats line");
                    rc = TE_OS_RC(TE_TA_UNIX, EINVAL);
                    goto cleanup;
                }
                break;

            default:
                break;
        }
    }

#endif

    rc = 0;

cleanup:
#if __linux__
    if (ifcfg_output != NULL)
        fclose(ifcfg_output);

    if (pid >=0)
        ta_waitpid(pid, &status, 0);
        
    if (out_fd > 0)
        close(out_fd);

    if (buf != NULL)
        free(buf);
#endif

    return rc;
}

static char *stats_net_snmp_ipv4_fmt =
  "Ip: %d %d %u %u %u %u "
  "%u %u %u %u %u %u "
  "%u %u %u %u %u %u %u";

static char *stats_snmp_icmp_fmt =
  "Icmp: %u %u %u %u %u %u "
  "%u %u %u %u %u %u "
  "%u %u %u %u %u %u "
  "%u %u %u %u %u "
  "%u %u %u";

typedef struct if_stats {
} if_stats;

#define MAX_PROC_NET_SNMP_SIZE  1024

static te_errno
net_stats_get(net_stats *stats)
{
    char *buf = NULL;

#if __linux__
    FILE *fstats;

    memset(stats, 0, sizeof(net_stats);

    buf = (char *)malloc(MAX_PROC_NET_SNMP_SIZE);
    if (buf == NULL)
    {
        return TE_OS_RC(TE_TA_UNIX, ENOMEM);
    }

    if ((fstats = fopen("/proc/net/snmp", "r")) == NULL)
    {
        ERROR("Cannot do fdopen() on ifconfig output file descriptor");
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        goto cleanup;
    }

    if (fread(buf, MAX_NET_SNMP_STATS_LEN, 1, fstats) < 0)
    {
        ERROR("Cannot read /proc/net/snmp file");
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        goto cleanup;
    }

    fclose(fstats);

    ptr = buf;

#define STATS_GO_TO_NEXT_LINE() \
    do                                                      \
    {                                                       \
        ptr = strchr(buf, '\n');                            \
        if ((ptr == NULL) ||                                \
           (ptr + 1 - buf >= MAX_PROC_NET_SNMP_SIZE))       \
        {                                                   \
            ERROR("Invalid /proc/net/snmp file format");    \
            return TE_OS_RC(TE_TA_UNIX, EINVAL);            \
        }                                                   \
        ptr++;                                              \
    }

    /* Skip IPv4 header */
    STATS_GO_TO_NEXT_LINE;

    /* Read IPv4 counters */
    if ((rc = sscanf(ptr, stats_snmp_ipv4_fmt,
                     &forwarding,
                     &default_ttl,
                     &stats.ipv4.in_recvs,
                     &stats.ipv4.in_hdr_errs,
                     &stats.ipv4.in_addr_errs,
                     &stats.ipv4.forw_dgrams,
                     &stats.ipv4.in_unknown_protos,
                     &stats.ipv4.in_discards,
                     &stats.ipv4.in_delivers,
                     &stats.ipv4.out_requests,
                     &stats.ipv4.out_discards,
                     &stats.ipv4.out_no_routes,
                     &stats.ipv4.reasm_timeout,
                     &stats.ipv4.reasm_reqds,
                     &stats.ipv4.reasm_oks,
                     &stats.ipv4.reasm_fails,
                     &stats.ipv4.frag_oks,
                     &stats.ipv4.frag_fails,
                     &stats.ipv4.frag_creates)) !=
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
    if ((rc = sscanf(ptr, stats_snmp_icmp_fmt,
                     &stats.icmp.in_msgs,
                     &stats.icmp.in_errs,
                     &stats.icmp.in_dest_unreachs,
                     &stats.icmp.in_time_excds,
                     &stats.icmp.in_parm_probs,
                     &stats.icmp.in_src_quenchs,
                     &stats.icmp.in_redirects,
                     &stats.icmp.in_echos,
                     &stats.icmp.in_echo_reps,
                     &stats.icmp.in_timestamps,
                     &stats.icmp.in_timestamp_reps,
                     &stats.icmp.in_addr_masks,
                     &stats.icmp.in_addr_mask_reps,
                     &stats.icmp.out_msgs,
                     &stats.icmp.out_errs,
                     &stats.icmp.out_dest_unreachs,
                     &stats.icmp.out_time_excds,
                     &stats.icmp.out_parm_probs,
                     &stats.icmp.out_src_quenchs,
                     &stats.icmp.out_redirects,
                     &stats.icmp.out_echos,
                     &stats.icmp.out_echo_reps,
                     &stats.icmp.out_timestamps,
                     &stats.icmp.out_timestamp_reps,
                     &stats.icmp.out_addr_masks,
                     &stats.icmp.out_addr_mask_reps)) != 
        STATS_SNMP_ICMP_PARAM_COUNT)
    {
        WARN("Invalid /proc/net/snmp file format, "
             "failed on ICMP statistics, rc=%d, expected %d", 
             rc, STATS_SNMP_ICMP_PARAM_COUNT);
        return TE_OS_RC(TE_TA_UNIX, EINVAL);
    }
#endif    
}

#define STATS_IFTABLE_COUNTER_GET(_counter_, _field_) \
static te_errno if_stats_ ## __counter_ ## _get(unsigned int gid_,      \
                                                const char  *oid_,      \
                                                int         *value,     \
                                                const char  *if_name_)  \
{                                                                       \
    if_stats   if_stats;                                                \
                                                                        \
    if ((rc = if_stats_get((if_name_), &if_stats)) != 0)                \
    {                                                                   \
        ERROR("Cannot get statistics for interface %s", (if_name_));    \
    }                                                                   \
                                                                        \
    snprintf(value, RCF_MAX_VAL, "%llu", if_stats. ##_field_);          \
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


#define STATS_NET_SNMP_IPV4_COUNTER_GET(_counter_, _field_) \
static te_errno if_stats_ ## __counter_ ## _get(unsigned int gid_,      \
                                                const char  *oid_,      \
                                                int         *value),    \
{                                                                       \
    net_stats   net_stats;                                              \
                                                                        \
    if ((rc = net_stats_get(&net_stats)) != 0)                          \
    {                                                                   \
        ERROR("Cannot get network statistics for system");              \
    }                                                                   \
                                                                        \
    snprintf(value, RCF_MAX_VAL, "%llu",                                \
             net_stats.ipv4. ##_field_);                                \
                                                                        \
    return 0;                                                           \
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
net_snmp_icmp_stats_ ## __counter_ ## _get(unsigned int gid_,   \
                                           const char  *oid_,   \
                                           int         *value), \
{                                                               \
    net_stats   net_stats;                                      \
                                                                \
    if ((rc = net_stats_get(&net_stats)) != 0)                  \
    {                                                           \
        ERROR("Cannot get network statistics for system");      \
    }                                                           \
                                                                \
    snprintf(value, RCF_MAX_VAL, "%llu",                        \
             net_stats.icmp. ##_field_);                        \
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

RCF_PCH_CFG_NODE_RO(node_stats_net_snmp_ipv4_in_recvs, "in_recvs",
                    NULL, NULL, net_snmp_ipv4_stats_in_recvs_get);

#define STATS_NET_SNMP_IPV4_ATTR(_name_, _next) \
    RCF_PCH_CFG_NODE_RO(node_stats_net_snmp_ipv4_##_name_, #_name_, \
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

RCF_PCH_CFG_NODE_RO(node_stats_net_snmp_ipv4_in_recvs, "in_msgs",
                    NULL, NULL, net_snmp_ipv4_stats_in_recvs_get);

#define STATS_NET_SNMP_ICMP_ATTR(_name_, _next) \
    RCF_PCH_CFG_NODE_RO(node_stats_net_snmp_icmp_##_name_, #_name_, \
                        NULL, &node_stats_net_snmp_icmp_##_next,    \
                        net_snmp_icmp_stats_##_name_##_get);

STATS_NET_SNMP_ICMP_ATTR(in_msgs, in_msgs);
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
