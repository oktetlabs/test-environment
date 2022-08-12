/** @file
 * @brief Test API to control Nginx configurator tree.
 *
 * Definition of TAPI to configure Nginx.
 *
 * Copyright (C) 2019-2022 OKTET Labs. All rights reserved.
 */

#ifndef __TE_TAPI_CFG_NGINX_H__
#define __TE_TAPI_CFG_NGINX_H__

#include "conf_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Nginx worker processes CPU affinity modes */
typedef enum te_nginx_cpu_aff_mode {
    TE_NGINX_CPU_AFF_MODE_NOTBOUND = 0, /**< Do not bind to any specific CPU */
    TE_NGINX_CPU_AFF_MODE_AUTO,         /**< Bind automatically */
    TE_NGINX_CPU_AFF_MODE_MANUAL,       /**< CPU set is specified for each
                                             worker via mask */
} te_nginx_cpu_aff_mode;

/** Nginx server response presentation mode */
typedef enum te_nginx_server_tokens_mode {
    TE_NGINX_SERVER_TOKENS_MODE_OFF = 0,  /**< Disable nginx tokens */
    TE_NGINX_SERVER_TOKENS_MODE_ON,       /**< Emit nginx version */
    TE_NGINX_SERVER_TOKENS_MODE_BUILD,    /**< Emit build name along with
                                               nginx version */
} te_nginx_server_tokens_mode;

/**
 * Add nginx instance
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_add(const char *ta, const char *inst_name);

/**
 * Delete nginx instance
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_del(const char *ta, const char *inst_name);

/**
 * Enable nginx instance.
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_enable(const char *ta, const char *inst_name);

/**
 * Disable nginx instance.
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_disable(const char *ta, const char *inst_name);

/**
 * Get nginx command line prefix
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param cmd_prefix    Location for command line prefix
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_cmd_prefix_get(const char *ta,
                                              const char *inst_name,
                                              char **cmd_prefix);

/**
 * Set nginx command line prefix
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param cmd_prefix    Command line prefix
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_cmd_prefix_set(const char *ta,
                                              const char *inst_name,
                                              const char *cmd_prefix);

/**
 * Get nginx configuration file path
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param config_path   Location for configuration file path
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_config_path_get(const char *ta,
                                               const char *inst_name,
                                               char **config_path);

/**
 * Get nginx error log file path
 *
 * @param ta                Test Agent
 * @param inst_name         Nginx instance name
 * @param error_log_path    Location for error log file path
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_error_log_path_get(const char *ta,
                                                  const char *inst_name,
                                                  char **error_log);

/**
 * Enable nginx error logging.
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_error_log_enable(const char *ta,
                                                const char *inst_name);

/**
 * Disable nginx error logging.
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_error_log_disable(const char *ta,
                                                 const char *inst_name);

/**
 * Get number of nginx worker processes
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param wrk_ps_num    Location for number of worker processes
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_wrk_ps_num_get(const char *ta,
                                              const char *inst_name,
                                              unsigned int *wrk_ps_num);

/**
 * Set number of nginx worker processes
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param wrk_ps_num    Number of worker processes
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_wrk_ps_num_set(const char *ta,
                                              const char *inst_name,
                                              unsigned int wrk_ps_num);

/**
 * Get CPU affinity mode for nginx worker processes
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param wrk_cpu_aff_mode      Location for affinity mode
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_wrk_cpu_aff_mode_get(
                                    const char *ta,
                                    const char *inst_name,
                                    te_nginx_cpu_aff_mode *wrk_cpu_aff_mode);

/**
 * Set CPU affinity mode for nginx worker processes
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param wrk_cpu_aff_mode      Affinity mode
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_wrk_cpu_aff_mode_set(
                                    const char *ta,
                                    const char *inst_name,
                                    te_nginx_cpu_aff_mode wrk_cpu_aff_mode);

/**
 * Get CPU affinity mask for nginx worker processes
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param wrk_cpu_aff_mask      Location for affinity mask
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_wrk_cpu_aff_mask_get(
                                            const char *ta,
                                            const char *inst_name,
                                            char **wrk_cpu_aff_mask);

/**
 * Set CPU affinity mask for nginx worker processes
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param wrk_cpu_aff_mask      Affinity mask
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_wrk_cpu_aff_mask_set(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *wrk_cpu_aff_mask);

/**
 * Get maximum number of open files for nginx worker processes
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param wrk_rlimit_nofile     Location for number of open files
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_wrk_rlimit_nofile_get(
                                         const char *ta,
                                         const char *inst_name,
                                         unsigned int *wrk_rlimit_nofile);

/**
 * Set maximum number of open files for nginx worker processes
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param wrk_rlimit_nofile     Number of open files
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_wrk_rlimit_nofile_set(
                                         const char *ta,
                                         const char *inst_name,
                                         unsigned int wrk_rlimit_nofile);

/**
 * Get number of nginx worker connections
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param evt_wrk_conn_num      Location for number of worker connections
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_evt_wrk_conn_num_get(
                                        const char *ta,
                                        const char *inst_name,
                                        unsigned int *evt_wrk_conn_num);


/**
 * Set number of nginx worker connections
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param evt_wrk_conn_num      Number of worker connections
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_evt_wrk_conn_num_set(
                                        const char *ta,
                                        const char *inst_name,
                                        unsigned int evt_wrk_conn_num);

/**
 * Get nginx connection processing method
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param evt_method    Location for connection processing method
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_evt_method_get(const char *ta,
                                              const char *inst_name,
                                              char **evt_method);

/**
 * Set nginx connection processing method
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param evt_method    Connection processing method, e.g. epoll
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_evt_method_set(const char *ta,
                                              const char *inst_name,
                                              const char *evt_method);

/**
 * Get status of accepting multiple connections
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param evt_multi_accept      Location for multiple accept status
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_evt_multi_accept_get(const char *ta,
                                                    const char *inst_name,
                                                    te_bool *evt_multi_accept);

/**
 * Enable accepting multiple connections
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_evt_multi_accept_enable(const char *ta,
                                                       const char *inst_name);

/**
 * Disable accepting multiple connections
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_evt_accept_mutex_disable(const char *ta,
                                                        const char *inst_name);

/**
 * Set status of handling connections by turn
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param evt_accept_mutex      Location for accept mutex status
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_evt_accept_mutex_get(const char *ta,
                                                    const char *inst_name,
                                                    te_bool *evt_accept_mutex);

/**
 * Enable handling connections by turn
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_evt_accept_mutex_enable(const char *ta,
                                                       const char *inst_name);

/**
 * Disable handling connections by turn
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_evt_accept_mutex_disable(const char *ta,
                                                        const char *inst_name);

/**
 * Add nginx server
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_add(const char *ta,
                                               const char *inst_name,
                                               const char *srv_name);

/**
 * Delete nginx server
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_del(const char *ta,
                                               const char *inst_name,
                                               const char *srv_name);

/**
 * Get nginx access log file path
 *
 * @param ta                Test Agent
 * @param inst_name         Nginx instance name
 * @param srv_name          Nginx server name
 * @param access_log_path   Location for error log file path
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_access_log_path_get(
                                                        const char *ta,
                                                        const char *inst_name,
                                                        const char *srv_name,
                                                        char **access_log);

/**
 * Enable nginx access logging.
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_access_log_enable(
                                                        const char *ta,
                                                        const char *inst_name,
                                                        const char *srv_name);

/**
 * Disable nginx access logging.
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_access_log_disable(
                                                        const char *ta,
                                                        const char *inst_name,
                                                        const char *srv_name);

/**
 * Get nginx server ssl name
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param ssl_name      Location for nginx ssl settings name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_ssl_name_get(const char *ta,
                                                        const char *inst_name,
                                                        const char *srv_name,
                                                        char **ssl_name);
/**
 * Set nginx server ssl name
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param ssl_name      Nginx ssl settings name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_ssl_name_set(const char *ta,
                                                        const char *inst_name,
                                                        const char *srv_name,
                                                        const char *ssl_name);

/**
 * Get nginx server ssl name
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param hostname      Location for nginx ssl settings name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_hostname_get(const char *ta,
                                                        const char *inst_name,
                                                        const char *srv_name,
                                                        char **hostname);
/**
 * Set nginx server ssl name
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param hostname      Nginx ssl settings name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_hostname_set(const char *ta,
                                                        const char *inst_name,
                                                        const char *srv_name,
                                                        const char *hostname);

/**
 * Get nginx server default mime type name
 *
 * @param ta                 Test Agent
 * @param inst_name          Nginx instance name
 * @param srv_name           Nginx server name
 * @param mime_type_def      Location for nginx ssl settings name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_mime_type_def_get(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name,
                                                    char **mime_type_def);

/**
 * Set nginx server default mime type
 *
 * @param ta                 Test Agent
 * @param inst_name          Nginx instance name
 * @param srv_name           Nginx server name
 * @param mime_type_def      Nginx default mime type
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_mime_type_def_set(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name,
                                                    const char *mime_type_def);

/**
 * Get nginx server keepalive timeout
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param keepalive_timeout     Location for keepalive timeout
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_keepalive_timeout_get(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int *keepalive_timeout);

/**
 * Set nginx server keepalive timeout
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param keepalive_timeout     Keepalive timeout
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_keepalive_timeout_set(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int keepalive_timeout);

/**
 * Get nginx server keepalive requests
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param keepalive_requests    Location for keepalive requests
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_keepalive_requests_get(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int *keepalive_requests);

/**
 * Set nginx server keepalive requests
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param keepalive_requests    Keepalive requests
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_keepalive_requests_set(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int keepalive_requests);

/**
 * Get nginx server send timeout
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param send_timeout          Location for send timeout
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_send_timeout_get(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int *send_timeout);

/**
 * Set nginx server send timeout
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param send_timeout          Send timeout
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_send_timeout_set(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int send_timeout);

/**
 * Get nginx server tokens mode
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param tokens_mode           Location for tokens mode
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_tokens_mode_get(
                                    const char *ta,
                                    const char *inst_name,
                                    const char *srv_name,
                                    te_nginx_server_tokens_mode *tokens_mode);

/**
 * Set nginx server tokens mode
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param tokens_mode           Tokens mode
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_tokens_mode_set(
                                    const char *ta,
                                    const char *inst_name,
                                    const char *srv_name,
                                    te_nginx_server_tokens_mode tokens_mode);

/**
 * Get nginx server sendfile option status
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param sendfile              Location for sendfile status
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_sendfile_get(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name,
                                                    te_bool *sendfile);

/**
 * Enable nginx server sendfile option
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_sendfile_enable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name);

/**
 * Disable nginx server sendfile option
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_sendfile_disable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name);

/**
 * Get nginx server TCP_NOPUSH option status
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param tcp_nopush            Location for TCP_NOPUSH status
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_tcp_nopush_get(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name,
                                                    te_bool *tcp_nopush);

/**
 * Enable nginx server TCP_NOPUSH option
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_tcp_nopush_enable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name);

/**
 * Disable nginx server TCP_NOPUSH option
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_tcp_nopush_disable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name);

/**
 * Get nginx server TCP_NODELAY option status
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param tcp_nodelay           Location for TCP_NODELAY status
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_tcp_nodelay_get(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name,
                                                    te_bool *tcp_nodelay);

/**
 * Enable nginx server TCP_NODELAY option
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_tcp_nodelay_enable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name);

/**
 * Disable nginx server TCP_NODELAY option
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_tcp_nodelay_disable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name);

/**
 * Get nginx server reset timedout connection option status
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param reset_timedout_conn   Location for reset timedout connection status
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_reset_timedout_conn_get(
                                                const char *ta,
                                                const char *inst_name,
                                                const char *srv_name,
                                                te_bool *reset_timedout_conn);

/**
 * Enable nginx server reset timedout connection option
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_reset_timedout_conn_enable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name);

/**
 * Disable nginx server reset timedout connection option
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_reset_timedout_conn_disable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name);

/**
 * Get nginx server proxy connect timeout
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param proxy_conn_timeout    Location for connect timeout
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_proxy_conn_timeout_get(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int *proxy_conn_timeout);

/**
 * Set nginx server proxy connect timeout
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param proxy_conn_timeout    Connect timeout
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_proxy_conn_timeout_set(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int proxy_conn_timeout);

/**
 * Get nginx server proxy number of buffers
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param proxy_buf_num         Location for proxy number of buffers
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_proxy_buf_num_get(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int *proxy_buf_num);

/**
 * Set nginx server proxy number of buffers
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param proxy_buf_num         Number of buffers
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_proxy_buf_num_set(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int proxy_buf_num);


/**
 * Get nginx server proxy default buffer size
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param proxy_buf_size        Location for buffer size
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_proxy_buf_def_size_get(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int *proxy_buf_def_size);

/**
 * Set nginx server proxy default buffer size
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param proxy_buf_size        Buffer size
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_proxy_buf_def_size_set(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int proxy_buf_def_size);

/**
 * Get nginx server proxy buffer size used for the first part of response
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param proxy_buf_init_size   Location for buffer size
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_proxy_buf_init_size_get(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int *proxy_buf_init_size);

/**
 * Set nginx server proxy buffer size used for the first part of response
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param proxy_buf_init_size   Buffer size
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_proxy_buf_init_size_set(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int proxy_buf_init_size);

/**
 * Get nginx server proxy buffering status
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param proxy_buf             Location for buffering status
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_proxy_buf_get(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name,
                                                    te_bool *proxy_buf);

/**
 * Enable nginx server proxy buffering
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_proxy_buf_enable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name);

/**
 * Disable nginx server proxy buffering
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_proxy_buf_disable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name);

/**
 * Get maximum number of nginx file cache elements
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param file_cache_max_num    Location for maximum number of cache elements
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_file_cache_max_num_get(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int *file_cache_max_num);

/**
 * Set maximum number of nginx server file cache elements
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param file_cache_max_num    Maximum number of cache elements
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_file_cache_max_num_set(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            unsigned int file_cache_max_num);

/**
 * Get nginx server file cache inactive timeout
 *
 * @param ta                        Test Agent
 * @param inst_name                 Nginx instance name
 * @param srv_name                  Nginx server name
 * @param file_cache_inactive_time  Location for inactive timeout (in seconds)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_file_cache_inactive_time_get(
                                    const char *ta,
                                    const char *inst_name,
                                    const char *srv_name,
                                    unsigned int *file_cache_inactive_time);

/**
 * Set nginx server file cache inactive timeout
 *
 * @param ta                        Test Agent
 * @param inst_name                 Nginx instance name
 * @param srv_name                  Nginx server name
 * @param file_cache_inactive_time  Inactive timeout (in seconds)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_file_cache_inactive_time_set(
                                    const char *ta,
                                    const char *inst_name,
                                    const char *srv_name,
                                    unsigned int file_cache_inactive_time);

/**
 * Get nginx server file cache validation timeout
 *
 * @param ta                      Test Agent
 * @param inst_name               Nginx instance name
 * @param srv_name                Nginx server name
 * @param file_cache_valid_time   Location for validation timeout (in seconds)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_file_cache_valid_time_get(
                                    const char *ta,
                                    const char *inst_name,
                                    const char *srv_name,
                                    unsigned int *file_cache_valid_time);

/**
 * Set nginx server file cache validation timeout
 *
 * @param ta                      Test Agent
 * @param inst_name               Nginx instance name
 * @param srv_name                Nginx server name
 * @param file_cache_valid_time   Validation timeout (in seconds)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_file_cache_valid_time_set(
                                    const char *ta,
                                    const char *inst_name,
                                    const char *srv_name,
                                    unsigned int file_cache_valid_time);

/**
 * Get nginx server file lookup errors caching status
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param file_cache_errors     Location for errors caching status
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_file_cache_errors_get(
                                            const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            te_bool *file_cache_errors);

/**
 * Enable nginx server file lookup errors caching
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_file_cache_errors_enable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name);

/**
 * Disable nginx server file lookup errors caching
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_file_cache_errors_disable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name);

/**
 * Get nginx server file information caching status
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 * @param file_cache            Location for file caching
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_file_cache_get(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name,
                                                    te_bool *file_cache);

/**
 * Enable nginx server file information caching
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_file_cache_enable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name);

/**
 * Disable nginx server file information caching
 *
 * @param ta                    Test Agent
 * @param inst_name             Nginx instance name
 * @param srv_name              Nginx server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_file_cache_disable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name);

/**
 * Get nginx server client body timeout
 *
 * @param ta                      Test Agent
 * @param inst_name               Nginx instance name
 * @param srv_name                Nginx server name
 * @param client_body_timeout     Location for client body timeout (in seconds)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_client_body_timeout_get(
                                    const char *ta,
                                    const char *inst_name,
                                    const char *srv_name,
                                    unsigned int *client_body_timeout);

/**
 * Set nginx server client body timeout
 *
 * @param ta                      Test Agent
 * @param inst_name               Nginx instance name
 * @param srv_name                Nginx server name
 * @param client_body_timeout     Client body timeout (in seconds)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_client_body_timeout_set(
                                    const char *ta,
                                    const char *inst_name,
                                    const char *srv_name,
                                    unsigned int client_body_timeout);

/**
 * Get nginx server client body maximum size
 *
 * @param ta                      Test Agent
 * @param inst_name               Nginx instance name
 * @param srv_name                Nginx server name
 * @param client_body_max_size    Location for maximum body size
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_client_body_max_size_get(
                                    const char *ta,
                                    const char *inst_name,
                                    const char *srv_name,
                                    unsigned int *client_body_max_size);

/**
 * Set nginx server client body maximum size
 *
 * @param ta                      Test Agent
 * @param inst_name               Nginx instance name
 * @param srv_name                Nginx server name
 * @param client_body_max_size    Maximum body size
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_client_body_max_size_set(
                                    const char *ta,
                                    const char *inst_name,
                                    const char *srv_name,
                                    unsigned int client_body_max_size);

/**
 * Get nginx server client header timeout
 *
 * @param ta                      Test Agent
 * @param inst_name               Nginx instance name
 * @param srv_name                Nginx server name
 * @param client_header_timeout   Location for header timeout (in seconds)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_client_header_timeout_get(
                                    const char *ta,
                                    const char *inst_name,
                                    const char *srv_name,
                                    unsigned int *client_header_timeout);

/**
 * Set nginx server client header timeout
 *
 * @param ta                      Test Agent
 * @param inst_name               Nginx instance name
 * @param srv_name                Nginx server name
 * @param client_header_timeout   Header timeout (in seconds)
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_client_header_timeout_set(
                                    const char *ta,
                                    const char *inst_name,
                                    const char *srv_name,
                                    unsigned int client_header_timeout);

/**
 * Get nginx server client header buffer size
 *
 * @param ta                          Test Agent
 * @param inst_name                   Nginx instance name
 * @param srv_name                    Nginx server name
 * @param client_header_buffer_size   Location for header buffer size
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_client_header_buffer_size_get(
                                    const char *ta,
                                    const char *inst_name,
                                    const char *srv_name,
                                    unsigned int *client_header_buffer_size);

/**
 * Set nginx server client header buffer size
 *
 * @param ta                          Test Agent
 * @param inst_name                   Nginx instance name
 * @param srv_name                    Nginx server name
 * @param client_header_buffer_size   Header buffer size
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_client_header_buffer_size_set(
                                    const char *ta,
                                    const char *inst_name,
                                    const char *srv_name,
                                    unsigned int client_header_buffer_size);

/**
 * Get nginx server client large header buffer number
 *
 * @param ta                              Test Agent
 * @param inst_name                       Nginx instance name
 * @param srv_name                        Nginx server name
 * @param client_large_header_buffer_num  Location for buffer number
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_client_large_header_buffer_num_get(
                                const char *ta,
                                const char *inst_name,
                                const char *srv_name,
                                unsigned int *client_large_header_buffer_num);

/**
 * Set nginx server client large header buffer number
 *
 * @param ta                              Test Agent
 * @param inst_name                       Nginx instance name
 * @param srv_name                        Nginx server name
 * @param client_large_header_buffer_num  Buffer number
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_client_large_header_buffer_num_set(
                                const char *ta,
                                const char *inst_name,
                                const char *srv_name,
                                unsigned int client_large_header_buffer_num);

/**
 * Get nginx server client large header buffer size
 *
 * @param ta                              Test Agent
 * @param inst_name                       Nginx instance name
 * @param srv_name                        Nginx server name
 * @param client_large_header_buffer_size  Location for buffer size
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_client_large_header_buffer_size_get(
                                const char *ta,
                                const char *inst_name,
                                const char *srv_name,
                                unsigned int *client_large_header_buffer_size);

/**
 * Set nginx server client large header buffer size
 *
 * @param ta                              Test Agent
 * @param inst_name                       Nginx instance name
 * @param srv_name                        Nginx server name
 * @param client_large_header_buffer_size  Buffer size
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_server_client_large_header_buffer_size_set(
                                const char *ta,
                                const char *inst_name,
                                const char *srv_name,
                                unsigned int client_large_header_buffer_size);

/**
 * Add nginx server listening entry
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param entry_name    Nginx listening entry name
 * @param addr_spec     Listening entry address specification,
 *                      e.g. port, unix socket, address:port
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_listen_entry_add(const char *ta,
                                                     const char *inst_name,
                                                     const char *srv_name,
                                                     const char *entry_name,
                                                     const char *addr_spec);

/**
 * Delete nginx server listening entry
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param entry_name    Nginx listening entry name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_listen_entry_del(const char *ta,
                                                     const char *inst_name,
                                                     const char *srv_name,
                                                     const char *entry_name);

/**
 * Enable ssl for nginx server listening entry
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param entry_name    Nginx listening entry name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_listen_entry_ssl_enable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name,
                                                    const char *entry_name);

/**
 * Disable ssl for nginx server listening entry
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param entry_name    Nginx listening entry name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_listen_entry_ssl_disable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name,
                                                    const char *entry_name);

/**
 * Enable port reusing for nginx server listening entry
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param entry_name    Nginx listening entry name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_listen_entry_reuseport_enable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name,
                                                    const char *entry_name);

/**
 * Disable port reusing for nginx server listening entry
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param entry_name    Nginx listening entry name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_listen_entry_reuseport_disable(
                                                    const char *ta,
                                                    const char *inst_name,
                                                    const char *srv_name,
                                                    const char *entry_name);

/**
 * Add nginx HTTP location entry
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param loc_name      HTTP location name
 * @param uri           URI specification
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_add(const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            const char *loc_name,
                                            const char *uri);

/**
 * Delete nginx HTTP location entry
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param loc_name      HTTP location name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_del(const char *ta,
                                            const char *inst_name,
                                            const char *srv_name,
                                            const char *loc_name);

/**
 * Add nginx HTTP location proxy http header
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param loc_name      HTTP location name
 * @param hdr_name      HTTP header name
 * @param hdr_value     HTTP header value
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_proxy_hdr_add(const char *ta,
                                                      const char *inst_name,
                                                      const char *srv_name,
                                                      const char *loc_name,
                                                      const char *hdr_name,
                                                      const char *hdr_value);

/**
 * Delete nginx HTTP location proxy http header
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param loc_name      HTTP location name
 * @param hdr_name      HTTP header name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_proxy_hdr_del(const char *ta,
                                                      const char *inst_name,
                                                      const char *srv_name,
                                                      const char *loc_name,
                                                      const char *hdr_name);

/**
 * Get nginx HTTP location URI
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param loc_name      HTTP location name
 * @param uri           Location for URI specification
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_uri_get(const char *ta,
                                                const char *inst_name,
                                                const char *srv_name,
                                                const char *loc_name,
                                                char **uri);

/**
 * Set nginx HTTP location URI
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param loc_name      HTTP location name
 * @param uri           URI specification
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_uri_set(const char *ta,
                                                const char *inst_name,
                                                const char *srv_name,
                                                const char *loc_name,
                                                const char *uri);

/**
 * Get nginx HTTP location return directive value
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param loc_name      HTTP location name
 * @param ret           Location for return directive
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_ret_get(const char *ta,
                                                const char *inst_name,
                                                const char *srv_name,
                                                const char *loc_name,
                                                char **ret);

/**
 * Set nginx HTTP location return directive value
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param loc_name      HTTP location name
 * @param ret           Return directive
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_ret_set(const char *ta,
                                                const char *inst_name,
                                                const char *srv_name,
                                                const char *loc_name,
                                                const char *ret);

/**
 * Get nginx HTTP location index file path
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param loc_name      HTTP location name
 * @param index         Location for index file
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_index_get(const char *ta,
                                                  const char *inst_name,
                                                  const char *srv_name,
                                                  const char *loc_name,
                                                  char **index);

/**
 * Set nginx HTTP location index file
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param loc_name      HTTP location name
 * @param index         Index file
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_index_set(const char *ta,
                                                  const char *inst_name,
                                                  const char *srv_name,
                                                  const char *loc_name,
                                                  const char *index);

/**
 * Get nginx HTTP location root path
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param loc_name      HTTP location name
 * @param root          Location for root path
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_root_get(const char *ta,
                                                 const char *inst_name,
                                                 const char *srv_name,
                                                 const char *loc_name,
                                                 char **root);

/**
 * Set nginx HTTP location root path
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param loc_name      HTTP location name
 * @param root          Root path
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_root_set(const char *ta,
                                                 const char *inst_name,
                                                 const char *srv_name,
                                                 const char *loc_name,
                                                 const char *root);

/**
 * Get nginx HTTP location proxy SSL settings name
 *
 * @param ta                Test Agent
 * @param inst_name         Nginx instance name
 * @param srv_name          Nginx server name
 * @param loc_name          HTTP location name
 * @param proxy_ssl_name    Location for SSL settings name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_proxy_ssl_name_get(
                                                const char *ta,
                                                const char *inst_name,
                                                const char *srv_name,
                                                const char *loc_name,
                                                char **proxy_ssl_name);

/**
 * Set nginx HTTP location proxy SSL settings name
 *
 * @param ta                Test Agent
 * @param inst_name         Nginx instance name
 * @param srv_name          Nginx server name
 * @param loc_name          HTTP location name
 * @param proxy_ssl_name    SSL settings name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_proxy_ssl_name_set(
                                                const char *ta,
                                                const char *inst_name,
                                                const char *srv_name,
                                                const char *loc_name,
                                                const char *proxy_ssl_name);

/**
 * Get nginx HTTP location proxy pass URL
 *
 * @param ta                Test Agent
 * @param inst_name         Nginx instance name
 * @param srv_name          Nginx server name
 * @param loc_name          HTTP location name
 * @param proxy_pass_url    Location for proxy pass URL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_proxy_pass_url_get(
                                                const char *ta,
                                                const char *inst_name,
                                                const char *srv_name,
                                                const char *loc_name,
                                                char **proxy_pass_url);

/**
 * Set nginx HTTP location proxy pass URL
 *
 * @param ta                Test Agent
 * @param inst_name         Nginx instance name
 * @param srv_name          Nginx server name
 * @param loc_name          HTTP location name
 * @param proxy_pass_url    Proxy pass URL
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_proxy_pass_url_set(
                                                const char *ta,
                                                const char *inst_name,
                                                const char *srv_name,
                                                const char *loc_name,
                                                const char *proxy_pass_url);

/**
 * Get nginx HTTP location proxy HTTP version
 *
 * @param ta                Test Agent
 * @param inst_name         Nginx instance name
 * @param srv_name          Nginx server name
 * @param loc_name          HTTP location name
 * @param proxy_http_vers   Location for HTTP version
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_proxy_http_vers_get(
                                                const char *ta,
                                                const char *inst_name,
                                                const char *srv_name,
                                                const char *loc_name,
                                                char **proxy_http_vers);

/**
 * Set nginx HTTP location proxy HTTP version
 *
 * @param ta                Test Agent
 * @param inst_name         Nginx instance name
 * @param srv_name          Nginx server name
 * @param loc_name          HTTP location name
 * @param proxy_http_vers   HTTP version
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_proxy_http_vers_set(
                                                const char *ta,
                                                const char *inst_name,
                                                const char *srv_name,
                                                const char *loc_name,
                                                const char *proxy_http_vers);

/**
 * Get nginx HTTP location SSL settings name
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param loc_name      HTTP location name
 * @param ssl_name      Location for SSL settings name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_ssl_name_get(const char *ta,
                                                     const char *inst_name,
                                                     const char *srv_name,
                                                     const char *loc_name,
                                                     char **ssl_name);

/**
 * Set nginx HTTP location SSL settings name
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param srv_name      Nginx server name
 * @param loc_name      HTTP location name
 * @param ssl_name      Location for SSL settings name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_loc_ssl_name_set(const char *ta,
                                                     const char *inst_name,
                                                     const char *srv_name,
                                                     const char *loc_name,
                                                     const char *ssl_name);

/**
 * Add nginx HTTP upstream group of servers
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param us_name       Upstream group name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_upstream_add(const char *ta,
                                                 const char *inst_name,
                                                 const char *us_name);

/**
 * Delete nginx HTTP upstream group of servers
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param us_name       Upstream group name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_upstream_del(const char *ta,
                                                 const char *inst_name,
                                                 const char *us_name);

/**
 * Get nginx HTTP upstream group number of keepalive connections
 *
 * @param ta                Test Agent
 * @param inst_name         Nginx instance name
 * @param us_name           Upstream group name
 * @param keepalive_num     Location for number of keepalive connections
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_upstream_keepalive_num_get(
                                                const char *ta,
                                                const char *inst_name,
                                                const char *us_name,
                                                unsigned int *keepalive_num);
/**
 * Set nginx HTTP upstream group number of keepalive connections
 *
 * @param ta                Test Agent
 * @param inst_name         Nginx instance name
 * @param us_name           Upstream group name
 * @param keepalive_num     Number of keepalive connections
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_upstream_keepalive_num_set(
                                                const char *ta,
                                                const char *inst_name,
                                                const char *us_name,
                                                unsigned int keepalive_num);
/**
 * Add server to upstream group
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param us_name       Upstream group name
 * @param srv_name      Server name
 * @param addr_spec     Server address specification,
 *                      e.g. IP-address:port or unix domain socket
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_us_server_add(const char *ta,
                                                  const char *inst_name,
                                                  const char *us_name,
                                                  const char *srv_name,
                                                  const char *addr_spec);

/**
 * Delete server from upstream group
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param us_name       Upstream group name
 * @param srv_name      Server name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_us_server_del(const char *ta,
                                                  const char *inst_name,
                                                  const char *us_name,
                                                  const char *srv_name);

/**
 * Get nginx HTTP upstream server weight
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param us_name       Upstream group name
 * @param srv_name      Server name
 * @param weight        Location for weight
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_us_server_weight_get(const char *ta,
                                                         const char *inst_name,
                                                         const char *us_name,
                                                         const char *srv_name,
                                                         unsigned int *weight);

/**
 * Set nginx HTTP upstream server weight
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param us_name       Upstream group name
 * @param srv_name      Server name
 * @param weight        Server weight
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_http_us_server_weight_set(const char *ta,
                                                         const char *inst_name,
                                                         const char *us_name,
                                                         const char *srv_name,
                                                         unsigned int weight);

/**
 * Add nginx ssl settings
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param ssl_name      Nginx ssl settings name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_ssl_add(const char *ta,
                                       const char *inst_name,
                                       const char *ssl_name);

/**
 * Del nginx ssl settings
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param ssl_name      Nginx ssl settings name
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_ssl_del(const char *ta,
                                       const char *inst_name,
                                       const char *ssl_name);

/**
 * Get nginx ssl certificate
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param ssl_name      Nginx ssl settings name
 * @param cert          Location for file path to certificate
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_ssl_cert_get(const char *ta,
                                            const char *inst_name,
                                            const char *ssl_name,
                                            char **cert);

/**
 * Set nginx ssl certificate
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param ssl_name      Nginx ssl settings name
 * @param cert          File path to certificate
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_ssl_cert_set(const char *ta,
                                            const char *inst_name,
                                            const char *ssl_name,
                                            const char *cert);

/**
 * Get nginx ssl key
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param ssl_name      Nginx ssl settings name
 * @param key           Location for file path to key
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_ssl_key_get(const char *ta,
                                           const char *inst_name,
                                           const char *ssl_name,
                                           char **key);

/**
 * Set nginx ssl key
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param ssl_name      Nginx ssl settings name
 * @param key           File path to key
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_ssl_key_set(const char *ta,
                                           const char *inst_name,
                                           const char *ssl_name,
                                           const char *key);

/**
 * Get nginx ssl ciphers
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param ssl_name      Nginx ssl settings name
 * @param ciphers       Location for ciphers list
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_ssl_ciphers_get(const char *ta,
                                               const char *inst_name,
                                               const char *ssl_name,
                                               char **ciphers);

/**
 * Set nginx ssl ciphers
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param ssl_name      Nginx ssl settings name
 * @param ciphers       Ciphers list
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_ssl_ciphers_set(const char *ta,
                                               const char *inst_name,
                                               const char *ssl_name,
                                               const char *ciphers);

/**
 * Get nginx ssl protocols
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param ssl_name      Nginx ssl settings name
 * @param protocols     Location for protocols list
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_ssl_protocols_get(const char *ta,
                                                 const char *inst_name,
                                                 const char *ssl_name,
                                                 char **protocols);

/**
 * Set nginx ssl protocols
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param ssl_name      Nginx ssl settings name
 * @param protocols     Protocols list
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_ssl_protocols_set(const char *ta,
                                                 const char *inst_name,
                                                 const char *ssl_name,
                                                 const char *protocols);

/**
 * Get nginx ssl session cache.
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param ssl_name      Nginx ssl settings name
 * @param cache         Location for session cache
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_ssl_session_cache_get(const char *ta,
                                                     const char *inst_name,
                                                     const char *ssl_name,
                                                     char **session_cache);

/**
 * Set nginx ssl session cache.
 *
 * Value: off | none | [builtin[:size]] [shared:name:size]
 *
 * @param ta            Test Agent
 * @param inst_name     Nginx instance name
 * @param ssl_name      Nginx ssl settings name
 * @param cache         Session cache
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_ssl_session_cache_set(const char *ta,
                                                     const char *inst_name,
                                                     const char *ssl_name,
                                                     const char *session_cache);

/**
 * Get nginx ssl session timeout
 *
 * @param ta                Test Agent
 * @param inst_name         Nginx instance name
 * @param ssl_name          Nginx ssl settings name
 * @param session_timeout   Location for session timeout
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_ssl_session_timeout_get(
                                           const char *ta,
                                           const char *inst_name,
                                           const char *ssl_name,
                                           unsigned int *session_timeout);

/**
 * Set nginx ssl session timeout
 *
 * @param ta                Test Agent
 * @param inst_name         Nginx instance name
 * @param ssl_name          Nginx ssl settings name
 * @param session_timeout   Session timeout
 *
 * @return Status code
 */
extern te_errno tapi_cfg_nginx_ssl_session_timeout_set(
                                               const char *ta,
                                               const char *inst_name,
                                               const char *ssl_name,
                                               unsigned int session_timeout);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_NGINX_H__ */
