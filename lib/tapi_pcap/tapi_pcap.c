/** @file
 * @brief Proteos, Tester API for Ethernet-PCAP CSAP.
 *
 * Implementation of Tester API for Ethernet-PCAP CSAP.
 *
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
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
#include "tapi_pcap.h"


/**
 * Create common Ethernet-PCAP CSAP.
 *
 * @param ta_name       Test Agent name
 * @param sid           RCF session
 * @param ifname        Interface name on TA host
 * @param iftype        Interface datalink type (see man pcap)
 * @param recv_mode     Receive mode, bit scale defined by elements of
 *                      'enum pcap_csap_receive_mode' in ndn_pcap.h.
 * @param pcap_csap     Identifier of created CSAP (OUT)
 *
 * @return Zero on success, otherwise standard or common TE error code.
 */
int
tapi_pcap_csap_create(const char *ta_name, int sid,
                      const char *ifname, int iftype, int recv_mode,
                      csap_handle_t *pcap_csap)
{
    int     rc;
    char    tmp_name[] = "/tmp/te_pcap_csap_create.XXXXXX";
    FILE   *f;

    if ((ta_name == NULL) || (ifname == NULL) || (pcap_csap == NULL))
        return TE_RC(TE_TAPI, TE_EINVAL);

    if ((rc = te_make_tmp_file(tmp_name)) != 0)
        return TE_RC(TE_TAPI, rc);

    VERB("%s() file: %s", __FUNCTION__, tmp_name);

    if ((f = fopen(tmp_name, "w+")) == NULL)
    {
        ERROR("fopen() of %s failed(%d)", tmp_name, errno);
        return TE_OS_RC(TE_TAPI, errno); /* return system errno */
    }

    fprintf(f, "{ pcap:{ ifname plain:\"%s\", "
               "iftype %d, receive-mode %d } }",
            ifname, iftype, recv_mode);
    fclose(f);

    rc = rcf_ta_csap_create(ta_name, sid, "pcap", tmp_name, pcap_csap);
    if (rc != 0)
    {
        ERROR("rcf_ta_csap_create() failed(%r) on TA %s:%d file %s",
              rc, ta_name, sid, tmp_name);
    }

    unlink(tmp_name);

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

/* See description in tapi_eth.h */
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
