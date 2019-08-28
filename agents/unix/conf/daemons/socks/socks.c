/** @file
 * @brief Unix Test Agent
 *
 * Socks configurator tree
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Svetlana Fishchuk <Svetlana.Fishchuk@oktetlabs.ru>
 */

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "conf_daemons.h"
#include "te_string.h"
#include "te_sockaddr.h"
#include "rcf_pch.h"
#include "te_str.h"
#include "te_queue.h"
#include "te_rpc_sys_socket.h"

#include <limits.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>
#include <libgen.h>

/** Default SOCKS port */
#define SOCKS_DEFAULT_PORT          (1080)

/** Path to srelay */
#define SOCKS_PATH                  "/usr/sbin/srelay"
/** Format string of path to PID file */
#define SOCKS_PID_PATH_FMT          "/tmp/socks_%s.pid"
/** Format string of path to configuration file on TA */
#define SOCKS_CONFIG_PATH_FMT       "/tmp/socks_%s.cfg"
/** Format string of path to user credentials configuration file on TA */
#define SOCKS_USER_PASS_PATH_FMT    "/tmp/socks_%s_users.cfg"

/** Socks implementation for srelay */
#define SOCKS_IMPLEMENTATION_SRELAY "srelay"

/** Size of arbitrary fixed buffers */
#define AUX_BUF_LEN (512)

/* Check Socks instance for validity */
#define SOCKS_CHECK(instance)                           \
    do {                                                \
        if (instance == NULL)                           \
            return TE_OS_RC(TE_TA_UNIX, TE_ENOENT);     \
    } while (0)

/* Macro used to simplify error propagation in config writing functions */
#define FPRINTF(x...) \
    do {                                        \
        if (fprintf(x) < 0)                     \
        {                                       \
            rc = TE_OS_RC(TE_TA_UNIX, errno);   \
            goto cleanup;                       \
        }                                       \
    } while (0)

/** Structure to define user details */
typedef struct te_socks_user {
    LIST_ENTRY(te_socks_user) list;     /**< Linked list of users */
    char                   *name;       /**< Friendly user name */
    char                   *next_hop;   /**< Next hop IP */
    char                   *username;   /**< Full username */
    char                   *password;   /**< Full password */
} te_socks_user;

/** Socks server protocol structure */
typedef struct te_socks_proto {
    LIST_ENTRY(te_socks_proto)  list;   /**< Linked list of protocols */
    char   *name;                       /**< User-friendly name */
    int     proto;                      /**< Protocol to support, e.g.
                                             RPC_IPPROTO_TCP */
} te_socks_proto;

/** Socks server interface structure */
typedef struct te_socks_interface {
    LIST_ENTRY(te_socks_interface)  list;   /**< Linked list of interfaces */
    char       *name;           /**< User-friendly name */
    char       *interface;      /**< Interface to bind to */
    int         addr_family;    /**< Address family to bind to */
    uint16_t    port;           /**< Port to listen at */
} te_socks_interface;

/** Socks server protocol structure */
typedef struct te_socks_cipher {
    LIST_ENTRY(te_socks_cipher) list;   /**< Linked list of cipher */
    char       *name;           /**< User-friendly name */
    char       *cipher;         /**< Cipher supported by implementation */
} te_socks_cipher;

/** Socks server configuration structure */
typedef struct te_socks_server {
    LIST_ENTRY(te_socks_server) list;  /**< Linked list of servers */
    char       *name;       /**< Instance name. */

    te_bool     status;     /**< Daemon status. Possible values:
                                 @c TRUE when running,
                                 @c FALSE otherwise */
    char       *impl;       /**< Used daemon implementation. Currently
                                 supported: "srelay" */
    LIST_HEAD(, te_socks_cipher)    ciphers;    /**< Cipher suites used for
                                                     encryption.
                                                     Not supported by most of
                                                     implementations. */
    LIST_HEAD(, te_socks_proto)     protocols;  /**< Used protocols (not all
                                                     implementations support
                                                     anything rather than
                                                     RPC_IPPROTO_TCP) */
    LIST_HEAD(, te_socks_interface) interfaces; /**< Interfaces to listen at */
    char       *outbound_interface;             /**< Interface to send packets
                                                     to */
    LIST_HEAD(, te_socks_user) users;           /**< Head of user list */

    /* Instance-specific paths to associated files */
    char   *pid_path;               /**< Path to PID file */
    char   *config_path;            /**< Path to configuration file on TA */
    char   *user_pass_path;         /**< Path to file containing user
                                         credentials */
} te_socks_server;

/* Forward declarations */
static rcf_pch_cfg_object node_socks;
static te_errno socks_server_start(te_socks_server *instance);
static te_errno socks_server_stop(te_socks_server *instance);
static te_errno socks_getifaddrs(const char *ifname,
                                 int family,
                                 char *ip,
                                 int ip_size);
static void socks_server_free(te_socks_server *instance);
static void te_socks_proto_free(te_socks_proto *u);
static void te_socks_interface_free(te_socks_interface *u);
static void te_socks_cipher_free(te_socks_cipher *c);
static void te_socks_user_free(te_socks_user *u);

/** Server linked list */
static LIST_HEAD(te_socks_servers, te_socks_server) servers;

/**
 * Put 'b' string to 'a' variable, duplicating 'b' content. Preserve old content
 * on failure.
 *
 * @param a String to put result to.
 * @param b String to duplicate.
 *
 * @return Status code
 */
static inline te_errno
socks_override_str(char **a, const char *b)
{
    char *old = *a;

    if (strlen(b) == 0)
    {
        *a = NULL;
    }
    else if ((*a = strdup(b)) == NULL)
    {
        *a = old;
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    free(old);

    return 0;
}

/**
 * Create a new instance of SOCKS server with given name.
 *
 * @param name    Instance name.
 *
 * @return Created instance.
 */
te_socks_server *
socks_server_create(const char *name)
{
    te_socks_server *instance;

    instance = calloc(1, sizeof(te_socks_server));
    if (instance == NULL)
        return NULL;

    instance->name = strdup(name);
    if (instance->name == NULL)
        goto error;

    instance->status = FALSE;
    instance->impl = strdup(SOCKS_IMPLEMENTATION_SRELAY);
    if (instance->impl == NULL)
        goto error;

    LIST_INIT(&instance->ciphers);
    LIST_INIT(&instance->protocols);
    LIST_INIT(&instance->interfaces);

    instance->outbound_interface = strdup("");
    if (instance->outbound_interface == NULL)
        goto error;

    LIST_INIT(&instance->users);

    instance->pid_path = te_sprintf(SOCKS_PID_PATH_FMT, instance->name);
    if (instance->pid_path == NULL)
        goto error;

    instance->config_path = te_sprintf(SOCKS_CONFIG_PATH_FMT,
                                       instance->name);
    if (instance->config_path == NULL)
        goto error;

    instance->user_pass_path = te_sprintf(SOCKS_USER_PASS_PATH_FMT,
                                          instance->name);
    if (instance->user_pass_path == NULL)
        goto error;

    return instance;

error:
    socks_server_free(instance);
    return NULL;
}

/** Free all data associated with the server */
static void
socks_server_free(te_socks_server *instance)
{
    te_socks_user      *u_tmp, *u;
    te_socks_proto     *p_tmp, *p;
    te_socks_interface *iface_tmp, *iface;
    te_socks_cipher    *c_tmp, *c;

    free(instance->name);
    free(instance->impl);

    LIST_FOREACH_SAFE(c, &instance->ciphers, list, c_tmp)
    {
        LIST_REMOVE(c, list);
        te_socks_cipher_free(c);
    }

    LIST_FOREACH_SAFE(p, &instance->protocols, list, p_tmp)
    {
        LIST_REMOVE(p, list);
        te_socks_proto_free(p);
    }
    LIST_FOREACH_SAFE(iface, &instance->interfaces, list, iface_tmp)
    {
        LIST_REMOVE(iface, list);
        te_socks_interface_free(iface);
    }
    free(instance->outbound_interface);

    free(instance->pid_path);
    free(instance->config_path);
    free(instance->user_pass_path);

    LIST_FOREACH_SAFE(u, &instance->users, list, u_tmp)
    {
        LIST_REMOVE(u, list);
        te_socks_user_free(u);
    }

    free(instance);
}


/**
 * Add server to the list of servers.
 *
 * @param instance Server instance to add.
 *
 * @remark It is assumed that function always succeeds.
 */
static void
socks_add_server(te_socks_server *instance)
{
    LIST_INSERT_HEAD(&servers, instance, list);
}

/**
 * Get Socks server instance by name.
 *
 * @param name    Instance name.
 *
 * @return Instance pointer, or @c NULL if instance is not found.
 */
static te_socks_server *
socks_get_server(const char *name)
{
    te_socks_server    *server;

    LIST_FOREACH(server, &servers, list)
    {
        if (strcmp(server->name, name) == 0)
            break;
    }

    return server;
}

/**
 * Delete Socks server instance from list by name.
 *
 * @param name    Instance name.
 *
 * @return Pointer to deleted instance, or @c NULL if it is not found.
 */
static te_socks_server *
socks_del_server(const char *name)
{
    te_socks_server *server = socks_get_server(name);

    if (server == NULL)
        return NULL;

    LIST_REMOVE(server, list);

    return server;
}


/** Restart SOCKS in case of need */
static inline te_errno
socks_server_restart(te_socks_server *instance)
{
    te_errno rc;

    if (!instance->status)
        return 0;

    rc = socks_server_stop(instance);
    if (rc != 0)
    {
        ERROR("Failed to stop instance during restart: %r", rc);
        return rc;
    }

    rc = socks_server_start(instance);
    if (rc != 0)
    {
        ERROR("Failed to restart instance: %r", rc);
        return rc;
    }

    return 0;
}

/**
 * Find the user in the list
 *
 * @param instance  Server instance.
 * @param name      User's friendly name.
 *
 * @return Pointer to user structure, or @c NULL.
 */
static te_socks_user *
te_socks_user_find(te_socks_server *instance, const char *name)
{
    te_socks_user *u = NULL;

    LIST_FOREACH(u, &instance->users, list)
    {
        if (strcmp(u->name, name) == 0)
            break;
    }

    return u;
}

/** Release all memory allocated for user structure */
static void
te_socks_user_free(te_socks_user *u)
{
    free(u->name);
    free(u->next_hop);
    free(u->password);
    free(u->username);
    free(u);
}

/**
 * Find the protocol in the list
 *
 * @param instance  Server instance.
 * @param name      Protocol's friendly name.
 *
 * @return Pointer to te_socks_proto structure, or @c NULL.
 */
static te_socks_proto *
te_socks_proto_find(te_socks_server *instance, const char *name)
{
    te_socks_proto *p = NULL;

    LIST_FOREACH(p, &instance->protocols, list)
    {
        if (strcmp(p->name, name) == 0)
            break;
    }

    return p;
}

/** Release all memory allocated for proto structure */
static void
te_socks_proto_free(te_socks_proto *p)
{
    free(p->name);
    free(p);
}

/**
 * Find the interface by user-friendly name in the list
 *
 * @param instance  Server instance.
 * @param name      Interface's friendly name.
 *
 * @return Pointer to te_socks_interface structure, or @c NULL.
 */
static te_socks_interface *
te_socks_interface_find(te_socks_server *instance, const char *name)
{
    te_socks_interface *iface = NULL;

    LIST_FOREACH(iface, &instance->interfaces, list)
    {
        if (strcmp(iface->name, name) == 0)
            break;
    }

    return iface;
}


/** Release all memory allocated for interface structure */
static void
te_socks_interface_free(te_socks_interface *iface)
{
    free(iface->name);
    free(iface->interface);
    free(iface);
}

/**
 * Find the cipher in the list
 *
 * @param instance  Server instance.
 * @param name      Cipher's friendly name.
 *
 * @return Pointer to te_socks_cipher structure, or @c NULL.
 */
static te_socks_cipher *
te_socks_cipher_find(te_socks_server *instance, const char *name)
{
    te_socks_cipher *c = NULL;

    LIST_FOREACH(c, &instance->ciphers, list)
    {
        if (strcmp(c->name, name) == 0)
            break;
    }

    return c;
}

/** Release all memory allocated for proto structure */
static void
te_socks_cipher_free(te_socks_cipher *c)
{
    free(c->name);
    free(c->cipher);
    free(c);
}



/** Check if currently selected implementation is srelay */
static te_bool
socks_server_is_srelay(te_socks_server *instance)
{
    return instance != NULL &&
           instance->impl != NULL &&
           strcmp(instance->impl, SOCKS_IMPLEMENTATION_SRELAY) == 0;
}

/** Write users to file */
static te_errno
socks_server_write_users(te_socks_server *instance)
{
    te_socks_user     *u;
    FILE              *f;
    te_errno           rc = 0;

    if (!socks_server_is_srelay(instance))
    {
        ERROR("Not implemented for server '%s'",
              instance != NULL ? instance->impl : "(null instance)");
        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }

    f = fopen(instance->user_pass_path, "w");
    if (f == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);

    /* Check users */
    LIST_FOREACH(u, &instance->users, list)
    {
        if (u->next_hop == NULL || u->username == NULL || u->password == NULL)
        {
            ERROR("Not all data is propagated for user");
            rc = TE_RC(TE_TA_UNIX, TE_ENOENT);
            goto cleanup;
        }

        FPRINTF(f, "%s %s %s\n", u->next_hop, u->username, u->password);
    }

cleanup:
    if (fclose(f) == EOF && rc == 0)
        rc = TE_OS_RC(TE_TA_UNIX, errno);

    return rc;
}

/**
 * Start Socks daemon with specified configuration file.
 *
 * @param instance    Server instance.
 *
 * @return Status code.
 */
static te_errno
socks_server_start(te_socks_server *instance)
{
    te_errno    rc;
    te_string   cmd = TE_STRING_INIT;
    te_bool     ip_at_least_one = FALSE;
    te_socks_interface *iface;
    te_socks_proto     *proto;

    /* Filter out unsupported implementations */
    if (!socks_server_is_srelay(instance))
    {
        ERROR("Not implemented for server '%s'",
              instance != NULL ? instance->impl : "(null instance)");
        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }

    /* Filter out wrong settings */
    if (instance->outbound_interface == NULL)
    {
        ERROR("Not all parameters are filled for Socks server");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    rc = te_string_append(&cmd,
                          "%s -p %s",
                          SOCKS_PATH,
                          instance->pid_path);
    if (rc != 0)
        return rc;

    /* Filter out unsupported protocols */
    LIST_FOREACH(proto, &instance->protocols, list)
    {
        if (proto->proto != RPC_IPPROTO_TCP)
        {
            ERROR("SOCKS server '%s' doesn't support protocol '%d'",
                  instance->impl, proto->proto);
            rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
            goto cleanup;
        }
    }

    /* Collect interface IPs */
    LIST_FOREACH(iface, &instance->interfaces, list)
    {
        char    ip[AUX_BUF_LEN];

        rc = socks_getifaddrs(iface->interface,
                              addr_family_rpc2h(iface->addr_family),
                              ip,
                              sizeof(ip));
        if (rc != 0)
        {
            ERROR("Interface '%s' addr family '%d' requested but not found: %r",
                  iface->interface, iface->addr_family, rc);
            goto cleanup;
        }

        rc = te_string_append(&cmd,
                              " -i %s%s%s:%u",
                              iface->addr_family == RPC_AF_INET6 ? "[" : "",
                              ip,
                              iface->addr_family == RPC_AF_INET6 ? "]" : "",
                              (unsigned int)iface->port);
        if (rc != 0)
            goto cleanup;

        ip_at_least_one = TRUE;
    }

    /* There should be at least one IP */
    if (!ip_at_least_one)
    {
        ERROR("No IP found to bind to");
        rc = TE_RC(TE_TA_UNIX, TE_EINVAL);
        goto cleanup;
    }

    /* Write user data */
    rc = socks_server_write_users(instance);
    if (rc != 0)
        goto cleanup;

    /* Set user data location */
    if (!LIST_EMPTY(&instance->users))
    {
        rc = te_string_append(&cmd,
                              " -u %s",
                              SOCKS_PATH,
                              instance->user_pass_path);
        if (rc != 0)
            goto cleanup;
    }

    /* Set outbound interface */
    rc = te_string_append(&cmd,
                          " -J %s",
                          instance->outbound_interface);
    if (rc != 0)
        goto cleanup;

    rc = te_string_append(&cmd,
                          " &");
    if (rc != 0)
        goto cleanup;

    if (ta_system(cmd.ptr) != 0)
    {
        ERROR("Couldn't start Socks daemon");
        rc = TE_RC(TE_TA_UNIX, TE_ESHCMD);
        goto cleanup;
    }

    te_string_free(&cmd);
    instance->status = TRUE;

    return 0;

cleanup:
    unlink(instance->user_pass_path);
    unlink(instance->config_path);
    unlink(instance->pid_path);
    te_string_free(&cmd);
    instance->status = FALSE;
    return rc;
}

/*
 * Read process id from pid file
 *
 * @param pid_path Path to PID file
 *
 * @return Process ID
 */
static int
read_pid(const char *pid_path)
{
    int         pid = -1;
    FILE       *f;

    if (pid_path == NULL)
        return -1;

    f = fopen(pid_path, "r");
    if (f != NULL && (fscanf(f, "%d", &pid) != 1 || fclose(f) == EOF))
        return -1;

    return pid;
}

/**
 * Send signal to the socks process.
 *
 * @param instance  Server instance.
 * @param sig       Target signal.
 *
 * @return Status code.
 */
static te_errno
socks_server_send_signal(te_socks_server *instance,
                         int sig)
{
    int pid = read_pid(instance->pid_path);

    if (pid == -1)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (kill(pid, sig) != 0)
    {
        ERROR("Couldn't send signal %d to Socks daemon (pid %d)", sig, pid);
        return te_rc_os2te(errno);
    }

    return 0;
}

/**
 * Stop Socks daemon if it is running.
 *
 * @param instance  Server instance.
 *
 * @return Status code.
 */
static te_errno
socks_server_stop(te_socks_server *instance)
{
    /* Don't care about result much */
    socks_server_send_signal(instance, SIGTERM);

    unlink(instance->pid_path);
    unlink(instance->config_path);
    unlink(instance->user_pass_path);

    instance->status = FALSE;

    return 0;
}


/**
 * Get the first IP for specific interface name/family.
 *
 * @param[in]   ifname  Target interface name.
 * @param[in]   family  Target address family.
 * @param[out]  ip      Buffer to carry obtained IP.
 * @param[in]   ip_size Size of @p ip buffer.
 *
 * @return Status code.
 */
static te_errno
socks_getifaddrs(const char *ifname, int family, char *ip, int ip_size)
{
    struct ifaddrs *ifaddr, *ifa;
    int             rc = ENOENT;
    char           *s;

    if (getifaddrs(&ifaddr) == -1)
        return TE_RC(TE_TA_UNIX, TE_EFAULT);

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        if (strcmp(ifname, ifa->ifa_name) != 0 ||
            family != ifa->ifa_addr->sa_family)
            continue;

        s = te_ip2str(ifa->ifa_addr);
        if (s == NULL)
            continue;
        if (strlen(s) < (size_t)ip_size)
        {
            strcpy(ip, s);
            rc = 0;
        }
        else
        {
            ERROR("IP string doesn't fit in suggested buffer: '%s'", s);
            rc = TE_RC(TE_TA_UNIX, TE_ENOMEM);
        }
        free(s);
        break;
    }

    freeifaddrs(ifaddr);

    return rc;
}

/** Obtain process ID of running SOCKS daemon, or @c -1 */
static te_errno
socks_process_id_get(unsigned int gid, const char *oid,
                     char *value, const char *instN, ...)
{
    te_socks_server *instance = socks_get_server(instN);
    int pid;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    pid = read_pid(instance->pid_path);
    sprintf(value, "%d", pid);

    return 0;
}

/** Get actual Socks daemon status */
static te_errno
socks_status_get(unsigned int gid, const char *oid,
                 char *value, const char *instN, ...)
{
    te_socks_server    *instance = socks_get_server(instN);
    int                 status_local = 0;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    strcpy(value, "0");

    if (instance->status)
    {
        if (socks_server_send_signal(instance, 0) == 0)
            status_local = 1;

        sprintf(value, "%d", status_local);
    }

    return 0;
}

/** Set desired Socks daemon status */
static te_errno
socks_status_set(unsigned int gid, const char *oid,
                 const char *value, const char *instN, ...)
{
    te_socks_server    *instance = socks_get_server(instN);
    int                 new_status;
    te_errno            rc;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    new_status = atoi(value);
    if (new_status != 1 && new_status != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (new_status != instance->status)
    {
        if (new_status == 1)
        {
            rc = socks_server_start(instance);
            if (rc != 0)
            {
                ERROR("Couldn't start server: %r", rc);
                return rc;
            }
        }
        else
        {
            rc = socks_server_stop(instance);
            if (rc != 0)
            {
                ERROR("Couldn't stop server: %r", rc);
                return rc;
            }
        }
    }

    return 0;
}

/** Obtain current implementation of socks server */
static te_errno
socks_impl_get(unsigned int gid, const char *oid,
                 char *value, const char *instN, ...)
{
    te_socks_server *instance = socks_get_server(instN);

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    strcpy(value, instance->impl != NULL ? instance->impl : "");

    return 0;
}

/** Set currently used socks implementation */
static te_errno
socks_impl_set(unsigned int gid, const char *oid,
               const char *value, const char *instN, ...)
{
    te_socks_server *instance = socks_get_server(instN);
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    if (strcmp(value, SOCKS_IMPLEMENTATION_SRELAY) != 0)
        return  TE_RC(TE_TA_UNIX, TE_EINVAL);

    rc = socks_override_str(&instance->impl, value);
    if (rc != 0)
        return rc;

    socks_server_restart(instance);

    return 0;
}

/** Definition of add method for protocols */
static te_errno
socks_proto_add(unsigned int gid, const char *oid, const char *value,
                const char *socks,
                const char *proto_name,  ...)
{
    te_socks_server *instance = socks_get_server(socks);
    te_socks_proto  *p;
    te_errno         rc;
    unsigned long    val;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    SOCKS_CHECK(instance);

    if ((p = te_socks_proto_find(instance, proto_name)) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if ((p = calloc(1, sizeof(te_socks_proto))) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((p->name = strdup(proto_name)) == NULL)
    {
        free(p);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    rc = te_strtoul(value, 10, &val);
    if (rc != 0)
    {
        free(p->name);
        free(p);
        return rc;
    }

    p->proto = val;

    LIST_INSERT_HEAD(&instance->protocols, p, list);

    socks_server_restart(instance);

    return 0;
}

/** Definition of delete method for protocol */
static te_errno
socks_proto_del(unsigned int gid, const char *oid,
                const char *socks,
                const char *proto_name, ...)
{
    te_socks_server *instance = socks_get_server(socks);
    te_socks_proto  *p;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    p = te_socks_proto_find(instance, proto_name);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_REMOVE(p, list);
    te_socks_proto_free(p);

    socks_server_restart(instance);

    return 0;
}

/** List users */
static te_errno
socks_proto_list(unsigned int gid, const char *oid, const char *sub_id,
                char **list, const char *socks, ...)
{
    te_socks_server *instance = socks_get_server(socks);
    te_socks_proto  *p;
    te_errno         rc;
    te_string        str = TE_STRING_INIT;

    UNUSED(oid);
    UNUSED(gid);
    UNUSED(sub_id);

    SOCKS_CHECK(instance);

    LIST_FOREACH(p, &instance->protocols, list)
    {
        rc = te_string_append(&str,
                              (str.ptr != NULL) ? " %s" : "%s",
                              p->name);
        if (rc != 0)
        {
            te_string_free(&str);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }
    *list = str.ptr;

    return 0;
}

/** Obtain protocol value */
static te_errno
socks_proto_get(unsigned int gid, const char *oid,
                char *value, const char *inst_name,
                const char *proto_name, ...)
{
    te_socks_server *instance = socks_get_server(inst_name);
    te_socks_proto  *p;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    p = te_socks_proto_find(instance, proto_name);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(value, "%u", p->proto);

    return 0;
}

/** Set protocol value for specific proto structure */
static te_errno
socks_proto_set(unsigned int gid, const char *oid,
                const char *value, const char *inst_name,
                const char *proto_name, ...)
{
    te_socks_server *instance = socks_get_server(inst_name);
    te_socks_proto  *p;
    te_errno         rc;
    unsigned long    val;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    p = te_socks_proto_find(instance, proto_name);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = te_strtoul(value, 10, &val);
    if (rc != 0)
        return rc;

    p->proto = val;
    socks_server_restart(instance);

    return 0;
}

/** Definition of add method for interfaces */
static te_errno
socks_interface_add(unsigned int gid, const char *oid, const char *value,
                    const char *socks,
                    const char *interface_name,  ...)
{
    te_socks_server    *instance = socks_get_server(socks);
    te_socks_interface *iface;
    te_errno            rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    SOCKS_CHECK(instance);

    if ((iface = te_socks_interface_find(instance, interface_name)) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if ((iface = calloc(1, sizeof(te_socks_interface))) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((iface->name = strdup(interface_name)) == NULL)
    {
        free(iface);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    iface->port = SOCKS_DEFAULT_PORT;
    iface->addr_family = RPC_AF_INET;

    rc = socks_override_str(&iface->interface, value);
    if (rc != 0)
    {
        free(iface->name);
        free(iface);
        return rc;
    }

    LIST_INSERT_HEAD(&instance->interfaces, iface, list);

    socks_server_restart(instance);

    return 0;
}

/** Definition of delete method for interfaces */
static te_errno
socks_interface_del(unsigned int gid, const char *oid,
                    const char *socks,
                    const char *interface_name, ...)
{
    te_socks_server    *instance = socks_get_server(socks);
    te_socks_interface *iface;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    iface = te_socks_interface_find(instance, interface_name);
    if (iface == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_REMOVE(iface, list);
    te_socks_interface_free(iface);

    socks_server_restart(instance);

    return 0;
}

/** List interfaces by friendly name */
static te_errno
socks_interface_list(unsigned int gid, const char *oid, const char *sub_id,
                     char **list, const char *socks, ...)
{
    te_socks_server    *instance = socks_get_server(socks);
    te_socks_interface *iface;
    te_errno            rc;
    te_string           str = TE_STRING_INIT;

    UNUSED(oid);
    UNUSED(gid);
    UNUSED(sub_id);

    SOCKS_CHECK(instance);

    LIST_FOREACH(iface, &instance->interfaces, list)
    {
        rc = te_string_append(&str,
                              (str.ptr != NULL) ? " %s" : "%s",
                              iface->name);
        if (rc != 0)
        {
            te_string_free(&str);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }
    *list = str.ptr;

    return 0;
}

/** Obtain interface system name */
static te_errno
socks_interface_get(unsigned int gid, const char *oid,
                    char *value, const char *inst_name,
                    const char *interface_name, ...)
{
    te_socks_server    *instance = socks_get_server(inst_name);
    te_socks_interface *iface;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    iface = te_socks_interface_find(instance, interface_name);
    if (iface == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(value, iface->interface != NULL ? iface->interface : "");

    return 0;
}

/** Set interface system name */
static te_errno
socks_interface_set(unsigned int gid, const char *oid,
                    const char *value, const char *inst_name,
                    const char *interface_name, ...)
{
    te_socks_server    *instance = socks_get_server(inst_name);
    te_socks_interface *iface;
    te_errno            rc;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    iface = te_socks_interface_find(instance, interface_name);
    if (iface == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = socks_override_str(&iface->interface, value);
    if (rc != 0)
        return rc;

    socks_server_restart(instance);

    return 0;
}

/** Obtain the port associated with the interface */
static te_errno
socks_interface_port_get(unsigned int gid, const char *oid,
                         char *value, const char *inst_name,
                         const char *interface_name, ...)
{
    te_socks_server    *instance = socks_get_server(inst_name);
    te_socks_interface *iface;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    iface = te_socks_interface_find(instance, interface_name);
    if (iface == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(value, "%u", iface->port);

    return 0;
}

/** Set the port associated with the interface */
static te_errno
socks_interface_port_set(unsigned int gid, const char *oid,
                         const char *value, const char *inst_name,
                         const char *interface_name, ...)
{
    te_socks_server    *instance = socks_get_server(inst_name);
    te_socks_interface *iface;
    te_errno            rc;
    unsigned long       port;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    iface = te_socks_interface_find(instance, interface_name);
    if (iface == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = te_strtoul(value, 10, &port);
    if (rc != 0)
        return rc;

    if (port > UINT16_MAX)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    iface->port = (uint16_t)port;
    socks_server_restart(instance);

    return 0;
}

/** Obtain the address family associated with the interface */
static te_errno
socks_interface_addr_family_get(unsigned int gid, const char *oid,
                                char *value, const char *inst_name,
                                const char *interface_name, ...)
{
    te_socks_server    *instance = socks_get_server(inst_name);
    te_socks_interface *iface;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    iface = te_socks_interface_find(instance, interface_name);
    if (iface == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    sprintf(value, "%d", iface->addr_family);

    return 0;
}

/** Set the address family associated with the interface */
static te_errno
socks_interface_addr_family_set(unsigned int gid, const char *oid,
                                const char *value, const char *inst_name,
                                const char *interface_name, ...)
{
    te_socks_server    *instance = socks_get_server(inst_name);
    te_socks_interface *iface;
    te_errno            rc;
    unsigned long       family;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    iface = te_socks_interface_find(instance, interface_name);
    if (iface == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = te_strtoul(value, 10, &family);
    if (rc != 0)
        return rc;

    if (family > SHRT_MAX)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    iface->addr_family = (int)family;
    socks_server_restart(instance);

    return 0;
}

/** Obtain authentication type */
static te_errno
socks_auth_get(unsigned int gid, const char *oid,
               char *value, const char *instN, ...)
{
    te_socks_server *instance = socks_get_server(instN);

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    strcpy(value, "plain");

    return 0;
}

/** Set the current authentication type */
static te_errno
socks_auth_set(unsigned int gid, const char *oid,
               const char *value, const char *instN, ...)
{
    te_socks_server *instance = socks_get_server(instN);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    SOCKS_CHECK(instance);

    return 0;
}

/** Obtain the current interface Socks binds to */
static te_errno
socks_outbound_interface_get(unsigned int gid, const char *oid,
                             char *value, const char *instN, ...)
{
    te_socks_server *instance = socks_get_server(instN);

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    strcpy(value,
           instance->outbound_interface != NULL ?
           instance->outbound_interface :
           "");

    return 0;
}

/** Set the current interface Socks binds to */
static te_errno
socks_outbound_interface_set(unsigned int gid, const char *oid,
                             const char *value, const char *instN, ...)
{
    te_socks_server *instance = socks_get_server(instN);
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    rc = socks_override_str(&instance->outbound_interface, value);
    if (rc != 0)
        return rc;

    socks_server_restart(instance);

    return 0;
}

/** Definition of add method for ciphers */
static te_errno
socks_cipher_add(unsigned int gid, const char *oid, const char *value,
                const char *socks,
                const char *cipher_name,  ...)
{
    te_socks_server *instance = socks_get_server(socks);
    te_socks_cipher *c;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    SOCKS_CHECK(instance);

    if ((c = te_socks_cipher_find(instance, cipher_name)) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if ((c = calloc(1, sizeof(te_socks_cipher))) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((c->name = strdup(cipher_name)) == NULL)
    {
        free(c);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    LIST_INSERT_HEAD(&instance->ciphers, c, list);

    socks_server_restart(instance);

    return 0;
}

/** Definition of delete method for cipher */
static te_errno
socks_cipher_del(unsigned int gid, const char *oid,
                 const char *socks,
                 const char *cipher_name, ...)
{
    te_socks_server *instance = socks_get_server(socks);
    te_socks_cipher *c;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    c = te_socks_cipher_find(instance, cipher_name);
    if (c == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_REMOVE(c, list);
    te_socks_cipher_free(c);

    socks_server_restart(instance);

    return 0;
}

/** List ciphers */
static te_errno
socks_cipher_list(unsigned int gid, const char *oid, const char *sub_id,
                  char **list, const char *socks, ...)
{
    te_socks_server *instance = socks_get_server(socks);
    te_socks_cipher *c;
    te_errno         rc;
    te_string        str = TE_STRING_INIT;

    UNUSED(oid);
    UNUSED(gid);
    UNUSED(sub_id);

    SOCKS_CHECK(instance);

    LIST_FOREACH(c, &instance->ciphers, list)
    {
        rc = te_string_append(&str,
                              (str.ptr != NULL) ? " %s" : "%s",
                              c->name);
        if (rc != 0)
        {
            te_string_free(&str);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }
    *list = str.ptr;

    return 0;
}

/** Obtain cipher from ciphers list */
static te_errno
socks_cipher_get(unsigned int gid, const char *oid,
                 char *value, const char *inst_name,
                 const char *cipher_name, ...)
{
    te_socks_server *instance = socks_get_server(inst_name);
    te_socks_cipher *c;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    c = te_socks_cipher_find(instance, cipher_name);
    if (c == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(value, c->cipher != NULL ? c->cipher : "");

    return 0;
}

/** Set cipher in the ciphers list */
static te_errno
socks_cipher_set(unsigned int gid, const char *oid,
                 const char *value, const char *inst_name,
                 const char *cipher_name, ...)
{
    te_socks_server *instance = socks_get_server(inst_name);
    te_socks_cipher *c;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    c = te_socks_cipher_find(instance, cipher_name);
    if (c == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = socks_override_str(&c->cipher, value);
    if (rc != 0)
        return rc;

    socks_server_restart(instance);

    return 0;
}

/** Definition of add method for users */
static te_errno
socks_user_add(unsigned int gid, const char *oid, const char *value,
               const char *socks,
               const char *name,  ...)
{
    te_socks_server *instance = socks_get_server(socks);
    te_socks_user   *u;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    SOCKS_CHECK(instance);

    if ((u = te_socks_user_find(instance, name)) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if ((u = calloc(1, sizeof(te_socks_user))) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((u->name = strdup(name)) == NULL)
    {
        free(u);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    LIST_INSERT_HEAD(&instance->users, u, list);

    socks_server_restart(instance);

    return 0;
}

/** Definition of delete method for users */
static te_errno
socks_user_del(unsigned int gid, const char *oid,
               const char *socks,
               const char *name, ...)
{
    te_socks_server *instance = socks_get_server(socks);
    te_socks_user   *u;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    u = te_socks_user_find(instance, name);
    if (u == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_REMOVE(u, list);
    te_socks_user_free(u);

    socks_server_restart(instance);

    return 0;
}

/** List users */
static te_errno
socks_user_list(unsigned int gid, const char *oid, const char *sub_id,
                char **list, const char *socks, ...)
{
    te_socks_server *instance = socks_get_server(socks);
    te_socks_user   *u;
    te_errno         rc;
    te_string        str = TE_STRING_INIT;

    UNUSED(oid);
    UNUSED(gid);
    UNUSED(sub_id);

    SOCKS_CHECK(instance);

    LIST_FOREACH(u, &instance->users, list)
    {
        rc = te_string_append(&str,
                              (str.ptr != NULL) ? " %s" : "%s",
                              u->name);
        if (rc != 0)
        {
            te_string_free(&str);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }
    *list = str.ptr;

    return 0;
}

/** Obtain user's next hop server IP */
static te_errno
socks_user_next_hop_get(unsigned int gid, const char *oid,
                        char *value, const char *inst_name,
                        const char *user_name, ...)
{
    te_socks_server *instance = socks_get_server(inst_name);
    te_socks_user   *user;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    user = te_socks_user_find(instance, user_name);
    if (user == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(value, user->next_hop != NULL ? user->next_hop : "");

    return 0;
}

/** Set user's next hop server IP */
static te_errno
socks_user_next_hop_set(unsigned int gid, const char *oid,
                        const char *value, const char *inst_name,
                        const char *user_name, ...)
{
    te_socks_server *instance = socks_get_server(inst_name);
    te_socks_user   *user;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    user = te_socks_user_find(instance, user_name);
    if (user == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = socks_override_str(&user->next_hop, value);
    if (rc != 0)
        return rc;

    socks_server_restart(instance);

    return 0;
}

/** Obtain user's username */
static te_errno
socks_user_username_get(unsigned int gid, const char *oid,
                        char *value, const char *inst_name,
                        const char *user_name, ...)
{
    te_socks_server *instance = socks_get_server(inst_name);
    te_socks_user   *user;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    user = te_socks_user_find(instance, user_name);
    if (user == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(value, user->username != NULL ? user->username : "");

    return 0;
}

/** Set user's username */
static te_errno
socks_user_username_set(unsigned int gid, const char *oid,
                        const char *value, const char *inst_name,
                        const char *user_name, ...)
{
    te_socks_server *instance = socks_get_server(inst_name);
    te_socks_user   *user;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    user = te_socks_user_find(instance, user_name);
    if (user == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = socks_override_str(&user->username, value);
    if (rc != 0)
        return rc;

    socks_server_restart(instance);

    return 0;
}

/** Get user's password */
static te_errno
socks_user_password_get(unsigned int gid, const char *oid,
                        char *value, const char *inst_name,
                        const char *user_name, ...)
{
    te_socks_server *instance = socks_get_server(inst_name);
    te_socks_user   *user;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    user = te_socks_user_find(instance, user_name);
    if (user == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    strcpy(value, user->password != NULL ? user->password : "");

    return 0;
}

/** Set user's password */
static te_errno
socks_user_password_set(unsigned int gid, const char *oid,
                        const char *value, const char *inst_name,
                        const char *user_name, ...)
{
    te_socks_server *instance = socks_get_server(inst_name);
    te_socks_user   *user;
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    SOCKS_CHECK(instance);

    user = te_socks_user_find(instance, user_name);
    if (user == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    rc = socks_override_str(&user->password, value);
    if (rc != 0)
        return rc;

    socks_server_restart(instance);

    return 0;
}

/**
 * Grab resources allocated by daemon.
 *
 * @param name Resource name.
 *
 * @return Status code.
 */
te_errno
socks_grab(const char *name)
{
    UNUSED(name);

    if (access(SOCKS_PATH, X_OK) != 0)
    {
        ERROR("Socks server executable was not found");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    return 0;
}

/**
 * Release resources allocated by daemon.
 *
 * @param name Resource name.
 *
 * @return Status code.
 */
te_errno
socks_release(const char *name)
{
    UNUSED(name);
    return 0;
}

/** Add a new server */
static te_errno
socks_add(unsigned int gid, const char *oid,
          const char *value, const char *socks, ...)
{
    te_socks_server  *instance;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    instance = socks_get_server(socks);
    if (instance != NULL)
    {
        ERROR("Server with such name already exists: %s", instance);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    instance = socks_server_create(socks);
    if (instance == NULL)
    {
        ERROR("Couldn't allocate server instance");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    socks_add_server(instance);

    return 0;
}

/** Delete server */
static te_errno
socks_del(unsigned int gid, const char *oid,
          const char *socks, ...)
{
    te_socks_server   *instance;
    te_errno           rc;

    UNUSED(gid);
    UNUSED(oid);

    instance = socks_get_server(socks);
    if (instance == NULL)
    {
        ERROR("Unknown server is ought to be stopped");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (instance->status)
    {
        rc = socks_server_stop(instance);
        if (rc != 0)
            WARN("Couldn't stop instance, continue removing anyway: %r", rc);
    }

    instance = socks_del_server(socks);
    if (instance == NULL)
    {
        ERROR("Unknown server is ought to be released");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    socks_server_free(instance);
    return 0;
}

/** List instances */
static te_errno
socks_list(unsigned int gid, const char *oid,
           const char *sub_id, char **list, ...)
{
    te_socks_server    *s;
    te_errno            rc;
    te_string           str = TE_STRING_INIT;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    LIST_FOREACH(s, &servers, list)
    {
        rc = te_string_append(&str,
                              (str.ptr != NULL) ? " %s" : "%s",
                              s->name);
        if (rc != 0)
        {
            te_string_free(&str);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }

    *list = str.ptr;
    return 0;
}

/** Initialize the tree */
te_errno
ta_unix_conf_socks_init(void)
{
    LIST_INIT(&servers);
    return rcf_pch_add_node("/agent/", &node_socks);
}

RCF_PCH_CFG_NODE_RW(node_socks_user_password, "password",
                    NULL, NULL,
                    socks_user_password_get,
                    socks_user_password_set);

RCF_PCH_CFG_NODE_RW(node_socks_user_username, "username",
                    NULL, &node_socks_user_password,
                    socks_user_username_get,
                    socks_user_username_set);

RCF_PCH_CFG_NODE_RW(node_socks_user_next_hop, "next_hop",
                    NULL, &node_socks_user_username,
                    socks_user_next_hop_get,
                    socks_user_next_hop_set);

RCF_PCH_CFG_NODE_RO(node_socks_process_id, "process_id",
                    NULL, NULL,
                    socks_process_id_get);

RCF_PCH_CFG_NODE_COLLECTION(node_socks_user, "user",
                            &node_socks_user_next_hop,
                            &node_socks_process_id,
                            socks_user_add, socks_user_del,
                            socks_user_list, NULL);
RCF_PCH_CFG_NODE_RW(node_socks_auth, "auth",
                    NULL, &node_socks_user,
                    socks_auth_get, socks_auth_set);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_socks_cipher, "cipher",
                               NULL, &node_socks_auth,
                               socks_cipher_get, socks_cipher_set,
                               socks_cipher_add, socks_cipher_del,
                               socks_cipher_list, NULL);

RCF_PCH_CFG_NODE_RW(node_socks_outbound_interface, "outbound_interface",
                    NULL, &node_socks_cipher,
                    socks_outbound_interface_get, socks_outbound_interface_set);

RCF_PCH_CFG_NODE_RW(node_socks_interface_addr_family, "addr_family",
                    NULL, NULL,
                    socks_interface_addr_family_get,
                    socks_interface_addr_family_set);

RCF_PCH_CFG_NODE_RW(node_socks_interface_port, "port",
                    NULL, &node_socks_interface_addr_family,
                    socks_interface_port_get,
                    socks_interface_port_set);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_socks_interface, "interface",
                               &node_socks_interface_port,
                               &node_socks_outbound_interface,
                               socks_interface_get, socks_interface_set,
                               socks_interface_add, socks_interface_del,
                               socks_interface_list, NULL);

RCF_PCH_CFG_NODE_RW_COLLECTION(node_socks_proto, "proto",
                               NULL, &node_socks_interface,
                               socks_proto_get, socks_proto_set,
                               socks_proto_add, socks_proto_del,
                               socks_proto_list, NULL);

RCF_PCH_CFG_NODE_RW(node_socks_impl, "impl",
                    NULL, &node_socks_proto,
                    socks_impl_get,
                    socks_impl_set);

RCF_PCH_CFG_NODE_RW(node_socks_status, "status",
                    NULL, &node_socks_impl,
                    socks_status_get,
                    socks_status_set);

/* Configuration subtree root /agent/socks */
RCF_PCH_CFG_NODE_RW_COLLECTION(node_socks, "socks",
                               &node_socks_status, NULL,
                               socks_status_get, socks_status_set,
                               socks_add, socks_del, socks_list, NULL);
