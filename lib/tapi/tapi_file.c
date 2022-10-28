/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API
 *
 * Functions for convinient work with the files on the engine and TA.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
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
#include "te_str.h"
#include "te_bufs.h"
#include "rcf_api.h"
#include "rcf_common.h"
#include "logger_api.h"
#include "tapi_file.h"

char *
tapi_file_make_name(te_string *dest)
{
    static unsigned int seq = 0;
    te_string tmp = TE_STRING_INIT;

    if (dest == NULL)
        dest = &tmp;

    te_string_append(dest, "te_tmp_%u_%u_%u", (unsigned)time(NULL), getpid(),
                     seq++);

    return dest->ptr;
}

char *
tapi_file_make_custom_pathname(te_string *dest, const char *dirname,
                               const char *suffix)
{
    te_string tmp = TE_STRING_INIT;

    if (dest == NULL)
        dest = &tmp;

    if (dirname != NULL)
        te_string_append(dest, "%s/", dirname);
    tapi_file_make_name(dest);
    if (suffix != NULL)
        te_string_append(dest, "%s", suffix);

    return dest->ptr;
}

char *
tapi_file_make_pathname(te_string *dest)
{
    const char *te_tmp = getenv("TE_TMP");

    if (te_tmp == NULL)
        TE_FATAL_ERROR("TE_TMP is empty");

    return tapi_file_make_custom_pathname(dest, te_tmp, NULL);
}

char *
tapi_file_generate_name(void)
{
    static char buf[RCF_MAX_PATH];
    te_string str = TE_STRING_BUF_INIT(buf);

    tapi_file_make_name(&str);

    return buf;
}

char *
tapi_file_generate_pathname(void)
{
    static char buf[RCF_MAX_PATH];
    te_string str = TE_STRING_BUF_INIT(buf);

    tapi_file_make_pathname(&str);

    return buf;
}

static te_errno
open_and_check(const char *filename, const char *mode, FILE **f)
{
    te_errno rc = 0;

    *f = fopen(filename, mode);
    if (*f == NULL)
    {
        rc = TE_OS_RC(TE_TAPI, errno);
        ERROR("Cannot open '%s': %r", filename, rc);
    }

    return rc;
}

static te_errno
write_and_check(FILE *f, const char *filename, const void *buf, size_t len)
{
    te_errno rc = 0;
    size_t actual_len;

    actual_len = fwrite(buf, 1, len, f);

    if (actual_len != len)
    {
        if (ferror(f))
        {
            rc = TE_OS_RC(TE_TAPI, errno);
            ERROR("Cannot write to %s: %r", filename, rc);
        }
        else
        {
            rc = TE_RC(TE_TAPI, TE_ENOSPC);
            ERROR("Too few bytes written: %zu < %zu", actual_len, len);
        }
    }
    return rc;
}

static te_errno
read_and_check(FILE *f, const char *filename, void *buf, size_t len)
{
    te_errno rc = 0;
    size_t actual_len;

    actual_len = fread(buf, 1, len, f);

    if (actual_len != len)
    {
        if (ferror(f))
        {
            rc = TE_OS_RC(TE_TAPI, errno);
            ERROR("Cannot read from %s: %r", filename, rc);
        }
        else
        {
            rc = TE_RC(TE_TAPI, TE_ENODATA);
            ERROR("Too few bytes read: %zu < %zu", actual_len, len);
        }
    }
    ((char *)buf)[len] = '\0';
    return rc;
}

static te_errno
close_and_check(FILE *f, const char *filename)
{
    te_errno rc = 0;

    if (fclose(f) != 0)
    {
        rc = TE_OS_RC(TE_TAPI, errno);
        ERROR("Cannot close %s: %r", filename, rc);
    }
    return rc;
}

/* See description in tapi_file.h */
char *
tapi_file_create_pattern(size_t len, char c)
{
    te_string pathname = TE_STRING_INIT;
    char  buf[1024];
    FILE *f;

    tapi_file_make_pathname(&pathname);

    if (open_and_check(pathname.ptr, "w", &f) != 0)
    {
        te_string_free(&pathname);
        return NULL;
    }

    memset(buf, c, sizeof(buf));

    while (len > 0)
    {
        size_t chunk = len > sizeof(buf) ? sizeof(buf) : len;

        if (write_and_check(f, pathname.ptr, buf, chunk) != 0)
        {
            fclose(f);
            te_string_free(&pathname);
            return NULL;
        }
        len -= chunk;
    }

    if (close_and_check(f, pathname.ptr) != 0)
    {
        te_string_free(&pathname);
        return NULL;
    }

    return pathname.ptr;
}

/* See description in tapi_file.h */
char *
tapi_file_create(size_t len, char *buf, te_bool random)
{
    te_string pathname = TE_STRING_INIT;
    FILE *f;

    tapi_file_make_pathname(&pathname);

    if (random)
        te_fill_buf(buf, len);

    if (open_and_check(pathname.ptr, "w", &f) != 0)
    {
        te_string_free(&pathname);
        return NULL;
    }

    if (write_and_check(f, pathname.ptr, buf, len) != 0)
    {
        fclose(f);
        te_string_free(&pathname);
        return NULL;
    }

    if (close_and_check(f, pathname.ptr) != 0)
    {
        te_string_free(&pathname);
        return NULL;
    }

    return pathname.ptr;
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
    te_string lfile_name = TE_STRING_INIT;
    FILE *f;
    te_errno rc;

    if (lfile == NULL)
        tapi_file_make_pathname(&lfile_name);
    else
        te_string_append(&lfile_name, "%s", lfile);

    if ((rc = open_and_check(lfile_name.ptr, "w", &f)) != 0)
        return rc;

    if (header != NULL)
        fputs(header, f);
    vfprintf(f, fmt, ap);

    rc = close_and_check(f, lfile_name.ptr);
    if (rc == 0)
    {
        rc = rcf_ta_put_file(ta, 0, lfile_name.ptr, rfile);
        if (rc != 0)
            ERROR("Cannot put file %s on TA %s: %r", rfile, ta, rc);
    }
    unlink(lfile_name.ptr);
    te_string_free(&lfile_name);

    return rc;
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
    te_string pathname = TE_STRING_INIT;
    te_errno rc = 0;
    struct stat st;
    te_bool need_unlink = FALSE;

    if (ta_src == NULL)
    {
        if (ta_dst == NULL)
        {
            ERROR("%s(): copying between local files is not supported",
                  __func__);
            te_string_free(&pathname);

            return TE_RC(TE_TAPI, TE_EOPNOTSUPP);
        }
        te_string_append(&pathname, "%s", src);
    }
    else if (ta_dst == NULL)
    {
        te_string_append(&pathname, "%s", dst);
    }
    else
    {
        tapi_file_make_pathname(&pathname);
        need_unlink = TRUE;
    }

    if (ta_src != NULL)
    {
        if ((rc = rcf_ta_get_file(ta_src, 0, src, pathname.ptr)) != 0)
        {
            ERROR("Cannot get file %s from TA %s: %r", src, ta_src, rc);
            te_string_free(&pathname);

            return rc;
        }
    }

    if (ta_dst != NULL)
    {
        if ((rc = rcf_ta_put_file(ta_dst, 0, pathname.ptr, dst)) != 0)
        {
            ERROR("Cannot put file %s to TA %s: %r", dst, ta_dst, rc);
            goto cleanup;
        }
    }

    if (stat(pathname.ptr, &st) != 0)
    {
        rc = TE_OS_RC(TE_TAPI, errno);
        ERROR("Cannot stat local file %s: %r", pathname.ptr, rc);
        goto cleanup;
    }

    RING("Copy file %s:%s to %s:%s using local %s size %lld",
         ta_src, src, ta_dst, dst, pathname.ptr,
         (long long)st.st_size);

cleanup:
    if (need_unlink)
        unlink(pathname.ptr);
    te_string_free(&pathname);

    return rc;
}

static te_errno
tapi_file_read_ta_gen(const char *ta, const char *filename,
                      te_bool may_not_exist, char **pbuf)
{
    te_string pathname = TE_STRING_INIT;
    te_errno rc = 0;
    char *buf = NULL;
    FILE *f = NULL;

    struct stat st;
    size_t to_read;

    tapi_file_make_pathname(&pathname);
    if ((rc = rcf_ta_get_file(ta, 0, filename, pathname.ptr)) != 0)
    {
        te_string_free(&pathname);

        if (may_not_exist && TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            *pbuf = strdup("");
            return 0;
        }

        ERROR("Cannot get file %s from TA %s: %r", filename, ta, rc);
        return rc;
    }
    if (stat(pathname.ptr, &st) != 0)
    {
        rc = TE_OS_RC(TE_TAPI, errno);
        ERROR("Failed to stat file %s: %r", pathname.ptr, rc);
        te_string_free(&pathname);

        return rc;
    }
    to_read = st.st_size;

    if ((buf = malloc(to_read + 1)) == NULL)
    {
        rc = TE_OS_RC(TE_TAPI, errno);
        ERROR("Out of memory");
    }
    else
    {
        rc = open_and_check(pathname.ptr, "r", &f);
        if (rc == 0)
            rc = read_and_check(f, pathname.ptr, buf, to_read);
    }

    if (f != NULL)
        fclose(f);
    unlink(pathname.ptr);
    te_string_free(&pathname);

    if (rc == 0)
        *pbuf = buf;
    else
        free(buf);

    return rc;
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

    va_start(ap, path_fmt);
    rc = te_vsnprintf(path, sizeof(path), path_fmt, ap);
    va_end(ap);
    if (rc != 0)
        return TE_RC_UPSTREAM(TE_TAPI, rc);

    rc = rcf_ta_del_file(ta, 0, path);
    if (rc != 0)
    {
        ERROR("%s(): rcf_ta_del_file() failed: %r", __FUNCTION__, rc);
        return rc;
    }

    return 0;
}
