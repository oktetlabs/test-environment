/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Test API
 *
 * Functions for convinient work with the files on the engine and TA.
 *
 *
 * Copyright (C) 2004-2023 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "File TAPI"

#include "te_config.h"

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
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
#include "te_file.h"

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

char *tapi_file_join_pathname(te_string *dest, const char *dirname,
                              const char *path, const char *suffix)
{
    if (path == NULL)
        return tapi_file_make_custom_pathname(dest, dirname, suffix);

    return te_file_join_filename(dest, dirname, path, suffix);
}

char *
tapi_file_resolve_ta_pathname(te_string *dest, const char *ta,
                              tapi_cfg_base_ta_dir base_dir,
                              const char *relname)
{
    char *dir = tapi_cfg_base_get_ta_dir(ta, base_dir);
    char *result = NULL;

    if (dir == NULL)
        return NULL;

    result = te_file_join_filename(dest, dir, relname, NULL);
    free(dir);

    return result;
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

/* See description in tapi_file.h */
char *
tapi_file_create_pattern(size_t len, char c)
{
    te_string pathname = TE_STRING_INIT;
    te_string buffer = TE_STRING_INIT_STATIC(1024);

    tapi_file_make_pathname(&pathname);

    memset(buffer.ptr, c, buffer.size);
    buffer.len = buffer.size;
    if (te_file_write_string(&buffer, len,
                             O_CREAT | O_TRUNC,
                             S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                             S_IROTH | S_IWOTH,
                             "%s", pathname.ptr) != 0)
    {
        unlink(pathname.ptr);
        te_string_free(&pathname);
        return NULL;
    }

    return pathname.ptr;
}

/* See description in tapi_file.h */
char *
tapi_file_create(size_t len, char *buf, bool random)
{
    te_string pathname = TE_STRING_INIT;
    te_string buffer = TE_STRING_EXT_BUF_INIT(buf, len + 1);
    te_errno rc;

    tapi_file_make_pathname(&pathname);

    if (random)
        te_fill_buf(buf, len);

    buffer.len = len;
    rc = te_file_write_string(&buffer, 0, O_CREAT | O_TRUNC,
                              S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                              S_IROTH | S_IWOTH,
                              "%s", pathname.ptr);
    if (rc != 0)
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
    te_string content = TE_STRING_INIT;
    te_errno rc;

    if (lfile == NULL)
        tapi_file_make_pathname(&lfile_name);
    else
        te_string_append(&lfile_name, "%s", lfile);

    if (header != NULL)
        te_string_append(&content, "%s", header);
    te_string_append_va(&content, fmt, ap);

    rc = te_file_write_string(&content, 0, O_CREAT | O_TRUNC,
                              S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                              S_IROTH | S_IWOTH, "%s",
                              lfile_name.ptr);
    if (rc == 0)
    {
        rc = rcf_ta_put_file(ta, 0, lfile_name.ptr, rfile);
        if (rc != 0)
            ERROR("Cannot put file %s on TA %s: %r", rfile, ta, rc);
    }
    unlink(lfile_name.ptr);
    te_string_free(&lfile_name);
    te_string_free(&content);

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
    bool need_unlink = false;

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
        need_unlink = true;
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
                      bool may_not_exist, char **pbuf)
{
    te_string pathname = TE_STRING_INIT;
    te_string content = TE_STRING_INIT;
    te_errno rc = 0;

    tapi_file_make_pathname(&pathname);
    if ((rc = rcf_ta_get_file(ta, 0, filename, pathname.ptr)) != 0)
    {
        te_string_free(&pathname);
        te_string_free(&content);

        if (may_not_exist && TE_RC_GET_ERROR(rc) == TE_ENOENT)
        {
            *pbuf = strdup("");
            return 0;
        }

        ERROR("Cannot get file %s from TA %s: %r", filename, ta, rc);
        return rc;
    }

    rc = te_file_read_string(&content, true, 0, "%s", pathname.ptr);

    unlink(pathname.ptr);
    te_string_free(&pathname);

    if (rc == 0)
        te_string_move(pbuf, &content);
    else
        te_string_free(&content);

    return rc;
}

/* See description in tapi_file.h */
te_errno
tapi_file_read_ta(const char *ta, const char *filename, char **pbuf)
{
    return tapi_file_read_ta_gen(ta, filename, false, pbuf);
}

/* See description in tapi_file.h */
te_errno
tapi_file_append_ta(const char *ta, const char *filename, const char *fmt, ...)
{
    va_list ap;
    char *old_contents = NULL;
    te_errno rc = tapi_file_read_ta_gen(ta, filename, true, &old_contents);

    if (rc != 0)
        return rc;

    va_start(ap, fmt);
    rc = tapi_file_create_ta_gen(ta, NULL, filename, old_contents, fmt, ap);
    va_end(ap);

    free(old_contents);
    return rc;
}

te_errno
tapi_file_expand_kvpairs(const char *ta, const char *template,
                         const char *posargs[TE_EXPAND_MAX_POS_ARGS],
                         const te_kvpair_h *kvpairs,
                         const char *filename_fmt, ...)
{
    va_list args;
    char path[RCF_MAX_PATH];
    te_errno rc = 0;
    te_string content = TE_STRING_INIT;

    va_start(args, filename_fmt);
    rc = te_vsnprintf(path, sizeof(path), filename_fmt, args);
    va_end(args);
    if (rc != 0)
        goto out;

    rc = te_string_expand_kvpairs(template, posargs, kvpairs, &content);
    if (rc != 0)
        goto out;

    if (ta != NULL)
        rc = tapi_file_create_ta(ta, path, "%s", content.ptr);
    else
    {
        rc = te_file_write_string(&content, 0, O_CREAT | O_TRUNC,
                                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                                  S_IROTH | S_IWOTH,
                                  "%s", path);
    }

out:
    te_string_free(&content);
    return TE_RC_UPSTREAM(TE_TAPI, rc);
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
