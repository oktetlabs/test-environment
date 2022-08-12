/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * ICMP messages generating routines.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#define TE_LGR_USER     "TAD ICMP"

#include "te_config.h"

#include <string.h>
#include <stdlib.h>

#include "te_ethernet.h"

#include "tad_ipstack_impl.h"

#ifndef IP4_HDR_LEN
#define IP4_HDR_LEN 20
#endif

#ifndef IP4_ADDR_LEN
#define IP4_ADDR_LEN 4
#endif

#ifndef IP6_HDR_LEN
#define IP6_HDR_LEN 40
#endif

#ifndef ICMP_HDR_LEN
#define ICMP_HDR_LEN 8
#endif

#ifndef IPV6_MTU_MIN_VAL
#define IPV6_MTU_MIN_VAL 1280
#endif

#ifndef DEFAULT_TTL
#define DEFAULT_TTL 64
#endif

/**
 * Copy _len bytes from _src to _dst and increment after _dst pointer
 *
 * @param   _dst    Destination (uint8_t *)
 * @param   _src    Source (uint8_t *)
 * @param   _len    Count of bytes to copy
 *
 */
#define COPY_DATA_INCR(_dst, _src, _len)    \
    do {                                    \
        memcpy(_dst, _src, _len);           \
        _dst += _len;                       \
    } while (0)

/**
 * Write data to ICMP header (without checksum).
 *
 * @param   ptr         Start of ICMP header
 * @param   type        ICMP type
 * @param   code        ICMP code
 * @param   rest_hdr    Rest of Header value
 * @param   csum_ptr    After return csum_ptr will contains address
 *                      of ICMP checksum
 *
 * @return  pointer to the next header
 */
static uint8_t *
tad_icmp_build_icmp_hdr(uint8_t *ptr, const uint8_t type, const uint8_t code,
                        const uint32_t rest_hdr, uint16_t **csum_ptr)
{
    /* common ICMP header */
    *ptr++ = type;
    *ptr++ = code;

    /* initialize checksum as 0 and leave place for header checksum */
    *(uint16_t *)ptr = 0;
    *csum_ptr = (uint16_t *)ptr;
    ptr += sizeof(uint16_t);

    *(uint32_t *)ptr = htonl(rest_hdr);
    ptr += sizeof(uint32_t);

    return ptr;
}

/**
 * Build IPv4 header for ICMP response from original (received) packet
 *
 * @param   ptr         Start of IPv4 header
 * @param   orig_pkt    Source packet
 * @param   ip_msg_len  Full length with ICMP and payload
 *
 * @return  pointer to the next header
 */
static uint8_t *
tad_icmp_build_ipv4_hdr(uint8_t *ptr, const uint8_t *orig_pkt, size_t ip_msg_len)
{
    uint8_t    *ptr_hdr;
    uint16_t   *csum_ptr;

    ptr_hdr = ptr;
    /* vers, hlen */
    *ptr++ = (IP4_VERSION << IP_HDR_VERSION_SHIFT) +
             (uint8_t)(IP4_HDR_LEN / sizeof(uint32_t));

    /* tos */
    COPY_DATA_INCR(ptr, orig_pkt + sizeof(uint8_t), sizeof(uint8_t));

    /* ip len */
    *(uint16_t *)ptr = htons(ip_msg_len);
    ptr += sizeof(uint16_t);

    /* ip ident */
    *(uint16_t *)ptr = (uint16_t)rand();
    ptr += sizeof(uint16_t);

    /* flags & offset */
    *(uint16_t *)ptr = 0;
    ptr += sizeof(uint16_t);

    /* TTL */
    *ptr++ = DEFAULT_TTL;

    *ptr++ = IPPROTO_ICMP;

    /* initialize checksum as 0 and leave place for header checksum */
    *(uint16_t *)ptr = 0;
    csum_ptr = (uint16_t *)ptr;
    ptr += sizeof(uint16_t);

    /* dst and srt ipv4 adresses */
    COPY_DATA_INCR(ptr, orig_pkt + sizeof(uint32_t) * IP4_HDR_DST_OFFSET,
                   IP4_ADDR_LEN);
    COPY_DATA_INCR(ptr, orig_pkt + sizeof(uint32_t) * IP4_HDR_SRC_OFFSET,
                   IP4_ADDR_LEN);

    /* set header checksum */
    *csum_ptr = ~calculate_checksum(ptr_hdr, IP4_HDR_LEN);

    return ptr;
}

/**
 * Build IPv6 header for ICMP response from original (received) packet
 *
 * @param   ptr         Start of IPv6 header
 * @param   orig_pkt    Source packet
 * @param   payload_len Length of payload data
 *
 * @return  pointer to the next header
 */
static uint8_t *
tad_icmp_build_ipv6_hdr(uint8_t *ptr, const uint8_t *orig_pkt,
                        size_t payload_len)
{
    /* vers, traffic class, flow label */
    COPY_DATA_INCR(ptr, orig_pkt, sizeof(uint32_t));

    /* payload len */
    *(uint16_t *)ptr = htons(ICMP_HDR_LEN + payload_len);
    ptr += sizeof(uint16_t);

    *ptr++ = IPPROTO_ICMPV6;

    /* hope limit */
    *ptr++ = DEFAULT_TTL;

    /* src and dst ipv6 adresses */
    COPY_DATA_INCR(ptr, orig_pkt + sizeof(uint32_t) * IP6_HDR_DST_OFFSET,
                   IP6_ADDR_LEN);
    COPY_DATA_INCR(ptr, orig_pkt + sizeof(uint32_t) * IP6_HDR_SRC_OFFSET,
                   IP6_ADDR_LEN);
    return ptr;
}

/**
 * Function for make ICMP error by IP packet, catched by '*.ip{4,6}.eth'
 * raw CSAP.
 * Prototype made according with 'tad_processing_pkt_method' function type.
 * This method uses write_cb callback of passed 'eth' CSAP for send reply.
 * User parameter should contain integer numbers, separated by colon:
 * "<type>:<code>[:<unused>[:<rate>]]".
 * <unused> contains number to be placed in the 'unused' 32-bit field of
 * ICMP error (in host order). Default value is zero.
 * <rate> contains number of original packets per one ICMP error.
 * Default value is 1.
 *
 * @param csap  CSAP descriptor structure.
 * @param usr_param   String passed by user.
 * @param orig_pkt    Packet binary data, as it was caught from net.
 * @param pkt_len     Length of pkt data.
 *
 * @return zero on success or error code.
 */
int
tad_icmp_error(csap_p csap, const char *usr_param,
               const uint8_t *orig_pkt, size_t pkt_len)
{
    csap_spt_type_p rw_layer_cbs;

    tad_pkt    *pkt;

    uint8_t     type,
                code;
    int         rc = 0;
    char       *endptr;
    size_t      msg_len;

    uint8_t    *msg, *p;
    uint16_t   *csum_ptr;
    uint32_t    unused = 0;
    int         rate = 1;
    uint16_t    eth_type;
    uint8_t     ip_version;

    uint16_t    payload_len;

    if (csap == NULL || usr_param == NULL || orig_pkt == NULL ||
        pkt_len < ETHER_HDR_LEN)
        return TE_EWRONGPTR;

    type = strtol(usr_param, &endptr, 10);
    if ((endptr == NULL) || (*endptr != ':'))
    {
        ERROR("%s(): wrong usr_param, should be <type>:<code>",
              __FUNCTION__);
        return TE_EINVAL;
    }
    endptr++;
    code = strtol(endptr, &endptr, 10);

    if (endptr != NULL && *endptr != '\0')
    {
        if (*endptr != ':')
        {
            ERROR("%s(): wrong usr_param, shouls be colon.", __FUNCTION__);
            return TE_EINVAL;
        }
        endptr++;
        unused = strtol(endptr, &endptr, 10);
        if (endptr != NULL && *endptr != '\0')
        {
            if (*endptr != ':')
            {
                ERROR("%s(): wrong usr_param, shouls be colon.",
                      __FUNCTION__);
                return TE_EINVAL;
            }
            endptr++;
            rate = atoi(endptr);
            if (rate == 0)
            {
                ERROR("%s(): wrong rate in usr_param, shouls be non-zero",
                      __FUNCTION__);
                return TE_EINVAL;
            }
        }
    }

    if ((rand() % rate) != 0)
        return 0;

    rw_layer_cbs = csap_get_proto_support(csap, csap_get_rw_layer(csap));
    if (rw_layer_cbs->prepare_send_cb != NULL &&
        (rc = rw_layer_cbs->prepare_send_cb(csap)) != 0)
    {
        ERROR("%s(): prepare for recv failed %r", __FUNCTION__, rc);
        return rc;
    }

/*
 * RFC792 requires to send IP header + 64 bits of payload, however,
 * 64 bits (8 bytes) are not sufficient even for TCP header without
 * any options (Solaris requires to have full TCP header in ICMP error).
 */
#define ICMP_PLD_SIZE 32

    eth_type = ntohs(*(uint16_t *)(orig_pkt + 2 * ETHER_ADDR_LEN));

    /* FIXME: VLANs is not supported */
    if (eth_type != ETHERTYPE_IP && eth_type != ETHERTYPE_IPV6)
    {
        ERROR("%s(): unsupported ethernet type received:0x%x", __FUNCTION__,
              eth_type);
        return TE_EPROTONOSUPPORT;
    }

    /* Detect IP version */
    if (pkt_len < ETHER_HDR_LEN + 1)
    {
        return TE_EWRONGPTR;
    }
    else
    {
        ip_version = orig_pkt[ETHER_HDR_LEN] >> IP_HDR_VERSION_SHIFT;
    }

    if (ip_version == IP4_VERSION && eth_type == ETHERTYPE_IP)
    {
        if (pkt_len < (ETHER_HDR_LEN + IP4_HDR_LEN))
            return TE_EWRONGPTR;

        payload_len = MIN(ICMP_PLD_SIZE, pkt_len - ETHER_HDR_LEN);

        /* ICMP response will send without IPv4 options */
        msg_len = ETHER_HDR_LEN + IP4_HDR_LEN + ICMP_HDR_LEN + payload_len;
    }
    else if (ip_version == IP6_VERSION && eth_type == ETHERTYPE_IPV6)
    {
        if (pkt_len < (ETHER_HDR_LEN + IP6_HDR_LEN))
            return TE_EWRONGPTR;

        payload_len = pkt_len - ETHER_HDR_LEN;

        /* Response length should be less or equal of minimum IPv6 MTU value */
        if (payload_len > IPV6_MTU_MIN_VAL)
            payload_len = IPV6_MTU_MIN_VAL;

        msg_len = ETHER_HDR_LEN + IP6_HDR_LEN + ICMP_HDR_LEN + payload_len;
    }
    else
    {
        ERROR("%s(): wrong IP:%u and/or ethertype:0x%x version!", __FUNCTION__,
              ip_version, eth_type);
        return TE_EPROTONOSUPPORT;
    }

    pkt = tad_pkt_alloc(1, msg_len);
    if (pkt == NULL)
    {
        ERROR("%s(): no memory!", __FUNCTION__);
        return TE_ENOMEM;
    }
    p = msg = tad_pkt_first_seg(pkt)->data_ptr;

    /* Ethernet header */
    memcpy(p, orig_pkt + ETHER_ADDR_LEN, ETHER_ADDR_LEN);
    memcpy(p + ETHER_ADDR_LEN, orig_pkt, ETHER_ADDR_LEN);
    memcpy(p + 2 * ETHER_ADDR_LEN, orig_pkt + 2 * ETHER_ADDR_LEN,
           sizeof(uint16_t));
    p += ETHER_HDR_LEN;
    orig_pkt += ETHER_HDR_LEN;
    pkt_len -= ETHER_HDR_LEN;

    if (ip_version == IP4_VERSION)
    {
        /* IPv4 header, now leave orig_pkt unchanged */
        p = tad_icmp_build_ipv4_hdr(p, orig_pkt, msg_len - ETHER_HDR_LEN);

        p = tad_icmp_build_icmp_hdr(p, type, code, unused, &csum_ptr);

        /* set ICMPv4 checksum */
        memcpy(p, orig_pkt, MIN(ICMP_PLD_SIZE, pkt_len));
        *csum_ptr = ~calculate_checksum(msg + ETHER_HDR_LEN + IP4_HDR_LEN,
                                        ICMP_HDR_LEN + ICMP_PLD_SIZE);
    }
    else
    {
        uint16_t    csum;
        uint8_t     ipv6_pseudo_header_part[6];

        /* IPv6 header, now leave orig_pkt unchanged */
        p = tad_icmp_build_ipv6_hdr(p, orig_pkt, payload_len);

        p = tad_icmp_build_icmp_hdr(p, type, code, unused, &csum_ptr);
        memcpy(p, orig_pkt, payload_len);

        /* IP pseudo header will not built, checksum is calculated
         * from a most of the data based on 'orig_pkt'. Remaining part is
         * filled in 'ipv6_pseudo_header_part' and after this, the final
         * checksum is calculated from three parts
         */

        /* calculate payload checksum */
        csum = ip_csum_part(0, orig_pkt, payload_len);

        /* calculate IPv6 addresses checksum */
        csum = ip_csum_part(csum,
                            orig_pkt + sizeof(uint32_t) * IP6_HDR_SRC_OFFSET,
                            2 * IP6_ADDR_LEN);

        /* prepare last part of data to be calculated */
        p = (uint8_t *)&ipv6_pseudo_header_part;
        *(uint16_t *)p = htons(ICMP_HDR_LEN + payload_len);
        p += sizeof(uint16_t);
        *p++ = 0;
        *p++ = IPPROTO_ICMPV6;
        *p++ = type;
        *p++ = code;

        /* calculate checksum for last part of data and set ICMPv6 checksum */
        *csum_ptr = ~(ip_csum_part(csum, &ipv6_pseudo_header_part,
                                   sizeof(ipv6_pseudo_header_part)));
    }

    rc = rw_layer_cbs->write_cb(csap, pkt);
    tad_pkt_free(pkt);

    if (rc != 0)
    {
        ERROR("%s() write error: %r", __FUNCTION__, rc);
    }

    return rc;
}
