/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI fio test
 *
 * Check TAPI fio
 */

/** @page tapi-tool-fio-fio Run fio and get report
 *
 * @objective Check tapi_fio
 *
 * @param numjobs   number of FIO jobs
 * @param ioengine  FIO I/O engine to use
 * @param rwtype    I/O pattern to use
 * @param runtime   run time in seconds
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME "fio"

#include "fio_suite.h"

int main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;

    tapi_job_factory_t *factory = NULL;

    tapi_fio *fio = NULL;
    tapi_fio_opts opts = TAPI_FIO_OPTS_DEFAULTS;
    tapi_fio_numjobs_t numjobs;
    tapi_fio_ioengine ioengine;
    tapi_fio_rwtype rwtype;
    int runtime;
    tarpc_off_t size;
    tapi_fio_report report;
    static const char filename_template[] = "te_tmp_fio_XXXXXX";
    char *filename = NULL;
    int fd = -1;

    TEST_START;

    TEST_GET_PCO(pco_iut);
    TEST_GET_FIO_IOENGINE_PARAM(ioengine);
    TEST_GET_FIO_RWTYPE_PARAM(rwtype);
    TEST_GET_FIO_NUMJOBS_PARAM(numjobs);
    TEST_GET_INT_PARAM(runtime);
    TEST_GET_INT64_PARAM(size);

    opts.ioengine = ioengine;
    opts.rwtype = rwtype;
    opts.numjobs = numjobs;
    opts.runtime_sec = runtime;

    TEST_STEP("Create a temporary file");
    fd = rpc_mkstemp(pco_iut, filename_template, &filename);
    rpc_ftruncate(pco_iut, fd, size);
    rpc_close(pco_iut, fd);
    fd = -1;
    opts.filename = filename;

    TEST_STEP("Initialize tapi_job_factory on pco_iut");
    CHECK_RC(tapi_job_factory_rpc_create(pco_iut, &factory));

    TEST_STEP("Initialize FIO");
    CHECK_NOT_NULL(fio = tapi_fio_create(&opts, factory, "fio"));

    TEST_STEP("Start fio");
    CHECK_RC(tapi_fio_start(fio));

    TEST_STEP("Wait for fio completion");
    CHECK_RC(tapi_fio_wait(fio, TAPI_FIO_TIMEOUT_DEFAULT));

    TEST_STEP("Get report");
    CHECK_RC(tapi_fio_get_report(fio, &report));
    tapi_fio_log_report(&report);

    TEST_SUCCESS;

cleanup:

    if (fd > 0)
        rpc_close(pco_iut, fd);
    if (filename != NULL)
    {
        rpc_unlink(pco_iut, filename);
        free(filename);
    }

    tapi_fio_destroy(fio);
    tapi_job_factory_destroy(factory);

    TEST_END;
}
