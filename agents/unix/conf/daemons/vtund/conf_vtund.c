/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Unix Test Agent
 *
 * VTun (Virtual Tunnel) daemon configuring
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WITH_VTUND

#include "te_queue.h"
#include "conf_daemons_internal.h"


/** Template for VTund configuration file name */
#define VTUND_TMP_FILE_TEMPLATE     "/tmp/vtund.XXXXXX"

/** Default VTund server address */
#define VTUND_SERVER_ADDR_DEF       "0.0.0.0"
/** Default port for VTund to communicate between each other */
#define VTUND_PORT_DEF              "5000"
/** Default 'timeout' session attribute value */
#define VTUND_TIMEOUT_DEF           "60"
/** Default 'persist' session attribute value */
#define VTUND_PERSIST_DEF           "no"
/** Default 'stat' session attribute value */
#define VTUND_STAT_DEF              "0"
/** Default 'tty' session attribute value */
#define VTUND_SESSION_TYPE_DEF      "tty"
/** Default 'device' session attribute value */
#define VTUND_DEVICE_DEF            ""
/** Default 'proto' session attribute value */
#define VTUND_PROTO_DEF             "tcp"
/** Default compression method */
#define VTUND_COMPRESS_METHOD_DEF   "no"
/** Default compression level */
#define VTUND_COMPRESS_LEVEL_DEF    "9"
/** Default 'encrypt' session attribute value */
#define VTUND_ENCRYPT_DEF           "0"
/** Default 'keepalive' session attribute value */
#define VTUND_KEEPALIVE_DEF         "1"
/** Default speed to client (0 - unlimited) */
#define VTUND_SPEED_TO_CLIENT_DEF   "0"
/** Default speed from client (0 - unlimited) */
#define VTUND_SPEED_FROM_CLIENT_DEF "0"
/** Default 'multi' session attribute value */
#define VTUND_MULTI_DEF             "no"


/** VTun daemon executable name */
static const char *vtund_exec = "/usr/sbin/vtund";


typedef struct vtund_server_session {

    TAILQ_ENTRY(vtund_server_session)   links;

    char   *name;
    char   *password;
    char   *type;
    char   *device;
    char   *proto;
    char   *compress_method;
    char   *compress_level;
    char   *encrypt;
    char   *keepalive;
    char   *stat;
    char   *speed_to_client;
    char   *speed_from_client;
    char   *multi;

} vtund_server_session;

typedef TAILQ_HEAD(vtund_server_sessions, vtund_server_session)
    vtund_server_sessions;


typedef struct vtund_server {

    SLIST_ENTRY(vtund_server)   links;

    vtund_server_sessions       sessions;

    char       *cfg_file;
    char       *port;

    te_bool     running;

} vtund_server;

static SLIST_HEAD(vtund_servers, vtund_server) servers;


typedef struct vtund_client {

    SLIST_ENTRY(vtund_client)   links;

    char   *cfg_file;
    char   *name;
    char   *server;
    char   *port;
    char   *password;
    char   *device;
    char   *timeout;
    char   *persist;
    char   *stat;

    te_bool running;

} vtund_client;

static SLIST_HEAD(vtund_clients, vtund_client) clients;


/** Auxiliary buffer */
static char buf[2048];


static vtund_server *vtund_server_find(unsigned int gid, const char *oid,
                                       const char *vtund, const char *port);


/*
 * VTund server sessions support routines
 */

static vtund_server_session *
vtund_server_session_find(unsigned int gid, const char *oid,
                          const char *vtund, const char *server_port,
                          const char *session, vtund_server **server)
{
    vtund_server           *srv;
    vtund_server_session   *p;

    srv = vtund_server_find(gid, oid, vtund, server_port);
    if (srv == NULL)
        return NULL;

    if (server != NULL)
        *server = srv;

    for (p = TAILQ_FIRST(&srv->sessions);
         p != NULL && strcmp(p->name, session) != 0;
         p = TAILQ_NEXT(p, links));

    return p;
}

static te_errno
vtund_server_session_interface_get(unsigned int gid, const char *oid,
                                   char *value, const char *vtund,
                                   const char *server_port,
                                   const char *session)
{
    vtund_server           *server;
    vtund_server_session   *p;

    p = vtund_server_session_find(gid, oid, vtund, server_port, session,
                                  &server);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    /* Ugly hard code */
    if (server->running)
        snprintf(value, RCF_MAX_VAL, "ppp0");
    else
        *value = '\0';

    return 0;
}

static te_errno
vtund_server_session_attr_get(unsigned int gid, const char *oid,
                              char *value, const char *vtund,
                              const char *server_port,
                              const char *session)
{
    vtund_server_session   *p;
    cfg_oid                *coid;
    const char             *attr;

    p = vtund_server_session_find(gid, oid, vtund, server_port, session,
                                  NULL);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    coid = cfg_convert_oid_str(oid);
    if (coid == NULL)
        return TE_RC(TE_TA_UNIX, TE_EFAULT);

    attr = ((cfg_inst_subid *)(coid->ids))[coid->len - 1].subid;

#define VTUND_SERVER_SESSION_ATTR_GET(_attr) \
    (strcmp(attr, #_attr) == 0)                         \
    {                                                   \
        snprintf(value, RCF_MAX_VAL, "%s", p->_attr);   \
    }

    if VTUND_SERVER_SESSION_ATTR_GET(type)
    else if VTUND_SERVER_SESSION_ATTR_GET(password)
    else if VTUND_SERVER_SESSION_ATTR_GET(device)
    else if VTUND_SERVER_SESSION_ATTR_GET(proto)
    else if VTUND_SERVER_SESSION_ATTR_GET(compress_method)
    else if VTUND_SERVER_SESSION_ATTR_GET(compress_level)
    else if VTUND_SERVER_SESSION_ATTR_GET(encrypt)
    else if VTUND_SERVER_SESSION_ATTR_GET(keepalive)
    else if VTUND_SERVER_SESSION_ATTR_GET(stat)
    else if VTUND_SERVER_SESSION_ATTR_GET(speed_to_client)
    else if VTUND_SERVER_SESSION_ATTR_GET(speed_from_client)
    else if VTUND_SERVER_SESSION_ATTR_GET(multi)
    else
    {
        ERROR("Unknown VTund server session attribute '%s'", attr);
        cfg_free_oid(coid);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#undef VTUND_SERVER_SESSION_ATTR_GET

    cfg_free_oid(coid);

    return 0;
}

static te_errno
vtund_server_session_attr_set(unsigned int gid, const char *oid,
                              const char *value, const char *vtund,
                              const char *server_port,
                              const char *session)
{
    vtund_server_session   *p;
    cfg_oid                *coid;
    const char             *attr;
    char                   *dup;

    p = vtund_server_session_find(gid, oid, vtund, server_port, session,
                                  NULL);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    coid = cfg_convert_oid_str(oid);
    if (coid == NULL)
        return TE_RC(TE_TA_UNIX, TE_EFAULT);

    attr = ((cfg_inst_subid *)(coid->ids))[coid->len - 1].subid;

    dup = strdup(value);
    if (dup == NULL)
    {
        cfg_free_oid(coid);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

#define VTUND_SERVER_SESSION_ATTR_SET(_attr) \
    (strcmp(attr, #_attr) == 0) \
    {                           \
        free(p->_attr);         \
        p->_attr = dup;         \
    }

    if VTUND_SERVER_SESSION_ATTR_SET(type)
    else if VTUND_SERVER_SESSION_ATTR_SET(password)
    else if VTUND_SERVER_SESSION_ATTR_SET(device)
    else if VTUND_SERVER_SESSION_ATTR_SET(proto)
    else if VTUND_SERVER_SESSION_ATTR_SET(compress_method)
    else if VTUND_SERVER_SESSION_ATTR_SET(compress_level)
    else if VTUND_SERVER_SESSION_ATTR_SET(encrypt)
    else if VTUND_SERVER_SESSION_ATTR_SET(keepalive)
    else if VTUND_SERVER_SESSION_ATTR_SET(stat)
    else if VTUND_SERVER_SESSION_ATTR_SET(speed_to_client)
    else if VTUND_SERVER_SESSION_ATTR_SET(speed_from_client)
    else if VTUND_SERVER_SESSION_ATTR_SET(multi)
    else
    {
        ERROR("Unknown VTund server session attribute '%s'", attr);
        cfg_free_oid(coid);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#undef VTUND_SERVER_SESSION_ATTR_SET

    cfg_free_oid(coid);

    return 0;
}

static void
vtund_server_session_free(vtund_server         *server,
                          vtund_server_session *session)
{
    TAILQ_REMOVE(&server->sessions, session, links);
    free(session->name);
    free(session->password);
    free(session->type);
    free(session->device);
    free(session->proto);
    free(session->compress_method);
    free(session->compress_level);
    free(session->encrypt);
    free(session->keepalive);
    free(session->stat);
    free(session->speed_to_client);
    free(session->speed_from_client);
    free(session->multi);
    free(session);
}

static te_errno
vtund_server_session_add(unsigned int gid, const char *oid, const char *value,
                         const char *vtund, const char *server_port,
                         const char *session)
{
    vtund_server           *server;
    vtund_server_session   *p;

    UNUSED(value);

    if (vtund_server_session_find(gid, oid, vtund, server_port, session,
                                  &server) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (server->running)
    {
        ERROR("Unable to add session '%s' to running VTund server '%s'",
              session, server_port);
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }

    p = calloc(1, sizeof(*p));
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    TAILQ_INSERT_TAIL(&server->sessions, p, links);

    p->name              = strdup(session);
    p->type              = strdup(VTUND_SESSION_TYPE_DEF);
    p->device            = strdup(VTUND_DEVICE_DEF);
    p->proto             = strdup(VTUND_PROTO_DEF);
    p->compress_method   = strdup(VTUND_COMPRESS_METHOD_DEF);
    p->compress_level    = strdup(VTUND_COMPRESS_LEVEL_DEF);
    p->encrypt           = strdup(VTUND_ENCRYPT_DEF);
    p->keepalive         = strdup(VTUND_KEEPALIVE_DEF);
    p->stat              = strdup(VTUND_STAT_DEF);
    p->speed_to_client   = strdup(VTUND_SPEED_TO_CLIENT_DEF);
    p->speed_from_client = strdup(VTUND_SPEED_FROM_CLIENT_DEF);
    p->multi             = strdup(VTUND_MULTI_DEF);
    if (p->name == NULL || p->type == NULL || p->device == NULL ||
        p->proto == NULL || p->compress_method == NULL ||
        p->compress_level == NULL || p->encrypt == NULL ||
        p->keepalive == NULL || p->stat == NULL || p->multi == NULL ||
        p->speed_to_client == NULL || p->speed_from_client == NULL)
    {
        vtund_server_session_free(server, p);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    return 0;
}

static te_errno
vtund_server_session_del(unsigned int gid, const char *oid,
                         const char *vtund, const char *server_port,
                         const char *session)
{
    vtund_server           *server;
    vtund_server_session   *p;

    p = vtund_server_session_find(gid, oid, vtund, server_port, session,
                                  &server);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (server->running)
    {
        ERROR("Unable to delete session '%s' from running VTund "
              "server '%s'", session, server_port);
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }

    vtund_server_session_free(server, p);

    return 0;
}

static te_errno
vtund_server_session_list(unsigned int gid, const char *oid,
                          const char *sub_id, char **list,
                          const char *vtund, const char *server_port)
{
    vtund_server           *srv;
    vtund_server_session   *p;

    UNUSED(sub_id);

    *buf = '\0';

    srv = vtund_server_find(gid, oid, vtund, server_port);
    if (srv != NULL)
    {
        TAILQ_FOREACH(p, &srv->sessions, links)
        {
            sprintf(buf + strlen(buf), "%s ",  p->name);
        }
    }

    return (*list = strdup(buf)) == NULL ?
               TE_RC(TE_TA_UNIX, TE_ENOMEM) : 0;
}


/*
 * VTund servers support routines
 */

static vtund_server *
vtund_server_find(unsigned int gid, const char *oid,
                  const char *vtund, const char *port)
{
    vtund_server   *p;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(vtund);

    for (p = SLIST_FIRST(&servers);
         p != NULL && strcmp(p->port, port) != 0;
         p = SLIST_NEXT(p, links));

    return p;
}

static te_errno
vtund_server_get(unsigned int gid, const char *oid, char *value,
                 const char *vtund, const char *server_port)
{
    vtund_server   *p;

    p = vtund_server_find(gid, oid, vtund, server_port);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%u", p->running);

    return 0;
}

static te_errno
vtund_server_start(vtund_server *server)
{
    static uint8_t  id = 0;

    FILE                   *f;
    vtund_server_session   *p;
    uint8_t                 id1;
    uint8_t                 id2;

    f = fopen(server->cfg_file, "w");
    if (f == NULL)
    {
        int err = errno;

        ERROR("Failed to open VTund server configuration file '%s'",
              server->cfg_file);
        return TE_OS_RC(TE_TA_UNIX, err);
    }

    TAILQ_FOREACH(p, &server->sessions, links)
    {
        const char * const vtund_server_session_fmt =
            "\n"
            "%s {\n"
            "  passwd %s;\n"
            "  type %s;\n"
            "  proto %s;\n"
            "%s%s%s"
            "  compress %s%s%s;\n"
            "  encrypt %s;\n"
            "  keepalive %s;\n"
            "  stat %s;\n"
            "  speed %s:%s;\n"
            "  multi %s;\n"
            "  up {\n"
            "    ppp \"10.0.0.%d:10.0.0.%d noauth nomagic nodeflate nobsdcomp silent mtu 10000 mru 10000\";\n"
            "  };\n"
            "  down {\n"
            "  };\n"
            "}\n";

        id1 = ++id; id2 = ++id;
        fprintf(f, vtund_server_session_fmt,
                p->name, (p->password != NULL) ? p->password : p->name,
                p->type, p->proto,
                strlen(p->device) != 0 ? "  device " : "",
                strlen(p->device) != 0 ? p->device : "",
                strlen(p->device) != 0 ? ";\n" : "",
                p->compress_method,
                (strcmp(p->compress_method, "no") == 0) ? "" : ":",
                (strcmp(p->compress_method, "no") == 0) ? "" :
                    p->compress_level,
                (strcmp(p->encrypt, "0") == 0) ? "no" : "yes",
                (strcmp(p->keepalive, "0") == 0) ? "no" : "yes",
                (strcmp(p->stat, "0") == 0) ? "no" : "yes",
                p->speed_to_client, p->speed_from_client, p->multi,
                id1, id2);
    }

    fclose(f);

    snprintf(buf, sizeof(buf), "%s -s -P %s -f %s",
             vtund_exec, server->port, server->cfg_file);

    ta_system(buf);

    server->running = TRUE;

    return 0;
}

static te_errno
vtund_server_stop(vtund_server *server)
{
    FILE   *f;
    char    line[128];
    pid_t   pid;
    pid_t   cmd_pid;
    int     rc = 0;

    snprintf(buf, sizeof(buf),
             PS_ALL_PID_ARGS " | grep 'vtund\\[s\\]' | grep '%s' | "
             "grep -v grep",
             server->port);

    if ((rc = ta_popen_r(buf, &cmd_pid, &f)) < 0)
        return rc;

    if (fgets(line, sizeof(line), f) == NULL)
    {
        pclose(f);
        ERROR("Failed to find VTund server '%s' PID", server->port);
        return TE_RC(TE_TA_UNIX, TE_EFAULT);
    }

    if ((rc = ta_pclose_r(cmd_pid, f)) < 0)
        return rc;

    pid = atoi(line);
    if (kill(pid, SIGTERM) != 0)
    {
        te_errno err = TE_OS_RC(TE_TA_UNIX, errno);

        ERROR("Failed to send SIGTERM to the process with PID %u: %r",
              (unsigned int)pid, err);
        return err;
    }

    server->running = FALSE;

    return 0;
}

static te_errno
vtund_server_set(unsigned int gid, const char *oid, const char *value,
                 const char *vtund, const char *server_port)
{
    vtund_server   *p;

    p = vtund_server_find(gid, oid, vtund, server_port);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (strcmp(value, "0") == 0)
        return (p->running) ? vtund_server_stop(p) : 0;
    else if (strcmp(value, "1") == 0)
        return (p->running) ? 0 : vtund_server_start(p);
    else
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
}


static te_errno
vtund_server_free(vtund_server *server)
{
    vtund_server_session   *session;

    if (server->running)
    {
        te_errno rc = vtund_server_stop(server);

        if (rc != 0)
            return rc;
    }

    SLIST_REMOVE(&servers, server, vtund_server, links);

    if (server->cfg_file != NULL)
        unlink(server->cfg_file);

    free(server->cfg_file);
    free(server->port);

    while ((session = TAILQ_FIRST(&server->sessions)) != NULL)
        vtund_server_session_free(server, session);

    free(server);

    return 0;
}

static te_errno
vtund_server_add(unsigned int gid, const char *oid, const char *value,
                 const char *vtund, const char *port)
{
    vtund_server   *p;
    te_errno        rc;
    int             fd;

    if (vtund_server_find(gid, oid, vtund, port) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    p = calloc(1, sizeof(*p));
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    TAILQ_INIT(&p->sessions);

    SLIST_INSERT_HEAD(&servers, p, links);

    p->cfg_file = strdup(VTUND_TMP_FILE_TEMPLATE);
    p->port = strdup(port);
    if (p->cfg_file == NULL || p->port == NULL)
    {
        (void)vtund_server_free(p);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    fd = mkstemp(p->cfg_file);
    if (fd < 0)
    {
        int err = errno;

        (void)vtund_server_free(p);
        return TE_OS_RC(TE_TA_UNIX, err);
    }
    (void)close(fd);

    rc = vtund_server_set(gid, oid, value, vtund, port);
    if (rc != 0)
        SLIST_REMOVE(&servers, p, vtund_server, links);

    return rc;
}

static te_errno
vtund_server_del(unsigned int gid, const char *oid,
                 const char *vtund, const char *port)
{
    vtund_server   *p;

    p = vtund_server_find(gid, oid, vtund, port);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return vtund_server_free(p);
}

static te_errno
vtund_server_list(unsigned int gid, const char *oid,
                  const char *sub_id, char **list)
{
    vtund_server *p;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    *buf = '\0';
    SLIST_FOREACH(p, &servers, links)
    {
        sprintf(buf + strlen(buf), "%s ",  p->port);
    }

    return (*list = strdup(buf)) == NULL ?
               TE_RC(TE_TA_UNIX, TE_ENOMEM) : 0;
}


/*
 * VTund client support routines
 */

static vtund_client *
vtund_client_find(unsigned int gid, const char *oid,
                  const char *vtund, const char *client)
{
    vtund_client   *p;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(vtund);

    for (p = SLIST_FIRST(&clients);
         p != NULL && strcmp(p->name, client) != 0;
         p = SLIST_NEXT(p, links));

    return p;
}

static te_errno
vtund_client_interface_get(unsigned int gid, const char *oid, char *value,
                           const char *vtund, const char *client)
{
    vtund_client   *p;

    p = vtund_client_find(gid, oid, vtund, client);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    /* Ugly hard code */
    if (p->running)
        snprintf(value, RCF_MAX_VAL, "ppp0");
    else
        *value = '\0';

    return 0;
}

static te_errno
vtund_client_attr_get(unsigned int gid, const char *oid, char *value,
                      const char *vtund, const char *client)
{
    vtund_client   *p;
    cfg_oid        *coid;
    const char     *attr;

    p = vtund_client_find(gid, oid, vtund, client);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    coid = cfg_convert_oid_str(oid);
    if (coid == NULL)
        return TE_RC(TE_TA_UNIX, TE_EFAULT);

    attr = ((cfg_inst_subid *)(coid->ids))[coid->len - 1].subid;

#define VTUND_CLIENT_ATTR_GET(_attr) \
    (strcmp(attr, #_attr) == 0)                         \
    {                                                   \
        snprintf(value, RCF_MAX_VAL, "%s", p->_attr);   \
    }

    if VTUND_CLIENT_ATTR_GET(server)
    else if VTUND_CLIENT_ATTR_GET(password)
    else if VTUND_CLIENT_ATTR_GET(port)
    else if VTUND_CLIENT_ATTR_GET(device)
    else if VTUND_CLIENT_ATTR_GET(timeout)
    else if VTUND_CLIENT_ATTR_GET(persist)
    else if VTUND_CLIENT_ATTR_GET(stat)
    else
    {
        ERROR("Unknown VTund client attribute '%s'", attr);
        cfg_free_oid(coid);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#undef VTUND_CLIENT_ATTR_SET

    cfg_free_oid(coid);

    return 0;
}

static te_errno
vtund_client_attr_set(unsigned int gid, const char *oid, const char *value,
                      const char *vtund, const char *client)
{
    vtund_client   *p;
    cfg_oid        *coid;
    const char     *attr;
    char           *dup;


    p = vtund_client_find(gid, oid, vtund, client);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (p->running)
    {
        ERROR("Failed to set VTund client '%s' attribute, since it is "
              "running", client);
        return TE_RC(TE_TA_UNIX, TE_EPERM);
    }

    coid = cfg_convert_oid_str(oid);
    if (coid == NULL)
        return TE_RC(TE_TA_UNIX, TE_EFAULT);

    attr = ((cfg_inst_subid *)(coid->ids))[coid->len - 1].subid;

    dup = strdup(value);
    if (dup == NULL)
    {
        cfg_free_oid(coid);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

#define VTUND_CLIENT_ATTR_SET(_attr) \
    (strcmp(attr, #_attr) == 0) \
    {                           \
        free(p->_attr);         \
        p->_attr = dup;         \
    }

    if VTUND_CLIENT_ATTR_SET(server)
    else if VTUND_CLIENT_ATTR_SET(password)
    else if VTUND_CLIENT_ATTR_SET(port)
    else if VTUND_CLIENT_ATTR_SET(device)
    else if VTUND_CLIENT_ATTR_SET(timeout)
    else if VTUND_CLIENT_ATTR_SET(persist)
    else if VTUND_CLIENT_ATTR_SET(stat)
    else
    {
        ERROR("Unknown VTund client attribute '%s'", attr);
        free(dup);
        cfg_free_oid(coid);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

#undef VTUND_CLIENT_ATTR_SET

    cfg_free_oid(coid);

    return 0;
}

static te_errno
vtund_client_start(vtund_client *client)
{
    const char * const vtund_client_fmt =
        "%s {\n"
        "  passwd %s;\n"
        "%s%s%s"
        "  timeout %s;\n"
        "  persist %s;\n"
        "  stat %s;\n"
        "  up {\n"
        "    ppp \"noipdefault noauth nodeflate nobsdcomp mtu 10000 mru 10000\";\n"
        "  };\n"
        "  down {\n"
        "  };\n"
        "}\n";

    FILE *f;


    if (strcmp(client->server, VTUND_SERVER_ADDR_DEF) == 0)
    {
        ERROR("Failed to start VTund client '%s' with unspecified "
              "server", client->name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    f = fopen(client->cfg_file, "w");
    if (f == NULL)
    {
        int err = errno;

        ERROR("Failed to open VTund client configuration file '%s'",
              client->cfg_file);
        return TE_OS_RC(TE_TA_UNIX, err);
    }

    fprintf(f, vtund_client_fmt,
            client->name,
            (client->password != NULL) ? client->password : client->name,
            strlen(client->device) != 0 ? "  device " : "",
            strlen(client->device) != 0 ? client->device : "",
            strlen(client->device) != 0 ? ";\n" : "",
            client->timeout, client->persist,
            (strcmp(client->stat, "0") == 0) ? "no" : "yes");

    (void)fclose(f);

    snprintf(buf, sizeof(buf), "%s -P %s -f %s %s %s",
             vtund_exec, client->port, client->cfg_file, client->name,
             client->server);
    ta_system(buf);

    client->running = TRUE;

    return 0;
}

static te_errno
vtund_client_stop(vtund_client *client)
{
    FILE   *f;
    char    line[128];
    char   *found;
    pid_t   pid;
    pid_t   cmd_pid;
    int     rc = 0;

    snprintf(buf, sizeof(buf),
             PS_ALL_PID_ARGS " | grep 'vtund\\[c\\]' | grep '%s' | "
             "grep -v grep",
             client->name);

    if ((rc = ta_popen_r(buf, &cmd_pid, &f)) < 0)
        return rc;

    found = fgets(line, sizeof(line), f);

    if ((rc = ta_pclose_r(cmd_pid, f)) < 0)
        return rc;

    if (found == NULL)
    {
        WARN("Failed to find VTund client '%s' PID, assuming that "
             "client has stopped", client->name);
    }
    else
    {
        pid = atoi(line);
        if (kill(pid, SIGTERM) != 0)
        {
            te_errno err = TE_OS_RC(TE_TA_UNIX, errno);

            ERROR("Failed to send SIGTERM to the process with PID %u: %r",
                  (unsigned int)pid, err);
            return err;
        }
    }

    client->running = FALSE;

    return 0;
}

static te_errno
vtund_client_set(unsigned int gid, const char *oid, const char *value,
                 const char *vtund, const char *client)
{
    vtund_client   *p;

    p = vtund_client_find(gid, oid, vtund, client);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (strcmp(value, "0") == 0)
        return (p->running) ? vtund_client_stop(p) : 0;
    else if (strcmp(value, "1") == 0)
        return (p->running) ? 0 : vtund_client_start(p);
    else
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
}

static te_errno
vtund_client_get(unsigned int gid, const char *oid, char *value,
                 const char *vtund, const char *client)
{
    vtund_client   *p;

    p = vtund_client_find(gid, oid, vtund, client);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    snprintf(value, RCF_MAX_VAL, "%u", p->running);

    return 0;
}

static te_errno
vtund_client_free(vtund_client *client)
{
    if (client->running)
    {
        te_errno rc = vtund_client_stop(client);

        if (rc != 0)
            return rc;
    }

    SLIST_REMOVE(&clients, client, vtund_client, links);

    if (client->cfg_file != NULL)
        unlink(client->cfg_file);

    free(client->cfg_file);
    free(client->name);
    free(client->server);
    free(client->port);
    free(client->password);
    free(client->device);
    free(client->timeout);
    free(client->persist);
    free(client->stat);
    free(client);

    return 0;
}

static te_errno
vtund_client_add(unsigned int gid, const char *oid, const char *value,
                 const char *vtund, const char *client)
{
    int             fd;
    vtund_client   *p;
    te_errno        rc;

    if (vtund_client_find(gid, oid, vtund, client) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    p = calloc(1, sizeof(*p));
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    SLIST_INSERT_HEAD(&clients, p, links);

    p->cfg_file = strdup(VTUND_TMP_FILE_TEMPLATE);
    p->name     = strdup(client);
    p->server   = strdup(VTUND_SERVER_ADDR_DEF);
    p->port     = strdup(VTUND_PORT_DEF);
    p->device   = strdup(VTUND_DEVICE_DEF);
    p->timeout  = strdup(VTUND_TIMEOUT_DEF);
    p->persist  = strdup(VTUND_PERSIST_DEF);
    p->stat     = strdup(VTUND_STAT_DEF);
    if (p->cfg_file == NULL || p->name == NULL || p->port == NULL ||
        p->device == NULL || p->timeout == NULL || p->persist == NULL ||
        p->stat == NULL)
    {
        (void)vtund_client_free(p);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    fd = mkstemp(p->cfg_file);
    if (fd < 0)
    {
        int err = errno;

        (void)vtund_client_free(p);
        return TE_OS_RC(TE_TA_UNIX, err);
    }
    (void)close(fd);

    rc = vtund_client_set(gid, oid, value, vtund, client);
    if (rc != 0)
        SLIST_REMOVE(&clients, p, vtund_client, links);

    return rc;
}

static te_errno
vtund_client_del(unsigned int gid, const char *oid,
                 const char *vtund, const char *client)
{
    vtund_client   *p;

    p = vtund_client_find(gid, oid, vtund, client);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return vtund_client_free(p);
}

static te_errno
vtund_client_list(unsigned int gid, const char *oid,
                  const char *sub_id, char **list)
{
    vtund_client *p;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    *buf = '\0';
    SLIST_FOREACH(p, &clients, links)
    {
        sprintf(buf + strlen(buf), "%s ",  p->name);
    }

    return (*list = strdup(buf)) == NULL ?
               TE_RC(TE_TA_UNIX, TE_ENOMEM) : 0;
}


/*
 * VTund server sessions configuration
 */

RCF_PCH_CFG_NODE_RO(node_vtund_server_session_interface, "interface",
                    NULL, NULL, vtund_server_session_interface_get);


#define VTUND_SERVER_SESSION_ATTR(_name, _next) \
    RCF_PCH_CFG_NODE_RW(node_vtund_server_session_##_name, #_name,   \
                        NULL, &node_vtund_server_session_##_next,    \
                        vtund_server_session_attr_get,               \
                        vtund_server_session_attr_set);

VTUND_SERVER_SESSION_ATTR(stat, interface);
VTUND_SERVER_SESSION_ATTR(multi, stat);
VTUND_SERVER_SESSION_ATTR(speed_from_client, multi);
VTUND_SERVER_SESSION_ATTR(speed_to_client, speed_from_client);
VTUND_SERVER_SESSION_ATTR(keepalive, speed_to_client);
VTUND_SERVER_SESSION_ATTR(encrypt, keepalive);
VTUND_SERVER_SESSION_ATTR(compress_level, encrypt);
VTUND_SERVER_SESSION_ATTR(compress_method, compress_level);
VTUND_SERVER_SESSION_ATTR(timeout, compress_method);
VTUND_SERVER_SESSION_ATTR(proto, timeout);
VTUND_SERVER_SESSION_ATTR(device, proto);
VTUND_SERVER_SESSION_ATTR(type, device);
VTUND_SERVER_SESSION_ATTR(password, type);

#undef VTUND_SERVER_SESSION_ATTR

RCF_PCH_CFG_NODE_COLLECTION(node_vtund_server_session, "session",
                            &node_vtund_server_session_password, NULL,
                            vtund_server_session_add,
                            vtund_server_session_del,
                            vtund_server_session_list, NULL);

static rcf_pch_cfg_object node_vtund_server =
    { "server", 0, &node_vtund_server_session, NULL,
      (rcf_ch_cfg_get)vtund_server_get,
      (rcf_ch_cfg_set)vtund_server_set,
      (rcf_ch_cfg_add)vtund_server_add,
      (rcf_ch_cfg_del)vtund_server_del,
      (rcf_ch_cfg_list)vtund_server_list, NULL, NULL, NULL };


/*
 * VTund clients configuration
 */

RCF_PCH_CFG_NODE_RO(node_vtund_client_interface, "interface",
                    NULL, NULL, vtund_client_interface_get);

#define VTUND_CLIENT_ATTR(_name, _next) \
    RCF_PCH_CFG_NODE_RW(node_vtund_client_##_name, #_name,   \
                        NULL, &node_vtund_client_##_next,    \
                        vtund_client_attr_get,               \
                        vtund_client_attr_set);

VTUND_CLIENT_ATTR(stat, interface);
VTUND_CLIENT_ATTR(persist, stat);
VTUND_CLIENT_ATTR(timeout, persist);
VTUND_CLIENT_ATTR(device, timeout);
VTUND_CLIENT_ATTR(password, device);
VTUND_CLIENT_ATTR(port, password);
VTUND_CLIENT_ATTR(server, port);

#undef VTUND_SESSION_ATTR

static rcf_pch_cfg_object node_vtund_client =
    { "client", 0, &node_vtund_client_server, &node_vtund_server,
      (rcf_ch_cfg_get)vtund_client_get,
      (rcf_ch_cfg_set)vtund_client_set,
      (rcf_ch_cfg_add)vtund_client_add,
      (rcf_ch_cfg_del)vtund_client_del,
      (rcf_ch_cfg_list)vtund_client_list, NULL, NULL, NULL };

/*
 * VTund root
 */
RCF_PCH_CFG_NODE_NA(node_ds_vtund, "vtund",
                    &node_vtund_client, NULL);


te_errno
vtund_grab(const char *name)
{
    UNUSED(name);

    SLIST_INIT(&clients);
    SLIST_INIT(&servers);

    return rcf_pch_add_node("/agent", &node_ds_vtund);
}

te_errno
vtund_release(const char *name)
{
    vtund_server *server;
    vtund_client *client;

    UNUSED(name);

    while ((server = SLIST_FIRST(&servers)) != NULL)
        vtund_server_free(server);
    while ((client = SLIST_FIRST(&clients)) != NULL)
        vtund_client_free(client);

    rcf_pch_del_node(&node_ds_vtund);

    return 0;
}

#endif /* WITH_VTUND */
