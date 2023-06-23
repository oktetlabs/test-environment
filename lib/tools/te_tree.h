/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Generic tree routines.
 *
 * @defgroup te_tools_te_tree Tree routines.
 * @ingroup te_tools
 * @{
 *
 * Trees are recursive objects which have an attached list
 * of named attributes.
 *
 * Attributes are just strings, but some attributes may have
 * special meaning for various functions, in particular defining
 * - the name of a given subtree;
 * - its textual value.
 *
 * Trees are mostly immutable: there are functions to build
 * them incrementally, but once a tree is complete, it is not
 * meant to change.
 *
 * Trees cannot be shared: if a tree is added as a child of
 * some other tree, it cannot be later added as a child of yet
 * another tree.
 *
 * The linear ordering of nodes is defined as follows:
 * - all children follow their parent;
 * - siblings follow each other.
 *
 * This means that if a node has children, its immediate successor
 * will be its first child, otherwise its next sibling if there is
 * one, otherwise the next sibling of its parent and so on.
 * In the same way, the immediate predecessor of a node will be
 * the rightmost leaf node of its previous sibling, if any, otherwise
 * its parent.
 *
 */

#ifndef __TE_TREE_H__
#define __TE_TREE_H__

#include <stdarg.h>

#include "te_errno.h"
#include "te_defs.h"
#include "te_kvpair.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An opaque object representing trees.
 */
typedef struct te_tree te_tree;

/**
 * @name Well-known attributes.
 *
 * A tree may have any other attributes and these
 * attributes have special meaning only for some
 * functions like te_tree_add_kvpair_children().
 * However, it is advised that API users adhere to
 * this convention whenever possible.
 * @{
 */

/** Tree name. */
#define TE_TREE_ATTR_NAME "name"
/** Tree value. */
#define TE_TREE_ATTR_VALUE "value"

/**@}*/

/**
 * Allocate an empty tree object.
 *
 * @return a dynamically allocated empty tree
 *         (should be freed with te_tree_free())
 */
extern te_tree *te_tree_alloc(void);

/**
 * Deallocate @p tree and all its subtrees.
 *
 * The function must only be called on a root tree,
 * i.e. the one that is not attached to any parent.
 *
 * @param tree  root tree to deallocate
 */
extern void te_tree_free(te_tree *tree);

/**
 * Add an attribute to a tree.
 *
 * @param tree      tree to modify
 * @param attr      attribute name
 * @param value_fmt attribute value format
 * @oaram va        variadic list
 *
 * @return status code
 * @retval TE_EEXIST @p attr already exists in @p tree
 */
extern te_errno te_tree_add_attr_va(te_tree *tree, const char *attr,
                                    const char *value_fmt, va_list va);

/**
 * Add an attribute to a tree.
 *
 * @param tree      tree to modify
 * @param attr      attribute name
 * @param value_fmt attribute value format
 * @oaram ...       variadic arguments
 *
 * @return status code
 * @retval TE_EEXIST  @p attr already exists in @p tree
 */
extern te_errno te_tree_add_attr(te_tree *tree, const char *attr,
                                 const char *value_fmt, ...);

/**
 * Add attributes from the kvpair @p attrs.
 *
 * @param tree  tree to modify
 * @param attrs list of attributes
 *
 * @return status code
 * @retval TE_EEXIST  some attributes from @p attrs were already in @p tree
 *
 * @note If the function returns TE_EEXIST code, all attributes from
 *       @p attrs that were not in @p tree, are still added to it.
 */
extern te_errno te_tree_add_attrs(te_tree *tree, const te_kvpair_h *attrs);

/**
 * Add a child to a tree.
 *
 * @p child becomes the last child of @p tree.
 *
 * @p child must not be already added to any other tree and it must not be
 * deallocated by the caller after it has been added.
 *
 * @param tree   tree to modify
 * @param child  child to add
 */
extern void te_tree_add_child(te_tree *tree, te_tree *child);

/**
 * Convert a key-value mapping to children of @p tree.
 *
 * All key-value pairs from @p kvpair are converted to elementary trees
 * each having two attributes: TE_TREE_ATTR_NAME holding the key and
 * TE_TREE_ATTR_VALUE holding the value. These trees are appended as
 * children to @p tree in the order of keys in @p kvpair.
 *
 * @param tree   tree to modify
 * @param kvpair list of child descriptions
 */
extern void te_tree_add_kvpair_children(te_tree *tree,
                                        const te_kvpair_h *kvpair);

/**
 * Get a value of an attribute of a tree.
 *
 * @param tree   tree
 * @param attr   attribute name
 *
 * @return attribute value or @c NULL if no such attribute
 */
extern const char *te_tree_get_attr(const te_tree *tree, const char *attr);

/**
 * @name Typed accessors.
 *
 * These functions allow to treat the value of a given attribute as
 * belonging to a certain type, such as integer. Note that all attributes
 * are still just strings, they have no attached type informations, so
 * all these functions just inspect the current value of an attribute.
 *
 * All the functions accept a pointer to an output location which may be
 * @c NULL, in which case the functions may be used just to check that
 * an attribute value conforms to a given type.
 *
 * @{
 */

/**
 * Get an integral value of an attribute of a tree.
 *
 * @param[in]  tree    tree
 * @param[in]  attr    attribute name
 * @param[out] result  resulting value
 *
 * @return status code
 * @retval TE_ENOENT   No attribute @p attr in @p tree.
 * @retval TE_ERANGE   The value does not fit into @c intmax_t.
 * @retval TE_EINVAL   The value is not a valid integer in any base.
 */
extern te_errno te_tree_get_int_attr(const te_tree *tree, const char *attr,
                                     intmax_t *result);

/**
 * Get a floating-point value of an attribute of a tree.
 *
 * @param[in]  tree    tree
 * @param[in]  attr    attribute name
 * @param[out] result  resulting value
 *
 * @return status code
 * @retval TE_ENOENT   No attribute @p attr in @p tree.
 * @retval TE_ERANGE   The value does not fit into @c double
 * @retval TE_EINVAL   The value is not a valid floating-point
 *                     representation.
 */
extern te_errno te_tree_get_float_attr(const te_tree *tree, const char *attr,
                                       double *result);

/**
 * Get a boolean value of an attribute of a tree.
 *
 * All "natural" way of representing booleans are supported:
 * - @c TRUE, @c true, @c True, @c T, @c t, @c YES, @c yes,
 *   @c Yes, @c Y, @c y, @c 1 all map to @c TRUE
 * - @c FALSE, @c false, @c False, @c F, @c f, @c NO, @c no,
 * - @c No, @c N, @xc n, @c 0 and empty string all map to @c FALSE.
 *
 * @param[in]  tree    tree
 * @param[in]  attr    attribute name
 * @param[out] result  resulting value
 *
 * @return status code
 * @retval TE_ENOENT   No attribute @p attr in @p tree.
 * @retval TE_EINVAL   The value is not a valid boolean representation.
 */
extern te_errno te_tree_get_bool_attr(const te_tree *tree, const char *attr,
                                      te_bool *result);

/**@}*/

/**
 * Test whether a tree has an attribute with a given name and value.
 *
 * The presence of the attribute is checked by te_kvpairs_has_kv(), so
 * both @p key and @p value may be @c NULL, meaning "any string".
 *
 * @param tree   tree
 * @param attr   attribute name (may be @c NULL)
 * @param value  attribute value (may be @c NULL)
 *
 * @return @c TRUE if there is an attribute with a given name and value
 */
extern te_bool te_tree_has_attr(const te_tree *tree, const char *attr,
                                const char *value);

/**
 * Test whether a tree has all attributes specified by @p attrs.
 *
 * @param tree   tree
 * @param attrs  list of name-value pairs
 *
 * @return @c TRUE if all attributes from @p attrs have matching
 *         values in @p tree
 */
extern te_bool te_tree_has_attrs(const te_tree *tree, const te_kvpair_h *attrs);

/**
 * Get an immutable attribute list.
 *
 * @param tree  tree
 *
 * @return a list of attribute name-value pairs
 */
extern const te_kvpair_h *te_tree_attrs(const te_tree *tree);

/**
 * Get the parent of a tree.
 *
 * @param tree  tree
 *
 * @return the parent of @p tree or @c NULL
 */
extern const te_tree *te_tree_parent(const te_tree *tree);

/**
 * Get the root of a tree.
 *
 * @param tree  tree
 *
 * @return the root of @p tree (never @c NULL)
 */
extern const te_tree *te_tree_root(const te_tree *tree);

/**
 * Get the level of a tree.
 *
 * The level is the length of a path from the root to
 * a given subtree. If the tree is the root, the level is 0.
 *
 * @param tree   tree
 *
 * @return the level of @p tree
 */
extern unsigned int te_tree_level(const te_tree *tree);

/**
 * Get the first (leftmost) child of a tree.
 *
 * @param tree   tree
 *
 * @return the first child of @p tree (@c NULL if none)
 */
extern const te_tree *te_tree_first_child(const te_tree *tree);

/**
 * Get the last (rightmost) child of a tree.
 *
 * @param tree   tree
 *
 * @return the first child of @p tree (@c NULL if none)
 */
extern const te_tree *te_tree_last_child(const te_tree *tree);

/**
 * Count the number of children of a tree.
 *
 * @param tree   tree
 *
 * @return the number of children of @p tree
 */
extern unsigned int te_tree_count_children(const te_tree *tree);

/**
 * Get the next sibling of @p tree.
 *
 * @param tree   tree
 *
 * @return the next sibling of @p tree or @c NULL
 */
extern const te_tree *te_tree_next(const te_tree *tree);

/**
 * Get the previous sibling of @p tree.
 *
 * @param tree   tree
 *
 * @return the previous sibling of @p tree or @c NULL
 */
extern const te_tree *te_tree_prev(const te_tree *tree);

/**
 * Get the position of a tree.
 *
 * The position is the ordinal number of a tree
 * among its siblings. The first child of its parent
 * has a position @c 0.
 *
 * @param tree   tree
 *
 * @return the position of @p tree
 */
extern unsigned int te_tree_position(const te_tree *tree);

/**
 * Get the leftmost leaf descendant of @p tree.
 *
 * @param tree   tree
 *
 * @return the leftmost leaf of @p tree (never @c NULL)
 */
extern const te_tree *te_tree_leftmost_leaf(const te_tree *tree);

/**
 * Get the rightmost leaf descendant of @p tree.
 *
 * @param tree   tree
 *
 * @return the leftmost leaf of @p tree (never @c NULL)
 */
extern const te_tree *te_tree_rightmost_leaf(const te_tree *tree);

/**
 * Get the tree immediately preceding @p tree in linear order.
 *
 * @param tree   tree
 *
 * @return the tree immediately preceding @p tree or @c NULL
 */
extern const te_tree *te_tree_left(const te_tree *tree);

/**
 * Get the tree immediately following @p tree in linear order.
 *
 * @param tree   tree
 *
 * @return the tree immediately following @p tree or @c NULL
 */
extern const te_tree *te_tree_right(const te_tree *tree);

/**
 * Get the nearest leaf following @p tree in linear order.
 *
 * @param tree   tree
 *
 * @return the leaf following @p tree or @c NULL
 */
extern const te_tree *te_tree_left_leaf(const te_tree *tree);

/**
 * Get the nearest leaf preceding @p tree in linear order.
 *
 * @param tree   tree
 *
 * @return the leaf preceding @p tree or @c NULL
 */
extern const te_tree *te_tree_right_leaf(const te_tree *tree);

/**
 * Get the @p nth child of @p tree.
 *
 * @param tree   tree
 * @param nth    0-based index
 *
 * @return the @p nth child of @p tree or @c NULL
 */
extern const te_tree *te_tree_nth_child(const te_tree *tree, unsigned int nth);

/**
 * Get the first child with a given attribute.
 *
 * The check is performed by te_tree_has_attr().
 *
 * @param tree   tree
 * @param attr   attribute name (may be @c NULL)
 * @param value  attribute name (may be @c NULL)
 *
 * @return the first matching child or @c NULL
 */
extern const te_tree *te_tree_child_by_attr(const te_tree *tree,
                                            const char *attr,
                                            const char *value);

/**
 * Get the first child with given attributes.
 *
 * The check is performed by te_tree_has_attrs().
 *
 * @param tree   tree
 * @param attr   attribute name (may be @c NULL)
 * @param value  attribute name (may be @c NULL)
 *
 * @return the first matching child or @c NULL
 */
extern const te_tree *te_tree_child_by_attrs(const te_tree *tree,
                                             const te_kvpair_h *attrs);

/**
 * Function type for tree traversal.
 *
 * If the function returns non-zero, the traversal stops immediately,
 * but a value of TE_EOK is returned as zero from te_tree_traverse().
 *
 * In a pre-callback TE_ESKIP may be returned to prevent descending into
 * the tree children.
 *
 * @param tree   current (sub)tree
 * @param data   user data
 *
 * @return status code
 * @retval TE_EOK   Stop traversing and report success to the caller.
 * @retval TE_ESKIP Do not descend into the children.
 */
typedef te_errno te_tree_traverse_fn(const te_tree *tree, void *data);

/**
 * Traverse the @p tree.
 *
 * Callbacks are only called for subtrees that are at least
 * @p minlevel below @p tree, and the traversal does not go
 * deeper than @p maxlevel below @p tree.
 * So for example to process only direct children of @p tree,
 * one may use:
 *
 * @code
 * te_tree_traverse(tree, 1, 1, child_cb, NULL, userdata);
 * @endcode
 *
 * @param tree       tree to traverse
 * @param minlevel   minimum level to call callbacks on
 * @param maxlevel   maximum level to descend
 * @param pre_cb     preorder callback (may be @c NULL)
 * @param post_cb    postorder callback (may be @c NULL)
 *
 * @return status code
 */
extern te_errno te_tree_traverse(const te_tree *tree, unsigned int minlevel,
                                 unsigned int maxlevel,
                                 te_tree_traverse_fn *pre_cb,
                                 te_tree_traverse_fn *post_cb,
                                 void *data);

/**
 * Attribute mapping function type.
 *
 * @p dst is empty upon entry to this function, so it must do copying
 * itself, if needed.
 *
 * @param[in]     src   source mapping
 * @param[out]    dst   destination mapping
 * @param[in,out] data  user data
 *
 * @return status code
 */
typedef te_errno te_tree_map_fn(const te_kvpair_h *src, te_kvpair_h *dst,
                                void *data);

/**
 * Construct a new tree from an old one converting attributes.
 *
 * If the conversion function @p fn ever returns non-zero, the new tree
 * is destroyed and @c NULL is returned.
 *
 * No attributes of the tree are automatically copied, the mapping function
 * should use te_kvpairs_copy() or te_kvpairs_copy_key() to copy the needed
 * attributes.
 *
 * @param tree   source tree
 * @param fn     attribute translation function
 * @param data   user data
 *
 * @return a fresh translated tree or @c NULL
 */
extern te_tree *te_tree_map(const te_tree *tree, te_tree_map_fn *fn,
                            void *data);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TREE_H__ */
/**@} <!-- END te_tools_te_tree --> */
