/** @file
 * @brief ARP TAD
 *
 * Traffic Application Domain Command Handler
 * ARP CSAP support internal declarations.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_ARP_IMPL_H__
#define __TE_TAD_ARP_IMPL_H__ 

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "asn_usr.h"

#include "tad_csap_support.h"
#include "tad_csap_inst.h"


typedef char *(*csap_get_param_cb_t)(csap_p        csap_descr,
                                     unsigned int  layer, 
                                     const char   *param);

typedef te_errno (*csap_low_resource_cb_t)(csap_p csap_descr);

typedef int (*csap_read_cb_t)(csap_p csap_descr, int timeout, 
                              char *buf, size_t buf_len);

typedef int (*csap_write_cb_t)(csap_p csap_descr, const char *buf,
                               size_t buf_len);

typedef int (*csap_write_read_cb_t)(csap_p csap_descr, int timeout,
                                    const char *w_buf, size_t w_buf_len,
                                    char *r_buf, size_t r_buf_len);
typedef te_errno (*csap_check_pdus_cb_t)(csap_p csap_descr, 
                                         asn_value *traffic_nds);

typedef te_errno (*csap_echo_method)(csap_p csap_descr, uint8_t *pkt, 
                                     size_t len);


typedef te_errno (*csap_nbr_init_cb_t)(int              csap_id,
                                       const asn_value *csap_nds,
                                       unsigned int     layer);

typedef te_errno (*csap_nbr_destroy_cb_t)(int          csap_id,
                                          unsigned int layer);

typedef te_errno (*csap_confirm_pdu_cb_t)(int           csap_id,
                                          unsigned int  layer, 
                                          asn_value    *traffic_pdu); 

typedef te_errno (*csap_gen_bin_cb_t)(csap_p                csap_descr,
                                      unsigned int          layer,
                                      const asn_value      *tmpl_pdu,
                                      const tad_tmpl_arg_t *args,
                                      size_t                arg_num,
                                      csap_pkts_p           up_payload,
                                      csap_pkts_p           pkts);

typedef te_errno (*csap_match_bin_cb_t)(int              csap_id,
                                        unsigned int     layer,
                                        const asn_value *pattern_pdu,
                                        const csap_pkts *pkt,
                                        csap_pkts       *payload,
                                        asn_value       *parsed_packet);

typedef te_errno (*csap_gen_pattern_cb_t)(int               csap_id,
                                          unsigned int      layer,
                                          const asn_value  *tmpl_pdu, 
                                          asn_value       **pattern_pdu);

#endif /* !__TE_TAD_ARP_IMPL_H__ */
