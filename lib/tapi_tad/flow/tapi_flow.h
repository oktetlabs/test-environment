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

/**
 * Flow Endpoint structure
 * Endpoint is an instance to send/receive pdus (with CSAP).
 */
struct tapi_flow_ep {
    SLIST_ENTRY(tapi_flow_ep) link; /**< endpoints linked list */

    char           *name;      /**< endpoint name */
    asn_value      *value;     /**< endpoint description in asn notation */

    char           *ta;        /**< endpoint test agent */
    const char     *ifname;    /**< endpoint interface name */
    int             sid;       /**< rcf session id to perform tad
                                    operations with endpoint csap */
    char           *csap_desc; /**< csap layers names like udp.ip4.eth */
    asn_value      *csap_spec; /**< csap asn specification */
    csap_handle_t   csap_id;   /**< endpoint csap */
};

typedef struct tapi_flow_traffic tapi_flow_traffic;

/**
 * Flow Traffic entry structure
 * Traffic entry contains all information to send and receive single pdu.
 */
struct tapi_flow_traffic {
    SLIST_ENTRY(tapi_flow_traffic) link; /**< traffic linked list */

    char           *name;           /**< traffic entry name */
    asn_value      *value;          /**< traffic asn notation */

    tapi_flow_ep   *snd;            /**< endpoint to send pdu from */
    tapi_flow_ep   *rcv;            /**< endpoint to receive pdu at */
    tapi_flow_ep   *rcv_base;       /**< second instance of receive endpoint
                                         to perform only base matching */

    asn_value      *send_tmpl;      /**< send pdu template */
    asn_value      *recv_ptrn;      /**< receive pdu pattern */
    asn_value      *recv_base_ptrn; /**< basic receive pattern
                                         (only mac/ip addresses matching) */

    int             plen;           /**< payload length */
    int             count;          /**< amount of times the pdu should be sent */

    te_bool         started;        /**< operating status of receive operation */

    /* Traffic flow counters  */
    int             total;          /**< total count of received pdus
                                         matching base rules */
    int             marked;         /**< amount of matched received pdus */
};

/**
 * General Flow structure
 * List of endpoints and traffic entries
 */
typedef struct tapi_flow_s {
    asn_value *flow_spec;

    SLIST_HEAD(, tapi_flow_ep) ep_list;
    SLIST_HEAD(, tapi_flow_traffic) traffic_list;

} tapi_flow_t;


/**
 * Preprocess parameter value and resolve all configurator links within.
 *
 * @param param Textual parameter value to preprocess
 *
 * @return Preprocessed value on success or NULL on any error
 */
extern char *tapi_flow_preprocess_links(const char *param);

/**
 * Preprocess parameter value and substitute quoted values.
 * Quoted notation is used to increase readability
 * Also dereference configurator links if quotting means
 * configurator link reduction.
 *
 * @param param Textual parameter value to preprocess
 *
 * @return Preprocessed value on success or NULL on any error
 */
extern char *tapi_flow_preprocess_quotes(const char *param);

/**
 * Preprocess all test arguments
 *
 * @param argc Amount of arguments to preprocess
 * @param argv Array of test arguments
 *
 * @return Array of preprocessed arguments on success or NULL on any error
 */
extern char **tapi_flow_preprocess_args(int argc, char **argv);

/**
 * Get and process flow parametrisation from test arguments
 *
 * @param argc Amount of arguments to preprocess
 * @param argv Array of test arguments
 * @param flow Initialised flow structure (OUT)
 *
 * @return 0, if success, or -1 otherwise
 */
extern te_errno tapi_flow_conf_get(int argc, char **argv,
                                   tapi_flow_t *flow);

/**
 * Find Endpoint by its name
 *
 * @param flow      Flow structure to find endpoint in
 * @param name      Name of endpoint to find
 *
 * @return Endpoint structure, if success, or NULL otherwise
 */
extern tapi_flow_ep *tapi_flow_find_ep(tapi_flow_t *flow,
                                       const char *name);

/**
 * Find Traffic Entry by its name
 *
 * @param flow      Flow structure to find traffic entry in
 * @param name      Name of traffic entry to find
 *
 * @return Endpoint structure, if success, or NULL otherwise
 */
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
extern te_errno tapi_flow_start(tapi_flow_t *flow, char *name);

extern te_errno tapi_flow_stop(tapi_flow_t *flow, char *name,
                               int *rcv_num_p, int *rcv_base_num_p);

extern te_errno tapi_flow_check(tapi_flow_t *flow, char *name,
                                int *rcv_num_p, int *rcv_base_num_p);

extern te_errno tapi_flow_check_sequence(tapi_flow_t *flow,...);

extern te_errno tapi_flow_check_all(tapi_flow_t *flow,
                                    const char *traffic_prefix);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_TAD_FLOW_H__ */
