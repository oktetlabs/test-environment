/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Proteos, Tester API for Ethernet-PCAP CSAP.
 *
 * Implementation of Tester API for Ethernet-PCAP CSAP.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAPI PCAP"

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
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "te_errno.h"
#include "rcf_api.h"
#include "ndn_pcap.h"
#include "logger_api.h"
#include "tapi_ndn.h"
#include "tapi_tad.h"
#include "tapi_pcap.h"

#include "tapi_test.h"


/* See the description in tapi_pcap.h */
te_errno
tapi_pcap_add_csap_layer(asn_value     **csap_spec,
                         const char     *ifname,
                         unsigned int    iftype,
                         unsigned int    recv_mode)
{
    asn_value  *layer;

    CHECK_RC(tapi_tad_csap_add_layer(csap_spec, ndn_pcap_csap, "#pcap",
                                     &layer));

    if (ifname != NULL)
        CHECK_RC(asn_write_string(layer, ifname, "ifname.#plain"));

    CHECK_RC(asn_write_int32(layer, iftype, "iftype"));
    CHECK_RC(asn_write_int32(layer, recv_mode, "receive-mode"));

    return 0;
}

/* See the description in tapi_pcap.h */
te_errno
tapi_pcap_csap_create(const char *ta_name, int sid,
                      const char *ifname, unsigned int iftype,
                      unsigned int recv_mode,
                      csap_handle_t *pcap_csap)
{
    asn_value  *csap_spec = NULL;
    te_errno    rc;

    if ((ta_name == NULL) || (ifname == NULL) || (pcap_csap == NULL))
        return TE_RC(TE_TAPI, TE_EINVAL);

    CHECK_RC(tapi_pcap_add_csap_layer(&csap_spec, ifname, iftype,
                                      recv_mode));

    rc = tapi_tad_csap_create(ta_name, sid, "pcap", csap_spec, pcap_csap);

    asn_free_value(csap_spec);

    return rc;
}


/**
 * Structure to be passed as @a user_param to rcf_ta_trrecv_wait(),
 * rcf_ta_trrecv_stop() and rcf_ta_trrecv_get(), if
 * tapi_pcap_pkt_handler() function as @a handler.
 */
typedef struct tapi_pcap_pkt_handler_data {
    tapi_pcap_recv_callback  callback;  /**< User callback function */
    void                    *user_data; /**< Real user data */
} tapi_pcap_pkt_handler_data;

/**
 * This function complies with rcf_pkt_handler prototype.
 * @a user_param must point to tapi_pcap_pkt_handler_data structure.
 */
static void
tapi_pcap_pkt_handler(asn_value *frame_val, void *user_param)
{
    struct tapi_pcap_pkt_handler_data *i_data =
        (struct tapi_pcap_pkt_handler_data *)user_param;

    te_errno   rc;

    asn_value *pcap_filtered_pdu;

    uint8_t   *pkt;
    size_t     pkt_len;

    int        filter_id;
    size_t     tmp_len;

    VERB("%s() started", __FUNCTION__);

    if (user_param == NULL)
    {
        fprintf(stderr, "TAPI ETH, pkt handler: bad user param!!!\n");
        return;
    }

    pcap_filtered_pdu = asn_read_indexed(frame_val, 0, "pdus");
    if (pcap_filtered_pdu == NULL)
    {
        ERROR("%s(): read_indexed error", __FUNCTION__);
        return;
    }

    tmp_len = sizeof(int);
    rc = asn_read_value_field(pcap_filtered_pdu, &filter_id,
                              &tmp_len, "filter-id");
    if (rc < 0)
    {
        filter_id = -1;
    }

    rc = asn_get_length(frame_val, "payload.#bytes");
    if (rc < 0)
    {
        ERROR("%s(): get_len error", __FUNCTION__);
        return;
    }
    pkt_len = rc;

    VERB("%s(): Packet payload length %u bytes",
         __FUNCTION__, (unsigned)pkt_len);

    pkt = malloc(pkt_len);
    if (pkt == NULL)
    {
        ERROR("There is no enough memory to allocate for packet data");
        return;
    }

    rc = asn_read_value_field(frame_val, pkt, &pkt_len, "payload.#bytes");
    if (rc < 0)
    {
        ERROR("%s(): read payload error %r", __FUNCTION__, rc);
        return;
    }

    i_data->callback(filter_id, pkt, pkt_len, i_data->user_data);

    free(pkt);

    asn_free_value(frame_val);
    asn_free_value(pcap_filtered_pdu);
}

/* See description in tapi_pcap.h */
tapi_tad_trrecv_cb_data *
tapi_pcap_trrecv_cb_data(tapi_pcap_recv_callback  callback,
                         void                    *user_data)
{
    tapi_pcap_pkt_handler_data *cb_data;
    tapi_tad_trrecv_cb_data    *res;

    cb_data = (tapi_pcap_pkt_handler_data *)calloc(1, sizeof(*cb_data));
    if (cb_data == NULL)
    {
        ERROR("%s(): failed to allocate memory", __FUNCTION__);
        return NULL;
    }
    cb_data->callback = callback;
    cb_data->user_data = user_data;

    res = tapi_tad_trrecv_make_cb_data(tapi_pcap_pkt_handler, cb_data);
    if (res == NULL)
        free(cb_data);

    return res;
}


/* See description in tapi_pcap.h */
int
tapi_pcap_pattern_add(const char *filter,
                      const int   filter_id,
                      asn_value **pattern)
{
    asn_value            *pcap_pdu = NULL;
    asn_value            *pcap_pattern = NULL;
    int                   tmp_len;
    int                   syms;
    int                   rc;

    RING("%s(\"%s\", %d) started", __FUNCTION__, filter, filter_id);

    if (pattern == NULL)
    {
        ERROR("not NULL pattern pointer required");
        return TE_EINVAL;
    }

    VERB("Call asn_init_value()");
    pcap_pdu = asn_init_value(ndn_pcap_filter);

    tmp_len = strlen(filter);
    VERB("Call asn_write_value_field(\"filter.#plain\")");
    if ((rc = asn_write_value_field(pcap_pdu, filter, tmp_len,
                                    "filter.#plain")) != 0)
    {
        ERROR("Cannot write ASN value \"filter.#plain\"");
        return rc;
    }

    VERB("Call asn_write_int32(\"filter-id\")");
    if ((rc = asn_write_int32(pcap_pdu, filter_id, "filter-id")) != 0)
    {
        ERROR("Cannot write ASN value \"filter-id\"");
        return rc;
    }


    VERB("Call asn_parse_value_text()");
    if ((rc = asn_parse_value_text("{ pdus { pcap: {}}}",
                                   ndn_traffic_pattern_unit,
                                   &pcap_pattern, &syms)) != 0)
    {
        ERROR("Cannot initialise PCAP PDU value");
        return rc;
    }

    VERB("Call asn_write_component_value()");
    if ((rc = asn_write_component_value(pcap_pattern, pcap_pdu,
                                        "pdus.0.#pcap")) != 0)
    {
        ERROR("Cannot initialise PCAP pattern value");
        return rc;
    }


    if (*pattern == NULL)
    {
        VERB("Call asn_init_value()");
        *pattern = asn_init_value(ndn_traffic_pattern);
        if (*pattern == NULL)
        {
            ERROR("Init traffic pattern value failed");
            return rc;
        }
    }

    VERB("Call asn_insert_indexed()");
    if ((rc = asn_insert_indexed(*pattern, pcap_pattern, -1, "")) != 0)
    {
        ERROR("Cannot insert PCAP pattern to traffic pattern");
        return rc;
    }

    return 0;
}
