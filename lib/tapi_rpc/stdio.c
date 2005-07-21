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

    TAPI_RPC_LOG("RPC (%s,%s): fopen(%s, %s) -> %p (%s)",
                 rpcs->ta, rpcs->name,
                 path, mode, out.mem_ptr, errno_rpc2str(RPC_ERRNO(rpcs)));

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
rpc_popen_fd(rcf_rpc_server *rpcs,
             const char *cmd, const char *mode)
{
    tarpc_popen_fd_in  in;
    tarpc_popen_fd_out out;
    
    char *cmd_dup = strdup(cmd);
    char *mode_dup = strdup(mode);

    memset(&in, 0, sizeof(in));
    memset(&out, 0, sizeof(out));

    if (rpcs == NULL)
    {
        ERROR("%s(): Invalid RPC parameter", __FUNCTION__);
        free(cmd_dup);
        free(mode_dup);
        RETVAL_INT(popen_fd, -1);
    }

    rpcs->op = RCF_RPC_CALL_WAIT;
    in.cmd.cmd_len = strlen(cmd) + 1;
    in.cmd.cmd_val = (char *)cmd_dup;
    in.mode.mode_len = strlen(mode) + 1;
    in.mode.mode_val = (char *)mode_dup;

    rcf_rpc_call(rpcs, "popen_fd", &in, &out);
                 
    free(cmd_dup);
    free(mode_dup);  

    TAPI_RPC_LOG("RPC (%s,%s): popen_fd(%s, %s) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 cmd, mode, out.fd, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(popen_fd, out.fd);
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

    TAPI_RPC_LOG("RPC (%s,%s): fileno(%p) -> %d (%s)",
                 rpcs->ta, rpcs->name,
                 f, out.fd, errno_rpc2str(RPC_ERRNO(rpcs)));

    RETVAL_INT(fileno, out.fd);
}

/**
 * Execute shell command on the IPC server and return file descriptor
 * for it's standard input.
 *
 * @param rpcs          RPC server handle
 * @param mode          "r" or "w"
 * @param cmd           format of the command to be executed
 *
 * @return File descriptor or -1 in the case of failure
 */
int 
rpc_cmd_spawn(rcf_rpc_server *rpcs, const char *mode, const char *cmd, ...)
{
    int         fd;
    char        cmdline[RPC_SHELL_CMDLINE_MAX];

    va_list ap;

    va_start(ap, cmd);
    vsnprintf(cmdline, sizeof(cmdline), cmd, ap);
    va_end(ap);
    
    if ((fd = rpc_popen_fd(rpcs, cmdline, mode)) < 0)
    {
        ERROR("Cannot execute the command: rpc_popen_fd() failed");
        return -1;
    }
    
    return fd;
}

/**
 * Execute shell command on the IPC server and read the output.
 *
 * @param rpcs        RPC server handle
 * @param buf           output buffer
 * @param buflen        output buffer length
 * @param cmd           format of the command to be executed
 *
 * @return 0 (success) or -1 (failure)
 */
int 
rpc_shell(rcf_rpc_server *rpcs,
          char *buf, int buflen, const char *cmd, ...)
{
    int         fd;
    int         rc = 0;
    char        cmdline[RPC_SHELL_CMDLINE_MAX];

    va_list     ap;

    va_start(ap, cmd);
    vsnprintf(cmdline, sizeof(cmdline), cmd, ap);
    va_end(ap);
    
    if ((fd = rpc_popen_fd(rpcs, cmdline, "r")) < 0)
    {
        ERROR("Cannot execute the command: rpc_popen_fd() failed");
        return -1;
    }
    
    buf[0] = 0;

    if (rpc_read_gen(rpcs, fd, buf, buflen, buflen) < 0)
    {
        ERROR("Cannot read command output: rpc_read failed");
        rc = -1;
    }

    rpc_close(rpcs, fd);
    
    return rc;
}

/** Chunk for memory allocation in rpc_shell_get_all */
#define RPC_SHELL_BUF_CHUNK     1024          

/**
 * Execute shell command on the IPC server and read the output.
 * The routine allocates memory for the output buffer and places
 * null-terminated string to it.
 *
 * @param rpcs        RPC server handle
 * @param buf           location for the command output buffer 
 * @param cmd           format of the command to be executed
 *
 * @return 0 (success) or -1 (failure)
 */
int 
rpc_shell_get_all(rcf_rpc_server *rpcs, char **pbuf, const char *cmd, ...)
{
    char       *buf = calloc(1, RPC_SHELL_BUF_CHUNK);
    int         buflen = RPC_SHELL_BUF_CHUNK;
    int         offset = 0;
    char        cmdline[RPC_SHELL_CMDLINE_MAX];
    int         fd;
    int         rc = -1;

    va_list ap;
    
    if (buf == NULL)
    {
        ERROR("Out of memory");
        return -1;
    }

    va_start(ap, cmd);
    vsnprintf(cmdline, sizeof(cmdline), cmd, ap);
    va_end(ap);
    
    if ((fd = rpc_popen_fd(rpcs, cmdline, "r")) < 0)
    {
        ERROR("Cannot execute the command: rpc_popen_fd() failed");
        free(buf);
        return -1;
    }
    
#if 1
    sleep(1);
#endif
    
    while (TRUE)
    {
        if (rpc_read(rpcs, fd, buf + offset, buflen - offset) < 0)
        {
            ERROR("Cannot read command output: rpc_read failed");
            break;
        }

        if (buf[buflen - 1] == 0)
        {
            rc = 0;
            break;
        }
        
        offset = buflen;    
        buflen = buflen * 2;
        
        if ((buf = realloc(buf, buflen)) == NULL)
        {
            ERROR("Out of memory");
            break;
        }
        memset(buf + offset, 0, buflen - offset);
    }
    rpc_close(rpcs, fd);

    if (rc == 0)
        *pbuf = buf;
    else
        free(buf);
    
    return rc;
    
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
