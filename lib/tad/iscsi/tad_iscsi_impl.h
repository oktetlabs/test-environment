/** @file
 * @brief iSCSI TAD
 *
 * Traffic Application Domain Command Handler
 * Ethernet CSAP implementaion internal declarations.
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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

#include "iscsi_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct packet_t {
    CIRCLEQ_ENTRY(packet_t) link;

    size_t   length;
    uint8_t *buffer;
    uint8_t *allocated_buffer;
} packet_t;

#if 0
typedef struct packet_queue_t {
    CIRCLEQ_HEAD(q_head, packet_t) head; 

    pthread_mutex_t  pkt_queue_lock;
    int              conn_fd[2]; /* pipe for signalling */ 
} packet_queue_t;
#endif
    

enum {
    TGT_SITE,
    CSAP_SITE
};

/**
 * iSCSI CSAP specific data
 */
typedef struct iscsi_csap_specific_data
{ 
    int             socket;
    pthread_t       iscsi_target_thread;

    size_t          wait_length;
    size_t          stored_length;
    uint8_t        *stored_buffer;
} iscsi_csap_specific_data_t;

extern iscsi_target_params_t target_params;


/**
 * Callback for read parameter value of iSCSI CSAP.
 *
 * The function complies with csap_get_param_cb_t prototype.
 */ 
extern char *tad_iscsi_get_param_cb(csap_p csap_descr, unsigned int layer,
                                    const char *param);

/**
 * Callback for read data from media of iSCSI CSAP. 
 *
 * The function complies with csap_read_cb_t prototype.
 */ 
extern int tad_iscsi_read_cb(csap_p csap_descr, int timeout, char *buf,
                             size_t buf_len);

/**
 * Callback for write data to media of ISCSI CSAP. 
 *
 * The function complies with csap_write_cb_t prototype.
 */ 
extern int tad_iscsi_write_cb(csap_p csap_descr, const char *buf,
                              size_t buf_len);

/**
 * Callback for write data to media of ISCSI CSAP and read data from 
 * media just after write, to get answer to sent request. 
 *
 * The function complies with csap_write_read_cb_t prototype.
 *
 * The function complies with csap_write_read_cb_t prototype.
 */ 
extern int tad_iscsi_write_read_cb(csap_p csap_descr, int timeout,
                                   const char *w_buf, size_t w_buf_len,
                                   char *r_buf, size_t r_buf_len);


/**
 * Callback for init 'file' CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_init_cb_t prototype.
 */ 
extern int tad_iscsi_single_init_cb(int csap_id, const asn_value *csap_nds,
                                    unsigned int layer);

/**
 * Callback for destroy 'file' CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_destroy_cb_t prototype.
 */ 
extern te_errno tad_iscsi_single_destroy_cb(int          csap_id,
                                            unsigned int layer);

/**
 * Callback for confirm PDU with ehternet CSAP parameters and possibilities.
 *
 * The function complies with csap_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_iscsi_confirm_pdu_cb(int csap_id, unsigned int layer,
                                         asn_value *tmpl_pdu); 

/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_gen_bin_cb_t prototype.
 */ 
extern te_errno tad_iscsi_gen_bin_cb(csap_p                 csap_descr,
                                     unsigned int           layer,
                                     const asn_value       *tmpl_pdu,
                                     const tad_tmpl_arg_t  *args,
                                     size_t                 arg_num,
                                     csap_pkts_p            up_payload,
                                     csap_pkts_p            pkts);

/**
 * Callback for parse received packet and match it with pattern. 
 *
 * The function complies with csap_match_bin_cb_t prototype.
 */
extern te_errno tad_iscsi_match_bin_cb(int              csap_id,
                                       unsigned int     layer,
                                       const asn_value *pattern_pdu,
                                       const csap_pkts *pkt,
                                       csap_pkts       *payload, 
                                       asn_value       *parsed_packet);

/**
 * Callback for generating pattern to filter just one response to 
 * the packet which will be sent by this CSAP according to this template. 
 *
 * The function complies with csap_gen_pattern_cb_t prototype.
 */
extern te_errno tad_iscsi_gen_pattern_cb(int                csap_id,
                                         unsigned int       layer,
                                         const asn_value   *tmpl_pdu, 
                                         asn_value        **pattern_pdu);

/**
 * Prepare send callback
 *
 * @param csap_descr    CSAP descriptor structure. 
 *
 * @return status code.
 */ 
extern te_errno tad_iscsi_prepare_send_cb(csap_p csap_descr);

/**
 * Prepare recv callback
 *
 * @param csap_descr    CSAP descriptor structure. 
 *
 * @return status code.
 */ 
extern int tad_iscsi_prepare_recv_cb(csap_p csap_descr);

/**
 * Target thread function.
 *
 * @param arg   start argument, should be pointer to
 *              'iscsi_target_thread_params_t' struct
 *
 * @return NULL.
 */
extern void *iscsi_server_rx_thread(void *arg);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAD_ISCSI_IMPL_H__ */
