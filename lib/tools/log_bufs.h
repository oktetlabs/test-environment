/** @file
 * @brief API to collect long log messages
 *
 * @defgroup te_tools_log_bufs Log buffers
 * @ingroup te_tools
 * @{
 *
 * Declaration of API to collect long log messages.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 *
 */

#ifndef __TE_LOG_BUFS_H__
#define __TE_LOG_BUFS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include "te_string.h"

/**
 * Free all resources allocated for logging buffers.
 * This function actually destroys all the existing buffers,
 * not just marks them free for further use.
 */
extern void te_log_bufs_cleanup(void);

/**
 * Obtain reusable TE string for logging.
 * The string can then be used like a normal TE string, however its buffer
 * cannot be replaced by te_string_set_buf().
 * Obtained string must be released with te_string_free().
 *
 * @return TE string pointer or @c NULL on failure.
 */
extern te_string *te_log_str_alloc(void);

/**
 * Append @a argc/@a argv arguments enclosed in double quotes and separated
 * by comma to TE string.
 *
 * @param str   Pointer to TE string
 * @param argc  Number of arguments
 * @param argv  Array with arguments
 *
 * @return Status code.
 */
extern te_errno te_args2te_str(te_string *str,
                               int argc, const char **argv);

/** Mapping of the bit number to string */
typedef struct te_log_buf_bit2str {
    unsigned int    bit; /**< Bit index */
    const char     *str; /**< String name of the bit */
} te_bit2str;

/** Mapping of the flag within mask to string */
typedef struct te_log_buf_flag2str {
    uint64_t        flag; /**< Flag. May be defined as more than
                               one bit set. */
    uint64_t        mask; /**< Mask applied when checking for flag.
                               In value X flag is set when
                               (X & mask) == flag */
    const char     *str;  /**< String name of the flag */
} te_flag2str;

/**
 * Append bit mask converted to string to TE string.
 *
 * Pipe '|' is used as a separator.
 *
 * @param str       Pointer to TE string
 * @param bit_mask  Bit mask
 * @param map       Bit to string map terminated by the element with @c NULL
 *                  string
 *
 * @return Status code.
 */
extern te_errno te_bit_mask2te_str(te_string *str,
                                   unsigned long long bit_mask,
                                   const te_bit2str *map);

/**
 * Extended version of te_bit_mask2te_str(). First it checks for
 * all bits from @p bm, unsetting them in @p bit_mask. Then the
 * bits which are left are checked against flags from @p fm.
 *
 * Pipe '|' is used as a separator.
 *
 * @param str       Pointer to TE string
 * @param bit_mask  Bit mask
 * @param bm        Bit to string map terminated by the element with @c NULL
 *                  string
 * @param fm        Flag within some mask to string map terminated by the
 *                  element with @c NULL string
 *
 * @return Status code.
 */
extern te_errno te_extended_bit_mask2te_str(
                                     te_string *str,
                                     unsigned long long bit_mask,
                                     const te_bit2str *bm,
                                     const te_flag2str *fm);

/*
 * NOTE: the following API is deprecated. It is better to allocate
 * te_string with te_log_str_alloc() and use te_string-specific functions
 * to avoid code duplication and simplify code reuse. te_string is used
 * in TE much more commonly and for various purposes.
 */

/*
 * Declaration of te_log_buf type, which is defined
 * in the implementation, so user can allocate and operate only
 * with pointer to this data structure.
 */
struct te_log_buf;
typedef struct te_log_buf te_log_buf;

/* Log buffer related functions */

/**
 * Allocates a buffer to be used for accumulating log message.
 * Mainly used in tapi_snmp itself.
 *
 * @note This function is deprecated. Use te_log_str_alloc().
 *
 * @return Pointer to the buffer.
 *
 * @note the caller does not have to check the returned
 * value against NULL, the function blocks the caller until it
 * gets available buffer.
 *
 * @note This is thread safe function
 */
extern te_log_buf *te_log_buf_alloc(void);

/**
 * Appends format string to the log message, the behaviour of
 * the function is the same as ordinary printf-like function.
 *
 * @param buf  Pointer to the buffer allocated with te_log_buf_alloc()
 * @param fmt  Format string followed by parameters
 *
 * @return The number of characters appended
 *
 * @note This is NOT thread safe function, so you are not allowed
 * to append the same buffer from different threads simultaneously.
 */
extern int te_log_buf_append(te_log_buf *buf, const char *fmt, ...);

/**
 * Returns current log message accumulated in the buffer.
 *
 * @param buf  Pointer to the buffer
 *
 * @return log message
 */
extern const char *te_log_buf_get(te_log_buf *buf);

/**
 * Release buffer allocated by te_log_buf_alloc()
 *
 * @param ptr  Pointer to the buffer
 *
 * @note This is thread safe function
 */
extern void te_log_buf_free(te_log_buf *buf);

/**
 * Put @a argc/@a argv arguments enclosed in double quotes and separated
 * by comma to log buffer.
 *
 * @note This function is deprecated. te_args2te_str() can be
 *       used with TE string instead.
 *
 * @param buf   Pointer to the buffer allocated with @b te_log_buf_alloc()
 * @param argc  Number of arguments
 * @param argv  Array with arguments
 *
 * @return @b te_log_buf_get() return value after addition
 */
extern const char *te_args2log_buf(te_log_buf *buf,
                                   int argc, const char **argv);


/**
 * Append bit mask converted to string to log buffer.
 *
 * Pipe '|' is used as a separator.
 *
 * @note This function is deprecated. te_bit_mask2te_str() can be
 *       used with TE string instead.
 *
 * @param buf       Pointer to the buffer
 * @param bit_mask  Bit mask
 * @param map       Bit to string map terminated by the element with #NULL
 *                  string
 *
 * @return te_log_buf_get() value.
 */
extern const char *te_bit_mask2log_buf(te_log_buf *buf,
                                       unsigned long long bit_mask,
                                       const struct te_log_buf_bit2str *map);

/**
 * Append extended bit mask converted to string to log buffer.
 *
 * Pipe '|' is used as a separator.
 *
 * @note This function is deprecated. te_extended_bit_mask2te_str() can be
 *       used with TE string instead.
 *
 * @param buf       Pointer to the buffer
 * @param bit_mask  Bit mask
 * @param bm        Bit to string map terminated by the element with #NULL
 *                  string
 * @param fm        Flag within some mask to string map terminated by the
 *                  element with #NULL string
 *
 * @return te_log_buf_get() value.
 */
extern const char *te_extended_bit_mask2log_buf(te_log_buf *buf,
                                        unsigned long long bit_mask,
                                        const struct te_log_buf_bit2str *bm,
                                        const struct te_log_buf_flag2str *fm);

/**
 * Put ether address to log buffer.
 *
 * @note This function is deprecated. te_mac_addr2te_str() can be used with
 *       TE string instead.
 *
 * @param buf   Pointer to the buffer allocated with @b te_log_buf_alloc()
 * @param argc  Pointer to the ether address
 *
 * @return @b te_log_buf_get() return value after addition
 */
extern const char *te_ether_addr2log_buf(te_log_buf *buf,
                                         const uint8_t * mac_addr);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LOG_BUFS_H__ */
/**@} <!-- END te_tools_log_bufs --> */
