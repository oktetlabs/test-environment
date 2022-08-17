/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD CLI
 *
 * Traffic Application Domain Command Handler.
 * CLI CSAP layer-related callbacks.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAD CLI"

#include "te_config.h"

#include <stdio.h>

#include "tad_cli_impl.h"


extern void cli_container_get_prompt_params(const asn_value *cli_container,
                                            cli_csap_prompts_t *cli_prompts);

/* See description in tad_cli_impl.h */
te_errno
tad_cli_gen_bin_cb(csap_p csap, unsigned int layer,
                   const asn_value *tmpl_pdu, void *opaque,
                   const tad_tmpl_arg_t *args, size_t arg_num,
                   tad_pkts *sdus, tad_pkts *pdus)
{
    cli_csap_specific_data_p cli_spec_data = csap_get_rw_data(csap);

    te_errno    rc;
    int         msg_len;
    size_t      read_len;
    char       *msg;

    UNUSED(layer);
    UNUSED(opaque);
    UNUSED(args);
    UNUSED(arg_num);

    /* FIXME: Use read_string here */
    msg_len = asn_get_length(tmpl_pdu, "message");
    if (msg_len <= 0)
    {
        ERROR("Unexpected length of the 'message' %d", msg_len);
        return TE_RC(TE_TAD_CSAP, TE_EINVAL);
    }
    if ((msg = malloc(msg_len)) == NULL)
    {
        ERROR("%s(): Failed to allocate %d bytes of memory",
              __FUNCTION__, msg_len);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }
    read_len = msg_len;
    rc = asn_read_value_field(tmpl_pdu, msg, &read_len, "message");
    if (rc != 0)
    {
        ERROR("Failed to read 'message' from NDS: %r", rc);
        free(msg);
        return rc;
    }
    if (read_len != (size_t)msg_len)
    {
        ERROR("%s(): Unexpected number of bytes %u read using "
              "asn_read_value_field(), expected %d", __FUNCTION__,
              (unsigned)read_len, msg_len);
    }

    /*
     * Prepare prompt patterns for the ongoing command run
     * 1. Copy patters initialized on CSAP create;
     * 2. Overwrite patterns with values specified in PDU.
     *
     * Right before the command is passed to cli_expect_main
     * process we pass this structure content.
     */
    cli_spec_data->cur_prompts = cli_spec_data->init_prompts;
    cli_container_get_prompt_params(tmpl_pdu, &cli_spec_data->cur_prompts);

    tad_pkts_move(pdus, sdus);
    rc = tad_pkts_add_new_seg(pdus, TRUE,
                              msg, msg_len, tad_pkt_seg_data_free);
    if (rc != 0)
    {
        free(msg);
        return rc;
    }

    return 0;
}


/* See description in tad_cli_impl.h */
te_errno
tad_cli_match_bin_cb(csap_p           csap,
                     unsigned int     layer,
                     const asn_value *ptrn_pdu,
                     void            *ptrn_opaque,
                     tad_recv_pkt    *meta_pkt,
                     tad_pkt         *pdu,
                     tad_pkt         *sdu)
{
    te_errno    rc;
#if 1
    asn_value  *cli_msg;
    char       *msg;
    size_t      msg_len;
#endif

    UNUSED(ptrn_pdu);
    UNUSED(ptrn_opaque);

#if 1

    UNUSED(sdu);

    if (csap->state & CSAP_STATE_RESULTS)
    {
        if ((cli_msg = meta_pkt->layers[layer].nds =
                 asn_init_value(ndn_cli_message)) == NULL)
        {
            ERROR_ASN_INIT_VALUE(ndn_cli_message);
            return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
        }

        assert(tad_pkt_seg_num(pdu) == 1);
        assert(tad_pkt_first_seg(pdu) != NULL);
        msg = tad_pkt_first_seg(pdu)->data_ptr;
        msg_len = tad_pkt_first_seg(pdu)->data_len;

        VERB("cli_match. len: %d, message: %s\n", msg_len, msg);

        rc = asn_write_value_field(cli_msg, msg, msg_len,
                                   "message.#plain");
        if (rc != 0)
        {
            ERROR(CSAP_LOG_FMT "%s(): asn_write_value_field() failed: "
                  "%r", CSAP_LOG_ARGS(csap), __FUNCTION__, rc);
        }
    }
    else
    {
        rc = 0;
    }

#else

    UNUSED(layer);
    UNUSED(meta_pkt);

    rc = tad_pkt_get_frag(sdu, pdu, 0, tad_pkt_len(pdu),
                          TAD_PKT_GET_FRAG_ERROR);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to prepare CLI SDU: %r",
              CSAP_LOG_ARGS(csap), rc);
    }

#endif

    return rc;
}

/* See description in tad_cli_impl.h */
te_errno
tad_cli_gen_pattern_cb(csap_p            csap,
                       unsigned int      layer,
                       const asn_value  *tmpl_pdu,
                       asn_value       **ptrn_pdu)
{
    UNUSED(csap);
    UNUSED(layer);
    UNUSED(tmpl_pdu);

    assert(ptrn_pdu != NULL);
    if ((*ptrn_pdu = asn_init_value(ndn_cli_message)) == NULL)
    {
        ERROR_ASN_INIT_VALUE(ndn_cli_message);
        return TE_RC(TE_TAD_CSAP, TE_ENOMEM);
    }

    return 0;
}
