/** @file
 * @brief TAD Ethernet
 *
 * Traffic Application Domain Command Handler.
 * Ethernet CSAP implementaion internal declarations.
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
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

#ifndef __TE_TAD_ETH_IMPL_H__
#define __TE_TAD_ETH_IMPL_H__ 

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
#include <unistd.h>
#include <fcntl.h>

#include "te_errno.h"

#include "asn_usr.h" 
#include "ndn_eth.h"

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"
#include "pkt_socket.h"


#ifndef ETH_TAG_EXC_LEN  /* Extra tagging octets in eth. header */
#define ETH_TAG_EXC_LEN    2       
#endif

#ifndef DEFAULT_ETH_TYPE
#define DEFAULT_ETH_TYPE 0
#endif

#ifndef IFNAME_SIZE
#define IFNAME_SIZE 256
#endif


#ifndef ETH_COMPLETE_FREE
#define ETH_COMPLETE_FREE 1
#endif

/* special eth type/length value for tagged frames. */
#define ETH_TAGGED_TYPE_LEN 0x8100


/* Default recv mode: all except OUTGOING packets. */
#define ETH_RECV_MODE_DEF (ETH_RECV_HOST      | ETH_RECV_BROADCAST | \
                           ETH_RECV_MULTICAST | ETH_RECV_OTHERHOST )

#ifdef __cplusplus
extern "C" {
#endif


/** Ethernet layer read/write specific data */
typedef struct tad_eth_rw_data {
    eth_interface_p interface;  /**< Ethernet interface data */

    int     out;            /**< Socket for sending data to the media */
    int     in;             /**< Socket for receiving data */

    uint8_t recv_mode;      /**< Receive mode, bit mask from values in 
                                 enum eth_csap_receive_mode in ndn_eth.h */
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
                                        asn_value_p    layer_pdu,
                                        void         **p_opaque); 

/**
 * Callback to release data prepared by confirm callback.
 *
 * The function complies with csap_layer_release_opaque_cb_t prototype.
 */
extern void tad_eth_release_tmpl_cb(csap_p csap, unsigned int layer,
                                    void *opaque);


/**
 * Callback for confirm pattern PDU with Ethernet protocol CSAP
 * arameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_eth_confirm_ptrn_cb(csap_p         csap,
                                        unsigned int   layer,
                                        asn_value_p    layer_pdu,
                                        void         **p_opaque); 

/**
 * Callback to release data prepared by confirm callback.
 *
 * The function complies with csap_layer_release_opaque_cb_t prototype.
 */
extern void tad_eth_release_ptrn_cb(csap_p csap, unsigned int layer,
                                    void *opaque);


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

extern void tad_eth_match_free_cb(csap_p csap, unsigned int layer,
                                  void *opaque);

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
