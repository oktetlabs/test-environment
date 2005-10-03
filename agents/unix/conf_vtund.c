/** @file
 * @brief Unix Test Agent
 *
 * VTun (Virtual Tunnel) daemon configuring
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
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
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef WITH_VTUND

#include <sys/queue.h>

#include "conf_daemons.h"


/** Default port for VTund to communicate between each other */
#define VTUND_PORT_DEF              "5000"
/** Default 'persist' session attribute value */
#define VTUND_PERSIST_DEF           "no"
/** Default 'stat' session attribute value */
#define VTUND_STAT_DEF              "no"
/** Default 'tty' session attribute value */
#define VTUN_SESSION_TYPE_DEF       "tty"
/** Default 'device' session attribute value */
#define VTUN_DEVICE_DEF             "tunXX"
/** Default 'proto' session attribute value */
#define VTUN_PROTO_DEF              "tcp"
/** Default compression method */
#define VTUN_COMPRESS_METHOD_DEF    "no"
/** Default compression level */
#define VTUN_COMPRESS_LEVEL_DEF     "9"
/** Default 'encrypt' session attribute value */
#define VTUN_ENCRYPT_DEF            "no"
/** Default 'keepalive' session attribute value */
#define VTUN_KEEPALIVE_DEF          "no"
/** Default speed to client (0 - unlimited) */
#define VTUN_SPEED_TO_CLIENT_DEF    "0"
/** Default speed from client (0 - unlimited) */
#define VTUN_SPEED_FROM_CLIENT_DEF  "0"
/** Default 'multi' session attribute value */
#define VTUN_MULTI_DEF              "no"


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
    
    LIST_ENTRY(vtund_server)    links;

    vtund_server_sessions       sessions;

    char   *port;

} vtund_server;

static LIST_HEAD(vtund_servers, vtund_server) servers;


typedef struct vtund_client {

    LIST_ENTRY(vtund_client)    links;

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

static LIST_HEAD(vtund_clients, vtund_client) clients;


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

    for (p = srv->sessions.tqh_first;
         p != NULL && strcmp(p->name, session) != 0;
         p = p->links.tqe_next);
        
    return p;
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
    p->type              = strdup(VTUN_SESSION_TYPE_DEF);
    p->device            = strdup(VTUN_DEVICE_DEF);
    p->proto             = strdup(VTUN_PROTO_DEF);
    p->compress_method   = strdup(VTUN_COMPRESS_METHOD_DEF);
    p->compress_level    = strdup(VTUN_COMPRESS_LEVEL_DEF);
    p->encrypt           = strdup(VTUN_ENCRYPT_DEF);
    p->keepalive         = strdup(VTUN_KEEPALIVE_DEF);
    p->stat              = strdup(VTUN_STAT_DEF);
    p->speed_to_client   = strdup(VTUN_SPEED_TO_CLIENT_DEF);
    p->speed_from_client = strdup(VTUN_SPEED_FROM_CLIENT_DEF);
    p->multi             = strdup(VTUN_MULTI_DEF);
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
vtund_server_session_list(unsigned int gid, const char *oid, char **list,
                          const char *unused, const char *server_port)
{
    vtund_server           *srv;
    vtund_server_session   *p;

    *buf = '\0';

    srv = vtund_server_find(gid, oid, vtund, server_port);
    if (srv != NULL)
    {
        for (p = srv->sessions.tqh_first;
             p != NULL;
             p = p->links.tqe_next)
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

    for (p = servers.lh_first;
         p != NULL && strcmp(p->port, port) != 0;
         p = p->links.le_next);
        
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
        rc = vtund_server_stop(server);
        if (rc != 0)
            return rc;
    }

    LIST_REMOVE(server, links);
    free(server->port);

    while ((session = server->sessions.tqh_first) != NULL)
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

    if (vtund_server_find(gid, oid, vtund, port) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    p = calloc(1, sizeof(*p));
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    TAILQ_INIT(&p->sessions);

    LIST_INSERT_HEAD(&servers, p, links);

    p->port = strdup(port);
    if (p->port == NULL)
    {
        (void)vtund_server_free(p);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    rc = vtund_server_set(gid, oid, value, vtund, port);
    if (rc != 0)
        LIST_REMOVE(p, links);

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

static int
vtund_server_list(unsigned int gid, const char *oid, char **list)
{
    vtund_server *p;

    UNUSED(gid);
    UNUSED(oid);
    
    *buf = '\0';
    for (p = servers.lh_first; p != NULL; p = p->links.le_next)
    {
        sprintf(buf + strlen(buf), "%u ",  (int)p->port);
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

    for (p = clients.lh_first;
         p != NULL && strcmp(p->name, client) != 0;
         p = p->links.le_next);
        
    return p;
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
    if (client->server == NULL)
    {
        ERROR("Failed to start VTund client '%s' with unspecified "
              "server", client->name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    snprintf(buf, sizeof(buf), "%s -f %s -P %s %s %s",
             vtund_exec, client->port, client->name, client->server);

    client->running = TRUE;
}

static te_errno
vtund_client_stop(vtund_client *client)
{
    client->running = FALSE;
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
        rc = vtund_client_stop(client);
        if (rc != 0)
            return rc;
    }

    LIST_REMOVE(client, links);
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
    vtund_client   *p;
    te_errno        rc;

    if (vtund_client_find(gid, oid, vtund, client) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    p = calloc(1, sizeof(*p));
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    LIST_INSERT_HEAD(&clients, p, links);

    p->name = strdup(client);
    p->port = strdup(VTUND_PORT_DEF);
    p->persist = strdup(VTUND_PERSIST_DEF);
    p->stat = strdup(VTUND_STAT_DEF);
    if (p->name == NULL || p->port == NULL || p->persist == NULL ||
        p->stat == NULL)
    {
        (void)vtund_client_free(p);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    rc = vtund_client_set(gid, oid, value, vtund, client);
    if (rc != 0)
        LIST_REMOVE(p, links);

    return rc;
}

static te_errno
vtund_client_del(unsigned int gid, const char *oid,
                 const char *vtund, const char *client)
{
    vtund_client   *p;
    te_errno        rc;

    p = vtund_client_find(gid, oid, vtund, client);
    if (p == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return vtund_client_free(p);
}

static te_errno
vtund_client_list(unsigned int gid, const char *oid, char **list)
{
    vtund_client *p;

    UNUSED(gid);
    UNUSED(oid);
    
    *buf = '\0';
    for (p = clients.lh_first; p != NULL; p = p->links.le_next)
    {
        sprintf(buf + strlen(buf), "%s ",  p->name);
    }

    return (*list = strdup(buf)) == NULL ?
               TE_RC(TE_TA_UNIX, TE_ENOMEM) : 0;
}


/*
 * VTund server sessions configuration
 */

RCF_PCH_CFG_NODE_RW(node_vtund_server_session_stats, "stat", NULL, NULL,
                    vtund_server_session_attr_get,
                    vtund_server_session_attr_set);

#define VTUND_SERVER_SESSION_ATTR(_name, _next) \
    RCF_PCH_CFG_NODE_RW(node_vtund_server_session_##_name, #_name,   \
                        NULL, &node_vtund_server_session_##_next,    \
                        vtund_server_session_attr_get,               \
                        vtund_server_session_attr_set);

VTUND_SERVER_SESSION_ATTR(multi, stat);
VTUND_SERVER_SESSION_ATTR(speed_from_client, multi);
VTUND_SERVER_SESSION_ATTR(speed_to_client, speed_from_client);
VTUND_SERVER_SESSION_ATTR(keepalive, speed_to_client);
VTUND_SERVER_SESSION_ATTR(encrypt, keepalive);
VTUND_SERVER_SESSION_ATTR(compress_level, encrypt);
VTUND_SERVER_SESSION_ATTR(compress_method, compress_level);
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
      (rcf_ch_cfg_list)vtund_server_list, NULL, NULL };


/*
 * VTund clients configuration
 */

RCF_PCH_CFG_NODE_RW(node_vtund_client_stats, "stat", NULL, NULL,
                    vtund_client_attr_get, vtund_client_attr_set);

#define VTUND_CLIENT_ATTR(_name, _next) \
    RCF_PCH_CFG_NODE_RW(node_vtund_client_##_name, #_name,   \
                        NULL, &node_vtund_client_##_next,    \
                        vtund_client_attr_get,               \
                        vtund_client_attr_set);

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
      (rcf_ch_cfg_list)vtund_client_list, NULL, NULL };

/* 
 * VTund root
 */
RCF_PCH_CFG_NODE_NA(node_vtund, "vtund",
                    &node_vtund_client, NULL);


/**
 * (Re)initialize VTund configuration support.
 *
 * @return status code
 */
void
ds_init_vtund(rcf_pch_cfg_object **last)
{
    LIST_INIT(&clients);
    LIST_INIT(&servers);

    DS_REGISTER(vtund);
}

/** Release all memory allocated for VTund */
void
ds_shutdown_vtund(void)
{
    vtund_server   *server;
    vtund_client   *client;

    while ((server = servers.lh_first) != NULL)
        vtund_server_free(server);
    while ((client = clients.lh_first) != NULL)
        vtund_client_free(client);
}

#endif /* WITH_VTUND */
