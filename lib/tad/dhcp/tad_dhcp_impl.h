/** @file
 * @brief TAD DHCP
 *
 * Traffic Application Domain Command Handler.
 * DHCP CSAP implementaion internal declarations.
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_DHCP_IMPL_H__
#define __TE_TAD_DHCP_IMPL_H__ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#include "te_defs.h"
#include "te_errno.h"

#include "asn_usr.h" 
#include "ndn.h"
#include "ndn_dhcp.h"

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"


#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DHCP CSAP specific data
 */
typedef struct dhcp_csap_specific_data {
    int   in;           /**< Socket for receiving data to the media  */
    int   out;          /**< Socket for sending data to the media  */
    int   mode;
    char *ipaddr;       /**< Address of asked interface, socket binds to it */

} dhcp_csap_specific_data_t;


/**
 * Callback for init 'file' CSAP layer as read/write layer.
 *
 * The function complies with csap_rw_init_cb_t prototype.
 */ 
extern te_errno tad_dhcp_rw_init_cb(csap_p csap);

/**
 * Callback for destroy 'file' CSAP layer as read/write layer.
 *
 * The function complies with csap_rw_destroy_cb_t prototype.
 */ 
extern te_errno tad_dhcp_rw_destroy_cb(csap_p csap);

/**
 * Callback for read data from media of DHCP CSAP. 
 *
 * The function complies with csap_read_cb_t prototype.
 */ 
extern te_errno tad_dhcp_read_cb(csap_p csap, unsigned int timeout,
                                 tad_pkt *pkt, size_t *pkt_len);

/**
 * Callback for write data to media of DHCP CSAP. 
 *
 * The function complies with csap_write_cb_t prototype.
 */ 
extern te_errno tad_dhcp_write_cb(csap_p csap, const tad_pkt *pkt);



/**
 * Callback for read parameter value of DHCP CSAP.
 *
 * The function complies with csap_layer_get_param_cb_t prototype.
 */ 
extern char *tad_dhcp_get_param_cb(csap_p        csap,
                                   unsigned int  layer,
                                   const char    *param);


/**
 * Callback for init 'dhcp' CSAP layer.
 *
 * The function complies with csap_layer_init_cb_t prototype.
 */ 
extern te_errno tad_dhcp_init_cb(csap_p csap, unsigned int layer);

/**
 * Callback for destroy 'dhcp' CSAP layer.
 *
 * The function complies with csap_layer_destroy_cb_t prototype.
 */ 
extern te_errno tad_dhcp_destroy_cb(csap_p csap, unsigned int layer);


/**
 * Callback for confirm template PDU with IPv4 CSAP
 * parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_dhcp_confirm_tmpl_cb(csap_p         csap,
                                         unsigned int   layer,
                                         asn_value     *layer_pdu,
                                         void         **p_opaque); 

/**
 * Callback for confirm pattern PDU with IPv4 CSAP
 * arameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_dhcp_confirm_ptrn_cb(csap_p         csap,
                                         unsigned int   layer,
                                         asn_value     *layer_pdu,
                                         void         **p_opaque); 


/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */ 
extern te_errno tad_dhcp_gen_bin_cb(csap_p                csap,
                                    unsigned int          layer,
                                    const asn_value      *tmpl_pdu,
                                    void                 *opaque,
                                    const tad_tmpl_arg_t *args,
                                    size_t                arg_num, 
                                    tad_pkts             *sdus,
                                    tad_pkts             *pdus);


extern te_errno tad_dhcp_match_pre_cb(csap_p              csap,
                                      unsigned int        layer,
                                      tad_recv_pkt_layer *meta_pkt_layer);

extern te_errno tad_dhcp_match_post_cb(csap_p              csap,
                                       unsigned int        layer,
                                       tad_recv_pkt_layer *meta_pkt_layer);

/**
 * Callback for parse received packet and match it with pattern. 
 *
 * The function complies with csap_layer_match_do_cb_t prototype.
 */
extern te_errno tad_dhcp_match_do_cb(csap_p           csap,
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
extern void tad_dhcp_release_pdu_cb(csap_p csap, unsigned int layer,
                                    void *opaque);


/**
 * Callback for generating pattern to filter 
 * just one response to the packet which will be sent by this CSAP 
 * according to this template. 
 *
 * The function complies with csap_layer_gen_pattern_cb_t prototype.
 */
extern te_errno tad_dhcp_gen_pattern_cb(csap_p            csap,
                                        unsigned int      layer,
                                        const asn_value  *tmpl_pdu, 
                                        asn_value       **ptrn_pdu);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAD_DHCP_IMPL_H__ */
