/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RCF Portable Command Handler
 *
 * RCF Portable Commands Handler implementation.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if defined(HAVE_PTHREAD_H) && defined(HAVE_PTHREAD_ATFORK)
#include <pthread.h>
#endif

#include "rcf_pch_internal.h"
#undef SEND_ANSWER

#include "te_errno.h"
#include "te_defs.h"
#include "te_stdint.h"
#include "te_str.h"
#include "rcf_common.h"
#include "rcf_internal.h"
#include "comm_agent.h"

#include "agentlib.h"
#include "rcf_pch.h"
#include "rcf_ch_api.h"
#include "rcf_pch_ta_cfg.h"
#include "logger_ta.h"

/** Include static array with string representation of types */
#define RCF_NEED_TYPES
#include "te_proto.h"
#undef RCF_NEED_TYPES


extern te_errno rcf_ch_get_sniffers(struct rcf_comm_connection *handle,
                                    char *cbuf, size_t buflen,
                                    size_t answer_plen,
                                    const char *sniff_id_str);
extern te_errno rcf_ch_get_snif_dump(struct rcf_comm_connection *handle,
                                     char *cbuf, size_t buflen,
                                     size_t answer_plen,
                                     const char *sniff_id_str);

/** Connection with the Test Engine */
static struct rcf_comm_connection *conn;

/* Buffer for raw log to be transmitted to the TEN */
static uint8_t log_data[RCF_PCH_LOG_BULK];

static char rcf_pch_id[RCF_PCH_MAX_ID_LEN];

/**
 * Get the rcf session identifier.
 *
 * @param id    Pointer to copy of RCF session indentifier (OUT).
 */
void
rcf_pch_get_id(char *id)
{
    if (id != NULL)
        te_strlcpy(id, rcf_pch_id, RCF_PCH_MAX_ID_LEN);
}

/**
 * Initialization of the rcf session identifier.
 *
 * @param port      RCF connection port.
 */
static void
rcf_pch_init_id(const char *port)
{
    int res;
    res = snprintf(rcf_pch_id, RCF_PCH_MAX_ID_LEN, "%u_%s", getuid(), port);
    if (res > RCF_PCH_MAX_ID_LEN)
        fprintf(stderr, "RCF session identifier is too long.");
}

/** See description in rcf_pch_internal.h */
void
write_str_in_quotes(char *dst, const char *src, size_t len)
{
    char   *p = dst;
    size_t  i;

    *p++ = ' ';
    *p++ = '\"';
    for (i = 0; (*src != '\0') && (i < len); ++i)
    {
        /* Encode '\n' also */
        if (*src == '\n')
        {
            *p++ ='\\';
            *p++ = 'n';
            src++;
            continue;
        }

        if (*src == '\"' || *src == '\\')
        {
            *p++ = '\\';
        }
        *p++ = *src++;
    }
    *p++ = '\"';
    *p = '\0';
}

/**
 * Parse the string stripping off quoting and escape symbols.
 * Parsed string is placed instead of old one. Pointer to next token
 * in the command line (or to end symbol). Pointer to the start
 * of parsed string is put to s.
 *
 * @param ptr   location of command pointer
 * @param str   location for parsed string beginning pointer
 *
 * @retval 0    success
 * @retval 1    no matching '\"' has been found
 */
static int
transform_str(char **ptr, char **str)
{
    char   *p = *ptr;
    char   *s;

    te_bool quotes = FALSE;

    SKIP_SPACES(p);
    s = p;
    *str = s;

    if (*p == '\"')
    {
        p++;
        quotes = TRUE;
    }

    while (*p != 0)
    {
        if (quotes && *p == '\\' &&
            (*(p + 1) == '\\' || *(p + 1) == '\"'))
            p++;
        else if (quotes && *p == '\"')
        {
            p++;
            break;
        }
        else if (!quotes && *p == ' ')
            break;

        if (*p == 0 && quotes)
            return 1;

        *s++ = *p++;
    }

    SKIP_SPACES(p);

    *s = 0;
    *ptr = p;

    return 0;
}

/**
 * Read type name and convert it to the type number.
 *
 * @param ptr           command buffer pointer (is updated by the routine)
 *
 * @return type number or RCF_TYPE_TOTAL
 */
static int
get_type(char **ptr)
{
     char *tmp;
     int   i;

     if (transform_str(ptr, &tmp) != 0)
         return RCF_TYPE_TOTAL;

     if (*ptr == 0)
         return RCF_TYPE_TOTAL;

     for (i = 0; i < RCF_TYPE_TOTAL; ++i)
         if (strcmp(rcf_types[i], tmp) == 0)
             break;

     return i;
}


/**
 * This routine parses routine parameters provided in the Test Protocol
 * commands start and execute.
 *
 * @param params        parameters string
 * @param is_argv       passing parameters mode location
 * @param argc          argc location
 * @param param         location for parameters
 *
 * @return Status code:
 * @retval 0               success
 * @retval TE_EINVAL       invalid command
 *
 * @alg It is assumed that all parameters are stored in the routine
 * stack as 32-bit words except int64_t and uint64_t which are stored
 * as two 32-bit words: most significant first for bin-endian architecture
 * and most significant second for little-endian architecture.
 */
static int
parse_parameters(char *params, te_bool *is_argv, int *argc, void **param)
{
    char *ptr = params;
    int   n = 0;

    ENTRY("params='%s' is_argv=%p argc=%p param=%p",
          params, is_argv, argc, param);

    memset((char *)param, 0, RCF_MAX_PARAMS * sizeof(void *));

    if (strncmp(ptr, "argv ", strlen("argv ")) == 0)
    {
        *is_argv = TRUE;
        ptr += strlen("argv ");
        SKIP_SPACES(ptr);
        while (*ptr != 0)
        {
            if (transform_str(&ptr, (char **)(param + n)) != 0)
                return TE_EINVAL;
            n++;
        }
        *argc = n;
        return 0;
    }
    else
    {
        *is_argv = FALSE;
        SKIP_SPACES(ptr);
        while (*ptr != 0)
        {
            uint64_t val;
            int      type;
            char    *tmp;

            if (n > RCF_MAX_PARAMS)
                return TE_EINVAL;

            if ((type = get_type(&ptr)) == RCF_TYPE_TOTAL)
                return TE_EINVAL;
            VERB("%s(): type is %d", __FUNCTION__, type);

            if (type == RCF_STRING)
            {
                if (transform_str(&ptr, (char **)(param + n)) != 0)
                    return TE_EINVAL;
                VERB("%s(): got string '%s'", __FUNCTION__, param[n]);
                n++;
                continue;
            }
            /* FIXME Use appropriate converter */
            val = strtoll(ptr, &tmp, 10);
            if (tmp == ptr || (*tmp != ' ' && *tmp != 0))
                return TE_EINVAL;
            VERB("%s(): got integer %d", __FUNCTION__, (int)val);

            ptr = tmp;
            SKIP_SPACES(ptr);

            if (type == RCF_INT64 || type == RCF_UINT64)
            {
                *(uint64_t *)(param + n) = val;
                n += sizeof(uint64_t) / sizeof(void *);
            }
            else
            {
#if (SIZEOF_VOID_P == 8)
                param[n] = (void *)val;
#else
                param[n] = (void *)(uint32_t)val;
#endif
                n++;
            }
        }
        *argc = n;
        return 0;
    }
}

/**
 * Read Test Protocol command and map it to operation code.
 *
 * @param ptr       - location of Test Protocol command pointer
 * @param opcode    - location for operation code
 *
 * @retval 0    - success
 * @retval 1    - error
 */
static int
get_opcode(char **ptr, rcf_op_t *opcode)
{
/**
 * Try to compare string name of the operation, set operation number
 * to *opcode, shift string pointer and return.
 *
 * @param cmd   - command name (suffix)
 */
#define TRY_CMD(cmd) \
    if (strncmp(TE_PROTO_##cmd, *ptr, strlen(TE_PROTO_##cmd)) == 0) \
    {                                                               \
        *opcode = RCFOP_##cmd;                                      \
        *ptr += strlen(TE_PROTO_##cmd);                             \
        if (**ptr != ' ' && **ptr != 0)                             \
            return 1;                                               \
        return 0;                                                   \
    }

    TRY_CMD(SHUTDOWN);
    TRY_CMD(REBOOT);
    TRY_CMD(CONFGET);
    TRY_CMD(CONFSET);
    TRY_CMD(CONFADD);
    TRY_CMD(CONFDEL);
    TRY_CMD(CONFGRP_START);
    TRY_CMD(CONFGRP_END);
    TRY_CMD(GET_LOG);
    TRY_CMD(VREAD);
    TRY_CMD(VWRITE);
    TRY_CMD(FPUT);
    TRY_CMD(FGET);
    TRY_CMD(FDEL);
    TRY_CMD(CSAP_CREATE);
    TRY_CMD(CSAP_PARAM);
    TRY_CMD(CSAP_DESTROY);
    TRY_CMD(TRSEND_START);
    TRY_CMD(TRSEND_STOP);
    TRY_CMD(TRRECV_START);
    TRY_CMD(TRRECV_STOP);
    TRY_CMD(TRRECV_GET);
    TRY_CMD(TRRECV_WAIT);
    TRY_CMD(TRSEND_RECV);
    TRY_CMD(TRPOLL);
    TRY_CMD(TRPOLL_CANCEL);
    TRY_CMD(EXECUTE);
    TRY_CMD(RPC);
    TRY_CMD(KILL);
    TRY_CMD(GET_SNIFFERS);
    TRY_CMD(GET_SNIF_DUMP);

#undef TRY_CMD

    return 1;
}

/**
 * Transmit log to the Test Engine.
 *
 * @param cbuf          command buffer
 * @param conn          connection handle
 * @param answer_plen   number of bytes to be copied from the command
 *                      to answer
 *
 * @return 0 or error returned by communication library
 */
static te_errno
transmit_log(struct rcf_comm_connection *conn, char *cbuf,
             size_t buflen, size_t answer_plen)
{
    size_t      len;
    te_errno    rc;
    int         ret;

    len = ta_log_get(sizeof(log_data), log_data);

    ret = snprintf(cbuf + answer_plen, buflen - answer_plen,
                   (len == 0) ? "%u" : "0 attach %u",
                   (len == 0) ? (unsigned)TE_RC(TE_RCF_PCH, TE_ENOENT) :
                                (unsigned)len);
    if ((size_t)ret >= (buflen - answer_plen))
    {
        ERROR("Command buffer too small");
        /* It MUST NOT happen */
        assert(FALSE);
    }

    RCF_CH_LOCK;
    rc = rcf_comm_agent_reply(conn, cbuf, strlen(cbuf) + 1);
    if (rc == 0)
    {
        rc = rcf_comm_agent_reply(conn, log_data, len);
    }
    RCF_CH_UNLOCK;

    return rc;
}

/** Detach from the Test Engine after fork() */
static void
rcf_pch_detach(void)
{
    rcf_comm_agent_close(&conn);
    rcf_pch_rpc_atfork();
}

static void *pch_vfork_saved_conn;

/** Detach from the Test Engine before vfork() */
static void
rcf_pch_detach_vfork(void)
{
    pch_vfork_saved_conn = conn;
    conn = NULL;
}

/** Attach to the Test Engine after vfork() in the parent process */
static void
rcf_pch_attach_vfork(void)
{
    /* Close connection created after vfork() but before exec(). */
    rcf_comm_agent_close(&conn);
    conn = pch_vfork_saved_conn;
}


/**
 * Start Portable Command Handler.
 *
 * @param confstr   configuration string for communication library
 * @param info      if not NULL, the string to be send to the engine
 *                  after initialisation
 *
 * @return Status code
 */
int
rcf_pch_run(const char *confstr, const char *info)
{
    char *cmd = NULL;
    int   rc = 0;
    int   sid = 0;
    int   cmd_buf_len = RCF_MAX_LEN;

    size_t   answer_plen = 0;
    rcf_op_t opcode = 0;
    te_errno rc2;

/**
 * Read any integer parameter from the command.
 *
 * @param n     - variable to save read number
 */
#define READ_INT(n) \
    do {                                                \
        char *tmp;                                      \
                                                        \
        SKIP_SPACES(ptr);                               \
        n = strtoll(ptr, &tmp, 10);                     \
        if (ptr == tmp || (*tmp != ' ' && *tmp != 0))   \
            goto bad_protocol;                          \
        ptr = tmp;                                      \
        SKIP_SPACES(ptr);                               \
    } while (FALSE)

/**
 * Read pending bytes, if necessary and send an answer to the RCF.
 *
 * @param _fmt  - answer format string
 */
#define SEND_ANSWER(_fmt...) \
    do {                                                            \
        size_t len;                                                 \
                                                                    \
        len = cmd_buf_len - answer_plen;                            \
        if (snprintf(cmd + answer_plen, len, _fmt) >= (int)len)     \
        {                                                           \
            ERROR("Answer truncated");                              \
            cmd[cmd_buf_len - 1] = '\0';                            \
        }                                                           \
        RCF_CH_LOCK;                                                \
        rc = rcf_comm_agent_reply(conn, cmd, strlen(cmd) + 1);      \
        RCF_CH_UNLOCK;                                              \
        if (rc != 0)                                                \
            goto communication_problem;                             \
    } while (FALSE)

    rcf_pch_init_id(confstr);

    VERB("Starting Portable Commands Handler");

    if (rcf_ch_init() != 0)
    {
        VERB("Initialization of CH library failed");
        goto exit;
    }
    rcf_pch_cfg_init();

    rc = rcf_ch_tad_init();
    if (TE_RC_GET_ERROR(rc) == TE_ENOSYS)
    {
        WARN("Traffic Application Domain operations are not supported");
    }
    else if (rc != 0)
    {
        ERROR("Traffic Application Domain initialization failed: %r", rc);
        /* Continue, but TAD operation will fail */
    }

    if ((cmd = (char *)malloc(RCF_MAX_LEN)) == NULL)
        return TE_RC(TE_RCF_PCH, TE_ENOMEM);

    if ((rc = rcf_comm_agent_init(confstr, &conn)) != 0 ||
        (info != NULL &&
         (rc = rcf_comm_agent_reply(conn, info, strlen(info) + 1)) != 0))
    {
        goto communication_problem;
    }

#if defined(HAVE_PTHREAD_ATFORK)
    pthread_atfork(NULL, NULL, rcf_pch_detach);
#endif
    register_vfork_hook(rcf_pch_detach_vfork, rcf_pch_attach_vfork,
                        rcf_pch_detach);

    while (TRUE)
    {
        size_t   len = cmd_buf_len;
        char    *ptr;
        void    *ba;         /* Binary attachment pointer */

        answer_plen = 0;

        if ((rc = rcf_comm_agent_wait(conn, cmd, &len, &ba)) != 0 &&
            TE_RC_GET_ERROR(rc) != TE_EPENDING)
            goto communication_problem;

        if (TE_RC_GET_ERROR(rc) == TE_EPENDING)
        {
            size_t tmp;

            int received = cmd_buf_len;
            int ba_offset = (ba == NULL) ? 0 :
                                ((uint8_t *)ba - (uint8_t *)cmd);

            char *old_cmd = cmd;

            if ((cmd = realloc(cmd, len)) == NULL)
            {
                old_cmd[128] = 0;

                LOG_PRINT("Failed to allocate enough memory for command <%s>",
                      old_cmd);

                free(old_cmd);
                return -1;
            }
            cmd_buf_len = len;
            tmp = len - received;
            if (ba_offset > 0)
                ba = (uint8_t *)cmd + ba_offset;

            if ((rc = rcf_comm_agent_wait(conn, cmd + received,
                                          &tmp, NULL)) != 0)
            {
                LOG_PRINT("Failed to read binary attachment for command <%s>",
                      cmd);
                goto communication_problem;
            }
        }
        VERB("Command <%s> is received", cmd);

        ptr = cmd;
        /* Skipping SID */
        if (strncmp(ptr, "SID ", strlen("SID ")) == 0)
        {
            ptr += strlen("SID ");

            READ_INT(sid);

            answer_plen = ptr - cmd;
        }

        if (get_opcode(&ptr, &opcode) != 0)
            goto bad_protocol;

        SKIP_SPACES(ptr);
        switch (opcode)
        {
            case RCFOP_SHUTDOWN:
                if (*ptr != 0 || ba != NULL)
                    goto bad_protocol;

                goto exit;
                break;

            case RCFOP_REBOOT:
            {
                char *params = NULL;

                if (*ptr != 0 && transform_str(&ptr, &params) != 0)
                    goto bad_protocol;

                if (rcf_ch_reboot(conn, cmd, cmd_buf_len, answer_plen,
                                  ba, len, params) < 0)
                {
                    ERROR("Reboot is NOT supported by CH");
                    SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_EOPNOTSUPP));
                }
                break;
            }

            case RCFOP_CONFGRP_START:
            case RCFOP_CONFGRP_END:
            {
                int op = (opcode == RCFOP_CONFGRP_START) ?
                             RCF_CH_CFG_GRP_START : RCF_CH_CFG_GRP_END;

                if (*ptr != 0)
                    goto bad_protocol;

                rc = rcf_ch_configure(conn, cmd, cmd_buf_len, answer_plen,
                                      ba, len, op, NULL, NULL);

                if (rc < 0)
                    rc = rcf_pch_configure(conn, cmd, cmd_buf_len,
                                           answer_plen, ba, len,
                                           op, NULL, NULL);

                if (rc != 0)
                    goto communication_problem;
                break;
            }

            case RCFOP_CONFGET:
            case RCFOP_CONFSET:
            case RCFOP_CONFADD:
            case RCFOP_CONFDEL:
            {
                int op = opcode == RCFOP_CONFGET ? RCF_CH_CFG_GET :
                         opcode == RCFOP_CONFSET ? RCF_CH_CFG_SET :
                         opcode == RCFOP_CONFADD ? RCF_CH_CFG_ADD :
                         RCF_CH_CFG_DEL;
                char *oid,
                     *val = NULL;

                if (*ptr == 0 || transform_str(&ptr, &oid) != 0)
                    goto bad_protocol;

                if (opcode == RCFOP_CONFGET || opcode == RCFOP_CONFDEL)
                {
                    if (*ptr != 0)
                        goto bad_protocol;
                }
                else if (*ptr == 0 && ba == NULL)
                {
                    if (opcode != RCFOP_CONFADD)
                        goto bad_protocol;

                    val = "";
                }
                else
                {
                    if (ba == NULL &&
                        (transform_str(&ptr, &val) != 0 || *ptr != 0))
                        goto bad_protocol;
                }

                rc = rcf_ch_configure(conn, cmd, cmd_buf_len, answer_plen,
                                      ba, len, op, oid, val);

                if (rc < 0)
                    rc = rcf_pch_configure(conn, cmd, cmd_buf_len,
                                           answer_plen, ba, len,
                                           op, oid, val);

                if (rc != 0)
                    goto communication_problem;
                break;
            }

            case RCFOP_GET_SNIF_DUMP:
             {
#ifndef WITH_SNIFFERS
                SEND_ANSWER("%d sniffers off",
                            TE_RC(TE_RCF_PCH, TE_ENOPROTOOPT));
                break;
#endif
                char       *var;
                int         rc;

                if (*ptr == 0 || ba != NULL ||
                    transform_str(&ptr, &var) != 0)
                    goto bad_protocol;

                rc = rcf_ch_get_snif_dump(conn, cmd, cmd_buf_len,
                                          answer_plen, var);
                if (rc == TE_RC(TE_RCF_PCH, TE_ENOPROTOOPT))
                {
                    SEND_ANSWER("%d sniffers off",
                                TE_RC(TE_RCF_PCH, TE_ENOPROTOOPT));
                }
                break;
            }

            case RCFOP_GET_SNIFFERS:
            {
#ifndef WITH_SNIFFERS
                SEND_ANSWER("%d sniffers off",
                            TE_RC(TE_RCF_PCH, TE_ENOPROTOOPT));
                break;
#endif
                char       *var;
                int         rc;

                if (*ptr == 0 || ba != NULL ||
                    transform_str(&ptr, &var) != 0)
                    goto bad_protocol;

                rc = rcf_ch_get_sniffers(conn, cmd, cmd_buf_len,
                                         answer_plen, var);
                if (rc == TE_RC(TE_RCF_PCH, TE_ENOPROTOOPT))
                {
                    SEND_ANSWER("%d sniffers off",
                                TE_RC(TE_RCF_PCH, TE_ENOPROTOOPT));
                }
                break;
            }

            case RCFOP_GET_LOG:
                if (*ptr != 0 || ba != NULL)
                    goto bad_protocol;

                rc = transmit_log(conn, cmd, cmd_buf_len, answer_plen);
                if (rc != 0)
                    goto communication_problem;

                break;

            case RCFOP_VREAD:
            case RCFOP_VWRITE:
            {
                char *var;
                int   type;

                if (*ptr == 0 || ba != NULL ||
                    transform_str(&ptr, &var) != 0)
                    goto bad_protocol;

                if (*ptr == 0)
                    type = RCF_STRING;
                else
                {
                    char *ptr0 = ptr;

                    if ((type = get_type(&ptr0)) == RCF_TYPE_TOTAL)
                        type = RCF_STRING;
                    else
                        ptr = ptr0;
                }

                if (opcode == RCFOP_VWRITE)
                {
                    char       *val_string = NULL;
                    uint64_t    val_int = 0;

                    if (type == RCF_STRING)
                    {
                        if (transform_str(&ptr, &val_string) != 0)
                            goto bad_protocol;
                    }
                    else
                    {
                        char *tmp;

                        val_int = strtoll(ptr, &tmp, 10);
                        if (tmp == ptr || (*tmp != ' ' && *tmp != 0))
                            goto bad_protocol;
                        ptr = tmp;
                        SKIP_SPACES(ptr);
                    }
                    if (*ptr != 0)
                        goto bad_protocol;

                    if (type == RCF_STRING)
                    {
                        rc = rcf_ch_vwrite(conn, cmd, cmd_buf_len,
                                           answer_plen, type, var,
                                           val_string);
                        if (rc < 0)
                            rc = rcf_pch_vwrite(conn, cmd, cmd_buf_len,
                                                answer_plen, type, var,
                                                val_string);
                    }
                    else
                    {
                        rc = rcf_ch_vwrite(conn, cmd, cmd_buf_len,
                                           answer_plen, type, var,
                                           val_int);
                        if (rc < 0)
                            rc = rcf_pch_vwrite(conn, cmd, cmd_buf_len,
                                                answer_plen, type, var,
                                                val_int);
                    }
                    if (rc != 0)
                        goto communication_problem;
                }
                else
                {
                    if (*ptr != 0)
                        goto bad_protocol;

                    rc = rcf_ch_vread(conn, cmd, cmd_buf_len,
                                      answer_plen, type, var);
                    if (rc < 0)
                        rc = rcf_pch_vread(conn, cmd, cmd_buf_len,
                                           answer_plen, type, var);
                    if (rc != 0)
                        goto communication_problem;
                }
                break;
            }

            case RCFOP_FPUT:
            case RCFOP_FGET:
            case RCFOP_FDEL:
            {
                char *filename;
                int   put = opcode == RCFOP_FPUT;

                if (*ptr == '\0' ||
                    transform_str(&ptr, &filename) != 0 ||
                    *ptr != '\0' ||
                    (put != (ba != NULL)))
                    goto bad_protocol;

                rc = rcf_ch_file(conn, cmd, cmd_buf_len, answer_plen,
                                 ba, len, opcode, filename);
                if (rc < 0)
                    rc = rcf_pch_file(conn, cmd, cmd_buf_len, answer_plen,
                                      ba, len, opcode, filename);

                if (rc != 0)
                    goto communication_problem;

                break;
            }

            case RCFOP_CSAP_CREATE:
            {
                char *params = NULL;
                char *stack;

                if (*ptr == 0 || transform_str(&ptr, &stack) != 0)
                    goto bad_protocol;

                if (ba == NULL)
                {
                    if (*ptr == 0 || transform_str(&ptr, &params) != 0 ||
                        *ptr != 0)
                        goto bad_protocol;
                }
                else
                {
                    if (*ptr != 0)
                        goto bad_protocol;
                }

                if (rcf_ch_csap_create(conn, cmd, cmd_buf_len, answer_plen,
                                       ba, len, stack, params) < 0)
                {
                    ERROR("CSAP stack %s (%s) is NOT supported", stack,
                          params);
                    SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_EOPNOTSUPP));
                }
                break;
            }

            case RCFOP_CSAP_PARAM:
            {
                int handle;
                char *var;

                if (*ptr == 0 || ba != NULL)
                    goto bad_protocol;

                READ_INT(handle);

                if (*ptr == 0 || transform_str(&ptr, &var) != 0 ||
                    *ptr != 0)
                    goto bad_protocol;

                if (rcf_ch_csap_param(conn, cmd, cmd_buf_len,
                                      answer_plen, handle, var) < 0)
                {
                    ERROR("CSAP parameter '%s' is NOT supported",
                                      var);
                    SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_EOPNOTSUPP));
                }

                break;
            }

            case RCFOP_CSAP_DESTROY:
            case RCFOP_TRSEND_STOP:
            case RCFOP_TRRECV_STOP:
            case RCFOP_TRRECV_WAIT:
            case RCFOP_TRRECV_GET:
            {
                int (*rtn)(struct rcf_comm_connection *, char *,
                           size_t, size_t, csap_handle_t) = NULL;
                int   handle;

                if (*ptr == 0 || ba != NULL)
                    goto bad_protocol;

                READ_INT(handle);
                if (*ptr != 0)
                    goto bad_protocol;

                switch (opcode)
                {
                    case RCFOP_CSAP_DESTROY:
                        rtn = rcf_ch_csap_destroy;
                        break;

                    case RCFOP_TRSEND_STOP:
                        rtn = rcf_ch_trsend_stop;
                        break;

                    case RCFOP_TRRECV_STOP:
                        rtn = rcf_ch_trrecv_stop;
                        break;

                    case RCFOP_TRRECV_GET:
                        rtn = rcf_ch_trrecv_get;
                        break;

                    case RCFOP_TRRECV_WAIT:
                        rtn = rcf_ch_trrecv_wait;
                        break;

                    default:
                        assert(FALSE);
                 }

                if (rtn(conn, cmd, cmd_buf_len, answer_plen, handle) < 0)
                    SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_EOPNOTSUPP));

                break;
            }

            case RCFOP_TRPOLL:
            case RCFOP_TRPOLL_CANCEL:
            {
                int (*rtn)(struct rcf_comm_connection *, char *,
                           size_t, size_t, csap_handle_t, unsigned int);

                int   handle;
                int   intparam;

                if (*ptr == 0 || ba != NULL)
                    goto bad_protocol;

                READ_INT(handle);
                READ_INT(intparam);
                if (*ptr != 0)
                    goto bad_protocol;

                switch (opcode)
                {
                    case RCFOP_TRPOLL:
                        rtn = rcf_ch_trpoll;
                        break;

                    case RCFOP_TRPOLL_CANCEL:
                        rtn = rcf_ch_trpoll_cancel;
                        break;

                    default:
                        assert(FALSE);
                        rtn = NULL;
                 }

                if (rtn(conn, cmd, cmd_buf_len, answer_plen,
                        handle, intparam) < 0)
                    SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_EOPNOTSUPP));

                break;
            }

            case RCFOP_TRSEND_START:
            {
                int handle;
                int postponed = 0;

                if (*ptr == 0 || ba == NULL)
                    goto bad_protocol;

                READ_INT(handle);
                if (strcmp_start("postponed", ptr) == 0)
                {
                    postponed = 1;
                    ptr += strlen("postponed");
                    SKIP_SPACES(ptr);
                }
                if (*ptr != 0)
                    goto bad_protocol;

                if (rcf_ch_trsend_start(conn, cmd, cmd_buf_len,
                                        answer_plen, ba, len, handle,
                                        postponed) < 0)
                {
                    ERROR("rcf_ch_trsend_start() returns - "
                                      "no support");
                    SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_EOPNOTSUPP));
                }

                break;
            }

            case RCFOP_TRRECV_START:
            {
                int          handle;
                int          num = 1;
                unsigned int timeout = TAD_TIMEOUT_INF;
                unsigned int mode = 0;

                if (*ptr == 0 || ba == NULL)
                    goto bad_protocol;

                READ_INT(handle);
                READ_INT(num);
                READ_INT(timeout);

                if (strncmp(ptr, "results", strlen("results")) == 0)
                {
                    mode |= RCF_CH_TRRECV_PACKETS;
                    ptr += strlen("results");
                    SKIP_SPACES(ptr);
                    if (strncmp(ptr, "no-payload",
                                strlen("no-payload")) == 0)
                    {
                        mode |= RCF_CH_TRRECV_PACKETS_NO_PAYLOAD;
                        ptr += strlen("no-payload");
                        SKIP_SPACES(ptr);
                    }
                }

                if (strncmp(ptr, "seq-match", strlen("seq-match")) == 0)
                {
                    mode |= RCF_CH_TRRECV_PACKETS_SEQ_MATCH;
                    ptr += strlen("seq-match");
                    SKIP_SPACES(ptr);
                }

                if (strncmp(ptr, "mismatch", strlen("mismatch")) == 0)
                {
                    mode |= RCF_CH_TRRECV_MISMATCH;
                    ptr += strlen("mismatch");
                    SKIP_SPACES(ptr);
                }

                if (*ptr != 0)
                    goto bad_protocol;

                if (rcf_ch_trrecv_start(conn, cmd, cmd_buf_len,
                                        answer_plen, ba, len, handle,
                                        num, timeout, mode) < 0)
                {
                    ERROR("rcf_ch_trrecv_start() returns - no support");
                    SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_EOPNOTSUPP));
                }

                break;
            }

            case RCFOP_TRSEND_RECV:
            {
                int             handle;
                int             timeout;
                unsigned int    mode = 0;

                if (*ptr == 0 || ba == NULL)
                    goto bad_protocol;

                READ_INT(handle);
                READ_INT(timeout);

                if (strcmp_start("results", ptr) == 0)
                {
                    mode |= RCF_CH_TRRECV_PACKETS;
                    ptr += strlen("results");
                    SKIP_SPACES(ptr);
                }

                if (*ptr != 0)
                    goto bad_protocol;

                if (rcf_ch_trsend_recv(conn, cmd, cmd_buf_len,
                                       answer_plen, ba, len, handle,
                                       timeout, mode) < 0)
                {
                    ERROR("rcf_ch_trsend_recv() returns - no support");
                    SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_EOPNOTSUPP));
                }

                break;
            }

            case RCFOP_EXECUTE:
            {
                void    *param[RCF_MAX_PARAMS];
                char    *rtn;
                int      argc;
                te_bool  is_argv;
                int      priority = -1;

                rcf_execute_mode mode;

                if (strcmp_start(TE_PROTO_FUNC " ", ptr) == 0)
                {
                    mode = RCF_FUNC;
                    ptr += strlen(TE_PROTO_FUNC);
                }
                else if(strcmp_start(TE_PROTO_THREAD " ", ptr) == 0)
                {
                    mode = RCF_THREAD;
                    ptr += strlen(TE_PROTO_THREAD);
                }
                else if(strcmp_start(TE_PROTO_PROCESS " ", ptr) == 0)
                {
                    mode = RCF_PROCESS;
                    ptr += strlen(TE_PROTO_PROCESS);
                }
                else
                {
                    goto bad_protocol;
                }
                SKIP_SPACES(ptr);

                if (*ptr == 0 || ba != NULL ||
                    transform_str(&ptr, &rtn) != 0)
                {
                    goto bad_protocol;
                }

                if (isdigit(*ptr))
                    READ_INT(priority);

                if (parse_parameters(ptr, &is_argv, &argc, param) != 0)
                {
                    goto bad_protocol;
                }

                switch(mode)
                {
                    case RCF_FUNC:
                    {
                        rc = rcf_ch_call(conn, cmd, cmd_buf_len,
                                         answer_plen,
                                         rtn, is_argv, argc, param);
                        if (rc < 0)
                            rc = rcf_pch_call(conn, cmd, cmd_buf_len,
                                              answer_plen,
                                              rtn, is_argv, argc, param);

                        if (rc != 0)
                            goto communication_problem;

                        break;
                    }

                    case RCF_PROCESS:
                    {
                        pid_t pid;

                        if ((rc = rcf_ch_start_process(&pid, priority,
                                                       rtn, is_argv,
                                                       argc, param)) != 0)
                        {
                            SEND_ANSWER("%d", rc);
                        }
                        else
                        {
                            SEND_ANSWER("0 %ld", (long)pid);
                        }

                        break;
                    }

                    case RCF_THREAD:
                    {
                        int tid;

                        if ((rc = rcf_ch_start_thread(&tid, priority,
                                                      rtn, is_argv,
                                                      argc, param)) != 0)
                        {
                            SEND_ANSWER("%d", rc);
                        }
                        else
                        {
                            SEND_ANSWER("0 %d", tid);
                        }

                        break;
                    }

                    default:
                        SEND_ANSWER("%d", TE_RC(TE_RCF_PCH, TE_EOPNOTSUPP));
                }
                break;
            }

            case RCFOP_RPC:
            {
                char    *server;
                uint32_t timeout;

                if (*ptr == 0 || transform_str(&ptr, &server) != 0)
                    goto bad_protocol;

                READ_INT(timeout);

                if (ba != NULL)
                {
                    len -= ((uint8_t *)ba - (uint8_t *)cmd);
                    ptr = (char *)ba;
                }
                else
                {
                    /* XML */
                    char *tmp;

                    if (transform_str(&ptr, &tmp) != 0)
                        goto bad_protocol;
                    ptr = tmp;
                    len = strlen(ptr);
                }

                rc = rcf_pch_rpc(conn, sid, ptr, len, server, timeout);

                if (rc != 0)
                     goto communication_problem;

                break;
            }

            case RCFOP_KILL:
            {
                unsigned int pid;

                rcf_execute_mode mode;

                if (*ptr == 0 || ba != NULL)
                    goto bad_protocol;

                if(strcmp_start(TE_PROTO_THREAD " ", ptr) == 0)
                {
                    mode = RCF_THREAD;
                    ptr += strlen(TE_PROTO_THREAD);
                }
                else if(strcmp_start(TE_PROTO_PROCESS " ", ptr) == 0)
                {
                    mode = RCF_PROCESS;
                    ptr += strlen(TE_PROTO_PROCESS);
                }
                else
                {
                    goto bad_protocol;
                }
                SKIP_SPACES(ptr);

                READ_INT(pid);
                if (*ptr != 0)
                    goto bad_protocol;

                if (mode == RCF_PROCESS)
                    SEND_ANSWER("%d", rcf_ch_kill_process(pid));
                else
                    SEND_ANSWER("%d", rcf_ch_kill_thread(pid));

                break;
            }

            default:
                assert(FALSE);
        }
        continue;

    bad_protocol:
        ERROR("Bad protocol command <%s> is received", cmd);
        SEND_ANSWER("%d bad command", TE_RC(TE_RCF_PCH, TE_EFMT));
    }

communication_problem:
    ERROR("Fatal communication error %r", rc);
    LOG_PRINT("Fatal communication error %s", te_rc_err2str(rc));

exit:
    rc2 = rcf_ch_tad_shutdown();
    if (rc2 != 0)
    {
        ERROR("Traffic Application Domain shutdown failed: %r", rc2);
        LOG_PRINT("Traffic Application Domain shutdown failed: %s",
              te_rc_err2str(rc2));
        TE_RC_UPDATE(rc, rc2);
    }
    rcf_ch_conf_fini();
    ta_obj_cleanup();
    rcf_pch_rpc_shutdown();
    if (opcode == RCFOP_SHUTDOWN &&
        rcf_ch_shutdown(conn, cmd, cmd_buf_len, answer_plen) < 0)
    {
        SEND_ANSWER("0");
    }
    rcf_comm_agent_close(&conn);
    free(cmd);

    VERB("Exiting");
    LOG_PRINT("Exiting: %d", rc);

    return rc;

#undef READ_INT
#undef SEND_ANSWER
}

