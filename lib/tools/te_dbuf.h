/** @file
 * @brief API to deal with dynamic buffers
 *
 * @defgroup te_tools_te_dbuf Dynamic buffers
 * @ingroup te_tools
 * @{
 *
 * Definition of functions to work with dynamic buffers.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 *
 *
 * Example of using:
 * @code
 * // Init the dynamic buffer with 100% overlength.
 * te_dbuf dbuf = TE_DBUF_INIT(100);
 * ...
 * // Put the "foo\0" into the dbuf
 * if (te_dbuf_append(&dbuf, "foo", 4) == -1) {
 *     ERROR("Memory allocation error");
 * }
 * ...
 * // Reserve 4 bytes into the dbuf
 * size_t pos = dbuf.len;   // Save current position in the buffer.
 * if (te_dbuf_append(&dbuf, NULL, 4) == -1) {
 *     ERROR("Memory allocation error");
 * }
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
 * Dynamically allocated buffer.
 */
typedef struct te_dbuf {
    uint8_t *ptr;       /**< Pointer to the buffer. */
    size_t   size;      /**< Size of the buffer. */
    size_t   len;       /**< Length of actual data. */
    uint8_t  grow_factor;   /**< Buffer extra size in percentages. I.e.
                             size = required_size * (1+grow_factor/100.). */
} te_dbuf;

/**
 * On-stack te_dbuf initializer.
 *
 * @param _grow_factor  Buffer extra size in percentages.
 */
#define TE_DBUF_INIT(_grow_factor) { \
    .ptr = NULL,                     \
    .size = 0,                       \
    .len = 0,                        \
    .grow_factor = _grow_factor      \
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
 *                      May be @c NULL if needs only reserve the area
 *                      without placing any data.
 * @param data_len      Length of the data.
 *
 * @return Status code.
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
 * @return Status code.
 */
extern te_errno te_dbuf_expand(te_dbuf *dbuf, size_t n);

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
