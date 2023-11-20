/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI HAProxy test
 *
 * Test the usage of HAProxy TAPI.
 */

#define TE_TEST_NAME "haproxy"

#include "haproxy.h"
#include "tapi_test.h"
#include "tapi_env.h"
#include "tapi_haproxy.h"
#include "tapi_haproxy_cfg.h"
#include "tapi_job.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_file.h"
#include "te_sockaddr.h"

#define BACKEND_EXAMPLE_ADDR "127.0.0.1"
#define SRV_PORT_START 1050

static te_errno
log_file(const char *ta, const char *filename)
{
    char *buf = NULL;
    te_errno rc;

    rc = tapi_file_read_ta(ta, filename, &buf);
    if (rc != 0)
    {
        ERROR("%s(): failed to read '%s' on '%s': %r", __FUNCTION__, filename,
              ta, rc);
        return rc;
    }

    RING("%s", buf);
    free(buf);

    return 0;
}

int
main(int argc, char **argv)
{
    rcf_rpc_server             *iut_rpcs = NULL;
    const struct sockaddr      *iut_addr = NULL;
    tapi_job_factory_t         *factory = NULL;
    tapi_haproxy_app           *app = NULL;
    tapi_haproxy_opt            opt = tapi_haproxy_default_opt;
    tapi_haproxy_cfg_opt        cfg_opt = tapi_haproxy_cfg_default_opt;
    tapi_haproxy_cfg_backend   *backends = NULL;
    unsigned int                threads_num;
    unsigned int                backends_num;
    unsigned int                i;

    TEST_START;

    TEST_GET_PCO(iut_rpcs);
    TEST_GET_ADDR(iut_rpcs, iut_addr);

    TEST_GET_UINT_PARAM(threads_num);
    TEST_GET_UINT_PARAM(backends_num);

    TEST_STEP("Configure HAProxy");
    cfg_opt.nbthread = TAPI_JOB_OPT_UINT_VAL(threads_num);
    cfg_opt.frontend.name = "MyFrontend";
    cfg_opt.frontend.frontend_addr.addr = te_sockaddr_get_ipstr(iut_addr);
    cfg_opt.frontend.frontend_addr.port =
        TAPI_JOB_OPT_UINT_VAL(ntohs(te_sockaddr_get_port(iut_addr)));

    backends = TE_ALLOC(sizeof(tapi_haproxy_cfg_backend) * backends_num);
    for (i = 0; i < backends_num; i++)
    {
        backends[i].name = te_string_fmt("WebServer%d", SRV_PORT_START + i);
        backends[i].backend_addr.addr = BACKEND_EXAMPLE_ADDR;
        backends[i].backend_addr.port =
            TAPI_JOB_OPT_UINT_VAL(SRV_PORT_START + i);
    }
    cfg_opt.backend.name = "MyBackend";
    cfg_opt.backend.n = backends_num;
    cfg_opt.backend.backends = backends;

    opt.cfg_opt = &cfg_opt;
    opt.verbose = TRUE;

    TEST_STEP("Start HAProxy on IUT");
    CHECK_RC(tapi_job_factory_rpc_create(iut_rpcs, &factory));
    CHECK_RC(tapi_haproxy_create(factory, &opt, &app));
    TEST_STEP_PUSH_INFO("Begin of HAProxy configuration file");
    CHECK_RC(log_file(iut_rpcs->ta, app->generated_cfg_file));
    TEST_STEP_POP_INFO("End of HAProxy configuration file");
    CHECK_RC(tapi_haproxy_start(app));
    VSLEEP(2, "Wait for HAProxy");

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tapi_haproxy_destroy(app));
    tapi_job_factory_destroy(factory);

    if (backends != NULL)
    {
        for (i = 0; i < backends_num; i++)
            free(backends[i].name);
        free(backends);
    }

    TEST_END;
}