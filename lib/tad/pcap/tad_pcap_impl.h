/** @file
 * @brief TAD PCAP
 *
 * Traffic Application Domain Command Handler
 * Ethernet-PCAP CSAP implementaion internal declarations.
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_TAD_PCAP_IMPL_H__
#define __TE_TAD_PCAP_IMPL_H__

#include "tad_csap_support.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Callback for init Ethernet-PCAP CSAP layer if single in stack.
 *
 * The function complies with csap_rw_init_cb_t prototype.
 */
extern te_errno tad_pcap_rw_init_cb(csap_p csap);

/**
 * Callback for destroy Ethernet-PCAP CSAP layer if single in stack.
 *
 * The function complies with csap_rw_destroy_cb_t prototype.
 */
extern te_errno tad_pcap_rw_destroy_cb(csap_p csap);

/**
 * Open receive socket for Ethernet-PCAP CSAP.
 *
 * This function complies with csap_low_resource_cb_t prototype.
 */
extern te_errno tad_pcap_prepare_recv(csap_p csap);

/**
 * Close receive socket for Ethernet-PCAP CSAP.
 *
 * This function complies with csap_low_resource_cb_t prototype.
 */
extern te_errno tad_pcap_shutdown_recv(csap_p csap);

/**
 * Callback for read data from media of Ethernet-PCAP CSAP.
 *
 * The function complies with csap_read_cb_t prototype.
 */
extern te_errno tad_pcap_read_cb(csap_p csap, unsigned int timeout,
                                 tad_pkt *pkt, size_t *pkt_len);


/**
 * Callback for init Ethernet-PCAP CSAP layer if single in stack.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */
extern te_errno tad_pcap_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback for destroy Ethernet-PCAP CSAP layer if single in stack.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */
extern te_errno tad_pcap_destroy_cb(csap_p csap, unsigned int layer);

/**
 * Callback for confirm pattern PDU with Ethernet-PCAP CSAP parameters
 * and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_pcap_confirm_ptrn_cb(csap_p         csap,
                                         unsigned int   layer,
                                         asn_value     *layer_pdu,
                                         void         **p_opaque);

/**
 * Callback for parse received packet and match it with pattern.
 *
 * The function complies with csap_layer_match_bin_cb_t prototype.
 */
extern te_errno tad_pcap_match_bin_cb(csap_p           csap,
                        unsigned int     layer,
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu);

/**
 * Callback to release data prepared by confirm callback.
 *
 * The function complies with csap_layer_release_opaque_cb_t prototype.
 */
extern void tad_pcap_release_ptrn_cb(csap_p csap, unsigned int layer,
                                     void *opaque);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_PCAP_IMPL_H__ */
