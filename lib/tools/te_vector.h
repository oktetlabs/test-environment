/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2019-2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief Dynamic vectors.
 *
 * @defgroup te_tools_te_vec Dynamic vectors.
 * @ingroup te_tools
 * @{
 *
 * Implementation of dynamic vectors.
 *
 * Example of usage:
 * @code
 * // Initialize the dynamic vector to store a value of type int.
 * te_vec vec = TE_VEC_INIT(int)
 * int number = 42;
 * ...
 * // Put a number into the dynamic vector.
 * TE_VEC_APPEND(&vec, number);
 * ...
 * // Copy from a C array
 * int numbers[] = {4, 2};
 * te_vec_append_array(&vec, numbers, TE_ARRAY_LEN(numbers));
 * ...
 * // Change the first element.
 * TE_VEC_GET(int, &vec, 0) = 100;
 * ...
 * // Free the memory.
 * te_vec_free(&vec);
 * @endcode
 */

#ifndef __TE_VEC_H__
#define __TE_VEC_H__

#include "te_defs.h"
#include "te_dbuf.h"

#if HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function type for vector element destructors.
 *
 * The destructor accepts a pointer to a vector element
 * which it is not supposed to modify, *not* a pointer
 * to a deallocatable memory, so e.g. a destructor
 * that frees a `char *` typed element would look like:
 * @code
 * static void item_destroy_str(const void *item)
 * {
 *     char *buf = *(void * const *)item;
 *     free(buf);
 * }
 * @endcode
 *
 * The destructor function shall be prepared to deal
 * correctly with all-zero initialized elements.
 *
 * @param item   A pointer to a vector element data.
 */
typedef void te_vec_item_destroy_fn(const void *item);


/**
 * A destructor to release vector elements that contain
 * a single heap memoy pointer.
 */
extern te_vec_item_destroy_fn te_vec_item_free_ptr;

/** Dynamic vector. */
typedef struct te_vec {
    te_dbuf data;           /**< Data of a vector. */
    size_t element_size;    /**< Size of one element. */
    te_vec_item_destroy_fn *destroy; /**< Element destructor. */
} te_vec;

/**
 * Complete initializer for a te_vec.
 *
 * @param type_         Element type.
 * @param grow_factor_  Grow factor (see #TE_DBUF_DEFAULT_GROW_FACTOR).
 * @param destroy_      Element destructor (or @c NULL).
 */
#define TE_VEC_INIT_COMPLETE(type_, grow_factor_, destroy_) \
    (te_vec){                                               \
        .data = TE_DBUF_INIT(grow_factor_),                 \
        .element_size = sizeof(type_),                      \
        .destroy = (destroy_),                              \
    }

/**
 * Vector initializer with a custom grow factor.
 *
 * The element destructor may be later set with
 * te_vec_set_destroy_fn_safe().
 *
 * @note Most users shall stick to #TE_DBUF_DEFAULT_GROW_FACTOR.
 *
 * @param type_          Element type.
 * @param grow_factor_   Grow factor (see #TE_DBUF_DEFAULT_GROW_FACTOR).
 */
#define TE_VEC_INIT_GROW_FACTOR(type_, grow_factor_) \
    TE_VEC_INIT_COMPLETE(type_, grow_factor_, NULL)

/**
 * Vector initializer with the default grow factor.
 *
 * The element destructor may be later set with
 * te_vec_set_destroy_fn_safe().
 *
 * @param type_          Element type.
 */
#define TE_VEC_INIT(type_) \
    TE_VEC_INIT_GROW_FACTOR(type_, TE_DBUF_DEFAULT_GROW_FACTOR)

/**
 * Vector initializer with a possibly non-null destructor.
 *
 * @param type_     Element type.
 * @param destroy_  Element destructor or @c NULL.
 */
#define TE_VEC_INIT_DESTROY(type_, destroy_) \
    TE_VEC_INIT_COMPLETE(type_, TE_DBUF_DEFAULT_GROW_FACTOR, destroy_)


/**
 * Vector initializer for heap-allocated pointers.
 *
 * The destructor is set to te_vec_item_free_ptr().
 * Additionally a check is made that @p type_ is at least
 * the same size as a pointer type.
 *
 * @param type_     Element type (which must be a pointer type).
 */
#define TE_VEC_INIT_AUTOPTR(type_) \
    TE_VEC_INIT_DESTROY(type_,                                          \
                        TE_COMPILE_TIME_ASSERT_EXPR(sizeof(type_) ==    \
                                                    sizeof(void *)) ?   \
                        te_vec_item_free_ptr : NULL)
/*
 * Initialize a vector with the same parameters as @p other_.
 *
 * Data are *not* copied.
 *
 * @param other_    A pointer to another vector.
 */
#define TE_VEC_INIT_LIKE(other_) \
    (te_vec){                                             \
        .data = TE_DBUF_INIT((other_)->data.grow_factor), \
        .element_size = (other_)->element_size,           \
        .destroy = (other_)->destroy,                     \
    }

/*
 * Set a vector element destructor with sanity checks.
 *
 * The following rules are used to minimize the risk of
 * calling a destructor on improper data:
 * - it is always possible to set a destructor to @c NULL;
 * - if a vector already has a non-null destructor,
 *   the new destructor must be the same;
 * - if a vector has a null destructor and has some elements,
 *   the destructor remains null to support wider range of
 *   legacy cases;
 * - otherwise the null destructor is replaced with the new one.
 *
 * @param vec       Vector.
 * @param destroy   New destructor function.
 *
 * @exception TE_FATAL_ERROR if the new destructor and the old
 *            one are not the same.
 */
extern void te_vec_set_destroy_fn_safe(te_vec *vec,
                                       te_vec_item_destroy_fn *destroy);

/**
 * Vector element accessor.
 *
 * @param type_     Type of element.
 * @param te_vec_   Dynamic vector.
 * @param index_    Index of element.
 *
 * @return Element of a vector.
 *
 * @note For vectors with a non-null destructor
 *       te_vec_replace() should be used instead of
 *       mutable TE_VEC_GET(), as it guarantees proper
 *       disposal of old element contents.
 */
#define TE_VEC_GET(type_, te_vec_, index_) \
    (*(type_ *)te_vec_get_safe(te_vec_, index_, sizeof(type_)))

/**
 * Iterate over all elements in a vector.
 *
 * Example:
 * @code
 *
 * struct netinterface {
 *     int index;
 *     const char *name;
 * };
 *
 * te_vec vec = TE_VEC_INIT(struct netinterface);
 * ... // Fill vector with values
 *
 * struct netinterface *netif;
 * TE_VEC_FOREACH(&vec, netif)
 * {
 *      printf("interface '%s' have index %d", netif->name, netif->index);
 * }
 * @endcode
 *
 * @param te_vec_   Dynamic vector.
 * @param elem_     Pointer of type contain in vector.
 */
#define TE_VEC_FOREACH(te_vec_, elem_) \
    for ((elem_) = te_vec_size(te_vec_) != 0 ?                                \
            te_vec_get_safe(te_vec_, 0, sizeof(*(elem_))) : NULL;             \
         te_vec_size(te_vec_) != 0 &&                                         \
         ((void *)(elem_)) <= te_vec_get(te_vec_, te_vec_size(te_vec_) - 1);  \
         (elem_)++)

/**
 * Add element to the vector's tail.
 *
 * @param te_vec_   Dynamic vector.
 * @param val_      New element.
 *
 * @return Status code (always 0).
 */
#define TE_VEC_APPEND(te_vec_, val_) \
    (te_vec_append_array_safe(te_vec_, &(val_), 1, sizeof(val_)))

/**
 * Add element to the vector's tail.
 *
 * @param te_vec_   Dynamic vector.
 * @param type_     Element type.
 * @param val_      New element.
 *
 * @return Status code (always 0).
 */
#define TE_VEC_APPEND_RVALUE(te_vec_, type_, val_) \
    (te_vec_append_array_safe(te_vec_, (type_[]){val_}, 1, sizeof(type_)))

/**
 * Append elements from a C array to the dynamic vector (safe version).
 *
 * @param te_vec_        Dynamic vector.
 * @param elements_      C array.
 * @param count_         Count of @p elements
 *
 * @return Status code (always 0).
 */
#define TE_VEC_APPEND_ARRAY(te_vec_, elements_, count_) \
    (te_vec_append_array_safe(te_vec_, elements_, size_, sizeof(*(elements_))))

/**
 * Count elements in a dynamic vector.
 *
 * @param vec       Dynamic vector.
 *
 * @return Number of elements.
 */
static inline size_t
te_vec_size(const te_vec *vec)
{
    assert(vec != NULL);
    assert(vec->element_size != 0);
    return vec->data.len / vec->element_size;
}

/**
 * Access an element of a dynamic vector.
 *
 * The macro returns either mutable or immutable
 * pointer depending on the constness of @p vec_.
 *
 * @param vec_       Dynamic vector.
 * @param index_     Index of element.
 *
 * @return Pointer to the content of an element.
 */
#define te_vec_get(vec_, index_) \
    (TE_TYPE_ALTERNATIVE(vec_, te_vec *, te_vec_get_mutable,            \
                         const te_vec *, te_vec_get_immutable)(vec_, index_))

static inline const void *
te_vec_get_immutable(const te_vec *vec, size_t index)
{
    assert(vec != NULL);
    assert(index < te_vec_size(vec));
    return vec->data.ptr + index * vec->element_size;
}

static inline void *
te_vec_get_mutable(te_vec *vec, size_t index)
{
    assert(vec != NULL);
    assert(index < te_vec_size(vec));
    return vec->data.ptr + index * vec->element_size;
}

/**
 * Safe version of te_vec_get().
 *
 * @param vec_              Dynamic vector.
 * @param index_            Index of element.
 * @param element_size_     Expected size of type in array.
 *
 * @return Pointer to element.
 */
#define te_vec_get_safe(vec_, index_, element_size_) \
    (TE_TYPE_ALTERNATIVE(vec_, te_vec *, te_vec_get_safe_mutable,      \
                         const te_vec *, te_vec_get_safe_immutable)    \
     (vec_, index_, element_size_))

static inline const void *
te_vec_get_safe_immutable(const te_vec *vec, size_t index, size_t element_size)
{
    assert(vec != NULL);
    assert(element_size == vec->element_size);
    return te_vec_get(vec, index);
}

static inline void *
te_vec_get_safe_mutable(te_vec *vec, size_t index, size_t element_size)
{
    assert(vec != NULL);
    assert(element_size == vec->element_size);
    return te_vec_get(vec, index);
}

/**
 * Append one element to the dynamic array.
 *
 * If @p element is @c NULL, the new element will be zeroed.
 *
 * @param vec        Dynamic vector.
 * @param element    Element to append (may be @c NULL).
 *
 * @return Status code (always 0).
 */
extern te_errno te_vec_append(te_vec *vec, const void *element);

/**
 * Append elements from @p other dynamic array,
 *
 * @param vec        Dynamic vector.
 * @param other      Other dynamic vector.
 *
 * @return Status code (always 0).
 *
 * @pre Both vectors must have a null element destructor.
 */
extern te_errno te_vec_append_vec(te_vec *vec, const te_vec *other);

/**
 * Append elements from a plain C array to the dynamic array.
 *
 * If @p elements is @c NULL, the news element will be zeroed.
 *
 * @param vec        Dynamic vector.
 * @param elements   Elements of array (may be @c NULL).
 * @param count      Number of @p elements.
 *
 * @return Status code (always 0).
 */
extern te_errno te_vec_append_array(te_vec *vec, const void *elements,
                                    size_t count);

/**
 * Append a formatted C string to the dynamic array.
 *
 * The string in the vector will be heap-allocated.
 *
 * @param vec        Dynamic vector of C strings.
 * @param fmt        Format string.
 * @param ...        Format string arguments.
 *
 * @return Status code (always 0).
 */
extern te_errno te_vec_append_str_fmt(te_vec *vec, const char *fmt, ...)
                                      TE_LIKE_PRINTF(2, 3);

/**
 * Replace the content of @p index'th element of @p vec with @p new_val.
 *
 * If @p new_val is @c NULL, the content of the element is zeroed.
 * A destructor function (if it is set) is called for the old content.
 *
 * If @p index is larger than the @p vec size, it is grown as needed.
 *
 * @param vec         Dynamic vector.
 * @param index       Index to replace.
 * @param new_val     New content (may be @c NULL).
 *
 * @return A pointer to the new content of the element.
 */
extern void *te_vec_replace(te_vec *vec, size_t index, const void *new_val);

/**
 * Move the content of a vector element to @p dest.
 *
 * This function is mostly useful for vectors with non-null destructors.
 * Basically, it implements move semantics for vector elements.
 *
 * If @p dest is not @c NULL, the content of @p index'th element is copied
 * to @p dest; destructors are not called.
 *
 * If @p dest is @c NULL, the destructor is called on @p index'th element.
 *
 * In both cases the element is zeroed afterwards. This ensures that calling
 * the destructor for the second time would not cause bugs like double-free.
 *
 * @param vec        Dynamic vector.
 * @param index      Index to move.
 * @param dest       Destination (may be @c NULL).
 */
extern void te_vec_transfer(te_vec *vec, size_t index, void *dest);

/**
 * Move at most @p count elements from @p vec to @p dest_vec.
 *
 * If @p dest_vec is not @c NULL, elements starting from @p start_index
 * are appended to it and the elements are zeroed like with
 * te_vec_transfer().
 *
 * If @p dest_vec is @c NULL, the destructor is called on the elements.
 *
 * If @p start_index + @p count is greater than the vector size, @p count
 * is decreased as needed.
 *
 * @p dest_vec must either have a null element destructor or the same
 * destructor as @p vec.
 *
 * @param vec           Source vector.
 * @param start_index   Starting index.
 * @param count         Number of elements.
 * @param dest_vec      Destination vector (may be @c NULL).
 *
 * @return The number of actually transferred elements.
 */
extern size_t te_vec_transfer_append(te_vec *vec, size_t start_index,
                                     size_t count, te_vec *dest_vec);

/**
 * Remove elements from a vector.
 *
 * If @p vec has a non-null element destructor,
 * it will be called for each element.
 *
 * If @p start_index + @p count is greater than the vector size,
 * @p count is decreased as needed.
 *
 * @param vec           Dynamic vector.
 * @param start_index   Starting index of elements to remove.
 * @param count         Number of elements to remove.
 */
extern void te_vec_remove(te_vec *vec, size_t start_index, size_t count);

/**
 * Remove an element from a vector.
 *
 * @param vec           Dynamic vector.
 * @param index         Index of an element to remove.
 */
static inline void
te_vec_remove_index(te_vec *vec, size_t index)
{
    return te_vec_remove(vec, index, 1);
}

/**
 * Safe version of te_vec_append_array().
 *
 * @param vec               Dynamic vector.
 * @param elements          Elements of an array.
 * @param count             Number of @p elements.
 * @param element_size      Size of one element in @p elements.
 *
 * @return Status code (always 0).
 */
static inline te_errno
te_vec_append_array_safe(te_vec *vec, const void *elements,
                         size_t count, size_t element_size)
{
    assert(vec != NULL);
    assert(element_size == vec->element_size);
    return te_vec_append_array(vec, elements, count);
}

/**
 * Reset a dynamic array.
 *
 * The number of elements in the array becomes zero.
 * A destructor function is called for each element if it is defined.
 * The memory of the vector itself is not released.
 *
 * @param vec          Dynamic vector.
 *
 * @sa te_vec_free()
 */
extern void te_vec_reset(te_vec *vec);

/**
 * Cleanup a dynamic array and free its storage memory.
 *
 * A destructor function is called for each element if it is defined.
 *
 * @param vec       Dynamic vector.
 */
extern void te_vec_free(te_vec *vec);

/**
 * Free the dynamic array along with its elements.
 *
 * The function does exactly the same as te_vec_free() unless
 * @p vec has a null destructor, in which case it treats elements
 * as pointers to heap memory and free() them.
 *
 * @param vec        Dynamic vector of pointers.
 *
 * @deprecated Use te_vec_free() with a proper destructor.
 */
extern void te_vec_deep_free(te_vec *vec);

/**
 * Append to a dynamic array of strings.
 *
 * The elements in the array will be strdup()'ed.
 *
 * @param vec           Dynamic vector to append the array of strings to.
 * @param elements      @c NULL terminated array of strings
 *
 * @return Status code (always 0).
 */
extern te_errno te_vec_append_strarray(te_vec *vec, const char **elements);

/**
 * Return an index of an element of @p vec pointed to by @p ptr.
 *
 * @param vec           Dynamic vector.
 * @param ptr           Pointer to the content of some of its elements.
 *
 * @return Zero-based index. The result is undefined if @p ptr is
 *         not pointing to the actual vector data
 */
static inline size_t
te_vec_get_index(const te_vec *vec, const void *ptr)
{
    size_t offset = (const uint8_t *)ptr - (const uint8_t *)vec->data.ptr;
    assert(offset < vec->data.len);

    return offset / vec->element_size;
}

/**
 * Split a string into chunks separated by @p sep.
 *
 * The copies of the chunks are pushed into the @p strvec.
 *
 * @note The element size of @p strvec must be `sizeof(char *)`.
 *
 * @note Adjacent separators are never skipped, so e.g.
 *       @c ':::' would be split into four chunks using colon as
 *       a separator. The only special case is an empty string
 *       which may be treated as no chunks depending on @p empty_is_none.
 *
 * @param[in]     str            String to split.
 * @param[in,out] strvec         Target vector for string chunks.
 *                               The original content is **not** destroyed,
 *                               new items are added to the end.
 * @param[in]     sep            Separator character.
 * @param[in]     empty_is_none  If @c true, empty string is treated
 *                               as having no chunks (so @p strvec is
 *                               not changed). Otherwise, an empty string
 *                               is treated as having a single empty chunk.
 *
 * @return Status code (always 0).
 */
extern te_errno te_vec_split_string(const char *str, te_vec *strvec, char sep,
                                    bool empty_is_none);

/**
 * Sort the elements of @p vec in place according to @p compar.
 *
 * @param vec      Vector to sort.
 * @param compar   Comparison function (as for @c qsort).
 */
extern void te_vec_sort(te_vec *vec, int (*compar)(const void *elt1,
                                                   const void *elt2));

/**
 * Search a sorted vector @p vec for an item equal to @p key
 * using @p compar as a comparison function.
 *
 * The function implements binary search, however unlike C standard
 * bsearch() it can be reliably used on non-unique matches, because
 * it returns the lowest and the highest indices of matching elements.
 *
 * @note Mind the order of arguments. @p compar expects the **first**
 * argument to be a key and the second argument to be an array element,
 * for a compatibility with bsearch(). However, the order of arguments of
 * the function itself is reverse wrt bsearch(): the vector goes first and
 * the key follows it for consistency with other vector functions.
 * For cases where the key has the same structure as the array element,
 * this should not matter.
 *
 * @note The vector must be sorted in a way compatible with @p compar, i.e.
 * by using te_vec_sort() with the same @p compar, however, the comparison
 * functions need not to be truly identical: the search comparison function
 * may treat more elements as equal than the sort comparison.
 *
 * @param[in]  vec     Vector to search in.
 * @param[in]  key     Key to search.
 * @param[in]  compar  Comparison function (as for @c bsearch).
 * @param[out] minpos  The lowest index of a matching element.
 * @param[out] maxpos  The highest index of a matchin element.
 *                     Any of @p minpos and @p maxpos may be @c NULL.
 *                     If they are both @c NULL, the function just checks
 *                     for an existence of a matching element.
 *
 * @return @c true iff an element matching @p key is found.
 */
 extern bool te_vec_search(const te_vec *vec, const void *key,
                              int (*compar)(const void *key, const void *elt),
                              unsigned int *minpos, unsigned int *maxpos);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_VEC_H__ */
/**@} <!-- END te_tools --> */
