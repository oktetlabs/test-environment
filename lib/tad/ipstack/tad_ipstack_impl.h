/** @file
 * @brief TAD IP Stack
 *
 * Traffic Application Domain Command Handler.
 * IP Stack CSAPs implementaion internal declarations.
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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

#ifndef __TE_TAD_IPSTACK_IMPL_H__
#define __TE_TAD_IPSTACK_IMPL_H__ 

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>


#include "te_defs.h"
#include "te_errno.h"

#include "asn_usr.h" 
#include "ndn_ipstack.h"

#include "logger_api.h"

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"


#ifdef __cplusplus
extern "C" {
#endif


/* 
 * TCP CSAP specific data
 */
typedef struct tcp_csap_specific_data
{
    unsigned short   local_port;    /**< Local TCP port, in HOST order*/
    unsigned short   remote_port;   /**< Remote TCP port, in HOST order*/ 

    unsigned short   src_port;    /**< Source TCP port for current packet*/
    unsigned short   dst_port;    /**< Dest.  TCP port for current packet*/

    uint16_t         data_tag;

    tad_data_unit_t  du_src_port;
    tad_data_unit_t  du_dst_port;
    tad_data_unit_t  du_seqn;
    tad_data_unit_t  du_ackn;
    tad_data_unit_t  du_hlen;
    tad_data_unit_t  du_flags;
    tad_data_unit_t  du_win_size;
    tad_data_unit_t  du_checksum;
    tad_data_unit_t  du_urg_p;

} tcp_csap_specific_data_t;




/*
 * IP callbacks
 */ 

/**
 * Callback to init IPv4 layer as read/write layer of the CSAP.
 *
 * The function complies with csap_rw_init_cb_t prototype.
 */
extern te_errno tad_ip4_rw_init_cb(csap_p csap);


/**
 * Callback to destroy IPv4 layer as read/write layer of the CSAP.
 *
 * The function complies with csap_rw_destroy_cb_t prototype.
 */
extern te_errno tad_ip4_rw_destroy_cb(csap_p csap);

/**
 * Callback for read data from media of IPv4 CSAP. 
 *
 * The function complies with csap_read_cb_t prototype.
 */ 
extern te_errno tad_ip4_read_cb(csap_p csap, unsigned int timeout,
                                tad_pkt *pkt, size_t *pkt_len);

/**
 * Callback for write data to media of IPv4 CSAP. 
 *
 * The function complies with csap_write_cb_t prototype.
 */ 
extern te_errno tad_ip4_write_cb(csap_p csap, const tad_pkt *pkt);


/**
 * Callback for init 'ip4' CSAP layer.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */ 
extern te_errno tad_ip4_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback for destroy 'ip4' CSAP layer.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */ 
extern te_errno tad_ip4_destroy_cb(csap_p csap, unsigned int layer);


/**
 * Callback for confirm template PDU with IPv4 CSAP
 * parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_ip4_confirm_tmpl_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque); 

/**
 * Callback for confirm pattern PDU with IPv4 CSAP
 * arameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_ip4_confirm_ptrn_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque); 


/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */ 
extern te_errno tad_ip4_gen_bin_cb(csap_p                csap,
                                   unsigned int          layer,
                                   const asn_value      *tmpl_pdu,
                                   void                 *opaque,
                                   const tad_tmpl_arg_t *args,
                                   size_t                arg_num, 
                                   tad_pkts             *sdus,
                                   tad_pkts             *pdus);


extern te_errno tad_ip4_match_pre_cb(csap_p              csap,
                                     unsigned int        layer,
                                     tad_recv_pkt_layer *meta_pkt_layer);

extern te_errno tad_ip4_match_post_cb(csap_p              csap,
                                      unsigned int        layer,
                                      tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Callback for parse received packet and match it with pattern. 
 *
 * The function complies with csap_layer_match_do_cb_t prototype.
 */
extern te_errno tad_ip4_match_do_cb(csap_p           csap,
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
extern void tad_ip4_release_pdu_cb(csap_p csap, unsigned int layer,
                                   void *opaque);


/*
 * UDP callbacks
 */ 

/**
 * Callback for init 'udp' CSAP layer.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */ 
extern te_errno tad_udp_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback for destroy 'udp' CSAP layer.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */ 
extern te_errno tad_udp_destroy_cb(csap_p csap, unsigned int layer);


/**
 * Callback for confirm template PDU with UDP CSAP
 * parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_udp_confirm_tmpl_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque); 

/**
 * Callback for confirm pattern PDU with UDP CSAP
 * arameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_udp_confirm_ptrn_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value     *layer_pdu,
                                        void         **p_opaque); 


/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */ 
extern te_errno tad_udp_gen_bin_cb(csap_p                csap,
                                   unsigned int          layer,
                                   const asn_value      *tmpl_pdu,
                                   void                 *opaque,
                                   const tad_tmpl_arg_t *args,
                                   size_t                arg_num, 
                                   tad_pkts             *sdus,
                                   tad_pkts             *pdus);


extern te_errno tad_udp_match_pre_cb(csap_p              csap,
                                     unsigned int        layer,
                                     tad_recv_pkt_layer *meta_pkt_layer);

extern te_errno tad_udp_match_post_cb(csap_p              csap,
                                      unsigned int        layer,
                                      tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Callback for parse received packet and match it with pattern. 
 *
 * The function complies with csap_layer_match_do_cb_t prototype.
 */
extern te_errno tad_udp_match_do_cb(csap_p           csap,
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
extern void tad_udp_release_pdu_cb(csap_p csap, unsigned int layer,
                                   void *opaque);


/*
 * TCP callbacks
 */ 

/**
 * Callback for init 'tcp' CSAP layer.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */ 
extern te_errno tad_tcp_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback for destroy 'tcp' CSAP layer.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */ 
extern te_errno tad_tcp_destroy_cb(csap_p csap, unsigned int layer);

/**
 * Callback for read parameter value of TCP CSAP.
 *
 * The function complies with csap_layer_get_param_cb_t prototype.
 */ 
extern char *tad_tcp_get_param_cb(csap_p csap, unsigned int layer,
                                  const char *param);

/**
 * Callback for confirm PDU with ehternet CSAP parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_tcp_confirm_pdu_cb(csap_p         csap,
                                       unsigned int   layer,
                                       asn_value     *tmpl_pdu,
                                       void         **p_opaque); 

/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */
extern te_errno tad_tcp_gen_bin_cb(csap_p                csap,
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
extern te_errno tad_tcp_match_bin_cb(csap_p           csap,
                        unsigned int     layer,
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_IPSTACK_IMPL_H__ */
