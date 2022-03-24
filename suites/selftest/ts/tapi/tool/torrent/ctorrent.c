/** @file
 * @brief TAPI ctorrent test
 *
 * Demonstrate the usage of TAPI ctorrent and TAPI bttrack
 *
 * Copyright (C) 2020 OKTET Labs. All rights reserved.
 */

/** @page tapi-tool-torrent-ctorrent Transfer file via tapi_ctorrent
 *
 * @objective Check tapi_ctorrent and tapi_bttrack implementation
 *
 * @par Scenario:
 *
 * @author Andrey Izrailev <Andrey.Izrailev@oktetlabs.ru>
 */

#define TE_TEST_NAME "ctorrent"

#include "ctorrent.h"
#include "tapi_ctorrent.h"
#include "tapi_job_factory_rpc.h"
#include "tapi_test.h"
#include "tapi_env.h"
#include "tapi_sockaddr.h"
#include "tapi_file.h"

#define TMP_DIR "/tmp"

void
generate_file_names(char *metainfo_iut, char *metainfo_tst, char *source_file,
                    char *dest_file)
{
    TE_SNPRINTF(metainfo_iut, RCF_MAX_PATH, "%s/%s_iut.torrent", TMP_DIR,
               tapi_file_generate_name());
    TE_SNPRINTF(metainfo_tst, RCF_MAX_PATH, "%s/%s_tst.torrent", TMP_DIR,
                tapi_file_generate_name());
    TE_SNPRINTF(source_file, RCF_MAX_PATH, "%s/%s_source", TMP_DIR,
                tapi_file_generate_name());
    TE_SNPRINTF(dest_file, RCF_MAX_PATH, "%s/%s_dest", TMP_DIR,
                tapi_file_generate_name());
}

void
check_dest_content(const char *dest_ta, const char *dest_file,
                   const char *content)
{
    char *dest_content = NULL;
    char *err_msg      = NULL;

    if (tapi_file_read_ta(dest_ta, dest_file, &dest_content) != 0)
        err_msg = "Failed to read destination file content";
    else if (strcmp(content, dest_content) != 0)
        err_msg = "Contents of source and destination files do not match";

    free(dest_content);

    if (err_msg == NULL)
        RING("The contents are equal");
    else
        TEST_FAIL(err_msg);
}

void
cleanup_unlink_file(const char *ta, const char *file)
{
    if (tapi_file_ta_unlink_fmt(ta, file) != 0)
        ERROR("Failed to remove file '%s' from TA '%s'", file, ta);
}

int
main(int argc, char **argv)
{
    rcf_rpc_server         *pco_iut     = NULL;
    rcf_rpc_server         *pco_tst     = NULL;
    const struct sockaddr  *iut_addr    = NULL;
    const char             *iut_ip      = NULL;
    tapi_job_factory_t     *factory_iut = NULL;
    tapi_job_factory_t     *factory_tst = NULL;

    tapi_bttrack_app  *tracker = NULL;
    tapi_bttrack_opt   opt     = tapi_bttrack_default_opt;

    tapi_ctorrent_app *app_iut = NULL;
    tapi_ctorrent_app *app_tst = NULL;
    tapi_ctorrent_opt  opt_iut = tapi_ctorrent_default_opt;
    tapi_ctorrent_opt  opt_tst = tapi_ctorrent_default_opt;

    char metainfo_iut[RCF_MAX_PATH];
    char metainfo_tst[RCF_MAX_PATH];
    char source_file[RCF_MAX_PATH];
    char dest_file[RCF_MAX_PATH];

    const char *content = "Source file content";

    te_bool  completed;
    te_errno rc_create_metainfo;
    te_errno rc_tracker_wait;

    TEST_START;

    generate_file_names(metainfo_iut, metainfo_tst, source_file, dest_file);

    TEST_GET_PCO(pco_iut);
    TEST_GET_PCO(pco_tst);

    TEST_STEP("Initialize factory on pco_iut");
    CHECK_RC(tapi_job_factory_rpc_create(pco_iut, &factory_iut));

    TEST_STEP("Initialize factory on pco_tst");
    CHECK_RC(tapi_job_factory_rpc_create(pco_tst, &factory_tst));

    TEST_STEP("Get IUT ip");
    TEST_GET_ADDR(pco_iut, iut_addr);
    iut_ip = te_sockaddr_get_ipstr(iut_addr);
    if (iut_ip == NULL)
        TEST_FAIL("Failed to get pco_iut ip address");

    TEST_STEP("Create torrent tracker on pco_iut");
    opt.dfile = te_string_fmt("%s/%s_dfile", TMP_DIR,
                              tapi_file_generate_name());
    if (opt.dfile == NULL)
        TEST_FAIL("Failed to make a string with dfile name");
    CHECK_RC(tapi_bttrack_create(factory_iut, iut_ip, &opt, &tracker));
    free(opt.dfile);

    TEST_STEP("Start the torrent tracker");
    CHECK_RC(tapi_bttrack_start(tracker));

    TEST_STEP("Check that torrent tracker is running");
    rc_tracker_wait = tapi_bttrack_wait(tracker, TE_SEC2MS(10));
    if (TE_RC_GET_ERROR(rc_tracker_wait) != TE_EINPROGRESS)
        TEST_FAIL("Torrent tracker is not running");

    TEST_STEP("Create a file to be transferred");
    if (tapi_file_create_ta(pco_iut->ta, source_file, content) != 0)
        TEST_FAIL("Failed to create a file to be transferred");

    TEST_STEP("Create metainfo file on iut");
    rc_create_metainfo = tapi_ctorrent_create_metainfo_file(factory_iut,
                                                            tracker,
                                                            metainfo_iut,
                                                            source_file,
                                                            -1);
    if (TE_RC_GET_ERROR(rc_create_metainfo) != TE_EEXIST)
    {
        if (rc_create_metainfo != 0)
        {
            TEST_FAIL("Failed to create metainfo file, error %r",
                      rc_create_metainfo);
        }
    }
    else
    {
        WARN("The file already exists");
        TEST_SUBSTEP("Remove the file");
        if (tapi_file_ta_unlink_fmt(pco_iut->ta, metainfo_iut) != 0)
            TEST_FAIL("Failed to remove the file");

        TEST_SUBSTEP("Create the metainfo file again");
        CHECK_RC(tapi_ctorrent_create_metainfo_file(factory_iut, tracker,
                                            metainfo_iut, source_file, -1));
    }

    TEST_STEP("Copy the file to tst");
    if (tapi_file_copy_ta(pco_iut->ta, metainfo_iut,
                          pco_tst->ta, metainfo_tst) != 0)
        TEST_FAIL("Failed to copy the file");

    TEST_STEP("Create ctorrent app on iut");
    opt_iut.metainfo_file = metainfo_iut;
    opt_iut.save_to_file  = source_file;
    CHECK_RC(tapi_ctorrent_create_app(factory_iut, &opt_iut, &app_iut));

    TEST_STEP("Start seeding on iut");
    CHECK_RC(tapi_ctorrent_start(app_iut));

    TEST_STEP("Create ctorrent app on tst");
    opt_tst.metainfo_file = metainfo_tst;
    opt_tst.save_to_file  = dest_file;
    CHECK_RC(tapi_ctorrent_create_app(factory_tst, &opt_tst, &app_tst));

    TEST_STEP("Start downloading on tst");
    CHECK_RC(tapi_ctorrent_start(app_tst));

    VSLEEP(10, "Wait for some time");

    TEST_STEP("Check completion");
    CHECK_RC(tapi_ctorrent_check_completion(app_tst,
                                            TE_SEC2MS(60), &completed));
    if (completed)
    {
        RING("The download is completed");
        TEST_STEP("Check that contents of source and destination files match");
        check_dest_content(pco_tst->ta, dest_file, content);

        TEST_SUCCESS;
    }
    else
    {
        RING("The download is not completed");
    }

    TEST_STEP("Stop ctorrent on tst");
    CHECK_RC(tapi_ctorrent_stop(app_tst, TE_SEC2MS(10)));

    VSLEEP(5, "Do not download on tst");

    TEST_STEP("Continue downloading on tst");
    CHECK_RC(tapi_ctorrent_start(app_tst));

    TEST_STEP("Wait for completion");
    CHECK_RC(tapi_ctorrent_wait_completion(app_tst, TE_SEC2MS(60)));

    TEST_STEP("Check that contents of source and destination files match");
    check_dest_content(pco_tst->ta, dest_file, content);

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tapi_bttrack_destroy(tracker));
    CLEANUP_CHECK_RC(tapi_ctorrent_destroy(app_iut, TE_SEC2MS(10)));
    CLEANUP_CHECK_RC(tapi_ctorrent_destroy(app_tst, TE_SEC2MS(10)));

    tapi_job_factory_destroy(factory_iut);
    tapi_job_factory_destroy(factory_tst);

    cleanup_unlink_file(pco_iut->ta, metainfo_iut);
    cleanup_unlink_file(pco_tst->ta, metainfo_tst);
    cleanup_unlink_file(pco_iut->ta, source_file);
    cleanup_unlink_file(pco_tst->ta, dest_file);

    TEST_END;
}
