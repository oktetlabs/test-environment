/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief RCF Portable Command Handler
 *
 * Default fget and fput commands handlers implementation.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "rcf_pch_internal.h"

#include "te_errno.h"
#include "te_defs.h"
#include "te_stdint.h"
#include "comm_agent.h"
#include "agentlib.h"
#include "rcf_pch.h"

/* See description in rcf_ch_api.h */
int
rcf_pch_file(struct rcf_comm_connection *conn, char *cbuf, size_t buflen,
             size_t answer_plen, const uint8_t *ba, size_t cmdlen,
             rcf_op_t op, const char *filename)
{
    size_t  reply_buflen = buflen - answer_plen;
    int     rc;
    int     fd = -1;

    ENTRY("conn=0x%x cbuf='%s' buflen=%u answer_plen=%u ba=0x%x "
          "cmdlen=%u op=%d filename=%s\n", conn, cbuf, buflen,
          answer_plen, ba, cmdlen, op, filename);
    VERB("Default file processing handler is executed");

    if (op == RCFOP_FDEL)
    {
        if (unlink(filename) < 0)
        {
            rc = TE_OS_RC(TE_RCF_PCH, errno);
            goto reject;
        }
        SEND_ANSWER("0");
    }

    fd = open(filename, op == RCFOP_FPUT ? (O_WRONLY | O_CREAT | O_TRUNC)
                                      : O_RDONLY,
              S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd < 0)
    {
        /* Caller should print an error message if it is a problem */
        rc = TE_OS_RC(TE_RCF_PCH, errno);
        goto reject;
    }

    if (op == RCFOP_FPUT)
    {
        size_t  rest = (cmdlen > buflen) ? (cmdlen - buflen) : 0;
        size_t  rw_len = ((cmdlen <= buflen) ? cmdlen : buflen) -
                             (ba - (uint8_t *)cbuf);
        ssize_t res;

        res = write(fd, ba, rw_len);
        if ((res < 0) || ((size_t)res < rw_len))
        {
            ERROR("failed to write to file '%s'\n", filename);
            rc = TE_OS_RC(TE_RCF_PCH, errno);
            goto reject;
        }
        while (rest > 0)
        {
            rw_len = reply_buflen;
            rc = rcf_comm_agent_wait(conn, cbuf + answer_plen, &rw_len,
                                     NULL);
            if ((rc != 0) && (TE_RC_GET_ERROR(rc) != TE_EPENDING))
            {
                ERROR("Communication error %r", rc);
                close(fd);
                EXIT("%r", rc);
                return rc;
            }
            /*
             * If buffer is not sufficient, total amount of pending
             * data is returned.  Actual number of returned bytes is
             * never greater than specified on input.
             */
            if (rw_len > reply_buflen)
                rw_len = reply_buflen;
            rest -= rw_len;
            if ((rw_len == 0) ||
                ((TE_RC_GET_ERROR(rc) == TE_EPENDING) != (rest != 0)))
            {
                ERROR("Communication error - %s",
                      (rw_len == 0) ? "empty read" : "extra/missing data");
                close(fd);
                EXIT("EIO");
                return TE_EIO;
            }

            res = write(fd, cbuf + answer_plen, rw_len);
            if ((res < 0) || ((size_t)res < rw_len))
            {
                rc = TE_OS_RC(TE_RCF_PCH, (res < 0) ? errno : ENOSPC);
                unlink(filename);
                ERROR("failed to write in file '%s'\n", filename);
                goto reject;
            }
        }
        close(fd);
        SEND_ANSWER("0");
    }
    else
    {
        struct stat stat_buf;
        int         len;

        if (fstat(fd, &stat_buf) != 0)
        {
            rc = TE_OS_RC(TE_RCF_PCH, errno);
            ERROR("rcfpch", "fstat() failed %r", rc);
            goto reject;
        }

        if ((size_t)snprintf(cbuf + answer_plen, reply_buflen,
                             "0 attach %u", (unsigned int)stat_buf.st_size)
                >= reply_buflen)
        {
            ERROR("Command buffer too small for reply");
            rc = TE_RC(TE_RCF_PCH, TE_E2BIG);
            goto reject;
        }
        RCF_CH_LOCK;
        rc = rcf_comm_agent_reply(conn, cbuf, strlen(cbuf) + 1);
        while ((rc == 0) && ((len = read(fd, cbuf, buflen)) > 0))
        {
            stat_buf.st_size -= len;
            rc = rcf_comm_agent_reply(conn, cbuf, len);
        }
        RCF_CH_UNLOCK;
        close(fd);
        if ((rc == 0) && (stat_buf.st_size != 0))
        {
            ERROR("Failed to read file '%s'", filename);
        }
        EXIT("%r", rc);
        return rc;
    }
    /* Unreachable */
    assert(FALSE);

reject:
    if (fd != -1)
        close(fd);
    if (cmdlen > buflen)
    {
        int     error;
        size_t  rest;

        do {
            rest = reply_buflen;
            error = rcf_comm_agent_wait(conn, cbuf + answer_plen,
                                        &rest, NULL);
            if (error != 0 && TE_RC_GET_ERROR(error) != TE_EPENDING)
            {
                return TE_RC(TE_RCF_PCH, error);
            }
        } while (error != 0);
    }
    EXIT("%r", rc);
    SEND_ANSWER("%d", rc);
    /* Unreachable */
}
