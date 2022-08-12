/** @file
 * @brief Unix Test Agent
 *
 * Nginx server support
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 */

#define TE_LGR_USER     "Unix Conf Nginx"

#include "te_config.h"
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "conf_daemons_internal.h"
#include "conf_nginx.h"
#include "rcf_pch.h"
#include "te_alloc.h"
#include "te_file.h"
#include "te_str.h"
#include "te_string.h"

/** Name of nginx executable */
#define NGINX_EXEC_NAME             "nginx"

/** Format string of path to PID file */
#define NGINX_PID_PATH_FMT          "/tmp/nginx_%s.pid"
/** Format string of path to configuration file on TA */
#define NGINX_CONFIG_PATH_FMT       "/tmp/nginx_%s.conf"
/** Format string of path to error log file on TA */
#define NGINX_ERROR_LOG_PATH_FMT    "/tmp/nginx_%s_error.log"

/** Level of nginx instance name in OID */
#define NGINX_OID_LEVEL_NAME        (2)

/** Default number of worker processes */
#define NGINX_WRK_PS_NUM_DEF        (1)
/** Default number of worker connections */
#define NGINX_WRK_CONN_NUM_DEF      (512)

/** Default SSL session timeout (in seconds) */
#define NGINX_SSL_SESS_TIMEOUT_DEF  (300)

/* Macro used to simplify error propagation in config writing functions */
#define FPRINTF(_file, _va_fmt...) \
    do {                                        \
        if (fprintf(_file, _va_fmt) < 0)        \
        {                                       \
            rc = TE_OS_RC(TE_TA_UNIX, errno);   \
            goto cleanup;                       \
        }                                       \
    } while (0)

/* Indentation prefixes of different levels for config writing */
#define IND1    "\t"
#define IND2    "\t\t"
#define IND3    "\t\t\t"

/* Forward declarations */
static rcf_pch_cfg_object node_nginx;
static te_errno nginx_inst_start(nginx_inst *inst);
static te_errno nginx_inst_stop(nginx_inst *inst);
static void nginx_inst_free(nginx_inst *inst);
static void nginx_ssl_entry_free(nginx_ssl_entry *entry);
static nginx_ssl_entry *nginx_inst_find_ssl_entry(const nginx_inst *inst,
                                                  const char *entry_name);

/* Head of nginx instances list */
static LIST_HEAD(, nginx_inst) nginxs = LIST_HEAD_INITIALIZER(nginxs);

/* Mapping of tokens mode to its string representation */
static const char *nginx_server_tokens_mode2str[] = {
    [NGINX_SERVER_TOKENS_MODE_OFF] = "off",
    [NGINX_SERVER_TOKENS_MODE_ON] = "on",
    [NGINX_SERVER_TOKENS_MODE_BUILD] = "build",
};

/* Helper for writing booleans parameters in nginx config */
static inline const char *
bool2str(te_bool par)
{
    return par ? "on" : "off";
}

/**
 * Write nginx daemon configuration file.
 *
 * @param inst      Nginx instance
 * @param f         Configuration file
 * @param indent    Indentation prefix
 * @param ssl_name  SSL settings entry name
 * @param is_proxy  Whether SSL settings are used for proxying
 *
 * @return Status code.
 */
static te_errno
nginx_inst_write_config_ssl_entry(nginx_inst *inst, FILE *f,
                                  const char *indent, const char *ssl_name,
                                  te_bool is_proxy)
{
    te_errno             rc = 0;
    nginx_ssl_entry     *ssl_entry;
    const char          *prefix = is_proxy ? "proxy_" : "";

    if (*ssl_name == '\0')
        return 0;

    ssl_entry = nginx_inst_find_ssl_entry(inst, ssl_name);
    if (ssl_entry == NULL)
    {
        ERROR("Failed to find SSL settings entry '%s'", ssl_name);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (*ssl_entry->cert != '\0')
    {
        FPRINTF(f, "%s%sssl_certificate %s;\n", indent, prefix,
                ssl_entry->cert);
    }

    if (*ssl_entry->key != '\0')
    {
        FPRINTF(f, "%s%sssl_certificate_key %s;\n", indent, prefix,
                ssl_entry->key);
    }

    if (*ssl_entry->ciphers != '\0')
    {
        FPRINTF(f, "%s%sssl_ciphers %s;\n", indent, prefix,
                ssl_entry->ciphers);
    }

    if (*ssl_entry->protocols != '\0')
    {
        FPRINTF(f, "%s%sssl_protocols %s;\n", indent, prefix,
                ssl_entry->protocols);
    }

    if (!is_proxy)
    {
        if (*ssl_entry->session_cache != '\0')
        {
            FPRINTF(f, "%sssl_session_cache %s;\n", indent,
                    ssl_entry->session_cache);
        }

        if (ssl_entry->session_timeout != 0)
        {
            FPRINTF(f, "%sssl_session_timeout %us;\n", indent,
                    ssl_entry->session_timeout);
        }
    }

cleanup:
    return rc;
}

/**
 * Write nginx daemon configuration file.
 *
 * @param inst    Nginx instance
 *
 * @return Status code.
 */
static te_errno
nginx_inst_write_config(nginx_inst *inst)
{
    te_errno    rc = 0;
    FILE       *f;

    nginx_http_server          *srv;
    nginx_http_listen_entry    *listen_entry;
    nginx_http_loc             *loc;
    nginx_http_header          *hdr;
    nginx_http_upstream        *us;
    nginx_http_us_server       *us_srv;

    f = fopen(inst->config_path, "w");
    if (f == NULL)
        return TE_OS_RC(TE_TA_UNIX, errno);

    if (inst->wrk_ps_num == 0)
        FPRINTF(f, "worker_processes auto;\n");
    else
        FPRINTF(f, "worker_processes %u;\n", inst->wrk_ps_num);

    if (inst->aff_mode == NGINX_CPU_AFF_MODE_MANUAL)
        FPRINTF(f, "worker_cpu_affinity %s;\n", inst->aff_mask);
    else if (inst->aff_mode == NGINX_CPU_AFF_MODE_AUTO)
        FPRINTF(f, "worker_cpu_affinity auto %s;\n", inst->aff_mask);

    if (inst->rlimit_nofile != 0)
        FPRINTF(f, "worker_rlimit_nofile %u;\n", inst->rlimit_nofile);

    FPRINTF(f, "error_log %s;\n", inst->error_log_enable ?
            inst->error_log_path : "/dev/null crit");

    FPRINTF(f, "pid %s;\n", inst->pid_path);

    /* Events section */
    FPRINTF(f, "events {\n");
    FPRINTF(f, IND1 "worker_connections %u;\n", inst->wrk_conn_num);

    if (*inst->evt_method != '\0')
        FPRINTF(f, IND1 "use %s;\n", inst->evt_method);

    FPRINTF(f, IND1 "multi_accept %s;\n", bool2str(inst->multi_accept));
    FPRINTF(f, IND1 "accept_mutex %s;\n", bool2str(inst->accept_mutex));
    FPRINTF(f, "}\n");

    /* HTTP section */
    FPRINTF(f, "http {\n");

    /* Server section */
    LIST_FOREACH(srv, &inst->http_servers, links)
    {
        FPRINTF(f, IND1 "server {\n");

        FPRINTF(f, IND2 "access_log %s;\n", srv->access_log_enable ?
                srv->access_log_path : "off");

        /* Listen section */
        LIST_FOREACH(listen_entry, &srv->listen_entries, links)
        {
            FPRINTF(f, IND2 "listen %s%s%s;\n", listen_entry->addr_spec,
                    listen_entry->reuseport ? " reuseport" : "",
                    listen_entry->ssl ? " ssl" : "");
        }

        FPRINTF(f, IND2 "server_name %s;\n",
                *srv->hostname == '\0' ? "\"\"" : srv->hostname);

        FPRINTF(f, IND2 "server_tokens %s;\n",
                nginx_server_tokens_mode2str[srv->tokens_mode]);

        rc = nginx_inst_write_config_ssl_entry(inst, f, IND2, srv->ssl_name,
                                               FALSE);
        if (rc != 0)
            goto cleanup;

        /* Proxy settings */
        if (srv->proxy.conn_timeout != 0)
        {
            FPRINTF(f, IND2 "proxy_connect_timeout %us;\n",
                    srv->proxy.conn_timeout);
        }

        FPRINTF(f, IND2 "proxy_buffering %s;\n",
                bool2str(srv->proxy.buffering_enable));

        if (srv->proxy.buffering_enable)
        {
            if (srv->proxy.buffering_num != 0)
            {
                FPRINTF(f, IND2 "proxy_buffers %u %uk;\n",
                        srv->proxy.buffering_num,
                        srv->proxy.buffering_def_size);
            }

            if (srv->proxy.buffering_init_size != 0)
            {
                FPRINTF(f, IND2 "proxy_buffer_size %uk;\n",
                        srv->proxy.buffering_init_size);
            }
        }

        /* File cache settings */
        if (srv->file_cache.enable)
        {
             FPRINTF(f, IND2 "open_file_cache max=%u inactive=%us;\n",
                     srv->file_cache.max_num, srv->file_cache.inactive_time);
             FPRINTF(f, IND2 "open_file_cache_errors %s;\n",
                     bool2str(srv->file_cache.errors_enable));
        }
        else
        {
             FPRINTF(f, IND2 "open_file_cache off;\n");
        }

        FPRINTF(f, IND2 "client_body_timeout %us;\n", srv->client.body_timeout);
        FPRINTF(f, IND2 "client_max_body_size %uk;\n",
                srv->client.body_max_size);
        FPRINTF(f, IND2 "client_header_timeout %us;\n",
                srv->client.header_timeout);
        FPRINTF(f, IND2 "client_header_buffer_size %uk;\n",
                srv->client.header_buffer_size);

        FPRINTF(f, IND2 "large_client_header_buffers %u %uk;\n",
                srv->client.large_header_buffer_num,
                srv->client.large_header_buffer_size);

        FPRINTF(f, IND2 "keepalive_timeout %us;\n", srv->keepalive_timeout);
        FPRINTF(f, IND2 "keepalive_requests %u;\n", srv->keepalive_requests);
        FPRINTF(f, IND2 "send_timeout %us;\n", srv->send_timeout);
        FPRINTF(f, IND2 "sendfile %s;\n", bool2str(srv->sendfile));
        FPRINTF(f, IND2 "tcp_nopush %s;\n", bool2str(srv->tcp_nopush));
        FPRINTF(f, IND2 "tcp_nodelay %s;\n", bool2str(srv->tcp_nodelay));
        FPRINTF(f, IND2 "reset_timedout_connection %s;\n",
                bool2str(srv->reset_timedout_conn));

        if (*srv->mime_type_default != '\0')
            FPRINTF(f, IND2 "default_type %s;\n", srv->mime_type_default);

        /* Location section */
        LIST_FOREACH(loc, &srv->locations, links)
        {
            FPRINTF(f, IND2 "location %s {\n", loc->uri);

            if (*loc->root != '\0')
                FPRINTF(f, IND3 "root %s;\n", loc->root);
            if (*loc->index != '\0')
                FPRINTF(f, IND3 "index %s;\n", loc->index);
            if (*loc->ret != '\0')
                FPRINTF(f, IND3 "return %s;\n", loc->ret);

            if (*loc->proxy_pass_url != '\0')
                FPRINTF(f, IND3 "proxy_pass %s;\n", loc->proxy_pass_url);

            if (*loc->proxy_http_version != '\0')
            {
                FPRINTF(f, IND3 "proxy_http_version %s;\n",
                        loc->proxy_http_version);
            }

            LIST_FOREACH(hdr, &loc->proxy_headers, links)
            {
                FPRINTF(f, IND3 "proxy_set_header %s %s;\n", hdr->name,
                        *hdr->value == '\0' ? "\"\"" : hdr->value);
            }

            rc = nginx_inst_write_config_ssl_entry(inst, f, IND3,
                                                   loc->ssl_name, FALSE);
            if (rc != 0)
                goto cleanup;

            rc = nginx_inst_write_config_ssl_entry(inst, f, IND3,
                                                   loc->proxy_ssl_name, TRUE);
            if (rc != 0)
                goto cleanup;

            /* End of location section */
            FPRINTF(f, IND2 "}\n");
        }

        /* End of server section */
        FPRINTF(f, IND1 "}\n");
    }

    LIST_FOREACH(us, &inst->http_upstreams, links)
    {
        /* Upstream section */
        FPRINTF(f, IND1 "upstream %s {\n", us->name);

        /* Upstream servers */
        LIST_FOREACH(us_srv, &us->servers, links)
        {
            FPRINTF(f, IND2 "server %s weight=%u;\n", us_srv->addr_spec,
                    us_srv->weight);
        }

        if (us->keepalive_num != 0)
            FPRINTF(f, IND2 "keepalive %u;\n", us->keepalive_num);

        /* End of upstream section */
        FPRINTF(f, IND1 "}\n");
    }

    /* End of HTTP section */
    FPRINTF(f, "}\n");

cleanup:
    if (fclose(f) == EOF && rc == 0)
        rc = TE_OS_RC(TE_TA_UNIX, errno);

    return rc;
}

/**
 * Free all data associated with nginx instance.
 *
 * @param inst    Nginx instance
 *
 * @return Status code.
 */
static void
nginx_inst_free(nginx_inst *inst)
{
    nginx_http_server   *srv;
    nginx_http_server   *srv_tmp;
    nginx_http_upstream *us;
    nginx_http_upstream *us_tmp;
    nginx_ssl_entry     *entry;
    nginx_ssl_entry     *entry_tmp;

    LIST_FOREACH_SAFE(srv, &inst->http_servers, links, srv_tmp)
    {
        LIST_REMOVE(srv, links);
        nginx_http_server_free(srv);
    }

    LIST_FOREACH_SAFE(us, &inst->http_upstreams, links, us_tmp)
    {
        LIST_REMOVE(us, links);
        nginx_http_upstream_free(us);
    }

    LIST_FOREACH_SAFE(entry, &inst->ssl_entries, links, entry_tmp)
    {
        LIST_REMOVE(entry, links);
        nginx_ssl_entry_free(entry);
    }

    free(inst->name);
    free(inst->pid_path);
    free(inst->config_path);
    free(inst->cmd_prefix);
    free(inst->error_log_path);
    free(inst->evt_method);
    free(inst->aff_mask);

    free(inst);
}

/**
 * Find nginx instance by name.
 *
 * @param name    Instance name.
 *
 * @return Instance pointer, or @c NULL if instance is not found.
 */
nginx_inst *
nginx_inst_find(const char *name)
{
    nginx_inst *inst;

    LIST_FOREACH(inst, &nginxs, links)
    {
        if (strcmp(inst->name, name) == 0)
            return inst;
    }

    return NULL;
}

/**
 * Restart nginx instance in case of need.
 *
 * @param inst    Nginx instance
 *
 * @return Status code.
 */
static te_errno
nginx_inst_restart(nginx_inst *inst)
{
    te_errno rc;

    if (!inst->is_running)
        return 0;

    rc = nginx_inst_stop(inst);
    if (rc != 0)
    {
        ERROR("Failed to stop inst during restart: %r", rc);
        return rc;
    }

    rc = nginx_inst_write_config(inst);
    if (rc != 0)
    {
        ERROR("Failed to write config file: %r", rc);
        return rc;
    }

    rc = nginx_inst_start(inst);
    if (rc != 0)
    {
        ERROR("Failed to restart inst: %r", rc);
        return rc;
    }

    return 0;
}

/**
 * Start nginx daemon with specified configuration file.
 *
 * @param inst    Nginx instance.
 *
 * @return Status code.
 */
static te_errno
nginx_inst_start(nginx_inst *inst)
{
    te_errno    rc;
    te_string   cmd = TE_STRING_INIT;

    rc = te_string_append(&cmd, "%s %s -c %s", inst->cmd_prefix,
                          NGINX_EXEC_NAME, inst->config_path);
    if (rc != 0)
        goto cleanup;

    if (ta_system(cmd.ptr) != 0)
    {
        ERROR("Couldn't start nginx daemon");
        rc = TE_RC(TE_TA_UNIX, TE_ESHCMD);
        goto cleanup;
    }

    te_string_free(&cmd);
    inst->is_running = TRUE;

    return 0;

cleanup:
    unlink(inst->pid_path);
    unlink(inst->config_path);
    unlink(inst->error_log_path);

    te_string_free(&cmd);
    inst->is_running = FALSE;
    return rc;
}

/**
 * Free all data associated with ssl entry.
 *
 * @param inst    Nginx instance
 *
 * @return Status code.
 */
static void
nginx_ssl_entry_free(nginx_ssl_entry *entry)
{
    free(entry->name);
    free(entry->cert);
    free(entry->key);
    free(entry->ciphers);
    free(entry->protocols);
    free(entry->session_cache);
    free(entry);
}

/**
 * Find nginx instance ssl entry by name.
 *
 * @param inst        Nginx instance
 * @param entry_name  ssl entry name
 *
 * @return ssl entry pointer, or @c NULL if entry is not found.
 */
static nginx_ssl_entry *
nginx_inst_find_ssl_entry(const nginx_inst *inst, const char *entry_name)
{
    nginx_ssl_entry *entry;

    LIST_FOREACH(entry, &inst->ssl_entries, links)
    {
        if (strcmp(entry->name, entry_name) == 0)
            return entry;
    }

    return NULL;
}

/**
 * Find ssl_entry group by its name and name of nginx instance.
 *
 * @param inst_name   Nginx instance name
 * @param entry_name     Upstream group name
 *
 * @return Upstream group pointer, or @c NULL if group is not found.
 */
static nginx_ssl_entry *
nginx_ssl_entry_find(const char *inst_name, const char *entry_name)
{
    nginx_inst   *inst;

    if ((inst = nginx_inst_find(inst_name)) == NULL)
        return NULL;

    return nginx_inst_find_ssl_entry(inst, entry_name);
}

/**
 * Send signal to the nginx process.
 *
 * @param inst  Nginx instance
 * @param sig   Target signal
 *
 * @return Status code.
 */
static te_errno
nginx_inst_send_signal(nginx_inst *inst, int sig)
{
    pid_t pid = te_file_read_pid(inst->pid_path);

    if (pid == -1)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (kill(pid, sig) != 0)
    {
        ERROR("Couldn't send signal %d to nginx daemon (pid %d)", sig, pid);
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

/**
 * Stop nginx daemon if it is running.
 *
 * @param inst  Server instance.
 *
 * @return Status code.
 */
static te_errno
nginx_inst_stop(nginx_inst *inst)
{
    te_errno           rc;
    nginx_http_server *srv;

    rc = nginx_inst_send_signal(inst, SIGTERM);

    unlink(inst->pid_path);
    unlink(inst->config_path);
    unlink(inst->error_log_path);

    LIST_FOREACH(srv, &inst->http_servers, links)
    {
        unlink(srv->access_log_path);
    }

    inst->is_running = FALSE;

    return rc;
}

/* Helpers for generic types get/set accessors */

/* See description in conf_nginx.h */
te_errno
nginx_param_get_string(char *value, const char *param)
{
    return te_snprintf(value, RCF_MAX_VAL, "%s", param);
}

/* See description in conf_nginx.h */
te_errno
nginx_param_set_string(char **param, const char *value)
{
    char *new_val;

    new_val = strdup(value);
    if (new_val == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    free(*param);
    *param = new_val;

    return 0;
}

/* See description in conf_nginx.h */
te_errno
nginx_param_get_uint(char *value, unsigned int param)
{
    return te_snprintf(value, RCF_MAX_VAL, "%u", param);
}

/* See description in conf_nginx.h */
te_errno
nginx_param_set_uint(unsigned int *param, const char *value)
{
    return te_strtoui(value, 0, param);
}

/* See description in conf_nginx.h */
te_errno
nginx_param_get_boolean(char *value, te_bool param)
{
    return te_snprintf(value, RCF_MAX_VAL, "%d", param);
}

/* Generic types helpers for get/set accessors */
te_errno
nginx_param_set_boolean(te_bool *param, const char *value)
{
    if (te_strtol_bool(value, param) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    return 0;
}

/* Helpers for affinity mode get/set accessors */
static te_errno
nginx_param_get_aff_mode(char *value, nginx_cpu_aff_mode param)
{
    return te_snprintf(value, RCF_MAX_VAL, "%d", param);
}

static te_errno
nginx_param_set_aff_mode(nginx_cpu_aff_mode *param, const char *value)
{
    unsigned int    mode;
    te_errno        rc;

    rc = te_strtoui(value, 0, &mode);

    if (rc != 0 || mode >= NGINX_CPU_AFF_MODE_MAX)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    *param = mode;
    return 0;
}

/* Nginx instance C structure fields get/set accessors */
#define NGINX_INST_PARAM_R(_param, _type) \
static te_errno                                                 \
nginx_##_param##_get(unsigned int gid, const char *oid,         \
                     char *value, const char *inst_name)        \
{                                                               \
    nginx_inst *inst = nginx_inst_find(inst_name);              \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
                                                                \
    if (inst == NULL)                                           \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
    return nginx_param_get_##_type(value, inst->_param);        \
}

#define NGINX_INST_PARAM_W(_param, _type) \
static te_errno                                                 \
nginx_##_param##_set(unsigned int gid, const char *oid,         \
                     char *value, const char *inst_name)        \
{                                                               \
    nginx_inst *inst = nginx_inst_find(inst_name);              \
                                                                \
    UNUSED(gid);                                                \
    UNUSED(oid);                                                \
                                                                \
    if (inst == NULL)                                           \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                    \
    return nginx_param_set_##_type(&inst->_param, value);       \
}

#define NGINX_INST_PARAM_RW(_param, _type) \
NGINX_INST_PARAM_R(_param, _type)          \
NGINX_INST_PARAM_W(_param, _type)

NGINX_INST_PARAM_RW(cmd_prefix, string)
NGINX_INST_PARAM_R(config_path, string)
NGINX_INST_PARAM_R(error_log_path, string)
NGINX_INST_PARAM_RW(error_log_enable, boolean)
NGINX_INST_PARAM_RW(wrk_ps_num, uint)
NGINX_INST_PARAM_RW(aff_mode, aff_mode)
NGINX_INST_PARAM_RW(aff_mask, string)
NGINX_INST_PARAM_RW(rlimit_nofile, uint)
NGINX_INST_PARAM_RW(wrk_conn_num, uint)
NGINX_INST_PARAM_RW(evt_method, string)
NGINX_INST_PARAM_RW(multi_accept, boolean)
NGINX_INST_PARAM_RW(accept_mutex, boolean)

/* Nginx ssl entry C structure fields get/set accessors */
#define NGINX_SSL_ENTRY_PARAM_R(_param, _type) \
static te_errno                                                            \
nginx_ssl_##_param##_get(unsigned int gid, const char *oid, char *value,   \
                         const char *inst_name, const char *entry_name)    \
{                                                                          \
    nginx_ssl_entry *entry = nginx_ssl_entry_find(inst_name, entry_name);  \
                                                                           \
    UNUSED(gid);                                                           \
    UNUSED(oid);                                                           \
                                                                           \
    if (entry == NULL)                                                     \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                               \
    return nginx_param_get_##_type(value, entry->_param);                  \
}

#define NGINX_SSL_ENTRY_PARAM_W(_param, _type) \
static te_errno                                                            \
nginx_ssl_##_param##_set(unsigned int gid, const char *oid,                \
                         const char *value, const char *inst_name,         \
                         const char *entry_name)                           \
{                                                                          \
    nginx_ssl_entry *entry = nginx_ssl_entry_find(inst_name, entry_name);  \
                                                                           \
    UNUSED(gid);                                                           \
    UNUSED(oid);                                                           \
                                                                           \
    if (entry == NULL)                                                     \
        return TE_RC(TE_TA_UNIX, TE_ENOENT);                               \
    return nginx_param_set_##_type(&entry->_param, value);                 \
}

#define NGINX_SSL_ENTRY_PARAM_RW(_param, _type) \
NGINX_SSL_ENTRY_PARAM_R(_param, _type)          \
NGINX_SSL_ENTRY_PARAM_W(_param, _type)

NGINX_SSL_ENTRY_PARAM_RW(cert, string)
NGINX_SSL_ENTRY_PARAM_RW(key, string)
NGINX_SSL_ENTRY_PARAM_RW(ciphers, string)
NGINX_SSL_ENTRY_PARAM_RW(protocols, string)
NGINX_SSL_ENTRY_PARAM_RW(session_cache, string)
NGINX_SSL_ENTRY_PARAM_RW(session_timeout, uint)

/** Get actual nginx daemon status */
static te_errno
nginx_status_get(unsigned int gid, const char *oid,
                 char *value, const char *inst_name)
{
    nginx_inst    *inst = nginx_inst_find(inst_name);
    int            status_local = 0;

    UNUSED(gid);
    UNUSED(oid);

    if (inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (inst->is_running)
    {
        if (nginx_inst_send_signal(inst, 0) == 0)
            status_local = 1;
    }

    return te_snprintf(value, RCF_MAX_VAL, "%d", status_local);
}

/** Set desired nginx daemon status */
static te_errno
nginx_status_set(unsigned int gid, const char *oid,
                 const char *value, const char *inst_name)
{
    nginx_inst  *inst = nginx_inst_find(inst_name);
    te_bool      status;
    te_errno     rc;

    UNUSED(gid);
    UNUSED(oid);

    if (inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (te_strtol_bool(value, &status) != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if (status != inst->is_running)
    {
        if (status)
        {
            rc = nginx_inst_write_config(inst);
            if (rc != 0)
            {
                ERROR("Failed to write config file: %r", rc);
                return rc;
            }

            rc = nginx_inst_start(inst);
            if (rc != 0)
            {
                ERROR("Couldn't start server: %r", rc);
                return rc;
            }
        }
        else
        {
            rc = nginx_inst_stop(inst);
            if (rc != 0)
            {
                ERROR("Couldn't stop server: %r", rc);
                return rc;
            }
        }
    }

    return 0;
}

/* SSL entry node basic operations */

static te_errno
nginx_ssl_entry_add(unsigned int gid, const char *oid, const char *value,
                    const char *inst_name, const char *entry_name)
{
    nginx_inst       *inst;
    nginx_ssl_entry  *entry;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if ((inst = nginx_inst_find(inst_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if (nginx_inst_find_ssl_entry(inst, entry_name) != NULL)
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

    entry->cert = strdup("");
    entry->key = strdup("");
    entry->ciphers = strdup("");
    entry->protocols = strdup("");
    entry->session_cache = strdup("");
    entry->session_timeout = NGINX_SSL_SESS_TIMEOUT_DEF;

    if (entry->cert == NULL || entry->key == NULL || entry->ciphers == NULL ||
        entry->protocols == NULL || entry->session_cache == NULL)
    {
        nginx_ssl_entry_free(entry);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    LIST_INSERT_HEAD(&inst->ssl_entries, entry, links);

    return 0;
}

static te_errno
nginx_ssl_entry_del(unsigned int gid, const char *oid,
                    const char *inst_name, const char *entry_name)
{
    nginx_ssl_entry     *entry;

    UNUSED(gid);
    UNUSED(oid);

    if ((entry = nginx_ssl_entry_find(inst_name, entry_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_REMOVE(entry, links);
    nginx_ssl_entry_free(entry);

    return 0;
}

static te_errno
nginx_ssl_entry_list(unsigned int gid, const char *oid, const char *sub_id,
                     char **list, const char *inst_name)
{
    te_string         str = TE_STRING_INIT;
    nginx_inst       *inst;
    nginx_ssl_entry  *entry;
    te_errno          rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    if ((inst = nginx_inst_find(inst_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_FOREACH(entry, &inst->ssl_entries, links)
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

/* Nginx node basic operations */

static te_errno
nginx_add(unsigned int gid, const char *oid,
          const char *value, const char *inst_name)
{
    nginx_inst  *inst;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    inst = nginx_inst_find(inst_name);
    if (inst != NULL)
    {
        ERROR("Instance with such name already exists: %s", inst_name);
        return TE_RC(TE_TA_UNIX, TE_EEXIST);
    }

    inst = TE_ALLOC(sizeof(nginx_inst));
    if (inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    inst->name = strdup(inst_name);
    inst->pid_path = te_sprintf(NGINX_PID_PATH_FMT, inst_name);
    inst->config_path = te_sprintf(NGINX_CONFIG_PATH_FMT, inst_name);
    inst->error_log_path = te_sprintf(NGINX_ERROR_LOG_PATH_FMT, inst_name);
    inst->cmd_prefix = strdup("");

    if (inst->name == NULL || inst->pid_path == NULL ||
        inst->config_path == NULL || inst->error_log_path == NULL ||
        inst->cmd_prefix == NULL)
    {
        nginx_inst_free(inst);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    inst->error_log_enable = TRUE;
    inst->is_running = FALSE;
    inst->wrk_ps_num = NGINX_WRK_PS_NUM_DEF;
    inst->wrk_conn_num = NGINX_WRK_CONN_NUM_DEF;

    inst->aff_mode = NGINX_CPU_AFF_MODE_NOTBOUND;
    inst->aff_mask = strdup("");
    inst->evt_method = strdup("");
    if (inst->aff_mask == NULL || inst->evt_method == NULL)
    {
        nginx_inst_free(inst);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    inst->rlimit_nofile = 0;
    inst->multi_accept = FALSE;
    inst->accept_mutex = FALSE;

    inst->to_be_deleted = FALSE;

    LIST_INIT(&inst->http_servers);
    LIST_INIT(&inst->http_upstreams);
    LIST_INSERT_HEAD(&nginxs, inst, links);

    return 0;
}

static te_errno
nginx_del(unsigned int gid, const char *oid, const char *inst_name)
{
    nginx_inst *inst;

    UNUSED(gid);
    UNUSED(oid);

    inst = nginx_inst_find(inst_name);
    if (inst == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    inst->to_be_deleted = TRUE;

    return 0;
}

static te_errno
nginx_list(unsigned int gid, const char *oid, const char *sub_id, char **list)
{
    nginx_inst      *inst;
    te_errno         rc;
    te_string        str = TE_STRING_INIT;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    LIST_FOREACH(inst, &nginxs, links)
    {
        rc = te_string_append(&str, (str.ptr != NULL) ? " %s" : "%s",
                              inst->name);
        if (rc != 0)
        {
            te_string_free(&str);
            return TE_RC(TE_TA_UNIX, rc);
        }
    }

    *list = str.ptr;
    return 0;
}

/* See description in conf_nginx.h */
te_errno
nginx_commit(unsigned int gid, const cfg_oid *p_oid)
{
    nginx_inst  *inst;
    const char  *name;

    UNUSED(gid);

    name = CFG_OID_GET_INST_NAME(p_oid, NGINX_OID_LEVEL_NAME);
    inst = nginx_inst_find(name);
    if (inst == NULL)
    {
        ERROR("Failed to find nginx instance '%s' on commit", name);
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }

    if (inst->to_be_deleted)
    {
        te_errno            rc;

        if (inst->is_running)
        {
            rc = nginx_inst_stop(inst);
            if (rc != 0)
                WARN("Failed to stop nginx while removing: %r", rc);
        }

        LIST_REMOVE(inst, links);
        nginx_inst_free(inst);

        return 0;
    }

    return nginx_inst_restart(inst);
}

RCF_PCH_CFG_NODE_RWC(node_nginx_ssl_session_timeout, "session_timeout",
                     NULL, NULL, nginx_ssl_session_timeout_get,
                     nginx_ssl_session_timeout_set, &node_nginx);

RCF_PCH_CFG_NODE_RWC(node_nginx_ssl_session_cache, "session_cache",
                     NULL, &node_nginx_ssl_session_timeout,
                     nginx_ssl_session_cache_get, nginx_ssl_session_cache_set,
                     &node_nginx);

RCF_PCH_CFG_NODE_RWC(node_nginx_ssl_protocols, "protocols",
                     NULL, &node_nginx_ssl_session_cache,
                     nginx_ssl_protocols_get, nginx_ssl_protocols_set,
                     &node_nginx);

RCF_PCH_CFG_NODE_RWC(node_nginx_ssl_ciphers, "ciphers", NULL,
                     &node_nginx_ssl_protocols, nginx_ssl_ciphers_get,
                     nginx_ssl_ciphers_set,
                     &node_nginx);

RCF_PCH_CFG_NODE_RWC(node_nginx_ssl_key, "key", NULL, &node_nginx_ssl_ciphers,
                     nginx_ssl_key_get, nginx_ssl_key_set, &node_nginx);

RCF_PCH_CFG_NODE_RWC(node_nginx_ssl_cert, "cert", NULL, &node_nginx_ssl_key,
                     nginx_ssl_cert_get, nginx_ssl_cert_set, &node_nginx);

static rcf_pch_cfg_object node_nginx_ssl_entry =
    { "ssl", 0, &node_nginx_ssl_cert, NULL,
      (rcf_ch_cfg_get)NULL, (rcf_ch_cfg_set)NULL,
      (rcf_ch_cfg_add)nginx_ssl_entry_add, (rcf_ch_cfg_del)nginx_ssl_entry_del,
      (rcf_ch_cfg_list)nginx_ssl_entry_list, (rcf_ch_cfg_commit)NULL,
      &node_nginx, NULL };

RCF_PCH_CFG_NODE_RWC(node_nginx_accept_mutex, "accept_mutex",
                     NULL, NULL, nginx_accept_mutex_get,
                     nginx_accept_mutex_set, &node_nginx);

RCF_PCH_CFG_NODE_RWC(node_nginx_multi_accept, "multi_accept",
                     NULL, &node_nginx_accept_mutex,
                     nginx_multi_accept_get, nginx_multi_accept_set,
                     &node_nginx);

RCF_PCH_CFG_NODE_RWC(node_nginx_evt_method, "method",
                     NULL, &node_nginx_multi_accept,
                     nginx_evt_method_get, nginx_evt_method_set, &node_nginx);

RCF_PCH_CFG_NODE_RWC(node_nginx_wrk_conn_num, "worker_connections",
                     NULL, &node_nginx_evt_method,
                     nginx_wrk_conn_num_get, nginx_wrk_conn_num_set,
                     &node_nginx);

RCF_PCH_CFG_NODE_NA(node_nginx_events, "events",
                    &node_nginx_wrk_conn_num, &node_nginx_ssl_entry);

RCF_PCH_CFG_NODE_RWC(node_nginx_rlimit_nofile, "rlimit_nofile",
                     NULL, NULL, nginx_rlimit_nofile_get,
                     nginx_rlimit_nofile_set, &node_nginx);

RCF_PCH_CFG_NODE_RWC(node_nginx_aff_mask, "mask",
                     NULL, NULL, nginx_aff_mask_get, nginx_aff_mask_set,
                     &node_nginx);

RCF_PCH_CFG_NODE_RWC(node_nginx_aff_mode, "mode",
                     NULL, &node_nginx_aff_mask,
                     nginx_aff_mode_get, nginx_aff_mode_set, &node_nginx);

RCF_PCH_CFG_NODE_NA(node_nginx_cpu_aff, "cpu_affinity",
                    &node_nginx_aff_mode, &node_nginx_rlimit_nofile);

RCF_PCH_CFG_NODE_RWC(node_nginx_wrk_ps_num, "processes",
                     NULL, &node_nginx_cpu_aff,
                     nginx_wrk_ps_num_get, nginx_wrk_ps_num_set,
                     &node_nginx);

RCF_PCH_CFG_NODE_NA(node_nginx_wrk, "worker",
                    &node_nginx_wrk_ps_num, &node_nginx_events);

RCF_PCH_CFG_NODE_RO(node_nginx_error_log_path, "path",
                    NULL, NULL, nginx_error_log_path_get);

RCF_PCH_CFG_NODE_RWC(node_nginx_error_log, "error_log",
                     &node_nginx_error_log_path, &node_nginx_wrk,
                     nginx_error_log_enable_get, nginx_error_log_enable_set,
                     &node_nginx);

RCF_PCH_CFG_NODE_RO(node_nginx_config_path, "config_path",
                    NULL, &node_nginx_error_log, nginx_config_path_get);

RCF_PCH_CFG_NODE_RWC(node_nginx_cmd_prefix, "cmd_prefix",
                     NULL, &node_nginx_config_path,
                     nginx_cmd_prefix_get, nginx_cmd_prefix_set,
                     &node_nginx);

RCF_PCH_CFG_NODE_RW(node_nginx_status, "status",
                    NULL, &node_nginx_cmd_prefix,
                    nginx_status_get, nginx_status_set);

RCF_PCH_CFG_NODE_COLLECTION(node_nginx, "nginx", &node_nginx_status, NULL,
                            nginx_add, nginx_del, nginx_list, nginx_commit);

te_errno
ta_unix_conf_nginx_init(void)
{
    te_errno rc;

    LIST_INIT(&nginxs);

    rc = rcf_pch_add_node("/agent", &node_nginx);
    if (rc != 0)
        return rc;

    return nginx_http_init();
}

