/** @file
 * @brief SNMP TAD
 *
 * Traffic Application Domain Command Handler
 * SNMP protocol implementaion internal declarations.
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

#ifndef __TE_TAD_SNMP_IMPL_H__
#define __TE_TAD_SNMP_IMPL_H__ 

#include "config.h" 

#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h> 
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "te_errno.h"

#include "asn_usr.h" 
#include "ndn_snmp.h"

#if HAVE_NET_SNMP_SESSION_API_H
#include <net-snmp/net-snmp-config.h> 
#include <net-snmp/session_api.h> 
#include <net-snmp/pdu_api.h> 
#include <net-snmp/snmpv3_api.h>
#include <net-snmp/library/tools.h>
#elif HAVE_UCD_SNMP_SNMP_CLIENT_H
#include <ucd-snmp/asn1.h>
#include <ucd-snmp/snmp.h>
#include <ucd-snmp/snmp_impl.h>
#include <ucd-snmp/snmp_api.h>
#include <ucd-snmp/snmp_client.h>
#else
#error This module can not be compiled without NET- or UCD-SNMP library.
#endif /* HAVE_SNMP */

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SNMP_CSAP_DEF_VERSION
#define SNMP_CSAP_DEF_VERSION 1
#endif 

#ifndef SNMP_CSAP_DEF_REMPORT
#define SNMP_CSAP_DEF_REMPORT 161
#endif 


#ifndef SNMP_CSAP_DEF_LOCPORT
#define SNMP_CSAP_DEF_LOCPORT 0
#endif 

#ifndef SNMP_CSAP_DEF_AGENT
#define SNMP_CSAP_DEF_AGENT "localhost"
#endif 

#ifndef SNMP_CSAP_DEF_COMMUNITY
#define SNMP_CSAP_DEF_COMMUNITY "public"
#endif 

#ifndef SNMP_CSAP_DEF_TIMEOUT
#define SNMP_CSAP_DEF_TIMEOUT 5
#endif 

/* number of variables in for GET-BULK */
#ifndef SNMP_CSAP_DEF_REPEATS
#define SNMP_CSAP_DEF_REPEATS 10
#endif 


/**
 * Callback for read parameter value of SNMP CSAP.
 *
 * The function complies with csap_get_param_cb_t prototype.
 */ 
extern char *tad_snmp_get_param_cb(int           csap_id,
                                   unsigned int  layer,
                                   const char   *param);

/**
 * Callback for read data from media of SNMP CSAP. 
 *
 * The function complies with csap_read_cb_t prototype.
 */ 
extern int tad_snmp_read_cb(csap_p csap_descr, int timeout,
                            char *buf, size_t buf_len);

/**
 * Callback for write data to media of SNMP CSAP. 
 *
 * The function complies with csap_write_cb_t prototype.
 */ 
extern int tad_snmp_write_cb(csap_p csap_descr,
                             const char *buf, size_t buf_len);

/**
 * Callback for write data to media of SNMP CSAP and read data from
 * media just after write, to get answer to sent request. 
 *
 * The function complies with csap_write_read_cb_t prototype.
 */ 
extern int tad_snmp_write_read_cb(csap_p csap_descr, int timeout,
                                  const char *w_buf, size_t w_buf_len,
                                  char *r_buf, size_t r_buf_len);


/**
 * Callback for init SNMP CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_init_cb_t prototype.
 */ 
extern te_errno tad_snmp_single_init_cb(int              csap_id,
                                        const asn_value *csap_nds,
                                        unsigned int     layer);

/**
 * Callback for destroy SNMP CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_destroy_cb_t prototype.
 */ 
extern te_errno tad_snmp_single_destroy_cb(int          csap_id,
                                           unsigned int layer);

/**
 * Callback for confirm PDU with SNMP CSAP parameters and possibilities.
 *
 * The function complies with csap_confirm_pdu_cb_t prototype.
 */ 
extern te_errno tad_snmp_confirm_pdu_cb(int          csap_id,
                                        unsigned int layer,
                                        asn_value_p  tmpl_pdu); 

/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_gen_bin_cb_t prototype.
 */ 
extern te_errno tad_snmp_gen_bin_cb(csap_p                csap_descr,
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
extern te_errno tad_snmp_match_bin_cb(int              csap_id,
                                      unsigned int     layer,
                                      const asn_value *pattern_pdu,
                                      const csap_pkts *pkt,
                                      csap_pkts       *payload,
                                      asn_value       *parsed_packet);

/**
 * Callback for generating pattern to filter 
 * just one response to the packet which will be sent by this CSAP 
 * according to this template. 
 *
 * The function complies with csap_gen_pattern_cb_t prototype.
 */
extern te_errno tad_snmp_gen_pattern_cb(int              csap_id,
                                        unsigned int     layer,
                                        const asn_value *tmpl_pdu, 
                                        asn_value_p     *pattern_pdu);

/**
 * Free snmp pdu
 */
extern void tad_snmp_free_pdu(void *ptr);


struct snmp_csap_specific_data;
typedef struct snmp_csap_specific_data *snmp_csap_specific_data_p;
typedef struct snmp_csap_specific_data {
    struct snmp_session *ss;
    struct snmp_pdu     *pdu;
    int                  sock;
} snmp_csap_specific_data_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_SNMP_IMPL_H__ */
