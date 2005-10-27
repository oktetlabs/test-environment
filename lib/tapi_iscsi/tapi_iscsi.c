/** @file
 * @brief Test API for TAD. ipstack CSAP
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


#include "te_config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <stdio.h>
#include <assert.h>

#include <netinet/in.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rcf_api.h"
#include "rcf_common.h"
#include "conf_api.h"

#define TE_LGR_USER "TAPI iSCSI"
#include "logger_api.h"

#include "tapi_iscsi.h"
#include "ndn_iscsi.h"

#include "tapi_tad.h"
#include "te_iscsi.h"
#include "tapi_tcp.h"

int
tapi_iscsi_csap_create(const char *ta_name, int sid, 
                       csap_handle_t *csap)
{
    return tapi_iscsi_sock_csap_create(ta_name, sid, 0, csap);
}


int
tapi_iscsi_sock_csap_create(const char *ta_name, int sid, 
                            int socket, csap_handle_t *csap)
{
    asn_value *csap_spec = NULL;
    int rc = 0, syms;

    rc = asn_parse_value_text("{ iscsi:{}}",
                              ndn_csap_spec, &csap_spec, &syms); 
    if (rc != 0)
    {
        ERROR("%s(): parse ASN csap_spec failed %X, sym %d", 
              __FUNCTION__, rc, syms);
        return rc;
    } 
    if (socket > 0)
    {
        asn_write_int32(csap_spec, socket, "0.#iscsi.socket");
    }

    rc = tapi_tad_csap_create(ta_name, sid, "iscsi", 
                              csap_spec, csap); 
    if (rc != 0)
        ERROR("%s(): csap create failed, rc %X", __FUNCTION__, rc);

    return rc;
}


int
tapi_iscsi_ini_csap_create(const char *ta_name, int sid, 
                           csap_handle_t listen_csap, 
                           int timeout, csap_handle_t *csap)
{
    int ini_socket; 
    int rc; 

    rc = tapi_tcp_server_recv(ta_name, sid, listen_csap, 
                              timeout, &ini_socket);
    if (rc != 0)
    {
        WARN("%s(): wait for accepted socket failed, %r", 
             __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    }

    return tapi_iscsi_sock_csap_create(ta_name, sid, ini_socket, csap);
}



struct iscsi_data_message {
    iscsi_target_params_t *params;

    uint8_t *data;
    size_t   length;
};



/* 
 * Pkt handler for TCP packets 
 */
static void
iscsi_msg_handler(const char *pkt_fname, void *user_param)
{
    asn_value  *pkt = NULL;
    struct iscsi_data_message *msg;

    int s_parsed = 0;
    int rc = 0;
    size_t len;

    if (user_param == NULL) 
    {
        ERROR("%s called with NULL user param", __FUNCTION__);
        return;
    } 
    msg = user_param;

    if ((rc = asn_parse_dvalue_in_file(pkt_fname, ndn_raw_packet,
                                       &pkt, &s_parsed)) != 0)
    {                                      
        ERROR("%s(): parse packet fails, rc = %r, sym %d",
              __FUNCTION__, rc, s_parsed);
        return;
    }

    len = asn_get_length(pkt, "payload.#bytes");

    if (len > msg->length)
        WARN("%s(): length of message greater then buffer", __FUNCTION__);

    len = msg->length;
    rc = asn_read_value_field(pkt, msg->data, &len, "payload.#bytes");
    if (rc != 0)
        ERROR("%s(): read payload failed %r", __FUNCTION__, rc);
    msg->length = len;

    if (msg->params != NULL)
        asn_read_int32(pkt, &(msg->params->param), "pdus.0.#iscsi.param");

    asn_free_value(pkt);
}

/* See description in tapi_iscsi.h */
int
tapi_iscsi_recv_pkt(const char *ta_name, int sid, csap_handle_t csap,
                    int timeout, csap_handle_t forward,
                    iscsi_digest_type digest,
                    iscsi_target_params_t *params,
                    uint8_t *buffer, size_t  *length)
{
    asn_value *pattern = NULL;
    struct iscsi_data_message msg;

    int rc = 0, syms, num;

    if (ta_name == NULL || socket == NULL)
        return TE_EWRONGPTR;

    if (buffer != NULL)
    {
        if (length == NULL)
            return TE_EWRONGPTR;

        msg.params = params;
        msg.data   = buffer;
        msg.length = *length;
    }

    rc = asn_parse_value_text("{{pdus { iscsi:{} } }}",
                              ndn_traffic_pattern, &pattern, &syms);
    if (rc != 0)
    {
        ERROR("%s(): parse ASN csap_spec failed %X, sym %d", 
              __FUNCTION__, rc, syms);
        return rc;
    } 

    if (forward != CSAP_INVALID_HANDLE)
    {
        rc = asn_write_int32(pattern, forward, "0.action.#forw-pld");
        if (rc != 0)
        {
            ERROR("%s():  write forward csap failed: %r",
                  __FUNCTION__, rc);
            goto cleanup;
        }
    }
    if ((digest & ISCSI_DIGEST_HEADER) == ISCSI_DIGEST_HEADER)
    {
        rc = asn_write_value_field(pattern, NULL, 0, "0.pdus.0.have-hdig");
        if (rc != 0)
        {
            ERROR("%s():  write header-digest failed: %r",
                  __FUNCTION__, rc);
            goto cleanup;
        }
    }

    if ((digest & ISCSI_DIGEST_DATA) == ISCSI_DIGEST_DATA)
    {
        rc = asn_write_value_field(pattern, NULL, 0, "0.pdus.0.have-ddig");
        if (rc != 0)
        {
            ERROR("%s():  write data-digest failed: %r",
                  __FUNCTION__, rc);
            goto cleanup;
        }
    }


    rc = tapi_tad_trrecv_start(ta_name, sid, csap, pattern, 
                               buffer == NULL ? NULL : iscsi_msg_handler,
                               buffer == NULL ? NULL : &msg, timeout, 1);
    if (rc != 0)
    {
        ERROR("%s(): trrecv_start failed %r", __FUNCTION__, rc);
        goto cleanup;
    }

    rc = rcf_ta_trrecv_wait(ta_name, sid, csap, &num);
    if (rc != 0)
        WARN("%s() trrecv_wait failed: %r", __FUNCTION__, rc);

    if (buffer != NULL)
        *length = msg.length;

cleanup:
    asn_free_value(pattern);
    return rc;
}


/* See description in tapi_iscsi.h */
int
tapi_iscsi_tcp_recv_pkt(const char *ta_name, int sid, 
                        csap_handle_t csap, int timeout,
                        csap_handle_t forward,
                        iscsi_digest_type digest,
                        uint8_t *buffer, size_t  *length)
{
    uint8_t bhs_buffer[ISCSI_BHS_LENGTH];
    int     rc; 
    size_t  len = ISCSI_BHS_LENGTH;

    rc = tapi_tcp_buffer_recv(ta_name, sid, csap, timeout, forward, 
                              TRUE, bhs_buffer, &len); 
    if (rc != 0)
    {
        WARN("%s(%s:%d) failed %r", __FUNCTION__,
             ta_name, (int)csap, rc);
        return TE_RC(TE_TAPI, rc);
    } 

    len = iscsi_rest_data_len(bhs_buffer, digest);
    RING("%s(%s:%d), on TCP connection, calculated rest bytes = %d", 
         __FUNCTION__, ta_name, (int)csap, (int)len);

    if (buffer != NULL)
    {
        if (length == NULL)
        {
            ERROR("%s(): len == NULL but buf != NULL", __FUNCTION__);
            return TE_RC(TE_TAPI, EINVAL);
        }
        if (*length < ISCSI_BHS_LENGTH + len)
        {
            ERROR("%s() length %d of passed buffer too small, " 
                  "rest part of iSCSI PDU %d", __FUNCTION__,
                  (int)(*length), (int)len);
            return TE_RC(TE_TAPI, TE_ESMALLBUF);
        }
        memcpy(buffer, bhs_buffer, ISCSI_BHS_LENGTH);
    }
    if (len > 0)
    { 
        rc = tapi_tcp_buffer_recv(ta_name, sid, csap, timeout, forward, 
                                  TRUE,
                                  buffer == NULL ? NULL : 
                                      buffer + ISCSI_BHS_LENGTH, 
                                  &len); 
    }

    if (length != NULL)
        *length = ISCSI_BHS_LENGTH + len;

    return rc;
}

/* See description in tapi_iscsi.h */
int
tapi_iscsi_send_pkt(const char *ta_name, int sid, csap_handle_t csap,
                    iscsi_target_params_t *params,
                    uint8_t *buffer, size_t  length)
{
    asn_value *template = NULL;

    int rc = 0, syms;

    if (ta_name == NULL || socket == NULL)
        return TE_EWRONGPTR;

    rc = asn_parse_value_text("{pdus { iscsi:{} } }",
                              ndn_traffic_template, &template, &syms);
    if (rc != 0)
    {
        ERROR("%s(): parse ASN csap_spec failed %X, sym %d", 
              __FUNCTION__, rc, syms);
        return rc;
    }

    rc = asn_write_value_field(template, buffer, length, "payload.#bytes");
    if (rc != 0)
    {
        ERROR("%s(): write payload failed %r", __FUNCTION__, rc);
        goto cleanup;
    } 

    if (params != NULL)
        asn_write_int32(template, params->param, "pdus.0.#iscsi.param");

    rc = tapi_tad_trsend_start(ta_name, sid, csap, template,
                               RCF_MODE_BLOCKING);
    if (rc != 0)
    {
        ERROR("%s(): trsend_start failed %r", __FUNCTION__, rc);
        goto cleanup;
    } 

cleanup:
    asn_free_value(template);
    return rc;
}


/* See description in tapi_iscsi.h */
int 
tapi_iscsi_get_key_num(iscsi_segment_data data)
{
    int segment_data_len;

    if ((segment_data_len = asn_get_length((asn_value *)data, "")) == -1)
    {
        ERROR("%s, %d: cannot get length",
              __FUNCTION__, __LINE__);
        return -1;
    }
    return segment_data_len;
}
    
/* See description in tapi_iscsi.h */
char *
tapi_iscsi_get_key_name(iscsi_segment_data segment_data, int key_index)
{
    int rc;
    const asn_value *key_pair;
    char            *name;

    if ((rc = asn_get_indexed(segment_data, 
                              &key_pair,
                              key_index)) != 0)
    {
        ERROR("%s, %d: cannot get key pair, %r",
              __FUNCTION__, __LINE__, rc);
        return NULL;
    }

    if ((rc = asn_read_string((asn_value *)key_pair,
                              &name, "key")) != 0)
    {    
        ERROR("%s, %d: cannot get key name, %r",
              __FUNCTION__, __LINE__, rc);
        return NULL;
    }
    return name;
}

/* See description in tapi_iscsi.h */
int
tapi_iscsi_get_key_index_by_name(iscsi_segment_data data, char *name)
{
    int rc;

    int key_num;
    int key_index; 

    char *key;

    const asn_value *key_pair;

    if ((key_num = asn_get_length(data, "")) == -1)
    {
        ERROR("%s, %d: cannot get length",
              __FUNCTION__, __LINE__);
        return TAPI_ISCSI_KEY_INVALID;
    }
    for (key_index = 0; key_index < key_num; key_index++)
    {
       if ((rc = asn_get_indexed(data, &key_pair, key_index)) != 0)
       {
           ERROR("%s, %d: cannot get key from segment data, %r",
                 __FUNCTION__, __LINE__, rc);
           return TAPI_ISCSI_KEY_INVALID;
       }
       if ((rc = asn_read_string(key_pair,
                                 &key, "key")) != 0)
       {    
            ERROR("%s, %d: cannot get key name, %r",
                  __FUNCTION__, __LINE__, rc);
            return TAPI_ISCSI_KEY_INVALID;
       }
       if ((strlen(key) == strlen(name)) && 
           (strcmp(key, name) == 0))
           break;
    }
    if (key_index == key_num)
    {
        RING("There is no key %s in Segment Data", name);
        return TAPI_ISCSI_KEY_INVALID;
    }
    return key_index;
}

/* See description in tapi_iscsi.h */
iscsi_key_values
tapi_iscsi_get_key_values(iscsi_segment_data data,
                          int key_index)
{
    int rc;

    const asn_value *key_pair;
    const asn_value *key_values;

    if ((rc = asn_get_indexed(data, 
                              &key_pair,
                              key_index)) != 0)
    {
        ERROR("%s, %d: cannot get key pair, %r",
              __FUNCTION__, __LINE__, rc);
        return NULL;
    }

    if ((rc = asn_get_child_value(key_pair,
                                  &key_values,
                                  PRIVATE,
                                  NDN_TAG_ISCSI_SD_VALUES)) != 0)
    {
        ERROR("%s, %d: cannot get child value, %r",
              __FUNCTION__, __LINE__, rc);
        return NULL;
    }
    return (iscsi_key_values)key_values;
}

/* See description in tapi_iscsi.h */
int
tapi_iscsi_get_key_values_num(iscsi_key_values values)
{
    int key_values_len;

    if ((key_values_len = asn_get_length((asn_value *)values, "")) == -1)
    {
        ERROR("%s, %d: cannot get length",
              __FUNCTION__, __LINE__);
        return -1;
    }
    return key_values_len;
}

/* See description in tapi_iscsi.h */
iscsi_key_value_type
tapi_iscsi_get_key_value_type(iscsi_key_values values, int key_value_index)
{
    int rc;
    
    iscsi_key_value_type type;
    const asn_value     *elem;
    const asn_value     *value;
    asn_tag_class        tag_class;
    uint16_t             tag_val;
        

    if ((rc = asn_get_indexed(values, &elem, key_value_index)) != 0)
    {
        ERROR("%s, %d: cannot get value, %r",
              __FUNCTION__, __LINE__, rc);
        return iscsi_key_value_type_invalid;
    }
    if ((rc = asn_get_choice_value(elem, &value, 
                                   &tag_class, &tag_val)) != 0)
    {
        ERROR("%s, %d: cannot get choice value, %r",
              __FUNCTION__, __LINE__, rc);
        return iscsi_key_value_type_invalid;
    }
    switch (tag_val)
    {
        case NDN_TAG_ISCSI_SD_INT_VALUE:
        {
            type = iscsi_key_value_type_int;
            break;
        }
        case NDN_TAG_ISCSI_SD_HEX_VALUE:
        {
            type = iscsi_key_value_type_hex;
            break;
        }
        case NDN_TAG_ISCSI_SD_STR_VALUE:
        {
            type = iscsi_key_value_type_string;
            break;
        }
        default:
        {
            ERROR("%s, %d: strange tag value in asn value",
                  __FUNCTION__, __LINE__);
            type = iscsi_key_value_type_invalid;
        }
    }
    return type;
}

/* See description in tapi_iscsi.h */
int
tapi_iscsi_get_string_key_value(iscsi_key_values values, 
                                int key_value_index, char **str)
{
    int rc;
    
    const asn_value     *elem;
    const asn_value     *value;
    asn_tag_class        tag_class;
    uint16_t             tag_val;
        

    if ((rc = asn_get_indexed(values, &elem, key_value_index)) != 0)
    {
        ERROR("%s, %d: cannot get value, %r",
              __FUNCTION__, __LINE__, rc);
        return rc;
    }
    if ((rc = asn_get_choice_value(elem, &value, 
                                   &tag_class, &tag_val)) != 0)
    {
        ERROR("%s, %d: cannot get choice value, %r",
              __FUNCTION__, __LINE__, rc);
        return rc;
    }
    if (tag_val != iscsi_key_value_type_string)
    {
        ERROR("%s, %d: bad type provided, %r",
              __FUNCTION__, __LINE__, rc);
        return TE_EASNWRONGTYPE;
    }
    if ((rc = asn_read_string(value, str, "")) != 0)
    {
        ERROR("%s, %d: cannot read string, %r",
              __FUNCTION__, __LINE__, rc);
        return rc;
    }
    return 0;
}

/* See description in tapi_iscsi.h */
int
tapi_iscsi_get_int_key_value(iscsi_key_values values, 
                             int key_value_index, int *int_val)
{
    int rc;
    
    const asn_value     *elem;
    const asn_value     *value;
    asn_tag_class        tag_class;
    uint16_t             tag_val;
        

    if ((rc = asn_get_indexed(values, &elem, 
                              key_value_index)) != 0)
    {
        ERROR("%s, %d: cannot get value, %r",
              __FUNCTION__, __LINE__, rc);
        return rc;
    }
    if ((rc = asn_get_choice_value(elem, &value, 
                                   &tag_class, &tag_val)) != 0)
    {
        ERROR("%s, %d: cannot get choice value, %r",
              __FUNCTION__, __LINE__, rc);
        return rc;
    }
    if ((tag_val != iscsi_key_value_type_int) && 
        (tag_val != iscsi_key_value_type_hex))
    {
        ERROR("%s, %d: bad type provided, %r",
              __FUNCTION__, __LINE__, rc);
        return TE_EASNWRONGTYPE;
    }
    if ((rc = asn_read_int32(value, int_val, "")) != 0)
    {
        ERROR("%s, %d: cannot read inetger_value, %r",
              __FUNCTION__, __LINE__, rc);
        return rc;
    }
    return 0;
}

/* See description in tapi_iscsi.h */
int
tapi_iscsi_add_new_key(iscsi_segment_data data, char *name, int key_index)
{
    int rc;

    int key_num;

    asn_value *key_pair;
    asn_value *key_values;

    if ((key_num = asn_get_length((asn_value *)data, "")) == -1)
    {
        ERROR("%s, %d: cannot get length",
              __FUNCTION__, __LINE__);
        return TAPI_ISCSI_KEY_INVALID;
    }

    if (key_index < TAPI_ISCSI_KEY_INVALID || key_index > key_num - 1)
    {
        ERROR("%s, %d: invalid key index parameter provided",
              __FUNCTION__, __LINE__);
        return TAPI_ISCSI_KEY_INVALID;
    }
    if ((key_values = asn_init_value(ndn_iscsi_key_values)) == NULL)
    {
        ERROR("%s, %d: cannot init asn_value",
              __FUNCTION__, __LINE__);
        return TE_ENOMEM;
    }

    if ((key_pair = asn_init_value(ndn_iscsi_key_pair)) == NULL)
    {
        ERROR("%s, %d: cannot init asn_value",
              __FUNCTION__, __LINE__);
        return TAPI_ISCSI_KEY_INVALID;
    }

    if ((rc = asn_write_string(key_pair, name, "key")) != 0)
    {
        ERROR("%s, %d: cannot write string, %r",
              __FUNCTION__, __LINE__, rc);
        return TAPI_ISCSI_KEY_INVALID;
    }

    if ((rc = asn_put_child_value_by_label(key_pair,
                                           key_values,
                                           "values")) != 0)
    {
        ERROR("%s, %d: cannot put child value, %r",
              __FUNCTION__, __LINE__, rc);
        return rc;
    }
    
    if ((rc = asn_insert_indexed(data, key_pair, key_index, "")) != 0)
    {
        ERROR("%s, %d: cannot insert element, %r",
              __FUNCTION__, __LINE__);
        return TAPI_ISCSI_KEY_INVALID;
    }

    asn_free_value(key_pair);
    return (key_index == TAPI_ISCSI_KEY_INVALID) ? key_num : key_index;
}


/* See description in tapi_iscsi.h */
iscsi_key_values 
tapi_iscsi_key_values_create(int num, ...)
{
    int rc = 0;
    
    va_list list;

    asn_value *key_values = NULL;
    asn_value *key_value = NULL;

    iscsi_key_value_type type;

    int i;

    int   int_val;
    char *str_val;

    const char *label;

    va_start(list, num);
    
    if ((key_values = asn_init_value(ndn_iscsi_key_values)) == NULL)
    {
        ERROR("%s, %d: cannot init asn_value",
              __FUNCTION__, __LINE__);
        goto cleanup;
    }

    for (i = 0; i < num; i++)
    {
        type = va_arg(list, iscsi_key_value_type);
        if ((key_value = asn_init_value(ndn_iscsi_key_value)) == NULL)
        {
            ERROR("%s, %d: cannot init asn_value",
                  __FUNCTION__, __LINE__);
            rc = -1;
            goto cleanup;
        }

        switch (type)
        {
            case iscsi_key_value_type_int:
            {
                int_val = va_arg(list, int);
                label = "#int";
                if ((rc = asn_write_int32(key_value, int_val, "#int")) != 0)
                {
                    ERROR("%s, %d: cannot write int value, %r",
                          __FUNCTION__, __LINE__, rc);
                    goto cleanup;
                }
                break;
            }
            case iscsi_key_value_type_hex:
            {
                int_val = va_arg(list, int);
                label = "#hex";
                if ((rc = asn_write_int32(key_value, int_val, "#hex")) != 0)
                {
                    ERROR("%s, %d: cannot write int value, %r",
                          __FUNCTION__, __LINE__, rc);
                    goto cleanup;
                }
                break;
            }
            case iscsi_key_value_type_string:
            {
                str_val = va_arg(list, char *);
                label = "#str";
                if ((rc = asn_write_string(key_value, 
                                           str_val, "#str")) != 0)
                {
                    ERROR("%s, %d: cannot write string value, %r",
                          __FUNCTION__, __LINE__, rc);
                    goto cleanup;
                }
                break;
            }
            default:
            {
                ERROR("%s, %d: bad type provided",
                      __FUNCTION__, __LINE__);
                goto cleanup;
            }
        }
        if ((rc = asn_insert_indexed(key_values, key_value, i, "")) != 0)
        {
            ERROR("%s, %d: cannot insert element, %r",
                  __FUNCTION__, __LINE__, rc);
            goto cleanup;
        }
        asn_free_value(key_value);
    }
cleanup:
    if (key_values == NULL || rc != 0)
    {
        asn_free_value(key_values);
        key_values = NULL;
    }
    return key_values;
}

/* See description in tapi_iscsi.h */
int
tapi_iscsi_set_key_values(iscsi_segment_data data,
                          int key_index,
                          iscsi_key_values values)
{
    int rc;

    asn_value *key_pair;

    if ((rc = asn_get_indexed(data, 
                              (const asn_value **)&key_pair, 
                              key_index)) != 0)
    {
        ERROR("%s, %d: cannot get element, %r",
              __FUNCTION__, __LINE__, rc);
        return rc;
    }
    if ((rc = asn_put_child_value_by_label(key_pair,
                                           values, 
                                           "values")) != 0)
    {
        ERROR("%s, %d: cannot put child, %r",
              __FUNCTION__, __LINE__, rc);
        return rc;
    }
    return 0;
}    

/* See description in tapi_iscsi.h */
void
tapi_iscsi_free_key_values(iscsi_key_values values)
{
    asn_free_value(values);
    return;
}

/* See description in tapi_iscsi.h */
int
tapi_iscsi_delete_key(iscsi_segment_data data, int key_index)
{
    int rc;

    if ((rc = asn_remove_indexed(data, key_index, "")) != 0)
    {
        ERROR("%s, %d: cannot remove element, %r",
              __FUNCTION__, __LINE__, rc);
        return rc;
    }
    return 0;
}

/* See description in tapi_iscsi.h */
iscsi_segment_data
tapi_iscsi_keys_create(int num, ...)
{
    int rc = 0;
    
    va_list list;

    char *key;

    asn_value *key_pair = NULL;
    asn_value *segment_data = NULL;

    int i;

    va_start(list, num);
    
    
    if ((segment_data = asn_init_value(ndn_iscsi_segment_data)) == NULL)
    {
        ERROR("%s, %d: cannot init asn_value",
              __FUNCTION__, __LINE__);
        goto cleanup;
    }

    if ((key_pair = asn_init_value(ndn_iscsi_key_pair)) == NULL)
    {
        ERROR("%s, %d: cannot init asn_value",
              __FUNCTION__, __LINE__);
        goto cleanup;
    }

    for (i = 0; i < num; i++)
    {
        key = va_arg(list, char *);
        if ((rc = asn_write_string(key_pair, key, "key")) != 0)
        {
            ERROR("%s, %d: cannot write string, %r",
                  __FUNCTION__, __LINE__, rc);
            goto cleanup;
        }
        if ((rc = asn_insert_indexed(segment_data, 
                                     key_pair, i, "")) != 0)
        {
            ERROR("%s, %d: cannot insert element, %r",
                  __FUNCTION__, __LINE__);
            goto cleanup;
        }
    }    
cleanup:
    if (key_pair == NULL || segment_data == NULL || rc != 0)
    {
        asn_free_value(segment_data);
        segment_data = NULL;
    }
    asn_free_value(key_pair);
    return segment_data;
}
           
/* see description in tapi_iscsi.h */
void
tapi_iscsi_keys_data_free(iscsi_segment_data segment_data)
{
    asn_free_value(segment_data);
    return;
}

int
tapi_iscsi_change_key_values(iscsi_segment_data segment_data,
                             char *key_name, 
                             tapi_iscsi_change_key_val_type change,
                             int num, ...)
{
    int                rc;
    
    int                key_index;
    iscsi_key_values   key_values;
    asn_value         *key_value;
    int                key_values_num;
    int                i = 0;

    va_list list;

    if ((key_index = 
         tapi_iscsi_get_key_index_by_name(segment_data, key_name)) ==
            TAPI_ISCSI_KEY_INVALID)
    {
        ERROR("%s, %d: No key with %s name",
              __FUNCTION__, __LINE__, key_name);
        return -1;
    }

    if ((key_values = tapi_iscsi_get_key_values(segment_data,
                                                key_index)) == NULL)
    {
        ERROR("%s, %d: cannot get key values",
              __FUNCTION__, __LINE__);
        return -1;
    }
    if ((key_values_num = 
         tapi_iscsi_get_key_values_num(key_values)) == -1)
    {
        ERROR("%s, %d: cannot get key values number",
              __FUNCTION__, __LINE__);
        return -1;
    }

    if (change == tapi_iscsi_replace_key_values)
    {
        for (i = key_values_num; i > 0; i--)
        {
            if ((rc = asn_remove_indexed(key_values, i - 1, "")) != 0)
            {
                ERROR("%s, %d: cannot remove key values",
                      __FUNCTION__, __LINE__);
                return rc;
            }
        }
    }

    va_start(list, num);

    while (i++ < num)
    {
        char *str = va_arg(list, char *);

        if ((key_value = asn_init_value(ndn_iscsi_key_value)) == NULL)
        {
            ERROR("%s, %d: cannot init key value",
                  __FUNCTION__, __LINE__);
            return -1;
        }
        if ((rc = parse_key_value(str, key_value)) != 0)
        {
            ERROR("%s, %d: cannot parse key value",
                  __FUNCTION__, __LINE__);
            return rc;
        }
        if (change == tapi_iscsi_replace_key_values ||
            change == tapi_iscsi_insert_key_values)
        {    
            if ((rc = asn_insert_indexed(key_values,
                                         key_value,
                                         -1, "")) != 0)
            {
                ERROR("%s, %d: cannot insert key value",
                      __FUNCTION__, __LINE__);
                return rc;
            }
        }
        if (change == tapi_iscsi_remove_key_values)
        {
            ERROR("%s, %d: sorry, remove is not supported yet",
                  __FUNCTION__, __LINE__);
            return -1;
        }
        asn_free_value(key_value);
    }
    va_end(list);
    return 0;
}


/* see description in tapi_iscsi.h */
int
tapi_iscsi_find_key_and_value(iscsi_segment_data segment_data,
                              const char *key_name, int num, ...)
{
    int                rc;
    
    int                key_index;
    iscsi_key_values   key_values;
    int                key_values_num;
    int                key_values_index;
    te_bool            found;
    int                i = 0;

    iscsi_key_value_type type;

    va_list list;

    if ((key_index = 
         tapi_iscsi_get_key_index_by_name(segment_data, 
                                          (char *)key_name)) ==
            TAPI_ISCSI_KEY_INVALID)
    {
        ERROR("%s, %d: No key with %s name",
              __FUNCTION__, __LINE__, key_name);
        return -1;
    }

    if ((key_values = tapi_iscsi_get_key_values(segment_data,
                                                key_index)) == NULL)
    {
        ERROR("%s, %d: cannot get key values",
              __FUNCTION__, __LINE__);
        return -1;
    }
    if ((key_values_num = 
         tapi_iscsi_get_key_values_num(key_values)) == -1)
    {
        ERROR("%s, %d: cannot get key values number",
              __FUNCTION__, __LINE__);
        return -1;
    }

    va_start(list, num);

    while (i++ < num)
    {
        type = va_arg(list, iscsi_key_value_type);
        found = FALSE;
        for (key_values_index = 0;
             (key_values_index < key_values_num) && (found == FALSE);
             key_values_index++)
        {
            if (type != 
                tapi_iscsi_get_key_value_type(key_values,
                                              key_values_index))
                continue;
            switch (type)
            {
                case iscsi_key_value_type_int:
                case iscsi_key_value_type_hex:
                {
                    int search_value = va_arg(list, int);
                    int key_value;

                    if ((rc = 
                        tapi_iscsi_get_int_key_value(
                            key_values,
                            key_values_index,
                            &key_value)) != 0)
                    {
                        ERROR("%s, %d: cannot get integer value",
                              __FUNCTION__, __LINE__);
                        return rc;
                    }
                    if (search_value == key_value)
                    {
                        found = TRUE;
                        continue;
                    }
                    break;
                }
                case iscsi_key_value_type_string:    
                {
                    char *search_value = va_arg(list, char *);
                    char *key_value;

                    if ((rc = 
                        tapi_iscsi_get_string_key_value(
                            key_values,
                            key_values_index,
                            &key_value)) != 0)
                    {
                        ERROR("%s, %d: cannot get string value",
                              __FUNCTION__, __LINE__);
                        return rc;
                    }
                    if (strcmp(search_value, key_value) == 0)
                    {
                        found = TRUE;
                        continue;
                    }
                    break;
                }
                default:
                {
                    /* Should never happen */
                    {
                        ERROR("%s, %d: strange value type",
                              __FUNCTION__, __LINE__);
                        return -1;
                    }
                }
            }
            if (found == FALSE)
            {
                va_end(list);
                ERROR("%s, %d: cannot find value for key %s",
                      __FUNCTION__, __LINE__, key_name);
                return -1;
            }
        }
    }
    va_end(list);
    return 0;
}


/* see description in tapi_iscsi.h */
int
tapi_iscsi_return_key_value(iscsi_segment_data segment_data,
                            const char *key_name,
                            const char *buf,
                            int buf_len)
{
    int                rc;

    int                key_index;
    iscsi_key_values   key_values;


    if ((key_index =
         tapi_iscsi_get_key_index_by_name(segment_data, 
                                          (char *)key_name)) ==
            TAPI_ISCSI_KEY_INVALID)
    {
        ERROR("%s, %d: No key with %s name",
              __FUNCTION__, __LINE__, key_name);
        return -1;
    }

    if ((key_values = tapi_iscsi_get_key_values(segment_data,
                                                key_index)) == NULL)
    {
        ERROR("%s, %d: cannot get key values",
              __FUNCTION__, __LINE__);
        return -1;
    }

    rc = asn_sprint_value(key_values, (char *)buf, buf_len, 0);
    return rc;
}

/* see description in tapi_iscsi.h */
int
tapi_iscsi_find_key_values(iscsi_segment_data segment_data,
                           const char *key_name,
                           iscsi_key_values *key_array)
{
    int key_index;


    if (key_array == NULL)
        return -1;

    if ((key_index =
         tapi_iscsi_get_key_index_by_name(segment_data, 
                                          (char *)key_name)) ==
            TAPI_ISCSI_KEY_INVALID)
    {
        return 0;
    }
    if ((*key_array = tapi_iscsi_get_key_values(segment_data,
                                                key_index)) == NULL)
    {
        ERROR("%s, %d: cannot get key values",
              __FUNCTION__, __LINE__);
        return -1;
    }


    return asn_get_length(*key_array, "");
}

/* see description in tapi_iscsi.h */
int
tapi_iscsi_key_value_read(iscsi_key_values key_array,
                          int val_index, char *buf, size_t *buf_len)
{
    int rc;

    const asn_value *key_value;

    if ((rc = asn_get_indexed(key_array, &key_value, val_index)) != 0)
    {
        ERROR("%s(): asn_get_indexed failed %r", __FUNCTION__, rc);
        return rc;
    }

    if ((rc = asn_read_value_field((asn_value *)key_value,
                                   buf, buf_len, "")) != 0)
    {    
        ERROR("%s(): cannot read key value %d, %r",
              __FUNCTION__, rc);
        return rc;
    } 

    return 0;
}


/* Target configuration */

/* see description in tapi_iscsi.h */
int
tapi_iscsi_target_set_parameter(const char *ta,
                                tapi_iscsi_parameter param, 
                                const char *value)
{
    static char *mapping[] = {
        "oper:/header_digest:",
        "oper:/data_digest:",
        "oper:/max_connections:",
        "oper:/send_targets:",  
        "oper:/target_name:",
        "oper:/initiator_name:",
        "oper:/target_alias:",
        "oper:/initiator_alias:",
        "oper:/target_address:",
        "oper:/target_port:",
        "oper:/initial_r2t:",
        "oper:/immediate_data:",
        "oper:/max_recv_data_segment_length:",
        "oper:/max_burst_length:",
        "oper:/first_burst_length:",
        "oper:/default_time2wait:",
        "oper:/default_time2retain:",
        "oper:/max_outstanding_r2t:",
        "oper:/data_pdu_in_order:",
        "oper:/data_sequence_in_order:",
        "oper:/error_recovery_level:",
        "oper:/session_type:",
        "oper:/of_marker:",
        "oper:/if_marker:",
        "oper:/of_mark_int:",
        "oper:/if_mark_int:",
        NULL,
        "chap:/lx:",
        "chap:/ln:",
        "chap:/t:/px:",
        "chap:/t:/pn:",
        "chap:/cl:",
        "chap:/b:",
        "chap:/t:",
        "chap:"
    };

    assert(ta != NULL);
    assert(param < sizeof(mapping) / sizeof(*mapping));
    assert(mapping[param] != NULL);
    return cfg_set_instance_fmt(CVT_STRING, value,
                                "/agent:%s/iscsi_target:/%s",
                                ta, mapping[param]);
} 

/* see description in tapi_iscsi.h */
int 
tapi_iscsi_target_customize(const char *ta, int id, 
                            const char *key, 
                            const char *value)
{
    te_errno remote_rc;
    te_errno local_rc;
    local_rc = rcf_ta_call(ta, 0, "iscsi_set_custom_value", &remote_rc,
                           3, FALSE, 
                           RCF_INT32, id,
                           RCF_STRING, key, RCF_STRING, value);
    return local_rc ? local_rc : 
        (remote_rc ? TE_RC(TE_TAPI, TE_ESRCH) : 0);
}

/* Initiator configuration */
static char *log_mapping[] = {
        "HeaderDigest",
        "DataDigest",
        "MaxConnections",
        "",
        "TargetName",
        "InitiatorName",
        "TargetAlias",
        "InitiatorAlias",
        "TargetAddr",
        "TargetPort",
        "InitialR2T",
        "ImmediateData",
        "MaxRecvDataSegmentLength",
        "MaxBurstLength",
        "FirstBurstLength",
        "DefaultTime2Wait",
        "DefaultTime2Retain",
        "MaxOutstandingR2T",
        "DataPDUInOrder",
        "DataSequenceInOrder",
        "ErrorRecoveryLevel",
        "SessionType",
        "OFMarker",
        "IFMarker",
        "OFMarkInt",
        "IFMarkInt",
        NULL,
        "LocalSecret",
        "LocalName",
        "PeerSecret",
        "PeerName",
        "ChallengeLength",
        "EncFmt",
        "TargetAuth",
        "AuthMethod"
    };

int
tapi_iscsi_initiator_not_advertize(const char *ta,
                                   iscsi_target_id target_id,
                                   tapi_iscsi_parameter param)
{
    static uint32_t offer_mapping[] = {
        OFFER_HEADER_DIGEST,
        OFFER_DATA_DIGEST,
        OFFER_MAX_CONNECTIONS,
        0,  
        0,
        0,
        0,
        0,
        0,
        0,
        OFFER_INITIAL_R2T,
        OFFER_IMMEDIATE_DATA,
        OFFER_MAX_RECV_DATA_SEGMENT_LENGTH,
        OFFER_MAX_BURST_LENGTH,
        OFFER_FIRST_BURST_LENGTH,
        OFFER_DEFAULT_TIME2WAIT,
        OFFER_DEFAULT_TIME2RETAIN,
        OFFER_MAX_OUTSTANDING_R2T,
        OFFER_DATA_PDU_IN_ORDER,
        OFFER_DATA_SEQUENCE_IN_ORDER,
        OFFER_ERROR_RECOVERY_LEVEL,
        0,
        0,
        0,
    };

    char         offer[50];
    int          par2adv;
    int          rc;
    cfg_val_type type = CVT_STRING;

    RING("Set not advertize %s param on (%s, taret_%d)",
         log_mapping[param], ta, target_id);

    memset(offer, 0, sizeof(offer));
    
    rc = cfg_get_instance_fmt(&type, offer,
                              "/agent:%s/iscsi_initiator:/target_data:"
                              "target_%d/parameters2advertize:",
                              ta, target_id);
    if (rc != 0)
    {
        ERROR("Failed to get current parameters2advertize");
        return rc;
    }

    par2adv = atoi(offer);
    
    par2adv &= ~(offer_mapping[param]);
    
    sprintf(offer, "%d", par2adv);

    rc = cfg_set_instance_fmt(CVT_STRING, offer,
                              "/agent:%s/iscsi_initiator:/target_data:"
                              "target_%d/parameters2advertize:",
                              ta, target_id);
    if (rc != 0)
    {
        ERROR("Failed to get current parameters2advertize");
        return rc;
    }
    
    return 0;
}

/* see description in tapi_iscsi.h */
int 
tapi_iscsi_initiator_set_parameter(const char *ta,
                                   iscsi_target_id target_id,
                                   tapi_iscsi_parameter param,
                                   const char *value)
{
    int rc = -1;
    
    static char *mapping[] = {
        "header_digest:",                         /* - */
        "data_digest:",                           /* - */
        "max_connections:",                       /* - */
        "",  
        "target_name:",                           /* - */
        "initiator_name:",
        "target_alias:",                          /* - */
        "initiator_alias:",
        "target_addr:",                           /* - */
        "target_port:",                           /* - */
        "initial_r2t:",                           /* - */
        "immediate_data:",                        /* - */
        "max_recv_data_segment_length:",          /* - */
        "max_burst_length:",                      /* - */
        "first_burst_length:",                    /* - */
        "default_time2wait:",                     /* - */
        "default_time2retain:",                   /* - */
        "max_outstanding_r2t:",                   /* - */
        "data_pdu_in_order:",                     /* - */
        "data_sequence_in_order:",                /* - */
        "error_recovery_level:",                  /* - */
        "session_type:",                          /* - */
        "of_marker:",
        "if_marker:",
        "of_mark_int:",
        "if_mark_int:",
        NULL,
        "chap:/local_secret:",
        "chap:/local_name:",                      /* - */
        "chap:/peer_secret:",                     /* - */
        "chap:/peer_name:",
        "chap:/challenge_length:",
        "chap:/enc_fmt:",
        "chap:/target_auth:",                      /* - */
        "chap:"
    };


    assert(ta != NULL);
    assert(param < sizeof(mapping) / sizeof(*mapping));
    assert(mapping[param] != NULL);
    
    RING("Set %s (%s, target=%d) to %s", log_mapping[param], 
         ta, target_id, value);

    rc = cfg_set_instance_fmt(CVT_STRING, value,
                              "/agent:%s/iscsi_initiator:/target_data:"
                              "target_%d/%s",
                              ta, target_id, mapping[param]);

    if (rc != 0)
    {
        ERROR("Failed to set %s parameter to %s, rc = %d (%r)",
              log_mapping[param], value, rc, rc);
        
        return rc;
    }

    return 0;
}

#define MAX_INI_CMD_SIZE 10
#define MAX_TARGETS_NUMBER 10
#define MAX_CONNECTION_NUMBER 100

static int iscsi_current_cid[MAX_CONNECTION_NUMBER];
static int iscsi_current_target = 0;

/* see description in tapi_iscsi.h */
iscsi_cid 
tapi_iscsi_initiator_conn_add(const char *ta,
                              iscsi_target_id tgt_id)
{
    int  rc;
    char cmd[MAX_INI_CMD_SIZE];

    cmd[0] = 0;
    
    sprintf(cmd, "up %d %d", iscsi_current_cid[tgt_id], tgt_id);

    rc = cfg_set_instance_fmt(CVT_STRING, (void *)cmd,
                              "/agent:%s/iscsi_initiator:", ta);
    return (rc == 0) ? (iscsi_current_cid[tgt_id]++) : (-rc);
}

/* see description in tapi_iscsi.h */
int 
tapi_iscsi_initiator_conn_del(const char *ta,
                              iscsi_target_id tgt_id,
                              iscsi_cid cid)
{
    char cmd[MAX_INI_CMD_SIZE];
    int  rc;

    cmd[0] = 0;
    sprintf(cmd, "down %d %d", cid, tgt_id);

    rc = cfg_set_instance_fmt(CVT_STRING, (void *)cmd,
                              "/agent:%s/iscsi_initiator:",
                              ta);
    if (rc != 0)
    {
        ERROR("Failed to delete connection with id %d from target %d",
              cid, tgt_id);
    }
    return rc;
}

/* see description in tapi_iscsi.h */
iscsi_target_id
tapi_iscsi_initiator_add_target(const char *ta,
                                const struct sockaddr *target_addr)
{
    int        rc;
    cfg_handle handle;

    char *target_addr_param = NULL;
    int   target_port = 0;

    char port[100];

    switch (target_addr->sa_family)
    {
        case PF_INET:
            target_addr_param = inet_ntoa(SIN(target_addr)->sin_addr);
            target_port = ntohs(SIN(target_addr)->sin_port);
            break;
        default:
            ERROR("%s(): Unsupported address family", __FUNCTION__);
            return -EINVAL;
    }

    RING("Initiator (%s): add Target: addr=%s, port=%d",
         ta, target_addr_param, target_port);
    
    rc = cfg_add_instance_fmt(&handle, CVT_STRING,
                        "",
                        "/agent:%s/iscsi_initiator:/target_data:target_%d",
                        ta, iscsi_current_target);
    if (rc != 0)
    {
        ERROR("Failed to add target_data instance to the initiator");
        return -rc;
    }

    rc = tapi_iscsi_initiator_set_parameter(ta,
                                            iscsi_current_target,
                                            ISCSI_PARAM_TARGET_ADDRESS,
                                            (void *)target_addr_param);
    if (rc != 0)
    {
        ERROR("Failed to set local parameter of the target rc = %d", rc);
        return -rc;
    }

    sprintf(port, "%d", target_port);
    rc = tapi_iscsi_initiator_set_parameter(ta,
                                            iscsi_current_target,
                                            ISCSI_PARAM_TARGET_PORT,
                                            (void *)port);
    if (rc != 0)
    {
        ERROR("Failed to add target_data instance to the initiator");
        return -rc;
    }
    
    iscsi_current_cid[(int)iscsi_current_target] = 0;
    
    VERB("Target with ID=%d added to Initiator on agent %s", 
         iscsi_current_target, ta);
    return (iscsi_current_target++);
}

/* see description in tapi_iscsi.h */
int 
tapi_iscsi_initiator_del_target(const char *ta,
                                iscsi_target_id tgt_id)
{
    int        rc;
    cfg_handle handle;

    rc = cfg_find_fmt(&handle, "/agent:%s/iscsi_initiator:/target_data:"
                      "target_%d", ta, tgt_id);
    if (rc != 0)
    {
        ERROR("No connection with such ID");
        return rc;
    }

    rc = cfg_del_instance(handle, FALSE);
    if (rc != 0)
    {
        ERROR("Failed to delete target with ID %d from agent %s",
              tgt_id, ta);
    }
    return rc;
}

/* see description in tapi_iscsi.h */
int
tapi_iscsi_initiator_get_devices(const char *ta, char **buffer)
{
    cfg_val_type type = CVT_STRING;
    

    return cfg_get_instance_fmt(&type, buffer,
                                "/agent:%s/iscsi_initiator:/host_device:",
                                ta);
}
                                 


/* see description in tapi_iscsi.h */
tapi_iscsi_parameter
tapi_iscsi_get_param_map(const char *param)
{
    static char *param_map[] = {
        "HeaderDigest",
        "DataDigest",
        "MaxConnections",
        "SendTargets",
        "TargetName",
        "InitiatorName",
        "TargetAlias",
        "InitiatorAlias",
        "TargetAddress",
        "TargetPort",
        "InitialR2T",
        "ImmediateData",
        "MaxRecvDataSegmentLength",
        "MaxBurstLength",
        "FirstBurstLength",
        "DefaultTime2Wait",
        "DefaultTime2Retain",
        "MaxOutstandingR2T",
        "DataPDUInOrder",
        "DataSequenceInOrder",
        "ErrorRecoveryLevel",
        "SessionType",
        "OFMarker",
        "IFMarker",
        "OFMarkInt",
        "IFMarkInt",
        "",
        "LocalSecret",
        "LocalName",
        "PeerSecret",
        "PeerName",
        "ChallengeLength",
        "EncodingFormat",
        "TargetAuthenticationRequired",
        "SecurityNegotiationPhase",
    };
    static int param_map_size = sizeof(param_map) / sizeof(char *);
    
    int param_id = -1;
    
    for (param_id = 0; param_id < param_map_size; param_id++)
    {
        if (strcmp(param, param_map[param_id]) == 0)
            break;
    }

    if (param_id >= param_map_size)
        param_id = -1;

    return param_id;
}

/* see description in tapi_iscsi.h */
int
tapi_iscsi_forward_all(const char *ta_name, int session,
                       csap_handle_t csap_rcv, csap_handle_t csap_fwd,
                       unsigned int timeout, int *forwarded)
{
    int rc, syms;

    asn_value *pattern;
    rc = asn_parse_value_text("{{pdus { iscsi:{} } }}",
                              ndn_traffic_pattern, &pattern, &syms);
    if (rc != 0)
    {
        ERROR("%s(): parse ASN csap_spec failed %X, sym %d", 
              __FUNCTION__, rc, syms);
        return rc;
    }

    rc = tapi_tad_forward_all(ta_name, session, csap_rcv, csap_fwd, 
                              pattern, timeout, forwarded);
    asn_free_value(pattern);
    return rc;
}



#undef MAX_INI_CMD_SIZE
