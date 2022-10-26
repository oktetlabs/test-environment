/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief RPC Test Suite
 *
 * Check RPC ioctls.
 */

/** @page ioctl Test for RPC ioctls
 *
 * @objective Check that RPC ioctls are not broken.
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "unistd/ioctl_blkflsbuf"

#include "unistd_suite.h"

#define BUFSIZE 64

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;
    char *tmp_name = NULL;
    int fd = -1;
    int status;

    TEST_START;
    TEST_GET_PCO(pco_iut);

    TEST_STEP("Create a temporary file");
    fd = rpc_mkstemp(pco_iut, "te_ioctl_XXXXXX", &tmp_name);
    rpc_unlink(pco_iut, tmp_name);
    free(tmp_name);

    TEST_STEP("Perform an ioctl on FD");

    /*
     * FIXME: test it actually works once loop block device
     * support is ready in TE.
     */
    RPC_AWAIT_ERROR(pco_iut);
    status = rpc_ioctl(pco_iut, fd, RPC_BLKFLSBUF, NULL);
    if (status == 0)
        TEST_VERDICT("ioctl() expected to fail but it succeeded");
    CHECK_RPC_ERRNO(pco_iut, RPC_ENOTTY, "ioctl(BLKFLSBUF)");

    TEST_SUCCESS;

cleanup:

    if (fd > 0)
        rpc_close(pco_iut, fd);

    TEST_END;
}
