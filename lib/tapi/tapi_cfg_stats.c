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

    memset(&stats, 0, sizeof(tapi_cfg_if_stats));

    /* 
     * Synchronize configuration trees and get assigned interfaces
     */
    
    rc = cfg_synchronize_fmt(TRUE, "/agent:%s/interface:%s/stats:/",
                             ta, ifname);
    if (rc != 0)
        return rc;


#define TAPI_CFG_STATS_IF_COUNTER_GET(_counter_) \
    do                                                      \
    {                                                       \
        char           *str = NULL;                         \
        cfg_val_type    val_type = CVT_STRING;              \
        rc = cfg_get_instance_fmt(&val_type, &str,          \
                 "/agent:%s/interface:%s/stats:/%s:",       \
                 ta, ifname, #_counter_);                   \
        if (rc != 0)                                        \
        {                                                   \
            ERROR("Failed to get %s counter for "           \
                  "interface %s on %s Test Agent: %r",      \
                  #_counter_, ifname, ta, rc);              \
            return rc;                                      \
        }                                                   \
        if ((sscanf(str, "llu", &stats->_counter_)) != 1)   \
        {                                                   \
            ERROR("Failed to get %s counter for "           \
                  "interface %s on %s Test Agent: %r",      \
                  #_counter_, ifname, ta, rc);              \
            return TE_OS_RC(TE_TAPI, errno);                \
        }                                                   \
        free(str);                                          \
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

    memset(&stats, 0, sizeof(tapi_cfg_net_stats));

    /* 
     * Synchronize configuration trees and get assigned interfaces
     */
    
    rc = cfg_synchronize_fmt(TRUE, "/agent:%s/stats:/", ta);
    if (rc != 0)
        return rc;


#define TAPI_CFG_STATS_IPV4_COUNTER_GET(_counter_) \
    do                                                          \
    {                                                           \
        char           *str = NULL;                             \
        cfg_val_type    val_type = CVT_STRING;                  \
        rc = cfg_get_instance_fmt(&val_type, &str,              \
                 "/agent:%s/stats:/ipv4_%s:",                   \
                 ta, #_counter_);                               \
        if (rc != 0)                                            \
        {                                                       \
            ERROR("Failed to get ipv4_%s counter from %s "      \
                  "Test Agent: %r", #_counter_, ta, rc);        \
            return rc;                                          \
        }                                                       \
        if ((sscanf(str, "llu", &stats->ipv4._counter_)) != 1)  \
        {                                                       \
            ERROR("Failed to get ipv4_%s counter from %s "      \
                  "Test Agent: %r", #_counter_, ta, rc);        \
            return TE_OS_RC(TE_TAPI, errno);                    \
        }                                                       \
        free(str);                                              \
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
        char           *str = NULL;                             \
        cfg_val_type    val_type = CVT_STRING;                  \
        rc = cfg_get_instance_fmt(&val_type, &str,              \
                 "/agent:%s/stats:/icmp_%s:",                   \
                 ta, #_counter_);                               \
        if (rc != 0)                                            \
        {                                                       \
            ERROR("Failed to get icmp_%s counter from %s "      \
                  "Test Agent: %r", #_counter_, ta, rc);        \
            return rc;                                          \
        }                                                       \
        if ((sscanf(str, "llu", &stats->icmp._counter_)) != 1)  \
        {                                                       \
            ERROR("Failed to get icmp_%s counter from %s "      \
                  "Test Agent: %r", #_counter_, ta, rc);        \
            return TE_OS_RC(TE_TAPI, errno);                    \
        }                                                       \
        free(str);                                              \
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

