/** @file
 * @brief TE Portability
 *
 * Definitions for Ethernet.
 * 
 * 
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
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
#include <sys/queue.h>
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
