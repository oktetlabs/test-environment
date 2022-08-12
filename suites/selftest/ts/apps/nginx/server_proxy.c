/** @file
 * @brief Nginx demo test suite
 *
 * Configure pair of proxy and server nginx instances and test it
 * using HTTP benchmarking tool.
 *
 * Copyright (C) 2019-2022 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 *
 */

#ifndef DOXYGEN_TEST_SPEC

/* Logging subsystem entity name */
#define TE_TEST_NAME  "server_proxy"

#include "te_config.h"

#include "logger_api.h"
#include "tapi_test.h"
#include "tapi_file.h"
#include "tapi_wrk.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg_nginx.h"
#include "tapi_rpc_signal.h"
#include "tapi_rpc_stdio.h"
#include "tapi_rpc_unistd.h"

#define SRV_NAME   "server"
#define PROXY_NAME "proxy"

#define DFLT_NAME        "dflt"
#define SRV_ADDR         "127.0.0.1"
#define SRV_PORT_START   1050
#define PROXY_ADDR_SPEC  "127.0.0.1:8111"

#define SRV_LOC_ROOT_NAME       "root"
#define SRV_LOC_ROOT_URI        "/"
#define SRV_LOC_ROOT_PATH       "/tmp/share"
#define SRV_LOC_ROOT_INDEX      "index.html"
#define SRV_LOC_ROOT_FILENAME   "download"
#define SRV_LOC_RET_NAME        "return"
#define SRV_LOC_RET_URI         "= /upload"
#define SRV_LOC_RET_STR         "200 'Thank you'"

#define PROXY_US_NAME           "backend"
#define PROXY_LOC_NAME          "proxy"
#define PROXY_LOC_URI           "/"
#define PROXY_LOC_PASS_URL      "http://" PROXY_US_NAME
#define PROXY_PORT_PAIRS_NUM    40

#define NGINX_WRK_PROC_NUM      2
#define NGINX_WRK_RLIMIT_NOFILE 1048576
#define NGINX_EVT_WRK_CONN_NUM  200000
#define NGINX_EVT_METHOD        "epoll"

#define NAME_LEN_MAX    16
#define VALUE_LEN_MAX   256

static void
nginx_setup_common(const char *ta, const char *inst_name)
{
    CHECK_RC(tapi_cfg_nginx_add(ta, inst_name));

    CHECK_RC(tapi_cfg_nginx_wrk_ps_num_set(ta, inst_name,
                                           NGINX_WRK_PROC_NUM));
    CHECK_RC(tapi_cfg_nginx_wrk_cpu_aff_mode_set(ta, inst_name,
                                                 TE_NGINX_CPU_AFF_MODE_AUTO));
    CHECK_RC(tapi_cfg_nginx_wrk_rlimit_nofile_set(ta, inst_name,
                                                  NGINX_WRK_RLIMIT_NOFILE));
    CHECK_RC(tapi_cfg_nginx_evt_wrk_conn_num_set(ta, inst_name,
                                                 NGINX_EVT_WRK_CONN_NUM));
    CHECK_RC(tapi_cfg_nginx_evt_method_set(ta, inst_name, NGINX_EVT_METHOD));

    CHECK_RC(tapi_cfg_nginx_http_server_add(ta, inst_name, DFLT_NAME));

#ifndef DEBUG
    CHECK_RC(tapi_cfg_nginx_error_log_disable(ta, inst_name));
    CHECK_RC(tapi_cfg_nginx_http_server_access_log_disable(ta, inst_name,
                                                           DFLT_NAME));
#endif
}

static void
nginx_setup_proxy_port_pairs(const char *ta_srv, const char *ta_proxy, int num)
{
    int         i;
    uint16_t    port = SRV_PORT_START;
    char        id[NAME_LEN_MAX];
    char        addr_spec[VALUE_LEN_MAX];

    for (i = 0; i < num; i++, port++)
    {
        TE_SPRINTF(id, "%d", i);
        TE_SPRINTF(addr_spec, "%s:%u", SRV_ADDR, port);

        CHECK_RC(tapi_cfg_nginx_http_listen_entry_add(ta_srv, SRV_NAME,
                                                      DFLT_NAME, id,
                                                      addr_spec));

        CHECK_RC(tapi_cfg_nginx_http_listen_entry_reuseport_enable(
                                                        ta_srv, SRV_NAME,
                                                        DFLT_NAME, id));

        CHECK_RC(tapi_cfg_nginx_http_us_server_add(ta_proxy, PROXY_NAME,
                                                   PROXY_US_NAME, id,
                                                   addr_spec));
    }
}

static void
share_put_file(rcf_rpc_server *rpcs, const char *filepath)
{
    char buf[RPC_SHELL_CMDLINE_MAX];
    te_string cmd = TE_STRING_BUF_INIT(buf);

    RPC_AWAIT_IUT_ERROR(rpcs);
    if (rpc_access(rpcs, SRV_LOC_ROOT_PATH, RPC_F_OK) == -1)
        rpc_mkdir(rpcs, SRV_LOC_ROOT_PATH, RPC_S_IRWXU | RPC_S_IRWXO);

    CHECK_RC(rcf_ta_put_file(rpcs->ta, 0, filepath,
                             SRV_LOC_ROOT_PATH "/" SRV_LOC_ROOT_FILENAME));

    CHECK_RC(te_string_append(&cmd, "chmod go+r %s",
                              SRV_LOC_ROOT_PATH "/" SRV_LOC_ROOT_FILENAME));

    rpc_system(rpcs, cmd.ptr);
}

static void
share_cleanup(rcf_rpc_server *rpcs)
{
    char buf[RPC_SHELL_CMDLINE_MAX];
    te_string cmd = TE_STRING_BUF_INIT(buf);

    CHECK_RC(te_string_append(&cmd, "rm -rf %s", SRV_LOC_ROOT_PATH));

    rpc_system(rpcs, cmd.ptr);
}

#ifdef DEBUG
static void
dump_logs(const char *ta, const char *inst_name)
{
    char *error_log_path = NULL;
    char *error_log = NULL;
    char *access_log_path = NULL;
    char *access_log = NULL;

    CHECK_RC(tapi_cfg_nginx_error_log_path_get(ta, inst_name,
                                               &error_log_path));
    CHECK_RC(tapi_file_read_ta(ta, error_log_path, &error_log));
    RING("Nginx '%s/%s' error log:\n%s", ta, inst_name, error_log);

    free(error_log_path);
    free(error_log);

    CHECK_RC(tapi_cfg_nginx_http_server_access_log_path_get(ta, inst_name,
                                                            DFLT_NAME,
                                                            &access_log_path));
    CHECK_RC(tapi_file_read_ta(ta, access_log_path, &access_log));
    RING("Nginx '%s/%s' nginx access log:\n%s", ta, inst_name, access_log);

    free(access_log_path);
    free(access_log);
}

static void
dump_config(const char *ta, const char *inst_name)
{
    char *config_path = NULL;
    char *config = NULL;

    CHECK_RC(tapi_cfg_nginx_config_path_get(ta, inst_name, &config_path));
    CHECK_RC(tapi_file_read_ta(ta, config_path, &config));
    RING("Nginx '%s/%s' config:\n%s", ta, inst_name, config);

    free(config_path);
    free(config);
}
#endif

int
main(int argc, char **argv)
{
    const char         *ta_srv = "Agt_A";
    const char         *ta_proxy = "Agt_B";
    rcf_rpc_server     *pco_srv = NULL;
    rcf_rpc_server     *pco_proxy = NULL;
    char               *filepath = NULL;
    int                 upstreams_num;
    int                 payload_size;
    int                 conns_num;
    int                 threads_num;
    int                 duration;
    tapi_wrk_app       *app = NULL;
    tapi_wrk_opt        opt = tapi_wrk_default_opt;
    tapi_wrk_report     report;
    tapi_job_factory_t *factory = NULL;

    TEST_START;

    TEST_GET_RPCS(ta_srv, "pco_srv", pco_srv);
    TEST_GET_RPCS(ta_proxy, "pco_proxy", pco_proxy);

    TEST_GET_INT_PARAM(upstreams_num);
    TEST_GET_INT_PARAM(payload_size);
    TEST_GET_INT_PARAM(conns_num);
    TEST_GET_INT_PARAM(threads_num);
    TEST_GET_INT_PARAM(duration);

    TEST_STEP("Prepare server files");
    filepath = tapi_file_create_pattern(payload_size, 'X');
    if (filepath == NULL)
        TEST_FAIL("Failed to create payload file");

    share_put_file(pco_srv, filepath);

    TEST_STEP("Configure nginx daemons");

    TEST_SUBSTEP("Configure nginx server");
    nginx_setup_common(ta_srv, SRV_NAME);

    CHECK_RC(tapi_cfg_nginx_http_loc_add(ta_srv, SRV_NAME, DFLT_NAME,
                                         SRV_LOC_ROOT_NAME, SRV_LOC_ROOT_URI));
    CHECK_RC(tapi_cfg_nginx_http_loc_root_set(ta_srv, SRV_NAME, DFLT_NAME,
                                              SRV_LOC_ROOT_NAME,
                                              SRV_LOC_ROOT_PATH));
    CHECK_RC(tapi_cfg_nginx_http_loc_index_set(ta_srv, SRV_NAME, DFLT_NAME,
                                               SRV_LOC_ROOT_NAME,
                                               SRV_LOC_ROOT_INDEX));

    CHECK_RC(tapi_cfg_nginx_http_loc_add(ta_srv, SRV_NAME, DFLT_NAME,
                                         SRV_LOC_RET_NAME, SRV_LOC_RET_URI));
    CHECK_RC(tapi_cfg_nginx_http_loc_ret_set(ta_srv, SRV_NAME, DFLT_NAME,
                                             SRV_LOC_RET_NAME,
                                             SRV_LOC_RET_STR));

    TEST_SUBSTEP("Configure nginx proxy");
    nginx_setup_common(ta_proxy, PROXY_NAME);

    CHECK_RC(tapi_cfg_nginx_http_listen_entry_add(ta_proxy, PROXY_NAME,
                                                  DFLT_NAME, DFLT_NAME,
                                                  PROXY_ADDR_SPEC));
    CHECK_RC(tapi_cfg_nginx_http_listen_entry_reuseport_enable(
                                            ta_proxy, PROXY_NAME,
                                            DFLT_NAME, DFLT_NAME));

    CHECK_RC(tapi_cfg_nginx_http_loc_add(ta_proxy, PROXY_NAME, DFLT_NAME,
                                         PROXY_LOC_NAME, PROXY_LOC_URI));
    CHECK_RC(tapi_cfg_nginx_http_loc_proxy_pass_url_set(ta_proxy, PROXY_NAME,
                                                        DFLT_NAME,
                                                        PROXY_LOC_NAME,
                                                        PROXY_LOC_PASS_URL));

    CHECK_RC(tapi_cfg_nginx_http_upstream_add(ta_proxy, PROXY_NAME,
                                              PROXY_US_NAME));

    TEST_SUBSTEP("Configure port pairs for proxying");
    nginx_setup_proxy_port_pairs(ta_srv, ta_proxy, upstreams_num);

    TEST_STEP("Start nginx processes");
    CHECK_RC(tapi_cfg_nginx_enable(ta_srv, SRV_NAME));
    CHECK_RC(tapi_cfg_nginx_enable(ta_proxy, PROXY_NAME));

    TEST_STEP("Run HTTP benchmarking test");
    opt.connections = conns_num;
    opt.duration_s = duration;
    opt.n_threads = threads_num;
    opt.host = "http://" PROXY_ADDR_SPEC "/" SRV_LOC_ROOT_FILENAME;

    CHECK_RC(tapi_job_factory_rpc_create(pco_proxy, &factory));
    CHECK_RC(tapi_wrk_create(factory, &opt, &app));
    CHECK_RC(tapi_wrk_start(app));
    CHECK_RC(tapi_wrk_wait(app, TE_SEC2MS(duration + 1)));
    CHECK_RC(tapi_wrk_get_report(app, &report));

    TEST_ARTIFACT("Average latency %.fus", report.thread_latency.mean);

#ifdef DEBUG
    dump_config(ta_srv, SRV_NAME);
    dump_config(ta_proxy, PROXY_NAME);
    dump_logs(ta_srv, SRV_NAME);
    dump_logs(ta_proxy, PROXY_NAME);
#endif

    TEST_STEP("Stop nginx processes");
    CHECK_RC(tapi_cfg_nginx_disable(ta_srv, SRV_NAME));
    CHECK_RC(tapi_cfg_nginx_disable(ta_proxy, PROXY_NAME));

    TEST_SUCCESS;

cleanup:
    tapi_wrk_destroy(app);
    tapi_job_factory_destroy(factory);

    if (pco_srv != NULL)
        share_cleanup(pco_srv);

    if (filepath != NULL)
    {
        unlink(filepath);
        free(filepath);
    }
    TEST_END;
}

#endif /* !DOXYGEN_TEST_SPEC */
