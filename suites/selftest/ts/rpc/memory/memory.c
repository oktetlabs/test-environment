/** @file
 * @brief
 * Allocate misaligned memory then free it.
 *
 * Copyright (C) 2022 OKTET Labs Ltd., St. Petersburg, Russia
 */

/** @page memory A sample of memory management TAPI
 *
 * @objective Check that memory management routines work correctly
 *
 * @par Scenario:
 *
 * @author Nikolai Kosovskii <nikolai.kosovskii@oktetlabs.ru>
 */

#define TE_TEST_NAME "memory/memory"

#include "memory_suite.h"

int
main(int argc, char **argv)
{
    /* Almost arbitrarily chosen constants */
    static const unsigned int size_interval_begin = 90;
    static const unsigned int size_interval_end = 100;
    static const int number_of_offsets = 11;
    static const int number_of_alignments = 10;

    rcf_rpc_server *pco_iut = NULL;
    size_t alignment = sizeof(void *);
    unsigned int size;
    size_t offset;
    int i;
    rpc_ptr ptr;

    TEST_START;
    TEST_GET_PCO(pco_iut);

    TEST_STEP("Allocate various misaligned pointers on TA using "
              "malloc_misaligned where offset < alignment");
    for (i = 0; i < number_of_alignments; i++, alignment *= 2)
    {
        for (offset = 0;
             offset < alignment;
             offset += ((alignment / number_of_offsets) + 1))
        {
            for (size = size_interval_begin; size < size_interval_end; size++)
            {
                size_t addr_result;

                RPC_AWAIT_ERROR(pco_iut);
                ptr = rpc_malloc_misaligned(pco_iut, alignment, size, offset);
                if (ptr == RPC_NULL)
                {
                    if (RPC_ERRNO(pco_iut) == TE_RC(TE_TA_UNIX, EINVAL))
                    {
                        TEST_VERDICT("rpc_malloc_misaligned() gets invalid "
                                     "values as arguments");
                    }
                    else if (RPC_ERRNO(pco_iut) ==
                             TE_RC(TE_TA_UNIX, TE_EOPNOTSUPP))
                    {
                        TEST_SKIP("rpc_malloc_misaligned() gets RPC that "
                                  "is not supported");
                    }
                    else
                    {
                        TEST_VERDICT("rpc_malloc_misaligned() failed");
                    }
                }

                addr_result = rpc_get_addr_by_id(pco_iut, ptr);
                if (RPC_ERRNO(pco_iut) != 0)
                {
                    TEST_VERDICT("rpc_malloc_misaligned() produce ptr for"
                                 "which rpc_get_addr_by_id() failed");
                }
                if((addr_result - offset) % alignment != 0)
                {
                    TEST_VERDICT("rpc_malloc_misaligned() produce "
                                 "wrong alignment");
                }
                rpc_free(pco_iut, ptr);
            }
        }
    }

    TEST_STEP("Try to allocate memory on TA using "
              "malloc_misaligned where offset > alignment");

    /* Almost arbitrarily chosen numbers. */
    offset = alignment + ((alignment / number_of_offsets) + 1);
    size = size_interval_begin;

    RPC_AWAIT_ERROR(pco_iut);
    ptr = rpc_malloc_misaligned(pco_iut, alignment, size, offset);
    if (ptr == RPC_NULL)
    {
        if (RPC_ERRNO(pco_iut) != TE_RC(TE_TA_UNIX, EINVAL))
            TEST_VERDICT("rpc_malloc_misaligned() failed");
    }
    else
    {
        TEST_VERDICT("rpc_malloc_misaligned() produce not RPC_NULL pointer "
                     "even if offset>alignment");
    }
    rpc_free(pco_iut, ptr);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
