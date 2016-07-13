/** @file
 * @brief Agent support
 *
 * Common agent routines
 *
 * Copyright (C) 2004-2016 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
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
 * @author Elena A. Vengerova <Elena.Vengerova@oktetlabs.ru>
 * @author Artem V. Andreev   <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */
 
#ifndef __TE_AGENTLIB_H__
#define __TE_AGENTLIB_H__

#include "te_config.h"
#include "te_errno.h"
#include "agentlib_config.h"

#if HAVE_NET_IF_H
#include <net/if.h>
#endif

/**
 * Get parent device name of VLAN interface.
 * If passed interface is not VLAN, method sets 'parent' to empty string
 * and return success.
 *
 * @param ifname        interface name
 * @param parent        location of parent interface name,
 *                      IF_NAMESIZE buffer length(OUT)
 *
 * @return status
 */
extern te_errno ta_vlan_get_parent(const char *ifname, char *parent);

/**
 * Get slaves devices names for bonding interface.
 * If passed interface is not bonding, method sets 'slaves_num' to zero
 * and return success.
 *
 * @param ifname        interface name
 * @param slvs          location of slaves interfaces names
 * @param slaves_num    Number of slaves interfaces
 *
 * @return status
 */
extern te_errno ta_bond_get_slaves(const char *ifname,
                                   char slvs[][IFNAMSIZ],
                                   int *slaves_num);

#if defined(ENABLE_FTP)

/**
 * Open FTP connection for reading/writing the file.
 *
 * @param uri           FTP uri: ftp://user:password@server/directory/file
 * @param flags         O_RDONLY or O_WRONLY
 * @param passive       if 1, passive mode
 * @param offset        file offset
 * @param sock          pointer on socket
 *
 * @return file descriptor, which may be used for reading/writing data
 */
extern int ftp_open(const char *uri, int flags, int passive,
                    int offset, int *sock);

/**
 * Close FTP control connection.
 *
 * @param control_socket socket to close
 *
 * @retval 0 success
 * @retval -1 failure
 */
extern int ftp_close(int control_socket);

#endif /* ENABLE_FTP */


/**
 * Initialize process management subsystem
 */
extern te_errno ta_process_mgmt_init(void);

/**
 * waitpid() analogue, with the same parameters/return value.
 * Only WNOHANG option is supported for now.
 * Process groups are not supported for now.
 */
extern pid_t ta_waitpid(pid_t pid, int *status, int options);

/**
 * system() analogue, with the same parameters/return value.
 */
extern int ta_system(const char *cmd);

/**
 * popen('r') analogue, with slightly modified parameters.
 */
extern te_errno ta_popen_r(const char *cmd, pid_t *cmd_pid, FILE **f);

/**
 * Perform cleanup actions for ta_popen_r() function.
 */
extern te_errno ta_pclose_r(pid_t cmd_pid, FILE *f);

/**
 * Kill a child process.
 *
 * @param pid PID of the child to be killed
 *
 * @retval 0 child was exited or killed successfully
 * @retval -1 there is no such child.
 */
extern int ta_kill_death(pid_t pid);

#if defined(ENABLE_TELEPHONY)
#include "telephony.h"
#endif

#if defined(ENABLE_POWER_SW)
/**
 * Turn ON, turn OFF or restart power lines specified by mask
 *
 * @param type      power switch device type tty/parport
 * @param dev       power switch device name
 * @param mask      power lines bitmask
 * @param cmd       power switch command turn ON, turn OFF or restart
 *
 * @return          0, otherwise -1
 */
extern int power_sw(int type, const char *dev, int mask, int cmd);
#endif


#endif /* __TE_AGENTLIB_H__ */
