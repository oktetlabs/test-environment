/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Traffic Application Domain definitions
 *
 * Common RCF Traffic Application Domain definitions.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */
#ifndef __TE_TAD_COMMON_H__
#define __TE_TAD_COMMON_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_stdint.h"

/** Maximum number of IPv4/IPv6 PDUs allowed in template */
#define TMPL_NB_IP_PDUS_MAX 2

/** The number of bits in a 32-bit word */
#define WORD_32BIT 32

/** The number of bytes in a 32-bit word */
#define WORD_4BYTE 4

/** The number of bits in a byte */
#define BITS_PER_BYTE 8

/**
 * Infinitive timeout to wait forever.
 *
 * @todo Put it in appropriate place.
 */
#define TAD_TIMEOUT_INF     (unsigned int)(-1)

/**
 * Default receive timeout of the CSAP.
 *
 * @todo Put it in appropriate place.
 */
#define TAD_TIMEOUT_DEF     (unsigned int)(-2)

#define CSAP_PARAM_STATUS               "status"
#define CSAP_PARAM_TOTAL_BYTES          "total_bytes"
#define CSAP_PARAM_TOTAL_SENT           "total_sent"
#define CSAP_PARAM_TOTAL_RECEIVED       "total_received"
#define CSAP_PARAM_FIRST_PACKET_TIME    "first_pkt_time"
#define CSAP_PARAM_LAST_PACKET_TIME     "last_pkt_time"
#define CSAP_PARAM_NO_MATCH_PKTS        "no_match_pkts"

/**
 * Type for CSAP handle, should have semantic unsigned integer,
 * because TAD Users Guide specify CSAP ID as positive integer, and
 * zero used as mark of invalid/uninitialized CSAP handle.
 */
typedef unsigned int csap_handle_t;
enum {
    CSAP_INVALID_HANDLE = 0, /**< Constant for invalid CSAP handle */
};

typedef enum {
    CSAP_IDLE,          /**< CSAP is ready for traffic operations or
                             destroy */
    CSAP_BUSY,          /**< CSAP is busy with some traffic processing */
    CSAP_COMPLETED,     /**< Last traffic processing completed and CSAP
                             waiting for *_stop command from Test. */
    CSAP_ERROR,         /**< Error was occured during processing, CSAP
                             waiting for *_stop command from Test. */
} tad_csap_status_t;

/**
 * Constants for ASN.1 tags of protocol choices in PDUs and CSAPs
 * and for marks in CSAP instance and CSAP support descriptors.
 */
typedef enum {
    TE_PROTO_INVALID = 0, /**< Invalid protocol, for error and undef */

    TE_PROTO_AAL5,
    TE_PROTO_ARP,
    TE_PROTO_ATM,
    TE_PROTO_BRIDGE,
    TE_PROTO_CLI,
    TE_PROTO_DHCP,
    TE_PROTO_DHCP6,
    TE_PROTO_ETH,
    TE_PROTO_ICMP4,
    TE_PROTO_IP4,
    TE_PROTO_ISCSI,
    TE_PROTO_PCAP,
    TE_PROTO_SNMP,
    TE_PROTO_TCP,
    TE_PROTO_UDP,
    TE_PROTO_SOCKET,
    TE_PROTO_IGMP,
    TE_PROTO_ICMP6,
    TE_PROTO_IP6,
    TE_PROTO_PPP,
    TE_PROTO_PPPOE,
    TE_PROTO_RTE_MBUF,
    TE_PROTO_VXLAN,
    TE_PROTO_GENEVE,
    TE_PROTO_GRE,
} te_tad_protocols_t;

/**
 * Calculate 16-bit checksum: one's complement sum of all 16 bit words.
 * Function works correctly with length less then 64k.
 *
 * @note One's complement of the computed value should be written
 *       to checksum field in IP/TCP/UDP headers.
 *
 * @param checksum  start checksum calculating from this value
 * @param data      pointer to the data which checksum should be calculated
 * @param length    length of the data
 *
 * @return calculated checksum.
 */
static inline uint16_t
ip_csum_part(uint32_t checksum, const void *data, size_t length)
{
    uint16_t *ch_p;

    for (ch_p = (uint16_t *)data; length >= 2; length -= 2)
        checksum += *(ch_p++);

    if (length == 1)
    {
        union {uint8_t bytes[2]; uint16_t num;} a;
        a.bytes[0] = *((uint8_t *)ch_p);
        a.bytes[1] = 0;
        checksum += a.num;
    }

    checksum = (checksum & 0xffff) + (checksum >> 16);
    return checksum + (checksum >> 16);
}

/**
 * Calculate 16-bit checksum: 16-bit one's complement sum of all 16 bit words.
 * Function works correctly with length less then 64k.
 *
 * @note One's complement of the computed value should be written
 *       to checksum field in IP/TCP/UDP headers.
 *
 * @param data      pointer to the data which checksum should be calculated
 * @param length    length of the data
 *
 * @return calculated checksum.
 */
static inline uint16_t
calculate_checksum(const void *data, size_t length)
{
    return ip_csum_part(0, data, length);
}

/**
 * Callback type for methods generating fully determined stream of data,
 * depending only from length and offset.
 *
 * @param offset        offset in stream of first byte in buffer
 * @param length        length of buffer
 * @param buffer        location for generated data (OUT)
 *
 * @return status code
 */
typedef int (*tad_stream_callback)(uint64_t offset, uint32_t length,
                                   uint8_t *buffer);

/**
 * Receive mode flags.
 *
 * @note There is no good place to define these flags, since they are
 *       common for Test Engine and Test Agent side and common for
 *       Ethernet and PCAP CSAPs.
 */
enum tad_eth_recv_mode {
    TAD_ETH_RECV_HOST  = 0x01,  /**< To us */
    TAD_ETH_RECV_BCAST = 0x02,  /**< To all */
    TAD_ETH_RECV_MCAST = 0x04,  /**< To multicast group */
    TAD_ETH_RECV_OTHER = 0x08,  /**< To someone else */
    TAD_ETH_RECV_OUT   = 0x10,  /**< Outgoing of any type */
    /** Do not enter promiscuous mode even if TAD_ETH_RECV_OTHER is given */
    TAD_ETH_RECV_NO_PROMISC = 0x100,
};

/** Receive all packets */
#define TAD_ETH_RECV_ALL    (TAD_ETH_RECV_HOST | TAD_ETH_RECV_OTHER |  \
                             TAD_ETH_RECV_BCAST | TAD_ETH_RECV_MCAST | \
                             TAD_ETH_RECV_OUT)

/** Default mode is to receive everything except outgoing packets */
#define TAD_ETH_RECV_DEF    (TAD_ETH_RECV_ALL & (~TAD_ETH_RECV_OUT))

/** Receive nothing */
#define TAD_ETH_RECV_NO     (0)

/** Default IPv4 header size (without options) */
#define TAD_IP4_HDR_LEN     20
/** Default IPv6 header size (without options) */
#define TAD_IP6_HDR_LEN     40
/** Default TCP header size (without options) */
#define TAD_TCP_HDR_LEN     20
/** Length of UDP header */
#define TAD_UDP_HDR_LEN     8

/**
 * Convert sockaddr structures to address/port arguments passed
 * to CSAP creation functions.
 *
 * @param _loc        Local address.
 * @param _rem        Remote address.
 */
#define TAD_SA2ARGS(_loc, _rem) \
    te_sockaddr_get_netaddr(_loc),    \
    te_sockaddr_get_netaddr(_rem),    \
    (_loc == NULL ?                   \
        -1 :                          \
        te_sockaddr_get_port(_loc)),  \
    (_rem == NULL ?                   \
        -1 :                          \
        te_sockaddr_get_port(_rem))

#endif /* !__TE_TAD_COMMON_H__ */
