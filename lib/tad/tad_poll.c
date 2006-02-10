/** @file
 * @brief TAD Poll Support
 *
 * Traffic Application Domain Command Handler.
 * Implementation of TAD poll support.
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id: tad_recv.h 23847 2006-02-09 06:45:22Z arybchik $
 */

#include "te_config.h"

#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_PTHREAD_H
#include <pthread.h>
#else
#error pthread.h is required for TAD poll support
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif
#if HAVE_ASSERT_H
#include <assert.h>
#endif

#include "te_errno.h"
#include "comm_agent.h"
#include "logger_api.h"

#include "tad_types.h"
#include "tad_csap_inst.h"
#include "tad_send_recv.h"
#include "tad_utils.h"
#include "tad_poll.h"


/* See the description in tad_poll.h */
void
tad_poll_free(tad_poll_context *context)
{
    /* Ignore errors, nothing can help */
    (void)tad_task_reply(&context->task, "%u %u",
                         (unsigned)context->status, context->id);
    tad_task_free(&context->task);

    CSAP_LOCK(context->csap);
    LIST_REMOVE(context, links);
    CSAP_UNLOCK(context->csap);

    free(context);
}

/* See the description in tad_poll.h */
te_errno
tad_poll_enqueue(csap_p csap, unsigned int timeout,
                 rcf_comm_connection *rcfc,
                 const char *answer_pfx, size_t pfx_len)
{
    tad_poll_context   *context;
    te_errno            rc;

    context = malloc(sizeof(*context));
    if (context == NULL)
        return TE_RC(TE_TAD_CH, TE_ENOMEM);

    rc = tad_task_init(&context->task, rcfc, answer_pfx, pfx_len);
    if (rc != 0)
    {
        free(context);
        return rc;
    }

    context->csap = csap;
    context->timeout = timeout;
    /* Status to be returned on cancellation */
    context->status = TE_RC(TE_TAD_CH, TE_ECANCELED);

    CSAP_LOCK(csap);

    csap->poll_id++;
    /* TODO: Check that it is not 0 and for uniqueness */
    context->id = csap->poll_id;

    LIST_INSERT_HEAD(&csap->poll_ops, context, links);

    rc = tad_pthread_create(&context->thread, tad_poll_thread, context);
    if (rc != 0)
    {
        free(context);
    }
    else
    {
        rc = tad_task_reply(&context->task, "0 %u", context->id);
        if (rc != 0)
        {
            /* Failed to send ID to the test, request can't be cancelled */
            if (pthread_cancel(context->thread) != 0)
            {
                ERROR("%s(): pthread_cancel() failed for just created "
                      "thread", __FUNCTION__);
                /* Do not update rc */
            }
        }
    }

    CSAP_UNLOCK(csap);

    return rc;
}

/* See the description in tad_poll.h */
void *
tad_poll_thread(void *arg)
{
    tad_poll_context   *context = arg;
    csap_p              csap;
    te_errno            rc;

    assert(context != NULL);
    csap = context->csap;
    assert(csap != NULL);

    pthread_cleanup_push((void (*)(void *))tad_poll_free, (void *)context);
    rc = csap_timedwait(csap, CSAP_STATE_DONE, context->timeout);
    if (rc == 0)
    {
        if (csap->state & CSAP_STATE_RECV)
        {
            context->status = csap_get_recv_context(csap)->status;
        }
        else if (csap->state & CSAP_STATE_SEND)
        {
            context->status = csap_get_send_context(csap)->status;
        }
        else
        {
            context->status = TE_RC(TE_TAD_CH, TE_ETADCSAPSTATE);
            assert(FALSE);
        }
    }
    else
    {
        context->status = rc;
    }
    pthread_cleanup_pop(!0);

    return NULL;
}
