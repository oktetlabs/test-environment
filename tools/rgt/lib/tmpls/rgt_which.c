/** @file
 * @brief API to locate the first entry of command name in PATH variable
 *
 * Command location
 *
 *
 * Copyright (C) 2018-2022 OKTET Labs. All rights reserved.
 *
 *
 */

#include "te_config.h"

#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include "te_defs.h"
#include "rgt_which.h"

static te_bool
file_in_directory_is_exe(const char *file_name, const char *directory,
                         size_t directory_length)
{
    char path[PATH_MAX];
    int n;

    if (directory_length == 0)
        n = snprintf(path, sizeof(path), "./%s", file_name);
    else
    {
        n = snprintf(path, sizeof(path), "%.*s/%s",
                     (int)directory_length, directory, file_name);
    }

    if (n < 0 || (size_t)n >= sizeof(path))
        return FALSE;

    if (access(path, X_OK) < 0)
        return FALSE;

    return TRUE;
}

static te_errno
write_directory_to_location(const char *directory, size_t directory_length,
                            size_t size, char *location)
{
    int n;

    if (directory_length == 0)
        n = snprintf(location, size, ".");
    else
        n = snprintf(location, size, "%.*s", (int)directory_length, directory);

    if (n < 0 || (size_t)n >= size)
        return TE_ESMALLBUF;

    return 0;
}

/* See description in rgt_which.h */
te_errno
rgt_which(const char *command, size_t size, char *location)
{
    const char *path;
    const char *path_delim = ":";

    assert(command != NULL);
    assert(location != NULL);

    if (strchr(command, '/') != NULL)
    {
        fputs("Command contains a path separator\n", stderr);
        return TE_EINVAL;
    }

    if ((path = getenv("PATH")) == NULL || strlen(path) == 0)
    {
        fputs("PATH is empty\n", stderr);
        return TE_ENOENT;
    }

    while (1)
    {
        size_t complen = strcspn(path, path_delim);

        if (file_in_directory_is_exe(command, path, complen))
            return write_directory_to_location(path, complen, size, location);

        if (*path == '\0')
            break;

        path += complen + 1;
    }

    return TE_ENOENT;
}
