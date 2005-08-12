/** @file
 * @brief Test Environment
 *
 * RCF API library implementation
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * 
 * $Id$
 */

#include "te_config.h"
#include "config.h"

#include <stdio.h>
#include <stdarg.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_printf.h"
#include "rcf_api.h"
#include "rcf_internal.h"
#define RCF_NEED_TYPE_LEN 1
#include "te_proto.h"
#include "ipc_client.h"

#define TE_LGR_USER     "RCF API"
#include "logger_api.h"

#include "rpc_xdr.h"

/** Busy CSAPs state */
#define CSAP_SEND       1
#define CSAP_RECV       2
#define CSAP_SENDRECV   3

/** Number of symbols for of int32_t + spaces */
#define RCF_MAX_INT     12


/* CSAP structure for checking is the CSAP busy or not */
typedef struct traffic_op {
    struct traffic_op *next;    /**< Next CSAP in the list */
    struct traffic_op *prev;    /**< Previous CSAP in the list */

    char            ta[RCF_MAX_NAME];   /**< Test Agent name */
    csap_handle_t   csap_id;    /**< CSAP handle returned by the TA */
    int             num_users;  /**< Number of pending requests for CSAP */
    int             state;      /**< CSAP_SEND, CSAP_RECV or
                                     CSAP_SENDRECV */
    int             sid;        /**< Session identifier */
    rcf_pkt_handler handler;    /**< handler for received packet */
    void           *user_param; /**< handler parameter */
} traffic_op_t;

typedef struct msg_buf_entry {
    CIRCLEQ_ENTRY(msg_buf_entry) link;
    rcf_msg                     *message;
} msg_buf_entry_t;

typedef CIRCLEQ_HEAD(msg_buf_head, msg_buf_entry) msg_buf_head_t;

typedef struct thread_ctx {
    struct ipc_client *ipc_handle;
    msg_buf_head_t     msg_buf_head;
    uint32_t           seqno;
} thread_ctx_t;


/* Busy CSAPs list anchor */
static traffic_op_t traffic_ops = { &traffic_ops, &traffic_ops, 
                                    "", 0, 0, 0, 0, NULL, NULL };

/* Forward declaration */
static int csap_tr_recv_get(const char *ta_name, int session, 
                            csap_handle_t csap_id, 
                            int *num, int opcode);

/* If pthread mutexes are supported - OK; otherwise hope for best... */
#ifdef HAVE_PTHREAD_H
#ifndef PTHREAD_SETSPECIFIC_BUG
static pthread_once_t   once_control = PTHREAD_ONCE_INIT;
static pthread_key_t    key;
#endif
static pthread_mutex_t  rcf_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/* Following two macros are necessary to provide tread safety */

#ifndef HAVE_PTHREAD_H
#define pthread_self()  0
#endif

/* Declare and initialize (or obtain old) IPC library handle. */
#define RCF_API_INIT \
    thread_ctx_t *ctx_handle = get_ctx_handle(TRUE);            \
    struct ipc_client *ipc_handle;                              \
                                                                \
    if (ctx_handle == NULL || ctx_handle->ipc_handle == NULL)   \
        return TE_RC(TE_RCF_API, TE_EIPC);                      \
    ipc_handle = ctx_handle->ipc_handle


/* Validate TA name argument */
#define BAD_TA (ta_name == NULL || strlen(ta_name) + 1 > RCF_MAX_NAME)


/**
 * Validate type of the variable or function parameter.
 *
 * @param var_type      variable/parameter type
 * @param var_len       variable/parameter size
 *
 * @return 0 (success) or 1 (type or length are bad)
 */
static int
validate_type(int var_type, int var_len)
{                                         
    return var_type == RCF_STRING ? 0 :
           ((var_type < RCF_INT8 || var_type > RCF_UINT64 || 
             rcf_type_len[var_type] != var_len) ? 1 : 0);
}


/**
 * Match RCF IPC message with opcode and session id. 
 *
 * @param msg      pointer to RCF message
 * @param opcode   RCF operational code
 * @param sid      session id
 *
 * @retval 0    message matches
 * @retval 1    message NOT matches
 */
static int
rcf_message_match(rcf_msg *msg, rcf_op_t opcode, int sid)
{
    if (msg == NULL) 
        return 1;

    if (opcode != msg->opcode)
        return 1;

    switch (opcode)
    {
        case RCFOP_TALIST:
        case RCFOP_TACHECK:
        case RCFOP_TATYPE:
        case RCFOP_SESSION:
        case RCFOP_REBOOT: 
            return 0;

            /* 
             * all other messages which may occure in RCF API has SID
             * and should be matched by SID 
             */
        default:
            if (msg->sid != sid)
                return 1;
            else 
                return 0;
    }
}
/**
 * Clear RCF message buffer
 *
 * @param buf_head      head of buffer list 
 *
 * @return zero on success or error code
 */
static int
msg_buffer_clear(msg_buf_head_t *buf_head)
{
    msg_buf_entry_t *entry;

    if (buf_head == NULL)
        return 0;

    while ((entry = buf_head->cqh_first) != (void *)buf_head)
    {
        CIRCLEQ_REMOVE(buf_head, entry, link);
        free(entry->message);
        free(entry);
    }
    return 0;
}

/**
 * Add RCF message to the buffer
 *
 * @param buf_head      head of buffer list 
 * @param message       message to be stored
 *
 * @return zero on success or error code
 */
static int
msg_buffer_insert(msg_buf_head_t *buf_head, rcf_msg *message)
{
    msg_buf_entry_t *buf_entry;
    size_t           msg_len;

    if (buf_head == NULL)
        return TE_EWRONGPTR;

    if (message == NULL)
        return 0; /* nothing to do */

    buf_entry = calloc(1, sizeof(*buf_entry));
    if (buf_entry == NULL)
        return TE_ENOMEM;

    msg_len = sizeof(rcf_msg) + message->data_len;
    if ((buf_entry->message = (rcf_msg *)calloc(1, msg_len)) == NULL)
        return TE_ENOMEM;

    memcpy(buf_entry->message, message, msg_len);

    CIRCLEQ_INSERT_TAIL(buf_head, buf_entry, link);

    return 0;
}

/**
 * Find message in RCF message buffer with desired SID, and remove 
 * it from buffer.
 *
 * @param msg_buf         RCF message buffer head
 * @param opcode          RCF operational code
 * @param session         SID which message must have
 *
 * @return pointer to found message or zero if not found
 * After ussage of message pointer should be freed. 
 */
rcf_msg *
msg_buffer_find(msg_buf_head_t *msg_buf, rcf_op_t opcode, int session)
{
    msg_buf_entry_t *buf_entry;

    if (msg_buf == NULL)
        return NULL; 

    for (buf_entry = msg_buf->cqh_first;
         buf_entry != (void *)msg_buf;
         buf_entry = buf_entry->link.cqe_next)
    { 
        if (rcf_message_match(buf_entry->message, opcode, session) == 0)
        {
            rcf_msg *msg = buf_entry->message;

            CIRCLEQ_REMOVE(msg_buf, buf_entry, link);
            free(buf_entry);
            return msg;
        }
    }

    return NULL;
}

/**
 * Receive complete message from IPC server.
 * If message is too long, the memory is allocated and its address
 * is placed to p_answer.
 *
 * @param ipcc            pointer to the ipc_client structure returned
 *                        by ipc_init_client()
 * @param recv_msg        pointer to the buffer for answer
 * @param recv_size       pointer to the variable to store:
 * @param p_answer        location for address of the memory
 *                        allocated for the answer or NULL
 *
 * @return zero on success or error code
 */
static int
rcf_ipc_receive_answer(struct ipc_client *ipcc, rcf_msg *recv_msg, 
                       size_t *recv_size, rcf_msg **p_answer)
{
    int     rc;
    size_t  buflen = *recv_size;
    size_t  len;
    
    if ((rc = ipc_receive_answer(ipcc, RCF_SERVER,
                                 recv_msg, recv_size)) == 0)
    {
        INFO("%s: got reply for %u:%d:'%s'", ipc_client_name(ipcc),
            (unsigned)recv_msg->seqno, recv_msg->sid,
            rcf_op_to_string(recv_msg->opcode));

        if (p_answer != NULL)
            *p_answer = NULL;
        return 0;
    }                                     
    else if (TE_RC_GET_ERROR(rc) != TE_ESMALLBUF)
    {
        return TE_RC(TE_RCF_API, TE_EIPC);
    }

    INFO("%s: got reply for %u:%d:'%s'", ipc_client_name(ipcc),
        (unsigned)recv_msg->seqno, recv_msg->sid,
        rcf_op_to_string(recv_msg->opcode));

    if (p_answer == NULL)
    {
        ERROR("Unexpected large message is received");
        return TE_RC(TE_RCF_API, TE_EIPC);
    }
    
    *p_answer = malloc(*recv_size);
    if (*p_answer == NULL)
        return TE_RC(TE_RCF_API, TE_ENOMEM);
    
    memcpy(*p_answer, (char *)recv_msg, buflen);
    len = *recv_size - buflen;
    if ((rc = ipc_receive_rest_answer(ipcc, RCF_SERVER, 
                                      ((uint8_t *)*p_answer) + buflen, 
                                      &len)) != 0)
    {
        return TE_RC(TE_RCF_API, TE_EIPC);
    }
    
    return 0;
}                       

/**
 * Wait for IPC RCF message with desired SID. 
 * If message is too long, the memory is allocated and its address
 * is placed to p_answer.
 *
 * @param ipcc            pointer to the ipc_client structure returned
 *                        by ipc_init_client()
 * @param rcf_msg_queue   message buffer head
 * @param opcode          RCF operational code
 * @param sid             session id
 * @param recv_msg        pointer to the buffer for answer
 * @param recv_size       pointer to the variable to store:
 * @param p_answer        location for address of the memory
 *                        allocated for the answer or NULL
 * 
 * @return zero on success or error code
 */
static int
wait_rcf_ipc_message(struct ipc_client *ipcc,
                     msg_buf_head_t *rcf_msg_queue,
                     rcf_op_t opcode, int session, 
                     rcf_msg *recv_msg, size_t *recv_size, 
                     rcf_msg **p_answer)
{
    rcf_msg *message;
    int      rc;

    if (ipcc == NULL || recv_msg == NULL || recv_size == NULL)
        return TE_EWRONGPTR;

    if ((message = msg_buffer_find(rcf_msg_queue, opcode, session)) != NULL)
    {
        size_t len = sizeof(*message) + message->data_len;
        
        VERB("Message found: TA %s, SID %d flags %x;"
             " waiting for SID %d", 
             message->ta, message->sid, message->flags, session);

        if (len > *recv_size)
        {
            if (p_answer == NULL)
            {
                ERROR("Unexpected large message is received");
                return TE_RC(TE_RCF_API, TE_EIPC);
            }
            *p_answer = message;
        }
        else
        {
            memcpy(recv_msg, message, len); 
            free(message);
        }
        
        *recv_size = len;

        return 0; 
    }

    while (TRUE)
    {
        if ((rc = rcf_ipc_receive_answer(ipcc, recv_msg, 
                                         recv_size, p_answer)) != 0)
            return TE_RC(TE_RCF_API, rc);
            
        message = (p_answer != NULL && *p_answer != NULL) ? *p_answer 
                                                          : recv_msg;

        VERB("Message cought: TA %s, SID %d flags %x;"
             " waiting for SID %d", 
             message->ta, message->sid, message->flags, session);

        if (rcf_message_match(message, opcode, session) == 0)
            break; 

        if (msg_buffer_insert(rcf_msg_queue, message) != 0)
            ERROR("RCF message is lost");
            
        if (message != recv_msg)
            free(message);
    }

    return 0;
}



/**
 * Send IPC RCF message and receive appropriate answer.
 * If message is too long, the memory is allocated and its address
 * is placed to p_answer.
 *
 * @param ctx             RCF client context
 * @param send_msg        pointer to the message to be sent
 * @param send_size       size of message to be sent
 * @param recv_msg        pointer to the buffer for answer
 * @param recv_size       pointer to the variable to store:
 *                          on entry - length of the available buffer;
 *                          on exit - length of the message received
 * @param p_answer        location for address of the memory
 *                        allocated for the answer or NULL
 * 
 * @return zero on success or error code
 */
static int
send_recv_rcf_ipc_message(thread_ctx_t *ctx,
                          rcf_msg *send_buf, size_t send_size,
                          rcf_msg *recv_buf, size_t *recv_size,
                          rcf_msg **p_answer)
{ 
    int      rc;
    int      sid;
    rcf_op_t opcode;
    rcf_msg *message;

    if (ctx == NULL || ctx->ipc_handle == NULL ||
        send_buf == NULL || recv_buf == NULL || recv_size == NULL)
        return TE_RC(TE_RCF_API, TE_EWRONGPTR);

    sid = send_buf->sid;
    opcode = send_buf->opcode;
    send_buf->seqno = ctx->seqno++;

    INFO("%s: send request %u:%d:'%s'", ipc_client_name(ctx->ipc_handle),
         (unsigned)send_buf->seqno, send_buf->sid,
         rcf_op_to_string(send_buf->opcode));

    if ((rc = ipc_send_message(ctx->ipc_handle, RCF_SERVER,
                               send_buf, send_size)) != 0)
    {
        ERROR("%s() failed with rc %r", __FUNCTION__, rc);
        return TE_RC(TE_RCF_API, TE_EIPC);
    }

    if ((rc = rcf_ipc_receive_answer(ctx->ipc_handle, recv_buf, 
                                     recv_size, p_answer)) != 0)
        return rc;

    message = (p_answer != NULL && *p_answer != NULL) ? *p_answer 
                                                      : recv_buf;
    if (rcf_message_match(recv_buf, opcode, sid) == 0)
        return 0;
        
    if (msg_buffer_insert(&ctx->msg_buf_head, message) != 0)
        ERROR("RCF message is lost");
    
    if (message != recv_buf)
        free(message);
            
    return wait_rcf_ipc_message(ctx->ipc_handle, &ctx->msg_buf_head,
                                opcode, sid, recv_buf, recv_size, p_answer);
}

#ifndef PTHREAD_SETSPECIFIC_BUG

#ifdef HAVE_PTHREAD_H
/**
 * Destructor of resources assosiated with thread.
 *
 * @handle      Handle assosiated with thread
 */
static void
rcf_api_thread_ctx_destroy(void *handle)
{
    thread_ctx_t *rcf_ctx_handle = (thread_ctx_t *)handle;

    if (handle == NULL)
        return;

    if (ipc_close_client(rcf_ctx_handle->ipc_handle) != 0)
    {
        ERROR("%s(): ipc_close_client() failed", __FUNCTION__);
        fprintf(stderr, "ipc_close_client() failed\n");
    } 

    msg_buffer_clear(&(rcf_ctx_handle->msg_buf_head));
    free(rcf_ctx_handle);
}

/**
 * Only once called function to create key.
 */
static void
rcf_api_key_create(void)
{
    atexit(rcf_api_cleanup);
    if (pthread_key_create(&key, rcf_api_thread_ctx_destroy) != 0)
    {
        ERROR("pthread_key_create() failed\n");
        fprintf(stderr, "pthread_key_create() failed\n");
    }
}
#endif /* HAVE_PTHREAD_H */

/**
 * Find thread context handle or initialize a new one.
 *
 * @param create    Create context client, if it is not exist or not
 *
 * @return context handle or NULL if some error occured
 */
thread_ctx_t *
get_ctx_handle(te_bool create)
{
#ifdef HAVE_PTHREAD_H
    thread_ctx_t          *handle;
#else
    static thread_ctx_t   *handle = NULL;
#endif

#ifdef HAVE_PTHREAD_H
    if (pthread_once(&once_control, rcf_api_key_create) != 0)
    {
        ERROR("pthread_once() failed\n");
        return NULL;
    }
    handle = (thread_ctx_t *)pthread_getspecific(key);
#endif
    if (handle == NULL && create)
    {
        int  rc;
        char name[RCF_MAX_NAME];

        handle = calloc(1, sizeof(*handle));
        if (handle == NULL)
        {
            ERROR("%s(): calloc(1, %u) failed",
                  __FUNCTION__, sizeof(*handle));
            return NULL;
        }

        sprintf(name, "rcf_client_%u_%u", (unsigned int)getpid(), 
                (unsigned int)pthread_self());

        rc = ipc_init_client(name, &(handle->ipc_handle));
        if (rc != 0)
        {
            ERROR("ipc_init_client() failed\n");
            fprintf(stderr, "ipc_init_client() failed\n");
            return NULL;
        }
        assert(handle->ipc_handle != NULL);

        CIRCLEQ_INIT(&handle->msg_buf_head);
#ifdef HAVE_PTHREAD_H
        if (pthread_setspecific(key, (void *)handle) != 0)
        {
            ERROR("pthread_setspecific() failed\n");
            fprintf(stderr, "pthread_setspecific() failed\n");
        }
#else
        atexit(rcf_api_cleanup);
#endif
    }

    return handle;
}

#else /* PTHREAD_SETSPECIFIC_BUG */

#ifndef HAVE_PTHREAD_H
#error PTHREAD_SETSPECIFIC_BUG may be defined only if we have pthread API.
#endif

#define RCF_MAX_THREADS     1024

/** Work-around against bug in RedHat */
static struct {
    pthread_t     tid;
    thread_ctx_t  ctx_handle;
} handles[RCF_MAX_THREADS];

/**
 * Find thread IPC handle or initialize a new one.
 *
 * @param create    Create IPC client, if it is not exist or not
 *
 * @return IPC handle or NULL if IPC library returned an error
 */
thread_ctx_t *
get_ctx_handle(te_bool create)
{
    pthread_t   mine = pthread_self();
    int         i;
    int         rc;
    
    pthread_mutex_lock(&rcf_lock);
    for (i = 0; i < RCF_MAX_THREADS; i++)
    {
        if (handles[i].tid == mine)
        {
            pthread_mutex_unlock(&rcf_lock);
            return handles[i].handle;
        }
    }
    
    if (create)
    {
        for (i = 0; i < RCF_MAX_THREADS; i++)
        {
            if (handles[i].tid == 0)
            {
                char name[RCF_MAX_NAME];
               
                handles[i].tid = mine;

                snprintf(name, RCF_MAX_NAME, "rcf_client_%u_%u",
                         (unsigned int)getpid(), (unsigned int)mine);
                   
                rc = ipc_init_client(name,
                                     &(handles[i].handle.ipc_handle));
                if (rc != 0)
                {
                    pthread_mutex_unlock(&rcf_lock);
                    fprintf(stderr, "ipc_init_client() failed\n");
                    return NULL;
                }
                assert(handles[i].handle.ipc_handle != NULL);
                CIRCLEQ_INIT(&(handles[i].handle.msg_buf_head));
                pthread_mutex_unlock(&rcf_lock);
                return &(handles[i].handle);
            }
        }
        fprintf(stderr, "too many threads\n");
    }
    pthread_mutex_unlock(&rcf_lock);
        
    return NULL;
}

/**
 * Free IPC client handle.
 */
static void
free_ctx_handle(void)
{
    pthread_t mine = pthread_self();
    int       i;

    pthread_mutex_lock(&rcf_lock);
    for (i = 0; i < RCF_MAX_THREADS; i++)
    { 
        if (handles[i].tid == mine)
        { 
            handles[i].tid = 0;
            handles[i].handle.ipc_handle = NULL;
            /* @todo clean message buffer */
            msg_buffer_clear(&(handles[i].handle.msg_buf_head));
            break;
        }
    }
    pthread_mutex_unlock(&rcf_lock);
}

#endif /* PTHREAD_SETSPECIFIC_BUG */


/**
 * Clean up resources allocated by RCF API.
 * This routine should be called from each thread used RCF API.
 */
void
rcf_api_cleanup(void)
{
#if HAVE_PTHREAD_H && !PTHREAD_SETSPECIFIC_BUG
    rcf_api_thread_ctx_destroy(get_ctx_handle(FALSE));
    /* Write NULL to key value */
    if (pthread_setspecific(key, NULL) != 0)
    {
        ERROR("pthread_setspecific() failed\n");
        fprintf(stderr, "pthread_setspecific() failed\n");
    }
#else
    struct ipc_client *ipcc = get_ctx_handle(FALSE);

    if (ipc_close_client(ipcc) != 0)
    {
        ERROR("%s(): ipc_close_client() failed", __FUNCTION__);
        fprintf(stderr, "ipc_close_client() failed\n");
    }
#if PTHREAD_SETSPECIFIC_BUG
    free_ctx_handle();
#endif
#endif
}

    
/**
 * Find CSAP in the list of busy CSAPs by its handle - should be called
 * under protection.
 *
 * @param ta_name   Test Agent name
 * @param handle    CSAP handle
 *
 * @return CSAP structure pointer or NULL
 */
static traffic_op_t *
find_traffic_op(const char *ta_name, csap_handle_t csap_id)
{
    traffic_op_t *tr_op;
    
    for (tr_op = traffic_ops.next; 
         tr_op != &traffic_ops;
         tr_op = tr_op->next)
        if ((tr_op->csap_id == csap_id) && 
            (strcmp(tr_op->ta, ta_name) == 0))
        {
            return tr_op;
        }

    return NULL;
}

/**
 * Insert CSAP into the list.
 *
 * @param cs            CSAP structure pointer
 * 
 * @return 0 (success) or 1 (CSAP is already in the list)
 */
static int
insert_traffic_op(traffic_op_t *cs)
{
    int rc;
    
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&rcf_lock);
#endif    
    
    if (rc = (find_traffic_op(cs->ta, cs->csap_id) != NULL), !rc)
    {
        QEL_INSERT(&traffic_ops, cs);
    }

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&rcf_lock);
#endif    

    return rc;
}

/**
 * Remove CSAP from the list.
 *
 * @param ta_name       Test Agent name
 * @param csap_id       csap_id of CSAP to be removed
 */
static void
remove_traffic_op(const char *ta_name, csap_handle_t csap_id)
{
    traffic_op_t *cs;
    
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&rcf_lock);
#endif    

    cs = find_traffic_op(ta_name, csap_id);
    
    if (cs != NULL)
        QEL_DELETE(cs);
    else
        WARN("%s: csap %d, traffic operation handler not found",
             __FUNCTION__, csap_id);

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&rcf_lock);
#endif
    
    free(cs);
}

/**
 * Check that length of parameter string after quoting and inserting
 * of escape sequences is not greater than maslen.
 *
 * @param params        parameters string (unquoted)
 * @param maxlen        maximum length
 * @param necessary     location for number of bytes necessary for
 *                      parameter or NULL
 *
 * @return 0 (parameters string is acceptable) or 1 (string is too long)
 */
static int
check_params_len(const char *params, int maxlen, int *necessary)
{
    int i, 
        len = 2; /* Two double quotes */
        
    for (i = 0; params[i] != 0 && len < maxlen; i++, len++)
        if (params[i] == '\\' || params[i] == '\"')
            len++;
            
    if (params[i] == 0)
    {
        if (necessary != NULL)
            *necessary = len;
        return 0;
    }
    
    return 1;
}

/**
 * This function returns list of Test Agents (names) running.
 *
 * @param buf           location for the name list; 
 *                      names are put to the buffer one-by-one and are
 *                      separated by '\0'; the list is finished by '\0' as
 *                      well
 *
 * @param len           pointer to length variable (should be set
 *                      to the length the buffer by caller;
 *                      is filled to amount of space used for the name list
 *                      by the routine)
 *          
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_ESMALLBUF the buffer is too small
 * @retval TE_EIPC      cannot interact with RCF 
 * @retval TE_ENOMEM       out of memory
 */
int 
rcf_get_ta_list(char *buf, size_t *len)
{
    rcf_msg  msg;
    rcf_msg *ans;
    size_t   anslen = sizeof(msg);
    int      rc;

    RCF_API_INIT;
    
    if (buf == NULL || len == NULL)
        return TE_RC(TE_RCF_API, TE_EWRONGPTR);
        
    memset(&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_TALIST;
        
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, &ans);
                                   
    if (rc != 0)                                   
        return rc;
        
    if (ans == NULL)
    {
        ERROR("Unexpected short message");
        return TE_RC(TE_RCF_API, TE_EIPC);
    }
        
    if (ans->error != 0)
    {
        rc = ans->error;
        free(ans);
        return rc;
    }
    
    if (ans->data_len > *len)
    {
        free(ans);
        return TE_RC(TE_RCF_API, TE_ESMALLBUF);
    }
        
    memcpy(buf, ans->data, ans->data_len);
    *len = ans->data_len;
    free(ans);
    
    return 0;
}

/**
 * This function maps name of the Test Agent to its type.
 *
 * @param ta_name       Test Agent name                 
 * @param ta_type       Test Agent type location (should point to
 *                      the buffer at least of RCF_MAX_NAME length)
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       name of non-running Test Agent is provided
 * @retval TE_EIPC      cannot interact with RCF 
 * @retval TE_ENOMEM       out of memory
 */
int 
rcf_ta_name2type(const char *ta_name, char *ta_type)
{
    rcf_msg msg;
    size_t  anslen = sizeof(msg);
    int     rc;
    
    RCF_API_INIT;
    
    if (ta_type == NULL || BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);
    
    memset((char *)&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_TATYPE;
    strcpy(msg.ta, ta_name);
    
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
                 
    if (rc == 0 && (rc = msg.error) == 0)                                   
        strcpy(ta_type, msg.id);

    return rc;
}

/**
 * This function returns unique session identifier for the Test Agent.
 * This session identifier may be passed to all other TA-related routines.
 * Command with session identifier X will be passed to the Test Agent 
 * before obtaining of answer on the command with session identifier Y
 * (X != Y). However if the Test Agent does not support simultaneous 
 * processing of several commands, this mechanism does not give any
 * advantage.
 *
 * @param ta_name       Test Agent name                 
 * @param session       location for the session identifier
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       name of non-running Test Agent is provided
 * @retval TE_EIPC      cannot interact with RCF 
 * @retval TE_ENOMEM       out of memory
 */
int 
rcf_ta_create_session(const char *ta_name, int *session)
{
    rcf_msg msg;
    size_t  anslen = sizeof(msg);
    int     rc;
    
    RCF_API_INIT;
    
    if (session == NULL || BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);
    
    memset((char *)&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_SESSION;
    strcpy(msg.ta, ta_name);
    
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
        
    if (rc == 0 && (rc = msg.error) == 0)                                   
        *session = msg.sid;
        
    return rc;
}

/**
 * This function reboots the Test Agent or NUT served by it. It sends 
 * "reboot" command to the Test Agent, calls reboot function provided by 
 * RCF TA-specific library, restarts the TA and re-connects to it.
 * The function may be called by Configurator only. 
 * It is prohibited to call the function for the TA running on the Testing
 * Node.
 *
 * @param ta_name       Test Agent name              
 * @param boot_params   complete parameter string to be passed to the TA
 *                      in the "reboot" command and to the reboot function
 *                      of RCF TA-specific library (without quotes) or NULL
 *
 * @param image         name of the bootable image (without path) to be 
 *                      passed to the Test Agent as binary attachment or
 *                      NULL (it is assumed that NUT images are located in
 *                      ${TE_INSTALL_NUT}/bin or ${TE_INSTALL}/nuts/bin)
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running or located on TN Test Agent 
 *                         is provided or parameter string is too long
 * @retval TE_EIPC         cannot interact with RCF 
 * @retval TE_EINPROGRESS  operation is in progress
 * @retval TE_ENOENT       cannot open NUT image file
 * @retval TE_ENOMEM       out of memory
 */
int 
rcf_ta_reboot(const char *ta_name,
              const char *boot_params, const char *image)
{
    rcf_msg *msg;
    
    size_t  anslen = sizeof(*msg);
    size_t  len = 0;
    int     fd;
    int     rc;

    RCF_API_INIT;
    
    if (BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);
        
    if (boot_params != NULL)
    {
        len = strlen(boot_params) + 1;
        
        if (len >  RCF_MAX_LEN - sizeof(msg) ||
            check_params_len(boot_params, 
                             RCF_MAX_LEN - TE_PROTO_OVERHEAD, NULL) != 0)
        {
            ERROR("Boot parameters are too long for TA '%s' - "
                  "change memory constants", ta_name);
            return TE_RC(TE_RCF_API, TE_EINVAL);
        }
    }
    
    if ((msg = (rcf_msg *)(calloc(sizeof(rcf_msg) + len, 1))) == NULL)
        return TE_RC(TE_RCF_API, TE_ENOMEM);
        
    if (boot_params != NULL)
    {
        strcpy(msg->data, boot_params);
        msg->data_len = len;
    }
    strcpy(msg->ta, ta_name);
    
    if (image != NULL)
    {
        /* Check file presence */
        static char *install_dir = NULL,
                    *tmp;
        
        if (install_dir == NULL &&
            (install_dir = getenv("TE_INSTALL_NUT")) == NULL)
        {
            if ((install_dir = getenv("TE_INSTALL")) == NULL)
            {
                ERROR("Neither TE_INSTALL_NUT nor TE_INSTALL are "
                      "exported - could not obtain NUT image");
                free(msg);
                return TE_RC(TE_RCF_API, TE_ENOENT);
            }
            tmp = (char *)malloc(strlen(install_dir) + 
                                 strlen("/nuts") + 1);
            if (install_dir == NULL)
            {
                free(msg);
                return TE_RC(TE_RCF_API, TE_ENOMEM);
            }
            sprintf(tmp, "%s/nuts", install_dir);
            if (setenv("TE_INSTALL_NUT", tmp, 1) != 0)
            {
                ERROR("Cannot set environment variable TE_INSTALL_NUT");
                free(msg);
                return TE_RC(TE_RCF_API, TE_ENOENT);
            }
            free(tmp);
            install_dir = getenv("TE_INSTALL_NUT");
        }
        
        if (strlen(install_dir) + strlen("/bin/") + strlen(image) + 1 >
            RCF_MAX_PATH)
        {
            ERROR("Too long full file name: %s/bin/%s\n",
                  install_dir, image);
            free(msg);
            return TE_RC(TE_RCF_API, TE_ENOENT);
        }
            
        sprintf(msg->file, "%s/bin/%s", install_dir, image);
        if ((fd = open(image, O_RDONLY)) < 0)
        {
            ERROR("Cannot open NUT image file %s for reading",
                  msg->file);
            free(msg);
            return TE_RC(TE_RCF_API, TE_ENOENT);
        }
        close(fd);
        msg->flags |= BINARY_ATTACHMENT;
    }

    msg->opcode = RCFOP_REBOOT;
     
    rc = send_recv_rcf_ipc_message(ctx_handle, msg, sizeof(rcf_msg) + len,
                                   msg, &anslen, NULL);

    if (rc == 0)
        rc = msg->error;

    free(msg);
        
    return rc;
}

/**
 * This function is used to obtain value of object instance by its
 * identifier.  The function may be called by Configurator only. 
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param oid           object instance identifier
 * @param val_buf       location for the object instance value 
 * @param len           location length
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided or OID string is
 *                      too long
 * @retval TE_EIPC      cannot interact with RCF 
 * @retval TE_ESMALLBUF the buffer is too small
 * @retval ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
int 
rcf_ta_cfg_get(const char *ta_name, int session, const char *oid, 
               char *val_buf, size_t len)
{
    rcf_msg msg;
    size_t  anslen = sizeof(msg);
    int     rc;
    
    RCF_API_INIT;

    if (oid == NULL || val_buf == NULL || strlen(oid) >= RCF_MAX_ID ||
        BAD_TA)
    {
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }
    
    memset((char *)&msg, 0, sizeof(msg));
    strcpy(msg.id, oid);
    strcpy(msg.ta, ta_name);
    msg.opcode = RCFOP_CONFGET;
    msg.sid = session;
    
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    if (rc != 0 || (rc = msg.error) != 0)
        return rc;
        
    if (msg.flags & BINARY_ATTACHMENT)
    {
        ssize_t n;
        int     fd;
        
        if ((fd = open(msg.file, O_RDONLY)) < 0)
        {
            ERROR("Cannot open file %s saved by RCF process", msg.file);
            return TE_RC(TE_RCF_API, TE_ENOENT);
        }
        if ((n = read(fd, val_buf, len)) < 0)
        {
            ERROR("Cannot read from file %s saved by RCF process",
                  msg.file);
            close(fd);
            return TE_RC(TE_RCF_API, TE_EIPC);
        }
        if (len == (size_t)n)
        {
            char tmp;

            if (read(fd, &tmp, 1) != 0)
            {
                close(fd);
                return TE_RC(TE_RCF_API, TE_ESMALLBUF);
            }
        }
        close(fd);
    }
    else
    {
        if (len <= strlen(msg.value))
            return TE_RC(TE_RCF_API, TE_ESMALLBUF);
        strcpy(val_buf, msg.value);
    }
    
    return 0;
}

/**
 * Implementation of rcf_ta_cfg_set and rcf_ta_cfg_add functionality - 
 * see description of these functions for details.
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param oid           object instance identifier
 * @param val           object instance value
 * @param opcode        RCFOP_CONFSET or RCFOP_CONFADD
 *
 * @return error code
 */
static int
conf_add_set(const char *ta_name, int session, const char *oid,
             const char *val, int opcode)
{
    rcf_msg msg;
    size_t  anslen = sizeof(msg);
    int     rc;
    
    RCF_API_INIT;
    
    if (oid == NULL || val == NULL || strlen(oid) >= RCF_MAX_ID ||
        strlen(val) >= RCF_MAX_VAL || BAD_TA)
    { 
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }
    
    memset((char *)&msg, 0, sizeof(msg));
    strcpy(msg.id, oid);
    strcpy(msg.value, val);
    strcpy(msg.ta, ta_name);
    msg.opcode = opcode;
    msg.sid = session;
    
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
                                   
    return rc == 0 ? msg.error : rc;
}

/**
 * This function is used to change value of object instance.
 * The function may be called by Configurator only. 
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param oid           object instance identifier
 * @param val           object instance value
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided or too OID or value
 *                      strings are too long
 * @retval TE_EIPC      cannot interact with RCF 
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
int 
rcf_ta_cfg_set(const char *ta_name, int session, const char *oid,
               const char *val)
{
    return conf_add_set(ta_name, session, oid, val, RCFOP_CONFSET);
}

/**
 * This function is used to create new object instance and assign the 
 * value to it.
 * The function may be called by Configurator only. 
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param oid           object instance identifier
 * @param val           object instance value
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided or too OID or value
 *                      strings are too long
 * @retval TE_EIPC      cannot interact with RCF 
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
int 
rcf_ta_cfg_add(const char *ta_name, int session, const char *oid,
               const char *val)
{
    return conf_add_set(ta_name, session, oid, val, RCFOP_CONFADD);
}

/**
 * This function is used to remove the object instance.
 * The function may be called by Configurator only. 
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param oid           object instance identifier
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided or OID string is
 *                      too long
 * @retval TE_EIPC      cannot interact with RCF 
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
int 
rcf_ta_cfg_del(const char *ta_name, int session, const char *oid)
{
    rcf_msg msg;
    size_t  anslen = sizeof(msg);
    int     rc;
    
    RCF_API_INIT;
    
    if (oid == NULL || strlen(oid) >= RCF_MAX_ID || BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);
    
    memset((char *)&msg, 0, sizeof(msg));
    strcpy(msg.id, oid);
    strcpy(msg.ta, ta_name);
    msg.opcode = RCFOP_CONFDEL;
    msg.sid = session;
    
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
                                   
    return rc == 0 ? msg.error : rc;
}

/* See description in rcf_api.h */
int
rcf_ta_cfg_group(const char *ta_name, int session, te_bool is_start)
{
    rcf_msg msg;
    size_t  anslen = sizeof(msg);
    int     rc;
    
    RCF_API_INIT;
    
    memset((char *)&msg, 0, sizeof(msg));
    strcpy(msg.ta, ta_name);
    msg.sid = session;
    msg.opcode = (is_start) ? RCFOP_CONFGRP_START : RCFOP_CONFGRP_END;
    
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    return rc == 0 ? msg.error : rc;
}

/**
 * This function is used to get bulk of log from the Test Agent.
 * The function may be called by Logger only. 
 *
 * @param ta_name       Test Agent name              
 * @param log_file      name of the local file where log should be put
 *                      (the file is truncated to zero length before
 *                      updating)
 *
 * @return error code
 *
 * @retval 0                success
 * @retval TE_EINVAL        name of non-running TN Test Agent or bad file
 *                          name are provided 
 * @retval TE_EIPC          cannot interact with RCF 
 * @retval TE_ETAREBOOTED   Test Agent is rebooted
 * @retval TE_ENOMEM        out of memory
 * @retval other            error returned by command handler on the TA
 */
int 
rcf_ta_get_log(const char *ta_name, char *log_file)
{
    rcf_msg msg;
    size_t  anslen = sizeof(msg);
    int     rc;
    
    RCF_API_INIT;
    
    if (log_file == NULL || strlen(log_file) >= RCF_MAX_PATH || BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);
    
    memset((char *)&msg, 0, sizeof(msg));
    strcpy(msg.file, log_file);
    strcpy(msg.ta, ta_name);
    msg.opcode = RCFOP_GET_LOG;
    msg.sid = RCF_TA_GET_LOG_SID;
    
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    if (rc == 0 && (rc = msg.error) == 0)
        strcpy(log_file, msg.file);
        
    return rc;    
}

/**
 * This function is used to obtain value of the variable from the Test Agent
 * or NUT served by it.
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param var_name      name of the variable to be read 
 * @param var_type      type according to which string returned from the TA
 *                      should be converted
 * 
 * @param var_len       length of the location buffer
 * @param val           location for variable value
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       one of arguments is invalid (NULL, too long or
 *                      has inappropriate value)
 * @retval TE_ENOENT       no such variable
 * @retval TE_EIPC      cannot interact with RCF 
 * @retval TE_ESMALLBUF the buffer is too small
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
int 
rcf_ta_get_var(const char *ta_name, int session, const char *var_name, 
               int var_type, size_t var_len, void *val)
{
    rcf_msg  msg;
    size_t   anslen = sizeof(msg);
    uint64_t value;
    char    *tmp;
    int      rc;
    
    RCF_API_INIT;
    
    if (var_name == NULL || val == NULL ||
        strlen(var_name) >= RCF_MAX_NAME || 
        BAD_TA || validate_type(var_type, var_len) != 0)
    {
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    memset((char *)&msg, 0, sizeof(msg));
    strcpy(msg.ta, ta_name);
    strcpy(msg.id, var_name);
    msg.intparm = var_type;
    msg.opcode = RCFOP_VREAD;
    msg.sid = session;

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
    
    if (rc != 0 || (rc = msg.error) != 0)    
        return rc;
    
    if (var_type == RCF_STRING)
    {
        if (var_len <= strlen(msg.value))
            return TE_RC(TE_RCF_API, TE_ESMALLBUF);
        strcpy(val, msg.value);
        return 0;
    }
    
    value = strtoll(msg.value, &tmp, 10);
    if (tmp == msg.value || *tmp != 0)
        return TE_RC(TE_RCF_API, TE_EFMT);
        
    switch (var_type)
    {
        case RCF_INT8:  
            *(int8_t *)val = (int8_t)value;
            break;
            
        case RCF_UINT8:  
            *(uint8_t *)val = (uint8_t)value;
            break;
            
        case RCF_INT16: 
            *(int16_t *)val = (int16_t)value;
            break;
            
        case RCF_UINT16:
            *(uint16_t *)val = (uint16_t)value;
            break;
            
        case RCF_INT32: 
            *(int32_t *)val = (int32_t)value;
            break;
            
        case RCF_UINT32:
            *(uint8_t *)val = (uint32_t)value;
            break;
            
        case RCF_INT64:
            *(int64_t *)val = (int64_t)value;
            break;

        case RCF_UINT64:
            *(int64_t *)val = value;
            break;
    }
    
    return 0;
}

/**
 * This function is used to change value of the variable from the Test Agent
 * or NUT served by it.
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param var_name      name of the variable to be changed
 * @param var_type      type according to which variable value should
 *                      appear in the protocol command
 *
 * @param val           new value of the variable
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       one of arguments is invalid (NULL, too long or
 *                      has inappropriate value)
 * @retval TE_ENOENT       no such variable
 * @retval TE_EIPC      cannot interact with RCF 
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 */
int 
rcf_ta_set_var(const char *ta_name, int session, const char *var_name, 
               int var_type, const char *val)
{
    rcf_msg msg;
    size_t  anslen = sizeof(msg);
    int     rc;
    
    RCF_API_INIT;
    
    if (var_name == NULL || val == NULL ||
        strlen(var_name) >= RCF_MAX_NAME ||
        BAD_TA || var_type < RCF_INT8 || var_type > RCF_STRING)
    {
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    memset((char *)&msg, 0, sizeof(msg));
    strcpy(msg.ta, ta_name);
    strcpy(msg.id, var_name);
    msg.opcode = RCFOP_VWRITE;
    msg.intparm = var_type;
    msg.sid = session;
    switch (var_type)
    {
        case RCF_INT8:  
            sprintf(msg.value, "%" TE_PRINTF_8 "d", *(int8_t *)val);
            break;
            
        case RCF_UINT8:  
            sprintf(msg.value, "%" TE_PRINTF_8 "u", *(uint8_t *)val);
            break;
            
        case RCF_INT16: 
            sprintf(msg.value, "%" TE_PRINTF_16 "d", *(int16_t *)val);
            break;
            
        case RCF_UINT16:
            sprintf(msg.value, "%" TE_PRINTF_16 "u", *(uint16_t *)val);
            break;
            
        case RCF_INT32: 
            sprintf(msg.value, "%" TE_PRINTF_32 "d", *(int32_t *)val);
            break;
            
        case RCF_UINT32:
            sprintf(msg.value, "%" TE_PRINTF_32 "u", *(uint32_t *)val);
            break;
            
        case RCF_INT64:
            sprintf(msg.value, "%" TE_PRINTF_64 "d", *(int64_t *)val);
            break;

        case RCF_UINT64:
            sprintf(msg.value, "%" TE_PRINTF_64 "u", *(uint64_t *)val);
            break;
            
        case RCF_STRING:
            if (strlen(val) >= RCF_MAX_VAL)
                return TE_RC(TE_RCF_API, TE_EINVAL);
            strcpy(msg.value, val);
            break;
    }
    
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
                                   
    return rc == 0 ? msg.error : rc;
}

/** 
 * Implementation of rcf_ta_get_file and rcf_ta_put_file functionality -
 * see description of these functions for details.
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param rfile         full name of the file in the TA/NUT file system
 * @param lfile         full name of the file in the TN file system
 * @param opcode        RCFOP_FPUT or RCFOP_FGET
 *
 * @return error code
 */
static int
get_put_file(const char *ta_name, int session,
             const char *rfile, const char *lfile, int opcode)
{
    rcf_msg *msg;
    size_t   anslen = sizeof(*msg);
    int      rc;
    
    RCF_API_INIT;
    
    if (rfile == NULL || lfile == NULL || strlen(lfile) >= RCF_MAX_PATH ||
        strlen(rfile) >= RCF_MAX_PATH || 
        (strlen(lfile) == 0 && opcode != RCFOP_FDEL) ||
        strlen(rfile) == 0 || BAD_TA)
    {
        ERROR("Invalid arguments are provided to rcf put/get");
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }
    
    if (opcode == RCFOP_FPUT)
    {
        int fd;
    
        if ((fd = open(lfile, O_RDONLY)) < 0)
        {
            ERROR("cannot open file %s for reading\n", lfile);
            return TE_RC(TE_RCF_API, TE_ENOENT);
        }
        close(fd);
    }

    msg = (rcf_msg *)calloc(1, sizeof(rcf_msg) + RCF_MAX_PATH);
    if (msg == NULL)
        return TE_RC(TE_RCF_API, TE_ENOMEM);
    
    msg->opcode = opcode;
    strcpy(msg->ta, ta_name);
    strcpy(msg->file, lfile);
    strcpy(msg->data, rfile);
    msg->data_len = strlen(rfile) + 1;
    msg->sid = session;
    if (opcode == RCFOP_FPUT)
        msg->flags |= BINARY_ATTACHMENT;
    
    rc = send_recv_rcf_ipc_message(ctx_handle,
                                   msg, sizeof(*msg) + msg->data_len,
                                   msg, &anslen, NULL);
    
    if (rc == 0)
        rc = msg->error;
        
    free(msg);
    
    return rc;
}

/**
 * This function loads file from Test Agent or NUT served by it to the 
 * testing node.
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param rfile         full name of the file in the TA/NUT file system
 * @param lfile         full name of the file in the TN file system
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       name of non-running TN Test Agent, non-existent
 *                      session identifier or bad file name are provided
 * @retval TE_EIPC      cannot interact with RCF 
 * @retval TE_ENOENT       no such file
 * @retval TE_EPERM        operation not permitted
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 */
int 
rcf_ta_get_file(const char *ta_name, int session,
                const char *rfile, const char *lfile)
{
    return get_put_file(ta_name, session, rfile, lfile, RCFOP_FGET);
}

/**
 * This function loads file from the testing node to Test Agent or NUT 
 * served by it.
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param lfile         full name of the file in the TN file system
 * @param rfile         full name of the file in the TA/NUT file system
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       name of non-running TN Test Agent, non-existent
 *                      session identifier or bad file name are provided
 * @retval TE_EIPC      cannot interact with RCF 
 * @retval TE_ENOENT       no such file
 * @retval TE_EPERM        operation not permitted
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 */
int 
rcf_ta_put_file(const char *ta_name, int session,
                const char *lfile, const char *rfile)
{
    return get_put_file(ta_name, session, rfile, lfile, RCFOP_FPUT);
}

/**
 * This function deletes file from the Test Agent or NUT served by it.
 *
 * @param ta_name       Test Agent name              
 * @param session       TA session or 0   
 * @param rfile         full name of the file in the TA/NUT file system
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       name of non-running TN Test Agent, non-existent
 *                      session identifier or bad file name are provided
 * @retval TE_EIPC      cannot interact with RCF 
 * @retval TE_ENOENT       no such file
 * @retval TE_EPERM        operation not permitted
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 */
int 
rcf_ta_del_file(const char *ta_name, int session, const char *rfile)
{
    return get_put_file(ta_name, session, rfile, "", RCFOP_FDEL);
}

/**
 * This function creates CSAP (communication service access point) on the 
 * Test Agent. 
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0   
 * @param stack_id      protocol stack identifier
 * @param params        parameters necessary for CSAP creation (string
 *                      or file name); if file with *params name exists,
 *                      it is attached to the CSAP creation command as
 *                      a binary attachment; otherwise the string is
 *                      appended to the command
 *
 * @param csap_id       location for unique CSAP handle
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided
 * @retval TE_EIPC    cannot interact with RCF 
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 * @retval other        error returned by command handler on the TA
 *
 * @sa rcf_ta_csap_destroy
 */
int 
rcf_ta_csap_create(const char *ta_name, int session,
                   const char *stack_id, const char *params,
                   csap_handle_t *csap_id)
{
    rcf_msg *msg;
    size_t   len = 0;
    int      flags = 0;
    size_t   anslen = sizeof(*msg);
    int      rc;
    
    RCF_API_INIT;
    
    if (stack_id == NULL || csap_id == NULL || BAD_TA)
    {
        ERROR("Invalid parameters");
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }
    
    if (params != NULL)
    {
        int fd;

        if ((fd = open(params, O_RDONLY)) < 0)
        {
            if ((len = strlen(params) + 1) >
                    (RCF_MAX_LEN - sizeof(rcf_msg)) ||
                check_params_len(params, RCF_MAX_LEN - TE_PROTO_OVERHEAD -
                                         strlen(stack_id) - 2, NULL) != 0)
            {
                return TE_RC(TE_RCF_API, TE_EINVAL);
            }
        }
        else if (strlen(params) >= RCF_MAX_PATH)
        {
            close(fd);
            ERROR("Too long file name '%s'", params);
            return TE_RC(TE_RCF_API, TE_EINVAL);
        }
        else
        {
            close(fd);
            flags = BINARY_ATTACHMENT;
        }
    }
    if ((msg = (rcf_msg *)calloc(1, sizeof(rcf_msg) + len)) == NULL)
        return TE_RC(TE_RCF_API, TE_ENOMEM);
        
    msg->opcode = RCFOP_CSAP_CREATE;
    msg->flags = flags;
    msg->sid = session;
    strcpy(msg->ta, ta_name);
    strcpy(msg->id, stack_id);
    if (flags & BINARY_ATTACHMENT)
        strcpy(msg->file, params);
    else if (params != NULL)
    {
        msg->data_len = len;
        memcpy(msg->data, params, len);
    }
    
    rc = send_recv_rcf_ipc_message(ctx_handle,
                                   msg, sizeof(rcf_msg) + len,
                                   msg, &anslen, NULL);
    
    if (rc == 0 && (rc = msg->error) == 0)
        *csap_id = msg->handle;
        
    free(msg);
        
    return rc;
}

/**
 * This function destroys CSAP.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0   
 * @param csap_id       CSAP csap_id
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided
 * @retval TE_EIPC         cannot interact with RCF 
 * @retval TE_EBADF        no such CSAP
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 *
 * @sa rcf_ta_csap_create
 */
int 
rcf_ta_csap_destroy(const char *ta_name, int session,
                    csap_handle_t csap_id)
{
    rcf_msg msg;
    size_t  anslen = sizeof(msg);
    int     rc;
    
    RCF_API_INIT;
    
    if (csap_id == CSAP_INVALID_HANDLE)
    {
        INFO("%s(): Called with CSAP_INVALID_HANDLE CSAP ID, IGNORE",
             __FUNCTION__);
        return 0;
    }

    if (BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);
    
    memset((char *)&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_CSAP_DESTROY;
    strcpy(msg.ta, ta_name);
    msg.sid = session;
    msg.handle = csap_id;
        
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
                                   
    return rc == 0 ? msg.error : rc;
}

/**
 * This function is used to obtain CSAP parameter value.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0   
 * @param csap_id       CSAP handle
 * @param var_name      parameter name
 * @param var_len       length of the location buffer
 * @param val           location for variable value
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided
 * @retval TE_EIPC         cannot interact with RCF 
 * @retval TE_EBADF        no such CSAP
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ESMALLBUF the buffer is too small
 */
int 
rcf_ta_csap_param(const char *ta_name, int session, csap_handle_t csap_id,
                  const char *var_name, size_t var_len, char *val)
{
    rcf_msg  msg;
    size_t   anslen = sizeof(msg);
    int      rc;
    
    RCF_API_INIT;
    
    if (var_name == NULL || val == NULL ||
        strlen(var_name) >= RCF_MAX_NAME || BAD_TA)
    {
        ERROR("Invalid parameters");
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    memset((char *)&msg, 0, sizeof(msg));
    strcpy(msg.ta, ta_name);
    strcpy(msg.id, var_name);
    msg.opcode = RCFOP_CSAP_PARAM;
    msg.sid = session;
    msg.handle = csap_id;
    
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
   
    if (rc != 0 || (rc = msg.error) != 0)
        return rc;
  
    if (var_len <= strlen(msg.value))
        return TE_RC(TE_RCF_API, TE_ESMALLBUF);
        
    strcpy(val, msg.value);
    
    return 0;
}

/**
 * This function is used to force sending of traffic via already created
 * CSAP.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0   
 * @param csap_id       CSAP handle
 * @param templ         full name of the file with traffic template
 * @param blk_mode      mode of the operation:
 *                      in blocking mode it suspends the caller
 *                      until sending of all the traffic finishes
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided
 * @retval TE_E   IPC      cannot interact with RCF 
 * @retval TE_EBADF        no such CSAP
 * @retval TE_EINPROGRESS  operation is already in progress
 * @retval TE_EBUSY        CSAP is used for receiving now
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 * @retval other           error returned by command handler on the TA
 *
 * @se It may block caller according to "blk_mode" parameter value
 *
 * @sa rcf_ta_trsend_stop
 */
int 
rcf_ta_trsend_start(const char *ta_name, int session, 
                    csap_handle_t csap_id, const char *templ,
                    rcf_call_mode_t blk_mode)
{
    rcf_msg       msg;
    size_t        anslen = sizeof(msg);
    int           fd;
    traffic_op_t *tr_op;
    int           rc;

    RCF_API_INIT;
    
    if (templ == NULL || strlen(templ) >= RCF_MAX_PATH || BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);
    
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&rcf_lock);
#endif    
    if ((tr_op = find_traffic_op(ta_name, csap_id)) != NULL)
    {
        int state = tr_op->state;
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&rcf_lock);
#endif
        ERROR("Cannot start sending traffic on active CSAP\n"
              "CSAP is %s",
              (state & CSAP_SEND) ? "sending" :
              (state & CSAP_RECV) ? "receiving" : "waiting for something");
        return TE_RC(TE_RCF_API, (state & CSAP_SEND) ? TE_EINPROGRESS 
                                                     : TE_EBUSY);
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&rcf_lock);
#endif    

    memset((char *)&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_TRSEND_START;
    msg.flags |= BINARY_ATTACHMENT;
    strcpy(msg.ta, ta_name);
    strcpy(msg.file, templ);
    msg.handle = csap_id;
    msg.intparm = (blk_mode == RCF_MODE_BLOCKING) ? TR_POSTPONED : 0; 
    msg.sid = session;
    
    if ((fd = open(templ, O_RDONLY)) < 0)
    {
        ERROR("Cannot open file %s for reading\n", templ);
        return TE_OS_RC(TE_RCF_API, errno);
    }
    close(fd);

    if ((tr_op = (traffic_op_t *)calloc(sizeof(traffic_op_t), 1)) == NULL)
        return TE_RC(TE_RCF_API, TE_ENOMEM);
    strncpy(tr_op->ta, ta_name, RCF_MAX_NAME);
    tr_op->csap_id = csap_id;
    tr_op->state = CSAP_SEND;
    tr_op->next = &traffic_ops;
    if (insert_traffic_op(tr_op) != 0)
    {
        free(tr_op);
        ERROR("Cannot insert CSAP control block in the list of "
              "active CSAPs");
        return TE_RC(TE_RCF_API, TE_EEXIST);
    }
    
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    if (rc != 0 || (rc = msg.error) != 0 || (blk_mode == RCF_MODE_BLOCKING))
        remove_traffic_op(ta_name, csap_id);

    return rc;
}

/**
 * This function is used to stop sending of traffic started by
 * rcf_ta_trsend_start called in non-blocking mode.
 *
 * @param ta_name       Test Agent name                 
 * @param csap_id       CSAP handle
 * @param num           location where number of sent packets should be
 *                      placed
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided
 * @retval TE_EIPC         cannot interact with RCF 
 * @retval TE_EBADF        no such CSAP
 * @retval TE_EALREADY     traffic sending is not in progress now
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 *
 * @sa rcf_ta_trsend_start
 */
int 
rcf_ta_trsend_stop(const char *ta_name, int session,
                   csap_handle_t csap_id, int *num)
{
    rcf_msg       msg;
    size_t        anslen = sizeof(msg);
    traffic_op_t *tr_op;
    int           rc;
    
    RCF_API_INIT;
    
    if (BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);
    
    memset((char *)&msg, 0, sizeof(msg));
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&rcf_lock);
#endif    
    if ((tr_op = find_traffic_op(ta_name, csap_id)) == NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&rcf_lock);
#endif
        ERROR("There is no entry in the list of active CSAPs for "
              "CSAP %d residing on Agent %s", csap_id, ta_name);
        return TE_RC(TE_RCF_API, TE_EBADFD);
    }
    if (tr_op->state != CSAP_SEND)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&rcf_lock);
#endif
        ERROR("CSAP %d residing on Agent %s is not sending",
              csap_id, ta_name);
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    msg.sid = session;
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&rcf_lock);
#endif    
    
    msg.opcode = RCFOP_TRSEND_STOP;
    strcpy(msg.ta, ta_name);
    msg.handle = csap_id;
        
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    if (rc == 0 && (rc = msg.error) == 0)
    {
        remove_traffic_op(ta_name, csap_id);
        if (num != NULL)
            *num = msg.num;
    }
    
    return rc;
}

/**
 * See the description in rcf_api.h
 */
int
rcf_ta_trrecv_start(const char *ta_name, int session,
                    csap_handle_t csap_id, const char *pattern,
                    rcf_pkt_handler handler, void *user_param, 
                    unsigned int timeout, int num)
{
    rcf_msg       msg;
    size_t        anslen = sizeof(msg);
    int           fd;
    int           rc;
    traffic_op_t *tr_op;

    RCF_API_INIT;

    if (pattern == NULL || strlen(pattern) >= RCF_MAX_PATH || BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);
    
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&rcf_lock);
#endif    
    if ((tr_op = find_traffic_op(ta_name, csap_id)) != NULL)
    {
        int state = tr_op->state;
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&rcf_lock);
#endif    
        return TE_RC(TE_RCF_API, (state & CSAP_RECV) ? TE_EINPROGRESS 
                                                     : TE_EBUSY);
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&rcf_lock);
#endif    

    memset((char *)&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_TRRECV_START;
    msg.flags |= BINARY_ATTACHMENT;
    strcpy(msg.ta, ta_name);
    strcpy(msg.file, pattern);
    msg.handle = csap_id;
    msg.intparm = (handler == NULL ? 0 : TR_RESULTS); 
    msg.sid = session;
    msg.num = num;
    msg.timeout = timeout;

    if ((fd = open(pattern, O_RDONLY)) < 0)
    {
        ERROR("Cannot open file %s for reading", pattern);
        return TE_OS_RC(TE_RCF_API, errno);
    }
    close(fd);

    if ((tr_op = (traffic_op_t *)calloc(sizeof(traffic_op_t), 1)) == NULL)
        return TE_RC(TE_RCF_API, TE_ENOMEM);
    strncpy(tr_op->ta, ta_name, RCF_MAX_NAME);
    tr_op->csap_id = csap_id;
    tr_op->state = CSAP_RECV;
    tr_op->handler = handler;
    tr_op->user_param = user_param;
    tr_op->sid = session;    
    if (insert_traffic_op(tr_op) != 0)
    {
        free(tr_op);
        return TE_RC(TE_RCF_API, TE_EEXIST);
    }
        
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
                                  
    if (rc != 0 || (rc = msg.error) != 0)
        remove_traffic_op(ta_name, csap_id);

    return rc;
}

/**
 * Implementation of rcf_ta_trrecv_stop and rcf_ta_trrecv_get
 * functionality - see description of these functions for details.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or -1, denoting session associated with
 *                      specified CSAP and TA. 
 * @param csap_id       CSAP handle
 * @param num           location where number of received packets 
 *                      should be placed
 * @param opcode        RCFOP_TRRECV_STOP, RCFOP_TRRECV_WAIT or 
 *                      RCFOP_TRRECV_GET
 *
 * @return error code
 */
static int
csap_tr_recv_get(const char *ta_name, int session, csap_handle_t csap_id,
                 int *num, int opcode)
{
    int           rc;
    rcf_msg       msg;
    size_t        anslen = sizeof(msg);
    traffic_op_t *tr_op;
    
    rcf_pkt_handler handler;
    void           *user_param;
    RCF_API_INIT;
    
    if (BAD_TA || num == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);
    
    memset((char *)&msg, 0, sizeof(msg));
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&rcf_lock);
#endif
    tr_op = find_traffic_op(ta_name, csap_id);
    if (tr_op == NULL && opcode != RCFOP_TRRECV_STOP)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&rcf_lock);
#endif
        ERROR("There is no entry in the list of active CSAPs for CSAP %d "
              "residing on Agent %s", csap_id, ta_name);
        return TE_RC(TE_RCF_API, TE_EBADFD);
    }
    if ((tr_op != NULL) && (tr_op->state != CSAP_RECV))
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&rcf_lock);
#endif
        ERROR("CSAP %d residing on Agent %s is not receiving, but %s",
              csap_id, ta_name, (tr_op->state & CSAP_SEND) ? "sending" :
                                   "waiting for something");
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    if (tr_op != NULL)
        tr_op->num_users++;

    msg.sid = session;

    handler    = (tr_op != NULL) ? tr_op->handler    : NULL;
    user_param = (tr_op != NULL) ? tr_op->user_param : NULL;

#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&rcf_lock);
#endif

    msg.opcode = opcode;
    strcpy(msg.ta, ta_name);
    msg.handle = csap_id;

    anslen = sizeof(msg);
    if ((rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                        &msg, &anslen, NULL)) != 0)
    {
        remove_traffic_op(ta_name, csap_id);
        ERROR("%s: IPC send with answer fails, rc %r", 
              __FUNCTION__, rc);
        return rc;
    }

    while ((msg.flags & INTERMEDIATE_ANSWER))
    {
        handler(msg.file, user_param);

        anslen = sizeof(msg);
        if ((rc = wait_rcf_ipc_message(ipc_handle, 
                                       &(ctx_handle->msg_buf_head),
                                       opcode, session, &msg, 
                                       &anslen, NULL)) != 0)
        {
            ERROR("%s: IPC receive answer fails, rc %r", 
                  __FUNCTION__, rc);
            return TE_RC(TE_RCF_API, TE_EIPC);
        }
    }
    
    if (msg.error == 0 || opcode != RCFOP_TRRECV_GET)
        *num = msg.num;

    if (tr_op != NULL)
        tr_op->num_users--;

    if ((opcode != RCFOP_TRRECV_GET) &&
        (tr_op != NULL) && (tr_op->num_users == 0))
    {
        /* 
         * For STOP and WAIT request descr should be removed 
         * @todo investigate of consistant removing csap request record 
         * in case of error. 
         */
        remove_traffic_op(ta_name, csap_id);
    }
    if (msg.error)
        WARN("RCF traffic operation fails with status code %r", msg.error);
    
    return msg.error;
}

/**
 * See the description in rcf_api.h
 */
int
rcf_ta_trrecv_wait(const char *ta_name, int session,
                   csap_handle_t csap_id, int *num)
{
    int rc;
    
    VERB("%s(ta %s, csap %d, *num  %p) called", 
         __FUNCTION__, ta_name, csap_id, num);
    rc = csap_tr_recv_get(ta_name, session, csap_id, num,
                          RCFOP_TRRECV_WAIT);
    VERB("%s(ta %s, csap %d, *num  %p) return %r, num %d", 
         __FUNCTION__, ta_name, csap_id, num, rc, 
         (num == NULL ? -1: *num));
         
    return rc;
}
                      
/**
 * This function is used to stop receiving of traffic started by
 * rcf_ta_trrecv_start called in blocking mode.
 * If handler was specified in the function rcf_ta_trrecv_start, 
 * it is called for all received packets.
 *
 * @param ta_name       Test Agent name                 
 * @param csap_id       CSAP handle
 * @param num           location where number of received packets 
 *                      should be placed
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided
 * @retval TE_EIPC         cannot interact with RCF 
 * @retval TE_EBADF        no such CSAP
 * @retval TE_EALREADY     traffic receiving is not in progress now
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 *
 * @sa rcf_ta_trrecv_start
 */ 
int 
rcf_ta_trrecv_stop(const char *ta_name, int session,
                        csap_handle_t csap_id, int *num)
{
    int rc;

    VERB("%s(ta %s, csap %d, *num  %p) called", 
         __FUNCTION__, ta_name, csap_id, num); 
    rc = csap_tr_recv_get(ta_name, session, csap_id, num,
                            RCFOP_TRRECV_STOP); 
    VERB("%s(ta %s, csap %d, *num  %p) return %r, num %d", 
         __FUNCTION__, ta_name, csap_id, num, rc,
         (num == NULL ? -1: *num));
         
    return rc;
}

/**
 * This function is used to force processing of received packets 
 * without stopping of traffic receiving (handler specified in
 * rcf_ta_trrecv_start is used for packets processing).
 *
 * @param ta_name       Test Agent name                 
 * @param csap_id       CSAP handle
 * @param num           location where number of processed packets 
 *                      should be placed
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided
 * @retval TE_EIPC         cannot interact with RCF 
 * @retval TE_EBADF        no such CSAP
 * @retval TE_EALREADY     traffic receiving is not in progress now
 * @retval TE_ENODATA      no data available on TA, because handler was not
 *                         specified in rcf_ta_trrecv_start
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 *
 * @sa rcf_ta_trrecv_start
 */
int 
rcf_ta_trrecv_get(const char *ta_name, int session,
                  csap_handle_t csap_id, int *num)
{
    int rc;

    VERB("%s(ta %s, csap %d, *num  %p) called", 
         ta_name, csap_id, num);
    rc = csap_tr_recv_get(ta_name, session, csap_id, num,
                          RCFOP_TRRECV_GET);

    VERB("%s(ta %s, csap %d, *num  %p) return %r, num %d", 
         ta_name, csap_id, num, rc, (num == NULL ? -1: *num)); 

    return rc;
}

/**
 * This function is used to send exactly one packet via CSAP and receive
 * an answer (it may be used for CLI, SNMP, ARP, ICMP, DNS, etc.)
 * This function blocks the caller until the packet is received by 
 * traffic application domain or timeout occures.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param csap_id       CSAP handle
 * @param templ         Full name of the file with traffic template
 * @param handler       Callback function used in processing of
 *                      received packet or NULL
 * @param user_param    User parameter to be passed to the handler
 * @param timeout       Timeout for answer waiting
 * @param error         Location for protocol-specific error extracted
 *                      from the answer or NULL (if error is 0, then answer
 *                      is positive; if error is -1, then it's not possible
 *                      to extract the error)
 *
 * @return error code
 *
 * @retval 0               success
 * @retval TE_ETIMEDOUT    timeout occured before a packet that matches
 *                         the template received
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                         session identifier is provided or flag blocking
 *                         is used together with num 0
 * @retval TE_EIPC         cannot interact with RCF 
 * @retval TE_EBADF        no such CSAP
 * @retval TE_EBUSY        CSAP is busy
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 * @retval other           error returned by command handler on the TA
 */
int 
rcf_ta_trsend_recv(const char *ta_name, int session, csap_handle_t csap_id,
                   const char *templ, rcf_pkt_handler handler, 
                   void *user_param, unsigned int timeout, int *error)
{
    rcf_msg       msg;
    traffic_op_t *tr_op;
    size_t        anslen = sizeof(msg);
    int           fd;
    int           rc;
    
    RCF_API_INIT;
    
    if (BAD_TA || templ == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);
    
    memset((char *)&msg, 0, sizeof(msg));
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&rcf_lock);
#endif    
    if ((tr_op = find_traffic_op(ta_name, csap_id)) != NULL)
    {
#ifdef HAVE_PTHREAD_H
        pthread_mutex_unlock(&rcf_lock);
#endif    
        return TE_RC(TE_RCF_API, TE_EBUSY);
    }
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&rcf_lock);
#endif    
    msg.sid = session;
    msg.num = 1; /* receive only one packet */
    msg.timeout = timeout;
    msg.opcode = RCFOP_TRSEND_RECV;
    msg.flags |= BINARY_ATTACHMENT;
    strcpy(msg.ta, ta_name);
    strcpy(msg.file, templ);
    msg.handle = csap_id;
    msg.intparm = handler == NULL ? 0 : TR_RESULTS;
    
    if ((fd = open(templ, O_RDONLY)) < 0)
    {
        ERROR("Cannot open file %s for reading", templ);
        return TE_RC(TE_RCF_API, TE_ENOENT);
    }
    close(fd);

    if ((tr_op = (traffic_op_t *)calloc(sizeof(traffic_op_t), 1)) == NULL)
        return TE_RC(TE_RCF_API, TE_ENOMEM);
    strncpy(tr_op->ta, ta_name, RCF_MAX_NAME);
    tr_op->csap_id = csap_id;
    tr_op->state = CSAP_SENDRECV;
    if (insert_traffic_op(tr_op) != 0)
    {
        free(tr_op);
        return TE_RC(TE_RCF_API, TE_EBUSY);
    }
        
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
        
    if (rc != 0)
    {
        remove_traffic_op(ta_name, csap_id);
        return rc;
    }
    
    while (msg.flags & INTERMEDIATE_ANSWER)
    {
        handler(msg.file, user_param);
        anslen = sizeof(msg);
        if ((rc = wait_rcf_ipc_message(ipc_handle, 
                                       &(ctx_handle->msg_buf_head),
                                       RCFOP_TRSEND_RECV, session,
                                       &msg, &anslen, NULL)) != 0)
        {
            return rc;
        }
    }
    
    remove_traffic_op(ta_name, csap_id);
    if (msg.error != 0)
        return msg.error;
    
    if (error != NULL)
        *error = msg.intparm;
    
    return 0;
}

/**
 * Auxiliary function to check and code routine of task
 * entry point parameters. 
 *
 * @param argv          if TRUE, paramaters should be in (argc, argv) form;
 *                      otherwise types are specified for parameters
 * @param argc          number of parameters
 * @param data          location for coded parameters
 * @param data_len      location for length of encoded parameters
 * @param ap            list of parameters to be checked and encoded
 *
 * @return error code
 */
static int
make_params(int argc,  int argv, char *data, size_t *data_len, va_list ap)
{
    int     i, n;
    size_t  len = 0;
    ssize_t maxlen = RCF_MAX_LEN - TE_PROTO_OVERHEAD - RCF_MAX_NAME -
                     RCF_MAX_INT;
    
#define CHECK_LENS \
    if (len > RCF_MAX_LEN - sizeof(rcf_msg) || maxlen < 0) \
        return TE_RC(TE_RCF_API, TE_EINVAL);
    
    /* Determine data length and check command size restrictions */
    if (argv)
    {
        for (i = 0; i < argc; i++)
        {
            char *s = va_arg(ap, char *);
            
            if (check_params_len(s, maxlen, &n) != 0)
                return TE_RC(TE_RCF_API, TE_EINVAL);
                
            len += strlen(s) + 1;
            maxlen -= n + 1;
            CHECK_LENS;
                
            strcpy(data, s);
            data += strlen(s) + 1;
        }
        *data_len = len;
        return 0;
    }
    
    for (i = 0; i < argc; i++)
    {
        int type = va_arg(ap, int);
        
        if (type < RCF_INT8 || type > RCF_STRING)
            return TE_RC(TE_RCF_API, TE_EINVAL);
            
        maxlen -= RCF_MAX_TYPE_NAME + 1;
        len++;
        CHECK_LENS;
            
        *(data++) = (unsigned char)type;
            
        if (type == RCF_STRING)
        {
            char *s = va_arg(ap, char *);
            
            if (check_params_len(s, maxlen, &n) != 0)
                return TE_RC(TE_RCF_API, TE_EINVAL);
                
            maxlen -= n + 1;
            len += strlen(s) + 1;
            CHECK_LENS;
            
            strcpy(data, s);
            data += strlen(s) + 1;
        }
        else
        {
            maxlen -= RCF_MAX_INT + 1;
            len += rcf_type_len[type];
            CHECK_LENS;

            switch (type)
            {
                case RCF_INT8:
                case RCF_UINT8:
                {
                    int8_t val = va_arg(ap, int);
                    memcpy(data, (char *)&val, rcf_type_len[type]);
                    data += rcf_type_len[type];
                    break;
                }

                case RCF_INT16:
                case RCF_UINT16:
                {
                    uint16_t val = va_arg(ap, int);
                    memcpy(data, (char *)&val, rcf_type_len[type]);
                    data += rcf_type_len[type];
                    break;
                }

                case RCF_INT32:
                case RCF_UINT32:
                {
                    uint32_t val = va_arg(ap, int32_t);
                    memcpy(data, (char *)&val, rcf_type_len[type]);
                    data += rcf_type_len[type];
                    break;
                }
                
                case RCF_INT64:
                case RCF_UINT64:
                {
                    uint64_t val = va_arg(ap, int64_t);
                    memcpy(data, (char *)&val, rcf_type_len[type]);
                    data += rcf_type_len[type];
                    break;
                }
            }
        }
    }
    
    *data_len = len;
    return 0;
}

/**
 * Implementation of rcf_ta_call and rcf_ta_start_task functionality -
 * see description of these functions for details.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0   
 * @param priority      process priority
 * @param rtn           routine name
 * @param opcode        RCFOP_START or RCFOP_EXECUTE
 * @param res           location for the code returned by the routine
 * @param argc          number of parameters
 * @param argv          if 1, parameters in ap are char *;
 *                      otherwise pairs: type, value.
 * @param ap            list of additional parameters
 * @param mode          start mode (rfc_start_func, rfc_start_thread,
 *                      rcf_start_fork)
 *
 * @return error code
 */
static int
call_start(const char *ta_name, int session, int priority, const char *rtn, 
           int *res, int argc, int argv, va_list ap, 
           enum rcf_start_modes mode)
{
    rcf_msg *msg;
    size_t   anslen = sizeof(*msg);
    int      rc;
    
    RCF_API_INIT;
    
    if (rtn == NULL || strlen(rtn) >= RCF_MAX_NAME || BAD_TA || res == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);
    
    if ((msg = (rcf_msg *)calloc(RCF_MAX_LEN, 1)) == NULL)
        return TE_RC(TE_RCF_API, TE_ENOMEM);

    msg->opcode = RCFOP_EXECUTE;
    msg->sid = session;
    strcpy(msg->ta, ta_name);
    strcpy(msg->id, rtn);
    
    msg->intparm = priority;
    msg->handle = mode;
        
    if (argc != 0)
    {
        if (argv)
            msg->flags |= PARAMETERS_ARGV;
        msg->num = argc;
        if ((rc = make_params(argc, argv, msg->data, 
                              &(msg->data_len), ap)) != 0)
        {
            ERROR("Possibly too many or too long routine "
                  "parameters are provided - change of "
                  "memory constants may help");
            free(msg);
            return TE_RC(TE_RCF_API, rc);
        }
    }
        
    rc = send_recv_rcf_ipc_message(ctx_handle,
                                   msg, sizeof(rcf_msg) + msg->data_len, 
                                   msg, &anslen, NULL);

    if (rc == 0 && (rc = msg->error) == 0)
    {
        *res = (mode == RCF_START_FUNC ? msg->intparm : msg->handle);
        VERB("Call/start %s on the TA %s", rtn, ta_name);
    }
        
    free(msg);
    
    return rc;
}


/* See description in rcf_api.h */
int 
rcf_ta_call(const char *ta_name, int session, const char *rtn, int *rc, 
            int argc, te_bool argv, ...)
{
    int     error;
    va_list ap;
    
    va_start(ap, argv);
    
    error = call_start(ta_name, session, 0, rtn, rc, argc, argv, ap, 
                       RCF_START_FUNC);
    
    va_end(ap);
    
    return error;
}
                       

/* See description in rcf_api.h */
int 
rcf_ta_start_task(const char *ta_name, int session, int priority,
                  const char *rtn, pid_t *pid, int argc, te_bool argv, ...)
{
    int     error;
    va_list ap;
    
    va_start(ap, argv);
    
    error = call_start(ta_name, session, priority, rtn, pid, argc, argv, ap,
                       RCF_START_FORK);
    
    va_end(ap);
    
    return error;
}

/* See description in rcf_api.h */
int 
rcf_ta_start_thread(const char *ta_name, int session, int priority,
                  const char *rtn, int *tid, int argc, te_bool argv, ...)
{
    int     error;
    va_list ap;
    
    va_start(ap, argv);
    
    error = call_start(ta_name, session, priority, rtn, tid, argc, argv, ap,
                       RCF_START_THREAD);
    
    va_end(ap);
    
    return error;
}

                                
/**
 * This function is used to kill a process on the Test Agent or 
 * NUT served by it.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0   
 * @param pid           identifier of the process to be killed
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_EINVAL       name of non-running TN Test Agent or non-existent
 *                      session identifier is provided
 * @retval TE_ENOENT       no such process
 * @retval TE_EIPC      cannot interact with RCF 
 * @retval TE_ETAREBOOTED  Test Agent is rebooted
 * @retval TE_ENOMEM       out of memory
 *
 * @sa rcf_ta_start_process
 */
int 
rcf_ta_kill_task(const char *ta_name, int session, pid_t pid)
{
    rcf_msg msg;
    size_t  anslen = sizeof(msg);
    int     rc;
    
    RCF_API_INIT;
    
    if (BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);
    
    memset((char *)&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_KILL;
    strcpy(msg.ta, ta_name);
    msg.handle = pid;
    msg.sid = session;
        
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
    
    return rc == 0 ? msg.error : rc;
}

/**
 * This function is used to check that all running are still
 * working.
 *
 * @return error code
 *
 * @retval 0            success
 * @retval TE_ETAREBOOTED  if at least one test agent has been normally
 *                      rebooted
 * @retval TE_ETADEAD   if at least one agent was dead
 * @retval TE_EIPC      cannot interact with RCF 
 * 
 */
int
rcf_check_agents(void)
{ 
    rcf_msg  msg;
    size_t   anslen = sizeof(msg);
    int      rc;
    
    RCF_API_INIT;
    
    memset((char *)&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_TACHECK;
        
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
    return rc == 0 ? msg.error : rc;
}

#include "tarpc.h"
/**
 * Call SUN RPC on the TA.
 *
 * @param ta_name       Test Agent name                 
 * @param session       TA session or 0
 * @param rpcserver     Name of the RPC server
 * @param timeout       RPC timeout in milliseconds or 0 (unlimited)
 * @param rpc_name      Name of the RPC (e.g. "bind")
 * @param in            Input parameter C structure
 * @param in            Output parameter C structure
 *
 * @return Status code
 */
int 
rcf_ta_call_rpc(const char *ta_name, int session, 
                const char *rpcserver, int timeout,
                const char *rpc_name, void *in, void *out)
{
/** Length of RPC data inside the message */     
#define INSIDE_LEN \
    (sizeof(((rcf_msg *)0)->file) + sizeof(((rcf_msg *)0)->value))
    
/** Length of message part before RPC data */
#define PREFIX_LEN  \
    (sizeof(rcf_msg) - INSIDE_LEN)

    char     msg_buf[PREFIX_LEN + RCF_RPC_BUF_LEN];
    rcf_msg *msg = (rcf_msg *)msg_buf;
    rcf_msg *ans = NULL;
    int      rc;
    size_t   anslen = sizeof(msg_buf);
    size_t   len;
    
    RCF_API_INIT;

    if (BAD_TA || rpcserver == NULL || rpc_name == NULL || 
        in == NULL || out == NULL)
    {
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }
    
    if (strlen(rpc_name) >= RCF_RPC_MAX_NAME)
    {
        ERROR("Too long RPC name: %s - change RCF_RPC_MAX_NAME constant", 
              rpc_name);
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }
    
    len = RCF_RPC_BUF_LEN;
    if ((rc = rpc_xdr_encode_call(rpc_name, msg->file, &len, in)) != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            ERROR("Unknown RPC %s", rpc_name);
            return rc;
        }
        
        /* Try to allocate more memory */
        len = RCF_RPC_HUGE_BUF_LEN;
        anslen = PREFIX_LEN + RCF_RPC_HUGE_BUF_LEN;
        if ((msg = (rcf_msg *)malloc(anslen)) == NULL)
            return TE_RC(TE_RCF_API, TE_ENOMEM);

        if ((rc = rpc_xdr_encode_call(rpc_name, msg->file, &len, in)) != 0)
        {
            ERROR("Encoding of RPC %s input parameters failed", rpc_name);
            free(msg);
            return rc;
        }
    }
    
    memset(msg, 0, PREFIX_LEN);
    msg->opcode = RCFOP_RPC;
    msg->sid = session;
    strcpy(msg->ta, ta_name);
    strcpy(msg->id, rpcserver);
    msg->timeout = timeout;
    msg->intparm = len;
    msg->data_len = len - INSIDE_LEN;

    rc = send_recv_rcf_ipc_message(ctx_handle, msg, PREFIX_LEN + len, 
                                   msg, &anslen, &ans);
    if (ans != NULL)
    {
        if ((char *)msg != msg_buf)
            free(msg);
            
        msg = ans;
    }
    
    if (rc != 0 || (rc = msg->error) != 0)
    {
       if ((char *)msg != msg_buf)
            free(msg);
        return rc;
    }
    
    rc = rpc_xdr_decode_result(rpc_name, msg->file, msg->intparm, out);
    
    if (rc != 0)
        ERROR("Decoding of RPC %s output parameters failed", rpc_name);
        
    if ((char *)msg != msg_buf)
        free(msg);
        
    return rc;        
    
#undef INSIDE_LEN    
#undef PREFIX_LEN    
}

int
rcf_shutdown_call(void)
{
    rcf_msg msg;
    size_t  anslen = sizeof(msg);
    int     rc;
    
    RCF_API_INIT;
    
    memset((char *)&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_SHUTDOWN;
    
    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    return rc == 0 ? msg.error : rc;
}
