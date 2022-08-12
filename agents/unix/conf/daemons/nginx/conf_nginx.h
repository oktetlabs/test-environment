/** @file
 * @brief Unix Test Agent
 *
 * Nginx server support
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 *
 * @author Marina Maslova <Marina.Maslova@oktetlabs.ru>
 */
#ifndef __TE_AGENTS_UNIX_CONF_NGINX_H_
#define __TE_AGENTS_UNIX_CONF_NGINX_H_

#include "te_errno.h"
#include "te_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Worker processes CPU affinity modes */
typedef enum nginx_cpu_aff_mode {
    NGINX_CPU_AFF_MODE_NOTBOUND = 0,    /**< Do not bind to any specific CPU */
    NGINX_CPU_AFF_MODE_AUTO,            /**< Bind automatically */
    NGINX_CPU_AFF_MODE_MANUAL,          /**< CPU set is specified for each
                                             worker via mask */
    NGINX_CPU_AFF_MODE_MAX,             /**< Enumeration elements number */
} nginx_cpu_aff_mode;

/** Nginx server response presentation mode */
typedef enum nginx_server_tokens_mode {
    NGINX_SERVER_TOKENS_MODE_OFF = 0,   /**< Disable nginx tokens */
    NGINX_SERVER_TOKENS_MODE_ON,        /**< Emit nginx version */
    NGINX_SERVER_TOKENS_MODE_BUILD,     /**< Emit build name along with
                                             nginx version */
    NGINX_SERVER_TOKENS_MODE_MAX,       /**< Enumeration elements number */
} nginx_server_tokens_mode;

/** HTTP server listening socket entry */
typedef struct nginx_http_listen_entry {
    LIST_ENTRY(nginx_http_listen_entry) links; /**< Linked list of entries */

    char       *addr_spec;      /**< Address specification, e.g.
                                     IP-address:port, hostname:port,
                                     unix domain socket path */
    te_bool     reuseport;      /**< Create an individual listening
                                     socket for each worker */
    te_bool     ssl;            /**< Use ssl for connections */
    char       *name;           /**< Friendly entry name */
} nginx_http_listen_entry;

/** HTTP upstream group server */
typedef struct nginx_http_us_server {
    LIST_ENTRY(nginx_http_us_server) links;  /**< Linked list of servers */

    char         *name;         /**< Friendly server name */
    char         *addr_spec;    /**< Address specification, e.g.
                                     IP-address:port, hostname:port,
                                     unix domain socket path */
    unsigned int  weight;       /**< Weight of server in group */
} nginx_http_us_server;

/** HTTP upstream servers group */
typedef struct nginx_http_upstream {
    LIST_ENTRY(nginx_http_upstream)   links;   /**< Linked list of groups */
    LIST_HEAD(, nginx_http_us_server) servers; /**< Upstream servers */

    char         *name;             /**< Name of upstream group */
    unsigned int  keepalive_num;    /**< Maximum number of idle keepalive
                                         connections */
} nginx_http_upstream;

/** SSL settings entry */
typedef struct nginx_ssl_entry {
    LIST_ENTRY(nginx_ssl_entry) links; /**< Linked list of entries */

    char         *name;         /**< Friendly name of ssl entry */
    char         *cert;         /**< File path to certificate */
    char         *key;          /**< File path to certificate secrete key */
    char         *ciphers;      /**< SSL ciphers in OpenSSL library format */
    char         *protocols;    /**< SSL protocols list */

    char         *session_cache;    /**< SSL sessions cache specification */
    unsigned int  session_timeout;  /**< Timeout in seconds during which
                                         a client may reuse the session
                                         parameters */
} nginx_ssl_entry;

/** HTTP header */
typedef struct nginx_http_header {
    LIST_ENTRY(nginx_http_header) links; /**< Linked list of headers */

    char       *name;   /**< Header name according to HTTP specification */
    char       *value;  /**< Header value */
} nginx_http_header;

/** HTTP location settings */
typedef struct nginx_http_loc {
    LIST_ENTRY(nginx_http_loc) links; /**< Linked list of locations */

    char    *name;      /**< Friendly name of location */
    char    *uri;       /**< URI specification for matching */
    char    *ret;       /**< Return directive value */
    char    *index;     /**< Index filename */
    char    *root;      /**< Root path */
    char    *ssl_name;  /**< SSL settings object instance name */

    char    *proxy_pass_url;        /**< URL for location proxying */
    char    *proxy_http_version;    /**< HTTP version to set on proxying */
    char    *proxy_ssl_name;        /**< SSL settings object instance name
                                         for proxying */

    LIST_HEAD(, nginx_http_header) proxy_headers; /**< HTTP headers for
                                                       rewriting/appending
                                                       on proxying */
} nginx_http_loc;

/** HTTP client requests handling settings */
typedef struct nginx_http_client {
    unsigned int body_timeout;          /**< Timeout in seconds for reading
                                             client request body */
    unsigned int body_max_size;         /**< Maximum allowed size in kilobytes
                                             of the client request body */
    unsigned int header_timeout;        /**< Timeout for reading client request
                                             header */
    unsigned int header_buffer_size;    /**< Buffer size for reading client
                                             request header */

    unsigned int large_header_buffer_num;   /**< Maximum number of buffers for
                                                 reading large client request
                                                 header */
    unsigned int large_header_buffer_size;  /**< Maximum allowed size of
                                                 buffers for reading large
                                                 client request header
                                                 (in kilobytes) */
} nginx_http_client;

/** HTTP proxy settings */
typedef struct nginx_http_proxy {
    unsigned int    conn_timeout;      /**< Timeout in seconds for
                                            establishing a connection */
    te_bool         buffering_enable;  /**< Enable buffering */
    unsigned int    buffering_num;     /**< Buffers number */

    unsigned int    buffering_def_size;  /**< Default proxy buffers size
                                              in kilobytes */
    unsigned int    buffering_init_size; /**< Buffer size in kilobytes used for
                                              the first part of response */
} nginx_http_proxy;

/** HTTP server file caching settings */
typedef struct nginx_http_file_cache {
    te_bool         enable;         /**< Enable caching */
    unsigned int    max_num;        /**< Maximum number of elements
                                         in the cache */
    unsigned int    inactive_time;  /**< Time in seconds after which inactive
                                         cache entry will be removed */
    unsigned int    valid_time;     /**< Time in seconds after which cache
                                         elements should be validated */
    te_bool         errors_enable;  /**< Do caching of file lookup errors */
} nginx_http_file_cache;

/** HTTP server settings */
typedef struct nginx_http_server {
    LIST_ENTRY(nginx_http_server) links;     /**< Linked list of servers */
    LIST_HEAD(, nginx_http_loc)   locations; /**< HTTP locations */

    LIST_HEAD(, nginx_http_listen_entry) listen_entries; /**< Listening
                                                              entries */

    char    *name;      /**< Friendly name */
    char    *hostname;  /**< Server hostname pattern */
    char    *ssl_name;  /**< SSL settings object instance name */

    te_bool  access_log_enable;     /**< Enable access logging */
    char    *access_log_path;       /**< Path to access log file on TA */

    char          *mime_type_default;   /**< Default MIME type */

    unsigned int   keepalive_timeout;   /**< Timeout in seconds for keep-alive
                                             client connection */
    unsigned int   keepalive_requests;  /**< Maximum number of requests for
                                             one keep-alive connection */
    unsigned int   send_timeout;        /**< Timeout for transmitting
                                             a response */
    te_bool        sendfile;    /**<  Whether sendfile() should be used */
    te_bool        tcp_nopush;  /**<  Whether TCP_NOPUSH socket option
                                      should be used */
    te_bool        tcp_nodelay; /**<  Whether TCP_NODELAY socket option should
                                      be used */

    te_bool        reset_timedout_conn;    /**< Whether timed out connections
                                                should be reset */

    nginx_server_tokens_mode tokens_mode;  /**< Server presentation mode */

    nginx_http_client      client;         /**< Client handling settings */
    nginx_http_proxy       proxy;          /**< Proxy settings */
    nginx_http_file_cache  file_cache;     /**< File cache settings */
} nginx_http_server;

/** Nginx daemon instance */
typedef struct nginx_inst {
    LIST_ENTRY(nginx_inst) links; /**< Linked list of nginx instances */

    LIST_HEAD(, nginx_http_server)     http_servers;   /**< HTTP servers */
    LIST_HEAD(, nginx_http_upstream)   http_upstreams; /**< HTTP upstream
                                                            servers groups */
    LIST_HEAD(, nginx_ssl_entry)       ssl_entries;    /**< SSL settings */

    char       *name;           /**< Friendly name of nginx instance */
    te_bool     is_running;     /**< Is daemon running */
    char       *pid_path;       /**< Path to PID file on TA */
    char       *config_path;    /**< Path to configuration file on TA */
    char       *cmd_prefix;     /**< Prefix to nginx command line */

    te_bool     error_log_enable;   /**< Enable error logging */
    char       *error_log_path;     /**< Path to error log file on TA */

    char       *evt_method;     /**< Method of connections processing,
                                     e.g. epoll */
    te_bool     multi_accept;   /**< Whether one worker can accept multiple
                                     connections at a time */
    te_bool     accept_mutex;   /**< Whether worker processes will accept
                                     new connections by turn */

    unsigned int    wrk_ps_num;    /**< Number of worker processes */
    unsigned int    wrk_conn_num;  /**< Number of worker connections */
    unsigned int    rlimit_nofile; /**< Maximum number of open files
                                        for worker processes */
    char           *aff_mask;      /**< Worker processes CPU affinity mask */

    nginx_cpu_aff_mode aff_mode;   /**< Worker processes CPU affinity mode */

    te_bool     to_be_deleted;     /**< Flag to delete instance on commit */
} nginx_inst;

/**
 * Apply locally stored changeѕ.
 *
 * @param gid       Group identifier
 * @param p_oid     Pointer to the OID
 *
 * @return Status code
 */
extern te_errno nginx_commit(unsigned int gid, const cfg_oid *p_oid);

/**
 * Find nginx instance.
 *
 * @param name  Instance name
 *
 * @return Nginx instance pointer
 */
extern nginx_inst *nginx_inst_find(const char *name);

/**
 * Initialize nginx HTTP configuration subtree.
 *
 * @return Status code
 */
extern te_errno nginx_http_init(void);

/**
 * Free nginx HTTP server resources.
 *
 * @param srv   Server pointer
 */
extern void nginx_http_server_free(nginx_http_server *srv);

/**
 * Free nginx HTTP upstream group resources.
 *
 * @param us    Upstream group pointer
 */
extern void nginx_http_upstream_free(nginx_http_upstream *us);

/**
 * Get configurator value from string buffer nginx parameter.
 *
 * @param value         Configurator value buffer
 * @param param         Parameter value
 *
 * @return Status code
 */
extern te_errno nginx_param_get_string(char *value, const char *param);

/**
 * Set string buffer parameter from configurator value.
 * Previous value will be freed.
 *
 * @param param         Parameter location
 * @param value         Configurator value
 *
 * @return Status code
 */
extern te_errno nginx_param_set_string(char **param, const char *value);

/**
 * Get configurator value from unsigned integer nginx parameter.
 *
 * @param value         Configurator value buffer
 * @param param         Parameter value
 *
 * @return Status code
 */
extern te_errno nginx_param_get_uint(char *value, unsigned int param);

/**
 * Set nginx unsigned integer parameter from configurator value.
 *
 * @param param         Parameter location
 * @param value         Configurator value
 *
 * @return Status code
 */
extern te_errno nginx_param_set_uint(unsigned int *param, const char *value);

/**
 * Get configurator value from nginx boolean parameter.
 *
 * @param value         Configurator value buffer
 * @param param         Parameter value
 *
 * @return Status code
 */
extern te_errno nginx_param_get_boolean(char *value, te_bool param);

/**
 * Set nginx boolean parameter from configurator value.
 *
 * @param param         Parameter location
 * @param value         Configurator value
 *
 * @return Status code
 */
extern te_errno nginx_param_set_boolean(te_bool *param, const char *value);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_AGENTS_UNIX_CONF_NGINX_H_ */
