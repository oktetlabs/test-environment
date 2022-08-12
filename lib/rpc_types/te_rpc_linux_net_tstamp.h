/** @file
 * @brief RPC types definitions
 *
 * RPC analogues of definitions from linux/net_tstamps.h.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */


#ifndef __TE_RPC_LINUX_NET_TSTAMP_H__
#define __TE_RPC_LINUX_NET_TSTAMP_H__

#include "te_rpc_defs.h"
#include "te_string.h"


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

/**
 * RPC constants corresponding to values defined in hwtstamp_tx_types
 * enum from linux/net_tstamp.h. See descriptions there.
 */
typedef enum rpc_hwtstamp_tx_types {
    RPC_HWTSTAMP_TX_OFF,
    RPC_HWTSTAMP_TX_ON,
    RPC_HWTSTAMP_TX_ONESTEP_SYNC,
    RPC_HWTSTAMP_TX_ONESTEP_P2P,

    RPC_HWTSTAMP_TX_UNKNOWN, /**< Unknown TX type */
} rpc_hwtstamp_tx_types;

/**
 * Convert RPC constant to corresponsing value from hwtstamp_tx_types enum.
 *
 * @param type      RPC constant
 *
 * @return Corresponding native value or @c -1 if no such value exists.
 */
extern int hwtstamp_tx_types_rpc2h(rpc_hwtstamp_tx_types type);

/**
 * Convert constant from hwtstamp_tx_types enum to corresponding RPC
 * constant.
 *
 * @param type      Native constant
 *
 * @return Corresponding RPC value or @c RPC_HWTSTAMP_TX_UNKNOWN if no such
 *         value exists.
 */
extern rpc_hwtstamp_tx_types hwtstamp_tx_types_h2rpc(int type);

/**
 * Get string name of a constant from rpc_hwtstamp_tx_types enum.
 *
 * @param type        RPC constant
 *
 * @return Constant name.
 */
extern const char *hwtstamp_tx_types_rpc2str(rpc_hwtstamp_tx_types type);

/**
 * Convert native flags from hwtstamp_tx_types to RPC ones.
 * These flags are returned by ioctl(SIOCETHTOOL/ETHTOOL_GET_TS_INFO).
 * Every member of enum hwtstamp_tx_types has a corresponding bit,
 * for example HWTSTAMP_TX_ON -> (1 << HWTSTAMP_TX_ON).
 * If this function sees (1 << HWTSTAMP_TX_ON) bit set, it sets
 * (1 << RPC_HWTSTAMP_TX_ON) bit in the result.
 *
 * @param flags   Native flags
 *
 * @return RPC flags.
 */
extern unsigned int hwtstamp_tx_types_flags_h2rpc(unsigned int flags);

/**
 * Append string representation of flags produced by
 * hwtstamp_tx_types_flags_h2rpc() to TE string.
 *
 * @param flags   Flags
 * @param str     Where to append string representation
 *
 * @return Status code.
 */
extern te_errno hwtstamp_tx_types_flags_rpc2te_str(unsigned int flags,
                                                   te_string *str);

/**
 * RPC constants corresponding to values defined in hwtstamp_rx_filters
 * enum from linux/net_tstamp.h. See descriptions there.
 */
typedef enum rpc_hwtstamp_rx_filters {
    RPC_HWTSTAMP_FILTER_NONE,
    RPC_HWTSTAMP_FILTER_ALL,
    RPC_HWTSTAMP_FILTER_SOME,
    RPC_HWTSTAMP_FILTER_PTP_V1_L4_EVENT,
    RPC_HWTSTAMP_FILTER_PTP_V1_L4_SYNC,
    RPC_HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ,
    RPC_HWTSTAMP_FILTER_PTP_V2_L4_EVENT,
    RPC_HWTSTAMP_FILTER_PTP_V2_L4_SYNC,
    RPC_HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ,
    RPC_HWTSTAMP_FILTER_PTP_V2_L2_EVENT,
    RPC_HWTSTAMP_FILTER_PTP_V2_L2_SYNC,
    RPC_HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ,
    RPC_HWTSTAMP_FILTER_PTP_V2_EVENT,
    RPC_HWTSTAMP_FILTER_PTP_V2_SYNC,
    RPC_HWTSTAMP_FILTER_PTP_V2_DELAY_REQ,
    RPC_HWTSTAMP_FILTER_NTP_ALL,

    RPC_HWTSTAMP_FILTER_UNKNOWN, /**< Unknown filter */
} rpc_hwtstamp_rx_filters;

/**
 * Convert RPC constant to corresponsing value from hwtstamp_rx_filters
 * enum.
 *
 * @param filter      RPC constant
 *
 * @return Corresponding native value or @c -1 if no such value exists.
 */
extern int hwtstamp_rx_filters_rpc2h(rpc_hwtstamp_rx_filters filter);

/**
 * Convert constant from hwtstamp_rx_filters enum to corresponding RPC
 * constant.
 *
 * @param filter     Native constant
 *
 * @return Corresponding RPC value or @c RPC_HWTSTAMP_FILTER_UNKNOWN if no
 *         such value exists.
 */
extern rpc_hwtstamp_rx_filters hwtstamp_rx_filters_h2rpc(int filter);

/**
 * Get string name of a constant from rpc_hwtstamp_rx_filters enum.
 *
 * @param filter        RPC constant
 *
 * @return Constant name.
 */
extern const char *hwtstamp_rx_filters_rpc2str(rpc_hwtstamp_rx_filters filter);

/**
 * Convert native flags from hwtstamp_rx_filters to RPC ones.
 * These flags are returned by ioctl(SIOCETHTOOL/ETHTOOL_GET_TS_INFO).
 * Every member of enum hwtstamp_rx_filters has a corresponding bit,
 * for example HWTSTAMP_FILTER_ALL -> (1 << HWTSTAMP_FILTER_ALL).
 * If this function sees (1 << HWTSTAMP_FILTER_ALL) bit set, it sets
 * (1 << RPC_HWTSTAMP_FILTER_ALL) bit in the result.
 *
 * @param flags   Native flags
 *
 * @return RPC flags.
 */
extern unsigned int hwtstamp_rx_filters_flags_h2rpc(unsigned int flags);

/**
 * Append string representation of flags produced by
 * hwtstamp_rx_filters_flags_h2rpc() to TE string.
 *
 * @param flags   Flags
 * @param str     Where to append string representation
 *
 * @return Status code.
 */
extern te_errno hwtstamp_rx_filters_flags_rpc2te_str(unsigned int flags,
                                                     te_string *str);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_RPC_LINUX_NET_TSTAMP_H__ */
