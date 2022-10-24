/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2022 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief RPC Test Suite
 *
 * Truncate a file on the agent.
 */

/** @page truncate Testing file truncation.
 *
 * @objective Test the implementation of truncate/ftruncate RPC.
 *
 * @param use_ftruncate  use ftruncate() if @c TRUE; otherwise truncate()
 * @param length         the length to set
 * @param trail_size     the size of a trailing chunk to check for zeroes
 *
 * @par Scenario:
 *
 */

#define TE_TEST_NAME    "truncate"

#include "unistd_suite.h"

int
main(int argc, char **argv)
{
    rcf_rpc_server *pco_iut = NULL;

    tarpc_off_t length = 0;
    tarpc_off_t trail_size;
    te_bool use_ftruncate;

    static const char file_template[] = "te_trunc_XXXXXX";
    char *filename = NULL;
    int fd = -1;

    rpc_stat stat;
    tarpc_off_t offset;
    uint8_t *buf = NULL;
    int i;

    TEST_START;
    TEST_GET_PCO(pco_iut);
    TEST_GET_VALUE_BIN_UNIT_PARAM(length);
    TEST_GET_INT_PARAM(trail_size);
    TEST_GET_BOOL_PARAM(use_ftruncate);

    buf = tapi_calloc(1, trail_size);

    TEST_STEP("Create a file on TA");
    fd = rpc_mkstemp(pco_iut, file_template, &filename);

    TEST_STEP("Expand the file");
    if (use_ftruncate)
        rpc_ftruncate(pco_iut, fd, length);
    else
        rpc_truncate(pco_iut, filename, length);

    TEST_STEP("Check the resulting size");
    rpc_fstat(pco_iut, fd, &stat);
    if ((tarpc_off_t)stat.st_size != length)
    {
        TEST_VERDICT("File has not been expanded: expected %llu, got %llu",
                     (unsigned long long)length,
                     (unsigned long long)stat.st_size);
    }

    TEST_STEP("Check trailing zeroes");
    offset = rpc_lseek(pco_iut, fd, -trail_size, RPC_SEEK_END);
    if (offset != length - trail_size)
    {
        TEST_VERDICT("Invalid seek: should be at %llu - %llu, "
                     "got %ll", length - trail_size, offset);
    }
    CHECK_LENGTH(rpc_read(pco_iut, fd, buf, trail_size), trail_size);
    for (i = 0; i < trail_size; i++)
    {
        if (buf[i] != 0)
        {
            TEST_VERDICT("Byte at -%llu is not zero: %02x",
                         trail_size - i, buf[i]);
        }
    }

    TEST_STEP("Shrink the file");
    if (use_ftruncate)
        rpc_ftruncate(pco_iut, fd, length / 2);
    else
        rpc_truncate(pco_iut, filename, length / 2);

    TEST_STEP("Check the resulting size");
    rpc_fstat(pco_iut, fd, &stat);
    if ((tarpc_off_t)stat.st_size != length / 2)
    {
        TEST_VERDICT("File has not been shrunk: expected %llu, got %llu",
                     (unsigned long long)length / 2,
                     (unsigned long long)stat.st_size);
    }


    TEST_SUCCESS;

cleanup:

    if (fd > 0)
        rpc_close(pco_iut, fd);
    if (filename != NULL)
        rpc_unlink(pco_iut, filename);
    free(buf);

    TEST_END;
}
