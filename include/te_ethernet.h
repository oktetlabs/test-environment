/** @file
 * @brief TE Portability
 *
 * Definitions for Ethernet.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#ifndef __TE_ETHERNET_H__
#define __TE_ETHERNET_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#if HAVE_NET_IF_ETHER_H
#if defined(__NetBSD__) /* FIXME */
#include "te_queue.h"
#include <net/if.h>
#endif
#include <net/if_ether.h>
#endif
#if HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#endif

#if HAVE_NETINET_IF_ETHER_H
/* Required on OpenBSD */
#if HAVE_NET_IF_ARP_H
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <net/if_arp.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#include <netinet/if_ether.h>
#endif

#if HAVE_SYS_ETHERNET_H
#include <sys/ethernet.h>
#endif

#ifdef __CYGWIN__
#define ETHER_ADDR_LEN 6
#define ETHERTYPE_IP   0x0800
#define ETHERTYPE_ARP  0x0806
#define ETHER_MIN_LEN  64
#define ETHER_CRC_LEN  4

#define ICMP_ECHOREPLY      0  /* Echo Reply */
#define ICMP_DEST_UNREACH   3  /* Destination Unreachable */
#define ICMP_SOURCE_QUENCH  4  /* Source Quench */
#define ICMP_REDIRECT       5  /* Redirect (change route) */
#define ICMP_ECHO           8  /* Echo Request */
#define ICMP_TIME_EXCEEDED  11 /* Time Exceeded */
#define ICMP_PARAMETERPROB  12 /* Parameter Problem */
#define ICMP_TIMESTAMP      13 /* Timestamp Request */
#define ICMP_TIMESTAMPREPLY 14 /* Timestamp Reply */
#define ICMP_INFO_REQUEST   15 /* Information Request */
#define ICMP_INFO_REPLY     16 /* Information Reply */
#define ICMP_ADDRESS        17 /* Address Mask Request */
#define ICMP_ADDRESSREPLY   18 /* Address Mask Reply */
#endif

#ifndef ETHER_ADDR_LEN
#ifdef ETHERADDRL
#define ETHER_ADDR_LEN  ETHERADDRL
#endif
#endif

#ifndef ETHER_TYPE_LEN
#define ETHER_TYPE_LEN  (2) /* FIXME */
#endif

#ifndef ETHER_HDR_LEN
#define ETHER_HDR_LEN   (2 * ETHER_ADDR_LEN + ETHER_TYPE_LEN) /* FIXME */
#endif

#ifndef ETHER_MIN_LEN
#if defined(ETHERMIN) && defined(ETHERFCSL)
#define ETHER_MIN_LEN   (ETHERMIN + ETHERFCSL)
#endif
#endif

#ifndef ETHER_CRC_LEN
#ifdef ETHERFCSL
#define ETHER_CRC_LEN   ETHERFCSL
#endif
#endif

#ifndef ETHER_DATA_LEN
#define ETHER_DATA_LEN 1500
#endif

#endif /* !__TE_ETHERNET_H__ */
