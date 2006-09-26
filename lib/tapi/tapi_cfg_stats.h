/** @file
 * @brief Test API to access network statistics via configurator.
 *
 * Definition of API to configure network statistics.
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
 * $Id$
 */

#ifndef __TE_TAPI_CFG_STATS_H__
#define __TE_TAPI_CFG_STATS_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "te_errno.h"
#include "conf_api.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct tapi_cfg_if_stats {
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
} tapi_cfg_if_stats;


typedef struct tapi_cfg_net_stats_ipv4{
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
} tapi_cfg_net_stats_ipv4;

typedef struct tapi_cfg_net_stats_icmp {
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
} tapi_cfg_net_stats_icmp;

typedef struct tapi_cfg_net_stats {
    tapi_cfg_net_stats_ipv4  ipv4;
    tapi_cfg_net_stats_icmp  icmp;
} tapi_cfg_net_stats;


/**
 * Get IfTable statistics for the certain network interface.
 *
 * @param ta            Test Agent to gather statistics on
 * @param ifname        Network interface to gather statistics of
 * @param stats         Resulted interface statistics structure
 * 
 * @return Status code
 */
extern te_errno 
tapi_cfg_stats_if_stats_get(const char          *ta,
                            const char          *ifname,
                            tapi_cfg_if_stats   *stats);

/**
 * Print IfTable statistics for the certain network interface.
 *
 * @param ta            Test Agent to gather statistics on
 * @param ifname        Network interface to gather statistics of
 * @param stats         Gathered interface statistics structure to print
 * 
 * @return Status code
 */
te_errno 
tapi_cfg_stats_if_stats_print(const char          *ta,
                              const char          *ifname,
                              tapi_cfg_if_stats   *stats);

/**
 * Get /proc/net/snmp like statistics for the host,
 * where Test Agent is running.
 *
 * @param ta            Test Agent to gather statistics on
 * @param stats         Resulted host statistics structure
 * 
 * @return Status code
 */
extern te_errno 
tapi_cfg_stats_net_stats_get(const char           *ta,
                             tapi_cfg_net_stats   *stats);

/**
 * Print /proc/net/snmp like statistics for the host,
 * where Test Agent is running.
 *
 * @param ta            Test Agent to gether statistics on
 * @param stats         Gathered host statistics structure to print
 * 
 * @return Status code
 */
extern te_errno 
tapi_cfg_stats_net_stats_print(const char           *ta,
                               tapi_cfg_net_stats   *stats);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_STATS_H__ */
