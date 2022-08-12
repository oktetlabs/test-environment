/** @file
 * @brief ARP NDN
 *
 * Declarations of ASN.1 types for NDN for an Ethernet Address
 * Resolution protocol (RFC 826).
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 */

#ifndef __TE_NDN_ARP_H__
#define __TE_NDN_ARP_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"


#ifdef __cplusplus
extern "C" {
#endif


/* ARP request operation - ARPOP_REQUEST */
/* ARP reply operation - ARPOP_REPLY */


/** Buffer size for hardware address */
#define NDN_ARP_HW_SIZE_MAX     6
/** Buffer size for protocol address */
#define NDN_ARP_PROTO_SIZE_MAX  4

/** Structure that represents ARP header (see RFC 826) */
typedef struct ndn_arp_header_plain {
    uint16_t hw_type;       /**< Hardware type filed */
    uint16_t proto_type;    /**< Protocol type filed */
    uint8_t  hw_size;       /**< Size of hardware address */
    uint8_t  proto_size;    /**< Size of protocol address */
    uint16_t opcode;        /**< Operation type */

    /** Sender hardware address */
    uint8_t  snd_hw_addr[NDN_ARP_HW_SIZE_MAX];
    /** Sender protocol address */
    uint8_t  snd_proto_addr[NDN_ARP_PROTO_SIZE_MAX];
    /** Target hardware address */
    uint8_t  tgt_hw_addr[NDN_ARP_HW_SIZE_MAX];
    /** Target protocol address */
    uint8_t  tgt_proto_addr[NDN_ARP_PROTO_SIZE_MAX];

} ndn_arp_header_plain;


typedef enum {
    NDN_TAG_ARP_HW_TYPE,
    NDN_TAG_ARP_PROTO,
    NDN_TAG_ARP_HW_SIZE,
    NDN_TAG_ARP_PROTO_SIZE,
    NDN_TAG_ARP_OPCODE,
    NDN_TAG_ARP_SND_HW_ADDR,
    NDN_TAG_ARP_SND_PROTO_ADDR,
    NDN_TAG_ARP_TGT_HW_ADDR,
    NDN_TAG_ARP_TGT_PROTO_ADDR,
} ndn_arp_tags_t;


extern const asn_type * const ndn_arp_header;
extern const asn_type * const ndn_arp_csap;


/**
 * Convert ARP-Header ASN value to plain C structure.
 *
 * @param pkt           ASN value of type ARP-Header or
 *                      Generic-PDU with choice "arp"
 * @param arp_header    Location for converted structure (OUT)
 *
 * @return Zero on success or error code.
 */
extern te_errno ndn_arp_packet_to_plain(const asn_value *pkt,
                                        ndn_arp_header_plain *arp_header);


/**
 * Convert plain C structure to ARP-Header ASN value.
 *
 * @param arp_header    Structure to be converted
 *
 * @return Pointer to new ASN value object or NULL.
 */
extern asn_value *ndn_arp_plain_to_packet(
                      const ndn_arp_header_plain *arp_header);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_ARP_H__ */
