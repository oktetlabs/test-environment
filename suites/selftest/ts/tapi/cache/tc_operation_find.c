/** @file
 * @brief TDD: Test Suite to test TAPI cache implementation
 *
 * Copyright (C) 2004-2019 OKTET Labs. All rights reserved.
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#define TE_TEST_NAME  "tc_operation_find"

#include "te_defs.h"
#include "logger_api.h"
#include "te_string.h"
#include "tapi_test.h"
#include "tapi_cache.h"
#include "tapi_mem.h"

/* Callbacks opaque */
typedef struct opaque_t {
    const char **expected_found;
    int          num_expected_found;
    const char **found;
    int          num_found;
} opaque_t;

static te_errno
cb_func(cfg_handle handle, void *opaque)
{
    opaque_t *op = opaque;
    char     *oid;
    te_string oid_expected = TE_STRING_INIT_STATIC(CFG_OID_MAX);
    te_errno  rc;
    int       i;

    rc = cfg_get_oid_str(handle, &oid);

    for (i = 0; i < op->num_expected_found; i++)
    {
        te_string_reset(&oid_expected);
        rc = te_string_append(&oid_expected, "%s/%s",
                              TAPI_CACHE_ROOT_INST, op->expected_found[i]);
        if (rc != 0)
            break;

        if (strcmp(oid, oid_expected.ptr) == 0)
        {
            op->found[op->num_found++] = op->expected_found[i];
            break;
        }
    }
    if (rc == 0 && i == op->num_expected_found)
    {
        ERROR_VERDICT("Found unexpected value: '%s'", oid);
        rc = TE_EEXIST;
    }

    free(oid);

    return rc;
}

int
main(int argc, char **argv)
{
    const char  *instance;
    char       **expected_found;
    int          num_expected_found;
    int          i;
    int          j;
    opaque_t     op;

    TEST_START;

    TEST_GET_STRING_PARAM(instance);
    TEST_GET_STRING_LIST_PARAM(expected_found, num_expected_found);
    op.expected_found = (const char **)expected_found;
    op.num_expected_found = num_expected_found;
    op.found = tapi_calloc(num_expected_found, sizeof(char *));
    op.num_found = 0;

    TEST_STEP("Find particular instances in the cache");
    rc = tapi_cache_find(cb_func, &op, "%s", instance);
    TEST_STEP("Verify found instances");
    if (op.num_found != op.num_expected_found)
    {
        rc = TE_ENOENT;
        for (i = 0; i < num_expected_found; i++)
        {
            for (j = 0; j < op.num_found; j++)
            {
                if (expected_found[i] == op.found[j])
                    break;
            }
            if (j == op.num_found)
            {
                ERROR_VERDICT("Value '%s' has not been found",
                              expected_found[i]);
            }
        }
    }

    if (rc != 0)
        TEST_VERDICT("Search function works improperly");

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
