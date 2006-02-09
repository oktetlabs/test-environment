/** @file
 * @brief TAD SNMP
 *
 * Traffic Application Domain Command Handler.
 * SNMP protocol implementaion internal declarations.
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
 * Callback for init SNMP CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_init_cb_t prototype.
 */ 
extern te_errno tad_snmp_rw_init_cb(csap_p csap, const asn_value *csap_nds);

/**
 * Callback for destroy SNMP CSAP layer if single in stack.
 *
 * The function complies with csap_nbr_destroy_cb_t prototype.
 */ 
extern te_errno tad_snmp_rw_destroy_cb(csap_p csap);


/**
 * Callback for read data from media of SNMP CSAP. 
 *
 * The function complies with csap_read_cb_t prototype.
 */ 
extern te_errno tad_snmp_read_cb(csap_p csap, unsigned int timeout,
                                 tad_pkt *pkt, size_t *pkt_len);

/**
 * Callback for write data to media of SNMP CSAP. 
 *
 * The function complies with csap_write_cb_t prototype.
 */ 
extern te_errno tad_snmp_write_cb(csap_p csap, const tad_pkt *pkt);


/**
 * Callback for release internal data after traffic processing. 
 *
 * The function complies with csap_low_resource_cb_t prototype.
 */
extern te_errno tad_snmp_release_cb(csap_p csap);


/**
 * Callback for generate binary data to be sent to media.
 *
 * The function complies with csap_layer_generate_pkts_cb_t prototype.
 */ 
extern te_errno tad_snmp_gen_bin_cb(csap_p                csap,
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
extern te_errno tad_snmp_match_bin_cb(csap_p           csap,
                        unsigned int     layer,
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu);

/**
 * Callback for generating pattern to filter 
 * just one response to the packet which will be sent by this CSAP 
 * according to this template. 
 *
 * The function complies with csap_layer_gen_pattern_cb_t prototype.
 */
extern te_errno tad_snmp_gen_pattern_cb(csap_p           csap,
                                        unsigned int     layer,
                                        const asn_value *tmpl_pdu, 
                                        asn_value_p     *pattern_pdu);

/**
 * Free snmp pdu
 */
extern void tad_snmp_free_pdu(void *ptr, size_t len);


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
