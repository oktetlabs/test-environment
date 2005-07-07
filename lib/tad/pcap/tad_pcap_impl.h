/** @file
 * @brief Test Environment: 
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
 * @param csap_id       identifier of CSAP.
 * @param level         Index of level in CSAP stack, which param is wanted.
 * @param param         Protocol-specific name of parameter.
 *
 * @return 
 *     String with textual presentation of parameter value, or NULL 
 *     if error occured. User have to free memory at returned pointer.
 */ 
extern char* pcap_get_param_cb (csap_p csap_descr, int level, const char *param);

/**
 * Callback for read data from media of Ethernet-PCAP CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param timeout       timeout of waiting for data.
 * @param buf           buffer for read data.
 * @param buf_len       length of available buffer.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
extern int pcap_read_cb (csap_p csap_id, int timeout, char *buf, size_t buf_len);

/**
 * Callback for write data to media of Ethernet-PCAP CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param buf           buffer with data to be written.
 * @param buf_len       length of data in buffer.
 *
 * @return 
 *      quantity of written octets, or -1 if error occured. 
 */ 
extern int pcap_write_cb (csap_p csap_id, char *buf, size_t buf_len);

/**
 * Callback for write data to media of Ethernet-PCAP CSAP and read
 *  data from media just after write, to get answer to sent request. 
 *
 * @param csap_id       identifier of CSAP.
 * @param timeout       timeout of waiting for data.
 * @param w_buf         buffer with data to be written.
 * @param w_buf_len     length of data in w_buf.
 * @param r_buf         buffer for data to be read.
 * @param r_buf_len     available length r_buf.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
extern int pcap_write_read_cb (csap_p csap_id, int timeout,
                               char *w_buf, size_t w_buf_len,
                               char *r_buf, size_t r_buf_len);


/**
 * Callback for init 'file' CSAP layer if single in stack.
 *
 * @param csap_id       identifier of CSAP.
 * @param csap_nds      asn_value with CSAP init parameters
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
extern int pcap_single_init_cb (int csap_id, const asn_value *csap_nds,
                                int layer);

/**
 * Callback for destroy 'file' CSAP layer if single in stack.
 *      This callback should free all undeground media resources used by 
 *      this layer and all memory used for layer-specific data and pointed 
 *      in respective structure in 'layer-data' in CSAP instance struct. 
 *
 * @param csap_id       identifier of CSAP.
 * @param layer         numeric index of layer in CSAP type to be processed. 
 *                      Layers are counted from zero, from up to down.
 *
 * @return zero on success or error code.
 */ 
extern int pcap_single_destroy_cb (int csap_id, int layer);

/**
 * Callback for confirm PDU with Ethernet-PCAP CSAP parameters and possibilities.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU (IN/OUT)
 *
 * @return zero on success or error code.
 */ 
extern int pcap_confirm_pdu_cb (int csap_id, int layer, asn_value_p tmpl_pdu); 

/**
 * Callback for generate binary data to be sent to media.
 *
 * @param csap_descr    CSAP instance
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU. 
 * @param up_payload    pointer to data which is already generated for upper 
 *                      layers and is payload for this protocol level. 
 *                      May be zero.  Presented as list of packets. 
 *                      Almost always this list will contain only one element, 
 *                      but need in fragmentation sometimes may occur. 
 *                      Of cause, on up level only one PDU is passed, 
 *                      but upper layer (if any present) may perform 
 *                      fragmentation, and current layer may have possibility 
 *                      to de-fragment payload.
 * @param pkts          Callback have to fill this structure with list of 
 *                      generated packets. Almost always this list will 
 *                      contain only one element, but need 
 *                      in fragmentation sometimes may occur. (OUT)
 *
 * @return zero on success or error code.
 */ 
extern int pcap_gen_bin_cb (csap_p csap_descr, int layer,
                            const asn_value *tmpl_pdu,
                            const tad_tmpl_arg_t *args, size_t  arg_num, 
                            csap_pkts_p up_payload, csap_pkts_p pkts);


/**
 * Callback for parse received packet and match it with pattern. 
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param pattern_pdu   pattern NDS 
 * @param pkt           recevied packet
 * @param payload       rest upper layer payload, if any exists. (OUT)
 * @param parsed_packet caller of method should pass here empty asn_value 
 *                      instance of ASN type of expected PDU. Callback 
 *                      have to fill this instance with values from 
 *                      parsed and matched packet
 *
 * @return zero on success or error code.
 */
extern int pcap_match_bin_cb (int csap_id, int layer,
                              const asn_value *pattern_pdu,
                              const csap_pkts *pkt, csap_pkts *payload, 
                              asn_value_p parsed_packet);

/**
 * Callback for generating pattern to filter 
 * just one response to the packet which will be sent by this CSAP 
 * according to this template. 
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      ASN value with template PDU.
 * @param pattern_pdu   OUT: ASN value with pattern PDU, generated according 
 *                      to passed template PDU and CSAP parameters. 
 *
 * @return zero on success or error code.
 */
extern int pcap_gen_pattern_cb(int csap_id, int layer,
                               const asn_value *tmpl_pdu, 
                               asn_value_p  *pattern_pdu);


/**
 * Free all memory allocated by Ethernet-PCAP csap specific data
 *
 * @param pcap_csap_specific_data_p poiner to structure
 * @param is_complete if not 0 the final free() will be called on passed pointer
 *
 */ 
extern void free_pcap_csap_data(pcap_csap_specific_data_p spec_data, te_bool is_complete);

/**
 * Find number of Ethernet-PCAP layer in CSAP stack.
 *
 * @param csap_descr    CSAP description structure.
 * @param layer_name    Name of the layer to find.
 *
 * @return number of layer (start from zero) or -1 if not found. 
 */ 
extern int find_csap_layer(csap_p csap_descr, char *layer_name);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_PCAP_IMPL_H__ */
