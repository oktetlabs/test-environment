/** @file
 * @brief Test API for TAD. IP stack CSAP
 *
 * Implementation of Test API
 *
 * Copyright (C) 2012 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
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
 * @author Konstantin Petrov <Konstantin.Petrov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER "TAPI IPv6"

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
#include "tapi_ip6.h"

#include "tapi_test.h"

typedef struct tapi_ip6_eth_pkt_handler_data {
    ip6_callback  callback;
    void         *user_data;
} tapi_ip6_eth_pkt_handler_data;

static void
ip6_pkt_handler(asn_value *pkt, void *user_param)
{
    tapi_ip6_eth_pkt_handler_data *cb_data =
        (tapi_ip6_eth_pkt_handler_data *)user_param;

    const asn_value    *ip_pdu;
    tapi_ip6_packet_t   plain_pkt;
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

    rc = asn_get_descendent(pkt, (asn_value **)&ip_pdu, "pdus.0.#ip6");
    CHECK_FAIL("%s(): get IP6 PDU fails, rc = %r",
              __FUNCTION__, rc);

    /* Field 'priority' if necessary */

    /* Flow label */
    rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP6_FLAB, &hdr_field);
    CHECK_FAIL("%s(): get IP6 data length fails, rc = %r",
               __FUNCTION__, rc);
    plain_pkt.data_len = hdr_field;

    /* Data length */
    rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP6_LEN, &hdr_field);
    CHECK_FAIL("%s(): get IP6 data length fails, rc = %r",
               __FUNCTION__, rc);
    plain_pkt.data_len = hdr_field;

    /* Next header */
    rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP6_NEXT_HEADER, &hdr_field);
    CHECK_FAIL("%s(): get IP6 next header fails, rc = %r", __FUNCTION__, rc);
    plain_pkt.next_header = hdr_field;

    /* Hop limit */
    rc = ndn_du_read_plain_int(ip_pdu, NDN_TAG_IP6_HLIM, &hdr_field);
    CHECK_FAIL("%s(): get IP6 hop limit fails, rc = %r",
               __FUNCTION__, rc);
    plain_pkt.hop_limit = hdr_field;

    /* Source addr */
    len = 16;
    rc = ndn_du_read_plain_oct(ip_pdu, NDN_TAG_IP6_SRC_ADDR,
                               plain_pkt.src_addr, &len);
    CHECK_FAIL("%s(): get IP6 src fails, rc = %r",
              __FUNCTION__, rc);

    /* Destination addr */
    len = 16;
    rc = ndn_du_read_plain_oct(ip_pdu, NDN_TAG_IP6_DST_ADDR,
                               plain_pkt.dst_addr, &len);
    CHECK_FAIL("%s(): get IP6 dst fails, rc = %r",
              __FUNCTION__, rc);

    /* Extension headers if necessary */

    len = plain_pkt.pld_len = asn_get_length(pkt, "payload");
    plain_pkt.payload = malloc(len);

    /* Payload */
    rc = asn_read_value_field(pkt, plain_pkt.payload, &len, "payload");

    cb_data->callback(&plain_pkt, cb_data->user_data);

    free(plain_pkt.payload);
#undef CHECK_FAIL
}

tapi_tad_trrecv_cb_data *
tapi_ip6_eth_trrecv_cb_data(ip6_callback  callback,
                            void         *user_data)
{
    tapi_ip6_eth_pkt_handler_data   *cb_data;
    tapi_tad_trrecv_cb_data         *res;

    cb_data = (tapi_ip6_eth_pkt_handler_data *)calloc(1, sizeof(*cb_data));
    if (cb_data == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return NULL;
    }
    cb_data->callback = callback;
    cb_data->user_data = user_data;

    res = tapi_tad_trrecv_make_cb_data(ip6_pkt_handler, cb_data);
    if (res == NULL)
        free(cb_data);

    return res;
}

te_errno
tapi_ip6_add_csap_layer(asn_value **csap_spec,
                        const uint8_t *local_addr,
                        const uint8_t *remote_addr,
                        int next_header)
{
    asn_value  *layer;

    if (next_header > 0xff)
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(tapi_tad_csap_add_layer(csap_spec, ndn_ip6_csap, "#ip6",
                                     &layer));

    if (local_addr != NULL && !IN6_IS_ADDR_UNSPECIFIED(local_addr))
        CHECK_RC(asn_write_value_field(layer,
                                       local_addr, sizeof(struct in6_addr),
                                       "local-addr.#plain"));
    if (remote_addr != NULL && !IN6_IS_ADDR_UNSPECIFIED(remote_addr))
        CHECK_RC(asn_write_value_field(layer,
                                       remote_addr, sizeof(struct in6_addr),
                                       "remote-addr.#plain"));
    if (next_header >= 0)
        CHECK_RC(asn_write_int32(layer, next_header, "next-header.#plain"));

    return 0;
}

te_errno
tapi_ip6_eth_csap_create(const char *ta_name, int sid,
                         const char *eth_dev, unsigned int receive_mode,
                         const uint8_t *loc_mac_addr,
                         const uint8_t *rem_mac_addr,
                         const uint8_t *loc_ip6_addr,
                         const uint8_t *rem_ip6_addr,
                         int next_header, csap_handle_t *ip6_csap)
{
    const uint16_t  ip_eth = ETHERTYPE_IPV6;
    te_errno        rc = 0;
    asn_value      *csap_spec = NULL;

    rc = tapi_ip6_add_csap_layer(&csap_spec,
                                 loc_ip6_addr, rem_ip6_addr,
                                 next_header);
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

    rc = tapi_tad_csap_create(ta_name, sid, "ip6.eth",
                              csap_spec, ip6_csap);
    if (rc != 0)
    {
        asn_free_value(csap_spec);
        return rc;
    }

    return 0;
}

te_errno
tapi_ip6_add_pdu(asn_value **tmpl_or_ptrn, asn_value **pdu,
                 te_bool is_pattern,
                 const uint8_t *src_addr, const uint8_t *dst_addr,
                 int next_header, int hop_limit)
{
    asn_value  *tmp_pdu;

    if (next_header > 0xff || hop_limit > 0xff)
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(tmpl_or_ptrn, is_pattern,
                                          ndn_ip6_header, "#ip6",
                                          &tmp_pdu));

    if (src_addr != NULL)
        CHECK_RC(asn_write_value_field(tmp_pdu, src_addr, 16,
                                       "src-addr.#plain"));

    if (dst_addr != NULL)
        CHECK_RC(asn_write_value_field(tmp_pdu, dst_addr, 16,
                                       "dst-addr.#plain"));
    if (next_header >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, next_header,
                                 "next-header.#plain"));

    if (hop_limit >= 0)
        CHECK_RC(asn_write_int32(tmp_pdu, hop_limit, "hop-limit.#plain"));

    if (pdu != NULL)
        *pdu = tmp_pdu;

    return 0;
}
