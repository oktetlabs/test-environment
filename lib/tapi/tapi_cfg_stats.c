/** @file
 * @brief Test API to access network statistics via configurator.
 *
 * Implementation of API to configure network statistics.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id: $
 */

#define TE_LGR_USER     "TAPI CFG VTund"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_defs.h"
#include "logger_api.h"
#include "tapi_sockaddr.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_stats.h"


/* See description in tapi_cfg_stats.h */
te_errno 
tapi_cfg_stats_if_stats_get(const char          *ta,
                            const char          *ifname,
                            tapi_cfg_if_stats   *stats)
{
    te_errno            rc;

    VERB("%s(ta=%s, ifname=%s, stats=%x) started",
         __FUNCTION__, ta, ifname, stats);

    if (stats == NULL)
    {
        ERROR("%s(): stats=NULL!", __FUNCTION__);
        return TE_OS_RC(TE_TAPI, EINVAL);
    }

    memset(stats, 0, sizeof(tapi_cfg_if_stats));

    VERB("%s(): stats=%x", __FUNCTION__, stats);

    /* 
     * Synchronize configuration trees and get assigned interfaces
     */

    VERB("Try to sync stats");
    
    rc = cfg_synchronize_fmt(TRUE, "/agent:%s/interface:%s/stats:",
                             ta, ifname);
    if (rc != 0)
        return rc;

    VERB("Get stats counters");

#define TAPI_CFG_STATS_IF_COUNTER_GET(_counter_) \
    do                                                      \
    {                                                       \
        cfg_val_type    val_type = CVT_INTEGER;             \
        VERB("IF_COUNTER_GET(%s)", #_counter_);             \
        rc = cfg_get_instance_fmt(&val_type,                \
                 &stats->_counter_,                         \
                 "/agent:%s/interface:%s/stats:/%s:",       \
                 ta, ifname, #_counter_);                   \
        if (rc != 0)                                        \
        {                                                   \
            ERROR("Failed to get %s counter for "           \
                  "interface %s on %s Test Agent: %r",      \
                  #_counter_, ifname, ta, rc);              \
            return rc;                                      \
        }                                                   \
    } while (0)

    TAPI_CFG_STATS_IF_COUNTER_GET(in_octets);
    TAPI_CFG_STATS_IF_COUNTER_GET(in_ucast_pkts);
    TAPI_CFG_STATS_IF_COUNTER_GET(in_nucast_pkts);
    TAPI_CFG_STATS_IF_COUNTER_GET(in_discards);
    TAPI_CFG_STATS_IF_COUNTER_GET(in_errors);
    TAPI_CFG_STATS_IF_COUNTER_GET(in_unknown_protos);
    TAPI_CFG_STATS_IF_COUNTER_GET(out_octets);
    TAPI_CFG_STATS_IF_COUNTER_GET(out_ucast_pkts);
    TAPI_CFG_STATS_IF_COUNTER_GET(out_nucast_pkts);
    TAPI_CFG_STATS_IF_COUNTER_GET(out_discards);
    TAPI_CFG_STATS_IF_COUNTER_GET(out_errors);

#undef TAPI_CFG_STATS_IF_COUNTER_GET

    return 0;
}


/* See description in tapi_cfg_stats.h */
te_errno 
tapi_cfg_stats_net_stats_get(const char          *ta,
                             tapi_cfg_net_stats  *stats)
{
    te_errno            rc;

    VERB("%s(ta=%s) started", __FUNCTION__, ta);

    if (stats == NULL)
    {
        ERROR("%s(): stats=NULL!", __FUNCTION__);
        return TE_OS_RC(TE_TAPI, EINVAL);
    }

    memset(stats, 0, sizeof(tapi_cfg_net_stats));

    /* 
     * Synchronize configuration trees and get assigned interfaces
     */

    VERB("Try to sync stats");
    
    rc = cfg_synchronize_fmt(TRUE, "/agent:%s/stats:", ta);
    if (rc != 0)
        return rc;

    VERB("Get stats counters");

#define TAPI_CFG_STATS_IPV4_COUNTER_GET(_counter_) \
    do                                                          \
    {                                                           \
        cfg_val_type    val_type = CVT_INTEGER;                 \
        VERB("IPV4_COUNTER_GET(%s)", #_counter_);               \
        rc = cfg_get_instance_fmt(&val_type,                    \
                                  &stats->ipv4._counter_,       \
                                  "/agent:%s/stats:/ipv4_%s:",  \
                                  ta, #_counter_);              \
        if (rc != 0)                                            \
        {                                                       \
            ERROR("Failed to get ipv4_%s counter from %s "      \
                  "Test Agent: %r", #_counter_, ta, rc);        \
            return rc;                                          \
        }                                                       \
    } while (0)

    TAPI_CFG_STATS_IPV4_COUNTER_GET(in_recvs);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(in_hdr_errs);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(in_addr_errs);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(forw_dgrams);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(in_unknown_protos);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(in_discards);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(in_delivers);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(out_requests);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(out_discards);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(out_no_routes);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(reasm_timeout);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(reasm_reqds);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(reasm_oks);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(reasm_fails);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(frag_oks);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(frag_fails);
    TAPI_CFG_STATS_IPV4_COUNTER_GET(frag_creates);

#undef TAPI_CFG_STATS_IPV4_COUNTER_GET

#define TAPI_CFG_STATS_ICMP_COUNTER_GET(_counter_) \
    do                                                          \
    {                                                           \
        cfg_val_type    val_type = CVT_INTEGER;                 \
        VERB("ICMP_COUNTER_GET(%s)", #_counter_);               \
        rc = cfg_get_instance_fmt(&val_type,                    \
                                  &stats->icmp._counter_,       \
                                  "/agent:%s/stats:/icmp_%s:",  \
                                  ta, #_counter_);              \
        if (rc != 0)                                            \
        {                                                       \
            ERROR("Failed to get icmp_%s counter from %s "      \
                  "Test Agent: %r", #_counter_, ta, rc);        \
            return rc;                                          \
        }                                                       \
    } while (0)

    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_msgs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_errs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_dest_unreachs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_time_excds);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_parm_probs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_src_quenchs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_redirects);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_echos);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_echo_reps);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_timestamps);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_timestamp_reps);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_addr_masks);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(in_addr_mask_reps);

    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_msgs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_errs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_dest_unreachs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_time_excds);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_parm_probs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_src_quenchs);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_redirects);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_echos);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_echo_reps);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_timestamps);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_timestamp_reps);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_addr_masks);
    TAPI_CFG_STATS_ICMP_COUNTER_GET(out_addr_mask_reps);

#undef TAPI_CFG_STATS_ICMP_COUNTER_GET

    return 0;
}



#define TAPI_CFG_IF_STATS_MAX_LINE_LEN     4096

/* See description in tapi_cfg_stats.h */
te_errno 
tapi_cfg_stats_if_stats_print(const char          *ta,
                              const char          *ifname,
                              tapi_cfg_if_stats   *stats)
{
    char *buf = (char *)malloc(TAPI_CFG_IF_STATS_MAX_LINE_LEN);
    
    if (stats == NULL)
    {
        free(buf);
        return TE_OS_RC(TE_TAPI, EINVAL);
    }

    snprintf(buf, TAPI_CFG_IF_STATS_MAX_LINE_LEN,
             "Network statistics for interface %s on Test Agent %s:\n"
             "  in_octets : %llu\n"
             "  in_ucast_pkts : %llu\n"
             "  in_nucast_pkts : %llu\n"
             "  in_discards : %llu\n"
             "  in_errors : %llu\n"
             "  in_unknown_protos : %llu\n"
             "  out_octets : %llu\n"
             "  out_ucast_pkts : %llu\n"
             "  out_nucast_pkts : %llu\n"
             "  out_discards : %llu\n"
             "  out_errors : %llu\n",
             ifname, ta,
             stats->in_octets,
             stats->in_ucast_pkts,
             stats->in_nucast_pkts,
             stats->in_discards,
             stats->in_errors,
             stats->in_unknown_protos,
             stats->out_octets,
             stats->out_ucast_pkts,
             stats->out_nucast_pkts,
             stats->out_discards,
             stats->out_errors);

    RING("%s", buf);

    free(buf);

    return 0;
}


#define TAPI_CFG_NET_STATS_MAX_LINE_LEN     4096

/* See description in tapi_cfg_stats.h */
te_errno 
tapi_cfg_stats_net_stats_print(const char          *ta,
                               tapi_cfg_net_stats  *stats)
{
    char *buf = (char *)malloc(TAPI_CFG_NET_STATS_MAX_LINE_LEN);
    
    if (stats == NULL)
    {
        free(buf);
        return TE_OS_RC(TE_TAPI, EINVAL);
    }

    snprintf(buf, TAPI_CFG_NET_STATS_MAX_LINE_LEN,
             "Network statistics for Test Agent %s:\n"
             "IPv4:\n"
             "  in_recvs : %llu\n"
             "  in_hdr_errs : %llu\n"
             "  in_addr_errs : %llu\n"
             "  forw_dgrams : %llu\n"
             "  in_unknown_protos : %llu\n"
             "  in_discards : %llu\n"
             "  in_delivers : %llu\n"
             "  out_requests : %llu\n"
             "  out_discards : %llu\n"
             "  out_no_routes : %llu\n"
             "  reasm_timeout : %llu\n"
             "  reasm_reqds : %llu\n"
             "  reasm_oks : %llu\n"
             "  reasm_fails : %llu\n"
             "  frag_oks : %llu\n"
             "  frag_fails : %llu\n"
             "  frag_creates : %llu\n"
             "ICMP:\n"
             "  in_msgs : %llu\n"
             "  in_errs : %llu\n"
             "  in_dest_unreachs : %llu\n"
             "  in_time_excds : %llu\n"
             "  in_parm_probs : %llu\n"
             "  in_src_quenchs : %llu\n"
             "  in_redirects : %llu\n"
             "  in_echos : %llu\n"
             "  in_echo_reps : %llu\n"
             "  in_timestamps : %llu\n"
             "  in_timestamp_reps : %llu\n"
             "  in_addr_masks : %llu\n"
             "  in_addr_mask_reps : %llu\n"
             "  out_msgs : %llu\n"
             "  out_errs : %llu\n"
             "  out_dest_unreachs : %llu\n"
             "  out_time_excds : %llu\n"
             "  out_parm_probs : %llu\n"
             "  out_src_quenchs : %llu\n"
             "  out_redirects : %llu\n"
             "  out_echos : %llu\n"
             "  out_echo_reps : %llu\n"
             "  out_timestamps : %llu\n"
             "  out_timestamp_reps : %llu\n"
             "  out_addr_masks : %llu\n"
             "  out_addr_mask_reps : %llu\n",
             ta,
             stats->ipv4.in_recvs,
             stats->ipv4.in_hdr_errs,
             stats->ipv4.in_addr_errs,
             stats->ipv4.forw_dgrams,
             stats->ipv4.in_unknown_protos,
             stats->ipv4.in_discards,
             stats->ipv4.in_delivers,
             stats->ipv4.out_requests,
             stats->ipv4.out_discards,
             stats->ipv4.out_no_routes,
             stats->ipv4.reasm_timeout,
             stats->ipv4.reasm_reqds,
             stats->ipv4.reasm_oks,
             stats->ipv4.reasm_fails,
             stats->ipv4.frag_oks,
             stats->ipv4.frag_fails,
             stats->ipv4.frag_creates,

             stats->icmp.in_msgs,
             stats->icmp.in_errs,
             stats->icmp.in_dest_unreachs,
             stats->icmp.in_time_excds,
             stats->icmp.in_parm_probs,
             stats->icmp.in_src_quenchs,
             stats->icmp.in_redirects,
             stats->icmp.in_echos,
             stats->icmp.in_echo_reps,
             stats->icmp.in_timestamps,
             stats->icmp.in_timestamp_reps,
             stats->icmp.in_addr_masks,
             stats->icmp.in_addr_mask_reps,

             stats->icmp.out_msgs,
             stats->icmp.out_errs,
             stats->icmp.out_dest_unreachs,
             stats->icmp.out_time_excds,
             stats->icmp.out_parm_probs,
             stats->icmp.out_src_quenchs,
             stats->icmp.out_redirects,
             stats->icmp.out_echos,
             stats->icmp.out_echo_reps,
             stats->icmp.out_timestamps,
             stats->icmp.out_timestamp_reps,
             stats->icmp.out_addr_masks,
             stats->icmp.out_addr_mask_reps);

    RING("%s", buf);

    free(buf);

    return 0;
}
