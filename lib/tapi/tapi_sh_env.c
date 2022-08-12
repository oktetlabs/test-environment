/** @file
 * @brief Test API to control environment variables on the agent side
 *
 * Implementation of TAPI to control environment variables on TA
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 */

#define TE_LGR_USER     "TAPI sh env"

#include "te_config.h"

#include "tapi_sh_env.h"
#include "rcf_api.h"

static rcf_ta_cb tapi_expand_path;
static te_errno
tapi_expand_path(const char *ta, void *cookie)
{
    static const char *default_dirs[] = {
        "/usr/local/sbin",
        "/usr/sbin",
        "/sbin",
        NULL,
    };
    const char *const *dirs;
    unsigned int i;
    te_errno rc;

    dirs = cookie != NULL ? cookie : default_dirs;

    for (i = 0; dirs[i] != NULL; i++)
    {
        rc = tapi_sh_env_ta_path_append(ta, dirs[i]);
        if (rc != 0)
        {
            ERROR("Failed to expand agent's %s PATH", ta);
            return rc;
        }
    }

    return 0;
}

int
tapi_expand_path_all_ta(const char **dirs)
{
    return rcf_foreach_ta(tapi_expand_path, dirs);
}
