/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TAPI ctorrent test
 *
 * Demonstrate the usage of TAPI ctorrent and TAPI bttrack
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

/** @page tapi-tool-torrent-ctorrent Transfer file via tapi_ctorrent
 *
 * @objective Check tapi_ctorrent and tapi_bttrack implementation
 *
 * @par Scenario:
 *
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
generate_file_names(te_string *metainfo_iut, te_string *metainfo_tst,
                    te_string *source_file, te_string *dest_file)
{
    tapi_file_make_custom_pathname(metainfo_iut, TMP_DIR, "_iut.torrent");
    tapi_file_make_custom_pathname(metainfo_tst, TMP_DIR, "_tst.torrent");
    tapi_file_make_custom_pathname(source_file, TMP_DIR, "_source");
    tapi_file_make_custom_pathname(dest_file, TMP_DIR, "_dest");
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
    if (tapi_file_ta_unlink_fmt(ta, "%s", file) != 0)
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

    te_string metainfo_iut = TE_STRING_INIT;
    te_string metainfo_tst = TE_STRING_INIT;
    te_string source_file = TE_STRING_INIT;
    te_string dest_file = TE_STRING_INIT;
    te_string dfile = TE_STRING_INIT;

    const char *content = "Source file content";

    bool completed;
    te_errno rc_create_metainfo;
    te_errno rc_tracker_wait;

    TEST_START;

    generate_file_names(&metainfo_iut, &metainfo_tst, &source_file, &dest_file);

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
    tapi_file_make_custom_pathname(&dfile, TMP_DIR, "_dfile");
    opt.dfile = dfile.ptr;
    CHECK_RC(tapi_bttrack_create(factory_iut, iut_ip, &opt, &tracker));

    TEST_STEP("Start the torrent tracker");
    CHECK_RC(tapi_bttrack_start(tracker));

    TEST_STEP("Check that torrent tracker is running");
    rc_tracker_wait = tapi_bttrack_wait(tracker, TE_SEC2MS(10));
    if (TE_RC_GET_ERROR(rc_tracker_wait) != TE_EINPROGRESS)
        TEST_FAIL("Torrent tracker is not running");

    TEST_STEP("Create a file to be transferred");
    if (tapi_file_create_ta(pco_iut->ta, source_file.ptr, "%s", content) != 0)
        TEST_FAIL("Failed to create a file to be transferred");

    TEST_STEP("Create metainfo file on iut");
    rc_create_metainfo = tapi_ctorrent_create_metainfo_file(factory_iut,
                                                            tracker,
                                                            metainfo_iut.ptr,
                                                            source_file.ptr,
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
        if (tapi_file_ta_unlink_fmt(pco_iut->ta, "%s", metainfo_iut.ptr) != 0)
            TEST_FAIL("Failed to remove the file");

        TEST_SUBSTEP("Create the metainfo file again");
        CHECK_RC(tapi_ctorrent_create_metainfo_file(factory_iut, tracker,
                                                    metainfo_iut.ptr,
                                                    source_file.ptr, -1));
    }

    TEST_STEP("Copy the file to tst");
    if (tapi_file_copy_ta(pco_iut->ta, metainfo_iut.ptr,
                          pco_tst->ta, metainfo_tst.ptr) != 0)
        TEST_FAIL("Failed to copy the file");

    TEST_STEP("Create ctorrent app on iut");
    opt_iut.metainfo_file = metainfo_iut.ptr;
    opt_iut.save_to_file  = source_file.ptr;
    CHECK_RC(tapi_ctorrent_create_app(factory_iut, &opt_iut, &app_iut));

    TEST_STEP("Start seeding on iut");
    CHECK_RC(tapi_ctorrent_start(app_iut));

    TEST_STEP("Create ctorrent app on tst");
    opt_tst.metainfo_file = metainfo_tst.ptr;
    opt_tst.save_to_file  = dest_file.ptr;
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
        check_dest_content(pco_tst->ta, dest_file.ptr, content);

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
    check_dest_content(pco_tst->ta, dest_file.ptr, content);

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(tapi_bttrack_destroy(tracker));
    CLEANUP_CHECK_RC(tapi_ctorrent_destroy(app_iut, TE_SEC2MS(10)));
    CLEANUP_CHECK_RC(tapi_ctorrent_destroy(app_tst, TE_SEC2MS(10)));

    tapi_job_factory_destroy(factory_iut);
    tapi_job_factory_destroy(factory_tst);

    cleanup_unlink_file(pco_iut->ta, metainfo_iut.ptr);
    cleanup_unlink_file(pco_tst->ta, metainfo_tst.ptr);
    cleanup_unlink_file(pco_iut->ta, source_file.ptr);
    cleanup_unlink_file(pco_tst->ta, dest_file.ptr);

    te_string_free(&metainfo_iut);
    te_string_free(&metainfo_tst);
    te_string_free(&source_file);
    te_string_free(&dest_file);

    TEST_END;
}
