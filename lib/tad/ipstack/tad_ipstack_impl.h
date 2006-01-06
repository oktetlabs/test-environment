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
 * IPv4 CSAP specific data
 */
typedef struct ip4_csap_specific_data
{
    int    socket;             /**< Socket for receiving data to the media */
    struct sockaddr_in sa_op;  /**< Sockaddr for current operation 
                                    to the media   */ 
    int    read_timeout;       /**< Number of second to wait for data   */ 

    uint16_t         protocol; /**< Up layer default protocol */

    struct in_addr   local_addr;
    struct in_addr   remote_addr;

    struct in_addr   src_addr;
    struct in_addr   dst_addr;

    tad_data_unit_t  du_version;
    tad_data_unit_t  du_header_len;
    tad_data_unit_t  du_tos;
    tad_data_unit_t  du_ip_len;
    tad_data_unit_t  du_ip_ident;
    tad_data_unit_t  du_flags;
    tad_data_unit_t  du_ip_offset;
    tad_data_unit_t  du_ttl;
    tad_data_unit_t  du_protocol;
    tad_data_unit_t  du_h_checksum;
    tad_data_unit_t  du_src_addr;
    tad_data_unit_t  du_dst_addr;

} ip4_csap_specific_data_t;



/* 
 * UDP CSAP specific data
 */
typedef struct udp_csap_specific_data
{
    unsigned short   local_port;    /**< Local UDP port */
    unsigned short   remote_port;   /**< Remote UDP port */ 

    unsigned short   src_port;    /**< Source UDP port for current packet*/
    unsigned short   dst_port;    /**< Dest.  UDP port for current packet*/

    tad_data_unit_t  du_src_port;
    tad_data_unit_t  du_dst_port;

} udp_csap_specific_data_t;


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
    size_t           wait_length;
    uint8_t         *stored_buffer;
    size_t           stored_length;

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
extern te_errno tad_ip4_rw_init_cb(csap_p csap, const asn_value *nds);


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
extern int tad_ip4_read_cb(csap_p csap, int timeout,
                           char *buf, size_t buf_len);

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
 * Callback for confirm PDU with ehternet CSAP parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_ip4_confirm_pdu_cb(csap_p       csap,
                                       unsigned int layer,
                                       asn_value_p  layer_pdu,
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


/**
 * Callback for parse received packet and match it with pattern. 
 *
 * The function complies with csap_layer_match_bin_cb_t prototype.
 */
extern te_errno tad_ip4_match_bin_cb(csap_p           csap,
                                     unsigned int     layer,
                                     const asn_value *pattern_pdu,
                                     const csap_pkts *pkt,
                                     csap_pkts       *payload, 
                                     asn_value_p      parsed_packet);


/*
 * ICMP callbacks
 */ 

/**
 * Callback for read parameter value of ICMP CSAP.
 *
 * The function complies with csap_layer_get_param_cb_t prototype.
 */ 
extern char *tad_icmp4_get_param_cb(csap_p        csap,
                                    unsigned int  layer,
                                    const char   *param);

/**
 * Callback for read data from media of ICMP CSAP. 
 *
 * The function complies with csap_read_cb_t prototype.
 */ 
extern int tad_icmp4_read_cb(csap_p csap, int timeout,
                             char *buf, size_t buf_len);

/**
 * Callback for write data to media of ICMP CSAP. 
 *
 * The function complies with csap_write_read_cb_t prototype.
 */ 
extern te_errno tad_icmp4_write_cb(csap_p csap, const tad_pkt *pkt);

/**
 * Callback for write data to media of ICMP CSAP and read
 * data from media just after write, to get answer to sent request. 
 *
 * The function complies with csap_write_cb_t prototype.
 */ 
extern int tad_icmp4_write_read_cb(csap_p csap, int timeout,
                                   const tad_pkt *w_pkt,
                                   char *r_buf, size_t r_buf_len);


/**
 * Callback for init 'file' CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_init_cb_t prototype.
 */ 
extern te_errno tad_icmp4_single_init_cb(csap_p           csap,
                                         unsigned int     layer,
                                         const asn_value *csap_nds);

/**
 * Callback for destroy 'file' CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_destroy_cb_t prototype.
 */ 
extern te_errno tad_icmp4_single_destroy_cb(csap_p       csap,
                                            unsigned int layer);

/**
 * Callback for confirm PDU with ehternet CSAP parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_icmp4_confirm_pdu_cb(csap_p       csap,
                                         unsigned int layer,
                                         asn_value_p  tmpl_pdu); 

/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */ 
extern te_errno tad_icmp4_gen_bin_cb(csap_p                csap,
                                     unsigned int          layer,
                                     const asn_value      *tmpl_pdu,
                                     const tad_tmpl_arg_t *args,
                                     size_t                arg_num, 
                                     tad_pkts             *sdus,
                                     tad_pkts             *pdus);


/**
 * Callback for parse received packet and match it with pattern. 
 *
 * The function complies with csap_layer_match_bin_cb_t prototype.
 */
extern te_errno tad_icmp4_match_bin_cb(csap_p           csap,
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
 * The function complies with csap_layer_gen_pattern_cb_t prototype.
 */
extern te_errno tad_icmp4_gen_pattern_cb(csap_p           csap,
                                         unsigned int     layer,
                                         const asn_value *tmpl_pdu, 
                                         asn_value_p     *pattern_pdu);




/*
 * UDP callbacks
 */ 


/**
 * Callback for init 'udp' CSAP layer if over 'ip4' in stack.
 *
 * The function complies with csap_nbr_init_cb_t prototype.
 */ 
extern te_errno tad_udp_ip4_init_cb(csap_p           csap,
                                    unsigned int     layer,
                                    const asn_value *csap_nds);

/**
 * Callback for destroy 'udp' CSAP layer if over 'ip4' in stack.
 *
 * The function complies with csap_nbr_destroy_cb_t prototype.
 */ 
extern te_errno tad_udp_ip4_destroy_cb(csap_p       csap,
                                       unsigned int layer);

/**
 * Callback for read data from media of UDP CSAP. 
 *
 * The function complies with csap_read_cb_t prototype.
 */ 
extern int tad_udp_read_cb(csap_p csap, int timeout,
                           char *buf, size_t buf_len);

/**
 * Callback for write data to media of UDP CSAP. 
 *
 * The function complies with csap_write_read_cb_t prototype.
 */ 
extern te_errno tad_udp_ip4_write_cb(csap_p csap, const tad_pkt *pkt);

/**
 * Callback for write data to media of UDP CSAP and read
 * data from media just after write, to get answer to sent request. 
 *
 * The function complies with csap_write_cb_t prototype.
 */ 
extern int tad_udp_ip4_write_read_cb(csap_p csap, int timeout,
                                     const tad_pkt *w_pkt,
                                     char *r_buf, size_t r_buf_len);


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
 * Callback for confirm PDU with ehternet CSAP parameters and possibilities.
 *
 * The function complies with csap_layer_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_udp_confirm_pdu_cb(csap_p         csap,
                                       unsigned int   layer,
                                       asn_value_p    tmpl_pdu,
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


/**
 * Callback for parse received packet and match it with pattern. 
 *
 * The function complies with csap_layer_match_bin_cb_t prototype.
 */
extern te_errno tad_udp_match_bin_cb(csap_p           csap,
                                     unsigned int     layer,
                                     const asn_value *pattern_pdu,
                                     const csap_pkts *pkt,
                                     csap_pkts       *payload, 
                                     asn_value_p      parsed_packet);


/*
 * TCP callbacks
 */ 

/**
 * Callback for read data from media of TCP CSAP. 
 *
 * The function complies with csap_read_cb_t prototype.
 */ 
extern int tad_tcp_read_cb(csap_p csap, int timeout,
                           char *buf, size_t buf_len);

/**
 * Callback for write data to media of TCP CSAP. 
 *
 * The function complies with csap_write_read_cb_t prototype.
 */ 
extern te_errno tad_tcp_write_cb(csap_p csap, const tad_pkt *pkt);

/**
 * Callback for write data to media of TCP CSAP and read
 *  data from media just after write, to get answer to sent request. 
 *
 * The function complies with csap_write_cb_t prototype.
 */ 
extern int tad_tcp_write_read_cb(csap_p csap, int timeout,
                                 const tad_pkt *w_pkt,
                                 char *r_buf, size_t r_buf_len);


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
                                       asn_value_p    tmpl_pdu,
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
                                     const asn_value *pattern_pdu,
                                     const csap_pkts *pkt,
                                     csap_pkts       *payload, 
                                     asn_value_p      parsed_packet);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_IPSTACK_IMPL_H__ */
