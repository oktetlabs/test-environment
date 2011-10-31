/** @file
 * @brief Unix Test Agent
 *
 * DHCP server configuring
 *
 * Copyright (C) 2004, 2005 Test Environment authors (see file AUTHORS
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
/* Still debugging, it switches off debug messages */
#ifdef WITH_DHCP_SERVER

#include "conf_daemons.h"
#include "dhcp_server.h"

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <libgen.h>

/*** Defines ***/
/**
 * Define it to use DHCP server native configuration:
 * - parse/backup/update/rollback of existing configuration file(s) and
 *   database of leases;
 * - use /etc/init.d/ script to start/stop daemon.
 *
 * Otherwise, DHCP server is stopped at TA start up and empty private
 * configuration file and leases database are used.
 */
#undef TA_UNIX_ISC_DHCPS_NATIVE_CFG

#define TE_DHCPD_CONF_FILENAME          "/tmp/te.dhcpd.conf"
#define TE_DHCPD_LEASES_FILENAME        "/tmp/te.dhcpd.leases"
#define TE_DHCPD6_LEASES_FILENAME       "/tmp/te.dhcpd6.leases"
#define TE_DHCPD_PID_FILENAME           "/var/run/dhcpd.pid"
#define TE_DHCPD6_PID_FILENAME          "/var/run/dhcpd6.pid"

#define TE_DHCP_SERVER_SCRIPT_NAME_1    "/etc/init.d/isc-dhcp-server"
#define TE_DHCP_SERVER_SCRIPT_NAME_2    "/etc/init.d/dhcpd"
#define TE_DHCP_SERVER_SCRIPT_NAME_3    "/etc/init.d/dhcp3-server"
#define TE_DHCP_SERVER_SCRIPT_NAME_4    "/etc/init.d/dhcp"

#define TE_DHCP_SERVER_EXEC_NAME_1      "/usr/sbin/dhcpd"
#define TE_DHCP_SERVER_EXEC_NAME_2      "/usr/sbin/dhcpd3"
#define TE_DHCP_SERVER_EXEC_NAME_3      "/usr/lib/inet/in.dhcpd"

#define TE_DHCP_SERVER_CONF_NAME_1      "/etc/dhcpd.conf"
#define TE_DHCP_SERVER_CONF_NAME_2      "/etc/dhcp3/dhcpd.conf"
#define TE_DHCP_SERVER_CONF_NAME_3      "/etc/inet/dhcpsvc.conf"

#define TE_DHCP_SERVER_AUX_CONF_NAME_1  "/etc/sysconfig/dhcpd"
#define TE_DHCP_SERVER_AUX_CONF_NAME_2  "/etc/default/dhcp3-server"

/** Check that dhcp server is initialised. Initialise if not. */
#define DHCP_SERVER_INIT_CHECK \
    do {                                                                    \
        int rc;                                                             \
        if ((rc = dhcpserver_init()) != 0)                                  \
        {                                                                   \
            ERROR("Failed to initialise dhcpserver structures, rc=%r", rc); \
            return rc;                                                      \
        }                                                                   \
    } while (0)

/** Release all memory allocated for option structure */
#define FREE_OPTION(_opt) \
    do {                        \
        free(_opt->name);       \
        free(_opt->value);      \
        free(_opt);             \
    } while (0)

/*** Macro definitions for uniformal configuring methods ***/
/** Definition of list method for host and groups */
#define LIST_METHOD(_gh) \
static te_errno \
ds_##_gh##_list(unsigned int gid, const char *oid, char **list) \
{                                                               \
    _gh *gh;                                                    \
                                                                \
    UNUSED(oid);                                                \
    UNUSED(gid);                                                \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    *buf = '\0';                                                \
                                                                \
    for (gh = _gh##s; gh != NULL; gh = gh->next)                \
    {                                                           \
        sprintf(buf + strlen(buf), "%s ",  gh->name);           \
    }                                                           \
                                                                \
    return (*list = strdup(buf)) == NULL ?                      \
               TE_RC(TE_TA_UNIX, TE_ENOMEM) : 0;                \
}

/** Definition of add method for host and groups */
#define ADD_METHOD(_gh) \
static te_errno \
ds_##_gh##_add(unsigned int gid, const char *oid, const char *value,    \
               const char *dhcpserver, const char *name)                \
{                                                                       \
    _gh *gh;                                                            \
                                                                        \
    UNUSED(gid);                                                        \
    UNUSED(oid);                                                        \
    UNUSED(dhcpserver);                                                 \
    UNUSED(value);                                                      \
                                                                        \
    DHCP_SERVER_INIT_CHECK;                                             \
                                                                        \
    if ((gh = find_##_gh(name)) != NULL)                                \
        return TE_RC(TE_TA_UNIX, TE_EEXIST);                            \
                                                                        \
    if ((gh = (_gh *)calloc(1, sizeof(_gh))) == NULL)                   \
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);                            \
                                                                        \
    if ((gh->name = strdup(name)) == NULL)                              \
    {                                                                   \
        free(gh);                                                       \
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);                            \
    }                                                                   \
                                                                        \
    gh->next = _gh##s;                                                  \
    _gh##s = gh;                                                        \
                                                                        \
    dhcp_server_changed = TRUE;                                         \
                                                                        \
    return 0;                                                           \
}

/** Definition of delete method for host and groups */
#define DEL_METHOD(_gh) \
static te_errno \
ds_##_gh##_del(unsigned int gid, const char *oid,       \
               const char *dhcpserver, const char *name)\
{                                                       \
    _gh *gh;                                            \
    _gh *prev;                                          \
                                                        \
    UNUSED(gid);                                        \
    UNUSED(oid);                                        \
    UNUSED(dhcpserver);                                 \
                                                        \
    DHCP_SERVER_INIT_CHECK;                             \
                                                        \
    for (gh = _gh##s, prev = NULL;                      \
         gh != NULL && strcmp(gh->name, name) != 0;     \
         prev = gh, gh = gh->next);                     \
                                                        \
    if (gh == NULL)                                     \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);            \
                                                        \
    if (prev)                                           \
        prev->next = gh->next;                          \
    else                                                \
        _gh##s = gh->next;                              \
    free_##_gh(gh);                                     \
                                                        \
    dhcp_server_changed = TRUE;                         \
                                                        \
    return 0;                                           \
}

/*
 * Definition of the routines for obtaining/changing of
 * host/group/subnet attributes (except options).
 */
#define ATTR_GET(_attr, _ghs, _ghs_type) \
static te_errno \
ds_##_ghs##_##_attr##_get(unsigned int gid, const char *oid,    \
                          char *value, const char *dhcpserver,  \
                          const char *name)                     \
{                                                               \
    _ghs_type *ghs;                                             \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    if ((ghs = find_##_ghs(name)) == NULL)                      \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    if (ghs->_attr == NULL)                                     \
        *value = 0;                                             \
    else                                                        \
        strcpy(value, ghs->_attr);                              \
                                                                \
    return 0;                                                   \
}

#define SSS(S) #S

#define ATTR_SET(_attr, _ghs, _ghs_type) \
static te_errno \
ds_##_ghs##_##_attr##_set(unsigned int gid, const char *oid,    \
                          const char *value,                    \
                          const char *dhcpserver,               \
                          const char *name)                     \
{                                                               \
    _ghs_type  *ghs;                                            \
    char *old_val;                                              \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    if ((ghs = find_##_ghs(name)) == NULL)                      \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    old_val = ghs->_attr;                                       \
                                                                \
    if (*value == 0)                                            \
        ghs->_attr = NULL;                                      \
    else                                                        \
    {                                                           \
        if ((ghs->_attr = strdup(value)) == NULL)               \
        {                                                       \
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);                \
        }                                                       \
    }                                                           \
                                                                \
    free(old_val);                                              \
                                                                \
    dhcp_server_changed = TRUE;                                 \
                                                                \
    return 0;                                                   \
}

/**
 * Definition of the method for obtaining of options list
 * for the host/group/subnet.
 */
#define GET_OPT_LIST(_ghs, _ghs_type, _opt_type) \
static te_errno \
ds_##_ghs##_option_list(unsigned int gid, const char *oid,      \
                       char **list, const char *dhcpserver,     \
                       const char *name)                        \
{                                                               \
   _ghs_type *ghs;                                              \
                                                                \
   _opt_type *opt;                                              \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    if ((ghs = find_##_ghs(name)) == NULL)                      \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    *buf = 0;                                                   \
    for (opt = ghs->options; opt != NULL; opt = opt->next)      \
    {                                                           \
        sprintf(buf + strlen(buf), "%s ", opt->name);           \
    }                                                           \
                                                                \
    return (*list = strdup(buf)) == NULL ?                      \
              TE_RC(TE_TA_UNIX, TE_ENOMEM) : 0;                 \
}

/* Method for adding of the option for group/host/subnet */
#define ADD_OPT(_ghs, _ghs_type) \
static te_errno \
ds_##_ghs##_option_add(unsigned int gid, const char *oid,       \
                      const char *value, const char *dhcpserver,\
                      const char *name, const char *optname)    \
{                                                               \
    _ghs_type *ghs;                                             \
                                                                \
    te_dhcp_option *opt;                                        \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    if (*value == 0)                                            \
        return TE_RC(TE_TA_UNIX, TE_EINVAL);                    \
                                                                \
    if ((ghs = find_##_ghs(name)) == NULL)                      \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    if (find_option(ghs->options, optname) != NULL)             \
        return TE_RC(TE_TA_UNIX, TE_EEXIST);                    \
                                                                \
    if ((opt = (te_dhcp_option *)calloc(sizeof(*opt), 1))       \
        == NULL || (opt->name = strdup(optname)) == NULL ||     \
        (opt->value = strdup(value)) == NULL)                   \
    {                                                           \
        FREE_OPTION(opt);                                       \
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);                    \
    }                                                           \
                                                                \
    opt->next = ghs->options;                                   \
    ghs->options = opt;                                         \
                                                                \
    dhcp_server_changed = TRUE;                                 \
                                                                \
    return 0;                                                   \
}

/* Method for deletion of the option value for group/host/subnet */
#define DEL_OPT(_ghs, _ghs_type) \
static te_errno      \
ds_##_ghs##_option_del(unsigned int gid, const char *oid,       \
                      const char *dhcpserver, const char *name, \
                      const char *optname)                      \
{                                                               \
    _ghs_type *ghs;                                             \
                                                                \
    te_dhcp_option *opt, *prev;                                 \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    if ((ghs = find_##_ghs(name)) == NULL)                      \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    for (opt = ghs->options, prev = NULL;                       \
         opt != NULL && strcmp(opt->name, optname) != 0;        \
         prev = opt, opt = opt->next);                          \
                                                                \
    if (opt == NULL)                                            \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    if (prev)                                                   \
        prev->next = opt->next;                                 \
    else                                                        \
        ghs->options = opt->next;                               \
                                                                \
    FREE_OPTION(opt);                                           \
                                                                \
    dhcp_server_changed = TRUE;                                 \
                                                                \
    return 0;                                                   \
}

/* Method for changing of the option value for group/host/subnet */
#define SET_OPT(_ghs, _ghs_type) \
static te_errno \
ds_##_ghs##_option_set(unsigned int gid, const char *oid,       \
                      const char *value, const char *dhcpserver,\
                      const char *name, const char *optname)    \
{                                                               \
    _ghs_type *ghs;                                             \
                                                                \
    char *old;                                                  \
                                                                \
    te_dhcp_option *opt;                                        \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    if ((ghs = find_##_ghs(name)) == NULL)                      \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    if ((opt = find_option(ghs->options, optname)) == NULL)     \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    old = opt->value;                                           \
    if (*value == 0)                                            \
        opt->value = NULL;                                      \
    else                                                        \
    {                                                           \
        if ((opt->value = strdup(value)) == NULL)               \
        {                                                       \
            opt->value = old;                                   \
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);                \
        }                                                       \
    }                                                           \
                                                                \
    free(old);                                                  \
                                                                \
    dhcp_server_changed = TRUE;                                 \
                                                                \
    return 0;                                                   \
}

/* Method for obtaining of the option for group/host/subnet */
#define GET_OPT(_ghs, _ghs_type) \
static te_errno \
ds_##_ghs##_option_get(unsigned int gid, const char *oid,       \
                      char *value, const char *dhcpserver,      \
                      const char *name, const char *optname)    \
{                                                               \
    _ghs_type *ghs;                                             \
                                                                \
    te_dhcp_option *opt;                                        \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
    UNUSED(dhcpserver);                                         \
                                                                \
    DHCP_SERVER_INIT_CHECK;                                     \
                                                                \
    if ((ghs = find_##_ghs(name)) == NULL)                      \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    if ((opt = find_option(ghs->options, optname)) == NULL)     \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
                                                                \
    strcpy(value, opt->value);                                  \
                                                                \
    return 0;                                                   \
}

/* Body for methods returning integer lease attributes */
#define GET_INT_LEASE_ATTR(_attr) \
{                                                       \
    int rc;                                             \
    int res;                                            \
                                                        \
    dhcpctl_data_string val;                            \
                                                        \
    UNUSED(gid);                                        \
    UNUSED(oid);                                        \
    UNUSED(dhcpserver);                                 \
                                                        \
    if ((rc = open_lease(name)) != 0)                   \
        return rc;                                      \
                                                        \
    CHECKSTATUS(dhcpctl_get_value(&val, lo, _attr));    \
    memcpy(&res, val->value, val->len);                 \
    res = ntohl(res);                                   \
    sprintf(value, "%d", *(int *)(val->value));         \
    dhcpctl_data_string_dereference(&val, MDL);         \
                                                        \
    return 0;                                           \
}

/*** Prototypes ***/
static te_bool  check_dhcpserver_init(te_bool);
static te_errno dhcpserver_init(void);
static rcf_pch_cfg_object node_ds_dhcpserver;
static te_bool ds_dhcpserver_is_run(void);
static te_errno ds_dhcpserver_script_stop(void);

/*** Globals ***/
/**
 * IPv6 subnets are specified.
 * We must start dhcpd in dhcpv6 mode (with '-6' key )
 */
static te_bool ipv6_subnets = FALSE;

/** List of known possible locations of DHCP server scripts */
static const char *dhcp_server_scripts[] = {
    TE_DHCP_SERVER_SCRIPT_NAME_1,
    TE_DHCP_SERVER_SCRIPT_NAME_2,
    TE_DHCP_SERVER_SCRIPT_NAME_3,
    TE_DHCP_SERVER_SCRIPT_NAME_4
};

/** List of known possible locations of DHCP server executables */
static const char *dhcp_server_execs[] = {
    TE_DHCP_SERVER_EXEC_NAME_1,
    TE_DHCP_SERVER_EXEC_NAME_2,
    TE_DHCP_SERVER_EXEC_NAME_3
};

#if defined TA_UNIX_ISC_DHCPS_NATIVE_CFG || defined __sun__
/** List of known possible locations of DHCP server configuration file */
static const char *dhcp_server_confs[] = {
    TE_DHCP_SERVER_CONF_NAME_1,
    TE_DHCP_SERVER_CONF_NAME_2,
    TE_DHCP_SERVER_CONF_NAME_3
};
#endif /* !TA_UNIX_ISC_DHCPS_NATIVE_CFG || __sun__ */

#ifdef TA_UNIX_ISC_DHCPS_NATIVE_CFG
/**
 * List of known possible locations of DHCP server auxilury
 * configuration file
 */
static const char *dhcp_server_aux_confs[] = {
    TE_DHCP_SERVER_AUX_CONF_NAME_1,
    TE_DHCP_SERVER_AUX_CONF_NAME_2
};
#endif /* !TA_UNIX_ISC_DHCPS_NATIVE_CFG */

/** Number of known possible locations of DHCP server scripts */
static unsigned int dhcp_server_n_scripts =
    sizeof(dhcp_server_scripts) / sizeof(dhcp_server_scripts[0]);

/** Number of known possible locations of DHCP server executables */
static unsigned int dhcp_server_n_execs =
    sizeof(dhcp_server_execs) / sizeof(dhcp_server_execs[0]);

#if defined TA_UNIX_ISC_DHCPS_NATIVE_CFG || defined __sun__
/** Number of known possible locations of DHCP server configuration file */
static unsigned int dhcp_server_n_confs =
    sizeof(dhcp_server_confs) / sizeof(dhcp_server_confs[0]);
#endif /* !TA_UNIX_ISC_DHCPS_NATIVE_CFG || __sun__ */

#ifdef TA_UNIX_ISC_DHCPS_NATIVE_CFG
/**
 * Number of known possible locations of DHCP server auxilury
 * configuration file
 */
static unsigned int dhcp_server_n_aux_confs =
    sizeof(dhcp_server_aux_confs) / sizeof(dhcp_server_aux_confs[0]);
#endif /* !TA_UNIX_ISC_DHCPS_NATIVE_CFG */

/** DHCP server script name */
static const char *dhcp_server_script = NULL;

/** DHCP server executable name */
static const char *dhcp_server_exec = NULL;

/** DHCP server configuration file name */
static const char *dhcp_server_conf = NULL;

#if defined __linux__
/** DHCP server leases database file name */
static const char *dhcp_server_leases = NULL;

/** DHCPv6 server leases database file name */
static const char *dhcp6_server_leases = NULL;
#endif

#if defined TA_UNIX_ISC_DHCPS_NATIVE_CFG || defined __sun__
/** Index of the DHCP server configuration file backup */
static int dhcp_server_conf_backup = -1;
#endif /* !TA_UNIX_ISC_DHCPS_NATIVE_CFG || __sun__ */

#ifdef TA_UNIX_ISC_DHCPS_NATIVE_CFG
/** DHCP server auxilury configuration file name */
static const char *dhcp_server_aux_conf = NULL;

/** Index of the DHCP server auxilury configuration file backup */
static int dhcp_server_aux_conf_backup = -1;
#else
/** Was DHCP server enabled at TA start up? */
static te_bool dhcp_server_was_run = FALSE;
#endif

/** DHCP server admin status */
static te_bool dhcp_server_started = FALSE;

/** Changed flag for DHCP server configuration */
static te_bool dhcp_server_changed = FALSE;

#if defined __linux__
/**
 * List of options, which should be quoted automatilally; for other
 * option quotes should be specified in value, if necessary.
 */
static char *isc_dhcp_quoted_options[] = {
    "bootfile-name",
    "domain-name",
    "extension-path-name",
    "merit-dump",
    "nis-domain",
    "nisplus-domain",
    "root-path",
    "uap-servers",
    "tftp-server-name",
    "uap-servers",
    "fqdn.fqdn"
};
#endif

/** DHCP server interfaces */
static char *dhcp_server_ifs = NULL;

static TAILQ_HEAD(te_dhcp_server_subnets, te_dhcp_server_subnet) subnets;

static host *hosts = NULL;

static group *groups = NULL;

static space *spaces = NULL;

/** Auxiliary buffer */
static char buf[2048];

#ifdef TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED
static FILE *f = NULL;  /* Pointer to opened /etc/dhcpd.conf */
#endif /* TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED */

/*** Functions ***/
/*** Structures and variables initialisation functions ***/
/**
 * Fuction to manage hidden variable 'dhcp_server_initialised'.
 * Returns current value before modification. Assignes it TRUE
 * if modify == TRUE
 */
static te_bool
check_dhcpserver_init(te_bool modify)
{
    /** DHCP server admin status */
    static te_bool  dhcp_server_initialised = FALSE;
    te_bool         retval;

    retval = dhcp_server_initialised;

    if (modify)
        dhcp_server_initialised = TRUE;

    return retval;
}

static te_errno
dhcpserver_init(void)
{
    int rc = 0;

    if (check_dhcpserver_init(FALSE))
    {
        /* Already initialised. Nothing to do. */
        return 0;
    }

    TAILQ_INIT(&subnets);

    /* Find DHCP server executable */
    if ((rc = find_file(dhcp_server_n_execs,
                        dhcp_server_execs, TRUE)) < 0)
    {
        ERROR("Failed to find DHCP server executable"
              " - DHCP will not be available");
        rcf_pch_del_node(&node_ds_dhcpserver);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    dhcp_server_exec = dhcp_server_execs[rc];

    /* Find DHCP server script */
    if ((rc = find_file(dhcp_server_n_scripts,
                        dhcp_server_scripts, TRUE)) < 0)
    {
        ERROR("Failed to find DHCP server script"
              " - DHCP will not be available");
        rcf_pch_del_node(&node_ds_dhcpserver);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    dhcp_server_script = dhcp_server_scripts[rc];

#if defined __sun__
    /* FIXME (original 'dhcpsvc.conf' gets erased after epilog) */
    if ((rc = ta_system("touch /etc/inet/dhcpsvc.conf")) != 0)
        return rc;
    if ((rc = ta_system("if test ! -d /var/mydhcp; "
                        "then mkdir -p /var/mydhcp; fi")) != 0)
        return rc;
#endif /* !__sun__ */

#if defined TA_UNIX_ISC_DHCPS_NATIVE_CFG || defined __sun__
    /* Find DHCP server configuration file */
    if ((rc = find_file(dhcp_server_n_confs,
                        dhcp_server_confs, FALSE)) < 0)
    {
        ERROR("Failed to find DHCP server configuration file"
             " - DHCP will not be available");
        rcf_pch_del_node(&node_ds_dhcpserver);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
    dhcp_server_conf = dhcp_server_confs[rc];
#endif /* !(defined TA_UNIX_ISC_DHCPS_NATIVE_CFG || defined __sun__) */

#if defined __sun__
    if((rc = ds_create_backup("/etc/inet/", "dhcpsvc.conf",
                              &dhcp_server_conf_backup)) != 0)
        return rc;
#elif defined __linux__
#if defined TA_UNIX_ISC_DHCPS_NATIVE_CFG
    /* Test existing configuration file and leases DB */
    snprintf(buf, sizeof(buf), "%s -q -t -T", dhcp_server_exec);
    if (ta_system(buf) != 0)
    {
        ERROR("Bad found DHCP server configution file '%s'"
             " - DHCP will not be available", dhcp_server_conf);
        rcf_pch_del_node(&node_ds_dhcpserver);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if ((rc = isc_dhcp_server_cfg_parse(dhcp_server_conf)) != 0)
    {
        ERROR("Failed to parse DHCP server configuration file '%s'"
              " - DHCP will not be available", dhcp_server_conf);
        ds_shutdown_dhcp_server();
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
#else
    dhcp_server_conf = TE_DHCPD_CONF_FILENAME;
    dhcp_server_leases = TE_DHCPD_LEASES_FILENAME;
    dhcp6_server_leases = TE_DHCPD6_LEASES_FILENAME;
#define OPEN_LEASES_FILE(_leases_filename) \
    do {                                                                    \
        struct passwd  *p;                                                  \
        int             f;                                                  \
                                                                            \
        if ((f = creat(_leases_filename, 00666)) < 0)                       \
        {                                                                   \
            ERROR("Failed to open '"                                        \
                  #_leases_filename                                         \
                  "' for writing: %s", strerror(errno));                    \
                                                                            \
            rcf_pch_del_node(&node_ds_dhcpserver);                          \
                                                                            \
            return TE_OS_RC(TE_TA_UNIX, errno);                             \
        }                                                                   \
                                                                            \
        close(f);                                                           \
                                                                            \
        if ((p = getpwnam("dhcpd")) != NULL)                                \
        {                                                                   \
            (void)chown(_leases_filename, p->pw_uid, p->pw_gid);            \
        }                                                                   \
    } while (0)
    OPEN_LEASES_FILE(dhcp_server_leases);
    OPEN_LEASES_FILE(dhcp6_server_leases);
#undef OPEN_LEASES_FILE

    if (ds_dhcpserver_is_run())
    {
        if ((rc = ds_dhcpserver_script_stop()) != 0)
        {
            ERROR("Failed to stop DHCP server"
                  " - DHCP will not be available");
            rcf_pch_del_node(&node_ds_dhcpserver);
            return rc;
        }
        dhcp_server_was_run = TRUE;
    }
#endif /* TA_UNIX_ISC_DHCPS_NATIVE_CFG */
#endif

    check_dhcpserver_init(TRUE);
    dhcp_server_changed = TRUE;

    return 0;
}

/*** Utilities ***/
#if defined __linux__
/* Check, if the option should be quoted */
static te_bool
is_quoted(const char *opt_name)
{
    unsigned int i;

    for (i = 0; i < sizeof(isc_dhcp_quoted_options) / sizeof(char *); i++)
        if (strcmp(opt_name, isc_dhcp_quoted_options[i]) == 0)
            return TRUE;

    return FALSE;
}
#endif

/* Find the host in the list */
static host *
find_host(const char *name)
{
    host *h;

    for (h = hosts; h != NULL && strcmp(h->name, name) != 0; h = h->next);

    return h;
}

/* Find the group in the list */
static group *
find_group(const char *name)
{
    group *g;

    for (g = groups; g != NULL && strcmp(g->name, name) != 0; g = g->next);

    return g;
}

/* Find the space in the list */
static space *
find_space(const char *name)
{
    space *s;

    for (s = spaces; s != NULL && strcmp(s->name, name) != 0; s = s->next);

    return s;
}

/* Find the option in specified options list */
static te_dhcp_option *
find_option(te_dhcp_option *opt, const char *name)
{
    for (; opt != NULL && strcmp(opt->name, name) != 0; opt = opt->next);

    return opt;
}

/* Find the space option in specified options list */
static te_dhcp_space_opt *
find_space_option(te_dhcp_space_opt *opt, const char *name)
{
    for (; opt != NULL && strcmp(opt->name, name) != 0; opt = opt->next);

    return opt;
}

/* Release all memory allocated for host structure */
static void
free_host(host *h)
{
    te_dhcp_option *opt, *next;

    free(h->name);
    free(h->chaddr);
    free(h->client_id);
    free(h->ip_addr);
    free(h->next_server);
    free(h->filename);
    free(h->flags);
    for (opt = h->options; opt != NULL; opt = next)
    {
        next = opt->next;
        FREE_OPTION(opt);
    }
}

static void
free_group(group * g)
{
    te_dhcp_option *opt, *next;

    free(g->name);
    free(g->next_server);
    free(g->filename);
    for (opt = g->options; opt != NULL; opt = next)
    {
        next = opt->next;
        FREE_OPTION(opt);
    }
}

static void
free_space(space *sp)
{
    te_dhcp_space_opt *opt, *next;
    free(sp->name);
    for (opt = sp->options; opt != NULL; opt = next)
    {
        next = opt->next;
        free(opt->name);
        free(opt->type);
        free(opt);
    }
}

static void
free_subnet(te_dhcp_server_subnet *s)
{
    te_dhcp_option *opt, *next;

    free(s->subnet);
    free(s->range);
    for (opt = s->options; opt != NULL; opt = next)
    {
        next = opt->next;
        FREE_OPTION(opt);
    }
    free(s);
}

static te_dhcp_server_subnet *
find_subnet(const char *subnet)
{
    te_dhcp_server_subnet  *s;

    for (s = TAILQ_FIRST(&subnets);
         s != NULL && strcmp(s->subnet, subnet) != 0;
         s = TAILQ_NEXT(s, links));

    return s;
}

#ifdef TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED
/** Open lease object with specified IP address */
static int
open_lease(const char *name)
{
    dhcpctl_data_string ip = NULL;

    isc_result_t rc;
    unsigned int addr;

    if (conn == NULL)
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    if (inet_aton(name, (struct in_addr *)&addr) == 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    omapi_data_string_new(&ip, sizeof(addr), MDL);
    memcpy(ip->value, &addr, sizeof(addr));
    CHECKSTATUS(dhcpctl_set_value(lo, ip, "ip-address"));
    CHECKSTATUS(dhcpctl_open_object(lo, conn, 0));
    CHECKSTATUS(dhcpctl_wait_for_completion(lo, &rc));
    CHECKSTATUS(rc);
    dhcpctl_data_string_dereference(&ip, MDL);

    return 0;
}
#endif /* TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED */

static inline const char *
get_last_label(const char *oid)
{
    const char *last_label;
    if (NULL == oid)
        return NULL;
    last_label = rindex(oid, '/');
    if (NULL == last_label)
        return oid;
    last_label++; /* shift to the label begin */
    return last_label;
}

/*** Manage dhcpd execution and configuration ***/
/** Save configuration to the file */
static int
ds_dhcpserver_save_conf(void)
{
    te_dhcp_server_subnet  *s;
    host                   *h;
    FILE                   *f;
#if defined __linux__
    te_dhcp_option         *opt;
    space                  *sp;
    te_bool                 ipv4_subnets;
    /*
     * No need 'ipv4_subnets' to be global like 'ipv6_subnets'.
     * Used only here to check consistency of dhcpd configuration.
     */
#elif defined __sun__
    int                    rc;
    char                   *p = dhcp_server_ifs;
#endif

    INFO("%s()", __FUNCTION__);

    if ((f = fopen(dhcp_server_conf, "w")) == NULL)
    {
        ERROR("Failed to open '%s' for writing: %s",
              dhcp_server_conf, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

#if defined __linux__
    ipv6_subnets = FALSE;
    ipv4_subnets = FALSE;

    /*
     * Hardcoded 'deny unknown-clients' to start server
     * with empty configuration
     */
    fprintf(f, "deny unknown-clients;\n\n");
    fprintf(f, "\n");

    /* Vendor option space specifications */
    for (sp = spaces; sp != NULL; sp = sp->next)
    {
        te_dhcp_space_opt *sp_opt;
        fprintf(f, "option space %s;\n",sp->name);
        for (sp_opt = sp->options; sp_opt != NULL; sp_opt = sp_opt->next)
            fprintf(f, "option %s code %d = %s;\n",
                    sp_opt->name, sp_opt->code, sp_opt->type);
    }

    TAILQ_FOREACH(s, &subnets, links)
    {
        struct in_addr  mask;
        struct in6_addr addr;
        int             pton_retval;
        te_bool         match;
        te_bool         ipv4_subnet;

#define FAMILY_MATCH(_family, _addr_str, _af_match) \
        if ((pton_retval = inet_pton(_family, _addr_str, &addr)) == -1) \
        {                                                               \
            return TE_OS_RC(TE_TA_UNIX, errno);                         \
        }                                                               \
        else if (pton_retval == 1)                                      \
        {                                                               \
            _af_match = TRUE;                                           \
        }                                                               \
        else                                                            \
        {                                                               \
            _af_match = FALSE;                                          \
        }
        do {
            FAMILY_MATCH(AF_INET, s->subnet, match)
            if (match)
            {
                ipv4_subnet = TRUE;
                break;
            }

            FAMILY_MATCH(AF_INET6, s->subnet, match)
            if (match)
            {
                ipv4_subnet = FALSE;
                break;
            }

            ERROR("%s(): failed to detect address family "
                  "in given subnet specification '%s'",
                  __FUNCTION__, s->subnet);
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        } while (0);
#undef FAMILY_MATCH

        /* Open 'subnet' or 'subnet6' specification block */
        if (ipv4_subnet && !ipv6_subnets)
        {
            /*
             * 'subnet' (DHCPv4) specification is allowed:
             * because no 'subnet6' (DHCPv6) specifications done.
             * Assign 'ipv4_subnets = TRUE' to forbid further
             * 'subnet6' specifications.
             */
            ipv4_subnets = TRUE;
            mask.s_addr = htonl(PREFIX2MASK(s->prefix_len));
            /* Add 'subnet' specification */
            fprintf(f, "subnet %s netmask %s {\n",
                    s->subnet, inet_ntop(AF_INET, &mask, buf, sizeof(buf)));
        }
        else if (!ipv4_subnet && !ipv4_subnets)
        {
            /*
             * 'subnet6' (DHCPv6) specification is allowed:
             * because no 'subnet' (DHCPv4) specifications done.
             * Assign 'ipv6_subnets = TRUE' to forbid further
             * 'subnet' specifications.
             */
            ipv6_subnets = TRUE;
            /* Add 'subnet6' specification */
            fprintf(f, "subnet6 %s/%d {\n",
                    s->subnet, s->prefix_len);
        }
        else
        {
            /*
             * Error cases:
             * 1) ipv4_subnet && ipv6_subnets - try to add 'subnet'
             *    when one or more 'subnet6' specifications exist
             * 2) !ipv4_subnet && ipv4_subnets - try to add 'subnet6'
             *    when one or more 'subnet' specifications exist
             */
            ERROR("%s(): configuration inconsistency: mixed "
                  "'subnet' and 'subnet6' specifications", __FUNCTION__);
            /*
             * This is fatal. Daemon dhcpd will not start with
             * like inconsistent configurations. We must return with
             * error.
             */
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
        }

        /* Address range in IPv4/IPv6 subnet */
        if (s->range != NULL)
        {
            fprintf(f, (ipv4_subnet) ?
                            "\trange %s;\n" :
                                "\trange6 %s;\n", s->range);
        }

        /* Options in subnet specification block */
        for (opt = s->options; opt != NULL; opt = opt->next)
        {
            te_bool quoted = is_quoted(opt->name);

            fprintf(f, "\toption %s %s%s%s;\n", opt->name,
                    quoted ? "\"" : "", opt->value, quoted ? "\"" : "");
        }

        /* Vendor specific options in subnet specification block */
        if (s->vos != NULL)
        {
            fprintf(f, "\tvendor-option-space %s;\n", s->vos);
        }

        /* Close 'subnet' or 'subnet6' specification block */
        fprintf(f, "}\n");
    }

    if (!ipv4_subnets && !ipv6_subnets)
    {
        /*
         * Not fatal. We may report error and start
         * with empty configuration.
         */
        ERROR("%s(): configuration inconsistency: neither "
              "'subnet' nor 'subnet6' specifications, "
              "all 'host' specifications will be skipped", __FUNCTION__);
    }
    else
    {
        /*
         * Continue with 'host' specifications when one or more
         * 'subnet' or 'subnet6' specifications done.
         */
        fprintf(f, "\n");

        for (h = hosts; h != NULL; h = h->next)
        {
            /* Open 'host' specification block */
            fprintf(f, "host %s {\n", h->name);

            /*
             * Due to consistency check we have
             * ipv4_subnets == !ipv6_subnets.
             * In all cases when ipv4_subnets == ipv6_subnets function
             * returns or skips 'host' specifications.
             */
            /* DHCPv4 specific */
            if (h->chaddr)
            {
                if (!ipv6_subnets)
                {
                    fprintf(f, "\thardware ethernet %s;\n", h->chaddr);
                }
                else
                {
                    ERROR("%s(): configuration inconsistency: "
                          "'hardware ethernet' is forbidden "
                          "in DHCPv6 mode", __FUNCTION__);
                    return TE_RC(TE_TA_UNIX, TE_EINVAL);
                }
            }

            if (h->client_id)
            {
                if (!ipv6_subnets)
                {
                    fprintf(f, "\tclient-id %s;\n", h->client_id);
                }
                else
                {
                    ERROR("%s(): configuration inconsistency: "
                          "'client-id' is forbidden "
                          "in DHCPv6 mode", __FUNCTION__);
                    return TE_RC(TE_TA_UNIX, TE_EINVAL);
                }
            }

            if (h->next_server)
            {
                if (!ipv6_subnets)
                {
                    fprintf(f, "\tnext-server %s;\n", h->next_server);
                }
                else
                {
                    ERROR("%s(): configuration inconsistency: "
                          "'next-server' is forbidden "
                          "in DHCPv6 mode", __FUNCTION__);
                    return TE_RC(TE_TA_UNIX, TE_EINVAL);
                }
            }

            /* DHCPv6 specific */
            if (h->host_id)
            {
                if (ipv6_subnets)
                {
                    fprintf(f,
                            "\thost-identifier option dhcp6.client-id %s;\n",
                            h->host_id);
                }
                else
                {
                    ERROR("%s(): configuration inconsistency: "
                          "'host-identifier' is forbidden "
                          "in DHCPv4 mode", __FUNCTION__);
                    return TE_RC(TE_TA_UNIX, TE_EINVAL);
                }
            }

            if (h->prefix6)
            {
                if (ipv6_subnets)
                {
                    fprintf(f, "\tfixed-prefix6 %s;\n", h->prefix6);
                }
                else
                {
                    ERROR("%s(): configuration inconsistency: "
                          "'fixed-prefix6' is forbidden "
                          "in DHCPv4 mode", __FUNCTION__);
                    return TE_RC(TE_TA_UNIX, TE_EINVAL);
                }
            }

            /* Common */
            if (h->ip_addr)
            {
                fprintf(f,
                        (ipv6_subnets) ?
                            "\tfixed-address6 %s;\n" :
                                "\tfixed-address %s;\n",
                        h->ip_addr);
            }

            if (h->filename)
            {
                fprintf(f, "\tfilename \"%s\";\n", h->filename);
            }

            for (opt = h->options; opt != NULL; opt = opt->next)
            {
                te_bool quoted = is_quoted(opt->name);

                fprintf(f, "\toption %s %s%s%s;\n", opt->name,
                        quoted ? "\"" : "", opt->value, quoted ? "\"" : "");
            }

            /* Close 'host' specification block */
            fprintf(f, "}\n");
        }
    }

    fprintf(f, "\n");

    if (fsync(fileno(f)) != 0)
    {
        int err = errno;

        ERROR("%s(): fsync() failed: %s", __FUNCTION__, strerror(err));
        (void)fclose(f);
        return TE_OS_RC(TE_TA_UNIX, err);
    }

    if (fclose(f) != 0)
    {
        ERROR("%s(): fclose() failed: %s", __FUNCTION__, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
#elif defined __sun__
    ds_config_touch(dhcp_server_conf_backup);

    if (p != NULL)
        for (p = strchr(p, ' '); p != NULL; p = strchr(p, ' '))
          *p = ',';

    fprintf(f,
            "BOOTP_COMPAT=automatic\n"
            "DAEMON_ENABLED=TRUE\n"
            "RESOURCE=SUNWbinfiles\n"
            "RUN_MODE=server\n"
            "PATH=/var/mydhcp\n" /** FIXME */
            "CONVER=1\n"
            "INTERFACES=%s\n",
            dhcp_server_ifs != NULL ? dhcp_server_ifs : "");

    if (fsync(fileno(f)) != 0)
    {
        int err = errno;

        ERROR("%s(): fsync() failed: %s", __FUNCTION__, strerror(err));
        (void)fclose(f);
        return TE_OS_RC(TE_TA_UNIX, err);
    }

    if (fclose(f) != 0)
    {
        ERROR("%s(): fclose() failed: %s", __FUNCTION__, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    ta_system("rm -f /var/mydhcp/*"); /** FIXME */

    TAILQ_FOREACH(s, &subnets, links)
    {
        struct in_addr  mask;

        TE_SPRINTF(buf, "/usr/sbin/pntadm -C %s", s->subnet);
        if ((rc = ta_system(buf)) != 0)
            return rc;

#if 0
        mask.s_addr = htonl(PREFIX2MASK(s->prefix_len));
        /* FIXME ('/etc/inet/netmasks' must be maintained) */
        add_xxx(s->subnet, inet_ntop(AF_INET, &mask, buf, sizeof(buf)));
#endif
    }

    for (h = hosts; h != NULL; h = h->next)
    {
        if (h->ip_addr)
        {
            char *p;

            TE_SPRINTF(buf, "pntadm -f %s -A %s %s",
                       h->flags, h->ip_addr, h->ip_addr);
            if ((p = strrchr(buf, '.')) != NULL)
            {
                p[1] = '0';
                p[2] = '\0';
            }
            if ((rc = ta_system(buf)) != 0)
                return rc;
        }
    }
#endif

    return 0;
}

#if defined __linux__
static te_bool
check_dhcpd_pid(const char *pid_filename)
{
    int     rc = 0;
    char   *name;

    sprintf(buf, "cat %s 2>/dev/null 1>/dev/null", pid_filename);

    if ((rc = ta_system(buf)) < 0 ||
        !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
    {
        return FALSE;
    }

    if ((name = strrchr(dhcp_server_exec, '/')) == NULL)
        name = (char *)dhcp_server_exec;
    else
        name++;

    sprintf(buf,
            PS_ALL_PID_ARGS " | grep $(cat %s) | "
            "grep -q %s >/dev/null 2>&1", pid_filename, name);

    return !((rc = ta_system(buf)) < 0 ||
             !WIFEXITED(rc) || WEXITSTATUS(rc) != 0);
}

#define KILL_DHCPD(__dhcpd_pid_file) \
    do {                                                                \
        TE_SPRINTF(buf, "kill $(cat " __dhcpd_pid_file ")");            \
                                                                        \
        if ((rc = ta_system(buf)) < 0 ||                                \
            !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)                     \
        {                                                               \
            ERROR("Command '%s' failed, rc=%r", buf, rc);               \
            return TE_RC(TE_TA_UNIX, TE_ESHCMD);                        \
        }                                                               \
    } while (0)
#endif

/** Is DHCP server daemon running */
static te_bool
ds_dhcpserver_is_run(void)
{
    int         rc = 0;
#if defined __linux__
    if (check_dhcpd_pid(TE_DHCPD_PID_FILENAME))
        return TRUE;

    return check_dhcpd_pid(TE_DHCPD6_PID_FILENAME);
#elif defined __sun__
    TE_SPRINTF(buf,
               "[ \"`/usr/bin/svcs -H -o STATE dhcp-server`\" "
               "= \"online\" ]");

    rc = ta_system(buf);

    return !(rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0);
#endif
    return FALSE;
}

/** Stop DHCP server using script from /etc/init.d */
static te_errno
ds_dhcpserver_script_stop(void)
{
    RING("%s() started", __FUNCTION__);
    TE_SPRINTF(buf, "%s stop", dhcp_server_script);
    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/** Stop DHCP server */
static te_errno
ds_dhcpserver_stop(void)
{
    int         rc = 0;

    ENTRY("%s()", __FUNCTION__);

#if defined __linux__
    if (check_dhcpd_pid(TE_DHCPD_PID_FILENAME))
    {
        KILL_DHCPD(TE_DHCPD_PID_FILENAME);
    }

    if (check_dhcpd_pid(TE_DHCPD6_PID_FILENAME))
    {
        KILL_DHCPD(TE_DHCPD6_PID_FILENAME);
    }
#elif defined __sun__
    TE_SPRINTF(buf, "/usr/sbin/svcadm disable -st %s",
               get_ds_name("dhcpserver"));

    rc = ta_system(buf);
    if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
    {
        ERROR("Command '%s' failed, rc=%r", buf, rc);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }
#endif

    return 0;
}

/** Start DHCP server using script from /etc/init.d */
static te_errno
ds_dhcpserver_script_start(void)
{
    INFO("%s()", __FUNCTION__);
    TE_SPRINTF(buf, "%s start", dhcp_server_script);
    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/** Start DHCP server */
static te_errno
ds_dhcpserver_start(void)
{
    te_errno rc;

    ENTRY("%s()", __FUNCTION__);
    INFO("%s()", __FUNCTION__);

    rc = ds_dhcpserver_save_conf();
    if (rc != 0)
    {
        ERROR("Failed to save DHCP server configuration file");
        return rc;
    }

#if defined __linux__
    TE_SPRINTF(buf,
               (ipv6_subnets) ?
                    "%s -6 -t -cf %s" :
                            "%s -t -cf %s",
               dhcp_server_exec, dhcp_server_conf);
    if (ta_system(buf) != 0)
    {
        ERROR("Configuration file verification failed, command '%s'", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    TE_SPRINTF(buf,
               (ipv6_subnets) ?
                    "%s -6 -T -cf %s -lf %s" :
                            "%s -T -cf %s -lf %s",
               dhcp_server_exec, dhcp_server_conf,
               (ipv6_subnets) ? dhcp6_server_leases : dhcp_server_leases);
    if (ta_system(buf) != 0)
    {
        ERROR("Leases database verification failed, command '%s'", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    TE_SPRINTF(buf,
               (ipv6_subnets) ?
                    "%s -6 -cf %s -lf %s %s" :
                            "%s -cf %s -lf %s %s",
               dhcp_server_exec, dhcp_server_conf,
               (ipv6_subnets) ? dhcp6_server_leases : dhcp_server_leases,
               dhcp_server_ifs ? : "");
#elif defined __sun__
    TE_SPRINTF(buf, "/usr/sbin/svcadm disable -st %s",
               get_ds_name("dhcpserver"));
    if (ta_system(buf) != 0)
    {
        ERROR("Failed to stop DHCP server, command '%s'", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    TE_SPRINTF(buf, "/usr/sbin/svcadm enable -rst %s",
               get_ds_name("dhcpserver"));
#else
    ERROR("DHCP server configuration is not supported");

    return TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP);
#endif

    if (ta_system(buf) != 0)
    {
        ERROR("Failed to start DHCP server, command '%s'", buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/*** Configuration tree methods ***/
/*** Node /agent/dhcpserver methods ***/
/** Get DHCP server daemon state on/off */
static te_errno
ds_dhcpserver_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    INFO("%s()", __FUNCTION__);

    DHCP_SERVER_INIT_CHECK;

    strcpy(value, ds_dhcpserver_is_run() ? "1" : "0");

    return 0;
}

/**
 * Set desired status of DHCP server
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         desired dhcp server status
 *
 * @return status code
 */
static te_errno
ds_dhcpserver_set(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s(): value=%s", __FUNCTION__, value);

    INFO("%s()", __FUNCTION__);

    DHCP_SERVER_INIT_CHECK;

    dhcp_server_started = (strcmp(value, "1") == 0);
    if (dhcp_server_started != ds_dhcpserver_is_run())
    {
        dhcp_server_changed = TRUE;
    }

    return 0;
}

/** On/off DHCP server */
static te_errno
ds_dhcpserver_commit(unsigned int gid, const char *oid)
{
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    ENTRY("%s()", __FUNCTION__);
    INFO("%s()", __FUNCTION__);

    DHCP_SERVER_INIT_CHECK;

    /*
     * We don't need to change state of DHCP Server:
     * The current state is the same as desired.
     */
    if (!dhcp_server_changed)
        return 0;

    /* Stop DHCP server */
    if (ds_dhcpserver_is_run())
    {
#ifdef TA_UNIX_ISC_DHCPS_NATIVE_CFG
        rc = ds_dhcpserver_script_stop();
#else
        rc = ds_dhcpserver_stop();
#endif
        if (rc != 0)
        {
            if (ds_dhcpserver_is_run())
            {
                ERROR("Failed to stop DHCP server");
                return rc;
            }
        }
    }

    /* (Re)start DHCP server, if required */
    if (dhcp_server_started)
    {
#ifdef TA_UNIX_ISC_DHCPS_NATIVE_CFG
        rc = ds_dhcpserver_script_start();
#else
        rc = ds_dhcpserver_start();
#endif
        if (rc != 0)
        {
            ERROR("Failed to start DHCP server");
            return rc;
        }
    }

    dhcp_server_changed = FALSE;

    return rc;
}

/*** Node /agent/dhcpserver/interfaces methods ***/
/** Get DHCP server interfaces */
static te_errno
ds_dhcpserver_ifs_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    INFO("%s()", __FUNCTION__);

    DHCP_SERVER_INIT_CHECK;

    strcpy(value, dhcp_server_ifs ? : "");

    return 0;
}

/** Get DHCP server interfaces */
static te_errno
ds_dhcpserver_ifs_set(unsigned int gid, const char *oid, const char *value)
{
    char *copy;

    UNUSED(gid);
    UNUSED(oid);

    /* TODO Check value */
    INFO("%s()", __FUNCTION__);

    DHCP_SERVER_INIT_CHECK;

    copy = strdup(value);
    if (copy == NULL)
    {
        ERROR("%s(): strdup(%s) failed", __FUNCTION__, value);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
    free(dhcp_server_ifs);
    dhcp_server_ifs = copy;

    dhcp_server_changed = TRUE;

    return 0;
}

/*** Node /agent/dhcpserver/subnet methods ***/
static te_errno
ds_subnet_get(unsigned int gid, const char *oid, char *value,
              const char *dhcpserver, const char *subnet)
{
    te_dhcp_server_subnet  *s;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((s = find_subnet(subnet)) == NULL)
        return TE_OS_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(value, "%d", s->prefix_len);

    return 0;
}

static te_errno
ds_subnet_set(unsigned int gid, const char *oid, const char *value,
              const char *dhcpserver, const char *subnet)
{
    te_dhcp_server_subnet  *s;
    int                     prefix_len;
    char                   *end;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((s = find_subnet(subnet)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    prefix_len = strtol(value, &end, 10);
    if (value == end || *end != '\0')
        return TE_RC(TE_TA_UNIX, TE_EFMT);

    s->prefix_len = prefix_len;

    dhcp_server_changed = TRUE;

    return 0;
}

static te_errno
ds_subnet_add(unsigned int gid, const char *oid, const char *value,
              const char *dhcpserver, const char *subnet)
{
    te_dhcp_server_subnet  *s;
    int                     prefix_len;
    char                   *end;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((s = find_subnet(subnet)) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    prefix_len = strtol(value, &end, 10);
    if (value == end || *end != '\0')
        return TE_RC(TE_TA_UNIX, TE_EFMT);

    if ((s = calloc(1, sizeof(*s))) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((s->subnet = strdup(subnet)) == NULL)
    {
        free(s);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    s->prefix_len = prefix_len;
    s->range = NULL;

    TAILQ_INSERT_TAIL(&subnets, s, links);

    dhcp_server_changed = TRUE;

    return 0;
}

static te_errno
ds_subnet_del(unsigned int gid, const char *oid,
              const char *dhcpserver, const char *subnet)
{
    te_dhcp_server_subnet  *s;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((s = find_subnet(subnet)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TAILQ_REMOVE(&subnets, s, links);
    free_subnet(s);

    dhcp_server_changed = TRUE;

    return 0;
}

static te_errno
ds_subnet_list(unsigned int gid, const char *oid, char **list)
{
    te_dhcp_server_subnet  *s;

    UNUSED(gid);
    UNUSED(oid);

    DHCP_SERVER_INIT_CHECK;

    *buf = '\0';
    TAILQ_FOREACH(s, &subnets, links)
    {
        sprintf(buf + strlen(buf), "%s ",  s->subnet);
    }

    return (*list = strdup(buf)) == NULL ?
               TE_RC(TE_TA_UNIX, TE_ENOMEM) : 0;
}

/*** Node /agent/dhcpserver/space methods ***/
ADD_METHOD(space)
DEL_METHOD(space)
LIST_METHOD(space)

/*** Node /agent/dhcpserver/host methods ***/
ADD_METHOD(host)
DEL_METHOD(host)
LIST_METHOD(host)

/*** Node /agent/dhcpserver/group methods ***/
ADD_METHOD(group)
DEL_METHOD(group)
LIST_METHOD(group)

#ifdef TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED
/*** Node /agent/dhcpserver/client methods ***/
/*** FIXME Where are they ? ***/

/*** Node /agent/dhcpserver/lease methods ***/
#define ADDR_LIST_BULK      128
/** Get lease node */
static te_errno
ds_lease_get(unsigned int gid, const char *oid, char *value,
             const char *dhcpserver, const char *name)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    return open_lease(name);
}

/** Obtain the list of leases */
static te_errno
ds_lease_list(unsigned int gid, const char *oid, char **list)
{
    FILE *f;
    char *name;
    int   len = ADDR_LIST_BULK;

    UNUSED(gid);
    UNUSED(oid);

    DHCP_SERVER_INIT_CHECK;

    if ((f = fopen("/var/lib/dhcp/dhcpd.leases", "r")) == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if ((*list = (char *)malloc(ADDR_LIST_BULK)) == NULL)
    {
        fclose(f);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    **list = 0;

    while (fgets(buf, sizeof(buf), f) != NULL)
    {
        if (strncmp(buf, "lease", strlen("lease")) != 0)
            continue;

        name = buf + strlen("lease") + 1;
        *(strchr(name, ' ') + 1) = 0;
        if (strstr(*list, name) != NULL)
            continue;

        if (strlen(*list) + strlen(name) >= len)
        {
            char *tmp;

            len += ADDR_LIST_BULK;
            if ((tmp = (char *)realloc(*list, len)) == NULL)
            {
                free(*list);
                fclose(f);
                return TE_OS_RC(TE_TA_UNIX, TE_ENOMEM);
            }
            *list = tmp;
        }
        strcat(*list, name);
    }
    fclose(f);

    return 0;
}
#endif /* TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED */

/*** Node /agent/dhcpserver/subnet/range methods ***/
ATTR_GET(range, subnet, te_dhcp_server_subnet)
ATTR_SET(range, subnet, te_dhcp_server_subnet)
/*** Node /agent/dhcpserver/subnet/vendor_option_space methods ***/
ATTR_GET(vos, subnet, te_dhcp_server_subnet)
ATTR_SET(vos, subnet, te_dhcp_server_subnet)
/*** Node /agent/dhcpserver/subnet/option methods ***/
GET_OPT(subnet, te_dhcp_server_subnet)
SET_OPT(subnet, te_dhcp_server_subnet)
ADD_OPT(subnet, te_dhcp_server_subnet)
DEL_OPT(subnet, te_dhcp_server_subnet)
GET_OPT_LIST(subnet, te_dhcp_server_subnet, te_dhcp_option)

/*** Node /agent/dhcpserver/space/option methods ***/
static te_errno
ds_sp_opt_add(unsigned int gid, const char *oid,
              const char *value, const char *dhcpserver,
              const char *name, const char *optname)
{
    space *sp;
    te_dhcp_space_opt *opt;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((sp = find_space(name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (find_space_option(sp->options, optname) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if ((opt = (te_dhcp_space_opt *)calloc(sizeof(*opt), 1))
        == NULL || (opt->name = strdup(optname)) == NULL)
    {
        free(opt);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    opt->next = sp->options;
    sp->options = opt;
    opt->type = NULL;

    dhcp_server_changed = TRUE;

    return 0;
}


static te_errno
ds_sp_opt_del(unsigned int gid, const char *oid,
              const char *dhcpserver, const char *name, const char *optname)
{
    space *sp;

    te_dhcp_space_opt *opt, *prev;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((sp = find_space(name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    for (opt = sp->options, prev = NULL;
         opt != NULL && strcmp(opt->name, optname) != 0;
         prev = opt, opt = opt->next);

    if (opt == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (prev)
        prev->next = opt->next;
    else
        sp->options = opt->next;

    free(opt->name);
    free(opt->type);
    free(opt);

    dhcp_server_changed = TRUE;

    return 0;
}

GET_OPT_LIST(space, space, te_dhcp_space_opt)

/*** Node /agent/dhcpserver/host/group methods ***/
/** Obtain the group of the host */
static te_errno
ds_host_group_get(unsigned int gid, const char *oid, char *value,
                  const char *dhcpserver, const char *name)
{
    host *h;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((h = find_host(name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (h->group == NULL)
        *value = 0;
    else
        strcpy(value, h->group->name);

    return 0;
}

/** Change the group of the host */
static te_errno
ds_host_group_set(unsigned int gid, const char *oid, const char *value,
                  const char *dhcpserver, const char *name)
{
    host  *h;
    group *old;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((h = find_host(name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    old = h->group;
    if (*value == 0)
        h->group = NULL;
    else if ((h->group = find_group(value)) == NULL)
    {
        h->group = old;
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    dhcp_server_changed = TRUE;

    return 0;
}
/*** Node /agent/dhcpserver/host/chaddr methods ***/
ATTR_GET(chaddr, host, host)
ATTR_SET(chaddr, host, host)
/*** Node /agent/dhcpserver/host/client_id methods ***/
ATTR_GET(client_id, host, host)
ATTR_SET(client_id, host, host)
/*** Node /agent/dhcpserver/host/ip_addr methods ***/
ATTR_GET(ip_addr, host, host)
ATTR_SET(ip_addr, host, host)
/*** Node /agent/dhcpserver/host/next_server methods ***/
ATTR_GET(next_server, host, host)
ATTR_SET(next_server, host, host)
/*** Node /agent/dhcpserver/host/filename methods ***/
ATTR_GET(filename, host, host)
ATTR_SET(filename, host, host)
/*** Node /agent/dhcpserver/host/flags methods ***/
ATTR_GET(flags, host, host)
ATTR_SET(flags, host, host)
/*** Node /agent/dhcpserver/host/host_id methods ***/
ATTR_GET(host_id, host, host)
ATTR_SET(host_id, host, host)
/*** Node /agent/dhcpserver/host/prefix6 methods ***/
ATTR_GET(prefix6, host, host)
ATTR_SET(prefix6, host, host)
/*** Node /agent/dhcpserver/host/option methods ***/
GET_OPT(host, host)
SET_OPT(host, host)
ADD_OPT(host, host)
DEL_OPT(host, host)
GET_OPT_LIST(host, host, te_dhcp_option)

/*** Node /agent/dhcpserver/group/next methods ***/
ATTR_GET(next_server, group, group)
ATTR_SET(next_server, group, group)
/*** Node /agent/dhcpserver/group/file methods ***/
ATTR_GET(filename, group, group)
ATTR_SET(filename, group, group)
/*** Node /agent/dhcpserver/group/option methods ***/
GET_OPT(group, group)
SET_OPT(group, group)
ADD_OPT(group, group)
DEL_OPT(group, group)
GET_OPT_LIST(group, group, te_dhcp_option)

#ifdef TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED
/*** Node /agent/dhcpserver/client/lease methods ***/
/*** FIXME Where are they? ***/

/*** Node /agent/dhcpserver/lease/state methods ***/
/** Get lease state */
static te_errno
ds_lease_state_get(unsigned int gid, const char *oid, char *value,
                   const char *dhcpserver, const char *name)
GET_INT_LEASE_ATTR("state")

/*** Node /agent/dhcpserver/lease/client_id methods ***/
/** Get lease client identifier */
static te_errno
ds_lease_client_id_get(unsigned int gid, const char *oid, char *value,
                       const char *dhcpserver, const char *name)
{
    int rc;

    dhcpctl_data_string val;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((rc = open_lease(name)) != 0)
        return rc;

    /*
     * Very bad hack to know the type of the particular
     * dhcp-client-identifier: srting or binary.
     */
    {
        omapi_value_t *tv = (omapi_value_t *)0;

        CHECKSTATUS(omapi_get_value_str(lo, (omapi_object_t *)0,
                    "dhcp-client-identifier", &tv));

        if (tv->value->type == omapi_datatype_string)
        {
            sprintf(value, "%.*s", tv->value->u.buffer.value,
                    tv->value->u.buffer.len);
        }
        else
        {
            int i;

            sprintf(value, "\"");
            for (i = 0; i < tv->value->u.buffer.len; i++)
                sprintf(value + strlen(value), "%02x%s",
                        ((unsigned char *)(tv->value->u.buffer.value))[i],
                        i == tv->value->u.buffer.len - 1 ? "\"" : ":");
        }

        omapi_value_dereference (&tv, MDL);
    }

    return 0;
}

/*** Node /agent/dhcpserver/lease/hostname methods ***/
/** Get lease hostname */
static te_errno
ds_lease_hostname_get(unsigned int gid, const char *oid, char *value,
                      const char *dhcpserver, const char *name)
{
    int rc;

    dhcpctl_data_string val;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((rc = open_lease(name)) != 0)
        return rc;

    CHECKSTATUS(dhcpctl_get_value(&val, lo, "client-hostname"));

    memcpy(value, val->value, val->len);
    *(value + val->len) = 0;

    dhcpctl_data_string_dereference(&val, MDL);

    return 0;
}

/*** Node /agent/dhcpserver/lease/host methods ***/
/** Get lease IP address */
static te_errno
ds_lease_host_get(unsigned int gid, const char *oid, char *value,
                  const char *dhcpserver, const char *name)
{
    int rc;

    dhcpctl_data_string val;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((rc = open_lease(name)) != 0)
        return rc;

    CHECKSTATUS(dhcpctl_get_value(&val, lo, "host"));

    /* It's not clear what to do next :) */

    return 0;
}

/*** Node /agent/dhcpserver/lease/chaddr methods ***/
/** Get lease MAC address */
static te_errno
ds_lease_chaddr_get(unsigned int gid, const char *oid, char *value,
                    const char *dhcpserver, const char *name)
{
    int rc;

    dhcpctl_data_string val;
    unsigned char      *mac;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((rc = open_lease(name)) != 0)
        return rc;

    CHECKSTATUS(dhcpctl_get_value(&val, lo, "hardware-address"));

    mac = (unsigned char *)(val->value);
    sprintf(value, "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    dhcpctl_data_string_dereference(&val, MDL);

    return 0;
}

/*** Node /agent/dhcpserver/lease/ends methods ***/
/* Get lease attributes */
static te_errno
ds_lease_ends_get(unsigned int gid, const char *oid, char *value,
                  char *dhcpserver, char *name)
GET_INT_LEASE_ATTR("ends")

/*** Node /agent/dhcpserver/lease/tstp methods ***/
static te_errno
ds_lease_tstp_get(unsigned int gid, const char *oid, char *value,
                  char *dhcpserver, char *name)
GET_INT_LEASE_ATTR("tstp")

/*** Node /agent/dhcpserver/lease/cltt methods ***/
static te_errno
ds_lease_cltt_get(unsigned int gid, const char *oid, char *value,
                  char *dhcpserver, char *name)
GET_INT_LEASE_ATTR("cltt")
#endif /* TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED */

/***
 *** Node /agent/dhcpserver/space/option/code methods
 *** Node /agent/dhcpserver/space/option/type methods (the same)
 ***/
static te_errno
ds_sp_opt_get(unsigned int gid, const char *oid,
              char *value, const char *dhcpserver,
              const char *name, const char *optname)
{
    space *sp;

    te_dhcp_space_opt *opt;
    const char *var_name = get_last_label(oid);

    UNUSED(gid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((sp = find_space(name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((opt = find_space_option(sp->options, optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);


    if (strcmp(var_name, "type:") == 0)
    {
        if (NULL == opt->type)
            value[0] = '\0';
        else
            strcpy(value, opt->type);
    }
    else if (strcmp(var_name, "code:") == 0)
        sprintf(value, "%d", opt->code);
    else
    {
        WARN("dhcp_server, get space option var, wrong var_name '%s'",
             var_name);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    return 0;
}

static te_errno
ds_sp_opt_set(unsigned int gid, const char *oid,
              char *value, const char *dhcpserver,
              const char *name, const char *optname)
{
    space *sp;

    te_dhcp_space_opt *opt;

    const char *var_name = get_last_label(oid);

    UNUSED(gid);
    UNUSED(dhcpserver);

    DHCP_SERVER_INIT_CHECK;

    if ((sp = find_space(name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((opt = find_space_option(sp->options, optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (strcmp(var_name, "type:") == 0)
    {
        if (NULL != opt->type)
            free(opt->type);
        opt->type = strdup(value);
    }
    else if (strcmp(var_name, "code:") == 0)
        sscanf(value, "%d", &(opt->code));
    else
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return 0;
}

/*** Cunfigurator subtree /agent/dhcpserver layout ***/
/*
 * Relations: left - son, down - brother.
 *
 * agent - dhcpserver - interfaces
 *                          |
 *                      subnet - range
 *                          |       |
 *                          |    vendor_option_space
 *                          |       |
 *                          |    option
 *                          |
 *                      space  - option - code
 *                          |               |
 *                          |             type
 *                          |
 *                      host   - group
 *                          |       |
 *                          |    chaddr
 *                          |       |
 *                          |    client-id
 *                          |       |
 *                          |    ip-address
 *                          |       |
 *                          |    next
 *                          |       |
 *                          |    file
 *                          |       |
 *                          |    flags
 *                          |       |
 *                          |    host-id
 *                          |       |
 *                          |    prefix6
 *                          |       |
 *                          |    option
 *                          |
 *                      group  - next
 *                          |       |
 *                          |    file
 *                          |       |
 *                          |    option
 *                          |
 * ----------------------------------------------------------
 * List of /agent/dhcpserver children goes further when
 * TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED is defined
 * ----------------------------------------------------------
 *                          |
 *                       client - lease
 *                          |
 *                       lease  - state
 *                                   |
 *                                client_id
 *                                   |
 *                                hostname
 *                                   |
 *                                host
 *                                   |
 *                                chaddr
 *                                   |
 *                                ends
 *                                   |
 *                                tstp
 *                                   |
 *                                cltt
 *
 */
/*** Node /agent/dhcpserver/space/option children ***/
RCF_PCH_CFG_NODE_RW(node_ds_sp_opt_type, "type",
                    NULL, NULL,
                    ds_sp_opt_get, ds_sp_opt_set);

RCF_PCH_CFG_NODE_RW(node_ds_sp_opt_code, "code",
                    NULL, &node_ds_sp_opt_type,
                    ds_sp_opt_get, ds_sp_opt_set);

#ifdef TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED
/*** Node /agent/dhcpserver/lease children***/
RCF_PCH_CFG_NODE_RO(node_ds_lease_cltt, "cltt",
                    NULL, NULL,
                    ds_lease_cltt_get);

RCF_PCH_CFG_NODE_RO(node_ds_lease_tstp, "tstp",
                    NULL, &node_ds_lease_cltt,
                    ds_lease_tstp_get);

RCF_PCH_CFG_NODE_RO(node_ds_lease_ends, "ends",
                    NULL, &node_ds_lease_tstp,
                    ds_lease_ends_get);

RCF_PCH_CFG_NODE_RO(node_ds_lease_chaddr, "chaddr",
                    NULL, &node_ds_lease_ends,
                    ds_lease_chaddr_get);

RCF_PCH_CFG_NODE_RO(node_ds_lease_host, "host",
                    NULL, &node_ds_lease_chaddr,
                    ds_lease_host_get);

RCF_PCH_CFG_NODE_RO(node_ds_lease_hostname, "hostname",
                    NULL, &node_ds_lease_host,
                    ds_lease_hostname_get);

RCF_PCH_CFG_NODE_RO(node_ds_lease_client_id, "client_id",
                    NULL, &node_ds_lease_hostname,
                    ds_lease_client_id_get);

RCF_PCH_CFG_NODE_RO(node_ds_lease_state, "state",
                    NULL, &node_ds_lease_client_id,
                    ds_lease_state_get);

/*** Node /agent/dhcpserver/client children ***/
static rcf_pch_cfg_object node_ds_client_lease =
    { "lease", 0, NULL, NULL,
      (rcf_ch_cfg_get)ds_client_lease_get, NULL, NULL, NULL,
      (rcf_ch_cfg_list)ds_client_lease_list, NULL, NULL };
#endif /* TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED */

/*** Node /agent/dhcpserver/group children ***/
static rcf_pch_cfg_object node_ds_group_option =
    { "option", 0, NULL, NULL,
      (rcf_ch_cfg_get)ds_group_option_get,
      (rcf_ch_cfg_set)ds_group_option_set,
      (rcf_ch_cfg_add)ds_group_option_add,
      (rcf_ch_cfg_del)ds_group_option_del,
      (rcf_ch_cfg_list)ds_group_option_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_ds_group_file, "file",
                    NULL, &node_ds_group_option,
                    ds_group_filename_get, ds_group_filename_set);

RCF_PCH_CFG_NODE_RW(node_ds_group_next, "next",
                    NULL, &node_ds_group_file,
                    ds_group_next_server_get, ds_group_next_server_set);

/*** Node /agent/dhcpserver/host children ***/
static rcf_pch_cfg_object node_ds_host_option =
    { "option", 0, NULL, NULL,
      (rcf_ch_cfg_get)ds_host_option_get,
      (rcf_ch_cfg_set)ds_host_option_set,
      (rcf_ch_cfg_add)ds_host_option_add,
      (rcf_ch_cfg_del)ds_host_option_del,
      (rcf_ch_cfg_list)ds_host_option_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_ds_host_prefix6, "prefix6",
                    NULL, &node_ds_host_option,
                    ds_host_prefix6_get, ds_host_prefix6_set);

RCF_PCH_CFG_NODE_RW(node_ds_host_host_id, "host-id",
                    NULL, &node_ds_host_prefix6,
                    ds_host_host_id_get, ds_host_host_id_set);

RCF_PCH_CFG_NODE_RW(node_ds_host_flags, "flags",
                    NULL, &node_ds_host_host_id,
                    ds_host_flags_get, ds_host_flags_set);

RCF_PCH_CFG_NODE_RW(node_ds_host_file, "file",
                    NULL, &node_ds_host_flags,
                    ds_host_filename_get, ds_host_filename_set);

RCF_PCH_CFG_NODE_RW(node_ds_host_next, "next",
                    NULL, &node_ds_host_file,
                    ds_host_next_server_get, ds_host_next_server_set);

RCF_PCH_CFG_NODE_RW(node_ds_host_ip_addr, "ip-address",
                    NULL, &node_ds_host_next,
                    ds_host_ip_addr_get, ds_host_ip_addr_set);

RCF_PCH_CFG_NODE_RW(node_ds_host_client_id, "client-id",
                    NULL, &node_ds_host_ip_addr,
                    ds_host_client_id_get, ds_host_client_id_set);

RCF_PCH_CFG_NODE_RW(node_ds_host_chaddr, "chaddr",
                    NULL, &node_ds_host_client_id,
                    ds_host_chaddr_get, ds_host_chaddr_set);

RCF_PCH_CFG_NODE_RW(node_ds_host_group, "group",
                    NULL, &node_ds_host_chaddr,
                    ds_host_group_get, ds_host_group_set);

/*** Node /agent/dhcpserver/space children ***/
RCF_PCH_CFG_NODE_COLLECTION(node_ds_space_options, "option",
                            &node_ds_sp_opt_code, NULL,
                            ds_sp_opt_add, ds_sp_opt_del,
                            ds_space_option_list, NULL);

/*** Node /agent/dhcpserver/subnet children ***/
static rcf_pch_cfg_object node_ds_subnet_option =
    { "option", 0, NULL, NULL,
      (rcf_ch_cfg_get)ds_subnet_option_get,
      (rcf_ch_cfg_set)ds_subnet_option_set,
      (rcf_ch_cfg_add)ds_subnet_option_add,
      (rcf_ch_cfg_del)ds_subnet_option_del,
      (rcf_ch_cfg_list)ds_subnet_option_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_ds_subnet_vendor_sp, "vendor_option_space",
                    NULL, &node_ds_subnet_option,
                    ds_subnet_vos_get, ds_subnet_vos_set);

RCF_PCH_CFG_NODE_RW(node_ds_subnet_range, "range",
                    NULL, &node_ds_subnet_vendor_sp,
                    ds_subnet_range_get, ds_subnet_range_set);

/*** Node /agent/dhcpserver children ***/
#ifdef TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED
/*** List of /agent/dhcpserver children is teminated here ***/
static rcf_pch_cfg_object node_ds_lease =
    { "lease", 0, &node_ds_lease_state, NULL,
      (rcf_ch_cfg_get)ds_lease_get, NULL, NULL, NULL,
      (rcf_ch_cfg_list)ds_lease_list, NULL, NULL};

static rcf_pch_cfg_object node_ds_client =
    { "client", 0, &node_ds_client_lease, &node_ds_lease,
      (rcf_ch_cfg_get)ds_client_get, NULL, NULL, NULL,
      (rcf_ch_cfg_list)ds_client_list, NULL, NULL};

RCF_PCH_CFG_NODE_COLLECTION(node_ds_group, "group",
                            &node_ds_group_next, &node_ds_client,
                            ds_group_add, ds_group_del,
                            ds_group_list, NULL);
#else /* TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED */
/*** List of /agent/dhcpserver children is teminated here ***/
RCF_PCH_CFG_NODE_COLLECTION(node_ds_group, "group",
                            &node_ds_group_next, NULL,
                            ds_group_add, ds_group_del,
                            ds_group_list, NULL);
#endif /* TA_UNIX_ISC_DHCPS_LEASES_SUPPORTED */

RCF_PCH_CFG_NODE_COLLECTION(node_ds_host, "host",
                            &node_ds_host_group, &node_ds_group,
                            ds_host_add, ds_host_del,
                            ds_host_list, NULL);

RCF_PCH_CFG_NODE_COLLECTION(node_ds_space, "space",
                            &node_ds_space_options, &node_ds_host,
                            ds_space_add, ds_space_del,
                            ds_space_list, NULL);

static rcf_pch_cfg_object node_ds_subnet =
    { "subnet", 0, &node_ds_subnet_range, &node_ds_space,
      (rcf_ch_cfg_get)ds_subnet_get,
      (rcf_ch_cfg_set)ds_subnet_set,
      (rcf_ch_cfg_add)ds_subnet_add,
      (rcf_ch_cfg_del)ds_subnet_del,
      (rcf_ch_cfg_list)ds_subnet_list, NULL, NULL };

RCF_PCH_CFG_NODE_RW(node_ds_dhcpserver_ifs, "interfaces",
                    NULL, &node_ds_subnet,
                    ds_dhcpserver_ifs_get, ds_dhcpserver_ifs_set);

/*** Configuration subtree root /agent/dhcpserver ***/
static rcf_pch_cfg_object node_ds_dhcpserver =
    { "dhcpserver", 0,
      &node_ds_dhcpserver_ifs, NULL,
      (rcf_ch_cfg_get)ds_dhcpserver_get,
      (rcf_ch_cfg_set)ds_dhcpserver_set,
      NULL, NULL, NULL,
      (rcf_ch_cfg_commit)ds_dhcpserver_commit, NULL };

/*** Resource /agent/dhcpserver grab and release functions ***/
te_errno
dhcpserver_grab(const char *name)
{
    int rc = 0;

    UNUSED(name);

    DHCP_SERVER_INIT_CHECK;

    /* Stop DHCP server */
    if (ds_dhcpserver_is_run())
    {
        WARN("Another DHCP server is running, shutting it down...");
#ifdef TA_UNIX_ISC_DHCPS_NATIVE_CFG
        rc = ds_dhcpserver_script_stop();
#else
        rc = ds_dhcpserver_stop();
#endif
        if (rc != 0)
        {
            /*
             * in case we failed to stop dhcp server and it's still
             * running
             */
            if (ds_dhcpserver_is_run())
            {
                ERROR("Failed to stop DHCP server");
                return rc;
            }
        }
    }

    dhcp_server_started = FALSE;
    dhcp_server_changed = FALSE;

    if ((rc = rcf_pch_add_node("/agent", &node_ds_dhcpserver)) != 0)
        return rc;

    return 0;
}

te_errno
dhcpserver_release(const char *name)
{
    te_errno    rc;
    host       *host, *host_tmp;
    group      *group, *group_tmp;

    UNUSED(name);

    if (!check_dhcpserver_init(FALSE))
    {
        /* DHCP server was not initialised. Nothing to do. */
        return 0;
    }

    rc = rcf_pch_del_node(&node_ds_dhcpserver);
    if (rc != 0)
        return rc;

    /* Free old lists */
    for (host = hosts; host != NULL; host = host_tmp)
    {
        host_tmp = host->next;
        free_host(host);
    }
    hosts = NULL;

    for (group = groups; group != NULL; group = group_tmp)
    {
        group_tmp = group->next;
        free_group(group);
    }
    groups = NULL;

#if defined __sun__
    ds_restore_backup(dhcp_server_conf_backup);

    /* FIXME */
    if ((rc = ta_system("rm -fr /var/mydhcp")) != 0)
        return rc;
#endif

#ifndef TA_UNIX_ISC_DHCPS_NATIVE_CFG
    if (dhcp_server_was_run)
    {
        if (ds_dhcpserver_is_run())
        {
            WARN("DHCP server was disabled at start up from TE point "
                 "of view, however it is enabled at shutdown. It looks "
                 "like you have configuration rollback issues.");
            (void)ds_dhcpserver_stop();
        }
        if (ds_dhcpserver_script_start() != 0)
        {
            ERROR("Failed to start DHCP server on rollback"
                 " - DHCP server will not be available");
        }
        dhcp_server_was_run = FALSE;
    }

    if (dhcp_server_conf != NULL && unlink(dhcp_server_conf) != 0 &&
        errno != ENOENT)
    {
        ERROR("Failed to delete DHCP server temporary configuration "
              "file '%s': %s", dhcp_server_conf, strerror(errno));
    }

#if defined __linux__
    if (dhcp_server_leases != NULL && unlink(dhcp_server_leases) != 0)
    {
        ERROR("Failed to delete DHCP server temporary leases data base "
              "file '%s': %s", dhcp_server_leases, strerror(errno));
    }
#endif
#endif

    return 0;
}

#endif /* WITH_DHCP_SERVER */
