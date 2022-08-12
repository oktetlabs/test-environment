/** @file
 * @brief Test for pread() and pwrite() functions
 *
 * Testing if rpc_pread() and rpc_pwrite() work correctly
 *
 * Copyright (C) 2022-2022 OKTET Labs. All rights reserved.
 */

#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "pread_pwrite.h"
#include "te_config.h"
#include "tapi_test.h"
#include "te_file.h"
#include "rcf_rpc.h"
#include "tapi_rpc_unistd.h"
#include "tapi_env.h"

#define TE_TEST_NAME "pread_pwrite"

int
main(int argc, char **argv)
{
    int fd;
    rcf_rpc_server *pco_iut = NULL;
    const char template[] = "This is a test";
    off_t arbitrary_offset = 5;
    const char *file_name = "te_rpc_unistd_tmp";
    const char lookup_word[] = "test";
    char read_out[sizeof(lookup_word)];
    off_t lookup_word_offset = strstr(template, lookup_word) - template
                               + arbitrary_offset;

    TEST_START;

    TEST_GET_PCO(pco_iut);

    TEST_STEP("Create and open %s file", file_name);
    fd = rpc_open(pco_iut, file_name, RPC_O_RDWR | RPC_O_CREAT, 0);

    TEST_STEP("Write to the file with pwrite command");
    rpc_pwrite(pco_iut, fd, template, sizeof(template), arbitrary_offset);

    TEST_STEP("Read the file with pread command");
    rpc_pread(pco_iut, fd, read_out, sizeof(lookup_word), lookup_word_offset);

    if (strncmp(read_out, lookup_word, sizeof(lookup_word)) != 0)
    {
        TEST_VERDICT("Read chunk \"%s\" is not equal to expected chunk \"%s\"",
                     read_out, lookup_word);
    }

    TEST_SUCCESS;

cleanup:
    rpc_close(pco_iut, fd);
    rpc_unlink(pco_iut, file_name);

    TEST_END;
}
