/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Common definitions for BPF programs and tests
 *
 * Common definitions for BPF programs and tests
 *
 * You should define TE_BPF_U8 before including this header.
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */
#ifndef __TE_BPF_COMMON_H__
#define __TE_BPF_COMMON_H__

/** Maximum size of IP address (is equal to IPv6 address size) */
#define TE_MAX_IP_ADDR_LEN 16

/**
 * Parameters of IPv4/IPv6 TCP/UDP filter.
 *
 * Note: here uint8 is used everywhere to avoid issues with padding and
 * ensure that structure filled on Test Engine host is interpreted
 * correctly on TA.
 */
typedef struct te_bpf_ip_tcpudp_filter {
    TE_BPF_U8 ipv4; /**< If nonzero, IPv4 packets are expected,
                         otherwise IPv6 packets */
    TE_BPF_U8 src_ip_addr[TE_MAX_IP_ADDR_LEN]; /**< Source IP address
                                                   (if it is all-zeroes
                                                   address, any address
                                                   will match) */
    TE_BPF_U8 dst_ip_addr[TE_MAX_IP_ADDR_LEN]; /**< Destination IP address
                                                    (if it is all-zeroes
                                                    address, any address
                                                    will match) */
    TE_BPF_U8 protocol; /**< IPPROTO_TCP or IPPROTO_UDP */
    TE_BPF_U8 src_port[2]; /**< Source TCP/UDP port (if zero, any port
                               matches) */
    TE_BPF_U8 dst_port[2]; /**< Destination TCP/UDP port (if zero, any port
                                matches) */
} te_bpf_ip_tcpudp_filter;

/**
 * Parameters for rxq_stats BPF program.
 */
typedef struct te_bpf_rxq_stats_params {
    TE_BPF_U8 enabled; /**< Nonzero if packets processing is enabled */
    te_bpf_ip_tcpudp_filter filter; /**< Filter telling which packets
                                         should be counted */
} te_bpf_rxq_stats_params;

#endif /* !__TE_BPF_COMMON_H__ */
