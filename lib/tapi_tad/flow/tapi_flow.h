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

#ifdef _SYS_QUEUE_H_
#include <sys/queue.h>
#else
#include "te_queue.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TAPI_FLOW_RECV_TIMEOUT 1000
#define TAPI_FLOW_RECV_COUNT_MAX 10

/**
 * Read configuration from test's parameters
 *
 * @param platfrom_config  Pointer to platform_config_s structure to
 *                         receive configuration (OUT)
 *
 */
#define TEST_GET_FLOW_CONFIG(_flow) \
    do {                                                            \
        te_errno rc;                                                \
                                                                    \
        tapi_flow_init((_flow));                                    \
        if (0 != (rc = tapi_flow_conf_get(argc, argv, (_flow))))    \
        {                                                           \
            TEST_FAIL("Failed to get ASN-based flow description, "  \
                      "rc=%r", rc);                                 \
        }                                                           \
    } while (0)

typedef struct tapi_flow_ep tapi_flow_ep;

struct tapi_flow_ep {
    SLIST_ENTRY(tapi_flow_ep) link; /**< endpoints linked list */

    char           *name;
    asn_value      *value;

    char           *ta;
    const char     *ifname;
    int             sid;
    char           *csap_desc;
    asn_value      *csap_spec;
    csap_handle_t   csap_id;
};

typedef struct tapi_flow_traffic tapi_flow_traffic;

struct tapi_flow_traffic {
    SLIST_ENTRY(tapi_flow_traffic) link; /**< traffic linked list */

    char           *name;
    asn_value      *value;

    tapi_flow_ep   *snd;
    tapi_flow_ep   *rcv;
    tapi_flow_ep   *rcv_base;

    asn_value      *send_tmpl;
    asn_value      *recv_ptrn;
    asn_value      *recv_base_ptrn;

    int             plen;
    int             count;

    /* Traffic flow counters  */
    int             total;
    int             marked;
};

typedef struct tapi_flow_s {
    asn_value *flow_spec;

    SLIST_HEAD(, tapi_flow_ep) ep_list;
    SLIST_HEAD(, tapi_flow_traffic) traffic_list;

} tapi_flow_t;


extern char *tapi_flow_preprocess(const char *flow_spec);

extern char **tapi_flow_preprocess_args(int argc, char **argv);

extern te_errno tapi_flow_conf_get(int argc, char **argv,
                                   tapi_flow_t *flow);

extern tapi_flow_ep *tapi_flow_find_ep(tapi_flow_t *flow,
                                       const char *name);

extern tapi_flow_traffic *tapi_flow_find_traffic(tapi_flow_t *flow,
                                                 const char *name);

extern te_errno tapi_flow_prepare_endpoints(tapi_flow_t *flow);
extern te_errno tapi_flow_prepare_traffic(tapi_flow_t *flow);

extern te_errno tapi_flow_csap_spec_to_stack(asn_value *spec, char **stack);

extern te_errno tapi_flow_gen_base_ptrn(asn_value *rcv_ptrn,
                                        asn_value **base_ptrn_p);

extern te_errno tapi_flow_setup_endpoints(tapi_flow_t *flow);

extern te_errno tapi_flow_prepare(int argc, char **argv, tapi_flow_t *flow);

extern void tapi_flow_init(tapi_flow_t *flow);

extern void tapi_flow_fini(tapi_flow_t *flow);

/* Send and receive traffic described in traffic pattern */
extern te_errno tapi_flow_check(tapi_flow_t *flow, char *name,
                                int *rcv_num_p, int *rcv_base_num_p);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_TAD_FLOW_H__ */
