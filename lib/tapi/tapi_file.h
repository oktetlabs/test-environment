/** @file
 * @brief Test API
 *
 * Functions for convinient work with the files on the engine and TA.
 *
 * Copyright (C) 2005 OKTET Labs Ltd., St.-Petersburg, Russia
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 * $Id$
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
#include "rcf_common.h"


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
    
    sprintf(buf, "te_tmp_%u_%u_%u", (uint32_t)time(NULL), getpid(), num++);
    
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

    sprintf(pathname, "%s/%s", te_tmp, tapi_file_generate_name());
    
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
extern char *tapi_file_create(int len, char c);

/**
 * Create file in the specified directory on the TA.
 *
 * @param ta            Test Agent name
 * @param filename      pathname of the file
 * @param fmt           format string for the file content
 *
 * @return 0 (success) or -1 (failure)
 *
 * @note the function is not thread-safe
 */
extern int tapi_file_create_ta(const char *ta, const char *filename, 
                               const char *fmt, ...);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_FILE_H__ */
