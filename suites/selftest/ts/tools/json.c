/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for JSON generating functions
 *
 * Testing JSON generating routines.
 */

/** @page tools_json JSON generation test
 *
 * @objective Testing JSON generating routines.
 *
 * Test the correctness of JSON generating functions.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/json"

#include "te_config.h"

#include "tapi_test.h"
#include "te_json.h"
#include "te_str.h"
#include "te_file.h"

static char *tmp_file = NULL;

static void
do_json_string(te_json_ctx_t *ctx, void *val)
{
    te_json_add_string(ctx, "%s", (const char *)val);
}

static void
do_json_append_string(te_json_ctx_t *ctx, void *val)
{
    const char **strs = val;
    unsigned int i;

    te_json_start_string(ctx);

    for (i = 0; strs[i] != NULL; i++)
        te_json_append_string(ctx, "%s", strs[i]);

    te_json_end(ctx);
}

static void
do_json_int(te_json_ctx_t *ctx, void *val)
{
    te_json_add_integer(ctx, *(intmax_t *)val);
}

static void
do_json_float(te_json_ctx_t *ctx, void *val)
{
    te_json_add_float(ctx, *(double *)val, 6);
}

static void
do_json_array(te_json_ctx_t *ctx, void *val)
{
    const char **strs = val;
    unsigned int i;

    te_json_start_array(ctx);
    for (i = 0; strs[i] != NULL; i++)
        te_json_add_string(ctx, "%s", strs[i]);

    te_json_end(ctx);
}

typedef struct key_value {
    const char *key;
    const char *value;
} key_value;

static void
do_json_object(te_json_ctx_t *ctx, void *val)
{
    key_value *kv = val;
    unsigned int i;

    te_json_start_object(ctx);
    for (i = 0; kv[i].key != NULL; i++)
    {
        te_json_add_key(ctx, kv[i].key);
        te_json_add_string(ctx, "%s", kv[i].value);
    }
    te_json_end(ctx);
}

static void
do_json_optkeys(te_json_ctx_t *ctx, void *val)
{
    key_value *kv = val;
    unsigned int i;

    te_json_start_object(ctx);
    for (i = 0; kv[i].key != NULL; i++)
        te_json_add_key_str(ctx, kv[i].key, kv[i].value);
    te_json_end(ctx);
}

static void
do_json_kvpair(te_json_ctx_t *ctx, void *val)
{
    key_value *kv = val;
    unsigned int i;
    te_kvpair_h kvp;

    te_kvpair_init(&kvp);
    for (i = 0; kv[i].key != NULL; i++)
        te_kvpair_add(&kvp, kv[i].key, "%s", kv[i].value);
    te_json_add_kvpair(ctx, &kvp);
    te_kvpair_fini(&kvp);
}

static void
do_json_array_of_arrays(te_json_ctx_t *ctx, void *val)
{
    int **rows = val;
    unsigned int i;

    te_json_start_array(ctx);
    for (i = 0; rows[i] != NULL; i++)
    {
        unsigned int j;

        te_json_start_array(ctx);

        for (j = 0; rows[i][j] != 0; j++)
            te_json_add_integer(ctx, rows[i][j]);

        te_json_end(ctx);
    }
    te_json_end(ctx);
}

typedef struct array_of_str {
    bool skip_null;
    unsigned int n_strs;
    const char **strs;
} array_of_str;

static void
do_json_array_of_str(te_json_ctx_t *ctx, void *val)
{
    array_of_str *array = val;

    te_json_add_array_str(ctx, array->skip_null, array->n_strs, array->strs);
}

static void
do_json_append_raw_gen(te_json_ctx_t *ctx, void *val, bool use_len)
{
    const char **strs = val;
    unsigned int i;

    te_json_start_raw(ctx);

    for (i = 0; strs[i] != NULL; i++)
        te_json_append_raw(ctx, strs[i], use_len ? strlen(strs[i]) : 0);

    te_json_end(ctx);
}

static void
do_json_append_raw(te_json_ctx_t *ctx, void *val)
{
    return do_json_append_raw_gen(ctx, val, false);
}

static void
do_json_append_raw_len(te_json_ctx_t *ctx, void *val)
{
    return do_json_append_raw_gen(ctx, val, true);
}

static void
check_json_result(te_json_ctx_t *ctx, const char *result,
                  const char *expected)
{
    if (ctx->current_level != 0)
        TEST_VERDICT("Invalid JSON nesting");

    if (strcmp(te_str_empty_if_null(result), expected) != 0)
    {
        ERROR("Unexpected JSON escaping: %s (expected %s)",
              result, expected);
        TEST_VERDICT("JSON escaping is wrong");
    }
}

static void
check_json_str(void *val, void (*func)(te_json_ctx_t *ctx, void *),
               const char *expected)
{
    te_string dest = TE_STRING_INIT;
    te_json_ctx_t ctx = TE_JSON_INIT_STR(&dest);

    func(&ctx, val);

    check_json_result(&ctx, dest.ptr, expected);

    te_string_free(&dest);
}

static void
check_json_file(void *val, void (*func)(te_json_ctx_t *ctx, void *),
                const char *expected, const char *tmp_file)
{
    te_json_ctx_t ctx;
    char buf[1024];
    size_t len;
    FILE *f;

    CHECK_NOT_NULL(f = fopen(tmp_file, "w+"));
    ctx = (te_json_ctx_t)TE_JSON_INIT_FILE(f);

    func(&ctx, val);

    rewind(f);

    len = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[len] = '\0';

    check_json_result(&ctx, buf, expected);
}

static void
check_json(void *val, void (*func)(te_json_ctx_t *ctx, void *),
           const char *expected)
{
    if (tmp_file != NULL)
        check_json_file(val, func, expected, tmp_file);
    else
        check_json_str(val, func, expected);
}

int
main(int argc, char **argv)
{
    bool use_file;

    TEST_START;
    TEST_GET_BOOL_PARAM(use_file);

    if (use_file)
    {
        CHECK_NOT_NULL(tmp_file = te_file_create_unique("/tmp/te_tmp_",
                                                        NULL));
    }

    TEST_STEP("Checking JSON integers");
    check_json((intmax_t[]){0}, do_json_int, "0");
    check_json((intmax_t[]){INT_MAX}, do_json_int, "2147483647");
    check_json((intmax_t[]){-1}, do_json_int, "-1");

    TEST_STEP("Checking JSON floats");
    check_json((double[]){0.0}, do_json_float, "0");
    check_json((double[]){0.5}, do_json_float, "0.5");
    check_json((double[]){-1.0}, do_json_float, "-1");
    check_json((double[]){1e6}, do_json_float, "1e+06");
    check_json((double[]){INFINITY}, do_json_float, "null");
    check_json((double[]){NAN}, do_json_float, "null");

    TEST_STEP("Checking JSON string escaping");
    check_json("", do_json_string, "\"\"");
    check_json("abc def", do_json_string, "\"abc def\"");
    check_json("\x1\a\b\f\n\r\t\v\\/\"\x7F",
               do_json_string,
               "\"\\u0001\\u0007\\b\\f\\n\\r\\t\\u000b"
               "\\\\\\/\\\"\\u007f\"");
    check_json((const char *[]){NULL},
               do_json_append_string, "\"\"");
    check_json((const char *[]){"a", "b", "c xyz", NULL},
               do_json_append_string, "\"abc xyz\"");

    TEST_STEP("Checking JSON arrays");
    check_json((const char *[]){NULL}, do_json_array, "[]");
    check_json((const char *[]){"a", NULL}, do_json_array, "[\"a\"]");
    check_json((const char *[]){"a", "b", NULL}, do_json_array,
               "[\"a\",\"b\"]");

    TEST_STEP("Checking JSON objects");
    check_json((key_value []){{.key = NULL}}, do_json_object, "{}");
    check_json((key_value []){{.key = "a", .value = "b"}, {.key = NULL}},
               do_json_object, "{\"a\":\"b\"}");
    check_json((key_value []){{.key = "a", .value = "b"},
                              {.key = "c", .value = "d"},
                              {.key = NULL}},
               do_json_object, "{\"a\":\"b\",\"c\":\"d\"}");

    TEST_STEP("Checking JSON objects with optional keys");
    check_json((key_value []){{.key = NULL}}, do_json_optkeys, "{}");
    check_json((key_value []){{.key = "a", .value = "b"}, {.key = NULL}},
               do_json_optkeys, "{\"a\":\"b\"}");
    check_json((key_value []){{.key = "c", .value = NULL}, {.key = NULL}},
               do_json_optkeys, "{}");
    check_json((key_value []){{.key = "a", .value = "b"},
                              {.key = "c", .value = "\n"},
                              {.key = NULL}},
               do_json_optkeys, "{\"a\":\"b\",\"c\":\"\\n\"}");
    check_json((key_value []){{.key = "a", .value = NULL},
                              {.key = "c", .value = "\n"},
                              {.key = NULL}},
               do_json_optkeys, "{\"c\":\"\\n\"}");

    TEST_STEP("Checking JSON arrays of arrays");
    check_json((int *[]){NULL}, do_json_array_of_arrays, "[]");
    check_json((int *[]){(int[]){1, 0}, NULL},
               do_json_array_of_arrays, "[[1]]");
    check_json((int *[]){(int[]){0}, NULL},
               do_json_array_of_arrays, "[[]]");
    check_json((int *[]){(int[]){1, 2, 0}, NULL},
               do_json_array_of_arrays, "[[1,2]]");
    check_json((int *[]){(int[]){1, 2, 0}, (int[]){3, 4, 0}, NULL},
               do_json_array_of_arrays, "[[1,2],[3,4]]");

    TEST_STEP("Checking JSON arrays of strings");
    check_json((array_of_str []){{.skip_null = true,
                    .n_strs = 0, .strs = NULL}},
        do_json_array_of_str, "[]");
    check_json((array_of_str []){{.skip_null = true,
                    .n_strs = 1,
                    .strs = (const char *[]){"abc"}}},
        do_json_array_of_str, "[\"abc\"]");
    check_json((array_of_str []){{.skip_null = true,
                    .n_strs = 2,
                    .strs = (const char *[]){"abc", "def"}}},
        do_json_array_of_str, "[\"abc\",\"def\"]");
    check_json((array_of_str []){{.skip_null = true,
                    .n_strs = 1,
                    .strs = (const char *[]){NULL}}},
        do_json_array_of_str, "[]");
    check_json((array_of_str []){{.skip_null = true,
                    .n_strs = 2,
                    .strs = (const char *[]){NULL, "abc"}}},
        do_json_array_of_str, "[\"abc\"]");
    check_json((array_of_str []){{.skip_null = false,
                    .n_strs = 1,
                    .strs = (const char *[]){NULL}}},
        do_json_array_of_str, "[null]");
    check_json((array_of_str []){{.skip_null = false,
                    .n_strs = 2,
                    .strs = (const char *[]){"abc", NULL}}},
        do_json_array_of_str, "[\"abc\",null]");

    TEST_STEP("Checking conversion of kvpairs");
    check_json((key_value []){{.key = NULL}}, do_json_kvpair, "{}");
    check_json((key_value []){{.key = "a", .value = "b"}, {.key = NULL}},
               do_json_kvpair, "{\"a\":\"b\"}");
    check_json((key_value []){{.key = "a", .value = "b"},
                              {.key = "c", .value = "d"},
                              {.key = NULL}},
               do_json_kvpair, "{\"a\":\"b\",\"c\":\"d\"}");

    TEST_STEP("Checking appending RAW json");
    check_json((const char *[]){NULL},
               do_json_append_raw, "");
    check_json((const char *[]){"{\"a\": ", "3, ", "\"b\": \"no\"}", NULL},
               do_json_append_raw, "{\"a\": 3, \"b\": \"no\"}");
    check_json((const char *[]){"{\"x\": 4, \"y\":", " 5}", NULL},
               do_json_append_raw_len, "{\"x\": 4, \"y\": 5}");

    TEST_SUCCESS;

cleanup:

    if (tmp_file != NULL)
    {
        unlink(tmp_file);
        free(tmp_file);
    }

    TEST_END;
}
