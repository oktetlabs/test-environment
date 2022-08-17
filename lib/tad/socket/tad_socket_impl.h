/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD Socket
 *
 * Traffic Application Domain Command Handler.
 * Socket CSAP implementaion internal declarations.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAD_SOCKET_IMPL_H__
#define __TE_TAD_SOCKET_IMPL_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_errno.h"
#include "te_stdint.h"
#include "asn_usr.h"

#include "tad_pkt.h"
#include "tad_csap_inst.h"
#include "tad_csap_support.h"


#ifdef __cplusplus
extern "C" {
#endif


/** Socket read/write specific data */
typedef struct tad_socket_rw_data {

    int         socket;

    uint16_t    data_tag;
    size_t      wait_length;
    uint8_t    *stored_buffer;
    size_t      stored_length;

    struct in_addr   local_addr;
    struct in_addr   remote_addr;
    unsigned short   local_port;    /**< Local UDP port */
    unsigned short   remote_port;   /**< Remote UDP port */

} tad_socket_rw_data;


/**
 * Callback for init 'socket' CSAP layer if single in stack.
 *
 * The function complies with csap_rw_init_cb_t prototype.
 */
extern te_errno tad_socket_rw_init_cb(csap_p csap);

/**
 * Callback for destroy 'socket' CSAP layer if single in stack.
 *
 * The function complies with csap_rw_destroy_cb_t prototype.
 */
extern te_errno tad_socket_rw_destroy_cb(csap_p csap);


/**
 * Callback for read data from media of Socket CSAP.
 *
 * The function complies with csap_read_cb_t prototype.
 */
extern te_errno tad_socket_read_cb(csap_p csap, unsigned int timeout,
                                   tad_pkt *pkt, size_t *pkt_len);

/**
 * Callback for write data to media of Socket CSAP.
 *
 * The function complies with csap_write_cb_t prototype.
 */
extern te_errno tad_socket_write_cb(csap_p csap, const tad_pkt *pkt);


/**
 * Callback for confirm template PDU with Socket protocol CSAP
 * parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */
extern te_errno tad_socket_confirm_tmpl_cb(csap_p         csap,
                                           unsigned int   layer,
                                           asn_value     *layer_pdu,
                                           void         **p_opaque);

/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_socket_gen_bin_cb(csap_p                csap,
                                      unsigned int          layer,
                                      const asn_value      *tmpl_pdu,
                                      void                 *opaque,
                                      const tad_tmpl_arg_t *args,
                                      size_t                arg_num,
                                      tad_pkts             *sdus,
                                      tad_pkts             *pdus);


/**
 * Callback for parse received packet and match it with pattern.
 *
 * The function complies with csap_layer_match_bin_cb_t prototype.
 */
extern te_errno tad_socket_match_bin_cb(csap_p           csap,
                                        unsigned int     layer,
                                        const asn_value *ptrn_pdu,
                                        void            *ptrn_opaque,
                                        tad_recv_pkt    *meta_pkt,
                                        tad_pkt         *pdu,
                                        tad_pkt         *sdu);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_SOCKET_IMPL_H__ */
