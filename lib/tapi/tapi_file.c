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

#define TE_LGR_USER     "File TAPI"

#include "config.h"

#include <stdio.h>

#ifdef STDC_HEADERS
#include <string.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_TIME_H
#include <time.h>
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"
#include "rcf_api.h"
#include "logger_api.h"
#include "tapi_file.h"


/**
 * Create file in the TE temporary directory.
 *
 * @param len   file length
 * @param c     file content pattern
 *
 * @return name (memory is allocated) of the file or
 *         NULL in the case of failure
 *
 * @note the function is not thread-safe 
 */
char *
tapi_file_create(int len, char c)
{
    char *pathname = strdup(tapi_file_generate_pathname());
    char  buf[1024];
    FILE *f;
    
    if (pathname == NULL)
        return NULL;
    
    if ((f = fopen(pathname, "w")) == NULL)
    {
        ERROR("Cannot open file %s errno %d\n", pathname, errno);
        return NULL;
    }
    
    memset(buf, c, sizeof(buf));
    
    while (len > 0)
    {
        int copy_len = ((unsigned int)len  > sizeof(buf)) ?
                           (int)sizeof(buf) : len;

        if ((copy_len = fwrite((void *)buf, sizeof(char), copy_len, f)) < 0)
        {
            fclose(f);
            ERROR("fwrite() faled: file %s errno %d\n", pathname, errno);
            return NULL;
        }
        len -= copy_len;
    }
    if (fclose(f) < 0)
    {
        ERROR("fclose() failed: file %s errno=%d", pathname, errno);
        return NULL;
    }
    return pathname;
}

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
int 
tapi_file_create_ta(const char *ta, const char *filename, 
                    const char *fmt, ...)
{
    char  pathname[RCF_MAX_PATH];
    char *te_tmp;
    FILE *f;
    int   rc;

    va_list ap;

    if ((te_tmp = getenv("TE_TMP")) == NULL)
    {
        ERROR("TE_TMP is empty");
        return -1;
    }
    sprintf(pathname, "%s/%s", getenv("TE_TMP"), tapi_file_generate_name());

    if ((f = fopen(pathname, "w")) == NULL)
    {
        ERROR("Cannot open file %s errno %d\n", pathname, errno);
        return -1;
    }

    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    
    if (fclose(f) < 0)
    {
        ERROR("fclose() failed: file %s errno=%d", pathname, errno);
        unlink(pathname);
        return -1;
    }
    
    if ((rc = rcf_ta_put_file(ta, 0, pathname, filename)) != 0)
    {
        ERROR("Cannot put file %s on ta %s; errno %d", filename, ta, errno);
        unlink(pathname);
        return -1;
    }

    unlink(pathname);
    return 0;
}

