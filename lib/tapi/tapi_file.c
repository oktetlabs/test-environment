/** @file
 * @brief Test API
 *
 * Functions for convinient work with the files on the engine and TA.
 *
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Elena Vengerova <Elena.Vengerova@oktetlabs.ru>
 *
 */

#define TE_LGR_USER     "File TAPI"

#include "te_config.h"

#include <stdio.h>
#if STDC_HEADERS
#include <string.h>
#elif HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"
#include "te_bufs.h"
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
tapi_file_create_pattern(size_t len, char c)
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
char *
tapi_file_create(size_t len, char *buf, te_bool random)
{
    char *pathname = strdup(tapi_file_generate_pathname());
    FILE *f;

    if (pathname == NULL)
        return NULL;

    if (random)
        te_fill_buf(buf, len);

    if ((f = fopen(pathname, "w")) == NULL)
    {
        ERROR("Cannot open file %s errno %d\n", pathname, errno);
        return NULL;
    }

    if (fwrite((void *)buf, sizeof(char), len, f) < len)
    {
        fclose(f);
        ERROR("fwrite() faled: file %s errno %d\n", pathname, errno);
        return NULL;
    }

    if (fclose(f) < 0)
    {
        ERROR("fclose() failed: file %s errno=%d", pathname, errno);
        return NULL;
    }
    return pathname;
}

/**
 * Create local file, copy it to TA, remove local file.
 *
 * @param ta            Test Agent name
 * @param lfile         Pathname of the local file
 *                      (generate it if it is @c NULL)
 * @param rfile         Pathname of the file on TA
 * @param header        If not @c NULL, put it before @p fmt contents
 * @param fmt           Format string for the file content
 * @param ap            Format string arguments
 *
 * @return Status code
 */
static te_errno
tapi_file_create_ta_gen(const char *ta,
                        const char *lfile,
                        const char *rfile,
                        const char *header,
                        const char *fmt, va_list ap)
{
    FILE *f;
    te_errno rc;

    if (lfile == NULL)
        lfile = tapi_file_generate_pathname();

    if ((f = fopen(lfile, "w")) == NULL)
    {
        rc = TE_OS_RC(TE_TAPI, errno);
        ERROR("Cannot open file %s: %r", lfile, rc);
        return rc;
    }

    if (header != NULL)
        fputs(header, f);
    vfprintf(f, fmt, ap);

    if (fclose(f) < 0)
    {
        rc = TE_OS_RC(TE_TAPI, errno);
        ERROR("fclose() failed: file %s: %r", lfile, rc);
        unlink(lfile);
        return rc;
    }
    if ((rc = rcf_ta_put_file(ta, 0, lfile, rfile)) != 0)
    {
        ERROR("Cannot put file %s on TA %s: %r", rfile, ta, rc);
        unlink(lfile);
        return rc;
    }

    unlink(lfile);
    return 0;
}

/* See description in tapi_file.h */
te_errno
tapi_file_create_ta(const char *ta, const char *filename,
                    const char *fmt, ...)
{
    va_list ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = tapi_file_create_ta_gen(ta, NULL, filename, NULL, fmt, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_file.h */
te_errno
tapi_file_create_ta_r(const char *ta,
                      const char *lfile,
                      const char *rfile,
                      const char *fmt, ...)
{
    va_list ap;
    te_errno rc;

    va_start(ap, fmt);
    rc = tapi_file_create_ta_gen(ta, lfile, rfile, NULL, fmt, ap);
    va_end(ap);

    return rc;
}

/* See description in tapi_file.h */
te_errno
tapi_file_copy_ta(const char *ta_src, const char *src,
                  const char *ta_dst, const char *dst)
{
    char *pathname = tapi_file_generate_pathname();
    te_errno rc;
    struct stat st;

    if ((rc = rcf_ta_get_file(ta_src, 0, src, pathname)) != 0)
    {
        ERROR("Cannot get file %s from TA %s: %r", src, ta_src, rc);
        return rc;
    }

    if ((rc = rcf_ta_put_file(ta_dst, 0, pathname, dst)) != 0)
    {
        ERROR("Cannot put file %s to TA %s: %r", dst, ta_dst, rc);
        unlink(pathname);
        return rc;
    }

    if (stat(pathname, &st) != 0)
    {
        rc = TE_OS_RC(TE_TAPI, errno);
        ERROR("Cannot stat local file %s: %r", pathname, rc);
        unlink(pathname);
        return rc;
    }

    RING("Copy file %s:%s to %s:%s using local %s size %lld",
         ta_src, src, ta_dst, dst, pathname,
         (long long)st.st_size);

    unlink(pathname);
    return 0;
}

static te_errno
tapi_file_read_ta_gen(const char *ta, const char *filename,
                      te_bool may_not_exist, char **pbuf)
{
    char *pathname = tapi_file_generate_pathname();
    te_errno rc;
    char *buf;
    FILE *f;

    struct stat st;

    if ((rc = rcf_ta_get_file(ta, 0, filename, pathname)) != 0)
    {
        if (may_not_exist && TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            *pbuf = strdup("");
            return 0;
        }

        ERROR("Cannot get file %s from TA %s: %r", filename, ta, rc);
        return rc;
    }
    if (stat(pathname, &st) != 0)
    {
        rc = TE_OS_RC(TE_TAPI, errno);
        ERROR("Failed to stat file %s: %r", pathname, rc);
        return rc;
    }

    if ((buf = calloc(st.st_size + 1, 1)) == NULL)
    {
        ERROR("Out of memory");
        unlink(pathname);
        return TE_RC(TE_TAPI, TE_ENOMEM);
    }

    if ((f = fopen(pathname, "r")) == NULL)
    {
        rc = TE_OS_RC(TE_TAPI, errno);
        ERROR("Failed to open file %s: %r", pathname, rc);
        unlink(pathname);
        return rc;
    }

    if (fread(buf, 1, st.st_size, f) != (size_t)st.st_size)
    {
        ERROR("Failed to read from file %s", pathname);
        unlink(pathname);
        return TE_RC(TE_TAPI, TE_EIO);
    }

    fclose(f);
    unlink(pathname);

    *pbuf = buf;

    return 0;
}

/* See description in tapi_file.h */
te_errno
tapi_file_read_ta(const char *ta, const char *filename, char **pbuf)
{
    return tapi_file_read_ta_gen(ta, filename, FALSE, pbuf);
}

/* See description in tapi_file.h */
te_errno
tapi_file_append_ta(const char *ta, const char *filename, const char *fmt, ...)
{
    va_list ap;
    char *old_contents = NULL;
    te_errno rc = tapi_file_read_ta_gen(ta, filename, TRUE, &old_contents);

    if (rc != 0)
        return rc;

    va_start(ap, fmt);
    rc = tapi_file_create_ta_gen(ta, NULL, filename, old_contents, fmt, ap);
    va_end(ap);

    free(old_contents);
    return rc;
}

/* See description in tapi_file.h */
te_errno
tapi_file_ta_unlink_fmt(const char *ta, const char *path_fmt, ...)
{
    va_list ap;
    char    path[RCF_MAX_PATH];
    te_errno rc;
    te_errno ta_rc;

    va_start(ap, path_fmt);
    rc = te_vsnprintf(path, sizeof(path), path_fmt, ap);
    va_end(ap);
    if (rc != 0)
        return TE_RC_UPSTREAM(TE_TAPI, rc);

    rc = rcf_ta_call(ta, 0, "ta_rtn_unlink", &ta_rc, 1, FALSE,
                     RCF_STRING, path);
    if (rc != 0)
    {
        ERROR("%s(): rcf_ta_call() failed: %r", __FUNCTION__, rc);
        return rc;
    }
    if (ta_rc != 0)
    {
        ERROR("%s(): ta_rtn_unlink(%s, %s) failed: %r", __FUNCTION__,
              ta, path, ta_rc);
        return ta_rc;
    }

    return 0;
}
