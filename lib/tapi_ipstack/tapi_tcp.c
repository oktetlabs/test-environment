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

#include "ndn_ipstack.h"
#include "ndn_eth.h"




#ifndef IFNAME_SIZE
#define IFNAME_SIZE 256
#endif

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
    char  csap_fname[] = "/tmp/te_tcp_csap.XXXXXX";
    int   rc;

    asn_value *csap_spec;

    do {
        int num = 0;

        mktemp(csap_fname);

        rc = asn_parse_value_text("{ tcp:{}, ip4:{}, eth:{}}", 
                                      ndn_csap_spec, &csap_spec, &num); 
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
        {
            loc_port = ntohs(loc_port);
            rc = asn_write_value_field(csap_spec, 
                &loc_port, sizeof(loc_port), "0.#tcp.local-port.#plain");
        }
        if (rc) break; 

        if(rem_port)
        {
            rem_port = ntohs(rem_port);
            rc = asn_write_value_field(csap_spec, 
                &rem_port, sizeof(rem_port), "0.#tcp.remote-port.#plain");
        }
        if (rc) break;

        rc = asn_save_to_file(csap_spec, csap_fname);
        VERB("TAPI: udp create csap, save to file %s, rc: %x\n",
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
tapi_tcp_ip4_pattern_unit(const uint8_t *src_addr, const uint8_t *dst_addr,
                        uint16_t src_port, uint16_t dst_port,
                        asn_value **result_value)
{
    int rc;
    int num;
    asn_value *pu = NULL;

    struct in_addr in_src_addr;
    struct in_addr in_dst_addr;

    if (src_addr) 
        in_src_addr.s_addr = *(unsigned long *)src_addr;
    else
        in_src_addr.s_addr = 0;

    if (dst_addr) 
        in_dst_addr.s_addr = *(unsigned long *)dst_addr;
    else
        in_dst_addr.s_addr = 0;

    VERB("%s, create pattern unit %s:%u -> %s:%u", __FUNCTION__,
         inet_ntoa(in_src_addr), (unsigned int)src_port, 
         inet_ntoa(in_dst_addr), (unsigned int)dst_port);

    do {
        if (result_value == NULL) { rc = ETEWRONGPTR; break; }

        rc = asn_parse_value_text("{ pdus { tcp:{}, ip4:{}, eth:{}}}", 
                              ndn_traffic_pattern_unit, &pu, &num);

        if (rc) break;
        if (src_addr)
            rc = asn_write_value_field(pu, src_addr, 4, 
                                       "pdus.1.#ip4.src-addr.#plain");

        if (dst_addr)
            rc = asn_write_value_field(pu, dst_addr, 4, 
                                       "pdus.1.#ip4.dst-addr.#plain");

        if (src_port)
            rc = asn_write_value_field(pu, &src_port, sizeof(src_port),
                                       "pdus.0.#tcp.src-port.#plain");

        if (dst_port)
            rc = asn_write_value_field(pu, &dst_port, sizeof(src_port),
                                       "pdus.0.#tcp.dst-port.#plain"); 

    } while (0);

    if (rc)
    {
        ERROR("%s: error %X", __FUNCTION__, rc);
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
                            const uint8_t *src_addr,
                            const uint8_t *dst_addr,
                            uint16_t src_port, uint16_t dst_port,
                            unsigned int timeout, int num)
{ 
    asn_value *pattern;
    asn_value *pattern_unit; 
    int rc = 0;

    
    if ((rc = tapi_tcp_ip4_pattern_unit(src_addr, dst_addr, 
                                        src_port, dst_port, 
                                        &pattern_unit)) != 0)
    {
        ERROR("%s: create pattern unit error %X", __FUNCTION__, rc);
        return rc;
    }

    pattern = asn_init_value(ndn_traffic_pattern); 

    if ((rc = asn_insert_indexed(pattern, pattern_unit, 0, "")) != 0)
    {
        asn_free_value(pattern_unit);
        asn_free_value(pattern);
        ERROR("%s: insert pattern unit error %X", __FUNCTION__, rc);
        return rc; 
    } 

    asn_free_value(pattern_unit);

    if ((rc = tapi_tad_trrecv_start(ta_name, sid, csap, pattern,
                                NULL, NULL, timeout, num)) != 0) 
    {
        ERROR("%s: trrecv_start failed: %X", __FUNCTION__, rc);
    }
    asn_free_value(pattern);

    return rc;
}



int
tapi_tcp_make_msg(uint16_t src_port, uint16_t dst_port,
                  tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn, 
                  te_bool syn_flag, te_bool ack_flag,
                  uint8_t *msg)
{
    if (msg == NULL)
        return TE_RC(TE_TAPI, ETEWRONGPTR); 

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

int
tapi_tcp_make_pdu(uint16_t src_port, uint16_t dst_port,
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
        return TE_RC(TE_TAPI, ETEWRONGPTR);

    if ((rc = asn_parse_value_text("tcp:{}", ndn_generic_pdu,
                                   &g_pdu, &syms)) != 0)
        return TE_RC(TE_TAPI, rc);

    if ((rc = asn_get_choice_value(g_pdu,
                                   (const asn_value **)&tcp_pdu,
                                   NULL, NULL))
            != 0)
    {
        ERROR("%s(): get tcp pdu subvalue failed 0x%X", __FUNCTION__, rc);
        asn_free_value(g_pdu);
        return TE_RC(TE_TAPI, rc);
    }

    if (src_port != 0 &&
        (rc = ndn_du_write_plain_int(tcp_pdu, NDN_TAG_TCP_SRC_PORT,
                                     src_port)) != 0)
    {
        ERROR("%s(): set TCP src port failed 0x%X", __FUNCTION__, rc);
        asn_free_value(g_pdu);
        return TE_RC(TE_TAPI, rc);
    }

    if (dst_port != 0 &&
        (rc = ndn_du_write_plain_int(tcp_pdu, NDN_TAG_TCP_DST_PORT,
                                     dst_port)) != 0)
    {
        ERROR("%s(): set TCP dst port failed 0x%X", __FUNCTION__, rc);
        asn_free_value(g_pdu);
        return TE_RC(TE_TAPI, rc);
    }

    if ((rc = ndn_du_write_plain_int(tcp_pdu, NDN_TAG_TCP_SEQN,
                                     seqn)) != 0)
    {
        ERROR("%s(): set TCP seqn failed 0x%X", __FUNCTION__, rc);
        asn_free_value(*pdu);
        return TE_RC(TE_TAPI, rc);
    }

    if (ack_flag && 
        (rc = ndn_du_write_plain_int(tcp_pdu, NDN_TAG_TCP_ACKN,
                                     ackn)) != 0)
    {
        ERROR("%s(): set TCP ackn failed 0x%X", __FUNCTION__, rc);
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
        ERROR("%s(): set TCP flags failed 0x%X", __FUNCTION__, rc);
        asn_free_value(g_pdu);
        return TE_RC(TE_TAPI, rc);
    }

    *pdu = g_pdu;
    return 0;
}





/*
 * ==================================================================
 * ------------------- TCP connections handlers ---------------------
 * ==================================================================
 */


/**
 * Structure for storage of received TCP messages, which 
 * are not asked yet by TAPI user. 
 */
typedef struct tapi_tcp_msg_queue_t {
    CIRCLEQ_ENTRY(tapi_tcp_msg_queue_t) link;

    uint8_t *data;
    size_t   len;

    tapi_tcp_pos_t seqn;
    tapi_tcp_pos_t ackn;

    uint8_t flags;

} tapi_tcp_msg_queue_t;

/**
 * Structure for descriptor of TCP connection, handled by TAPI and TAD
 */
typedef struct tapi_tcp_connection_t {
    CIRCLEQ_ENTRY(tapi_tcp_connection_t) link;

    tapi_tcp_handler_t id;

    const char    *agt; 
    int            rcv_sid;
    int            snd_sid;
    csap_handle_t  rcv_csap;
    csap_handle_t  snd_csap;

    char loc_iface[IFNAME_SIZE];

    uint8_t loc_mac[ETH_ALEN];
    uint8_t rem_mac[ETH_ALEN];

    struct sockaddr_storage loc_addr;
    struct sockaddr_storage rem_addr;

    int window;

    tapi_tcp_pos_t seq_got;
    tapi_tcp_pos_t seq_sent;

    tapi_tcp_pos_t ack_got;
    tapi_tcp_pos_t ack_sent;

    size_t         last_len_got;
    size_t         last_len_sent;


    CIRCLEQ_HEAD(tapi_tcp_msg_queue_head, tapi_tcp_msg_queue_t) 
        *messages; 
} tapi_tcp_connection_t;


CIRCLEQ_HEAD(tapi_tcp_conn_list_head, tapi_tcp_connection_t) 
    *conns_root = NULL;


static inline void
tapi_tcp_conns_db_init(void)
{
    if (conns_root == NULL)
    {
        conns_root = calloc(1, sizeof(*conns_root));
        CIRCLEQ_INIT(conns_root);
    }
}



/**
 * Find TCP connection descriptor by handle
 *
 * @param handler       identifier of TCP connecion
 *
 * @return pointer to found descriptor or NULL if not found.
 */
static inline tapi_tcp_connection_t *
tapi_tcp_find_conn(tapi_tcp_handler_t handler)
{
    tapi_tcp_connection_t *conn;

    tapi_tcp_conns_db_init();

    for (conn = conns_root->cqh_first; conn != (void *) conns_root;
         conn = (void *) conn->link.cqe_next)
    {
        if (conn->id == handler)
            return conn;
    }
    return NULL;
}

/**
 * Add new TCP connection descriptor into databse and assign hanlder to it.
 * Descriptor structure should be already filled with TCP-related info.
 *
 * @param descr         pointer to descriptor to be inserted
 *
 * @return status code
 */
/* see description in tapi_tcp.h */
static inline int
tapi_tcp_insert_conn(tapi_tcp_connection_t *descr)
{
    tapi_tcp_conns_db_init();

    descr->id = conns_root->cqh_last->id + 1;

    CIRCLEQ_INSERT_TAIL(conns_root, descr, link);

    return 0;
}
/**
 * Remove TCP connection descriptor from data base
 *
 * @param conn_descr    pointer to descriptor 
 *
 * @return status code
 */
static inline int
tapi_tcp_destroy_conn_descr(tapi_tcp_connection_t *conn_descr)
{ 
    tapi_tcp_conns_db_init();

    if (conn_descr == NULL || conn_descr == (void *)conns_root)
        return 0;

    if (conn_descr->rcv_csap != CSAP_INVALID_HANDLE)
    {
        int rc = rcf_ta_csap_destroy(conn_descr->agt, conn_descr->rcv_sid,
                                     conn_descr->rcv_csap);
        if (rc != 0)
        {
            WARN("%s(id %d): rcv CSAP %d on agt %s destroy failed %X", 
                 __FUNCTION__, conn_descr->id, conn_descr->rcv_csap,
                 conn_descr->agt, rc);
        }
    } 

    if (conn_descr->snd_csap != CSAP_INVALID_HANDLE)
    {
        int rc = rcf_ta_csap_destroy(conn_descr->agt, conn_descr->snd_sid,
                                     conn_descr->snd_csap);
        if (rc != 0)
        {
            WARN("%s(id %d): snd CSAP %d on agt %s destroy failed %X", 
                 __FUNCTION__, conn_descr->id, conn_descr->snd_csap,
                 conn_descr->agt, rc);
        }
    } 
    CIRCLEQ_REMOVE(conns_root, conn_descr, link);
    free(conn_descr);
    return 0;
}


/* handler for data, see description of 'rcf_pkt_handler' type */
static void 
tapi_tad_pkt_handler(char *pkt_file, void *user_param)
{
    tapi_tcp_connection_t *conn_descr = (tapi_tcp_connection_t *)user_param;
    asn_value *tcp_message = NULL;
    int rc, syms; 

    const asn_value *val, *subval;
    const asn_value *tcp_pdu;

    if (pkt_file == NULL || user_param == NULL)
    {
        WARN("%s(): receive strange arguments", __FUNCTION__);
        return;
    }
#define CHECK_ERROR(msg_) \
    do {                                                        \
        if (rc != 0)                                            \
        {                                                       \
            asn_free_value(tcp_message);                        \
            ERROR("%s(): %s, rc %X", __FUNCTION__, msg_, rc);   \
            return;                                             \
        }                                                       \
    } while (0)

    rc = asn_parse_dvalue_in_file(pkt_file, ndn_raw_packet,
                                  &tcp_message, &syms);
    if (rc != 0)
    {
        ERROR("%s(): cannot parse message file: %X, sym %d", 
              __FUNCTION__, rc, syms);
        return;
    }
    val = tcp_message;
    rc = asn_get_child_value(val, &subval, PRIVATE, NDN_PKT_PDUS);
    CHECK_ERROR("get pdus error");

    val = subval;
    rc = asn_get_indexed(val, &subval, 0);
    CHECK_ERROR("get TCP gen pdu error");

    val = subval;
    rc = asn_get_choice_value(val, &subval, NULL, NULL);
    CHECK_ERROR("get TCP special choice error");

    tcp_pdu = subval;

    UNUSED(conn_descr);

#undef CHECK_ERROR
}



/* See description in tapi_tcp.h */
int
tapi_tcp_init_connection(const char *agt, tapi_tcp_mode_t mode, 
                         struct sockaddr *local_addr, 
                         struct sockaddr *remote_addr, 
                         const char *local_iface,
                         uint8_t *local_mac, uint8_t *remote_mac,
                         int window, int timeout,
                         tapi_tcp_handler_t *handler)
{
    int rc;
    int syms;
    int rcv_sid;
    int snd_sid;
    csap_handle_t rcv_csap = CSAP_INVALID_HANDLE;
    csap_handle_t snd_csap = CSAP_INVALID_HANDLE;

    struct sockaddr_in *local_in_addr  = (struct sockaddr_in *)local_addr;
    struct sockaddr_in *remote_in_addr = (struct sockaddr_in *)remote_addr;

    tapi_tcp_connection_t *conn_descr = NULL;

    tapi_tcp_conns_db_init();

    if (agt == NULL || local_addr == NULL || remote_addr == NULL)
        return TE_RC(TE_TAPI, ETEWRONGPTR);

    /*
     * TODO: Make automatic investigation of local interface and
     * MAC addresses.
     */
    if (local_iface == NULL || local_mac == NULL || remote_mac == NULL)
        return TE_RC(TE_TAPI, ETENOSUPP);

#define CHECK_ERROR(cond_, msg_...)\
    do {                           \
        if (cond_)                 \
        {                          \
            ERROR(msg_);           \
            goto cleanup;          \
        }                          \
    } while (0)

    rc = rcf_ta_create_session(agt, &rcv_sid);
    CHECK_ERROR(rc != 0, "%s(); create rcv session failed %X",
                __FUNCTION__, rc);

    rc = rcf_ta_create_session(agt, &snd_sid);
    CHECK_ERROR(rc != 0, "%s(); create snd session failed %X",
                __FUNCTION__, rc);

    rc = tapi_tcp_ip4_eth_csap_create(agt, rcv_sid, local_iface,
                                      local_mac, remote_mac,
                                      local_in_addr->sin_addr.s_addr,
                                      remote_in_addr->sin_addr.s_addr,
                                      local_in_addr->sin_port,
                                      remote_in_addr->sin_port,
                                      &rcv_csap); 
    CHECK_ERROR(rc != 0, "%s(): rcv csap create failed %X",
                __FUNCTION__, rc);

    rc = tapi_tcp_ip4_eth_csap_create(agt, snd_sid, local_iface,
                                      local_mac, remote_mac,
                                      local_in_addr->sin_addr.s_addr,
                                      remote_in_addr->sin_addr.s_addr,
                                      local_in_addr->sin_port,
                                      remote_in_addr->sin_port,
                                      &snd_csap);
    CHECK_ERROR(rc != 0, "%s(): snd csap create failed %X",
                __FUNCTION__, rc);

    conn_descr = calloc(1, sizeof(*conn_descr));
    conn_descr->rcv_csap = rcv_csap;
    conn_descr->rcv_sid = rcv_sid;
    conn_descr->snd_csap = snd_csap;
    conn_descr->snd_sid = snd_sid;
    conn_descr->agt = agt;
    conn_descr->window = ((window == 0) ? 1000 : window);
    tapi_tcp_insert_conn(conn_descr); 

    /* wait SYN - if we are server */
    if (mode == TAPI_TCP_SERVER)
    {
        asn_value *syn_pattern;
        uint8_t flag_mask, flag_value;

        rc = asn_parse_value_text("{{pdus {tcp:{}, ip4:{}, eth:{}}}}",
                                  ndn_traffic_pattern,
                                  &syn_pattern, &syms);
        CHECK_ERROR(rc != 0, "%s(): parse pattern failed, rc %X, sym %d",
                    __FUNCTION__, rc, syms);
        flag_mask = 0xff;
        flag_value = TCP_SYN_FLAG;

        asn_write_value_field(syn_pattern, &flag_mask, sizeof(flag_mask),
                              "0.pdus.0.#tcp.flags.#mask.m");
        asn_write_value_field(syn_pattern, &flag_value, sizeof(flag_value),
                              "0.pdus.0.#tcp.flags.#mask.v");

        rc = tapi_tad_trrecv_start(agt, conn_descr->rcv_sid,
                                   conn_descr->rcv_csap, syn_pattern, 
                                   tapi_tad_pkt_handler, conn_descr, 
                                   timeout, 1); 
    }

    UNUSED(timeout);
    UNUSED(handler);
    return 0;

cleanup:
    tapi_tcp_destroy_conn_descr(conn_descr);
    return TE_RC(TE_TAPI, rc);
}
