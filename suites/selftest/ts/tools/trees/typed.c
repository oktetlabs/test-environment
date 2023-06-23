/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Test for te_tree.h typed values.
 *
 * Testing the typed values of trees.
 */

/** @page tools_trees_typed te_tree.h test
 *
 * @objective Testing the correctness of typed value handling.
 *
 * Test the correctness of typed value handlong
 *
 * @par Test sequence:
 */

/** Logging subsystem entity name */
#define TE_TEST_NAME    "tools/trees/typed"

#include "te_config.h"

#include "tapi_test.h"
#include "te_tree.h"

#include "te_bufs.h"

static void check_string(const char *expected, const char *actual)
{
    if (actual == NULL)
        TEST_VERDICT("Expected '%s', got NULL", expected);

    if (strcmp(expected, actual) != 0)
        TEST_VERDICT("Expected '%s', got '%s'", expected, actual);
}

static void check_validate(const te_tree *tree)
{
    if (!te_tree_validate_types(tree, FALSE, NULL))
        TEST_VERDICT("The tree is expected to be valid, but it is not");
}

static void check_invalid(const te_tree *tree, const te_tree *exp_invalid)
{
    const te_tree *invalid = NULL;

    if (te_tree_validate_types(tree, FALSE, &invalid))
        TEST_VERDICT("The tree is expected to be invvalid, but it validates");

    if (invalid != exp_invalid)
        TEST_VERDICT("Unexpeced invalid node found");
}

int
main(int argc, char **argv)
{
    te_tree *tree = NULL;
    te_tree *annot = NULL;
    te_tree *invalid = NULL;
    intmax_t intval;
    te_bool bval;
    double fval;

    TEST_START;

    TEST_STEP("Testing scalar values");

    TEST_SUBSTEP("Testing explicitly typed string");
    tree = te_tree_make_typed("node", TE_TREE_ATTR_TYPE_STRING, "string");
    check_string(TE_TREE_ATTR_TYPE_STRING, te_tree_get_type(tree));
    check_string("string", te_tree_get_attr(tree, TE_TREE_ATTR_VALUE));
    check_validate(tree);
    te_tree_free(tree);

    TEST_SUBSTEP("Testing integer");
    tree = te_tree_make_typed("node", TE_TREE_ATTR_TYPE_INT, 42);
    check_string(TE_TREE_ATTR_TYPE_INT, te_tree_get_type(tree));
    check_string("42", te_tree_get_attr(tree, TE_TREE_ATTR_VALUE));
    CHECK_RC(te_tree_get_int_attr(tree, TE_TREE_ATTR_VALUE, &intval));
    if (intval != 42)
        TEST_VERDICT("Unexpected integral value: %jd", intval);
    check_validate(tree);
    te_tree_free(tree);

    TEST_SUBSTEP("Testing booleans");
    tree = te_tree_make_typed("node", TE_TREE_ATTR_TYPE_BOOL, TRUE);
    check_string(TE_TREE_ATTR_TYPE_BOOL, te_tree_get_type(tree));
    check_string("true", te_tree_get_attr(tree, TE_TREE_ATTR_VALUE));
    CHECK_RC(te_tree_get_bool_attr(tree, TE_TREE_ATTR_VALUE, &bval));
    if (!bval)
        TEST_VERDICT("Boolean value is false but true is expected ");
    check_validate(tree);
    te_tree_free(tree);

    TEST_SUBSTEP("Testing floating-point values");
    tree = te_tree_make_typed("node", TE_TREE_ATTR_TYPE_FLOAT, 42.0);
    check_string(TE_TREE_ATTR_TYPE_FLOAT, te_tree_get_type(tree));
    check_string("42", te_tree_get_attr(tree, TE_TREE_ATTR_VALUE));
    CHECK_RC(te_tree_get_float_attr(tree, TE_TREE_ATTR_VALUE, &fval));
    if (fval != 42.0)
        TEST_VERDICT("Unexpected integral value: %g", fval);
    check_validate(tree);
    te_tree_free(tree);

    TEST_SUBSTEP("Auto-detecting string nodes");
    tree = te_tree_alloc();
    te_tree_add_attr(tree, TE_TREE_ATTR_VALUE, "string");
    check_string(TE_TREE_ATTR_TYPE_STRING, te_tree_get_type(tree));
    check_validate(tree);

    te_tree_add_attr(tree, TE_TREE_ATTR_TYPE, TE_TREE_ATTR_TYPE_AUTO);
    check_string(TE_TREE_ATTR_TYPE_STRING, te_tree_get_type(tree));
    check_validate(tree);
    te_tree_free(tree);

    TEST_STEP("Testing arrays");
    TEST_SUBSTEP("Testing explicitly typed arrays");
    tree = te_tree_make_typed("node", TE_TREE_ATTR_TYPE_ARRAY,
                              te_tree_make_typed(NULL, TE_TREE_ATTR_TYPE_STRING,
                                                 "string1"),
                              te_tree_make_typed(NULL, TE_TREE_ATTR_TYPE_STRING,
                                                 "string2"), NULL);
    check_string(TE_TREE_ATTR_TYPE_ARRAY, te_tree_get_type(tree));
    if (te_tree_count_children(tree) != 2)
        TEST_VERDICT("Unexpected number of elements in an array");
    check_validate(tree);
    te_tree_free(tree);

    TEST_SUBSTEP("Testing imlicitly typed arrays");
    tree = te_tree_alloc();
    te_tree_add_child(tree, te_tree_make_typed(NULL, TE_TREE_ATTR_TYPE_STRING,
                                               "string1"));
    check_string(TE_TREE_ATTR_TYPE_ARRAY, te_tree_get_type(tree));
    check_validate(tree);

    te_tree_add_attr(tree, TE_TREE_ATTR_TYPE, TE_TREE_ATTR_TYPE_AUTO);
    check_string(TE_TREE_ATTR_TYPE_ARRAY, te_tree_get_type(tree));
    check_validate(tree);

    te_tree_free(tree);

    tree = te_tree_alloc();
    check_string(TE_TREE_ATTR_TYPE_ARRAY, te_tree_get_type(tree));
    check_validate(tree);
    te_tree_free(tree);

    annot = te_tree_alloc();
    te_tree_add_attr(annot, TE_TREE_ATTR_TYPE, TE_TREE_ATTR_TYPE_ANNOTATION);
    te_tree_add_attr(annot, TE_TREE_ATTR_NAME, "annot1");
    tree = te_tree_alloc();
    te_tree_add_child(tree, annot);
    check_string(TE_TREE_ATTR_TYPE_ARRAY, te_tree_get_type(tree));
    check_validate(tree);
    te_tree_add_child(tree, te_tree_make_typed(NULL, TE_TREE_ATTR_TYPE_STRING,
                                               "string1"));
    check_string(TE_TREE_ATTR_TYPE_ARRAY, te_tree_get_type(tree));
    check_validate(tree);
    te_tree_free(tree);

    TEST_STEP("Testing dictionaries");

    TEST_SUBSTEP("Testing explicitly typed dictionaries");
    tree = te_tree_make_typed("node", TE_TREE_ATTR_TYPE_DICT,
                              "name1",
                              te_tree_make_typed(NULL, TE_TREE_ATTR_TYPE_STRING,
                                                 "string1"),
                              "name2",
                              te_tree_make_typed(NULL, TE_TREE_ATTR_TYPE_STRING,
                                                 "string2"),
                              NULL);
    check_string(TE_TREE_ATTR_TYPE_DICT, te_tree_get_type(tree));
    if (te_tree_count_children(tree) != 2)
        TEST_VERDICT("Unexpected number of elements in an array");
    if (te_tree_child_by_attr(tree, TE_TREE_ATTR_NAME, "name1") !=
        te_tree_first_child(tree))
        TEST_VERDICT("Unexpected child found by name");
    check_validate(tree);
    te_tree_free(tree);

    TEST_SUBSTEP("Testing imlicitly typed dictionaries");
    tree = te_tree_alloc();
    te_tree_add_child(tree, te_tree_make_typed("name1",
                                               TE_TREE_ATTR_TYPE_STRING,
                                               "string1"));
    check_string(TE_TREE_ATTR_TYPE_DICT, te_tree_get_type(tree));
    check_validate(tree);
    te_tree_free(tree);

    annot = te_tree_alloc();
    te_tree_add_attr(annot, TE_TREE_ATTR_TYPE, TE_TREE_ATTR_TYPE_ANNOTATION);
    tree = te_tree_alloc();
    te_tree_add_child(tree, annot);
    te_tree_add_child(tree, te_tree_make_typed("name1",
                                               TE_TREE_ATTR_TYPE_STRING,
                                               "string1"));
    check_string(TE_TREE_ATTR_TYPE_DICT, te_tree_get_type(tree));
    check_validate(tree);
    te_tree_free(tree);

    TEST_STEP("Testing unknown types");
    tree = te_tree_alloc();
    te_tree_add_attr(tree, TE_TREE_ATTR_TYPE, "unsupported");
    check_string("unsupported", te_tree_get_type(tree));
    if (te_tree_validate_types(tree, FALSE, NULL))
        TEST_VERDICT("The tree with an unknown type should not validate");
    if (!te_tree_validate_types(tree, TRUE, NULL))
        TEST_VERDICT("The tree with an unknown type should validate");
    te_tree_free(tree);

    TEST_STEP("Testing invalid values");

    TEST_SUBSTEP("Null node with value");
    tree = te_tree_alloc();
    te_tree_add_attr(tree, TE_TREE_ATTR_TYPE, TE_TREE_ATTR_TYPE_NULL);
    te_tree_add_attr(tree, TE_TREE_ATTR_VALUE, "value");
    check_invalid(tree, tree);
    te_tree_free(tree);

    TEST_SUBSTEP("String node with no value");
    tree = te_tree_alloc();
    te_tree_add_attr(tree, TE_TREE_ATTR_TYPE, TE_TREE_ATTR_TYPE_STRING);
    check_invalid(tree, tree);
    te_tree_free(tree);

    TEST_SUBSTEP("Invalid integer node");
    tree = te_tree_alloc();
    te_tree_add_attr(tree, TE_TREE_ATTR_TYPE, TE_TREE_ATTR_TYPE_INT);
    te_tree_add_attr(tree, TE_TREE_ATTR_VALUE, "value");
    check_invalid(tree, tree);
    te_tree_free(tree);

    TEST_SUBSTEP("Invalid float node");
    tree = te_tree_alloc();
    te_tree_add_attr(tree, TE_TREE_ATTR_TYPE, TE_TREE_ATTR_TYPE_FLOAT);
    te_tree_add_attr(tree, TE_TREE_ATTR_VALUE, "value");
    check_invalid(tree, tree);
    te_tree_free(tree);

    TEST_SUBSTEP("Invalid boolean node");
    tree = te_tree_alloc();
    te_tree_add_attr(tree, TE_TREE_ATTR_TYPE, TE_TREE_ATTR_TYPE_BOOL);
    te_tree_add_attr(tree, TE_TREE_ATTR_VALUE, "value");
    check_invalid(tree, tree);
    te_tree_free(tree);

    TEST_SUBSTEP("A mixture of named and unnamed nodes");
    tree = te_tree_alloc();
    te_tree_add_child(tree, te_tree_make_typed("name1",
                                               TE_TREE_ATTR_TYPE_STRING,
                                               "value1"));
    te_tree_add_child(tree, te_tree_make_typed(NULL,
                                               TE_TREE_ATTR_TYPE_STRING,
                                               "value2"));
    te_tree_add_attr(tree, TE_TREE_ATTR_TYPE, TE_TREE_ATTR_TYPE_ARRAY);
    check_invalid(tree, tree);
    te_tree_free(tree);

    TEST_SUBSTEP("Invalid subnode");
    tree = te_tree_alloc();
    invalid = te_tree_alloc();
    te_tree_add_attr(invalid, TE_TREE_ATTR_TYPE, TE_TREE_ATTR_TYPE_INT);
    te_tree_add_attr(invalid, TE_TREE_ATTR_VALUE, "value");
    te_tree_add_child(tree, te_tree_make_typed(NULL,
                                               TE_TREE_ATTR_TYPE_INT, 42));
    te_tree_add_child(tree, invalid);
    check_invalid(tree, invalid);
    te_tree_free(tree);

    TEST_SUBSTEP("Invalid subnode in annotation");
    tree = te_tree_alloc();
    annot = te_tree_alloc();
    te_tree_add_attr(annot, TE_TREE_ATTR_TYPE, TE_TREE_ATTR_TYPE_ANNOTATION);
    invalid = te_tree_alloc();
    te_tree_add_attr(invalid, TE_TREE_ATTR_TYPE, TE_TREE_ATTR_TYPE_INT);
    te_tree_add_attr(invalid, TE_TREE_ATTR_VALUE, "value");
    te_tree_add_child(annot, invalid);
    te_tree_add_child(tree, annot);
    check_validate(tree);
    te_tree_free(tree);

    TEST_SUCCESS;

cleanup:

    TEST_END;
}
