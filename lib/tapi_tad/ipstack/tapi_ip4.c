/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API for TAD. IP stack CSAP
 *
 * Implementation of Test API
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "TAPI IPv4"

#include "te_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_stdint.h"
#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "tapi_ndn.h"
#include "tapi_tad.h"
#include "ndn_eth.h"
#include "tapi_eth.h"
#include "ndn_ipstack.h"
#include "tapi_ip4.h"

#include "tapi_test.h"


/* See the description in tapi_ip4.h */
te_errno
tapi_ip4_add_csap_layer(asn_value **csap_spec,
                        in_addr_t   local_addr,
                        in_addr_t   remote_addr,
                        int         ip_proto,
                        int         ttl,
                        int         tos)
{
    asn_value  *layer;

    if (ip_proto > 0xff || ttl > 0xff || tos > 0xff)
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(tapi_tad_csap_add_layer(csap_spec, ndn_ip4_csap, "#ip4",
                                     &layer));

    if (local_addr != htonl(INADDR_ANY))
        CHECK_RC(asn_write_value_field(layer,
                                       &local_addr, sizeof(local_addr),
                                       "local-addr.#plain"));
    if (remote_addr != htonl(INADDR_ANY))
        CHECK_RC(asn_write_value_field(layer,
                                       &remote_addr, sizeof(remote_addr),
                                       "remote-addr.#plain"));
    if (ip_proto >= 0)
        CHECK_RC(asn_write_int32(layer, ip_proto, "protocol.#plain"));
    if (ttl >= 0)
        CHECK_RC(asn_write_int32(layer, ttl, "time-to-live.#plain"));
    if (tos >= 0)
        CHECK_RC(asn_write_int32(layer, tos, "type-of-service.#plain"));

    return 0;
}


/* See the description in tapi_ip4.h */
te_errno
tapi_ip4_add_pdu(asn_value **tmpl_or_ptrn, asn_value **pdu,
                 te_bool is_pattern,
                 in_addr_t src_addr, in_addr_t dst_addr,
                 int ip_proto, int ttl, int tos)
{
    asn_value  *tmp_pdu;

    if (ip_proto > 0xff || ttl > 0xff || tos > 0xff)
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(tmpl_or_ptrn, is_pattern,
                                          ndn_ip4_header, "#ip4",
                                          &tmp_pdu));

    if (src_addr != htonl(INADDR_ANY))
        CHECK_RC(asn_write_value_field(tmp_pdu,
                                       &src_addr, sizeof(src_addr),
                                       "src-addr.#plain"));
    if (dst_addr != htonl(INADDR_ANY))
        CHECK_RC(asn_write_value_field(tmp_pdu,
                                       &dst_addr, sizeof(dst_addr),
                                       "dst-addr.#plain"));
    if (ip_proto >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, ip_proto, "protocol.#plain"));
    if (ttl >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, ttl, "time-to-live.#plain"));
    if (tos >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, tos, "type-of-service.#plain"));

    if (pdu != NULL)
        *pdu = tmp_pdu;

    return 0;
}

/* See the description in tapi_ip4.h */
te_errno
tapi_ip4_pdu_tmpl_fragments(asn_value **tmpl, asn_value **pdu,
                            tapi_ip_frag_spec *fragments,
                            unsigned int num_frags)
{
    return tapi_ip_pdu_tmpl_fragments(tmpl, pdu, TRUE,
                                      fragments, num_frags);
}

/* See the description in tapi_ip4.h */
te_errno
tapi_ip4_eth_csap_create(const char *ta_name, int sid,
                         const char *eth_dev, unsigned int receive_mode,
                         const uint8_t *loc_mac_addr,
                         const uint8_t *rem_mac_addr,
                         in_addr_t      loc_ip4_addr,
                         in_addr_t      rem_ip4_addr,
                         int            ip_proto,
                         csap_handle_t *ip4_csap)
{
    const uint16_t  ip_eth = ETHERTYPE_IP;
    te_errno        rc = 0;
    asn_value      *csap_spec = NULL;

    rc = tapi_ip4_add_csap_layer(&csap_spec,
                                 loc_ip4_addr, rem_ip4_addr,
                                 ip_proto, -1 /* default TTL */,
                                 -1 /* default TOS */);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        return rc;
    }
    rc = tapi_eth_add_csap_layer(&csap_spec, eth_dev, receive_mode,
                                 rem_mac_addr, loc_mac_addr, &ip_eth,
                                 TE_BOOL3_ANY /* tagged/untagged */,
                                 TE_BOOL3_ANY /* Ethernet2/LLC+SNAP */);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        return rc;
    }

    rc = tapi_tad_csap_create(ta_name, sid, "ip4.eth",
                              csap_spec, ip4_csap);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        return rc;
    }

    return 0;
}

/* See the description in tapi_ip4.h */
te_errno
tapi_ip4_csap_create(const char *ta_name, int sid,
                     in_addr_t      loc_ip4_addr,
                     in_addr_t      rem_ip4_addr,
                     int            ip_proto,
                     csap_handle_t *ip4_csap)
{
    te_errno        rc = 0;
    asn_value      *csap_spec = NULL;

    rc = tapi_ip4_add_csap_layer(&csap_spec,
                                 loc_ip4_addr, rem_ip4_addr,
                                 ip_proto, -1 /* default TTL */,
                                 -1 /* default TOS */);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        return rc;
    }

    rc = tapi_tad_csap_create(ta_name, sid, "ip4",
                              csap_spec, ip4_csap);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        return rc;
    }

    return 0;
}


typedef struct tapi_ip4_eth_pkt_handler_data {
    ip4_callback  callback;
    void         *user_data;
} tapi_ip4_eth_pkt_handler_data;

/*
 * Pkt handler for IP packets
 */
static void
ip4_pkt_handler(asn_value *pkt, void *user_param)
{
    tapi_ip4_eth_pkt_handler_data *cb_data =
        (tapi_ip4_eth_pkt_handler_data *)user_param;

    const asn_value    *ip_pdu;
    tapi_ip4_packet_t   plain_pkt;
    size_t              len;

    int rc = 0;

    int32_t hdr_field;

    if (user_param == NULL)
    {
        WARN("%s called with NULL user param", __FUNCTION__);
        return;
    }

    if (cb_data->callback == NULL)
    {
        WARN("%s called with NULL user cb", __FUNCTION__);
        return;
    }

#define CHECK_FAIL(msg_...) \
    do {                        \
        if (rc != 0)            \
        {                       \
            ERROR(msg_);        \
            return;             \
        }                       \
    } while (0)

    rc = asn_get_descendent(pkt, (asn_value **)&ip_pdu, "pdus.0.#ip4");
    CHECK_FAIL("%s(): get IP4 PDU fails, rc = %r",
              __FUNCTION__, rc);

    len = sizeof(plain_pkt.src_addr);
    rc = ndn_du_read_plain_oct(ip_pdu, NDN_TAG_IP4_SRC_ADDR,
                               (uint8_t *)&(plain_pkt.src_addr), &len);
    CHECK_FAIL("%s(): get IP4 src fails, rc = %r",
              __FUNCTION__, rc);

    len = sizeof(plain_pkt.dst_addr);
    rc = ndn_du_read_plain_oct(ip_pdu, NDN_TAG_IP4_DST_ADDR,
                               (uint8_t *)&(plain_pkt.dst_addr), &len);
    CHECK_FAIL("%s(): get IP4 dst fails, rc = %r",
              __FUNCTION__, rc);

    rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP4_PROTOCOL, &hdr_field);
    CHECK_FAIL("%s(): get IP4 proto fails, rc = %r", __FUNCTION__, rc);
    plain_pkt.ip_proto = hdr_field;

    len = plain_pkt.pld_len = asn_get_length(pkt, "payload");
    plain_pkt.payload = malloc(len);

    rc = asn_read_value_field(pkt, plain_pkt.payload, &len, "payload");

    rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP4_HLEN, &hdr_field);
    CHECK_FAIL("%s(): get IP4 header length fails, rc = %r",
               __FUNCTION__, rc);
    plain_pkt.hlen = hdr_field;

    rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP4_LEN, &hdr_field);
    CHECK_FAIL("%s(): get IP4 total length fails, rc = %r",
               __FUNCTION__, rc);
    plain_pkt.len = hdr_field;

    rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP4_IDENT, &hdr_field);
    CHECK_FAIL("%s(): get IP4 ident fails, rc = %r", __FUNCTION__, rc);
    plain_pkt.ip_ident = hdr_field;

    rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP4_FRAG_OFFSET, &hdr_field);
    CHECK_FAIL("%s(): get IP4 frag offset fails, rc = %r",
               __FUNCTION__, rc);
    plain_pkt.offset = hdr_field;

    rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP4_TTL, &hdr_field);
    CHECK_FAIL("%s(): get IP4 TTL fails, rc = %r",
               __FUNCTION__, rc);
    plain_pkt.ttl = hdr_field;

    rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP4_TOS, &hdr_field);
    CHECK_FAIL("%s(): get IP4 ToS fails, rc = %r",
               __FUNCTION__, rc);
    plain_pkt.tos = hdr_field;

    rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP4_MORE_FRAGS, &hdr_field);
    CHECK_FAIL("%s(): get IP4 more_frags flag fails, rc = %r",
               __FUNCTION__, rc);
    plain_pkt.more_frags = hdr_field != 0 ? TRUE : FALSE ;

    cb_data->callback(&plain_pkt, cb_data->user_data);

    free(plain_pkt.payload);
#undef CHECK_FAIL
}

/* see description in tapi_ip4.h */
tapi_tad_trrecv_cb_data *
tapi_ip4_eth_trrecv_cb_data(ip4_callback  callback,
                            void         *user_data)
{
    tapi_ip4_eth_pkt_handler_data   *cb_data;
    tapi_tad_trrecv_cb_data         *res;

    cb_data = (tapi_ip4_eth_pkt_handler_data *)calloc(1, sizeof(*cb_data));
    if (cb_data == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return NULL;
    }
    cb_data->callback = callback;
    cb_data->user_data = user_data;

    res = tapi_tad_trrecv_make_cb_data(ip4_pkt_handler, cb_data);
    if (res == NULL)
        free(cb_data);

    return res;
}

/* See the description in tapi_ip4.h */
te_errno
tapi_ip4_get_payload_len(asn_value *pdu, size_t *len)
{
    size_t      ip_len;
    size_t      ip_hdr_len;
    int32_t     hdr_field;
    te_errno    rc;

    rc = ndn_du_read_plain_int(pdu, NDN_TAG_IP4_HLEN, &hdr_field);
    if (rc != 0)
    {
        ERROR("%s(): ndn_du_read_plain_int(NDN_TAG_IP4_HLEN) failed, rc=%r",
              __FUNCTION__, rc);
        return rc;
    }
    ip_hdr_len = hdr_field * 4;

    rc = ndn_du_read_plain_int(pdu, NDN_TAG_IP4_LEN, &hdr_field);
    if (rc != 0)
    {
        ERROR("%s(): ndn_du_read_plain_int(NDN_TAG_IP4_LEN) failed, rc=%r",
              __FUNCTION__, rc);
        return rc;
    }
    ip_len = hdr_field;

    if (ip_len < ip_hdr_len)
    {
        ERROR("%s(): IPv4 header length %" TE_PRINTF_SIZE_T "u is greater "
              "than IPv4 length %" TE_PRINTF_SIZE_T "u", __FUNCTION__,
              ip_hdr_len, ip_len);
        return TE_EINVAL;
    }

    *len = ip_len - ip_hdr_len;
    return 0;
}




/*
 * See the description in tapi_ip4.h.
 *
 * Avoid usage of this function.
 */
te_errno
tapi_ip4_template(tapi_ip_frag_spec *fragments, unsigned int num_frags,
                  int ttl, int protocol,
                  const uint8_t *payload, size_t pld_len,
                  asn_value **result_value)
{
    te_errno    rc;
    asn_value *ip4_pdu;

#define MY_CHECK_RC(msg_...) \
    do {                                \
        if (rc != 0)                    \
        {                               \
            ERROR(msg_);                \
            return TE_RC(TE_TAPI, rc);  \
        }                               \
    } while (0)

    if (result_value == NULL)
        return TE_RC(TE_TAPI, TE_EWRONGPTR);

    rc = tapi_ip4_add_pdu(result_value, &ip4_pdu, FALSE /* template */,
                          htonl(INADDR_ANY), htonl(INADDR_ANY),
                          protocol, ttl, -1 /* default TOS */);
    MY_CHECK_RC("%s(): tapi_ip4_add_pdu() failed: %r", __FUNCTION__, rc);

    rc = tapi_ip4_pdu_tmpl_fragments(NULL, &ip4_pdu, fragments, num_frags);
    MY_CHECK_RC("%s(): tapi_ip4_pdu_tmpl_fragments() failed: %r",
                __FUNCTION__, rc);

    rc = asn_write_value_field(*result_value, payload, pld_len,
                               "payload.#bytes");
    MY_CHECK_RC("%s(): write payload error %X", __FUNCTION__, rc);

#undef MY_CHECK_RC

    return 0;
}
