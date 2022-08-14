/** @file
 * @brief API to modify TRC tags from prologues
 *
 * Implementation of API to modify TRC tags from prologues.
 *
 * Copyright (C) 2022-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "TAPI Tags"

#include "te_config.h"

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "te_errno.h"
#include "conf_api.h"
#include "tapi_test.h"

/* See the description from tapi_tags.h */
te_errno
tapi_tags_add_tag(const char *tag, const char *value)
{
    te_errno rc;

    if (tag == NULL)
        return TE_RC(TE_TAPI, TE_EINVAL);

    if (strcspn(tag, "/:") < strlen(tag))
    {
        ERROR("TRC tag name contains invalid characters");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    /*
     * The check does not guarantee that it is root prologue, but it should
     * filter out almost all misuses.
     */
    if (te_test_id != TE_TEST_ID_ROOT_PROLOGUE)
    {
        ERROR("The root prologue only may modify TRC tags: %d", te_test_id);
        return TE_RC(TE_TAPI, TE_EPERM);
    }

    if (value == NULL)
        value = "";

    rc = cfg_add_instance_fmt(NULL, CFG_VAL(STRING, value), TE_CFG_TRC_TAGS_FMT,
                              tag);
    if (rc != 0)
    {
        ERROR("%s(): cfg_add_instance_fmt(" TE_CFG_TRC_TAGS_FMT ") failed: %r",
              __FUNCTION__, tag, rc);
    }
    return rc;
}
