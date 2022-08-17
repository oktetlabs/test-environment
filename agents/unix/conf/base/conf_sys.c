/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Some system wide settings
 *
 * Unix TA system wide settings support. Objects defined in
 * this file are obsolete; new interface is defined in conf_sys_tree.c
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

/* Solaris sream interface */
#ifdef HAVE_INET_ND_H
#include <inet/nd.h>
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

#ifdef HAVE_SYS_KLOG_H
#include <sys/klog.h>
#endif

#if __linux__
static char trash[128];
#endif

/*
 * System wide settings both max and default parameters of sndbuf/rcvbuf:
 * Linux UDP: /proc/sys/net/core/
 *             [rmem_max, rmem_default, wmem_max, wmem_default]
 * Solaris UDP: 'ndd' utility
 * Linux TCP: /proc/sys/net/ipv4/
 *             [tcp_rmem, tcp_wmem]
 * Solaris TCP: 'ndd' utility
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

static te_errno proc_sys_common_set(unsigned int, const char *,
                                    const char *);

static te_errno proc_sys_common_get(unsigned int, const char *,
                                    char *);

static te_errno console_loglevel_set(unsigned int, const char *,
                                     const char *);

static te_errno console_loglevel_get(unsigned int, const char *,
                                     char *);

static te_errno core_pattern_set(unsigned int, const char *,
                                     const char *);

static te_errno core_pattern_get(unsigned int, const char *,
                                     char *);


#define SYSTEM_WIDE_PARAM(_name, _next) \
    RCF_PCH_CFG_NODE_RW(node_##_name,               \
                        #_name,                     \
                        NULL, &node_##_next,        \
                        _name##_get, _name##_set);

#define SYSTEM_WIDE_PARAM_COMMON(_name, _next) \
    RCF_PCH_CFG_NODE_RW(node_##_name, #_name,   \
                        NULL, &node_##_next,    \
                        proc_sys_common_get, proc_sys_common_set);

RCF_PCH_CFG_NODE_RW(node_udp_rcvbuf_def,
                    "udp_rcvbuf_def",
                    NULL, NULL,
                    udp_rcvbuf_def_get, udp_rcvbuf_def_set);
SYSTEM_WIDE_PARAM(console_loglevel, udp_rcvbuf_def);
SYSTEM_WIDE_PARAM(core_pattern, console_loglevel);
SYSTEM_WIDE_PARAM(udp_rcvbuf_max, core_pattern);
SYSTEM_WIDE_PARAM(udp_sndbuf_def, udp_rcvbuf_max);
SYSTEM_WIDE_PARAM(udp_sndbuf_max, udp_sndbuf_def);
SYSTEM_WIDE_PARAM(tcp_rcvbuf_def, udp_sndbuf_max);
SYSTEM_WIDE_PARAM(tcp_rcvbuf_max, tcp_rcvbuf_def);
SYSTEM_WIDE_PARAM(tcp_sndbuf_def, tcp_rcvbuf_max);
SYSTEM_WIDE_PARAM(tcp_sndbuf_max, tcp_sndbuf_def);
SYSTEM_WIDE_PARAM_COMMON(tcp_max_syn_backlog, tcp_sndbuf_max);
SYSTEM_WIDE_PARAM_COMMON(neigh_gc_thresh3, tcp_max_syn_backlog);
SYSTEM_WIDE_PARAM_COMMON(somaxconn, neigh_gc_thresh3);
SYSTEM_WIDE_PARAM_COMMON(optmem_max, somaxconn);
SYSTEM_WIDE_PARAM_COMMON(igmp_max_memberships, optmem_max);
SYSTEM_WIDE_PARAM_COMMON(tcp_synack_retries, igmp_max_memberships);
SYSTEM_WIDE_PARAM_COMMON(tcp_syn_retries, tcp_synack_retries);
SYSTEM_WIDE_PARAM_COMMON(tcp_keepalive_time, tcp_syn_retries);
SYSTEM_WIDE_PARAM_COMMON(tcp_keepalive_probes, tcp_keepalive_time);
SYSTEM_WIDE_PARAM_COMMON(tcp_keepalive_intvl, tcp_keepalive_probes);
SYSTEM_WIDE_PARAM_COMMON(tcp_retries2, tcp_keepalive_intvl);
SYSTEM_WIDE_PARAM_COMMON(tcp_orphan_retries, tcp_retries2);
SYSTEM_WIDE_PARAM_COMMON(tcp_fin_timeout, tcp_orphan_retries);
SYSTEM_WIDE_PARAM_COMMON(tcp_syncookies, tcp_fin_timeout);
SYSTEM_WIDE_PARAM_COMMON(tcp_timestamps, tcp_syncookies);
SYSTEM_WIDE_PARAM_COMMON(route_mtu_expires, tcp_timestamps);
SYSTEM_WIDE_PARAM(rcvbuf_def, route_mtu_expires);
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
    int     fd;
    int     rc;
    int     error;

    UNUSED(gid);
    UNUSED(oid);

    fd = open("/proc/sys/kernel/printk", O_WRONLY);
    if (fd < 0)
    {
        error = errno;
        ERROR("open failed: %s", strerror(error));
        return TE_OS_RC(TE_TA_UNIX, error);
    }
    rc = write(fd, value, strlen(value));
    error = errno;
    close(fd);
    if (rc < 0)
    {
        ERROR("write failed to write %d bytes: rc=%d %s",
              strlen(value), rc, strerror(error));
        return TE_OS_RC(TE_TA_UNIX, error);
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
    int     fd;
    ssize_t res;
    int     error;

    UNUSED(gid);
    UNUSED(oid);

    fd = open("/proc/sys/kernel/printk", O_RDONLY);
    if (fd < 0)
    {
        int error = errno;
        ERROR("open failed: %s", strerror(error));
        return TE_OS_RC(TE_TA_UNIX, error);
    }

    res = read(fd, trash, sizeof(trash) - 1);
    error = errno;
    close(fd);
    if (res < 0)
    {
        ERROR("read failed: %s", strerror(error));
        return TE_OS_RC(TE_TA_UNIX, error);
    }

    trash[sizeof(trash) - 1] = '\0';
    level = atoi(trash);

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
#if defined(__sun)
static te_errno
sun_ioctl(char *drv, char *param, int cmd, char *value)
{
    int       out_fd;
    FILE     *fp;
    char      shell_cmd[80];
    te_errno  rc = 0;
    pid_t     pid;
    int       status;

    if (cmd == ND_GET)
        snprintf(shell_cmd, sizeof(shell_cmd),
                 "/usr/sbin/ndd -get /dev/%s %s", drv, param);
    else
        snprintf(shell_cmd, sizeof(shell_cmd),
                 "/usr/sbin/ndd -set /dev/%s %s %s", drv, param, value);

    if ((pid = te_shell_cmd(shell_cmd, -1, NULL, &out_fd, NULL)) < 0)
    {
        ERROR("Failed to execute '%s': (%s)",
              shell_cmd, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    if ((fp = fdopen(out_fd, "r")) == NULL)
    {
        ERROR("Failed to obtain file pointer for shell command output");
        rc = TE_OS_RC(TE_TA_UNIX, errno);
        goto cleanup;
    }

    if (cmd == ND_GET)
    {
        if (fgets(value, RCF_MAX_VAL, fp) == NULL)
        {
            ERROR("Failed to get shell command execution result '%s'",
                  shell_cmd);
            rc = TE_RC(TE_TA_UNIX, TE_EFAULT);
        }
        else
        {
            size_t len = strlen(value);

            /* Cut trailing new line character */
            if (len != 0)
                value[len - 1] = '\0';
        }
    }

cleanup:
    if (fp != NULL)
        fclose(fp);
    close(out_fd);
    ta_waitpid(pid, &status, 0);

    return rc;
}
#endif

#if __linux__
static te_errno
tcp_mem_get(const char *proc_file, int *par_array, int len)
{
    int       fd;
    int       i = 0;
    char     *tmp;
    char     *next_token;
    char     *save_ptr;
    int       res;
    int       error;

    if ((fd = open(proc_file, O_RDONLY)) < 0)
    {
        ERROR("%s(): Failed to open file %s: %s",
               __FUNCTION__, proc_file, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    res = read(fd, trash, sizeof(trash) - 1);
    error = errno;
    close(fd);
    if (res < 0)
    {
        ERROR("%s(): Failed to read file %s: %s",
              __FUNCTION__, proc_file, strerror(error));
        return TE_OS_RC(TE_TA_UNIX, error);
    }
    trash[sizeof(trash) - 1] = '\0';

    next_token = strtok_r(trash, "\t", &save_ptr);
    do {
        par_array[i] = strtol(next_token, &tmp, 10);
        if ((tmp == next_token) && (par_array[i] == 0))
        {
            ERROR("%s(%d:%s): strtol convertation failure",
                  __FUNCTION__, i, next_token);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }
        i++;
        next_token = strtok_r(NULL, "\t", &save_ptr);
    } while ((i != len) || (next_token != NULL));
    if (i != len)
    {
        ERROR("%s(): %s format failure", __FUNCTION__, proc_file);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    return TE_RC(TE_TA_UNIX, TE_EINVAL);
}

static int
tcp_mem_set(const char *proc_file, int *par_array, int len)
{
    int       fd;
    int       error;
    int       res = 0;

    if ((fd = open(proc_file, O_WRONLY)) < 0)
    {
        ERROR("%s(): Failed to open file %s: %s",
               __FUNCTION__, proc_file, strerror(errno));
        return -1;
    }
    if (len == 3)
    {
        res = snprintf(trash, sizeof(trash), "%d\t%d\t%d",
                       par_array[0], par_array[1], par_array[2]);
    }
    else if(len == 1)
    {
        res = snprintf(trash, sizeof(trash), "%d", par_array[0]);
    }
    else
    {
        ERROR("%s(): %s format failure", __FUNCTION__, proc_file);
        close(fd);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    res = write(fd, trash, MIN(res + 1, (int)sizeof(trash)));
    if (res < 0)
    {
        error = errno;
        ERROR("%s(): Failed to write file %s with \"%s\": %s",
              __FUNCTION__, proc_file, trash, strerror(error));
        return TE_OS_RC(TE_TA_UNIX, error);
    }

    close(fd);
    return 0;
}

/**
 * Put a number value in a system file like
 * /proc/sys/net/ipv4/tcp_timestamps
 *
 * @param path      Full path with file name
 * @param value     String with value
 *
 * return Status code
 */
static te_errno
proc_sys_set_value(const char *path, const char *value)
{
    char       *tmp;
    int         bmem = 0;
    int         rc;

    rc = tcp_mem_get(path, &bmem, 1);
    if (rc != 0)
        return rc;

    bmem = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set(path, &bmem, 1);
    if (rc != 0)
        return rc;

    return 0;
}

/**
 * Get a number value from a system file like
 * /proc/sys/net/ipv4/tcp_timestamps
 *
 * @param path      Full path with file name
 * @param value     Buffer for string with value
 *
 * return Status code
 */
static te_errno
proc_sys_get_value(const char *path, char *value)
{
    int bmem = 0;
    int rc;

    rc = tcp_mem_get(path, &bmem, 1);
    if (rc != 0)
        return rc;

    sprintf(value, "%d", bmem);

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
        return rc;

    bmem[2] = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/ipv4/tcp_wmem", bmem, 3);
    if (rc != 0)
        return rc;
#elif defined(__sun)
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
        return rc;

    sprintf(value,"%d", bmem[2]);
#elif defined(__sun)
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
        return rc;

    bmem[1] = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/ipv4/tcp_wmem", bmem, 3);
    if (rc != 0)
        return rc;
#elif defined(__sun)
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
        return rc;

    sprintf(value,"%d", bmem[1]);
#elif defined(__sun)
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
        return rc;

    bmem[2] = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/ipv4/tcp_rmem", bmem, 3);
    if (rc != 0)
        return rc;
#elif defined(__sun)
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
        return rc;

    sprintf(value,"%d", bmem[2]);

#elif defined(__sun)
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
        return rc;

    bmem[1] = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/ipv4/tcp_rmem", bmem, 3);
    if (rc != 0)
        return rc;
#elif defined(__sun)
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
        return rc;

    sprintf(value,"%d", bmem[1]);
#elif defined(__sun)
    rc = sun_ioctl("tcp", "tcp_recv_hiwat", ND_GET, value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Common function to set a value in /proc/sys.
 * Supported nodes: tcp_timestamps, tcp_syncookies, tcp_keepalive_time,
 * tcp_keepalive_probes, tcp_keepalive_intvl, tcp_retries2,
 * tcp_orphan_retries, tcp_fin_timeout, tcp_syn_retries, tcp_synack_retries,
 * tcp_max_syn_backlog,
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         string with a new value
 *
 * @return              Status code
 */
static te_errno
proc_sys_common_set(unsigned int gid, const char *oid,
                    const char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#if __linux__
#define ELSE_IF_IPV4_FIELD(_field) \
    else if (strstr(oid, "/" #_field ":") != NULL) \
        return proc_sys_set_value("/proc/sys/net/ipv4/" #_field, value);

    if (strstr(oid, "/tcp_timestamps:") != NULL)
        return proc_sys_set_value("/proc/sys/net/ipv4/tcp_timestamps",
                                  value);
    else if (strstr(oid, "/somaxconn:") != NULL)
        return proc_sys_set_value("/proc/sys/net/core/somaxconn",
                                  value);
    else if (strstr(oid, "/optmem_max:") != NULL)
        return proc_sys_set_value("/proc/sys/net/core/optmem_max",
                                  value);
    else if (strstr(oid, "/neigh_gc_thresh3:") != NULL)
        return proc_sys_set_value("/proc/sys/net/ipv4/neigh/default/"
                                  "gc_thresh3", value);
    else if (strstr(oid, "/route_mtu_expires:") != NULL)
        return proc_sys_set_value("/proc/sys/net/ipv4/route/mtu_expires",
                                  value);
    ELSE_IF_IPV4_FIELD(tcp_syncookies)
    ELSE_IF_IPV4_FIELD(tcp_keepalive_time)
    ELSE_IF_IPV4_FIELD(tcp_keepalive_probes)
    ELSE_IF_IPV4_FIELD(tcp_keepalive_intvl)
    ELSE_IF_IPV4_FIELD(tcp_retries2)
    ELSE_IF_IPV4_FIELD(tcp_orphan_retries)
    ELSE_IF_IPV4_FIELD(tcp_fin_timeout)
    ELSE_IF_IPV4_FIELD(tcp_syn_retries)
    ELSE_IF_IPV4_FIELD(tcp_synack_retries)
    ELSE_IF_IPV4_FIELD(igmp_max_memberships)
    ELSE_IF_IPV4_FIELD(tcp_max_syn_backlog)
#undef ELSE_IF_IPV4_FIELD

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#else
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Common function to get a value from /proc/sys.
 * Supported nodes: tcp_timestamps, tcp_syncookies, tcp_keepalive_time,
 * tcp_keepalive_probes, tcp_keepalive_intvl, tcp_retries2,
 * tcp_orphan_retries, tcp_fin_timeout, tcp_syn_retries, tcp_synack_retries,
 * tcp_max_syn_backlog,
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         location for obtained value
 *
 * @return              Status code
 */
static te_errno
proc_sys_common_get(unsigned int gid, const char *oid,
                    char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#if __linux__
#define ELSE_IF_IPV4_FIELD(_field) \
    else if (strstr(oid, "/" #_field ":") != NULL) \
        return proc_sys_get_value("/proc/sys/net/ipv4/" #_field, value);

    if (strstr(oid, "/tcp_timestamps:") != NULL)
        return proc_sys_get_value("/proc/sys/net/ipv4/tcp_timestamps",
                                  value);
    else if (strstr(oid, "/somaxconn:") != NULL)
        return proc_sys_get_value("/proc/sys/net/core/somaxconn", value);
    else if (strstr(oid, "/optmem_max:") != NULL)
        return proc_sys_get_value("/proc/sys/net/core/optmem_max",
                                  value);
    else if (strstr(oid, "/neigh_gc_thresh3:") != NULL)
        return proc_sys_get_value("/proc/sys/net/ipv4/neigh/default/"
                                  "gc_thresh3", value);
    else if (strstr(oid, "/route_mtu_expires:") != NULL)
        return proc_sys_get_value("/proc/sys/net/ipv4/route/mtu_expires",
                                  value);
    ELSE_IF_IPV4_FIELD(tcp_syncookies)
    ELSE_IF_IPV4_FIELD(tcp_keepalive_time)
    ELSE_IF_IPV4_FIELD(tcp_keepalive_probes)
    ELSE_IF_IPV4_FIELD(tcp_keepalive_intvl)
    ELSE_IF_IPV4_FIELD(tcp_retries2)
    ELSE_IF_IPV4_FIELD(tcp_orphan_retries)
    ELSE_IF_IPV4_FIELD(tcp_fin_timeout)
    ELSE_IF_IPV4_FIELD(tcp_syn_retries)
    ELSE_IF_IPV4_FIELD(tcp_synack_retries)
    ELSE_IF_IPV4_FIELD(igmp_max_memberships)
    ELSE_IF_IPV4_FIELD(tcp_max_syn_backlog)
#undef ELSE_IF_IPV4_FIELD
#endif

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
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
        return rc;

    bmem = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/core/wmem_max", &bmem, 1);
    if (rc != 0)
        return rc;
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
        return rc;

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
        return rc;

    bmem = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/core/wmem_default", &bmem, 1);
    if (rc != 0)
        return rc;
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
        return rc;

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
        return rc;

    bmem = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/core/rmem_max", &bmem, 1);
    if (rc != 0)
        return rc;
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
        return rc;

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
        return rc;

    bmem = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = tcp_mem_set("/proc/sys/net/core/rmem_default", &bmem, 1);
    if (rc != 0)
        return rc;
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
        return rc;

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
#elif defined(__sun)
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
#elif defined(__sun)
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
#elif defined(__sun)
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
#elif defined(__sun)
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
#elif defined(__sun)
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
#elif defined(__sun)
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
#elif defined(__sun)
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
#elif defined(__sun)
    rc = sun_ioctl("udp", "udp_recv_hiwat", ND_GET, value);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Try to open a file for reading and writing to check
 * whether it is accessible.
 *
 * @param pn        File pathname.
 *
 * @return @c 0 - file is accessible for read/write;
 *         @c -1 - failed to open a file.
 */
static int
try_open_file_rw(const char *pn)
{
    int fd;

    fd = open(pn, O_RDWR);
    if (fd >= 0)
    {
        close(fd);
        return 0;
    }

    return -1;
}

/**
 * Set core pattern used when dumpling a core (because of segmentation
 * fault or something alike).
 *
 * @param gid          Group identifier (unused)
 * @param oid          Full object instance identifier (unused)
 * @param value        The pattern
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
core_pattern_set(unsigned int gid, const char *oid, const char *value)
{
#ifdef __linux__
    int rc = 0;
    int error;
    int fd;
#endif

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#ifdef __linux__

    /*
     * We do not want this node to be available if agent is not
     * run under root. See bug 10419.
     */
    if (try_open_file_rw("/proc/sys/kernel/core_pattern") < 0)
    {
        if (errno == EACCES)
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        else
            return TE_OS_RC(TE_TA_UNIX, errno);
    }

    fd = open("/proc/sys/kernel/core_pattern", O_WRONLY);
    if (fd < 0)
    {
        error = errno;
        ERROR("open(/proc/sys/kernel/core_pattern) failed: %s", strerror(error));
        return TE_OS_RC(TE_TA_UNIX, error);
    }
    rc = write(fd, value, strlen(value) + 1);
    error = errno;
    close(fd);
    if (rc < 0)
    {
        ERROR("write failed to write %d bytes: rc=%d %s",
              strlen(value) + 1, rc, strerror(error));
        return TE_OS_RC(TE_TA_UNIX, error);
    }
    return 0;
#else
    /*
     * In case of Solaris it must be something like
     * /usr/bin/coreadm -g %(pattern)s -e global
     * Other systems were never supported.
     */
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get core pattern used when dumpling a core (because of segmentation
 * fault or something alike).
 *
 * @param gid          Group identifier (unused)
 * @param oid          Full object instance identifier (unused)
 * @param value        The pattern
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
core_pattern_get(unsigned int gid, const char *oid, char *value)
{
#ifdef __linux__
    int  rc = 0;
    int  error;
    int  fd;
    size_t len;
#endif

    UNUSED(gid);
    UNUSED(oid);

    if (value == NULL)
    {
        ERROR("A value to set is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#ifdef __linux__

    /*
     * We do not want this node to be available if agent is not
     * run under root. See bug 10419.
     */
    if (try_open_file_rw("/proc/sys/kernel/core_pattern") < 0)
    {
        if (errno == EACCES)
            return TE_RC(TE_TA_UNIX, TE_ENOENT);
        else
            return TE_OS_RC(TE_TA_UNIX, errno);
    }

    fd = open("/proc/sys/kernel/core_pattern", O_RDONLY);
    if (fd < 0)
    {
        error = errno;
        ERROR("open(/proc/sys/kernel/core_pattern) failed: %s",
              strerror(error));
        return TE_OS_RC(TE_TA_UNIX, error);
    }
    rc = read(fd, trash, sizeof(trash) - 1);
    error = errno;
    close(fd);
    if (rc < 0)
    {
        ERROR("read failed: %s", strerror(error));
        return TE_OS_RC(TE_TA_UNIX, error);
    }
    trash[sizeof(trash) - 1] = '\0';
    len = strnlen(trash, rc);
    if (trash[len - 1] == '\n' || (int)len == rc)
        trash[len - 1] = '\0';
    assert(len < RCF_MAX_VAL);
    memcpy(value, trash, len + 1);
    return 0;
#else
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

