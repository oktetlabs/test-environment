/** @file
 * @brief TAD RTE mbuf
 *
 * Traffic Application Domain Command Handler
 * RTE mbuf CSAP implementation internal declarations
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAD_RTE_MBUF_IMPL_H__
#define __TE_TAD_RTE_MBUF_IMPL_H__


#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "te_stdint.h"
#include "te_errno.h"

#include "asn_usr.h"
#include "ndn_rte_mbuf.h"

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_pkt.h"
#include "tad_utils.h"
#include "tad_rte_mbuf_sap.h"

#ifdef __cplusplus
extern "C" {
#endif

/** RTE mbuf layer read / write specific data */
typedef struct tad_rte_mbuf_rw_data {
    tad_rte_mbuf_sap    sap;            /**< RTE mbuf service access point */
} tad_rte_mbuf_rw_data;

/**
 * Callback to copy binary data prepared by the upper layers to the final packet;
 * the function complies with @b csap_layer_generate_pkts_cb_t prototype
 */
extern te_errno tad_rte_mbuf_gen_bin_cb(csap_p                  csap,
                                        unsigned int            layer,
                                        const asn_value        *tmpl_pdu,
                                        void                   *opaque,
                                        const tad_tmpl_arg_t   *args,
                                        size_t                  arg_num,
                                        tad_pkts               *sdus,
                                        tad_pkts               *pdus);

/**
 * Callback to initialize RTE mbuf NDS for the corresponding layer
 * in the meta-packet known to match the CSAP pattern;
 * the function complies with @b csap_layer_match_post_cb_t prototype
 */
extern te_errno tad_rte_mbuf_match_post_cb(csap_p              csap,
                                           unsigned int        layer,
                                           tad_recv_pkt_layer *meta_pkt_layer);

/** Stub callback which complies with @b csap_layer_match_do_cb_t prototype */
extern te_errno tad_rte_mbuf_match_do_cb(csap_p              csap,
                                         unsigned int        layer,
                                         const asn_value    *ptrn_pdu,
                                         void               *ptrn_opaque,
                                         tad_recv_pkt       *meta_pkt,
                                         tad_pkt            *pdu,
                                         tad_pkt            *sdu);

/**
 * Callback for 'rte_mbuf' CSAP layer initialization (single in stack);
 * the function complies with @b csap_rw_init_cb_t prototype
 */
extern te_errno tad_rte_mbuf_rw_init_cb(csap_p  csap);

/**
 * Callback to destroy 'rte_mbuf' CSAP layer (single in stack);
 * the function complies with @b csap_rw_destroy_cb_t prototype
 */
extern te_errno tad_rte_mbuf_rw_destroy_cb(csap_p  csap);

/**
 * Callback (aka "read data") for converting RTE mbuf to TAD packet;
 * the function complies with @b csap_read_cb_t prototype
 */
extern te_errno tad_rte_mbuf_read_cb(csap_p          csap,
                                     unsigned int    timeout,
                                     tad_pkt        *pkt,
                                     size_t         *pkt_len);

/**
 * Callback (aka "write data") for converting TAD packet to RTE mbuf;
 * the function complies with @b csap_write_cb_t prototype
 */
extern te_errno tad_rte_mbuf_write_cb(csap_p          csap,
                                      const tad_pkt  *pkt);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_RTE_MBUF_IMPL_H__ */
