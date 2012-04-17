/** @file
 * @brief Some system wide settings
 *
 * Unix TA system wide settings support
 *
 *
 * Copyright (C) 2006 Test Environment authors (see file AUTHORS
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
 * @author Igor Vasiliev <Igor.Vasiliev@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "Conf Sys Wide"

#include "te_config.h"
#include "config.h"

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

/* Solaris sream interface */
#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif
#ifdef HAVE_INET_ND_H
#include <inet/nd.h>
#endif
#if defined(HAVE_STROPTS_H) && defined(HAVE_INET_ND_H) && \
    !defined(SUN_I_STR)
#define SUN_I_STR 1
#endif

#include "te_stdint.h"
#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "logger_api.h"
#include "unix_internal.h"
#include "te_shell_cmd.h"

#include <sys/klog.h>

#if __linux__
static char trash[128];
#endif

/*
 * System wide settings both max and default parameters of sndbuf/rcvbuf:
 * Linux UDP: /proc/sys/net/core/
 *             [rmem_max, rmem_default, wmem_max, wmem_default]
 * Solaris UDP: ioctl()
 * Linux TCP: /proc/sys/net/ipv4/
 *             [tcp_rmem, tcp_wmem]
 * Solaris TCP: ioctl()
 */

static te_errno rcvbuf_def_set(unsigned int, const char *,
                               const char *);

static te_errno rcvbuf_def_get(unsigned int, const char *,
                               char *);

static te_errno rcvbuf_max_set(unsigned int, const char *,
                               const char *);

static te_errno rcvbuf_max_get(unsigned int, const char *,
                               char *);

static te_errno sndbuf_def_set(unsigned int, const char *,
                               const char *);

static te_errno sndbuf_def_get(unsigned int, const char *,
                               char *);

static te_errno sndbuf_max_set(unsigned int, const char *,
                               const char *);

static te_errno sndbuf_max_get(unsigned int, const char *,
                               char *);

static te_errno udp_rcvbuf_def_set(unsigned int, const char *,
                                   const char *);

static te_errno udp_rcvbuf_def_get(unsigned int, const char *,
                                   char *);

static te_errno udp_rcvbuf_max_set(unsigned int, const char *,
                                   const char *);

static te_errno udp_rcvbuf_max_get(unsigned int, const char *,
                                   char *);

static te_errno udp_sndbuf_def_set(unsigned int, const char *,
                                   const char *);

static te_errno udp_sndbuf_def_get(unsigned int, const char *,
                                   char *);

static te_errno udp_sndbuf_max_set(unsigned int, const char *,
                                   const char *);

static te_errno udp_sndbuf_max_get(unsigned int, const char *,
                                   char *);

static te_errno tcp_rcvbuf_def_set(unsigned int, const char *,
                                   const char *);

static te_errno tcp_rcvbuf_def_get(unsigned int, const char *,
                                   char *);

static te_errno tcp_rcvbuf_max_set(unsigned int, const char *,
                                   const char *);

static te_errno tcp_rcvbuf_max_get(unsigned int, const char *,
                                   char *);

static te_errno tcp_sndbuf_def_set(unsigned int, const char *,
                                   const char *);

static te_errno tcp_sndbuf_def_get(unsigned int, const char *,
                                   char *);

static te_errno tcp_sndbuf_max_set(unsigned int, const char *,
                                   const char *);

static te_errno tcp_sndbuf_max_get(unsigned int, const char *,
                                   char *);

static te_errno console_loglevel_set(unsigned int, const char *,
                                     const char *);

static te_errno console_loglevel_get(unsigned int, const char *,
                                     char *);


#define SYSTEM_WIDE_PARAM(_name_, _next) \
    RCF_PCH_CFG_NODE_RW(node_##_name_,               \
                        #_name_,                     \
                        NULL, &node_##_next,         \
                        _name_##_get, _name_##_set); \

RCF_PCH_CFG_NODE_RW(node_udp_rcvbuf_def,
                    "udp_rcvbuf_def",
                    NULL, NULL,
                    udp_rcvbuf_def_get, udp_rcvbuf_def_set);
SYSTEM_WIDE_PARAM(console_loglevel, udp_rcvbuf_def);
SYSTEM_WIDE_PARAM(udp_rcvbuf_max, console_loglevel);
SYSTEM_WIDE_PARAM(udp_sndbuf_def, udp_rcvbuf_max);
SYSTEM_WIDE_PARAM(udp_sndbuf_max, udp_sndbuf_def);
SYSTEM_WIDE_PARAM(tcp_rcvbuf_def, udp_sndbuf_max);
SYSTEM_WIDE_PARAM(tcp_rcvbuf_max, tcp_rcvbuf_def);
SYSTEM_WIDE_PARAM(tcp_sndbuf_def, tcp_rcvbuf_max);
SYSTEM_WIDE_PARAM(tcp_sndbuf_max, tcp_sndbuf_def);
SYSTEM_WIDE_PARAM(rcvbuf_def, tcp_sndbuf_max);
SYSTEM_WIDE_PARAM(rcvbuf_max, rcvbuf_def);
SYSTEM_WIDE_PARAM(sndbuf_def, rcvbuf_max);
SYSTEM_WIDE_PARAM(sndbuf_max, sndbuf_def);
RCF_PCH_CFG_NODE_NA(node_sys, "sys", &node_sndbuf_max, NULL);

te_errno
ta_unix_conf_sys_init(void)
{
#if 0
    /* disable code was disabled as normal linux is a prio */
    /* Temporarily disable to be able to run on openvz host */
    return 0;
#endif
    return rcf_pch_add_node("/agent", &node_sys);
}

/**
 * Set console log level.
 *
 * @param gid          Group identifier (unused)
 * @param oid          Full object instance identifier (unused)
 * @param value        Value of the log level
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
console_loglevel_set(unsigned int gid, const char *oid, const char *value)
{
    int level = atoi(value);

    UNUSED(gid);
    UNUSED(oid);

    if (klogctl(8, NULL, level) < 0)
    {
        ERROR("klogctl: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    return 0;
}

/**
 * Get console log level.
 *
 * @param gid          Group identifier (unused)
 * @param oid          Full object instance identifier (unused)
 * @param value        Value of the log level
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
console_loglevel_get(unsigned int gid, const char *oid, char *value)
{
    int     level = 0;
    char    str[] = {0, 0};
    FILE   *f;
    int     res;

    UNUSED(gid);
    UNUSED(oid);

    f = fopen("/proc/sys/kernel/printk", "r");
    if (f == NULL)
    {
        ERROR("fopen failed: %s", strerror(errno));
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    res = fread(str, 1, 1, f);
    if (res != 1)
    {
        ERROR("fread failed: %s", strerror(errno));
        fclose(f);
        return TE_RC(TE_TA_UNIX, TE_EIO);
    }

    level = atoi(str);
    fclose(f);

    snprintf(value, RCF_MAX_VAL, "%d", level);
    return 0;
}

/**
 * Set or Get the appropriate driver value on Solaris.
 *
 * @param drv           One of 'udp', 'tcp'
 * @param param         An appropriate parameter name:
 *                      udp_xmit_hiwat, udp_recv_hiwat, udp_max_buf,
 *                      tcp_xmit_hiwat, tcp_recv_hiwat, tcp_max_buf
 * @param cmd           What should be done? ND_SET : ND_GET
 * @param value         the value to be set/get as a string
 *
 * @return              Status code.
 */
#if SUN_I_STR
static te_errno
sun_ioctl(char *drv, char *param, int cmd, char *value)
{
    int             fd;
    struct strioctl si;
    int             rc;
    char            dev[80];
    char            cmd_req[80];
    int             flags;

    if (cmd == ND_GET)
    {
        si.ic_cmd = ND_GET;
        flags = O_RDONLY;
        sprintf(cmd_req,"%s%c", param, '\0');
    }
    else
    {
        si.ic_cmd = ND_SET;
        flags = O_WRONLY;
        snprintf(cmd_req, 80, "%s%c%s%c", param, '\0', value, '\0');
    }

    snprintf(dev, 80, "/dev/%s", drv);
    if ((fd = open(dev, flags)) < 0)
        return TE_OS_RC(TE_TA_UNIX, errno);

    si.ic_timout = 0;
    si.ic_len = sizeof(cmd_req);
    si.ic_dp = cmd_req;

    if ((rc = ioctl(fd, I_STR, &si)) < 0)
    {
        close(fd);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    if (cmd == ND_GET)
    {
        strncpy(value, cmd_req, strlen(cmd_req));
    }

    close(fd);
    return 0;
}
#endif

#if __linux__
static int
tcp_mem_get(const char *proc_file, int *par_array, int len)
{
    FILE     *f;
    int       i = 0;
    char     *tmp;
    char     *next_token;
    char     *token;
    char     *save_ptr;

    if ((f = fopen(proc_file, "r")) == NULL)
    {
        ERROR("%s(): Failed to open file %s; errno %d",
               __FUNCTION__, proc_file, errno);
        return -1;
    }

    while (fgets(trash, sizeof(trash), f) != NULL)
    {
        if (i == len)
        {
            ERROR("%s(): %s unexpected format", __FUNCTION__, proc_file);
            fclose(f);
            return -1;
        }
        next_token = strtok_r(trash, "\t", &save_ptr);
        token = trash;
        do {
            par_array[i] = strtol(next_token, &tmp, 10);
            if ((tmp == next_token) && (par_array[i] == 0))
            {
                ERROR("%s(%d:%s): strtol convertation failure",
                      __FUNCTION__, i, next_token);
                fclose(f);
                return -1;
            }
            i++;
            next_token = strtok_r(NULL, "\t", &save_ptr);
        } while ((i != len) || (next_token != NULL));
        if (i != len)
        {
            ERROR("%s(): %s format failure", __FUNCTION__, proc_file);
            fclose(f);
            return -1;
        }
    }
    fclose(f);
    return 0;
}

static int
tcp_mem_set(const char *proc_file, int *par_array, int len)
{
    FILE     *f;
    char      tmp[80];

    if ((f = fopen(proc_file, "w")) == NULL)
    {
        ERROR("%s(): Failed to open file %s; errno %d",
               __FUNCTION__, proc_file, errno);
        return -1;
    }
    if (len == 3)
    {
        snprintf(tmp, 128, "%d\t%d\t%d",
                 par_array[0], par_array[1], par_array[2]);
    }
    else if(len == 1)
    {
        snprintf(tmp, 128, "%d", par_array[0]);
    }
    else
    {
        ERROR("%s(): %s format failure", __FUNCTION__, proc_file);
        fclose(f);
        return -1;
    }

    fputs(tmp, f);

    fclose(f);
    return 0;
}
#endif


/**
 * Set TCP send buffer max size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be set as sndbuf max size
 *
 * @return              Status code
 */
static te_errno
tcp_sndbuf_max_set(unsigned int gid, const char *oid,
                   const char *value)
{
    te_errno  rc = 0;

#if __linux__
    char     *tmp;
    int       bmem[3] = { 0, };
#endif

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_wmem", bmem, 3);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    bmem[2] = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/ipv4/tcp_wmem", bmem, 3);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#elif SUN_I_STR
    rc = sun_ioctl("tcp", "tcp_max_buf", ND_SET, (char *)value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get TCP send buffer max size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be get as sndbuf max size
 *
 * @return              Status code
 */
static te_errno
tcp_sndbuf_max_get(unsigned int gid, const char *oid,
                   char *value)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem[3] = { 0, };
#endif

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_wmem", bmem, 3);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    sprintf(value,"%d", bmem[2]);
#elif SUN_I_STR
    rc = sun_ioctl("tcp", "tcp_max_buf", ND_GET, value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set TCP send buffer default size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be set as sndbuf default size
 *
 * @return              Status code
 */
static te_errno
tcp_sndbuf_def_set(unsigned int gid, const char *oid,
                   const char *value)
{
    te_errno  rc = 0;
#if __linux__
    char     *tmp;
    int       bmem[3] = { 0, };
#endif

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_wmem", bmem, 3);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    bmem[1] = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/ipv4/tcp_wmem", bmem, 3);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#elif SUN_I_STR
    rc = sun_ioctl("tcp", "tcp_xmit_hiwat", ND_SET, (char *)value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get TCP send buffer default size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be get as sndbuf default size
 *
 * @return              Status code
 */
static te_errno
tcp_sndbuf_def_get(unsigned int gid, const char *oid,
                   char *value)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem[3] = { 0, };
#endif

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_wmem", bmem, 3);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    sprintf(value,"%d", bmem[1]);
#elif SUN_I_STR
    rc = sun_ioctl("tcp", "tcp_xmit_hiwat", ND_GET, value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set TCP receive buffer max size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be set as rcvbuf max size
 *
 * @return              Status code
 */
static te_errno
tcp_rcvbuf_max_set(unsigned int gid, const char *oid,
                   const char *value)
{
    te_errno  rc = 0;
#if __linux__
    char     *tmp;
    int       bmem[3] = { 0, };
#endif
    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_rmem", bmem, 3);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    bmem[2] = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/ipv4/tcp_rmem", bmem, 3);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#elif SUN_I_STR
    rc = sun_ioctl("tcp", "tcp_max_buf", ND_SET, (char *)value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get TCP receive buffer max size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be get as rcvbuf max size
 *
 * @return              Status code
 */
static te_errno
tcp_rcvbuf_max_get(unsigned int gid, const char *oid,
                   char *value)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem[3] = { 0, };
#endif

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_rmem", bmem, 3);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    sprintf(value,"%d", bmem[2]);

#elif SUN_I_STR
    rc = sun_ioctl("tcp", "tcp_max_buf", ND_GET, value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set TCP receive buffer default size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be set as rcvbuf default size
 *
 * @return              Status code
 */
static te_errno
tcp_rcvbuf_def_set(unsigned int gid, const char *oid,
                   const char *value)
{
    te_errno  rc = 0;
#if __linux__
    char     *tmp;
    int       bmem[3] = { 0, };
#endif
    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_rmem", bmem, 3);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    bmem[1] = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/ipv4/tcp_rmem", bmem, 3);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#elif SUN_I_STR
    rc = sun_ioctl("tcp", "tcp_recv_hiwat", ND_SET, (char *)value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get TCP receive buffer default size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be get as rcvbuf default size
 *
 * @return              Status code
 */
static te_errno
tcp_rcvbuf_def_get(unsigned int gid, const char *oid,
                   char *value)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem[3] = { 0, };
#endif

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_rmem", bmem, 3);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    sprintf(value,"%d", bmem[1]);
#elif SUN_I_STR
    rc = sun_ioctl("tcp", "tcp_recv_hiwat", ND_GET, value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set socket send buffer max size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be set as sndbuf max size
 *
 * @return              Status code
 */
static te_errno
sndbuf_max_set(unsigned int gid, const char *oid,
               const char *value)
{
    te_errno  rc = 0;
#if __linux__
    char     *tmp;
    int       bmem = 0;
#endif

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = tcp_mem_get("/proc/sys/net/core/wmem_max", &bmem, 1);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    bmem = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/core/wmem_max", &bmem, 1);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get socket send buffer max size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be get as sndbuf max size
 *
 * @return              Status code
 */
static te_errno
sndbuf_max_get(unsigned int gid, const char *oid,
               char *value)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem = 0;
#endif
    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/core/wmem_max", &bmem, 1);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    sprintf(value, "%d", bmem);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set socket send buffer default size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be set as sndbuf default size
 *
 * @return              Status code
 */
static te_errno
sndbuf_def_set(unsigned int gid, const char *oid,
               const char *value)
{
    te_errno  rc = 0;
#if __linux__
    char     *tmp;
    int       bmem = 0;
#endif

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = tcp_mem_get("/proc/sys/net/core/wmem_default", &bmem, 1);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    bmem = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/core/wmem_default", &bmem, 1);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get socket send buffer default size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be get as sndbuf default size
 *
 * @return              Status code
 */
static te_errno
sndbuf_def_get(unsigned int gid, const char *oid,
               char *value)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem = 0;
#endif
    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = tcp_mem_get("/proc/sys/net/core/wmem_default", &bmem, 1);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    sprintf(value, "%d", bmem);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set socket receive buffer max size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be set as rcvbuf max size
 *
 * @return              Status code
 */
static te_errno
rcvbuf_max_set(unsigned int gid, const char *oid,
               const char *value)
{
    te_errno  rc = 0;
#if __linux__
    char     *tmp;
    int       bmem = 0;
#endif
    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = tcp_mem_get("/proc/sys/net/core/rmem_max", &bmem, 1);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    bmem = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/core/rmem_max", &bmem, 1);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get socket receive buffer max size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be get as rcvbuf max size
 *
 * @return              Status code
 */
static te_errno
rcvbuf_max_get(unsigned int gid, const char *oid,
               char *value)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem = 0;
#endif
    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = tcp_mem_get("/proc/sys/net/core/rmem_max", &bmem, 1);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    sprintf(value, "%d", bmem);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set socket receive buffer default size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be set as rcvbuf default size
 *
 * @return              Status code
 */
static te_errno
rcvbuf_def_set(unsigned int gid, const char *oid,
               const char *value)
{
    te_errno  rc = 0;
#if __linux__
    char     *tmp;
    int       bmem = 0;
#endif
    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/core/rmem_default", &bmem, 1);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    bmem = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/core/rmem_default", &bmem, 1);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get socket receive buffer default size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be get as rcvbuf default size
 *
 * @return              Status code
 */
static te_errno
rcvbuf_def_get(unsigned int gid, const char *oid,
               char *value)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem = 0;
#endif
    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = tcp_mem_get("/proc/sys/net/core/rmem_default", &bmem, 1);
    if (rc != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    sprintf(value, "%d", bmem);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set UDP send buffer max size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be set as sndbuf max size
 *
 * @return              Status code
 */
static te_errno
udp_sndbuf_max_set(unsigned int gid, const char *oid,
                   const char *value)
{
    te_errno  rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = sndbuf_max_set(gid, oid, value);
#elif SUN_I_STR
    rc = sun_ioctl("udp", "udp_max_buf", ND_SET, (char *)value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get UDP send buffer max size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be get as sndbuf max size
 *
 * @return              Status code
 */
static te_errno
udp_sndbuf_max_get(unsigned int gid, const char *oid,
                   char *value)
{
    te_errno  rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#if __linux__
    rc = sndbuf_max_get(gid, oid, value);
#elif SUN_I_STR
    rc = sun_ioctl("udp", "udp_max_buf", ND_GET, value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set UDP send buffer default size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be set as sndbuf default size
 *
 * @return              Status code
 */
static te_errno
udp_sndbuf_def_set(unsigned int gid, const char *oid,
                   const char *value)
{
    te_errno  rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = sndbuf_def_set(gid, oid, value);
#elif SUN_I_STR
    rc = sun_ioctl("udp", "udp_xmit_hiwat", ND_SET, (char *)value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get UDP send buffer default size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be get as sndbuf default size
 *
 * @return              Status code
 */
static te_errno
udp_sndbuf_def_get(unsigned int gid, const char *oid,
                   char *value)
{
    te_errno  rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = sndbuf_def_get(gid, oid, value);
#elif SUN_I_STR
    rc = sun_ioctl("udp", "udp_xmit_hiwat", ND_GET, value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set UDP receive buffer max size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be set as rcvbuf max size
 *
 * @return              Status code
 */
static te_errno
udp_rcvbuf_max_set(unsigned int gid, const char *oid,
                   const char *value)
{
    te_errno  rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = rcvbuf_max_set(gid, oid, value);
#elif SUN_I_STR
    rc = sun_ioctl("udp", "udp_max_buf", ND_SET, (char *)value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get UDP receive buffer max size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be get as rcvbuf max size
 *
 * @return              Status code
 */
static te_errno
udp_rcvbuf_max_get(unsigned int gid, const char *oid,
                   char *value)
{
    te_errno  rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = rcvbuf_max_get(gid, oid, value);
#elif SUN_I_STR
    rc = sun_ioctl("udp", "udp_max_buf", ND_GET, value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set UDP receive buffer default size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be set as rcvbuf default size
 *
 * @return              Status code
 */
static te_errno
udp_rcvbuf_def_set(unsigned int gid, const char *oid,
                   const char *value)
{
    te_errno  rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#if __linux__
    rc = rcvbuf_def_set(gid, oid, value);
#elif SUN_I_STR
    rc = sun_ioctl("udp", "udp_recv_hiwat", ND_SET, (char *)value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get UDP receive buffer default size.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         to be get as rcvbuf default size
 *
 * @return              Status code
 */
static te_errno
udp_rcvbuf_def_get(unsigned int gid, const char *oid,
                   char *value)
{
    te_errno  rc = 0;

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#if __linux__
    rc = rcvbuf_def_get(gid, oid, value);
#elif SUN_I_STR
    rc = sun_ioctl("udp", "udp_recv_hiwat", ND_GET, value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}
