/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for te_kvpair.h functions
 *
 * Testing key-value pair handling routines.
 */

/** @page tools_kvpair te_kvpair.h test
 *
 * @objective Testing key-value pair routines.
 *
 * Test the correctness of key-value handling functions.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/kvpair"

#include "te_config.h"

#include "tapi_test.h"
#include "te_kvpair.h"
#include "te_bufs.h"
#include "te_vector.h"

#include "te_str.h"
#include "te_string.h"


int
main(int argc, char **argv)
{
    unsigned int n_keys;
    unsigned int min_key_len;
    unsigned int max_key_len;
    unsigned int min_value_len;
    unsigned int max_value_len;
    te_vec keys = TE_VEC_INIT(char *);
    te_vec values = TE_VEC_INIT(char *);
    te_kvpair_h kvpairs;
    unsigned int i;
    te_string expected_content = TE_STRING_INIT;
    te_string actual_content = TE_STRING_INIT;

    te_kvpair_init(&kvpairs);

    TEST_START;
    TEST_GET_UINT_PARAM(n_keys);
    TEST_GET_UINT_PARAM(min_key_len);
    TEST_GET_UINT_PARAM(max_key_len);
    TEST_GET_UINT_PARAM(min_value_len);
    TEST_GET_UINT_PARAM(max_value_len);

    TEST_STEP("Generating keys and values");
    for (i = 0; i < n_keys; i++)
    {
        char *key = te_make_printable_buf(min_key_len, max_key_len, NULL);
        char *value = te_make_printable_buf(min_value_len, max_value_len, NULL);

        TE_VEC_APPEND(&keys, key);
        TE_VEC_APPEND(&values, value);
    }

    TEST_STEP("Checking the empty kvpair");
    for (i = 0; i < n_keys; i++)
    {
        const char *key = TE_VEC_GET(char *, &keys, i);

        if (te_kvpairs_get(&kvpairs, key) != NULL)
        {
            ERROR("Key '%s' is found in an empty kvpairs", key);
            TEST_VERDICT("Found a key in an empty kvpairs");
        }
    }

    TEST_STEP("Adding keys");
    for (i = 0; i < n_keys; i++)
    {
        char *key = TE_VEC_GET(char *, &keys, i);
        const char *value = TE_VEC_GET(char *, &values, i);
        te_errno rc = te_kvpair_add(&kvpairs, key, "%s", value);

        /*
         * At this stage we can't be sure that all our generated
         * keys are unique, so just ignore duplicates.
         */
        if (rc == TE_EEXIST)
        {
            free(key);
            TE_VEC_GET(char *, &keys, i) = NULL;
            continue;
        }
        CHECK_RC(rc);
    }

    TEST_STEP("Trying to add keys the second time");
    for (i = 0; i < n_keys; i++)
    {
        const char *key = TE_VEC_GET(char *, &keys, i);
        const char *value = TE_VEC_GET(char *, &values, i);
        te_errno rc;

        if (key == NULL)
            continue;
        rc = te_kvpair_add(&kvpairs, key, "%s", value);
        if (rc != TE_EEXIST)
        {
            ERROR("Key '%s' was added twice: rc=%r", key, rc);
            TEST_VERDICT("Duplicate key added");
        }
    }

    TEST_STEP("Checking added keys");
    for (i = 0; i < n_keys; i++)
    {
        const char *key = TE_VEC_GET(char *, &keys, i);
        const char *value = TE_VEC_GET(char *, &values, i);
        const char *got;

        if (key == NULL)
            continue;

        CHECK_NOT_NULL(got = te_kvpairs_get(&kvpairs, key));

        if (strcmp(got, value) != 0)
        {
            ERROR("Key '%s' should be associated with '%s', but got '%s'",
                  key, value, got);
            TEST_VERDICT("Obtained unexpected key value");
        }
    }

    TEST_STEP("Deleting keys");
    for (i = 0; i < n_keys; i++)
    {
        const char *key = TE_VEC_GET(char *, &keys, i);
        const char *got;
        te_errno rc;

        if (key == NULL)
            continue;

        CHECK_RC(te_kvpairs_del(&kvpairs, key));
        got = te_kvpairs_get(&kvpairs, key);

        if (got != NULL)
        {
            ERROR("Deleted key '%s' has a value '%s'",
                  key, got);
            TEST_VERDICT("Deleted key has a value");
        }
        rc = te_kvpairs_del(&kvpairs, key);
        if (rc != TE_ENOENT)
        {
            ERROR("Deleted key '%s' can be deleted twice, rc=%r", key, rc);
            TEST_VERDICT("Deleted key can be deleted twice");
        }
    }

    TEST_STEP("Adding keys again");
    for (i = 0; i < n_keys; i++)
    {
        const char *key = TE_VEC_GET(char *, &keys, i);
        const char *value = TE_VEC_GET(char *, &values, i);

        if (key == NULL)
            continue;

        CHECK_RC(te_kvpair_add(&kvpairs, key, "%s", value));
    }

    TEST_STEP("Checking kvpair-to-string serialization");
    for (i = 0; i < n_keys; i++)
    {
        const char *key = TE_VEC_GET(char *, &keys, i);
        const char *value = TE_VEC_GET(char *, &values, i);

        if (key == NULL)
            continue;

        te_string_append(&expected_content, "%s%s=%s",
                         i == 0 ? "" : ":", key, value);
    }
    CHECK_RC(te_kvpair_to_str(&kvpairs, &actual_content));
    if (strcmp(expected_content.ptr, actual_content.ptr) != 0)
    {
        ERROR("Expected '%s', got '%s'", expected_content.ptr,
              actual_content.ptr);
        TEST_VERDICT("Unexpected kvpairs serialization");
    }


    TEST_STEP("Clean up kvpairs");
    te_kvpair_fini(&kvpairs);
    for (i = 0; i < n_keys; i++)
    {
        const char *key = TE_VEC_GET(char *, &keys, i);

        if (key == NULL)
            continue;
        if (te_kvpairs_get(&kvpairs, key) != NULL)
        {
            ERROR("Key '%s' is found after cleanup", key);
            TEST_VERDICT("Found a key after cleanup");
        }
    }

    TEST_SUCCESS;

cleanup:

    te_kvpair_fini(&kvpairs);
    te_string_free(&actual_content);
    te_string_free(&expected_content);
    te_vec_deep_free(&keys);
    te_vec_deep_free(&values);
    TEST_END;
}
