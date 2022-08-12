/** @file
 * @brief API to deal with buffers
 *
 * @defgroup te_tools_te_bufs Regular binary buffers
 * @ingroup te_tools
 * @{
 *
 * Allocation of buffers, fill in by random numbers, etc.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#ifndef __TE_TOOLS_BUFS_H__
#define __TE_TOOLS_BUFS_H__

#include <stdlib.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Fill buffer by random numbers.
 *
 * @param buf      Buffer pointer
 * @param len      Buffer length
 */
extern void te_fill_buf(void *buf, size_t len);

/**
 * Allocate buffer of random size from @b min to @b max and preset
 * it by random numbers.
 * Note, that this function will never return NULL and so all function
 * which call it.
 *
 * @param min     minimum size of buffer
 * @param max     maximum size of buffer
 * @param p_len   location for real size of allocated buffer
 *
 * @return Pointer to allocated buffer or exit(1)!
 */
extern void *te_make_buf(size_t min, size_t max, size_t *p_len);


/**
 * Create a buffer of specified size
 *
 * @param len    Buffer length
 *
 * @return Pointer to allocated buffer.
 */
static inline void *
te_make_buf_by_len(size_t len)
{
    size_t ret_len;

    return te_make_buf(len, len, &ret_len);
}

/**
 * Create a buffer not shorter that specified length.
 *
 * @param min       Minimum buffer length
 * @param p_len     Buffer length (OUT)
 *
 * @return Pointer to allocated buffer.
 */
static inline void *
te_make_buf_min(size_t min, size_t *p_len)
{
    return te_make_buf(min, min + 10, p_len);
}

/**
 * Allocate memory and fill it with the @p byte
 *
 * @param num   Items number
 * @param size  Item instance size
 * @param byte  Byte to fill memory
 *
 * @return Pointer to the memory block
 */
static inline void *
te_calloc_fill(size_t num, size_t size, int byte)
{
    void *buf = calloc(num, size);

    memset(buf, byte, num * size);
    return buf;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TOOLS_BUFS_H__ */
/**@} <!-- END te_tools_te_bufs --> */
