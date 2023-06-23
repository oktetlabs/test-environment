/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for te_tree.h build and traversal functions.
 *
 * Testing tree building and traversal routines.
 */

/** @page tools_trees_traverse te_tree.h test
 *
 * @objective Testing tree building and traversal correctness.
 *
 * Test the correctness of tree build and traversal functions.
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/trees/traverse"

#include "te_config.h"

#include "tapi_test.h"
#include "te_tree.h"

#include "te_str.h"
#include "te_string.h"

#define SEQNO_ATTR "seqno"
#define LEAFNO_ATTR "leafno"
#define LEVEL_ATTR "level"
#define ISLEAF_ATTR "isleaf"
#define POSITION_ATTR "position"
#define N_CHILDREN_ATTR "n_children"

typedef struct build_tree_state {
    unsigned int n_nodes;
    unsigned int n_leaves;
} build_tree_state;

te_tree *
build_random_tree(unsigned int level, unsigned int max_depth,
                  unsigned int max_branching, build_tree_state *state)
{
    te_tree *newt = te_tree_alloc();
    unsigned int new_depth;

    CHECK_NOT_NULL(newt);
    assert(max_depth > 0);
    assert(max_branching > 0);

    CHECK_RC(te_tree_add_attr(newt, LEVEL_ATTR, "%u", level));
    CHECK_RC(te_tree_add_attr(newt, SEQNO_ATTR, "%u", state->n_nodes++));

    new_depth = rand_range(0, max_depth - 1);
    if (new_depth == 0)
    {
        CHECK_RC(te_tree_add_attr(newt, ISLEAF_ATTR, "true"));
        CHECK_RC(te_tree_add_attr(newt, LEAFNO_ATTR, "%u", state->n_leaves++));
    }
    else
    {
        unsigned int n_branches = rand_range(1, max_branching);
        unsigned int pos;

        CHECK_RC(te_tree_add_attr(newt, N_CHILDREN_ATTR, "%u", n_branches));
        for (pos = 0; pos < n_branches; pos++)
        {
            te_tree *child = build_random_tree(level + 1, new_depth,
                                               max_branching, state);

            CHECK_RC(te_tree_add_attr(child, POSITION_ATTR, "%u", pos));
            te_tree_add_child(newt, child);
        }
    }

    return newt;
}

static te_errno
check_tree_integrity(const te_tree *tree, void *data)
{
    const te_tree *root = data;
    unsigned int n_children;
    intmax_t stored_n_children;
    unsigned int level;
    intmax_t stored_level;
    unsigned int i;

    if (te_tree_root(tree) != root)
        TEST_VERDICT("Incorrect tree root");
    if (tree == root)
    {
        if (te_tree_parent(tree) != NULL)
            TEST_VERDICT("Root has parent");
        if (te_tree_has_attr(tree, POSITION_ATTR, NULL))
            TEST_VERDICT("Root has position attribute");
    }
    else
    {
        unsigned int pos = te_tree_position(tree);
        intmax_t stored_pos;

        if (te_tree_parent(tree) == NULL)
            TEST_VERDICT("Lower node has no parent");

        CHECK_RC(te_tree_get_int_attr(tree, POSITION_ATTR, &stored_pos));
        if (stored_pos != (intmax_t)pos)
        {
            TEST_VERDICT("Stored and calculated positions differ: %jd vs %u",
                         stored_pos, pos);
        }

        if (pos == 0)
        {
            if (te_tree_prev(tree) != NULL)
                TEST_VERDICT("First child has previous sibling");
        }
        else
        {
            if (te_tree_prev(tree) == NULL)
                TEST_VERDICT("Non-first child has no previous sibling");
        }
    }

    level = te_tree_level(tree);
    CHECK_RC(te_tree_get_int_attr(tree, LEVEL_ATTR, &stored_level));
    if (stored_level != (intmax_t)level)
    {
        TEST_VERDICT("Stored and calculated levels differ: %jd vs %u",
                     stored_level, level);
    }

    n_children = te_tree_count_children(tree);

    if (n_children == 0)
    {
        if (te_tree_has_attr(tree, N_CHILDREN_ATTR, NULL))
            TEST_VERDICT("Leaf has stored number of children");
        if (!te_tree_has_attr(tree, ISLEAF_ATTR, "true"))
            TEST_VERDICT("Leaf has no isleaf attribute");
        if (te_tree_first_child(tree) != NULL)
            TEST_VERDICT("Leaf has a child");
    }
    else
    {
        CHECK_NOT_NULL(te_tree_first_child(tree));
        CHECK_NOT_NULL(te_tree_last_child(tree));

        if (te_tree_has_attr(tree, ISLEAF_ATTR, NULL))
            TEST_VERDICT("Non-leaf has isleaf attribute");

        CHECK_RC(te_tree_get_int_attr(tree, N_CHILDREN_ATTR,
                                      &stored_n_children));

        if (stored_level != (intmax_t)level)
        {
            TEST_VERDICT("Stored and calculated number of children differ: "
                         "%jd vs %u", stored_n_children, n_children);
        }

        for (i = 0; i < n_children; i++)
        {
            char n[16];
            const te_tree *child, *child0;
            TE_SPRINTF(n, "%u", i);
            CHECK_NOT_NULL(child = te_tree_nth_child(tree, i));
            CHECK_NOT_NULL(child0 = te_tree_child_by_attr(tree,
                                                          POSITION_ATTR, n));

            if (child0 != child)
            {
                TEST_VERDICT("Inconsistency between index and "
                             "by-attribute lookup:");
            }
        }
        if (te_tree_nth_child(tree, n_children) != NULL)
            TEST_VERDICT("Unexpected child returned by index lookup");
    }

    return 0;
}

static te_errno
copy_seqno(const te_kvpair_h *src, te_kvpair_h *dest, void *data)
{
    UNUSED(data);
    te_kvpairs_copy_key(dest, src, SEQNO_ATTR);

    return 0;
}

static te_errno
check_tree_copy(const te_tree *tree, void *data)
{
    intmax_t *exp_seqno = data;
    intmax_t stored_seqno;

    CHECK_RC(te_tree_get_int_attr(tree, SEQNO_ATTR, &stored_seqno));

    if (stored_seqno != *exp_seqno)
    {
        TEST_VERDICT("Sequence number improperly copied: %jd vs %jd",
                     *exp_seqno, stored_seqno);
    }

    if (te_kvpairs_count(te_tree_attrs(tree), NULL) != 1)
        TEST_VERDICT("Too many attributes copied");

    (*exp_seqno)++;

    return 0;
}

static te_errno
check_max_depth(const te_tree *tree, void *data)
{
    unsigned int max_level = *(unsigned int *)data;

    if (te_tree_level(tree) > max_level)
        TEST_VERDICT("Max level limitation was ignored");

    return 0;
}

static te_errno
check_min_depth(const te_tree *tree, void *data)
{
    unsigned int min_level = *(unsigned int *)data;

    if (te_tree_level(tree) < min_level)
        TEST_VERDICT("Min level limitation was ignored");

    return 0;
}

static te_errno
check_block_descend(const te_tree *tree, void *data)
{
    unsigned int max_level = *(unsigned int *)data;
    unsigned int level = te_tree_level(tree);

    if (level > max_level)
        TEST_VERDICT("Descending was not blocked");

    return level == max_level ? TE_ESKIP : 0;
}

static te_errno
check_children_traverse(const te_tree *tree, void *data)
{
    unsigned int *count = data;

    if (te_tree_level(tree) != 1)
        TEST_VERDICT("Non-children traversed");

    (*count)++;

    return 0;
}

static te_errno
check_stop(const te_tree *tree, void *data)
{
    unsigned int max_seqno = *(unsigned int *)data;
    intmax_t this_seqno;

    CHECK_RC(te_tree_get_int_attr(tree, SEQNO_ATTR, &this_seqno));

    if (this_seqno > (intmax_t)max_seqno)
        TEST_VERDICT("Traversal was not stopped");

    return this_seqno == (intmax_t)max_seqno ? TE_EOK : 0;
}

int
main(int argc, char **argv)
{
    unsigned int n_iterations;
    unsigned int max_depth;
    unsigned int max_branching;
    unsigned int i;

    TEST_START;
    TEST_GET_UINT_PARAM(n_iterations);
    TEST_GET_UINT_PARAM(max_depth);
    TEST_GET_UINT_PARAM(max_branching);

    TEST_STEP("Testing random trees");
    for (i = 0; i < n_iterations; i++)
    {
        build_tree_state state = {0, 0};
        te_tree *tree = build_random_tree(0, max_depth, max_branching, &state);
        te_tree *mapped = NULL;
        const te_tree *iter = NULL;
        const te_tree *prev_iter = NULL;
        intmax_t exp_no;
        intmax_t actual_no;
        unsigned int limit;
        unsigned int count;

        CHECK_NOT_NULL(tree);

        TEST_SUBSTEP("Check tree integrity");
        CHECK_RC(te_tree_traverse(tree, 0, UINT_MAX, check_tree_integrity, NULL,
                                  tree));

        TEST_SUBSTEP("Check limited traversal");
        limit = rand_range(0, max_depth);
        CHECK_RC(te_tree_traverse(tree, 0, limit,
                                  check_max_depth, check_max_depth, &limit));
        limit = rand_range(0, max_depth);
        CHECK_RC(te_tree_traverse(tree, limit, UINT_MAX,
                                  check_min_depth, check_min_depth, &limit));
        limit = rand_range(0, max_depth);
        CHECK_RC(te_tree_traverse(tree, 0, UINT_MAX,
                                  check_block_descend, check_max_depth,
                                  &limit));
        count = 0;
        CHECK_RC(te_tree_traverse(tree, 1, 1,
                                  check_children_traverse,
                                  check_children_traverse,
                                  &count));
        /*
         * Multiple by two because check_children_traverse was called both
         * as a pre-callback and as a post-callback.
         */
        if (count != te_tree_count_children(tree) * 2)
            TEST_VERDICT("Not all children of the root have been traversed");

        limit = rand_range(0, max_branching * max_branching);
        CHECK_RC(te_tree_traverse(tree, 0, UINT_MAX, check_stop, NULL,
                                  &limit));

        TEST_SUBSTEP("Check tree linear ordering");
        for (iter = tree, prev_iter = NULL, exp_no = 0;
             iter != NULL;
             prev_iter = iter, iter = te_tree_right(iter), exp_no++)
        {
            CHECK_RC(te_tree_get_int_attr(iter, SEQNO_ATTR, &actual_no));
            if (actual_no != exp_no)
            {
                TEST_VERDICT("Unexpected sequence number: %jd != %jd",
                             exp_no, actual_no);
            }
            if (te_tree_left(iter) != prev_iter)
                TEST_VERDICT("Invalid left neighbour");
        }
        if (te_tree_rightmost_leaf(tree) != prev_iter)
            TEST_VERDICT("Invalid rightmost node");

        TEST_SUBSTEP("Check tree leaf ordering");
        for (iter = te_tree_leftmost_leaf(tree), prev_iter = NULL, exp_no = 0;
             iter != NULL;
             prev_iter = iter, iter = te_tree_right_leaf(iter), exp_no++)
        {
            CHECK_RC(te_tree_get_int_attr(iter, LEAFNO_ATTR, &actual_no));
            if (actual_no != exp_no)
            {
                TEST_VERDICT("Unexpected leaf sequence number: %jd != %jd",
                             exp_no, actual_no);
            }
            if (te_tree_left_leaf(iter) != prev_iter)
            {
                TEST_VERDICT("Invalid left leaf");
            }
            if (!te_tree_has_attr(iter, ISLEAF_ATTR, "true"))
                TEST_VERDICT("Leaf is not leaf");
        }
        if (te_tree_rightmost_leaf(tree) != prev_iter)
            TEST_VERDICT("Invalid rightmost node");

        TEST_SUBSTEP("Check tree mapping");
        CHECK_NOT_NULL(mapped = te_tree_map(tree, copy_seqno, NULL));
        exp_no = 0;
        CHECK_RC(te_tree_traverse(mapped, 0, UINT_MAX, check_tree_copy, NULL,
                                  &exp_no));
        te_tree_free(mapped);

        te_tree_free(tree);
    }

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
