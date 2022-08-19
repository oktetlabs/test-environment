/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief API for BPF programs
 *
 * Common functions and structures to use in BPF programs.
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */
#ifndef __TE_BPF_HELPERS_H__
#define __TE_BPF_HELPERS_H__

#include <stddef.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/ipv6.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <sys/socket.h>

#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

#define TE_BPF_U8 __u8
#include "te_bpf_common.h"

#ifndef NULL
#define NULL (void *)0
#endif

/**
 * Maximum number of IPv6 extension headers.
 * There are 4 known extension headers which may be encountered in TCP or
 * UDP packets, one of them (Destination Options) can appear twice.
 */
#define TE_IPV6_MAX_EXT_HEADERS 5

/** Minimum value of Ethertype field */
#define TE_MIN_ETH_TYPE 1536

/** Structure describing an Ethernet frame for XDP programs */
typedef struct te_xdp_frame {
    __u8 *start; /**< Start of the frame */
    __u8 * const end; /**< End of the frame */

    __u8 *l2_hdr; /**< Pointer to level 2 header */
    __u32 l2_type; /**< Type of level 2 header */

    __u8 *l3_hdr; /**< Pointer to level 3 header */
    __u32 l3_type; /**< Type of level 3 header */

    __u8 *end_parsed; /**< Pointer to the first byte after
                           parsed headers */
} te_xdp_frame;

/**
 * Initializer for te_xdp_frame.
 *
 * @param _ctx    Pointer to struct xdp_md.
 */
#define TE_XDP_FRAME_INIT(_ctx) \
    { .start = (__u8 *)(long)(_ctx->data), \
      .end = (__u8 *)(long)(_ctx->data_end) }

/**
 * Check whether packet processing function succeeded,
 * return error (@c -1) if it did not.
 *
 * @param _exp    Expression to check.
 */
#define TE_BPF_CHECK_RC(_expr) \
    do {                      \
        int _rc = (_expr);    \
        if (_rc < 0)          \
            return -1;        \
    } while (0)

/**
 * Get uint16 value from a packet in XDP program
 * (converting it to host byte order).
 *
 * @param p       Where to get value
 * @param p_end   Pointer to the end of packet
 * @param result  Where to save obtained value
 *
 * @return Status code
 *
 * @retval 0      Success
 * @retval -1     Failure
 */
static __always_inline int
te_xdp_get_u16_ho(__u8 *p, __u8 * const p_end, __u16 *result)
{
    if (p + sizeof(__u16) > p_end)
        return -1;

    *result = __bpf_ntohs(*(__u16 *)p);
    return 0;
}

/**
 * Advance current offset checking that it is still inside
 * the packet.
 *
 * @param p       Pointer to the pointer holding current position
 * @param p_end   Pointer to the end of packet
 * @param num     How many bytes to skip
 *
 * @return Status code
 *
 * @retval 0      Success
 * @retval -1     Failure
 */
static __always_inline int
te_xdp_skip_bytes(__u8 **p, __u8 * const p_end, int num)
{
    if (*p + num > p_end)
        return -1;

    *p += num;
    return 0;
}

/**
 * Skip VLAN tag in Ethernet header.
 *
 * @param p       Pointer to the pointer holding current position
 * @param p_end   Pointer to the end of packet
 *
 * @return Status code
 *
 * @retval 0      Success (if there is no VLAN tag, success is returned)
 * @retval -1     Failure (not enough bytes in the packet)
 */
static __always_inline int
te_xdp_skip_vlan(__u8 **p, __u8 * const p_end)
{
    __u16 tag;

    if (te_xdp_get_u16_ho(*p, p_end, &tag) < 0)
        return -1;

    if (tag == ETH_P_8021Q || tag == ETH_P_8021AD)
        return te_xdp_skip_bytes(p, p_end, sizeof(tag));

    return 0;
}

/**
 * Parse IPv4 header (it is assumed that it is level 2 header in Ethernet
 * frame).
 *
 * @param frame       Pointer to frame structure
 *
 * @return Status code
 *
 * @retval 0      Success
 * @retval -1     Failure
 */
static __always_inline int
te_xdp_parse_ipv4(te_xdp_frame *frame)
{
    __u8 *p;
    struct iphdr *hdr;
    int proto;
    int hdr_len;

    if (frame == NULL)
        return -1;

    p = frame->l2_hdr;
    hdr = (struct iphdr *)(p);

    if (p + sizeof(*hdr) > frame->end)
        return -1;

    if (hdr->version != 4)
        return -1;

    proto = hdr->protocol;
    hdr_len = hdr->ihl * 4;

    if (hdr_len < sizeof(*hdr))
        return -1;

    if (p + hdr_len > frame->end)
        return -1;

    frame->l3_type = proto;
    if (proto != IPPROTO_IP)
        frame->l3_hdr = p + hdr_len;

    frame->end_parsed = p + hdr_len;

    return 0;
}

/*
 * Skip IPv6 extension header.
 *
 * @param p           Pointer to the pointer holding current position
 * @param p_end       Pointer to the end of packet
 * @param hdr_type    Pointer to type of the current extension header
 *                    (updated by this function)
 *
 * @return Status code
 *
 * @retval 1      Current header is not extension header
 * @retval 0      Success
 * @retval -1     Failure
 */
static __always_inline int
te_xdp_skip_ipv6_ext_hdr(__u8 **p, __u8 * const p_end, __u8 *hdr_type)
{
    int len;

    if (*hdr_type == IPPROTO_NONE ||
        (*hdr_type != IPPROTO_HOPOPTS && *hdr_type != IPPROTO_ROUTING &&
         *hdr_type != IPPROTO_FRAGMENT && *hdr_type != IPPROTO_DSTOPTS))
        return 1;

    if (*p + 2 > p_end)
        return -1;

    *hdr_type = **p;

    /* IPv6 fragment header does not have length field */
    if (*hdr_type == IPPROTO_FRAGMENT)
        len = 0;
    else
        len = *(*p + 1);

    /* Length is in 8-bytes units, excluding the first 8 bytes */
    return te_xdp_skip_bytes(p, p_end, len * 8 + 8);
}

/**
 * Parse IPv6 header (it is assumed that it is level 2 header in Ethernet
 * frame).
 *
 * @param frame       Pointer to frame structure
 *
 * @return Status code
 *
 * @retval 0      Success
 * @retval -1     Failure
 */
static __always_inline int
te_xdp_parse_ipv6(te_xdp_frame *frame)
{
    __u8 nxt_hdr;
    int i;
    int rc;

    __u8 *p;
    struct ipv6hdr *hdr;

    if (frame == NULL)
        return -1;

    p = frame->l2_hdr;
    hdr = (struct ipv6hdr *)(p);

    if (p + sizeof(*hdr) > frame->end)
        return -1;

    nxt_hdr = hdr->nexthdr;

    p += sizeof(*hdr);

#pragma unroll
    /*
     * Here "+1" helps to check that the last header is not extension one.
     */
    for (i = 0; i < TE_IPV6_MAX_EXT_HEADERS + 1; i++)
    {
        rc = te_xdp_skip_ipv6_ext_hdr(&p, frame->end, &nxt_hdr);
        if (rc == 1)
            break;
        if (rc < 0)
            return -1;
    }

    if (rc == 1 && nxt_hdr != IPPROTO_NONE)
    {
        frame->l3_hdr = p;
        frame->l3_type = nxt_hdr;
    }

    frame->end_parsed = p;

    return 0;
}

/**
 * Parse headers in Ethernet frame, saving pointers to discovered
 * headers and their types in the frame structure.
 *
 * @param frame       Pointer to frame structure
 *
 * @return Status code
 *
 * @retval 0      Success
 * @retval -1     Failure
 */
static __always_inline int
te_xdp_parse_eth_frame(te_xdp_frame *frame)
{
    __u8 *p;
    __u16 ether_type;

    if (frame == NULL)
        return -1;

    p = frame->start;
    frame->l2_hdr = NULL;
    frame->l3_hdr = NULL;

    TE_BPF_CHECK_RC(te_xdp_skip_bytes(&p, frame->end, ETH_ALEN * 2));
    /* Two VLAN tags may be present */
    TE_BPF_CHECK_RC(te_xdp_skip_vlan(&p, frame->end));
    TE_BPF_CHECK_RC(te_xdp_skip_vlan(&p, frame->end));

    TE_BPF_CHECK_RC(te_xdp_get_u16_ho(p, frame->end, &ether_type));

    p += sizeof(ether_type);
    frame->end_parsed = p;

    if (ether_type < TE_MIN_ETH_TYPE)
        return 0;

    frame->l2_hdr = p;
    frame->l2_type = ether_type;

    switch (ether_type)
    {
        case ETH_P_IP:
            TE_BPF_CHECK_RC(te_xdp_parse_ipv4(frame));
            break;

        case ETH_P_IPV6:
            TE_BPF_CHECK_RC(te_xdp_parse_ipv6(frame));
            break;
    }

    return 0;
}

/*
 * The following macros are used because generic loops were not allowed
 * in BPF until Linux 5.3, and "pragma unroll" is used as a workaround.
 */

/**
 * Macro defining a function which checks whether a given sequence of bytes
 * is the same in a packet and in a memory array.
 *
 * Resulting function returns @c 0 if there is no match (or not enough
 * bytes left in a packet) and @c 1 if there is a match.
 *
 * @param _n      Number of bytes to compare.
 */
#define TE_XDP_EQ_FUNC(_n) \
    static __always_inline int                                        \
    te_xdp_eq_ ## _n(__u8 *p_pkt, __u8 * const p_end, __u8 *p_mem)    \
    {                                                                 \
        if (p_pkt + _n > p_end)                                       \
            return 0;                                                 \
                                                                      \
        _Pragma("unroll")                                             \
        for (int i = 0; i < _n; i++)                                  \
        {                                                             \
            if (p_pkt[i] != p_mem[i])                                 \
                return 0;                                             \
        }                                                             \
                                                                      \
        return 1;                                                     \
    }

TE_XDP_EQ_FUNC(2);
TE_XDP_EQ_FUNC(4);
TE_XDP_EQ_FUNC(16);

/**
 * Macro defining a function which checks whether a given sequence of bytes
 * in memory is all zeroes.
 *
 * Resulting function returns @c 0 if there is no match and
 * @c 1 if there is a match.
 *
 * @param _n      Number of bytes to check.
 */
#define TE_IS_ZERO_FUNC(_n) \
    static __always_inline int                                        \
    te_is_zero_ ## _n(__u8 *p_mem)                                    \
    {                                                                 \
        _Pragma("unroll")                                             \
        for (int i = 0; i < _n; i++)                                  \
        {                                                             \
            if (p_mem[i] != 0)                                        \
                return 0;                                             \
        }                                                             \
                                                                      \
        return 1;                                                     \
    }

TE_IS_ZERO_FUNC(2);
TE_IS_ZERO_FUNC(4);
TE_IS_ZERO_FUNC(16);

/**
 * Macro defining a function which checks that either a given sequence of
 * bytes is the same in a packet and in a memory array, or memory array
 * contains only zeroes.
 *
 * Resulting function returns @c 0 if there is no match and
 * @c 1 if there is a match.
 *
 * @param _n      Number of bytes to compare.
 */
#define TE_XDP_EQ_OR_ZERO_FUNC(_n) \
    static __always_inline int                                        \
    te_xdp_eq_or_zero_ ## _n(__u8 *p_pkt, __u8 * const p_end,         \
                             __u8 *p_mem)                             \
    {                                                                 \
        int rc;                                                       \
        rc = te_xdp_eq_ ## _n(p_pkt, p_end, p_mem);                   \
        if (rc != 0)                                                  \
            return rc;                                                \
                                                                      \
        return te_is_zero_ ## _n(p_mem);                              \
    }

TE_XDP_EQ_OR_ZERO_FUNC(2);
TE_XDP_EQ_OR_ZERO_FUNC(4);
TE_XDP_EQ_OR_ZERO_FUNC(16);

/**
 * Check whether a parsed frame matches a given IP+TCP/UDP filter.
 *
 * @param frame         Frame structure
 * @param filter        Filter structure
 *
 * @return Matching result
 *
 * @retval 1      Matched
 * @retval 0      Not matched
 */
static __always_inline int
te_xdp_frame_match_ip_tcpudp(const te_xdp_frame *frame,
                             te_bpf_ip_tcpudp_filter *filter)
{
    if (frame == NULL)
        return 0;

    if (frame->l2_hdr == NULL)
        return 0;

    if (filter->ipv4 && frame->l2_type != ETH_P_IP)
        return 0;

    if (!filter->ipv4 && frame->l2_type != ETH_P_IPV6)
        return 0;

    if (frame->l3_hdr == NULL ||
        (frame->l3_type != IPPROTO_TCP &&
         frame->l3_type != IPPROTO_UDP))
        return 0;

    if (filter->protocol != frame->l3_type)
        return 0;

    if (filter->ipv4)
    {
        struct iphdr *hdr = (struct iphdr *)(frame->l2_hdr);

        if (!te_xdp_eq_or_zero_4((__u8 *)(&hdr->saddr), frame->end,
                                 filter->src_ip_addr))
            return 0;

        if (!te_xdp_eq_or_zero_4((__u8 *)(&hdr->daddr), frame->end,
                                 filter->dst_ip_addr))
            return 0;
    }
    else
    {
        struct ipv6hdr *hdr = (struct ipv6hdr *)(frame->l2_hdr);

        if (!te_xdp_eq_or_zero_16((__u8 *)(&hdr->saddr), frame->end,
                                  filter->src_ip_addr))
            return 0;

        if (!te_xdp_eq_or_zero_16((__u8 *)(&hdr->daddr), frame->end,
                                  filter->dst_ip_addr))
            return 0;
    }

    if (!te_xdp_eq_or_zero_2(frame->l3_hdr, frame->end,
                             filter->src_port))
        return 0;

    if (!te_xdp_eq_or_zero_2(frame->l3_hdr + 2, frame->end,
                             filter->dst_port))
        return 0;

    return 1;
}

#endif /* !__TE_BPF_HELPERS_H__ */
