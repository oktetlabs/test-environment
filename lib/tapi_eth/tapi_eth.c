/** @file
 * @brief TAPI TAD Ethernet
 *
 * Implementation of test API for Ethernet TAD.
 *
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI Ethernet"

#include "te_config.h"

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

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "logger_api.h"
#include "asn_usr.h"
#include "ndn_eth.h"
#include "tapi_tad.h"
#include "tapi_ndn.h"
#include "tapi_eth.h"

#include "tapi_test.h"


/* See the description in tapi_eth.h */
te_errno
tapi_eth_add_csap_layer(asn_value      **csap_spec,
                        const char      *device,
                        unsigned int     recv_mode,
                        const uint8_t   *remote_addr,
                        const uint8_t   *local_addr,
                        const uint16_t  *len_type,
                        te_bool3         tagged,
                        te_bool3         llc)
{
    asn_value  *layer;
    
    UNUSED(tagged);
    UNUSED(llc);
#if 0
    if (!is_tagged &&
        (priority != NULL || cfi != NULL || vlan_id != NULL))
    {
        ERROR("%s(): Priority/CFI/VLAN-ID cannot be specified for "
              "untagged frame", __FUNCTION__);
        return TE_RC(TE_TAPI, TE_EINVAL);
    }
    if (is_tagged)
    {
        /* FIXME: Add frame-type.#tagged container */
        if (priority != NULL)
            CHECK_RC(asn_write_int32(layer, *priority, "priority.#plain"));
        if (cfi != NULL)
            CHECK_RC(asn_write_int32(layer, *cfi, "cfi.#plain"));
        if (vlan_id != NULL)
            CHECK_RC(asn_write_int32(layer, *vlan_id, "vlan-id.#plain"));
    }
#endif

    CHECK_RC(tapi_tad_csap_add_layer(csap_spec, ndn_eth_csap, "#eth",
                                     &layer));

    if (device != NULL)
        CHECK_RC(asn_write_string(layer, device, "device-id.#plain"));
    CHECK_RC(asn_write_int32(layer, recv_mode, "receive-mode"));

    if (remote_addr != NULL)
        CHECK_RC(asn_write_value_field(layer, remote_addr, ETHER_ADDR_LEN,
                                       "remote-addr.#plain"));
    if (local_addr != NULL)
        CHECK_RC(asn_write_value_field(layer, local_addr, ETHER_ADDR_LEN,
                                       "local-addr.#plain"));
    if (len_type != NULL)
        CHECK_RC(asn_write_int32(layer, *len_type, "ether-type.#plain"));

    return 0;
}


/* See the description in tapi_eth.h */
te_errno
tapi_eth_add_pdu(asn_value      **tmpl_or_ptrn,
                 asn_value      **pdu,
                 te_bool          is_pattern,
                 const uint8_t   *dst_addr,
                 const uint8_t   *src_addr,
                 const uint16_t  *ether_type,
                 te_bool3         tagged,
                 te_bool3         llc)
{
    asn_value  *tmp_pdu;

    CHECK_RC(tapi_tad_tmpl_ptrn_add_layer(tmpl_or_ptrn, is_pattern,
                                          ndn_eth_header, "#eth",
                                          &tmp_pdu));

    if (dst_addr != NULL)
        CHECK_RC(asn_write_value_field(tmp_pdu, dst_addr, ETHER_ADDR_LEN,
                                       "dst-addr.#plain"));
    if (src_addr != NULL)
        CHECK_RC(asn_write_value_field(tmp_pdu, src_addr, ETHER_ADDR_LEN,
                                       "src-addr.#plain"));
    if (ether_type != NULL)
        CHECK_RC(asn_write_int32(tmp_pdu, *ether_type,
                                 "ether-type.#plain"));

    if (tagged == TE_BOOL3_ANY)
    {
        /* Nothing to do */
    }
    else if (tagged == TE_BOOL3_FALSE)
    {
        CHECK_NOT_NULL(asn_retrieve_descendant(tmp_pdu, NULL,
                                               "tagged.#untagged"));
    }
    else if (tagged == TE_BOOL3_TRUE)
    {
        CHECK_NOT_NULL(asn_retrieve_descendant(tmp_pdu, NULL,
                                               "tagged.#tagged"));
    }
    else
    {
        assert(FALSE);
    }

    if (llc == TE_BOOL3_ANY)
    {
        /* Nothing to do */
    }
    else if (llc == TE_BOOL3_FALSE)
    {
        CHECK_NOT_NULL(asn_retrieve_descendant(tmp_pdu, NULL,
                                               "encap.#ethernet2"));
    }
    else if (llc == TE_BOOL3_TRUE)
    {
        CHECK_NOT_NULL(asn_retrieve_descendant(tmp_pdu, NULL,
                                               "encap.#llc"));
    }
    else
    {
        assert(FALSE);
    }

    if (pdu != NULL)
        *pdu = tmp_pdu;

    return 0;
}

/* See the description in tapi_eth.h */
te_errno
tapi_eth_pdu_length_type(asn_value      *pdu,
                         const uint16_t  len_type)
{
    return asn_write_int32(pdu, len_type, "length-type.#plain");
}

/* See the description in tapi_eth.h */
te_errno
tapi_eth_pdu_tag_header(asn_value      *pdu,
                        const uint8_t  *priority,
                        const uint16_t *vlan_id)
{
    if (priority != NULL)
        CHECK_RC(asn_write_int32(pdu, *priority,
                                 "tagged.#tagged.priority.#plain"));
    if (vlan_id != NULL)
        CHECK_RC(asn_write_int32(pdu, *vlan_id,
                                 "tagged.#tagged.vlan-id.#plain"));

    return 0;
}

/* See the description in tapi_eth.h */
te_errno
tapi_eth_pdu_llc_snap(asn_value *pdu)
{
    CHECK_RC(asn_write_int32(pdu, 0,    "encap.#llc.i-g.#plain"));
    CHECK_RC(asn_write_int32(pdu, 0x55, "encap.#llc.dsap.#plain"));
    CHECK_RC(asn_write_int32(pdu, 0,    "encap.#llc.c-r.#plain"));
    CHECK_RC(asn_write_int32(pdu, 0x55, "encap.#llc.ssap.#plain"));
    CHECK_RC(asn_write_int32(pdu, 0x03, "encap.#llc.ctl.#plain"));

    CHECK_RC(asn_write_int32(pdu, 0, "encap.#llc.snap.oui.#plain"));

    return 0;
}


/* See the description in tapi_eth.h */
te_errno
tapi_eth_csap_create(const char *ta_name, int sid,
                     const char *device,
                     unsigned int receive_mode,
                     const uint8_t *remote_addr,
                     const uint8_t *local_addr,
                     const uint16_t *len_type,
                     csap_handle_t *eth_csap)
{
    asn_value  *csap_spec = NULL;
    te_errno    rc;

    if ((ta_name == NULL) || (device == NULL) || (eth_csap == NULL))
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(tapi_eth_add_csap_layer(&csap_spec, device, receive_mode,
                                     remote_addr, local_addr,
                                     len_type,
                                     TE_BOOL3_ANY /* tagged/untagged */,
                                     TE_BOOL3_ANY /* Ethernet2/LLC */));

    rc = tapi_tad_csap_create(ta_name, sid, "eth", csap_spec, eth_csap);

    asn_free_value(csap_spec);

    return rc;
}


/**
 * Structure to be passed as @a user_param to rcf_ta_trrecv_wait(),
 * rcf_ta_trrecv_stop() and rcf_ta_trrecv_get(), if
 * tapi_eth_pkt_handler() function as @a handler.
 */
typedef struct tapi_eth_pkt_handler_data {
    tapi_eth_frame_callback  callback;  /**< User callback function */
    void                    *user_data; /**< Real user data */
} tapi_eth_pkt_handler_data;

/**
 * This function complies with tapi_tad_trrecv_cb prototype.
 * @a user_param must point to tapi_eth_pkt_handler_data structure.
 */
static void
tapi_eth_pkt_handler(asn_value *packet, void *user_param)
{
    struct tapi_eth_pkt_handler_data *i_data =
        (struct tapi_eth_pkt_handler_data *)user_param;

    ndn_eth_header_plain header;
    te_errno rc;
    const asn_value *eth_hdr_val;

    size_t p_len;
    uint8_t *payload;

    if (user_param == NULL)
    {
        ERROR("TAPI ETH, pkt handler: bad user param!!!");
        return;
    }

    rc = asn_get_indexed(packet, (asn_value **)&eth_hdr_val, -1, "pdus");
    if (rc != 0)
    {
        ERROR("%s(): cannot get the last PDU from packet: %r",
              __FUNCTION__, rc);
        return;
    }

    memset(&header, 0, sizeof(header));
    rc = ndn_eth_packet_to_plain(eth_hdr_val, &header);
    if (rc != 0)
    {
        ERROR("%s(): packet to plain conversion error: %r",
              __FUNCTION__, rc);
        return;
    }

    p_len = asn_get_length(packet, "payload.#bytes");

    payload = malloc(p_len + 1);
    rc = asn_read_value_field(packet, payload, &p_len, "payload.#bytes");

    if (rc)
    {
        ERROR( "tapi eth int cb, read payload error %x\n", rc);
        return;
    }

    i_data->callback(packet, -1, &header, payload, p_len,
                     i_data->user_data);

    free(payload);
    asn_free_value(packet);

    return;
}

/* See the description in tapi_eth.h */
tapi_tad_trrecv_cb_data *
tapi_eth_trrecv_cb_data(tapi_eth_frame_callback  callback,
                        void                    *user_data)
{
    tapi_eth_pkt_handler_data  *cb_data;
    tapi_tad_trrecv_cb_data    *res;

    cb_data = (tapi_eth_pkt_handler_data *)calloc(1, sizeof(*cb_data));
    if (cb_data == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return NULL;
    }
    cb_data->callback = callback;
    cb_data->user_data = user_data;
    
    res = tapi_tad_trrecv_make_cb_data(tapi_eth_pkt_handler, cb_data);
    if (res == NULL)
        free(cb_data);

    return res;
}
