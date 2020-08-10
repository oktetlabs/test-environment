/** @file
 * @brief API to deal with files
 *
 * Functions to operate the files.
 *
 * Copyright (C) 2018 OKTET Labs. All rights reserved.
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#include "te_config.h"
#include <libgen.h>
#include "te_defs.h"
#include "te_errno.h"
#include "te_file.h"
#include "te_string.h"
#include "logger_api.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

    return strdup(name);
}

/* See description in te_file.h */
char *
te_dirname(const char *pathname)
{
    char path[(pathname == NULL ? 0 : strlen(pathname)) + 1];

    if (pathname == NULL)
        return NULL;

    memcpy(path, pathname, sizeof(path));
    return strdup(dirname(path));
}

/* See description in te_file.h */
int
te_file_create_unique_fd_va(char **filename, const char *prefix_format,
                            const char *suffix, va_list ap)
{
    int fd;
    te_errno rc;
    te_string file = TE_STRING_INIT;
    int suffix_len;

    assert(filename != NULL);

    rc = te_string_append_va(&file, prefix_format, ap);
    if (rc != 0)
        return -1;

    if (suffix == NULL)
    {
        rc = te_string_append(&file, "%s", "XXXXXX");
        suffix_len = 0;
    }
    else
    {
        rc = te_string_append(&file, "%s%s", "XXXXXX", suffix);
        suffix_len = strlen(suffix);
    }
    if (rc != 0)
    {
        te_string_free(&file);
        return -1;
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
    te_errno rc;
    va_list ap;
    FILE *f;

    va_start(ap, path_fmt);
    rc = te_string_append_va(&path, path_fmt, ap);
    va_end(ap);
    if (rc != 0)
    {
        ERROR("%s(): te_string_append_va() failed to fill file path, rc=%r",
              __FUNCTION__, rc);
        te_string_free(&path);
        return NULL;
    }

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
te_file_check_executable(const char *path)
{
    te_errno rc = 0;
    char *env_path;
    char *env_path_tmp = NULL;
    const char *env_path_delim = ":";
    te_string full_path = TE_STRING_INIT;
    te_bool found = FALSE;
    char *tmp;
    char *saveptr;

    if (strchr(path, '/') != NULL)
    {
        found = access(path, X_OK) == 0;
    }
    else
    {
        env_path = getenv("PATH");
        if (env_path == NULL)
        {
            ERROR("PATH is unset");
            rc = TE_ENOENT;
            goto out;
        }

        env_path_tmp = strdup(env_path);
        if (env_path_tmp == NULL)
        {
            rc = ENOMEM;
            goto out;
        }

        for (tmp = strtok_r(env_path_tmp, env_path_delim, &saveptr);
             tmp != NULL;
             tmp = strtok_r(NULL, env_path_delim, &saveptr))
        {
            rc = te_string_append(&full_path, "%s/%s", tmp, path);
            if (rc != 0)
                goto out;

            found = access(full_path.ptr, X_OK) == 0;

            if (found)
                break;

            te_string_reset(&full_path);
        }
    }

out:
    if (rc != 0)
    {
         ERROR("The check file '%s' was unsuccessful rc=%r",
                path, rc);
    }
    if (!found)
        rc = TE_ENOENT;

    te_string_free(&full_path);
    free(env_path_tmp);
    return rc;
}
