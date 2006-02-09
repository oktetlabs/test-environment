/** @file
 * @brief TAD Socket
 *
 * Traffic Application Domain Command Handler.
 * Socket CSAP implementaion internal declarations.
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
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
 * The function complies with csap_nbr_init_cb_t prototype.
 */ 
extern te_errno tad_socket_rw_init_cb(csap_p           csap,
                                      const asn_value *csap_nds);

/**
 * Callback for destroy 'socket' CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_destroy_cb_t prototype.
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
                                           asn_value_p    layer_pdu,
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
 * Callback for confirm pattern PDU with Socket protocol CSAP
 * arameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_socket_confirm_ptrn_cb(csap_p         csap,
                                           unsigned int   layer,
                                           asn_value_p    layer_pdu,
                                           void         **p_opaque); 

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
