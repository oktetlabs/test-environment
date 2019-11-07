/** @file
 * @brief Test API to control nginx configurator tree.
 *
 * Implementation of TAPI to configure nginx.
 *
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Marina Maslova <Marina.Maslova@oktetlabs.ru>
 */

#include "te_config.h"
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "tapi_cfg_nginx.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#include "te_defs.h"
#include "logger_api.h"
#include "tapi_cfg.h"
#include "tapi_cfg_base.h"
#include "conf_api.h"
#include "tapi_mem.h"

/* Macros to ensure no typos in common part of addresses */
#define TE_CFG_TA_NGINX_FMT     "/agent:%s/nginx:%s"
#define TE_CFG_TA_NGINX_ARGS(_ta, _inst_name)  (_ta), (_inst_name)

#define TE_CFG_TA_NGINX_SRV_FMT "/agent:%s/nginx:%s/http:/server:%s"
#define TE_CFG_TA_NGINX_SRV_ARGS(_ta, _inst_name, _srv_name) \
    (_ta), (_inst_name), (_srv_name)

#define TE_CFG_TA_NGINX_LISTEN_FMT \
    "/agent:%s/nginx:%s/http:/server:%s/listen:%s"
#define TE_CFG_TA_NGINX_LISTEN_ARGS(_ta, _inst_name, _srv_name, _entry_name) \
    (_ta), (_inst_name), (_srv_name), (_entry_name)

#define TE_CFG_TA_NGINX_LOC_FMT \
    "/agent:%s/nginx:%s/http:/server:%s/location:%s"
#define TE_CFG_TA_NGINX_LOC_ARGS(_ta, _inst_name, _srv_name, _loc_name) \
    (_ta), (_inst_name), (_srv_name), (_loc_name)

#define TE_CFG_TA_NGINX_US_FMT  "/agent:%s/nginx:%s/http:/upstream:%s"
#define TE_CFG_TA_NGINX_US_ARGS(_ta, _inst_name, _srv_name) \
    (_ta), (_inst_name), (_srv_name)

#define TE_CFG_TA_NGINX_US_SRV_FMT \
    "/agent:%s/nginx:%s/http:/upstream:%s/server:%s"
#define TE_CFG_TA_NGINX_US_SRV_ARGS(_ta, _inst_name, _us_name, _srv_name) \
    (_ta), (_inst_name), (_us_name), (_srv_name)

#define TE_CFG_TA_NGINX_SSL_FMT "/agent:%s/nginx:%s/ssl:%s"
#define TE_CFG_TA_NGINX_SSL_ARGS(_ta, _inst_name, _ssl_name) \
    (_ta), (_inst_name), (_ssl_name)

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_add(const char *ta, const char *inst_name)
{
    return cfg_add_instance_fmt(NULL, CVT_NONE, NULL, TE_CFG_TA_NGINX_FMT,
                                TE_CFG_TA_NGINX_ARGS(ta, inst_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_del(const char *ta, const char *inst_name)
{
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_NGINX_FMT,
                                TE_CFG_TA_NGINX_ARGS(ta, inst_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_enable(const char *ta, const char *inst_name)
{
    return tapi_cfg_set_int_fmt(1, NULL, TE_CFG_TA_NGINX_FMT "/status:",
                                TE_CFG_TA_NGINX_ARGS(ta, inst_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_disable(const char *ta, const char *inst_name)
{
    return tapi_cfg_set_int_fmt(0, NULL, TE_CFG_TA_NGINX_FMT "/status:",
                                TE_CFG_TA_NGINX_ARGS(ta, inst_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_server_add(const char *ta, const char *inst_name,
                               const char *srv_name)
{
    return cfg_add_instance_fmt(NULL, CVT_NONE, NULL,
                TE_CFG_TA_NGINX_SRV_FMT,
                TE_CFG_TA_NGINX_SRV_ARGS(ta, inst_name, srv_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_server_del(const char *ta, const char *inst_name,
                               const char *srv_name)
{
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_NGINX_SRV_FMT,
                TE_CFG_TA_NGINX_SRV_ARGS(ta, inst_name, srv_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_listen_entry_add(const char *ta, const char *inst_name,
                                     const char *srv_name,
                                     const char *entry_name,
                                     const char *addr_spec)
{
    return cfg_add_instance_fmt(NULL, CFG_VAL(STRING, addr_spec),
            TE_CFG_TA_NGINX_LISTEN_FMT,
            TE_CFG_TA_NGINX_LISTEN_ARGS(ta, inst_name, srv_name, entry_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_listen_entry_del(const char *ta, const char *inst_name,
                                     const char *srv_name,
                                     const char *entry_name)
{
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_NGINX_LISTEN_FMT,
            TE_CFG_TA_NGINX_LISTEN_ARGS(ta, inst_name, srv_name, entry_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_listen_entry_ssl_enable(const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            const char *entry_name)
{
    return tapi_cfg_set_int_fmt(1, NULL,
            TE_CFG_TA_NGINX_LISTEN_FMT "/ssl:",
            TE_CFG_TA_NGINX_LISTEN_ARGS(ta, inst_name, srv_name, entry_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_listen_entry_ssl_disable(const char *ta,
                                             const char *inst_name,
                                             const char *srv_name,
                                             const char *entry_name)
{
    return tapi_cfg_set_int_fmt(0, NULL,
            TE_CFG_TA_NGINX_LISTEN_FMT "/ssl:",
            TE_CFG_TA_NGINX_LISTEN_ARGS(ta, inst_name, srv_name, entry_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_listen_entry_reuseport_enable(const char *ta,
                                                  const char *inst_name,
                                                  const char *srv_name,
                                                  const char *entry_name)
{
    return tapi_cfg_set_int_fmt(1, NULL,
            TE_CFG_TA_NGINX_LISTEN_FMT "/reuseport:",
            TE_CFG_TA_NGINX_LISTEN_ARGS(ta, inst_name, srv_name, entry_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_listen_entry_reuseport_disable(const char *ta,
                                                   const char *inst_name,
                                                   const char *srv_name,
                                                   const char *entry_name)
{
    return tapi_cfg_set_int_fmt(0, NULL,
            TE_CFG_TA_NGINX_LISTEN_FMT "/reuseport:",
            TE_CFG_TA_NGINX_LISTEN_ARGS(ta, inst_name, srv_name, entry_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_loc_add(const char *ta, const char *inst_name,
                            const char *srv_name, const char *loc_name,
                            const char *uri)
{
    te_errno rc;

    rc = cfg_add_instance_fmt(NULL, CVT_NONE, NULL,
            TE_CFG_TA_NGINX_LOC_FMT,
            TE_CFG_TA_NGINX_LOC_ARGS(ta, inst_name, srv_name, loc_name));
    if (rc != 0)
        return rc;

    rc = cfg_set_instance_fmt(CFG_VAL(STRING, uri),
            TE_CFG_TA_NGINX_LOC_FMT "/uri:",
            TE_CFG_TA_NGINX_LOC_ARGS(ta, inst_name, srv_name, loc_name));
    if (rc != 0)
    {
       (void)cfg_del_instance_fmt(FALSE, TE_CFG_TA_NGINX_LOC_FMT,
                TE_CFG_TA_NGINX_LOC_ARGS(ta, inst_name, srv_name, loc_name));
    }

    return rc;
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_loc_del(const char *ta, const char *inst_name,
                            const char *srv_name, const char *loc_name)
{
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_NGINX_LOC_FMT,
            TE_CFG_TA_NGINX_LOC_ARGS(ta, inst_name, srv_name, loc_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_loc_proxy_hdr_add(const char *ta,
                                      const char *inst_name,
                                      const char *srv_name,
                                      const char *loc_name,
                                      const char *hdr_name,
                                      const char *hdr_value)
{
    return cfg_add_instance_fmt(NULL, CFG_VAL(STRING, hdr_value),
            TE_CFG_TA_NGINX_LOC_FMT "/proxy:/set_header:%s",
            TE_CFG_TA_NGINX_LOC_ARGS(ta, inst_name, srv_name, loc_name),
            hdr_name);
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_loc_proxy_hdr_del(const char *ta,
                                      const char *inst_name,
                                      const char *srv_name,
                                      const char *loc_name,
                                      const char *hdr_name)
{
    return cfg_del_instance_fmt(FALSE,
            TE_CFG_TA_NGINX_LOC_FMT "/proxy:/set_header:%s",
            TE_CFG_TA_NGINX_LOC_ARGS(ta, inst_name, srv_name, loc_name),
            hdr_name);
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_upstream_add(const char *ta, const char *inst_name,
                                 const char *us_name)
{
    return cfg_add_instance_fmt(NULL, CVT_NONE, NULL,
                TE_CFG_TA_NGINX_US_FMT,
                TE_CFG_TA_NGINX_US_ARGS(ta, inst_name, us_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_upstream_del(const char *ta, const char *inst_name,
                                 const char *us_name)
{
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_NGINX_SRV_FMT,
                TE_CFG_TA_NGINX_US_ARGS(ta, inst_name, us_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_upstream_keepalive_num_get(const char *ta,
                                               const char *inst_name,
                                               const char *us_name,
                                               unsigned int *keepalive_num)
{
    cfg_val_type val_type = CVT_INTEGER;

    return cfg_get_instance_fmt(&val_type, keepalive_num,
                TE_CFG_TA_NGINX_US_FMT,
                TE_CFG_TA_NGINX_US_ARGS(ta, inst_name, us_name));
}


/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_upstream_keepalive_num_set(const char *ta,
                                               const char *inst_name,
                                               const char *us_name,
                                               unsigned int keepalive_num)
{

    return cfg_set_instance_fmt(CFG_VAL(INTEGER, keepalive_num),
                TE_CFG_TA_NGINX_US_FMT,
                TE_CFG_TA_NGINX_US_ARGS(ta, inst_name, us_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_us_server_add(const char *ta, const char *inst_name,
                                  const char *us_name, const char *srv_name,
                                  const char *addr_spec)
{
    return cfg_add_instance_fmt(NULL, CFG_VAL(STRING, addr_spec),
                TE_CFG_TA_NGINX_US_SRV_FMT,
                TE_CFG_TA_NGINX_US_SRV_ARGS(ta, inst_name, us_name, srv_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_us_server_del(const char *ta, const char *inst_name,
                                  const char *us_name, const char *srv_name)
{
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_NGINX_US_SRV_FMT,
                TE_CFG_TA_NGINX_US_SRV_ARGS(ta, inst_name, us_name, srv_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_us_server_weight_get(const char *ta,
                                         const char *inst_name,
                                         const char *us_name,
                                         const char *srv_name,
                                         unsigned int *weight)
{
    cfg_val_type val_type = CVT_INTEGER;

    return cfg_get_instance_fmt(&val_type, weight,
                TE_CFG_TA_NGINX_US_SRV_FMT,
                TE_CFG_TA_NGINX_US_SRV_ARGS(ta, inst_name, us_name, srv_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_http_us_server_weight_set(const char *ta,
                                         const char *inst_name,
                                         const char *us_name,
                                         const char *srv_name,
                                         unsigned int weight)
{
    return cfg_set_instance_fmt(CFG_VAL(INTEGER, weight),
                TE_CFG_TA_NGINX_US_SRV_FMT,
                TE_CFG_TA_NGINX_US_SRV_ARGS(ta, inst_name, us_name, srv_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_ssl_add(const char *ta, const char *inst_name,
                       const char *ssl_name)
{
    return cfg_add_instance_fmt(NULL, CVT_NONE, NULL,
                TE_CFG_TA_NGINX_SSL_FMT,
                TE_CFG_TA_NGINX_SSL_ARGS(ta, inst_name, ssl_name));
}

/* See description in tapi_cfg_nginx.h */
te_errno
tapi_cfg_nginx_ssl_del(const char *ta, const char *inst_name,
                       const char *ssl_name)
{
    return cfg_del_instance_fmt(FALSE, TE_CFG_TA_NGINX_SSL_FMT,
                TE_CFG_TA_NGINX_SSL_ARGS(ta, inst_name, ssl_name));
}

/*
 * Helpers for simple accessors of nginx instance subnodes.
 * See accessors description in tapi_cfg_nginx.h
 */
#define NGINX_INST_PARAM_R(_subpath, _param, _arg_type, _cfg_type)      \
te_errno                                                                \
tapi_cfg_nginx_##_param##_get(const char *ta, const char *inst_name,    \
                              _arg_type *_param)                        \
{                                                                       \
    cfg_val_type val_type = CVT_##_cfg_type;                            \
                                                                        \
    return cfg_get_instance_fmt(&val_type, _param,                      \
                                TE_CFG_TA_NGINX_FMT _subpath,           \
                                TE_CFG_TA_NGINX_ARGS(ta, inst_name));   \
}

#define NGINX_INST_PARAM_W(_subpath, _param, _arg_type, _cfg_type)      \
te_errno                                                                \
tapi_cfg_nginx_##_param##_set(const char *ta, const char *inst_name,    \
                              const _arg_type _param)                   \
{                                                                       \
    return cfg_set_instance_fmt(CFG_VAL(_cfg_type, _param),             \
                                TE_CFG_TA_NGINX_FMT _subpath,           \
                                TE_CFG_TA_NGINX_ARGS(ta, inst_name));   \
}

#define NGINX_INST_PARAM_RW(_subpath, _param, _arg_type, _cfg_type)     \
NGINX_INST_PARAM_R(_subpath, _param, _arg_type, _cfg_type)              \
NGINX_INST_PARAM_W(_subpath, _param, _arg_type, _cfg_type)

#define NGINX_INST_PARAM_RW_BOOL(_subpath, _param) \
NGINX_INST_PARAM_R(_subpath, _param, te_bool, INTEGER)                    \
                                                                          \
te_errno                                                                  \
tapi_cfg_nginx_##_param##_enable(const char *ta, const char *inst_name)   \
{                                                                         \
    return tapi_cfg_set_int_fmt(1, NULL,                                  \
                                TE_CFG_TA_NGINX_FMT _subpath,             \
                                TE_CFG_TA_NGINX_ARGS(ta, inst_name));     \
}                                                                         \
                                                                          \
te_errno                                                                  \
tapi_cfg_nginx_##_param##_disable(const char *ta, const char *inst_name)  \
{                                                                         \
    return tapi_cfg_set_int_fmt(0, NULL,                                  \
                                TE_CFG_TA_NGINX_FMT _subpath,             \
                                TE_CFG_TA_NGINX_ARGS(ta, inst_name));     \
}

NGINX_INST_PARAM_RW("/cmd_prefix:", cmd_prefix, char *, STRING)
NGINX_INST_PARAM_R("/config_path:", config_path, char *, STRING)
NGINX_INST_PARAM_R("/error_log:/path:", error_log_path, char *, STRING)
NGINX_INST_PARAM_RW_BOOL("/error_log:", error_log)

NGINX_INST_PARAM_RW("/worker:/processes:", wrk_ps_num, unsigned int, INTEGER)
NGINX_INST_PARAM_RW("/worker:/cpu_affinity:/mode:", wrk_cpu_aff_mode,
                    te_nginx_cpu_aff_mode, INTEGER)
NGINX_INST_PARAM_RW("/worker:/cpu_affinity:/mask:", wrk_cpu_aff_mask,
                    char *, STRING)
NGINX_INST_PARAM_RW("/worker:/rlimit_nofile:", wrk_rlimit_nofile, unsigned int,
                    INTEGER)
NGINX_INST_PARAM_RW("/events:/worker_connections:", evt_wrk_conn_num,
                    unsigned int, INTEGER)
NGINX_INST_PARAM_RW("/events:/method:", evt_method, char *, STRING)

NGINX_INST_PARAM_RW_BOOL("/events:/multi_accept:", evt_multi_accept)
NGINX_INST_PARAM_RW_BOOL("/events:/accept_mutex:", evt_accept_mutex)

/*
 * Helpers for simple accessors of nginx ssl settings subnodes.
 * See accessors description in tapi_cfg_nginx.h
 */
#define NGINX_SSL_PARAM_R(_subpath, _param, _arg_type, _cfg_type) \
te_errno                                                                    \
tapi_cfg_nginx_ssl_##_param##_get(const char *ta, const char *inst_name,    \
                                  const char *ssl_name,                     \
                                  _arg_type *_param)                        \
{                                                                           \
    cfg_val_type val_type = CVT_##_cfg_type;                                \
                                                                            \
    return cfg_get_instance_fmt(&val_type, _param,                          \
                                TE_CFG_TA_NGINX_SSL_FMT _subpath,           \
                                TE_CFG_TA_NGINX_SSL_ARGS(ta, inst_name,     \
                                                         ssl_name));        \
}

#define NGINX_SSL_PARAM_W(_subpath, _param, _arg_type, _cfg_type) \
te_errno                                                                    \
tapi_cfg_nginx_ssl_##_param##_set(const char *ta, const char *inst_name,    \
                                  const char *ssl_name,                     \
                                  const _arg_type _param)                   \
{                                                                           \
    return cfg_set_instance_fmt(CFG_VAL(_cfg_type, _param),                 \
                                TE_CFG_TA_NGINX_SSL_FMT _subpath,           \
                                TE_CFG_TA_NGINX_SSL_ARGS(ta, inst_name,     \
                                                         ssl_name));        \
}

#define NGINX_SSL_PARAM_RW(_subpath, _param, _arg_type, _cfg_type)          \
NGINX_SSL_PARAM_R(_subpath, _param, _arg_type, _cfg_type)                   \
NGINX_SSL_PARAM_W(_subpath, _param, _arg_type, _cfg_type)

NGINX_SSL_PARAM_RW("/cert:", cert, char *, STRING)
NGINX_SSL_PARAM_RW("/key:", key, char *, STRING)
NGINX_SSL_PARAM_RW("/ciphers:", ciphers, char *, STRING)
NGINX_SSL_PARAM_RW("/protocols:", protocols, char *, STRING)
NGINX_SSL_PARAM_RW("/session_cache:", session_cache, char *, STRING)
NGINX_SSL_PARAM_RW("/session_timeout:", session_timeout, unsigned int, INTEGER)

/*
 * Helpers for simple accessors of nginx server subnodes.
 * See accessors description in tapi_cfg_nginx.h
 */
#define NGINX_SRV_PARAM_R(_subpath, _param, _arg_type, _cfg_type) \
te_errno                                                                    \
tapi_cfg_nginx_http_server_##_param##_get(const char *ta,                   \
                                          const char *inst_name,            \
                                          const char *srv_name,             \
                                          _arg_type *_param)                \
{                                                                           \
    cfg_val_type val_type = CVT_##_cfg_type;                                \
                                                                            \
    return cfg_get_instance_fmt(&val_type, _param,                          \
                                TE_CFG_TA_NGINX_SRV_FMT _subpath,           \
                                TE_CFG_TA_NGINX_SRV_ARGS(ta, inst_name,     \
                                                         srv_name));        \
}

#define NGINX_SRV_PARAM_W(_subpath, _param, _arg_type, _cfg_type) \
te_errno                                                                    \
tapi_cfg_nginx_http_server_##_param##_set(const char *ta,                   \
                                          const char *inst_name,            \
                                          const char *srv_name,             \
                                          const _arg_type _param)           \
{                                                                           \
    return cfg_set_instance_fmt(CFG_VAL(_cfg_type, _param),                 \
                                TE_CFG_TA_NGINX_SRV_FMT _subpath,           \
                                TE_CFG_TA_NGINX_SRV_ARGS(ta, inst_name,     \
                                                         srv_name));        \
}

#define NGINX_SRV_PARAM_RW(_subpath, _param, _arg_type, _cfg_type) \
NGINX_SRV_PARAM_R(_subpath, _param, _arg_type, _cfg_type) \
NGINX_SRV_PARAM_W(_subpath, _param, _arg_type, _cfg_type)

#define NGINX_SRV_PARAM_RW_BOOL(_subpath, _param) \
NGINX_SRV_PARAM_R(_subpath, _param, te_bool, INTEGER)                       \
                                                                            \
te_errno                                                                    \
tapi_cfg_nginx_http_server_##_param##_enable(const char *ta,                \
                                             const char *inst_name,         \
                                             const char *srv_name)          \
{                                                                           \
    return tapi_cfg_set_int_fmt(1, NULL,                                    \
                                TE_CFG_TA_NGINX_SRV_FMT _subpath,           \
                                TE_CFG_TA_NGINX_SRV_ARGS(ta, inst_name,     \
                                                         srv_name));        \
}                                                                           \
                                                                            \
te_errno                                                                    \
tapi_cfg_nginx_http_server_##_param##_disable(const char *ta,               \
                                              const char *inst_name,        \
                                              const char *srv_name)         \
{                                                                           \
    return tapi_cfg_set_int_fmt(0, NULL,                                    \
                                TE_CFG_TA_NGINX_SRV_FMT _subpath,           \
                                TE_CFG_TA_NGINX_SRV_ARGS(ta, inst_name,     \
                                                         srv_name));        \
}

NGINX_SRV_PARAM_R("/access_log:/path:", access_log_path, char *, STRING)
NGINX_SRV_PARAM_RW_BOOL("/access_log:", access_log)

NGINX_SRV_PARAM_RW("/hostname:", hostname, char *, STRING)
NGINX_SRV_PARAM_RW("/keepalive_timeout:", keepalive_timeout,
                   unsigned int, INTEGER)
NGINX_SRV_PARAM_RW("/keepalive_requests:", keepalive_requests,
                   unsigned int, INTEGER)
NGINX_SRV_PARAM_RW("/send_timeout:", send_timeout, unsigned int, INTEGER)
NGINX_SRV_PARAM_RW("/tokens_mode:", tokens_mode, te_nginx_server_tokens_mode,
                   INTEGER)
NGINX_SRV_PARAM_RW("/mime_type:/default:", mime_type_def, char *, STRING)
NGINX_SRV_PARAM_RW("/ssl_name:", ssl_name, char *, STRING)
NGINX_SRV_PARAM_RW_BOOL("/sendfile:", sendfile)
NGINX_SRV_PARAM_RW_BOOL("/tcp_nopush:", tcp_nopush)
NGINX_SRV_PARAM_RW_BOOL("/tcp_nodelay:", tcp_nodelay)
NGINX_SRV_PARAM_RW_BOOL("/reset_timedout_connection:", reset_timedout_conn)

NGINX_SRV_PARAM_RW("/proxy:/connect_timeout:", proxy_conn_timeout,
                   unsigned int, INTEGER)
NGINX_SRV_PARAM_RW("/proxy:/buffering:/num:", proxy_buf_num,
                   unsigned int, INTEGER)
NGINX_SRV_PARAM_RW("/proxy:/buffering:/def_size:", proxy_buf_def_size,
                   unsigned int, INTEGER)
NGINX_SRV_PARAM_RW("/proxy:/buffering:/init_size:", proxy_buf_init_size,
                   unsigned int, INTEGER)
NGINX_SRV_PARAM_RW_BOOL("/proxy:/buffering:", proxy_buf)

NGINX_SRV_PARAM_RW("/open_file_cache:/max:", file_cache_max_num,
                   unsigned int, INTEGER)
NGINX_SRV_PARAM_RW("/open_file_cache:/inactive:", file_cache_inactive_time,
                   unsigned int, INTEGER)
NGINX_SRV_PARAM_RW("/open_file_cache:/valid:", file_cache_valid_time,
                   unsigned int, INTEGER)
NGINX_SRV_PARAM_RW_BOOL("/open_file_cache:/errors:", file_cache_errors)
NGINX_SRV_PARAM_RW_BOOL("/open_file_cache:", file_cache)

NGINX_SRV_PARAM_RW("/client:/body_timeout:", client_body_timeout,
                   unsigned int, INTEGER)
NGINX_SRV_PARAM_RW("/client:/body_max_size:", client_body_max_size,
                   unsigned int, INTEGER)
NGINX_SRV_PARAM_RW("/client:/header_timeout:", client_header_timeout,
                   unsigned int, INTEGER)
NGINX_SRV_PARAM_RW("/client:/header_buffer_size:", client_header_buffer_size,
                   unsigned int, INTEGER)
NGINX_SRV_PARAM_RW("/client:/large_header_buffer_num:",
                   client_large_header_buffer_num, unsigned int, INTEGER)
NGINX_SRV_PARAM_RW("/client:/large_header_buffer_size:",
                   client_large_header_buffer_size, unsigned int, INTEGER)

/*
 * Helpers for simple accessors of nginx http location fields.
 * See accessors description in tapi_cfg_nginx.h
 */
#define NGINX_LOC_PARAM_R(_subpath, _param, _arg_type, _cfg_type) \
te_errno                                                                    \
tapi_cfg_nginx_http_loc_##_param##_get(const char *ta,                      \
                                       const char *inst_name,               \
                                       const char *srv_name,                \
                                       const char *loc_name,                \
                                       _arg_type *_param)                   \
{                                                                           \
    cfg_val_type val_type = CVT_##_cfg_type;                                \
                                                                            \
    return cfg_get_instance_fmt(&val_type, _param,                          \
                                TE_CFG_TA_NGINX_LOC_FMT _subpath,           \
                                TE_CFG_TA_NGINX_LOC_ARGS(ta, inst_name,     \
                                                         srv_name,          \
                                                         loc_name));        \
}

#define NGINX_LOC_PARAM_W(_subpath, _param, _arg_type, _cfg_type) \
te_errno                                                                    \
tapi_cfg_nginx_http_loc_##_param##_set(const char *ta,                      \
                                       const char *inst_name,               \
                                       const char *srv_name,                \
                                       const char *loc_name,                \
                                       const _arg_type _param)              \
{                                                                           \
    return cfg_set_instance_fmt(CFG_VAL(_cfg_type, _param),                 \
                                TE_CFG_TA_NGINX_LOC_FMT _subpath,           \
                                TE_CFG_TA_NGINX_LOC_ARGS(ta, inst_name,     \
                                                         srv_name,          \
                                                         loc_name));        \
}

#define NGINX_LOC_PARAM_RW(_subpath, _param, _arg_type, _cfg_type) \
NGINX_LOC_PARAM_R(_subpath, _param, _arg_type, _cfg_type) \
NGINX_LOC_PARAM_W(_subpath, _param, _arg_type, _cfg_type)

NGINX_LOC_PARAM_RW("/uri:", uri, char *, STRING)
NGINX_LOC_PARAM_RW("/return:", ret, char *, STRING)
NGINX_LOC_PARAM_RW("/index:", index, char *, STRING)
NGINX_LOC_PARAM_RW("/root:", root, char *, STRING)
NGINX_LOC_PARAM_RW("/ssl_name:", ssl_name, char *, STRING)

NGINX_LOC_PARAM_RW("/proxy:/ssl_name:", proxy_ssl_name, char *, STRING)
NGINX_LOC_PARAM_RW("/proxy:/pass_url:", proxy_pass_url, char *, STRING)
NGINX_LOC_PARAM_RW("/proxy:/http_version:", proxy_http_vers, char *, STRING)

