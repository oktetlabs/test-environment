/** @file
 * @brief TAD Receiver
 *
 * Traffic Application Domain Command Handler.
 * Receive module. 
 *
 * Copyright (C) 2003-2006 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAD Recv"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>

#include "comm_agent.h"
#include "logger_api.h"
#include "logger_ta_fast.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "ndn.h" 

#include "tad_csap_inst.h"
#include "tad_csap_support.h"
#include "tad_utils.h"
#include "tad_recv.h"


#define ANS_BUF 100
#define RBUF 0x4000

/**
 * Preprocess traffic pattern sequence of PDUs using protocol-specific
 * callbacks.
 *
 * @param csap          CSAP instance
 * @param ptrn_unit     Traffic pattern unit
 * @param data          Location for pattern unit auxiluary data to be
 *                      prepared during preprocessing and used during
 *                      binary data generation
 *
 * @return Status code.
 */
static te_errno
tad_recv_preprocess_pdus(csap_p csap, const asn_value *ptrn_unit,
                         tad_recv_ptrn_unit_data *data)
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
    rc = asn_get_child_value(ptrn_unit, &nds_pdus, PRIVATE, NDN_PU_PDUS);
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        VERB(CSAP_LOG_FMT "No PDUs in pattern unit",
             CSAP_LOG_ARGS(csap));
        nds_pdus = NULL;
    }
    else if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to get PDUs specification from "
              "pattern: %r", CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    /* FIXME: Remove type cast */
    rc = tad_confirm_pdus(csap, TRUE, (asn_value *)nds_pdus,
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
 * Preprocess traffic pattern payload specification.
 *
 * @param csap          CSAP instance
 * @param ptrn_unit     Traffic pattern unit
 * @param data          Location for pattern unit auxiluary data to be
 *                      prepared during preprocessing and used during
 *                      binary data generation
 *
 * @return Status code.
 */
static te_errno
tad_recv_preprocess_payload(csap_p csap, const asn_value *ptrn_unit,
                            tad_recv_ptrn_unit_data *data)
{
    te_errno            rc;
    const asn_value    *nds_payload;

    /*
     * Get payload specification and convert to convinient
     * representation.
     */
    rc = asn_get_child_value(ptrn_unit, &nds_payload,
                             PRIVATE, NDN_PU_PAYLOAD); 
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        VERB(CSAP_LOG_FMT "No payload in pattern unit",
             CSAP_LOG_ARGS(csap));
        data->pld_spec.type = TAD_PLD_UNSPEC;
        return 0;
    }
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to get payload specification from "
              "pattern: %r", CSAP_LOG_ARGS(csap), rc);
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

    return 0;
}

/**
 * Preprocess specification of one action in traffic pattern unit.
 *
 * @param nds_action    ASN.1 value with action specification
 * @param data          Location for action specification data
 *
 * @return Status code.
 */
static te_errno
tad_recv_preprocess_action(const asn_value *nds_action,
                           tad_action_spec *data)
{
    const asn_value *action_ch_val;
    asn_tag_class    t_class;
    asn_tag_value    t_val; 
    te_errno         rc;

    assert(nds_action != NULL);
    assert(data != NULL);

    rc = asn_get_choice_value(nds_action, &action_ch_val,
                              &t_class, &t_val);
    VERB("%s(): get action choice rc %r, class %d, tag %d", 
         __FUNCTION__, rc, (int)t_class, (int)t_val);
    if (rc != 0)
        return rc;

    data->type = t_val;

    switch (t_val)
    {
        case NDN_ACT_BREAK: 
        case NDN_ACT_NO_REPORT: 
        case NDN_ACT_ECHO:
            break;

        case NDN_ACT_FUNCTION: 
        {
            char    buffer[200] = {0,};
            size_t  buf_len = sizeof(buffer);
            char   *sep;

            rc = asn_read_value_field(action_ch_val, buffer,
                                      &buf_len, "");
            if (rc != 0)
            {
                ERROR("%s(): asn_read_value_field() for function "
                      "action specification: %r", __FUNCTION__, rc);
                break;
            }
            sep = index(buffer, ':');
            if (sep != NULL)
            {
                *sep = '\0';
                data->function.opaque = strdup(sep + 1);
            }
            VERB("%s(): action function name: '%s';, opaque '%s'",
                 __FUNCTION__, buffer, data->function.opaque);

            data->function.func = rcf_ch_symbol_addr(buffer, 1);
            if (data->function.func == NULL)
            {
                ERROR("No funcion named '%s' found", buffer);
                rc = TE_ENOENT; 
            }
            break;
        }

        case NDN_ACT_FORWARD_PLD:
        {
            int32_t target_csap_id;
            csap_p  target_csap;

            rc = asn_read_int32(action_ch_val, &target_csap_id, "");
            if (rc != 0)
            {
                ERROR("%s(): asn_read_int32() failed to target CSAP ID "
                      "of the forward payload action", __FUNCTION__);
                break;
            }
            target_csap = csap_find(target_csap_id);
            if (target_csap == NULL)
            {
                ERROR("Target CSAP #%u of forward payload action does "
                      "not exist", target_csap_id);
                rc = TE_ETADCSAPNOTEX;
            }
            else if (csap_get_proto_support(target_csap,
                         csap_get_rw_layer(target_csap))->write_cb == NULL)
            {
                ERROR("Target CSAP #%u of forward payload action unable "
                      "to send anything", target_csap_id);
                rc = TE_EOPNOTSUPP;
            }
            else
            {
                data->fwd_pld.csap_id = target_csap_id;
            }
            break; 
        }

        default:
            WARN("Unsupported action tag %d", (int)t_val);
            rc = TE_EINVAL;
            break;
    }

    return TE_RC(TE_TAD_CH, rc);
}

/**
 * Preprocess specification of actions in traffic pattern unit.
 *
 * @param csap          CSAP instance
 * @param ptrn_unit     Traffic pattern unit
 * @param data          Location for pattern unit auxiluary data to be
 *                      prepared during preprocessing and used during
 *                      binary data matching
 *
 * @return Status code.
 */
static te_errno
tad_recv_preprocess_actions(csap_p csap, const asn_value *ptrn_unit,
                            tad_recv_ptrn_unit_data *data)
{
    te_errno            rc;
    const asn_value    *nds_actions = NULL;
    int                 tmp;
    unsigned int        i;

    rc = asn_get_child_value(ptrn_unit, &nds_actions,
                             PRIVATE, NDN_PU_ACTIONS);
    if (TE_RC_GET_ERROR(rc) == TE_EASNINCOMPLVAL)
    {
        INFO(CSAP_LOG_FMT "No actions in pattern unit",
             CSAP_LOG_ARGS(csap));
        return 0;
    }
    else if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to get actions specification from "
              "pattern: %r", CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    tmp = asn_get_length(nds_actions, ""); 
    if (tmp < 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to get length of actions "
              "specification from pattern: %r", CSAP_LOG_ARGS(csap), rc);
        return rc;
    }
    else if (tmp == 0)
    {
        INFO(CSAP_LOG_FMT "Empty sequence of actions in pattern unit",
             CSAP_LOG_ARGS(csap));
        return 0;
    }
    data->n_actions = tmp;

    data->actions = calloc(data->n_actions, sizeof(data->actions[0]));
    if (data->actions == NULL)
        return TE_RC(TE_TAD_CH, TE_ENOMEM);

    for (i = 0; i < data->n_actions; ++i)
    { 
        const asn_value *nds_action;

        /* FIXME: Avoid type cast */
        rc = asn_get_indexed((asn_value *)nds_actions,
                             (asn_value **)&nds_action, i, NULL);
        if (rc != 0)
        {
            ERROR(CSAP_LOG_FMT "Get action #%u failed: %r",
                  CSAP_LOG_ARGS(csap), i, rc);
            break;
        }
        rc = tad_recv_preprocess_action(nds_action,
                                        data->actions + i);
        if (rc != 0)
        {
            ERROR(CSAP_LOG_FMT "Preprocessing of action #%u failed: %r",
                  CSAP_LOG_ARGS(csap), i, rc);
            break;
        }
    }
    if (rc == 0)
    {
        for (i = 0;
             (i < data->n_actions) &&
             (data->actions[i].type != NDN_ACT_NO_REPORT);
             ++i);
        data->no_report = (i < data->n_actions);
    }

    return rc;
}

/**
 * Preprocess traffic pattern unit. Check its correctness. Set default
 * values based on CSAP parameters.
 *
 * @param csap          CSAP instance
 * @param ptrn_unit     Traffic pattern unit
 * @param data          Location for pattern unit auxiluary data to be
 *                      prepared during preprocessing and used during
 *                      binary data generation
 *
 * @return Status code.
 */
static te_errno
tad_recv_preprocess_pattern_unit(csap_p csap, asn_value *ptrn_unit,
                                 tad_recv_ptrn_unit_data *data)
{
    te_errno    rc;

    data->nds = ptrn_unit;

    rc = tad_recv_preprocess_pdus(csap, ptrn_unit, data);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Preprocessing of PDUs failed: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    rc = tad_recv_preprocess_payload(csap, ptrn_unit, data);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Preprocessing of payload failed: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    rc = tad_recv_preprocess_actions(csap, ptrn_unit, data);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Preprocessing of payload failed: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    return 0;
}

/**
 * Preprocess traffic pattern.
 *
 * @param csap          CSAP instance
 * @param pattern       Traffic pattern (owned by the routine in any
 *                      case)
 * @param data          Location for pattern auxiluary data to be
 *                      prepared during preprocessing and used during
 *                      binary data generation
 *
 * @return Status code.
 */
static te_errno
tad_recv_preprocess_pattern(csap_p csap, asn_value *pattern,
                            tad_recv_pattern_data *data)
{
    int             n_units;
    unsigned int    i;
    te_errno        rc = 0;

    data->nds = pattern;

    n_units = asn_get_length(data->nds, "");
    if (n_units <= 0)
    {
        ERROR(CSAP_LOG_FMT "Invalid number of units (%d) in pattern",
              CSAP_LOG_ARGS(csap), n_units);
        return TE_RC(TE_TAD_CH, TE_ETADWRONGNDS);
    }
    data->n_units = n_units;

    data->units = calloc(data->n_units, sizeof(data->units[0]));
    if (data->units == NULL)
        return TE_RC(TE_TAD_CH, TE_ENOMEM);

    for (i = 0; i < data->n_units; ++i)
    {
        asn_value *pattern_unit = NULL;

        rc = asn_get_indexed(data->nds, &pattern_unit, i, NULL);
        if (rc != 0)
        {
            ERROR(CSAP_LOG_FMT "Failed to get pattern unit #%u: %r",
                  CSAP_LOG_ARGS(csap), i, rc);
            break;
        }

        rc = tad_recv_preprocess_pattern_unit(csap, pattern_unit,
                                                  data->units + i);
        if (rc != 0)
        {
            ERROR(CSAP_LOG_FMT "Preprocessing of pattern unit #%u "
                  "failed: %r", CSAP_LOG_ARGS(csap), i, rc);
            break;
        }
    }

    return rc;
}


/**
 * Free TAD Receiver data associated with traffic pattern unit.
 *
 * @param data  TAD Receiver data associated with traffic pattern unit
 */
static void
tad_recv_free_pattern_unit_data(csap_p csap,
                                tad_recv_ptrn_unit_data *data)
{
    /* ASN.1 value freed for whole pattern */
    unsigned int layer;

    for (layer = 0; layer < csap->depth; ++layer)
    {
        csap_layer_release_opaque_cb_t  release_ptrn_cb =
            csap_get_proto_support(csap, layer)->release_ptrn_cb;

        if (release_ptrn_cb != NULL)
            release_ptrn_cb(csap, layer, data->layer_opaque[layer]);
    }

    free(data->layer_opaque);

    tad_payload_spec_clear(&data->pld_spec);
}

/**
 * Free TAD Receiver data associated with traffic pattern.
 *
 * @param data  TAD Receiver data associated with traffic pattern
 */
static void
tad_recv_free_pattern_data(csap_p csap, tad_recv_pattern_data *data)
{
    unsigned int    i;

    for (i = 0; i < data->n_units; ++i)
        tad_recv_free_pattern_unit_data(csap, data->units + i);
    data->n_units = 0;

    free(data->units);
    data->units = NULL;
    asn_free_value(data->nds);
    data->nds = NULL;
}

/**
 * Release TAD Receiver context (received packets queue, status and
 * counters are preserved).
 *
 * @param csap          CSAP instance
 * @param context       Context to be released
 */
static void
tad_recv_release_context(csap_p csap, tad_recv_context *context)
{
    tad_recv_free_pattern_data(csap, &context->ptrn_data);
}


/* See description in tad_recv.h */
void
tad_recv_init_context(tad_recv_context *context)
{
    memset(context, 0, sizeof(*context));
    TAILQ_INIT(&context->packets);
}

/* See description in tad_recv.h */
te_errno
tad_recv_prepare(csap_p csap, asn_value *pattern, unsigned int num, 
                     unsigned int timeout, rcf_comm_connection *rcfc,
                     const char *answer_pfx, size_t pfx_len)
{
    tad_recv_context       *my_ctx = csap_get_recv_context(csap);
    te_errno                rc;
    csap_low_resource_cb_t  prepare_recv_cb;

    assert(csap != NULL);
    assert(pattern != NULL);

    assert(my_ctx->packets.tqh_first == NULL);

    my_ctx->status = 0;
    my_ctx->wait_pkts = num;
    my_ctx->match_pkts = my_ctx->got_pkts = 0;

    if (timeout && timeout != TAD_TIMEOUT_INF)
    { 
        gettimeofday(&csap->wait_for, NULL);
        csap->wait_for.tv_usec += timeout * 1000;
        csap->wait_for.tv_sec += 
            (csap->wait_for.tv_usec / 1000000);
        csap->wait_for.tv_usec %= 1000000;

        VERB("%s(): csap %u, wait_for set to %u.%u", __FUNCTION__,
             csap->id,
             csap->wait_for.tv_sec,
             csap->wait_for.tv_usec);
    }
    else 
        memset(&csap->wait_for, 0, sizeof(struct timeval));

    rc = tad_task_init(&my_ctx->task, rcfc, answer_pfx, pfx_len);
    if (rc != 0)
    {
        tad_recv_release_context(csap, my_ctx);
        return rc;
    }

    rc = tad_recv_preprocess_pattern(csap, pattern, &my_ctx->ptrn_data);
    if (rc != 0)
    {
        ERROR(CSAP_LOG_FMT "Failed to preprocess pattern: %r",
              CSAP_LOG_ARGS(csap), rc);
        tad_recv_release_context(csap, my_ctx);
        return rc;
    }

    prepare_recv_cb = csap_get_proto_support(csap,
                          csap_get_rw_layer(csap))->prepare_recv_cb;

    if (prepare_recv_cb != NULL && (rc = prepare_recv_cb(csap)) != 0)
    {
        ERROR(CSAP_LOG_FMT "Prepare for receive failed: %r",
              CSAP_LOG_ARGS(csap), rc);
        tad_recv_release_context(csap, my_ctx);
        return rc;
    }

    return 0;
}

/**
 * Shutdown receiver on the CSAP.
 *
 * @param csap      CSAP structure
 *
 * @return Status code.
 */
static te_errno
tad_recv_shutdown(csap_p csap)
{
    te_errno                rc;
    csap_low_resource_cb_t  shutdown_recv_cb;

    shutdown_recv_cb = csap_get_proto_support(csap,
                           csap_get_rw_layer(csap))->shutdown_recv_cb;

    if (shutdown_recv_cb != NULL && (rc = shutdown_recv_cb(csap)) != 0)
    {
        ERROR(CSAP_LOG_FMT "Shut down receiver failed: %r",
              CSAP_LOG_ARGS(csap), rc);
        return rc;
    }

    return 0;
}

/* See description in tad_recv.h */
te_errno
tad_recv_release(csap_p csap, tad_recv_context *context)
{
    te_errno    rc = 0;

    assert(csap != NULL);
    assert(context != NULL);

    rc = tad_recv_shutdown(csap);

    tad_recv_release_context(csap, context);

    return rc;
}


/*
 * Time critical processing (receive and match).
 */

/**
 * Process action for received packet.
 *
 * @param csap          CSAP descriptor
 * @param action_spec   Preprocessed action specification
 * @param low_pkts      Lowest layer packets
 * @param payload       Packet with payload
 *
 * @return Status code.
 */
static te_errno
tad_recv_do_action(csap_p csap, tad_action_spec *action_spec,
                   const tad_pkts *low_pkts, const tad_pkt *payload)
{
    te_errno    rc = 0;

    assert(csap != NULL);
    assert(action_spec != NULL);

    switch (action_spec->type)
    {
        case NDN_ACT_BREAK: 
            csap->state |= CSAP_STATE_COMPLETE;
            break;

        case NDN_ACT_NO_REPORT: 
            /* do nothing: processed on higher layers. */
            break;

#if 0
        case NDN_ACT_ECHO:
            if (csap->echo_cb != NULL)
            {
                rc = csap->echo_cb(csap, raw_pkt, raw_len);
                if (rc)
                    ERROR("csap #%d, echo_cb returned %r code.", 
                          csap->id, rc);
                /* Have no reason to stop receiving. */
                rc = 0;
            }
            break;
#endif

        case NDN_ACT_FUNCTION: 
            if (tad_pkts_get_num(low_pkts) == 1)
            {
                uint8_t    *raw_pkt = NULL;
                size_t      raw_len = 0;

                rc = tad_pkt_flatten_copy(tad_pkts_first_pkt(low_pkts),
                                          &raw_pkt, &raw_len);
                if (rc != 0)
                {
                    ERROR("Failed to make flatten copy of packet: %r",
                          rc);
                }
                else
                {
                    rc = action_spec->function.func(csap,
                             action_spec->function.opaque, raw_pkt,
                             raw_len);
                    if (rc != 0)
                    {
                        WARN(CSAP_LOG_FMT "User function failed: %r",
                             CSAP_LOG_ARGS(csap), rc);
                    }
                }
                /* Don't want to stop receiver */
                rc = 0;
            }
            else
            {
                WARN("Unsupported number %u of the lowest layer "
                     "packets in 'function' action", 
                     tad_pkts_get_num(low_pkts));
                /* Don't want to stop receiver */
            }
            break;

        case NDN_ACT_FORWARD_PLD:
        {
            csap_p          target_csap;
            csap_spt_type_p cbs;

            target_csap = csap_find(action_spec->fwd_pld.csap_id);
            if (target_csap == NULL)
            {
                WARN(CSAP_LOG_FMT "target CSAP #%u for 'forward "
                     "payload' action disappeared",
                     CSAP_LOG_ARGS(csap), action_spec->fwd_pld.csap_id);
                /* Don't want to stop receiver */
            }
            else
            {
                cbs = csap_get_proto_support(target_csap,
                          csap_get_rw_layer(target_csap));
                if (cbs != NULL && cbs->write_cb != NULL)
                {
                    rc = cbs->write_cb(target_csap, payload);
                    F_VERB(CSAP_LOG_FMT "action 'forward payload' to "
                           "CSAP #%u processed: %r", CSAP_LOG_ARGS(csap),
                           target_csap->id, rc);
                    /* Don't want to stop receiver */
                    rc = 0;
                }
                else
                {
                    WARN(CSAP_LOG_FMT "target CSAP #%u for 'forward "
                         "payload' action invalid", CSAP_LOG_ARGS(csap),
                         action_spec->fwd_pld.csap_id);
                    /* Don't want to stop receiver */
                }
            } 
            break; 
        }

        default:
            /* It have to be caught by preprocessing */
            assert(FALSE);
    }

    return rc;
}

/**
 * Process actions for received packet.
 *
 * @param csap          CSAP descriptor
 * @param n_action      Number of actions to do
 * @param action_specs  Preprocessed actions specification array
 * @param low_pkts      Lowest layer packets
 * @param payload       Packet with payload
 *
 * @return Status code.
 */
static te_errno
tad_recv_do_actions(csap_p csap, unsigned int n_actions,
                    tad_action_spec *action_specs,
                    const tad_pkts *low_pkts, const tad_pkt *payload)
{
    te_errno        rc = 0;
    unsigned int    i;

    for (i = 0; i < n_actions; ++i)
    { 
        rc = tad_recv_do_action(csap, action_specs + i,
                                low_pkts, payload);
        if (rc != 0)
        {
            ERROR(CSAP_LOG_FMT "Action #%u failed: %r",
                  CSAP_LOG_ARGS(csap), i, rc);
            break;
        }
    }
    return rc;
}

/**
 * Match received payload against specified in pattern.
 *
 * @param pattern       Preprocessed payload specificatin from pattern
 * @param payload       Received payload
 *
 * @return Status code.
 */
static te_errno
tad_recv_match_payload(tad_payload_spec_t *pattern, const tad_pkt *payload)
{
    te_errno    rc;

    ENTRY("payload_type=%d payload=%p payload_len=%u", pattern->type,
          payload, (unsigned)tad_pkt_len(payload));

    switch (pattern->type)
    {
        case TAD_PLD_MASK:
            rc = tad_pkt_match_mask(payload, pattern->mask.length,
                                    pattern->mask.mask,
                                    pattern->mask.value,
                                    pattern->mask.exact_len);
            break;

        default:
            rc = TE_EOPNOTSUPP;
    }

    EXIT("%r", rc);

    return rc;
}
 
/**
 * Try match binary data with Traffic-Pattern-Unit and prepare ASN value 
 * with packet if it satisfies to the pattern unit.
 *
 * @param csap          CSAP instance
 * @param unit_data     Unit data prepared during preprocessing
 * @param meta_pkt      Receiver meta packet
 *
 * @return Status code.
 * @retval 0                Received packet match, meta_pkt is owned and
 *                          segments from pkt are extracted
 * @retval TE_ETADLESSDATA  Need more data, meta_pkt is owned and
 *                          segments from pkt are extracted
 * @retval TE_ETADNOTMATCH  Received packet not match, meta_pkt is not
 *                          owned
 * @retval other            Unexpected error, meta_pkt is not owned
 */
static te_errno
tad_recv_match_with_unit(csap_p csap, tad_recv_ptrn_unit_data *unit_data,
                         tad_recv_pkt *meta_pkt)
{
    const asn_value    *pattern_unit = unit_data->nds;
    unsigned int        layer;
    te_errno            rc;
    tad_pkt            *pdu;
    tad_pkt            *sdu;

    char label[20] = "pdus";

    /* Start from the bottom */
    layer = csap->depth - 1;
    sdu = tad_pkts_first_pkt(&meta_pkt->layers[layer].pkts);
    assert(sdu != NULL);

    /* Match layer by layer */
    do {
        csap_spt_type_p  csap_spt_descr; 
        const asn_value *layer_pdu = NULL; 

        sprintf(label + sizeof("pdus") - 1, ".%d", layer);
        rc = asn_get_subvalue(pattern_unit, &layer_pdu, label); 
        VERB("get subval with pattern unit for label %s rc %r",
             label, rc);

        csap_spt_descr = csap_get_proto_support(csap, layer);

        pdu = sdu;
        sdu = (layer == 0) ? &meta_pkt->payload :
                  tad_pkts_first_pkt(&meta_pkt->layers[layer - 1].pkts);
        assert(sdu != NULL);

        rc = csap_spt_descr->match_do_cb(csap, layer, layer_pdu, 
                                         unit_data->layer_opaque[layer],
                                         meta_pkt, pdu, sdu);
        INFO("match cb 0x%x for layer %u sdu_len=%u returned %r",
             csap_spt_descr->match_do_cb, layer,
             (unsigned)tad_pkt_len(sdu), rc);
        if (rc != 0)
            return rc;
    } while (layer-- > 0);

    /* Match payload */
    if ((unit_data->pld_spec.type != TAD_PLD_UNSPEC) &&
        ((rc = tad_recv_match_payload(&unit_data->pld_spec,
                                      &meta_pkt->payload)) != 0))
    {
        return rc;
    }

    /* Do action, if it presents. */
    if (rc == 0 && unit_data->n_actions > 0) 
    { 
        rc = tad_recv_do_actions(csap, unit_data->n_actions,
                                 unit_data->actions,
                                 &meta_pkt->layers[csap->depth - 1].pkts,
                                 &meta_pkt->payload);
        /* Errors are logged in the called function */
    }

    return rc; 
}

/**
 * Try match binary data with Traffic-Pattern.
 *
 * @param csap          CSAP instance
 * @param ptrn_data     Traffic pattern data prepared during
 *                      preprocessing
 * @param meta_pkt      Receiver meta packet (the first raw packet
 *                      contains just received data)
 * @param pkt_len       Real length of usefull data in pkt
 * @param no_report     If match, include in statistics but does not
 *                      report raw packet to the test
 *
 * @return Status code.
 */
static te_errno
tad_recv_match(csap_p csap, tad_recv_pattern_data *ptrn_data,
               tad_recv_pkt *meta_pkt, size_t pkt_len, te_bool *no_report)
{
    unsigned int    unit = 0;
    te_errno        rc;

    /* Create a packet with received data only for the bottom layer */
    rc = tad_pkt_get_frag(
             tad_pkts_first_pkt(&meta_pkt->layers[csap->depth - 1].pkts),
             tad_pkts_first_pkt(&meta_pkt->raw), 0, pkt_len,
             TAD_PKT_GET_FRAG_ERROR);
    if (rc != 0)
    {
        assert(TE_RC_GET_ERROR(rc) != TE_ETADLESSDATA);
        assert(TE_RC_GET_ERROR(rc) != TE_ETADNOTMATCH);
        return rc;
    }

    assert(ptrn_data->n_units > 0);
    do {
        rc = tad_recv_match_with_unit(csap, ptrn_data->units + unit,
                                      meta_pkt);
        switch (TE_RC_GET_ERROR(rc))
        {
            case 0: /* received data matches to this pattern unit */
                assert(no_report != NULL);
                *no_report = ptrn_data->units[unit].no_report;
            case TE_ETADLESSDATA:
                F_VERB(CSAP_LOG_FMT "Match packet with unit #%u - %r", 
                       CSAP_LOG_ARGS(csap), unit, rc);
                /* Meta-packet with received data is owned */
                return rc;

            case TE_ETADNOTMATCH:
                F_VERB(CSAP_LOG_FMT "Match packet with unit #%u - %r", 
                       CSAP_LOG_ARGS(csap), unit, rc);
                continue;

            default: 
                ERROR(CSAP_LOG_FMT "Match with pattern unit #%u failed: "
                      "%r", CSAP_LOG_ARGS(csap), unit, rc);
                break;
        }
        if (rc != 0)
            break;

    } while (++unit < ptrn_data->n_units);

    return rc;
}

/**
 * Add packet into the queue of received packet.
 *
 * @param csap          CSAP instance
 * @param pkts          Queue of received packets
 * @param pkt           Receiver meta packet
 */
static void
tad_recv_pkt_enqueue(csap_p csap, tad_recv_pkts *pkts, tad_recv_pkt *pkt)
{
    int ret;

    CSAP_LOCK(csap);
    TAILQ_INSERT_TAIL(pkts, pkt, links);
    if ((ret = pthread_cond_broadcast(&csap->event)) != 0)
    {
        te_errno rc = TE_OS_RC(TE_TAD_CH, ret);
        
        assert(rc != 0);
        ERROR(CSAP_LOG_FMT "Failed to broadcast CSAP event - received "
              "packet: %r - ignore", CSAP_LOG_ARGS(csap), rc);
    }
    CSAP_UNLOCK(csap);
}


/* See description in tad_recv.h */
void *
tad_recv_thread(void *arg)
{
    csap_p              csap = arg;
    tad_recv_context   *context;
    csap_read_cb_t      read_cb;
    te_bool             stop_on_timeout = FALSE;
    te_errno            rc;
    te_bool             no_report = FALSE;
    tad_recv_pkt       *meta_pkt = NULL;
    tad_pkt            *pkt;
    size_t              read_len;


    assert(csap != NULL);
    read_cb = csap_get_proto_support(csap,
                  csap_get_rw_layer(csap))->read_cb;
    assert(read_cb != NULL);

    context = csap_get_recv_context(csap);
    assert(context != NULL);
    assert(context->match_pkts == 0);
    assert(context->packets.tqh_first == NULL);

    ENTRY(CSAP_LOG_FMT, CSAP_LOG_ARGS(csap));

    if (csap->state & CSAP_STATE_SEND)
    {
        /*
         * When traffic receive start is executed together with send
         * (it can be send/receive only), there is no necessity to
         * send TE proto ACK, since it will be done by Sender.
         */
        tad_task_free(&context->task);

        /* Start receiver only when send is done. */
        rc = csap_wait(csap, CSAP_STATE_SEND_DONE);
        if (rc != 0)
            goto exit;

        /* Check Sender status. */
        rc = csap_get_send_context(csap)->status;
        if (rc != 0)
        {
            ERROR(CSAP_LOG_FMT "send/receive Sender failed, do not "
                  "start Receiver", CSAP_LOG_ARGS(csap));
            goto exit;
        }
    }
    else
    {
        /*
         * When traffic receive start is executed stand alone (always
         * non-blocking mode), notify that operation is ready to start.
         */
        rc = tad_task_reply(&context->task, "0 0");
        tad_task_free(&context->task);
        if (rc != 0)
            goto exit;
    }

    /* 
     * Allocate Receiver packet to avoid extra memory allocation on 
     * failed match path.
     */
    if ((meta_pkt = tad_recv_pkt_alloc(csap)) == NULL)
    {
        ERROR(CSAP_LOG_FMT "Failed to initialize Receiver packet",
              CSAP_LOG_ARGS(csap));
        rc = TE_RC(TE_TAD_CH, TE_ENOMEM);
        goto exit;
    }

    while (TRUE)
    {
        unsigned int    timeout;

        /* Check CSAP state */
        if (csap->state & CSAP_STATE_COMPLETE)
        {
            INFO(CSAP_LOG_FMT "Receive operation completed",
                 CSAP_LOG_ARGS(csap));
            assert(rc == 0);
            break;
        }
        if (csap->state & CSAP_STATE_STOP)
        {
            INFO(CSAP_LOG_FMT "Receive operation terminated",
                 CSAP_LOG_ARGS(csap));
            rc = TE_RC(TE_TAD_CH, TE_EINTR);
            break;
        }

        /* Check for timeout */
        timeout = csap->timeout;
        if (csap->wait_for.tv_sec != 0)
        {
            struct timeval  current;
            int             wait_timeout;

            gettimeofday(&current, NULL);
            wait_timeout =
                TE_SEC2US(csap->wait_for.tv_sec - current.tv_sec) +
                csap->wait_for.tv_usec - current.tv_usec;

            if (wait_timeout < 0)
            {
                if (stop_on_timeout)
                {
                    INFO("CSAP %d status complete by timeout, "
                         "wait for: %u.%u, current: %u.%u",
                         csap->id,
                         (unsigned)csap->wait_for.tv_sec,
                         (unsigned)csap->wait_for.tv_usec,
                         (unsigned)current.tv_sec,
                         (unsigned)current.tv_usec);
                    rc = TE_RC(TE_TAD_CH, TE_ETIMEDOUT);
                    break;
                }
                else
                {
                    INFO(CSAP_LOG_FMT "timed out, but don't want to "
                         "stop ", CSAP_LOG_ARGS(csap));
                    wait_timeout = 0;
                }
            }
            /* Here, it is guaranteed that wait_timeout is not negative */
            timeout = MIN(timeout, (unsigned int)wait_timeout);
        }

        if ((meta_pkt == NULL) &&
            ((meta_pkt = tad_recv_pkt_alloc(csap)) == NULL))
        {
            ERROR(CSAP_LOG_FMT "Failed to initialize Receiver packet",
                  CSAP_LOG_ARGS(csap));
            rc = TE_RC(TE_TAD_CH, TE_ENOMEM);
            goto exit;
        }
        pkt = tad_pkts_first_pkt(&meta_pkt->raw);
        assert(pkt != NULL);

        /* Read one packet from media */
        rc = read_cb(csap, timeout, pkt, &read_len); 
        gettimeofday(&meta_pkt->ts, NULL);

        /* We have read something, now allow to stop on timeout */
        stop_on_timeout = TRUE;

        if (TE_RC_GET_ERROR(rc) == TE_ETIMEDOUT)
        {
            VERB(CSAP_LOG_FMT "read callback timed out, check state and "
                 "total timeout", CSAP_LOG_ARGS(csap));
            continue;
        }
        if (rc != 0)
        {
            /* Unexpected read callback error */
            ERROR(CSAP_LOG_FMT "read callback failed: %r",
                  CSAP_LOG_ARGS(csap), rc);
            break;
        }

        /* Match received packet against pattern */
        rc = tad_recv_match(csap, &context->ptrn_data, meta_pkt,
                            read_len, &no_report);
        if (TE_RC_GET_ERROR(rc) == TE_ETADNOTMATCH)
        {
            VERB(CSAP_LOG_FMT "received packet does not match",
                 CSAP_LOG_ARGS(csap));
            /* Nothing is owned by match routine */
            tad_recv_pkt_cleanup(csap, meta_pkt);
            continue;
        }
        if (TE_RC_GET_ERROR(rc) == TE_ETADLESSDATA)
        {
            VERB(CSAP_LOG_FMT "received packet does not match since "
                 "more data are available", CSAP_LOG_ARGS(csap));

            /* Receiver meta packet is owned by match */
            meta_pkt = NULL;

            /* 
             * Packet can match, if more data is available. Therefore,
             * don't want to stop because of timeout, at least continue
             * to poll with zero timeout.
             */
            stop_on_timeout = FALSE;
            continue;
        }
        if (rc != 0) /* Unexpected match error */
        {
            /* Nothing is owned by match routine */
            ERROR(CSAP_LOG_FMT "match failed: %r",
                  CSAP_LOG_ARGS(csap), rc);
            break;
        }

        /* Here packet is successfully received, parsed and matched */
        csap->last_pkt = meta_pkt->ts;
        if (context->match_pkts == 0)
            csap->first_pkt = csap->last_pkt;
        context->match_pkts++;

        if ((csap->state & CSAP_STATE_RESULTS) && !no_report)
        { 
            F_VERB(CSAP_LOG_FMT "put packet into the queue",
                   CSAP_LOG_ARGS(csap));
            tad_recv_pkt_enqueue(csap, &context->packets, meta_pkt);
            meta_pkt = NULL;
        } 
        else
        {
            no_report = FALSE;
            tad_recv_pkt_cleanup(csap, meta_pkt);
        }

        /* Check for total number of packets to be received */
        if ((context->wait_pkts != 0) &&
            (context->match_pkts >= context->wait_pkts))
        {
            assert(context->match_pkts == context->wait_pkts);
            INFO(CSAP_LOG_FMT "received all packets",
                 CSAP_LOG_ARGS(csap)); 
            assert(rc == 0);
            break;
        }
    }

exit:
    context->status = rc;

    /* 
     * Shutdown receiver and release resources allocated during pattern
     * preprocessing.
     */
    rc = tad_recv_release(csap, context);
    TE_RC_UPDATE(context->status, rc);

    tad_recv_pkt_free(csap, meta_pkt);

    INFO(CSAP_LOG_FMT "receive process finished, %u packets match: %r",
         CSAP_LOG_ARGS(csap), context->match_pkts, context->status);

    /* 
     * Log exit before DONE command on the CSAP, since it can be
     * destroyed just after the command.
     */
    F_EXIT(CSAP_LOG_FMT, CSAP_LOG_ARGS(csap));

    /* 
     * Notify that operation has been finished. CSAP can't be used
     * in this context after the command, since it can already be
     * destroyed.
     * Ignore errors, since all are logged inside the function and
     * we can do nothing helpfull here.
     */
    (void)csap_command(csap, TAD_OP_RECV_DONE);

    return NULL;
}



/*
 * Traffic receive get/wait/stop and busy CSAP destroy processing.
 */

/**
 * Send received packet to the test via RCF.
 *
 * @param packet        ASN.1 value with received packets
 * @param rcfc          Handle of RCF connection
 * @param answer_buffer Buffer with begin of answer
 * @param ans_len       Index of first significant symbols in answer_buffer
 *
 * @return Status code.
 */
static te_errno
tad_recv_report_packet(const asn_value *packet, rcf_comm_connection *rcfc, 
                       const char *answer_buffer, size_t ans_len) 
{
/* 
 * It is an upper estimation for "attach" and decimal presentation
 * of attach length.  
 */
#define EXTRA_BUF_SPACE     20

    te_errno    rc;
    int         ret;
    size_t      attach_len;
#if 1 /* FIXME ASN */
    int         attach_rlen;
#endif
    char       *buffer;
    size_t      cmd_len;

    assert(packet != NULL);

    attach_len = asn_count_txt_len(packet, 0);
    VERB("%s(): attach len %u", __FUNCTION__, (unsigned)attach_len);

    buffer = calloc(1, ans_len + EXTRA_BUF_SPACE + attach_len);
    if (buffer == NULL)
    {
        return TE_ENOMEM;
    }

    memcpy(buffer, answer_buffer, ans_len);
    ret = snprintf(buffer + ans_len, EXTRA_BUF_SPACE, " attach %u",
                   (unsigned)attach_len);
    if (ret >= EXTRA_BUF_SPACE)
    {
        ERROR("%s(): Upper estimation on required buffer space is wrong",
              __FUNCTION__);
        free(buffer);
        return TE_ESMALLBUF;
    }
    cmd_len = strlen(buffer) + 1;

    if ((attach_rlen =
         asn_sprint_value(packet, buffer + cmd_len, attach_len
#if 1 /* FIXME ASN */
                          + 1
#endif
                          , 0))
        != (int)attach_len)
    {
        ERROR("%s(): asn_sprint_value() returns unexpected number: "
              "expected %u, got %d", __FUNCTION__, (unsigned)attach_len,
              attach_rlen);
#if 0 /* FIXME ASN */
        free(buffer);
        return TE_EFAULT;
#endif
    } 

    rcf_ch_lock();
    rc = rcf_comm_agent_reply(rcfc, buffer, cmd_len + attach_len);
    rcf_ch_unlock(); 
    free(buffer);

    return rc;

#undef EXTRA_BUF_SPACE
}


/**
 * Get packets from queue of received packets.
 *
 * @param csap          CSAP
 * @param wait          Block until packet is available or receive is
 *                      finished
 * @param pkt           Location for received packet pointer
 *
 * @return Status code.
 */
static te_errno
tad_recv_get_packet(csap_p csap, te_bool wait, tad_recv_pkt **pkt)
{
    tad_recv_context   *ctx = csap_get_recv_context(csap);
    te_errno            rc = 0;
    int                 ret;

    CSAP_LOCK(csap);
    while (((*pkt = ctx->packets.tqh_first) == NULL) && wait &&
           (~csap->state & CSAP_STATE_DONE))
    {
        ret = pthread_cond_wait(&csap->event, &csap->lock);
        if (ret != 0)
        {
            rc = TE_OS_RC(TE_TAD_CH, ret);
            assert(TE_RC_GET_ERROR(rc) != TE_ENOENT);
            ERROR("%s(): pthread_cond_wait() failed: %r",
                  __FUNCTION__, rc);
        }
    }
    if (*pkt != NULL)
    {
        TAILQ_REMOVE(&ctx->packets, *pkt, links);
    }
    else if (rc == 0)
    {
        rc = TE_RC(TE_TAD_CH, TE_ENOENT);
    }
    CSAP_UNLOCK(csap);

    return rc;
}

/**
 * Get matched packet from TAD receiver packets queue.
 *
 * @param csap      CSAP structure
 * @param task      TAD task context for reporting
 * @param wait      Wait for more packets or end of processing
 * @param got       Location for number of got packets
 *
 * @return Status code.
 */
static te_errno
tad_recv_get_packets(csap_p csap, tad_task_context *task, te_bool wait,
                     unsigned int *got)
{
    te_errno        rc;
    tad_recv_pkt   *pkt = NULL;
    asn_value      *pdus;
    asn_value      *pdu;
    unsigned int    layer;
    uint8_t        *payload = NULL;
    size_t          payload_len = 0;

    ENTRY(CSAP_LOG_FMT "wait=%u got=%p(%u)", CSAP_LOG_ARGS(csap),
          (unsigned)wait, got, (got == NULL) ? 0 : (int)*got);

    while ((rc = tad_recv_get_packet(csap, wait, &pkt)) == 0)
    {
        (*got)++;

        /* Process packet */
        pkt->nds = asn_init_value(ndn_raw_packet);
        asn_write_int32(pkt->nds, pkt->ts.tv_sec,
                        "received.seconds");
        asn_write_int32(pkt->nds, pkt->ts.tv_usec,
                        "received.micro-seconds");
        pdus = asn_init_value(ndn_generic_pdu_sequence);
        if (asn_put_child_value(pkt->nds, pdus, PRIVATE, NDN_PKT_PDUS) != 0)
            ERROR("ERROR: %s:%u", __FILE__, __LINE__);
        for (layer = 0; layer < csap->depth; ++layer)
        {
            if (csap_get_proto_support(csap, layer)->match_post_cb != NULL)
            {
                rc = csap_get_proto_support(csap, layer)->match_post_cb(
                         csap, layer, pkt->layers + layer);
                if (rc != 0)
                    ERROR("post_cb: %r", rc);
            }

            pdu = asn_init_value(ndn_generic_pdu);
            if (asn_put_child_value(pdu, pkt->layers[layer].nds, PRIVATE,
                                    csap->layers[layer].proto_tag) != 0)
                ERROR("ERROR: %s:%u", __FILE__, __LINE__);
            if (asn_insert_indexed(pdus, pdu, layer, "") != 0)
                ERROR("ERROR: %s:%u", __FILE__, __LINE__);
        }

        rc = tad_pkt_flatten_copy(&pkt->payload, &payload, &payload_len);
        if (rc != 0)
        {
            ERROR(CSAP_LOG_FMT "Failed to make flatten copy of payload: "
                  "%r", CSAP_LOG_ARGS(csap), rc);
            /* TODO: Is it better to continue or to report an error? */
        }
        else
        {
            rc = asn_write_value_field(pkt->nds, payload, payload_len, 
                                       "payload.#bytes");
            if (rc != 0)
            {
                ERROR("ASN error in add rest payload %r", rc);
            }
            free(payload); /* FIXME: Avoid it */
            payload = NULL;
            payload_len = 0;
        }

        rc = tad_recv_report_packet(pkt->nds, task->rcfc,
                                    task->answer_buf, task->prefix_len);
        if (rc != 0)
        {
            /* TODO: Error processing here */
        }

        tad_recv_pkt_free(csap, pkt);
    }

    VERB(CSAP_LOG_FMT "%s() status before correction is %r",
         CSAP_LOG_ARGS(csap), __FUNCTION__, rc);

    if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
    {
        rc = 0;
    }

    EXIT(CSAP_LOG_FMT "%r", CSAP_LOG_ARGS(csap), rc);

    return rc;
}

/**
 * Execute traffic receive get/wait/stop or CSAP destroy operation.
 *
 * @param csap          CSAP
 * @param op_context    Operation context
 */
static void
tad_recv_op(csap_p csap, tad_recv_op_context *op_context)
{
    tad_recv_context   *recv_context;
    te_errno            rc;
    unsigned int        got;


    assert(csap != NULL);
    assert(op_context != NULL);

    ENTRY(CSAP_LOG_FMT "op=%u", CSAP_LOG_ARGS(csap), op_context->op);

    recv_context = csap_get_recv_context(csap);
    assert(recv_context != NULL);

    if (csap->state & CSAP_STATE_RESULTS)
    {
        got = 0;
        rc = tad_recv_get_packets(csap, &op_context->task,
                                  op_context->op != TAD_OP_GET,
                                  &got);
    }
    else
    {
        rc = 0; /* Just initialize */

        if ((op_context->op != TAD_OP_GET) &&
            (~csap->state & CSAP_STATE_DONE))
        {
            rc = csap_wait(csap, CSAP_STATE_DONE);
        }

        /* 
         * Nobody can modify got_pkts at the time and match_pkts can
         * grow only, so do calculations without lock.
         */
        got = recv_context->match_pkts - recv_context->got_pkts;
    }

    /* 
     * Nobody can modify got_pkts at the time and match_pkts can
     * grow only, so do increment and assert without lock.
     */
    recv_context->got_pkts += got;
    assert(recv_context->got_pkts <= recv_context->match_pkts);

    if ((op_context->op != TAD_OP_GET) && (rc == 0))
    {
        /* It is not a get request and all go smoothly */

        /* Received packets queue has to be empty */
        assert(recv_context->packets.tqh_first == NULL);

        (void)csap_command(csap, TAD_OP_IDLE);

        /* 
         * In the case of wait/stop requests total number of matched
         * packets should be reported.
         */
        got = recv_context->got_pkts;

        /*
         * Return status of the Receiver.
         */
        rc = recv_context->status;
        if ((TE_RC_GET_ERROR(rc) == TE_EINTR) &&
            (op_context->op == TAD_OP_STOP))
        {
            rc = 0;
        }
    }

    INFO(CSAP_LOG_FMT "Traffic receive op %u finished: rc=%r, got=%u",
         CSAP_LOG_ARGS(csap), op_context->op, rc, got);

    /* 
     * We have no more chance to report and error (logged of course),
     * just ignore it.
     */
    (void)tad_task_reply(&op_context->task, "%u %u", rc, got);

    EXIT();
}


/**
 * Free traffic receive stop/wait/get operation context.
 *
 * @parma context       Context pointer
 */
static void
tad_recv_op_free(tad_recv_op_context *context)
{
    tad_task_free(&context->task);
    free(context);
}


/**
 * Start routine for stop/wait/get receive operation.
 * It forwards received packets to test.
 *
 * @param arg           Start argument, should be pointer to 
 *                      CSAP structure
 *
 * @return NULL 
 */
static void *
tad_recv_op_thread(void *arg)
{
    csap_p                  csap = arg;
    tad_recv_op_context    *context;


    assert(csap != NULL);
    F_ENTRY(CSAP_LOG_FMT, CSAP_LOG_ARGS(csap));

    CSAP_LOCK(csap);

    while ((context = csap->recv_ops.tqh_first) != NULL)
    {
        CSAP_UNLOCK(csap);

        tad_recv_op(csap, context);

        CSAP_LOCK(csap);

        assert(context == csap->recv_ops.tqh_first);
        TAILQ_REMOVE(&csap->recv_ops, context, links);
        tad_recv_op_free(context);
    }

    /* 
     * Log exit under CSAP lock, since CSAP can be destroyed just after
     * unlocking.
     */
    F_EXIT(CSAP_LOG_FMT, CSAP_LOG_ARGS(csap));

    CSAP_UNLOCK(csap);

    return NULL;
}


/* See description in tad_recv.h */
te_errno
tad_recv_op_enqueue(csap_p csap, tad_traffic_op_t op,
                    rcf_comm_connection *rcfc,
                    const char *answer_pfx, size_t pfx_len)
{
    tad_recv_op_context    *context;
    int                     ret;
    te_errno                rc;
    te_bool                 start_thread;

    context = malloc(sizeof(*context));
    if (context == NULL)
        return TE_RC(TE_TAD_CH, TE_ENOMEM);

    rc = tad_task_init(&context->task, rcfc, answer_pfx, pfx_len);
    if (rc != 0)
    {
        free(context);
        return rc;
    }

    context->op = op;


    CSAP_LOCK(csap);

    start_thread = (csap->recv_ops.tqh_first == NULL);

    TAILQ_INSERT_TAIL(&csap->recv_ops, context, links);

    if (start_thread)
    {
        ret = tad_pthread_create(NULL, tad_recv_op_thread, csap);
        if (ret != 0)
        {
            rc = te_rc_os2te(ret);
            TAILQ_REMOVE(&csap->recv_ops, context, links);
        }
    }

    /* 
     * Do not unlock CSAP before send of ACK, since unlocking allows
     * thread to process request and it can be finished very fast
     * (final reply is sent and task context freed).
     */

    if (rc == 0)
    {
        /*
         * Processing of traffic receive get/wait/stop/destroy has
         * enqueued, sent TE proto ACK.
         */
        rc = tad_task_reply(&context->task,
                            "%u", TE_RC(TE_TAD_CH, TE_EACK));
        CSAP_UNLOCK(csap);
        if (rc != 0)
        {
            /* 
             * In general, nothing can help, error has already been
             * logged. Operation is enqueue and it will try to send
             * final answer at the end of processing, so do not forward
             * an error to caller.
             */
            rc = 0;
        }
    }
    else
    {
        CSAP_UNLOCK(csap);
        tad_recv_op_free(context);
    }

    return rc;
}
