/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAD CSAP Instance
 *
 * Traffic Application Domain Command Handler.
 * Implementation of CSAP instance methods.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "TAD CSAP instance"

#include "te_config.h"

#include <stdlib.h>
#include <string.h>

#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta_fast.h"

#include "csap_id.h"
#include "tad_csap_support.h"
#include "tad_csap_inst.h"
#include "tad_utils.h"


/** Default CSAP stop latency timeout in milliseconds. */
#define TAD_CSAP_STOP_LATENCY_TIMEOUT_DEF   100
/** Default CSAP receive timeout in milliseconds. */
#define TAD_CSAP_RECV_TIMEOUT_DEF           1000

/** Max number of CSAP layers */
#define MAX_CSAP_DEPTH 200


/**
 * Free memory allocatad for all common CSAP data.
 *
 * @param csap      Pointer to CSAP descriptor
 */
static void
csap_free(csap_p csap)
{
    if (csap == NULL)
        return;

    VERB("%s(): csap %d, layers %u", __FUNCTION__,
         csap->id, csap->layers);

    free(csap->csap_type);

    if (csap->layers != NULL)
    {
        unsigned int i;

        for (i = 0; i < csap->depth; i++)
        {
            /*
             * Nothing to be done per layer:
             *  - NDS is freed as whole;
             *  - opaque data have to be freed by destroy callbacks.
             */
        }

        free(csap->layers);
    }

    free(csap);
}


/* See description in tad_csap_inst.h */
te_errno
csap_create(const char *type, csap_p *csap)
{
    csap_p          new_csap;
    char           *csap_type;
    unsigned int    depth;
    unsigned int    i;
    te_errno        rc;
    int             ret;
    char           *proto;
    char           *layer_protos[MAX_CSAP_DEPTH];


    ENTRY("%s", type);

    if (type == NULL || csap == NULL)
        return TE_RC(TE_TAD_CH, TE_EFAULT);

    new_csap = calloc(1, sizeof(*new_csap));
    if (new_csap != NULL)
    {
        TAILQ_INIT(&new_csap->recv_ops);
        LIST_INIT(&new_csap->poll_ops);
        tad_recv_init_context(&new_csap->receiver);
        new_csap->csap_type = csap_type = strdup(type);
        new_csap->stop_latency_timeout =
            TE_MS2US(TAD_CSAP_STOP_LATENCY_TIMEOUT_DEF);
        new_csap->recv_timeout =
            TE_MS2US(TAD_CSAP_RECV_TIMEOUT_DEF);
    }
    else
    {
        csap_type = NULL;
    }

/**
 * Macro for failure processing in csap_create function.
 *
 * @param errno_        error status code
 * @param fmt_          format string with possible firther arguments
 *                      for error log message
 */
#define CSAP_CREATE_ERROR(errno_, fmt_...) \
    do {               \
        rc = (errno_); \
        ERROR(fmt_);   \
        goto error;    \
    } while (0)

    if ((new_csap == NULL) || (csap_type == NULL))
        CSAP_CREATE_ERROR(TE_ENOMEM, "%s(): no memory for new CSAP",
                          __FUNCTION__);

    new_csap->id = csap_id_new(new_csap);
    if (new_csap->id == CSAP_INVALID_HANDLE)
        CSAP_CREATE_ERROR(TE_ENOMEM, "Failed to allocate a new CSAP ID");
    VERB("%s(): new id: %u", __FUNCTION__, new_csap->id);

    if ((ret = pthread_cond_init(&new_csap->event, NULL)) != 0)
        CSAP_CREATE_ERROR(te_rc_os2te(ret),
                          "%s(): pthread_cond_init() failed: %d",
                          __FUNCTION__, ret);
    if ((ret = pthread_mutex_init(&new_csap->lock, NULL)) != 0)
        CSAP_CREATE_ERROR(te_rc_os2te(ret),
                          "%s(): pthread_mutex_init() failed: %d",
                          __FUNCTION__, ret);

    depth = 0;
    layer_protos[depth] = csap_type;
    depth++;

    for (proto = strchr(csap_type, '.');
         proto != NULL && *proto != '\0';
         proto = strchr(proto, '.'))
    {
        *proto = '\0';
        proto++;
        layer_protos[depth] = proto;
        depth++;
        VERB("%s(): next_layer: %s\n", __FUNCTION__, proto);
    }

    new_csap->depth = depth;

    /* Allocate memory for stack arrays */
    new_csap->layers = calloc(depth, sizeof(new_csap->layers[0]));

    if (new_csap->layers == NULL)
        CSAP_CREATE_ERROR(TE_ENOMEM, "%s(): no memory for layers",
                          __FUNCTION__);

    for (i = 0; i < depth; i++)
    {
        new_csap->layers[i].proto = layer_protos[i];

        new_csap->layers[i].proto_tag = te_proto_from_str(layer_protos[i]);
        if (new_csap->layers[i].proto_tag == TE_PROTO_INVALID)
            CSAP_CREATE_ERROR(TE_EINVAL, "Failed to convert protocol "
                              "'%s' to tag", layer_protos[i]);

        new_csap->layers[i].proto_support =
            csap_spt_find(new_csap->layers[i].proto);

        VERB("%s(): layer %d: %s\n", __FUNCTION__,
             i, new_csap->layers[i].proto);

        if (new_csap->layers[i].proto_support == NULL)
            CSAP_CREATE_ERROR(TE_EOPNOTSUPP,
                              "%s(): no support for proto '%s'",
                              __FUNCTION__, new_csap->layers[i].proto);
    }

    /* Ready for processing */
    rc = csap_command(new_csap, TAD_OP_IDLE);
    if (rc != 0)
        CSAP_CREATE_ERROR(rc, "%s(): csap_command(IDLE) failed: %r",
                          __FUNCTION__, rc);

#undef CSAP_CREATE_ERROR

    /* Initialize CSAP reference count */
    new_csap->ref = 1;

    *csap = new_csap;

    EXIT("ID=%u", new_csap->id);

    return 0;

error:
    if (new_csap != NULL && new_csap->id != CSAP_INVALID_HANDLE)
        csap_id_delete(new_csap->id);
    csap_free(new_csap);

    EXIT("ERROR %r", rc);
    return rc;
}

/* See description in tad_csap_inst.h */
te_errno
csap_destroy(csap_handle_t csap_id)
{
    csap_p  csap = csap_id_delete(csap_id);

    VERB("%s(): CSAP ID %u -> %p", __FUNCTION__, csap_id, csap);

    if (csap == NULL)
        return TE_RC(TE_TAD_CH, TE_ENOENT);

    assert(csap->ref == 1);

    asn_free_value(csap->nds);
    csap_free(csap);

    return 0;
}

/* See description in tad_csap_inst.h */
csap_p
csap_find(csap_handle_t csap_id)
{
    return csap_id_get(csap_id);
}


/* See description in tad_csap_inst.h */
te_errno
csap_command_under_lock(csap_p csap, tad_traffic_op_t command)
{
    te_errno    rc = 0;
    int         ret;

    assert(pthread_mutex_trylock(&csap->lock) == EBUSY);

    /* At first, check current state agains this command */
    switch (command)
    {
        case TAD_OP_IDLE:
            /* Idle is internal command and allowed in any state */
            break;

        case TAD_OP_SEND_DONE:
            /* Internal command, have to be called in right way only */
            assert(csap->state & CSAP_STATE_SEND);
            assert(~csap->state & CSAP_STATE_RECV_DONE);
            assert(~csap->state & CSAP_STATE_DONE);
            break;

        case TAD_OP_RECV_DONE:
            /* Internal command, have to be called in right way only */
            assert(csap->state & CSAP_STATE_RECV);
            assert(~csap->state & CSAP_STATE_DONE);
            break;

        case TAD_OP_SEND:
        case TAD_OP_SEND_RECV:
        case TAD_OP_RECV:
            if (csap->state & CSAP_STATE_DESTROY)
            {
                ERROR(CSAP_LOG_FMT "Not exist (destroying)",
                      CSAP_LOG_ARGS(csap));
                rc = TE_ETADCSAPNOTEX;
            }
            else if (~csap->state & CSAP_STATE_IDLE)
            {
                ERROR(CSAP_LOG_FMT "Busy", CSAP_LOG_ARGS(csap));
                rc = TE_ETADCSAPSTATE;
            }
            break;

        case TAD_OP_GET:
        case TAD_OP_WAIT:
            if (csap->state & CSAP_STATE_DESTROY)
            {
                ERROR(CSAP_LOG_FMT "Not exist (destroying)",
                      CSAP_LOG_ARGS(csap));
                rc = TE_ETADCSAPNOTEX;
            }
            else if (~csap->state & CSAP_STATE_RECV)
            {
                ERROR(CSAP_LOG_FMT "Not receiving", CSAP_LOG_ARGS(csap));
                rc = TE_ETADCSAPSTATE;
            }
            else if (csap->state & (CSAP_STATE_WAIT | CSAP_STATE_STOP))
            {
                if (csap->state & CSAP_STATE_STOP)
                {
                    ERROR(CSAP_LOG_FMT "Stop operation is already in "
                          "progress", CSAP_LOG_ARGS(csap));
                }
                else
                {
                    ERROR(CSAP_LOG_FMT "Waiting for end of processing "
                          "is already in progress", CSAP_LOG_ARGS(csap));
                }
                rc = TE_EINPROGRESS;
            }
            break;

        case TAD_OP_STOP:
            if (csap->state & CSAP_STATE_DESTROY)
            {
                ERROR(CSAP_LOG_FMT "Not exist (destroying)",
                      CSAP_LOG_ARGS(csap));
                rc = TE_ETADCSAPNOTEX;
            }
            else if ((csap->state & (CSAP_STATE_SEND |
                                     CSAP_STATE_RECV)) == 0)
            {
                ERROR(CSAP_LOG_FMT "Stop neither sending nor receiving",
                      CSAP_LOG_ARGS(csap));
                rc = TE_ETADCSAPSTATE;
            }
            else if (csap->state & CSAP_STATE_STOP)
            {
                ERROR(CSAP_LOG_FMT "Stop operation is already in progress",
                      CSAP_LOG_ARGS(csap));
                rc = TE_EINPROGRESS;
            }
            break;

        case TAD_OP_DESTROY:
            if (csap->state & CSAP_STATE_DESTROY)
            {
                ERROR(CSAP_LOG_FMT "Not exist (destroying)",
                      CSAP_LOG_ARGS(csap));
                rc = TE_ETADCSAPNOTEX;
            }
            break;

        default:
            assert(FALSE);
            rc = TE_EINVAL;
    }

    if (rc == 0)
    {
        /* We can change the state */
        switch (command)
        {
            case TAD_OP_IDLE:
                if (csap->state & CSAP_STATE_DONE)
                    csap->state |= CSAP_STATE_IDLE;
                else
                    csap->state = CSAP_STATE_IDLE;
                break;

            case TAD_OP_SEND_DONE:
                csap->state |= CSAP_STATE_SEND_DONE;
                if (~csap->state & CSAP_STATE_RECV)
                {
                    csap->state |= CSAP_STATE_DONE;
                    if (csap->state & CSAP_STATE_FOREGROUND)
                        csap->state |= CSAP_STATE_IDLE;
                }
                break;

            case TAD_OP_RECV_DONE:
                csap->state |= CSAP_STATE_RECV_DONE;
                if ((~csap->state & CSAP_STATE_SEND) |
                    (csap->state & CSAP_STATE_SEND_DONE))
                {
                    csap->state |= CSAP_STATE_DONE;
                }
                break;

            case TAD_OP_SEND:
                csap->state = CSAP_STATE_SEND;
                break;

            case TAD_OP_SEND_RECV:
                csap->state = CSAP_STATE_SEND | CSAP_STATE_RECV |
                              CSAP_STATE_FOREGROUND;
                break;

            case TAD_OP_RECV:
                csap->state = CSAP_STATE_RECV;
                break;

            case TAD_OP_GET:
                /* Nothing to do */
                break;

            case TAD_OP_WAIT:
                csap->state |= CSAP_STATE_WAIT;
                break;

            case TAD_OP_STOP:
                csap->state |= CSAP_STATE_STOP;
                break;

            case TAD_OP_DESTROY:
                csap->state |= CSAP_STATE_DESTROY;
                if ((csap->state & (CSAP_STATE_SEND | CSAP_STATE_RECV)) &&
                    (~csap->state & CSAP_STATE_DONE))
                {
                    csap->state |= CSAP_STATE_STOP;
                }
                break;

            default:
                assert(FALSE);
                rc = TE_EINVAL;
        }

        if ((ret = pthread_cond_broadcast(&csap->event)) != 0)
        {
            rc = TE_OS_RC(TE_TAD_CH, ret);
            assert(rc != 0);
            ERROR(CSAP_LOG_FMT "Failed to broadcast CSAP event: %r",
                  CSAP_LOG_ARGS(csap), rc);
        }
    }

    return TE_RC(TE_TAD_CH, rc);
}

/* See description in tad_csap_inst.h */
te_errno
csap_timedwait(csap_p csap, unsigned int state_bits, unsigned int ms)
{
    struct timeval      now;
    struct timespec     timeout;
    te_errno            rc = 0;
    int                 ret;

    CSAP_LOCK(csap);
    gettimeofday(&now, NULL);
    timeout.tv_sec = now.tv_sec + TE_MS2SEC(ms);
    timeout.tv_nsec = now.tv_usec * 1000 + TE_MS2NS(ms % 1000);
    if (timeout.tv_nsec >= TE_SEC2NS(1))
    {
        timeout.tv_sec++;
        timeout.tv_nsec -= TE_SEC2NS(1);
        assert(timeout.tv_nsec < TE_SEC2NS(1));
    }
    while (~csap->state & state_bits)
    {
        ret = pthread_cond_timedwait(&csap->event, &csap->lock, &timeout);
        if (ret == ETIMEDOUT)
        {
            rc = TE_RC(TE_TAD_CH, TE_ETIMEDOUT);
            break;
        }
        if (ret != 0)
        {
            rc = TE_OS_RC(TE_TAD_CH, ret);
            assert(TE_RC_GET_ERROR(rc) != TE_ENOENT);
            ERROR("%s(): pthread_cond_wait() failed: %r",
                  __FUNCTION__, rc);
            break;
        }
    }
    CSAP_UNLOCK(csap);

    return rc;
}
