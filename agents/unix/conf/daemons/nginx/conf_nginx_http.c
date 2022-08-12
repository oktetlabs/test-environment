/** @file
 * @brief Unix Test Agent
 *
 * Nginx HTTP servers support
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 *
 * @author Marina Maslova <Marina.Maslova@oktetlabs.ru>
 */

#define TE_LGR_USER     "Unix Conf Nginx HTTP"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "conf_daemons_internal.h"
#include "conf_nginx.h"
#include "rcf_pch.h"
#include "te_alloc.h"
#include "te_str.h"
#include "te_string.h"

/** Default HTTP server mime type */
#define NGINX_HTTP_SRV_MIME_TYPE_DEF            "text/plain"

/** Format string of path to access log file on TA */
#define NGINX_HTTP_SRV_ACCESS_LOG_PATH_FMT      "/tmp/nginx_%s_%s_access.log"

/** Default HTTP server keepalive timeout (in seconds) */
#define NGINX_HTTP_SRV_KEEPALIVE_TIMEOUT_DEF    (75)
/** Default HTTP server keepalive requests number */
#define NGINX_HTTP_SRV_KEEPALIVE_REQS_DEF       (100)
/** Default HTTP server send timeout (in seconds) */
#define NGINX_HTTP_SRV_SEND_TIMEOUT_DEF         (60)

/** Default HTTP server file cache maximum number of entries */
#define NGINX_HTTP_FILE_CACHE_MAX_NUM_DEF       (1000)
/** Default HTTP server file cache inactive timeout (in seconds) */
#define NGINX_HTTP_FILE_CACHE_INACT_TIMEOUT_DEF (60)
/** Default HTTP server file cache validation timeout (in seconds) */
#define NGINX_HTTP_FILE_CACHE_VALID_TIMEOUT_DEF (60)

/** Default HTTP server client body timeout (in seconds) */
#define NGINX_HTTP_CLI_BODY_TIMEOUT_DEF         (60)
/** Default HTTP server client body maximum size */
#define NGINX_HTTP_CLI_BODY_MAX_SIZE_DEF        (1024)
/** Default HTTP server client header timeout (in seconds) */
#define NGINX_HTTP_CLI_HDR_TIMEOUT_DEF          (60)
/** Default HTTP server client header buffer maximum size */
#define NGINX_HTTP_CLI_HDR_BUF_SIZE_DEF         (1)
/** Default HTTP server client large header buffers number */
#define NGINX_HTTP_CLI_LRG_HDR_BUF_NUM_DEF      (4)
/** Default HTTP server client large header buffer maximum size */
#define NGINX_HTTP_CLI_LRG_HDR_BUF_SIZE_DEF     (8)

/** Default HTTP server proxy connect timeout (in seconds) */
#define NGINX_HTTP_PROXY_CONN_TIMEOUT_DEF       (60)

/** Default upstream group server weight */
#define NGINX_HTTP_US_SRV_WEIGHT_DEF            (1)

/* Forward declarations */
static rcf_pch_cfg_object node_nginx_http;
static void nginx_http_us_server_free(nginx_http_us_server *srv);
static void nginx_http_listen_entry_free(nginx_http_listen_entry *entry);
static void nginx_http_loc_free(nginx_http_loc *loc);
static void nginx_http_loc_proxy_hdr_free(nginx_http_header *hdr);

/* See description in conf_nginx.h */
void
nginx_http_server_free(nginx_http_server *srv)
{
    nginx_http_listen_entry  *entry;
    nginx_http_listen_entry  *entry_tmp;
    nginx_http_loc           *loc;
    nginx_http_loc           *loc_tmp;

    LIST_FOREACH_SAFE(entry, &srv->listen_entries, links, entry_tmp)
    {
        LIST_REMOVE(entry, links);
        nginx_http_listen_entry_free(entry);
    }

    LIST_FOREACH_SAFE(loc, &srv->locations, links, loc_tmp)
    {
        LIST_REMOVE(loc, links);
        nginx_http_loc_free(loc);
    }

    free(srv->access_log_path);
    free(srv->ssl_name);
    free(srv->hostname);
    free(srv->mime_type_default);
    free(srv->name);
    free(srv);
}

/**
 * Find nginx instance server by name.
 *
 * @param inst        Nginx instance
 * @param srv_name    Server name
 *
 * @return Server pointer, or @c NULL if server is not found.
 */
static nginx_http_server *
nginx_inst_find_server(const nginx_inst *inst, const char *srv_name)
{
    nginx_http_server *srv;

    LIST_FOREACH(srv, &inst->http_servers, links)
    {
        if (strcmp(srv->name, srv_name) == 0)
            return srv;
    }

    return NULL;
}

/**
 * Find server by name and name of nginx instance.
 *
 * @param inst_name   Nginx instance name
 * @param srv_name    Server name
 *
 * @return Server pointer, or @c NULL if server is not found.
 */
static nginx_http_server *
nginx_http_server_find(const char *inst_name, const char *srv_name)
{
    nginx_inst   *inst;

    if ((inst = nginx_inst_find(inst_name)) == NULL)
        return NULL;

    return nginx_inst_find_server(inst, srv_name);
}

/* See description in conf_nginx.h */
void
nginx_http_upstream_free(nginx_http_upstream *us)
{
    nginx_http_us_server     *srv;
    nginx_http_us_server     *srv_tmp;

    LIST_FOREACH_SAFE(srv, &us->servers, links, srv_tmp)
    {
        LIST_REMOVE(srv, links);
        nginx_http_us_server_free(srv);
    }

    free(us->name);
    free(us);
}

/**
 * Find nginx instance upstream group by name.
 *
 * @param inst        Nginx instance
 * @param us_name     Upstream group name
 *
 * @return Upstream group pointer, or @c NULL if group is not found.
 */
static nginx_http_upstream *
nginx_inst_find_http_upstream(const nginx_inst *inst, const char *us_name)
{
    nginx_http_upstream *us;

    LIST_FOREACH(us, &inst->http_upstreams, links)
    {
        if (strcmp(us->name, us_name) == 0)
            return us;
    }

    return NULL;
}

/**
 * Find upstream group by its name and name of nginx instance.
 *
 * @param inst_name   Nginx instance name
 * @param us_name     Upstream group name
 *
 * @return Upstream group pointer, or @c NULL if group is not found.
 */
static nginx_http_upstream *
nginx_http_upstream_find(const char *inst_name, const char *us_name)
{
    nginx_inst   *inst;

    if ((inst = nginx_inst_find(inst_name)) == NULL)
        return NULL;

    return nginx_inst_find_http_upstream(inst, us_name);
}

/** Free all data associated with the server */
static void
nginx_http_us_server_free(nginx_http_us_server *srv)
{
    free(srv->addr_spec);
    free(srv->name);
    free(srv);
}

/**
 * Find nginx upstream server by name.
 *
 * @param us          Upstream group
 * @param srv_name    Server name
 *
 * @return Server pointer, or @c NULL if server is not found.
 */
static nginx_http_us_server *
nginx_http_upstream_find_server(const nginx_http_upstream *us,
                                const char *srv_name)
{
    nginx_http_us_server *srv;

    LIST_FOREACH(srv, &us->servers, links)
    {
        if (strcmp(srv->name, srv_name) == 0)
            return srv;
    }

    return NULL;
}

/**
 * Find upstream server by its name and name of upstream group.
 *
 * @param inst_name   Nginx instance name
 * @param us_name     Upstream group name
 * @param srv_name    Server name
 *
 * @return Server pointer, or @c NULL if server is not found.
 */
static nginx_http_us_server *
nginx_http_us_server_find(const char *inst_name, const char *us_name,
                          const char *srv_name)
{
    nginx_http_upstream   *us;

    if ((us = nginx_http_upstream_find(inst_name, us_name)) == NULL)
        return NULL;

    return nginx_http_upstream_find_server(us, srv_name);
}

/** Free all data associated with the listening entry */
static void
nginx_http_listen_entry_free(nginx_http_listen_entry *entry)
{
    free(entry->name);
    free(entry->addr_spec);
    free(entry);
}

/**
 * Find server listening entry by name.
 *
 * @param srv         Server
 * @param entry_name  Listening entry name
 *
 * @return Listening entry pointer, or @c NULL if server is not found.
 */
static nginx_http_listen_entry *
nginx_http_server_find_listen_entry(nginx_http_server *srv, const char *entry_name)
{
    nginx_http_listen_entry *entry;

    LIST_FOREACH(entry, &srv->listen_entries, links)
    {
        if (strcmp(entry->name, entry_name) == 0)
            return entry;
    }

    return NULL;
}

/**
 * Find listening entry by name and names of nginx instance and server.
 *
 * @param inst_name   Nginx instance name
 * @param srv_name    Server name
 * @param entry_name  Listening entry name
 *
 * @return Listening entry pointer, or @c NULL if entry is not found.
 */
static nginx_http_listen_entry *
nginx_http_listen_entry_find(const char *inst_name, const char *srv_name,
                        const char *entry_name)
{
    nginx_http_server *srv;

    if ((srv = nginx_http_server_find(inst_name, srv_name)) == NULL)
        return NULL;

    return nginx_http_server_find_listen_entry(srv, entry_name);
}

/** Free all data associated with the location */
static void
nginx_http_loc_free(nginx_http_loc *loc)
{
    nginx_http_header   *hdr;
    nginx_http_header   *hdr_tmp;

    LIST_FOREACH_SAFE(hdr, &loc->proxy_headers, links, hdr_tmp)
    {
        LIST_REMOVE(hdr, links);
        nginx_http_loc_proxy_hdr_free(hdr);
    }

    free(loc->uri);
    free(loc->ret);
    free(loc->index);
    free(loc->root);
    free(loc->ssl_name);
    free(loc->proxy_pass_url);
    free(loc->proxy_http_version);
    free(loc->proxy_ssl_name);
    free(loc->name);
    free(loc);
}

/**
 * Find server location by name.
 *
 * @param srv         Server
 * @param loc_name    Location name
 *
 * @return Location pointer, or @c NULL if location is not found.
 */
static nginx_http_loc *
nginx_http_server_find_loc(nginx_http_server *srv, const char *loc_name)
{
    nginx_http_loc *loc = NULL;

    LIST_FOREACH(loc, &srv->locations, links)
    {
        if (strcmp(loc->name, loc_name) == 0)
            return loc;
    }

    return NULL;
}

/**
 * Find location by name and names of nginx instance and server.
 *
 * @param inst_name   Nginx instance name
 * @param srv_name    Server name
 * @param entry_name  Listening entry name
 *
 * @return Location pointer, or @c NULL if server is not found.
 */
static nginx_http_loc *
nginx_http_loc_find(const char *inst_name, const char *srv_name,
                    const char *loc_name)
{
    nginx_http_server *srv;

    if ((srv = nginx_http_server_find(inst_name, srv_name)) == NULL)
        return NULL;

    return nginx_http_server_find_loc(srv, loc_name);
}

/** Free all data associated with the header */
static void
nginx_http_loc_proxy_hdr_free(nginx_http_header *hdr)
{
    free(hdr->name);
    free(hdr->value);
    free(hdr);
}

/**
 * Find location proxy header by name.
 *
 * @param loc         Location
 * @param hdr_name    Header name
 *
 * @return Header pointer, or @c NULL if header is not found.
 */
static nginx_http_header *
nginx_http_loc_find_proxy_hdr(nginx_http_loc *loc, const char *hdr_name)
{
    nginx_http_header *hdr;

    LIST_FOREACH(hdr, &loc->proxy_headers, links)
    {
        if (strcmp(hdr->name, hdr_name) == 0)
            return hdr;
    }

    return NULL;
}

/**
 * Find http header by name and parents names.
 *
 * @param inst_name   Nginx instance name
 * @param srv_name    Server name
 * @param loc_name    Location name
 * @param hdr_name    Header name
 *
 * @return Header pointer, or @c NULL if header is not found.
 */
static nginx_http_header *
nginx_proxy_hdr_find(const char *inst_name, const char *srv_name,
                     const char *loc_name, const char *hdr_name)
{
    nginx_http_loc *loc;

    if ((loc = nginx_http_loc_find(inst_name, srv_name, loc_name)) == NULL)
        return NULL;

    return nginx_http_loc_find_proxy_hdr(loc, hdr_name);
}

/* Location proxy header get/set accessors */

static te_errno
nginx_http_loc_proxy_hdr_get(unsigned int gid, const char *oid,
                             char *value, const char *inst_name,
                             const char *http_name, const char *srv_name,
                             const char *loc_name, const char *proxy_name,
                             const char *hdr_name)
{
    nginx_http_header  *hdr;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);
    UNUSED(proxy_name);

    if ((hdr = nginx_proxy_hdr_find(inst_name, srv_name, loc_name,
                                    hdr_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return nginx_param_get_string(value, hdr->value);
}

static te_errno
nginx_http_loc_proxy_hdr_set(unsigned int gid, const char *oid,
                             const char *value, const char *inst_name,
                             const char *http_name, const char *srv_name,
                             const char *loc_name, const char *proxy_name,
                             const char *hdr_name)
{
    nginx_http_header  *hdr;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);
    UNUSED(proxy_name);

    if ((hdr = nginx_proxy_hdr_find(inst_name, srv_name, loc_name,
                                    hdr_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return nginx_param_set_string(&hdr->value, value);
}

/* Upstream group get/set accessors */

static te_errno
nginx_http_us_keepalive_num_get(unsigned int gid, const char *oid, char *value,
                                const char *inst_name, const char *http_name,
                                const char *us_name)
{
    nginx_http_upstream *us = nginx_http_upstream_find(inst_name, us_name);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);

    if (us == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return nginx_param_get_uint(value, us->keepalive_num);
}

static te_errno
nginx_http_us_keepalive_num_set(unsigned int gid, const char *oid,
                                const char *value, const char *inst_name,
                                const char *http_name, const char *us_name)
{
    nginx_http_upstream *us = nginx_http_upstream_find(inst_name, us_name);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);

    if (us == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return nginx_param_set_uint(&us->keepalive_num, value);
}

static te_errno
nginx_http_us_server_weight_get(unsigned int gid, const char *oid, char *value,
                                const char *inst_name, const char *http_name,
                                const char *us_name, const char *srv_name)
{
    nginx_http_us_server *srv = nginx_http_us_server_find(inst_name, us_name,
                                                          srv_name);
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);

    if (srv == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return nginx_param_get_uint(value, srv->weight);
}

static te_errno
nginx_http_us_server_weight_set(unsigned int gid, const char *oid,
                                const char *value, const char *inst_name,
                                const char *http_name, const char *us_name,
                                const char *srv_name)
{
    nginx_http_us_server *srv = nginx_http_us_server_find(inst_name, us_name,
                                                          srv_name);
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);

    if (srv == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return nginx_param_set_uint(&srv->weight, value);
}

static te_errno
nginx_http_us_server_get(unsigned int gid, const char *oid, char *value,
                         const char *inst_name, const char *http_name,
                         const char *us_name, const char *srv_name)
{
    nginx_http_us_server *srv = nginx_http_us_server_find(inst_name, us_name,
                                                          srv_name);
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);

    if (srv == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return nginx_param_get_string(value, srv->addr_spec);
}

static te_errno
nginx_http_us_server_set(unsigned int gid, const char *oid, const char *value,
                         const char *inst_name, const char *http_name,
                         const char *us_name, const char *srv_name)
{
    nginx_http_us_server *srv = nginx_http_us_server_find(inst_name, us_name,
                                                          srv_name);
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);

    if (srv == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return nginx_param_set_string(&srv->addr_spec, value);
}

/* Helpers for tokens mode get/set accessors */
static te_errno
nginx_param_get_tokens_mode(char *value, nginx_server_tokens_mode param)
{
    snprintf(value, RCF_MAX_VAL, "%d", param);
    return 0;
}

static te_errno
nginx_param_set_tokens_mode(nginx_server_tokens_mode *param,
                            const char *value)
{
    int mode;

    mode = atoi(value);

    if (mode < 0 || mode >= NGINX_SERVER_TOKENS_MODE_MAX)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *param = mode;
    return 0;
}

/* Nginx HTTP server C structure fields get/set accessors */
#define NGINX_SERVER_MAIN_PARAM_R(_param, _ptype) \
static te_errno                                                               \
nginx_http_server_##_param##_get(unsigned int gid, const char *oid,           \
                                 char *value, const char *inst_name,          \
                                 const char *http_name, const char *srv_name) \
{                                                                             \
    nginx_http_server *srv = nginx_http_server_find(inst_name, srv_name);     \
                                                                              \
    UNUSED(gid);                                                              \
    UNUSED(oid);                                                              \
    UNUSED(http_name);                                                        \
                                                                              \
    if (srv == NULL)                                                          \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                                  \
    return nginx_param_get_##_ptype(value, srv->_param);                      \
}

#define NGINX_SERVER_MAIN_PARAM_W(_param, _ptype) \
static te_errno                                                               \
nginx_http_server_##_param##_set(unsigned int gid, const char *oid,           \
                                 const char *value, const char *inst_name,    \
                                 const char *http_name, const char *srv_name) \
{                                                                             \
    nginx_http_server *srv = nginx_http_server_find(inst_name, srv_name);     \
                                                                              \
    UNUSED(gid);                                                              \
    UNUSED(oid);                                                              \
    UNUSED(http_name);                                                        \
                                                                              \
    if (srv == NULL)                                                          \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                                  \
    return nginx_param_set_##_ptype(&srv->_param, value);                     \
}

#define NGINX_SERVER_MAIN_PARAM_RW(_param, _ptype) \
NGINX_SERVER_MAIN_PARAM_R(_param, _ptype)          \
NGINX_SERVER_MAIN_PARAM_W(_param, _ptype)

NGINX_SERVER_MAIN_PARAM_RW(hostname, string)
NGINX_SERVER_MAIN_PARAM_RW(keepalive_timeout, uint)
NGINX_SERVER_MAIN_PARAM_RW(keepalive_requests, uint)
NGINX_SERVER_MAIN_PARAM_RW(send_timeout, uint)
NGINX_SERVER_MAIN_PARAM_RW(sendfile, boolean)
NGINX_SERVER_MAIN_PARAM_RW(tcp_nopush, boolean)
NGINX_SERVER_MAIN_PARAM_RW(tcp_nodelay, boolean)
NGINX_SERVER_MAIN_PARAM_RW(reset_timedout_conn, boolean)
NGINX_SERVER_MAIN_PARAM_RW(tokens_mode, tokens_mode)
NGINX_SERVER_MAIN_PARAM_RW(ssl_name, string)
NGINX_SERVER_MAIN_PARAM_RW(mime_type_default, string)
NGINX_SERVER_MAIN_PARAM_RW(access_log_enable, boolean)
NGINX_SERVER_MAIN_PARAM_R(access_log_path, string)

/* Nginx HTTP server C structure subfields get/set accessors */
#define NGINX_SERVER_SUBFIELD_PARAM_R(_field, _param, _ptype) \
static te_errno                                                               \
nginx_http_server_##_field##_##_param##_get(unsigned int gid,                 \
                                            const char *oid, char *value,     \
                                            const char *inst_name,            \
                                            const char *http_name,            \
                                            const char *srv_name)             \
{                                                                             \
    nginx_http_server *srv = nginx_http_server_find(inst_name, srv_name);     \
                                                                              \
    UNUSED(gid);                                                              \
    UNUSED(oid);                                                              \
    UNUSED(http_name);                                                        \
                                                                              \
    if (srv == NULL)                                                          \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                                  \
    return nginx_param_get_##_ptype(value, srv->_field._param);               \
}

#define NGINX_SERVER_SUBFIELD_PARAM_W(_field, _param, _ptype) \
static te_errno                                                               \
nginx_http_server_##_field##_##_param##_set(unsigned int gid,                 \
                                            const char *oid,                  \
                                            const char *value,                \
                                            const char *inst_name,            \
                                            const char *http_name,            \
                                            const char *srv_name)             \
{                                                                             \
    nginx_http_server *srv = nginx_http_server_find(inst_name, srv_name);     \
                                                                              \
    UNUSED(gid);                                                              \
    UNUSED(oid);                                                              \
    UNUSED(http_name);                                                        \
                                                                              \
    if (srv == NULL)                                                          \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                                  \
    return nginx_param_set_##_ptype(&srv->_field._param, value);              \
}

#define NGINX_SERVER_SUBFIELD_PARAM_RW(_field, _param, _ptype) \
NGINX_SERVER_SUBFIELD_PARAM_R(_field, _param, _ptype)          \
NGINX_SERVER_SUBFIELD_PARAM_W(_field, _param, _ptype)

NGINX_SERVER_SUBFIELD_PARAM_RW(proxy, conn_timeout, uint)
NGINX_SERVER_SUBFIELD_PARAM_RW(proxy, buffering_enable, boolean)
NGINX_SERVER_SUBFIELD_PARAM_RW(proxy, buffering_num, uint)
NGINX_SERVER_SUBFIELD_PARAM_RW(proxy, buffering_def_size, uint)
NGINX_SERVER_SUBFIELD_PARAM_RW(proxy, buffering_init_size, uint)

NGINX_SERVER_SUBFIELD_PARAM_RW(file_cache, enable, boolean)
NGINX_SERVER_SUBFIELD_PARAM_RW(file_cache, max_num, uint)
NGINX_SERVER_SUBFIELD_PARAM_RW(file_cache, inactive_time, uint)
NGINX_SERVER_SUBFIELD_PARAM_RW(file_cache, valid_time, uint)
NGINX_SERVER_SUBFIELD_PARAM_RW(file_cache, errors_enable, boolean)

NGINX_SERVER_SUBFIELD_PARAM_RW(client, body_timeout, uint)
NGINX_SERVER_SUBFIELD_PARAM_RW(client, body_max_size, uint)
NGINX_SERVER_SUBFIELD_PARAM_RW(client, header_timeout, uint)
NGINX_SERVER_SUBFIELD_PARAM_RW(client, header_buffer_size, uint)
NGINX_SERVER_SUBFIELD_PARAM_RW(client, large_header_buffer_num, uint)
NGINX_SERVER_SUBFIELD_PARAM_RW(client, large_header_buffer_size, uint)

/* Nginx HTTP server C structure list field subfields get/set accessors */
#define NGINX_SERVER_SUBLIST_PARAM_R(_stype, _param, _ptype) \
static te_errno                                                           \
nginx_http_##_stype##_##_param##_get(unsigned int gid, const char *oid,   \
                                     char *value, const char *inst_name,  \
                                     const char *http_name,               \
                                     const char *srv_name,                \
                                     const char *entry_name)              \
{                                                                         \
    nginx_http_##_stype *entry = nginx_http_##_stype##_find(inst_name,    \
                                                            srv_name,     \
                                                            entry_name);  \
                                                                          \
    UNUSED(gid);                                                          \
    UNUSED(oid);                                                          \
    UNUSED(http_name);                                                    \
                                                                          \
    if (entry == NULL)                                                    \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                              \
    return nginx_param_get_##_ptype(value, entry->_param);                \
}

#define NGINX_SERVER_SUBLIST_PARAM_W(_stype, _param, _ptype) \
static te_errno                                                           \
nginx_http_##_stype##_##_param##_set(unsigned int gid, const char *oid,   \
                                     const char *value,                   \
                                     const char *inst_name,               \
                                     const char *http_name,               \
                                     const char *srv_name,                \
                                     const char *entry_name)              \
{                                                                         \
    nginx_http_##_stype *entry = nginx_http_##_stype##_find(inst_name,    \
                                                            srv_name,     \
                                                            entry_name);  \
                                                                          \
    UNUSED(gid);                                                          \
    UNUSED(oid);                                                          \
    UNUSED(http_name);                                                    \
                                                                          \
    if (entry == NULL)                                                    \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                              \
    return nginx_param_set_##_ptype(&entry->_param, value);               \
}

#define NGINX_SERVER_SUBLIST_PARAM_RW(_stype, _param, _ptype) \
NGINX_SERVER_SUBLIST_PARAM_R(_stype, _param, _ptype)          \
NGINX_SERVER_SUBLIST_PARAM_W(_stype, _param, _ptype)

NGINX_SERVER_SUBLIST_PARAM_RW(listen_entry, addr_spec, string)
NGINX_SERVER_SUBLIST_PARAM_RW(listen_entry, reuseport, boolean)
NGINX_SERVER_SUBLIST_PARAM_RW(listen_entry, ssl, boolean)
NGINX_SERVER_SUBLIST_PARAM_RW(loc, uri, string)
NGINX_SERVER_SUBLIST_PARAM_RW(loc, ret, string)
NGINX_SERVER_SUBLIST_PARAM_RW(loc, index, string)
NGINX_SERVER_SUBLIST_PARAM_RW(loc, root, string)
NGINX_SERVER_SUBLIST_PARAM_RW(loc, ssl_name, string)
NGINX_SERVER_SUBLIST_PARAM_RW(loc, proxy_pass_url, string)
NGINX_SERVER_SUBLIST_PARAM_RW(loc, proxy_http_version, string)
NGINX_SERVER_SUBLIST_PARAM_RW(loc, proxy_ssl_name, string)

/* Upstream group server node basic operations */

static te_errno
nginx_http_us_server_add(unsigned int gid, const char *oid, const char *value,
                         const char *inst_name, const char *http_name,
                         const char *us_name, const char *srv_name)
{
    nginx_http_upstream   *us;
    nginx_http_us_server  *srv;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);

    if ((us = nginx_http_upstream_find(inst_name, us_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (nginx_http_upstream_find_server(us, srv_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    srv = TE_ALLOC(sizeof(*srv));
    if (srv == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    srv->name = strdup(srv_name);
    if (srv->name == NULL)
    {
        free(srv);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    srv->addr_spec = strdup(value);
    if (srv->addr_spec == NULL)
    {
        free(srv->name);
        free(srv);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    srv->weight = NGINX_HTTP_US_SRV_WEIGHT_DEF;

    LIST_INSERT_HEAD(&us->servers, srv, links);

    return 0;
}

static te_errno
nginx_http_us_server_del(unsigned int gid, const char *oid,
                         const char *inst_name, const char *http_name,
                         const char *us_name, const char *srv_name)
{
    nginx_http_us_server     *srv;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);

    if ((srv = nginx_http_us_server_find(inst_name, us_name,
                                         srv_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_REMOVE(srv, links);
    nginx_http_us_server_free(srv);

    return 0;
}

static te_errno
nginx_http_us_server_list(unsigned int gid, const char *oid,
                          const char *sub_id, char **list,
                          const char *inst_name, const char *http_name,
                          const char *us_name)
{
    te_string                   str = TE_STRING_INIT;
    nginx_http_upstream        *us;
    nginx_http_us_server       *srv;
    te_errno                    rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(http_name);

    if ((us = nginx_http_upstream_find(inst_name, us_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_FOREACH(srv, &us->servers, links)
    {
        rc = te_string_append(&str, (str.ptr != NULL) ? " %s" : "%s",
                              srv->name);
        if (rc != 0)
        {
            te_string_free(&str);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }

    *list = str.ptr;
    return 0;
}

/* Upstream group node basic operations */

static te_errno
nginx_http_upstream_add(unsigned int gid, const char *oid, const char *value,
                        const char *inst_name, const char *http_name,
                        const char *us_name)
{
    nginx_inst          *inst;
    nginx_http_upstream *us;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(http_name);

    if ((inst = nginx_inst_find(inst_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (nginx_inst_find_http_upstream(inst, us_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    us = TE_ALLOC(sizeof(*us));
    if (us == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    us->name = strdup(us_name);
    if (us->name == NULL)
    {
        free(us);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    us->keepalive_num = 0;

    LIST_INIT(&us->servers);
    LIST_INSERT_HEAD(&inst->http_upstreams, us, links);

    return 0;
}

static te_errno
nginx_http_upstream_del(unsigned int gid, const char *oid,
                        const char *inst_name, const char *http_name,
                        const char *us_name)
{
    nginx_http_upstream *us;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);

    if ((us = nginx_http_upstream_find(inst_name, us_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_REMOVE(us, links);
    nginx_http_upstream_free(us);

    return 0;
}

static te_errno
nginx_http_upstream_list(unsigned int gid, const char *oid,
                         const char *sub_id, char **list,
                         const char *inst_name, const char *http_name)
{
    te_string                   str = TE_STRING_INIT;
    nginx_inst                 *inst;
    nginx_http_upstream        *us;
    te_errno                    rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(http_name);

    if ((inst = nginx_inst_find(inst_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_FOREACH(us, &inst->http_upstreams, links)
    {
        rc = te_string_append(&str,
                              (str.ptr != NULL) ? " %s" : "%s", us->name);
        if (rc != 0)
        {
            te_string_free(&str);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }

    *list = str.ptr;
    return 0;
}

/* Location proxy header node basic operations */

static te_errno
nginx_http_loc_proxy_hdr_add(unsigned int gid, const char *oid,
                             const char *value, const char *inst_name,
                             const char *http_name, const char *srv_name,
                             const char *loc_name, const char *proxy_name,
                             const char *hdr_name)
{
    nginx_http_loc     *loc;
    nginx_http_header  *hdr;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);
    UNUSED(proxy_name);

    if ((loc = nginx_http_loc_find(inst_name, srv_name, loc_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (nginx_http_loc_find_proxy_hdr(loc, hdr_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    hdr = TE_ALLOC(sizeof(*hdr));
    if (hdr == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    hdr->name = strdup(hdr_name);
    if (hdr->name == NULL)
    {
        free(hdr);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    hdr->value = strdup(value);
    if (hdr->value == NULL)
    {
        free(hdr->name);
        free(hdr);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    LIST_INSERT_HEAD(&loc->proxy_headers, hdr, links);

    return 0;
}

static te_errno
nginx_http_loc_proxy_hdr_del(unsigned int gid, const char *oid,
                             const char *inst_name, const char *http_name,
                             const char *srv_name, const char *loc_name,
                             const char *proxy_name, const char *hdr_name)
{
    nginx_http_header  *hdr;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);
    UNUSED(proxy_name);

    if ((hdr = nginx_proxy_hdr_find(inst_name, srv_name, loc_name,
                                    hdr_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_REMOVE(hdr, links);
    nginx_http_loc_proxy_hdr_free(hdr);

    return 0;
}

static te_errno
nginx_http_loc_proxy_hdr_list(unsigned int gid, const char *oid,
                              const char *sub_id, char **list,
                              const char *inst_name, const char *http_name,
                              const char *srv_name, const char *loc_name)
{
    te_string           str = TE_STRING_INIT;
    nginx_http_loc     *loc;
    nginx_http_header  *hdr;
    te_errno            rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(http_name);

    if ((loc = nginx_http_loc_find(inst_name, srv_name, loc_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_FOREACH(hdr, &loc->proxy_headers, links)
    {
        rc = te_string_append(&str, (str.ptr != NULL) ? " %s" : "%s",
                              loc->name);
        if (rc != 0)
        {
            te_string_free(&str);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }

    *list = str.ptr;
    return 0;
}

/* Listening entry node basic operations */

static te_errno
nginx_http_listen_entry_add(unsigned int gid, const char *oid, const char *value,
                 const char *inst_name, const char *http_name,
                 const char *srv_name, const char *entry_name)
{
    nginx_http_server     *srv;
    nginx_http_listen_entry     *entry;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);

    if ((srv = nginx_http_server_find(inst_name, srv_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (nginx_http_server_find_listen_entry(srv, entry_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    entry = TE_ALLOC(sizeof(*entry));
    if (entry == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    entry->name = strdup(entry_name);
    if (entry->name == NULL)
    {
        free(entry);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    entry->addr_spec = strdup(value);
    if (entry->addr_spec == NULL)
    {
        free(entry->name);
        free(entry);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    entry->reuseport = FALSE;
    entry->ssl = FALSE;

    LIST_INSERT_HEAD(&srv->listen_entries, entry, links);

    return 0;
}

static te_errno
nginx_http_listen_entry_del(unsigned int gid, const char *oid,
                            const char *inst_name, const char *http_name,
                            const char *srv_name, const char *entry_name)
{
    nginx_http_listen_entry     *entry;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);

    if ((entry = nginx_http_listen_entry_find(inst_name, srv_name,
                                              entry_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_REMOVE(entry, links);
    nginx_http_listen_entry_free(entry);

    return 0;
}

static te_errno
nginx_http_listen_entry_list(unsigned int gid, const char *oid,
                             const char *sub_id, char **list,
                             const char *inst_name, const char *http_name,
                             const char *srv_name)
{
    te_string                 str = TE_STRING_INIT;
    nginx_http_server        *srv;
    nginx_http_listen_entry  *entry;
    te_errno                  rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(http_name);

    if ((srv = nginx_http_server_find(inst_name, srv_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_FOREACH(entry, &srv->listen_entries, links)
    {
        rc = te_string_append(&str, (str.ptr != NULL) ? " %s" : "%s",
                              entry->name);
        if (rc != 0)
        {
            te_string_free(&str);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }

    *list = str.ptr;
    return 0;
}

/* Location node basic operations */

static te_errno
nginx_http_loc_add(unsigned int gid, const char *oid, const char *value,
                   const char *inst_name, const char *http_name,
                   const char *srv_name, const char *loc_name)
{
    nginx_http_server     *srv;
    nginx_http_loc   *loc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(http_name);

    if ((srv = nginx_http_server_find(inst_name, srv_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (nginx_http_server_find_loc(srv, loc_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    loc = TE_ALLOC(sizeof(*loc));
    if (loc == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    loc->name = strdup(loc_name);
    if (loc->name == NULL)
    {
        free(loc);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    loc->uri = strdup("");
    loc->ret = strdup("");
    loc->index = strdup("");
    loc->root = strdup("");
    loc->ssl_name = strdup("");
    loc->proxy_pass_url= strdup("");
    loc->proxy_http_version = strdup("");
    loc->proxy_ssl_name = strdup("");

    if (loc->uri == NULL || loc->ret == NULL || loc->index == NULL ||
        loc->root == NULL || loc->ssl_name == NULL ||
        loc->proxy_pass_url == NULL || loc->proxy_http_version == NULL ||
        loc->proxy_ssl_name == NULL)
    {
        nginx_http_loc_free(loc);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    LIST_INIT(&loc->proxy_headers);
    LIST_INSERT_HEAD(&srv->locations, loc, links);

    return 0;
}

static te_errno
nginx_http_loc_del(unsigned int gid, const char *oid, const char *inst_name,
                   const char *http_name, const char *srv_name,
                   const char *loc_name)
{
    nginx_http_loc  *loc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);

    if ((loc = nginx_http_loc_find(inst_name, srv_name, loc_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_REMOVE(loc, links);
    nginx_http_loc_free(loc);

    return 0;
}

static te_errno
nginx_http_loc_list(unsigned int gid, const char *oid, const char *sub_id,
                    char **list, const char *inst_name, const char *http_name,
                    const char *srv_name)
{
    te_string           str = TE_STRING_INIT;
    nginx_http_server  *srv;
    nginx_http_loc     *loc;
    te_errno            rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(http_name);

    if ((srv = nginx_http_server_find(inst_name, srv_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_FOREACH(loc, &srv->locations, links)
    {
        rc = te_string_append(&str, (str.ptr != NULL) ? " %s" : "%s",
                              loc->name);
        if (rc != 0)
        {
            te_string_free(&str);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }

    *list = str.ptr;
    return 0;
}

/* HTTP server node basic operations */

static te_errno
nginx_http_server_add(unsigned int gid, const char *oid, const char *value,
                      const char *inst_name, const char *http_name,
                      const char *srv_name)
{
    nginx_inst         *inst;
    nginx_http_server  *srv;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(http_name);

    if ((inst = nginx_inst_find(inst_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (nginx_inst_find_server(inst, srv_name) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    srv = TE_ALLOC(sizeof(*srv));
    if (srv == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    srv->name = strdup(srv_name);
    srv->hostname = strdup("");
    srv->ssl_name = strdup("");
    srv->mime_type_default = strdup(NGINX_HTTP_SRV_MIME_TYPE_DEF);
    srv->access_log_path = te_sprintf(NGINX_HTTP_SRV_ACCESS_LOG_PATH_FMT,
                                      inst_name, srv_name);

    if (srv->name == NULL || srv->hostname == NULL || srv->ssl_name == NULL ||
        srv->mime_type_default == NULL || srv->access_log_path == NULL)
    {
        nginx_http_server_free(srv);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    srv->access_log_enable = TRUE;

    srv->proxy.conn_timeout = NGINX_HTTP_PROXY_CONN_TIMEOUT_DEF;
    srv->proxy.buffering_enable = FALSE;
    srv->proxy.buffering_num = 0;
    srv->proxy.buffering_def_size = 0;
    srv->proxy.buffering_init_size = 0;

    srv->file_cache.enable = FALSE;
    srv->file_cache.max_num = NGINX_HTTP_FILE_CACHE_MAX_NUM_DEF;
    srv->file_cache.inactive_time = NGINX_HTTP_FILE_CACHE_INACT_TIMEOUT_DEF;
    srv->file_cache.valid_time = NGINX_HTTP_FILE_CACHE_VALID_TIMEOUT_DEF;
    srv->file_cache.errors_enable = FALSE;

    srv->client.body_timeout = NGINX_HTTP_CLI_BODY_TIMEOUT_DEF;
    srv->client.body_max_size = NGINX_HTTP_CLI_BODY_MAX_SIZE_DEF;
    srv->client.header_timeout = NGINX_HTTP_CLI_HDR_TIMEOUT_DEF;
    srv->client.header_buffer_size = NGINX_HTTP_CLI_HDR_BUF_SIZE_DEF;
    srv->client.large_header_buffer_num = NGINX_HTTP_CLI_LRG_HDR_BUF_NUM_DEF;
    srv->client.large_header_buffer_size = NGINX_HTTP_CLI_LRG_HDR_BUF_SIZE_DEF;

    srv->keepalive_timeout = NGINX_HTTP_SRV_KEEPALIVE_TIMEOUT_DEF;
    srv->keepalive_requests = NGINX_HTTP_SRV_KEEPALIVE_REQS_DEF;
    srv->send_timeout = NGINX_HTTP_SRV_SEND_TIMEOUT_DEF;
    srv->sendfile = FALSE;
    srv->tcp_nopush = FALSE;
    srv->tcp_nodelay = TRUE;
    srv->reset_timedout_conn = FALSE;
    srv->tokens_mode = NGINX_SERVER_TOKENS_MODE_ON;

    LIST_INIT(&srv->listen_entries);
    LIST_INIT(&srv->locations);
    LIST_INSERT_HEAD(&inst->http_servers, srv, links);

    return 0;
}

static te_errno
nginx_http_server_del(unsigned int gid, const char *oid, const char *inst_name,
                      const char *http_name, const char *srv_name)
{
    nginx_http_server   *srv;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(http_name);

    if ((srv = nginx_http_server_find(inst_name, srv_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_REMOVE(srv, links);
    nginx_http_server_free(srv);

    return 0;
}

static te_errno
nginx_http_server_list(unsigned int gid, const char *oid, const char *sub_id,
                       char **list, const char *inst_name,
                       const char *http_name)
{
    te_string           str = TE_STRING_INIT;
    nginx_inst         *inst;
    nginx_http_server  *srv;
    te_errno            rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);
    UNUSED(http_name);

    if ((inst = nginx_inst_find(inst_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_FOREACH(srv, &inst->http_servers, links)
    {
        rc = te_string_append(&str, (str.ptr != NULL) ? " %s" : "%s",
                              srv->name);
        if (rc != 0)
        {
            te_string_free(&str);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }

    *list = str.ptr;
    return 0;
}

RCF_PCH_CFG_NODE_RWC(node_nginx_http_us_keepalive_num, "keepalive",
                     NULL, NULL, nginx_http_us_keepalive_num_get,
                     nginx_http_us_keepalive_num_set, &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_us_server_weight, "weight",
                     NULL, NULL, nginx_http_us_server_weight_get,
                     nginx_http_us_server_weight_set, &node_nginx_http);

static rcf_pch_cfg_object node_nginx_http_us_server =
    { "server", 0, &node_nginx_http_us_server_weight,
      &node_nginx_http_us_keepalive_num,
      (rcf_ch_cfg_get)nginx_http_us_server_get,
      (rcf_ch_cfg_set)nginx_http_us_server_set,
      (rcf_ch_cfg_add)nginx_http_us_server_add,
      (rcf_ch_cfg_del)nginx_http_us_server_del,
      (rcf_ch_cfg_list)nginx_http_us_server_list, (rcf_ch_cfg_commit)NULL,
      &node_nginx_http, NULL };

static rcf_pch_cfg_object node_nginx_http_upstream =
    { "upstream", 0, &node_nginx_http_us_server, NULL,
      (rcf_ch_cfg_get)NULL, (rcf_ch_cfg_set)NULL,
      (rcf_ch_cfg_add)nginx_http_upstream_add,
      (rcf_ch_cfg_del)nginx_http_upstream_del,
      (rcf_ch_cfg_list)nginx_http_upstream_list, (rcf_ch_cfg_commit)NULL,
      &node_nginx_http, NULL };

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_mime_type_default, "default",
                     NULL, NULL,
                     nginx_http_server_mime_type_default_get,
                     nginx_http_server_mime_type_default_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_NA(node_nginx_http_server_mime_type, "mime_type",
                    &node_nginx_http_server_mime_type_default, NULL);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_reset_timedout_conn,
                     "reset_timedout_connection",
                     NULL, &node_nginx_http_server_mime_type,
                     nginx_http_server_reset_timedout_conn_get,
                     nginx_http_server_reset_timedout_conn_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_tcp_nodelay_timeout, "tcp_nodelay",
                     NULL, &node_nginx_http_server_reset_timedout_conn,
                     nginx_http_server_tcp_nodelay_get,
                     nginx_http_server_tcp_nodelay_set, &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_tcp_nopush, "tcp_nopush",
                     NULL, &node_nginx_tcp_nodelay_timeout,
                     nginx_http_server_tcp_nopush_get,
                     nginx_http_server_tcp_nopush_set, &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_sendfile, "sendfile",
                     NULL, &node_nginx_http_server_tcp_nopush,
                     nginx_http_server_sendfile_get,
                     nginx_http_server_sendfile_set, &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_send_timeout, "send_timeout",
                     NULL, &node_nginx_http_server_sendfile,
                     nginx_http_server_send_timeout_get,
                     nginx_http_server_send_timeout_set, &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_keepalive_requests,
                     "keepalive_requests",
                     NULL, &node_nginx_http_server_send_timeout,
                     nginx_http_server_keepalive_requests_get,
                     nginx_http_server_keepalive_requests_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_keepalive_timeout,
                     "keepalive_timeout",
                     NULL, &node_nginx_http_server_keepalive_requests,
                     nginx_http_server_keepalive_timeout_get,
                     nginx_http_server_keepalive_timeout_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_client_large_header_buffer_size,
                     "size", NULL, NULL,
                     nginx_http_server_client_large_header_buffer_size_get,
                     nginx_http_server_client_large_header_buffer_size_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_client_large_header_buffer_num,
                     "num", NULL,
                     &node_nginx_http_server_client_large_header_buffer_size,
                     nginx_http_server_client_large_header_buffer_num_get,
                     nginx_http_server_client_large_header_buffer_num_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_NA(node_nginx_http_server_client_large_header_buffer,
                    "large_header_buffer",
                    &node_nginx_http_server_client_large_header_buffer_num,
                    NULL);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_client_header_buffer_size,
                    "header_buffer_size",
                    NULL, &node_nginx_http_server_client_large_header_buffer,
                    nginx_http_server_client_header_buffer_size_get,
                    nginx_http_server_client_header_buffer_size_set,
                    &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_client_header_timeout,
                     "header_timeout",
                     NULL, &node_nginx_http_server_client_header_buffer_size,
                     nginx_http_server_client_header_timeout_get,
                     nginx_http_server_client_header_timeout_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_client_body_max_size,
                     "max_body_size",
                     NULL, &node_nginx_http_server_client_header_timeout,
                     nginx_http_server_client_body_max_size_get,
                     nginx_http_server_client_body_max_size_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_client_body_timeout,
                     "body_timeout",
                     NULL, &node_nginx_http_server_client_body_max_size,
                     nginx_http_server_client_body_timeout_get,
                     nginx_http_server_client_body_timeout_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_NA(node_nginx_http_server_client, "client",
                    &node_nginx_http_server_client_body_timeout,
                    &node_nginx_http_server_keepalive_timeout);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_file_cache_errors_enable, "errors",
                     NULL, NULL,
                     nginx_http_server_file_cache_errors_enable_get,
                     nginx_http_server_file_cache_errors_enable_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_file_cache_valid_time, "valid",
                     NULL, &node_nginx_http_server_file_cache_errors_enable,
                     nginx_http_server_file_cache_valid_time_get,
                     nginx_http_server_file_cache_valid_time_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_file_cache_inactive_time,
                     "inactive",
                     NULL, &node_nginx_http_server_file_cache_valid_time,
                     nginx_http_server_file_cache_inactive_time_get,
                     nginx_http_server_file_cache_inactive_time_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_file_cache_max_num, "max",
                     NULL, &node_nginx_http_server_file_cache_inactive_time,
                     nginx_http_server_file_cache_max_num_get,
                     nginx_http_server_file_cache_max_num_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_file_cache, "open_file_cache",
                     &node_nginx_http_server_file_cache_max_num,
                     &node_nginx_http_server_client,
                     nginx_http_server_file_cache_enable_get,
                     nginx_http_server_file_cache_enable_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_proxy_buffering_init_size,
                     "init_size", NULL, NULL,
                     nginx_http_server_proxy_buffering_init_size_get,
                     nginx_http_server_proxy_buffering_init_size_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_proxy_buffering_def_size,
                     "def_size",
                     NULL, &node_nginx_http_server_proxy_buffering_init_size,
                     nginx_http_server_proxy_buffering_def_size_get,
                     nginx_http_server_proxy_buffering_def_size_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_proxy_buffering_num, "num",
                     NULL, &node_nginx_http_server_proxy_buffering_def_size,
                     nginx_http_server_proxy_buffering_num_get,
                     nginx_http_server_proxy_buffering_num_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_proxy_buffering, "buffering",
                     &node_nginx_http_server_proxy_buffering_num, NULL,
                     nginx_http_server_proxy_buffering_enable_get,
                     nginx_http_server_proxy_buffering_enable_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_proxy_conn_timeout,
                     "connect_timeout",
                     NULL, &node_nginx_http_server_proxy_buffering,
                     nginx_http_server_proxy_conn_timeout_get,
                     nginx_http_server_proxy_conn_timeout_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_NA(node_nginx_http_server_proxy, "proxy",
                    &node_nginx_http_server_proxy_conn_timeout,
                    &node_nginx_http_server_file_cache);

static rcf_pch_cfg_object node_nginx_http_loc_proxy_hdr =
    { "set_header", 0, NULL, NULL,
      (rcf_ch_cfg_get)nginx_http_loc_proxy_hdr_get,
      (rcf_ch_cfg_set)nginx_http_loc_proxy_hdr_set,
      (rcf_ch_cfg_add)nginx_http_loc_proxy_hdr_add,
      (rcf_ch_cfg_del)nginx_http_loc_proxy_hdr_del,
      (rcf_ch_cfg_list)nginx_http_loc_proxy_hdr_list, (rcf_ch_cfg_commit)NULL,
      &node_nginx_http, NULL };

RCF_PCH_CFG_NODE_RWC(node_nginx_http_loc_proxy_ssl_name, "ssl_name",
                     NULL, &node_nginx_http_loc_proxy_hdr,
                     nginx_http_loc_proxy_ssl_name_get,
                     nginx_http_loc_proxy_ssl_name_set, &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_loc_proxy_http_version, "http_version",
                     NULL, &node_nginx_http_loc_proxy_ssl_name,
                     nginx_http_loc_proxy_http_version_get,
                     nginx_http_loc_proxy_http_version_set, &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_loc_proxy_pass_url, "pass_url",
                     NULL, &node_nginx_http_loc_proxy_http_version,
                     nginx_http_loc_proxy_pass_url_get,
                     nginx_http_loc_proxy_pass_url_set, &node_nginx_http);

RCF_PCH_CFG_NODE_NA(node_nginx_http_proxy, "proxy",
                    &node_nginx_http_loc_proxy_pass_url, NULL);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_loc_ssl_name, "ssl_name",
                     NULL, &node_nginx_http_proxy,
                     nginx_http_loc_ssl_name_get, nginx_http_loc_ssl_name_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_loc_root, "root",
                     NULL, &node_nginx_http_loc_ssl_name,
                     nginx_http_loc_root_get, nginx_http_loc_root_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_loc_index, "index",
                     NULL, &node_nginx_http_loc_root,
                     nginx_http_loc_index_get, nginx_http_loc_index_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_loc_ret, "return",
                     NULL, &node_nginx_http_loc_index,
                     nginx_http_loc_ret_get, nginx_http_loc_ret_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_loc_uri, "uri",
                     NULL, &node_nginx_http_loc_ret,
                     nginx_http_loc_uri_get, nginx_http_loc_uri_set,
                     &node_nginx_http);

static rcf_pch_cfg_object node_nginx_http_loc =
    { "location", 0, &node_nginx_http_loc_uri, &node_nginx_http_server_proxy,
      (rcf_ch_cfg_get)NULL, (rcf_ch_cfg_set)NULL,
      (rcf_ch_cfg_add)nginx_http_loc_add,
      (rcf_ch_cfg_del)nginx_http_loc_del,
      (rcf_ch_cfg_list)nginx_http_loc_list, (rcf_ch_cfg_commit)NULL,
      &node_nginx_http, NULL };

RCF_PCH_CFG_NODE_RO(node_nginx_http_server_access_log_path, "path",
                    NULL, NULL, nginx_http_server_access_log_path_get);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_access_log, "access_log",
                     &node_nginx_http_server_access_log_path,
                     &node_nginx_http_loc,
                     nginx_http_server_access_log_enable_get,
                     nginx_http_server_access_log_enable_set,
                     &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_hostname, "hostname",
                     NULL, &node_nginx_http_server_access_log,
                     nginx_http_server_hostname_get,
                     nginx_http_server_hostname_set, &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_tokens_mode, "tokens",
                     NULL, &node_nginx_http_server_hostname,
                     nginx_http_server_tokens_mode_get,
                     nginx_http_server_tokens_mode_set, &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_server_ssl_name, "ssl_name",
                     NULL, &node_nginx_http_server_tokens_mode,
                     nginx_http_server_ssl_name_get,
                     nginx_http_server_ssl_name_set, &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_listen_entry_ssl, "ssl",
                     NULL, NULL,
                     nginx_http_listen_entry_ssl_get,
                     nginx_http_listen_entry_ssl_set, &node_nginx_http);

RCF_PCH_CFG_NODE_RWC(node_nginx_http_listen_entry_reuseport, "reuseport",
                     NULL, &node_nginx_http_listen_entry_ssl,
                     nginx_http_listen_entry_reuseport_get,
                     nginx_http_listen_entry_reuseport_set,
                     &node_nginx_http);

static rcf_pch_cfg_object node_nginx_http_listen_entry =
    { "listen", 0, &node_nginx_http_listen_entry_reuseport,
      &node_nginx_http_server_ssl_name,
      (rcf_ch_cfg_get)nginx_http_listen_entry_addr_spec_get,
      (rcf_ch_cfg_set)nginx_http_listen_entry_addr_spec_set,
      (rcf_ch_cfg_add)nginx_http_listen_entry_add,
      (rcf_ch_cfg_del)nginx_http_listen_entry_del,
      (rcf_ch_cfg_list)nginx_http_listen_entry_list, (rcf_ch_cfg_commit)NULL,
      &node_nginx_http, NULL };

static rcf_pch_cfg_object node_nginx_http_server =
    { "server", 0, &node_nginx_http_listen_entry,
      &node_nginx_http_upstream,
      (rcf_ch_cfg_get)NULL, (rcf_ch_cfg_set)NULL,
      (rcf_ch_cfg_add)nginx_http_server_add,
      (rcf_ch_cfg_del)nginx_http_server_del,
      (rcf_ch_cfg_list)nginx_http_server_list, (rcf_ch_cfg_commit)NULL,
      &node_nginx_http, NULL };

RCF_PCH_CFG_NODE_NA_COMMIT(node_nginx_http, "http", &node_nginx_http_server,
                           NULL, nginx_commit);

/* See description in conf_nginx.h */
te_errno
nginx_http_init(void)
{
    return rcf_pch_add_node("/agent/nginx", &node_nginx_http);
}

