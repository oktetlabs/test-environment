/** @file
 * @brief Test API to deal with buffers
 *
 * Allocation of buffers, fill in by random numbers, etc.
 *
 * Copyright (C) 2004 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LIB_TAPI_BUFS_H__
#define __TE_LIB_TAPI_BUFS_H__

#ifdef HAVE_SYS_TYPE_H
#include <sys/type.h>
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
extern void tapi_fill_buf(void *buf, size_t len);

/**
 * Allocate buffer of random size from @b min to @b max and preset
 * it by random numbers.
 *
 * @param min   - minimum size of buffer
 * @param max   - maximum size of buffer
 * @param p_len - location for real size of allocated buffer
 *
 * @return Pointer to allocated buffer.
 */
extern void * tapi_make_buf(size_t min, size_t max, size_t *p_len);


/** 
 * Create a buffer of specified size
 * 
 * @param len  - Buffer length
 *
 * @return Pointer to allocated buffer.
 */
static inline void *
tapi_make_buf_by_len(size_t len)
{
    size_t ret_len;

    return tapi_make_buf(len, len, &ret_len);
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
tapi_make_buf_min(size_t min, size_t *p_len)
{
    return tapi_make_buf(min, min + 10, p_len);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LIB_TAPI_BUFS_H__ */
