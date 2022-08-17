/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test Environment
 *
 * RCF API library implementation
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "RCF API"

#include "te_config.h"

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
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
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

#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_printf.h"
#include "te_queue.h"
#include "te_str.h"
#include "logger_api.h"
#include "logger_ten.h"
#include "rcf_api.h"
#include "rcf_internal.h"
#include "rcf_methods.h"
#include "conf_messages.h"
#define RCF_NEED_TYPE_LEN 1
#include "te_proto.h"
#include "ipc_client.h"


/** Busy CSAPs state */
#define CSAP_SEND       1
#define CSAP_RECV       2
#define CSAP_SENDRECV   3

/** Number of symbols for of int32_t + spaces */
#define RCF_MAX_INT     12


typedef struct msg_buf_entry {
    CIRCLEQ_ENTRY(msg_buf_entry) link;
    rcf_msg                     *message;
} msg_buf_entry_t;

typedef CIRCLEQ_HEAD(msg_buf_head, msg_buf_entry) msg_buf_head_t;

typedef struct thread_ctx {
    struct ipc_client *ipc_handle;
    msg_buf_head_t     msg_buf_head;
    uint32_t           seqno;
    te_bool            log_cfg_changes;
} thread_ctx_t;


/* Forward declaration */
static int csap_tr_recv_get(const char *ta_name, int session,
                            csap_handle_t csap_id,
                            rcf_pkt_handler handler, void *user_param,
                            unsigned int *num, int opcode);

/* If pthread mutexes are supported - OK; otherwise hope for best... */
#ifdef HAVE_PTHREAD_H
static pthread_once_t   once_control = PTHREAD_ONCE_INIT;
static pthread_key_t    key;
#endif

/**
 * Log traffic operations using RING (TRUE) or INFO (FALSE) level.
 *
 * It is per library instance configuration variable. It is not
 * protected by any means.
 */
static te_bool  rcf_tr_op_ring = TRUE;


/* Following two macros are necessary to provide tread safety */

#ifndef HAVE_PTHREAD_H
#define pthread_self()  0
#endif

/* Declare and initialize (or obtain old) IPC library handle. */
#define RCF_API_INIT \
    thread_ctx_t *ctx_handle = get_ctx_handle(TRUE);            \
                                                                \
    if (ctx_handle == NULL || ctx_handle->ipc_handle == NULL)   \
        return TE_RC(TE_RCF_API, TE_EIPC);                      \


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
 * Callback to match RCF message.
 *
 * @param msg           RCF message
 * @param opaque        Opaque data
 *
 * @retval 0    message matches
 * @retval 1    message NOT matches
 */
typedef int (*rcf_message_match_cb)(rcf_msg *msg, void *opaque);


/** Opaque data for simple RCF message matching. */
typedef struct rcf_message_match_simple {
    rcf_op_t    opcode;     /**< Operation code */
    const char *ta_name;    /**< Test Agent name */
    int         sid;        /**< RCF session */
} rcf_message_match_simple;

/**
 * Match RCF IPC message with TA name, opcode and session id.
 *
 * The function complies with rcf_message_match_cb prototype.
 */
static int
rcf_message_match(rcf_msg *msg, void *opaque)
{
    rcf_message_match_simple   *data = opaque;

    if (msg == NULL)
        return 1;

    if (msg->opcode != data->opcode)
        return 1;

    switch (msg->opcode)
    {
        case RCFOP_TALIST:
        case RCFOP_TACHECK:
            /* These requests do not have TA name and SID */
            return 0;

        default:
            if (msg->sid != data->sid)
                return 1;
            /* FALLTHROUGH */

        case RCFOP_TATYPE:
        case RCFOP_SESSION:
        case RCFOP_REBOOT:
            if (strcmp(msg->ta, data->ta_name) != 0)
                return 1;
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
    {
        free(buf_entry);
        return TE_ENOMEM;
    }

    memcpy(buf_entry->message, message, msg_len);

    CIRCLEQ_INSERT_TAIL(buf_head, buf_entry, link);

    return 0;
}

/**
 * Find message in RCF message buffer with desired SID, and remove
 * it from buffer.
 *
 * @param msg_buf       RCF message buffer head
 * @param match_cb      Callback function to be used to match
 * @param opaque        Opaque data of the callback function
 *
 * @return pointer to found message or zero if not found
 * After ussage of message pointer should be freed.
 */
static rcf_msg *
msg_buffer_find(msg_buf_head_t *msg_buf, rcf_message_match_cb match_cb,
                void *opaque)
{
    msg_buf_entry_t *buf_entry;

    if (msg_buf == NULL)
        return NULL;

    for (buf_entry = msg_buf->cqh_first;
         buf_entry != (void *)msg_buf;
         buf_entry = buf_entry->link.cqe_next)
    {
        if (match_cb(buf_entry->message, opaque) == 0)
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
    te_errno    rc;
    size_t      buflen = *recv_size;
    size_t      len;

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

    memcpy(*p_answer, recv_msg, buflen);
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
 * @param ipcc          pointer to the ipc_client structure returned
 *                      by ipc_init_client()
 * @param rcf_msg_queue message buffer head
 * @param match_cb      Callback function to be used to match
 * @param opaque        Opaque data of the callback function
 * @param recv_msg      pointer to the buffer for answer
 * @param recv_size     pointer to the variable to store:
 * @param p_answer      location for address of the memory
 *                      allocated for the answer or NULL
 *
 * @return zero on success or error code
 */
static int
wait_rcf_ipc_message(struct ipc_client *ipcc,
                     msg_buf_head_t *rcf_msg_queue,
                     rcf_message_match_cb match_cb, void *opaque,
                     rcf_msg *recv_msg, size_t *recv_size,
                     rcf_msg **p_answer)
{
    rcf_msg    *message;
    te_errno    rc;

    if (ipcc == NULL || recv_msg == NULL || recv_size == NULL)
        return TE_EWRONGPTR;

    message = msg_buffer_find(rcf_msg_queue, match_cb, opaque);
    if (message != NULL)
    {
        size_t len = sizeof(*message) + message->data_len;

        VERB("Message found: TA %s, SID %d flags %x;"
             " waiting for SID %d",
             message->ta, message->sid, message->flags, message->sid);

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
             message->ta, message->sid, message->flags, message->sid);

        if (match_cb(message, opaque) == 0)
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
static te_errno
send_recv_rcf_ipc_message(thread_ctx_t *ctx,
                          rcf_msg *send_buf, size_t send_size,
                          rcf_msg *recv_buf, size_t *recv_size,
                          rcf_msg **p_answer)
{
    te_errno                    rc;
    char                        ta[RCF_MAX_NAME];
    rcf_msg                    *message;
    rcf_message_match_simple    match_data;


    if (ctx == NULL || ctx->ipc_handle == NULL ||
        send_buf == NULL || recv_buf == NULL || recv_size == NULL)
        return TE_RC(TE_RCF_API, TE_EWRONGPTR);

    /* The same memory may be used for send_buf and recv_buf */
    te_strlcpy(ta, send_buf->ta, sizeof(ta));
    match_data.ta_name = ta;
    match_data.sid = send_buf->sid;
    match_data.opcode = send_buf->opcode;

    send_buf->seqno = ctx->seqno++;

    INFO("%s: send request %u:%d:'%s'", ipc_client_name(ctx->ipc_handle),
         (unsigned)send_buf->seqno, send_buf->sid,
         rcf_op_to_string(send_buf->opcode));

    if ((rc = ipc_send_message(ctx->ipc_handle, RCF_SERVER,
                               send_buf, send_size)) != 0)
    {
        /*
         * Encountering EPIPE is the only way to know that RCF is down,
         * so it is a part of normal operation. However, a message should
         * still be printed for debugging purposes.
         */
        if (TE_RC_GET_ERROR(rc) == TE_EPIPE)
            INFO("%s() failed with rc %r", __FUNCTION__, rc);
        else
            ERROR("%s() failed with rc %r", __FUNCTION__, rc);
        return TE_RC(TE_RCF_API, TE_EIPC);
    }

    if ((rc = rcf_ipc_receive_answer(ctx->ipc_handle, recv_buf,
                                     recv_size, p_answer)) != 0)
        return rc;

    message = (p_answer != NULL && *p_answer != NULL) ? *p_answer
                                                      : recv_buf;
    if (rcf_message_match(recv_buf, &match_data) == 0)
        return 0;

    if (msg_buffer_insert(&ctx->msg_buf_head, message) != 0)
        ERROR("RCF message is lost");

    if (message != recv_buf)
        free(message);

    return wait_rcf_ipc_message(ctx->ipc_handle, &ctx->msg_buf_head,
                                rcf_message_match, &match_data,
                                recv_buf, recv_size, p_answer);
}

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
static thread_ctx_t *
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
        te_errno    rc;
        char        name[RCF_MAX_NAME];

        handle = calloc(1, sizeof(*handle));
        if (handle == NULL)
        {
            ERROR("%s(): calloc(1, %u) failed",
                  __FUNCTION__, sizeof(*handle));
            return NULL;
        }

        sprintf(name, "rcf_client_%u_%u", (unsigned int)getpid(),
                (unsigned int)pthread_self());

        rc = ipc_init_client(name, RCF_IPC, &(handle->ipc_handle));
        if (rc != 0)
        {
            free(handle);
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


/* See description in rcf_api.h */
void
rcf_api_cleanup(void)
{
#if HAVE_PTHREAD_H
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
#endif
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

static char const rcfunix_name[] = "rcfunix";

/**
 * Delete Test Agent from RCF (main executive part).
 *
 * @param name          Test Agent name
 *
 * @return Error code
 * @retval 0            success
 * @retval TE_ENOENT    Test Agent with such name does not exist
 * @retval TE_EPERM     Test Agent with such name exists but is
 *                      specified in RCF configuration file
 */
static te_errno
del_ta_executive(const char *name)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    memset(&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_DEL_TA;
    te_strlcpy(msg.ta, name, sizeof(msg.ta));

    return (rc = send_recv_rcf_ipc_message(ctx_handle, &msg,
                                           sizeof(msg), &msg, &anslen,
                                           NULL)) != 0 ? rc : msg.error;
}

/* See description in rcf_api.h */
te_errno
rcf_add_ta(const char *name, const char *type,
           const char *rcflib, const char *confstr,
           unsigned int flags)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    if (name == NULL || type == NULL || rcflib == NULL || confstr == NULL)
        return TE_RC(TE_RCF_API, TE_EWRONGPTR);

    if (strlen(name) + 1 > sizeof(msg.ta))
    {
        ERROR("Too long (%u chars, must be not more than %u ones) "
              "TA name = '%s'", strlen(name), sizeof(msg.ta) - 1, name);
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    /* Check lengths of values */
    if (strlen(rcflib) + 1 > sizeof(msg.file))
    {
        ERROR("Too long 'rcflib' value = '%s'", rcflib);
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    if (strlen(confstr) + 1 > sizeof(msg.value))
    {
        ERROR("Too long 'confstr' value = '%s'", confstr);
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    if (flags & RCF_TA_UNIX_SUDO)
    {
        ERROR("RCF_TA_UNIX_SUDO flag is specified for rcf_add_ta(): "
              "use 'sudo' specification in 'confstr' value instead");
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    RCF_API_INIT;

    memset(&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_ADD_TA;
    te_strlcpy(msg.ta, name, sizeof(msg.ta));
    te_strlcpy(msg.id, type, sizeof(msg.id));
    te_strlcpy(msg.file, rcflib, sizeof(msg.file));
    te_strlcpy(msg.value, confstr, sizeof(msg.value));

    if (flags & RCF_TA_REBOOTABLE)
        msg.flags |= TA_REBOOTABLE;

    if (flags & RCF_TA_NO_HKEY_CHK)
        msg.flags |= TA_NO_HKEY_CHK;

    msg.intparm = (flags & RCF_TA_NO_SYNC_TIME) ? 0 : 1;

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    /* On success perform postprocessing */
    if (rc == 0 && (rc = msg.error) == 0)
    {
        /* Add logger hanlder for just added TA */

        size_t const       pl         = strlen(LGR_SRV_FOR_TA_PREFIX);
        size_t const       nl         = strlen(name);
        te_log_nfl const   nfl        = pl + nl;
        te_log_nfl const   nfl_net    = htons(nfl);
        char               msg[sizeof(nfl_net) + nfl];
        struct ipc_client *log_client = NULL;

        /* Prepare msg with 'LGR_SRV_FOR_TA_PREFIX + name' entity name */
        memcpy(msg, &nfl_net, sizeof(nfl_net));
        memcpy(msg + sizeof(nfl_net), LGR_SRV_FOR_TA_PREFIX, pl);
        memcpy(msg + sizeof(nfl_net) + pl, name, nl);

        /* We do not fail if logger TA handler is not invoked */

        /* Here the local 'rc' is used */
        if ((rc = ipc_init_client("RCF API: rcf_add_ta()",
                                  LOGGER_IPC, &log_client)) == 0)
        {
            /* Here the local 'rc' is used */
            if ((rc = ipc_send_message(log_client, LGR_SRV_NAME,
                                       msg, sizeof(msg))) != 0)
            {
                ERROR("Failed to send IPC message to logger "
                      "in order to invoke logger TA handler: %r", rc);
                del_ta_executive(name); /** Rollback */
            }
            else
            {
                if (ipc_close_client(log_client) != 0)
                    WARN("Failed to close IPC clienti: %r", rc);
            }
        }
        else
        {
            ERROR("Failed to init IPC client in order "
                  "to invoke logger TA handler: %r", rc);
            del_ta_executive(name); /** Rollback */
        }
    }

    return rc;
}

/* See description in rcf_api.h */
te_errno
rcf_add_ta_unix(const char *name, const char *type,
                const char *host, uint16_t port,
                unsigned int copy_timeout,
                unsigned int kill_timeout,
                unsigned int flags)
{
    char confstr[RCF_MAX_VAL];
    char copy_timeout_str[RCF_MAX_ID];
    char kill_timeout_str[RCF_MAX_ID];
    int  n;

    /* Other parameters are checked within 'rcf_add_ta' */
    if (host == NULL)
        return TE_RC(TE_RCF_API, TE_EWRONGPTR);

#define SNPRINTF(buf_, fmt_...) \
    ((n = snprintf(buf_, sizeof(buf_), fmt_)) < 0 || \
     (unsigned)n > sizeof(buf_))
    if (SNPRINTF(copy_timeout_str,
                 copy_timeout ? "copy_timeout=%u:" : "", copy_timeout) ||
        SNPRINTF(kill_timeout_str,
                 kill_timeout ? "kill_timeout=%u:" : "", kill_timeout) ||
        SNPRINTF(confstr, "host=%s:port=%u:%s%s%s", host, port,
                 copy_timeout_str, kill_timeout_str,
                 flags & RCF_TA_UNIX_SUDO ? "sudo:" : ""))
#undef SNPRINTF
    {
        ERROR("Failed to form 'confstr' string");
        return n < 0 ? TE_OS_RC(TE_RCF_API, errno) :
                       TE_RC(TE_RCF_API, TE_EFAIL);
    }

    /**
     * Remove this specific flag (if present) since it is
     * already processed and is taken into account in 'confstr'
     */
    flags &= ~RCF_TA_UNIX_SUDO;

    /* Continue processing */
    return rcf_add_ta(name, type, rcfunix_name, confstr, flags);
}

/* See description in rcf_api.h */
te_errno
rcf_del_ta(const char *name)
{
    return del_ta_executive(name);
}

/* See description in rcf_api.h */
te_errno
rcf_get_ta_list(char *buf, size_t *len)
{
    rcf_msg     msg;
    rcf_msg    *ans;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

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
        /* There is no TA configured */
        *len = 0;
        return 0;
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

/* See description in rcf_api.h */
te_errno
rcf_ta_name2type(const char *ta_name, char *ta_type)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    if (ta_type == NULL || BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    memset(&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_TATYPE;
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    if (rc == 0 && (rc = msg.error) == 0)
        te_strlcpy(ta_type, msg.id, RCF_MAX_ID);

    return rc;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_create_session(const char *ta_name, int *session)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    if (session == NULL || BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    memset(&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_SESSION;
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    if (rc == 0 && (rc = msg.error) == 0)
        *session = msg.sid;

    return rc;
}

static int
rcf_reboot_type2reboot_type(rcf_reboot_type rt)
{
    switch (rt)
    {
        case RCF_REBOOT_TYPE_AGENT:
            return AGENT_REBOOT;

        case RCF_REBOOT_TYPE_WARM:
            return HOST_REBOOT;

        case RCF_REBOOT_TYPE_COLD:
            return COLD_REBOOT;

        case RCF_REBOOT_TYPE_FORCE:
            return AGENT_REBOOT | HOST_REBOOT | COLD_REBOOT;

        default:
            ERROR("Unsupported reboot type");
            return -1;
    }
}

/* See description in rcf_api.h */
te_errno
rcf_ta_reboot(const char *ta_name,
              const char *boot_params, const char *image,
              rcf_reboot_type reboot_type)
{
    rcf_msg    *msg;

    size_t      anslen = sizeof(*msg);
    size_t      len = 0;
    int         fd;
    te_errno    rc;

    rcf_reboot_type rt;

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
        te_strlcpy(msg->data, boot_params, len);
        msg->data_len = len;
    }
    te_strlcpy(msg->ta, ta_name, sizeof(msg->ta));

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
    rt = rcf_reboot_type2reboot_type(reboot_type);
    if (rt < 0)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    msg->flags |= rt;

    rc = send_recv_rcf_ipc_message(ctx_handle, msg, sizeof(rcf_msg) + len,
                                   msg, &anslen, NULL);

    if (rc == 0)
        rc = msg->error;

    free(msg);

    return rc;
}

/* See description in rcf_api.h */
void
rcf_log_cfg_changes(te_bool enable)
{
    thread_ctx_t *ctx_handle = get_ctx_handle(TRUE);

    if (ctx_handle != NULL)
        ctx_handle->log_cfg_changes = enable;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_cfg_get(const char *ta_name, int session, const char *oid,
               char *val_buf, size_t len)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    if (oid == NULL || val_buf == NULL || strlen(oid) >= RCF_MAX_ID ||
        BAD_TA)
    {
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    memset(&msg, 0, sizeof(msg));
    te_strlcpy(msg.id, oid, sizeof(msg.id));
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
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
                if (unlink(msg.file) != 0)
                    ERROR("Cannot unlink file %s saved by RCF process", msg.file);
                return TE_RC(TE_RCF_API, TE_ESMALLBUF);
            }
        }
        close(fd);
        if (unlink(msg.file) != 0)
            ERROR("Cannot unlink file %s saved by RCF process", msg.file);
    }
    else
    {
        if (len <= strlen(msg.value))
            return TE_RC(TE_RCF_API, TE_ESMALLBUF);
        te_strlcpy(val_buf, msg.value, len);
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
static te_errno
conf_add_set(const char *ta_name, int session, const char *oid,
             const char *val, int opcode)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    if (oid == NULL || val == NULL || strlen(oid) >= RCF_MAX_ID ||
        strlen(val) >= RCF_MAX_VAL || BAD_TA)
    {
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    memset(&msg, 0, sizeof(msg));
    te_strlcpy(msg.id, oid, sizeof(msg.id));
    te_strlcpy(msg.value, val, sizeof(msg.value));
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    msg.opcode = opcode;
    msg.sid = session;

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    if (rc == 0)
        rc = msg.error;

    if (ctx_handle->log_cfg_changes)
    {
        if (opcode == RCFOP_CONFSET)
            LOG_MSG(rc == 0 ? TE_LL_RING : TE_LL_ERROR,
                    "Set %s to %s: %r", oid, val, rc);
        else if (strlen(val) == 0)
            LOG_MSG(rc == 0 ? TE_LL_RING : TE_LL_ERROR,
                    "Add %s: %r", oid, rc);
        else
            LOG_MSG(rc == 0 ? TE_LL_RING : TE_LL_ERROR,
                    "Add %s with value %s: %r", oid, val, rc);

    }

    return rc;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_cfg_set(const char *ta_name, int session, const char *oid,
               const char *val)
{
    return conf_add_set(ta_name, session, oid, val, RCFOP_CONFSET);
}

/* See description in rcf_api.h */
te_errno
rcf_ta_cfg_add(const char *ta_name, int session, const char *oid,
               const char *val)
{
    /*
     * This function should accept @p val parameter with NULL value,
     * which might be used in adding an instance without value
     * (instance of NONE value type).
     */
    if (val == NULL)
        val = "";

    return conf_add_set(ta_name, session, oid, val, RCFOP_CONFADD);
}

/* See description in rcf_api.h */
te_errno
rcf_ta_cfg_del(const char *ta_name, int session, const char *oid)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    if (oid == NULL || strlen(oid) >= RCF_MAX_ID || BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    memset(&msg, 0, sizeof(msg));
    te_strlcpy(msg.id, oid, sizeof(msg.id));
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    msg.opcode = RCFOP_CONFDEL;
    msg.sid = session;

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    if (rc == 0)
        rc = msg.error;

    if (ctx_handle->log_cfg_changes)
        LOG_MSG(rc == 0 ? TE_LL_RING : TE_LL_ERROR,
                "Delete %s: %r", oid, rc);

    return rc;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_cfg_group(const char *ta_name, int session, te_bool is_start)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    memset(&msg, 0, sizeof(msg));
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    msg.sid = session;
    msg.opcode = (is_start) ? RCFOP_CONFGRP_START : RCFOP_CONFGRP_END;

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    return rc == 0 ? msg.error : rc;
}


/* See description in rcf_api.h */
te_errno
rcf_get_sniffer_dump(const char *ta_name, const char *snif_id,
                     char *fname, unsigned long long *offset)
{
    rcf_msg     msg;
    te_errno    rc;
    size_t      anslen = sizeof(msg);

    RCF_API_INIT;

    if (BAD_TA || fname == NULL || strlen(fname) == 0 ||
        snif_id == NULL || strlen(snif_id) >= RCF_MAX_VAL)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    memset(&msg, 0, sizeof(msg));

    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    te_strlcpy(msg.id, snif_id, sizeof(msg.id));

    msg.opcode = RCFOP_GET_SNIF_DUMP;
    msg.sid = RCF_TA_GET_LOG_SID;
    te_strlcpy(msg.file, fname, sizeof(msg.file));

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    if (rc == 0 && (rc = msg.error) == 0)
    {
        te_strlcpy(fname, msg.file, RCF_MAX_PATH);
        if (strlen(msg.value) == 0)
            return TE_RC(TE_RCF_API, TE_ENODATA);
        *offset = strtoll(msg.value, NULL, 10);
    }
    return rc;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_get_sniffers(const char *ta_name, const char *snif_id, char **buf,
                    size_t *len, te_bool sync)
{
    rcf_msg     msg;
    te_errno    rc;
    size_t      anslen   = sizeof(msg) + 4096;
    rcf_msg    *rep_msg;

    RCF_API_INIT;

    if (BAD_TA || (snif_id != NULL && strlen(snif_id) >= RCF_MAX_VAL))
        return TE_RC(TE_RCF_API, TE_EINVAL);

    rep_msg = malloc(anslen);
    memset(&msg, 0, sizeof(msg));
    rep_msg->data_len = 0;
    rep_msg->error    = 0;

    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));

    msg.opcode = RCFOP_GET_SNIFFERS;
    msg.sid = RCF_TA_GET_LOG_SID;
    if (snif_id == NULL)
        msg.intparm = sync;
    else
        te_strlcpy(msg.id, snif_id, sizeof(msg.id));

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   rep_msg, &anslen, NULL);
    if (rc == 0 && rep_msg->data_len > 0 && rep_msg->error == 0)
    {
        if (*len < rep_msg->data_len)
        {
            *buf = realloc(*buf, rep_msg->data_len);
            if (*buf == NULL)
            {
                *len = 0;
                free(rep_msg);
                return TE_RC(TE_RCF_PCH, TE_ENOMEM);
            }
        }
        memcpy(*buf, rep_msg->data, rep_msg->data_len);
        *len = rep_msg->data_len;
    }
    else
        *len = 0;
    if (rc == 0)
        rc = rep_msg->error;
    free(rep_msg);
    return rc;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_get_log(const char *ta_name, char *log_file)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    if (log_file == NULL || strlen(log_file) >= RCF_MAX_PATH || BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    memset(&msg, 0, sizeof(msg));
    te_strlcpy(msg.file, log_file, sizeof(msg.file));
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    msg.opcode = RCFOP_GET_LOG;
    msg.sid = RCF_TA_GET_LOG_SID;

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    if (rc == 0 && (rc = msg.error) == 0)
        te_strlcpy(log_file, msg.file, RCF_MAX_PATH);

    return rc;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_get_var(const char *ta_name, int session, const char *var_name,
               rcf_var_type_t var_type, size_t var_len, void *val)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    uint64_t    value;
    char       *tmp;
    te_errno    rc;

    RCF_API_INIT;

    if (var_name == NULL || val == NULL ||
        strlen(var_name) >= RCF_MAX_NAME ||
        BAD_TA || validate_type(var_type, var_len) != 0)
    {
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    memset(&msg, 0, sizeof(msg));
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    te_strlcpy(msg.id, var_name, sizeof(msg.id));
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
        te_strlcpy(val, msg.value, var_len);
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
            *(uint32_t *)val = (uint32_t)value;
            break;

        case RCF_INT64:
            *(int64_t *)val = (int64_t)value;
            break;

        case RCF_UINT64:
            *(int64_t *)val = value;
            break;

        default:
            assert(0);
    }

    return 0;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_set_var(const char *ta_name, int session, const char *var_name,
               rcf_var_type_t var_type, const char *val)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    if (var_name == NULL || val == NULL ||
        strlen(var_name) >= RCF_MAX_NAME ||
        BAD_TA || var_type >= RCF_TYPE_TOTAL)
    {
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    memset(&msg, 0, sizeof(msg));
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    te_strlcpy(msg.id, var_name, sizeof(msg.id));
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
            te_strlcpy(msg.value, val, sizeof(msg.value));
            break;

        default:
            assert(0);
    }

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    return rc == 0 ? msg.error : rc;
}

/**
 * Implementation of rcf_ta_(get|put|del)_file functionality -
 * see description of these functions for details.
 *
 * @param ta_name       Test Agent name
 * @param session       TA session or 0
 * @param rfile         full name of the file in the TA/NUT file system
 * @param lfile         full name of the file in the TN file system
 * @param opcode        RCFOP_FPUT, RCFOP_FGET or RCFOP_DEL
 *
 * @return error code
 */
static te_errno
handle_file(const char *ta_name, int session,
            const char *rfile, const char *lfile, int opcode)
{
    rcf_msg    *msg;
    size_t      anslen = sizeof(*msg);
    te_errno    rc;

    RCF_API_INIT;

#define BAD_RFILE \
    (rfile == NULL || strlen(rfile) >= RCF_MAX_PATH || strlen(rfile) == 0)

#define BAD_LFILE \
    ((lfile == NULL || strlen(lfile) >= RCF_MAX_PATH) || \
     (strlen(lfile) == 0 && opcode != RCFOP_FDEL))

#if 0
    if (rfile == NULL || lfile == NULL || strlen(lfile) >= RCF_MAX_PATH ||
        strlen(rfile) >= RCF_MAX_PATH ||
        (strlen(lfile) == 0 && opcode != RCFOP_FDEL) ||
        strlen(rfile) == 0 || BAD_TA)
#else
    if (BAD_RFILE || BAD_LFILE || BAD_TA)
#endif
    {
        ERROR("%s: Invalid arguments provided: "
              "BAD_TA: %s BAD_RFILE: %s BAD_LFILE: %s", __FUNCTION__,
              BAD_TA ? "T" : "F",
              BAD_RFILE ? "T" : "F",
              BAD_LFILE ? "T" : "F");
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    if (opcode == RCFOP_FPUT)
    {
        int fd;

        if ((fd = open(lfile, O_RDONLY)) < 0)
        {
            ERROR("Cannot open file %s for reading", lfile);
            return TE_RC(TE_RCF_API, TE_ENOENT);
        }
        close(fd);
    }

    msg = (rcf_msg *)calloc(1, sizeof(rcf_msg) + RCF_MAX_PATH);
    if (msg == NULL)
        return TE_RC(TE_RCF_API, TE_ENOMEM);

    msg->opcode = opcode;
    te_strlcpy(msg->ta, ta_name, sizeof(msg->ta));
    te_strlcpy(msg->file, lfile, sizeof(msg->file));
    te_strlcpy(msg->data, rfile, RCF_MAX_PATH);
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

#undef BAD_RFILE
#undef BAD_LFILE

    return rc;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_get_file(const char *ta_name, int session,
                const char *rfile, const char *lfile)
{
    return handle_file(ta_name, session, rfile, lfile, RCFOP_FGET);
}

/* See description in rcf_api.h */
te_errno
rcf_ta_put_file(const char *ta_name, int session,
                const char *lfile, const char *rfile)
{
    return handle_file(ta_name, session, rfile, lfile, RCFOP_FPUT);
}

/* See description in rcf_api.h */
te_errno
rcf_ta_del_file(const char *ta_name, int session, const char *rfile)
{
    return handle_file(ta_name, session, rfile, "", RCFOP_FDEL);
}


/* See description in rcf_api.h */
te_errno
rcf_tr_op_log(te_bool ring)
{
    if (rcf_tr_op_ring != ring)
    {
        RING("Turn RCF traffic operations logging %s",
             ring ? "ON" : "OFF");
        rcf_tr_op_ring = ring;
    }
    return 0;
}

/* See description in rcf_api.h */
te_bool
rcf_tr_op_log_get(void)
{
    return rcf_tr_op_ring;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_csap_create(const char *ta_name, int session,
                   const char *stack_id, const char *params,
                   csap_handle_t *csap_id)
{
    rcf_msg    *msg;
    size_t      len = 0;
    int         flags = 0;
    size_t      anslen = sizeof(*msg);
    te_errno    rc;

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
    te_strlcpy(msg->ta, ta_name, sizeof(msg->ta));
    te_strlcpy(msg->id, stack_id, sizeof(msg->id));
    if (flags & BINARY_ATTACHMENT)
        te_strlcpy(msg->file, params, sizeof(msg->file));
    else if (params != NULL)
    {
        msg->data_len = len;
        memcpy(msg->data, params, len);
    }

    rc = send_recv_rcf_ipc_message(ctx_handle,
                                   msg, sizeof(rcf_msg) + len,
                                   msg, &anslen, NULL);

    if (rc == 0 && (rc = msg->error) == 0)
    {
        *csap_id = msg->handle;
        LOG_MSG(rcf_tr_op_ring ? TE_LL_RING : TE_LL_INFO,
                "Created CSAP %u '%s' (%s:%d) with parameters:\n%Tf",
                msg->handle, stack_id, ta_name, session, params);
    }
    else
    {
        ERROR("Create CSAP '%s' (%s:%d) failed: %r\n%Tf",
              stack_id, ta_name, session, rc, params);
    }

    free(msg);

    return rc;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_csap_destroy(const char *ta_name, int session,
                    csap_handle_t csap_id)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    if (csap_id == CSAP_INVALID_HANDLE)
    {
        INFO("%s(): Called with CSAP_INVALID_HANDLE CSAP ID, IGNORE",
             __FUNCTION__);
        return 0;
    }

    if (BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    memset(&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_CSAP_DESTROY;
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    msg.sid = session;
    msg.handle = csap_id;

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    TE_RC_UPDATE(rc, msg.error);

    LOG_MSG((rc != 0) ? TE_LL_ERROR :
            (rcf_tr_op_ring ? TE_LL_RING : TE_LL_INFO),
            "Destroy CSAP %u (%s:%d): %r", csap_id, ta_name, session, rc);

    return rc == 0 ? msg.error : rc;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_csap_param(const char *ta_name, int session, csap_handle_t csap_id,
                  const char *var_name, size_t var_len, char *val)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    if (var_name == NULL || val == NULL ||
        strlen(var_name) >= RCF_MAX_NAME || BAD_TA)
    {
        ERROR("Invalid parameters");
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    memset(&msg, 0, sizeof(msg));
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    te_strlcpy(msg.id, var_name, sizeof(msg.id));
    msg.opcode = RCFOP_CSAP_PARAM;
    msg.sid = session;
    msg.handle = csap_id;

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    if (rc != 0 || (rc = msg.error) != 0)
        return rc;

    if (var_len <= strlen(msg.value))
        return TE_RC(TE_RCF_API, TE_ESMALLBUF);

    te_strlcpy(val, msg.value, var_len);

    return 0;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_trsend_start(const char *ta_name, int session,
                    csap_handle_t csap_id, const char *templ,
                    rcf_call_mode_t blk_mode)
{
    rcf_msg       msg;
    size_t        anslen = sizeof(msg);
    int           fd;
    te_errno      rc;

    RCF_API_INIT;

    if (templ == NULL || strlen(templ) >= RCF_MAX_PATH || BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    LOG_MSG(rcf_tr_op_ring ? TE_LL_RING : TE_LL_INFO,
            "Start %s send operation on the CSAP %d (%s:%d) with "
            "template:\n%Tf", rcf_call_mode2str(blk_mode), csap_id,
            ta_name, session, templ);

    memset(&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_TRSEND_START;
    msg.flags |= BINARY_ATTACHMENT;
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    te_strlcpy(msg.file, templ, sizeof(msg.file));
    msg.handle = csap_id;
    msg.intparm = (blk_mode == RCF_MODE_BLOCKING) ? TR_POSTPONED : 0;
    msg.sid = session;

    if ((fd = open(templ, O_RDONLY)) < 0)
    {
        ERROR("Cannot open file %s for reading\n", templ);
        return TE_OS_RC(TE_RCF_API, errno);
    }
    close(fd);

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    if (rc == 0)
        rc = msg.error;

    return rc;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_trsend_stop(const char *ta_name, int session,
                   csap_handle_t csap_id, int *num)
{
    rcf_msg       msg;
    size_t        anslen = sizeof(msg);
    te_errno      rc;

    RCF_API_INIT;

    if (BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    memset(&msg, 0, sizeof(msg));

    msg.sid = session;
    msg.opcode = RCFOP_TRSEND_STOP;
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    msg.handle = csap_id;

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    if (rc == 0 && (rc = msg.error) == 0 && (num != NULL))
        *num = msg.num;

    return rc;
}

/* See the description in rcf_api.h */
te_errno
rcf_ta_trrecv_start(const char *ta_name, int session,
                    csap_handle_t csap_id, const char *pattern,
                    unsigned int timeout, unsigned int num,
                    unsigned int mode)
{
    rcf_msg       msg;
    size_t        anslen = sizeof(msg);
    int           fd;
    te_errno      rc;
    unsigned int  report_flag;

    RCF_API_INIT;

    if (pattern == NULL || strlen(pattern) >= RCF_MAX_PATH || BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    if ((fd = open(pattern, O_RDONLY)) < 0)
    {
        ERROR("Cannot open file %s for reading", pattern);
        return TE_OS_RC(TE_RCF_API, errno);
    }
    close(fd);

    memset(&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_TRRECV_START;
    msg.flags |= BINARY_ATTACHMENT;
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    te_strlcpy(msg.file, pattern, sizeof(msg.file));
    msg.handle = csap_id;
    report_flag = mode & RCF_TRRECV_REPORT_MASK;
    msg.intparm = (report_flag == RCF_TRRECV_COUNT ? 0 : (TR_RESULTS |
                      (report_flag == RCF_TRRECV_NO_PAYLOAD ?
                          TR_NO_PAYLOAD : 0)));

    msg.intparm |= (mode & RCF_TRRECV_SEQ_MATCH) ? TR_SEQ_MATCH : 0;
    msg.intparm |= (mode & RCF_TRRECV_MISMATCH) ? TR_MISMATCH : 0;
    msg.sid = session;
    msg.num = num;
    msg.timeout = timeout;

    LOG_MSG(rcf_tr_op_ring ? TE_LL_RING : TE_LL_INFO,
            "Starting receive operation on the CSAP %d (%s:%d) "
            "timeout %u ms waiting for %u%s packets with pattern\n%Tf",
            csap_id, ta_name, session, timeout, num,
            (num == 0) ? "(unlimited)" : "", pattern);

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    if (rc == 0)
        rc = msg.error;

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
 *                      should be placed or NULL
 * @param opcode        RCFOP_TRRECV_STOP, RCFOP_TRRECV_WAIT or
 *                      RCFOP_TRRECV_GET
 *
 * @return error code
 */
static te_errno
csap_tr_recv_get(const char *ta_name, int session, csap_handle_t csap_id,
                 rcf_pkt_handler handler, void *user_param,
                 unsigned int *num, int opcode)
{
    te_errno                    rc;
    rcf_msg                     msg;
    size_t                      anslen = sizeof(msg);
    rcf_message_match_simple    match_data = { opcode, ta_name, session };

    RCF_API_INIT;

    if (BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    memset(&msg, 0, sizeof(msg));

    msg.sid = session;
    msg.opcode = opcode;
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    msg.handle = csap_id;

    anslen = sizeof(msg);
    if ((rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                        &msg, &anslen, NULL)) != 0)
    {
        ERROR("%s: IPC send with answer fails, rc %r",
              __FUNCTION__, rc);
        return rc;
    }

    while ((msg.flags & INTERMEDIATE_ANSWER))
    {
        assert(msg.file != NULL);

        LOG_MSG(rcf_tr_op_ring ? TE_LL_RING : TE_LL_INFO,
                "Traffic receive operation on the CSAP %d (%s:%d) got "
                 "packet\n%Tf", csap_id, ta_name, session, msg.file);
        if (handler != NULL)
            handler(msg.file, user_param);

        /*
         * Delete temporary file if it has not be removed or renamed by
         * the handler specified by the caller.
         */
        (void)unlink(msg.file);

        anslen = sizeof(msg);
        if ((rc = wait_rcf_ipc_message(ctx_handle->ipc_handle,
                                       &(ctx_handle->msg_buf_head),
                                       rcf_message_match, &match_data,
                                       &msg, &anslen, NULL)) != 0)
        {
            ERROR("%s: IPC receive answer fails, rc %r",
                  __FUNCTION__, rc);
            return TE_RC(TE_RCF_API, TE_EIPC);
        }
    }

    if ((num != NULL) && (msg.error == 0 || opcode != RCFOP_TRRECV_GET))
        *num = msg.num;

    return msg.error;
}

/* See the description in rcf_api.h */
te_errno
rcf_ta_trrecv_wait(const char *ta_name, int session,
                   csap_handle_t csap_id,
                   rcf_pkt_handler handler, void *user_param,
                   unsigned int *num)
{
    te_errno        rc;
    unsigned int    n = 0;

    LOG_MSG(rcf_tr_op_ring ? TE_LL_RING : TE_LL_INFO,
            "Waiting for receive operation on the CSAP %d (%s:%d) ...",
             csap_id, ta_name, session);

    rc = csap_tr_recv_get(ta_name, session, csap_id, handler,
                          user_param, &n, RCFOP_TRRECV_WAIT);

    LOG_MSG(rcf_tr_op_ring ? TE_LL_RING : TE_LL_INFO,
            "Finished receive operation on the CSAP %d (%s:%d) got %u "
            "packets : %r", csap_id, ta_name, session, n, rc);

    if (num != NULL)
        *num = n;

    return rc;
}

/* See the description in rcf_api.h */
te_errno
rcf_ta_trrecv_stop(const char *ta_name, int session,
                   csap_handle_t csap_id,
                   rcf_pkt_handler handler, void *user_param,
                   unsigned int *num)
{
    te_errno        rc;
    unsigned int    n = 0;

    LOG_MSG(rcf_tr_op_ring ? TE_LL_RING : TE_LL_INFO,
            "Stopping receive operation on the CSAP %d (%s:%d) ...",
            csap_id, ta_name, session);

    rc = csap_tr_recv_get(ta_name, session, csap_id, handler,
                          user_param, &n, RCFOP_TRRECV_STOP);

    LOG_MSG(rcf_tr_op_ring ? TE_LL_RING : TE_LL_INFO,
            "Stopped receive operation on the CSAP %d (%s:%d) got %u "
            "packets : %r", csap_id, ta_name, session, n, rc);

    if (num != NULL)
        *num = n;

    return rc;
}

/* See the description in rcf_api.h */
te_errno
rcf_ta_trrecv_get(const char *ta_name, int session,
                  csap_handle_t csap_id,
                  rcf_pkt_handler handler, void *user_param,
                  unsigned int *num)
{
    te_errno        rc;
    unsigned int    n = 0;

    VERB("%s(ta %s, csap %d, *num  %p) called",
         ta_name, csap_id, num);

    rc = csap_tr_recv_get(ta_name, session, csap_id, handler,
                          user_param, &n, RCFOP_TRRECV_GET);

    LOG_MSG(rcf_tr_op_ring ? TE_LL_RING : TE_LL_INFO,
            "Traffic receive operation on the CSAP %d (%s:%d) got %u "
            "packets : %r", csap_id, ta_name, session, n, rc);

    if (num != NULL && rc == 0)
        *num = n;

    return rc;
}


/* See description in rcf_api.h */
te_errno
rcf_ta_trsend_recv(const char *ta_name, int session, csap_handle_t csap_id,
                   const char *templ, rcf_pkt_handler handler,
                   void *user_param, unsigned int timeout, int *error)
{
    rcf_msg                     msg;
    size_t                      anslen = sizeof(msg);
    int                         fd;
    te_errno                    rc;
    rcf_message_match_simple    match_data = { RCFOP_TRSEND_RECV,
                                               ta_name, session };

    RCF_API_INIT;

    /* First, validate parameters */
    if (BAD_TA || templ == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    /* Third, check that template file is OK */
    if ((fd = open(templ, O_RDONLY)) < 0)
    {
        ERROR("Cannot open file %s for reading", templ);
        return TE_RC(TE_RCF_API, TE_ENOENT);
    }
    (void)close(fd);

    /* Fourth, compose the message */
    memset(&msg, 0, sizeof(msg));
    msg.sid = session;
    msg.num = 1; /* receive only one packet */
    msg.timeout = timeout;
    msg.opcode = RCFOP_TRSEND_RECV;
    msg.flags |= BINARY_ATTACHMENT;
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    te_strlcpy(msg.file, templ, sizeof(msg.file));
    msg.handle = csap_id;
    msg.intparm = handler == NULL ? 0 : TR_RESULTS;

    LOG_MSG(rcf_tr_op_ring ? TE_LL_RING : TE_LL_INFO,
            "Start send/receive operation on the CSAP %d (%s:%d) "
            "with timeout %u ms, handler=%p (param=%p), pattern:\n%Tf",
            csap_id, ta_name, session, timeout, handler, user_param, templ);

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    if (rc != 0)
        return rc;

    while (msg.flags & INTERMEDIATE_ANSWER)
    {
        assert(handler != NULL);
        assert(msg.file != NULL);

        LOG_MSG(rcf_tr_op_ring ? TE_LL_RING : TE_LL_INFO,
                "Traffic send/receive operation on the CSAP %d (%s:%d) "
                "got packet\n%Tf", csap_id, ta_name, session, msg.file);

        handler(msg.file, user_param);
        anslen = sizeof(msg);
        if ((rc = wait_rcf_ipc_message(ctx_handle->ipc_handle,
                                       &(ctx_handle->msg_buf_head),
                                       rcf_message_match, &match_data,
                                       &msg, &anslen, NULL)) != 0)
        {
            return rc;
        }
    }

    if (msg.error != 0)
        return msg.error;

    if (error != NULL)
        *error = msg.intparm;

    return 0;
}

/**
 * Send poll request to CSAP located on TA using specified RCF session
 * ID.
 *
 * @param ta_name       Test Agent name
 * @param session       RCF session
 * @param csap_id       CSAP handle
 * @param timeout       Timeout of the poll request (in milliseconds)
 * @param poll_id       Location for poll request ID (may be NULL, iff
 *                      timeout is equal to zero)
 *
 * @return Status code.
 */
static te_errno
rcf_ta_trpoll_start(const char *ta_name, int session,
                    csap_handle_t csap_id, unsigned int timeout,
                    unsigned int *poll_id)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    if (BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    if ((poll_id == NULL) && (timeout > 0))
    {
        ERROR("%s(): Location for poll ID may be NULL, iff timeout is "
              "zero", __FUNCTION__);
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    memset(&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_TRPOLL;
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    msg.sid = session;
    msg.handle = csap_id;
    msg.timeout = timeout;

    LOG_MSG(rcf_tr_op_ring ? TE_LL_RING : TE_LL_INFO,
            "Start poll operation on the CSAP %d (%s:%d) with timeout "
            "%u ms", csap_id, ta_name, session, timeout);

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
    if (rc != 0)
        return rc;

    if (msg.flags & INTERMEDIATE_ANSWER)
    {
        assert(poll_id != NULL);
        *poll_id = msg.intparm;
    }

    return msg.error;
}

/**
 * Cancel poll request to CSAP located on TA using specified RCF session
 * ID.
 *
 * @param ta_name       Test Agent name
 * @param session       RCF session
 * @param csap_id       CSAP handle
 * @param poll_id       Poll request ID returned by rcf_ta_trpoll_start()
 *
 * @return Status code.
 */
static te_errno
rcf_ta_trpoll_cancel(const char *ta_name, int session,
                     csap_handle_t csap_id, unsigned int poll_id)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    if (BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    if (poll_id == 0)
    {
        ERROR("%s(): Poll ID cannot be 0", __FUNCTION__);
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }

    memset(&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_TRPOLL_CANCEL;
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    msg.sid = session;
    msg.handle = csap_id;
    msg.intparm = poll_id;

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
    if (rc != 0)
        return rc;

    rc = msg.error;

    LOG_MSG(rcf_tr_op_ring ? TE_LL_RING : TE_LL_INFO,
            "Canceled poll operation #%u on the CSAP %d (%s:%d): %r",
            poll_id, csap_id, ta_name, session, rc);

    return rc;
}


/** Traffic poll operation internal data for each request. */
typedef struct rcf_trpoll_int {
    unsigned int    poll_id;    /**< Poll request ID returned from TA */
    int             start_sid;  /**< RCF session allocated for start */
    int             cancel_sid; /**< RCF session allocated for cancel */
} rcf_trpoll_int;

/** Opaque data of the rcf_message_match_poll callback. */
typedef struct rcf_message_match_poll_data {
    unsigned int     n_csaps;   /**< Number of CSAPs */
    rcf_trpoll_csap *csaps;     /**< Request data */
    rcf_trpoll_int  *internal;  /**< Poll internal data */
} rcf_message_match_poll_data;

/**
 */
static int
rcf_message_match_poll(rcf_msg *msg, void *opaque)
{
    rcf_message_match_poll_data *data = opaque;
    unsigned int                 i;
    rcf_message_match_simple     match_data;

    assert(msg != NULL);
    assert(data != NULL);

    match_data.opcode = RCFOP_TRPOLL;
    for (i = 0; i < data->n_csaps; ++i)
    {
        if (data->internal[i].poll_id == 0)
            continue;

        match_data.ta_name = data->csaps[i].ta;
        match_data.sid = data->internal[i].start_sid;
        if (rcf_message_match(msg, &match_data) == 0)
        {
            data->csaps[i].status = msg->error;
            data->internal[i].poll_id = 0;
            return 0;
        }
    }
    return 1;
}

/* See the description in rcf_api.h */
te_errno
rcf_trpoll(rcf_trpoll_csap *csaps, unsigned int n_csaps,
           unsigned int timeout)
{
    rcf_message_match_poll_data poll_match_data;

    unsigned int    i;
    rcf_trpoll_int *data;
    te_bool         cancel;
    te_errno        rc;
    unsigned int    n_active = 0;
    rcf_msg         msg;
    size_t          anslen;

    RCF_API_INIT;

    if (n_csaps == 0)
    {
        ERROR("%s(): No CSAPs to be polled", __FUNCTION__);
        return TE_RC(TE_RCF_API, TE_EINVAL);
    }
    if (csaps == NULL)
    {
        ERROR("%s(): Invalid pointer to array with per CSAP parameters",
              __FUNCTION__);
        return TE_RC(TE_RCF_API, TE_EFAULT);
    }

    data = calloc(n_csaps, sizeof(*data));
    if (data == NULL)
        return TE_RC(TE_RCF_API, TE_ENOMEM);

    /* Start poll operation for all CSAPs */
    for (cancel = FALSE, i = 0; i < n_csaps; ++i)
    {
        if (csaps[i].csap_id == CSAP_INVALID_HANDLE)
        {
            csaps[i].status = TE_RC(TE_RCF_API, TE_ETADCSAPNOTEX);
            continue;
        }

        csaps[i].status = rcf_ta_create_session(csaps[i].ta,
                                                &data[i].start_sid);
        if (csaps[i].status == 0)
        {
            csaps[i].status = rcf_ta_trpoll_start(csaps[i].ta,
                                                  data[i].start_sid,
                                                  csaps[i].csap_id,
                                                  timeout,
                                                  &data[i].poll_id);
        }
        if (csaps[i].status != 0 || data[i].poll_id == 0)
            cancel = TRUE;
        else
            n_active++;
    }

    poll_match_data.n_csaps = n_csaps;
    poll_match_data.csaps = csaps;
    poll_match_data.internal = data;

    while (!cancel && (n_active > 0))
    {
        anslen = sizeof(msg);
        rc = wait_rcf_ipc_message(ctx_handle->ipc_handle,
                                  &(ctx_handle->msg_buf_head),
                                  rcf_message_match_poll, &poll_match_data,
                                  &msg, &anslen, NULL);
        if (rc != 0)
        {
            cancel = TRUE;
        }
        else
        {
            n_active--;
            cancel = (TE_RC_GET_ERROR(msg.error) != TE_ETIMEDOUT);
        }
    }

    /* Cancel started poll operations */
    for (i = 0; i < n_csaps; ++i)
    {
        if (data[i].poll_id != 0)
        {
            rc = rcf_ta_create_session(csaps[i].ta, &data[i].cancel_sid);
            if (rc == 0)
            {
                rc = rcf_ta_trpoll_cancel(csaps[i].ta,
                                          data[i].cancel_sid,
                                          csaps[i].csap_id,
                                          data[i].poll_id);
            }
            else
            {
                /*
                 * Failed to allocate session, can't cancel poll
                 * request, however, it doesn't matter, since something
                 * critical has happened.
                 */
            }
            TE_RC_UPDATE(csaps[i].status, rc);
        }
    }

    /* Pick up final replies of started and not picked up poll operations */
    while (n_active > 0)
    {
        anslen = sizeof(msg);
        rc = wait_rcf_ipc_message(ctx_handle->ipc_handle,
                                  &(ctx_handle->msg_buf_head),
                                  rcf_message_match_poll, &poll_match_data,
                                  &msg, &anslen, NULL);
        if (rc == 0)
        {
            n_active--;
        }
        else
        {
            /* TODO: Try to understand usefull actions here */
        }
    }

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

            te_strlcpy(data, s, maxlen);
            data += strlen(s) + 1;
        }
        *data_len = len;
        return 0;
    }

    for (i = 0; i < argc; i++)
    {
        rcf_var_type_t  type = va_arg(ap, rcf_var_type_t);

        if (type >= RCF_TYPE_TOTAL)
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

            te_strlcpy(data, s, maxlen);
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
                    memcpy(data, &val, rcf_type_len[type]);
                    data += rcf_type_len[type];
                    break;
                }

                case RCF_INT16:
                case RCF_UINT16:
                {
                    uint16_t val = va_arg(ap, int);

                    memcpy(data, &val, rcf_type_len[type]);
                    data += rcf_type_len[type];
                    break;
                }

                case RCF_INT32:
                case RCF_UINT32:
                {
                    uint32_t val = va_arg(ap, int32_t);

                    memcpy(data, &val, rcf_type_len[type]);
                    data += rcf_type_len[type];
                    break;
                }

                case RCF_INT64:
                case RCF_UINT64:
                {
                    uint64_t val = va_arg(ap, int64_t);

                    memcpy(data, &val, rcf_type_len[type]);
                    data += rcf_type_len[type];
                    break;
                }

                default:
                    assert(0);
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
static te_errno
call_start(const char *ta_name, int session, int priority, const char *rtn,
           int *res, int argc, int argv, va_list ap,
           rcf_execute_mode mode)
{
    rcf_msg    *msg;
    size_t      anslen = sizeof(*msg);
    te_errno    rc;

    RCF_API_INIT;

    if (rtn == NULL || strlen(rtn) >= RCF_MAX_NAME || BAD_TA || res == NULL)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    if ((msg = (rcf_msg *)calloc(RCF_MAX_LEN, 1)) == NULL)
        return TE_RC(TE_RCF_API, TE_ENOMEM);

    msg->opcode = RCFOP_EXECUTE;
    msg->sid = session;
    te_strlcpy(msg->ta, ta_name, sizeof(msg->ta));
    te_strlcpy(msg->id, rtn, sizeof(msg->id));

    msg->intparm = mode;
    msg->num = priority;

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
        *res = (mode == RCF_FUNC ? msg->intparm : msg->handle);
        VERB("Call/start %s on the TA %s", rtn, ta_name);
    }

    free(msg);

    return rc;
}


/* See description in rcf_api.h */
te_errno
rcf_ta_call(const char *ta_name, int session, const char *rtn,
            te_errno *error, int argc, te_bool argv, ...)
{
    te_errno    rc;
    va_list     ap;

    va_start(ap, argv);

    rc = call_start(ta_name, session, 0, rtn, error, argc, argv, ap,
                    RCF_FUNC);

    va_end(ap);

    return rc;
}


/* See description in rcf_api.h */
te_errno
rcf_ta_start_task(const char *ta_name, int session, int priority,
                  const char *rtn, pid_t *pid, int argc, te_bool argv, ...)
{
    te_errno    rc;
    va_list     ap;

    va_start(ap, argv);

    rc = call_start(ta_name, session, priority, rtn, pid, argc, argv, ap,
                    RCF_PROCESS);

    va_end(ap);

    return rc;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_start_thread(const char *ta_name, int session, int priority,
                  const char *rtn, int *tid, int argc, te_bool argv, ...)
{
    te_errno    rc;
    va_list     ap;

    va_start(ap, argv);

    rc = call_start(ta_name, session, priority, rtn, tid, argc, argv, ap,
                    RCF_THREAD);

    va_end(ap);

    return rc;
}


/* See description in rcf_api.h */
te_errno
rcf_ta_kill_task(const char *ta_name, int session, pid_t pid)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    if (BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    memset(&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_KILL;
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    msg.handle = pid;
    msg.intparm = RCF_PROCESS;
    msg.sid = session;

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    return rc == 0 ? msg.error : rc;
}

/* See description in rcf_api.h */
te_errno
rcf_ta_kill_thread(const char *ta_name, int session, int tid)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    if (BAD_TA)
        return TE_RC(TE_RCF_API, TE_EINVAL);

    memset(&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_KILL;
    te_strlcpy(msg.ta, ta_name, sizeof(msg.ta));
    msg.handle = tid;
    msg.intparm = RCF_PROCESS;
    msg.sid = session;

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    return rc == 0 ? msg.error : rc;
}

/* See description in rcf_api.h */
te_errno
rcf_check_agent(const char *ta_name)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    memset(&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_TACHECK;
    te_strlcpy(msg.ta, ta_name, RCF_MAX_NAME);

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
    return rc == 0 ? msg.error : rc;
}

/* See description in rcf_api.h */
te_errno
rcf_check_agents(void)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    memset(&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_TACHECK;

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);
    return rc == 0 ? msg.error : rc;
}

/* See description in rcf_api.h */
te_errno
rcf_shutdown_call(void)
{
    rcf_msg     msg;
    size_t      anslen = sizeof(msg);
    te_errno    rc;

    RCF_API_INIT;

    memset(&msg, 0, sizeof(msg));
    msg.opcode = RCFOP_SHUTDOWN;

    rc = send_recv_rcf_ipc_message(ctx_handle, &msg, sizeof(msg),
                                   &msg, &anslen, NULL);

    return rc == 0 ? msg.error : rc;
}

/**
 * Send/receive RCF message (should not be used directly - only for
 * implementation of RCF API functions outside this C module).
 *
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
int
rcf_send_recv_msg(rcf_msg *send_buf, size_t send_size,
                  rcf_msg *recv_buf, size_t *recv_size, rcf_msg **p_answer)
{
    RCF_API_INIT;

    return send_recv_rcf_ipc_message(ctx_handle, send_buf, send_size,
                                     recv_buf, recv_size, p_answer);
}

/* See description in rcf_api.h */
te_errno
rcf_foreach_ta(rcf_ta_cb *cb, void *cookie)
{
    char agents[RCF_MAX_VAL];
    size_t len = sizeof(agents);
    size_t i;
    int rc;

    if (cb == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    rc = rcf_get_ta_list(agents, &len);
    if (rc != 0)
        return rc;

    for (i = 0; i < len; i += strlen(&agents[i]) + 1)
    {
        rc = cb(&agents[i], cookie);
        if (rc != 0)
            return rc;
    }

    return 0;
}

te_errno
rcf_get_dead_agents(te_vec *dead_agents)
{
    char agents[RCF_MAX_VAL];
    size_t len = sizeof(agents);
    size_t i;
    te_errno rc;

    rc = rcf_get_ta_list(agents, &len);
    if (rc != 0)
        return rc;

    for (i = 0; i < len; i += strlen(&agents[i]) + 1)
    {
        rc = rcf_check_agent(&agents[i]);
        if (rc == 0)
            continue;

        if (TE_RC_GET_ERROR(rc) == TE_ETADEAD)
        {
            rc = te_vec_append_str_fmt(dead_agents, "%s", &agents[i]);
            if (rc != 0)
                break;
        }
        else
        {
            break;
        }
    }

    return rc;
}