/** @file
 * @brief Test API for TAD. ipstack CSAP
 *
 * Implementation of Test API
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 * @author: Konstantin Abramenko <konst@oktetlabs.ru>
 *
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

#include "ndn_ipstack.h"
#include "ndn_eth.h"
#include "ndn_socket.h"

#include "tapi_ndn.h"
#include "tapi_tad.h"
#include "tapi_tad_internal.h"
#include "tapi_eth.h"
#include "tapi_ip4.h"
#include "tapi_ip6.h"
#include "tapi_tcp.h"
#include "tapi_arp.h"

#include "tapi_test.h"

#include "te_string.h"

/* See the description in tapi_tcp.h */
te_errno
tapi_tcp_add_csap_layer(asn_value **csap_spec,
                        int         local_port,
                        int         remote_port)
{
    asn_value  *layer;

    if (local_port > 0xffff || remote_port > 0xffff)
    {
        WARN("%s() EINVAL: local port %d, remote port %d",
             __FUNCTION__, local_port, remote_port);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    CHECK_RC(tapi_tad_csap_add_layer(csap_spec, ndn_tcp_csap, "#tcp",
                                     &layer));

    if (local_port >= 0)
        CHECK_RC(asn_write_int32(layer, ntohs(local_port),
                                 "local-port.#plain"));
    if (remote_port >= 0)
        CHECK_RC(asn_write_int32(layer, ntohs(remote_port),
                                 "remote-port.#plain"));

    return 0;
}

/* See description in tapi_tcp.h */
te_errno
tapi_tcp_ip4_eth_csap_create(const char *ta_name, int sid,
                             const char *eth_dev,
                             unsigned int receive_mode,
                             const uint8_t *loc_mac,
                             const uint8_t *rem_mac,
                             in_addr_t loc_addr,
                             in_addr_t rem_addr,
                             int loc_port,
                             int rem_port,
                             csap_handle_t *tcp_csap)
{
    te_errno    rc;

    asn_value *csap_spec = NULL;

    do {
        int num = 0;

        rc = asn_parse_value_text("{ layers { tcp:{}, ip4:{}, eth:{} } }",
                                  ndn_csap_spec, &csap_spec, &num);
        if (rc) break;

        if (receive_mode != 0)
            rc = asn_write_int32(csap_spec, receive_mode,
                                 "layers.2.#eth.receive-mode");
        if (rc) break;

        if (eth_dev)
            rc = asn_write_value_field(csap_spec, eth_dev, strlen(eth_dev),
                                       "layers.2.#eth.device-id.#plain");
        if (rc) break;

        if (loc_mac)
            rc = asn_write_value_field(csap_spec,
                    loc_mac, 6, "layers.2.#eth.local-addr.#plain");
        if (rc) break;

        if (rem_mac)
            rc = asn_write_value_field(csap_spec,
                    rem_mac, 6, "layers.2.#eth.remote-addr.#plain");
        if (rc) break;

        if(loc_addr)
            rc = asn_write_value_field(csap_spec,
                                       &loc_addr, sizeof(loc_addr),
                                       "layers.1.#ip4.local-addr.#plain");
        if (rc) break;

        if(rem_addr)
            rc = asn_write_value_field(csap_spec,
                                       &rem_addr, sizeof(rem_addr),
                                       "layers.1.#ip4.remote-addr.#plain");
        if (rc) break;

        if (loc_port >= 0)
            rc = asn_write_int32(csap_spec, ntohs(loc_port),
                                 "layers.0.#tcp.local-port.#plain");
        if (rc) break;

        if (rem_port >= 0)
            rc = asn_write_int32(csap_spec, ntohs(rem_port),
                                 "layers.0.#tcp.remote-port.#plain");
        if (rc) break;

        rc = tapi_tad_csap_create(ta_name, sid, "tcp.ip4.eth",
                                  csap_spec, tcp_csap);
    } while (0);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}

/* See description in tapi_tcp.h */
te_errno
tapi_tcp_ip4_csap_create(const char *ta_name, int sid,
                         const char *ifname,
                         in_addr_t loc_addr,
                         in_addr_t rem_addr,
                         int loc_port,
                         int rem_port,
                         csap_handle_t *tcp_csap)
{
    te_errno    rc;

    asn_value *csap_spec = NULL;

    do {
        rc = tapi_tcp_add_csap_layer(&csap_spec, loc_port, rem_port);
        if (rc != 0)
        {
            WARN("%s(): add UDP csap layer failed %r", __FUNCTION__, rc);
            break;
        }

        rc = tapi_ip4_add_csap_layer(&csap_spec, loc_addr, rem_addr,
                                     -1 /* default proto */,
                                     -1 /* default ttl */,
                                     -1 /* default tos */);
        if (rc != 0)
        {
            WARN("%s(): add IP4 csap layer failed %r", __FUNCTION__, rc);
            break;
        }

        rc = asn_write_string(csap_spec, ifname,
                              "layers.1.#ip4.ifname.#plain");
        if (rc != 0)
        {
            WARN("%s(): write IP4 layer value 'ifname' failed %r",
                 __FUNCTION__, rc);
            break;
        }

        rc = tapi_tad_csap_create(ta_name, sid, "tcp.ip4",
                                  csap_spec, tcp_csap);
    } while (0);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}

/**
 * data for tcp/ip4 callback
 */
typedef struct {
    tcp4_message   *msg;
    void           *user_data;
    tcp4_callback   callback;
} tcp4_cb_data_t;

/**
 * data for tcp callback
 */
typedef struct {
    tcp_message_t  *msg;
    void           *user_data;
    tcp_callback    callback;
} tcp_cb_data_t;

/**
 * Convert TCP packet ASN value to plain C structure
 *
 * @param pkt           ASN value of type DHCPv4 message or Generic-PDU with
 *                      choice "dhcp"
 * @param udp_dgram     converted structure (OUT)
 *
 * @return zero on success or error code
 *
 * @note Function allocates memory under dhcp_message data structure, which
 * should be freed with dhcpv4_message_destroy
 */
int
ndn_tcp4_message_to_plain(asn_value *pkt, tcp4_message **tcp_msg)
{
    int         rc = 0;
    int32_t     hdr_field;
    size_t      len;
    size_t      ip_pld_len;
    size_t      tcp_hdr_len;
    size_t      payload_len;
    asn_value  *pdu;

    *tcp_msg = (struct tcp4_message *)malloc(sizeof(**tcp_msg));
    if (*tcp_msg == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    memset(*tcp_msg, 0, sizeof(**tcp_msg));

    pdu = asn_read_indexed(pkt, 0, "pdus"); /* this should be TCP PDU */

    if (pdu == NULL)
        ERROR_CLEANUP(rc, TE_EASNINCOMPLVAL, "failed to get TCP PDU");

    READ_PACKET_FIELD(rc, pdu, *tcp_msg, src, port);
    READ_PACKET_FIELD(rc, pdu, *tcp_msg, dst, port);

    rc = ndn_du_read_plain_int(pdu, NDN_TAG_TCP_FLAGS, &hdr_field);
    CHECK_ERROR_CLEANUP(rc, "failed to get TCP flags");
    (*tcp_msg)->flags = hdr_field;

    rc = ndn_du_read_plain_int(pdu, NDN_TAG_TCP_HLEN, &hdr_field);
    CHECK_ERROR_CLEANUP(rc, "failed to get TCP header length");
    tcp_hdr_len = hdr_field * sizeof(uint32_t);

    pdu = asn_read_indexed(pkt, 1, "pdus"); /* this should be IPv4 PDU */
    if (pdu == NULL)
        ERROR_CLEANUP(rc, TE_EASNINCOMPLVAL, "failed to get IPv4 PDU");

    READ_PACKET_FIELD(rc, pdu, *tcp_msg, src, addr);
    READ_PACKET_FIELD(rc, pdu, *tcp_msg, dst, addr);

    rc = tapi_ip4_get_payload_len(pdu, &ip_pld_len);
    CHECK_ERROR_CLEANUP(rc, "tapi_ip4_get_payload_len() fails");

    if (ip_pld_len < tcp_hdr_len)
    {
        ERROR_CLEANUP(rc, TE_EINVAL,
                      "IPv4 payload length is less than TCP header length");
    }

    payload_len = ip_pld_len - tcp_hdr_len;

    rc = asn_get_length(pkt, "payload");
    if (rc < 0)
    {
        WARN("%s(): failed to get payload length, assuming there was none",
             __FUNCTION__);
        len = 0;
    }
    else
    {
        len = rc;
    }
    rc = 0;

    if (len < payload_len)
    {
        ERROR_CLEANUP(rc, TE_EINVAL,
                      "obtained payload length is less than specified by "
                      "length fields in IPv4 and TCP headers");
    }

    if (len > 0)
    {
        (*tcp_msg)->payload_len = payload_len;
        (*tcp_msg)->payload = malloc(len);

        rc = asn_read_value_field(pkt, (*tcp_msg)->payload,
                                  &len, "payload");
        CHECK_ERROR_CLEANUP(rc, "failed to read payload");
    }

cleanup:

    if (rc != 0)
    {
        if ((*tcp_msg)->payload != NULL)
            free((*tcp_msg)->payload);

        free(*tcp_msg);
        *tcp_msg = NULL;

        return TE_RC(TE_TAPI, rc);
    }

    return TE_RC(TE_TAPI, rc);
}

/* See description in tapi_tcp.h */
te_errno
ndn_tcp_message_to_plain(asn_value *pkt, tcp_message_t **tcp_msg)
{
    int         rc = 0;
    asn_value  *pdu;
    int32_t     hdr_field;
    uint8_t     ip_version;
    size_t      ip_pld_len;
    size_t      tcp_hdr_len;
    size_t      payload_len;
    uint16_t    src_port;
    uint16_t    dst_port;
    size_t      len;

    *tcp_msg = (tcp_message_t *)TE_ALLOC(sizeof(tcp_message_t));
    if (*tcp_msg == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    pdu = asn_read_indexed(pkt, 0, "pdus"); /* this should be TCP PDU */
    if (pdu == NULL)
        ERROR_CLEANUP(rc, TE_EASNINCOMPLVAL, "failed to get TCP PDU");

    rc = ndn_du_read_plain_int(pdu, NDN_TAG_TCP_FLAGS, &hdr_field);
    CHECK_ERROR_CLEANUP(rc, "failed to get TCP flags");
    (*tcp_msg)->flags = hdr_field;

    rc = ndn_du_read_plain_int(pdu, NDN_TAG_TCP_HLEN, &hdr_field);
    CHECK_ERROR_CLEANUP(rc, "failed to get TCP header length");
    tcp_hdr_len = hdr_field * sizeof(uint32_t);

    rc = ndn_du_read_plain_int(pdu, NDN_TAG_TCP_SRC_PORT, &hdr_field);
    CHECK_ERROR_CLEANUP(rc, "failed to get TCP src port");
    src_port = hdr_field;

    rc = ndn_du_read_plain_int(pdu, NDN_TAG_TCP_DST_PORT, &hdr_field);
    CHECK_ERROR_CLEANUP(rc, "failed to get TCP dst port");
    dst_port = hdr_field;

    pdu = asn_read_indexed(pkt, 1, "pdus"); /* this should be IP PDU */
    if (pdu == NULL)
        ERROR_CLEANUP(rc, TE_EASNINCOMPLVAL, "failed to get IP PDU");

    len = sizeof(ip_version);
    rc = asn_read_value_field(pdu, &ip_version, &len, "version");
    CHECK_ERROR_CLEANUP(rc, "failed to get IP version");

    switch (ip_version)
    {
        case 4:
            len = sizeof(SIN(&((*tcp_msg)->source_sa))->sin_addr);
            rc = asn_read_value_field(
                 pdu,
                 &(SIN(&((*tcp_msg)->source_sa))->sin_addr),
                 &len,
                 "src-addr");
            CHECK_ERROR_CLEANUP(rc, "failed to get IP src addr");

            rc = asn_read_value_field(
                 pdu,
                 &(SIN(&((*tcp_msg)->dest_sa))->sin_addr),
                 &len,
                 "dst-addr");
            CHECK_ERROR_CLEANUP(rc, "failed to get IP dst addr");

            SIN(&((*tcp_msg)->source_sa))->sin_port = src_port;
            SIN(&((*tcp_msg)->dest_sa))->sin_port = dst_port;

            rc = tapi_ip4_get_payload_len(pdu, &ip_pld_len);
            CHECK_ERROR_CLEANUP(rc, "tapi_ip4_get_payload_len() fails");

            break;

        case 6:
            len = sizeof(SIN6(&((*tcp_msg)->source_sa))->sin6_addr);
            rc = asn_read_value_field(
                 pdu,
                 &(SIN6(&((*tcp_msg)->source_sa))->sin6_addr),
                 &len,
                 "src-addr");
            CHECK_ERROR_CLEANUP(rc, "failed to get IP src addr");

            rc = asn_read_value_field(
                 pdu,
                 &(SIN6(&((*tcp_msg)->dest_sa))->sin6_addr),
                 &len,
                 "dst-addr");
            CHECK_ERROR_CLEANUP(rc, "failed to get IP dst addr");

            SIN6(&((*tcp_msg)->source_sa))->sin6_port = src_port;
            SIN6(&((*tcp_msg)->dest_sa))->sin6_port = dst_port;

            rc = tapi_ip6_get_payload_len(pdu, &ip_pld_len);
            CHECK_ERROR_CLEANUP(rc, "tapi_ip6_get_payload_len() fails");

            break;

        default:
            ERROR_CLEANUP(rc, TE_EINVAL, "Unknown IP version: %i", ip_version);
    }

    if (ip_pld_len < tcp_hdr_len)
    {
        ERROR_CLEANUP(rc, TE_EINVAL,
                      "IP payload length is less than TCP header length");
    }

    payload_len = ip_pld_len - tcp_hdr_len;

    rc = asn_get_length(pkt, "payload");
    if (rc < 0)
    {
        WARN("%s(): failed to get payload length, assuming there was none",
             __FUNCTION__);
        len = 0;
    }
    else
    {
        len = rc;
    }
    rc = 0;

    if (len < payload_len)
    {
        ERROR_CLEANUP(rc, TE_EINVAL,
                      "obtained payload length is less than specified by "
                      "length fields in IP and TCP headers");
    }

    if (len > 0)
    {
        (*tcp_msg)->pld_len = payload_len;
        (*tcp_msg)->payload = TE_ALLOC(len);

        if ((*tcp_msg)->payload == NULL)
            ERROR_CLEANUP(rc, TE_ENOMEM, "Fail to allocate memory for TCP "
                                         "payload");

        rc = asn_read_value_field(pkt, (*tcp_msg)->payload,
                                  &len, "payload");
        CHECK_ERROR_CLEANUP(rc, "failed to read payload");
    }

cleanup:
    if (rc != 0)
    {
        if ((*tcp_msg)->payload != NULL)
            free((*tcp_msg)->payload);

        free(*tcp_msg);
        *tcp_msg = NULL;
    }

    return TE_RC(TE_TAPI, rc);
}

static void
tcp4_asn_pkt_handler(asn_value *pkt, void *user_param)
{
    tcp4_cb_data_t *cb_data = (tcp4_cb_data_t *)user_param;
    te_errno        rc;

    rc = ndn_tcp4_message_to_plain(pkt, &cb_data->msg);
    if (rc != 0)
    {
        fprintf(stderr, "ndn_tcp4_message_to_plain fails, rc = %x\n", rc);
        return;
    }
    if (cb_data->callback != NULL)
    {
        cb_data->callback(cb_data->msg, cb_data->user_data);
        if (cb_data->msg->payload)
            free(cb_data->msg->payload);
        free(cb_data->msg);
        cb_data->msg = NULL;
    }
    asn_free_value(pkt);
}

static void
tcp_asn_pkt_handler(asn_value *pkt, void *user_param)
{
    tcp_cb_data_t  *cb_data = (tcp_cb_data_t *)user_param;
    te_errno        rc;

    rc = ndn_tcp_message_to_plain(pkt, &cb_data->msg);
    if (rc != 0)
    {
        ERROR("ndn_tcp_message_to_plain fails, rc = %r", rc);
        return;
    }
    if (cb_data->callback != NULL)
    {
        cb_data->callback(cb_data->msg, cb_data->user_data);
        free(cb_data->msg->payload);
        free(cb_data->msg);
        cb_data->msg = NULL;
    }
    asn_free_value(pkt);
}

/* See description in tapi_tcp.h */
tapi_tad_trrecv_cb_data *
tapi_tcp_ip4_eth_trrecv_cb_data(tcp4_callback callback, void *user_data)
{
    tcp4_cb_data_t             *cb_data;
    tapi_tad_trrecv_cb_data    *res;

    cb_data = (tcp4_cb_data_t *)calloc(1, sizeof(*cb_data));
    if (cb_data == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return NULL;
    }
    cb_data->callback = callback;
    cb_data->user_data = user_data;

    res = tapi_tad_trrecv_make_cb_data(tcp4_asn_pkt_handler, cb_data);
    if (res == NULL)
        free(cb_data);

    return res;
}

/* See description in tapi_tcp.h */
tapi_tad_trrecv_cb_data *
tapi_tcp_ip_eth_trrecv_cb_data(tcp_callback callback, void *user_data)
{
    tcp_cb_data_t              *cb_data;
    tapi_tad_trrecv_cb_data    *res;

    cb_data = (tcp_cb_data_t *)TE_ALLOC(sizeof(tcp_cb_data_t));
    if (cb_data == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return NULL;
    }
    cb_data->callback = callback;
    cb_data->user_data = user_data;

    res = tapi_tad_trrecv_make_cb_data(tcp_asn_pkt_handler, cb_data);
    if (res == NULL)
        free(cb_data);

    return res;
}

/* See description in tapi_tcp.h */
te_errno
tapi_tcp_ip4_pattern_unit(in_addr_t src_addr, in_addr_t dst_addr,
                          int src_port, int dst_port,
                          asn_value **result_value)
{
    int rc;
    int num;
    asn_value *pu = NULL;

    do {
        if (result_value == NULL) { rc = TE_EWRONGPTR; break; }

        rc = asn_parse_value_text("{ pdus { tcp:{}, ip4:{}, eth:{}}}",
                              ndn_traffic_pattern_unit, &pu, &num);

        if (rc) break;
        if (src_addr != htonl(INADDR_ANY))
            rc = asn_write_value_field(pu, &src_addr, 4,
                                       "pdus.1.#ip4.src-addr.#plain");

        if (rc) break;
        if (dst_addr != htonl(INADDR_ANY))
            rc = asn_write_value_field(pu, &dst_addr, 4,
                                       "pdus.1.#ip4.dst-addr.#plain");

        if (rc) break;
        if (src_port >= 0) /* SRC port passed here in network byte order */
            rc = asn_write_int32(pu, ntohs(src_port),
                                 "pdus.0.#tcp.src-port.#plain");

        if (rc) break;
        if (dst_port >= 0) /* DST port passed here in network byte order */
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
te_errno
tapi_tcp_ip4_eth_recv_start(const char *ta_name, int sid,
                            csap_handle_t csap,
                            in_addr_t src_addr, in_addr_t dst_addr,
                            int src_port, int dst_port,
                            unsigned int timeout, unsigned int num,
                            rcf_trrecv_mode mode)
{
    asn_value  *pattern;
    asn_value  *pattern_unit;
    te_errno    rc;

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
te_errno
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
te_errno
tapi_tcp_pdu(int src_port, int dst_port,
             tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn,
             te_bool syn_flag, te_bool ack_flag,
             asn_value **pdu)
{
    te_errno    rc;
    int         syms;
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

    if (src_port >= 0 &&
        (rc = ndn_du_write_plain_int(tcp_pdu, NDN_TAG_TCP_SRC_PORT,
                                     ntohs(src_port))) != 0)
    {
        ERROR("%s(): set TCP src port failed %r", __FUNCTION__, rc);
        asn_free_value(g_pdu);
        return TE_RC(TE_TAPI, rc);
    }

    if (dst_port >= 0 &&
        (rc = ndn_du_write_plain_int(tcp_pdu, NDN_TAG_TCP_DST_PORT,
                                     htons(dst_port))) != 0)
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
tapi_tcp_template_gen(te_bool is_eth_pdu, te_bool force_ip6,
                      tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn,
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

    rc = asn_parse_value_text(!force_ip6 ?
                              (is_eth_pdu ? "{ pdus { ip4:{}, eth:{} } }" :
                                            "{ pdus { ip4:{} } }") :
                              (is_eth_pdu ? "{ pdus { ip6:{}, eth:{} } }" :
                                            "{ pdus { ip6:{} } }"),
                              ndn_traffic_template, tmpl, &syms);

    if (rc != 0)
    {
        ERROR("%s(): cannot parse template: %r, sym %d",
              __FUNCTION__, rc, syms);
        return TE_RC(TE_TAPI, rc);
    }

    if (!force_ip6)
    {
        rc = asn_write_bool(*tmpl, TRUE, "pdus.0.#ip4.dont-frag.#plain");
        if (rc != 0)
        {
            ERROR("%s(): write ip4 dont-frag flag error: %r", __FUNCTION__, rc);
            goto cleanup;
        }
    }

    rc = tapi_tcp_pdu(-1, -1, seqn, ackn, syn_flag, ack_flag, &tcp_pdu);
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

/* See description in tapi_tcp.h */
int
tapi_tcp_template(te_bool force_ip6, tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn,
                  te_bool syn_flag, te_bool ack_flag,
                  uint8_t *data, size_t pld_len,
                  asn_value **tmpl)
{
    return tapi_tcp_template_gen(TRUE, force_ip6, seqn, ackn, syn_flag,
                                 ack_flag, data, pld_len, tmpl);
}

int
tapi_tcp_ip_segment_pattern_gen(te_bool is_eth_pdu, te_bool force_ip6,
                                tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn,
                                te_bool urg_flag, te_bool ack_flag,
                                te_bool psh_flag, te_bool rst_flag,
                                te_bool syn_flag, te_bool fin_flag,
                                asn_value **pattern)
{
    int         rc = 0;
    int         syms;
    int32_t     flags;
    asn_value  *tcp_pdu = NULL;
    asn_value  *raw_tcp_pdu = NULL;

    if (pattern == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    *pattern = NULL;

    if ((rc = asn_parse_value_text(
                (is_eth_pdu && force_ip6) ?
                    "{{ pdus { ip6:{}, eth:{} } }}" :
                        (is_eth_pdu && !force_ip6) ?
                            "{{ pdus { ip4:{}, eth:{} } }}" :
                                (force_ip6) ?
                                    "{{ pdus { ip6:{} } }}" :
                                        "{{ pdus { ip4:{} } }}",
                ndn_traffic_pattern, pattern, &syms)) != 0)
    {
        ERROR("%s(): cannot parse pattern: %r, sym %d",
              __FUNCTION__, rc, syms);
        return TE_RC(TE_TAPI, rc);
    }

    if ((rc = tapi_tcp_pdu(-1, -1, seqn, ackn, syn_flag,
                           ack_flag, &tcp_pdu)) != 0)
    {
        ERROR("%s(): make tcp pdu eror: %r", __FUNCTION__, rc);
        goto cleanup;
    }

    if (seqn == 0)
    {
        rc = asn_free_subvalue(tcp_pdu, "#tcp.seqn");
        WARN("%s(): free seqn rc %r", __FUNCTION__, rc);
    }

    if (ackn == 0)
    {
        rc = asn_free_subvalue(tcp_pdu, "#tcp.ackn");
        WARN("%s(): free seqn rc %r", __FUNCTION__, rc);
    }

    asn_get_choice_value(tcp_pdu, &raw_tcp_pdu, NULL, NULL);

    ndn_du_read_plain_int(raw_tcp_pdu, NDN_TAG_TCP_FLAGS, &flags);

    /* Set more flags if necessary */
    if (urg_flag)
        flags |= TCP_URG_FLAG;

    if (psh_flag)
        flags |= TCP_PSH_FLAG;

    if (rst_flag)
        flags |= TCP_RST_FLAG;

    if (fin_flag)
        flags |= TCP_FIN_FLAG;

    ndn_du_write_plain_int(raw_tcp_pdu, NDN_TAG_TCP_FLAGS, flags);

    if ((rc = asn_insert_indexed(*pattern, tcp_pdu, 0, "0.pdus")) != 0)
    {
        ERROR("%s(): insert tcp pdu eror: %r", __FUNCTION__, rc);
        goto cleanup;
    }

cleanup:
    if (rc != 0)
        asn_free_value(*pattern);

    return TE_RC(TE_TAPI, rc);
}

int
tapi_tcp_ip_pattern_gen(te_bool is_eth_pdu, te_bool force_ip6,
                        tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn,
                        te_bool syn_flag, te_bool ack_flag,
                        asn_value **pattern)
{
    return tapi_tcp_ip_segment_pattern_gen(is_eth_pdu, force_ip6,
                                           seqn, ackn,
                                           FALSE, ack_flag,
                                           FALSE, FALSE,
                                           syn_flag, FALSE,
                                           pattern);
}

int
tapi_tcp_segment_pattern(tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn,
                         te_bool urg_flag, te_bool ack_flag,
                         te_bool psh_flag, te_bool rst_flag,
                         te_bool syn_flag, te_bool fin_flag,
                         asn_value **pattern)
{
    return tapi_tcp_ip_segment_pattern_gen(TRUE, FALSE, seqn, ackn,
                                           urg_flag, ack_flag,
                                           psh_flag, rst_flag,
                                           syn_flag, fin_flag,
                                           pattern);
}

int
tapi_tcp_ip_segment_pattern(te_bool force_ip6,
                            tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn,
                            te_bool urg_flag, te_bool ack_flag,
                            te_bool psh_flag, te_bool rst_flag,
                            te_bool syn_flag, te_bool fin_flag,
                            asn_value **pattern)
{
    return tapi_tcp_ip_segment_pattern_gen(TRUE, force_ip6, seqn, ackn,
                                           urg_flag, ack_flag,
                                           psh_flag, rst_flag,
                                           syn_flag, fin_flag,
                                           pattern);
}

/* See description in tapi_tcp.h */
int
tapi_tcp_pattern_gen(te_bool is_eth_pdu,
                     tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn,
                     te_bool syn_flag, te_bool ack_flag,
                     asn_value **pattern)
{
    int rc = 0,
        syms;

    asn_value *tcp_pdu = NULL;

    if (pattern == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    *pattern = NULL;

    rc = asn_parse_value_text(is_eth_pdu ?
                              "{{ pdus {ip4:{}, eth:{} } }}" :
                              "{{ pdus {ip4:{}} }}",
                              ndn_traffic_pattern,
                              pattern, &syms);
    if (rc != 0)
    {
        ERROR("%s(): cannot parse template: %r, sym %d",
              __FUNCTION__, rc, syms);
        return TE_RC(TE_TAPI, rc);
    }

    rc = tapi_tcp_pdu(-1, -1, seqn, ackn, syn_flag, ack_flag, &tcp_pdu);
    if (rc != 0)
    {
        ERROR("%s(): make tcp pdu eror: %r", __FUNCTION__, rc);
        goto cleanup;
    }

    if (seqn == 0)
    {
        rc = asn_free_subvalue(tcp_pdu, "#tcp.seqn");
        WARN("%s(): free seqn rc %r", __FUNCTION__, rc);
    }

    if (ackn == 0)
    {
        rc = asn_free_subvalue(tcp_pdu, "#tcp.ackn");
        if (ack_flag)
            WARN("%s(): free ackn rc %r", __FUNCTION__, rc);
    }

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
tapi_tcp_pattern(tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn,
                 te_bool syn_flag, te_bool ack_flag,
                 asn_value **pattern)
{
    return tapi_tcp_pattern_gen(TRUE, seqn, ackn,
                                syn_flag, ack_flag, pattern);
}

/* See description in tapi_tcp.h */
te_errno
tapi_tcp_segment_pdu(int src_port, int dst_port,
                     tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn,
                     te_bool urg_flag, te_bool ack_flag,
                     te_bool psh_flag, te_bool rst_flag,
                     te_bool syn_flag, te_bool fin_flag,
                     asn_value **pdu)
{
    te_errno    rc;
    int         syms;
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

    if (src_port >= 0 &&
        (rc = ndn_du_write_plain_int(tcp_pdu, NDN_TAG_TCP_SRC_PORT,
                                     ntohs(src_port))) != 0)
    {
        ERROR("%s(): set TCP src port failed %r", __FUNCTION__, rc);
        asn_free_value(g_pdu);
        return TE_RC(TE_TAPI, rc);
    }

    if (dst_port >= 0 &&
        (rc = ndn_du_write_plain_int(tcp_pdu, NDN_TAG_TCP_DST_PORT,
                                     htons(dst_port))) != 0)
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
    if (urg_flag)
        flags |= TCP_URG_FLAG;
    if (ack_flag)
        flags |= TCP_ACK_FLAG;
    if (psh_flag)
        flags |= TCP_PSH_FLAG;
    if (rst_flag)
        flags |= TCP_RST_FLAG;
    if (syn_flag)
        flags |= TCP_SYN_FLAG;
    if (fin_flag)
        flags |= TCP_FIN_FLAG;

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
tapi_tcp_ip_segment_template_gen(te_bool is_eth_pdu, te_bool force_ip6,
                                 tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn,
                                 te_bool urg_flag, te_bool ack_flag,
                                 te_bool psh_flag, te_bool rst_flag,
                                 te_bool syn_flag, te_bool fin_flag,
                                 uint8_t *data, size_t pld_len,
                                 asn_value **tmpl)
{
    int         rc = 0;
    int         syms;
    asn_value  *tcp_pdu = NULL;

    if (tmpl == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    *tmpl = NULL;

    if ((rc = asn_parse_value_text(
                (is_eth_pdu && force_ip6) ?
                    "{ pdus {ip6:{}, eth:{} } }" :
                        (is_eth_pdu && !force_ip6) ?
                            "{ pdus {ip4:{}, eth:{} } }" :
                                (force_ip6) ?
                                    "{ pdus {ip6:{} } }" :
                                        "{ pdus {ip4:{} } }",
                ndn_traffic_template, tmpl, &syms)) != 0)
    {
        ERROR("%s(): cannot parse template: %r, sym %d",
              __FUNCTION__, rc, syms);
        return TE_RC(TE_TAPI, rc);
    }

    if ((rc = tapi_tcp_segment_pdu(-1, -1, seqn, ackn,
                                   urg_flag, ack_flag, psh_flag,
                                   rst_flag, syn_flag, fin_flag,
                                   &tcp_pdu)) != 0)
    {
        ERROR("%s(): make tcp pdu eror: %r", __FUNCTION__, rc);
        goto cleanup;
    }

    if (data != NULL && pld_len > 0)
    {
        if ((rc = asn_write_value_field(*tmpl, data, pld_len,
                                        "payload.#bytes")) != 0)
        {
            ERROR("%s(): write payload eror: %r", __FUNCTION__, rc);
            goto cleanup;
        }
    }

    if ((rc = asn_insert_indexed(*tmpl, tcp_pdu, 0, "pdus")) != 0)
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
tapi_tcp_ip_template_gen(te_bool is_eth_pdu, te_bool force_ip6,
                         tapi_tcp_pos_t seqn, tapi_tcp_pos_t ackn,
                         te_bool syn_flag, te_bool ack_flag,
                         uint8_t *data, size_t pld_len,
                         asn_value **tmpl)
{
    return tapi_tcp_ip_segment_template_gen(is_eth_pdu, force_ip6,
                                            seqn, ackn,
                                            FALSE, ack_flag,
                                            FALSE, FALSE,
                                            syn_flag, FALSE,
                                            data, pld_len,
                                            tmpl);
}

/* See description in tapi_tcp.h */
int
tapi_tcp_segment_template(tapi_tcp_pos_t seqn,
                          tapi_tcp_pos_t ackn,
                          te_bool urg_flag, te_bool ack_flag,
                          te_bool psh_flag, te_bool rst_flag,
                          te_bool syn_flag, te_bool fin_flag,
                          uint8_t *data, size_t pld_len,
                          asn_value **tmpl)
{
    return tapi_tcp_ip_segment_template_gen(TRUE, FALSE, seqn, ackn,
                                            urg_flag, ack_flag,
                                            psh_flag, rst_flag,
                                            syn_flag, fin_flag,
                                            data, pld_len,
                                            tmpl);
}

/* See description in tapi_tcp.h */
int
tapi_tcp_ip_segment_template(te_bool force_ip6,
                             tapi_tcp_pos_t seqn,
                             tapi_tcp_pos_t ackn,
                             te_bool urg_flag, te_bool ack_flag,
                             te_bool psh_flag, te_bool rst_flag,
                             te_bool syn_flag, te_bool fin_flag,
                             uint8_t *data, size_t pld_len,
                             asn_value **tmpl)
{
    return tapi_tcp_ip_segment_template_gen(TRUE, force_ip6, seqn, ackn,
                                            urg_flag, ack_flag,
                                            psh_flag, rst_flag,
                                            syn_flag, fin_flag,
                                            data, pld_len,
                                            tmpl);
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
                                      -1, /* port will be in pattern */
                                      -1, /* we dont know remote port */
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

    context->catched = FALSE;

    return 0;
}


void
tcp_reset_hack_pkt_handler(const char *pkt_file, void *user_param)
{
    int      rc;
    int      syms = 0;
    int32_t  i32_tmp;
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

    rc = asn_read_int32(pkt, &i32_tmp, "pdus.0.#tcp.seqn.#plain");
    if (rc != 0)
    {
        ERROR("%s(): read loc seq failed %r", __FUNCTION__, rc);
        goto cleanup;
    }
    context->loc_start_seq = i32_tmp;
    INFO("%s(): read loc start seq: %u",
         __FUNCTION__, context->loc_start_seq);

    rc = asn_read_int32(pkt, &i32_tmp, "pdus.0.#tcp.ackn.#plain");
    if (rc != 0)
    {
        ERROR("%s(): read rem seq failed %r", __FUNCTION__, rc);
        goto cleanup;
    }
    context->rem_start_seq = i32_tmp;
    INFO("%s(): read rem start seq: %u",
         __FUNCTION__, context->rem_start_seq);


    rc = asn_read_int32(pkt, &i32_tmp, "pdus.0.#tcp.dst-port.#plain");
    if (rc != 0)
    {
        ERROR("%s(): read dst-port for 'ini' side failed %r",
              __FUNCTION__, rc);
        goto cleanup;
    }
    INFO("%s(): read rem port: %d", __FUNCTION__, i32_tmp);
    context->rem_port = i32_tmp;

    if (context->loc_port == 0)
    {
        asn_read_int32(pkt, &i32_tmp, "pdus.0.#tcp.src-port.#plain");
        context->loc_port = i32_tmp;
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

    context->catched = TRUE;

cleanup:
    asn_free_value(pkt);
}

int
tapi_tcp_reset_hack_catch(const char *ta_name, int session,
                          tapi_tcp_reset_hack_t *context)
{
    unsigned int syn_ack_num = 0;
    int rc;

    if (context == NULL)
    {
        ERROR("%s(): null context passed", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = rcf_ta_trrecv_stop(ta_name, session, context->tcp_hack_csap,
                            tcp_reset_hack_pkt_handler,
                            context, &syn_ack_num);

    return rc != 0 ? rc : (context->catched ? 0 : -1);
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

    rc = tapi_tcp_template(FALSE, context->loc_start_seq + sent,
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

int
tapi_tcp_reset_hack_clear(const char *ta_name, int session, 
                          tapi_tcp_reset_hack_t *context)
{
    int rc = 0;

    if (context == NULL)
    {
        ERROR("%s(): null context passed", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (context->tcp_hack_csap != CSAP_INVALID_HANDLE &&
        (rc = tapi_tad_csap_destroy(ta_name, session,
                                    context->tcp_hack_csap)) != 0)
        ERROR("%s(): Failed to destroy CSAP", __FUNCTION__);

    return rc;
}

te_errno
tapi_tcp_ip6_eth_csap_create(const char *ta_name, int sid,
                             const char *eth_dev,
                             unsigned int receive_mode,
                             const uint8_t *loc_mac,
                             const uint8_t *rem_mac,
                             const uint8_t *loc_addr,
                             const uint8_t *rem_addr,
                             int loc_port,
                             int rem_port,
                             csap_handle_t *tcp_csap)
{
    te_errno    rc;

    asn_value *csap_spec = NULL;

    do {
        int num = 0;

        rc = asn_parse_value_text("{ layers { tcp:{}, ip6:{}, eth:{} } }",
                                  ndn_csap_spec, &csap_spec, &num);
        if (rc) break;

        if (receive_mode != 0)
            rc = asn_write_int32(csap_spec, receive_mode,
                                 "layers.2.#eth.receive-mode");
        if (rc) break;

        if (eth_dev)
            rc = asn_write_value_field(csap_spec, eth_dev, strlen(eth_dev),
                                       "layers.2.#eth.device-id.#plain");
        if (rc) break;

        if (loc_mac)
            rc = asn_write_value_field(csap_spec,
                    loc_mac, 6, "layers.2.#eth.local-addr.#plain");
        if (rc) break;

        if (rem_mac)
            rc = asn_write_value_field(csap_spec,
                    rem_mac, 6, "layers.2.#eth.remote-addr.#plain");
        if (rc) break;

        if(loc_addr)
            rc = asn_write_value_field(csap_spec,
                                       loc_addr, 16,
                                       "layers.1.#ip6.local-addr.#plain");
        if (rc) break;

        if(rem_addr)
            rc = asn_write_value_field(csap_spec,
                                       rem_addr, 16,
                                       "layers.1.#ip6.remote-addr.#plain");
        if (rc) break;

        if (loc_port >= 0)
            rc = asn_write_int32(csap_spec, ntohs(loc_port),
                                 "layers.0.#tcp.local-port.#plain");
        if (rc) break;

        if (rem_port >= 0)
            rc = asn_write_int32(csap_spec, ntohs(rem_port),
                                 "layers.0.#tcp.remote-port.#plain");
        if (rc) break;

        rc = tapi_tad_csap_create(ta_name, sid, "tcp.ip6.eth",
                                  csap_spec, tcp_csap);
    } while (0);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}

/**
 * Locate TCP PDU, TCP options and TCP timestamp option in a given ASN
 * value.
 *
 * @param val         Pointer to ASN value which stores either whole network
 *                    packet or TCP PDU.
 * @param p_tcp_pdu   Where to save pointer to TCP PDU.
 * @param p_options   Where to save pointer to TCP options.
 * @param p_ts_opt    Where to save pointer to TCP timestamp option.
 *
 * @return Status code.
 */
static te_errno
find_ts_opt(asn_value *val, asn_value **p_tcp_pdu, asn_value **p_options,
            asn_value **p_ts_opt)
{
    asn_value *pdus = NULL;
    asn_value *tcp_pdu = NULL;
    asn_value *options = NULL;
    asn_value *ts_opt = NULL;
    te_errno   rc = 0;

    rc = asn_get_subvalue(val, &pdus, "pdus");
    if (rc == 0)
    {
        tcp_pdu = asn_find_child_choice_value(pdus, TE_PROTO_TCP);
        if (tcp_pdu == NULL)
        {
            ERROR("%s(): failed to find TCP PDU", __FUNCTION__);
            return TE_ENOENT;
        }
    }
    else
    {
        tcp_pdu = val;
    }

    if (p_tcp_pdu != NULL)
        *p_tcp_pdu = tcp_pdu;

    rc = asn_get_subvalue(tcp_pdu, &options, "options");
    if (rc != 0)
        return TE_ENOENT;

    if (p_options != NULL)
        *p_options = options;

    ts_opt = asn_find_child_choice_value(options,
                                         NDN_TAG_TCP_OPT_TIMESTAMP);
    if (ts_opt == NULL)
        return TE_ENOENT;

    if (p_ts_opt != NULL)
        *p_ts_opt = ts_opt;

    return 0;
}

/* See description in tapi_tcp.h */
te_errno
tapi_tcp_get_ts_opt(const asn_value *val,
                    uint32_t *ts_value, uint32_t *ts_echo)
{
    asn_value *ts_opt = NULL;
    te_errno   rc = 0;

    rc = find_ts_opt((asn_value *)val, NULL, NULL, &ts_opt);
    if (rc != 0)
        return rc;

    if (ts_value != NULL)
    {
        rc = asn_read_uint32(ts_opt, ts_value, "value");
        if (rc != 0)
        {
            ERROR("%s(): failed to read TCP timestamp value: %r",
                  __FUNCTION__, rc);
            return rc;
        }
    }

    if (ts_echo != NULL)
    {
        rc = asn_read_uint32(ts_opt, ts_echo, "echo-reply");
        if (rc != 0)
        {
            ERROR("%s(): failed to read TCP timestamp echo-reply: %r",
                  __FUNCTION__, rc);
            return rc;
        }
    }

    return 0;
}

/* See description in tapi_tcp.h */
te_errno
tapi_tcp_set_ts_opt(asn_value *val,
                    uint32_t ts_value, uint32_t ts_echo)
{
#define CHECK_WRITE_OPT(_pdu, _is_val, _val, _labels...) \
    do {                                                        \
        char        _buf[200];                                  \
        te_string   _labels_str = TE_STRING_BUF_INIT(_buf);     \
                                                                \
        rc = te_string_append(&_labels_str, _labels);           \
        if (rc != 0)                                            \
            return rc;                                          \
        if (_is_val)                                            \
            rc = asn_write_uint32(_pdu, _val, _labels_str.ptr); \
        else                                                    \
            rc = asn_write_value_field(_pdu, NULL, 0,           \
                                       _labels_str.ptr);        \
        if (rc != 0)                                            \
        {                                                       \
            ERROR("%s(): failed to fill '%s'",                  \
                  __FUNCTION__, _labels_str.ptr);               \
            return rc;                                          \
        }                                                       \
    } while (0)

    asn_value *tcp_pdu = NULL;
    asn_value *options = NULL;
    asn_value *ts_opt = NULL;
    int        opts_num;
    te_errno   rc = 0;

    rc = find_ts_opt(val, &tcp_pdu, &options, &ts_opt);
    if (rc != 0 && tcp_pdu == NULL)
    {
        ERROR("%s() failed to find TCP PDU", __FUNCTION__);
        return rc;
    }

    if (ts_opt != NULL)
    {
        CHECK_WRITE_OPT(ts_opt, TRUE, ts_value, "value.#plain");
        CHECK_WRITE_OPT(ts_opt, TRUE, ts_echo, "echo.#plain");
    }
    else if (options != NULL)
    {
        opts_num = asn_get_length(options, "");
        if (opts_num < 0)
        {
            ERROR("%s(): failed to get number of options",
                  __FUNCTION__);
            return TE_EFAIL;
        }

        CHECK_WRITE_OPT(options, TRUE, ts_value,
                        "%u.#timestamp.value.#plain",
                        (unsigned int)opts_num);
        CHECK_WRITE_OPT(options, TRUE, ts_echo,
                        "%u.#timestamp.echo-reply.#plain",
                        (unsigned int)opts_num);

        /*
         * This is done because length of TCP options is defined
         * in 32bit words ("Data offset" field in TCP header), and TCP
         * timestamp takes 80 bits, so we need additional 16 bits
         * for alignment.
         */
        CHECK_WRITE_OPT(options, FALSE, 0,
                        "%u.#nop", (unsigned int)opts_num + 1);
        CHECK_WRITE_OPT(options, FALSE, 0,
                        "%u.#nop", (unsigned int)opts_num + 2);
    }
    else
    {
        CHECK_WRITE_OPT(tcp_pdu, TRUE, ts_value,
                        "options.0.#timestamp.value.#plain");
        CHECK_WRITE_OPT(tcp_pdu, TRUE, ts_echo,
                        "options.0.#timestamp.echo-reply.#plain");
        CHECK_WRITE_OPT(tcp_pdu, FALSE, 0, "options.1.#nop");
        CHECK_WRITE_OPT(tcp_pdu, FALSE, 0, "options.2.#nop");
    }

    return 0;
}

/* See description in tapi_tcp.h */
int
tapi_tcp_compare_seqn(uint32_t seqn1, uint32_t seqn2)
{
    uint32_t diff;

    diff = seqn2 - seqn1;

    if (diff == 0)
        return 0;
    else if ((int32_t)diff > 0)
        return -1;
    else
        return 1;
}

/* See description in tapi_tcp.h */
extern te_errno
tapi_tcp_ip_eth_csap_create(const char        *ta_name,
                            int                sid,
                            const char        *eth_dev,
                            unsigned int       receive_mode,
                            const uint8_t     *loc_mac,
                            const uint8_t     *rem_mac,
                            int                ip_family,
                            const void        *loc_addr,
                            const void        *rem_addr,
                            int                loc_port,
                            int                rem_port,
                            csap_handle_t     *tcp_csap)
{
    if (ip_family == AF_INET)
    {
        in_addr_t loc_ipv4 = htonl(INADDR_ANY);
        in_addr_t rem_ipv4 = htonl(INADDR_ANY);

        if (loc_addr != NULL)
            loc_ipv4 = ((struct in_addr *)loc_addr)->s_addr;
        if (rem_addr != NULL)
            rem_ipv4 = ((struct in_addr *)rem_addr)->s_addr;

        return tapi_tcp_ip4_eth_csap_create(ta_name, sid, eth_dev,
                                            receive_mode,
                                            loc_mac, rem_mac,
                                            loc_ipv4, rem_ipv4,
                                            loc_port, rem_port,
                                            tcp_csap);
    }
    else if (ip_family == AF_INET6)
    {
        return tapi_tcp_ip6_eth_csap_create(ta_name, sid, eth_dev,
                                            receive_mode,
                                            loc_mac, rem_mac,
                                            loc_addr, rem_addr,
                                            loc_port, rem_port,
                                            tcp_csap);
    }

    ERROR("%s(): not supported address family %d", __FUNCTION__, ip_family);
    return TE_RC(TE_TAPI, TE_EINVAL);

}

/* See description in tapi_tcp.h */
te_errno tapi_tcp_get_hdrs_payload_len(asn_value *pkt,
                                       unsigned int *hdrs_len,
                                       unsigned int *pld_len)
{
    uint32_t ip_total_len = 0;
    uint32_t ip_hdr_len = 0;
    uint32_t tcp_hdr_len = 0;
    te_errno rc = 0;

    rc = asn_read_uint32(pkt, &ip_total_len,
                         "pdus.1.#ip4.total-length");
    if (rc == 0)
    {
        rc = asn_read_uint32(pkt, &ip_hdr_len,
                             "pdus.1.#ip4.h-length");
        if (rc != 0)
        {
            ERROR("%s(): failed to get IP4 h-length from CSAP packet: %r",
                  __FUNCTION__, rc);
            return rc;
        }
        ip_hdr_len = ip_hdr_len * 4;
    }
    else
    {
        rc = asn_read_uint32(pkt, &ip_total_len,
                             "pdus.1.#ip6.payload-length");
        if (rc != 0)
        {
            ERROR("%s(): neither IP4 total-length nor IPv6 payload-length "
                  "can be obtained from CSAP packet: %r", __FUNCTION__, rc);
            return rc;
        }

        ip_hdr_len = 40;
        ip_total_len += ip_hdr_len;

        /*
         * TODO: IPv6 extension headers are not processed here.
         */
    }

    rc = asn_read_uint32(pkt, &tcp_hdr_len,
                         "pdus.0.#tcp.hlen");
    if (rc != 0)
    {
        ERROR("%s(): failed to get TCP hlen from CSAP packet: %r",
              __FUNCTION__, rc);
        return rc;
    }
    tcp_hdr_len = tcp_hdr_len * 4;

    if (pld_len != NULL)
        *pld_len = ip_total_len - ip_hdr_len - tcp_hdr_len;
    if (hdrs_len != NULL)
        *hdrs_len = ip_hdr_len + tcp_hdr_len;

    return 0;
}
