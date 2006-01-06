/** @file
 * @brief TAD Socket
 *
 * Traffic Application Domain Command Handler.
 * Socket CSAP layer-related callbacks.
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id: tad_socket_layer.c 22403 2006-01-05 15:59:36Z arybchik $
 */

#define TE_LGR_USER     "TAD Socket"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "te_defs.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "tad_socket_impl.h"


/* See description in tad_socket_impl.h */
te_errno
tad_socket_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                           asn_value *layer_pdu, void **p_opaque)
{
#if 0
    if (spec_data->data_tag == NDN_TAG_TCP_DATA_SERVER)
    {
        ERROR(CSAP_LOG_FMT "write to TCP 'server' socket is not allowed",
              CSAP_LOG_ARGS(csap));
        return TE_RC(TE_TAD_CSAP, TE_ETADLOWER);
    }
#endif
    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}


/* See description in tad_socket_impl.h */
te_errno
tad_socket_gen_bin_cb(csap_p csap, unsigned int layer,
                      const asn_value *tmpl_pdu, void *opaque,
                      const tad_tmpl_arg_t *args, size_t arg_num, 
                      tad_pkts *sdus, tad_pkts *pdus)
{
    UNUSED(csap);
    UNUSED(layer);
    UNUSED(tmpl_pdu);
    UNUSED(opaque);
    UNUSED(args);
    UNUSED(arg_num);

    tad_pkts_move(pdus, sdus);

    return 0;
}


/* See description in tad_socket_impl.h */
te_errno
tad_socket_confirm_ptrn_cb(csap_p csap, unsigned int layer,
                           asn_value *layer_pdu, void **p_opaque)
{
#if 0
    if (spec_data->data_tag != NDN_TAG_TCP_DATA_SERVER)
    {
        uint32_t len;
        rc = asn_read_int32(tcp_pdu, &len, "length");

        INFO("TCP data CSAP confirm, length read rc %r, value %d",
             rc, len);

        if (rc == 0)
            spec_data->wait_length = len;
    }
    return 0;
#endif
    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}


/* See description in tad_socket_impl.h */
te_errno
tad_socket_match_bin_cb(csap_p csap, unsigned int layer,
                        const asn_value *pattern_pdu,
                        const csap_pkts *pkt, csap_pkts *payload, 
                        asn_value_p parsed_packet)
{
#if 0
    if (csap->type == TAD_CSAP_DATA) 
    {
        INFO("%s(CSAP %d) data tag %d", __FUNCTION__, csap->id, 
             spec_data->data_tag);
        if (spec_data->data_tag == NDN_TAG_TCP_DATA_SERVER)
        {
            int acc_sock = *((int *)data);

            INFO("match data server CSAP, socket %d", acc_sock);

            if (parsed_packet != NULL)
                rc = asn_write_int32(tcp_header_pdu, acc_sock, "socket");
            if (rc != 0)
                ERROR("write socket error: %r", rc);
            pld_len = 0;
        }
        else
        {
            if (spec_data->wait_length > 0)
            {
                int defect = spec_data->wait_length - 
                            (spec_data->stored_length + pkt->len);

                if (spec_data->stored_buffer == NULL)
                    spec_data->stored_buffer =
                        malloc(spec_data->wait_length);

                if (defect > 0) 
                {
                    rc = TE_ETADLESSDATA;
                    INFO("%s(CSAP %d): less data, "
                         "wait %d, stored %d, get %d",
                         __FUNCTION__, csap->id, 
                          spec_data->wait_length,
                          spec_data->stored_length,
                          pkt->len);

                    memcpy(spec_data->stored_buffer + 
                           spec_data->stored_length, 
                           pkt->data, pkt->len); 
                    spec_data->stored_length += pkt->len; 
                }
                else if (defect == 0)
                {
                    INFO("%s(CSAP %d): got last data, "
                         "wait %d, stored %d, get %d",
                         __FUNCTION__, csap->id, 
                          spec_data->wait_length,
                          spec_data->stored_length,
                          pkt->len);
                    memcpy(spec_data->stored_buffer + 
                               spec_data->stored_length, 
                           pkt->data, pkt->len); 
                    pld_data = spec_data->stored_buffer;
                    pld_len = spec_data->wait_length;

                    goto put_payload;
                }
                else 
                {
                    ERROR("read data more then asked: "
                          "want %d, stored %d, last get %d", 
                          spec_data->wait_length,
                          spec_data->stored_length,
                          pkt->len);
                    rc = TE_ESMALLBUF;
                }

                goto cleanup;
            }
        }
        goto put_payload;
    }
#endif
    return TE_RC(TE_TAD_CSAP, TE_EOPNOTSUPP);
}
