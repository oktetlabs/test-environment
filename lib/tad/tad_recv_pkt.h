/** @file
 * @brief TAD Receiver
 *
 * Traffic Application Domain Command Handler.
 * Declarations of types and functions for TAD Receiver packet
 * representation.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_RECV_PKT_H__
#define __TE_TAD_RECV_PKT_H__ 

#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#else
#error TAD cannot be compiled without sys/queue.h
#endif

#include "asn_usr.h" 
#include "tad_pkt.h"
#include "tad_types.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Received packet layer data.
 */
typedef struct tad_recv_pkt_layer {
    asn_value  *nds;    /**< ASN.1 representation of the layer */
    tad_pkts    pkts;   /**< Packets */
    void       *opaque; /**< Opaque data to help matching */
} tad_recv_pkt_layer;

/**
 * Element in queue of received packets. 
 */
typedef struct tad_recv_pkt {
    TAILQ_ENTRY(tad_recv_pkt)   links;  /**< List links */

    asn_value          *nds;        /**< ASN.1 representation */

    unsigned int        n_layers;   /**< Number of layers */
    tad_recv_pkt_layer *layers;     /**< Per-layer data */

    tad_pkt             payload;    /**< Payload of the packet */

    tad_pkts            raw;        /**< Raw packets */

    struct timeval      ts;         /**< Timestamp of the whole packet
                                         (timestamp of the last fragment
                                         in the case of reassembly) */
} tad_recv_pkt;


/** Queue of received packets */
typedef TAILQ_HEAD(, tad_recv_pkt)  tad_recv_pkts;


extern void tad_recv_pkt_free(csap_p csap, tad_recv_pkt *pkt);
extern void tad_recv_pkts_free(csap_p csap, tad_recv_pkts *pkts);
extern tad_recv_pkt *tad_recv_pkt_alloc(csap_p csap);
extern void tad_recv_pkt_cleanup(csap_p csap, tad_recv_pkt *pkt);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_RECV_PKT_H__ */
