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

#ifndef __TE_LIB_TAPI_FILE_H__
#define __TE_LIB_TAPI_FILE_H__

/**
 * Generate unique basename for file.
 *
 * @return generated name
 *
 * @note the function is not thread-safe
 */
static inline char *
tapi_file_generate_name()
{
    static int  num = 0;
    static char buf[RCF_MAX_PATH];
    
    sprintf(buf, "te_tmp_%u_%u_%u", (uint32_t)time(NULL), getpid(), num++);
    
    return buf;
}

/**
 * Create file in the TE temporary directory.
 *
 * @param len   file length
 * @param c     file content
 *
 * @return name (memory is allocated) of the file or NULL in the case of failure
 *
 * @note the function is not thread-safe 
 */
extern char *tapi_file_create(int len, char c);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_LIB_TAPI_FILE_H__ */
