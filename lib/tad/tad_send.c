/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD Sender
 *
 * Traffic Application Domain Command Handler.
 * Transmit module.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAD Send"

#include "te_config.h"

#include <stdio.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_tools.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "rcf_ch_api.h"
#include "ndn.h"

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"
#include "tad_send.h"


/* buffer for send answer */
#define RBUF 100


/**
 * Preprocess traffic template sequence of PDUs using protocol-specific
 * callbacks.
 *
 * @param csap          CSAP instance
 * @param tmpl_unit     Traffic template unit
 * @param data          Location for template unit auxiluary data to be
 *                      prepared during preprocessing and used during
 *                      binary data generation
 *
 * @return Status code.
 */
static te_errno
tad_send_preprocess_pdus(csap_p csap, const asn_value *tmpl_unit,
                           tad_send_tmpl_unit_data *data)
{
    te_errno            rc;
    const asn_value    *nds_pdus = NULL;

    data->layer_opaque = calloc(csap->depth, sizeof(data->layer_opaque[0]));
    if (data->layer_opaque == NULL)
        return TE_RC(TE_TAD_CH, TE_ENOMEM);

    /*
     * Get sequence of PDUs and preprocess by protocol-specific
     * callbacks
     */
    rc = asn_get_child_value(tmpl_unit, &nds_pdus, PRIVATE, NDN_TMPL_PDUS);
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        VERB(CSAP_LOG_FMT "No PDUs in template unit",
             CSAP_LOG_ARGS(csap));
        nds_pdus = NULL;
    }
    else if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to get PDUs specification from "
              "template: %r", CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    /* FIXME: Remove type cast */
    rc = tad_confirm_pdus(csap, FALSE, (asn_value *)nds_pdus,
                          data->layer_opaque);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Confirmation of PDUs to send failed: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    return 0;
}

/**
 * Preprocess traffic template payload specification.
 *
 * @param csap          CSAP instance
 * @param tmpl_unit     Traffic template unit
 * @param data          Location for template unit auxiluary data to be
 *                      prepared during preprocessing and used during
 *                      binary data generation
 *
 * @return Status code.
 */
static te_errno
tad_send_preprocess_payload(csap_p csap, const asn_value *tmpl_unit,
                              tad_send_tmpl_unit_data *data)
{
    te_errno            rc;
    const asn_value    *nds_payload;

    /*
     * Get payload specification and convert to convinient
     * representation.
     */
    rc = asn_get_child_value(tmpl_unit, &nds_payload,
                             PRIVATE, NDN_TMPL_PAYLOAD);
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        VERB(CSAP_LOG_FMT "No payload in template unit",
             CSAP_LOG_ARGS(csap));
        data->pld_spec.type = TAD_PLD_UNSPEC;
        return 0;
    }
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to get payload specification from "
              "template: %r", CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    memset(&data->pld_spec, 0, sizeof(data->pld_spec));
    rc = tad_convert_payload(nds_payload, &data->pld_spec);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to preprocess payload specification: "
              "%r", CSAP_LOG_ARGS(csap), rc);
        return rc;
    }
    if (data->pld_spec.type == TAD_PLD_MASK)
    {
        ERROR(CSAP_LOG_FMT "Payload cannot be specified using mask",
              CSAP_LOG_ARGS(csap));
        return TE_RC(TE_TAD_CH, TE_ETADWRONGNDS);
    }

    return 0;
}

/* See description in tad_api.h */
te_errno
tad_send_preprocess_args(csap_p csap, const asn_value *tmpl_unit,
                           tad_send_tmpl_unit_data *data)
{
    te_errno            rc;
    const asn_value    *arg_sets;
    int                 len;

    rc = asn_get_descendent(tmpl_unit, (asn_value **)&arg_sets, "arg-sets");

    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        VERB(CSAP_LOG_FMT "No arguments in template unit",
             CSAP_LOG_ARGS(csap));
        return 0;
    }
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to get 'arg-sets' from template: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    len = asn_get_length(arg_sets, "");
    if (len <= 0)
    {
        ERROR(CSAP_LOG_FMT "Set of argument is specified but empty or "
              "incorrect", CSAP_LOG_ARGS(csap));
        return TE_RC(TE_TAD_CH, TE_EINVAL);
    }
    data->arg_num = len;

    data->arg_specs = calloc(data->arg_num, sizeof(data->arg_specs[0]));
    data->arg_iterated = calloc(data->arg_num,
                                sizeof(data->arg_iterated[0]));
    if (data->arg_specs == NULL || data->arg_iterated == NULL)
        return TE_RC(TE_TAD_CH, TE_ENOMEM);

    rc = tad_get_tmpl_arg_specs(arg_sets, data->arg_specs, data->arg_num);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to get arguments from template: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    rc = tad_init_tmpl_args(data->arg_specs, data->arg_num,
                            data->arg_iterated);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to initialize/iterate template "
              "arguments: %r", CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    return 0;
}

/**
 * Preprocess traffic template delays.
 *
 * @param csap          CSAP instance
 * @param tmpl_unit     Traffic template unit
 * @param data          Location for template unit auxiluary data to be
 *                      prepared during preprocessing and used during
 *                      binary data generation
 *
 * @return Status code.
 */
static te_errno
tad_send_preprocess_delays(csap_p csap, const asn_value *tmpl_unit,
                             tad_send_tmpl_unit_data *data)
{
    te_errno    rc;
    size_t      len = sizeof(data->delay);

    rc = asn_read_value_field(tmpl_unit, &data->delay, &len, "delays");

    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        VERB(CSAP_LOG_FMT "Delays are not specified",
             CSAP_LOG_ARGS(csap));
        data->delay = 0;
        return 0;
    }

    return rc;
}

/**
 * Preprocess traffic template unit. Check its correctness. Set default
 * values based on CSAP parameters.
 *
 * @param csap          CSAP instance
 * @param tmpl_unit     Traffic template unit
 * @param data          Location for template unit auxiluary data to be
 *                      prepared during preprocessing and used during
 *                      binary data generation
 *
 * @return Status code.
 */
static te_errno
tad_send_preprocess_template_unit(csap_p csap, asn_value *tmpl_unit,
                                    tad_send_tmpl_unit_data *data)
{
    te_errno    rc;

    data->nds = tmpl_unit;

    rc = tad_send_preprocess_pdus(csap, tmpl_unit, data);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Preprocessing of PDUs failed: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    rc = tad_send_preprocess_payload(csap, tmpl_unit, data);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Preprocessing of payload failed: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    rc = tad_send_preprocess_args(csap, tmpl_unit, data);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Preprocessing of arguments failed: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    rc = tad_send_preprocess_delays(csap, tmpl_unit, data);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Preprocessing of delays failed: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    return 0;
}

/**
 * Preprocess traffic template.
 *
 * @param csap          CSAP instance
 * @param template      Traffic template (owned by the routine in any
 *                      case)
 * @param data          Location for template auxiluary data to be
 *                      prepared during preprocessing and used during
 *                      binary data generation
 *
 * @return Status code.
 */
static te_errno
tad_send_preprocess_template(csap_p csap, asn_value *template,
                               tad_send_template_data *data)
{
    te_errno    rc;

    data->nds = template;

    /*
     * Current traffic template NDS support only one template unit
     * to send.
     */
    data->n_units = 1;

    data->units = calloc(data->n_units, sizeof(data->units[0]));
    if (data->units == NULL)
        return TE_RC(TE_TAD_CH, TE_ENOMEM);

    rc = tad_send_preprocess_template_unit(csap, template,
                                             data->units);

    return rc;
}


/**
 * Free TAD Sender data associated with traffic template unit.
 *
 * @param data  TAD Sender data associated with traffic template unit
 */
static void
tad_send_free_template_unit_data(csap_p csap,
                                   tad_send_tmpl_unit_data *data)
{
    /* ASN.1 value freed for whole template */
    unsigned int layer;

    for (layer = 0; layer < csap->depth; ++layer)
    {
        csap_layer_release_opaque_cb_t  release_tmpl_cb =
            csap_get_proto_support(csap, layer)->release_tmpl_cb;

        if (release_tmpl_cb != NULL)
            release_tmpl_cb(csap, layer, data->layer_opaque[layer]);
    }

    free(data->layer_opaque);

    free(data->arg_iterated);

    tad_tmpl_args_clear(data->arg_specs, data->arg_num);
    free(data->arg_specs);

    tad_payload_spec_clear(&data->pld_spec);
}

/**
 * Free TAD Sender data associated with traffic template.
 *
 * @param data  TAD Sender data associated with traffic template
 */
static void
tad_send_free_template_data(csap_p csap, tad_send_template_data *data)
{
    unsigned int    i;

    for (i = 0; i < data->n_units; ++i)
        tad_send_free_template_unit_data(csap, data->units + i);

    free(data->units);
    asn_free_value(data->nds);
}

/**
 * Free TAD Sender context.
 *
 * @param csap          CSAP
 * @param context       Context to be freed
 */
static void
tad_send_free_context(csap_p csap, tad_send_context *context)
{
    tad_send_free_template_data(csap, &context->tmpl_data);
    tad_reply_cleanup(&context->reply_ctx);
}


/* See description in tad_send.h */
void
tad_send_init_context(tad_send_context *context)
{
    memset(context, 0, sizeof(*context));
}

/* See description in tad_send.h */
te_errno
tad_send_prepare(csap_p csap, asn_value *template,
                 const tad_reply_context *reply_ctx)
{
    tad_send_context     *my_ctx = csap_get_send_context(csap);
    te_errno                rc;
    csap_low_resource_cb_t  prepare_send_cb;

    assert(csap != NULL);
    assert(template != NULL);

    my_ctx->status = 0;

    rc = tad_reply_clone(&my_ctx->reply_ctx, reply_ctx);
    if (rc != 0)
    {
        tad_send_free_context(csap, my_ctx);
        return rc;
    }

    rc = tad_send_preprocess_template(csap, template,
                                      &my_ctx->tmpl_data);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to preprocess traffic template: %r",
              CSAP_LOG_ARGS(csap), rc);
        tad_send_free_context(csap, my_ctx);
        return rc;
    }

    prepare_send_cb = csap_get_proto_support(csap,
                          csap_get_rw_layer(csap))->prepare_send_cb;

    if (prepare_send_cb != NULL && (rc = prepare_send_cb(csap)) != 0)
    {
        ERROR(CSAP_LOG_FMT "Prepare for send failed: %r",
              CSAP_LOG_ARGS(csap), rc);
        tad_send_free_context(csap, my_ctx);
        return rc;
    }

    return 0;
}

/* See description in tad_api.h */
te_errno
tad_send_release(csap_p csap, tad_send_context *context)
{
    te_errno                result = 0;
    te_errno                rc;
    csap_low_resource_cb_t  shutdown_send_cb;

    assert(csap != NULL);
    assert(context != NULL);

    shutdown_send_cb = csap_get_proto_support(csap,
                           csap_get_rw_layer(csap))->shutdown_send_cb;

    if (shutdown_send_cb != NULL && (rc = shutdown_send_cb(csap)) != 0)
    {
        ERROR(CSAP_LOG_FMT "Shut down sending failed: %r",
              CSAP_LOG_ARGS(csap), rc);
        TE_RC_UPDATE(result, rc);
    }

    tad_send_free_context(csap, context);

    return result;
}

/* See description in tad_api.h */
te_errno
tad_send_start_prepare(csap_p csap, const char *tmpl_str, te_bool postponed,
                       const tad_reply_context *reply_ctx)
{
    te_errno       rc;
    int            syms;
    asn_value     *nds;

    F_ENTRY(CSAP_LOG_FMT ", postponed=%u", CSAP_LOG_ARGS(csap), postponed);

    rc = csap_command(csap, TAD_OP_SEND);
    if (rc != 0)
        goto fail_csap_command;

    if (tmpl_str == NULL)
    {
        ERROR(CSAP_LOG_FMT "No NDS attached to traffic send start "
              "command", CSAP_LOG_ARGS(csap));
        rc = TE_ETADMISSNDS;
        goto fail_no_tmpl;
    }

    rc = asn_parse_value_text(tmpl_str, ndn_traffic_template,
                              &nds, &syms);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Parse error in attached NDS on symbol %d: %r",
              CSAP_LOG_ARGS(csap), syms, rc);
        goto fail_bad_tmpl;
    }

    CSAP_LOCK(csap);

    if (postponed)
        csap->state |= CSAP_STATE_FOREGROUND;

    csap->first_pkt = tad_tv_zero;
    csap->last_pkt  = tad_tv_zero;

    CSAP_UNLOCK(csap);

    rc = tad_send_prepare(csap, nds, reply_ctx);
    if (rc != 0)
        goto fail_send_prepare;

    F_EXIT("OK");
    return 0;

fail_send_prepare:
fail_bad_tmpl:
fail_no_tmpl:
    (void)csap_command(csap, TAD_OP_IDLE);
fail_csap_command:
    F_EXIT("%r", rc);
    return rc;
}

/* See description in tad_send.h */
te_errno
tad_send_stop(csap_p csap, unsigned int *sent_pkts)
{
    te_errno    rc;
    te_errno    status = 0;

    F_ENTRY(CSAP_LOG_FMT, CSAP_LOG_ARGS(csap));

    rc = csap_command(csap, TAD_OP_STOP);
    if (rc != 0)
        goto fail_csap_command;

    if (csap->state & CSAP_STATE_SEND)
    {
        rc = csap_wait(csap, CSAP_STATE_DONE);
        if (rc == 0)
            status = csap_get_send_context(csap)->status;
    }
    else
    {
        rc = TE_RC(TE_TAD_CH, TE_ETADCSAPSTATE);
    }

    if ((rc == 0) && (~csap->state & CSAP_STATE_FOREGROUND))
    {
        rc = csap_command(csap, TAD_OP_IDLE);
    }

    TE_RC_UPDATE(rc, status);

    *sent_pkts = csap_get_send_context(csap)->sent_pkts;

fail_csap_command:
    F_EXIT("%r", rc);
    return rc;
}


#if 0
static te_errno
tad_send_delay()
{
    uint32_t    delay;
    struct timeval npt; /* Next packet time moment */

        /* Delay for send, if necessary. */
        if (sent == 0)
        {
            gettimeofday(&npt, NULL);
            F_VERB("check start send moment: %u.%u",
                    npt.tv_sec, npt.tv_usec);
        }
        else if (delay > 0)
        {
            struct timeval tv_delay = {0, 0};
            struct timeval cm;

            gettimeofday(&cm, NULL);
            F_VERB("calc of delay, current moment: %u.%u",
                    cm.tv_sec, cm.tv_usec);

            /* calculate moment until we should wait before next
             * get log */
            npt.tv_sec  += delay / 1000;
            npt.tv_usec += delay * 1000;

            if (npt.tv_usec >= 1000000)
            {
                npt.tv_usec -= 1000000;
                npt.tv_sec ++;
            }
            F_VERB("wait for next send moment: %u.%u",
                    npt.tv_sec, npt.tv_usec);

            /* calculate delay of waiting we should wait before
             * next get log */
            if (npt.tv_sec >= cm.tv_sec)
                tv_delay.tv_sec  = npt.tv_sec  - cm.tv_sec;

            if (npt.tv_usec >= cm.tv_usec)
                tv_delay.tv_usec = npt.tv_usec - cm.tv_usec;
            else if (tv_delay.tv_sec > 0)
            {
                tv_delay.tv_usec = (npt.tv_usec + 1000000) - cm.tv_usec;
                tv_delay.tv_sec--;
            }

            F_VERB("calculated delay: %u.%u",
                    tv_delay.tv_sec, tv_delay.tv_usec);
            select(0, NULL, NULL, NULL, &tv_delay);
        }
}
#endif


/**
 * TAD Sender callback to send one packet.
 *
 * The function complies with tad_pkt_enum_cb prototype.
 */
static te_errno
tad_send_cb(tad_pkt *pkt, void *opaque)
{
    csap_p          csap = opaque;
    te_errno        rc;
    csap_write_cb_t write_cb;

    write_cb = csap_get_proto_support(csap,
                   csap_get_rw_layer(csap))->write_cb;
    rc = write_cb(csap, pkt);
    if (rc != 0)
    {
        /* An error occured */
        F_ERROR(CSAP_LOG_FMT "Write callback error: %r",
                CSAP_LOG_ARGS(csap), rc);
        /* Stop packets enumeration */
        return rc;
    }
    /* Written successfull */

    gettimeofday(&csap->last_pkt, NULL);
    if (csap->sender.sent_pkts == 0)
        csap->first_pkt = csap->last_pkt;

    csap->sender.sent_pkts++;

    F_VERB(CSAP_LOG_FMT "write callback OK, sent %u packets",
           CSAP_LOG_ARGS(csap), csap->sender.sent_pkts);

    /* Continue packets enumeration */
    return 0;
}

/**
 * Send list of packets.
 *
 * @param csap      CSAP instance
 *
 * @return Status code.
 */
static te_errno
tad_send_packets(csap_p csap, tad_pkts *pkts)
{
    return tad_pkt_enumerate(pkts, tad_send_cb, csap);
}


/**
 * Free array of lists with packets.
 *
 * @param pkts      Array pointer
 * @param num       Number of elements in array
 */
static void
tad_send_free_packets(tad_pkts *pkts, unsigned int num)
{
    unsigned int    i;

    for (i = 0; i < num; ++i)
    {
        tad_free_pkts(pkts + i);
    }
}

/**
 * Send traffic in accordance with specification in one template unit.
 *
 * @param csap          CSAP instance
 * @param tu_data       Template unit auxiluary data
 *
 * @return Status code.
 */
static te_errno
tad_send_by_template_unit(csap_p csap, tad_send_tmpl_unit_data *tu_data)
{
    te_errno        rc;
    tad_pkts       *pkts;
    unsigned int    i;

#if 1 /* FIXME: More part of this processing to prepare stage */
    tad_special_send_pkt_cb  send_cb = NULL;
    char                    *send_cb_name = NULL;
    char                    *send_cb_userdata = NULL;
#endif

    F_ENTRY();

    pkts = malloc((csap->depth + 1) * sizeof(*pkts));
    if (pkts == NULL)
        return TE_RC(TE_TAD_CH, TE_ENOMEM);

    for (i = 0; i <= csap->depth; i++)
    {
        tad_pkts_init(&pkts[i]);
    }

#if 1 /* FIXME: More part of this processing to prepare stage */
    rc = asn_read_string(tu_data->nds, &send_cb_name, "send-func");
    if (rc == 0)
    {
        send_cb_userdata = index(send_cb_name, ':');
        if (send_cb_userdata != NULL)
        {
            *send_cb_userdata = '\0';
            send_cb_userdata++;
        }

        /* FIXME: Correct error processing here */
        send_cb = rcf_ch_symbol_addr(send_cb_name, 1);
        if (send_cb == NULL)
            ERROR("Not send method '%s' found, send via usual callback",
                  send_cb_name);
    }
    else
#endif
        rc = 0;

    do {

        /* Check CSAP state */
        if (csap->state & CSAP_STATE_STOP)
        {
            INFO(CSAP_LOG_FMT "Send operation terminated",
                 CSAP_LOG_ARGS(csap));
            rc = TE_RC(TE_TAD_CH, TE_EINTR);
            break;
        }

        /* Generate packets to be send */
        rc = tad_send_prepare_bin(csap, tu_data->nds,
                                  tu_data->arg_iterated,
                                  tu_data->arg_num,
                                  &tu_data->pld_spec,
                                  tu_data->layer_opaque,
                                  pkts);
        F_VERB("send_prepare_bin rc: %r", rc);

        /* Delay requested amount of time between iterations */
        if (rc == 0)
        {
#if 0
            rc = tad_send_delay();
#endif
        }

        /* Send generated packets */
        if (rc == 0)
        {
            if (send_cb == NULL)
                rc = tad_send_packets(csap, pkts);
            else
                rc = send_cb(csap, send_cb_userdata, pkts);
            F_VERB(CSAP_LOG_FMT "send done for a template unit "
                   "iteration: %r", CSAP_LOG_ARGS(csap), rc);
        }

        /* Free resources allocated for packets */
        tad_send_free_packets(pkts, csap->depth + 1);

    } while (rc == 0 &&
             tad_iterate_tmpl_args(tu_data->arg_specs,
                                   tu_data->arg_num,
                                   tu_data->arg_iterated) > 0);

#if 0
    /* Looks like double free */
    tad_send_free_packets(pkts, csap->depth + 1);
#endif
    free(pkts);
    free(send_cb_name);

    F_EXIT("%r", rc);

    return rc;
}

/**
 * Send traffic in accordace with specification in template using
 * data prepared during preprocessing.
 *
 * @param csap          CSAP instance
 * @param tmpl_data     Template auxiluary data
 *
 * @return Status code.
 */
extern te_errno
tad_send_by_template(csap_p csap,
                            tad_send_template_data *tmpl_data)
{
    te_errno        rc;
    unsigned int    i;

    F_ENTRY();

    for (rc = 0, i = 0; rc == 0 && i < tmpl_data->n_units; ++i)
    {
        rc = tad_send_by_template_unit(csap,
                                              tmpl_data->units + i);
        F_VERB(CSAP_LOG_FMT "send done for a template unit: %r",
               CSAP_LOG_ARGS(csap), rc);
    }

    F_EXIT("%r", rc);

    return rc;
}

/* See description in tad_api.h */
te_errno
tad_send_do(csap_p csap)
{
    tad_send_context   *context;
    tad_reply_context   reply_ctx;
    te_errno            rc;

    assert(csap != NULL);
    F_ENTRY(CSAP_LOG_FMT, CSAP_LOG_ARGS(csap));

    context = csap_get_send_context(csap);
    assert(context != NULL);

    /* Clone reply context since we want to use it after send release */
    rc = tad_reply_clone(&reply_ctx, &context->reply_ctx);
    if (rc != 0)
    {
        (void)tad_reply_status(&context->reply_ctx, TE_RC(TE_TAD_CH, rc));
        (void)tad_send_release(csap, context);
        return rc;
    }

    if (csap->state & CSAP_STATE_FOREGROUND)
    {
        /*
         * When traffic send start is executed in foreground (with
         * waiting for end of operation or in the case of send/receive),
         * just send TE proto ACK to release the RCF session, since
         * we have gone to own thread.
         */
        rc = tad_reply_status(&reply_ctx, TE_RC(TE_TAD_CH, TE_EACK));
    }
    else
    {
        /*
         * When traffic send start is executed in background
         * (non-blocking mode), notify that operation is ready to start.
         */
        rc = tad_reply_pkts(&reply_ctx, 0, 0);
        tad_reply_cleanup(&reply_ctx);
    }
    /*
     * May be if TE proto reply is failed, it's better not to start at
     * all, but let's try.
     */
    TE_RC_UPDATE(context->status, rc);

    /* Send by preprocessed template */
    rc = tad_send_by_template(csap, &context->tmpl_data);
    F_VERB(CSAP_LOG_FMT "send done: %r", CSAP_LOG_ARGS(csap), rc);
    TE_RC_UPDATE(context->status, rc);

    /* Release all resources */
    rc = tad_send_release(csap, context);
    TE_RC_UPDATE(context->status, rc);

    /*
     * Transition of the CSAP state to DONE and send of TE protocol
     * reply have to be done under common lock. Otherwise,
     *  - if we send TE protocol reply and then transit to DONE-IDLE
     *    state, it is possible to get the next command before state
     *    transition and reply that CSAP is busy;
     *  - if we transit to DONE-IDLE state and then send TE protocol
     *    reply using Sender context structures, CSAP can be destroyed
     *    before processing with reply.
     *
     * When both operations are done under common lock, order does not
     * matter, since processing of any other command or continue with
     * CSAP destruction requires lock.
     *
     * Note that CSAP can be destroyed when lock is released.
     */
    CSAP_LOCK(csap);

    /* Ignore errors, since we can do nothing usefull here. */
    (void)csap_command_under_lock(csap, TAD_OP_SEND_DONE);

    if (csap->state & CSAP_STATE_RECV)
    {
        /*
         * Send/receive request - nothing to be reported by Sender.
         */
    }
    else if (csap->state & CSAP_STATE_FOREGROUND)
    {
        if (context->status != 0)
            rc = tad_reply_status(&reply_ctx, context->status);
        else
            rc = tad_reply_pkts(&reply_ctx, 0, context->sent_pkts);

        /* We can do nothing helpfull, if reply failed, just remember it */
        TE_RC_UPDATE(context->status, rc);
    }
    else
    {
        /*
         * Send operation was started in background, we have to preserve
         * the state and status of the operation to be reported on stop.
         */
    }

    /*
     * Log under the lock, since CSAP can be destroyed when lock is
     * released.
     */
    F_EXIT(CSAP_LOG_FMT, CSAP_LOG_ARGS(csap));

    CSAP_UNLOCK(csap);

    tad_reply_cleanup(&reply_ctx);

    return rc;
}

/* See description in tad_send.h */
void *
tad_send_thread(void *arg)
{
    csap_p  csap = arg;

    (void)tad_send_do(csap);

    return NULL;
}


/**
 * Callback for default payload memset, prototype complies to
 * type 'tad_pkt_seg_enum_cb'.
 */
static te_errno
tad_send_payload_default_fill(const tad_pkt *pkt,
                              tad_pkt_seg   *seg,
                              unsigned int   seg_num,
                              void          *opaque)
{
    UNUSED(pkt);
    UNUSED(seg_num);
    UNUSED(opaque);

    te_fill_buf(seg->data_ptr, seg->data_len);
    return 0;
}

/* See the description in tad_send.h */
te_errno
tad_send_prepare_bin(csap_p csap, asn_value *nds,
                     const tad_tmpl_arg_t *args, size_t arg_num,
                     tad_payload_spec_t *pld_data, void **layer_opaque,
                     tad_pkts *pkts_per_layer)
{
    tad_pkts   *pdus = pkts_per_layer + csap->depth;
    tad_pkts   *sdus;

    unsigned int    layer = 0;
    te_errno        rc = 0;
    const asn_value *layer_pdu = NULL;
    char  label[32];
    csap_layer_t *rw_layer;

    tad_pkts_init(pdus);
    rc = tad_pkts_alloc(pdus, 1, 0, 0);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "tad_pkts_alloc() for payload failed: %r",
              CSAP_LOG_ARGS(csap), rc);
        return TE_RC(TE_TAD_CH, rc);
    }

    switch (pld_data->type)
    {
        case TAD_PLD_UNSPEC:
            break;

        case TAD_PLD_FUNCTION:
        {
            size_t d_len = asn_get_length(nds, "payload.#bytes");
            void  *data = malloc(d_len);

            if (pld_data->func == NULL)
            {
                ERROR(CSAP_LOG_FMT "NULL function to generate payload",
                      CSAP_LOG_ARGS(csap));
                return TE_RC(TE_TAD_CH, TE_ETADWRONGNDS);
            }

            rc = pld_data->func(csap->id, -1 /* payload */,
                                nds);
            if (rc != 0)
            {
                ERROR(CSAP_LOG_FMT "Function to generate payload "
                      "failed: %r", CSAP_LOG_ARGS(csap), rc);
                return TE_RC(TE_TAD_CH, rc);
            }

            rc = asn_read_value_field(nds, data, &d_len, "payload.#bytes");
            if (rc != 0)
            {
                free(data);
                return TE_RC(TE_TAD_CH, rc);
            }
            rc = tad_pkts_add_new_seg(pdus, TRUE, data, d_len,
                                      tad_pkt_seg_data_free);
            break;
        }
        case TAD_PLD_BYTES:
            rc = tad_pkts_add_new_seg(pdus, TRUE,
                                      pld_data->plain.data,
                                      pld_data->plain.length,
                                      NULL);
            break;

        case TAD_PLD_LENGTH:
            rc = tad_pkts_add_new_seg(pdus, TRUE, NULL,
                                      pld_data->plain.length, NULL);
            if (rc != 0)
                break;
            /*
             * We know here that payloads are the first segments in
             * packets - since they are single yet, we start from
             * payload.
             */
            rc = tad_pkts_enumerate_first_segs(pdus,
                     tad_send_payload_default_fill, NULL);
            break;

        case TAD_PLD_STREAM:
        {
            uint32_t offset;
            uint32_t length;

            if (pld_data->stream.func == NULL)
            {
                ERROR(CSAP_LOG_FMT "NULL stream function to generate "
                      "payload", CSAP_LOG_ARGS(csap));
                return TE_RC(TE_TAD_CH, TE_ETADWRONGNDS);
            }
            rc = tad_data_unit_to_bin(&pld_data->stream.length,
                                      args, arg_num,
                                      (uint8_t *)&length,
                                      sizeof(length));
            if (rc != 0)
                break;
            length = ntohl(length);

            rc = tad_data_unit_to_bin(&pld_data->stream.offset,
                                      args, arg_num,
                                      (uint8_t *)&offset,
                                      sizeof(offset));
            if (rc != 0)
                break;
            offset = ntohl(offset);

            rc = tad_pkts_add_new_seg(pdus, TRUE, NULL, length, NULL);
            if (rc != 0)
            {
                ERROR(CSAP_LOG_FMT "Failed to add a new segment with "
                      "%u bytes of data for all PDUs",
                      CSAP_LOG_ARGS(csap), (unsigned)length);
                break;
            }
#if 0 /* FIXME */
            rc = pld_data->stream.func(offset, length,
                                       up_packets->data);
#endif
            break;
        }

        default:
            rc = TE_RC(TE_TAD_CH, TE_EOPNOTSUPP);
            break;
        /* TODO: processing of other choices should be inserted here. */
    }

    if (rc != 0)
    {
        tad_free_pkts(pdus);
        return TE_RC(TE_TAD_CH, rc);
    }

    rw_layer = &csap->layers[csap_get_rw_layer(csap)];
    if (rw_layer->rw_use_tad_pkt_seg_tagging)
    {
        /*
         * Flag payload segments as purportedly containing a custom layer tag
         * value so that they won't be marked with a bona fide layer tag once
         * that layer prepends extra segments containing the protocol headers.
         */
        rc = tad_pkt_enumerate(pdus, tad_pkt_mark_layer_segments_cb, NULL);
        if (rc != 0)
        {
            tad_free_pkts(pdus);
            return TE_RC(TE_TAD_CH, rc);
        }
    }

    /* All layers should be passed in any case to initialize pdus */
    for (layer = 0; layer < csap->depth; layer++)
    {
        sdus = pdus;
        pdus--;
        tad_pkts_init(pdus);

        if (rc == 0)
        {
            sprintf(label, "pdus.%u.#%s", layer, csap->layers[layer].proto);
            rc = asn_get_descendent(nds, (asn_value **)&layer_pdu, label);
            if (rc != 0)
            {
                ERROR(CSAP_LOG_FMT "Failed to get PDU template for "
                      "layer %u: %r", CSAP_LOG_ARGS(csap), layer, rc);
            }
        }
        if (rc == 0)
        {
            rc = csap_get_proto_support(csap, layer)->generate_pkts_cb(
                     csap, layer, layer_pdu, layer_opaque[layer],
                     args, arg_num, sdus, pdus);
            if (rc != 0)
            {
                ERROR(CSAP_LOG_FMT "Generate binary data on layer %u "
                      "(%s) failed: %r", CSAP_LOG_ARGS(csap), layer,
                      csap_get_proto_support(csap, layer)->proto, rc);
            }
        }
        if (rw_layer->rw_use_tad_pkt_seg_tagging && rc == 0)
        {
            rc = tad_pkt_enumerate(pdus, tad_pkt_mark_layer_segments_cb,
                                   &csap->layers[layer].proto_tag);
        }
    }

    return TE_RC(TE_TAD_CH, rc);
}


/**
 * Description see in tad_utils.h
 */
int
tad_iterate_tmpl_args(tad_tmpl_iter_spec_t *arg_specs,
                      size_t arg_specs_num,
                      tad_tmpl_arg_t *arg_iterated)
{
    int     dep = arg_specs_num - 1;
    te_bool performed = FALSE;

    tad_tmpl_iter_spec_t *arg_spec_p;

    if (arg_specs == NULL)
        return 0;

    if (arg_iterated == NULL)
        return -1;

    for (arg_spec_p = arg_specs + dep;
         dep >= 0 && !performed;
         dep--, arg_spec_p--)
    {
        switch(arg_spec_p->type)
        {
            case TAD_TMPL_ITER_FOR:
                if (arg_iterated[dep].arg_int < arg_spec_p->simple_for.end)
                {
                    arg_iterated[dep].arg_int +=
                        arg_spec_p->simple_for.step;
                    performed = TRUE;
                }
                else
                {
                    arg_iterated[dep].arg_int =
                        arg_spec_p->simple_for.begin;
                    /* formally, it's unnecessary here, but algorithm
                       is more clear with this operator. */
                }
                VERB("for, value %d, dep %d, performed %d",
                     arg_iterated[dep].arg_int, dep, (int)performed);
                break;

            case TAD_TMPL_ITER_INT_ASSOC:
            case TAD_TMPL_ITER_INT_SEQ:
                {
                    int new_index = arg_spec_p->int_seq.last_index + 1;

                    if ((size_t)new_index == arg_spec_p->int_seq.length)
                        new_index = 0;
                    else if (arg_spec_p->type == TAD_TMPL_ITER_INT_SEQ)
                        performed = TRUE;

                    arg_iterated[dep].arg_int =
                        arg_spec_p->int_seq.ints[new_index];

                    VERB("ints, index %d, value %d, dep %d, performed %d",
                         new_index, arg_iterated[dep].arg_int,
                         dep, (int)performed);

                    arg_spec_p->int_seq.last_index = new_index;
                }
                break;

            case TAD_TMPL_ITER_STR_SEQ:
                /* TODO: implement other choices in template argument */
                return -TE_EOPNOTSUPP;
                break;
        }
    }

    return (int)performed;
}

/**
 * Description see in tad_utils.h
 */
int
tad_get_tmpl_arg_specs(const asn_value *arg_set,
                       tad_tmpl_iter_spec_t *arg_specs, size_t arg_num)
{
    unsigned i;
    int      rc = 0;
    int32_t  arg_param = 0;
    size_t   enum_len;

    uint16_t      t_val;
    asn_tag_class t_class;

    if (arg_set == NULL || arg_specs == NULL)
        return -1;

    for (i = 0; i < arg_num; i++)
    {
        const asn_value *arg_val;
        const asn_value *arg_spec_elem;

        rc = asn_get_indexed(arg_set, (asn_value **)&arg_spec_elem,
                             i, NULL);
        if (rc)
        {
            WARN("%s(): asn_get_indexed(%d) failed %r",
                 __FUNCTION__, i, rc);
            break;
        }

        rc = asn_get_choice_value(arg_spec_elem, (asn_value **)&arg_val,
                                  &t_class, &t_val);
        if (rc)
        {
            WARN("%s(): asn_get_choice_value failed %r",
                 __FUNCTION__, rc);
            break;
        }

        VERB("iter tag class %d, tag val %d", (int)t_class, (int)t_val);


        switch (t_val)
        {
            case NDN_ITER_INTS:
            case NDN_ITER_INTS_ASSOC:
                arg_specs[i].type = ((t_val == NDN_ITER_INTS) ?
                        TAD_TMPL_ITER_INT_SEQ : TAD_TMPL_ITER_INT_ASSOC);
                enum_len = arg_specs[i].int_seq.length =
                                                asn_get_length(arg_val, "");
                arg_specs[i].int_seq.last_index = -1;
                arg_specs[i].int_seq.ints = calloc(enum_len,
                                                   sizeof(int32_t));
                if (arg_specs[i].int_seq.ints == NULL)
                {
                    rc = TE_RC(TE_TAD_CH, TE_ENOMEM);
                }
                else
                {
                    unsigned j;

                    for (j = 0; j < enum_len; j++)
                    {
                        const asn_value *int_val;

                        asn_get_indexed(arg_val, (asn_value **)&int_val,
                                        j, NULL);
                        asn_read_int32(int_val, &arg_param, "");
                        arg_specs[i].int_seq.ints[j] = arg_param;
                    }
                }
                break;

            case NDN_ITER_FOR:
                arg_specs[i].type = TAD_TMPL_ITER_FOR;
                rc = asn_read_int32(arg_val, &arg_param, "begin");
                arg_specs[i].simple_for.begin = ((rc == 0) ? arg_param :
                                            TAD_ARG_SIMPLE_FOR_BEGIN_DEF);

                VERB("simple-for, begin %d", arg_specs[i].simple_for.begin);

                rc = asn_read_int32(arg_val, &arg_param, "step");
                arg_specs[i].simple_for.step = ((rc == 0) ? arg_param :
                                            TAD_ARG_SIMPLE_FOR_STEP_DEF);
                VERB("simple-for, step %d", arg_specs[i].simple_for.step);

                rc = asn_read_int32(arg_val, &arg_param, "end");
                if (rc != 0)
                    break; /* There is no default for end of 'simple-for' */
                arg_specs[i].simple_for.end = arg_param;
                VERB("simple-for, end %d", arg_specs[i].simple_for.end);
                break;

            default:
            /* TODO: implement other choices in template argument */
                rc = TE_EOPNOTSUPP;
                break;
        }
        if (rc != 0)
            break;
    }
    return rc;
}



/**
 * Description see in tad_utils.h
 */
int
tad_init_tmpl_args(tad_tmpl_iter_spec_t *arg_specs, size_t arg_specs_num,
                   tad_tmpl_arg_t *arg_iterated)
{
    unsigned i;

    if (arg_specs == NULL || arg_specs_num == 0)
        return 0;

    if (arg_iterated == NULL)
        return TE_EWRONGPTR;

    memset(arg_iterated, 0, arg_specs_num * sizeof(tad_tmpl_arg_t));

    for (i = 0; i < arg_specs_num; i++)
    {
        switch (arg_specs[i].type)
        {
            case TAD_TMPL_ITER_INT_SEQ:
            case TAD_TMPL_ITER_INT_ASSOC:
                arg_iterated[i].arg_int = arg_specs[i].int_seq.ints[0];
                arg_specs[i].int_seq.last_index = 0;
                arg_iterated[i].type = TAD_TMPL_ARG_INT;
                break;

            case TAD_TMPL_ITER_FOR:
                arg_iterated[i].arg_int = arg_specs[i].simple_for.begin;
                arg_iterated[i].type = TAD_TMPL_ARG_INT;
                break;

            case TAD_TMPL_ITER_STR_SEQ:
                arg_iterated[i].type = TAD_TMPL_ARG_STR;
                break;
        }
    }
    return 0;
}

/**
 * Description see in tad_utils.h
 */
void
tad_tmpl_args_clear(tad_tmpl_iter_spec_t *arg_specs, unsigned int arg_num)
{
    unsigned int    i;

    if (arg_specs == NULL)
        return;
    for (i = 0; i < arg_num; i++)
    {
        switch (arg_specs[i].type)
        {
            case TAD_TMPL_ITER_INT_SEQ:
            case TAD_TMPL_ITER_INT_ASSOC:
                free(arg_specs[i].int_seq.ints);
                break;
            case TAD_TMPL_ITER_STR_SEQ:
            case TAD_TMPL_ITER_FOR:
                /* nothing to do */
                break;
        }
    }
}

