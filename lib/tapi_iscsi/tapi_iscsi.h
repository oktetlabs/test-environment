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

/**
 * Free an iSCSI PDU Segment Data.
 */ 
extern void tapi_iscsi_keys_data_free(iscsi_segment_data);

/* Target configuration */
int tapi_iscsi_set_local_secret(const char *ta, const char *secret);

int tapi_iscsi_set_local_name(const char *ta, const char *name);

int tapi_iscsi_set_peer_secret(const char *ta, const char *secret);

int tapi_iscsi_set_peer_name(const char *ta, const char *name);

int tapi_iscsi_set_challenge_length(const char *ta, int len);

int tapi_iscsi_set_encoding_format(const char *ta, int fmt);

int tapi_iscsi_set_tgt_auth_req(const char *ta, int tgt_auth);

int tapi_iscsi_set_security_negotiations_phase(const char *ta,
                                               int use);

#endif /* !__TE_TAPI_ISCSI_H__ */
