/** @file
 * @brief TAD Sender/Receiver
 *
 * Traffic Application Domain Command Handler.
 * Declarations of types and functions common for TAD Sender and Receiver.
 *
 * Copyright (C) 2005-2006 Test Environment authors (see file AUTHORS
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
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TAD_SEND_RECV_H__
#define __TE_TAD_SEND_RECV_H__ 

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "te_defs.h"
#include "asn_usr.h" 
#include "comm_agent.h"
#include "logger_api.h"
#include "rcf_ch_api.h"
#include "tad_csap_inst.h"


/* TODO: this constant should be placed to more appropriate header! */
/**
 * Maximum length of the test protocol answer to be sent by TAD.
 */
#define TAD_ANSWER_LEN  0x100


#ifdef __cplusplus
extern "C" {
#endif


/** Traffic operation thread special data. */
typedef struct tad_task_context {

    rcf_comm_connection    *rcfc;       /**< RCF connection to answer */

    char    answer_buf[TAD_ANSWER_LEN]; /**< Prefix for test-protocol
                                             answer to the current
                                             command */
    size_t  prefix_len;                 /**< Length of the Test Protocol
                                             answer prefix */
} tad_task_context;

/* See description in tad_send_recv.h */
static inline te_errno
tad_task_init(tad_task_context *task, rcf_comm_connection *rcfc,
              const char *answer_pfx, size_t pfx_len)
{
    if (pfx_len >= sizeof(task->answer_buf))
    {
        ERROR("Too small buffer for Test Protocol command answer in TAD "
              "task structure");
        return TE_RC(TE_TAD_CH, TE_ESMALLBUF);
    }

    task->rcfc = rcfc;
    task->prefix_len = pfx_len;
    memcpy(task->answer_buf, answer_pfx, pfx_len);

    return 0;
}

static inline te_errno
tad_task_reply(tad_task_context *task, const char *fmt, ...)
{
    te_errno    rc;
    va_list     ap;
    int         buf_len = sizeof(task->answer_buf) - task->prefix_len;

    va_start(ap, fmt);
    if (vsnprintf(task->answer_buf + task->prefix_len, buf_len,
                  fmt, ap) >= buf_len)
    {
        ERROR("TE protocol answer is truncated");
        /* Try to continue */
    }
    INFO("Sending reply: '%s'", task->answer_buf);
    rcf_ch_lock();
    rc = rcf_comm_agent_reply(task->rcfc, task->answer_buf,
                              strlen(task->answer_buf) + 1);
    rcf_ch_unlock();

    return rc;
}

/**
 * Clean up TAD task parameters (to make sure that no more answers
 * are send in this task).
 *
 * @param task          TAD task structure
 */
static inline void
tad_task_free(tad_task_context *task)
{
    task->rcfc = NULL;
    task->prefix_len = 0;
    memset(task->answer_buf, 0, sizeof(task->answer_buf));
}


/**
 * Generate Traffic Pattern NDS by template for send/receive command.
 *
 * @param csap          Structure with CSAP parameters
 * @param template      Traffic template
 * @param pattern       Generated Traffic Pattern
 *
 * @return Status code.
 */
extern te_errno tad_send_recv_generate_pattern(csap_p       csap,
                                               asn_value_p  template, 
                                               asn_value_p *pattern);

/**
 * Confirm traffic template or pattern PDUS set with CSAP settings and 
 * protocol defaults. 
 * This function changes passed ASN value, user have to ensure that changes
 * will be set in traffic template or pattern ASN value which will be used 
 * in next operation. This may be done by such ways:
 *
 * Pass pointer got by asn_get_subvalue method, or write modified value 
 * into original NDS. 
 *
 * @param csap    CSAP descriptor.
 * @param recv          Is receive flow or send flow.
 * @param pdus          ASN value with SEQUENCE OF Generic-PDU (IN/OUT).
 * @param layer_opaque  Array for per-layer opaque data pointers
 *
 * @return zero on success, otherwise error code.
 */
extern int tad_confirm_pdus(csap_p csap, te_bool recv,
                            asn_value *pdus, void **layer_opaque);


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /*  __TE_TAD_SEND_RECV_H__ */
