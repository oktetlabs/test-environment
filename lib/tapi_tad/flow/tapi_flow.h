/** @file
 * @brief Packet flow processing API
 *
 * Definition of Test API for common Traffic Application Domain
 * features.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id: $
 */

#ifndef __TE_TAPI_TAD_FLOW_H__
#define __TE_TAPI_TAD_FLOW_H__

#include "te_defs.h"
#include "te_stdint.h"
#include "asn_usr.h"
#include "asn_impl.h"
#include "ndn.h"
#include "ndn_internal.h"
#include "ndn_flow.h"
#include "tapi_tad.h"
#include "logger_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#if 1
typedef struct tapi_flow_ep_s {
    char           *ta;
    const char     *ifname;
    int             sid;
    char           *csap_desc;
    asn_value      *csap_spec;
    csap_handle_t   csap_id;
} tapi_flow_ep_t;

typedef struct tapi_flow_s {
    asn_value          *flow_spec;
    char               *name;

    tapi_flow_ep_t      snd;
    tapi_flow_ep_t      rcv;
    tapi_flow_ep_t      exp;
    asn_value          *traffic;

    /* Traffic flow counters  */
    int                 total;
    int                 marked;
} tapi_flow_t;

extern char * tapi_flow_preprocess(char *flow_spec);

extern te_errno tapi_flow_parse(tapi_flow_t **flow_p, asn_value *flow_spec);

#define TAPI_FLOW_RECV_TIMEOUT 1000
#define TAPI_FLOW_RECV_COUNT_MAX 10

/* Remove ip4.ip-tos & eth.eth-prio fields from receive pattern */
extern te_errno tapi_flow_remove_marking_ptrn(asn_value *ptrn);

/* Send and receive traffic described in traffic pattern */
extern te_errno tapi_flow_check(tapi_flow_t *flow, te_bool expected,
                                int *rcv_num_p, int *exp_num_p);

extern te_errno tapi_flow_init(tapi_flow_t *flow);

extern te_errno tapi_flow_prepare(tapi_flow_t **flow_p, const char *flow_text);

extern void tapi_flow_fini(tapi_flow_t *flow);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_TAD_FLOW_H__ */
