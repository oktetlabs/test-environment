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

#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
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

#include "tapi_tcp.h"
#include "tapi_tad.h"
#include "tapi_eth.h"
#include "tapi_arp.h"

#include "ndn_ipstack.h"
#include "ndn_eth.h"



/** Data to be passed to intermediate callback */
typedef struct tapi_ip4_cb_data_t {
    tcp_row_callback  user_cb;
    void             *user_data;
} tapi_tcp_cb_data_t;

/* 
 * Pkt handler for TCP packets 
 */
static void
tcp_pkt_handler(const char *pkt_fname, void *user_param)
{
    tapi_tcp_cb_data_t *cb_data = (tapi_tcp_cb_data_t *)user_param;
    asn_value          *pkt = NULL;

    int s_parsed = 0;
    int rc;

    if (user_param == NULL) 
    {
        ERROR("%s called with NULL user param", __FUNCTION__);
        return;
    }

    if (cb_data->user_cb == NULL)
    {
        ERROR("%s called with NULL user cb", __FUNCTION__);
        return; 
    }

    if ((rc = asn_parse_dvalue_in_file(pkt_fname, ndn_raw_packet,
                                       &pkt, &s_parsed)) != 0)
    {                                      
        ERROR("%s(): parse packet fails, rc = %r, sym %d",
              __FUNCTION__, rc, s_parsed);
        return;
    }

    cb_data->user_cb(pkt, cb_data->user_data);
    
    asn_free_value(pkt);
}

/* see description in tapi_tcp.h */
int 
tapi_tcp_ip4_eth_csap_create(const char *ta_name, int sid, 
                             const char *eth_dev,
                             const uint8_t *loc_mac,
                             const uint8_t *rem_mac,
                             in_addr_t loc_addr, in_addr_t rem_addr,
                             uint16_t loc_port, uint16_t rem_port,
                             csap_handle_t *tcp_csap)
{
    return tapi_tcp_ip4_eth_mode_csap_create(ta_name, sid, eth_dev, 0, 
                                             loc_mac, rem_mac,
                                             loc_addr, rem_addr, 
                                             loc_port, rem_port, 
                                             tcp_csap);
}

int
tapi_tcp_ip4_eth_mode_csap_create(const char *ta_name, int sid, 
                                  const char *eth_dev,
                                  uint8_t eth_mode,
                                  const uint8_t *loc_mac,
                                  const uint8_t *rem_mac,
                                  in_addr_t loc_addr,
                                  in_addr_t rem_addr,
                                  uint16_t loc_port,
                                  uint16_t rem_port,
                                  csap_handle_t *tcp_csap)
{
    char  csap_fname[] = "/tmp/te_tcp_csap.XXXXXX";
    int   rc;

    asn_value *csap_spec = NULL;

    do {
        int num = 0;

        mktemp(csap_fname);

        rc = asn_parse_value_text("{ tcp:{}, ip4:{}, eth:{}}", 
                                      ndn_csap_spec, &csap_spec, &num); 
        if (rc) break; 

        if (eth_mode |= 0)
            rc = asn_write_int32(csap_spec, eth_mode,
                                 "2.#eth.receive-mode");
        if (rc) break; 

        if (eth_dev)
            rc = asn_write_value_field(csap_spec, 
                    eth_dev, strlen(eth_dev), "2.#eth.device-id.#plain");
        if (rc) break; 

        if (loc_mac)
            rc = asn_write_value_field(csap_spec, 
                    loc_mac, 6, "2.#eth.local-addr.#plain");
        if (rc) break; 

        if (rem_mac)
            rc = asn_write_value_field(csap_spec, 
                    rem_mac, 6, "2.#eth.remote-addr.#plain");
        if (rc) break; 

        if(loc_addr)
            rc = asn_write_value_field(csap_spec,
                                       &loc_addr, sizeof(loc_addr),
                                       "1.#ip4.local-addr.#plain");
        if (rc) break; 

        if(rem_addr)
            rc = asn_write_value_field(csap_spec,
                                       &rem_addr, sizeof(rem_addr),
                                       "1.#ip4.remote-addr.#plain");
        if (rc) break; 

        if(loc_port)
            rc = asn_write_int32(csap_spec, ntohs(loc_port),
                                 "0.#tcp.local-port.#plain");
        if (rc) break; 

        if(rem_port)
            rc = asn_write_int32(csap_spec, ntohs(rem_port), 
                                 "0.#tcp.remote-port.#plain");
        if (rc) break;

        rc = asn_save_to_file(csap_spec, csap_fname);
        VERB("TAPI: udp create csap, save to file %s, rc: %r\n",
                csap_fname, rc);
        if (rc) break;


        rc = rcf_ta_csap_create(ta_name, sid, "tcp.ip4.eth", 
                                csap_fname, tcp_csap); 
    } while (0);

    asn_free_value(csap_spec);
    unlink(csap_fname);

    return TE_RC(TE_TAPI, rc);
}

/* see description in tapi_tcp.h */
int 
tapi_tcp_ip4_pattern_unit(in_addr_t  src_addr, in_addr_t  dst_addr,
                          uint16_t src_port, uint16_t dst_port,
                          asn_value **result_value)
{
    int rc;
    int num;
    asn_value *pu = NULL;

    struct in_addr in_src_addr;
    struct in_addr in_dst_addr;

    if (src_addr) 
        in_src_addr.s_addr = src_addr;
    else
        in_src_addr.s_addr = 0;

    if (dst_addr) 
        in_dst_addr.s_addr = dst_addr;
    else
        in_dst_addr.s_addr = 0;

    VERB("%s, create pattern unit %s:%u -> %s:%u", __FUNCTION__,
         inet_ntoa(in_src_addr), (unsigned int)ntohs(src_port), 
         inet_ntoa(in_dst_addr), (unsigned int)ntohs(dst_port));

    do {
        if (result_value == NULL) { rc = TE_EWRONGPTR; break; }

        rc = asn_parse_value_text("{ pdus { tcp:{}, ip4:{}, eth:{}}}", 
                              ndn_traffic_pattern_unit, &pu, &num);

        if (rc) break;
        if (src_addr)
            rc = asn_write_value_field(pu, &src_addr, 4, 
                                       "pdus.1.#ip4.src-addr.#plain");

        if (rc) break;
        if (dst_addr)
            rc = asn_write_value_field(pu, &dst_addr, 4, 
                                       "pdus.1.#ip4.dst-addr.#plain");

        if (rc) break;
        if (src_port) /* SRC port passed here in network byte order */
            rc = asn_write_int32(pu, ntohs(src_port),
                                 "pdus.0.#tcp.src-port.#plain");

        if (rc) break;
        if (dst_port) /* DST port passed here in network byte order */
            rc = asn_write_int32(pu, ntohs(dst_port),
                                 "pdus.0.#tcp.dst-port.#plain");
        if (rc) break;

    } while (0);

    if (rc)
    {
        ERROR("%s: error %r", __FUNCTION__, rc);
        asn_free_value(pu);
    }
    else
        *result_value = pu;

    return TE_RC(TE_TAPI, rc);
}


/* see description in tapi_tcp.h */
int
tapi_tcp_ip4_eth_recv_start(const char *ta_name, int sid, 
                            csap_handle_t csap,
                            in_addr_t      src_addr,
                            in_addr_t      dst_addr,
                            uint16_t src_port, uint16_t dst_port,
                            unsigned int timeout, int num,
                            tcp_row_callback callback, void *userdata)
{ 
    asn_value *pattern;
    asn_value *pattern_unit; 
    int        rc;

    tapi_tcp_cb_data_t *cb_data = NULL;
    
    if ((rc = tapi_tcp_ip4_pattern_unit(src_addr, dst_addr, 
                                        src_port, dst_port, 
                                        &pattern_unit)) != 0)
    {
        ERROR("%s: create pattern unit error %r", __FUNCTION__, rc);
        return rc;
    }

    pattern = asn_init_value(ndn_traffic_pattern); 

    if ((rc = asn_insert_indexed(pattern, pattern_unit, 0, "")) != 0)
    {
        asn_free_value(pattern_unit);
        asn_free_value(pattern);
        ERROR("%s: insert pattern unit error %r", __FUNCTION__, rc);
        return rc; 
    } 

    asn_free_value(pattern_unit);
    
    if (callback != NULL)
    {
        cb_data = malloc(sizeof(*cb_data));
        cb_data->user_cb = callback;
        cb_data->user_data = userdata;
    }

    if ((rc = tapi_tad_trrecv_start(ta_name, sid, csap, pattern,
                                    (callback == NULL) ? NULL : 
                                        tcp_pkt_handler,
                                    cb_data, timeout, num)) != 0) 
    {
        ERROR("%s: trrecv_start failed: %r", __FUNCTION__, rc);
    }
    asn_free_value(pattern);

    return rc;
}



/* see description in tapi_tcp.h */
int
tapi_tcp_make_msg(uint16_t src_port, uint16_t dst_port,
                  tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn, 
                  te_bool syn_flag, te_bool ack_flag,
                  uint8_t *msg)
{
    if (msg == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR); 

    *((uint16_t *) msg) = src_port;
    msg += 2;

    *((uint16_t *) msg) = dst_port;
    msg += 2;

    *((uint32_t *) msg) = htonl(seqn);
    msg += 4;

    if (ack_flag)
        *((uint32_t *) msg) = htonl(ackn);
    msg += 4;

    *msg = (5 << 4); 
    msg++;

    *msg = 0;

    if (syn_flag)
        *msg |= TCP_SYN_FLAG; 
    if (ack_flag)
        *msg |= TCP_ACK_FLAG; 
    msg++;

    /* window: rather reasonable value? */
    *((uint16_t *) msg) = htons(2000); 
    msg += 2;

    /* checksum */
    *((uint16_t *) msg) = 0; 
    msg += 2;

    /* urg pointer */
    *((uint16_t *) msg) = 0; 

    return 0;
}

/* see description in tapi_tcp.h */
int
tapi_tcp_pdu(uint16_t src_port, uint16_t dst_port,
             tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn, 
             te_bool syn_flag, te_bool ack_flag,
             asn_value **pdu)
{
    int         rc,
                syms;
    asn_value  *g_pdu;
    asn_value  *tcp_pdu;
    uint8_t     flags;

    if (pdu == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    if ((rc = asn_parse_value_text("tcp:{}", ndn_generic_pdu,
                                   &g_pdu, &syms)) != 0)
        return TE_RC(TE_TAPI, rc);

    if ((rc = asn_get_choice_value(g_pdu,
                                   (const asn_value **)&tcp_pdu,
                                   NULL, NULL))
            != 0)
    {
        ERROR("%s(): get tcp pdu subvalue failed %r", __FUNCTION__, rc);
        asn_free_value(g_pdu);
        return TE_RC(TE_TAPI, rc);
    }

    if (src_port != 0 &&
        (rc = ndn_du_write_plain_int(tcp_pdu, NDN_TAG_TCP_SRC_PORT,
                                     src_port)) != 0)
    {
        ERROR("%s(): set TCP src port failed %r", __FUNCTION__, rc);
        asn_free_value(g_pdu);
        return TE_RC(TE_TAPI, rc);
    }

    if (dst_port != 0 &&
        (rc = ndn_du_write_plain_int(tcp_pdu, NDN_TAG_TCP_DST_PORT,
                                     dst_port)) != 0)
    {
        ERROR("%s(): set TCP dst port failed %r", __FUNCTION__, rc);
        asn_free_value(g_pdu);
        return TE_RC(TE_TAPI, rc);
    }

    if ((rc = ndn_du_write_plain_int(tcp_pdu, NDN_TAG_TCP_SEQN,
                                     seqn)) != 0)
    {
        ERROR("%s(): set TCP seqn failed %r", __FUNCTION__, rc);
        asn_free_value(*pdu);
        return TE_RC(TE_TAPI, rc);
    }

    if (ack_flag && 
        (rc = ndn_du_write_plain_int(tcp_pdu, NDN_TAG_TCP_ACKN,
                                     ackn)) != 0)
    {
        ERROR("%s(): set TCP ackn failed %r", __FUNCTION__, rc);
        asn_free_value(g_pdu);
        return TE_RC(TE_TAPI, rc);
    }
    flags = 0;
    if (syn_flag)
        flags |= TCP_SYN_FLAG;
    if (ack_flag)
        flags |= TCP_ACK_FLAG;

    if ((rc = ndn_du_write_plain_int(tcp_pdu, NDN_TAG_TCP_FLAGS,
                                     flags)) != 0)
    {
        ERROR("%s(): set TCP flags failed %r", __FUNCTION__, rc);
        asn_free_value(g_pdu);
        return TE_RC(TE_TAPI, rc);
    }

    *pdu = g_pdu;
    return 0;
}


/* see description in tapi_tcp.h */
int
tapi_tcp_template(tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn, 
                  te_bool syn_flag, te_bool ack_flag,
                  uint8_t *data, size_t pld_len,
                  asn_value **tmpl)
{
    int rc = 0,
        syms; 

    asn_value *tcp_pdu = NULL;

    if (tmpl == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    *tmpl = NULL; 

    rc = asn_parse_value_text("{ pdus {ip4:{}, eth:{} } }", 
                              ndn_traffic_template, 
                              tmpl, &syms);
    if (rc != 0)
    {
        ERROR("%s(): cannot parse template: %r, sym %d", 
              __FUNCTION__, rc, syms);
        return TE_RC(TE_TAPI, rc);
    }

    rc = tapi_tcp_pdu(0, 0, seqn, ackn, syn_flag, ack_flag, &tcp_pdu);
    if (rc != 0)
    {
        ERROR("%s(): make tcp pdu eror: %r", __FUNCTION__, rc);
        goto cleanup;
    } 

    if (data != NULL && pld_len > 0)
    {
        int32_t flags;
        asn_value *raw_tcp_pdu = NULL;

        asn_get_choice_value(tcp_pdu, (const asn_value **)&raw_tcp_pdu,
                                  NULL, NULL);

        ndn_du_read_plain_int(raw_tcp_pdu, NDN_TAG_TCP_FLAGS, &flags);
        flags |= TCP_PSH_FLAG;
        ndn_du_write_plain_int(raw_tcp_pdu, NDN_TAG_TCP_FLAGS, flags);

        rc = asn_write_value_field(*tmpl, data, pld_len, "payload.#bytes");
        if (rc != 0)
        {
            ERROR("%s(): write payload eror: %r", __FUNCTION__, rc);
            goto cleanup;
        }

    }

    rc = asn_insert_indexed(*tmpl, tcp_pdu, 0, "pdus");
    if (rc != 0)
    { ERROR("%s(): insert tcp pdu eror: %r", __FUNCTION__, rc);
        goto cleanup;
    }

cleanup:
    if (tcp_pdu != NULL)
        asn_free_value(tcp_pdu); 

    if (rc != 0 && *tmpl != NULL)
        asn_free_value(*tmpl); 

    return TE_RC(TE_TAPI, rc); 
}








/*
 * ======================= data TCP CSAP routines ==================
 */


/* see description in tapi_tcp.h */
int
tapi_tcp_server_csap_create(const char *ta_name, int sid, 
                            in_addr_t loc_addr, uint16_t loc_port,
                            csap_handle_t *tcp_csap)
{
    asn_value *csap_spec = NULL;
    int rc = 0, syms;

    rc = asn_parse_value_text("{  tcp:{}, ip4:{} }",
                              ndn_csap_spec, &csap_spec, &syms); 
    if (rc != 0)
    {
        ERROR("%s(): parse ASN csap_spec failed %X, sym %d", 
              __FUNCTION__, rc, syms);
        return rc;
    }

    rc = asn_write_value_field(csap_spec, (uint8_t *)&loc_addr,
                               sizeof(loc_addr),
                               "1.#ip4.local-addr.#plain");
    if (rc != 0)
    {
        ERROR("%s(): write ip addr failed, rc %X", __FUNCTION__, rc);
        goto cleanup;
    }

    rc = asn_write_int32(csap_spec, ntohs(loc_port),
                         "0.#tcp.local-port.#plain");
    if (rc != 0)
    {
        ERROR("%s(): write port failed, rc %X", __FUNCTION__, rc);
        goto cleanup;
    }

    rc = asn_write_value_field(csap_spec, NULL, 0,
                               "0.#tcp.data.#server");
    if (rc != 0)
    {
        ERROR("%s(): write server failed, rc %X", __FUNCTION__, rc);
        goto cleanup;
    }

    rc = tapi_tad_csap_create(ta_name, sid, "data.tcp.ip4", 
                              csap_spec, tcp_csap); 
    if (rc != 0)
        ERROR("%s(): csap create failed, rc %X", __FUNCTION__, rc);
    else
        INFO("'data.tcp.ip4' CSAP created succesfully");

cleanup:
    asn_free_value(csap_spec);
    return rc;
}

/* see description in tapi_tcp.h */
int
tapi_tcp_client_csap_create(const char *ta_name, int sid, 
                            in_addr_t loc_addr, in_addr_t rem_addr,
                            uint16_t loc_port, uint16_t rem_port,
                            csap_handle_t *tcp_csap)
{
    UNUSED(ta_name);
    UNUSED(sid);
    UNUSED(loc_addr);
    UNUSED(rem_addr);
    UNUSED(loc_port);
    UNUSED(rem_port);
    UNUSED(tcp_csap);
    return TE_RC(TE_TAPI, TE_EOPNOTSUPP);
}

/* see description in tapi_tcp.h */
int
tapi_tcp_socket_csap_create(const char *ta_name, int sid, 
                            int socket, csap_handle_t *tcp_csap)
{
    asn_value *csap_spec = NULL;
    int rc = 0, syms;

    rc = asn_parse_value_text("{ tcp:{}, ip4:{} }",
                              ndn_csap_spec, &csap_spec, &syms); 
    if (rc != 0)
    {
        ERROR("%s(): parse ASN csap_spec failed %X, sym %d", 
              __FUNCTION__, rc, syms);
        return rc;
    }

    rc = asn_write_int32(csap_spec, socket, "0.#tcp.data.#socket");
    if (rc != 0)
    {
        ERROR("%s(): write socket failed, rc %X", __FUNCTION__, rc);
        goto cleanup;
    } 

    rc = tapi_tad_csap_create(ta_name, sid, "data.tcp.ip4", 
                              csap_spec, tcp_csap); 
    if (rc != 0)
        ERROR("%s(): csap create failed, rc %X", __FUNCTION__, rc);

cleanup:
    asn_free_value(csap_spec);
    return rc;
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
    rc = asn_read_int32(pkt, socket, "pdus.0.socket");
    if (rc != 0)
        ERROR("%s(): read socket failed, rc %r", __FUNCTION__, rc);

    INFO("%s(): received socket: %d", __FUNCTION__, *socket);

    asn_free_value(pkt);
}

/* see description in tapi_tcp.h */
int
tapi_tcp_server_recv(const char *ta_name, int sid, 
                     csap_handle_t tcp_csap, 
                     unsigned int timeout, int *socket)
{
    asn_value *pattern = NULL;

    int rc = 0, syms, num;

    if (ta_name == NULL || socket == NULL)
        return TE_EWRONGPTR;

    rc = asn_parse_value_text("{{pdus { tcp:{}, ip4:{} } }}",
                              ndn_traffic_pattern, &pattern, &syms);
    if (rc != 0)
    {
        ERROR("%s(): parse ASN csap_spec failed %X, sym %d", 
              __FUNCTION__, rc, syms);
        return rc;
    }

    rc = tapi_tad_trrecv_start(ta_name, sid, tcp_csap, pattern, 
                               tcp_server_handler, socket, timeout, 1);
    if (rc != 0)
    {
        ERROR("%s(): trrecv_start failed %r", __FUNCTION__, rc);
        goto cleanup;
    }

    rc = rcf_ta_trrecv_wait(ta_name, sid, tcp_csap, &num);
    if (rc != 0)
        WARN("%s() trrecv_wait failed: %r", __FUNCTION__, rc);

cleanup: 
    asn_free_value(pattern);
    return rc;
} 











struct data_message {
    uint8_t *data;
    size_t   length;
}; 

/* 
 * Pkt handler for TCP packets 
 */
static void
tcp_data_csap_handler(const char *pkt_fname, void *user_param)
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
    RING("%s(): %d bytes received", __FUNCTION__, (int)len);

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




/* see description in tapi_tcp.h */
int
tapi_tcp_buffer_recv(const char *ta_name, int sid, 
                     csap_handle_t tcp_csap, 
                     unsigned int timeout, 
                     csap_handle_t forward, te_bool len_exact,
                     uint8_t *buf, size_t *length)
{
    asn_value *pattern = NULL;
    struct data_message msg;

    int rc = 0, syms, num;

    if (ta_name == NULL)
        return TE_EWRONGPTR;

    rc = asn_parse_value_text("{{pdus { tcp:{}, ip4:{} } }}",
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

    msg.data = buf;
    msg.length = ((length == NULL) ? 0 : *length);

    if (len_exact)
    {
        if (length == NULL)
            return TE_EWRONGPTR;
        else
            asn_write_int32(pattern, *length, "0.pdus.0.length");
    }

    rc = tapi_tad_trrecv_start(ta_name, sid, tcp_csap, pattern, 
                               buf == NULL ? NULL : tcp_data_csap_handler,
                               buf == NULL ? NULL : &msg, timeout, 1);
    if (rc != 0)
    {
        ERROR("%s(): trrecv_start failed %r", __FUNCTION__, rc);
        goto cleanup;
    }

    rc = rcf_ta_trrecv_wait(ta_name, sid, tcp_csap, &num);
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

/* see description in tapi_tcp.h */
int
tapi_tcp_buffer_send(const char *ta_name, int sid, 
                     csap_handle_t tcp_csap, 
                     uint8_t *buf, size_t length)
{
    asn_value *template = NULL;

    int rc = 0, syms;

    if (ta_name == NULL)
        return TE_EWRONGPTR;

    rc = asn_parse_value_text("{pdus { tcp:{}, ip4:{} } }",
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

    rc = tapi_tad_trsend_start(ta_name, sid, tcp_csap, template,
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





/* see description in tapi_tcp.h */
int
tapi_tcp_forward_all(const char *ta_name, int session,
                     csap_handle_t csap_rcv, csap_handle_t csap_fwd,
                     unsigned int timeout, int *forwarded)
{
    int rc, syms;

    asn_value *pattern;
    rc = asn_parse_value_text("{{pdus { tcp:{}, ip4:{} } }}",
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

