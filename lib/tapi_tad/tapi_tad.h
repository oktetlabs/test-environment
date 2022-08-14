/** @file
 * @brief TAPI TAD Generic
 *
 * @defgroup tapi_tad Generic TAD API
 * @ingroup tapi_tad_main
 * @{
 *
 * Definition of Test API for common Traffic Application Domain
 * features.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
#include "rcf_api.h"

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
                                     int64_t *p_llint);

/**
 * Get timestamp CSAP parameter from TA in format "<sec>.<usec>".
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
 * @param stack_id      protocol stack identifier or @c NULL
 * @param csap_spec     ASN value of type CSAP-spec
 * @param handle        location for unique CSAP handle
 *
 * @note If @p stack_id is @c NULL, then it will be determined by @p csap_spec
 *
 * @return zero on success or error code
 */
extern int tapi_tad_csap_create(const char *ta_name, int session,
                                const char *stack_id,
                                const asn_value *csap_spec,
                                csap_handle_t *handle);

/**
 * Destroys CSAP (communication service access point) on the Test Agent.
 *
 * @note In comparison with rcf_ta_csap_destroy() the function
 *       synchronizes /agent/csap subtree of the corresponding
 *       Test Agent.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param handle        CSAP handle
 *
 * @return Status code.
 */
extern te_errno tapi_tad_csap_destroy(const char *ta_name, int session,
                                      csap_handle_t handle);

/**
 * This function is used to force sending of traffic via already created
 * CSAP.
 * Started sending process may be managed via standard function rcf_ta_*.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param csap          CSAP handle
 * @param templ         ASN value of type Traffic-Template
 * @param blk_mode      mode of the operation:
 *                      in blocking mode it suspends the caller
 *                      until sending of all the traffic finishes
 *
 * @return zero on success or error code
 */
extern int tapi_tad_trsend_start(const char *ta_name, int session,
                                 csap_handle_t csap, const asn_value *templ,
                                 rcf_call_mode_t blk_mode);

/**
 * Start receiving of traffic via already created CSAP.
 * Started receiving process may be managed via standard function rcf_ta_*.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param handle        CSAP handle
 * @param pattern       ASN value of type Traffic-Pattern
 * @param timeout       Timeout for traffic receive operation. After this
 *                      time interval CSAP stops capturing any traffic on
 *                      the agent and will be waiting until
 *                      rcf_ta_trrecv_stop() or rcf_ta_trrecv_wait() are
 *                      called.
 * @param num           Number of packets that needs to be captured;
 *                      if it is zero, the number of received packets
 *                      is not limited.
 * @param mode          The flags allows to specify the receive mode.
 *                      Count received packets only, store packets
 *                      to get to the test side later or use pattern sequence
 *                      matching.
 *
 * @return Zero on success or error code
 */
extern te_errno tapi_tad_trrecv_start(const char      *ta_name,
                                      int              session,
                                      csap_handle_t    handle,
                                      const asn_value *pattern,
                                      unsigned int     timeout,
                                      unsigned int     num,
                                      unsigned int     mode);


/**
 * Type for callback which will receive catched packets.
 *
 * @param packet        ASN value with received packet
 * @param user_data     Pointer to opaque data, specified by user for his
 *                      callback,
 *
 * @return none
 */
typedef void (*tapi_tad_trrecv_cb)(asn_value *packet,
                                   void      *user_data);

/**
 * Structure for with parameters for receiving packets
 */
typedef struct tapi_tad_trrecv_cb_data {
    tapi_tad_trrecv_cb  callback;   /**< Callback */
    void               *user_data;  /**< Pointer to user data for it */
} tapi_tad_trrecv_cb_data;

/**
 * Standard method to make struct with parameters for receiving packet.
 *
 * @param callback      User callback
 * @param user_data     Pointer to user data for it
 *
 * @return pointer to new instance of structure.
 */
extern tapi_tad_trrecv_cb_data *tapi_tad_trrecv_make_cb_data(
                                    tapi_tad_trrecv_cb  callback,
                                    void               *user_data);

/**
 * Continue already started receiving process on CSAP.
 * Blocks until reception is finished.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param handle        CSAP handle
 * @param cb_data       Struct with user-specified data for
 *                      catching packets
 * @param num           Location for number of received packets
 *
 * @return status code
 */
extern te_errno tapi_tad_trrecv_wait(const char              *ta_name,
                                     int                      session,
                                     csap_handle_t            handle,
                                     tapi_tad_trrecv_cb_data *cb_data,
                                     unsigned int            *num);

/**
 * Stops already started receiving process on CSAP.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param handle        CSAP handle
 * @param cb_data       Struct with user-specified data for
 *                      catching packets
 * @param num           Location for number of received packets
 *
 * @return status code
 */
extern te_errno tapi_tad_trrecv_stop(const char              *ta_name,
                                     int                      session,
                                     csap_handle_t            handle,
                                     tapi_tad_trrecv_cb_data *cb_data,
                                     unsigned int            *num);

/**
 * Get received packets from already started receiving process on CSAP.
 * Don't block, don't stop receiving process.
 * Received packets are removed from CSAP cache, and will not be returned
 * again, on _wait, _stop or _get calls.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param handle        CSAP handle
 * @param cb_data       Struct with user-specified data for
 *                      catching packets
 * @param num           Location for number of received packets
 *
 * @return status code
 */
extern te_errno tapi_tad_trrecv_get(const char              *ta_name,
                                    int                      session,
                                    csap_handle_t            handle,
                                    tapi_tad_trrecv_cb_data *cb_data,
                                    unsigned int            *num);



/**
 * Insert arithmetical progression iterator argument into Traffic-Template
 * ASN value, at the end of iterator list.
 * New iterator became most internal.
 *
 * @param templ         ASN value of Traffic-Template type
 * @param begin         start of arithmetic progression
 * @param end           end of arithmetic progression
 * @param step          step of arithmetic progression
 *
 * @return status code
 */
extern int tapi_tad_add_iterator_for(asn_value *templ, int begin, int end,
                                     int step);

/**
 * Insert 'enumeration' iterator argument into Traffic-Template ASN value,
 * at the end of iterator list.
 * New iterator became most internal.
 *
 * @param templ         ASN value of Traffic-Template type
 * @param array         pointer to array with enumerated values
 * @param length        length of array
 *
 * @return status code
 */
extern int tapi_tad_add_iterator_ints(asn_value *templ, int *array,
                                      size_t length);



/**
 * Receive all data which currently are waiting for receive in
 * specified CSAP and forward them into another CSAP, without
 * passing via RCF to test.
 *
 * @param ta_name       TA name
 * @param session       RCF session id
 * @param csap_rcv      identifier of recieve CSAP
 * @param csap_fwd      identifier of CSAP which should obtain data
 * @param pattern       traffic Pattern to receive data
 * @param timeout       timeout to wait data, in milliseconds
 * @param forwarded     number of forwarded PDUs (OUT)
 *
 * @return status code
 */
extern int tapi_tad_forward_all(const char *ta_name, int session,
                                csap_handle_t csap_rcv,
                                csap_handle_t csap_fwd,
                                asn_value *pattern,
                                unsigned int timeout,
                                unsigned int *forwarded);

/**
 * Add socket layer over existing file descriptor in CSAP specification.
 *
 * @param csap_spec     Location of CSAP specification pointer.
 * @param fd            File descriptor to read/write data
 *
 * @retval Status code.
 */
extern te_errno tapi_tad_socket_add_csap_layer(asn_value **csap_spec,
                                               int         fd);

/**
 * Get number of unmatched packets using a parameter of CSAP.
 *
 * @param ta_name   - name of the Test Agent
 * @param session   - session identfier to be used
 * @param csap_id   - CSAP handle
 * @param val       - location for number of unmatched packets (OUT)
 *
 * @return Status code.
 */
extern te_errno tapi_tad_csap_get_no_match_pkts(const char *ta_name,
                                                int session,
                                                csap_handle_t csap_id,
                                                unsigned int *val);

/**
 * Finalise all CSAP instances on all Test Agents using RCF.
 *
 * @param session RCF session ID
 *
 * @note The function will synchronise @c /agent/csap
 *       subtrees for all the agents.
 *
 * @return Status code.
 */
extern te_errno tapi_tad_csap_destroy_all(int session);

/**
 * Get timestamp from a packet captured by CSAP.
 *
 * @param pkt     Packet captured by CSAP (described in ASN).
 * @param tv      Where to save obtained timestamp.
 *
 * @return Status code.
 */
extern te_errno tapi_tad_get_pkt_rx_ts(asn_value *pkt, struct timeval *tv);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_TAD_H__ */

/**@} <!-- END tapi_tad --> */
