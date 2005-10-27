/** @file
 * @brief PCAP TAD
 *
 * Traffic Application Domain Command Handler
 * Ethernet-PCAP CSAP implementaion internal declarations.
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


/* Default recv mode: all except OUTGOING packets. */
#define PCAP_RECV_MODE_DEF (PCAP_RECV_HOST      | PCAP_RECV_BROADCAST | \
                            PCAP_RECV_MULTICAST | PCAP_RECV_OTHERHOST )

#ifdef __cplusplus
extern "C" {
#endif


#define PCAP_COMPILED_BPF_PROGRAMS_MAX  1024

#ifndef DEFAULT_PCAP_TYPE
#define DEFAULT_PCAP_TYPE 0
#endif

#ifndef IFNAME_SIZE
#define IFNAME_SIZE 256
#endif

#ifndef PCAP_COMPLETE_FREE
#define PCAP_COMPLETE_FREE 1
#endif

#ifndef PCAP_CSAP_DEFAULT_TIMEOUT  /* Seconds to wait for incoming data */ 
#define PCAP_CSAP_DEFAULT_TIMEOUT 5 
#endif

#define PCAP_LINKTYPE_DEFAULT   DLT_EN10MB

#define TAD_PCAP_SNAPLEN        0xffff


/* 
 * Ethernet-PCAP CSAP specific data
 */

struct pcap_csap_specific_data;

typedef struct pcap_csap_specific_data *pcap_csap_specific_data_p;

typedef struct pcap_csap_specific_data
{
    char *ifname;       /**< Name of the net interface to filter packet on */
    int   iftype;       /**< Default link type (see man 3 pcap) */

    int   out;          /**< Socket for sending data to the media       */
    int   in;           /**< Socket for receiving data                  */
   
    uint8_t recv_mode;  /**< Receive mode, bit mask from values in 
                             enum pcap_csap_receive_mode in ndn_pcap.h  */
    
    int   read_timeout; /**< Number of second to wait for data          */

    size_t total_bytes; /**< Total number of sent or received bytes     */ 
    size_t total_packets; /**< Total amount of sent or received packets */ 
    size_t filtered_packets; /**< Total amount of matched packets */ 
    
    /** Array of pre-compiled BPF programs */
    struct bpf_program *bpfs[PCAP_COMPILED_BPF_PROGRAMS_MAX];
    int    bpf_count;   /**< Total count of pre-compiled BPF programs */

    pcap_t *pcap_session; /**< Dead PCAP session used for compilation of
                               matching patterns in format of tcpdump
                               arguments to BPF programs */

} pcap_csap_specific_data_t;


/**
 * Callback for read parameter value of Ethernet-PCAP CSAP.
 *
 * The function complies with csap_get_param_cb_t prototype.
 */ 
extern char *tad_pcap_get_param_cb(csap_p        csap_descr,
                                   unsigned int  layer,
                                   const char   *param);

/**
 * Callback for read data from media of Ethernet-PCAP CSAP. 
 *
 * The function complies with csap_read_cb_t prototype.
 */ 
extern int tad_pcap_read_cb(csap_p csap_id, int timeout,
                            char *buf, size_t buf_len);


/**
 * Callback for init 'file' CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_init_cb_t prototype.
 */ 
extern te_errno tad_pcap_single_init_cb(int              csap_id,
                                        const asn_value *csap_nds,
                                        unsigned int     layer);

/**
 * Callback for destroy 'file' CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_destroy_cb_t prototype.
 */ 
extern te_errno tad_pcap_single_destroy_cb(int          csap_id,
                                           unsigned int layer);

/**
 * Callback for confirm PDU with Ethernet-PCAP CSAP parameters and
 * possibilities.
 *
 * The function complies with csap_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_pcap_confirm_pdu_cb(int csap_id, unsigned int layer,
                                        asn_value_p tmpl_pdu); 


/**
 * Callback for parse received packet and match it with pattern. 
 *
 * The function complies with csap_match_bin_cb_t prototype.
 */
extern te_errno tad_pcap_match_bin_cb(int              csap_id,
                                      unsigned int     layer,
                                      const asn_value *pattern_pdu,
                                      const csap_pkts *pkt,
                                      csap_pkts       *payload, 
                                      asn_value_p      parsed_packet);


/**
 * Free all memory allocated by Ethernet-PCAP csap specific data
 *
 * @param pcap_csap_specific_data_p poiner to structure
 * @param is_complete if not 0 the final free() will be called on passed pointer
 *
 */ 
extern void free_pcap_csap_data(pcap_csap_specific_data_p spec_data,
                                te_bool is_complete);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_PCAP_IMPL_H__ */
