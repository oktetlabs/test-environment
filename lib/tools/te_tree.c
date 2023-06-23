/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Tree routines.
 *
 * Routines to deal with tree objects.
 */
#define TE_LGR_USER "TE generic trees"

#include "te_config.h"

#include <assert.h>

#include "logger_api.h"

#include "te_alloc.h"
#include "te_queue.h"
#include "te_tree.h"
#include "te_str.h"
#include "te_enum.h"

typedef TAILQ_HEAD(te_tree_list, te_tree) te_tree_list;

struct te_tree {
    /** Parent tree. */
    const te_tree *parent;
    /** Attributes. */
    te_kvpair_h attrs;
    /** Sibling chain. */
    TAILQ_ENTRY(te_tree) chain;
    /** Children. */
    te_tree_list children;
};

te_tree *
te_tree_alloc(void)
{
    te_tree *t = TE_ALLOC(sizeof(*t));

    te_kvpair_init(&t->attrs);
    TAILQ_INIT(&t->children);

    return t;
}

void
te_tree_free(te_tree *t)
{
    te_tree *iter;
    te_tree *tmp;

    assert(t->parent == NULL);
    TAILQ_FOREACH_SAFE(iter, &t->children, chain, tmp)
    {
        iter->parent = NULL;
        te_tree_free(iter);
    }
    te_kvpair_fini(&t->attrs);
    free(t);
}

te_errno
te_tree_add_attr_va(te_tree *tree, const char *attr, const char *value_fmt,
                    va_list va)
{
    return te_kvpair_add_va(&tree->attrs, attr, value_fmt, va);
}


te_errno
te_tree_add_attr(te_tree *tree, const char *attr, const char *value_fmt, ...)
{
    va_list args;
    te_errno rc;

    va_start(args, value_fmt);
    rc = te_tree_add_attr_va(tree, attr, value_fmt, args);
    va_end(args);

    return rc;
}

typedef struct add_attrs_data {
    te_tree *tree;
    te_errno rc;
} add_attrs_data;

static te_errno
add_attrs_cb(const char *key, const char *value, void *user)
{
    add_attrs_data *data = user;

    if (te_tree_add_attr(data->tree, key, "%s", value) != 0)
        data->rc = TE_EEXIST;

    return 0;
}

te_errno
te_tree_add_attrs(te_tree *tree, const te_kvpair_h *attrs)
{
    add_attrs_data data = {tree, 0};

    te_kvpairs_foreach(attrs, add_attrs_cb, NULL, &data);

    return data.rc;
}

void
te_tree_add_child(te_tree *tree, te_tree *child)
{
    assert(child->parent == NULL);
    child->parent = tree;
    TAILQ_INSERT_TAIL(&tree->children, child, chain);
}

static te_errno
add_child_cb(const char *key, const char *value, void *user)
{
    te_tree *tree = user;
    te_tree *new_child = te_tree_alloc();

    te_tree_add_attr(new_child, TE_TREE_ATTR_NAME, "%s", key);
    te_tree_add_attr(new_child, TE_TREE_ATTR_VALUE, "%s", value);
    te_tree_add_child(tree, new_child);

    return 0;
}

void
te_tree_add_kvpair_children(te_tree *tree, const te_kvpair_h *kvpair)
{
    te_kvpairs_foreach(kvpair, add_child_cb, NULL, tree);
}

const char *
te_tree_get_attr(const te_tree *tree, const char *attr)
{
    return te_kvpairs_get(&tree->attrs, attr);
}

te_errno
te_tree_get_int_attr(const te_tree *tree, const char *attr, intmax_t *result)
{
    const char *str = te_tree_get_attr(tree, attr);

    if (str == NULL)
        return TE_ENOENT;

    return te_strtoimax(str, 0, result);
}

te_errno
te_tree_get_float_attr(const te_tree *tree, const char *attr, double *result)
{
    const char *str = te_tree_get_attr(tree, attr);

    if (str == NULL)
        return TE_ENOENT;

    return te_strtod(str, result);
}

te_errno
te_tree_get_bool_attr(const te_tree *tree, const char *attr, te_bool *result)
{
    static const te_enum_map bool_map[] = {
        {.name = "TRUE", .value = TRUE},
        {.name = "True", .value = TRUE},
        {.name = "true", .value = TRUE},
        {.name = "T", .value = TRUE},
        {.name = "t", .value = TRUE},
        {.name = "YES", .value = TRUE},
        {.name = "Yes", .value = TRUE},
        {.name = "yes", .value = TRUE},
        {.name = "Y", .value = TRUE},
        {.name = "y", .value = TRUE},
        {.name = "1", .value = TRUE},
        {.name = "FALSE", .value = FALSE},
        {.name = "False", .value = FALSE},
        {.name = "false", .value = FALSE},
        {.name = "F", .value = FALSE},
        {.name = "f", .value = FALSE},
        {.name = "NO", .value = FALSE},
        {.name = "No", .value = FALSE},
        {.name = "no", .value = FALSE},
        {.name = "N", .value = FALSE},
        {.name = "n", .value = FALSE},
        {.name = "0", .value = FALSE},
        {.name = "", .value = FALSE},
        TE_ENUM_MAP_END
    };
    const char *str = te_tree_get_attr(tree, attr);
    int bval;

    if (str == NULL)
        return TE_ENOENT;
    bval = te_enum_map_from_str(bool_map, str, -1);
    if (bval < 0)
        return TE_EINVAL;

    if (result != NULL)
        *result = (te_bool)bval;

    return 0;
}

te_bool
te_tree_has_attr(const te_tree *tree, const char *attr, const char *value)
{
    return te_kvpairs_has_kv(&tree->attrs, attr, value);
}

te_bool
te_tree_has_attrs(const te_tree *tree, const te_kvpair_h *attrs)
{
    return te_kvpairs_is_submap(attrs, &tree->attrs);
}

const te_kvpair_h *
te_tree_attrs(const te_tree *tree)
{
    return &tree->attrs;
}

const te_tree *
te_tree_parent(const te_tree *tree)
{
    return tree->parent;
}

const te_tree *
te_tree_root(const te_tree *tree)
{
    while (tree->parent != NULL)
        tree = tree->parent;

    return tree;
}

unsigned int
te_tree_level(const te_tree *tree)
{
    unsigned int level = 0;

    while (tree->parent != NULL)
    {
        level++;
        tree = tree->parent;
    }

    return level;
}

const te_tree *
te_tree_first_child(const te_tree *tree)
{
    return TAILQ_FIRST(&tree->children);
}

const te_tree *
te_tree_last_child(const te_tree *tree)
{
    return TAILQ_LAST(&tree->children, te_tree_list);
}

unsigned int
te_tree_count_children(const te_tree *tree)
{
    const te_tree *child;
    unsigned int count = 0;

    TAILQ_FOREACH(child, &tree->children, chain)
        count++;
    return count;
}

const te_tree *
te_tree_next(const te_tree *tree)
{
    return TAILQ_NEXT(tree, chain);
}

const te_tree *
te_tree_prev(const te_tree *tree)
{
    return tree->parent == NULL ? NULL : TAILQ_PREV(tree, te_tree_list, chain);
}

unsigned int
te_tree_position(const te_tree *tree)
{
    unsigned int position = 0;

    for (; TAILQ_PREV(tree, te_tree_list, chain) != NULL;
         tree = TAILQ_PREV(tree, te_tree_list, chain))
        position++;

    return position;
}

const te_tree *
te_tree_leftmost_leaf(const te_tree *tree)
{
    while (!TAILQ_EMPTY(&tree->children))
        tree = TAILQ_FIRST(&tree->children);
    return tree;
}

const te_tree *
te_tree_rightmost_leaf(const te_tree *tree)
{
    while (!TAILQ_EMPTY(&tree->children))
        tree = TAILQ_LAST(&tree->children, te_tree_list);
    return tree;
}

const te_tree *
te_tree_left(const te_tree *tree)
{
    const te_tree *prev = te_tree_prev(tree);

    if (prev == NULL)
        return te_tree_parent(tree);

    return te_tree_rightmost_leaf(prev);
}

const te_tree *
te_tree_right(const te_tree *tree)
{
    const te_tree *next = te_tree_first_child(tree);

    if (next != NULL)
        return next;

    while ((next = te_tree_next(tree)) == NULL)
    {
        tree = te_tree_parent(tree);
        if (tree == NULL)
            return NULL;
    }

    return next;
}

const te_tree *
te_tree_left_leaf(const te_tree *tree)
{
    const te_tree *prev;

    while ((prev = te_tree_prev(tree)) == NULL)
    {
        tree = te_tree_parent(tree);
        if (tree == NULL)
            return NULL;
    }

    return te_tree_rightmost_leaf(prev);
}

const te_tree *
te_tree_right_leaf(const te_tree *tree)
{
    const te_tree *right = te_tree_right(tree);

    if (right == NULL)
        return NULL;

    return te_tree_leftmost_leaf(right);
}


const te_tree *
te_tree_nth_child(const te_tree *tree, unsigned int nth)
{
    const te_tree *iter;

    TAILQ_FOREACH(iter, &tree->children, chain)
    {
        if (nth == 0)
            return iter;
        nth--;
    }

    return NULL;
}

const te_tree *
te_tree_child_by_attr(const te_tree *tree, const char *attr, const char *value)
{
    const te_tree *iter;

    TAILQ_FOREACH(iter, &tree->children, chain)
    {
        if (te_tree_has_attr(iter, attr, value))
            return iter;
    }
    return NULL;
}

const te_tree *
te_tree_child_by_attrs(const te_tree *tree, const te_kvpair_h *attrs)
{
    const te_tree *iter;

    TAILQ_FOREACH(iter, &tree->children, chain)
    {
        if (te_tree_has_attrs(iter, attrs))
            return iter;
    }

    return NULL;
}

static te_errno
tree_traverse(const te_tree *tree, unsigned int curlevel,
              unsigned int minlevel, unsigned int maxlevel,
              te_tree_traverse_fn *pre_cb, te_tree_traverse_fn *post_cb,
              void *data)
{
    te_errno rc = 0;
    const te_tree *child;

    if (curlevel >= minlevel && pre_cb != NULL)
    {
        rc = pre_cb(tree, data);
        if (rc != 0 && rc != TE_ESKIP)
            return rc;
    }

    if (curlevel < maxlevel && rc != TE_ESKIP)
    {
        TAILQ_FOREACH(child, &tree->children, chain)
        {
            rc = tree_traverse(child, curlevel + 1,
                               minlevel, maxlevel, pre_cb, post_cb, data);
            if (rc != 0)
                return rc;
        }
    }

    if (curlevel >= minlevel && post_cb != NULL)
    {
        rc = post_cb(tree, data);
        if (rc != 0)
            return rc;
    }

    return 0;
}

te_errno
te_tree_traverse(const te_tree *tree, unsigned int minlevel,
                 unsigned int maxlevel,
                 te_tree_traverse_fn *pre_cb, te_tree_traverse_fn *post_cb,
                 void *data)
{
    te_errno rc = tree_traverse(tree, 0, minlevel, maxlevel,
                                pre_cb, post_cb, data);

    if (rc == TE_EOK)
        rc = 0;
    return rc;
}

te_tree *
te_tree_map(const te_tree *tree, te_tree_map_fn *fn, void *data)
{
    te_tree *new_tree = te_tree_alloc();
    te_errno rc = fn(&tree->attrs, &new_tree->attrs, data);
    const te_tree *child;

    if (rc != 0)
    {
        te_tree_free(new_tree);
        return NULL;
    }

    TAILQ_FOREACH(child, &tree->children, chain)
    {
        te_tree *new_child = te_tree_map(child, fn, data);

        if (new_child == NULL)
        {
            te_tree_free(new_tree);
            return NULL;
        }

        te_tree_add_child(new_tree, new_child);
    }

    return new_tree;
}
