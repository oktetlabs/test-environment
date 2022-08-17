/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD Socket
 *
 * Traffic Application Domain Command Handler.
 * Socket CSAP layer-related callbacks.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAD Socket"

#include "te_config.h"

#include "te_defs.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "ndn_socket.h"

#include "tad_utils.h"

#include "tad_socket_impl.h"


/* See description in tad_socket_impl.h */
te_errno
tad_socket_confirm_tmpl_cb(csap_p csap, unsigned int layer,
                           asn_value *layer_pdu, void **p_opaque)
{
    tad_socket_rw_data *spec_data = csap_get_rw_data(csap);

    UNUSED(layer);
    UNUSED(layer_pdu);
    UNUSED(p_opaque);

    if (spec_data->data_tag == NDN_TAG_SOCKET_TYPE_TCP_SERVER)
    {
        ERROR(CSAP_LOG_FMT "write to TCP 'server' socket is not allowed",
              CSAP_LOG_ARGS(csap));
        return TE_RC(TE_TAD_CSAP, TE_ETADLOWER);
    }

    return 0;
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
tad_socket_match_bin_cb(csap_p           csap,
                        unsigned int     layer,
                        const asn_value *ptrn_pdu,
                        void            *ptrn_opaque,
                        tad_recv_pkt    *meta_pkt,
                        tad_pkt         *pdu,
                        tad_pkt         *sdu)
{
    tad_socket_rw_data *spec_data = csap_get_rw_data(csap);
    te_errno            rc = 0;
    asn_value          *nds = NULL;

    UNUSED(ptrn_pdu);
    UNUSED(ptrn_opaque);

    assert(csap_get_rw_layer(csap) == layer);

    ENTRY(CSAP_LOG_FMT "type is %d", CSAP_LOG_ARGS(csap),
          spec_data->data_tag);

    if ((csap->state & CSAP_STATE_RESULTS) &&
        ((nds = meta_pkt->layers[layer].nds) == NULL) &&
        ((nds = meta_pkt->layers[layer].nds =
              asn_init_value(ndn_socket_message)) == NULL))
    {
        ERROR_ASN_INIT_VALUE(ndn_socket_message);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    if (spec_data->data_tag == NDN_TAG_SOCKET_TYPE_TCP_SERVER)
    {
        if (csap->state & CSAP_STATE_RESULTS)
        {
            tad_pkt_seg    *seg = tad_pkt_first_seg(pdu);
            int             acc_sock;

            if (seg == NULL || seg->data_len != sizeof(int))
            {
                ERROR(CSAP_LOG_FMT "Invalid PDU for TCP socket server",
                      CSAP_LOG_ARGS(csap));
                rc = TE_RC(TE_TAD_CSAP, TE_EFAULT);
                goto cleanup;
            }
            assert(seg->data_ptr != NULL);

            memcpy(&acc_sock, seg->data_ptr, sizeof(int));
            INFO("Match data server CSAP, socket %d", acc_sock);

            rc = asn_write_int32(nds, acc_sock, "file-descr");
            if (rc != 0)
            {
                ERROR(CSAP_LOG_FMT "Failed to write 'file-descr' to "
                      "NDS: %r", CSAP_LOG_ARGS(csap), rc);
                goto cleanup;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
#if 0
        uint8_t    *pld_data = pkt->data;
        size_t      pld_len = pkt->len;

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
                goto cleanup;
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
            }
            else
            {
                ERROR("read data more then asked: "
                      "want %d, stored %d, last get %d",
                      spec_data->wait_length,
                      spec_data->stored_length,
                      pkt->len);
                rc = TE_ESMALLBUF;
                goto cleanup;
            }
        }
        /* passing payload to upper layer */
        memset(payload, 0 , sizeof(*payload));

        if ((payload->len = pld_len) > 0)
        {
            payload->data = malloc(pld_len);
            memcpy(payload->data, pld_data, payload->len);
        }
        if (spec_data->wait_length > 0)
        {
            free(spec_data->stored_buffer);
            spec_data->stored_buffer = NULL;
            spec_data->stored_length = 0;
            spec_data->wait_length = 0;
        }
#else
        rc = tad_pkt_get_frag(sdu, pdu, 0, tad_pkt_len(pdu),
                              TAD_PKT_GET_FRAG_ERROR);
        if (rc != 0)
        {
            ERROR(CSAP_LOG_FMT "Failed to prepare Socket SDU: %r",
                  CSAP_LOG_ARGS(csap), rc);
            return rc;
        }
#endif
    }


cleanup:

    return rc;
}
