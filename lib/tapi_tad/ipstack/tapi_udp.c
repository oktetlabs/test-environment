/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API for TAD. ipstack CSAP
 *
 * Implementation of Test API
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "TAPI UDP"

#include "te_config.h"

#include <stdio.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include <netinet/in.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "rcf_api.h"
#include "conf_api.h"
#include "logger_api.h"

#include "ndn_ipstack.h"
#include "ndn_eth.h"
#include "ndn_socket.h"
#include "tad_common.h"
#include "tapi_ndn.h"
#include "tapi_tad.h"
#include "tapi_tad_internal.h"
#include "tapi_eth.h"
#include "tapi_ip4.h"
#include "tapi_ip6.h"
#include "tapi_udp.h"

#include "tapi_test.h"

/* See the description in tapi_udp.h */
te_errno
tapi_udp_add_csap_layer(asn_value **csap_spec,
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

    CHECK_RC(tapi_tad_csap_add_layer(csap_spec, ndn_udp_csap, "#udp",
                                     &layer));

    if (local_port >= 0)
        CHECK_RC(asn_write_int32(layer, ntohs(local_port),
                                 "local-port.#plain"));
    if (remote_port >= 0)
        CHECK_RC(asn_write_int32(layer, ntohs(remote_port),
                                 "remote-port.#plain"));

    return 0;
}

/* See the description in tapi_udp.h */
te_errno
tapi_udp_add_pdu(asn_value **tmpl_or_ptrn, asn_value **pdu,
                 te_bool is_pattern,
                 int src_port, int dst_port)
{
    asn_value  *tmp_pdu;

    if (src_port > 0xffff || dst_port > 0xffff)
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(tmpl_or_ptrn, is_pattern,
                                          ndn_udp_header, "#udp",
                                          &tmp_pdu));

    if (src_port >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, ntohs(src_port),
                                 "src-port.#plain"));
    if (dst_port >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, ntohs(dst_port),
                                 "dst-port.#plain"));

    if (pdu != NULL)
        *pdu = tmp_pdu;

    return 0;
}

/* See the description in tapi_udp.h */
te_errno
tapi_udp_ip4_eth_csap_create(const char    *ta_name,
                             int            sid,
                             const char    *eth_dev,
                             unsigned int   receive_mode,
                             const uint8_t *loc_mac,
                             const uint8_t *rem_mac,
                             in_addr_t      loc_addr,
                             in_addr_t      rem_addr,
                             int            loc_port,
                             int            rem_port,
                             csap_handle_t *udp_csap)
{
    te_errno    rc;
    asn_value  *csap_spec = NULL;

    rc = tapi_udp_add_csap_layer(&csap_spec, loc_port, rem_port);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add UDP csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }

    rc = tapi_ip4_add_csap_layer(&csap_spec, loc_addr, rem_addr,
                                 -1 /* default proto */,
                                 -1 /* default ttl */,
                                 -1 /* default tos */);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add IP4 csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }

    rc = tapi_eth_add_csap_layer(&csap_spec, eth_dev, receive_mode,
                                 rem_mac, loc_mac,
                                 NULL /* automatic length/type */,
                                 TE_BOOL3_ANY /* untagged/tagged */,
                                 TE_BOOL3_ANY /* Ethernet2/LLC+SNAP */);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add ETH csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }


    rc = tapi_tad_csap_create(ta_name, sid, "udp.ip4.eth", csap_spec,
                              udp_csap);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}

/* See the description in tapi_udp.h */
te_errno
tapi_udp_ip4_csap_create(const char    *ta_name,
                         int            sid,
                         const char    *ifname,
                         in_addr_t      loc_addr,
                         in_addr_t      rem_addr,
                         int            loc_port,
                         int            rem_port,
                         csap_handle_t *udp_csap)
{
    te_errno    rc;
    asn_value  *csap_spec = NULL;

    rc = tapi_udp_add_csap_layer(&csap_spec, loc_port, rem_port);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add UDP csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }

    rc = tapi_ip4_add_csap_layer(&csap_spec, loc_addr, rem_addr,
                                 -1 /* default proto */,
                                 -1 /* default ttl */,
                                 -1 /* default tos */);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add IP4 csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }

    rc = asn_write_string(csap_spec, ifname,
                          "layers.1.#ip4.ifname.#plain");
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): write IP4 layer value 'ifname' failed %r",
             __FUNCTION__, rc);
        return rc;
    }

    rc = tapi_tad_csap_create(ta_name, sid, "udp.ip4", csap_spec,
                              udp_csap);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}

/**
 * data for udp callback
 */
typedef struct {
    udp4_datagram  *dgram;
    void           *user_data;
    udp4_callback   callback;
} udp4_cb_data_t;

/**
 * Convert UDP packet ASN value to plain C structure
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
ndn_udp4_dgram_to_plain(asn_value *pkt, udp4_datagram **udp_dgram)
{
    int         rc = 0;
    int32_t     hdr_field;
    size_t      len;
    size_t      payload_len;
    size_t      ip_pld_len;
    asn_value  *pdu;

    *udp_dgram = (struct udp4_datagram *)malloc(sizeof(**udp_dgram));
    if (*udp_dgram == NULL)
        return TE_RC(TE_TAPI, TE_ENOMEM);

    memset(*udp_dgram, 0, sizeof(**udp_dgram));

    rc = ndn_get_timestamp(pkt, &((*udp_dgram)->ts));
    CHECK_ERROR_CLEANUP(rc, "ndn_get_timestamp() failed");

    pdu = asn_read_indexed(pkt, 0, "pdus"); /* this should be UDP PDU */
    if (pdu == NULL)
        ERROR_CLEANUP(rc, TE_EASNINCOMPLVAL, "failed to get UDP PDU");

    READ_PACKET_FIELD(rc, pdu, *udp_dgram, src, port);
    READ_PACKET_FIELD(rc, pdu, *udp_dgram, dst, port);

    rc = ndn_du_read_plain_int(pdu, NDN_TAG_UDP_CHECKSUM, &hdr_field);
    CHECK_ERROR_CLEANUP(rc, "get UDP checksum fails");

    (*udp_dgram)->checksum = hdr_field;

    pdu = asn_read_indexed(pkt, 1, "pdus"); /* this should be IPv4 PDU */
    if (pdu == NULL)
        ERROR_CLEANUP(rc, TE_EASNINCOMPLVAL, "failed to get IPv4 PDU");

    READ_PACKET_FIELD(rc, pdu, *udp_dgram, src, addr);
    READ_PACKET_FIELD(rc, pdu, *udp_dgram, dst, addr);

    /*
     * Payload length is computed here because CSAP may
     * report padding bytes in small Ethernet frames
     * (which should not be less than 60 bytes even if
     * there is not enough data) as part of payload.
     */

    rc = tapi_ip4_get_payload_len(pdu, &ip_pld_len);
    CHECK_ERROR_CLEANUP(rc, "tapi_ip4_get_payload_len() fails");

    if (ip_pld_len < TAD_UDP_HDR_LEN)
        ERROR_CLEANUP(rc, TE_EINVAL,
                      "IPv4 payload length is less than UDP header length");

    payload_len = ip_pld_len - TAD_UDP_HDR_LEN;

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
                      "length fields in IPv4 header");
    }

    if (len > 0)
    {
        (*udp_dgram)->payload_len = payload_len;
        (*udp_dgram)->payload = malloc(len);

        rc = asn_read_value_field(pkt, (*udp_dgram)->payload,
                                  &len, "payload");
        CHECK_ERROR_CLEANUP(rc, "failed to read payload");
    }

cleanup:

    if (rc != 0)
    {
        if ((*udp_dgram)->payload != NULL)
            free((*udp_dgram)->payload);

        free(*udp_dgram);
        *udp_dgram = NULL;

        return TE_RC(TE_TAPI, rc);
    }

    return 0;
}

/**
 * Create Traffic-Pattern-Unit for udp.ip4.eth CSAP
 *
 * @param src_addr      IPv4 source address (or NULL)
 * @param dst_addr      IPv4 destination address (or NULL)
 * @param src_port      UDP source port in network byte order or -1
 * @param dst_port      UDP destination port network byte order or -1
 * @param result_value  Location for resulting ASN pattern
 *
 * @return Zero on success or error code
 */
static int
tapi_udp_ip4_eth_pattern_unit(const uint8_t *src_addr,
                              const uint8_t *dst_addr,
                              int src_port, int dst_port,
                              asn_value **result_value)
{
    int        rc;
    int        num;
    asn_value *pattern = NULL;

    if (result_value == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    do {
        rc = asn_parse_value_text("{ pdus { udp:{}, ip4:{}, eth:{}}}",
                                  ndn_traffic_pattern_unit, &pattern, &num);
        if (rc != 0)
            break;

        if (src_addr != NULL)
        {
            rc = asn_write_value_field(pattern, src_addr, sizeof(in_addr_t),
                                       "pdus.1.#ip4.src-addr.#plain");
            if (rc != 0)
                break;
        }
        if (dst_addr != NULL)
        {
            rc = asn_write_value_field(pattern, dst_addr, sizeof(in_addr_t),
                                       "pdus.1.#ip4.dst-addr.#plain");
            if (rc != 0)
                break;
        }
        if (src_port >= 0)
        {
            rc = asn_write_int32(pattern, ntohs(src_port),
                                 "pdus.0.#udp.src-port.#plain");
            if (rc != 0)
                break;
        }
        if (dst_port >= 0)
        {
            rc = asn_write_int32(pattern, ntohs(dst_port),
                                 "pdus.0.#udp.dst-port.#plain");
            if (rc != 0)
                break;
        }
    } while (0);

    if (rc != 0)
    {
        ERROR("%s: error %X", __FUNCTION__, rc);
        asn_free_value(pattern);
        return TE_RC(TE_TAPI, rc);
    }
    *result_value = pattern;
    return 0;
}


/* see description in tapi_udp.h */
int
tapi_udp4_csap_create(const char *ta_name, int sid,
                      const char *loc_addr_str, const char *rem_addr_str,
                      uint16_t loc_port, uint16_t rem_port,
                      csap_handle_t *udp_csap)
{
    te_errno        rc;
    asn_value      *csap_spec;
    asn_value      *csap_layers;
    asn_value      *csap_layer_spec;
    asn_value      *csap_socket;

    struct in_addr  loc_addr = { 0 };
    struct in_addr  rem_addr = { 0 };

    if ((loc_addr_str != NULL && inet_aton(loc_addr_str, &loc_addr) == 0) ||
        (rem_addr_str != NULL && inet_aton(rem_addr_str, &rem_addr) == 0))
    {
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    csap_spec       = asn_init_value(ndn_csap_spec);
    csap_layers     = asn_init_value(ndn_csap_layers);
    csap_layer_spec = asn_init_value(ndn_generic_csap_layer);
    csap_socket     = asn_init_value(ndn_socket_csap);

    asn_put_child_value(csap_spec, csap_layers, PRIVATE, NDN_CSAP_LAYERS);
    asn_write_value_field(csap_socket, NULL, 0,
                          "type.#udp");
    asn_write_value_field(csap_socket,
                          &loc_addr, sizeof(loc_addr),
                          "local-addr.#plain");
    asn_write_value_field(csap_socket,
                          &rem_addr, sizeof(rem_addr),
                          "remote-addr.#plain");
    asn_write_int32(csap_socket, loc_port, "local-port.#plain");
    asn_write_int32(csap_socket, rem_port, "remote-port.#plain");

    asn_write_component_value(csap_layer_spec, csap_socket, "#socket");

    asn_insert_indexed(csap_layers, csap_layer_spec, 0, "");

    rc = tapi_tad_csap_create(ta_name, sid, "socket",
                              csap_spec, udp_csap);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}


/* see description in tapi_udp.h */
static void
udp4_asn_pkt_handler(asn_value *pkt, void *user_param)
{
    udp4_cb_data_t *cb_data = (udp4_cb_data_t *)user_param;
    te_errno        rc;

    rc = ndn_udp4_dgram_to_plain(pkt, &cb_data->dgram);
    if (rc != 0)
    {
        fprintf(stderr, "ndn_udp4_dgram_to_plain fails, rc = %x\n", rc);
        return;
    }
    if (cb_data->callback != NULL)
    {
        cb_data->callback(cb_data->dgram, cb_data->user_data);
        if (cb_data->dgram->payload)
            free(cb_data->dgram->payload);
        free(cb_data->dgram);
        cb_data->dgram = NULL;
    }
    asn_free_value(pkt);
}

/* See description in tapi_udp.h */
tapi_tad_trrecv_cb_data *
tapi_udp_ip4_eth_trrecv_cb_data(udp4_callback callback, void *user_data)
{
    udp4_cb_data_t             *cb_data;
    tapi_tad_trrecv_cb_data    *res;

    cb_data = (udp4_cb_data_t *)calloc(1, sizeof(*cb_data));
    if (cb_data == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return NULL;
    }
    cb_data->callback = callback;
    cb_data->user_data = user_data;

    res = tapi_tad_trrecv_make_cb_data(udp4_asn_pkt_handler, cb_data);
    if (res == NULL)
        free(cb_data);

    return res;
}



/* See description in tapi_udp.h */
int
tapi_udp_ip4_eth_recv_start(const char *ta_name,  int sid,
                            csap_handle_t csap,
                            rcf_trrecv_mode mode)
{
    int              rc;
    unsigned int     timeout = TAD_TIMEOUT_INF;
    asn_value       *pattern;
    asn_value       *pattern_unit;

    rc = tapi_udp_ip4_eth_pattern_unit(NULL, NULL, -1, -1, &pattern_unit);
    if (rc != 0)
    {
        ERROR("%s: pattern unit creation error: %r", __FUNCTION__, rc);
        return rc;
    }
    pattern = asn_init_value(ndn_traffic_pattern);
    if ((rc = asn_insert_indexed(pattern, pattern_unit, 0, "")) != 0)
    {
        asn_free_value(pattern);
        ERROR("%s: pattern unit insertion error: %r", __FUNCTION__, rc);
        return rc;
    }

    rc = tapi_tad_trrecv_start(ta_name, sid, csap, pattern,
                               timeout, 0, mode);
    asn_free_value(pattern);

    return rc;
}

te_errno
tapi_udp_ip6_eth_csap_create(const char    *ta_name,
                             int            sid,
                             const char    *eth_dev,
                             unsigned int   receive_mode,
                             const uint8_t *loc_mac,
                             const uint8_t *rem_mac,
                             const uint8_t *loc_addr,
                             const uint8_t *rem_addr,
                             int            loc_port,
                             int            rem_port,
                             csap_handle_t *udp_csap)
{
    te_errno    rc;
    asn_value  *csap_spec = NULL;

    rc = tapi_udp_add_csap_layer(&csap_spec, loc_port, rem_port);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add UDP csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }

    rc = tapi_ip6_add_csap_layer(&csap_spec,
                                 loc_addr, rem_addr,
                                 IPPROTO_UDP);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add IP6 csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }

    rc = tapi_eth_add_csap_layer(&csap_spec, eth_dev, receive_mode,
                                 rem_mac, loc_mac,
                                 NULL /* automatic length/type */,
                                 TE_BOOL3_ANY /* untagged/tagged */,
                                 TE_BOOL3_ANY /* Ethernet2/LLC+SNAP */);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        WARN("%s(): add ETH csap layer failed %r", __FUNCTION__, rc);
        return rc;
    }


    rc = tapi_tad_csap_create(ta_name, sid, "udp.ip6.eth", csap_spec,
                              udp_csap);

    asn_free_value(csap_spec);

    return TE_RC(TE_TAPI, rc);
}

/* See description in tapi_udp.h */
te_errno
tapi_udp_ip_eth_csap_create(const char    *ta_name,
                            int            sid,
                            const char    *eth_dev,
                            unsigned int   receive_mode,
                            const uint8_t *loc_mac,
                            const uint8_t *rem_mac,
                            int            ip_family,
                            const void    *loc_addr,
                            const void    *rem_addr,
                            int            loc_port,
                            int            rem_port,
                            csap_handle_t *udp_csap)
{
    if (ip_family == AF_INET)
    {
        in_addr_t loc_ipv4 = htonl(INADDR_ANY);
        in_addr_t rem_ipv4 = htonl(INADDR_ANY);

        if (loc_addr != NULL)
            loc_ipv4 = ((struct in_addr *)loc_addr)->s_addr;
        if (rem_addr != NULL)
            rem_ipv4 = ((struct in_addr *)rem_addr)->s_addr;

        return tapi_udp_ip4_eth_csap_create(ta_name, sid, eth_dev,
                                            receive_mode,
                                            loc_mac, rem_mac,
                                            loc_ipv4, rem_ipv4,
                                            loc_port, rem_port,
                                            udp_csap);
    }
    else if (ip_family == AF_INET6)
    {
        return tapi_udp_ip6_eth_csap_create(ta_name, sid, eth_dev,
                                            receive_mode,
                                            loc_mac, rem_mac,
                                            loc_addr, rem_addr,
                                            loc_port, rem_port,
                                            udp_csap);
    }

    ERROR("%s(): not supported address family %d", __FUNCTION__, ip_family);
    return TE_RC(TE_TAPI, TE_EINVAL);
}
