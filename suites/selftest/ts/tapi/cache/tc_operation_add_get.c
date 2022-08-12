/** @file
 * @brief TDD: Test Suite to test TAPI cache implementation
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 */

#define TE_TEST_NAME  "tc_operation_add_get"

#include "te_defs.h"
#include "logger_api.h"
#include "te_sockaddr.h"
#include "tapi_test.h"
#include "tapi_cache.h"

#define VALUE_TYPE_MAPPING_LIST             \
    { "integer",        CVT_INTEGER },      \
    { "string",         CVT_STRING },       \
    { "address",        CVT_ADDRESS },      \
    { "none",           CVT_NONE },         \
    { "unspecified",    CVT_UNSPECIFIED }

#define TEST_GET_VALUE_TYPE(_var_name) \
    TEST_GET_ENUM_PARAM(_var_name, VALUE_TYPE_MAPPING_LIST)


int
main(int argc, char **argv)
{
    const char   *instance;
    cfg_val_type  type;

    TEST_START;

    TEST_GET_STRING_PARAM(instance);
    TEST_GET_VALUE_TYPE(type);

    TEST_STEP("Test simple operations on the cache values");
    TEST_SUBSTEP("Add a new value to the cache area");
    TEST_SUBSTEP("Get this value back from the cache area");
    TEST_SUBSTEP("Check if added value does not match to read one");
    switch (type)
    {
        case CVT_INTEGER:
        {
            int value;
            int got_value;

            TEST_GET_INT_PARAM(value);
            CHECK_RC(tapi_cache_add_int(value, "%s", instance));
            CHECK_RC(tapi_cache_get_int(&got_value, "%s", instance));
            if (got_value != value)
            {
                ERROR_VERDICT("Values mismatch");
                TEST_FAIL("Value mismatch: set(%d) != got(%d)",
                          value, got_value);
            }
            break;
        }

        case CVT_STRING:
        {
            const char *value;
            char       *got_value;

            TEST_GET_STRING_PARAM(value);
            CHECK_RC(tapi_cache_add_string(value, "%s", instance));
            CHECK_RC(tapi_cache_get_string(&got_value, "%s", instance));
            if (strcmp(value, got_value) != 0)
            {
                ERROR_VERDICT("Values mismatch");
                TEST_FAIL("Value mismatch: set('%s') != got('%s')",
                          value, got_value);
            }
            free(got_value);
            break;
        }

        case CVT_ADDRESS:
        {
            const char *value;
            struct sockaddr_storage addr;
            struct sockaddr *got_addr;

            TEST_GET_STRING_PARAM(value);
            CHECK_RC(te_sockaddr_netaddr_from_string(value, SA(&addr)));
            CHECK_RC(tapi_cache_add_addr(CONST_SA(&addr), "%s", instance));
            CHECK_RC(tapi_cache_get_addr(&got_addr, "%s", instance));
            if (te_sockaddrcmp_no_ports(
                    CONST_SA(&addr), te_sockaddr_get_size(CONST_SA(&addr)),
                    got_addr, te_sockaddr_get_size(got_addr)) != 0)
            {
                ERROR_VERDICT("Values mismatch");
                TEST_FAIL("Value mismatch: set('%s') != got('%s')",
                          te_sockaddr_get_ipstr(CONST_SA(&addr)),
                          te_sockaddr_get_ipstr(got_addr));
            }
            free(got_addr);
            break;
        }

        default:
            TEST_VERDICT("Test does not support a value type");
    }

    TEST_SUCCESS;

cleanup:
    CLEANUP_CHECK_RC(cfg_tree_print(NULL, TE_LL_RING, TAPI_CACHE_ROOT_INST));
    TEST_END;
}
