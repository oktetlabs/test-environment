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
#include "conf_api.h"

#define TE_LGR_USER "TAPI iSCSI"
#include "logger_api.h"

#include "tapi_iscsi.h"
#include "ndn_iscsi.h"

#include "tapi_tad.h"

int
tapi_iscsi_csap_create(const char *ta_name, int sid, 
                       csap_handle_t *csap)
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

    rc = tapi_tad_csap_create(ta_name, sid, "iscsi", 
                              csap_spec, csap); 
    if (rc != 0)
        ERROR("%s(): csap create failed, rc %X", __FUNCTION__, rc);

    return rc;
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

int
tapi_iscsi_recv_pkt(const char *ta_name, int sid, csap_handle_t csap,
                    int timeout, csap_handle_t forward,
                    iscsi_target_params_t *params,
                    uint8_t *buffer, size_t  *length)
{
    asn_value *pattern = NULL;
    struct iscsi_data_message msg;

    int rc = 0, syms, num;

    if (ta_name == NULL || socket == NULL ||
        buffer == NULL || length == NULL)
        return TE_EWRONGPTR;

    rc = asn_parse_value_text("{{pdus { iscsi:{} } }}",
                              ndn_traffic_pattern, &pattern, &syms);
    if (rc != 0)
    {
        ERROR("%s(): parse ASN csap_spec failed %X, sym %d", 
              __FUNCTION__, rc, syms);
        return rc;
    }

    msg.params = params;
    msg.data   = buffer;
    msg.length = *length;

    if (forward != CSAP_INVALID_HANDLE)
    {
        rc = asn_write_int32(pattern, forward, "0.action.#forward");
        if (rc != 0)
        {
            ERROR("%s():  write forward csap failed: %r",
                  __FUNCTION__, rc);
            goto cleanup;
        }
    }


    rc = tapi_tad_trrecv_start(ta_name, sid, csap, pattern, 
                               iscsi_msg_handler, &msg, timeout, 1);
    if (rc != 0)
    {
        ERROR("%s(): trrecv_start failed %r", __FUNCTION__, rc);
        goto cleanup;
    }

    rc = rcf_ta_trrecv_wait(ta_name, sid, csap, &num);
    if (rc != 0)
        WARN("%s() trrecv_wait failed: %r", __FUNCTION__, rc);

    *length = msg.length;

cleanup:
    asn_free_value(pattern);
    return rc;
}



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

int
tapi_iscsi_add_new_key(iscsi_segment_data data, char *name, int key_index)
{
    int rc;

    int key_num;

    asn_value *key_pair;

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
    if ((rc = asn_insert_indexed(data, key_pair, key_index, "")) != 0)
    {
        ERROR("%s, %d: cannot insert element, %r",
              __FUNCTION__, __LINE__);
        return TAPI_ISCSI_KEY_INVALID;
    }

    asn_free_value(key_pair);
    return (key_index == TAPI_ISCSI_KEY_INVALID) ? key_num : key_index;
}


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

void
tapi_iscsi_free_key_values(iscsi_key_values values)
{
    asn_free_value(values);
    return;
}

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
           
void
tapi_iscsi_keys_data_free(iscsi_segment_data segment_data)
{
    asn_free_value(segment_data);
    return;
}

/* Target configuration */

int
tapi_iscsi_set_local_secret(const char *ta,
                            const char *secret)
{
    assert(ta != NULL && secret != NULL && strlen(secret) == 16);

    return cfg_set_instance_fmt(CVT_STRING, (void *)secret,
                                "/agent:%s/iscsi_target:/chap:/lx:",
                                ta);
}                                

int
tapi_iscsi_set_local_name(const char *ta,
                          const char *name)
{
    assert(ta != NULL && name != NULL);

    return cfg_set_instance_fmt(CVT_STRING, (void *)name,
                                "/agent:%s/iscsi_target:/chap:/ln:",
                                ta);
}                                

int
tapi_iscsi_set_peer_secret(const char *ta,
                          const char *secret)
{
    assert(ta != NULL && secret != NULL && strlen(secret) == 16);

    return cfg_set_instance_fmt(CVT_STRING, (void *)secret,
                                "/agent:%s/iscsi_target:/chap:/t:/lx:",
                                ta);
}                                

int
tapi_iscsi_set_peer_name(const char *ta,
                         const char *name)
{
    assert(ta != NULL && name != NULL);

    return cfg_set_instance_fmt(CVT_STRING, (void *)name,
                                "/agent:%s/iscsi_target:/chap:/t:/ln:",
                                ta);
}                                

int
tapi_iscsi_set_challenge_length(const char *ta,
                                int len)
{
    assert(ta != NULL && len > 255 && len < 1025);

    return cfg_set_instance_fmt(CVT_INTEGER, (void *)len,
                                "/agent:%s/iscsi_target:/chap:/cl:",
                                ta);
}

int
tapi_iscsi_set_encoding_format(const char *ta,
                               int fmt)
{
    assert(ta != NULL && (fmt == 0 || fmt == 1));

    return cfg_set_instance_fmt(CVT_INTEGER, (void *)fmt,
                                "/agent:%s/iscsi_target:/chap:/b:",
                                ta);
}

int
tapi_iscsi_set_tgt_auth_req(const char *ta,
                            int tgt_auth)
{
    assert(ta != NULL && (tgt_auth == 0 || tgt_auth == 1));

    return cfg_set_instance_fmt(CVT_INTEGER, (void *)tgt_auth,
                                "/agent:%s/iscsi_target:/chap:/t:",
                                ta);
}

int
tapi_iscsi_set_security_negotiations_phase(const char *ta,
                                           int use)
{
    assert(ta != NULL && (use == 0 || use == 1));

    return cfg_set_instance_fmt(CVT_INTEGER, (void *)use,
                                "/agent:%s/iscsi_target:/chap:",
                                ta);
}
