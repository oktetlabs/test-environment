/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Unix TA definitions
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
#include "te_str.h"
#include "rcf_pch.h"

#include "agentlib.h"

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

/** Test Agent name */
extern const char *ta_name;
/** Test Agent executable name */
extern const char *ta_execname;
/** Test Agent data and binaries location */
extern char ta_dir[RCF_MAX_PATH];
/** Directory for temporary files */
extern char ta_tmp_dir[RCF_MAX_PATH];
/** Directory for kernel module files */
extern char ta_lib_mod_dir[RCF_MAX_PATH];
/** Directory for library files */
extern char ta_lib_bin_dir[RCF_MAX_PATH];

/**
 * Get oper status of the interface (TRUE - RUNNING).
 *
 * @param ifname        name of the interface (like "eth0")
 * @param status        location to put status of the interface
 *
 * @return              Status code
 */
extern te_errno ta_interface_oper_status_get(const char *ifname,
                                             te_bool *status);

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

#endif /* __TE_TA_UNIX_INTERNAL_H__ */
