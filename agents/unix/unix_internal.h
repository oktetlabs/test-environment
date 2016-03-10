/** @file
 * @brief Unix Test Agent
 *
 * Unix TA definitions
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 *
 * $Id$
 */
 
#ifndef __TE_TA_UNIX_INTERNAL_H__
#define __TE_TA_UNIX_INTERNAL_H__

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NET_IF_H
#include <net/if.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "te_sockaddr.h"
#include "rcf_pch.h"

/** Fast conversion of the network mask to prefix */
#define MASK2PREFIX(mask, prefix)            \
    switch (mask)                            \
    {                                        \
        case 0x0: prefix = 0; break;         \
        case 0x80000000: prefix = 1; break;  \
        case 0xc0000000: prefix = 2; break;  \
        case 0xe0000000: prefix = 3; break;  \
        case 0xf0000000: prefix = 4; break;  \
        case 0xf8000000: prefix = 5; break;  \
        case 0xfc000000: prefix = 6; break;  \
        case 0xfe000000: prefix = 7; break;  \
        case 0xff000000: prefix = 8; break;  \
        case 0xff800000: prefix = 9; break;  \
        case 0xffc00000: prefix = 10; break; \
        case 0xffe00000: prefix = 11; break; \
        case 0xfff00000: prefix = 12; break; \
        case 0xfff80000: prefix = 13; break; \
        case 0xfffc0000: prefix = 14; break; \
        case 0xfffe0000: prefix = 15; break; \
        case 0xffff0000: prefix = 16; break; \
        case 0xffff8000: prefix = 17; break; \
        case 0xffffc000: prefix = 18; break; \
        case 0xffffe000: prefix = 19; break; \
        case 0xfffff000: prefix = 20; break; \
        case 0xfffff800: prefix = 21; break; \
        case 0xfffffc00: prefix = 22; break; \
        case 0xfffffe00: prefix = 23; break; \
        case 0xffffff00: prefix = 24; break; \
        case 0xffffff80: prefix = 25; break; \
        case 0xffffffc0: prefix = 26; break; \
        case 0xffffffe0: prefix = 27; break; \
        case 0xfffffff0: prefix = 28; break; \
        case 0xfffffff8: prefix = 29; break; \
        case 0xfffffffc: prefix = 30; break; \
        case 0xfffffffe: prefix = 31; break; \
        case 0xffffffff: prefix = 32; break; \
         /* Error indication */              \
        default: prefix = 33; break;         \
    }


#if defined(__linux__)
/** Strip off .VLAN from interface name */
static inline char *
ifname_without_vlan(const char *ifname)
{
    static char tmp[IFNAMSIZ];
    char       *s;
    
    strcpy(tmp, ifname);
    if ((s = strchr(tmp, '.')) != NULL)
        *s = '\0';
        
    return tmp;
}
#elif defined(__sun__)
static inline char *
ifname_without_vlan(const char *ifname)
{
    static char tmp[IFNAMSIZ];

    int i, k, tok_found;
    int len = strlen(ifname);
    memset(tmp, 0, IFNAMSIZ);

    for (i = 0; i < len; i++)
    {
        if (isdigit(ifname[i]) != 0)
            break;

        tmp[i] = ifname[i];
    } /* Got the drv name part */

    k = i;

    tok_found = 0;
    for (; i < len; i++) /* Look for 00 VLAN token */
    {
        if (ifname[i] == '0' && ifname[i+1] == '0')
        {
            tok_found = 1;
            i++; i++; /* Move beyond the token */
            break;
        }
    }   /* Skipped everything up to 00 token including */


    if (tok_found == 0) /* no 00 token, just a regular ifname: we copy */
    {
        for (; k < len; k++)       
            tmp[k] = ifname[k];
    }
    else
    {
        if (i >=len)
            ERROR("Dangling 00 token in ifname: %s\n", ifname);

        for (; i < len; i++) /* Copy the rest: instance number */
            tmp[k++] = ifname[i];
    }

    tmp[k]='\0';
    return tmp;
}
#else
static inline char *
ifname_without_vlan(const char *ifname)
{
    UNUSED(ifname);

    ERROR("This test agent does not support VLANs");
    return NULL;
}
#endif  /* __linux__ or __sun__ */

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


#define PRINT(msg...) \
    do {                                                \
       printf(msg); printf("\n"); fflush(stdout);       \
    } while (0)


/** Test Agent name */
extern const char *ta_name;
/** Test Agent executable name */ 
extern const char *ta_execname;
/** Test Agent data and binaries location */ 
extern char ta_dir[RCF_MAX_PATH];

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

/**
 * Special signal handler which registers signals.
 * 
 * @param signum    received signal
 */
extern void signal_registrar(int signum);

/**
 * Special signal handler which registers signals and also
 * saves signal information.
 * 
 * @param signum    received signal
 * @param siginfo   pointer to siginfo_t structure
 * @param context   pointer to user context
 */
extern void signal_registrar_siginfo(int signum, siginfo_t *siginfo,
                                     void *context);

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

/**
 * Get status of the interface (FALSE - down or TRUE - up).
 *
 * @param ifname        name of the interface (like "eth0")
 * @param status        location to put status of the interface
 *
 * @return              Status code
 */
extern te_errno ta_interface_status_get(const char *ifname, 
                                        te_bool *status);

/**
 * Change status of the interface. If virtual interface is put to down
 * state,it is de-installed and information about it is stored in the list
 * of down interfaces.
 *
 * @param ifname        name of the interface (like "eth0")
 * @param status        TRUE to get interface up and FALSE to down
 *
 * @return              Status code
 */
extern te_errno ta_interface_status_set(const char *ifname, te_bool status);

/* Sockets to be used by various parts of configurator */
extern int cfg_socket;
extern int cfg6_socket;

/**
 * Initialize context for aux threads which are used to make non-blocking
 * RPC call.
 */
extern te_errno aux_threads_init(void);

/**
 * Cleanup the aux threads context.
 */
extern te_errno aux_threads_cleanup(void);

/**
 * Save thread identifier which is used for non-blocking RPC call.
 */
extern void aux_threads_add(pthread_t tid);

/**
 * Remove thread identifier which is used for non-blocking RPC call from the
 * context.
 */
extern void aux_threads_del(void);

/** TR-069 stuff */

#ifdef WITH_TR069_SUPPORT

#include "tarpc.h"

/** Executive routines for mapping of the CPE CWMP RPC methods
 * (defined in conf/base/conf_acse.c, used from rpc/tarpc_server.c)
 */


extern int cwmp_op_call(tarpc_cwmp_op_call_in *in,
                        tarpc_cwmp_op_call_out *out);

extern int cwmp_op_check(tarpc_cwmp_op_check_in *in,
                         tarpc_cwmp_op_check_out *out);

extern int cwmp_conn_req(tarpc_cwmp_conn_req_in *in,
                         tarpc_cwmp_conn_req_out *out);

extern int cwmp_acse_start(tarpc_cwmp_acse_start_in *in,
                           tarpc_cwmp_acse_start_out *out);

#endif /* WITH_TR069_SUPPORT */

#endif /* __TE_TA_UNIX_INTERNAL_H__ */
