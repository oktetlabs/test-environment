/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of some standard input/output
 * functions and useful extensions.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
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

#ifndef __TE_TAPI_RPC_STDIO_H__
#define __TE_TAPI_RPC_STDIO_H__

#include <stdio.h>

#include "rcf_rpc.h"


extern int rpc_fileno(rcf_rpc_server *rpcs,
                      FILE *f);

extern FILE *rpc_fopen(rcf_rpc_server *rpcs,
                       const char *path, const char *mode);

extern FILE *rpc_popen(rcf_rpc_server *rpcs,
                       const char *cmd, const char *mode);


/** Maximum resulting command length for rpc_shell() */
#define RPC_SHELL_CMDLINE_MAX   256

/**
 * Execute shell command on the IPC server and read the output.
 *
 * @param rpcs          RPC server rpcs
 * @param buf           output buffer
 * @param buflen        output buffer length
 * @param cmd           format of the command to be executed
 *
 * @return 0 (success) or -1 (failure)
 */
extern int rpc_shell(rcf_rpc_server *rpcs,
                     char *buf, int buflen, const char *cmd,...);

/**
 * Execute shell command on the IPC server and read the output.
 * The routine allocates memory for the output buffer and places
 * null-terminated string to it.
 *
 * @param rpcs          RPC server rpcs
 * @param buf           location for the command output buffer 
 * @param cmd           format of the command to be executed
 *
 * @return 0 (success) or -1 (failure)
 */
extern int rpc_shell_get_all(rcf_rpc_server *rpcs,
                             char **pbuf, const char *cmd,...);

/**
 * Execute shell command on the IPC server and return file descriptor
 * for it's standard input or output.
 *
 * @param rpcs          RPC server rpcs
 * @param mode          "r" or "w"
 * @param cmd           format of the command to be executed
 *
 * @return File descriptor or -1 in the case of failure
 */
extern int rpc_cmd_spawn(rcf_rpc_server *rpcs, const char *mode,
                         const char *cmd,...);


#endif /* !__TE_TAPI_RPC_STDIO_H__ */
