/** @file
 * @brief Test API for TAD. iSCSI CSAP
 *
 * Implementation of Test API
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */


#ifndef __TE_TAPI_ISCSI_H__
#define __TE_TAPI_ISCSI_H__

#include <assert.h>
#include <netinet/in.h>
#include <semaphore.h>

#include "te_stdint.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "ndn_iscsi.h"
#include "tapi_rpc.h"

#define ISCSI_TARGET_SET_PARAM(ta_, param_id_, value_) \
    do {                                                            \
        if (param_id < 0)                                           \
            TEST_FAIL("Invalid parameter name used");               \
                                                                    \
        CHECK_RC(tapi_iscsi_target_set_parameter((ta_), (param_id_),\
                 (value_)));                                        \
    } while (0)

#define ISCSI_TARGET_SET_PARAM_BY_NAME(_ta_, _param_name_, _value_) \
    do {                                                            \
        int param_id = tapi_iscsi_get_param_map(_param_name_);      \
                                                                    \
        if (param_id < 0)                                           \
            TEST_FAIL("Invalid parameter name used");               \
                                                                    \
        CHECK_RC(tapi_iscsi_target_set_parameter((_ta_), param_id,  \
                 (_value_)));                                       \
        RING("%s: Target: set %s to %s",                            \
             (_ta_), (_param_name_), (_value_));                    \
    } while (0)

/**
 * Macro configures the Initiator. The configured parameter
 * will be advertized by the Initiator during the Login State.
 *
 * @param ta_         Agent on which the Initiator is configured
 * @param target_id_  ID of the Target for which the parameter is configured
 * @param param_name_ Name of the parameter to configure
 * @param value_      New value of the parameter (in string form)
 */
#define ISCSI_INITIATOR_SET_ADVERTIZE(_ta_, _target_id_, _cid_, \
                                      _param_name_, _value_)            \
    do {                                                                \
        int param_id = tapi_iscsi_get_param_map(_param_name_);          \
                                                                        \
        if (param_id < 0)                                               \
            TEST_FAIL("Invalid parameter name used");                   \
                                                                        \
        CHECK_RC(tapi_iscsi_initiator_set_parameter((_ta_),             \
                                                    (_target_id_),      \
                                                    (_cid_),            \
                                                    param_id,           \
                                                    (_value_),          \
                                                    TRUE));             \
        RING("%s: Initiator: set %s for session ID %d, CID=%d to %s, "  \
             "with advertizing",                                        \
             (_ta_), (_param_name_),                                    \
             (_target_id_), (_cid_), (_value_));                        \
    } while (0)

/**
 * Macro configures the Initiator. The configured parameter
 * will NOT be advertized by the Initiator during the Login State.
 *
 * @param ta_         Agent on which the Initiator is configured
 * @param target_id_  ID of the Target for which the parameter is configured
 * @param param_name_ Name of the parameter to configure
 * @param value_      New value of the parameter (in string form)
 */
#define ISCSI_INITIATOR_SET_NOT_ADVERTIZE(_ta_, _target_id_, _cid_, \
                                          _param_name_, _value_)        \
    do {                                                                \
        int param_id = tapi_iscsi_get_param_map(_param_name_);          \
                                                                        \
        if (param_id < 0)                                               \
            TEST_FAIL("Invalid parameter name used");                   \
                                                                        \
        CHECK_RC(tapi_iscsi_initiator_set_parameter((_ta_),             \
                                                    (_target_id_),      \
                                                    (_cid_),            \
                                                    param_id,           \
                                                    (_value_),          \
                                                    FALSE));            \
        RING("%s: Initiator: set %s for session ID %d, CID=%d to %s, "  \
             "without advertizing",                                     \
             (_ta_), (_param_name_),                                    \
             (_target_id_), (_cid_), (_value_));                        \
    } while (0)



#define PDU_CONTAINS_KEY_VALUE_PAIR(segment_data_, \
                                    key_name_,     \
                                   key_value_)    \
    (!tapi_iscsi_find_key_and_value(segment_data_, (char *)key_name_, 1, \
                                    key_value_))
#define PDU_CONTAINS_KEY(segment_data_, key_name_) \
    (tapi_iscsi_get_key_index_by_name(segment_data_, key_name_) !=  \
     TAPI_ISCSI_KEY_INVALID)


/**
 * Create 'iscsi' target CSAP.
 *
 * Run iSCSI UNH Target Rx thread and create a socket pair to
 * communicate with it.
 *
 * @param ta_name       Test Agent name
 * @param hdr_dig       Header digests used by iSCSI protocol
 * @param data_dig      Data digests used by iSCSI protocol
 * @param csap          Location for handle of new CSAP
 *
 * @return Status code of the operation.
 */
extern te_errno tapi_iscsi_tgt_csap_create(const char        *ta_name,
                                           iscsi_digest_type  hdr_dig,
                                           iscsi_digest_type  data_dig,
                                           csap_handle_t     *csap);

/**
 * Create 'iscsi' initiator CSAP.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID to be used to wait connection on
 *                      @a listen_csap
 * @param listen_csap   TCP listening CSAP, on which TCP connection
 *                      from the Initiator sould be received
 * @param timeout       Timeout to wait connection
 * @param hdr_dig       Header digests used by iSCSI protocol
 * @param data_dig      Data digests used by iSCSI protocol
 * @param csap          Location for handle of new CSAP
 *
 * @return Status code of the operation.
 */
extern te_errno tapi_iscsi_ini_csap_create(const char        *ta_name,
                                           int                sid,
                                           csap_handle_t      listen_csap,
                                           int                timeout,
                                           iscsi_digest_type  hdr_dig,
                                           iscsi_digest_type  data_dig,
                                           csap_handle_t     *csap);

/**
 * Create 'iscsi' CSAP over connectєd TCP socket on TA.
 *
 * @param ta_name       Test Agent name
 * @param socket        File descriptor of TCP socket on TA 
 * @param hdr_dig       Header digests used by iSCSI protocol
 * @param data_dig      Data digests used by iSCSI protocol
 * @param csap          Location for handle of new CSAP
 *
 * @return Status code of the operation.
 */
extern te_errno tapi_iscsi_sock_csap_create(const char        *ta_name,
                                            int                socket,
                                            iscsi_digest_type  hdr_dig,
                                            iscsi_digest_type  data_dig,
                                            csap_handle_t     *csap);


/**
 * Receive one message via iSCSI CSAP.
 * 
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param csap          Identifier of CSAP
 * @param timeout       Timeout of operation in milliseconds
 * @param forward_csap  Identifier of CSAP, to which received
 *                      data should be forwarded, may be
 *                      CSAP_INVALID_HANDLE
 * @param params        Location for iSCSI current params (OUT)
 * @param buffer        Location for received data (OUT)
 * @param length        Length of buffer / received data (IN/OUT)
 * 
 * @return Zero on success or error code.
 */
extern int tapi_iscsi_recv_pkt(const char            *ta_name,
                               int                    sid, 
                               csap_handle_t          csap,
                               int                    timeout,
                               csap_handle_t          forward_csap,
                               iscsi_target_params_t *params,
                               uint8_t               *buffer,
                               size_t                *length);

/**
 * Send one message via iSCSI CSAP.
 * 
 * @param ta_name       test Agent name
 * @param sid           RCF SID
 * @param csap          identifier of CSAP
 * @param params        iSCSI new params
 * @param buffer        data to be sent
 * @param length        length of buffer
 * 
 * @return Zero on success or error code.
 */
extern int tapi_iscsi_send_pkt(const char            *ta_name,
                               int                    sid, 
                               csap_handle_t          csap,
                               iscsi_target_params_t *params,
                               uint8_t               *buffer,
                               size_t                 length);

/**
 * Send LAST message via iSCSI CSAP, and try to ensure sending
 * FIN in the last TCP push.
 * Note: use only with network TCP connection, not AF_LOCAL.
 * 
 * @param ta_name       test Agent name
 * @param sid           RCF SID
 * @param csap          identifier of CSAP
 * @param params        iSCSI new params
 * @param buffer        data to be sent
 * @param length        length of buffer
 * 
 * @return Zero on success or error code.
 */
extern int tapi_iscsi_send_pkt_last(const char   *ta_name,
                                    int           sid, 
                                    csap_handle_t csap,
                                    uint8_t      *buffer,
                                    size_t        length);

/**
 * Receive all data which currently are waiting for receive in 
 * specified iSCSI CSAP and forward them into another CSAP, without 
 * passing via RCF to test. 
 *
 * @param ta            TA name
 * @param sid           RCF session id
 * @param csap_rcv      identifier of recieve CSAP
 * @param csap_fwd      identifier of CSAP which should obtain data
 * @param timeout       timeout to wait data, in milliseconds
 * @param forwarded     number of forwarded PDUs (OUT)
 *
 * @return status code
 */
extern int tapi_iscsi_forward_all(const char *ta_name, int session,
                                  csap_handle_t csap_rcv,
                                  csap_handle_t csap_fwd,
                                  unsigned int timeout, int *forwarded);



/**
 * Pass all iSCSI PDUs from one iSCSI CSAP to another and reverse, 
 * until in both directions silence will be established. 
 *
 * @param ta            TA name
 * @param sid           RCF session id
 * @param csap_a        identifier of one side CSAP
 * @param csap_b        identifier of another side CSAP
 * @param timeout       timeout to wait data, in milliseconds
 *
 * @return status code
 */
extern int tapi_iscsi_exchange_until_silent(const char *ta, int session, 
                                            csap_handle_t csap_a,
                                            csap_handle_t csap_b,
                                            unsigned int timeout);
#define TAPI_ISCSI_KEY_INVALID     -1

/* To read iSCSI PDU Segment Data */

/**
 * Get number of keys in iSCSI PDU Segment Data.
 *
 * @param segment_data iSCSI PDU Segment Data in asn format
 * 
 * @return number of keys ot -1 if error occured.
 */ 
extern int tapi_iscsi_get_key_num(iscsi_segment_data segment_data);

/**
 * Get key name from iSCSI PDU Segment Data.
 *
 * @param segment_data iSCSI PDU Segment Data in asn format
 * @param key_index    key index in iSCSI PDU Segment Data
 *
 * @return key name or NULL if error occured.
 */ 
extern char * tapi_iscsi_get_key_name(iscsi_segment_data segment_data, 
                                      int key_index);

/**
 * Get key index in iSCSI PDU Segment Data by key name.
 *
 * @param segment_data iSCSI PDU Segment Data in asn format
 * @param name         key name
 *
 * @return key index or TAPI_ISCSI_KEY_INVALID if error occured.
 */ 
extern int tapi_iscsi_get_key_index_by_name(
               iscsi_segment_data segment_data, 
               char *name);

/**
 * Get values of key from iSCSI PDU Segment Data.
 *
 * @param segment_data  iSCSI PDU Segment Data in asn format 
 * @param key_index     key index
 *
 * @return key values in asn format or NULL if error occured.
 */ 
extern iscsi_key_values tapi_iscsi_get_key_values(
                            iscsi_segment_data segment_data,
                            int key_index);

/**
 * Get number of key values.
 *
 * @param values key values in asn format
 *
 * @return number of values or -1 if error occured.
 */ 
extern int tapi_iscsi_get_key_values_num(iscsi_key_values values);

/**
 * Get key value from list of values.
 *
 * @param values            key values list in asn format
 * @param key_value_index   value index in the list
 * @param value             location for value (OUT)
 *
 * @return 0 or error code
 */ 
extern int tapi_iscsi_get_key_value(iscsi_key_values values, 
                                    int key_value_index, 
                                    char **value);

/* To modify iSCSI PDU Segment Data */

/**
 * Add a new key into an iSCSI PDU Segment Data.
 *
 * @param segment_data    iSCSI PDU Segment Data in asn format
 * @param name            key name
 * @param key_index       index of key in iSCSI PDU Segment Data,
 *                        if key_index is TAPI_ISCSI_KEY_INVALID
 *                        then key is to be inserted to the end
 *                        of key list
 *
 * @return key index or -1 if error occured.
 */ 
extern int tapi_iscsi_add_new_key(iscsi_segment_data segment_data, 
                                  char *name, int key_index);

/**
 * Create list of key values in asn format
 *
 * @param num      number of values
 * @param ...      list of pairs (type, value), where type
 *                 is iscsi_key_value_type_xxx, and value is
 *                 an appropriate type value
 *
 * @return list of values or NULL if error occured.                
 */ 
extern iscsi_key_values tapi_iscsi_key_values_create(int num, ...);

/**
 * Assign a list of values to a key in iSCSI PDU Segment Data
 *
 * @param segment_data  iSCSI PDU Segment Data in asn format
 * @param key_index     key index
 * @param values        list of key values in asn format
 *
 * @return 0 or error code.
 */ 
extern int tapi_iscsi_set_key_values(iscsi_segment_data segment_data,
                                     int key_index, 
                                     iscsi_key_values values);

/**
 * Free list of key values
 *
 * @param values list of key values.
 */ 
extern void tapi_iscsi_free_key_values(iscsi_key_values values);

/**
 * Delete a key from iSCSI PDU Segment Data in asn format
 *
 * @param segment_data    iSCSI PDU Segment Data in asn format
 * @param key_index       key index
 *
 * @return 0 or error code.
 */ 
extern int tapi_iscsi_delete_key(iscsi_segment_data segment_data, 
                                 int key_index);

/**
 * Create an iSCSI PDU Segment Data in asn format.
 *
 * @param num           number of keys in iSCSI PDU Segment Data
 * @param ...           list of keys names
 *
 * @return              iSCSI PDU Segment Data in  asn format
 *                      or NULL if error occured
 */                      
extern iscsi_segment_data tapi_iscsi_keys_create(int num, ...);

/**
 * Check that iSCSI PDU SEgment Data contains
 * a key with a given name and its list of values
 * contains list of values followed by num
 *
 * @param segment_data   iSCSI PDU Segment Data in asn format
 * @param key_name       Key name
 * @param num            number of key values to be checked
 * @param ...            List of key values, they should be
 *                       contained in key values list
 * @return Status code                      
 */ 
extern int tapi_iscsi_find_key_and_value(
    iscsi_segment_data segment_data,
    const char *key_name, int num, ...);

/**
 * Find the key and return its value
 *
 * @param segment_data    iSCSI PDU Segment Data in asn format
 * @param key_name        the name of key (according to RFC3720)
 * @param buf             Location to return values of the key
 * @param buf_len         The lenght of the buf
 *
 * @return 0 or error code.
 */
extern int
tapi_iscsi_return_key_value(iscsi_segment_data segment_data,
                            const char *key_name,
                            const char *buf, int buf_len);

/**
 * Type of action upon key values list.
 */ 
typedef enum {
    tapi_iscsi_insert_key_values, /**< Add new key values to key 
                                       values list */
    tapi_iscsi_replace_key_values,/**< Remove all key values, 
                                       add new key values to key 
                                       values list */
    tapi_iscsi_remove_key_values, /**< Remove given key values, if
                                       any, from key values list */
} tapi_iscsi_change_key_val_type;    

/**
 * Change key values list according action given (See comments to
 * tapi_iscsi_change_key_val_type)
 *
 * @param segment_data    iSCSI PDU Segment Data in asn format
 * @param key_name        Key name
 * @param change          Action to take on key values list
 * @param num             Number of given key values
 * @param ...             List of key values
 *
 * @return Status code
 */ 
extern int tapi_iscsi_change_key_values(
    iscsi_segment_data segment_data,
    char *key_name, 
    tapi_iscsi_change_key_val_type change,
    int num, ...);

/**
 * Free an iSCSI PDU Segment Data.
 */ 
extern void tapi_iscsi_keys_data_free(iscsi_segment_data);

typedef enum {
    ISCSI_PARAM_HEADER_DIGEST,                          /**< Local */
    ISCSI_PARAM_DATA_DIGEST,                            /**< Local */
    ISCSI_PARAM_MAX_CONNECTIONS,                        /**< Local */
    ISCSI_PARAM_SEND_TARGETS,                           /**< Global */
    ISCSI_PARAM_TARGET_NAME,                            /**< Local */
    ISCSI_PARAM_INITIATOR_NAME,                         /**< Global */
    ISCSI_PARAM_TARGET_ALIAS,                           /**< Local */
    ISCSI_PARAM_INITIATOR_ALIAS,                        /**< Global */
    ISCSI_PARAM_TARGET_ADDRESS,                         /**< Local */
    ISCSI_PARAM_TARGET_PORT,                            /**< Local */
    ISCSI_PARAM_INITIAL_R2T,                            /**< Local */
    ISCSI_PARAM_IMMEDIATE_DATA,                         /**< Local */
    ISCSI_PARAM_MAX_RECV_DATA_SEGMENT_LENGTH            /**< Local */,
    ISCSI_PARAM_MAX_BURST_LENGTH,                       /**< Local */
    ISCSI_PARAM_FIRST_BURST_LENGTH,                     /**< Local */
    ISCSI_PARAM_DEFAULT_TIME2WAIT,                      /**< Local */
    ISCSI_PARAM_DEFAULT_TIME2RETAIN,                    /**< Local */
    ISCSI_PARAM_MAX_OUTSTANDING_R2T,                    /**< Local */
    ISCSI_PARAM_DATA_PDU_IN_ORDER,                      /**< Local */
    ISCSI_PARAM_DATA_SEQUENCE_IN_ORDER,                 /**< Local */
    ISCSI_PARAM_ERROR_RECOVERY_LEVEL,                   /**< Local */
    ISCSI_PARAM_SESSION_TYPE,                           /**< Local */
    ISCSI_PARAM_OF_MARKER,
    ISCSI_PARAM_IF_MARKER,
    ISCSI_PARAM_OF_MARKER_INT,
    ISCSI_PARAM_IF_MARKER_INT,
    ISCSI_PARAM_LAST_OPERATIONAL,        /* just a delimiter */
/* TODO: Global and Local parameters */
    ISCSI_PARAM_LOCAL_SECRET,                           /**< Global */
    ISCSI_PARAM_LOCAL_NAME,                             /**< Local */
    ISCSI_PARAM_PEER_SECRET,                            /**< Local */
    ISCSI_PARAM_PEER_NAME,                              /**< Global */
    ISCSI_PARAM_CHANLLENGE_LENGTH,                      /**< Global */
    ISCSI_PARAM_ENCODING_FORMAT,                        /**< Global */
    ISCSI_PARAM_TGT_AUTH_REQ,                           /**< Global */
    ISCSI_PARAM_SECURITY_NEGOTIATION_PHASE              /**< Global */
} tapi_iscsi_parameter;

typedef enum {
    INT,
    STRING
} tapi_iscsi_parameter_type;

/* Target configuration */
extern int tapi_iscsi_target_set_parameter(const char *ta, 
                                           tapi_iscsi_parameter param, 
                                           const char *value);

extern int tapi_iscsi_target_customize(const char *ta,
                                       int id,
                                       const char *key,
                                       const char *value);

extern int tapi_iscsi_target_customize_intval(const char *ta, int id, 
                                              const char *key, int value);

extern int tapi_iscsi_target_cause_logout(const char *ta, int id, 
                                          int timeout);


extern int tapi_iscsi_target_cause_renegotiate(const char *ta, int id, 
                                               int timeout);

extern int tapi_iscsi_target_will_drop(const char *ta, int id, 
                                       te_bool drop_all,
                                       int time2wait, int time2retain);


typedef int iscsi_target_id;
typedef int iscsi_cid;

/**
 * Function configures parameter not to be advertized.
 * By default the parameter is not advertized. When
 * "SET" operation is called on it the Initiator is
 * told to advertize the parameter.
 *
 * @param ta         Name of the TA on which the Initiator is configured
 * @param target_id  ID of the Taraget
 * @param param      Parameter to configure
 */
extern int tapi_iscsi_initiator_advertize_set(const char *ta,
                                              iscsi_target_id target_id,
                                              iscsi_cid cid,
                                              tapi_iscsi_parameter param,
                                              te_bool advertize);
/**
 * Function configures parameters of the Initiator.
 *
 * @param ta         Name of the TA on which the Initiator is configured
 * @param target_id  ID of the target
 * @param param      Parameter to configure
 * @param value      New value added
 *
 * @return return of the cfg_set_instance_fmt
 */
extern int tapi_iscsi_initiator_set_parameter(const char *ta,
                                              iscsi_target_id target_id,
                                              iscsi_cid cid,
                                              tapi_iscsi_parameter param,
                                              const char *value,
                                              te_bool advertize);

#ifdef ISCSI_INITIATOR_GET_SUPPORT
/**
 * Function configures  parameters of the Initiator.
 *
 * @param ta         Name of the TA on which the Initiator is configured
 * @param target_id  ID of the target
 * @param param      Parameter to configure
 * @param value      New value added
 *
 * @return return of the cfg_set_instance_fmt
 */
extern int tapi_iscsi_initiator_get_parameter(const char *ta,
                                              iscsi_target_id target_id,
                                              tapi_iscsi_parameter param,
                                              const char *value);
#endif

/**
 * Function adds target to the Initiator targets list.
 *
 * @param ta           Name of the TA on which the Initiator is configured
 * @param target_addr  Address of the target
 *
 * @return id of the target or -errno
 */
extern iscsi_target_id tapi_iscsi_initiator_add_target(const char *ta,
                                      const struct sockaddr *target_addr);

/**
 * Function adds target to the Initiator targets list.
 *
 * @param ta           Name of the TA on which the Initiator is configured
 * @param tgt_id       ID of the target to destroy
 *
 * @return id of the target or -errno
 */
extern int tapi_iscsi_initiator_del_target(const char *ta,
                                      iscsi_target_id tgt_id);

/**
 * Function adds the connection. It should be called before the connection
 * can be configured.
 *
 * @param ta         Name of the TA on which the Initiator is placed
 * @param tgt_id     Id of the Target to establish connection with
 *
 * @return CID of the newly created connection
 */
extern iscsi_cid tapi_iscsi_initiator_conn_add(const char *ta,
                                               iscsi_target_id tgt_id);

/**
 * Function tries to establish the connection with the given cid
 * between the target and the initiator. Before calling this function
 * the tapi_iscsi_initiator_conn_add() function should be called.
 * 
 * @param ta         Name of the TA on which the Initiator is placed
 * @param tgt_id     Id of the Target to establish connection with
 * @param cid        ID of the connection to establish
 *
 * @return CID of the newly created connection
 */
extern iscsi_cid tapi_iscsi_initiator_conn_establish(const char *ta,
                                                     iscsi_target_id tgt_id,
                                                     iscsi_cid cid);

/**
 * Function tries to delete connection from the Initiator.
 *
 * @param ta         Name of the TA on which the Initiator is placed
 * @param tgt_id     Id of the Target the connection with which should be
 *                   deleted
 * @param cid        ID of the connection to delete
 *
 * @return           errno
 */
extern int tapi_iscsi_initiator_conn_del(const char *ta,
                                         iscsi_target_id tgt_id,
                                         iscsi_cid cid);

/**
 * Function stops the connection with the given cid between the initiator
 * and the target.
 *
 * @param ta         Name of the TA on which the Initiator is placed
 * @param tgt_id     Id of the Target the connection with which should be
 *                   deleted
 * @param cid        ID of the connection to delete
 *
 * @return           errno
 */
extern int tapi_iscsi_initiator_conn_down(const char *ta,
                                          iscsi_target_id tgt_id,
                                          iscsi_cid cid);


/**
 * Function converts string representation of iSCSI parameter to 
 * corresponding enum value of tapi_iscsi_parameter type.
 *
 * @param param      Name of the TA on which the Initiator is placed
 *
 * @return           iSCSI parameter index or -1, if fails
 */
extern tapi_iscsi_parameter tapi_iscsi_get_param_map(const char *param);

/**
 * Find specified key name in segment data and determine number of
 * key values for its name. 
 *
 * @param segment_data    iSCSI PDU Segment Data in asn format
 * @param key_name        the name of key (according to RFC3720)
 * @param key_array       location for pointer to found ASN value
 *                        to key values array (OUT)
 *
 * @return number of key values for passed key name, zero if none, 
 *         or -1 if error encountered. 
 */
extern int tapi_iscsi_find_key_values(iscsi_segment_data segment_data,
                                      const char *key_name,
                                      iscsi_key_values *key_array);

/**
 * Read key value by index of value in array.
 *
 * @param val_array       array with values for some key, ASN Value
 * @param val_index       index of key value to be read
 * @param buf             location for key value (OUT)
 * @param buf_len         length of buffer/read data (IN/OUT)
 *
 * @return status of operation
 */
extern int tapi_iscsi_key_value_read(iscsi_key_values val_array,
                                     int val_index, char *buf, 
                                     size_t *buf_len);
/**
 * Write key value by index of value in array.
 *
 * @param val_array       array with values for some key, ASN Value
 * @param val_index       index of key value to be read
 * @param string          new string key value, NULL for remove
 *
 * @return status of operation
 */
extern int tapi_iscsi_key_value_write(iscsi_key_values val_array,
                                      int val_index, const char *string);


/** The following functions are DEPRECATED!!! 
    They will be removed as soon as all the tests use the new API
*/

static inline int 
tapi_iscsi_set_local_secret(const char *ta, const char *secret)
{
    return tapi_iscsi_target_set_parameter(ta, ISCSI_PARAM_LOCAL_SECRET,
                                           secret);
}


static inline int 
tapi_iscsi_set_local_name(const char *ta, const char *name)
{
    return tapi_iscsi_target_set_parameter(ta, ISCSI_PARAM_LOCAL_NAME,
                                           name);
}


static inline int 
tapi_iscsi_set_peer_secret(const char *ta, const char *secret)
{
    return tapi_iscsi_target_set_parameter(ta, ISCSI_PARAM_PEER_SECRET,
                                           secret);
}


static inline int 
tapi_iscsi_set_peer_name(const char *ta, const char *name)
{
    return tapi_iscsi_target_set_parameter(ta, ISCSI_PARAM_PEER_NAME,
                                           name);
}


static inline int 
tapi_iscsi_set_challenge_length(const char *ta, int len)
{
    char buf[8];
    sprintf(buf, "%d", len);
    return tapi_iscsi_target_set_parameter(ta, 
                                           ISCSI_PARAM_CHANLLENGE_LENGTH,
                                           buf);    
}


static inline int
tapi_iscsi_set_encoding_format(const char *ta, int fmt)
{
    return tapi_iscsi_target_set_parameter(ta,
                                           ISCSI_PARAM_ENCODING_FORMAT,
                                           fmt ? "1" : "0");
}

static inline int
tapi_iscsi_set_tgt_auth_req(const char *ta, int tgt_auth)
{
    return tapi_iscsi_target_set_parameter(ta,
                                           ISCSI_PARAM_TGT_AUTH_REQ,
                                           tgt_auth ? "1" : "0");
}


static inline int 
tapi_iscsi_set_security_negotiations_phase(const char *ta,
                                               int use)
{
    return tapi_iscsi_target_set_parameter \
        (ta, ISCSI_PARAM_SECURITY_NEGOTIATION_PHASE, use ? "1" : "0");
}

extern iscsi_digest_type iscsi_digest_str2enum(const char *digest_type);

extern char* iscsi_digest_enum2str(iscsi_digest_type digest_type);


/*
 * Informs the iSCSI target that a new test is being run.
 * Note: this function should not be necessary in most situations,
 * as any change of target parameters will do the job
 *
 * @param ta    Test Agent name
 *
 * @return Status code
 */
extern int tapi_iscsi_target_inform_new_test(const char *ta);

/*** Functions for data transfer between Target and Initiator ***/

/**
 * Mount a target backing store.
 * Note: this will work only if the target is using 
 * a file-based backing store.
 * 
 * @param ta    Test Agent name
 * 
 * @return Status code
 */
extern te_errno tapi_iscsi_target_mount(const char *ta);

/**
 * Unmount a target backing store.
 * 
 * @param ta    Test Agent name
 * 
 * @return Status code
 */
extern te_errno tapi_iscsi_target_unmount(const char *ta);

/**
 * Write data to a target's backing store filesystem 
 * to a file with a given name.
 *
 * Note: This function should be called only after 
 * tapi_iscsi_target_mount().
 * 
 * @param ta            Test Agent name
 * @param fname         A filename to write
 * @param data          Buffer to write
 * @param length        Length of data to write
 * @param multiply      Write the buffer that many times
 * 
 * @return Status code
 *
 * @sa tapi_iscsi_initiator_raw_read
 */
extern te_errno tapi_iscsi_target_file_write(const char *ta, 
                                             const char *fname,
                                             const void *data,
                                             size_t length,
                                             size_t multiply);

/**
 * Read data from a target's backing store filesystem 
 * from a file with a given name.
 *
 * Note: This function should be called only after 
 * tapi_iscsi_target_mount(). 
 * 
 * @param ta            Test Agent name
 * @param fname         A filename to read from
 * @param data          Buffer to store data
 * @param length        Length of buffer
 * 
 * @return Status code
 *
 * @sa tapi_iscsi_initiator_raw_read
 */
extern te_errno tapi_iscsi_target_file_read(const char *ta, 
                                            const char *fname,
                                            void *data, size_t length);


/**
 * Write raw data to a target's backing store.
 *
 * Note: This function is NOT to be used in conjunction with
 * tapi_iscsi_target_mount().
 *
 * @param ta            Test Agent name
 * @param offset        A position to write
 * @param data          Buffer to write
 * @param length        Length of data to write
 * @param multiply      Write the buffer that many times
 * 
 * @return Status code
 *
 * @sa tapi_iscsi_initiator_raw_read
 */
extern te_errno tapi_iscsi_target_raw_write(const char *ta, off_t offset,
                                            const void *data,
                                            size_t length,
                                            size_t multiply);

/**
 * Reads data from a target's device at a given position
 *
 * Note: This function is NOT to be used in conjunction with
 * tapi_iscsi_target_mount().
 *
 * @param ta            Test Agent name
 * @param offset        A position to read from
 * @param data          Buffer to store data
 * @param length        Length of data  
 * 
 * @return Status code
 *
 * @sa tapi_iscsi_initiator_raw_write
 */
extern te_errno tapi_iscsi_target_raw_read(const char *ta, off_t offset,
                                           void *data, size_t length);


/*** Functions to initiate I/O on an iSCSI device at TA 
 *
 * This functions need to be asynchronous because they will
 * cause an initiator to generate traffic that is captured by
 * a test.
 * 
 * So, the basic action sequence in the test is as follows:
 * -# create a I/O handler with tapi_iscsi_io_prepare()
 * -# issues all the necessary I/O tasks, remembering
 *    the task id of the last task
 * -# proceed with passing iSCSI packets to/from the target,
 *    performing the necessary checks etc, until 
 *    tapi_iscsi_io_is_complete() returns TRUE
 * -# destroy the handler with tapi_iscsi_io_finish()
 * 
 * Note: the processing thread will send a signal to the main thread,
 * so the user should be prepared to handle "interrupted system call"
 * condition, or use tapi_iscsi_io_block_signal()
 */
typedef struct iscsi_io_handle_t iscsi_io_handle_t;
typedef unsigned iscsi_io_taskid;

/**
 * Create a new asynchronous I/O handler 
 * 
 * @param ta            Test Agent name
 * @param id            target id to connect
 * @param use_signal    Should a signal be sent after
 *                      an I/O operation is complete
 * @param use_fs        TRUE if using a filesystem on
 *                      an ISCSI device
 * @param bufsize       Buffer size for file copying operations,
 *                      that is, tapi_iscsi_initiator_read_file() and
 *                      tapi_iscsi_initiator_write_file(). 
 * @param ioh           A pointer to resulting handler (OUT)
 * 
 * @return Status code
 */
extern te_errno tapi_iscsi_io_prepare(const char *ta, 
                                      iscsi_target_id id, 
                                      te_bool use_signal,
                                      te_bool use_fs,
                                      size_t  bufsize,
                                      iscsi_io_handle_t **ioh);


/**
 * Resets the I/O handler task queue
 * 
 * @param ioh   I/O handler
 * 
 * @return Status code
 * @retval TE_EINPROGRESS   There are incomplete tasks pending
 */
extern te_errno tapi_iscsi_io_reset(iscsi_io_handle_t *ioh);


/**
 * Destroy an asynchronous I/O handler
 * 
 * @param ioh   I/O handler
 * 
 * @return Status code
 */
extern te_errno tapi_iscsi_io_finish(iscsi_io_handle_t *ioh);


/**
 * 
 * 
 * @param ioh I/O handler
 * @param enable 
 *
 * @return Previous state
 */
extern te_bool tapi_iscsi_io_enable_signal(iscsi_io_handle_t *ioh, 
                                           te_bool enable);

/**
 *  Gets the status code of a task of a I/O handler
 * 
 * @param ioh           I/O handle
 * @param taskid        Task ID
 * 
 * @return Status code of a given task
 * @retval TE_EINPROGRESS if the task has not completed
 */
extern te_errno tapi_iscsi_io_get_status(iscsi_io_handle_t *ioh, 
                                         iscsi_io_taskid taskid);


/**
 *  Checks whether a task has completed
 * 
 * @param ioh           I/O handle
 * @param taskid        Task ID
 * 
 * @return TRUE if the task has completed
 */
extern te_bool tapi_iscsi_io_is_complete(iscsi_io_handle_t *ioh, 
                                         iscsi_io_taskid taskid);


/**
 * Checks whether an iSCSI device is present and ready
 *
 * @param ta  Test Agent
 * @param id  Target id
 *
 * @return TRUE if the device is ready
 */
extern te_bool tapi_iscsi_initiator_is_device_ready(const char *ta,
                                                    iscsi_target_id id);

/**
 * Request mounting an iSCSI device on the TA, if the I/O handler 
 * has been created with "use_fs" == TRUE. Otherwise a no-op.
 *
 * Note: you need to call tapi_iscsi_target_mount() before this
 * function to create a filesystem on a target backing store.
 * 
 * @param ioh           I/O handler
 * @param taskid        A pointer to store a task ID or NULL (OUT)
 * 
 * @return Status code
 */
extern te_errno tapi_iscsi_initiator_mount(iscsi_io_handle_t *ioh, 
                                           iscsi_io_taskid *taskid);

/**
 * Request unmounting an iSCSI device on the TA.
 * No-op if "use_fs" is not TRUE for the I/O handler. 
 *
 * @param ioh           I/O handler
 * @param taskid        A pointer to store a task ID or NULL (OUT)
 * 
 * @return Status code
 */
extern te_errno tapi_iscsi_initiator_unmount(iscsi_io_handle_t *ioh,
                                             iscsi_io_taskid *taskid);


/**
 * Opens a file on an iSCSI filesystem
 *
 * @param ioh           I/O handler
 * @param taskid        A pointer to store a task ID or NULL (OUT)
 * @param fname         Filename
 * @param mode          Standard UNIX file open mode
 *
 * @return Status code
 */
extern te_errno tapi_iscsi_initiator_open(iscsi_io_handle_t *ioh,
                                          iscsi_io_taskid *taskid,
                                          const char *fname,
                                          int mode);


/**
 * Closes a file on an iSCSI filesystem previously opened by
 * tapi_iscsi_initiator_open().
 *
 * @param ioh           I/O handler
 * @param taskid        A pointer to store a task ID or NULL (OUT)
 *
 * @return Status code
 */
extern te_errno tapi_iscsi_initiator_close(iscsi_io_handle_t *ioh,
                                           iscsi_io_taskid *taskid);


/**
 * Request a seek operation on an iSCSI device.
 *
 * @param ioh           I/O handler
 * @param taskid        A pointer to store a task ID or NULL (OUT)
 * @param pos           Positiion to seek to
 *
 * @return Status code
 */
extern te_errno tapi_iscsi_initiator_seek(iscsi_io_handle_t *ioh,
                                          iscsi_io_taskid *taskid,
                                          off_t pos);

/**
 * Request a write operation on an iSCSI device.
 *
 * @param ioh           I/O handler
 * @param taskid        A pointer to store a task ID or NULL (OUT)
 * @param data          A pointer to a buffer with data
 * @param length        Length of data to write
 *
 * @return Status code
 */
extern te_errno tapi_iscsi_initiator_write(iscsi_io_handle_t *ioh,
                                           iscsi_io_taskid *taskid,
                                           void *data,
                                           size_t length);

/**
 * Request a read operation on an iSCSI device.
 *
 * @param ioh           I/O handler
 * @param taskid        A pointer to store a task ID or NULL (OUT)
 * @param data          A pointer to a buffer to store data
 * @param length        Length of the buffer
 *
 * @return Status code
 */
extern te_errno tapi_iscsi_initiator_read(iscsi_io_handle_t *ioh,
                                          iscsi_io_taskid *taskid,
                                          void *data,
                                          size_t length);

/**
 * Request a write operation on an iSCSI device using data from a file.
 * The copying is done in chunks of size 'bufsize' passed 
 * to iscsi_io_prepare().
 *
 * @param ioh           I/O handler
 * @param data          A pointer to a zero-terminated string to write
 * @param taskid        A pointer to store a task ID or NULL (OUT)
 * @param fname         Name of a file with data
 *
 * @return Status code
 */
extern te_errno tapi_iscsi_initiator_write_file(iscsi_io_handle_t *ioh,
                                                iscsi_io_taskid *taskid,
                                                const char *fname);

/**
 * Request a read operation on an iSCSI device putting data to a file.
 * The copying is done in chunks of size 'bufsize' passed 
 * to iscsi_io_prepare().
 *
 * @param ioh           I/O handler
 * @param data          A pointer to a zero-terminated string to verify
 * @param fname         Name of a file to store data
 *
 * @return Status code
 */
extern te_errno tapi_iscsi_initiator_read_file(iscsi_io_handle_t *ioh,
                                               iscsi_io_taskid *taskid,
                                               const char *fname);


#endif /* !__TE_TAPI_ISCSI_H__ */
