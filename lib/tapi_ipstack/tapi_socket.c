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

#define TE_LGR_USER "TAPI TCP"

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

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/ether.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include "rcf_api.h"
#include "conf_api.h"

#include "logger_api.h"

#include "tapi_tad.h"
#include "tapi_socket.h"

#include "ndn.h"
#include "ndn_socket.h"





/*
 * ======================= data TCP CSAP routines ==================
 */


/* See description in tapi_tcp.h */
int
tapi_tcp_server_csap_create(const char *ta_name, int sid, 
                            in_addr_t loc_addr, uint16_t loc_port,
                            csap_handle_t *tcp_csap)
{
    te_errno        rc = 0;
    asn_value      *csap_spec;
    asn_value      *csap_layers;
    asn_value      *csap_layer_spec;
    asn_value      *csap_socket;

    csap_spec       = asn_init_value(ndn_csap_spec);
    csap_layers     = asn_init_value(ndn_csap_layers);
    asn_put_child_value(csap_spec, csap_layers, PRIVATE, NDN_CSAP_LAYERS);

    csap_layer_spec = asn_init_value(ndn_generic_csap_layer);
#define EXP 1

#if EXP
    csap_socket     = asn_retrieve_descendant(csap_layer_spec, &rc,
                                              "#socket");
#else
    csap_socket     = asn_init_value(ndn_socket_csap);
#endif

    rc = asn_write_value_field(csap_socket, NULL, 0,
                               "type.#tcp-server");
    if (rc != 0) goto cleanup;

    rc = asn_write_value_field(csap_socket,
                               &loc_addr, sizeof(loc_addr),
                               "local-addr.#plain");
    if (rc != 0) goto cleanup;

    rc = asn_write_int32(csap_socket, ntohs(loc_port),
                         "local-port.#plain");
    if (rc != 0) goto cleanup;

#if !EXP
    rc = asn_write_component_value(csap_layer_spec, csap_socket,
                                   "#socket");
    if (rc != 0) goto cleanup;
#endif

    rc = asn_insert_indexed(csap_layers, csap_layer_spec, 0, "");
    if (rc != 0) goto cleanup;

    rc = tapi_tad_csap_create(ta_name, sid, "socket", 
                              csap_spec, tcp_csap);

cleanup:
    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}

int
tapi_socket_csap_create(const char *ta_name, int sid, int type,
                        in_addr_t loc_addr, in_addr_t rem_addr,
                        uint16_t loc_port, uint16_t rem_port,
                        csap_handle_t *csap)
{
    te_errno        rc = 0;
    asn_value      *csap_spec;
    asn_value      *csap_layers;
    asn_value      *csap_layer_spec;
    asn_value      *csap_socket;

    csap_spec       = asn_init_value(ndn_csap_spec);
    csap_layers     = asn_init_value(ndn_csap_layers);
    asn_put_child_value(csap_spec, csap_layers, PRIVATE, NDN_CSAP_LAYERS);

    csap_layer_spec = asn_init_value(ndn_generic_csap_layer);
    csap_socket     = asn_init_value(ndn_socket_csap);

    rc = asn_write_value_field(csap_socket, NULL, 0, 
                               type == NDN_TAG_SOCKET_TYPE_UDP ? 
                               "type.#udp" : "type.#tcp-client");
    if (rc != 0) goto cleanup;

    if (loc_addr != INADDR_ANY)
        rc = asn_write_value_field(csap_socket,
                                   &loc_addr, sizeof(loc_addr),
                                   "local-addr.#plain");
    if (rc != 0) goto cleanup;

    if (rem_addr != INADDR_ANY)
        rc = asn_write_value_field(csap_socket,
                                   &rem_addr, sizeof(rem_addr),
                                   "remote-addr.#plain");
    if (rc != 0) goto cleanup;

    if (loc_port != 0)
        rc = asn_write_int32(csap_socket, ntohs(loc_port),
                             "local-port.#plain");
    if (rc != 0) goto cleanup;


    if (rem_port != 0)
        rc = asn_write_int32(csap_socket, ntohs(rem_port),
                             "remote-port.#plain");

    rc = asn_write_component_value(csap_layer_spec, csap_socket, "#socket");
    if (rc != 0) goto cleanup; 

    rc = asn_insert_indexed(csap_layers, csap_layer_spec, 0, "");
    if (rc != 0) goto cleanup; 

    rc = tapi_tad_csap_create(ta_name, sid, "socket", 
                              csap_spec, csap);

cleanup:
    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}





/* See description in tapi_socket.h */
int
tapi_tcp_client_csap_create(const char *ta_name, int sid, 
                            in_addr_t loc_addr, in_addr_t rem_addr,
                            uint16_t loc_port, uint16_t rem_port,
                            csap_handle_t *tcp_csap)
{
    return tapi_socket_csap_create(ta_name, sid, 
                                   NDN_TAG_SOCKET_TYPE_TCP_CLIENT,
                                   loc_addr, rem_addr, loc_port, rem_port,
                                   tcp_csap);
}

/* See description in tapi_socket.h */
int
tapi_udp_csap_create(const char *ta_name, int sid,
                     in_addr_t loc_addr, in_addr_t rem_addr,
                     uint16_t loc_port, uint16_t rem_port,
                     csap_handle_t *udp_csap)
{
    return tapi_socket_csap_create(ta_name, sid, 
                                   NDN_TAG_SOCKET_TYPE_UDP,
                                   loc_addr, rem_addr, loc_port, rem_port,
                                   udp_csap);
}


/* See description in tapi_socket.h */
int
tapi_tcp_socket_csap_create(const char *ta_name, int sid, 
                            int socket, csap_handle_t *tcp_csap)
{
    te_errno        rc;
    asn_value      *csap_spec;
    asn_value      *csap_layers;
    asn_value      *csap_layer_spec;
    asn_value      *csap_socket;

    csap_spec       = asn_init_value(ndn_csap_spec);
    csap_layers     = asn_init_value(ndn_csap_layers);
    asn_put_child_value(csap_spec, csap_layers, PRIVATE, NDN_CSAP_LAYERS);

    csap_layer_spec = asn_init_value(ndn_generic_csap_layer);
    csap_socket     = asn_init_value(ndn_socket_csap);

    asn_write_int32(csap_socket, socket, "type.#file-descr");

    asn_write_component_value(csap_layer_spec, csap_socket, "#socket");

    asn_insert_indexed(csap_layers, csap_layer_spec, 0, "");

    rc = tapi_tad_csap_create(ta_name, sid, "socket", 
                              csap_spec, tcp_csap);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}


/* 
 * Pkt handler for TCP packets 
 */
static void
tcp_server_handler(const char *pkt_fname, void *user_param)
{
    asn_value  *pkt = NULL;
    int        *socket = user_param;

    int s_parsed = 0;
    int rc = 0;

    if (user_param == NULL) 
    {
        ERROR("%s called with NULL user param", __FUNCTION__);
        return;
    } 

    if ((rc = asn_parse_dvalue_in_file(pkt_fname, ndn_raw_packet,
                                       &pkt, &s_parsed)) != 0)
    {                                      
        ERROR("%s(): parse packet fails, rc = %r, sym %d",
              __FUNCTION__, rc, s_parsed); 

        return;
    }
    rc = asn_read_int32(pkt, socket, "pdus.0.#socket.file-descr");
    if (rc != 0)
        ERROR("%s(): read socket failed, rc %r", __FUNCTION__, rc);

    INFO("%s(): received socket: %d", __FUNCTION__, *socket);

    asn_free_value(pkt);
}

/* See description in tapi_tcp.h */
int
tapi_tcp_server_recv(const char *ta_name, int sid, 
                     csap_handle_t tcp_csap, 
                     unsigned int timeout, int *socket)
{
    te_errno        rc;
    unsigned int    num;

    if (ta_name == NULL || socket == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    rc = tapi_tad_trrecv_start(ta_name, sid, tcp_csap, NULL, 
                               timeout, 1, RCF_TRRECV_PACKETS);
    if (rc != 0)
    {
        ERROR("%s(): trrecv_start failed %r", __FUNCTION__, rc);
        return rc;
    }

    rc = rcf_ta_trrecv_wait(ta_name, sid, tcp_csap,
                            tcp_server_handler, socket, &num);
    if (rc != 0)
        WARN("%s() trrecv_wait failed: %r", __FUNCTION__, rc);

    return rc;
} 











struct data_message {
    uint8_t *data;
    size_t   length;
}; 

/* 
 * Pkt handler for 'socket' CSAP incoming data
 */
static void
socket_csap_handler(const char *pkt_fname, void *user_param)
{
    asn_value  *pkt = NULL;
    struct data_message *msg;

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
    INFO("%s(): %d bytes received", __FUNCTION__, (int)len);

    if (len > msg->length)
        WARN("%s(): length of message greater then buffer", __FUNCTION__);

    len = msg->length;
    rc = asn_read_value_field(pkt, msg->data, &len, "payload.#bytes");
    if (rc != 0)
        ERROR("%s(): read payload failed %r", __FUNCTION__, rc);
    else
        INFO("%s(): received payload %Tm", __FUNCTION__, msg->data, len);

    msg->length = len;

    asn_free_value(pkt);
}




/* See description in tapi_socket.h */
int
tapi_socket_recv(const char *ta_name, int sid, csap_handle_t csap, 
                 unsigned int timeout, csap_handle_t forward,
                 te_bool len_exact, uint8_t *buf, size_t *length)
{
    asn_value *pattern = NULL;
    struct data_message msg;

    int rc = 0, syms, num;

    if (ta_name == NULL)
        return TE_EWRONGPTR;

    rc = asn_parse_value_text("{{pdus { socket:{} } }}",
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

    msg.data = buf;
    msg.length = ((length == NULL) ? 0 : *length);

    if (len_exact)
    {
        if (length == NULL)
            return TE_EWRONGPTR;
        else
            asn_write_int32(pattern, *length, "0.pdus.0.#socket.length");
    }

    rc = tapi_tad_trrecv_start(ta_name, sid, csap, pattern, timeout, 1,
                               buf == NULL ? RCF_TRRECV_COUNT
                                           : RCF_TRRECV_PACKETS);
    if (rc != 0)
    {
        ERROR("%s(): trrecv_start failed %r", __FUNCTION__, rc);
        goto cleanup;
    }

    rc = rcf_ta_trrecv_wait(ta_name, sid, csap,
                            buf == NULL ? NULL : socket_csap_handler,
                            buf == NULL ? NULL : &msg, &num);
    if (rc != 0)
        WARN("%s() trrecv_wait failed: %r", __FUNCTION__, rc);

    if (buf != NULL && length != NULL)
    {
        *length = msg.length;
    }

cleanup:
    asn_free_value(pattern);
    return rc;
}

/* See description in tapi_socket.h */
int
tapi_socket_send(const char *ta_name, int sid, csap_handle_t csap, 
                 uint8_t *buf, size_t length)
{
    asn_value *template = NULL;

    int rc = 0, syms;

    if (ta_name == NULL)
        return TE_EWRONGPTR;

    rc = asn_parse_value_text("{pdus { socket:{} } }",
                              ndn_traffic_template, &template, &syms);
    if (rc != 0)
    {
        ERROR("%s(): parse ASN csap_spec failed %X, sym %d", 
              __FUNCTION__, rc, syms);
        return rc;
    }

    rc = asn_write_value_field(template, buf, length, "payload.#bytes");
    if (rc != 0)
    {
        ERROR("%s(): write payload failed %r", __FUNCTION__, rc);
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




