/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for te_tree.h trees and key-value pairs interactions.
 *
 * Testing the interaction between trees and key-value pairs.
 */

/** @page tools_trees_kvpair te_tree.h test
 *
 * @objective Testing tree building and traversal correctness.
 *
 * Test the correctness of tree build and traversal functions.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/trees/kvpair"

#include "te_config.h"

#include "tapi_test.h"
#include "te_tree.h"

#include "te_bufs.h"

static void
build_random_kvpair(te_kvpair_h *kvpair,
                    unsigned int max_name,
                    unsigned int max_value,
                    unsigned int max_items)
{
    unsigned int n = rand_range(1, max_items);
    unsigned int i;

    for (i = 0; i < n; i++)
    {
        char *name = te_make_printable_buf(2, max_name, NULL);
        char *value = te_make_printable_buf(2, max_value, NULL);

        /*
         * There is an unlikely case that a generated name does already
         * have a binding in kvpair. We don't care, just ignore such
         * cases.
         */
        te_kvpair_add(kvpair, name, "%s", value);
        free(name);
        free(value);
    }
}

static te_errno
check_added_node(const te_tree *node, void *data)
{
    const te_kvpair_h *kv = data;
    const char *name = te_tree_get_attr(node, TE_TREE_ATTR_NAME);
    const char *value = te_tree_get_attr(node, TE_TREE_ATTR_VALUE);

    if (te_tree_level(node) != 1)
        TEST_VERDICT("Improper traversal");

    CHECK_NOT_NULL(name);
    CHECK_NOT_NULL(value);

    if (!te_kvpairs_has_kv(kv, name, value))
        TEST_VERDICT("A tree contains unexpected node");

    return 0;
}

static te_errno
check_added_kv(const char *name, const char *value, void *data)
{
    const te_tree *tree = data;
    const te_tree *child = te_tree_child_by_attr(tree, TE_TREE_ATTR_NAME, name);
    const te_tree *child0;
    const char *name0;
    const char *value0;
    te_kvpair_h kvpair2;

    CHECK_NOT_NULL(child);
    CHECK_NOT_NULL(name0 = te_tree_get_attr(child, TE_TREE_ATTR_NAME));
    CHECK_NOT_NULL(value0 = te_tree_get_attr(child, TE_TREE_ATTR_VALUE));

    if (strcmp(name0, name) != 0)
        TEST_VERDICT("Unexpected node name");

    if (strcmp(value0, value) != 0)
        TEST_VERDICT("Unexpected value name");

    te_kvpair_init(&kvpair2);
    te_kvpair_add(&kvpair2, TE_TREE_ATTR_NAME, "%s", name);
    te_kvpair_add(&kvpair2, TE_TREE_ATTR_VALUE, "%s", value);
    CHECK_NOT_NULL(child0 = te_tree_child_by_attrs(tree, &kvpair2));
    if (child != child0)
        TEST_VERDICT("A different child has been found by multi-key lookup");
    CHECK_RC(te_kvpairs_del(&kvpair2, TE_TREE_ATTR_VALUE));
    CHECK_RC(te_kvpair_add(&kvpair2, TE_TREE_ATTR_VALUE, "%s", ""));
    if (te_tree_child_by_attrs(tree, &kvpair2) != NULL)
        TEST_VERDICT("A child is found by impossible lookup");

    te_kvpair_fini(&kvpair2);

    return 0;
}

int
main(int argc, char **argv)
{
    unsigned int n_iterations;
    unsigned int max_name_len;
    unsigned int max_value_len;
    unsigned int max_items;
    unsigned int i;

    TEST_START;
    TEST_GET_UINT_PARAM(n_iterations);
    TEST_GET_UINT_PARAM(max_name_len);
    TEST_GET_UINT_PARAM(max_value_len);
    TEST_GET_UINT_PARAM(max_items);

    TEST_STEP("Checking adding named children");
    for (i = 0; i < n_iterations; i++)
    {
        te_tree *tree = te_tree_alloc();
        te_kvpair_h kvpair;

        te_kvpair_init(&kvpair);
        build_random_kvpair(&kvpair, max_name_len, max_value_len, max_items);

        te_tree_add_kvpair_children(tree, &kvpair);

        CHECK_RC(te_tree_traverse(tree, 1, UINT_MAX, check_added_node, NULL,
                                  &kvpair));
        CHECK_RC(te_kvpairs_foreach(&kvpair, check_added_kv, NULL, tree));

        te_kvpair_fini(&kvpair);
        te_tree_free(tree);
    }

    TEST_STEP("Checking adding attributes in a batch");
    for (i = 0; i < n_iterations; i++)
    {
        te_tree *tree = te_tree_alloc();
        te_kvpair_h kvpair;
        const te_kvpair_h *tree_attrs;

        te_kvpair_init(&kvpair);
        build_random_kvpair(&kvpair, max_name_len, max_value_len, max_items);

        CHECK_RC(te_tree_add_attrs(tree, &kvpair));

        CHECK_NOT_NULL(tree_attrs = te_tree_attrs(tree));

        if (te_kvpairs_count(&kvpair, NULL) !=
            te_kvpairs_count(tree_attrs, NULL))
        {
            TEST_VERDICT("Mismatching number of added and stored attributes");
        }

        if (!te_kvpairs_is_submap(&kvpair, tree_attrs) ||
            !te_kvpairs_is_submap(tree_attrs, &kvpair))
        {
            TEST_VERDICT("Added and stored attributes are not equivalent");
        }

        te_kvpair_fini(&kvpair);
        te_tree_free(tree);
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
