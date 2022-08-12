/** @file
 * @brief TAD Receiver
 *
 * Traffic Application Domain Command Handler.
 * Declarations of types and functions for TAD Receiver packet
 * representation.
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#ifndef __TE_TAD_RECV_PKT_H__
#define __TE_TAD_RECV_PKT_H__

#include "te_queue.h"
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

    int                 match_unit;    /**< Index of matched pattern unit,
                                            -1 if packet mismatched */
} tad_recv_pkt;


/** Queue of received packets */
typedef TAILQ_HEAD(, tad_recv_pkt)  tad_recv_pkts;


extern void tad_recv_pkt_free(csap_p csap, tad_recv_pkt *pkt);
extern void tad_recv_pkts_free(csap_p csap, tad_recv_pkts *pkts);
extern tad_recv_pkt *tad_recv_pkt_alloc(csap_p csap);
extern void tad_recv_pkt_cleanup_upper(csap_p csap, tad_recv_pkt *pkt);
extern void tad_recv_pkt_cleanup(csap_p csap, tad_recv_pkt *pkt);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_RECV_PKT_H__ */
