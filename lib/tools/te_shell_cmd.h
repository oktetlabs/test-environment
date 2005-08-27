/** @file
 * @brief API to call shell commands
 *
 * Routines to call shell commands
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
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
 *
 * @author Alexandra Kossovsky <Alexandra.Kossovsky@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_SHELL_CMD_H__
#define __TE_SHELL_CMD_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function to base system()-like and popen()-like functions on it.
 * You MUST use uid parameter instead of "su - user -c", because su makes
 * one more fork, and you do not know how to kill this grandchild process.
 * Process spawned by this function SHOULD be catched by ta_waitpid() or
 * killed by ta_kill_death().
 *
 * @param cmd    command to run in the shell
 * @param uid    user id to run the shell; use -1 for default
 * @param in_fd  location to store file descriptor of pipe to stdin of 
 *               the shell command; use NULL for standard stdin
 * @param out_fd location to store file descriptor of pipe to stdout of
 *               the shell command; use NULL for standard stdout
 * 
 * @return pid value, positive on success, negative on failure
 */
extern pid_t te_shell_cmd(const char *cmd, uid_t uid, 
                          int *in_fd, int *out_fd);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_SHELL_CMD_H__ */
