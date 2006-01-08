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

#ifndef ETH_CSAP_DEFAULT_TIMEOUT  /* Seconds to wait for incoming data */ 
#define ETH_CSAP_DEFAULT_TIMEOUT 5 
#endif

/* special eth type/length value for tagged frames. */
#define ETH_TAGGED_TYPE_LEN 0x8100


/* Default recv mode: all except OUTGOING packets. */
#define ETH_RECV_MODE_DEF (ETH_RECV_HOST      | ETH_RECV_BROADCAST | \
                           ETH_RECV_MULTICAST | ETH_RECV_OTHERHOST )

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Callback for init 'eth' CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_init_cb_t prototype.
 */ 
extern te_errno tad_eth_rw_init_cb(csap_p           csap,
                                   const asn_value *csap_nds);

/**
 * Callback for destroy 'eth' CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_destroy_cb_t prototype.
 */ 
extern te_errno tad_eth_rw_destroy_cb(csap_p csap);


/**
 * Callback for read data from media of Ethernet CSAP. 
 *
 * The function complies with csap_read_cb_t prototype.
 */ 
extern int tad_eth_read_cb(csap_p csap, int timeout,
                           char *buf, size_t buf_len);

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
 * Callback for write data to media of Ethernet CSAP and read
 * data from media just after write, to get answer to sent request. 
 *
 * The function complies with csap_write_read_cb_t prototype.
 */ 
extern int tad_eth_write_read_cb(csap_p csap, int timeout,
                                 const tad_pkt *w_pkt,
                                 char *r_buf, size_t r_buf_len);


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


/**
 * Callback for parse received packet and match it with pattern. 
 *
 * The function complies with csap_layer_match_bin_cb_t prototype.
 */
extern te_errno tad_eth_match_bin_cb(csap_p           csap,
                                     unsigned int     layer,
                                     const asn_value *pattern_pdu,
                                     const csap_pkts *pkt,
                                     csap_pkts       *payload,
                                     asn_value_p      parsed_packet);

/**
 * Create and bind raw socket to listen specified interface
 *
 * @param eth_type  Etherner protocol type 
 * @param pkt_type  Type of packet socket (PACKET_HOST, PACKET_OTHERHOST,
 *                  PACKET_OUTGOING
 * @param if_index  interface index
 * @param sock      pointer to place where socket handler will be saved
 *
 * @param 0 on succees, -1 on fail
 */
extern int open_raw_socket(int eth_type, int pkt_type, int if_index,
                           int *sock);



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
