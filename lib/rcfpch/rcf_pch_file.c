/** @file
 * @brief RCF Portable Command Handler
 *
 * Default fget and fput commands handlers implementation.
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
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
#include "rcf_pch.h"


/** Default file system directory for temporary files */
#define RCF_FILE_TMP_DEF_DIR    "/tmp/te/"


/* See description in rcf_ch_api.h */
int
rcf_pch_file(struct rcf_comm_connection *conn, char *cbuf, size_t buflen,
             size_t answer_plen, const uint8_t *ba, size_t cmdlen,
             rcf_op_t op, const char *filename)
{
    size_t  reply_buflen = buflen - answer_plen;
    int     rc;
    int     fd = -1;
    char    fname[RCF_MAX_PATH * 2];

    ENTRY("conn=0x%x cbuf='%s' buflen=%u answer_plen=%u ba=0x%x "
          "cmdlen=%u op=%d filename=%s\n", conn, cbuf, buflen, 
          answer_plen, ba, cmdlen, op, filename);
    VERB("Default file processing handler is executed");

    if (strncmp(RCF_FILE_MEM_PREFIX, filename,
                strlen(RCF_FILE_MEM_PREFIX)) == 0)
    {
        const char *ptr = filename + strlen(RCF_FILE_MEM_PREFIX);
        const char *sep = strchr(ptr, RCF_FILE_MEM_SEP);
        char       *tmp;

        long int    addr_int;
        void       *addr_ptr;
        void       *addr;

        unsigned long int   len = 0;

        if (op == RCFOP_FDEL)
        {
            ERROR("File delete operation for memory file");
            rc = TE_RC(TE_RCF_PCH, TE_EPERM);
            goto reject;
        }

        VERB("memory access file operation");
        /* Separator must present in get operation only */
        if ((op == RCFOP_FGET) != (sep != NULL))
        {
            ERROR("Invalid format - %s separator",
                  op == RCFOP_FGET ? "missing" : "extra");
            rc = TE_RC(TE_RCF_PCH, TE_EFMT);
            goto reject;
        }
        /* Try to consider address as hexadecimal number */
        addr_int = strtol(ptr, &tmp, 16);
        if (tmp != ptr)
        {
            if (tmp != sep)
            {
                ERROR("Unrecongnized symbols in address "
                                  "'%s'", tmp);
                rc = TE_RC(TE_RCF_PCH, TE_EFMT);
                goto reject;
            }
            addr = (void *)addr_int;
        }
        else /* Failed to convert address from string to number */
        {
            size_t  tmplen = (sep != NULL) ? (sep - ptr) : 0;
            char    buf[tmplen + 1];

            if (sep != NULL)
            {
                memcpy(buf, ptr, tmplen);
                buf[tmplen] = '\0';
            }
            /* Try to consider address as name from symbol table */
            addr_ptr = rcf_ch_symbol_addr((sep != NULL) ? buf : ptr, 0);
            if (addr_ptr == NULL)
            {
                ERROR("No such name '%s' in symbol table",
                                  (sep != NULL) ? buf : ptr);
                rc = TE_RC(TE_RCF_PCH, TE_ENOENT);
                goto reject;
            }
            addr = *(uint8_t **)addr_ptr;
        }
        /* Address has successfully been recognized */

        if (op == RCFOP_FGET)
        {
            len = strtol(sep + 1, &tmp, 10);
            if ((tmp == (sep + 1)) || (*tmp != '\0'))
            {
                ERROR("Incorrect length field '%s' format",
                                  sep + 1);
                rc = TE_RC(TE_RCF_PCH, TE_EFMT);
                goto reject;
            }
            /* Length to read has successfully been recognized */
        }

        if (op == RCFOP_FPUT)
        {
            size_t  got_len = ((cmdlen <= buflen) ? cmdlen : buflen) -
                                    (ba - (uint8_t *)cbuf);
            size_t  more_len;

            memcpy(addr, ba, got_len);
            if (cmdlen > buflen)
            {
                /* Not all data fit in command buffer */
                more_len = cmdlen - buflen;
                /* Write binary attachment to 'addr' on receive */
                rc = rcf_comm_agent_wait(conn, 
                                         ((uint8_t *)addr) + got_len,
                                         &more_len, NULL);
                if (rc != 0)
                {
                    ERROR("rcfpch", "rcf_comm_agent_wait() failed %r", rc);
                    EXIT("failed %r", rc);
                    return rc;
                }
            }
            SEND_ANSWER("0");
        }
        else /* get operation */
        {
            if ((size_t)snprintf(cbuf + answer_plen, reply_buflen,
                                 "0 attach %lu", len) >= reply_buflen)
            {
                ERROR("Command buffer too small for reply");
                rc = TE_RC(TE_RCF_PCH, TE_E2BIG);
                goto reject;
            }

            rcf_ch_lock();
            rc = rcf_comm_agent_reply(conn, cbuf, strlen(cbuf) + 1);
            if (rc == 0)
                rc = rcf_comm_agent_reply(conn, addr, len);
            rcf_ch_unlock();
            EXIT("%r", rc);
            return rc;
        }
        /* Unreachable */
        assert(FALSE);
    }
    else if (strncmp(RCF_FILE_TMP_PREFIX, filename,
                     strlen(RCF_FILE_TMP_PREFIX)) == 0)
    {
        VERB("temporary file operation");
#ifdef HAVE_DIRENT_H
        /* Default directory for temprary files is /tmp/te/ */
        DIR    *dir = opendir(RCF_FILE_TMP_DEF_DIR);

        if (dir == NULL)
        {
            /* Try to create */
            if (mkdir(RCF_FILE_TMP_DEF_DIR,
                      S_IRWXU | S_IRWXO | S_IRWXG) < 0)
            {
                VERB("cannot create directory '%s'",
                                  RCF_FILE_TMP_DEF_DIR);
                rc = TE_RC(TE_RCF_PCH, TE_ENOENT);
                goto reject;
            }
        }
        else
        {
            closedir(dir);
        }
        /* Substiute filename */
        sprintf(fname, RCF_FILE_TMP_DEF_DIR "%s", 
                filename + strlen(RCF_FILE_TMP_PREFIX));
#else
        rc = TE_RC(TE_RCF_PCH, TE_ENOENT);
        goto reject;
#endif
    }
    else if (strncmp(RCF_FILE_FTP_PREFIX, filename, 
                     strlen(RCF_FILE_FTP_PREFIX)) == 0)
    {
        
        sprintf(fname, "/var/ftp/%s", 
                filename + strlen(RCF_FILE_FTP_PREFIX));
    }            
    else
        strcpy(fname, filename);    
    
    if (op == RCFOP_FDEL)
    {
        if (unlink(fname) < 0)
        {
            rc = TE_OS_RC(TE_RCF_PCH, errno);
            goto reject;
        }
        SEND_ANSWER("0");
    }

    fd = open(fname, op == RCFOP_FPUT ? (O_WRONLY | O_CREAT | O_TRUNC) 
                                      : O_RDONLY, 
              S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd < 0)
    {
        ERROR("failed to open file '%s'", fname);
        rc = TE_RC(TE_RCF_PCH, TE_ENOENT);
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
            ERROR("failed to write to file '%s'\n", fname);
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
             * data is returned.  Actual number of retured bytes is
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
                unlink(fname);
                ERROR("failed to write in file '%s'\n", fname);
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
        rcf_ch_lock();
        rc = rcf_comm_agent_reply(conn, cbuf, strlen(cbuf) + 1);
        while ((rc == 0) && ((len = read(fd, cbuf, buflen)) > 0))
        {
            stat_buf.st_size -= len;
            rc = rcf_comm_agent_reply(conn, cbuf, len);
        }
        rcf_ch_unlock();
        close(fd);
        if ((rc == 0) && (stat_buf.st_size != 0))
        {
            ERROR("Failed to read file '%s'", fname);
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
