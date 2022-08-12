/** @file
 * @brief Testing Results Comparator: common
 *
 * Routines to work with TRC tags sets.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#include "te_config.h"
#include "trc_config.h"

#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#include <jansson.h>

#include "te_queue.h"
#include "logger_api.h"

#include "trc_tags.h"
#include "te_trc.h"
#include "te_string.h"


/**
 * Parse string with TRC tags and add them into the list.
 *
 * @param tags_str      String with tags
 * @param tags          List to add tags
 *
 * @return Status code.
 */
te_errno
trc_tags_str_to_list(tqh_strings *tags, char *tags_str)
{
    char *tag, *space;
    int   rc = 0;

    for (tag = tags_str + strspn(tags_str, " ");
         rc == 0 && tag != NULL && *tag != '\0';
         tag = (space == NULL) ? NULL : (space + strspn(space, " ")))
    {
        space = strchr(tag, ' ');
        if (space != NULL)
            *space++ = '\0';
        rc = trc_add_tag(tags, tag);
    }
    return rc;
}

/* See the description in trc_tags.h */
te_errno
trc_tags_json_to_list(tqh_strings *parsed_tags, char *json_buf)
{
    int       ret;
    size_t    i;
    te_errno  rc = 0;
    json_t   *type = NULL;
    json_t   *version = 0;

    json_t *root = NULL;
    json_t *tags = NULL;
    json_t *tag = NULL;

    json_error_t  err;
    const char   *cur_tag = NULL;
    const char   *cur_tag_value = NULL;
    te_string     tag_combine = TE_STRING_INIT;

    root = json_loadb(json_buf, strlen(json_buf), 0, &err);
    if (root == NULL)
    {
        ERROR("Incorrect json_buf: %s", err.text);
        return TE_EINVAL;
    }

    type = json_object_get(root, "type");
    if (!json_is_string(type))
    {
        ERROR("Failed to get the \"type\" field from the trc_tags message");
        rc = TE_EFAIL;
        goto cleanup;
    }
    if (strcmp(json_string_value(type), "trc_tags") != 0)
    {
        ERROR("Wrong MI type");
        free(type);
        rc = TE_EINVAL;
        goto cleanup;
    }
    free(type);

    version = json_object_get(root, "version");
    if (!json_is_integer(version))
    {
        ERROR("Failed to get the \"version\" field from the trc_tags message");
        rc = TE_EFAIL;
        goto cleanup;
    }
    if (json_integer_value(version) != 1)
    {
        ERROR("Wrong TRC tags version");
        rc = TE_EINVAL;
        goto cleanup;
    }

    tags = json_object_get(root, "tags");
    if (!json_is_array(tags))
    {
        ERROR("Failed to get the \"tags\" field from the trc_tags message");
        rc = TE_EFAIL;
        goto cleanup;
    }

    json_array_foreach(tags, i, tag)
    {
        cur_tag_value = NULL;
        ret = json_unpack_ex(tag, &err, JSON_STRICT, "{s:s, s?s}",
                             "name", &cur_tag, "value", &cur_tag_value);
        if (ret != 0)
        {
            ERROR("Error unpacking trc_tags JSON log message: %s (line %d, "
                  "column %d)",
                  err.text, err.line, err.column);
            json_decref(tag);
            rc = TE_EFAIL;
            goto cleanup;
        }

        if (cur_tag_value != NULL)
            rc = te_string_append(&tag_combine, "%s:%s", cur_tag, cur_tag_value);
        else
            rc = te_string_append(&tag_combine, "%s", cur_tag);

        json_decref(tag);

        if (rc != 0)
        {
            ERROR("Failed to combine tag name and tag value");
            goto cleanup;
        }

        rc = trc_add_tag(parsed_tags, te_string_value(&tag_combine));
        te_string_reset(&tag_combine);
        if (rc != 0)
        {
            ERROR("Failed to add TRC tag to tags list");
            goto cleanup;
        }
    }

cleanup:
    json_decref(root);
    json_decref(tags);
    te_string_free(&tag_combine);

    return rc;
}
