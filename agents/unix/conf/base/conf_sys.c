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
#include "rcf_pch_tree.h"
#include "logger_api.h"
#include "unix_internal.h"
#include "te_shell_cmd.h"
#include "te_str.h"

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

/**
 * Get an integer tunable via 'ndd' on Solaris.
 *
 * @param drv           One of 'udp', 'tcp'
 * @param param         Parameter name
 * @param val           Where to save the obtained value
 *
 * @return              Status code.
 */
static te_errno
sun_ioctl_get_int(char *drv, char *param, int32_t *val)
{
    char     strval[RCF_MAX_VAL] = "";
    te_errno rc;

    rc = sun_ioctl(drv, param, ND_GET, strval);
    if (rc != 0)
        return rc;

    return te_strtoi(strval, 10, val);
}

/**
 * Set an integer tunable via 'ndd' on Solaris.
 *
 * @param drv           One of 'udp', 'tcp'
 * @param param         Parameter name
 * @param val           Value to set
 *
 * @return              Status code.
 */
static te_errno
sun_ioctl_set_int(char *drv, char *param, int32_t val)
{
    char strval[RCF_MAX_VAL] = "";

    snprintf(strval, sizeof(strval), "%" PRId32, val);

    return sun_ioctl(drv, param, ND_SET, strval);
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
            ERROR("%s(%d:%s): strtol conversion failure",
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
 * Get a number value from a system file like
 * /proc/sys/net/ipv4/tcp_timestamps, holding a single field.
 *
 * @param path      Full path with file name
 * @param val       Where to save the obtained value
 *
 * return Status code
 */
static te_errno
proc_sys_common_get(const char *path, int32_t *val)
{
    int      bmem = 0;
    te_errno rc;

    rc = tcp_mem_get(path, &bmem, 1);
    if (rc != 0)
        return rc;

    *val = bmem;

    return 0;
}

/**
 * Put a number value in a system file like
 * /proc/sys/net/ipv4/tcp_timestamps, holding a single field.
 *
 * @param path      Full path with file name
 * @param val       Value to set
 *
 * return Status code
 */
static te_errno
proc_sys_common_set(const char *path, int32_t val)
{
    int      bmem = 0;
    te_errno rc;

    rc = tcp_mem_get(path, &bmem, 1);
    if (rc != 0)
        return rc;

    bmem = val;

    return tcp_mem_set(path, &bmem, 1);
}
#endif

/**
 * Define a get/set pair for a /proc/sys/ file holding a single
 * numeric field, dispatched by node identity rather than by parsing
 * the OID.
 */
#define SYS_COMMON_FIELD(_name, _path) \
static te_errno                                                       \
_name##_get(const ta_conf_ctx *ctx, int32_t *val)                           \
{                                                                      \
    UNUSED(ctx);                                                      \
    UNUSED(val);                                                      \
    return IF_LINUX_COMMON_GET(_path);                                \
}                                                                      \
                                                                        \
static te_errno                                                       \
_name##_set(const ta_conf_ctx *ctx, int32_t val)                            \
{                                                                      \
    UNUSED(ctx);                                                      \
    UNUSED(val);                                                      \
    return IF_LINUX_COMMON_SET(_path);                                \
}

#if __linux__
#define IF_LINUX_COMMON_GET(_path) proc_sys_common_get(_path, val)
#define IF_LINUX_COMMON_SET(_path) proc_sys_common_set(_path, val)
#else
#define IF_LINUX_COMMON_GET(_path) TE_RC(TE_TA_UNIX, TE_ENOENT)
#define IF_LINUX_COMMON_SET(_path) TE_RC(TE_TA_UNIX, TE_ENOSYS)
#endif

SYS_COMMON_FIELD(route_mtu_expires, "/proc/sys/net/ipv4/route/mtu_expires")
SYS_COMMON_FIELD(tcp_timestamps, "/proc/sys/net/ipv4/tcp_timestamps")
SYS_COMMON_FIELD(tcp_syncookies, "/proc/sys/net/ipv4/tcp_syncookies")
SYS_COMMON_FIELD(tcp_fin_timeout, "/proc/sys/net/ipv4/tcp_fin_timeout")
SYS_COMMON_FIELD(tcp_orphan_retries, "/proc/sys/net/ipv4/tcp_orphan_retries")
SYS_COMMON_FIELD(tcp_retries2, "/proc/sys/net/ipv4/tcp_retries2")
SYS_COMMON_FIELD(tcp_keepalive_intvl, "/proc/sys/net/ipv4/tcp_keepalive_intvl")
SYS_COMMON_FIELD(tcp_keepalive_probes,
                 "/proc/sys/net/ipv4/tcp_keepalive_probes")
SYS_COMMON_FIELD(tcp_keepalive_time, "/proc/sys/net/ipv4/tcp_keepalive_time")
SYS_COMMON_FIELD(tcp_syn_retries, "/proc/sys/net/ipv4/tcp_syn_retries")
SYS_COMMON_FIELD(tcp_synack_retries, "/proc/sys/net/ipv4/tcp_synack_retries")
SYS_COMMON_FIELD(igmp_max_memberships,
                 "/proc/sys/net/ipv4/igmp_max_memberships")
SYS_COMMON_FIELD(optmem_max, "/proc/sys/net/core/optmem_max")
SYS_COMMON_FIELD(somaxconn, "/proc/sys/net/core/somaxconn")
SYS_COMMON_FIELD(busy_poll, "/proc/sys/net/core/busy_poll")
SYS_COMMON_FIELD(busy_read, "/proc/sys/net/core/busy_read")
SYS_COMMON_FIELD(neigh_gc_thresh3,
                 "/proc/sys/net/ipv4/neigh/default/gc_thresh3")
SYS_COMMON_FIELD(tcp_max_syn_backlog,
                 "/proc/sys/net/ipv4/tcp_max_syn_backlog")

#undef SYS_COMMON_FIELD
#undef IF_LINUX_COMMON_GET
#undef IF_LINUX_COMMON_SET

/**
 * Get socket send buffer max size.
 *
 * @param ctx           request context (unused)
 * @param val           to be got as sndbuf max size
 *
 * @return              Status code
 */
static te_errno
sndbuf_max_get(const ta_conf_ctx *ctx, int32_t *val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem = 0;
#endif

    UNUSED(ctx);

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/core/wmem_max", &bmem, 1);
    if (rc != 0)
        return rc;

    *val = bmem;
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set socket send buffer max size.
 *
 * @param ctx           request context (unused)
 * @param val           to be set as sndbuf max size
 *
 * @return              Status code
 */
static te_errno
sndbuf_max_set(const ta_conf_ctx *ctx, int32_t val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem;
#endif

    UNUSED(ctx);

#if __linux__
    bmem = val;
    rc = tcp_mem_set("/proc/sys/net/core/wmem_max", &bmem, 1);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get socket send buffer default size.
 *
 * @param ctx           request context (unused)
 * @param val           to be got as sndbuf default size
 *
 * @return              Status code
 */
static te_errno
sndbuf_def_get(const ta_conf_ctx *ctx, int32_t *val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem = 0;
#endif
    UNUSED(ctx);

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/core/wmem_default", &bmem, 1);
    if (rc != 0)
        return rc;

    *val = bmem;
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set socket send buffer default size.
 *
 * @param ctx           request context (unused)
 * @param val           to be set as sndbuf default size
 *
 * @return              Status code
 */
static te_errno
sndbuf_def_set(const ta_conf_ctx *ctx, int32_t val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem;
#endif

    UNUSED(ctx);

#if __linux__
    bmem = val;
    rc = tcp_mem_set("/proc/sys/net/core/wmem_default", &bmem, 1);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get socket receive buffer max size.
 *
 * @param ctx           request context (unused)
 * @param val           to be got as rcvbuf max size
 *
 * @return              Status code
 */
static te_errno
rcvbuf_max_get(const ta_conf_ctx *ctx, int32_t *val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem = 0;
#endif
    UNUSED(ctx);

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/core/rmem_max", &bmem, 1);
    if (rc != 0)
        return rc;

    *val = bmem;
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set socket receive buffer max size.
 *
 * @param ctx           request context (unused)
 * @param val           to be set as rcvbuf max size
 *
 * @return              Status code
 */
static te_errno
rcvbuf_max_set(const ta_conf_ctx *ctx, int32_t val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem;
#endif
    UNUSED(ctx);

#if __linux__
    bmem = val;
    rc = tcp_mem_set("/proc/sys/net/core/rmem_max", &bmem, 1);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get socket receive buffer default size.
 *
 * @param ctx           request context (unused)
 * @param val           to be got as rcvbuf default size
 *
 * @return              Status code
 */
static te_errno
rcvbuf_def_get(const ta_conf_ctx *ctx, int32_t *val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem = 0;
#endif
    UNUSED(ctx);

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/core/rmem_default", &bmem, 1);
    if (rc != 0)
        return rc;

    *val = bmem;
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set socket receive buffer default size.
 *
 * @param ctx           request context (unused)
 * @param val           to be set as rcvbuf default size
 *
 * @return              Status code
 */
static te_errno
rcvbuf_def_set(const ta_conf_ctx *ctx, int32_t val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem;
#endif
    UNUSED(ctx);

#if __linux__
    bmem = val;
    rc = tcp_mem_set("/proc/sys/net/core/rmem_default", &bmem, 1);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get TCP send buffer max size.
 *
 * @param ctx           request context (unused)
 * @param val           to be got as sndbuf max size
 *
 * @return              Status code
 */
static te_errno
tcp_sndbuf_max_get(const ta_conf_ctx *ctx, int32_t *val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem[3] = { 0, };
#endif

    UNUSED(ctx);

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_wmem", bmem, 3);
    if (rc != 0)
        return rc;

    *val = bmem[2];
#elif defined(__sun)
    rc = sun_ioctl_get_int("tcp", "tcp_max_buf", val);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set TCP send buffer max size.
 *
 * @param ctx           request context (unused)
 * @param val           to be set as sndbuf max size
 *
 * @return              Status code
 */
static te_errno
tcp_sndbuf_max_set(const ta_conf_ctx *ctx, int32_t val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem[3] = { 0, };
#endif

    UNUSED(ctx);

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_wmem", bmem, 3);
    if (rc != 0)
        return rc;

    bmem[2] = val;

    rc = tcp_mem_set("/proc/sys/net/ipv4/tcp_wmem", bmem, 3);
#elif defined(__sun)
    rc = sun_ioctl_set_int("tcp", "tcp_max_buf", val);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get TCP send buffer default size.
 *
 * @param ctx           request context (unused)
 * @param val           to be got as sndbuf default size
 *
 * @return              Status code
 */
static te_errno
tcp_sndbuf_def_get(const ta_conf_ctx *ctx, int32_t *val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem[3] = { 0, };
#endif

    UNUSED(ctx);

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_wmem", bmem, 3);
    if (rc != 0)
        return rc;

    *val = bmem[1];
#elif defined(__sun)
    rc = sun_ioctl_get_int("tcp", "tcp_xmit_hiwat", val);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set TCP send buffer default size.
 *
 * @param ctx           request context (unused)
 * @param val           to be set as sndbuf default size
 *
 * @return              Status code
 */
static te_errno
tcp_sndbuf_def_set(const ta_conf_ctx *ctx, int32_t val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem[3] = { 0, };
#endif

    UNUSED(ctx);

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_wmem", bmem, 3);
    if (rc != 0)
        return rc;

    bmem[1] = val;

    rc = tcp_mem_set("/proc/sys/net/ipv4/tcp_wmem", bmem, 3);
#elif defined(__sun)
    rc = sun_ioctl_set_int("tcp", "tcp_xmit_hiwat", val);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get TCP receive buffer max size.
 *
 * @param ctx           request context (unused)
 * @param val           to be got as rcvbuf max size
 *
 * @return              Status code
 */
static te_errno
tcp_rcvbuf_max_get(const ta_conf_ctx *ctx, int32_t *val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem[3] = { 0, };
#endif

    UNUSED(ctx);

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_rmem", bmem, 3);
    if (rc != 0)
        return rc;

    *val = bmem[2];
#elif defined(__sun)
    rc = sun_ioctl_get_int("tcp", "tcp_max_buf", val);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set TCP receive buffer max size.
 *
 * @param ctx           request context (unused)
 * @param val           to be set as rcvbuf max size
 *
 * @return              Status code
 */
static te_errno
tcp_rcvbuf_max_set(const ta_conf_ctx *ctx, int32_t val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem[3] = { 0, };
#endif
    UNUSED(ctx);

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_rmem", bmem, 3);
    if (rc != 0)
        return rc;

    bmem[2] = val;

    rc = tcp_mem_set("/proc/sys/net/ipv4/tcp_rmem", bmem, 3);
#elif defined(__sun)
    rc = sun_ioctl_set_int("tcp", "tcp_max_buf", val);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get TCP receive buffer default size.
 *
 * @param ctx           request context (unused)
 * @param val           to be got as rcvbuf default size
 *
 * @return              Status code
 */
static te_errno
tcp_rcvbuf_def_get(const ta_conf_ctx *ctx, int32_t *val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem[3] = { 0, };
#endif

    UNUSED(ctx);

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_rmem", bmem, 3);
    if (rc != 0)
        return rc;

    *val = bmem[1];
#elif defined(__sun)
    rc = sun_ioctl_get_int("tcp", "tcp_recv_hiwat", val);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
    return rc;
}

/**
 * Set TCP receive buffer default size.
 *
 * @param ctx           request context (unused)
 * @param val           to be set as rcvbuf default size
 *
 * @return              Status code
 */
static te_errno
tcp_rcvbuf_def_set(const ta_conf_ctx *ctx, int32_t val)
{
    te_errno  rc = 0;
#if __linux__
    int       bmem[3] = { 0, };
#endif
    UNUSED(ctx);

#if __linux__
    rc = tcp_mem_get("/proc/sys/net/ipv4/tcp_rmem", bmem, 3);
    if (rc != 0)
        return rc;

    bmem[1] = val;

    rc = tcp_mem_set("/proc/sys/net/ipv4/tcp_rmem", bmem, 3);
#elif defined(__sun)
    rc = sun_ioctl_set_int("tcp", "tcp_recv_hiwat", val);
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
    return rc;
}

/**
 * Get UDP send buffer max size (same underlying value as sndbuf_max
 * on Linux).
 *
 * @param ctx           request context
 * @param val           to be got as sndbuf max size
 *
 * @return              Status code
 */
static te_errno
udp_sndbuf_max_get(const ta_conf_ctx *ctx, int32_t *val)
{
#if __linux__
    return sndbuf_max_get(ctx, val);
#elif defined(__sun)
    UNUSED(ctx);
    return sun_ioctl_get_int("udp", "udp_max_buf", val);
#else
    UNUSED(ctx);
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
}

/**
 * Set UDP send buffer max size (same underlying value as sndbuf_max
 * on Linux).
 *
 * @param ctx           request context
 * @param val           to be set as sndbuf max size
 *
 * @return              Status code
 */
static te_errno
udp_sndbuf_max_set(const ta_conf_ctx *ctx, int32_t val)
{
#if __linux__
    return sndbuf_max_set(ctx, val);
#elif defined(__sun)
    UNUSED(ctx);
    return sun_ioctl_set_int("udp", "udp_max_buf", val);
#else
    UNUSED(ctx);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get UDP send buffer default size (same underlying value as
 * sndbuf_def on Linux).
 *
 * @param ctx           request context
 * @param val           to be got as sndbuf default size
 *
 * @return              Status code
 */
static te_errno
udp_sndbuf_def_get(const ta_conf_ctx *ctx, int32_t *val)
{
#if __linux__
    return sndbuf_def_get(ctx, val);
#elif defined(__sun)
    UNUSED(ctx);
    return sun_ioctl_get_int("udp", "udp_xmit_hiwat", val);
#else
    UNUSED(ctx);
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
}

/**
 * Set UDP send buffer default size (same underlying value as
 * sndbuf_def on Linux).
 *
 * @param ctx           request context
 * @param val           to be set as sndbuf default size
 *
 * @return              Status code
 */
static te_errno
udp_sndbuf_def_set(const ta_conf_ctx *ctx, int32_t val)
{
#if __linux__
    return sndbuf_def_set(ctx, val);
#elif defined(__sun)
    UNUSED(ctx);
    return sun_ioctl_set_int("udp", "udp_xmit_hiwat", val);
#else
    UNUSED(ctx);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get UDP receive buffer max size (same underlying value as
 * rcvbuf_max on Linux).
 *
 * @param ctx           request context
 * @param val           to be got as rcvbuf max size
 *
 * @return              Status code
 */
static te_errno
udp_rcvbuf_max_get(const ta_conf_ctx *ctx, int32_t *val)
{
#if __linux__
    return rcvbuf_max_get(ctx, val);
#elif defined(__sun)
    UNUSED(ctx);
    return sun_ioctl_get_int("udp", "udp_max_buf", val);
#else
    UNUSED(ctx);
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
}

/**
 * Set UDP receive buffer max size (same underlying value as
 * rcvbuf_max on Linux).
 *
 * @param ctx           request context
 * @param val           to be set as rcvbuf max size
 *
 * @return              Status code
 */
static te_errno
udp_rcvbuf_max_set(const ta_conf_ctx *ctx, int32_t val)
{
#if __linux__
    return rcvbuf_max_set(ctx, val);
#elif defined(__sun)
    UNUSED(ctx);
    return sun_ioctl_set_int("udp", "udp_max_buf", val);
#else
    UNUSED(ctx);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Get UDP receive buffer default size (same underlying value as
 * rcvbuf_def on Linux).
 *
 * @param ctx           request context
 * @param val           to be got as rcvbuf default size
 *
 * @return              Status code
 */
static te_errno
udp_rcvbuf_def_get(const ta_conf_ctx *ctx, int32_t *val)
{
#if __linux__
    return rcvbuf_def_get(ctx, val);
#elif defined(__sun)
    UNUSED(ctx);
    return sun_ioctl_get_int("udp", "udp_recv_hiwat", val);
#else
    UNUSED(ctx);
    return TE_RC(TE_TA_UNIX, TE_ENOENT);
#endif
}

/**
 * Set UDP receive buffer default size (same underlying value as
 * rcvbuf_def on Linux).
 *
 * @param ctx           request context
 * @param val           to be set as rcvbuf default size
 *
 * @return              Status code
 */
static te_errno
udp_rcvbuf_def_set(const ta_conf_ctx *ctx, int32_t val)
{
#if __linux__
    return rcvbuf_def_set(ctx, val);
#elif defined(__sun)
    UNUSED(ctx);
    return sun_ioctl_set_int("udp", "udp_recv_hiwat", val);
#else
    UNUSED(ctx);
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
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
 * @param ctx          request context (unused)
 * @param val          The pattern
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
core_pattern_set(const ta_conf_ctx *ctx, const char *val)
{
#ifdef __linux__
    int rc = 0;
    int error;
    int fd;
#endif

    UNUSED(ctx);

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
    rc = write(fd, val, strlen(val) + 1);
    error = errno;
    close(fd);
    if (rc < 0)
    {
        ERROR("write failed to write %d bytes: rc=%d %s",
              strlen(val) + 1, rc, strerror(error));
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
 * @param ctx          request context (unused)
 * @param val          The pattern
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
core_pattern_get(const ta_conf_ctx *ctx, te_string *val)
{
#ifdef __linux__
    int  rc = 0;
    int  error;
    int  fd;
    size_t len;
#endif

    UNUSED(ctx);

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
    te_string_append(val, "%s", trash);
    return 0;
#else
    return TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif
}

/**
 * Set console log level.
 *
 * @param ctx          request context (unused)
 * @param val          Value of the log level
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
console_loglevel_set(const ta_conf_ctx *ctx, const char *val)
{
    int     fd;
    int     rc;
    int     error;

    UNUSED(ctx);

    fd = open("/proc/sys/kernel/printk", O_WRONLY);
    if (fd < 0)
    {
        error = errno;
        ERROR("open failed: %s", strerror(error));
        return TE_OS_RC(TE_TA_UNIX, error);
    }
    rc = write(fd, val, strlen(val));
    error = errno;
    close(fd);
    if (rc < 0)
    {
        ERROR("write failed to write %d bytes: rc=%d %s",
              strlen(val), rc, strerror(error));
        return TE_OS_RC(TE_TA_UNIX, error);
    }
    return 0;
}

/**
 * Get console log level.
 *
 * @param ctx          request context (unused)
 * @param val          Value of the log level
 *
 * @return Status code
 * @retval 0        Success
 */
static te_errno
console_loglevel_get(const ta_conf_ctx *ctx, te_string *val)
{
    int     level = 0;
    int     fd;
    ssize_t res;
    int     error;

    UNUSED(ctx);

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

    te_string_append(val, "%d", level);
    return 0;
}

static const ta_conf_node *const node_sys =
    TA_CONF_NA("sys",
        TA_CONF_RW_INT32("sndbuf_max", sndbuf_max_get, sndbuf_max_set),
        TA_CONF_RW_INT32("sndbuf_def", sndbuf_def_get, sndbuf_def_set),
        TA_CONF_RW_INT32("rcvbuf_max", rcvbuf_max_get, rcvbuf_max_set),
        TA_CONF_RW_INT32("rcvbuf_def", rcvbuf_def_get, rcvbuf_def_set),
        TA_CONF_RW_INT32("route_mtu_expires", route_mtu_expires_get,
                       route_mtu_expires_set),
        TA_CONF_RW_INT32("tcp_timestamps",
                       tcp_timestamps_get, tcp_timestamps_set),
        TA_CONF_RW_INT32("tcp_syncookies",
                       tcp_syncookies_get, tcp_syncookies_set),
        TA_CONF_RW_INT32("tcp_fin_timeout", tcp_fin_timeout_get,
                       tcp_fin_timeout_set),
        TA_CONF_RW_INT32("tcp_orphan_retries", tcp_orphan_retries_get,
                       tcp_orphan_retries_set),
        TA_CONF_RW_INT32("tcp_retries2", tcp_retries2_get, tcp_retries2_set),
        TA_CONF_RW_INT32("tcp_keepalive_intvl", tcp_keepalive_intvl_get,
                       tcp_keepalive_intvl_set),
        TA_CONF_RW_INT32("tcp_keepalive_probes", tcp_keepalive_probes_get,
                       tcp_keepalive_probes_set),
        TA_CONF_RW_INT32("tcp_keepalive_time", tcp_keepalive_time_get,
                       tcp_keepalive_time_set),
        TA_CONF_RW_INT32("tcp_syn_retries", tcp_syn_retries_get,
                       tcp_syn_retries_set),
        TA_CONF_RW_INT32("tcp_synack_retries", tcp_synack_retries_get,
                       tcp_synack_retries_set),
        TA_CONF_RW_INT32("igmp_max_memberships", igmp_max_memberships_get,
                       igmp_max_memberships_set),
        TA_CONF_RW_INT32("optmem_max", optmem_max_get, optmem_max_set),
        TA_CONF_RW_INT32("somaxconn", somaxconn_get, somaxconn_set),
        TA_CONF_RW_INT32("busy_poll", busy_poll_get, busy_poll_set),
        TA_CONF_RW_INT32("busy_read", busy_read_get, busy_read_set),
        TA_CONF_RW_INT32("neigh_gc_thresh3", neigh_gc_thresh3_get,
                       neigh_gc_thresh3_set),
        TA_CONF_RW_INT32("tcp_max_syn_backlog", tcp_max_syn_backlog_get,
                       tcp_max_syn_backlog_set),
        TA_CONF_RW_INT32("tcp_sndbuf_max",
                       tcp_sndbuf_max_get, tcp_sndbuf_max_set),
        TA_CONF_RW_INT32("tcp_sndbuf_def",
                       tcp_sndbuf_def_get, tcp_sndbuf_def_set),
        TA_CONF_RW_INT32("tcp_rcvbuf_max",
                       tcp_rcvbuf_max_get, tcp_rcvbuf_max_set),
        TA_CONF_RW_INT32("tcp_rcvbuf_def",
                       tcp_rcvbuf_def_get, tcp_rcvbuf_def_set),
        TA_CONF_RW_INT32("udp_sndbuf_max",
                       udp_sndbuf_max_get, udp_sndbuf_max_set),
        TA_CONF_RW_INT32("udp_sndbuf_def",
                       udp_sndbuf_def_get, udp_sndbuf_def_set),
        TA_CONF_RW_INT32("udp_rcvbuf_max",
                       udp_rcvbuf_max_get, udp_rcvbuf_max_set),
        TA_CONF_RW_STR("core_pattern", core_pattern_get, core_pattern_set),
        TA_CONF_RW_STR("console_loglevel", console_loglevel_get,
                       console_loglevel_set),
        TA_CONF_RW_INT32("udp_rcvbuf_def",
                       udp_rcvbuf_def_get, udp_rcvbuf_def_set));

te_errno
ta_unix_conf_sys_init(void)
{
#if 0
    /* disable code was disabled as normal linux is a prio */
    /* Temporarily disable to be able to run on openvz host */
    return 0;
#endif
    return ta_conf_register("/agent", node_sys);
}
