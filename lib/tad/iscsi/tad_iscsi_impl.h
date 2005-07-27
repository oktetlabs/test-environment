/** @file
 * @brief Test Environment: 
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
 * Author: Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * @(#) $Id$
 */

#ifndef __TE_TAD_ISCSI_IMPL_H__
#define __TE_TAD_ISCSI_IMPL_H__ 

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
#include "ndn_iscsi.h"

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"

#include <sys/queue.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct packet_t {
    CIRCLEQ_ENTRY(packet_t) link;

    uint8_t *buffer;
    size_t   length;
} packet_t;

/**
 * iSCSI target CSAP specific parameters
 */
typedef struct {
    CIRCLEQ_HEAD(packets_head, packet_t) packets_root;

    int             conn_fd[2]; /* pipe for signalling */
    pthread_mutex_t pkt_queue_lock;

} iscsi_target_csap_params_t;
    
/**
 * iSCSI CSAP specific data
 */
typedef struct iscsi_csap_specific_data
{ 
    ndn_iscsi_type_t type;

    iscsi_target_csap_params_t *tgt_params;

    int sock;
} iscsi_csap_specific_data_t;


/**
 * Callback for read parameter value of ISCSI CSAP.
 *
 * @param csap_descr    CSAP descriptor structure.
 * @param level         Index of level in CSAP stack, which param is wanted.
 * @param param         Protocol-specific name of parameter.
 *
 * @return 
 *     String with textual presentation of parameter value, or NULL 
 *     if error occured. User have to free memory at returned pointer.
 */ 
extern char* iscsi_get_param_cb (csap_p csap_descr, int level, const char *param);

/**
 * Callback for read data from media of ISCSI CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param timeout       timeout of waiting for data.
 * @param buf           buffer for read data.
 * @param buf_len       length of available buffer.
 *
 * @return 
 *      quantity of read octets, or -1 if error occured, 0 if timeout expired. 
 */ 
extern int iscsi_read_cb(csap_p csap_descr, int timeout, char *buf,
                        size_t buf_len);

/**
 * Callback for write data to media of ISCSI CSAP. 
 *
 * @param csap_id       identifier of CSAP.
 * @param buf           buffer with data to be written.
 * @param buf_len       length of data in buffer.
 *
 * @return 
 *      quantity of written octets, or -1 if error occured. 
 */ 
extern int iscsi_write_cb(csap_p csap_descr, char *buf, size_t buf_len);

/**
 * Callback for write data to media of ISCSI CSAP and read
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
extern int iscsi_write_read_cb(csap_p csap_descr, int timeout,
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
extern int iscsi_single_init_cb(int csap_id, const asn_value *csap_nds,
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
extern int iscsi_single_destroy_cb (int csap_id, int layer);

/**
 * Callback for confirm PDU with ehternet CSAP parameters and possibilities.
 *
 * @param csap_id       identifier of CSAP
 * @param layer         numeric index of layer in CSAP type to be processed.
 * @param tmpl_pdu      asn_value with PDU (IN/OUT)
 *
 * @return zero on success or error code.
 */ 
extern int iscsi_confirm_pdu_cb (int csap_id, int layer, asn_value *tmpl_pdu); 

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
extern int iscsi_gen_bin_cb(csap_p csap_descr, int layer,
                           const asn_value *tmpl_pdu,
                           const tad_tmpl_arg_t *args, size_t arg_num,
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
extern int iscsi_match_bin_cb(int csap_id, int layer,
                             const asn_value *pattern_pdu,
                             const csap_pkts *pkt, csap_pkts *payload, 
                             asn_value *parsed_packet );

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
extern int iscsi_gen_pattern_cb(int csap_id, int layer, 
                               const asn_value *tmpl_pdu, 
                               asn_value **pattern_pdu);


/**
 * Method for receive data from iSCSI 'target' CSAP to emulator of iSCSI 
 * target.
 *
 * @param csap          ♣SAP id, which should pass data to target
 * @param buffer        location for read data
 * @param len           length of buffer
 *
 * @return amount of passed bytes or -1 on error
 */
extern int iscsi_tad_recv(int csap, uint8_t *buffer, size_t len);

/**
 * Method for send data to iSCSI 'target' CSAP from emulator of iSCSI 
 * target.
 *
 * @param csap          ♣SAP id, which should pass data from target
 * @param buffer        location with data to be sent
 * @param len           length of data
 *
 * @return amount of passed bytes or -1 on error
 */
extern int iscsi_tad_send(int csap, uint8_t *buffer, size_t len);

/**
 * Parameters for iSCSI target emulator thread
 */
typedef struct {
    int send_recv_csap;
} iscsi_target_thread_params_t;


/**
 * Target thread function.
 *
 * @param arg   start argument, should be pointer to
 *              'iscsi_target_thread_params_t' struct
 *
 * @return NULL
 */
extern void *iscsi_server_rx_thread(void *arg);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAD_ISCSI_IMPL_H__ */
