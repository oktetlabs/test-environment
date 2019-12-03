/** @file
 * @brief Agent library
 *
 * Functions equivalent to <unistd> ones and supporting specific extra actions
 *
 * Copyright (C) 2003-2019 OKTET Labs. All rights reserved.
 *
 * @author Ivan Melnikov <Ivan.Melnikov@oktetlabs.ru>
 */

#define TE_LGR_USER      "Agent library"

#include "agentlib.h"
#include <sys/stat.h>
#include <sys/types.h>
#include "te_errno.h"
#include "logger_api.h"

/* See description in agentlib.h */
te_errno
mkdirp(const char *path, int mode)
{
    int st;
    te_errno rc = 0;
    char dir[strlen(path) + 1];
    char *next;

#define SKIP_SLASHES(_p) \
    while (*(_p) == '/') {      \
       (_p)++;                  \
    }

    memcpy(dir, path, sizeof(dir));
    next = *dir == '/' ? dir + 1 : dir;
    SKIP_SLASHES(next);
    if (*next == '\0')
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    while (next != NULL && *next != '\0')
    {
        next = strchr(next, '/');
        if (next != NULL)
            *next = '\0';

        errno = 0;  /* To reset possible parent's EEXIST */
        st = mkdir(dir, mode);
        if (st != 0 && errno != EEXIST)
        {
            int mkdir_errno = errno;

            ERROR("%s(): mkdir '%s' failed: %s",
                  __FUNCTION__, path, strerror(mkdir_errno));
            errno = mkdir_errno;
            rc = TE_OS_RC(TE_TA_UNIX, errno);
            break;
        }

        if (next != NULL)
        {
            *next = '/';
            next++;
            SKIP_SLASHES(next);
        }
    }
#undef SKIP_SLASHES

    return rc;
}
