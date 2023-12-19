/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs. All rights reserved. */
/** @file
 * @brief Test for ring buffers.
 *
 * Testing vector ring buffers.
 */

/** @page tools_rings te_ring.h test
 *
 * @objective Testing ring buffer functions.
 *
 * Test the correctness of ring buffer implementation.
 *
 * The test is more useful when run under valgrind.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/rings"

#include "te_config.h"

#include <string.h>

#include "tapi_test.h"
#include "te_ring.h"

static te_vec_item_destroy_fn count_destroy;
static unsigned int destroy_cnt;
static void
count_destroy(const void *item)
{
    UNUSED(item);
    destroy_cnt++;
}

static void
check_ring_put(unsigned int max_ring_size)
{
    unsigned int ring_size = rand_range(1, max_ring_size);
    te_ring ring = TE_RING_INIT(int, count_destroy, ring_size);
    unsigned int i;
    te_errno put_rc;
    static const int value = 1;

    destroy_cnt = 0;
    for (i = 0; i < ring_size; i++)
    {
        CHECK_RC(te_ring_put(&ring, &value));
        if (ring.fill != i + 1)
        {
            ERROR("Fill should be %u, but it is %zu", i + 1, ring.fill);
            TEST_VERDICT("Ring buffer is not filled properly");
        }
    }

    put_rc = te_ring_put(&ring, &value);
    if (put_rc != TE_ENOBUFS)
        TEST_VERDICT("No ring buffer overrun detected");

    te_ring_free(&ring);
    if (destroy_cnt != ring_size + 1)
    {
        TEST_VERDICT("A destructor is called %u times instead of %u",
                     destroy_cnt, ring_size + 1);
    }
}

static void
check_ring_get(unsigned int max_ring_size)
{
    unsigned int ring_size = rand_range(1, max_ring_size);
    te_ring ring = TE_RING_INIT(unsigned int, count_destroy, ring_size);
    unsigned int i;
    unsigned int n;

    for (n = 0; n < ring_size; n++)
    {
        unsigned int batch_size = rand_range(1, ring_size);
        te_errno get_rc = te_ring_get(&ring, NULL);

        if (get_rc != TE_ENODATA)
            TEST_VERDICT("Successful get from an empty ring");

        for (i = 1; i <= batch_size; i++)
            CHECK_RC(te_ring_put(&ring, &i));

        for (i = 0; i < batch_size; i++)
        {
            unsigned int value = 0;

            CHECK_RC(te_ring_get(&ring, &value));
            if (value != i + 1)
                TEST_VERDICT("Unexpected read value: %u != %u", value, i + 1);
        }
    }

    te_ring_free(&ring);
}

static void
check_ring_get_many(unsigned int max_ring_size)
{
    unsigned int ring_size = rand_range(1, max_ring_size);
    te_ring ring = TE_RING_INIT(unsigned int, count_destroy, ring_size);
    unsigned int i;
    unsigned int n;

    destroy_cnt = 0;
    for (n = 0; n < ring_size; n++)
    {
        unsigned int batch_size = rand_range(1, ring_size);
        unsigned int elems[batch_size];
        te_vec tmp = TE_VEC_INIT(unsigned int);
        size_t actual = te_ring_get_many(&ring, batch_size, &tmp);

        if (actual != 0)
            TEST_VERDICT("Successful get from an empty ring");

        for (i = 0; i < batch_size; i++)
            elems[i] = i;

        actual = te_ring_put_many(&ring, batch_size, elems);
        if (actual != batch_size)
        {
            TEST_VERDICT("Only %zu elements put instead of %u",
                         actual, batch_size);
        }

        destroy_cnt = 0;
        actual = te_ring_get_many(&ring, batch_size, &tmp);
        if (actual != batch_size)
        {
            TEST_VERDICT("Only %zu elements got instead of %u",
                         actual, batch_size);
        }
        if (te_vec_size(&tmp) != actual)
        {
            TEST_VERDICT("Only %zu elements are appended instead of %zu",
                         te_vec_size(&tmp), actual);
        }

        for (i = 0; i < batch_size; i++)
        {
            if (TE_VEC_GET(unsigned int, &tmp, i) != i)
            {
                TEST_VERDICT("%u'th element has unexpected value: %u",
                             i, TE_VEC_GET(unsigned int, &tmp, i));
            }
        }
        if (destroy_cnt > 0)
            TEST_VERDICT("A destructor was called when it should not");

        te_vec_free(&tmp);
    }

    te_ring_free(&ring);
}

static void
check_ring_resize(unsigned int max_ring_size)
{
    unsigned int ring_size = rand_range(1, max_ring_size);
    te_ring ring = TE_RING_INIT(unsigned int, count_destroy, ring_size);
    unsigned int batch_size = rand_range(1, ring_size);
    unsigned int drop = rand_range(0, batch_size);
    unsigned int new_ring_size = rand_range(1, max_ring_size);
    unsigned int batch[batch_size];
    te_vec result = TE_VEC_INIT(unsigned int);
    unsigned int result_count;
    unsigned int i;
    unsigned int expected_count;
    unsigned int expected_start;

    for (i = 0; i < batch_size; i++)
        batch[i] = i;

    destroy_cnt = 0;
    te_ring_put_many(&ring, batch_size, batch);
    te_ring_get_many(&ring, drop, NULL);
    if (destroy_cnt != drop)
    {
        TEST_VERDICT("Destructor called %u times instead of %u",
                     destroy_cnt, drop);
    }

    te_ring_resize(&ring, new_ring_size);

    if (new_ring_size > batch_size - drop)
    {
        expected_count = batch_size - drop;
        expected_start = drop;
    }
    else
    {
        expected_count = new_ring_size;
        /*
         * Though it may not be evident from the start,
         * but this is the correct value of `expected_start`,
         * not `batch_size - drop - new_ring_size`:
         * - initially there were `batch_size` items;
         * - then `drop` items have been dropped,
         *   so `batch_size - drop` remain and the first non-dropped element
         *   has the value of `drop`;
         * - since `new_ring_size < batch_size - drop` in this branch,
         *   te_ring_resize() drops _additional_
         *   `batch_size - drop - new_ring_size items`, so the first
         *   non-dropped element has the value of
         *   `drop + batch_size - drop - new_ring_size` which is exactly
         *   `batch_size - new_ring_size`.
         */
        expected_start = batch_size - new_ring_size;
    }

    result_count = te_ring_copy(&ring, batch_size, &result);
    if (result_count != expected_count)
    {
        TEST_VERDICT("Only %u items copied instead of %u", result_count,
                     expected_count);
    }
    if (result_count != te_vec_size(&result))
    {
        TEST_VERDICT("Result vector contains only %zu instead of %u",
                     te_vec_size(&result), result_count);
    }
    for (i = 0; i < result_count; i++)
    {
        unsigned int v = TE_VEC_GET(unsigned int, &result, i);

        if (v != i + expected_start)
        {
            TEST_VERDICT("Unexpected value at %u: %u != %u",
                         i, v, i + expected_start);
        }
    }

    te_vec_free(&result);
    te_ring_free(&ring);
}

static void
check_ring_null_destructor(unsigned int max_ring_size)
{
    unsigned int ring_size = rand_range(1, max_ring_size);
    te_ring ring = TE_RING_INIT(unsigned int, NULL, ring_size);
    unsigned int i;

    for (i = 0; i < ring_size; i++)
    {
        unsigned int value = rand();
        unsigned int got_value;

        CHECK_RC(te_ring_put(&ring, &value));
        CHECK_RC(te_ring_get(&ring, &got_value));

        if (value != got_value)
            TEST_VERDICT("Unexpected value: %u != %u", value, got_value);

        CHECK_RC(te_ring_put(&ring, &value));
        CHECK_RC(te_ring_get(&ring, NULL));
    }

    te_ring_free(&ring);
}

/*
 * This test actually makes real sense only under valgrind:
 * we check that there are indeed no memory leaks from ring elements.
 *
 * So there are intentionally no correctness checks for return values etc:
 * all this is covered by previous tests.
 */
static void
check_ring_heap_buf(unsigned int max_ring_size)
{
    unsigned int ring_size = rand_range(1, max_ring_size);
    te_ring ring = TE_RING_INIT_AUTOPTR(void *, ring_size);
    unsigned int i;

    for (i = 0; i < ring_size; i++)
    {
        unsigned int put_size = rand_range(1, max_ring_size);
        unsigned int get_size = rand_range(1, put_size);
        void *elements[put_size];
        unsigned int j;
        te_vec tmp = TE_VEC_INIT_DESTROY(void *, te_vec_item_free_ptr);
        size_t actual;

        for (j = 0; j < put_size; j++)
            elements[j] = TE_ALLOC(1);

        actual = te_ring_put_many(&ring, put_size, elements);
        for (j = actual; j < put_size; j++)
            free(elements[j]);
        te_ring_get_many(&ring, get_size, &tmp);

        te_vec_free(&tmp);
    }

    te_ring_free(&ring);
}

int
main(int argc, char **argv)
{
    unsigned int i;
    unsigned int n_iterations;
    unsigned int max_ring_size;

    TEST_START;

    TEST_GET_UINT_PARAM(n_iterations);
    TEST_GET_UINT_PARAM(max_ring_size);

    TEST_STEP("Checking ring put");
    for (i = 0; i < n_iterations; i++)
        check_ring_put(max_ring_size);

    TEST_STEP("Checking ring get");
    for (i = 0; i < n_iterations; i++)
        check_ring_get(max_ring_size);

    TEST_STEP("Checking bulk ring set/get");
    for (i = 0; i < n_iterations; i++)
        check_ring_get_many(max_ring_size);

    TEST_STEP("Checking ring resize");
    for (i = 0; i < n_iterations; i++)
        check_ring_resize(max_ring_size);

    TEST_STEP("Checking ring ops with null destructor");
    for (i = 0; i < n_iterations; i++)
        check_ring_null_destructor(max_ring_size);

    TEST_STEP("Checking ring ops with real heap buffers");
    for (i = 0; i < n_iterations; i++)
        check_ring_heap_buf(max_ring_size);

    TEST_SUCCESS;

cleanup:
    TEST_END;
}
