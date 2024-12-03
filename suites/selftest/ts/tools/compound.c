/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2025 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for compound strings.
 *
 * Testing compound string routines.
 */

/** @page tools_compound te_compound.h test
 *
 * @objective Testing compound string routines.
 *
 * Test the correctness of compound string implementation.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/compound"

#include "te_config.h"

#include "tapi_test.h"
#include "te_vector.h"
#include "te_kvpair.h"
#include "te_compound.h"
#include "te_bufs.h"

static te_errno
check_unnamed_kvp(const char *key, const char *value, void *user)
{
    const te_vec *vec = user;
    unsigned int idx;

    CHECK_RC(te_strtoui(key, 10, &idx));
    if (strcmp(TE_VEC_GET(const char *, vec, idx), value) != 0)
        TEST_VERDICT("Unexpected value at index %u", idx);
    return 0;
}

static void
must_validate(const te_string *compound)
{
    if (!te_compound_validate(compound))
        TEST_VERDICT("Compound does not validate");
}

static void
generate_random_bufs(te_vec *bufs, unsigned int n_items,
                     unsigned int min_len, unsigned int max_len)
{
    unsigned int i;

    for (i = 0; i < n_items; i++)
    {
        TE_VEC_APPEND_RVALUE(bufs, char *,
                             te_make_printable_buf(min_len, max_len + 1, NULL));
    }

}

static void
test_unnamed_compound(unsigned int min_len, unsigned int max_len,
                      unsigned int min_items, unsigned int max_items)
{
    unsigned int i;
    unsigned int n_items = rand_range(min_items, max_items);
    te_vec bufs = TE_VEC_INIT_AUTOPTR(char *);
    te_vec bufs2 = TE_VEC_INIT_AUTOPTR(char *);
    size_t first_buf_len;
    char *first_buf = te_make_printable_buf(min_len, max_len + 1,
                                            &first_buf_len);
    te_string compound = TE_STRING_INIT;
    te_string tgt = TE_STRING_INIT;
    te_kvpair_h kvp;

    te_kvpair_init(&kvp);
    generate_random_bufs(&bufs, n_items, min_len, max_len);

    if (te_compound_classify(&compound) != TE_COMPOUND_NULL)
        TEST_VERDICT("Empty string is not considered null");
    must_validate(&compound);

    te_string_append(&compound, "%s", first_buf);
    if (te_compound_classify(&compound) != TE_COMPOUND_PLAIN)
        TEST_VERDICT("Plain string is not considered plain");
    must_validate(&compound);

    if (te_compound_count(&compound, NULL) != 1)
        TEST_VERDICT("Invalid count for a plain string");

    if (!te_compound_extract(&tgt, &compound, NULL, 0))
        TEST_VERDICT("Plain string not extracted at index zero");
    if (!te_compare_bufs(compound.ptr, compound.len + 1, 1,
                         tgt.ptr, tgt.len + 1, TE_LL_RING))
        TEST_VERDICT("Plain string is not extrated correctly");

    if (te_compound_extract(&tgt, &compound, NULL, 1))
        TEST_VERDICT("Plain string extracted by a non-zero index");
    if (te_compound_extract(&tgt, &compound, "", 0))
        TEST_VERDICT("Plain string extracted by a non-null key");


    te_vec2compound(&compound, &bufs);
    if (te_compound_classify(&compound) != TE_COMPOUND_ARRAY)
        TEST_VERDICT("Compound array is not considered array");
    if (te_compound_count(&compound, NULL) != n_items + 1)
        TEST_VERDICT("Invalid number of elements in a compound");
    must_validate(&compound);

    te_string_reset(&tgt);
    if (!te_compound_extract(&tgt, &compound, NULL, 0))
        TEST_VERDICT("Item not extracted by a valid index");
    if (!te_compare_bufs(first_buf, first_buf_len, 1,
                         tgt.ptr, tgt.len + 1, TE_LL_RING))
        TEST_VERDICT("First item is not equal to the first buffer");
    for (i = 0; i < n_items; i++)
    {
        const char *expected = TE_VEC_GET(const char *, &bufs, i);
        te_string_reset(&tgt);
        if (!te_compound_extract(&tgt, &compound, NULL, i + 1))
            TEST_VERDICT("Item not extracted by a valid index");
        if (!te_compare_bufs(expected, strlen(expected) + 1, 1,
                             tgt.ptr, tgt.len + 1, TE_LL_RING))
        {
            TEST_VERDICT("Item %u is not equal to a corresponding buffer",
                         i + 1);
        }
    }
    if (te_compound_extract(&tgt, &compound, NULL, n_items + 2))
        TEST_VERDICT("Item extracted by an out-of-bounds index");
    if (te_compound_extract(&tgt, &compound, "", 0))
        TEST_VERDICT("Item extracted by a non-null key");

    te_compound2vec(&bufs2, &compound);
    if (te_vec_size(&bufs2) != n_items + 1)
        TEST_VERDICT("Invalid number of items in the extracted vector");
    if (strcmp(first_buf, TE_VEC_GET(const char *, &bufs2, 0)) != 0)
        TEST_VERDICT("Unexpected first item of the extracted vector");
    for (i = 0; i < n_items; i++)
    {
        if (strcmp(TE_VEC_GET(const char *, &bufs, i),
                   TE_VEC_GET(const char *, &bufs2, i + 1)) != 0)
            TEST_VERDICT("Unexpected item %zu of the extracted vector", i + 1);
    }

    te_compound2kvpair(&kvp, &compound);
    CHECK_RC(te_kvpairs_foreach(&kvp, check_unnamed_kvp, NULL, &bufs2));

    te_kvpair_fini(&kvp);
    te_vec_free(&bufs);
    te_vec_free(&bufs2);
    te_string_free(&compound);
    te_string_free(&tgt);
    free(first_buf);
}

static te_errno
check_all_kvp(const char *key, const char *value, void *user)
{
    te_string *compound = user;
    te_string str = TE_STRING_INIT;

    if (te_compound_count(compound, key) != 1)
        TEST_VERDICT("Invalid number of elements for a key");
    if (!te_compound_extract(&str, compound, key, 0))
        TEST_VERDICT("A key is not extacted");
    if (strcmp(str.ptr, value) != 0)
        TEST_VERDICT("Unexpected value for a key");

    if (te_compound_extract(&str, compound, key, 1))
        TEST_VERDICT("A key with non-zero index is extacted");

    te_string_free(&str);

    return 0;
}

static te_errno
check_named_compound(char *key, size_t idx, char *value, bool has_more,
                     void *user)
{
    const te_kvpair_h *kvp = user;
    const char *obtained = te_kvpairs_get_nth(kvp, key, idx);

    if (obtained == NULL)
        TEST_VERDICT("A key from compound is not found in the source");
    if (strcmp(obtained, value) != 0)
        TEST_VERDICT("Invalid value for a key");

    UNUSED(has_more);
    return 0;
}

static void
generate_random_kv(te_kvpair_h *kvp, unsigned int n_items,
                   unsigned int min_len, unsigned int max_len)
{
    unsigned int i;

    te_kvpair_init(kvp);
    for (i = 0; i < n_items; i++)
    {
        char *key = te_make_printable_buf(min_len, max_len, NULL);
        char *value = te_make_printable_buf(min_len, max_len, NULL);

        /* In an unlikely case there is a duplicate key we just move on. */
        te_kvpair_add(kvp, key, "%s", value);
        free(key);
        free(value);
    }

}

static void
test_named_compound(unsigned int min_len, unsigned int max_len,
                    unsigned int min_items, unsigned int max_items)
{
    unsigned int n_items = rand_range(min_items, max_items);
    te_kvpair_h kvp;
    te_kvpair_h kvp2;
    te_string compound = TE_STRING_INIT;

    generate_random_kv(&kvp, n_items, min_len, max_len);
    te_kvpair2compound(&compound, &kvp);
    if (te_compound_classify(&compound) != TE_COMPOUND_OBJECT)
        TEST_VERDICT("The object compound is not classified as object");
    must_validate(&compound);

    CHECK_RC(te_kvpairs_foreach(&kvp, check_all_kvp, NULL, &compound));
    CHECK_RC(te_compound_iterate(&compound, check_named_compound, &kvp));

    te_kvpair_init(&kvp2);
    te_compound2kvpair(&kvp2, &compound);
    if (!te_kvpairs_is_submap(&kvp, &kvp2) ||
        !te_kvpairs_is_submap(&kvp2, &kvp))
        TEST_VERDICT("Extracted kvpair differs from the original");
    te_kvpair_fini(&kvp2);

    te_kvpair_fini(&kvp);
    te_string_free(&compound);
}

static void
test_compound_modify_mode(unsigned int min_len, unsigned int max_len,
                          bool unnamed)
{
    static char key_sep = TE_COMPOUND_KEY_SEP;
    static char item_sep = TE_COMPOUND_ITEM_SEP;
    size_t key_len = 0;
    char *key = unnamed ? NULL :
        te_make_printable_buf(min_len, max_len, &key_len);
    size_t init_len;
    char *initial = te_make_printable_buf(min_len, max_len, &init_len);
    size_t suffix_len;
    char *suffix = te_make_printable_buf(min_len, max_len, &suffix_len);
    size_t prefix_len;
    char *prefix = te_make_printable_buf(min_len, max_len, &prefix_len);
    size_t repl_len;
    char *repl = te_make_printable_buf(min_len, max_len, &repl_len);
    te_string compound = TE_STRING_INIT;

    te_compound_set(&compound, key, TE_COMPOUND_MOD_OP_APPEND, "%s", initial);
    must_validate(&compound);
    if (!te_compare_iovecs(5, (const struct iovec[5]){
                {.iov_base = key, .iov_len = key_len > 0 ? key_len - 1 : 0},
                {.iov_base = &key_sep,
                 .iov_len = key_len > 0 ? sizeof(key_sep) : 0},
                {.iov_base = initial, .iov_len = init_len - 1},
                {.iov_base = &item_sep, .iov_len = sizeof(item_sep)},
                {.iov_base = NULL, .iov_len = 1},
            }, 1, (const struct iovec[1]){
                {.iov_base = compound.ptr, .iov_len = compound.len + 1}
            }, TE_LL_RING))
        TEST_VERDICT("Invalid content after initial insertion");

    te_compound_set(&compound, key, TE_COMPOUND_MOD_OP_APPEND, "%s", suffix);
    must_validate(&compound);
    if (!te_compare_iovecs(9, (const struct iovec[9]){
                {.iov_base = key, .iov_len = key_len > 0 ? key_len - 1 : 0},
                {.iov_base = &key_sep,
                 .iov_len = key_len > 0 ? sizeof(key_sep) : 0},
                {.iov_base = initial, .iov_len = init_len - 1},
                {.iov_base = &item_sep, .iov_len = sizeof(item_sep)},
                {.iov_base = key, .iov_len = key_len > 0 ? key_len - 1 : 0},
                {.iov_base = &key_sep,
                 .iov_len = key_len > 0 ? sizeof(key_sep) : 0},
                {.iov_base = suffix, .iov_len = suffix_len - 1},
                {.iov_base = &item_sep, .iov_len = sizeof(item_sep)},
                {.iov_base = NULL, .iov_len = 1},
            }, 1, (const struct iovec[1]){
                {.iov_base = compound.ptr, .iov_len = compound.len + 1}
            }, TE_LL_RING))
        TEST_VERDICT("Invalid content after appending");

    te_compound_set(&compound, key, TE_COMPOUND_MOD_OP_PREPEND, "%s", prefix);
    must_validate(&compound);
    if (!te_compare_iovecs(13, (const struct iovec[13]){
                {.iov_base = key, .iov_len = key_len > 0 ? key_len - 1 : 0},
                {.iov_base = &key_sep,
                 .iov_len = key_len > 0 ? sizeof(key_sep) : 0},
                {.iov_base = prefix, .iov_len = prefix_len - 1},
                {.iov_base = &item_sep, .iov_len = sizeof(item_sep)},
                {.iov_base = key, .iov_len = key_len > 0 ? key_len - 1 : 0},
                {.iov_base = &key_sep,
                 .iov_len = key_len > 0 ? sizeof(key_sep) : 0},
                {.iov_base = initial, .iov_len = init_len - 1},
                {.iov_base = &item_sep, .iov_len = sizeof(item_sep)},
                {.iov_base = key, .iov_len = key_len > 0 ? key_len - 1 : 0},
                {.iov_base = &key_sep,
                 .iov_len = key_len > 0 ? sizeof(key_sep) : 0},
                {.iov_base = suffix, .iov_len = suffix_len - 1},
                {.iov_base = &item_sep, .iov_len = sizeof(item_sep)},
                {.iov_base = NULL, .iov_len = 1},
            }, 1, (const struct iovec[1]){
                {.iov_base = compound.ptr, .iov_len = compound.len + 1}
            }, TE_LL_RING))
        TEST_VERDICT("Invalid content after prepending");

    te_compound_set(&compound, key, TE_COMPOUND_MOD_OP_REPLACE, "%s", repl);
    must_validate(&compound);
    if (!te_compare_iovecs(5, (const struct iovec[5]){
                {.iov_base = key, .iov_len = key_len > 0 ? key_len - 1 : 0},
                {.iov_base = &key_sep,
                 .iov_len = key_len > 0 ? sizeof(key_sep) : 0},
                {.iov_base = repl, .iov_len = repl_len - 1},
                {.iov_base = &item_sep, .iov_len = sizeof(item_sep)},
                {.iov_base = NULL, .iov_len = 1},
            }, 1, (const struct iovec[1]){
                {.iov_base = compound.ptr, .iov_len = compound.len + 1}
            }, TE_LL_RING))
        TEST_VERDICT("Invalid content after replacement");

    te_compound_set(&compound, key, TE_COMPOUND_MOD_OP_REPLACE, NULL);
    must_validate(&compound);
    if (compound.len != 0 || *compound.ptr != '\0')
        TEST_VERDICT("String not empty after deletion");

    free(key);
    free(initial);
    free(suffix);
    free(prefix);
    free(repl);

    te_string_free(&compound);
}

static te_errno
generate_values(const char *key, const char *value, void *user)
{
    te_kvpair_h *target_kvp = user;
    char *new_value = te_make_printable_buf_by_len(strlen(value) + 1);

    te_kvpair_add(target_kvp, key, "%s", new_value);
    free(new_value);
    return 0;
}

static te_errno
check_append_first_keys(const char *key, const char *value, void *user)
{
    te_string *compound = user;
    te_string tmp = TE_STRING_INIT;

    if (te_compound_count(compound, key) != 2)
        TEST_VERDICT("Unexpected number of values associated to a key");

    if (!te_compound_extract(&tmp, compound, key, 0))
        TEST_VERDICT("First value not extracted");

    if (strcmp(tmp.ptr, value) != 0)
        TEST_VERDICT("Unexpected first value");

    te_string_free(&tmp);
    return 0;
}


static te_errno
check_append_second_keys(const char *key, const char *value, void *user)
{
    te_string *compound = user;
    te_string tmp = TE_STRING_INIT;

    if (te_compound_count(compound, key) != 2)
        TEST_VERDICT("Unexpected number of values associated to a key");

    if (!te_compound_extract(&tmp, compound, key, 1))
        TEST_VERDICT("Second value not extracted");

    if (strcmp(tmp.ptr, value) != 0)
        TEST_VERDICT("Unexpected second value");

    te_string_free(&tmp);
    return 0;
}

static void
check_unnamed_fields(const te_string *compound, const te_vec *src,
                     size_t base_idx, size_t n_items)
{
    te_string tmp = TE_STRING_INIT;
    unsigned int i;

    for (i = 0; i < n_items; i++)
    {
        const char *expected = TE_VEC_GET(const char *, src, i);

        if (!te_compound_extract(&tmp, compound, NULL, i + base_idx))
            TEST_VERDICT("%u+%u'th value not extracted", i, base_idx);

        if (strcmp(expected, tmp.ptr) != 0)
            TEST_VERDICT("%u+%u'th value is unexpected", i, base_idx);

        te_string_reset(&tmp);
    }
    te_string_free(&tmp);
}

static void
test_merge(unsigned int min_items, unsigned int max_items,
           unsigned int min_len, unsigned int max_len)
{
    unsigned int n_items = rand_range(min_items, max_items);
    unsigned int n_keys = rand_range(min_items, max_items);
    te_vec bufs = TE_VEC_INIT_AUTOPTR(char *);
    te_kvpair_h kvp;
    unsigned int n_merge_items = rand_range(min_items, max_items);
    te_vec merge_bufs = TE_VEC_INIT_AUTOPTR(char *);
    te_kvpair_h merge_kvp;
    te_string compound1 = TE_STRING_INIT;
    te_string compound2 = TE_STRING_INIT;
    te_string compound3 = TE_STRING_INIT;
    te_string merge = TE_STRING_INIT;

    generate_random_bufs(&bufs, n_items, min_len, max_len);
    te_vec2compound(&compound1, &bufs);
    generate_random_kv(&kvp, n_keys, min_len, max_len);
    te_kvpair2compound(&compound1, &kvp);

    generate_random_bufs(&merge_bufs, n_merge_items, min_len, max_len);
    te_vec2compound(&merge, &merge_bufs);
    te_kvpair_init(&merge_kvp);
    te_kvpairs_foreach(&kvp, generate_values, NULL, &merge_kvp);
    te_kvpair2compound(&merge, &merge_kvp);

    te_string_append(&compound2, "%s", compound1.ptr);
    te_string_append(&compound3, "%s", compound1.ptr);

    TEST_SUBSTEP("Merge with append");
    te_compound_merge(&compound1, &merge, TE_COMPOUND_MOD_OP_APPEND);
    must_validate(&compound1);

    if (te_compound_count(&compound1, NULL) != n_items + n_merge_items)
        TEST_VERDICT("Invalid number of merged unnamed fields");
    check_unnamed_fields(&compound1, &bufs, 0, n_items);
    check_unnamed_fields(&compound1, &merge_bufs, n_items, n_merge_items);

    te_kvpairs_foreach(&kvp, check_append_first_keys, NULL, &compound1);
    te_kvpairs_foreach(&merge_kvp, check_append_second_keys, NULL, &compound1);

    TEST_SUBSTEP("Merge with prepend");
    te_compound_merge(&compound2, &merge, TE_COMPOUND_MOD_OP_PREPEND);
    must_validate(&compound2);

    if (te_compound_count(&compound2, NULL) != n_items + n_merge_items)
        TEST_VERDICT("Invalid number of merged unnamed fields");
    check_unnamed_fields(&compound2, &merge_bufs, 0, n_merge_items);
    check_unnamed_fields(&compound2, &bufs, n_merge_items, n_items);

    te_kvpairs_foreach(&merge_kvp, check_append_first_keys, NULL, &compound2);
    te_kvpairs_foreach(&kvp, check_append_second_keys, NULL, &compound2);

    TEST_SUBSTEP("Merge with replace");
    te_compound_merge(&compound3, &merge, TE_COMPOUND_MOD_OP_REPLACE);
    must_validate(&compound3);

    if (strcmp(compound3.ptr, merge.ptr) != 0)
        TEST_VERDICT("Replacement merge produced unexpedcted result");

    TEST_SUBSTEP("Merge into empty");
    te_string_reset(&compound3);

    te_compound_merge(&compound3, &merge, TE_COMPOUND_MOD_OP_APPEND);
    must_validate(&compound3);

    if (strcmp(compound3.ptr, merge.ptr) != 0)
        TEST_VERDICT("Merge into a empty string produced unexpedcted result");

    te_string_free(&compound1);
    te_string_free(&compound2);
    te_string_free(&compound3);
    te_string_free(&merge);
    te_vec_free(&bufs);
    te_vec_free(&merge_bufs);
    te_kvpair_fini(&kvp);
    te_kvpair_fini(&merge_kvp);
}

static te_errno
distribute_kv(const char *key, const char *value, void *user)
{
    te_string **compounds = user;
    te_string *target = rand() > RAND_MAX / 2 ? compounds[1] : compounds[0];
    te_compound_set(target, key, TE_COMPOUND_MOD_OP_REPLACE,
                    "%s", value);

    return 0;
}

static void
test_nonoverlap_merge(unsigned int min_items, unsigned int max_items,
                      unsigned int min_len, unsigned int max_len)
{
    unsigned int n_keys = rand_range(min_items, max_items);
    te_kvpair_h kvp;
    te_string base = TE_STRING_INIT;
    te_string compound = TE_STRING_INIT;
    te_string merge = TE_STRING_INIT;
    te_string all = TE_STRING_INIT;
    te_compound_mod_op op;

    generate_random_kv(&kvp, n_keys, min_len, max_len);
    te_kvpair2compound(&all, &kvp);
    te_kvpairs_foreach(&kvp, distribute_kv, NULL,
                       (te_string *[2]){&base, &merge});
    must_validate(&base);
    must_validate(&merge);

    for (op = TE_COMPOUND_MOD_OP_APPEND; op <= TE_COMPOUND_MOD_OP_REPLACE; op++)
    {
        te_string_reset(&compound);
        te_string_append(&compound, "%s", base.ptr);
        te_compound_merge(&compound, &merge, op);
        must_validate(&compound);

        if (!te_compare_bufs(all.ptr, all.len + 1, 1,
                             compound.ptr, compound.len + 1, TE_LL_RING))
            TEST_VERDICT("Incorrect merge");
    }

    te_kvpair_fini(&kvp);
    te_string_free(&base);
    te_string_free(&compound);
    te_string_free(&merge);
    te_string_free(&all);
}

static te_errno
serialize_keys(char *key, size_t idx, char *value, bool has_more, void *user)
{
    te_json_ctx_t *ctx = user;

    if (key == NULL)
    {
        char buf[64];
        TE_SPRINTF(buf, "%zu", idx);
        te_json_add_key(ctx, buf);
    }
    else if (idx > 0)
    {
        char buf[strlen(key) + 2];
        TE_SPRINTF(buf, "%s%zu", key, idx);
        te_json_add_key(ctx, buf);
    }
    else
    {
        te_json_add_key(ctx, key);
    }
    te_json_add_string(ctx, "%s", value);
    UNUSED(has_more);

    return 0;
}

static void
test_json(unsigned int min_items, unsigned int max_items,
          unsigned int min_len, unsigned int max_len)
{
    unsigned int n_items = rand_range(min_items, max_items);
    unsigned int n_keys = rand_range(min_items, max_items);
    te_vec bufs = TE_VEC_INIT_AUTOPTR(char *);
    te_kvpair_h kvp;
    te_string compound = TE_STRING_INIT;
    te_string expected_json = TE_STRING_INIT;
    te_string actual_json = TE_STRING_INIT;
    te_json_ctx_t json_ctx;
    te_json_ctx_t exp_json_ctx;
    //const char **iter;

    generate_random_bufs(&bufs, n_items, min_len, max_len);
    generate_random_kv(&kvp, n_keys, min_len, max_len);

    TEST_SUBSTEP("Checking null serialization");
    json_ctx = (te_json_ctx_t)TE_JSON_INIT_STR(&actual_json);
    te_json_add_compound(&json_ctx, &compound);
    if (strcmp(actual_json.ptr, "null") != 0)
    {
        TEST_VERDICT("Empty compound is not serialized as 'null': '%s'",
                     actual_json.ptr);
    }
    te_string_reset(&actual_json);

    TEST_SUBSTEP("Checking plain string serialization");
    exp_json_ctx = (te_json_ctx_t)TE_JSON_INIT_STR(&expected_json);
    te_json_add_string(&exp_json_ctx, "%s", TE_VEC_GET(const char *, &bufs, 0));
    json_ctx = (te_json_ctx_t)TE_JSON_INIT_STR(&actual_json);
    te_string_append(&compound, "%s", TE_VEC_GET(const char *, &bufs, 0));
    te_json_add_compound(&json_ctx, &compound);
    if (strcmp(actual_json.ptr, expected_json.ptr) != 0)
        TEST_VERDICT("Plain string is incorrectly serialized");
    te_string_reset(&compound);
    te_string_reset(&expected_json);
    te_string_reset(&actual_json);

    TEST_SUBSTEP("Checking array serialization");
    exp_json_ctx = (te_json_ctx_t)TE_JSON_INIT_STR(&expected_json);
    te_json_add_array_str(&exp_json_ctx, true, te_vec_size(&bufs),
                          te_vec_get(&bufs, 0));
    json_ctx = (te_json_ctx_t)TE_JSON_INIT_STR(&actual_json);
    te_vec2compound(&compound, &bufs);
    te_json_add_compound(&json_ctx, &compound);
    if (strcmp(actual_json.ptr, expected_json.ptr) != 0)
        TEST_VERDICT("Array is incorrectly serialized");
    te_string_reset(&compound);
    te_string_reset(&expected_json);
    te_string_reset(&actual_json);

    TEST_SUBSTEP("Checking object serialization");
    exp_json_ctx = (te_json_ctx_t)TE_JSON_INIT_STR(&expected_json);
    json_ctx = (te_json_ctx_t)TE_JSON_INIT_STR(&actual_json);
    te_kvpair2compound(&compound, &kvp);
    te_json_start_object(&exp_json_ctx);
    te_compound_iterate(&compound, serialize_keys, &exp_json_ctx);
    te_json_end(&exp_json_ctx);
    te_json_add_compound(&json_ctx, &compound);
    if (strcmp(actual_json.ptr, expected_json.ptr) != 0)
        TEST_VERDICT("Object is incorrectly serialized");
    te_string_reset(&compound);
    te_string_reset(&expected_json);
    te_string_reset(&actual_json);

    TEST_SUBSTEP("Checking object with duplicate serialization");
    exp_json_ctx = (te_json_ctx_t)TE_JSON_INIT_STR(&expected_json);
    json_ctx = (te_json_ctx_t)TE_JSON_INIT_STR(&actual_json);
    te_kvpair2compound(&compound, &kvp);
    /* Adding the same keys second time! */
    te_kvpair2compound(&compound, &kvp);
    te_json_start_object(&exp_json_ctx);
    te_compound_iterate(&compound, serialize_keys, &exp_json_ctx);
    te_json_end(&exp_json_ctx);
    te_json_add_compound(&json_ctx, &compound);
    if (strcmp(actual_json.ptr, expected_json.ptr) != 0)
        TEST_VERDICT("Object with duplicates is incorrectly serialized");
    te_string_reset(&compound);
    te_string_reset(&expected_json);
    te_string_reset(&actual_json);

    TEST_SUBSTEP("Checking hybrid object serialization");
    exp_json_ctx = (te_json_ctx_t)TE_JSON_INIT_STR(&expected_json);
    json_ctx = (te_json_ctx_t)TE_JSON_INIT_STR(&actual_json);
    te_vec2compound(&compound, &bufs);
    te_kvpair2compound(&compound, &kvp);
    te_json_start_object(&exp_json_ctx);
    te_compound_iterate(&compound, serialize_keys, &exp_json_ctx);
    te_json_end(&exp_json_ctx);
    te_json_add_compound(&json_ctx, &compound);
    if (strcmp(actual_json.ptr, expected_json.ptr) != 0)
        TEST_VERDICT("Object with duplicates is incorrectly serialized");
    te_string_reset(&compound);
    te_string_reset(&expected_json);
    te_string_reset(&actual_json);

    te_string_free(&expected_json);
    te_string_free(&actual_json);
    te_string_free(&compound);
    te_kvpair_fini(&kvp);
    te_vec_free(&bufs);
}

typedef struct key_value {
    const char *key;
    const char *value;
} key_value;

static te_errno
check_fast_append(char *key, size_t idx, char *value, bool has_more, void *user)
{
    const key_value **iter = user;

    if (value == NULL)
        TEST_VERDICT("value is NULL");
    if ((*iter)->value == NULL)
        TEST_VERDICT("Excessive elements in iterator");

    if (strcmp_null((*iter)->key, key) != 0)
        TEST_VERDICT("Unexpected key: '%s'", te_str_empty_if_null(key));

    if (strcmp((*iter)->value, value) != 0)
        TEST_VERDICT("Unexpected value: '%s'", value);

    (*iter)++;
    if ((*iter)->value == NULL && has_more)
        TEST_VERDICT("Iterator is expecting more elements when it should not");
    if ((*iter)->value != NULL && !has_more)
        TEST_VERDICT("Iterator is not expecting more elements when it should");

    UNUSED(idx);
    return 0;
}

static void
test_fastpath(void)
{
    /* The order of elements is crucial! */
    static const key_value prepared[] = {
        {NULL, "a"},
        {NULL, "b"},
        {"a", "c"},
        {"a", "d"},
        {"b", "e"},
        {"c", "f"},
        {NULL, NULL},
    };
    te_string compound = TE_STRING_INIT;
    const key_value *iter;

    for (iter = prepared; iter->value != NULL; iter++)
        te_compound_append_fast(&compound, iter->key, iter->value);

    if (!te_compound_validate_str(compound.ptr))
        TEST_VERDICT("Constructed compound is invalid");

    iter = prepared;
    CHECK_RC(te_compound_iterate_str(compound.ptr, check_fast_append, &iter));
    if (iter->value != NULL)
        TEST_VERDICT("Some elements were not iterated");

    te_string_free(&compound);
}

static te_errno
populate_kv(const char *key, const char *value, void *user)
{
    te_kvpair_h *target = user;
    const char *chk_value;

    te_kvpair_set_compound(target, key, NULL, TE_COMPOUND_MOD_OP_REPLACE,
                           "%s", value);
    CHECK_NOT_NULL(chk_value = te_kvpairs_get(target, key));
    if (strcmp(chk_value, value) != 0)
        TEST_VERDICT("Singleton value improperly added");

    return 0;
}

static te_errno
modify_kv(const char *key, const char *value, void *user)
{
    static char key_sep = TE_COMPOUND_KEY_SEP;
    static char item_sep = TE_COMPOUND_ITEM_SEP;
    te_kvpair_h *target = user;
    char *buf = te_make_printable_buf_by_len(strlen(value) + 1);
    const char *chk_value;

    te_kvpair_set_compound(target, key, value, TE_COMPOUND_MOD_OP_REPLACE,
                           "%s", buf);
    CHECK_NOT_NULL(chk_value = te_kvpairs_get(target, key));
    if (!te_compare_iovecs(7, (const struct iovec[7]){
                {.iov_base = TE_CONST_PTR_CAST(char, value),
                 .iov_len = strlen(value)
                },
                {.iov_base = &item_sep, .iov_len = 1},
                {.iov_base = TE_CONST_PTR_CAST(char, value),
                 .iov_len = strlen(value)
                },
                {.iov_base = &key_sep, .iov_len = 1},
                {.iov_base = buf, .iov_len = strlen(buf)},
                {.iov_base = &item_sep, .iov_len = 1},
                {.iov_base = NULL, .iov_len = 1}
            }, 1, (const struct iovec[1]){
                {.iov_base = TE_CONST_PTR_CAST(char, chk_value),
                 .iov_len = strlen(chk_value) + 1
                }
            }, TE_LL_RING))
    {
        TEST_VERDICT("Unexpected value after setting inner key value");
    }
    te_kvpair_set_compound(target, key, value, TE_COMPOUND_MOD_OP_REPLACE,
                           NULL);
    CHECK_NOT_NULL(chk_value = te_kvpairs_get(target, key));
    te_kvpair_set_compound(target, key, NULL, TE_COMPOUND_MOD_OP_REPLACE, NULL);
    chk_value = te_kvpairs_get(target, key);
    if (chk_value != NULL)
        TEST_VERDICT("Empty compound was not deleted");
    free(buf);

    return 0;
}

static void
test_kvpair_compound(unsigned int min_items, unsigned int max_items,
                     unsigned int min_len, unsigned int max_len)
{
    unsigned int n_items = rand_range(min_items, max_items);
    te_kvpair_h seed_kvp;
    te_kvpair_h target_kvp;

    te_kvpair_init(&target_kvp);
    generate_random_kv(&seed_kvp, n_items, min_len, max_len);
    CHECK_RC(te_kvpairs_foreach(&seed_kvp, populate_kv, NULL, &target_kvp));
    CHECK_RC(te_kvpairs_foreach(&seed_kvp, modify_kv, NULL, &target_kvp));

    if (te_kvpairs_count(&target_kvp, NULL) != 0)
        TEST_VERDICT("Not all keys have been removed");

    te_kvpair_fini(&seed_kvp);
    te_kvpair_fini(&target_kvp);
}

static te_errno
check_deref_value(char *field, size_t idx, char *value,
                  bool has_more, void *user)
{
    if (strcmp_null(value, user) != 0)
    {
        TEST_VERDICT("Unexpected value for %s:%zu",
                     te_str_empty_if_null(field), idx);
    }

    UNUSED(has_more);
    return 0;
}

static te_errno
check_dereference(char *field, size_t idx, char *value,
                  bool has_more, void *user)
{
#define STEM "stem"
#define OTHER_STEM "other_stem"
    te_string name = TE_STRING_INIT;

    te_compound_build_name(&name, STEM, field, idx);
    CHECK_RC(te_compound_dereference_str((const char *)user,
                                         STEM, name.ptr,
                                         check_deref_value,
                                         value));
    if (te_compound_dereference_str((const char *)user,
                                    OTHER_STEM, name.ptr,
                                    check_deref_value,
                                    value) != TE_ENODATA)
    {
        TEST_VERDICT("Value dereferenced for a wrong stem");
    }
    if (!has_more)
    {
        te_string_reset(&name);
        te_compound_build_name(&name, STEM, field, idx + 1);
        if (te_compound_dereference_str((const char *)user,
                                        OTHER_STEM, name.ptr,
                                        check_deref_value,
                                        value) != TE_ENODATA)
        {
            TEST_VERDICT("Value dereferenced for an out of bounds index");
        }
    }

    te_string_free(&name);
#undef STEM
#undef OTHER_STEM
    return TE_EOK;
}

static void
test_dereference(unsigned int min_items, unsigned int max_items,
                 unsigned int min_len, unsigned int max_len)
{
    unsigned int n_items = rand_range(min_items, max_items);
    unsigned int n_keys = rand_range(min_items, max_items);
    te_vec bufs = TE_VEC_INIT_AUTOPTR(char *);
    te_kvpair_h kvp;
    te_string compound = TE_STRING_INIT;

    generate_random_bufs(&bufs, n_items, min_len, max_len);
    te_vec2compound(&compound, &bufs);
    generate_random_kv(&kvp, n_keys, min_len, max_len);
    te_kvpair2compound(&compound, &kvp);
    /* Twice */
    te_kvpair2compound(&compound, &kvp);

    CHECK_RC(te_compound_iterate(&compound, check_dereference, compound.ptr));

    te_kvpair_fini(&kvp);
    te_vec_free(&bufs);
    te_string_free(&compound);
}

static void
test_dereference_corner_cases(void)
{
#define STEM "stem"
    te_string compound = TE_STRING_INIT;
    static const struct {
        const char *name;
        unsigned int copies;
    } fields[] = {
        {NULL, 2},
        {"field", 2},
        {"field0", 1},
        {"field1", 1},
        {"field2", 1},
        {"field_", 1},
    };
    static const struct {
        const char *key;
        const char *exp_value;
    } tests[] = {
        {STEM, ":0"},
        {STEM "1", ":1"},
        {STEM "2", NULL},
        {STEM "_field", "field:0"},
        {STEM "_field0", "field:0"},
        {STEM "_field0_0", "field0:0"},
        {STEM "_field2", "field2:0"},
        {STEM "_field1", "field:1"},
        {STEM "_field1_0", "field1:0"},
        {STEM "_field_", "field_:0"},
        {STEM "_field_0", "field:0"},
        {STEM "_field__0", "field_:0"},
        {STEM "field", NULL},
        {STEM "3", NULL},
        {STEM "field3", NULL},
        {STEM "field1_1", NULL},
    };
    unsigned int i;

    for (i = 0; i < TE_ARRAY_LEN(fields); i++)
    {
        unsigned int n;
        for (n = 0; n < fields[i].copies; n++)
        {
            te_compound_set(&compound, fields[i].name,
                            TE_COMPOUND_MOD_OP_APPEND,
                            "%s:%u", te_str_empty_if_null(fields[i].name), n);
        }
    }

    for (i = 0; i < TE_ARRAY_LEN(tests); i++)
    {
        te_errno rc =
            te_compound_dereference(&compound, STEM,
                                    tests[i].key,
                                    check_deref_value,
                                    TE_CONST_PTR_CAST(char,
                                                      tests[i].exp_value));
        if (tests[i].exp_value != NULL)
        {
            if (rc != 0)
                TEST_VERDICT("Invalid result of dereferencing: %r (%s)", rc, tests[i].key);
        }
        else
        {
            if (rc != TE_ENODATA)
                TEST_VERDICT("Dereferenced value for invalid key");
        }
    }

    te_string_free(&compound);
#undef STEM
}

int
main(int argc, char **argv)
{
    unsigned int min_len;
    unsigned int max_len;
    unsigned int min_items;
    unsigned int max_items;

    TEST_START;
    TEST_GET_UINT_PARAM(min_len);
    TEST_GET_UINT_PARAM(max_len);
    TEST_GET_UINT_PARAM(min_items);
    TEST_GET_UINT_PARAM(max_items);

    TEST_STEP("Check compound string with unnamed fields");
    test_unnamed_compound(min_len, max_len, min_items, max_items);

    TEST_STEP("Check compound string with named fields");
    test_named_compound(min_len, max_len, min_items, max_items);

    TEST_STEP("Check compound string modification modes");
    TEST_SUBSTEP("Unnamed fields");
    test_compound_modify_mode(min_len, max_len, false);
    TEST_SUBSTEP("Named fields");
    test_compound_modify_mode(min_len, max_len, true);

    TEST_STEP("Merge test");
    test_merge(min_items, max_items, min_len, max_len);

    TEST_STEP("Non-overlapping merge test");
    test_nonoverlap_merge(min_items, max_items, min_len, max_len);

    TEST_STEP("Test JSON serialization");
    test_json(min_items, max_items, min_len, max_len);

    TEST_STEP("Test fastpath functions");
    test_fastpath();

    TEST_STEP("Test kvpair of compounds");
    test_kvpair_compound(min_items, max_items, min_len, max_len);

    TEST_STEP("Test dereferencing");
    test_dereference(min_items, max_items, min_len, max_len);
    test_dereference_corner_cases();

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
