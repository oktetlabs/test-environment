/** @file
 * @brief Test API for RPC
 *
 * TAPI for remote calls of stdio routines
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
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
 *
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 * @author Alexandra Kossovsky <Alexandra.Kossovsky@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "tapi_rpc_internal.h"
#include "tapi_rpc_unistd.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpc_signal.h"


rpc_file_p
rpc_fopen(rcf_rpc_server *rpcs,
          const char *path, const char *mode)
{
    tarpc_fopen_in  in;
    tarpc_fopen_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL || path == NULL || mode == NULL)
    {
        ERROR("%s(): Invalid RPC parameter", __FUNCTION__);
        RETVAL_RPC_PTR(fopen, RPC_NULL);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    in.path.path_len = strlen(path) + 1;
    in.path.path_val = (char *)strdup(path);    /* FIXME */
    in.mode.mode_len = strlen(mode) + 1;
    in.mode.mode_val = (char *)strdup(mode);    /* FIXME */

    rcf_rpc_call(rpcs, "fopen", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): fopen(%s, %s) -> 0x%x (%s)",
                 rpcs->ta, rpcs->name,
                 path, mode,
                 (unsigned)out.mem_ptr, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_RPC_PTR(fopen, out.mem_ptr);
}

int
rpc_fclose(rcf_rpc_server *rpcs, rpc_file_p file)
{
    tarpc_fclose_in  in;
    tarpc_fclose_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(fclose, EOF);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    in.mem_ptr = (tarpc_ptr)file;

    rcf_rpc_call(rpcs, "fclose", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): fclose(%x) -> %d (%s)",
                 rpcs->ta, rpcs->name, file,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(fclose, out.retval);
}

int
rpc_fileno(rcf_rpc_server *rpcs,
           rpc_file_p f)
{
    tarpc_fileno_in  in;
    tarpc_fileno_out out;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_INT(fileno, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    in.mem_ptr = (unsigned int)f;

    rcf_rpc_call(rpcs, "fileno", &in, &out);

    CHECK_RETVAL_VAR_IS_GTE_MINUS_ONE(fileno, out.fd);

    TAPI_RPC_LOG("RPC (%s,%s): fileno(0x%x) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 (unsigned)f, out.fd, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(fileno, out.fd);
}

/** 
 * See te_shell_cmd function.
 * cmd parameter should not be changed after call.
 */
static tarpc_pid_t
rpc_te_shell_cmd(rcf_rpc_server *rpcs, const char *cmd, 
                 tarpc_uid_t uid, int *in_fd, int *out_fd)
{
    tarpc_te_shell_cmd_in  in;
    tarpc_te_shell_cmd_out out;
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC parameter", __FUNCTION__);
        RETVAL_INT(te_shell_cmd, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    in.cmd.cmd_len = strlen(cmd) + 1;
    in.cmd.cmd_val = (char *)cmd;
    in.uid = uid;
    in.in_fd = (in_fd != NULL);
    in.out_fd = (out_fd != NULL);

    rcf_rpc_call(rpcs, "te_shell_cmd", &in, &out);
                 
    if (in_fd != NULL)
        *in_fd = out.in_fd;
    if (out_fd != NULL)
        *out_fd = out.out_fd;

    TAPI_RPC_LOG("RPC (%s,%s): te_shell_cmd(%s, %d, %p(%d), %p(%d)) "
                 "-> %d (%s)",
                 rpcs->ta, rpcs->name, cmd, uid, 
                 in_fd, (in_fd == NULL) ? 0 : *in_fd,
                 out_fd, (out_fd == NULL) ? 0 : *out_fd,
                 out.pid, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(te_shell_cmd, out.pid);
}

/* See description in tapi_rpc_stdio.h */
tarpc_pid_t
rpc_ta_shell_cmd_ex(rcf_rpc_server *rpcs, const char *cmd, 
                    tarpc_uid_t uid, int *in_fd, int *out_fd, ...)
{
    char    cmdline[RPC_SHELL_CMDLINE_MAX];
    va_list ap;

    va_start(ap, out_fd);
    vsnprintf(cmdline, sizeof(cmdline), cmd, ap);
    va_end(ap);

    return rpc_te_shell_cmd(rpcs, cmdline, uid, in_fd, out_fd);
}

/** Chunk for memory allocation in rpc_read_all */
#define RPC_READ_ALL_BUF_CHUNK     1024          

int
rpc_read_all(rcf_rpc_server *rpcs, int fd, char **pbuf, int *bytes)
{
    char   *buf = NULL;
    int     buflen = RPC_READ_ALL_BUF_CHUNK;
    int     offset = 0;
    int     rc = 0;

    *bytes = 0;
    if (rpcs == NULL || pbuf == NULL)
    {
        ERROR("%s(): Invalid parameters", __FUNCTION__);
        rc = -1;
    }
    else if ((buf = calloc(1, RPC_READ_ALL_BUF_CHUNK)) == NULL)
    {
        ERROR("Out of memory");
        rc = -1;
    }
    else while (TRUE)
    {
        int used;

        if ((used = rpc_read(rpcs, fd, 
                              buf + offset, buflen - offset)) < 0)
        {
            ERROR("Cannot read from file descriptor: rpc_read failed");
            rc = -1;
            break;
        }
        if (used == 0)
            break;

        offset += used;
        if (offset == buflen)
        {
            buflen = buflen * 2;
            if ((buf = realloc(buf, buflen)) == NULL)
            {
                ERROR("Out of memory");
                rc = -1;
                break;
            }
            memset(buf + offset, 0, buflen - offset);
        }
    }
    
    *pbuf = buf;
    *bytes = offset;
    rpc_close(rpcs, fd);
    return rc;
}

rpc_wait_status
rpc_shell_get_all(rcf_rpc_server *rpcs, char **pbuf, const char *cmd, 
                  tarpc_uid_t uid, ...)
{
    int     bytes;
    char    cmdline[RPC_SHELL_CMDLINE_MAX];
    int     fd;

    tarpc_pid_t     pid;
    rpc_wait_status rc;
    int iut_err_jump;

    va_list ap;

    va_start(ap, uid);
    vsnprintf(cmdline, sizeof(cmdline), cmd, ap);
    va_end(ap);

    rc.flag = RPC_WAIT_STATUS_UNKNOWN;
    if (rpcs == NULL || pbuf == NULL)
    {
        ERROR("%s(): Invalid parameters", __FUNCTION__);
        return rc;
    }
    
    iut_err_jump = rpcs->iut_err_jump;
    if ((pid = rpc_te_shell_cmd(rpcs, cmdline, uid, NULL, &fd)) < 0)
    {
        ERROR("Cannot execute the command: rpc_te_shell_cmd() failed");
        return rc;
    }

    if (rpc_read_all(rpcs, fd, pbuf, &bytes) != 0)
        rpc_kill(rpcs, pid, RPC_SIGKILL);

    /* Restore jump setting to avoid jump after command crash. */
    rpcs->iut_err_jump = iut_err_jump;
    /* 
     * @todo if we will jump, we'd better free(buf).
     * As test is failed in any way, this memory leak is not important.
     * Let's think that its test responsibility to free the buf in any
     * case.
     */
    rpc_waitpid(rpcs, pid, &rc, 0);

    return rc;
}

rpc_wait_status
rpc_system(rcf_rpc_server *rpcs, const char *cmd)
{
    tarpc_system_in     in;
    tarpc_system_out    out;
    rpc_wait_status     rc;

    char *cmd_dup = strdup(cmd);

    rc.flag = RPC_WAIT_STATUS_UNKNOWN;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC server", __FUNCTION__);
        free(cmd_dup);
        RETVAL_WAIT_STATUS(system, rc);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    in.cmd.cmd_len = strlen(cmd) + 1;
    in.cmd.cmd_val = (char *)cmd_dup;

    rcf_rpc_call(rpcs, "system", &in, &out);
    rc.flag = out.status_flag;
    rc.value = out.status_value;
    TAPI_RPC_LOG("RPC (%s,%s): system(%s) -> %s %d (%s)",
                 rpcs->ta, rpcs->name,
                 cmd, wait_status_flag_rpc2str(rc.flag), rc.value, 
                 errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_WAIT_STATUS(system, rc);
}

/**
 * Get environment variable.
 *
 * @param rpcs          RPC server handle
 * @param name          variable name
 *
 * @return variable value (memory is allocated by the function) or NULL
 */
char *
rpc_getenv(rcf_rpc_server *rpcs, const char *name)
{
    tarpc_getenv_in  in;
    tarpc_getenv_out out;
    
    char *val;

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    if (rpcs == NULL || name == NULL)
    {
        ERROR("%s(): Invalid RPC parameter", __FUNCTION__);
        RETVAL_PTR(getenv, RPC_NULL);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    in.name = (char *)name;

    rcf_rpc_call(rpcs, "getenv", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): getenv(%s) -> %s (%s)",
                 rpcs->ta, rpcs->name,
                 name, out.val, errno_rpc2str(RPC_ERRNO(rpcs)));
                 
    val = out.val;
    out.val = NULL;                 

    RETVAL_PTR(getenv, val);
}

/**
 * Add the variable with specified value to the environment or change
 * value of the existing variable.
 *
 * @param rpcs          RPC server handle
 * @param name          variable name
 * @param value         new value
 * @param overwrite     0, value of the existing variable is not changed
 *
 * @return variable value (memory is allocated by the function) or NULL
 */
int 
rpc_setenv(rcf_rpc_server *rpcs, const char *name, 
           const char *value, int overwrite)
{
    tarpc_setenv_in  in;
    tarpc_setenv_out out;
    
    char *val = (value == NULL) ? "" : (char *)value;
    
    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));
    
    if (rpcs == NULL || name == NULL)
    {
        ERROR("%s(): Invalid RPC server handle", __FUNCTION__);
        RETVAL_RPC_PTR(setenv, RPC_NULL);
    }
    
    rpcs->op = RCF_RPC_CALL_WAIT;
    in.name = (char *)name;
    in.val = val;
    in.overwrite = overwrite;

    rcf_rpc_call(rpcs, "setenv", &in, &out);

    TAPI_RPC_LOG("RPC (%s,%s): setenv(%s, %s, %d) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 name, val, overwrite,
                 out.retval, errno_rpc2str(RPC_ERRNO(rpcs)));
                 
    RETVAL_INT(fclose, out.retval);
}
