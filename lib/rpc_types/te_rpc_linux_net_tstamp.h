/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from linux/net_tstamps.h.
 * 
 * 
 * Copyright (C) 2013 Test Environment authors (see file AUTHORS
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
 * @author Yurij Plotnikov <Yurij.Plotnikov@oktetlabs.ru>
 *
 * $Id: te_rpc_linux_net_tstamp.h 26840 2006-04-21 11:43:28Z arybchik $
 */

 
#ifndef __TE_RPC_LINUX_NET_TSTAMP_H__
#define __TE_RPC_LINUX_NET_TSTAMP_H__

#include "te_rpc_defs.h"


#ifdef __cplusplus
extern "C" {
#endif

/* SO_TIMESTAMPING gets an integer bit field comprised of these values */
enum {
    RPC_SOF_TIMESTAMPING_TX_HARDWARE = (1<<0),
    RPC_SOF_TIMESTAMPING_TX_SOFTWARE = (1<<1),
    RPC_SOF_TIMESTAMPING_RX_HARDWARE = (1<<2),
    RPC_SOF_TIMESTAMPING_RX_SOFTWARE = (1<<3),
    RPC_SOF_TIMESTAMPING_SOFTWARE = (1<<4),
    RPC_SOF_TIMESTAMPING_SYS_HARDWARE = (1<<5),
    RPC_SOF_TIMESTAMPING_RAW_HARDWARE = (1<<6),
    RPC_SOF_TIMESTAMPING_OPT_ID = (1<<7),
    RPC_SOF_TIMESTAMPING_TX_SCHED = (1<<8),
    RPC_SOF_TIMESTAMPING_TX_ACK = (1<<9),
    RPC_SOF_TIMESTAMPING_OPT_CMSG = (1<<10),
    RPC_SOF_TIMESTAMPING_OPT_TSONLY = (1<<11),
    RPC_ONLOAD_SOF_TIMESTAMPING_STREAM = (1<<23),

    RPC_SOF_TIMESTAMPING_LAST = RPC_SOF_TIMESTAMPING_OPT_TSONLY,
    RPC_SOF_TIMESTAMPING_MASK = (RPC_SOF_TIMESTAMPING_LAST - 1) |
                                RPC_SOF_TIMESTAMPING_LAST
};

/* Mappilg list for conversion flags to string. */
#define HWTSTAMP_INSTR_MAPPING_LIST \
    RPC_BIT_MAP_ENTRY(SOF_TIMESTAMPING_TX_HARDWARE), \
    RPC_BIT_MAP_ENTRY(SOF_TIMESTAMPING_TX_SOFTWARE), \
    RPC_BIT_MAP_ENTRY(SOF_TIMESTAMPING_RX_HARDWARE), \
    RPC_BIT_MAP_ENTRY(SOF_TIMESTAMPING_RX_SOFTWARE), \
    RPC_BIT_MAP_ENTRY(SOF_TIMESTAMPING_SOFTWARE), \
    RPC_BIT_MAP_ENTRY(SOF_TIMESTAMPING_SYS_HARDWARE), \
    RPC_BIT_MAP_ENTRY(SOF_TIMESTAMPING_RAW_HARDWARE), \
    RPC_BIT_MAP_ENTRY(SOF_TIMESTAMPING_OPT_ID), \
    RPC_BIT_MAP_ENTRY(SOF_TIMESTAMPING_TX_SCHED), \
    RPC_BIT_MAP_ENTRY(SOF_TIMESTAMPING_TX_ACK), \
    RPC_BIT_MAP_ENTRY(SOF_TIMESTAMPING_OPT_CMSG), \
    RPC_BIT_MAP_ENTRY(SOF_TIMESTAMPING_OPT_TSONLY), \
    RPC_BIT_MAP_ENTRY(ONLOAD_SOF_TIMESTAMPING_STREAM)

/**
 * timestamping_flags_rpc2str()
 */
RPCBITMAP2STR(timestamping_flags, HWTSTAMP_INSTR_MAPPING_LIST)

extern unsigned int hwtstamp_instr_rpc2h(unsigned int instrs);

extern unsigned int hwtstamp_instr_h2rpc(unsigned int instrs);

typedef struct rpc_hwtstamp_config {
    int flags;
    int tx_type;
    int rx_filter;
} rpc_hwtstamp_config;

/* possible values for hwtstamp_config->tx_type */
typedef enum rpc_hwtstamp_tx_types {
    RPC_HWTSTAMP_TX_OFF, /* No outgoing packet will need hardware time
                            stamping; should a packet arrive which asks
                            for it, no hardware time stamping will be
                            done. */
    RPC_HWTSTAMP_TX_ON, /* Enables hardware time stamping for outgoing
                           packets; the sender of the packet decides which
                           are to be time stamped by setting
                           %SOF_TIMESTAMPING_TX_SOFTWARE before sending the
                           packet. */
    RPC_HWTSTAMP_TX_ONESTEP_SYNC, /* Enables time stamping for outgoing
                                     packets just as HWTSTAMP_TX_ON does,
                                     but also enables time stamp insertion
                                     directly into Sync packets. In this
                                     case, transmitted Sync packets will
                                     not received a time stamp via the
                                     socket error queue. */
} rpc_hwtstamp_tx_types;

extern int hwtstamp_tx_types_rpc2h(unsigned int type);

/* possible values for hwtstamp_config->rx_filter */
typedef enum rpc_hwtstamp_rx_filters {
    RPC_HWTSTAMP_FILTER_NONE, /* time stamp no incoming packet at all */
    RPC_HWTSTAMP_FILTER_ALL, /* time stamp any incoming packet */
    RCP_HWTSTAMP_FILTER_SOME, /* return value: time stamp all packets
                                 requested plus some others */
    RPC_HWTSTAMP_FILTER_PTP_V1_L4_EVENT, /* PTP v1, UDP, any kind of event
                                            packet */
    RPC_HWTSTAMP_FILTER_PTP_V1_L4_SYNC, /* PTP v1, UDP, Sync packet */
    RPC_HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ, /* PTP v1, UDP, Delay_req
                                                packet */
    RPC_HWTSTAMP_FILTER_PTP_V2_L4_EVENT, /* PTP v2, UDP, any kind of event
                                            packet */
    RPC_HWTSTAMP_FILTER_PTP_V2_L4_SYNC, /* PTP v2, UDP, Sync packet */
    RPC_HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ, /* PTP v2, UDP, Delay_req
                                                packet */
    RPC_HWTSTAMP_FILTER_PTP_V2_L2_EVENT, /* 802.AS1, Ethernet, any kind of
                                            event packet */
    RPC_HWTSTAMP_FILTER_PTP_V2_L2_SYNC, /* 802.AS1, Ethernet, Sync packet */
    RPC_HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ, /* 802.AS1, Ethernet, Delay_req
                                                packet */
    RPC_HWTSTAMP_FILTER_PTP_V2_EVENT, /* PTP v2/802.AS1, any layer, any kind
                                         of event packet */
    RPC_HWTSTAMP_FILTER_PTP_V2_SYNC, /* PTP v2/802.AS1, any layer, Sync
                                        packet */
    RPC_HWTSTAMP_FILTER_PTP_V2_DELAY_REQ, /* PTP v2/802.AS1, any layer,
                                             Delay_req packet */
} rpc_hwtstamp_rx_filters;

extern int hwtstamp_rx_filters_rpc2h(unsigned int filter);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_LINUX_NET_TSTAMP_H__ */
