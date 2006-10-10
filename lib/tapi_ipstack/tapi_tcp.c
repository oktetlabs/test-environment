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

#include "tapi_tcp.h"
#include "tapi_tad.h"
#include "tapi_eth.h"
#include "tapi_arp.h"

#include "ndn_ipstack.h"
#include "ndn_eth.h"
#include "ndn_socket.h"


/* See description in tapi_tcp.h */
int
tapi_tcp_ip4_eth_csap_create(const char *ta_name, int sid, 
                             const char *eth_dev,
                             unsigned int receive_mode,
                             const uint8_t *loc_mac,
                             const uint8_t *rem_mac,
                             in_addr_t loc_addr,
                             in_addr_t rem_addr,
                             uint16_t loc_port,
                             uint16_t rem_port,
                             csap_handle_t *tcp_csap)
{
    int   rc;

    asn_value *csap_spec = NULL;

    do {
        int num = 0;

        rc = asn_parse_value_text("{ tcp:{}, ip4:{}, eth:{}}", 
                                      ndn_csap_spec, &csap_spec, &num); 
        if (rc) break; 

        if (receive_mode != 0)
            rc = asn_write_int32(csap_spec, receive_mode,
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

        rc = tapi_tad_csap_create(ta_name, sid, "tcp.ip4.eth", 
                                  csap_spec, tcp_csap); 
    } while (0);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}

/* See description in tapi_tcp.h */
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


/* See description in tapi_tcp.h */
int
tapi_tcp_ip4_eth_recv_start(const char *ta_name, int sid, 
                            csap_handle_t csap,
                            in_addr_t src_addr, in_addr_t dst_addr,
                            uint16_t src_port, uint16_t dst_port,
                            unsigned int timeout, unsigned int num,
                            rcf_trrecv_mode mode)
{ 
    asn_value *pattern;
    asn_value *pattern_unit; 
    int        rc;
    
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
        asn_free_value(pattern);
        ERROR("%s: insert pattern unit error %r", __FUNCTION__, rc);
        return rc; 
    } 

    if ((rc = tapi_tad_trrecv_start(ta_name, sid, csap, pattern,
                                    timeout, num, mode)) != 0) 
    {
        ERROR("%s: trrecv_start failed: %r", __FUNCTION__, rc);
    }
    asn_free_value(pattern);

    return rc;
}



/* See description in tapi_tcp.h */
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

/* See description in tapi_tcp.h */
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

    if ((rc = asn_get_choice_value(g_pdu, &tcp_pdu, NULL, NULL))
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


/* See description in tapi_tcp.h */
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

        asn_get_choice_value(tcp_pdu, &raw_tcp_pdu, NULL, NULL);

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
    { 
        ERROR("%s(): insert tcp pdu eror: %r", __FUNCTION__, rc);
        goto cleanup;
    }

cleanup:
    if (rc != 0)
        asn_free_value(*tmpl); 

    return TE_RC(TE_TAPI, rc); 
}



int
tapi_tcp_pattern(tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn, 
                 te_bool syn_flag, te_bool ack_flag,
                 asn_value **pattern)
{
    int rc = 0,
        syms; 

    asn_value *tcp_pdu = NULL;

    if (pattern == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    *pattern = NULL; 

    rc = asn_parse_value_text("{{ pdus {ip4:{}, eth:{} } }}", 
                              ndn_traffic_pattern, 
                              pattern, &syms);
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

    if (seqn == 0)
        rc = asn_free_subvalue(tcp_pdu, "#tcp.seqn");
    WARN("%s(): free seqn rc %r", __FUNCTION__, rc);

    if (ackn == 0)
        rc = asn_free_subvalue(tcp_pdu, "#tcp.ackn");
    WARN("%s(): free seqn rc %r", __FUNCTION__, rc);

    rc = asn_insert_indexed(*pattern, tcp_pdu, 0, "0.pdus");
    if (rc != 0)
    { 
        ERROR("%s(): insert tcp pdu eror: %r", __FUNCTION__, rc);
        goto cleanup;
    }

cleanup:
    if (rc != 0)
        asn_free_value(*pattern); 

    return TE_RC(TE_TAPI, rc); 
}










/* See description in tapi_tcp.h */
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


int
tapi_tcp_reset_hack_init(const char *ta_name, int session,
                         const char *iface, te_bool dir_out,
                         tapi_tcp_reset_hack_t *context)
{
    int rc;
    asn_value *syn_ack_pat;

    if (context == NULL)
    {
        ERROR("%s(): null context passed", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    rc = tapi_tcp_ip4_eth_csap_create(ta_name, session, iface, 
                                      dir_out ? TAD_ETH_RECV_OUT : 
                                      TAD_ETH_RECV_HOST, 
                                      NULL, NULL, 
                                      context->loc_ip_addr,
                                      context->rem_ip_addr,
                                      0, /* port will be in pattern */
                                      0, /* we dont know remote port */
                                      &(context->tcp_hack_csap));
    if (rc != 0)
    {
        ERROR("%s(): create tcp.ip4.eth CSAP failed %r", __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    }

    rc = tapi_tcp_pattern(0, 0, TRUE, TRUE, &syn_ack_pat);

    if (context->loc_port)
        asn_write_int32(syn_ack_pat, context->loc_port,
                        "0.pdus.0.#tcp.src-port.#plain"); 

    if (context->rem_ip_addr)
        asn_write_value_field(syn_ack_pat, &(context->rem_ip_addr), 4, 
                              "0.pdus.1.#ip4.dst-addr.#plain");

    if (context->loc_ip_addr)
        asn_write_value_field(syn_ack_pat, &(context->loc_ip_addr), 4, 
                              "0.pdus.1.#ip4.src-addr.#plain");

    rc = tapi_tad_trrecv_start(ta_name, session, 
                               context->tcp_hack_csap,  syn_ack_pat,
                               TAD_TIMEOUT_INF, 1,
                               RCF_TRRECV_PACKETS);

    asn_free_value(syn_ack_pat);
    if (rc != 0)
    {
        ERROR("%s(): receive start on CSAP failed %r", __FUNCTION__, rc);
        return TE_RC(TE_TAPI, rc);
    }

    return 0;
}


void
tcp_reset_hack_pkt_handler(const char *pkt_file, void *user_param)
{
    int      rc;
    int      syms = 0;
    int32_t  port;
    size_t   v_len;

    tapi_tcp_reset_hack_t *context = user_param;

    asn_value *pkt = NULL;

    if (pkt_file == NULL || user_param == NULL)
    {
        ERROR("%s(): called with wrong arguments!", __FUNCTION__);
        goto cleanup;
    } 

    rc = asn_parse_dvalue_in_file(pkt_file, ndn_raw_packet, &pkt, &syms);
    if (rc != 0)
    {
        ERROR("%s(): parse got packet failed %r, sym %d", 
              __FUNCTION__, rc, syms);
        goto cleanup;
    }

    rc = asn_read_int32(pkt, &(context->loc_start_seq), 
                        "pdus.0.#tcp.seqn.#plain");
    if (rc != 0)
    {
        ERROR("%s(): read loc seq failed %r", __FUNCTION__, rc);
        goto cleanup;
    }
    INFO("%s(): read loc start seq: %u",
         __FUNCTION__, context->loc_start_seq);

    rc = asn_read_int32(pkt, &(context->rem_start_seq), 
                        "pdus.0.#tcp.ackn.#plain");
    if (rc != 0)
    {
        ERROR("%s(): read rem seq failed %r", __FUNCTION__, rc);
        goto cleanup;
    }
    INFO("%s(): read rem start seq: %u",
         __FUNCTION__, context->rem_start_seq);


    rc = asn_read_int32(pkt, &port, "pdus.0.#tcp.dst-port.#plain");
    if (rc != 0)
    {
        ERROR("%s(): read dst-port for 'ini' side failed %r",
              __FUNCTION__, rc);
        goto cleanup;
    }
    INFO("%s(): read rem port: %u", __FUNCTION__, port);
    context->rem_port = port;

    if (context->loc_port == 0)
    {
        asn_read_int32(pkt, &port, "pdus.0.#tcp.src-port.#plain");
        context->loc_port = port;
    }
    v_len = sizeof(context->rem_mac);
    asn_read_value_field(pkt, context->rem_mac, &v_len, 
                         "pdus.2.#eth.dst-addr.#plain");
    asn_read_value_field(pkt, context->loc_mac, &v_len, 
                         "pdus.2.#eth.src-addr.#plain");

    v_len = sizeof(context->rem_ip_addr);
    if (context->rem_ip_addr == 0)
        asn_read_value_field(pkt, &(context->rem_ip_addr), &v_len, 
                             "pdus.1.#ip4.dst-addr.#plain"); 
    if (context->loc_ip_addr == 0)
        asn_read_value_field(pkt, &(context->loc_ip_addr), &v_len, 
                             "pdus.1.#ip4.src-addr.#plain");

cleanup:
    asn_free_value(pkt);
}

int
tapi_tcp_reset_hack_catch(const char *ta_name, int session,
                          tapi_tcp_reset_hack_t *context)
{
    int syn_ack_num = 0;
    if (context == NULL)
    {
        ERROR("%s(): null context passed", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    } 

    return rcf_ta_trrecv_stop(ta_name, session, context->tcp_hack_csap, 
                              tcp_reset_hack_pkt_handler, 
                              context, &syn_ack_num);
}


int
tapi_tcp_reset_hack_send(const char *ta_name, int session, 
                         tapi_tcp_reset_hack_t *context,
                         size_t received, size_t sent)
{
    int rc;
    asn_value *reset_tmpl;

    if (context == NULL)
    {
        ERROR("%s(): null context passed", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    } 

    rc = tapi_tcp_template(context->loc_start_seq + sent,
                           context->rem_start_seq + received, 
                           FALSE, TRUE, NULL, 0, &reset_tmpl);
    if (rc != 0)
        ERROR("make reset template failed %r", rc);
    asn_write_int32(reset_tmpl, TCP_RST_FLAG | TCP_ACK_FLAG, 
                    "pdus.0.#tcp.flags.#plain");

    asn_write_value_field(reset_tmpl, context->rem_mac, 6, 
                          "pdus.2.#eth.dst-addr.#plain");
    asn_write_value_field(reset_tmpl, context->loc_mac, 6, 
                          "pdus.2.#eth.src-addr.#plain");

    asn_write_value_field(reset_tmpl, &(context->rem_ip_addr), 4, 
                          "pdus.1.#ip4.dst-addr.#plain");
    asn_write_value_field(reset_tmpl, &(context->loc_ip_addr), 4, 
                          "pdus.1.#ip4.src-addr.#plain");

    asn_write_int32(reset_tmpl, context->rem_port,
                    "pdus.0.#tcp.dst-port.#plain");
    asn_write_int32(reset_tmpl, context->loc_port,
                    "pdus.0.#tcp.src-port.#plain");

    rc = tapi_tad_trsend_start(ta_name, session, context->tcp_hack_csap,
                               reset_tmpl, RCF_MODE_BLOCKING);
    if (rc != 0)
        ERROR("send RST failed %r", rc);

    asn_free_value(reset_tmpl);

    return rc;
}

