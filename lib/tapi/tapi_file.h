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
 * Create file in the TE temporary directory.
 *
 * @param len   File length
 * @param c     File content
 *
 * @return Name (memory is allocated) of the file or
 *         NULL in the case of failure.
 *
 * @note The function is not thread-safe 
 */
extern char *tapi_file_create(int len, char c);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_FILE_H__ */
