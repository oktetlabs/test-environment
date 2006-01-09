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

#define TE_LGR_USER "TAPI iSCSI"

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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "rcf_api.h"
#include "rcf_common.h"
#include "conf_api.h"
#include "logger_api.h"

#include "tapi_iscsi.h"
#include "ndn_iscsi.h"

#include "tapi_tad.h"
#include "te_iscsi.h"
#include "tapi_tcp.h"
#include "tapi_test.h"
#include "tapi_rpc.h"
#include "tapi_file.h"

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

/* See description in tapi_iscsi.h */
te_errno
tapi_iscsi_sock_csap_create(const char        *ta_name,
                            int                socket,
                            iscsi_digest_type  hdr_dig,
                            iscsi_digest_type  data_dig,
                            csap_handle_t     *csap)
{
    asn_value  *csap_spec = NULL;
    te_errno    rc = 0;
    int         syms;

    if (ta_name == NULL || socket < 0 || csap == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(asn_parse_value_text("{ iscsi:{} }",
                                  ndn_csap_spec, &csap_spec, &syms));
    CHECK_RC(asn_write_int32(csap_spec, socket, "0.#iscsi.socket"));
    CHECK_RC(asn_write_int32(csap_spec, hdr_dig, "0.#iscsi.header-digest"));
    CHECK_RC(asn_write_int32(csap_spec, data_dig, "0.#iscsi.data-digest"));

    rc = tapi_tad_csap_create(ta_name, 0 /* sid */, "iscsi", 
                              csap_spec, csap); 
    if (rc != 0)
        ERROR("%s(): csap create failed, rc %X", __FUNCTION__, rc);

    asn_free_value(csap_spec);

    return rc;
}


/* See description in tapi_iscsi.h */
te_errno
tapi_iscsi_tgt_csap_create(const char        *ta_name,
                           iscsi_digest_type  hdr_dig,
                           iscsi_digest_type  data_dig,
                           csap_handle_t     *csap)
{
    te_errno    rc;
    int         sock;

    rc = rcf_ta_call(ta_name, 0 /* sid */,
                     "iscsi_target_start_rx_thread", &sock, 0, FALSE);
    if (rc != 0)
    {
        ERROR("Failed to call iscsi_target_start_rx_thread() on TA "
              "'%s': %r", ta_name, rc);
        return rc;
    }
    if (sock < 0)
    {
        ERROR("iscsi_target_start_rx_thread() on TA '%s' failed");
        return TE_RC(TE_TAPI, TE_EFAULT);
    }

    return tapi_iscsi_sock_csap_create(ta_name, sock, hdr_dig, data_dig,
                                       csap);
}


/* See description in tapi_iscsi.h */
te_errno
tapi_iscsi_ini_csap_create(const char        *ta_name,
                           int                sid,
                           csap_handle_t      listen_csap,
                           int                timeout,
                           iscsi_digest_type  hdr_dig,
                           iscsi_digest_type  data_dig,
                           csap_handle_t     *csap)
{
    te_errno    rc; 
    int         ini_socket; 

    rc = tapi_tcp_server_recv(ta_name, sid, listen_csap, 
                              timeout, &ini_socket);
    if (rc != 0)
    {
        WARN("%s(): wait for accepted socket failed: %r", 
             __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    }

    return tapi_iscsi_sock_csap_create(ta_name, ini_socket,
                                       hdr_dig, data_dig, csap);
}



struct iscsi_data_message {
    iscsi_target_params_t *params;

    uint8_t *data;
    size_t   length;
    int      error;
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
        ERROR("%s(): parse packet fails, rc = %r, sym %d, pkt file: %Tf",
              __FUNCTION__, rc, s_parsed, pkt_fname);
        msg->error = rc; 
        return;
    }

    len = asn_get_length(pkt, "payload.#bytes");

    if (len > msg->length)
        WARN("%s(): length %u of message greater then buffer %u",
             __FUNCTION__, (unsigned)len, (unsigned)msg->length);

    len = msg->length;
    rc = asn_read_value_field(pkt, msg->data, &len, "payload.#bytes");
    if (rc != 0)
    {
        msg->error = rc;
        ERROR("%s(): read payload failed %r", __FUNCTION__, rc);
    }
    msg->length = len;

    if (msg->params != NULL)
        asn_read_int32(pkt, &(msg->params->param), "pdus.0.#iscsi.param");

    asn_free_value(pkt);
}

/* See description in tapi_iscsi.h */
int
tapi_iscsi_recv_pkt(const char *ta_name, int sid, csap_handle_t csap,
                    int timeout, csap_handle_t forward,
                    iscsi_target_params_t *params,
                    uint8_t *buffer, size_t  *length)
{
    asn_value *pattern = NULL;
    struct iscsi_data_message msg;

    int rc = 0, syms, num;

    if (ta_name == NULL || socket == NULL)
        return TE_EWRONGPTR;

    msg.error  = 0;

    if (buffer != NULL)
    {
        if (length == NULL)
            return TE_EWRONGPTR;

        RING("%s(): called with length %d", __FUNCTION__, *length);
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
        rc = asn_write_int32(pattern, forward, "0.actions.0.#forw-pld");
        if (rc != 0)
        {
            ERROR("%s():  write forward csap failed: %r",
                  __FUNCTION__, rc);
            goto cleanup;
        }
    }

    rc = tapi_tad_trrecv_start(ta_name, sid, csap, pattern, timeout, 1,
                               buffer == NULL ? RCF_TRRECV_COUNT
                                              : RCF_TRRECV_PACKETS);
    if (rc != 0)
    {
        ERROR("%s(): trrecv_start failed %r", __FUNCTION__, rc);
        goto cleanup;
    }

    rc = rcf_ta_trrecv_wait(ta_name, sid, csap,
                            buffer == NULL ? NULL : iscsi_msg_handler,
                            buffer == NULL ? NULL : &msg, &num);
    if (rc != 0)
        WARN("%s() trrecv_wait failed: %r", __FUNCTION__, rc);

    if (buffer != NULL)
        *length = msg.length;

    if (msg.error != 0)
    {
        rc = msg.error;
        ERROR("%s(): iscsi callback failed: %r", __FUNCTION__, rc);
    }

cleanup:
    asn_free_value(pattern);
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

int
tapi_iscsi_send_pkt_last(const char   *ta_name, int sid, 
                         csap_handle_t csap, uint8_t *buffer,
                         size_t        length)
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

    rc = asn_write_value_field(template, NULL, 0,
                               "pdus.0.#iscsi.last-data");
    if (rc != 0)
    {
        ERROR("%s(): write last-data flag failed %r", __FUNCTION__, rc);
        goto cleanup;
    } 

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
tapi_iscsi_exchange_until_silent(const char *ta, int session, 
                                 csap_handle_t csap_a,
                                 csap_handle_t csap_b,
                                 unsigned int timeout)
{
    int         rc = 0, syms;
    asn_value  *pattern = NULL;
    unsigned    pkts_a = 0, pkts_b = 0, 
                prev_pkts_a, prev_pkts_b;

    struct timespec ts_timeout;

    if (csap_a == CSAP_INVALID_HANDLE || 
        csap_b == CSAP_INVALID_HANDLE)
    {

        ERROR("%s(): both CSAPs should be valid", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = asn_parse_value_text("{{pdus { iscsi:{} } }}",
                              ndn_traffic_pattern, &pattern, &syms);
    if (rc != 0)
    {
        ERROR("%s(): parse ASN csap_spec failed %X, sym %d", 
              __FUNCTION__, rc, syms);
        return rc;
    } 

    /*First, start receive on A */
    asn_write_int32(pattern, csap_b, "0.actions.0.#forw-pld");

    rc = tapi_tad_trrecv_start(ta, session, csap_a, pattern, 
                               TAD_TIMEOUT_INF, 0, RCF_TRRECV_COUNT);
    if (rc != 0)
    {
        ERROR("%s(): trrecv_start failed %r", __FUNCTION__, rc);
        goto cleanup;
    }

    /* Then, start receive on B */
    asn_write_int32(pattern, csap_a, "0.actions.0.#forw-pld");

    rc = tapi_tad_trrecv_start(ta, session, csap_b, pattern, 
                               TAD_TIMEOUT_INF, 0, RCF_TRRECV_COUNT);
    if (rc != 0)
    {
        ERROR("%s(): trrecv_start failed %r", __FUNCTION__, rc);
        goto cleanup;
    }

    do { 
        prev_pkts_a = pkts_a;
        prev_pkts_b = pkts_b;

        ts_timeout.tv_sec  = timeout / 1000;
        ts_timeout.tv_nsec = (timeout % 1000) * 1000000;
        INFO("%s(): Sleeping %u milliseconds", __FUNCTION__, timeout);
        if (nanosleep(&ts_timeout, NULL) != 0)
        {
            INFO("Sleep interrupted");
            rc = TE_OS_RC(TE_TAPI, errno);
            break;
        }

        rc = rcf_ta_trrecv_get(ta, session, csap_a, NULL, NULL, &pkts_a);
        if (rc != 0)
        {
            ERROR("%s(): trrecv_get on A failed %r", __FUNCTION__, rc);
            goto cleanup;
        }
        rc = rcf_ta_trrecv_get(ta, session, csap_b, NULL, NULL, &pkts_b);
        if (rc != 0)
        {
            ERROR("%s(): trrecv_get on B failed %r", __FUNCTION__, rc);
            goto cleanup;
        } 
        INFO("%s(): a %d, b %d, new a %d, new b %d", 
             __FUNCTION__, prev_pkts_a, prev_pkts_b, pkts_a, pkts_b);

    } while(prev_pkts_a < pkts_a || prev_pkts_b < pkts_b);

    tapi_tad_trrecv_stop(ta, session, csap_a, NULL, &pkts_a);
    tapi_tad_trrecv_stop(ta, session, csap_b, NULL, &pkts_b);

cleanup:
    asn_free_value(pattern);
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
        INFO("There is no key %s in Segment Data", name);
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
int
tapi_iscsi_get_key_value(iscsi_key_values values, 
                         int key_value_index, char **str)
{
    int rc;
    
    const asn_value     *elem;

    if ((rc = asn_get_indexed(values, &elem, key_value_index)) != 0)
    {
        ERROR("%s, %d: cannot get value, %r",
              __FUNCTION__, __LINE__, rc);
        return rc;
    }
    if ((rc = asn_read_string(elem, str, "")) != 0)
    {
        ERROR("%s, %d: cannot read string, %r",
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

    int i;

    char *str_val;

    va_start(list, num);
    
    if ((key_values = asn_init_value(ndn_iscsi_key_values)) == NULL)
    {
        ERROR("%s, %d: cannot init asn_value",
              __FUNCTION__, __LINE__);
        goto cleanup;
    }

    for (i = 0; i < num; i++)
    {
        if ((key_value = asn_init_value(asn_base_charstring)) == NULL)
        {
            ERROR("%s, %d: cannot init asn_value",
                  __FUNCTION__, __LINE__);
            rc = -1;
            goto cleanup;
        }

        str_val = va_arg(list, char *);
        if ((rc = asn_write_string(key_value, 
                                   str_val, "")) != 0)
        {
            ERROR("%s, %d: cannot write string value, %r",
                  __FUNCTION__, __LINE__, rc);
            goto cleanup;
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
           
/* See description in tapi_iscsi.h */
void
tapi_iscsi_keys_data_free(iscsi_segment_data segment_data)
{
    asn_free_value(segment_data);
    return;
}

/* See description in tapi_iscsi.h */
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

        if ((key_value = asn_init_value(asn_base_charstring)) == NULL)
        {
            ERROR("%s, %d: cannot init key value",
                  __FUNCTION__, __LINE__);
            return -1;
        }
        if ((rc = asn_write_string(key_value,
                                   str, "")) != 0)
        {
            ERROR("%s, %d: cannot write string",
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


/* See description in tapi_iscsi.h */
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
        found = FALSE;
        for (key_values_index = 0;
             (key_values_index < key_values_num) && (found == FALSE);
             key_values_index++)
        {
            char *search_value = va_arg(list, char *);
            char *key_value;

            if ((rc = 
                 tapi_iscsi_get_key_value(
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
        }
        if (found == FALSE)
        {
            va_end(list);
            ERROR("%s, %d: cannot find value for key %s",
                  __FUNCTION__, __LINE__, key_name);
            return -1;
        }
    }    
    va_end(list);
    return 0;
}


/* See description in tapi_iscsi.h */
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

/* See description in tapi_iscsi.h */
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

/* See description in tapi_iscsi.h */
int
tapi_iscsi_key_value_read(iscsi_key_values val_array,
                          int val_index, char *buf, size_t *buf_len)
{
    int rc;

    const asn_value *key_value;

    if ((rc = asn_get_indexed(val_array, &key_value, val_index)) != 0)
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

/* See description in tapi_iscsi.h */
int
tapi_iscsi_key_value_write(iscsi_key_values val_array,
                           int val_index, const char *string)
{
    asn_value       *key_value;
    int              rc;

    if (string == NULL)
    {
        rc = asn_remove_indexed(val_array, val_index, "");
        if (rc != 0)
            ERROR("%s(): asn_remove_indexed failed %r", __FUNCTION__, rc);
        return rc;
    }

    if ((rc = asn_get_indexed(val_array, (const asn_value **)&key_value, 
                              val_index)) != 0)
    {
        ERROR("%s(): asn_get_indexed failed %r", __FUNCTION__, rc);
        return rc;
    }

    if ((rc = asn_write_string(key_value, string, "")) != 0)
    {
        ERROR("%s(): cannot write key value %d, %r",
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
    
    INFO("Setting %s to %s on %s:%d", key, value, ta, id);
    local_rc = rcf_ta_call(ta, 0, "iscsi_set_custom_value", &remote_rc,
                           3, FALSE, 
                           RCF_INT32, id,
                           RCF_STRING, key, RCF_STRING, value);
    return local_rc != 0 ? local_rc : 
        (remote_rc != 0 ? TE_RC(TE_TAPI, TE_ESRCH) : 0);
}

int
tapi_iscsi_target_customize_intval(const char *ta, int id, 
                                   const char *key, int value)
{
    char buf[16];
    
    snprintf(buf, sizeof(buf), "%d", value);
    return tapi_iscsi_target_customize(ta, id, key, buf);
}

int
tapi_iscsi_target_cause_logout(const char *ta, int id, int timeout)
{
    int  rc;
    char buf[16];
    
    sprintf(buf, "%d", timeout);
    rc = tapi_iscsi_target_customize(ta, id, "async_logout_timeout", buf);
    if (rc != 0)
        return rc;
    return tapi_iscsi_target_customize(ta, id, "send_async", 
                                       "logout_request");
}

int
tapi_iscsi_target_cause_renegotiate(const char *ta, int id, int timeout)
{
    int  rc;
    char buf[16];
    
    sprintf(buf, "%d", timeout);
    rc = tapi_iscsi_target_customize(ta, id, "async_text_timeout", buf);
    if (rc != 0)
        return rc;
    return tapi_iscsi_target_customize(ta, id, "send_async", "renegotiate");
}


int
tapi_iscsi_target_will_drop(const char *ta, int id, te_bool drop_all, 
                            int time2wait, int time2retain)
{
    int  rc;
    char buf[16];
    
    sprintf(buf, "%d", time2wait);
    rc = tapi_iscsi_target_customize(ta, id, "async_drop_time2wait", buf);
    if (rc != 0)
        return rc;
    sprintf(buf, "%d", time2retain);
    rc = tapi_iscsi_target_customize(ta, id, "async_drop_time2retain", buf);
    if (rc != 0)
        return rc;

    return tapi_iscsi_target_customize(ta, id, "send_async", 
                                       drop_all ? "drop_all_connections" : 
                                       "drop_connection");
}



int
tapi_iscsi_initiator_advertize_set(const char *ta,
                                   iscsi_target_id target_id,
                                   iscsi_cid cid,
                                   tapi_iscsi_parameter param,
                                   te_bool advertize)
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

    char         *offer = NULL;
    int          par2adv = 0;
    int          rc;
    cfg_val_type type = CVT_STRING;

    rc = cfg_get_instance_fmt(&type, &offer,
                              "/agent:%s/iscsi_initiator:/target_data:"
                              "target_%d/conn:%d/parameters2advertize:",
                              ta, target_id, cid);
    if (rc != 0)
    {
        ERROR("Failed to get current parameters2advertize: "
              "/agent:%s/iscsi_initiator:/target_data:"
              "target_%d/conn:%d/parameters2advertize:",
              ta, target_id, cid);
        return rc;
    }

    par2adv = atoi(offer);
    free(offer);
    
    if (offer_mapping[param] != 0)
    {
        if (advertize == TRUE)
            par2adv |= offer_mapping[param];
        else
            par2adv &= ~(offer_mapping[param]);
    }
    
#define MAX_OFFER_LENGTH 15
    offer = calloc(MAX_OFFER_LENGTH, 1);
#undef MAX_OFFER_LENGTH

    sprintf(offer, "%d", par2adv);

    rc = cfg_set_instance_fmt(CVT_STRING, offer,
                              "/agent:%s/iscsi_initiator:/target_data:"
                              "target_%d/conn:%d/parameters2advertize:",
                              ta, target_id, cid);
    if (rc != 0)
    {
        ERROR("Failed to set current parameters2advertize");
        return rc;
    }
    
    free(offer);
    return 0;
}

/* see description in tapi_iscsi.h */
int 
tapi_iscsi_initiator_set_parameter(const char *ta,
                                   iscsi_target_id target_id,
                                   iscsi_cid cid,
                                   tapi_iscsi_parameter param,
                                   const char *value, 
                                   te_bool advertize)
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
    
    INFO("Set %s (%s, target=%d, cid=%d param=%d) to %s, %s advertizing", 
         log_mapping[param], 
         ta, target_id, cid, param, value, advertize ? "with":"without");
    

    if (cid == ISCSI_ALL_CONNECTIONS)
    {
        rc = cfg_set_instance_fmt(CVT_STRING, value,
                                  "/agent:%s/iscsi_initiator:/target_data:"
                                  "target_%d/%s",
                                  ta, target_id, mapping[param]);
    }
    else
    {
        if (strncmp(mapping[param], "chap:/",
                    strlen("chap:/")) != 0)
        {
            rc = tapi_iscsi_initiator_advertize_set(ta, target_id, cid,
                                                    param, 
                                                    advertize);
            if (rc != 0)
            {
                ERROR("Failed to set %sadvertize for the parameter %s",
                      advertize ? "":"not ", log_mapping[param]);
                return rc;
            }
        }

        rc = cfg_set_instance_fmt(CVT_STRING, value,
                                  "/agent:%s/iscsi_initiator:/target_data:"
                                  "target_%d/conn:%d/%s",
                                      ta, target_id, cid, mapping[param]);
    }

    if (rc != 0)
    {
        ERROR("Failed to set %s parameter to %s, cid=%d, rc = %d (%r)",
              log_mapping[param], value, cid, rc, rc);
        
        return rc;
    }

    return 0;
}

#define MAX_INI_CMD_SIZE 10
#define MAX_TARGETS_NUMBER 10
#define MAX_CONNECTION_NUMBER 100

static int iscsi_current_cid[MAX_TARGETS_NUMBER];
static int iscsi_current_target = 0;

/* see description in tapi_iscsi.h */
iscsi_cid 
tapi_iscsi_initiator_conn_add(const char *ta,
                              iscsi_target_id tgt_id)
{
    int        rc;
    cfg_handle handle;

    rc = cfg_add_instance_fmt(&handle, CVT_STRING,
                              "",
                              "/agent:%s/iscsi_initiator:/"
                              "target_data:target_%d"
                              "/conn:%d",
                              ta, tgt_id, iscsi_current_cid[tgt_id]);
    if (rc != 0)
    {
        ERROR("Failed to add connection instance to the initiator");
        return -rc;
    }

    return iscsi_current_cid[tgt_id]++;
}

int
tapi_iscsi_initiator_conn_establish(const char *ta,
                                    iscsi_target_id tgt_id,
                                    iscsi_cid cid)
{
    int  rc;
    char cmd[10];
    
    sprintf(cmd, "%d", cid);

    INFO("Setting: /agent:%s/iscsi_initiator:/target_data:"
         "target_%d/conn:%d/cid:", ta, tgt_id, cid);

    rc = cfg_set_instance_fmt(CVT_STRING, (void *)cmd,
                              "/agent:%s/iscsi_initiator:/target_data:"
                              "target_%d/conn:%d/cid:", ta,
                              tgt_id, cid);
    if (rc != 0)
    {
        ERROR("Failed to establish the connection "
              "with cid=%d for target %d", cid, tgt_id);
        return rc;
    }

    return 0;
}

int
tapi_iscsi_initiator_conn_down(const char *ta,
                               iscsi_target_id tgt_id,
                               iscsi_cid cid)
{
    int  rc;
    char cmd[10];

    sprintf(cmd, "%d", ISCSI_CONNECTION_DOWN);

    rc = cfg_set_instance_fmt(CVT_STRING, (void *)cmd,
                              "/agent:%s/iscsi_initiator:/target_data:"
                              "target_%d/conn:%d/cid:", ta,
                              tgt_id,
                              cid);
    if (rc != 0)
    {
        ERROR("Failed to down the connection");
        return rc;
    }
    
    return 0;
}

int 
tapi_iscsi_initiator_conn_del(const char *ta,
                              iscsi_target_id tgt_id,
                              iscsi_cid cid)
{
    int        rc;
    cfg_handle handle;

    rc = cfg_find_fmt(&handle, "/agent:%s/iscsi_initiator:/target_data:"
                      "target_%d/conn:%d", ta, tgt_id, cid);
    if (rc != 0)
    {
        ERROR("No connection with such ID");
        return rc;
    }

    rc = cfg_del_instance(handle, FALSE);
    if (rc != 0)
    {
        ERROR("Failed to delete connection with ID %d from agent %s",
              tgt_id, ta);
    }
    iscsi_current_cid[tgt_id]--;

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
    int   target_port       = 0;

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

    INFO("Initiator (%s): add Target: addr=%s, port=%d",
         ta, target_addr_param, target_port);
    
    rc = cfg_add_instance_fmt(&handle, CVT_STRING,
                        "",
                        "/agent:%s/iscsi_initiator:/target_data:target_%d",
                        ta, iscsi_current_target);
    if (rc != 0)
    {
        ERROR("Failed to add target_data instance to the initiator, rc=%r");
        return -rc;
    }

    rc = tapi_iscsi_initiator_set_parameter(ta,
                                            iscsi_current_target,
                                            ISCSI_ALL_CONNECTIONS,
                                            ISCSI_PARAM_TARGET_ADDRESS,
                                            (void *)target_addr_param,
                                            FALSE);
    if (rc != 0)
    {
        ERROR("Failed to set TargetAddress parameter of "
              "the target rc = %r", 
              rc);
        return -rc;
    }

    sprintf(port, "%d", target_port);
    rc = tapi_iscsi_initiator_set_parameter(ta,
                                            iscsi_current_target,
                                            ISCSI_ALL_CONNECTIONS,
                                            ISCSI_PARAM_TARGET_PORT,
                                            (void *)port,
                                            FALSE);
    if (rc != 0)
    {
        ERROR("Failed to set TargetPort aprameter of the initiator, rc=%r");
        return -rc;
    }

    iscsi_current_cid[(int)iscsi_current_target] = 0;

    VERB("Target with ID=%d added to Initiator on "
         "agent %s, addr=%s, port=%d",
         iscsi_current_target, ta,
         target_addr_param, target_port);
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
        "TargetAuth",
        "AuthMethod",
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

iscsi_digest_type 
iscsi_digest_str2enum(const char *digest_type)
{
    if (strcmp(digest_type, "CRC32C") == 0)
        return ISCSI_DIGEST_CRC32C;

    return -1;
}

char* 
iscsi_digest_enum2str(iscsi_digest_type digest_type)
{
    static char *digest_map[] = {
        "None",
        "CRC32C",
    };

    return digest_map[digest_type];
}

#undef MAX_INI_CMD_SIZE


/*** Functions for data transfer between Target and Initiator ***/


static char *
get_target_mountpoint(void)
{
    static char mountpoint[64];
    snprintf(mountpoint, sizeof(mountpoint), 
             "/tmp/te_target_fs.%lu", 
            (unsigned long)getpid());
    return mountpoint;
}

te_errno
tapi_iscsi_target_inform_new_test(const char *ta)
{
    int  unused;

    return rcf_ta_call(ta, 0, "iscsi_start_new_session_group", 
                       &unused,
                       0, FALSE);
}

te_errno
tapi_iscsi_target_mount(const char *ta)
{
    int  rc;
    int  unused;

    rc = rcf_ta_call(ta, 0, "iscsi_sync_device", &unused,
                     2, FALSE,
                     RCF_UINT8, 0, RCF_UINT8, 0);
    if (rc != 0)
        return rc;

    return cfg_set_instance_fmt(CVT_STRING, 
                                get_target_mountpoint(),
                                "/agent:%s/iscsi_target:/backing_store_mp:",
                                ta);    
}


te_errno
tapi_iscsi_target_unmount(const char *ta)
{
    return cfg_set_instance_fmt(CVT_STRING, 
                                get_target_mountpoint(),
                                "/agent:%s/iscsi_target:/backing_store_mp:",
                                ta);
}

static te_bool
check_mounted(const char *ta)
{
    cfg_val_type type = CVT_STRING;
    char         mountpoint[64];
    char        *expected_mountpoint;
    int          rc;

    expected_mountpoint = get_target_mountpoint();
    rc  = cfg_get_instance_fmt(&type, mountpoint,
                               "/agent:%s/iscsi_target:/backing_store_mp:",
                               ta);
    return (rc != 0 || 
            strcmp(mountpoint, expected_mountpoint) != 0);
}

static te_errno
multiply_file_content(const char *fname, int multiply,
                      const void *data, size_t length)
{
    int fd = open(fname, O_WRONLY | O_APPEND, 0);

    ssize_t  result_len = 0;
    te_errno status     = 0;
    
    if (fd < 0)
        return TE_OS_RC(TE_TAPI, errno);
    for (; multiply != 0; multiply--)
    {
        result_len = write(fd, data, length);
        if (result_len != 0)
        {
            status = (result_len < 0 ? TE_OS_RC(TE_TAPI, errno) : 
                      TE_RC(TE_TAPI, TE_ENOSPC));
            break;
        }
    }
    close(fd);
    return status;
}

int
tapi_iscsi_target_file_read(const char *ta, const char *fname,
                            void *buf, size_t length)
{
    const char *localfname;
    char        source[128];
    int         rc;
    int         fd;
    ssize_t     result_len;

    snprintf(source, sizeof(source),
             "%s/%s", 
             get_target_mountpoint(),
             fname);

    if (!check_mounted(ta))
        return TE_RC(TE_TAPI, TE_ENXIO);
    localfname = tapi_file_generate_pathname();
    if (localfname == NULL)
        return TE_RC(TE_TAPI, TE_EBADF);
    rc = rcf_ta_get_file(ta, 0, source, localfname);
    if (rc != 0)
        return rc;
    fd = open(localfname, O_RDONLY, 0);
    if (fd < 0)
        return TE_OS_RC(TE_TAPI, errno);
    result_len = read(fd, buf, length);
    if (result_len < 0)
        rc = TE_OS_RC(TE_TAPI, errno);
    else
        rc = ((size_t)result_len == length ? 0 : TE_RC(TE_TAPI, TE_EFAIL));
    close(fd);
    if (remove(localfname) != 0)
        ERROR("Cannot remove '%s': %s", localfname, strerror(errno));
    return rc;
}

int 
tapi_iscsi_target_file_write(const char *ta, const char *fname,
                             const void *buf, size_t length,
                             size_t multiply)
{
    const char *localfname;
    char        destination[128];
    int         rc;

    snprintf(destination, sizeof(destination),
             "%s/%s", 
             get_target_mountpoint(),
             fname);

    if (!check_mounted(ta))
        return TE_RC(TE_TAPI, TE_ENXIO);
    /** tapi_file_create() may modify buf, but only if the
     *  third parameter is TRUE, i.e. randomize the buffer
     */
    localfname = tapi_file_create(length, (void *)buf, FALSE);
    if (localfname == NULL)
        return TE_RC(TE_TAPI, TE_EBADF);
    if (multiply > 1)
        multiply_file_content(localfname, multiply - 1, buf, length);

    rc = rcf_ta_put_file(ta, 0, localfname, destination);
    if (remove(localfname) != 0)
        ERROR("Cannot remove '%s': %s", localfname, strerror(errno));
    return rc;
}


int
tapi_iscsi_target_raw_write(const char *ta, off_t offset,
                            const void *data, size_t length,
                            size_t multiply)
{
    int rc;
    int result;
    const char *localfname; 
    const char *remotefname;

    remotefname = tapi_file_generate_name();
    if (remotefname == NULL)
        return TE_RC(TE_TAPI, TE_EBADF);
    /** tapi_file_create() may modify buf, but only if the
     *  third parameter is TRUE, i.e. randomize the buffer
     */
    localfname = tapi_file_create(length, (void *)data, FALSE);
    if (localfname == NULL)
        return TE_RC(TE_TAPI, TE_EBADF);

    if (multiply > 1)
        multiply_file_content(localfname, multiply, data, length);

    rc = rcf_ta_put_file(ta, 0, localfname, remotefname);
    if (rc != 0)
    {
        remove(localfname);
        return rc;
    }

    rc = rcf_ta_call(ta, 0 /* sid */,
                     "iscsi_write_to_device", &result, 5, FALSE,
                     RCF_UINT8, 0, RCF_UINT8, 0,
                     RCF_UINT32, offset, 
                     RCF_STRING, remotefname,
                     RCF_UINT32, length);
    return rc == 0 ? result : rc;
}

int
tapi_iscsi_target_raw_read(const char *ta, off_t offset,
                           void *data, size_t length)
{
   int rc;
   int result;
   int fd;
    
   ssize_t result_len;

   const char *localfname; 
   const char *remotefname;
   
   remotefname = tapi_file_generate_name();
   if (remotefname == NULL)
       return TE_RC(TE_TAPI, TE_EBADF);
   localfname = tapi_file_generate_name();
   if (localfname == NULL)
       return TE_RC(TE_TAPI, TE_EBADF);

   rc = rcf_ta_call(ta, 0 /* sid */,
                    "iscsi_read_from_device", &result, 5, FALSE,
                    RCF_UINT8, 0, RCF_UINT8, 0,
                    RCF_UINT32, offset, 
                    RCF_STRING, remotefname, 
                    RCF_UINT32, length);
   if (rc != 0)
       return rc;
   if (result != 0)
       return rc;
   rc = rcf_ta_get_file(ta, 0, remotefname, localfname);
   rcf_ta_del_file(ta, 0, remotefname);
   if (rc != 0)
       return rc;
   fd = open(localfname, O_RDONLY);
   remove(localfname);    
   if (fd < 0)
   {
       rc = TE_OS_RC(TE_TAPI, errno);
       return rc;
   }
   result_len = read(fd, data, length);
   if (result_len < 0)
       rc = TE_OS_RC(TE_TAPI, errno);
   else 
       rc = ((size_t)result_len == length ? 0 : TE_RC(TE_TAPI, TE_EFAIL));
   close(fd);
   return rc;
}

/** Asynchronous requests handler type */
typedef te_errno (*iscsi_io_command_t)(iscsi_io_handle_t *ioh,
                                       int *fd, void *data, 
                                       ssize_t length);

typedef void (*iscsi_io_data_destroy_t)(void *);

/** Asynchronous iSCSI I/O request */
typedef struct iscsi_io_cmd
{
    iscsi_io_command_t       cmd; /**< Command handler */
    volatile te_errno        status; /**< Status */
    int                      fd; /**< File descriptor to operate on */
    ssize_t                  length; /**< Data length or seek position */
    te_bool                  spread_fd; /**< If TRUE, current fd is used for
                                      later commands */
    te_bool                  do_signal; /**< Signal when a task is done */
    te_bool                  is_complete; /**< TRUE after the task
                                           * completion */
    te_bool                  leader; /**< Leading command */
    void                    *data; /**< Pointer to data */
    iscsi_io_data_destroy_t  destroy; /**< Data destruction function */
} iscsi_io_cmd_t;

#define MAX_ISCSI_IO_CMDS  16

/** Asynchronous iSCSI I/O channel */
struct iscsi_io_handle_t
{
    pthread_t       thread;     /**< A thread id that does all the stuff */
    rcf_rpc_server *rpcs;       /**< RPC server handler for actual I/O */
    te_bool         use_signal; /**< TRUE if a signal is to be sent when an
                                     operation is complete */
    te_bool         use_fs;     /**< TRUE when using filesystem on the
                                     device, as opposed to raw access */
    size_t          chunksize;  /**< Chunk size for file copying */
    size_t          bufsize;    /**< Current buffer size */
    tarpc_ptr       buffer;     /**< Aligned buffer */
    iscsi_io_cmd_t  cmds[MAX_ISCSI_IO_CMDS]; /**< Buffer for requests */
    sem_t           cmd_wait;   /**< Posted when a new request is added */
    int             next_cmd;   /**< Next request number */
    char            agent[RCF_MAX_NAME]; /**< Agent name */
    char            mountpoint[RCF_MAX_NAME]; /**< A mount point on the
                                                 initiator's side */
    char            device[RCF_MAX_NAME]; /**< A SCSI device associated with
                                             an iSCSI initiator */
};


/**
 * Get a SCSI device corresponding to a given iSCSI session
 * 
 * @param ta    Test Agent name
 * @param id    session id
 * 
 * @return a pointer to a malloc'ed string containing the device name
 */
static char *
get_host_device(const char *ta, unsigned id)
{
    cfg_val_type type = CVT_STRING;
    char        *device;
    int          rc;

    rc = cfg_get_instance_fmt(&type, &device,
                              "/agent:%s/iscsi_initiator:"
                              "/target_data:target_%d/host_device:",
                              ta, id);
    if (rc != 0)
    {
        ERROR("Cannot obtain host device string: %r", rc);
        return NULL;
    }

    VERB("Got device list from the Initiator: %s", device);

    return device;
}

te_bool
tapi_iscsi_initiator_is_device_ready(const char *ta, iscsi_target_id id)
{
    char    *dev = get_host_device(ta, id);
    te_bool  result;
    
    if (dev == NULL)
        return FALSE;
    result = (*dev != '\0');
    free(dev);
    return result;
}

#define ISCSI_IO_SIGNAL        SIGPOLL

static void
rpc_server_destructor (void *arg)
{
    iscsi_io_handle_t *ioh = arg;
    
    if (ioh->buffer != RPC_NULL)
        rpc_free(ioh->rpcs, ioh->buffer);
    rcf_rpc_server_destroy(ioh->rpcs);
}

/**
 * The thread routine that interacts with a TA 
 * when performing I/O operations
 */
static void *
tapi_iscsi_io_thread(void *param)
{
    iscsi_io_handle_t *ioh = param;
    iscsi_io_cmd_t    *cmd;
    int                i;
    sigset_t           mask;

    sigemptyset(&mask);
    sigaddset(&mask, ISCSI_IO_SIGNAL);
    sigaddset(&mask, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
    pthread_cleanup_push(rpc_server_destructor, ioh);
    for (;;)
    {
        sem_wait(&ioh->cmd_wait);
        cmd = NULL;
        for (i = 0; i < MAX_ISCSI_IO_CMDS; i++)
        {
            if (ioh->cmds[i].cmd != NULL)
            {
                cmd = &ioh->cmds[i];
                break;
            }
        }
        if (cmd != NULL)
        {
            iscsi_io_command_t op = cmd->cmd;
            
            cmd->cmd    = NULL;
            RING("Executing task %d: fd = %d, data = %p, length = %lu",
                 i, cmd->fd, cmd->data, (unsigned long)cmd->length);
            if (cmd->leader || cmd->status == 0)
            {
                int next;

                cmd->status = op(ioh, &cmd->fd, cmd->data, cmd->length);
                RING("I/O Task status for task %d is %r, fd = %d", 
                     i, cmd->status, cmd->fd);
                if (cmd->spread_fd)
                {
                    if (cmd->status == 0)
                    {
                        for (next = i + 1; next < MAX_ISCSI_IO_CMDS; next++)
                        {
                            ioh->cmds[next].fd = cmd->fd;
                        }
                    }
                }

                if (cmd->status != 0)
                {
                    for (next = i + 1; next < MAX_ISCSI_IO_CMDS; next++)
                    {
                        if (ioh->cmds[next].leader)
                            break;
                        ioh->cmds[next].status = cmd->status;
                    }
                }
            }
            cmd->is_complete = TRUE;
            if (ioh->use_signal && cmd->do_signal)
            {
                RING("Sending task completion signal");
                kill(getpid(), ISCSI_IO_SIGNAL);
            }
            pthread_testcancel();
        }
    }
    pthread_cleanup_pop(1);
}

static te_errno
command_open(iscsi_io_handle_t *ioh, int *fd, 
             void *data, ssize_t length)
{
    *fd = rpc_open(ioh->rpcs, data, length | RPC_O_DIRECT,
                   RPC_S_IREAD | RPC_S_IWRITE);
    return (*fd < 0 ? RPC_ERRNO(ioh->rpcs) : 0);
}

static te_bool
realloc_buffer(iscsi_io_handle_t *ioh, size_t bufsize)
{
    if (ioh->bufsize < bufsize)
    {
        if (ioh->buffer != RPC_NULL)
            rpc_free(ioh->rpcs, ioh->buffer);
        ioh->buffer = rpc_memalign(ioh->rpcs, 512, bufsize);
    }
    return ioh->buffer != RPC_NULL;
}

static te_errno
command_close(iscsi_io_handle_t *ioh, int *fd, 
              void *data, ssize_t length)
{
    UNUSED(data);
    UNUSED(length);
    if (*fd < 0)
        return TE_RC(TE_TAPI, TE_EBADF);
    else
    {
        return rpc_close(ioh->rpcs, *fd) == 0 ? 0 :
            RPC_ERRNO(ioh->rpcs);
    }
}

static te_errno
command_seek(iscsi_io_handle_t *ioh, int *fd, 
             void *data, ssize_t length)
{
    off_t result = rpc_lseek(ioh->rpcs, *fd, 
                             (off_t)length, RPC_SEEK_SET);
    UNUSED(data);
    return (result == (off_t)-1 ? RPC_ERRNO(ioh->rpcs) : 0);
}
 
static te_errno
command_read(iscsi_io_handle_t *ioh, int *fd, 
                void *data, ssize_t length)
{
    ssize_t   result_len;
    te_errno  status;
         
    if (!realloc_buffer(ioh, length))
        return RPC_ERRNO(ioh->rpcs);

    result_len = rpc_readbuf(ioh->rpcs, *fd, ioh->buffer, length);
    status = (result_len < 0 ? RPC_ERRNO(ioh->rpcs) : 
              (result_len != length ? TE_RC(TE_TAPI, TE_EFAIL) : 0));
    if (status == 0)
    {
        rpc_get_buf(ioh->rpcs, ioh->buffer, 0, length, data);
    }
    return status;
}

static te_errno
command_write(iscsi_io_handle_t *ioh, int *fd, 
              void *data, ssize_t length)
{
    ssize_t  result_len;

    if (!realloc_buffer(ioh, length))
        return RPC_ERRNO(ioh->rpcs);
    
    rpc_set_buf(ioh->rpcs, data, length, ioh->buffer, 0);
    result_len = rpc_write(ioh->rpcs, *fd, data, length);
    return (result_len < 0 ?
            RPC_ERRNO(ioh->rpcs) :
            (result_len != length ?
             TE_RC(TE_TAPI, TE_ENOSPC) :
             0));
}

#define ISCSI_COPY_FILE_IN  0
#define ISCSI_COPY_FILE_OUT 1

static te_errno
command_copy_file(iscsi_io_handle_t *ioh, int *fd, 
                  void *data, ssize_t direction)
{
    ssize_t   length;
    ssize_t   result_len;
    te_errno  status;
    te_errno  status_close;
    int       dest_fd;
    int       src_fd;
    int       open_fd;


    if (!realloc_buffer(ioh, ioh->chunksize))
        return RPC_ERRNO(ioh->rpcs);

    if (direction == ISCSI_COPY_FILE_IN)
    {
        open_fd = src_fd = rpc_open(ioh->rpcs, data, 
                                    RPC_O_RDONLY | RPC_O_SYNC, 0);
        dest_fd = *fd;
        status  = (src_fd < 0 ? RPC_ERRNO(ioh->rpcs) : 0);
    }
    else
    {
        open_fd = dest_fd = rpc_open(ioh->rpcs, data, 
                                     RPC_O_WRONLY | RPC_O_SYNC,
                                     RPC_S_IREAD | RPC_S_IWRITE);
        src_fd = *fd;
        status = (dest_fd < 0 ? RPC_ERRNO(ioh->rpcs) : 0);
    }
    
    if (status != 0)
        return status;
    do {
        length = rpc_readbuf(ioh->rpcs, src_fd, 
                             ioh->buffer, ioh->chunksize);
        if (length < 0)
        {
            status = RPC_ERRNO(ioh->rpcs);
            break;
        }
        result_len = rpc_writebuf(ioh->rpcs, dest_fd, ioh->buffer, 
                                  ((size_t)length > ioh->chunksize ? 
                                   ioh->chunksize : 
                                   (size_t)length));
        if (result_len < 0)
            status = RPC_ERRNO(ioh->rpcs);
        else if (result_len != length)
            status = TE_RC(TE_TAPI, TE_ENOSPC);
    } while (status == 0 && length != 0);

    status_close = (rpc_close(ioh->rpcs, open_fd) == 0 ? 0 : 
                    RPC_ERRNO(ioh->rpcs));
    return status == 0 ? status_close : status;
}


static te_errno
command_shell(iscsi_io_handle_t *ioh, int *fd, 
              void *data, ssize_t length)
{
    rpc_wait_status status;
    UNUSED(length);
    UNUSED(fd);
    status = rpc_system(ioh->rpcs, data);
    return (status.flag == RPC_WAIT_STATUS_EXITED &&
            status.value == 0 ? 0 : 
            TE_RC(TE_TAPI, TE_ESHCMD));
}

static sig_atomic_t completed_tasks;

static void
iscsi_io_signal_handler(int signo)
{
    UNUSED(signo);
    completed_tasks++;
}


te_errno
tapi_iscsi_io_prepare(const char *ta, iscsi_target_id id, 
                      te_bool use_signal, te_bool use_fs,
                      size_t chunksize,
                      iscsi_io_handle_t **ioh)
{
    int   rc;
    char  name[10];
    int   i;
    char *dev;

    struct sigaction action;
    sigset_t         mask;

    *ioh = malloc(sizeof(**ioh));
    if (*ioh == NULL)
    {
        return TE_OS_RC(TE_TAPI, errno);
    }

    strncpy((*ioh)->agent, ta, RCF_MAX_NAME - 1);
    sprintf((*ioh)->mountpoint, 
            "/tmp/te_iscsi_fs_%s.%u", ta, id);
    dev = get_host_device(ta, id);
    if (dev == NULL || *dev == '\0')
    {
        if (dev != NULL)
            free(dev);
        free(*ioh);
        *ioh = NULL;
        return TE_RC(TE_TAPI, TE_ENODEV);
    }
    RING("The device detected is %s", dev);
    strcpy((*ioh)->device, dev);
    free(dev);

    (*ioh)->use_signal     = use_signal;
    (*ioh)->use_fs         = use_fs;
    (*ioh)->chunksize      = chunksize;
    (*ioh)->bufsize        = 0;
    (*ioh)->buffer         = RPC_NULL;

    sprintf(name, "iscsi_%u", id);
    rc = rcf_rpc_server_create(ta, name, &((*ioh)->rpcs));
    if (rc != 0)
    {
        free(*ioh);
        *ioh = NULL;
        return rc;
    }
    sem_init(&(*ioh)->cmd_wait, 0, 0);
    for (i = 0; i < MAX_ISCSI_IO_CMDS; i++)
    {
        (*ioh)->cmds[i].cmd         = NULL;
        (*ioh)->cmds[i].data        = NULL;
        (*ioh)->cmds[i].fd          = -1;
        (*ioh)->cmds[i].is_complete = TRUE;
        (*ioh)->cmds[i].status      = 0;
        (*ioh)->cmds[i].leader      = FALSE;
        (*ioh)->cmds[i].do_signal   = FALSE;
        (*ioh)->cmds[i].destroy     = NULL;
    }
    (*ioh)->next_cmd = 0;

    if (use_signal)
    {
        action.sa_handler = iscsi_io_signal_handler;
        action.sa_flags   = 0;
        sigemptyset(&action.sa_mask);
        sigaction(ISCSI_IO_SIGNAL, &action, NULL);
        sigemptyset(&mask);
        sigaddset(&mask, ISCSI_IO_SIGNAL);
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
    }
    
    rc = pthread_create(&((*ioh)->thread), NULL, 
                        tapi_iscsi_io_thread, *ioh);
    if (rc != 0)
    {
        rcf_rpc_server_destroy((*ioh)->rpcs);
        free(*ioh);
        *ioh = NULL;
        return TE_OS_RC(TE_TAPI, rc);
    }
    return 0;
}

te_bool
tapi_iscsi_io_enable_signal(iscsi_io_handle_t *ioh, te_bool enable)
{
    te_bool prev = ioh->use_signal;
    ioh->use_signal = enable;
    return prev;
}

te_errno
tapi_iscsi_io_finish(iscsi_io_handle_t *ioh)
{
    int i;

    pthread_cancel(ioh->thread);
    pthread_join(ioh->thread, NULL);
    for (i = 0; i < MAX_ISCSI_IO_CMDS; i++)
    {
        if (ioh->cmds[i].destroy != NULL)
            ioh->cmds[i].destroy(ioh->cmds[i].data);
    }
    free(ioh);
    return 0;
}

te_errno
tapi_iscsi_io_reset(iscsi_io_handle_t *ioh)
{
    int i;
    
    for (i = 0; i < MAX_ISCSI_IO_CMDS; i++)
    {
        if (!ioh->cmds[i].is_complete)
            return TE_RC(TE_TAPI, TE_EINPROGRESS);
    }
    for (i = 0; i < MAX_ISCSI_IO_CMDS; i++)
    {
        ioh->cmds[i].cmd         = NULL;
        if (ioh->cmds[i].destroy != NULL)
        {
            ioh->cmds[i].destroy(ioh->cmds[i].data);
            ioh->cmds[i].data        = NULL;
            ioh->cmds[i].destroy     = NULL;
        }
        ioh->cmds[i].fd          = -1;
        ioh->cmds[i].is_complete = TRUE;
        ioh->cmds[i].status      = 0;
        ioh->cmds[i].leader      = FALSE;
        ioh->cmds[i].do_signal   = FALSE;
    }
    ioh->next_cmd = 0;
    return 0;
}

te_errno
tapi_iscsi_io_get_status(iscsi_io_handle_t *ioh, iscsi_io_taskid taskid)
{
    if (taskid >= MAX_ISCSI_IO_CMDS)
        return TE_RC(TE_TAPI, TE_EINVAL);
    return ioh->cmds[taskid].is_complete ? ioh->cmds[taskid].status :
        TE_RC(TE_TAPI, TE_EINPROGRESS);
}

te_bool
tapi_iscsi_io_is_complete(iscsi_io_handle_t *ioh, iscsi_io_taskid taskid)
{
    if (taskid >= MAX_ISCSI_IO_CMDS)
    {
        ERROR("Invalid task id %u", taskid);
        return FALSE;
    }
    if (ioh->cmds[taskid].is_complete && completed_tasks != 0)
    {
        completed_tasks--;
    }
    return ioh->cmds[taskid].is_complete;
}

static inline te_bool
is_enough_cmds_avail(iscsi_io_handle_t *ioh, int ncmds)
{
    return ioh->next_cmd + ncmds < MAX_ISCSI_IO_CMDS;
}

static te_errno
post_command(iscsi_io_handle_t *ioh, iscsi_io_cmd_t *src,
             iscsi_io_taskid *taskid)
{
    iscsi_io_cmd_t *ioc = ioh->cmds + ioh->next_cmd;

    int tmp_fd;
    int tmp_status;

    if (ioh->next_cmd == MAX_ISCSI_IO_CMDS)
        return TE_RC(TE_TAPI, TE_ETOOMANY);

    if (taskid != NULL)
        *taskid = ioh->next_cmd;
    ioh->next_cmd++;

    tmp_fd           = src->fd >= 0 ? src->fd : ioc->fd;
    tmp_status       = ioc->status;
    *ioc             = *src;
    ioc->is_complete = FALSE;
    ioc->fd          = tmp_fd;
    ioc->status      = tmp_status;

    VERB("Posting task %d: fd = %d, data = %p, length = %lu", 
         ioh->next_cmd - 1, 
         ioc->fd, ioc->data, (unsigned long)ioc->length);
    sem_post(&ioh->cmd_wait);
    return 0;
}

static te_errno
tapi_iscsi_disable_read_ahead(iscsi_io_handle_t *ioh)
{
    iscsi_io_cmd_t cmd;
    char           blockdev[128];

    snprintf(blockdev, sizeof(blockdev),
             "blockdev --setra 0 %s", ioh->device);
    cmd.leader    = TRUE;
    cmd.do_signal = FALSE;
    cmd.cmd       = command_shell;
    cmd.fd        = -1;
    cmd.data      = strdup(blockdev);
    cmd.length    = 0;
    cmd.spread_fd = FALSE;
    cmd.destroy   = free;
    return post_command(ioh, &cmd, NULL);
}

te_errno
tapi_iscsi_initiator_mount(iscsi_io_handle_t *ioh, iscsi_io_taskid *taskid)
{
    int rc;
    
    iscsi_io_cmd_t cmd;

    if ((rc = tapi_iscsi_disable_read_ahead(ioh)) != 0)
        return rc;

    cmd.leader    = TRUE;
    cmd.do_signal = (taskid != NULL);

    if (!ioh->use_fs)
    {
        cmd.cmd       = command_open;
        cmd.fd        = -1;
        cmd.length    = (ssize_t)(RPC_O_RDWR | RPC_O_SYNC);
        cmd.data      = ioh->device;
        cmd.spread_fd = TRUE;
        cmd.destroy   = NULL;
    }
    else
    {
        char  mount[128];
        
        snprintf(mount, sizeof(mount), 
                 "mkdir %s && /bin/mount -o sync %s %s", ioh->mountpoint,
                 ioh->device, ioh->mountpoint);
        cmd.cmd       = command_shell;
        cmd.fd        = 0;
        cmd.length    = 0;
        cmd.data      = strdup(mount);
        cmd.spread_fd = FALSE;
        cmd.destroy   = free;
    }
    return post_command(ioh, &cmd, taskid);
}

te_errno
tapi_iscsi_initiator_unmount(iscsi_io_handle_t *ioh, 
                             iscsi_io_taskid *taskid)
{
    iscsi_io_cmd_t cmd;

    cmd.leader    = FALSE;
    cmd.do_signal = (taskid != NULL);
    if (!ioh->use_fs)
    {
        cmd.cmd       = command_close;
        cmd.fd        = -1;
        cmd.length    = 0;
        cmd.data      = NULL;
        cmd.spread_fd = FALSE;
        cmd.destroy   = NULL;
    }
    else
    {
        char  umount[128];
        
        snprintf(umount, sizeof(umount), 
                 "/bin/umount %s && rmdir %s", 
                ioh->mountpoint, ioh->mountpoint);

        cmd.cmd       = command_shell;
        cmd.fd        = 0;
        cmd.length    = 0;
        cmd.data      = strdup(umount);
        cmd.spread_fd = FALSE;
        cmd.destroy   = free;
    }
    return post_command(ioh, &cmd, taskid);
}

te_errno
tapi_iscsi_initiator_open(iscsi_io_handle_t *ioh,
                          iscsi_io_taskid *taskid,
                          const char *fname, int mode)
{
    iscsi_io_cmd_t cmd;

    if (!ioh->use_fs)
        return TE_RC(TE_TAPI, TE_ENOTBLK);
    
    cmd.cmd       = command_open;
    cmd.fd        = -1;
    cmd.length    = fcntl_flags_h2rpc(mode) | RPC_O_SYNC;
    cmd.data      = strdup(fname);
    cmd.spread_fd = TRUE;
    cmd.leader    = TRUE;
    cmd.do_signal = (taskid != NULL);
    cmd.destroy   = free;
    return post_command(ioh, &cmd, taskid);
}

te_errno
tapi_iscsi_initiator_close(iscsi_io_handle_t *ioh,
                          iscsi_io_taskid *taskid)
{
    iscsi_io_cmd_t cmd;

    if (!ioh->use_fs)
        return TE_RC(TE_TAPI, TE_ENOTBLK);
    
    cmd.cmd       = command_close;
    cmd.fd        = -1;
    cmd.length    = 0;
    cmd.data      = NULL;
    cmd.spread_fd = FALSE;
    cmd.leader    = FALSE;
    cmd.do_signal = (taskid != NULL);
    cmd.destroy   = NULL;
    return post_command(ioh, &cmd, taskid);
}

te_errno
tapi_iscsi_initiator_seek(iscsi_io_handle_t *ioh,
                          iscsi_io_taskid *taskid,
                          off_t pos)
{
    iscsi_io_cmd_t cmd;
    
    cmd.cmd       = command_seek;
    cmd.fd        = -1;
    cmd.length    = pos;
    cmd.data      = NULL;
    cmd.spread_fd = FALSE;
    cmd.leader    = FALSE;
    cmd.do_signal = (taskid != NULL);
    cmd.destroy   = NULL;
    return post_command(ioh, &cmd, taskid);
}


te_errno
tapi_iscsi_initiator_write(iscsi_io_handle_t *ioh,
                           iscsi_io_taskid *taskid,
                           void *data, size_t length)
{
    iscsi_io_cmd_t cmd;
    
    cmd.cmd       = command_write;
    cmd.fd        = -1;
    cmd.length    = length;
    cmd.data      = data;
    cmd.spread_fd = FALSE;
    cmd.leader    = FALSE;
    cmd.do_signal = (taskid != NULL);
    cmd.destroy   = NULL;
    return post_command(ioh, &cmd, taskid);
}

te_errno
tapi_iscsi_initiator_read(iscsi_io_handle_t *ioh,
                          iscsi_io_taskid *taskid,
                          void *data, size_t length)
{
    iscsi_io_cmd_t cmd;
    
    cmd.cmd       = command_read;
    cmd.fd        = -1;
    cmd.length    = length;
    cmd.data      = data;
    cmd.spread_fd = FALSE;
    cmd.leader    = FALSE;
    cmd.do_signal = (taskid != NULL);
    cmd.destroy   = NULL;
    return post_command(ioh, &cmd, taskid);
}


te_errno
tapi_iscsi_initiator_write_file(iscsi_io_handle_t *ioh,
                                iscsi_io_taskid *taskid,
                                const char *filename)
{
    iscsi_io_cmd_t cmd;
    
    cmd.cmd       = command_copy_file;
    cmd.fd        = -1;
    cmd.length    = ISCSI_COPY_FILE_IN;
    cmd.data      = strdup(filename);
    cmd.spread_fd = FALSE;
    cmd.leader    = FALSE;
    cmd.do_signal = (taskid != NULL);
    cmd.destroy   = free;
    return post_command(ioh, &cmd, taskid);
}

te_errno
tapi_iscsi_initiator_read_file(iscsi_io_handle_t *ioh,
                          iscsi_io_taskid *taskid,
                          const char *filename)
{
    iscsi_io_cmd_t cmd;
    
    cmd.cmd       = command_copy_file;
    cmd.fd        = -1;
    cmd.length    = ISCSI_COPY_FILE_OUT;
    cmd.data      = strdup(filename);
    cmd.spread_fd = FALSE;
    cmd.leader    = FALSE;
    cmd.do_signal = (taskid != NULL);
    cmd.destroy   = free;
    return post_command(ioh, &cmd, taskid);
}

