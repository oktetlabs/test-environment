/** @file
 * @brief Test API
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAPI_TAD_H__
#define __TE_TAPI_TAD_H__

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <unistd.h>

#include "te_defs.h"
#include "te_stdint.h"
#include "tad_common.h"
#include "asn_usr.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get status parameter of CSAP.
 *
 * @param ta_name   - name of the Test Agent
 * @param ta_sid    - session identfier to be used
 * @param csap_id   - CSAP handle
 * @param status    - location for status of csap (OUT)
 *
 * @return Status code.
 */
extern int tapi_csap_get_status(const char *ta_name, int ta_sid,
                                csap_handle_t csap_id,
                                tad_csap_status_t *status);

/**
 * Get total number of bytes parameter of CSAP.
 *
 * @param ta_name   - name of the Test Agent
 * @param ta_sid    - session identfier to be used
 * @param csap_id   - CSAP handle
 * @param p_bytes   - location for total number of bytes (OUT)
 *
 * @return Status code.
 */
extern int tapi_csap_get_total_bytes(const char *ta_name, int ta_sid,
                                     csap_handle_t csap_id,
                                     unsigned long long int *p_bytes);

/**
 * Get duration of the last traffic receiving session on TA CSAP.
 *
 * Returned value is calculated as difference between timestamp
 * of the last packet and timestamp of the first packet.
 *
 * @param ta_name   - name of the Test Agent
 * @param ta_sid    - session identfier to be used
 * @param csap_id   - CSAP handle
 * @param p_dur     - location for duration (OUT)
 *
 * @return Status code.
 */
extern int tapi_csap_get_duration(const char *ta_name, int ta_sid,
                                  csap_handle_t csap_id,
                                  struct timeval *p_dur);

/**
 * Get 'long long int' CSAP parameter from TA.
 *
 * @param ta_name       - name of the Test Agent
 * @param ta_sid        - session identfier to be used
 * @param csap_id       - CSAP handle
 * @param param_name    - parameter name
 * @param p_llint       - location for 'long long int'
 *
 * @return Status code.
 */
extern int tapi_csap_param_get_llint(const char *ta_name, int ta_sid,
                                     csap_handle_t csap_id,
                                     const char *param_name,
                                     long long int *p_llint);

/**
 * Get timestamp CSAP parameter from TA in format '<sec>.<usec>'.
 *
 * @param ta_name       - name of the Test Agent
 * @param ta_sid        - session identfier to be used
 * @param csap_id       - CSAP handle
 * @param param_name    - parameter name
 * @param p_timestamp   - location for timestamp
 *
 * @return Status code.
 */
extern int tapi_csap_param_get_timestamp(const char *ta_name, int ta_sid,
                                         csap_handle_t csap_id,
                                         const char *param_name,
                                         struct timeval *p_timestamp);

/**
 * Creates CSAP (communication service access point) on the Test Agent. 
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0   
 * @param stack_id      protocol stack identifier
 * @param csap_spec     ASN value of type CSAP-spec
 * @param handle        location for unique CSAP handle
 *
 * @return zero on success or error code
 */
extern int tapi_tad_csap_create(const char *ta_name, int session,
                                const char *stack_id, 
                                const asn_value *csap_spec, int *handle);

/**
 * This function is used to force sending of traffic via already created
 * CSAP.
 * Started sending process may be managed via standard function rcf_ta_*.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0   
 * @param handle        CSAP handle
 * @param templ         ASN value of type Traffic-Template
 * @param blk_mode      mode of the operation:
 *                      in blocking mode it suspends the caller
 *                      until sending of all the traffic finishes
 *
 * @return zero on success or error code
 */
extern int tapi_tad_trsend_start(const char *ta_name, int session, 
                                 int handle, const asn_value *templ,
                                 rcf_call_mode_t blk_mode);

/**
 * Start receiving of traffic via already created CSAP. 
 * Started receiving process may be managed via standard function rcf_ta_*.
 *
 * @param ta_name      - Test Agent name.
 * @param session      - TA session or 0.
 * @param handle       - CSAP handle.
 * @param pattern      - ASN value of type Traffic-Pattern
 * @param handler      - Address of function to be used to process
 *                       received packets or NULL.
 * @param user_param   - User parameter to be passed to the handler.
 * @param timeout      - Timeout for traffic receive operation. After this
 *                       time interval CSAP stops capturing any traffic on
 *                       the agent and will be waiting until
 *                       rcf_ta_trrecv_stop or rcf_ta_trrecv_wait are
 *                       called.
 * @param num          - Number of packets that needs to be captured;
 *                       if it is zero, the number of received packets
 *                       is not limited.
 *
 * @return zero on success or error code
 */
extern int tapi_tad_trrecv_start(const char *ta_name, int session,
                                 int handle, const asn_value *pattern,
                                 rcf_pkt_handler handler, void *user_param, 
                                 unsigned int timeout, int num);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_TAD_H__ */
