/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief API to deal with files
 *
 * Functions to operate the files.
 *
 * Copyright (C) 2018-2023 OKTET Labs Ltd. All rights reserved.
 */

#include "te_config.h"
#include <libgen.h>
#include "te_defs.h"
#include "te_errno.h"
#include "te_file.h"
#include "te_str.h"
#include "te_string.h"
#include "te_dbuf.h"
#include "te_alloc.h"
#include "te_rand.h"
#include "te_bufs.h"
#include "logger_api.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#endif

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

/* See description in te_file.h */
char *
te_basename(const char *pathname)
{
    char path[(pathname == NULL ? 0 : strlen(pathname)) + 1];
    char *name;

    if (pathname == NULL)
        return NULL;

    memcpy(path, pathname, sizeof(path));
    name = basename(path);
    if (*name == '/' || *name == '.')
        return NULL;

    return TE_STRDUP(name);
}

/* See description in te_file.h */
char *
te_readlink_fmt(const char *path_fmt, ...)
{
    va_list ap;
    te_string path = TE_STRING_INIT;
    te_dbuf linkbuf = TE_DBUF_INIT(100);
    bool resolved = false;

    va_start(ap, path_fmt);
    te_string_append_va(&path, path_fmt, ap);
    va_end(ap);

    te_dbuf_append(&linkbuf, NULL, path.len + 1);

    while (!resolved)
    {
        ssize_t nbytes = readlink(path.ptr, (char *)linkbuf.ptr, linkbuf.len);

        if (nbytes < 0)
        {
            ERROR("%s(): cannot resolve '%s': %r", __FUNCTION__, path.ptr,
                  TE_OS_RC(TE_MODULE_NONE, errno));

            te_string_free(&path);
            te_dbuf_free(&linkbuf);
            return NULL;
        }
        if (nbytes < linkbuf.len)
        {
            resolved = true;
            linkbuf.ptr[nbytes] = '\0';
        }
        else
        {
            te_dbuf_append(&linkbuf, NULL, linkbuf.len);
        }
    }

    te_string_free(&path);
    return (char *)linkbuf.ptr;
}

/* See description in te_file.h */
char *
te_dirname(const char *pathname)
{
    char path[(pathname == NULL ? 0 : strlen(pathname)) + 1];

    if (pathname == NULL)
        return NULL;

    memcpy(path, pathname, sizeof(path));
    return TE_STRDUP(dirname(path));
}

/* See description in te_file.h */
char *
te_file_join_filename(te_string *result, const char *dirname, const char *path,
                      const char *suffix)
{
    te_string tmp = TE_STRING_INIT;

    if (result == NULL)
        result = &tmp;

    if (path == NULL || *path != '/')
        te_string_append(result, "%s", te_str_empty_if_null(dirname));

    if (!te_str_is_null_or_empty(path))
    {
        bool need_slash = result->len > 0 && result->ptr[result->len - 1] != '/';\
        te_string_append(result, "%s%s", need_slash ? "/" : "", path);
    }
    if (!te_str_is_null_or_empty(suffix))
    {
        te_string_chop(result, "/");
        te_string_append(result, "%s", suffix);
    }

    return result->ptr;
}

/* See description in te_file.h */
int
te_file_create_unique_fd_va(char **filename, const char *prefix_format,
                            const char *suffix, va_list ap)
{
    int fd;
    te_string file = TE_STRING_INIT;
    int suffix_len;

    assert(filename != NULL);

    te_string_append_va(&file, prefix_format, ap);

    if (suffix == NULL)
    {
        te_string_append(&file, "%s", "XXXXXX");
        suffix_len = 0;
    }
    else
    {
        te_string_append(&file, "%s%s", "XXXXXX", suffix);
        suffix_len = strlen(suffix);
    }

    fd = mkstemps(file.ptr, suffix_len);
    if (fd != -1)
    {
        INFO("File has been created: '%s'", file.ptr);
        *filename = file.ptr;
    }
    else
    {
        ERROR("Failed to create file '%s': %s", file.ptr, strerror(errno));
        te_string_free(&file);
    }

    return fd;
}

/* See description in te_file.h */
int
te_file_create_unique_fd(char **filename, const char *prefix_format,
                         const char *suffix, ...)
{
    int fd;
    va_list ap;

    va_start(ap, suffix);
    fd = te_file_create_unique_fd_va(filename, prefix_format, suffix, ap);
    va_end(ap);

    return fd;
}

/* See description in te_file.h */
char *
te_file_create_unique(const char *prefix_format, const char *suffix, ...)
{
    va_list ap;
    char *filename = NULL;
    int fd;

    va_start(ap, suffix);
    fd = te_file_create_unique_fd_va(&filename, prefix_format, suffix, ap);
    va_end(ap);
    if (fd != -1)
        close(fd);

    return filename;
}

/* See description in te_file.h */
pid_t
te_file_read_pid(const char *pid_path)
{
    pid_t       pid = -1;
    FILE       *f;

    assert(pid_path != NULL);

    f = fopen(pid_path, "r");
    if (f == NULL)
        return -1;

    if (fscanf(f, "%d", &pid) != 1)
    {
        fclose(f);
        return -1;
    }

    fclose(f);

    return pid;
}

/* See description in te_file.h */
FILE *
te_fopen_fmt(const char *mode, const char *path_fmt, ...)
{
    te_string path = TE_STRING_INIT;
    va_list ap;
    FILE *f;

    va_start(ap, path_fmt);
    te_string_append_va(&path, path_fmt, ap);
    va_end(ap);

    f = fopen(path.ptr, mode);
    if (f == NULL)
    {
        ERROR("%s(): failed to open '%s' with mode '%s', errno=%d ('%s')",
              __FUNCTION__, path.ptr, mode, errno, strerror(errno));
    }

    te_string_free(&path);

    return f;
}

te_errno
te_file_resolve_pathname_vec(const char *filename, const te_vec *pathvec,
                             int mode, char **resolved)
{
    te_errno rc = 0;

    if (filename == NULL || pathvec == NULL)
        return TE_EFAULT;

    assert(pathvec->element_size == sizeof(const char *));

    if (*filename == '/' || te_vec_size(pathvec) == 0)
    {
        if (access(filename, mode))
            rc = TE_OS_RC(TE_MODULE_NONE, errno);
        else if (resolved != NULL)
            *resolved = TE_STRDUP(filename);
    }
    else
    {
        const char * const *dir;
        te_string fullpath = TE_STRING_INIT;

        TE_VEC_FOREACH(pathvec, dir)
        {
            te_string_append(&fullpath, "%s/%s", *dir, filename);

            if (!access(fullpath.ptr, mode))
            {
                if (resolved != NULL)
                    *resolved = fullpath.ptr;
                else
                    te_string_free(&fullpath);
                return 0;
            }
            rc = TE_OS_RC(TE_MODULE_NONE, errno);
            te_string_reset(&fullpath);
        }
        te_string_free(&fullpath);
    }

    return rc;
}

te_errno
te_file_resolve_pathname(const char *filename, const char *path,
                         int mode, const char *basename, char **resolved)
{
    te_errno rc;
    te_vec pathvec = TE_VEC_INIT(char *);

    if (basename != NULL)
    {
        char *basedir;
        struct stat st;

        if (stat(basename, &st))
        {
            WARN("Cannot stat '%s': %r", basename,
                 TE_OS_RC(TE_MODULE_NONE, errno));
        }
        else
        {
            if (S_ISDIR(st.st_mode))
                basedir = TE_STRDUP(basename);
            else
                basedir = te_dirname(basename);

            if (basedir == NULL)
            {
                rc = TE_OS_RC(TE_MODULE_NONE, errno);
                ERROR("Cannot determine dirname for '%s'", basename);
                te_vec_free(&pathvec);
                return rc;
            }
            TE_VEC_APPEND(&pathvec, basedir);
        }
    }

    te_vec_split_string(path, &pathvec, ':', true);

    rc = te_file_resolve_pathname_vec(filename, &pathvec, mode, resolved);
    te_vec_deep_free(&pathvec);

    return rc;
}

te_errno
te_file_check_executable(const char *path)
{
    return te_file_resolve_pathname(path, getenv("PATH"),
                                    X_OK, NULL, NULL);
}

te_errno
te_access_fmt(int mode, const char *fmt, ...)
{
    te_string path = TE_STRING_INIT;
    te_errno rc = 0;
    va_list args;

    va_start(args, fmt);
    te_string_append_va(&path, fmt, args);
    va_end(args);

    if (access(path.ptr, mode) != 0)
        rc = TE_OS_RC(TE_MODULE_NONE, errno);

    te_string_free(&path);

    return rc;
}

te_errno
te_unlink_fmt(const char *fmt, ...)
{
    te_string path = TE_STRING_INIT;
    te_errno rc = 0;
    va_list args;

    va_start(args, fmt);
    te_string_append_va(&path, fmt, args);
    va_end(args);

    if (unlink(path.ptr) != 0)
        rc = TE_OS_RC(TE_MODULE_NONE, errno);

    te_string_free(&path);

    return rc;
}

te_errno
te_file_read_string(te_string *dest, bool binary,
                    size_t maxsize, const char *path_fmt, ...)
{
    te_string pathname = TE_STRING_INIT;
    va_list args;
    struct stat st;
    te_errno rc = 0;
    int fd = -1;
    ssize_t actual = 0;

    va_start(args, path_fmt);
    te_string_append_va(&pathname, path_fmt, args);
    va_end(args);

    if (stat(pathname.ptr, &st))
    {
        rc = te_rc_os2te(errno);
        ERROR("Cannot stat '%s': %r", pathname.ptr, rc);
        goto out;
    }

    /* Some filesystems, e.g. sysfs, do not report file size correctly. */
    if (st.st_blocks == 0 && st.st_size > 0)
        st.st_size = 0;

    if (maxsize != 0 && st.st_size > maxsize)
    {
        ERROR("File %s's size (%zu) is larger than %zu",
              pathname.ptr, (size_t)st.st_size, maxsize);
        rc = TE_EFBIG;
        goto out;
    }

    fd  = open(pathname.ptr, O_RDONLY);
    if (fd < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("Cannot open '%s' for reading: %r",
              pathname.ptr, rc);
        goto out;
    }

    if (st.st_size > 0)
    {
        if (dest->ext_buf && dest->size < dest->len + st.st_size + 1)
        {
            ERROR("Not enough space in the external buffer to fit the contents of '%s'",
                  pathname.ptr);
            rc = TE_ESMALLBUF;
            goto out;
        }
        te_string_reserve(dest, dest->len + st.st_size + 1);
        actual = read(fd, dest->ptr + dest->len, st.st_size);
        if (actual < 0)
        {
            rc = te_rc_os2te(errno);
            ERROR("Cannot read from '%s': %r", pathname.ptr, rc);
            goto out;
        }

        if ((size_t)actual != (size_t)st.st_size)
        {
            ERROR("Could not read %zu bytes from %s, only %zu were read",
                  (size_t)st.st_size, pathname.ptr, (size_t)actual);
            rc = TE_EIO;
            goto out;
        }
    }
    else
    {
        /* The value is pretty arbitrary */
        ssize_t chunk_size = 4096;
        ssize_t actual_chunk;

        if (dest->ext_buf)
        {
            ssize_t space_left = dest->size - dest->len - 1;

            if (space_left <= 0)
            {
                ERROR("No space in the external buffer");
                rc = TE_ESMALLBUF;
                goto out;
            }
            if (maxsize == 0)
                maxsize = space_left;
            else
                maxsize = MIN(maxsize, space_left);
        }

        do
        {
            if (maxsize != 0 && actual + chunk_size > maxsize)
            {
                chunk_size = maxsize - actual;
                if (chunk_size == 0)
                {
                    ERROR("Cannot read more than %zu bytes from %s",
                          maxsize, pathname.ptr);
                    rc = TE_EFBIG;
                    goto out;
                }
            }
            te_string_reserve(dest, dest->len + actual + chunk_size);
            actual_chunk = read(fd, dest->ptr + dest->len + actual, chunk_size);
            if (actual_chunk < 0)
            {
                rc = te_rc_os2te(errno);
                ERROR("Cannot read from '%s' after %zd bytes: %r",
                      pathname.ptr, actual, rc);
                goto out;
            }
            actual += actual_chunk;
        } while (actual_chunk > 0);
    }

    dest->len += actual;
    dest->ptr[dest->len] = '\0';

    if (!binary)
    {
        ssize_t i;

        for (i = 0; i < actual; i++)
        {
            if (dest->ptr[dest->len - actual + i] == '\0')
            {
                ERROR("File '%s' contains an embedded zero at %zu",
                      pathname.ptr, (size_t)i);
                te_string_cut(dest, actual);
                rc = TE_EILSEQ;
                goto out;
            }
        }

        te_string_chop(dest, "\n");
    }

out:
    if (fd >= 0)
        close(fd);
    te_string_free(&pathname);

    return rc;
}

te_errno
te_file_write_string_va(const te_string *src, size_t fitlen,
                        int flags, mode_t mode,
                        const char *path_fmt, va_list args)
{
    te_string pathname = TE_STRING_INIT;
    te_errno rc = 0;
    int fd = -1;
    ssize_t actual;
    size_t remaining = fitlen == 0 ? src->len : fitlen;

    te_string_append_va(&pathname, path_fmt, args);

    fd  = open(pathname.ptr, O_WRONLY | flags, mode);
    if (fd < 0)
    {
        rc = te_rc_os2te(errno);
        ERROR("Cannot open '%s' for writing: %r",
              pathname.ptr, rc);
        goto out;
    }

    while (remaining > 0)
    {
        size_t chunk = src->len > remaining ? remaining : src->len;

        actual = write(fd, src->ptr, chunk);
        if (actual < 0)
        {
            rc = te_rc_os2te(errno);
            ERROR("Cannot write to '%s': %r", pathname.ptr, rc);
            goto out;
        }

        if ((size_t)actual != chunk)
        {
            ERROR("Could not write %zu bytes to %s, only %zu were written",
                  chunk, pathname.ptr, (size_t)actual);
            rc = TE_EIO;
            goto out;
        }

        remaining -= chunk;
    }
out:
    if (fd >= 0)
    {
        if (close(fd) != 0)
        {
            rc = te_rc_os2te(errno);
            ERROR("Error closing '%s': %r", pathname.ptr, rc);
        }
    }
    te_string_free(&pathname);

    return rc;
}

te_errno
te_file_write_string(const te_string *src, size_t fitlen,
                     int flags, mode_t mode,
                     const char *path_fmt, ...)
{
    va_list args;
    te_errno rc;

    va_start(args, path_fmt);
    rc = te_file_write_string_va(src, fitlen, flags, mode,
                                 path_fmt, args);
    va_end(args);

    return rc;
}

te_errno te_file_write_spec_buf(size_t minlen, size_t maxlen,
                                size_t *p_len,
                                const char *spec,
                                int flags, mode_t mode,
                                const char *path_fmt, ...)
{
    size_t size = te_rand_unsigned(minlen, maxlen);
    te_string str = TE_STRING_INIT;
    te_errno rc;
    va_list args;

    te_string_reserve(&str, size + 1);
    rc = te_fill_spec_buf(str.ptr, size, spec);
    if (rc != 0)
    {
        te_string_free(&str);
        return rc;
    }

    str.len = size;
    va_start(args, path_fmt);
    rc = te_file_write_string_va(&str, 0, flags, mode, path_fmt, args);
    va_end(args);

    te_string_free(&str);

    if (rc == 0 && p_len != NULL)
        *p_len = size;
    return rc;
}

te_errno
te_file_read_text(const char *path, char *buffer, size_t bufsize)
{
    te_string bufstr = TE_STRING_EXT_BUF_INIT(buffer, bufsize);

    return te_file_read_string(&bufstr, false, bufsize - 1, "%s", path);
}

static te_errno
do_scandir(const char *dirname, const char *pattern,
           te_file_scandir_callback *callback, void *data)
{
    te_errno rc = 0;
    DIR *dir = opendir(dirname);
    struct dirent *entry = NULL;
    te_string pathname = TE_STRING_INIT;
    size_t dirname_len;

    te_string_append(&pathname, "%s/", dirname);
    dirname_len = pathname.len;

    if (dir == NULL)
    {
        rc = TE_OS_RC(TE_MODULE_NONE, errno);
        ERROR("Cannot open the directory '%s': %r", dirname, rc);
        te_string_free(&pathname);
        return rc;
    }

    do {
        errno = 0;
        entry = readdir(dir);
        if (entry == NULL)
        {
            if (errno != 0)
            {
                rc = TE_OS_RC(TE_MODULE_NONE, errno);
                ERROR("Error scanning '%s': %r", dirname, rc);
            }
        }
        else
        {
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            if (pattern != NULL &&
                fnmatch(pattern, entry->d_name, FNM_PATHNAME | FNM_PERIOD) != 0)
            {
                continue;
            }

            pathname.len = dirname_len;
            pathname.ptr[pathname.len] = '\0';
            te_string_append(&pathname, "%s", entry->d_name);
            rc = callback(pattern, pathname.ptr, data);
            if (rc != 0)
            {
                entry = NULL;
                if (rc == TE_EOK)
                    rc = 0;
            }
        }
    } while (entry != NULL);

    closedir(dir);
    te_string_free(&pathname);

    return rc;
}

te_errno
te_file_scandir(const char *dirname,
                te_file_scandir_callback *callback, void *data,
                const char *pattern_fmt, ...)
{
    te_string pattern = TE_STRING_INIT;
    te_errno rc;

    if (pattern_fmt != NULL)
    {
       va_list args;

       va_start(args, pattern_fmt);
       te_string_append_va(&pattern, pattern_fmt, args);
       va_end(args);
    }

    rc = do_scandir(dirname, pattern_fmt == NULL ? NULL : pattern.ptr,
                    callback, data);

    te_string_free(&pattern);
    return rc;
}

static void
analyze_pattern(const char *pattern, size_t *prefix_len, size_t *suffix_len)
{
    bool seen_wildcard = false;
    size_t count = 0;

    while (*pattern != '\0')
    {
        switch (*pattern)
        {
            case '\\':
                count++;
                assert(pattern[1] != '\0');
                pattern += 2;
                break;
            case '[':
                count++;
                pattern++;
                if (*pattern == '!')
                    pattern++;
                if (*pattern == ']')
                    pattern++;
                while (*pattern != ']')
                {
                    assert(*pattern != '\0');
                    if (*pattern == '[' &&
                        (pattern[1] == ':' ||
                         pattern[1] == '.' ||
                         pattern[1] == '='))
                    {
                        pattern = strchr(pattern, ']');
                        assert(pattern != NULL);
                    }
                    pattern++;
                }
                pattern++;
                break;
            case '*':
                if (seen_wildcard)
                    TE_FATAL_ERROR("Multiple wildcards in the pattern");
                *prefix_len = count;
                pattern++;
                count = 0;
                seen_wildcard = true;
                break;
            default:
                pattern++;
                count++;
                break;
        }
    }

    if (!seen_wildcard)
        TE_FATAL_ERROR("No wildcard in the pattern");
    *suffix_len = count;
}

char *
te_file_extract_glob(const char *filename, const char *pattern,
                     bool basename)
{
    size_t prefix_len;
    size_t suffix_len;
    size_t fname_len;
    char *result = NULL;

    if (basename)
    {
        const char *rdir = strrchr(filename, '/');

        if (rdir != NULL)
            filename = rdir + 1;
    }

    if (fnmatch(pattern, filename, FNM_PATHNAME | FNM_PERIOD) != 0)
        return NULL;

    analyze_pattern(pattern, &prefix_len, &suffix_len);
    fname_len = strlen(filename);
    assert(prefix_len + suffix_len <= fname_len);

    result = TE_ALLOC(fname_len - prefix_len - suffix_len + 1);

    memcpy(result, filename + prefix_len, fname_len - prefix_len - suffix_len);
    result[fname_len - prefix_len - suffix_len] = '\0';

    return result;
}
