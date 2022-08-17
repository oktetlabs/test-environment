/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD Ethernet
 *
 * Traffic Application Domain Command Handler.
 * Ethernet CSAP implementaion internal declarations.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAD_ETH_IMPL_H__
#define __TE_TAD_ETH_IMPL_H__


#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"

#include "asn_usr.h"
#include "ndn_eth.h"

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_pkt.h"
#include "tad_utils.h"
#include "tad_eth_sap.h"


#ifndef ETH_TAG_EXC_LEN  /* Extra tagging octets in eth. header */
#define ETH_TAG_EXC_LEN    2
#endif

#ifndef DEFAULT_ETH_TYPE
#define DEFAULT_ETH_TYPE 0
#endif

#ifndef ETH_COMPLETE_FREE
#define ETH_COMPLETE_FREE 1
#endif

/* special eth type/length value for tagged frames. */
#define ETH_TAGGED_TYPE_LEN 0x8100


#ifdef __cplusplus
extern "C" {
#endif


/** Ethernet layer read/write specific data */
typedef struct tad_eth_rw_data {
    tad_eth_sap     sap;        /**< Ethernet service access point */
    unsigned int    recv_mode;  /**< Default receive mode */
} tad_eth_rw_data;


/**
 * Callback for init 'eth' CSAP layer if single in stack.
 *
 * The function complies with csap_rw_init_cb_t prototype.
 */
extern te_errno tad_eth_rw_init_cb(csap_p csap);

/**
 * Callback for destroy 'eth' CSAP layer if single in stack.
 *
 * The function complies with csap_rw_destroy_cb_t prototype.
 */
extern te_errno tad_eth_rw_destroy_cb(csap_p csap);


/**
 * Callback for read data from media of Ethernet CSAP.
 *
 * The function complies with csap_read_cb_t prototype.
 */
extern te_errno tad_eth_read_cb(csap_p csap, unsigned int timeout,
                                tad_pkt *pkt, size_t *pkt_len);

/**
 * Open transmit socket for Ethernet CSAP.
 *
 * The function complies with csap_low_resource_cb_t prototype.
 */
extern te_errno tad_eth_prepare_send(csap_p csap);

/**
 * Close transmit socket for Ethernet CSAP.
 *
 * The function complies with csap_low_resource_cb_t prototype.
 */
extern te_errno tad_eth_shutdown_send(csap_p csap);

/**
 * Callback for write data to media of Ethernet CSAP.
 *
 * The function complies with csap_write_cb_t prototype.
 */
extern te_errno tad_eth_write_cb(csap_p csap, const tad_pkt *pkt);

/**
 * Open receive socket for Ethernet CSAP.
 *
 * The function complies with csap_low_resource_cb_t prototype.
 */
extern te_errno tad_eth_prepare_recv(csap_p csap);

/**
 * Close receive socket for Ethernet CSAP.
 *
 * The function complies with csap_low_resource_cb_t prototype.
 */
extern te_errno tad_eth_shutdown_recv(csap_p csap);


/**
 * Callback to initialize 'eth' CSAP layer.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */
extern te_errno tad_eth_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback for destroy 'eth' CSAP layer.
 *
 * The function complies with csap_destroy_cb_t prototype.
 */
extern te_errno tad_eth_destroy_cb(csap_p csap, unsigned int layer);

/**
 * Callback for read parameter value of Ethernet CSAP.
 *
 * The function complies with csap_layer_get_param_cb_t prototype.
 */
extern char *tad_eth_get_param_cb(csap_p        csap,
                                  unsigned int  layer,
                                  const char   *param);


/**
 * Callback for confirm template PDU with Ethernet protocol CSAP
 * parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_eth_confirm_tmpl_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque);

/**
 * Callback for confirm pattern PDU with Ethernet protocol CSAP
 * arameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_eth_confirm_ptrn_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque);


/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_eth_gen_bin_cb(csap_p                csap,
                                   unsigned int          layer,
                                   const asn_value      *tmpl_pdu,
                                   void                 *opaque,
                                   const tad_tmpl_arg_t *args,
                                   size_t                arg_num,
                                   tad_pkts             *sdus,
                                   tad_pkts             *pdus);


extern te_errno tad_eth_match_pre_cb(csap_p              csap,
                                     unsigned int        layer,
                                     tad_recv_pkt_layer *meta_pkt_layer);

extern te_errno tad_eth_match_post_cb(csap_p              csap,
                                      unsigned int        layer,
                                      tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Callback for parse received packet and match it with pattern.
 *
 * The function complies with csap_layer_match_do_cb_t prototype.
 */
extern te_errno tad_eth_match_do_cb(csap_p           csap,
                        unsigned int     layer,
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu);


/**
 * Callback to release data prepared by confirm callback or packet match.
 *
 * The function complies with csap_layer_release_opaque_cb_t prototype.
 */
extern void tad_eth_release_pdu_cb(csap_p csap, unsigned int layer,
                                   void *opaque);


/**
 * Ethernet echo CSAP method.
 * Method should prepare binary data to be send as "echo" and call
 * respective write method to send it.
 * Method may change data stored at passed location.
 *
 * @param csap    identifier of CSAP
 * @param pkt           Got packet, plain binary data.
 * @param len           length of data.
 *
 * @return zero on success or error code.
 */
extern int eth_echo_method(csap_p csap, uint8_t *pkt, size_t len);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_ETH_IMPL_H__ */
