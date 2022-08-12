/** @file
 * @brief TAPI TAD Ethernet
 *
 * Implementation of test API for Ethernet TAD.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
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


struct tapi_eth_packet_storage {
    asn_value     **packets_captured;
    unsigned int    n_packets_captured;
    te_errno        err;
};

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
    UNUSED(tagged);
    UNUSED(llc);

    CHECK_RC(tapi_tad_csap_add_layer(csap_spec, ndn_eth_csap, "#eth", NULL));

    CHECK_RC(tapi_eth_set_csap_layer(*csap_spec, device, recv_mode, remote_addr,
                                     local_addr, len_type));

    return 0;
}

/* See the description in tapi_eth.h */
te_errno
tapi_eth_set_csap_layer(asn_value       *csap_spec,
                        const char      *device,
                        unsigned int     recv_mode,
                        const uint8_t   *remote_addr,
                        const uint8_t   *local_addr,
                        const uint16_t  *len_type)
{
    asn_value        *layers;
    asn_child_desc_t *layers_eth = NULL;
    unsigned int      nb_layers_eth;
    asn_value        *layer_eth_outer;

    CHECK_RC(asn_get_subvalue(csap_spec, &layers, "layers"));

    /*
     * CSAP specification may have more than one layer of one type,
     * namely, if one sends or receives Ethernet frames belonging
     * to a virtual network by means of encapsulation into real network
     * packets (and thus into "outer" Ethernet frames), then at least two
     * layers with TE_PROTO_ETH tag will be present in CSAP specification;
     * the read-write Ethernet layer (which needs to be configured here)
     * is always the "outer" (and the last) one, therefore, all Ethernet
     * layers are detected here and only the last one is configured
     */
    CHECK_RC(asn_find_child_choice_values(layers, TE_PROTO_ETH,
                                          &layers_eth, &nb_layers_eth));
    CHECK_NOT_NULL(layers_eth);
    layer_eth_outer = layers_eth[nb_layers_eth - 1].value;

    if (device != NULL)
        CHECK_RC(asn_write_string(layer_eth_outer,
                                  device, "device-id.#plain"));

    CHECK_RC(asn_write_int32(layer_eth_outer, recv_mode, "receive-mode"));

    if (remote_addr != NULL)
        CHECK_RC(asn_write_value_field(layer_eth_outer,
                                       remote_addr, ETHER_ADDR_LEN,
                                       "remote-addr.#plain"));
    if (local_addr != NULL)
        CHECK_RC(asn_write_value_field(layer_eth_outer,
                                       local_addr, ETHER_ADDR_LEN,
                                       "local-addr.#plain"));

    if (len_type != NULL)
        CHECK_RC(asn_write_int32(layer_eth_outer, (int32_t)*len_type,
                                 "ether-type.#plain"));

    free(layers_eth);

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

/* See the description in tapi_eth.h */
te_errno
tapi_eth_based_csap_create_by_tmpl(const char      *ta_name,
                                   int              sid,
                                   const char      *device,
                                   unsigned int     recv_mode,
                                   const asn_value *tmpl,
                                   csap_handle_t   *eth_csap)
{
    asn_value *csap_spec;
    te_errno   rc;

    csap_spec = ndn_csap_spec_by_traffic_template(tmpl);
    if (csap_spec == NULL)
    {
        rc = TE_RC(TE_TAPI, TE_EINVAL);
        goto cleanup;
    }

    rc = tapi_eth_set_csap_layer(csap_spec, device, recv_mode,
                                 NULL, NULL, NULL);
    if (rc != 0)
        goto cleanup;

    rc = tapi_tad_csap_create(ta_name, sid, NULL, (const asn_value *)csap_spec,
                              eth_csap);

cleanup:
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

static void
tapi_eth_store_packet_cb(asn_value *packet, void *user_data)
{
    struct tapi_eth_packet_storage *csap_sniff_storage = user_data;
    asn_value                     **packets_captured;
    unsigned int                    n_packets_captured;

    if (csap_sniff_storage->err != 0)
        return;

    n_packets_captured = csap_sniff_storage->n_packets_captured;

    packets_captured = realloc(csap_sniff_storage->packets_captured,
                               (n_packets_captured + 1) *
                               sizeof(*packets_captured));
    if (packets_captured == NULL)
    {
        csap_sniff_storage->err = TE_ENOMEM;
        return;
    }

    csap_sniff_storage->packets_captured = packets_captured;
    packets_captured[n_packets_captured] = packet;
    csap_sniff_storage->n_packets_captured++;
}

/* See the description in 'tapi_eth.h' */
te_errno
tapi_eth_gen_traffic_sniff_pattern(const char     *ta_name,
                                   int             sid,
                                   const char     *if_name,
                                   asn_value      *template,
                                   send_transform *transform,
                                   asn_value     **pattern_out)
{
    te_errno                        err = 0;
    csap_handle_t                   csap_xmit = CSAP_INVALID_HANDLE;
    csap_handle_t                   csap_sniff = CSAP_INVALID_HANDLE;
    asn_value                      *pattern_by_template = NULL;
    tapi_tad_trrecv_cb_data         csap_packet_handler_data;
    struct tapi_eth_packet_storage  csap_packets_storage;
    asn_value                      *pattern;

    memset(&csap_packets_storage, 0, sizeof(csap_packets_storage));

    err = tapi_eth_based_csap_create_by_tmpl(ta_name, sid, if_name,
                                             TAD_ETH_RECV_NO, template,
                                             &csap_xmit);
    if (err != 0)
        goto out;

    if (pattern_out != NULL)
    {
        err = tapi_eth_based_csap_create_by_tmpl(ta_name, sid, if_name,
                                                 TAD_ETH_RECV_OUT, template,
                                                 &csap_sniff);
        if (err != 0)
            goto out;

        pattern_by_template = tapi_tad_mk_pattern_from_template(template);
        if (pattern_by_template == NULL)
        {
            err = TE_ENOMEM;
            goto out;
        }

        err = tapi_tad_trrecv_start(ta_name, sid, csap_sniff,
                                    pattern_by_template,
                                    TAD_TIMEOUT_INF, 0, RCF_TRRECV_PACKETS);
        if (err != 0)
            goto out;
    }

    err = tapi_tad_trsend_start(ta_name, sid, csap_xmit, template,
                                RCF_MODE_BLOCKING);
    if (err != 0)
    {
        if (pattern_out != NULL)
            (void)tapi_tad_trrecv_stop(ta_name, sid, csap_sniff, NULL, NULL);

        goto out;
    }

    if (pattern_out == NULL)
        goto out;

    csap_packet_handler_data.callback = tapi_eth_store_packet_cb;
    csap_packet_handler_data.user_data = &csap_packets_storage;

    err = tapi_tad_trrecv_stop(ta_name, sid, csap_sniff,
                               &csap_packet_handler_data, NULL);
    if (err != 0)
        goto out;

    err = csap_packets_storage.err;
    if (err != 0)
        goto out;

    if (csap_packets_storage.n_packets_captured == 0)
    {
        err = TE_EFAIL;
        goto out;
    }

    err = tapi_tad_packets_to_pattern(csap_packets_storage.packets_captured,
                                      csap_packets_storage.n_packets_captured,
                                      transform, &pattern);
    if (err != 0)
        goto out;

    *pattern_out = pattern;

out:
    if (csap_packets_storage.packets_captured != NULL)
    {
        unsigned int i;

        for (i = 0; i < csap_packets_storage.n_packets_captured; ++i)
        {
            if (csap_packets_storage.packets_captured[i] != NULL)
                asn_free_value(csap_packets_storage.packets_captured[i]);
        }

        free(csap_packets_storage.packets_captured);
    }

    if (pattern_by_template != NULL)
        asn_free_value(pattern_by_template);

    if (csap_sniff != CSAP_INVALID_HANDLE)
        (void)tapi_tad_csap_destroy(ta_name, sid, csap_sniff);

    if (csap_xmit != CSAP_INVALID_HANDLE)
        (void)tapi_tad_csap_destroy(ta_name, sid, csap_xmit);

    return TE_RC(TE_TAPI, err);
}
