/** @file
 * @brief TAD PCAP
 *
 * Traffic Application Domain Command Handler
 * Ethernet-PCAP CSAP implementaion internal declarations.
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_PCAP_IMPL_H__
#define __TE_TAD_PCAP_IMPL_H__ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <netinet/if_ether.h>

#include <pcap.h>
#include <pcap-bpf.h>

#include "te_errno.h"

#include "asn_usr.h" 
#include "ndn_pcap.h"

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"
#include "pkt_socket.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Callback for init Ethernet-PCAP CSAP layer if single in stack.
 *
 * The function complies with csap_rw_init_cb_t prototype.
 */ 
extern te_errno tad_pcap_rw_init_cb(csap_p csap, const asn_value *csap_nds);

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
                                         asn_value_p    layer_pdu,
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
