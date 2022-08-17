/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API to work with the files on the engine and TA.
 *
 * @defgroup ts_tapi_file Engine and TA files management
 * @ingroup te_ts_tapi
 * @{
 *
 * Functions for convinient work with the files on the engine and TA.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_TAPI_FILE_H__
#define __TE_TAPI_FILE_H__

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "te_stdint.h"
#include "te_str.h"
#include "rcf_common.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generate unique basename for file.
 *
 * @return generated name
 *
 * @note the function is not thread-safe
 */
static inline char *
tapi_file_generate_name(void)
{
    static int  num = 0;
    static char buf[RCF_MAX_PATH];

    TE_SPRINTF(buf, "te_tmp_%u_%u_%u", (uint32_t)time(NULL), getpid(), num++);

    return buf;
}

/**
 * Generate unique pathname for file on the engine.
 *
 * @return generated name
 *
 * @note the function is not thread-safe
 */
static inline char *
tapi_file_generate_pathname(void)
{
    static char  pathname[RCF_MAX_PATH];
    static char *te_tmp;

    if (te_tmp == NULL && (te_tmp = getenv("TE_TMP")) == NULL)
    {
        ERROR("TE_TMP is empty");
        return NULL;
    }

    TE_SPRINTF(pathname, "%s/%s", te_tmp, tapi_file_generate_name());

    return pathname;
}

/**
 * Create file in the TE temporary directory.
 *
 * @param len   file length
 * @param c     file content pattern
 *
 * @return name (memory is allocated) of the file or
 *         NULL in the case of failure
 *
 * @note The function is not thread-safe
 */
extern char *tapi_file_create_pattern(size_t len, char c);

/**
 * Create file in the TE temporary directory with the specified content.
 *
 * @param len     file length
 * @param buf     buffer with the file content
 * @param random  if TRUE, fill buffer with random data
 *
 * @return name (memory is allocated) of the file or
 *         NULL in the case of failure
 *
 * @note The function is not thread-safe
 */
extern char *tapi_file_create(size_t len, char *buf, te_bool random);

/**
 * Create file in the specified directory on the TA.
 *
 * @param ta            Test Agent name
 * @param filename      pathname of the file
 * @param fmt           format string for the file content
 *
 * @return Status code
 *
 * @note the function is not thread-safe
 */
extern te_errno tapi_file_create_ta(const char *ta, const char *filename,
                                    const char *fmt, ...);

/**
 * Create local file, copy it to TA, remove local file.
 * The function does the same thing as tapi_file_create_ta(),
 * but it creates local file with specified name instead of
 * using automatically generated name. This is useful when
 * you need to create files in a thread safe manner.
 *
 * @param ta            Test Agent name
 * @param lfile         pathname of the local file
 * @param rfile         pathname of the file on TA
 * @param fmt           format string for the file content
 *
 * @return Status code
 */
extern te_errno tapi_file_create_ta_r(const char *ta,
                                      const char *lfile,
                                      const char *rfile,
                                      const char *fmt, ...);

/**
 * Read file content from the TA.
 *
 * @param ta            Test Agent name
 * @param filename      pathname of the file
 * @param pbuf          location for buffer allocated by the routine
 *
 * @return Status code
 *
 * @note the function is not thread-safe
 */
extern te_errno tapi_file_read_ta(const char *ta, const char *filename,
                                  char **pbuf);


/**
 * Like tapi_file_create_ta(), but it appends data to the file.
 *
 * If the file does not exist, it is created.
 *
 * @param ta            Test Agent name
 * @param filename      pathname of the file
 * @param fmt           format string for the file content
 *
 * @return Status code
 *
 * @note the function is not thread-safe
 */
extern te_errno tapi_file_append_ta(const char *ta, const char *filename,
                                    const char *fmt, ...);



/**
 * Copy file from the one TA to other.
 *
 * @param ta_src        source Test Agent
 * @param src           source file name
 * @param ta_dst        destination Test Agent
 * @param dst           destination file name
 *
 * @return Status code
 */
extern te_errno tapi_file_copy_ta(const char *ta_src, const char *src,
                                  const char *ta_dst, const char *dst);

/**
 * Unlink file on the TA.
 *
 * @param ta            Test Agent name
 * @param path_fmt      Format string to make path to be deleted
 *
 * @return Status code.
 */
extern te_errno tapi_file_ta_unlink_fmt(const char *ta,
                                        const char *path_fmt, ...);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_FILE_H__ */

/**@} <!-- END ts_tapi_file --> */
