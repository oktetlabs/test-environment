/** @file
 * @brief Dynamic array
 *
 * @defgroup te_tools_te_vec Dymanic array
 * @ingroup te_tools
 * @{
 *
 * Implementation of dymanic array
 *
 * Example of using:
 * @code
 * // Initialize the dynamic vector to store a value of type int
 * te_vec vec = TE_VEC_INIT(int)
 * int number = 42;
 * ...
 * // Put number into dynamic array
 * CHECK_RC(TE_VEC_APPEND(&vec, number));
 * ...
 * // Copy from c array
 * int numbers[] = {4, 2};
 * CHECK_RC(te_vec_append_array(&vec, numbers, TE_ARRAY_LEN(numbers)));
 * ...
 * // Change the first element
 * TE_VEC_GET(int, &vec, 0) = 100;
 * ...
 * // Finish work with vector, free the memory.
 * te_vec_free(&vec);
 * @endcode
 *
 * Copyright (C) 2019 OKTET Labs. All rights reserved.
 *
 * @author Nikita Somenkov <Nikita.Somenkov@oktetlabs.ru>
 */

#ifndef __TE_VEC_H__
#define __TE_VEC_H__

#include "te_dbuf.h"

#if HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Dymanic array */
typedef struct te_vec {
    te_dbuf data;           /**< Data of array */
    size_t element_size;    /**< Size of one element in bytes */
} te_vec;

/** Initialization from type and custom grow factor and type */
#define TE_VEC_INIT_GROW_FACTOR(_type, _grow_factor) (te_vec)   \
{                                                               \
    .data = TE_DBUF_INIT(_grow_factor),                         \
    .element_size = sizeof(_type),                              \
}

/** Initialization from type only */
#define TE_VEC_INIT(_type) TE_VEC_INIT_GROW_FACTOR(_type, 50)

/**
 * Access for element in array
 *
 * @param _type     Type of element
 * @param _te_vec   Dynamic vector
 * @param _index    Index of element
 *
 * @return Element of array
 */
#define TE_VEC_GET(_type, _te_vec, _index) \
    (*(_type *)te_vec_get_safe(_te_vec, _index, sizeof(_type)))

/**
 * For each element in vector
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
 * @param _te_vec   Dynamic vector
 * @param _elem     Pointer of type contain in vector
 */
#define TE_VEC_FOREACH(_te_vec, _elem)                                        \
    for ((_elem) = te_vec_size(_te_vec) != 0 ?                                \
            te_vec_get_safe(_te_vec, 0, sizeof(*(_elem))) : NULL;             \
         te_vec_size(_te_vec) != 0 &&                                         \
         ((void *)(_elem)) <= te_vec_get(_te_vec, te_vec_size(_te_vec) - 1);  \
         (_elem)++)

/**
 * Add element to the vector's tail
 *
 * @param _te_vec   Dynamic vector
 * @param _val      New element
 *
 * @return Status code
 */
#define TE_VEC_APPEND(_te_vec, _val) \
    (te_vec_append_array_safe(_te_vec, &(_val), 1, sizeof(_val)))

/**
 * Add element to the vector's tail
 * @param _te_vec   Dynamic vector
 * @param _type     Element type
 * @param _val      New element
 *
 * @return Status code
 */
#define TE_VEC_APPEND_RVALUE(_te_vec, _type, _val) \
    (te_vec_append_array_safe(_te_vec, (_type[]){_val}, 1, sizeof(_type)))

/**
 * Append elements from C-like array to the dynamic array (safe version)
 *
 * @param _te_vec        Dymanic vector
 * @param _elements      Elements of array
 * @param _count         Count of @p elements
 *
 * @return Status code
 */
#define TE_VEC_APPEND_ARRAY(_te_vec, _elements, _count) \
    (te_vec_append_array_safe(_te_vec, _elements, _size, sizeof(*(_elements))))

/**
 * Count of elements contains in dynamic array
 *
 * @param vec       Dynamic vector
 *
 * @return Count of elements
 */
static inline size_t
te_vec_size(const te_vec *vec)
{
    assert(vec != NULL);
    assert(vec->element_size != 0);
    return vec->data.len / vec->element_size;
}

/**
 * Access to a pointer of element in array
 *
 * @param vec       Dynamic vector
 * @param index     Index of element
 *
 * @return Pointer to element
 */
static inline void *
te_vec_get(te_vec *vec, size_t index)
{
    assert(vec != NULL);
    assert(index < te_vec_size(vec));
    return vec->data.ptr + index * vec->element_size;
}

/**
 * Safe version of @ref te_vec_get
 *
 * @param vec              Dynamic vector
 * @param index            Index of element
 * @param element_size     Expected size of type in array
 *
 * @return Pointer to element
 */
static inline void *
te_vec_get_safe(te_vec *vec, size_t index, size_t element_size)
{
    assert(vec != NULL);
    assert(element_size == vec->element_size);
    return te_vec_get(vec, index);
}

/**
 * Append one element to the dynamic array
 *
 * @param vec        Dymanic vector
 * @param element    Element for appending
 *
 * @return Status code
 */
extern te_errno te_vec_append(te_vec *vec, const void *element);

/**
 * Append elements from @p other dynamic array
 *
 * @param vec        Dymanic vector
 * @param other      Other dymanic vector
 *
 * @return Status code
 */
extern te_errno te_vec_append_vec(te_vec *vec, const te_vec *other);

/**
 * Append elements from C-like array to the dynamic array
 *
 * @param vec        Dymanic vector
 * @param elements   Elements of array
 * @param count      Count of @p elements
 *
 * @return Status code
 */
extern te_errno te_vec_append_array(te_vec *vec, const void *elements,
                                    size_t count);

/**
 * Append a formatted C-string to the dynamic array
 *
 * @param vec        Dynamic vector of C-strings
 * @param fmt        Format string
 * @param ...        Format string arguments
 *
 * @return Status code
 */
extern te_errno te_vec_append_str_fmt(te_vec *vec, const char *fmt, ...)
                                      __attribute__((format(printf, 2, 3)));

/**
 * Remove elements from a vector
 *
 * @note If the elements of the vector are themselves pointers,
 *       they won't be automatically freed.
 *
 * @param vec           Dynamic vector
 * @param start_index   Starting index of elements to remove
 * @param count         Number of elements to remove
 */
extern void
te_vec_remove(te_vec *vec, size_t start_index, size_t count);

/**
 * Safe version of @ref te_vec_append_array
 *
 * @param vec               Dymanic vector
 * @param elements          Elements of array
 * @param count             Count of @p elements
 * @param element_size      Size of one element in @p elements
 *
 * @return Status code
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
 * Reset dynamic array (makes it empty), memory is not freed
 *
 * @param vec          Dynamic vector
 */
extern void te_vec_reset(te_vec *vec);

/**
 * Cleanup dynamic array and storage memory
 *
 * @param vec       Dynamic vector
 */
extern void te_vec_free(te_vec *vec);

/**
 * Free the dynamic array along with its elements which must be pointers
 * deallocatable by free()
 *
 * @param vec        Dynamic vector of pointers
 *
 * @return Status code
 */
extern void te_vec_deep_free(te_vec *vec);

/**
 * Append to a dynamic array of strings
 *
 * @param vec           Dynamic vector to append the array of strings to
 * @param elements      @c NULL terminated array of strings
 *
 * @return Status code
 */
extern te_errno te_vec_append_strarray(te_vec *vec, const char **elements);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_VEC_H__ */
/**@} <!-- END te_tools --> */
