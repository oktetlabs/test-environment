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

#include "te_stdint.h"
#include "tad_common.h"
#include "asn_usr.h"
#include "ndn_iscsi.h"

#define ISCSI_TARGET_SET_PARAM(ta_, param_id_, value_) \
    do {                                                            \
        if (param_id < 0)                                           \
            TEST_FAIL("Invalid parameter name used");               \
                                                                    \
        CHECK_RC(tapi_iscsi_target_set_parameter((ta_), (param_id_),\
                 (value_)));                                        \
    } while (0)

#define ISCSI_TARGET_SET_PARAM_BY_NAME(ta_, param_name_, value_) \
    do {                                                            \
        int param_id = tapi_iscsi_get_param_map(param_name_);       \
                                                                    \
        if (param_id < 0)                                           \
            TEST_FAIL("Invalid parameter name used");               \
                                                                    \
        CHECK_RC(tapi_iscsi_target_set_parameter((ta_), param_id,   \
                 (value_)));                                        \
    } while (0)

#define ISCSI_INITIATOR_SET_ADVERTIZE(ta_, target_id_, \
                                      param_name_, value_)              \
    do {                                                                \
        int param_id = tapi_iscsi_get_param_map(param_name_);           \
                                                                        \
        if (param_id < 0)                                               \
            TEST_FAIL("Invalid parameter name used");                   \
                                                                        \
        CHECK_RC(tapi_iscsi_initiator_set_parameter((ta_),              \
                                                    (target_id_),       \
                                                    param_id,           \
                                                    (value_)));         \
    } while (0)

#define ISCSI_INITIATOR_SET_NOT_ADVERTIZE(ta_, target_id_, \
                                          param_name_, value_)          \
    do {                                                                \
        int param_id = tapi_iscsi_get_param_map(param_name_);           \
                                                                        \
        if (param_id < 0)                                               \
            TEST_FAIL("Invalid parameter name used");                   \
                                                                        \
        CHECK_RC(tapi_iscsi_initiator_set_parameter((ta_),              \
                                                    (target_id_),       \
                                                    param_id,           \
                                                    (value_)));         \
                                                                        \
        CHECK_RC(tapi_iscsi_initiator_not_advertize((ta_),              \
                                                    (target_id_),       \
                                                    param_id));         \
    } while (0)



#define PDU_CONTAINS_KEY_VALUE_PAIR_STRING(segment_data_, \
                                           key_name_,     \
                                           key_value_)    \
    (!tapi_iscsi_find_key_and_value(segment_data_, (char *)key_name_, 1, \
                                    iscsi_key_value_type_string,         \
                                    key_value_))

#define PDU_CONTAINS_KEY_VALUE_PAIR_INT(segment_data_, \
                                        key_name_, key_value_) \
    (!tapi_iscsi_find_key_and_value(segment_data_, (char *)key_name_, 1,  \
                                    iscsi_key_value_type_int, key_value_))

#define PDU_CONTAINS_KEY_VALUE_PAIR_HEX(segment_data_, \
                                        key_name_, key_value_) \
    (!tapi_iscsi_find_key_and_value(segment_data_, (char *)key_name_, 1,  \
                                    iscsi_key_value_type_hex, key_value_))

#define PDU_CONTAINS_KEY(segment_data_, key_name_) \
    (tapi_iscsi_get_key_index_by_name(segment_data_, key_name_) !=  \
     TAPI_ISCSI_KEY_INVALID)

/**
 * Creates 'iscsi' server CSAP
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF SID
 * @param csap          location for handle of new CSAP
 *
 * @return  Status code of the operation
 */
extern int tapi_iscsi_csap_create(const char *ta_name, int sid, 
                                  csap_handle_t *csap);


/**
 * Receive one message from internal TA iSCSI target.
 * 
 * @param ta_name       test Agent name
 * @param sid           RCF SID
 * @param csap          identifier of CSAP
 * @param timeout       timeout of operation in milliseconds
 * @param csap          identifier of CSAP, to which received
 *                      data should be forwarded, may be
 *                      CSAP_INVALID_HANDLE
 * @param header_digest flag if there is HeaderDigest field
 * @param data_digest   flag if there is DataDigest field
 * @param params        location for iSCSI current params (OUT)
 * @param buffer        location for received data (OUT)
 * @param length        length of buffer / received data (IN/OUT)
 * 
 * @return Zero on success or error code.
 */
extern int tapi_iscsi_recv_pkt(const char *ta_name, int sid, 
                               csap_handle_t csap,
                               int timeout,
                               csap_handle_t forward,
                               te_bool header_digest,
                               te_bool data_digest,
                               iscsi_target_params_t *params,
                               uint8_t *buffer,
                               size_t  *length);

/**
 * Send one message to internal TA iSCSI target.
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
extern int tapi_iscsi_send_pkt(const char *ta_name, int sid, 
                               csap_handle_t csap,
                               iscsi_target_params_t *params,
                               uint8_t *buffer,
                               size_t  length);
/**
 * Receive one message from iSCSI TCP connection on TA.
 * 
 * @param ta_name       test Agent name
 * @param sid           RCF SID
 * @param csap          identifier of TCP CSAP
 * @param timeout       timeout of operation in milliseconds
 * @param csap          identifier of CSAP, to which received
 *                      data should be forwarded, may be
 *                      CSAP_INVALID_HANDLE
 * @param header_digest flag if there is HeaderDigest field
 * @param data_digest   flag if there is DataDigest field
 * @param params        location for iSCSI current params (OUT)
 * @param buffer        location for received data (OUT)
 * @param length        length of buffer / received data (IN/OUT)
 * 
 * @return Zero on success or error code.
 */
extern int tapi_iscsi_tcp_recv_pkt(const char *ta_name, int sid, 
                                   csap_handle_t csap,
                                   int timeout,
                                   csap_handle_t forward,
                                   te_bool header_digest,
                                   te_bool data_digest,
                                   uint8_t *buffer,
                                   size_t  *length);

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
 * Get type of key value.
 *
 * @param values           key values in asn format
 * @param key_value_index  index of key value
 *
 * @return type of key value or type_invalid if error occured.
 */ 
extern iscsi_key_value_type tapi_iscsi_get_key_value_type(
                                iscsi_key_values values, 
                                int key_value_index);

/**
 * Get string key value from list of values.
 *
 * @param values            key values list in asn format
 * @param key_value_index   value index in the list
 * @param str_value         location for string value (OUT)
 *
 * @return 0 or error code
 */ 
extern int tapi_iscsi_get_string_key_value(iscsi_key_values values, 
                                           int key_value_index, 
                                           char **str_value);

/**
 * Get integer key value from list of values.
 *
 * @param values            key values list in asn format
 * @param key_value_index   value index in the list
 * @param int_value         location for integer value (OUT)
 *
 * @return 0 or error code
 */ 
extern int tapi_iscsi_get_int_key_value(iscsi_key_values values, 
                                        int key_value_index, 
                                        int *int_value);

/* To modify iSCSI PDU Segment Data */

/**
 * Add a new key into an iSCSI PDU Segment Data.
 *
 * @param segment_data    iSCSI PDU Segment Data in asn format
 * @param name            key name
 * @param key_index       index of key in iSCSI PDU Segment Data
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

typedef enum {
    tapi_iscsi_insert_key_values,
    tapi_iscsi_replace_key_values,
    tapi_iscsi_remove_key_values,
} tapi_iscsi_change_key_val_type;    

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
    ISCSI_PARAM_MAX_RECV_DATA_SEGMENT_LENGHT            /**< Local */,
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
extern int tapi_iscsi_initiator_not_advertize(const char *ta,
                                              iscsi_target_id target_id,
                                              tapi_iscsi_parameter param);
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
                                              tapi_iscsi_parameter param,
                                              const char *value);

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
 * Function tries to establish connection with the Target.
 *
 * @param ta         Name of the TA on which the Initiator is placed
 * @param tgt_id     Id of the Target to establish connection with
 * @param cid        ID of the connection to establish
 *
 * @return CID of the newly created connection
 */
extern iscsi_cid tapi_iscsi_initiator_conn_add(const char *ta,
                                               iscsi_target_id tgt_id);

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
 * Read key value by name and index of value.
 *
 * @param segment_data    iSCSI PDU Segment Data in asn format
 * @param key_name        the name of key (according to RFC3720)
 * @param val_index       index of key value to be read
 * @param buf             location for key value (OUT)
 * @param buf_len         length of buffer/read data (IN/OUT)
 *
 * @return status of operation
 */
extern int tapi_iscsi_key_value_read(iscsi_key_values key_array,
                                     int val_index, char *buf, 
                                     size_t *buf_len);

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



#endif /* !__TE_TAPI_ISCSI_H__ */
