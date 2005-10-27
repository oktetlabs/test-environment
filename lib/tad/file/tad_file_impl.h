/** @file
 * @brief Dummy FILE Protocol TAD
 *
 * Traffic Application Domain Command Handler
 * Dummy FILE protocol implementaion internal declarations.
 *
 * Copyright (C) 2003 Test Environment authors (see tad_file AUTHORS in the
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

#ifndef __TE_TAD_FILE_IMPL_H__
#define __TE_TAD_FILE_IMPL_H__ 

#include "te_errno.h"

#include "asn_usr.h" 
#include "ndn.h"

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"


#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Add structure for CSAP support for respective protocol.
 *
 * @param spt_descr     CSAP layer support structure. 
 *
 * @return zero on success, otherwise error code. 
 */
extern int add_csap_spt(csap_spt_type_p spt_descr);


/**
 * Callback for read parameter value of "file" CSAP.
 *
 * The function complies with csap_get_param_cb_t prototype.
 */ 
extern char *tad_file_get_param_cb(int           csap_id,
                                   unsigned int  layer,
                                   const char   *param);

/**
 * Callback for read data from media of 'file' CSAP. 
 *
 * The function complies with csap_read_cb_t prototype.
 */ 
extern int tad_file_read_cb(csap_p csap_descr, int timeout,
                            char *buf, size_t buf_len);

/**
 * Callback for write data to media of 'file' CSAP. 
 *
 * The function complies with csap_write_cb_t prototype.
 */ 
extern int tad_file_write_cb(csap_p csap_descr,
                             const char *buf, size_t buf_len);

/**
 * Callback for write data to media of 'file' CSAP and read data from
 * media just after write, to get answer to sent request. 
 *
 * The function complies with csap_write_read_cb_t prototype.
 */ 
extern int tad_file_write_read_cb(csap_p csap_descr, int timeout,
                                  const char *w_buf, size_t w_buf_len,
                                  char *r_buf, size_t r_buf_len);


/**
 * Callback for init 'file' CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_init_cb_t prototype.
 */ 
extern te_errno tad_file_single_init_cb(int              csap_id,
                                        const asn_value *csap_nds,
                                        unsigned int     layer);

/**
 * Callback for destroy 'file' CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_destroy_cb_t prototype.
 */ 
extern te_errno tad_file_single_destroy_cb(int          csap_id,
                                           unsigned int layer);

/**
 * Callback for confirm PDU with 'file' CSAP parameters and possibilities.
 *
 * The function complies with csap_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_file_confirm_pdu_cb(int          csap_id,
                                        unsigned int layer,
                                        asn_value_p  tmpl_pdu); 

/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_gen_bin_cb_t prototype.
 */ 
extern int tad_file_gen_bin_cb(csap_p                csap_descr,
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
extern int tad_file_match_bin_cb(int              csap_id,
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
extern int tad_file_gen_pattern_cb(int              csap_id,
                                   unsigned int     layer,
                                   const asn_value *tmpl_pdu, 
                                   asn_value_p     *pattern_pdu);

struct file_csap_specific_data;
typedef struct file_csap_specific_data *file_csap_specific_data_p;
typedef struct file_csap_specific_data {
    char *filename;
    FILE *fstream;
} file_csap_specific_data_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* __TE_TAD_FILE_IMPL_H__ */
