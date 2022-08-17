/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * Unix daemons common internal staff
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TA_UNIX_CONF_DAEMONS_INTERNAL_H__
#define __TE_TA_UNIX_CONF_DAEMONS_INTERNAL_H__

#ifndef TE_LGR_USER
#define TE_LGR_USER      "Daemons"
#endif

#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "te_errno.h"
#include "te_defs.h"
#include "comm_agent.h"
#include "logger_api.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"

#include "unix_internal.h"

#include "conf_daemons.h"

#ifndef PATH_MAX
#define PATH_MAX        4096
#endif

/** Directory where xinetd service configuration files are located */
#define XINETD_ETC_DIR      "/etc/xinetd.d/"

/** Command line for all processes list */
#if defined __linux__
/*
 * On Linux, 'comm' does not provide full path to the executable which
 * is important for us.
 */
#define PS_ALL_COMM     "ps -eo 'args'"
#define PS_ALL_ARGS     "ps -eo 'args'"
#define PS_ALL_PID_ARGS "ps -eo 'pid args'"
#elif defined __sun__
#define PS_ALL_COMM     "ps -eo 'comm'"
#define PS_ALL_ARGS     "ps -eo 'args'"
#define PS_ALL_PID_ARGS "ps -eo 'pid args'"
#elif defined __FreeBSD__
#define PS_ALL_COMM     "ps -axo 'comm'"
#define PS_ALL_ARGS     "ps -axo 'args'"
#define PS_ALL_PID_ARGS "ps -axo 'pid args'"
#error FreeBSD is not supported yet         /**< conf_daemons.c         */
#else
#error Unknown platform (Linux, Solaris, FreeBSD, etc)
#endif

#ifdef WITH_FTP_SERVER
extern const char *get_ftp_daemon_name(void);
#else
#if defined __linux__
#define get_ftp_daemon_name() "ftpd"
#elif defined __sun__
#define get_ftp_daemon_name() "svc:/network/ftp:default"
#endif
#endif

/** Get name of the service by the object identifier */
static inline const char *
get_ds_name(const char *oid)
{
    return
#if defined __linux__
         (strstr(oid, "nginx") != NULL) ? "nginx" :
         (strstr(oid, "l2tp") != NULL) ? "l2tp" :
         (strstr(oid, "openvpn") != NULL) ? "openvpn" :
         (strstr(oid, "socks") != NULL) ? "socks" :
         (strstr(oid, "radvd") != NULL) ? "radvd" :
         (strstr(oid, "dhcpserver") != NULL) ? "dhcpd" :
         (strstr(oid, "pppoeserver") != NULL) ? "pppoe-server" :
         (strstr(oid, "dnsserver") != NULL) ? "named" :
         (strstr(oid, "todudpserver") != NULL) ? "daytime-udp" :
         (strstr(oid, "tftpserver") != NULL) ? "tftp" :
         (strstr(oid, "ftpserver") != NULL) ? get_ftp_daemon_name() :
         (strstr(oid, "telnetd") != NULL) ? "telnet" :
         (strstr(oid, "rshd") != NULL) ? "rsh" :
         (strstr(oid, "echoserver") != NULL) ? "echo" : oid;
#elif defined __sun__
         (strstr(oid, "dhcpserver") != NULL) ? "svc:/network/dhcp-server:default" :
         (strstr(oid, "dnsserver") != NULL) ? "svc:/network/dns/server:default" :
         (strstr(oid, "todudpserver") != NULL) ? "daytime-udp" :
         (strstr(oid, "tftpserver") != NULL) ? "tftp" :
         (strstr(oid, "ftpserver") != NULL) ? get_ftp_daemon_name() :
         (strstr(oid, "telnetd") != NULL) ? "svc:/network/telnet:default" :
         (strstr(oid, "rshd") != NULL) ? "svc:/network/shell:default" :
         (strstr(oid, "echoserver") != NULL) ? "echo" : oid;
#endif
}

/** Check, if the file exists and accessible */
static inline int
file_exists(char *file)
{
    struct stat st;

    return stat(file, &st) == 0;
}

/*--------------- Backup of configuration files --------------------*/

/** Maximum number of services the implemntation supports */
#define UNIX_SERVICE_MAX    16

/** Directory where all TE temporary files are located */
#define TE_TMP_PATH         "/tmp/"

/** Suffix for service backup files */
#define TE_TMP_BKP_SUFFIX   ".te_backup"

/** Suffix for temporary files */
#define TE_TMP_FILE_SUFFIX  ".tmpf"

/**
 * Create backup for the daemon/service configuration file.
 *
 * @param dir           configuration file directory
 * @param name          configuration file basename
 * @param index         location for daemon/service index
 *
 * @return Status code
 */
extern int ds_create_backup(const char *dir, const char *name, int *index);

/**
 * Restore initial state of the service.
 *
 * @param index         service index
 */
extern void ds_restore_backup(int index);

/** Restore initial state of all services */
extern void ds_restore_backups(void);

/**
 * Get configuration file name for the daemon/service.
 *
 * @param index index returned by the ds_create_backup
 *
 * @return Pathname of the configuration file
 */
extern const char *ds_config(int index);

/**
 * Get name of the configuration file name backup for the daemon/service.
 *
 * @param index index returned by the ds_create_backup
 *
 * @return Pathname of the configuration file
 */
extern const char *ds_backup(int index);

/**
 * Check, if the daemon/service configuration file was changed.
 *
 * @param index index returned by the ds_create_backup
 *
 * @return TRUE, if the file was changed
 */
extern te_bool ds_config_changed(int index);


/**
 * Look for registered service with specified configuration directory
 * and file name.
 *
 * @param dir   configuration directory name
 * @param name  service name
 *
 * @return index or -1
 */
extern int ds_lookup(const char *dir, const char *name);

/**
 * Notify backup manager that the configuration file was touched.
 *
 * @param index         daemon/service index
 */
extern void ds_config_touch(int index);

/* Open backup for reading */
#define OPEN_BACKUP(_index, _f) \
    do {                                                        \
        if ((_f = fopen(ds_backup(_index), "r")) == NULL)       \
        {                                                       \
            ERROR("Cannot open file %s for reading; errno %d",  \
                  ds_backup(_index), errno);                    \
            return TE_OS_RC(TE_TA_UNIX, errno);                 \
        }                                                       \
    } while (0)

/* Open config for writing */
#define OPEN_CONFIG(_index, _f) \
    do {                                                        \
        if ((_f = fopen(ds_config(_index), "w")) == NULL)       \
        {                                                       \
            ERROR("Cannot open file %s for writing; errno %d",  \
                  ds_config(_index), errno);                    \
            return TE_OS_RC(TE_TA_UNIX, errno);                 \
        }                                                       \
    } while (0)

/* Daemon handling */

/**
 * Get current state daemon.
 *
 * @param gid   unused
 * @param oid   daemon name
 * @param value value location
 *
 * @return Status code
 */
extern te_errno daemon_get(unsigned int gid, const char *oid, char *value);

/**
 * Start/stop daemon.
 *
 * @param gid   unused
 * @param oid   daemon name
 * @param value new value
 *
 * @return Status code
 */
extern te_errno daemon_set(unsigned int gid, const char *oid,
                           const char *value);

/**
 * Check, if daemon/service is running (enabled).
 *
 * @param daemon    daemon/service name
 *
 * @return TRUE, if daemon is running
 */
static inline te_bool
daemon_running(const char *daemon)
{
    char enable[2];

    if (daemon_get(0, daemon, enable) != 0)
        return 0;

    return enable[0] == '1';
}

/**
 * Find the first existing file in the list.
 *
 * @param n         Number of entries
 * @param files     Array with file names
 * @param exec      Should the file be executable
 *
 * @return Index of the found file or -1
 */
extern int find_file(unsigned int n, const char * const *files,
                     te_bool exec);

/*
 * Grab/release functions for daemons/services - see rcfpch/rcf_pch.h
 * for details and prototypes.
 */
extern te_errno l2tp_grab(const char *name);
extern te_errno l2tp_release(const char *name);

extern te_errno openvpn_grab(const char *name);
extern te_errno openvpn_release(const char *name);

extern te_errno socks_grab(const char *name);
extern te_errno socks_release(const char *name);

extern te_errno radvd_grab(const char *name);
extern te_errno radvd_release(const char *radvd);

extern te_errno dhcpserver_grab(const char *name);
extern te_errno dhcpserver_release(const char *name);

extern te_errno pppoeserver_grab(const char *name);
extern te_errno pppoeserver_release(const char *name);

extern te_errno echoserver_grab(const char *name);
extern te_errno echoserver_release(const char *name);

extern te_errno todudpserver_grab(const char *name);
extern te_errno todudpserver_release(const char *name);

extern te_errno telnetd_grab(const char *name);
extern te_errno telnetd_release(const char *name);

extern te_errno rshd_grab(const char *name);
extern te_errno rshd_release(const char *name);

extern te_errno tftpserver_grab(const char *name);
extern te_errno tftpserver_release(const char *name);

extern te_errno ftpserver_grab(const char *name);
extern te_errno ftpserver_release(const char *name);

extern te_errno smtp_grab(const char *name);
extern te_errno smtp_release(const char *name);

extern te_errno vncserver_grab(const char *name);
extern te_errno vncserver_release(const char *name);

extern te_errno dnsserver_grab(const char *name);
extern te_errno dnsserver_release(const char *name);

extern te_errno radiusserver_grab(const char *name);
extern te_errno radiusserver_release(const char *name);

extern te_errno vtund_grab(const char *name);
extern te_errno vtund_release(const char *name);

/**
 * Add slapd node to the configuration tree.
 *
 * @return Status code.
 */
extern te_errno slapd_add(void);

/**
 * Add PPPoE client support nodes to the configuration tree.
 *
 * @return Status of the operation.
 */
extern te_errno pppoe_client_add(void);

#endif /* __TE_TA_UNIX_CONF_DAEMONS_INTERNAL_H__ */
