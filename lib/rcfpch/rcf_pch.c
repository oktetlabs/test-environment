/** @file
 * @brief RCF Portable Command Handler
 *
 * RCF Portable Commands Handler implementation.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

#include "te_errno.h"
#include "te_defs.h"
#include "te_stdint.h"
#include "rcf_common.h"
#include "rcf_internal.h"
#include "comm_agent.h"
#include "rcf_pch.h"
#include "rcf_ch_api.h"

#include "rcf_pch_internal.h"
#undef SEND_ANSWER

/** Include static array with string representation of types */
#define RCF_NEED_TYPES
#include "te_proto.h"
#undef RCF_NEED_TYPES


/* Buffer for raw log to be transmitted to the TEN */
static char log_data[RCF_PCH_LOG_BULK];

/**
 * Parse the string stripping off quoting and excape symbols.
 * Parsed string is placed instead of old one. Pointer to next token
 * in the command line (or to end symbol). Pointer to the start
 * of parsed string is put to s.
 *
 * @param ptr   - location of command pointer
 * @param str   - location for parsed string beginning pointer
 *
 * @retval 0    - success
 * @retval 1    - no matching '\"' has been found
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
 * @return error code
 * @retval 0            success
 * @retval EINVAL       invalid command
 *
 * @alg It is assumed that all parameters are stored in the routine
 * stack as 32-bit words except int64_t and uint64_t which are stored
 * as two 32-bit words: most significant first for bin-endian architecture
 * and most significant second for little-endian architecture.
 */
static int
parse_parameters(char *params, te_bool *is_argv, int *argc, uint32_t *param)
{
    char *ptr = params;
    int   n = 0;

    memset((char *)param, 0, RCF_MAX_PARAMS * sizeof(uint32_t));

    if (strncmp(ptr, "argv ", strlen("argv ")) == 0)
    {
        *is_argv = TRUE;
        ptr += strlen("argv ");
        SKIP_SPACES(ptr);
        while (*ptr != 0)
        {
            if (transform_str(&ptr, (char **)(param + n)) != 0)
                return EINVAL;
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
                return EINVAL;

            if ((type = get_type(&ptr)) == RCF_TYPE_TOTAL)
                return EINVAL;

            if (type == RCF_STRING)
            {
                if (transform_str(&ptr, (char **)(param + n)) != 0)
                    return EINVAL;
                n++;
                continue;
            }
            val = strtoll(ptr, &tmp, 10);
            if (tmp == ptr || (*tmp != ' ' && *tmp != 0))
                return EINVAL;

            ptr = tmp;
            SKIP_SPACES(ptr);

            if (type == RCF_INT64 || type == RCF_UINT64)
            {
                *(uint64_t *)(param + n) = val;
                n += 2;
            }
            else
            {
                param[n] = (uint32_t)val;
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
    TRY_CMD(START);
    TRY_CMD(EXECUTE);
    TRY_CMD(KILL);

#undef TRY_COMMAND

    return 1;
}

/**
 * Transmit log to the Test Engine.
 *
 * @param cbuf          command buffer
 * @param conn          connection handle
 * @param answer_plen   number of bytes to be copied from the command to answer
 *
 * @return 0 or error returned by communication library
 */
static int
transmit_log(struct rcf_comm_connection *conn, char *cbuf,
             size_t buflen, size_t answer_plen)
{
    int len;
    int rc;

    len = log_get(sizeof(log_data), log_data);

    if ((size_t)snprintf(cbuf + answer_plen, buflen - answer_plen,
                         "0 attach %d", len) >= (buflen - answer_plen))
    {
        ERROR("Command buffer too small");
        /* It MUST NOT happen */
        assert(FALSE);
    }

    rcf_ch_lock();
    rc = rcf_comm_agent_reply(conn, cbuf, strlen(cbuf) + 1);
    if (rc == 0)
    {
        rc = rcf_comm_agent_reply(conn, log_data, len);
    }
    rcf_ch_unlock();

    return rc;
}

/**
 * Start Portable Command Handler.
 *
 * @param confstr   configuration string for communication library
 * @param info      if not NULL, the string to be send to the engine
 *                  after initialisation
 *
 * @return error code
 */
int
rcf_pch_run(const char *confstr, const char *info)
{
    struct rcf_comm_connection *conn;

    char   *cmd = NULL;
    int     rc;
    
    te_bool  pending = FALSE;

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
        int len;                                                    \
                                                                    \
        while (pending)                                             \
        {                                                           \
            len = RCF_MAX_LEN - answer_plen;                        \
            if ((rc = rcf_comm_agent_wait(conn, cmd + answer_plen,  \
                                          &len, NULL)) != 0 &&      \
                rc != ETEPENDING)                                   \
                goto communication_problem;                         \
            pending = (rc == ETEPENDING);                           \
        }                                                           \
        len = RCF_MAX_LEN - answer_plen;                            \
        if (snprintf(cmd + answer_plen, len, _fmt) >= len)          \
        {                                                           \
            ERROR("Answer truncated");                  \
            cmd[RCF_MAX_LEN - 1] = '\0';                            \
        }                                                           \
        rcf_ch_lock();                                              \
        rc = rcf_comm_agent_reply(conn, cmd, strlen(cmd) + 1);      \
        rcf_ch_unlock();                                            \
        if (rc != 0)                                                \
            goto communication_problem;                             \
    } while (FALSE)

    VERB("Starting Portable Commands Handler");

    if (rcf_ch_init() != 0)
    {
        VERB("Initialization of CH library failed");
        goto exit;
    }
    rcf_pch_cfg_init();

    if ((cmd = (char *)malloc(RCF_MAX_LEN)) == NULL)
        return ENOMEM;

    if ((rc = rcf_comm_agent_init(confstr, &conn)) != 0 ||
        (info != NULL &&
         (rc = rcf_comm_agent_reply(conn, info, strlen(info) + 1)) != 0))
    {
        goto communication_problem;
    }

    while (1)
    {
        size_t   len = RCF_MAX_LEN;
        size_t   answer_plen = 0;   /* Length of data to be copied to answer */
        rcf_op_t opcode;

        char   *ptr = cmd;
        char   *ba;         /* Binary attachment pointer */


        if ((rc = rcf_comm_agent_wait(conn, cmd, &len, &ba)) != 0 &&
            rc != ETEPENDING)
            goto communication_problem;

        pending = (rc == ETEPENDING);

        VERB("Command <%s> is received", cmd);

        /* Skipping SID */
        if (strncmp(ptr, "SID ", strlen("SID ")) == 0)
        {
            int sid;

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

                if (rcf_ch_shutdown(conn, cmd, RCF_MAX_LEN, answer_plen) < 0)
                    SEND_ANSWER("0");
                goto exit;
                break;

            case RCFOP_REBOOT:
            {
                char *params = NULL;

                if (*ptr != 0 && transform_str(&ptr, &params) != 0)
                    goto bad_protocol;

                if (rcf_ch_reboot(conn, cmd, RCF_MAX_LEN, answer_plen,
                                  ba, len, params) < 0)
                {
                    ERROR("Reboot is NOT supported by CH");
                    SEND_ANSWER("%d", EOPNOTSUPP);
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

                rc = rcf_ch_configure(conn, cmd, RCF_MAX_LEN, answer_plen,
                                      ba, len, op, NULL, NULL);

                if (rc < 0)
                    rc = rcf_pch_configure(conn, cmd, RCF_MAX_LEN,
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

                rc = rcf_ch_configure(conn, cmd, RCF_MAX_LEN, answer_plen,
                                      ba, len, op, oid, val);

                if (rc < 0)
                    rc = rcf_pch_configure(conn, cmd, RCF_MAX_LEN,
                                           answer_plen, ba, len, 
                                           op, oid, val);

                if (rc != 0)
                    goto communication_problem;
                break;
            }

            case RCFOP_GET_LOG:
                if (*ptr != 0 || ba != NULL)
                    goto bad_protocol;

                rc = transmit_log(conn, cmd, RCF_MAX_LEN, answer_plen);
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
                        rc = rcf_ch_vwrite(conn, cmd, RCF_MAX_LEN,
                                           answer_plen, type, var,
                                           val_string);
                        if (rc < 0)
                            rc = rcf_pch_vwrite(conn, cmd, RCF_MAX_LEN,
                                                answer_plen, type, var,
                                                val_string);
                    }
                    else
                    {
                        rc = rcf_ch_vwrite(conn, cmd, RCF_MAX_LEN,
                                           answer_plen, type, var,
                                           val_int);
                        if (rc < 0)
                            rc = rcf_pch_vwrite(conn, cmd, RCF_MAX_LEN,
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

                    rc = rcf_ch_vread(conn, cmd, RCF_MAX_LEN,
                                      answer_plen, type, var);
                    if (rc < 0)
                        rc = rcf_pch_vread(conn, cmd, RCF_MAX_LEN, 
                                           answer_plen, type, var);
                    if (rc != 0)
                        goto communication_problem;
                }
                break;
            }

            case RCFOP_FPUT:
            case RCFOP_FGET:
            {
                char *filename;
                int   put = opcode == RCFOP_FPUT;

                if (*ptr == 0 || transform_str(&ptr, &filename) != 0 ||
                    *ptr != 0 || (put != (ba != 0)))
                    goto bad_protocol;

                rc = rcf_ch_file(conn, cmd, RCF_MAX_LEN, answer_plen,
                                 ba, len, put, filename);
                if (rc < 0)
                    rc = rcf_pch_file(conn, cmd, RCF_MAX_LEN, answer_plen,
                                      ba, len, put, filename);

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

                if (rcf_ch_csap_create(conn, cmd, RCF_MAX_LEN, answer_plen,
                                       ba, len, stack, params) < 0)
                {
                    ERROR("rcfpch", "CSAP stack %s (%s) is NOT supported",
                                      stack, params);
                    SEND_ANSWER("%d", EOPNOTSUPP);
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

                if (*ptr == 0 || transform_str(&ptr, &var) != 0 || *ptr != 0)
                    goto bad_protocol;

                if (rcf_ch_csap_param(conn, cmd, RCF_MAX_LEN, 
                                      answer_plen, handle, var) < 0)
                {
                    ERROR("CSAP parameter '%s' is NOT supported",
                                      var);
                    SEND_ANSWER("%d", EOPNOTSUPP);
                }

                break;
            }

            case RCFOP_CSAP_DESTROY:
            case RCFOP_TRSEND_STOP:
            case RCFOP_TRRECV_STOP:
            case RCFOP_TRRECV_WAIT:
            case RCFOP_TRRECV_GET:
            {
                int handle;
                int (*rtn)(struct rcf_comm_connection *, char *,
                           size_t, size_t, csap_handle_t);

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

                if (rtn(conn, cmd, RCF_MAX_LEN, answer_plen, handle) < 0)
                    SEND_ANSWER("%d", EOPNOTSUPP);

                break;
            }

            case RCFOP_TRSEND_START:
            {
                int handle;
                int postponed = 0;

                if (*ptr == 0 || ba == NULL)
                    goto bad_protocol;

                READ_INT(handle);
                if (strncmp(ptr, "postponed", strlen("postponed")) == 0)
                {
                    postponed = 1;
                    ptr += strlen("postponed");
                    SKIP_SPACES(ptr);
                }
                if (*ptr != 0)
                    goto bad_protocol;

                if (rcf_ch_trsend_start(conn, cmd, RCF_MAX_LEN,
                                        answer_plen, ba, len, handle,
                                        postponed) < 0)
                {
                    ERROR("rcf_ch_trsend_start() returns - "
                                      "no support");
                    SEND_ANSWER("%d", EOPNOTSUPP);
                }

                break;
            }

            case RCFOP_TRRECV_START:
            {
                int          handle;
                int          num = 1;
                unsigned int timeout = TAD_TIMEOUT_INF;
                te_bool      results = FALSE;


                if (*ptr == 0 || ba == NULL)
                    goto bad_protocol;

                READ_INT(handle);

                READ_INT(num);

                if (isdigit(*ptr))
                    READ_INT(timeout);

                if (strncmp(ptr, "results", strlen("results")) == 0)
                {
                    results = 1;
                    ptr += strlen("results");
                    SKIP_SPACES(ptr);
                }

                if (*ptr != 0)
                    goto bad_protocol;

                if (rcf_ch_trrecv_start(conn, cmd, RCF_MAX_LEN, 
                                        answer_plen, ba, len, handle,
                                        num, results, timeout) < 0)
                {
                    ERROR("rcf_ch_trrecv_start() returns - "
                                      "no support");
                    SEND_ANSWER("%d", EOPNOTSUPP);
                }

                break;
            }

            case RCFOP_TRSEND_RECV:
            {
                int handle;
                int timeout = 0;
                int results = 0;

                if (*ptr == 0 || ba == NULL)
                    goto bad_protocol;

                READ_INT(handle);

                if (isdigit(*ptr))
                    READ_INT(timeout);

                if (strncmp(ptr, "results", strlen("results")) == 0)
                {
                    results = 1;
                    ptr += strlen("results");
                    SKIP_SPACES(ptr);
                }

                if (*ptr != 0)
                    goto bad_protocol;

                if (rcf_ch_trsend_recv(conn, cmd, RCF_MAX_LEN,
                                       answer_plen, ba, len, handle,
                                       results, timeout) < 0)
                {
                    ERROR("rcf_ch_trsend_recv() returns - "
                                      "no support");
                    SEND_ANSWER("%d", EOPNOTSUPP);
                }

                break;
            }

            case RCFOP_EXECUTE:
            {
                uint32_t param[RCF_MAX_PARAMS];
                char    *rtn;
                int      argc;
                te_bool  is_argv;

                if (*ptr == 0 || ba != NULL ||
                    transform_str(&ptr, &rtn) != 0 ||
                    parse_parameters(ptr, &is_argv, &argc, param) != 0)
                {
                    goto bad_protocol;
                }

                rc = rcf_ch_call(conn, cmd, RCF_MAX_LEN, answer_plen,
                                 rtn, is_argv, argc, param);
                if (rc < 0)
                    rc = rcf_pch_call(conn, cmd, RCF_MAX_LEN, answer_plen,
                                      rtn, is_argv, argc, param);

                if (rc != 0)
                    goto communication_problem;

                break;
            }

            case RCFOP_START:
            {
                uint32_t param[RCF_MAX_PARAMS];
                char    *rtn;
                te_bool  is_argv;
                int      priority = -1;
                int      argc;

                if (*ptr == 0 || ba != NULL || transform_str(&ptr, &rtn) != 0)
                    goto bad_protocol;

                if (isdigit(*ptr))
                    READ_INT(priority);

                if (parse_parameters(ptr, &is_argv, &argc, param) != 0)
                    goto bad_protocol;

                if (rcf_ch_start_task(conn, cmd, RCF_MAX_LEN, answer_plen,
                                      priority, rtn, is_argv, argc, param) < 0)
                {
                    ERROR("rcf_ch_start_task() returns - "
                                      "no support for %s(%d)", rtn, priority);
                    SEND_ANSWER("%d", EOPNOTSUPP);
                }

                break;
            }

            case RCFOP_KILL:
            {
                unsigned int pid;

                if (*ptr == 0 || ba != NULL)
                    goto bad_protocol;

                READ_INT(pid);
                if (*ptr != 0)
                    goto bad_protocol;

                if (rcf_ch_kill_task(conn, cmd, RCF_MAX_LEN,
                                     answer_plen, pid) < 0)
                    SEND_ANSWER("%d", EOPNOTSUPP);

                break;
            }

            default:
                assert(FALSE);
        }
        continue;

    bad_protocol:
        VERB("Bad protocol command is received");
        SEND_ANSWER("%d bad command", ETEBADFORMAT);
    }

communication_problem:
    VERB("Fatal communication error %d", rc);
    goto exit;

exit:
    rcf_ch_conf_release();
    rcf_comm_agent_close(&conn);
    free(cmd);

    VERB("Exiting");

    return rc;

#undef READ_INT
#undef SEND_ANSWER
}
