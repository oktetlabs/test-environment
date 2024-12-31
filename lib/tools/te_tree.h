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
 * - its textual value;
 * - its type.
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
/** Tree value type. */
#define TE_TREE_ATTR_TYPE "type"

/**@}*/

/**
 * @name Tree value types.
 *
 * The list of possible types is open. All users are advised
 * to treat unknown types as TE_TREE_ATTR_TYPE_STRING.
 *
 * Listed here are basic types that can be checked by
 * te_tree_validate_types()
 *
 * @{
 */

/** Auto-detect type (the default if no type attribute). */
#define TE_TREE_ATTR_TYPE_AUTO "auto"
/** Null type. */
#define TE_TREE_ATTR_TYPE_NULL "null"
/** String type. */
#define TE_TREE_ATTR_TYPE_STRING "string"
/** Integer type. */
#define TE_TREE_ATTR_TYPE_INT "int"
/** Floating-point type. */
#define TE_TREE_ATTR_TYPE_FLOAT "float"
/** Boolean type. */
#define TE_TREE_ATTR_TYPE_BOOL "bool"
/** Linear array. */
#define TE_TREE_ATTR_TYPE_ARRAY "array"
/** Dictionary (an associative array). */
#define TE_TREE_ATTR_TYPE_DICT "dict"
/** A node with metadata that should not be serialized in-band. */
#define TE_TREE_ATTR_TYPE_ANNOTATION "annotation"

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
 * @param va        variadic list
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
 * @param ...       variadic arguments
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
 * Create a tree with a name and a typed value.
 *
 * @p name becomes TE_TREE_ATTR_NAME, @p type become TE_TREE_ATTR_TYPE,
 * and the value is determined from the type and variadic arguments:
 * - TE_TREE_ATTR_TYPE_AUTO is not supported;
 * - TE_TREE_ATTR_TYPE_NULL accepts no additional arguments;
 * - TE_TREE_ATTR_TYPE_STRING should have a single string argument;
 * - TE_TREE_ATTR_TYPE_INT should have a single int argument;
 * - TE_TREE_ATTR_TYPE_FLOAT should have a single double argument;
 * - TE_TREE_ATTR_TYPE_BOOL should have a single int argument
 *   treated as boolean;
 * - TE_TREE_ATTR_TYPE_ARRAY should have a @c NULL-terminated sequence of
 *   subtrees, none of which must have a TE_TREE_ATTR_NAME;
 * - TE_TREE_ATTR_TYPE_DICT should have a @c NULL-terminated sequence
 *   of pairs - name/subtree. Names will be added to subtrees as
 *   TE_TREE_ATTR_NAME attributes;
 * - TE_TREE_ATTR_TYPE_ANNOTATION is not supported.
 *
 * If @p name is @c NULL, the resulting tree is unnamed.
 *
 * @attention In most other places TE_TREE_ATTR_TYPE_INT values are
 *            represented by @c intmax_t. However, here a plain int
 *            argument is expected, since it should be a more common
 *            usecase.
 *
 * @par Examples
 * @code
 * te_tree_make_typed("node0", TE_TREE_ATTR_TYPE_NULL);
 * te_tree_make_typed("node1", TE_TREE_ATTR_TYPE_STRING,
 *                    "string value");
 * te_tree_make_typed("node2", TE_TREE_ATTR_TYPE_INT, 42);
 * te_tree_make_typed("node3", TE_TREE_ATTR_TYPE_FLOAT, 42.0);
 * te_tree_make_typed("node4", TE_TREE_ATTR_TYPE_BOOL, true);
 * te_tree_make_typed("node6", TE_TREE_ATTR_TYPE_ARRAY,
 *                    item1, item2, item3, NULL);
 * te_tree_make_typed("node7", TE_TREE_ATTR_TYPE_DICT,
 *                    "subnode1", subnode1, "subnode2", subnode2, NULL);
 * @endcode
 *
 * @param name   name of a tree (may be @c NULL)
 * @param type   type of its value
 * @param ...    variadic arguments defining value
 *
 * @return a fresh tree or @c NULL if there are any inconsistencies
 */
extern te_tree *te_tree_make_typed(const char *name, const char *type, ...);

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
 * In particular, the accessors are orthogonal to the type of the node
 * as specified by  * TE_TREE_ATTR_TYPE attribute, for example,
 * if a node has TE_TREE_ATTR_TYPE_INT type, its value attribute may be
 * accessed by te_tree_get_float_attr() as well as by te_tree_get_int_attr().
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
 * - @c true, @c true, @c True, @c T, @c t, @c YES, @c yes,
 *   @c Yes, @c Y, @c y, @c 1 all map to @c true
 * - @c false, @c false, @c False, @c F, @c f, @c NO, @c no,
 * - @c No, @c N, @xc n, @c 0 and empty string all map to @c false.
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
                                      bool *result);

/**@}*/

/**
 * Get the type of a tree.
 *
 * If the tree has TE_TREE_ATTR_TYPE attribute and its value is
 * not TE_TREE_ATTR_TYPE_AUTO, it is returned.
 *
 * Otherwise:
 * - if it has TE_TREE_ATTR_VALUE and no children, the type is
 *   assumed to be TE_TREE_ATTR_TYPE_STRING;
 * - otherwise if if has at least one child with a TE_TREE_ATTR_NAME
 *   which is not of TE_TREE_ATTR_TYPE_ANNOTATION type,
 *   the type is TE_TREE_ATTR_TYPE_DICT;
 * - otherwise the type is TE_TREE_ATTR_TYPE_ARRAY; in particular,
 *   if a node has no TE_TREE_ATTR_VALUE and no children, it is
 *   assumed to represent an empty array.
 *
 * @note All scalar nodes without explicit type are detected as
 *       TE_TREE_ATTR_TYPE_STRING, even if the value matches some
 *       more specific type such as an integer.
 *
 * @param tree  tree to inspect
 *
 * @return type label of @p tree (never @c NULL)
 */
extern const char *te_tree_get_type(const te_tree *tree);

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
 * @return @c true if there is an attribute with a given name and value
 */
extern bool te_tree_has_attr(const te_tree *tree, const char *attr,
                                const char *value);

/**
 * Test whether a tree has all attributes specified by @p attrs.
 *
 * @param tree   tree
 * @param attrs  list of name-value pairs
 *
 * @return @c true if all attributes from @p attrs have matching
 *         values in @p tree
 */
extern bool te_tree_has_attrs(const te_tree *tree, const te_kvpair_h *attrs);

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

/**
 * Check that all subtrees have correct types and values.
 *
 * The type of each node is detected by te_tree_get_type() and
 * then the following constraints apply:
 * - if the type is TE_TREE_ATTR_TYPE_NULL, the node must have
 *   no TE_TREE_ATTR_VALUE and no children;
 * - if the type is scalar (one of TE_TREE_ATTR_TYPE_STRING,
 *   TE_TREE_ATTR_TYPE_BOOL, TE_TREE_ATTR_TYPE_INT,
 *   TE_TREE_ATTR_TYPE_FLOAT) it must have a TE_TREE_ATTR_VALUE
 *   and no children;
 * - if the type is TE_TREE_ATTR_TYPE_BOOL, the value must be valid
 *   as per te_tree_get_bool_attr();
 * - if the type is TE_TREE_ATTR_TYPE_INT, the value must be valid
 *   as per te_tree_get_int_attr();
 * - if the type is TE_TREE_ATTR_TYPE_FLOAT, the value must be valid
 *   as per te_tree_get_float_attr();
 * - if the type is TE_TREE_ATTR_TYPE_ARRAY, all children that are
 *   not annotations must have no names;
 * - if the type is TE_TREE_ATTR_TYPE_DICT, all children that are
 *   not annotations must have names;
 * - if the type is unknown and @p allow_unknown is true, the node
 *   itself is always considered valid; children are recursively checked;
 *   otherwise the node is invalid;
 * - if the type is TE_TREE_ATTR_TYPE_ANNOTATION, the node is considered
 *   valid and no children are further checked.
 *
 * @param[in]  tree            tree to check
 * @param[in]  allow_unknown   allow unknown types for tree nodes
 * @param[out] bad_node        location for a pointer to the first detected
 *                             bad node
 *
 * @return @c true if the tree is valid
 */
extern bool te_tree_validate_types(const te_tree *tree,
                                      bool allow_unknown,
                                      const te_tree **bad_node);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TREE_H__ */
/**@} <!-- END te_tools_te_tree --> */
