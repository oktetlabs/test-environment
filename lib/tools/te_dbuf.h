/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief API to deal with dynamic buffers
 *
 * @defgroup te_tools_te_dbuf Dynamic buffers
 * @ingroup te_tools
 * @{
 *
 * Definition of functions to work with dynamic buffers.
 *
 * Example of using:
 * @code
 * // Init the dynamic buffer with the default growth parameters.
 * te_dbuf dbuf = TE_DBUF_INIT(TE_DBUF_DEFAULT_GROW_FACTOR);
 * ...
 * // Put the "foo\0" into the dbuf
 * te_dbuf_append(&dbuf, "foo", 4);
 * ...
 * // Reserve 4 bytes into the dbuf
 * size_t pos = dbuf.len;   // Save current position in the buffer.
 * te_dbuf_append(&dbuf, NULL, 4);
 *
 * (uint32_t *)&dbuf.ptr[pos] = 5;  // Put the number to reserved place.
 * ...
 * // Reset the buffer to start filling it from the beginning again.
 * te_dbuf_reset(&dbuf);
 * ...
 * // Finish work with dynamic buffer, free the memory.
 * te_dbuf_free(&dbuf);
 * @endcode
 */

#ifndef __TE_DBUF_H__
#define __TE_DBUF_H__

#include "te_stdint.h"
#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A grow factor is a percentage of extra memory
 * to reserve for a dbuf when new data are appended to it.
 *
 * For example, with a grow factor of @c 100, the amount
 * of reserved memory is doubled every time the buffer is
 * reallocated.
 *
 * The purpose of grow factos is to optimize memory usage for
 * different buffer grow scenario, however, most users should
 * not bother with it and use the default factor.
 */
#define TE_DBUF_DEFAULT_GROW_FACTOR 50

/**
 * Dynamically allocated buffer.
 */
typedef struct te_dbuf {
    uint8_t *ptr;       /**< Pointer to the buffer. */
    size_t   size;      /**< Size of the buffer. */
    size_t   len;       /**< Length of actual data. */
    uint8_t  grow_factor;   /**< See TE_DBUF_DEFAULT_GROW_FACTOR. */
} te_dbuf;

/**
 * On-stack te_dbuf initializer.
 *
 * @param grow_factor_  see TE_DBUF_DEFAULT_GROW_FACTOR
 */
#define TE_DBUF_INIT(grow_factor_) { \
    .ptr = NULL,                     \
    .size = 0,                       \
    .len = 0,                        \
    .grow_factor = (grow_factor_)    \
}

/**
 * Reset dynamic buffer (makes its empty).
 *
 * @param dbuf          Dynamic buffer.
 */
static inline void
te_dbuf_reset(te_dbuf *dbuf)
{
    dbuf->len = 0;
}

/**
 * Append additional data to the dynamic buffer. Allocate/reallocate the
 * memory for the buffer if needed. @p dbuf should be released with
 * @p te_dbuf_free when it is no longer needed.
 *
 * @param dbuf          Dynamic buffer.
 * @param data          Data to append to the buffer pointed by @p dbuf.
 *                      If @c NULL, then a block of @p data_len zeroes
 *                      is appended.
 * @param data_len      Length of the data.
 *
 * @return Status code (always 0).
 *
 * @sa te_dbuf_free
 */
extern te_errno te_dbuf_append(te_dbuf    *dbuf,
                               const void *data,
                               size_t      data_len);

/**
 * Increase the size of dynamic buffer on @p n bytes. It reallocates the
 * te_dbuf container with new size. @p dbuf should be released with
 * @p te_dbuf_free when it is no longer needed.
 *
 * @param dbuf      Dynamic buffer.
 * @param n         Number of bytes to add to the buffer size to expand it.
 *
 * @return Status code (always 0).
 */
extern te_errno te_dbuf_expand(te_dbuf *dbuf, size_t n);

/**
 * Cut a region of a dynamic buffer starting from @p start_index
 * of length @p count
 *
 * @note Function does not reallocate memory
 *
 * @param dbuf          Dynamic buffer
 * @param start_index   Starting index of a region
 * @param count         Length of a region
 */
extern void te_dbuf_cut(te_dbuf *dbuf, size_t start_index, size_t count);

/**
 * Free dynamic buffer that was allocated with @p te_dbuf_append or
 * @p te_dbuf_expand.
 *
 * @param dbuf          Dynamic buffer.
 *
 * @sa te_dbuf_append, te_dbuf_expand
 */
extern void te_dbuf_free(te_dbuf *dbuf);


/**
 * Prints the @p dbuf info and its data using VERB function.
 * This function should be used for debugging purpose.
 *
 * @param dbuf      Dynamic buffer.
 */
extern void te_dbuf_print(const te_dbuf *dbuf);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_DBUF_H__ */
/**@} <!-- END te_tools_te_dbuf --> */
