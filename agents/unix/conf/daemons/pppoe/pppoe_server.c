/** @file
 * @brief Unix Test Agent
 *
 * PPPoE server configuration support
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Igor Labutin <Igor.Labutin@oktetlabs.ru>
 * @author Alexander Kukuta <Alexander.Kukuta@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <netinet/in.h>
#include "conf_daemons.h"

#include "te_defs.h"
#include "te_queue.h"
#include "te_sockaddr.h"
#include "te_string.h"
#include "rcf_pch.h"
#include "logger_api.h"

#ifndef __linux__
#warn PPPoE server support may not work properly on non-linux test agents
#endif

/** PPPoE server executable name */
#define PPPOE_SERVER_EXEC "/usr/sbin/pppoe-server"

/** PPPoE server configuration file name */
#define PPPOE_SERVER_CONF "/tmp/te.pppoe-server.conf"

/** Default buffer size for command-line construction */
#define PPPOE_MAX_CMD_SIZE 1024

/** Default prefix for pppoe server subnet option */
#define PPPOE_SUBNET_PREFIX_DEFAULT 32

/** Default number of pppoe clients supported by pppoe-server */
#define PPPOE_MAX_SESSIONS 64

/**
 * Default amount of memory allocated for list methods
 * of pppoeserver subtreee
 */
#define PPPOE_SERVER_LIST_SIZE 1024

/** Default local IP address, see manual pppoe-server(8) */
#define PPPOE_SERVER_LADDR_DEFAULT  "10.0.0.1"

/** Default starting remote IP address, see manual pppoe-server(8) */
#define PPPOE_SERVER_RADDR_DEFAULT  "10.67.15.1"


/* Definitions of types for PPPoE configuring */

/** Options written to pppoe-server configuration file */
typedef struct te_pppoe_option {
    SLIST_ENTRY(te_pppoe_option) list;

    char *name;                 /**< Option name */
    char *value;                /**< Option value */
} te_pppoe_option;

/** Interfaces specified with -I parameter to pppoe-server */
typedef struct te_pppoe_if {
    SLIST_ENTRY(te_pppoe_if) list;

    char *ifname;               /* Interface name to listen on */
} te_pppoe_if;

/** PPPoE server configuration structure */
typedef struct te_pppoe_server {
    SLIST_HEAD(, te_pppoe_if) ifs; /**< Interfaces specified with
                                       -I parameter to pppoe-server */
    SLIST_HEAD(, te_pppoe_option) options; /**< Options written to
                                                pppoe-server configuration
                                                file */

    in_addr_t subnet; /**< Subnet used for generating local and
                           remote addresses (-L and -R options) */
    int       prefix; /**< Subnet prefix  */
    int       max_sessions; /**< Maximum allowed ppp sessions */

    te_bool   initialised; /**< structure initialisation flag */
    te_bool   started;     /**< admin status for pppoe server */
    te_bool   changed;     /**< configuration changed flag, used to detect
                                if pppoe-server restart is required */

    struct sockaddr_storage laddr;  /**< Local IP address */
    struct sockaddr_storage raddr;  /**< Starting remote IP address */
} te_pppoe_server;

static te_bool
pppoe_server_is_running(te_pppoe_server *pppoe);

/** Static pppoe server structure */
static te_pppoe_server pppoe_server;


/**
 * Generate local IP address based on subnet option.
 *
 * @param pppoe     pppoe server structure.
 */
static void
generate_laddr(te_pppoe_server *pppoe)
{
    struct in_addr *addr = &SIN(&pppoe->laddr)->sin_addr;
    te_errno        rc;

    SIN(&pppoe->laddr)->sin_family = AF_INET;
    SIN(&pppoe->laddr)->sin_port = 0;

    if (pppoe->subnet != ntohs(INADDR_ANY))
        addr->s_addr = htonl(ntohl(pppoe->subnet) + 1);
    else
    {
        /* Use default, see manual pppoe-server(8) */
        rc = te_sockaddr_str2h(PPPOE_SERVER_LADDR_DEFAULT, SA(&pppoe->laddr));
        assert(rc == 0);
    }
}

/**
 * Generate starting remote IP address based on subnet option.
 *
 * @param pppoe     pppoe server structure.
 */
static void
generate_raddr(te_pppoe_server *pppoe)
{
    struct in_addr *addr = &SIN(&pppoe->raddr)->sin_addr;
    te_errno        rc;

    SIN(&pppoe->raddr)->sin_family = AF_INET;
    SIN(&pppoe->raddr)->sin_port = 0;

    if (pppoe->subnet != ntohs(INADDR_ANY))
    {
        /* This fix is ugly, indeed. Just to prevent overlapping of local
         * and remote IPs for multiple pppoe clients. To be replaced by
         * more nice one after all problems with pppoe and dhcp servers
         * have been fixed.
         */
        addr->s_addr = htonl(ntohl(pppoe->subnet) + pppoe->max_sessions + 1);
    }
    else
    {
        /* Use default, see manual pppoe-server(8) */
        rc = te_sockaddr_str2h(PPPOE_SERVER_RADDR_DEFAULT, SA(&pppoe->raddr));
        assert(rc == 0);
    }
}

/**
 * Initialise pppoe server structure with default values
 *
 * @param pppoe         PPPoE server structure to initialise
 *
 * @return N/A
 */
static void
pppoe_server_init(te_pppoe_server *pppoe)
{
    ENTRY("Initialize pppoe server object");

    SLIST_INIT(&pppoe->ifs);
    SLIST_INIT(&pppoe->options);
    pppoe->subnet = INADDR_ANY;
    pppoe->prefix = 0;
    pppoe->max_sessions = PPPOE_MAX_SESSIONS;
    pppoe->started = pppoe_server_is_running(pppoe);
    pppoe->changed = pppoe->started;
    pppoe->initialised = TRUE;
    generate_laddr(pppoe);
    generate_raddr(pppoe);

    EXIT("pppoe server object has been initialized");
}

/**
 * Return pointer to static pppoe server structure
 *
 * @return pppoe server structure
 */
static te_pppoe_server *
pppoe_server_find(void)
{
    te_pppoe_server *pppoe = &pppoe_server;
    if (!pppoe->initialised)
        pppoe_server_init(pppoe);

    return pppoe;
}

/**
 * Prepare configuration file for pppoe-server
 *
 * @param pppoe   pppoe server structure
 *
 * @return Status code
 */
static te_errno
pppoe_server_save_conf(te_pppoe_server *pppoe)
{
    FILE                    *f = NULL;
    te_pppoe_option         *opt;

    ENTRY("Save pppoe server configuration into '%s'", PPPOE_SERVER_CONF);

    f = fopen(PPPOE_SERVER_CONF, "w");
    if (f == NULL)
    {
        ERROR("Failed to open '%s' for writing: %s",
              PPPOE_SERVER_CONF, strerror(errno));
        EXIT("Failed to save configuration");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    /* TODO: initialise option list with default options */
    fprintf(f, "noauth\n");
    fprintf(f, "lcp-echo-interval 10\n");
    fprintf(f, "lcp-echo-failure 2\n");
    fprintf(f, "nodefaultroute\n");
    fprintf(f, "mru 1492\n");
    fprintf(f, "mtu 1492\n");

    for (opt = SLIST_FIRST(&pppoe->options);
         opt != NULL; opt = SLIST_NEXT(opt, list))
    {
        fprintf(f, "%s %s\n", opt->name, opt->value);
    }

    if (fsync(fileno(f)) != 0)
    {
        int err = errno;

        ERROR("%s(): fsync() failed: %s", __FUNCTION__, strerror(err));
        (void)fclose(f);
        EXIT("Failed to save configuration");
        return TE_OS_RC(TE_TA_UNIX, err);
    }

    if (fclose(f) != 0)
    {
        ERROR("%s(): fclose() failed: %s", __FUNCTION__, strerror(errno));
        EXIT("Failed to save configuration");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    EXIT("pppoe server configuration has been saved");

    return 0;
}


/**
 * Build command to run pppoe-server.
 *
 * @param[in]  pppoe    pppoe server structure.
 * @param[out] cmd      Buffer contains pppoe server run command.
 *
 * @return Status code
 */
static te_errno
pppoe_server_build_cmd(te_pppoe_server *pppoe, te_string *cmd)
{
    te_pppoe_if *iface;
    te_errno     rc;

    te_string_reset(cmd);

    rc = te_string_append(cmd, "%s -O %s",
                          PPPOE_SERVER_EXEC, PPPOE_SERVER_CONF);
    if (rc != 0)
        return rc;

    if (SIN(&pppoe->laddr)->sin_addr.s_addr != INADDR_ANY)
    {
        rc = te_string_append(cmd, " -L %s",
                              te_sockaddr_get_ipstr(SA(&pppoe->laddr)));
        if (rc != 0)
            return rc;
    }

    if (SIN(&pppoe->raddr)->sin_addr.s_addr != INADDR_ANY)
    {
        rc = te_string_append(cmd, " -R %s",
                              te_sockaddr_get_ipstr(SA(&pppoe->raddr)));
        if (rc != 0)
            return rc;
    }

    SLIST_FOREACH(iface, &pppoe->ifs, list)
    {
        rc = te_string_append(cmd, " -I %s", iface->ifname);
        if (rc != 0)
            return rc;
    }

    INFO("Command to run pppoe server: '%s'", cmd->ptr);

    return 0;
}

/**
 * Check if pppoe-server is running
 *
 * @param pppoe   pppoe server structure
 *
 * @return pppoe server running status
 */
static te_bool
pppoe_server_is_running(te_pppoe_server *pppoe)
{
    te_bool is_running;
    char buf[PPPOE_MAX_CMD_SIZE];

    UNUSED(pppoe);

    sprintf(buf, PS_ALL_COMM " | grep -v grep | grep -q %s >/dev/null 2>&1",
            PPPOE_SERVER_EXEC);

    is_running = (ta_system(buf) == 0);

    INFO("pppoe server is%s running", (is_running ? "" : " not"));

    return is_running;
}

/**
 * Stop pppoe-server process
 *
 * @param pppoe   pppoe server structure
 *
 * @return status code
 */
static te_errno
pppoe_server_stop(te_pppoe_server *pppoe)
{
    char buf[PPPOE_MAX_CMD_SIZE];

    UNUSED(pppoe);

    ENTRY("Stop pppoe server");

    /* Quit if pppoe server is not running */
    if (!pppoe_server_is_running(pppoe))
    {
        EXIT("pppoe server is not running");
        return 0;
    }

    TE_SPRINTF(buf, "killall %s", PPPOE_SERVER_EXEC);
    if (ta_system(buf) != 0)
    {
        ERROR("Command '%s' failed", buf);
        EXIT("Failed to stop pppoe server");
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    /*
     * We should kill all pppd sessions. Killing with some polite signals
     * does not help
     */
    (void)ta_system("killall -KILL pppd");

    if (unlink(PPPOE_SERVER_CONF) != 0 &&
        errno != ENOENT)
    {
        WARN("Failed to delete PPPoE server temporary configuration "
             "file '%s': %s", PPPOE_SERVER_CONF, strerror(errno));
    }

    EXIT("pppoe server has been stopped");

    return 0;
}

/**
 * Start pppoe-server process
 *
 * @param pppoe   pppoe server structure
 *
 * @return status code
 */
static te_errno
pppoe_server_start(te_pppoe_server *pppoe)
{
    te_string cmd = TE_STRING_INIT;
    te_errno  rc;

    ENTRY("Start pppoe server");

#define PPPOE_SERVER_START_FAIL(_rc, _msg...)   \
    do {                                        \
        ERROR(_msg);                            \
        te_string_free(&cmd);                   \
        EXIT("Failed to start pppoe server");   \
        return _rc;                             \
    } while (0)

    rc = pppoe_server_save_conf(pppoe);
    if (rc != 0)
        PPPOE_SERVER_START_FAIL(rc,
                            "Failed to save PPPoE server configuration file");

    rc = pppoe_server_build_cmd(pppoe, &cmd);
    if (rc != 0)
        PPPOE_SERVER_START_FAIL(rc,
                            "Failed to prepare arguments to run pppoe-server");

    RING("Run command: %s", cmd.ptr);
    if (ta_system(cmd.ptr) != 0)
        PPPOE_SERVER_START_FAIL(TE_RC(TE_TA_UNIX, TE_ESHCMD),
                                "Command '%s' failed", cmd.ptr);

#undef PPPOE_SERVER_START_FAIL

    te_string_free(&cmd);
    EXIT("pppoe server has been started");

    return 0;
}

/**
 * Get method forthat returns Stop pppoe-server process
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         location for the pppoe server status result
 *
 * @return status code
 */
static te_errno
pppoe_server_get(unsigned int gid, const char *oid, char *value)
{
    te_pppoe_server *pppoe = pppoe_server_find();

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("Get pppoe server status");

    sprintf(value, "%s", pppoe_server_is_running(pppoe) ? "1" : "0");

    EXIT("pppoe server status: %s", value);

    return 0;
}

/**
 * Set desired status of pppoe server
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         desired pppoe server status
 *
 * @return status code
 */
static te_errno
pppoe_server_set(unsigned int gid, const char *oid, const char *value)
{
    te_pppoe_server *pppoe = pppoe_server_find();

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("Set pppoe server status to %s", value);

    pppoe->started = (strcmp(value, "1") == 0);
    if (pppoe->started != pppoe_server_is_running(pppoe))
    {
        pppoe->changed = TRUE;
    }

    EXIT("pppoe server status has been set");

    return 0;
}

/**
 * Commit changes in pppoe server configuration.
 * (Re)star/stop pppoe server if required
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         location for the pppoe server status result
 *
 * @return status code
 */
static te_errno
pppoe_server_commit(unsigned int gid, const char *oid)
{
    te_pppoe_server *pppoe = pppoe_server_find();
    te_errno         rc;

    UNUSED(gid);
    UNUSED(oid);

    ENTRY("Commit pppoe server changes");

    /*
     * We don't need to change state of PPPoE Server:
     * The current state is the same as desired.
     */
    if (!pppoe->changed)
    {
        EXIT("There are no pppoe server changes");
        return 0;
    }

    /* Stop pppoe_server if required */
    if ((rc = pppoe_server_stop(pppoe)) != 0)
    {
        ERROR("Failed to stop PPPoE server");
        EXIT("Failed to commit pppoe server changes");
        return rc;
    }

    /* Start pppoe_server with new parameters */
    if (pppoe->started)
    {
        if ((rc = pppoe_server_start(pppoe)) != 0)
        {
            ERROR("Failed to start PPPoE server");
            EXIT("Failed to commit pppoe server changes");
            return rc;
        }
    }

    pppoe->changed = FALSE;

    EXIT("pppoe server changes have been committed");

    return 0;
}


/**
 * Find pppoe server option in options list
 *
 * @param pppoe         pppoe server structure
 * @param name          option name to look for
 *
 * @return pppoe server option structure
 */
static te_pppoe_option *
pppoe_find_option(te_pppoe_server *pppoe, const char *name)
{
    te_pppoe_option *opt;

    for (opt = SLIST_FIRST(&pppoe->options);
         opt != NULL && strcmp(opt->name, name) != 0;
         opt = SLIST_NEXT(opt, list));

    return opt;
}

/**
 * Get callback for /agent/pppoeserver/option node.
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         location for the pppoe server option value
 * @param pppoe_name    dummy parameter due to name of ppppoeserver instance
 *                      name is always empty
 * @param option        option name to look for
 *
 * @return status code
 */
static te_errno
pppoe_server_option_get(unsigned int gid, const char *oid, char *value,
                        const char *pppoe_name, const char *option)
{
    te_pppoe_server *pppoe = pppoe_server_find();
    te_pppoe_option *opt;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(pppoe_name);

    ENTRY("Get pppoe server option '%s'", option);

    if ((opt = pppoe_find_option(pppoe, option)) != NULL)
    {
        strcpy(value, opt->value);
        EXIT("pppoe server option '%s' = '%s'", option, value);
        return 0;
    }

    EXIT("Failed to get pppoe server option");

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/**
 * Set callback for /agent/pppoeserver/option node.
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         pppoe server option value
 * @param pppoe_name    dummy parameter due to name of ppppoeserver instance
 *                      name is always empty
 * @param option        name of option to modify
 *
 * @return status code
 */
static te_errno
pppoe_server_option_set(unsigned int gid, const char *oid,
                        const char *value, const char *pppoe_name,
                        const char *option)
{
    te_pppoe_server *pppoe = pppoe_server_find();
    te_pppoe_option *opt;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(pppoe_name);

    ENTRY("Set pppoe server option '%s' = '%s'", option, value);

    if ((opt = pppoe_find_option(pppoe, option)) != NULL)
    {
        free(opt->value);
        opt->value = strdup(value);
        pppoe->changed = TRUE;
        EXIT("pppoe server option '%s' has been set", option);
        return 0;
    }

    EXIT("Failed to set pppoe server option");

    return TE_RC(TE_TA_UNIX, TE_ENOENT);
}

/**
 * Add callback for /agent/pppoeserver/option node.
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         pppoe server option value
 * @param pppoe_name    dummy parameter due to name of ppppoeserver instance
 *                      name is always empty
 * @param option        name of option to add
 *
 * @return status code
 */
static te_errno
pppoe_server_option_add(unsigned int gid, const char *oid,
                        const char *value, const char *pppoe_name,
                        const char *option)
{
    te_pppoe_server *pppoe = pppoe_server_find();
    te_pppoe_option *opt;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(pppoe_name);

    ENTRY("Add pppoe server option '%s' = '%s'", option, value);

    /* Check if option already exists */
    if ((opt = pppoe_find_option(pppoe, option)) != NULL)
    {
        EXIT("Failed to add pppoe server option");
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    opt = (te_pppoe_option *)calloc(1, sizeof(te_pppoe_option));
    opt->name = strdup(option);
    opt->value = strdup(value);
    SLIST_INSERT_HEAD(&pppoe->options, opt, list);
    pppoe->changed = TRUE;

    EXIT("pppoe server option '%s' has been added", option);

    return 0;
}

/**
 * Delete callback for /agent/pppoeserver/option node.
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param pppoe_name    dummy parameter due to name of ppppoeserver instance
 *                      name is always empty
 * @param option        name of option to delete
 *
 * @return status code
 */
static te_errno
pppoe_server_option_del(unsigned int gid, const char *oid,
                        const char *pppoe_name, const char *option)
{
    te_pppoe_server *pppoe = pppoe_server_find();
    te_pppoe_option *opt;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(pppoe_name);

    ENTRY("Delete pppoe server option '%s'", option);

    /* Check if option already exists */
    if ((opt = pppoe_find_option(pppoe, option)) == NULL)
    {
        EXIT("Failed to delete pppoe server option");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    SLIST_REMOVE(&pppoe->options, opt, te_pppoe_option, list);
    pppoe->changed = TRUE;

    EXIT("pppoe server option '%s' has been deleted", option);

    return 0;
}

/**
 * List callback for /agent/pppoeserver/option node.
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          location for options list
 * @param pppoe_name    dummy parameter due to name of ppppoeserver instance
 *                      name is always empty
 *
 * @return status code
 */
static te_errno
pppoe_server_option_list(unsigned int gid, const char *oid,
                         const char *sub_id, char **list,
                         const char *pppoe_name)
{
    te_pppoe_server *pppoe = pppoe_server_find();
    te_pppoe_option *opt;
    uint32_t         list_size = 0;
    uint32_t         list_len  = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(pppoe_name);

    ENTRY("List pppoe server options");

    list_size = PPPOE_SERVER_LIST_SIZE;
    if ((*list = (char *)calloc(1, list_size)) == NULL)
    {
        EXIT("Failed to list pppoe server options");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    list_len = 0;

    SLIST_FOREACH(opt, &pppoe->options, list)
    {
        if (list_len + strlen(opt->name) + 1 >= list_size)
        {
            list_size *= 2;
            if ((*list = realloc(*list, list_size)) == NULL)
            {
                EXIT("Failed to list pppoe server options");
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
            }
        }

        list_len += sprintf(*list + list_len, "%s ", opt->name);
    }

    EXIT("pppoe server options have been listed");

    return 0;
}

/** Find the option in specified options list */
/**
 * Find interface structure in pppoe server interface list
 *
 * @param pppoe         pppoe server structure
 * @param ifname        interface name to find
 *
 * @return pppoe server option structure
 */
static te_pppoe_if *
pppoe_find_if(te_pppoe_server *pppoe, const char *ifname)
{
    te_pppoe_if *iface;
    for (iface = SLIST_FIRST(&pppoe->ifs);
         iface != NULL && strcmp(iface->ifname, ifname) != 0;
         iface = SLIST_NEXT(iface, list));

    return iface;
}

/**
 * Add callback for /agent/pppoeserver/interface node.
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         unused value of /agent/pppoeserverinterface instance
 * @param pppoe_name    dummy parameter due to name of ppppoeserver instance
 *                      name is always empty
 * @param ifname        interface name to add to the list
 *
 * @return status code
 */
static te_errno
pppoe_server_ifs_add(unsigned int gid, const char *oid,
                     const char *value, const char *pppoe_name,
                     const char *ifname)
{
    te_pppoe_server *pppoe = pppoe_server_find();
    te_pppoe_if     *iface;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(pppoe_name);

    ENTRY("Add pppoe server interface '%s'", ifname);

    /* Check if interface already added */
    if ((iface = pppoe_find_if(pppoe, ifname)) != NULL)
    {
        EXIT("Failed to add pppoe server interface");
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    iface = (te_pppoe_if *)calloc(1, sizeof(te_pppoe_if));
    iface->ifname = strdup(ifname);
    SLIST_INSERT_HEAD(&pppoe->ifs, iface, list);
    pppoe->changed = TRUE;

    EXIT("pppoe server interface '%s' has been added", ifname);

    return 0;
}

/**
 * Delete callback for /agent/pppoeserver/interface node.
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param pppoe_name    dummy parameter due to name of ppppoeserver instance
 *                      name is always empty
 * @param ifname        interface name to delete from the list
 *
 * @return status code
 */
static te_errno
pppoe_server_ifs_del(unsigned int gid, const char *oid,
                     const char *pppoe_name, const char *ifname)
{
    te_pppoe_server *pppoe = pppoe_server_find();
    te_pppoe_if     *iface;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(pppoe_name);

    ENTRY("Delete pppoe server interface '%s'", ifname);

    /* Check if interface does not exists */
    if ((iface = pppoe_find_if(pppoe, ifname)) == NULL)
    {
        EXIT("Failed to delete pppoe server interface");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    SLIST_REMOVE(&pppoe->ifs, iface, te_pppoe_if, list);
    pppoe->changed = TRUE;

    EXIT("pppoe server interface '%s' has been deleted", ifname);

    return 0;
}

/**
 * List callback for /agent/pppoeserver/interface node.
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          location to the pppoe server interfaces list
 * @param pppoe_name    dummy parameter due to name of ppppoeserver instance
 *                      name is always empty
 *
 * @return status code
 */
static te_errno
pppoe_server_ifs_list(unsigned int gid, const char *oid,
                      const char *sub_id, char **list,
                      const char *pppoe_name)
{
    te_pppoe_server *pppoe = pppoe_server_find();
    te_pppoe_if     *iface;
    uint32_t         list_size = 0;
    uint32_t         list_len  = 0;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(pppoe_name);

    ENTRY("List pppoe server interfaces");

    list_size = PPPOE_SERVER_LIST_SIZE;
    if ((*list = (char *)calloc(1, list_size)) == NULL)
    {
        EXIT("Failed to list pppoe server interfaces");
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }
    list_len = 0;

    SLIST_FOREACH(iface, &pppoe->ifs, list)
    {
        if (list_len + strlen(iface->ifname) + 1 >= list_size)
        {
            list_size *= 2;
            if ((*list = realloc(*list, list_size)) == NULL)
            {
                EXIT("Failed to list pppoe server interfaces");
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
            }
        }

        list_len += sprintf(*list + list_len, "%s ", iface->ifname);
    }

    EXIT("pppoe server interfaces have been listed");

    return 0;
}

/**
 * Get callback for /agent/pppoeserver/subnet node.
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         location to the subnet value
 * @param pppoeserver   dummy parameter due to name of ppppoeserver
 *                      instance name is always empty
 *
 * @return status code
 */
static te_errno
pppoe_server_subnet_get(unsigned int gid, const char *oid,
                        char *value, const char *pppoeserver)
{
    te_pppoe_server *pppoe = pppoe_server_find();
    struct in_addr addr;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(pppoeserver);

    ENTRY("Get pppoe server subnet IP address");

    addr.s_addr = pppoe->subnet;
    sprintf(value, "%s|%d", inet_ntoa(addr), pppoe->prefix);

    EXIT("pppoe server subnet IP address: %s", value);

    return 0;
}

/**
 * Set callback for /agent/pppoeserver/subnet node.
 * Subnet address and prefix is encoded into one value  addr|prefix
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param value         location to the subnet value
 * @param pppoeserver   dummy parameter due to name of ppppoeserver
 *                      instance name is always empty
 *
 * @return status code
 */
static te_errno
pppoe_server_subnet_set(unsigned int gid, const char *oid,
                        const char *value, const char *pppoeserver)
{
    te_pppoe_server *pppoe = pppoe_server_find();
    char            *p;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(pppoeserver);

    ENTRY("Set pppoe server subnet IP address: %s", value);

    if ((p = strchr(value, '|')) != NULL)
    {
        *p++ = '\0';
        pppoe->prefix = atoi(p);
    }
    else
        pppoe->prefix = PPPOE_SUBNET_PREFIX_DEFAULT;

    if (inet_aton(value, (struct in_addr *)&pppoe->subnet) == 0)
    {
        ERROR("Invalid pppoe subnet format: %s", value);
        EXIT("Failed to set pppoe server subnet IP address");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }
    pppoe->subnet = htonl(ntohl(pppoe->subnet) & PREFIX2MASK(pppoe->prefix));

    generate_laddr(pppoe);
    generate_raddr(pppoe);

    pppoe->changed = TRUE;

    EXIT("pppoe server subnet IP address has been set");

    return 0;
}

/**
 * Get callback for /agent/pppoeserver/laddr node.
 *
 * @param gid           Group identifier.
 * @param oid           Full identifier of the father instance.
 * @param value         Location to the local address value.
 * @param pppoeserver   Dummy parameter due to name of ppppoeserver
 *                      instance is always empty.
 *
 * @return Status code.
 */
static te_errno
pppoe_server_laddr_get(unsigned int gid, const char *oid,
                       char *value, const char *pppoeserver)
{
    te_pppoe_server *pppoe = pppoe_server_find();

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(pppoeserver);

    ENTRY("Get pppoe server local address");

    sprintf(value, "%s", te_sockaddr_get_ipstr(SA(&pppoe->laddr)));

    EXIT("pppoe server local address: %s", value);

    return 0;
}

/**
 * Get callback for /agent/pppoeserver/raddr node.
 *
 * @param gid           Group identifier.
 * @param oid           Full identifier of the father instance.
 * @param value         Location to the starting remote address value.
 * @param pppoeserver   Dummy parameter due to name of ppppoeserver
 *                      instance is always empty.
 *
 * @return Status code.
 */
static te_errno
pppoe_server_raddr_get(unsigned int gid, const char *oid,
                       char *value, const char *pppoeserver)
{
    te_pppoe_server *pppoe = pppoe_server_find();

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(pppoeserver);

    ENTRY("Get pppoe server starting remote address");

    sprintf(value, "%s", te_sockaddr_get_ipstr(SA(&pppoe->raddr)));

    EXIT("pppoe server starting remote address: %s", value);

    return 0;
}


static rcf_pch_cfg_object node_pppoe_server;

static rcf_pch_cfg_object node_pppoe_server_options =
    { "option", 0, NULL, NULL,
      (rcf_ch_cfg_get)pppoe_server_option_get,
      (rcf_ch_cfg_set)pppoe_server_option_set,
      (rcf_ch_cfg_add)pppoe_server_option_add,
      (rcf_ch_cfg_del)pppoe_server_option_del,
      (rcf_ch_cfg_list)pppoe_server_option_list, NULL, &node_pppoe_server };

RCF_PCH_CFG_NODE_RO(node_pppoe_server_raddr, "raddr",
                    NULL, &node_pppoe_server_options,
                    pppoe_server_raddr_get);

RCF_PCH_CFG_NODE_RO(node_pppoe_server_laddr, "laddr",
                    NULL, &node_pppoe_server_raddr,
                    pppoe_server_laddr_get);

static rcf_pch_cfg_object node_pppoe_server_subnet =
    { "subnet", 0, NULL, &node_pppoe_server_laddr,
      (rcf_ch_cfg_get)pppoe_server_subnet_get,
      (rcf_ch_cfg_set)pppoe_server_subnet_set,
      NULL, NULL, NULL, NULL, &node_pppoe_server };

static rcf_pch_cfg_object node_pppoe_server_ifs =
    { "interface", 0, NULL, &node_pppoe_server_subnet,
      NULL, NULL,
      (rcf_ch_cfg_add)pppoe_server_ifs_add,
      (rcf_ch_cfg_del)pppoe_server_ifs_del,
      (rcf_ch_cfg_list)pppoe_server_ifs_list, NULL, &node_pppoe_server };


static rcf_pch_cfg_object node_pppoe_server =
    { "pppoeserver", 0, &node_pppoe_server_ifs, NULL,
      (rcf_ch_cfg_get)pppoe_server_get,
      (rcf_ch_cfg_set)pppoe_server_set,
      NULL, NULL, NULL,
      (rcf_ch_cfg_commit)pppoe_server_commit, NULL };

/**
 * Grab callback for pppoeserver resource
 *
 * @param name  dummy name of pppoe server
 *
 * @return status code
 */
te_errno
pppoeserver_grab(const char *name)
{
    te_pppoe_server *pppoe = pppoe_server_find();
    const char      *pppoe_paths[] = { PPPOE_SERVER_EXEC };
    te_errno         rc = 0;

    UNUSED(name);

    ENTRY("Grab pppoe server resources");

    /* Find PPPoE server executable */
    rc = find_file(1, pppoe_paths, TRUE);
    if (rc < 0)
    {
        ERROR("Failed to find PPPoE server executable - "
              "PPPoE will not be available");
        EXIT("Failed to grab pppoe server resources");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if ((rc = rcf_pch_add_node("/agent", &node_pppoe_server)) != 0)
        return rc;

    if ((rc = pppoe_server_stop(pppoe)) != 0)
    {
        ERROR("Failed to stop PPPoE server - PPPoE will not be available");
        rcf_pch_del_node(&node_pppoe_server);
        EXIT("Failed to grab pppoe server resources");
        return rc;
    }

    pppoe->started = FALSE;

    EXIT("pppoe server resources have been grabbed");

    return 0;
}

/**
 * Release callback for pppoeserver resource
 *
 * @param name  dummy name of pppoe server
 *
 * @return status code
 */
te_errno
pppoeserver_release(const char *name)
{
    te_pppoe_server *pppoe = pppoe_server_find();
    te_errno      rc;

    UNUSED(name);

    ENTRY("Release pppoe server resources");

    rc = rcf_pch_del_node(&node_pppoe_server);
    if (rc != 0)
    {
        EXIT("Failed to release pppoe server resources");
        return rc;
    }

    if ((rc = pppoe_server_stop(pppoe)) != 0)
    {
        ERROR("Failed to stop pppoe server");
        /* FIXME do we need to return rc here? */
    }

    EXIT("pppoe server resources have been released");

    return 0;
}

