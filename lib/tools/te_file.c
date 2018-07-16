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
#include "te_defs.h"
#include "te_errno.h"
#include "te_file.h"
#include "te_string.h"
#include "logger_api.h"


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
