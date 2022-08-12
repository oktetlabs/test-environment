/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Declarations of ASN.1 types for NDN for Ethernet-PCAP CSAP protocol.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 */
#ifndef __TE_NDN_PCAP_H__
#define __TE_NDN_PCAP_H__

#ifdef HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#endif

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif

#include "te_stdint.h"
#include "asn_usr.h"
#include "ndn.h"

#ifdef __cplusplus
extern "C" {
#endif

enum pcap_csap_receive_mode {
    PCAP_RECV_HOST      = 0x01,
    PCAP_RECV_BROADCAST = 0x02,
    PCAP_RECV_MULTICAST = 0x04,
    PCAP_RECV_OTHERHOST = 0x08,
    PCAP_RECV_OUTGOING  = 0x10,
    PCAP_RECV_ALL       = 0x1F,
};

extern const asn_type * const ndn_pcap_filter;
extern const asn_type * const ndn_pcap_csap;

extern asn_type ndn_pcap_filter_s;
extern asn_type ndn_pcap_csap_s;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_NDN_PCAP_H__ */
