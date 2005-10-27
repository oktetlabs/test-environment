/** @file
 * @brief Ethernet TAD
 *
 * Traffic Application Domain Command Handler
 * Ethernet CSAP implementaion internal declarations.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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

#include "te_config.h"

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


#ifndef ETH_ALEN          /* Octets in one ethernet addr     */
#define ETH_ALEN    6 
#endif

#ifndef ETH_HLEN         /* Total octets in header.         */
#define ETH_HLEN    14 
#endif

#ifndef ETH_ZLEN         /* Min. octets in frame sans FCS   */
#define ETH_ZLEN    60 
#endif

#ifndef ETH_DATA_LEN    /* Max. octets in payload          */
#define ETH_DATA_LEN    1500
#endif

#ifndef ETH_FRAME_LEN   /* Max. octets in frame sans FCS   */
#define ETH_FRAME_LEN    1514
#endif


#ifndef ETH_TYPE_LEN      /* Octets in ethernet type         */
#define ETH_TYPE_LEN    2 
#endif

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

/* Linux does not process ethernet checksum */
#define ETH_TAILING_CHECKSUM 4

/* special eth type/length value for tagged frames. */
#define ETH_TAGGED_TYPE_LEN 0x8100


/* Default recv mode: all except OUTGOING packets. */
#define ETH_RECV_MODE_DEF (ETH_RECV_HOST      | ETH_RECV_BROADCAST | \
                           ETH_RECV_MULTICAST | ETH_RECV_OTHERHOST )

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * Ethernet CSAP specific data
 */
struct eth_csap_specific_data;
typedef struct eth_csap_specific_data *eth_csap_specific_data_p;
typedef struct eth_csap_specific_data
{
    eth_interface_p interface; /**< Ethernet interface data        */

    int   out;          /**< Socket for sending data to the media       */
    int   in;           /**< Socket for receiving data                  */
   
    uint16_t eth_type;  /**< Ethernet protocol type                     */

    uint8_t recv_mode;  /**< Receive mode, bit mask from values in 
                             enum eth_csap_receive_mode in ndn_eth.h    */
    
    char *remote_addr;  /**< Default remote address (NULL if undefined) */
    char *local_addr;   /**< Default local address (NULL if undefined)  */

    int   read_timeout; /**< Number of second to wait for data          */
    int   cfi;
    int   vlan_id;
    int   priority;
    size_t total_bytes; /**< Total number of sent or received bytes     */ 


    /* integer scripts for iteration: */ 
    tad_data_unit_t   du_dst_addr;
    tad_data_unit_t   du_src_addr;
    tad_data_unit_t   du_eth_type;
    tad_data_unit_t   du_cfi;
    tad_data_unit_t   du_vlan_id;
    tad_data_unit_t   du_priority;

} eth_csap_specific_data_t;


/**
 * Callback for read parameter value of ethernet CSAP.
 *
 * The function complies with csap_get_param_cb_t prototype.
 */ 
extern char *tad_eth_get_param_cb(csap_p        csap_descr,
                                  unsigned int  layer,
                                  const char   *param);

/**
 * Callback for read data from media of ethernet CSAP. 
 *
 * The function complies with csap_read_cb_t prototype.
 */ 
extern int tad_eth_read_cb(csap_p csap_id, int timeout,
                           char *buf, size_t buf_len);

/**
 * Callback for write data to media of ethernet CSAP. 
 *
 * The function complies with csap_write_cb_t prototype.
 */ 
extern int tad_eth_write_cb(csap_p csap_id,
                            const char *buf, size_t buf_len);

/**
 * Callback for write data to media of ethernet CSAP and read
 * data from media just after write, to get answer to sent request. 
 *
 * The function complies with csap_write_read_cb_t prototype.
 */ 
extern int tad_eth_write_read_cb(csap_p csap_id, int timeout,
                                 const char *w_buf, size_t w_buf_len,
                                 char *r_buf, size_t r_buf_len);


/**
 * Callback for init 'file' CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_init_cb_t prototype.
 */ 
extern te_errno tad_eth_single_init_cb(int              csap_id,
                                       const asn_value *csap_nds,
                                       unsigned int     layer);

/**
 * Callback for destroy 'file' CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_destroy_cb_t prototype.
 */ 
extern te_errno tad_eth_single_destroy_cb(int csap_id, unsigned int layer);

/**
 * Callback for confirm PDU with ehternet CSAP parameters and possibilities.
 *
 * The function complies with csap_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_eth_confirm_pdu_cb(int csap_id, unsigned int layer,
                                       asn_value_p tmpl_pdu); 

/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_gen_bin_cb_t prototype.
 */ 
extern te_errno tad_eth_gen_bin_cb(csap_p                csap_descr,
                                   unsigned int          layer,
                                   const asn_value      *tmpl_pdu,
                                   const tad_tmpl_arg_t *args,
                                   size_t                arg_num, 
                                   csap_pkts_p           up_payload,
                                   csap_pkts_p           pkts);


/**
 * Callback for parse received packet and match it with pattern. 
 *
 * The function complies with csap_match_bin_cb_t prototype.
 */
extern te_errno tad_eth_match_bin_cb(int              csap_id,
                                     unsigned int     layer,
                                     const asn_value *pattern_pdu,
                                     const csap_pkts *pkt,
                                     csap_pkts       *payload,
                                     asn_value_p      parsed_packet);

/**
 * Callback for generating pattern to filter 
 * just one response to the packet which will be sent by this CSAP 
 * according to this template. 
 *
 * The function complies with csap_gen_pattern_cb_t prototype.
 */
extern te_errno tad_eth_gen_pattern_cb(int csap_id, unsigned int layer,
                                       const asn_value *tmpl_pdu, 
                                       asn_value_p *pattern_pdu);

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
 * Free all memory allocated by eth csap specific data
 *
 * @param eth_csap_specific_data_p  poiner to structure
 * @param is_complete               if not 0 the final free() will be 
 *                                  called on passed pointer
 */ 
extern void free_eth_csap_data(eth_csap_specific_data_p spec_data,
                               char is_colmplete);


/**
 * Open receive socket for ethernet CSAP 
 *
 * @param csap_descr    CSAP descriptor
 *
 * @return status code
 */
extern int eth_prepare_recv(csap_p csap_descr);

/**
 * Ethernet echo CSAP method. 
 * Method should prepare binary data to be send as "echo" and call 
 * respective write method to send it. 
 * Method may change data stored at passed location.  
 *
 * @param csap_descr    identifier of CSAP
 * @param pkt           Got packet, plain binary data. 
 * @param len           length of data.
 *
 * @return zero on success or error code.
 */
extern int eth_echo_method(csap_p csap_descr, uint8_t *pkt, size_t len);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_ETH_IMPL_H__ */
