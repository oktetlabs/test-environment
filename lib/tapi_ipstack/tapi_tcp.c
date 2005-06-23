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

    if (ack_flag)
        *msg |= (1 << 4); 
    if (syn_flag)
        *msg |= (1 << 1); 
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



typedef struct tapi_tcp_connection_t {
    struct tapi_tcp_connection_t *next,
                                 *prev;
    tapi_tcp_handler_t id;

    const char    *agt; 
    int            sid;
    csap_handle_t  tcp_csap;

    char loc_iface[IFNAME_SIZE];

    uint8_t loc_mac[ETH_ALEN];
    uint8_t rem_mac[ETH_ALEN];

    struct sockaddr_storage loc_addr;
    struct sockaddr_storage rem_addr;

    int window;

    tapi_tcp_pos_t pos_incoming;
    tapi_tcp_pos_t pos_outgoing;


} tapi_tcp_connection_t;


tapi_tcp_connection_t conns_root = {NULL, NULL, 0, };

int
tapi_tcp_init_connection(const char *agt, tapi_tcp_mode_t mode, 
                         struct sockaddr *local_addr, 
                         struct sockaddr *remote_addr, 
                         const char *local_iface,
                         uint8_t *local_mac, uint8_t *remote_mac,
                         int timeout, tapi_tcp_handler_t *handler)
{
    int rc;
    int sid;
    csap_handle_t tcp_csap = CSAP_INVALID_HANDLE;

    struct sockaddr_in *local_in_addr  = (struct sockaddr_in *)local_addr;
    struct sockaddr_in *remote_in_addr = (struct sockaddr_in *)remote_addr;

    tapi_tcp_connection_t *conn_descr = NULL;

    if (agt == NULL || local_addr == NULL || remote_addr == NULL)
        return TE_RC(TE_TAPI, ETEWRONGPTR);

    /*
     * TODO: Make automatic investigation of local interface and
     * MAC addresses.
     */
    if (local_iface == NULL || local_mac == NULL || remote_mac == NULL)
        return TE_RC(TE_TAPI, ETENOSUPP);

#define CHECK_ERROR(cond_, msg_...)\
    do {                                \
        if (cond_)                      \
        {                               \
            ERROR(msg_);                \
            return TE_RC(TE_TAPI, rc);  \
        }                               \
    } while (0)

    rc = rcf_ta_create_session(agt, &sid);
    CHECK_ERROR(rc != 0, "%s(); create session failed %X",
                __FUNCTION__, rc);

    rc = tapi_tcp_ip4_eth_csap_create(agt, sid, local_iface,
                                      local_mac, remote_mac,
                                      local_in_addr->sin_addr.s_addr,
                                      remote_in_addr->sin_addr.s_addr,
                                      local_in_addr->sin_port,
                                      remote_in_addr->sin_port,
                                      &tcp_csap);

    UNUSED(conn_descr);
    UNUSED(mode);
    UNUSED(timeout);
    UNUSED(handler);
    return 0;
}
