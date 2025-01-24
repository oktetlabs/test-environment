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

static char *
update_kv(const te_kvpair_h *kvpairs, const char *key, const char *oldval,
          void *user)
{
    const char *newval = user;

    if (newval == NULL)
        return NULL;

    if (oldval == NULL)
        return TE_STRDUP(newval);

    UNUSED(kvpairs);
    UNUSED(key);
    return te_str_concat(oldval, newval);
}

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
    te_kvpair_h copy;
    unsigned int i;
    te_string expected_content = TE_STRING_INIT;
    te_string actual_content = TE_STRING_INIT;
    unsigned int count;
    unsigned int n_unique_keys = 0;

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

        count = te_kvpairs_count(&kvpairs, key);
        if (count != 0)
        {
            ERROR("Key '%s' counted as %u in an empty kvpairs", key, count);
            TEST_VERDICT("Key is counted in an empty kvpairs");
        }
    }
    count = te_kvpairs_count(&kvpairs, NULL);
    if (count != 0)
    {
        ERROR("%u keys counted in an empty kvpairs", count);
        TEST_VERDICT("Keys counted in an empty kvpairs");
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
        n_unique_keys++;
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
        te_kvpair_h submap;

        if (key == NULL)
            continue;

        CHECK_NOT_NULL(got = te_kvpairs_get(&kvpairs, key));

        if (strcmp(got, value) != 0)
        {
            ERROR("Key '%s' should be associated with '%s', but got '%s'",
                  key, value, got);
            TEST_VERDICT("Obtained unexpected key value");
        }

        count = te_kvpairs_count(&kvpairs, key);
        if (count != 1)
        {
            ERROR("Key '%s' counted %u times", key, count);
            TEST_VERDICT("Unexpected count of key bindings");
        }

        if (!te_kvpairs_has_kv(&kvpairs, key, value))
        {
            ERROR("Key-value pairs '%s':'%s' should be present",
                  key, value);
            TEST_VERDICT("Expected key-value pair not found");
        }

        if (!te_kvpairs_has_kv(&kvpairs, key, NULL))
        {
            ERROR("Key '%s' should be present",
                  key, value);
            TEST_VERDICT("Expected key not found");
        }

        te_kvpair_init(&submap);
        if (!te_kvpairs_is_submap(&submap, &kvpairs))
            TEST_VERDICT("Empty mapping should be a submap of any map");

        if (te_kvpairs_is_submap(&kvpairs, &submap))
            TEST_VERDICT("No non-empty map can be a submap of an empty map");

        CHECK_RC(te_kvpair_add(&submap, key, "%s", value));
        if (!te_kvpairs_is_submap(&submap, &kvpairs))
            TEST_VERDICT("A singleton kvpair is not a submap of the whole map");
        if (n_unique_keys > 1 && te_kvpairs_is_submap(&kvpairs, &submap))
        {
            TEST_VERDICT("A map with more than one pair cannot be a submap "
                         "of a singleton");
        }
        te_kvpair_push(&submap, key, "%s", value);
        if (!te_kvpairs_is_submap(&submap, &kvpairs))
            TEST_VERDICT("Cardinality should not matter for submaps");
        te_kvpair_push(&submap, "", "%s", "");
        if (te_kvpairs_is_submap(&submap, &kvpairs))
            TEST_VERDICT("An extra element has not been accounted for");

        te_kvpair_fini(&submap);
    }
    if (!te_kvpairs_is_submap(&kvpairs, &kvpairs))
        TEST_VERDICT("A mapping is not a submap of itself");

    TEST_STEP("Counting keys");
    count = te_kvpairs_count(&kvpairs, NULL);
    if (count != n_unique_keys)
    {
        ERROR("Counted %u keys, but expected %u", count, n_unique_keys);
        TEST_VERDICT("Unexpected count of keys");
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

        count = te_kvpairs_count(&kvpairs, key);
        if (count != 0)
        {
            ERROR("Deleted key '%s' counted %u times", key, count);
            TEST_VERDICT("Unexpected count of key bindings");
        }
    }
    count = te_kvpairs_count(&kvpairs, NULL);
    if (count != 0)
    {
        ERROR("Emptied kvpairs report %u keys", count);
        TEST_VERDICT("Unexpected count of key bindings");
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

    TEST_STEP("Testing multiple-valued keys");
    for (i = 0; i < n_keys; i++)
    {
        char *key = TE_VEC_GET(char *, &keys, i);
        const char *old_value = TE_VEC_GET(char *, &values, i);
        char *new_value = te_make_printable_buf(min_value_len, max_value_len,
                                                NULL);
        const char *got;
        te_vec all_vals = TE_VEC_INIT(const char *);

        if (key == NULL)
        {
            free(new_value);
            continue;
        }

        te_kvpair_push(&kvpairs, key, "%s", new_value);

        CHECK_NOT_NULL(got = te_kvpairs_get(&kvpairs, key));
        if (strcmp(got, new_value) != 0)
        {
            ERROR("Key '%s' should be associated with '%s' at index 0, "
                  "but got '%s'", key, new_value, got);
            TEST_VERDICT("Obtained unexpected key value");
        }
        CHECK_NOT_NULL(got = te_kvpairs_get_nth(&kvpairs, key, 1));
        if (strcmp(got, old_value) != 0)
        {
            ERROR("Key '%s' should be associated with '%s' at index 1, "
                  "but got '%s'", key, old_value, got);
            TEST_VERDICT("Obtained unexpected key value");
        }
        got = te_kvpairs_get_nth(&kvpairs, key, 2);
        if (got != NULL)
        {
            ERROR("Key '%s' should not be associated with any value at "
                  "index 2, but got '%s'", key, got);
            TEST_VERDICT("Obtained unexpected key value");
        }
        count = te_kvpairs_count(&kvpairs, key);
        if (count != 2)
        {
            ERROR("Key '%s' should count twice, but counted %u", key, count);
            TEST_VERDICT("Unexpected count of key bindings");
        }

        CHECK_RC(te_kvpairs_get_all(&kvpairs, key, &all_vals));
        if (te_vec_size(&all_vals) != 2)
            TEST_VERDICT("Invalid all-values vector size");
        if (strcmp(TE_VEC_GET(const char *, &all_vals, 0), new_value) != 0 ||
            strcmp(TE_VEC_GET(const char *, &all_vals, 1), old_value) != 0)
            TEST_VERDICT("Unexpected value(s) in all-values vector");

        CHECK_RC(te_kvpairs_del(&kvpairs, key));
        CHECK_NOT_NULL(got = te_kvpairs_get(&kvpairs, key));
        if (strcmp(got, old_value) != 0)
        {
            ERROR("Key '%s' should now be associated with '%s' at index 0, "
                  "but got '%s'", key, old_value, got);
            TEST_VERDICT("Obtained unexpected key value");
        }
        count = te_kvpairs_count(&kvpairs, key);
        if (count != 1)
        {
            ERROR("Key '%s' should count once, but counted %u", key, count);
            TEST_VERDICT("Unexpected count of key bindings");
        }

        free(new_value);
        te_vec_free(&all_vals);
    }

    TEST_STEP("Testing delete-all for multiple-valued keys");
    TEST_SUBSTEP("Adding multiple copies of keys");
    for (i = 0; i < n_keys; i++)
    {
        char *key = TE_VEC_GET(char *, &keys, i);
        const char *value = TE_VEC_GET(char *, &values, i);

        if (key == NULL)
            continue;

        te_kvpair_push(&kvpairs, key, "%s", value);
        te_kvpair_push(&kvpairs, key, "%s", value);
    }
    TEST_SUBSTEP("Deleteing keys");
    for (i = 0; i < n_keys; i++)
    {
        char *key = TE_VEC_GET(char *, &keys, i);
        te_errno rc;

        if (key == NULL)
            continue;

        CHECK_RC(te_kvpairs_del_all(&kvpairs, key));
        count = te_kvpairs_count(&kvpairs, key);
        if (count != 0)
        {
            ERROR("Deleted key '%s' counted %u times", key, count);
            TEST_VERDICT("Key was not properly deleted");
        }

        rc = te_kvpairs_del_all(&kvpairs, key);
        if (rc != TE_ENOENT)
        {
            ERROR("Deleted key '%s' was deleted again", key);
            TEST_VERDICT("Key was not properly deleted");
        }
    }
    TEST_SUBSTEP("Adding keys back");
    for (i = 0; i < n_keys; i++)
    {
        char *key = TE_VEC_GET(char *, &keys, i);
        const char *value = TE_VEC_GET(char *, &values, i);

        if (key == NULL)
            continue;

        CHECK_RC(te_kvpair_add(&kvpairs, key, "%s", value));
    }

    TEST_SUBSTEP("Checking key copying");
    for (i = 0; i < n_keys; i++)
    {
        char *key = TE_VEC_GET(char *, &keys, i);
        const char *value = TE_VEC_GET(char *, &values, i);
        const char *copied_val = NULL;

        te_kvpair_init(&copy);
        te_kvpairs_copy_key(&copy, &kvpairs, key);
        CHECK_NOT_NULL(copied_val = te_kvpairs_get(&copy, key));

        if (strcmp(value, copied_val) != 0)
        {
            ERROR("Copied value '%s' differs from the original '%s'",
                  copied_val, value);
            TEST_VERDICT("Copying a key failed");
        }

        te_kvpairs_copy_key(&copy, &kvpairs, key);
        if (te_kvpairs_count(&copy, key) != 2)
            ERROR("Some key bindings are lost during copy");

        te_kvpair_fini(&copy);
    }

    te_kvpair_init(&copy);
    te_kvpairs_copy(&copy, &kvpairs);
    if (!te_kvpairs_is_submap(&kvpairs, &copy))
        TEST_VERDICT("The original is not a submap of the copy");
    if (!te_kvpairs_is_submap(&copy, &kvpairs))
        TEST_VERDICT("The copy is not a submap of the original");
    te_kvpair_fini(&copy);

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

    TEST_STEP("Testing key update");
    for (i = 0; i < n_keys; i++)
    {
        const char *key = TE_VEC_GET(const char *, &keys, i);
        char *value = TE_VEC_GET(char *, &values, i);
        const char *chk_value;

        te_kvpair_push(&kvpairs, key, "%s", value);
        te_kvpair_update(&kvpairs, key, update_kv, value);
        CHECK_NOT_NULL(chk_value = te_kvpairs_get(&kvpairs, key));
        if (!te_compare_bufs(value, strlen(value), 2,
                             chk_value, strlen(chk_value), TE_LL_ERROR))
            TEST_VERDICT("The value not properly modified");
        CHECK_NOT_NULL(chk_value = te_kvpairs_get_nth(&kvpairs, key, 1));
        if (strcmp(chk_value, value) != 0)
            TEST_VERDICT("Non-last binding affected");

        te_kvpair_update(&kvpairs, key, update_kv, NULL);
        CHECK_NOT_NULL(chk_value = te_kvpairs_get(&kvpairs, key));
        if (strcmp(chk_value, value) != 0)
            TEST_VERDICT("Wrong value deleted");

        CHECK_RC(te_kvpairs_del(&kvpairs, key));
        te_kvpair_update(&kvpairs, key, update_kv, NULL);
        if (te_kvpairs_has_kv(&kvpairs, key, NULL))
            TEST_VERDICT("A value was added when it should not");

        te_kvpair_update(&kvpairs, key, update_kv, value);
        if (!te_kvpairs_has_kv(&kvpairs, key, value))
            TEST_VERDICT("A value was not added when it should");
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
