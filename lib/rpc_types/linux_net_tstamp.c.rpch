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
 * $Id$
 */

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_LINUX_NET_TSTAMP_H
#include <linux/net_tstamp.h>
#endif

#include "te_defs.h"
#include "te_rpc_defs.h"
#include "te_rpc_linux_net_tstamp.h"

#define HWTSTAMP_UNKNOWN 0xFFFF

/**
 * Solarflare Onload specific flag. It is hardcoded because Onload header
 * must not be included here, but the flag is needed here to be used in
 * functions recvmsg() and recvmmsg().
 */
#ifndef ONLOAD_SOF_TIMESTAMPING_STREAM
#define ONLOAD_SOF_TIMESTAMPING_STREAM (1 << 23)
#endif

/** Temporary hack: define flags which have been added in recent linux
 * versions.  */
#ifndef SOF_TIMESTAMPING_OPT_ID
#define SOF_TIMESTAMPING_OPT_ID (1<<7)
#endif
#ifndef SOF_TIMESTAMPING_TX_SCHED
#define SOF_TIMESTAMPING_TX_SCHED (1<<8)
#endif
#ifndef SOF_TIMESTAMPING_TX_ACK
#define SOF_TIMESTAMPING_TX_ACK (1<<9)
#endif
#ifndef SOF_TIMESTAMPING_OPT_CMSG
#define SOF_TIMESTAMPING_OPT_CMSG (1<<10)
#endif
#ifndef SOF_TIMESTAMPING_OPT_TSONLY
#define SOF_TIMESTAMPING_OPT_TSONLY (1<<11)
#endif

unsigned int
hwtstamp_instr_rpc2h(unsigned flags)
{
#if HAVE_LINUX_NET_TSTAMP_H
    if ((flags & ~RPC_SOF_TIMESTAMPING_MASK) != 0 &&
        (flags & RPC_ONLOAD_SOF_TIMESTAMPING_STREAM) == 0)
        return HWTSTAMP_UNKNOWN;

    return (!!(flags & RPC_SOF_TIMESTAMPING_TX_HARDWARE) *
              SOF_TIMESTAMPING_TX_HARDWARE)
           | (!!(flags & RPC_SOF_TIMESTAMPING_TX_SOFTWARE) *
              SOF_TIMESTAMPING_TX_SOFTWARE)
           | (!!(flags & RPC_SOF_TIMESTAMPING_RX_HARDWARE) *
              SOF_TIMESTAMPING_RX_HARDWARE)
           | (!!(flags & RPC_SOF_TIMESTAMPING_RX_SOFTWARE) *
              SOF_TIMESTAMPING_RX_SOFTWARE)
           | (!!(flags & RPC_SOF_TIMESTAMPING_SOFTWARE) *
              SOF_TIMESTAMPING_SOFTWARE)
           | (!!(flags & RPC_SOF_TIMESTAMPING_SYS_HARDWARE) *
              SOF_TIMESTAMPING_SYS_HARDWARE)
           | (!!(flags & RPC_SOF_TIMESTAMPING_RAW_HARDWARE) *
              SOF_TIMESTAMPING_RAW_HARDWARE)
#ifdef SOF_TIMESTAMPING_OPT_ID
           | (!!(flags & RPC_SOF_TIMESTAMPING_OPT_ID) *
              SOF_TIMESTAMPING_OPT_ID)
#endif
#ifdef SOF_TIMESTAMPING_TX_SCHED
           | (!!(flags & RPC_SOF_TIMESTAMPING_TX_SCHED) *
              SOF_TIMESTAMPING_TX_SCHED)
#endif
#ifdef SOF_TIMESTAMPING_TX_ACK
           | (!!(flags & RPC_SOF_TIMESTAMPING_TX_ACK) *
              SOF_TIMESTAMPING_TX_ACK)
#endif
#ifdef SOF_TIMESTAMPING_OPT_CMSG
           | (!!(flags & RPC_SOF_TIMESTAMPING_OPT_CMSG) *
              SOF_TIMESTAMPING_OPT_CMSG)
#endif
#ifdef  SOF_TIMESTAMPING_OPT_TSONLY
           | (!!(flags & RPC_SOF_TIMESTAMPING_OPT_TSONLY) *
              SOF_TIMESTAMPING_OPT_TSONLY)
#endif
           | (!!(flags & RPC_ONLOAD_SOF_TIMESTAMPING_STREAM) *
              ONLOAD_SOF_TIMESTAMPING_STREAM)
           ;
#else
       return HWTSTAMP_UNKNOWN;
#endif
}

unsigned int
hwtstamp_instr_h2rpc(unsigned int flags)
{
#if HAVE_LINUX_NET_TSTAMP_H
    if ((flags & ~SOF_TIMESTAMPING_MASK) != 0 &&
        (flags & ONLOAD_SOF_TIMESTAMPING_STREAM) == 0)
        return HWTSTAMP_UNKNOWN;

    return (!!(flags & SOF_TIMESTAMPING_TX_HARDWARE) *
            RPC_SOF_TIMESTAMPING_TX_HARDWARE)
           | (!!(flags & SOF_TIMESTAMPING_TX_SOFTWARE) *
              RPC_SOF_TIMESTAMPING_TX_SOFTWARE)
           | (!!(flags & SOF_TIMESTAMPING_RX_HARDWARE) *
              RPC_SOF_TIMESTAMPING_RX_HARDWARE)
           | (!!(flags & SOF_TIMESTAMPING_RX_SOFTWARE) *
              RPC_SOF_TIMESTAMPING_RX_SOFTWARE)
           | (!!(flags & SOF_TIMESTAMPING_SOFTWARE) *
              RPC_SOF_TIMESTAMPING_SOFTWARE)
           | (!!(flags & SOF_TIMESTAMPING_SYS_HARDWARE) *
              RPC_SOF_TIMESTAMPING_SYS_HARDWARE)
           | (!!(flags & SOF_TIMESTAMPING_RAW_HARDWARE) *
              RPC_SOF_TIMESTAMPING_RAW_HARDWARE)
#ifdef SOF_TIMESTAMPING_OPT_ID
           | (!!(flags & SOF_TIMESTAMPING_OPT_ID) *
              RPC_SOF_TIMESTAMPING_OPT_ID)
#endif
#ifdef SOF_TIMESTAMPING_TX_SCHED
           | (!!(flags & SOF_TIMESTAMPING_TX_SCHED) *
              RPC_SOF_TIMESTAMPING_TX_SCHED)
#endif
#ifdef SOF_TIMESTAMPING_TX_ACK
           | (!!(flags & SOF_TIMESTAMPING_TX_ACK) *
              RPC_SOF_TIMESTAMPING_TX_ACK)
#endif
#ifdef SOF_TIMESTAMPING_OPT_CMSG
           | (!!(flags & SOF_TIMESTAMPING_OPT_CMSG) *
              RPC_SOF_TIMESTAMPING_OPT_CMSG)
#endif
#ifdef  SOF_TIMESTAMPING_OPT_TSONLY
           | (!!(flags & SOF_TIMESTAMPING_OPT_TSONLY) *
              RPC_SOF_TIMESTAMPING_OPT_TSONLY)
#endif
           | (!!(flags & ONLOAD_SOF_TIMESTAMPING_STREAM) *
              RPC_ONLOAD_SOF_TIMESTAMPING_STREAM)
           ;
#else
       return HWTSTAMP_UNKNOWN;
#endif
}

#define FT_UNKNOWN        0xFFFFFFFF

/**
 * Convert RPC possible values for hwtstamp_config->tx_type
 * to native ones.
 */
int
hwtstamp_tx_types_rpc2h(rpc_hwtstamp_tx_types type)
{
    switch(type)
    {
#if HAVE_LINUX_NET_TSTAMP_H
        RPC2H_CHECK(HWTSTAMP_TX_OFF);
        RPC2H_CHECK(HWTSTAMP_TX_ON);
        RPC2H_CHECK(HWTSTAMP_TX_ONESTEP_SYNC);
#endif
        default: return FT_UNKNOWN;
    }
}

int
hwtstamp_rx_filters_rpc2h(rpc_hwtstamp_rx_filters filter)
{
    switch(filter)
    {
#if HAVE_LINUX_NET_TSTAMP_H
        RPC2H_CHECK(HWTSTAMP_FILTER_NONE);
        RPC2H_CHECK(HWTSTAMP_FILTER_ALL);
        RPC2H_CHECK(HWTSTAMP_FILTER_SOME);
        RPC2H_CHECK(HWTSTAMP_FILTER_PTP_V1_L4_EVENT);
        RPC2H_CHECK(HWTSTAMP_FILTER_PTP_V1_L4_SYNC);
        RPC2H_CHECK(HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ);
        RPC2H_CHECK(HWTSTAMP_FILTER_PTP_V2_L4_EVENT);
        RPC2H_CHECK(HWTSTAMP_FILTER_PTP_V2_L4_SYNC);
        RPC2H_CHECK(HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ);
        RPC2H_CHECK(HWTSTAMP_FILTER_PTP_V2_L2_EVENT);
        RPC2H_CHECK(HWTSTAMP_FILTER_PTP_V2_L2_SYNC);
        RPC2H_CHECK(HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ);
        RPC2H_CHECK(HWTSTAMP_FILTER_PTP_V2_EVENT);
        RPC2H_CHECK(HWTSTAMP_FILTER_PTP_V2_SYNC);
        RPC2H_CHECK(HWTSTAMP_FILTER_PTP_V2_DELAY_REQ);
#endif
        default: return FT_UNKNOWN;
    }
}
