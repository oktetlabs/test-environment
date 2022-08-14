/** @file
 * @brief Log processing
 *
 * YAML parsing for log filters.
 *
 * Copyright (C) 2020-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER "Log processing"

#include "te_config.h"
#include "logger_api.h"
#include "log_filters_yaml.h"
#include "te_yaml.h"

#include <stddef.h>

/** Log level to bitmask mapping */
typedef struct log_level {
    const char   *name;
    te_log_level  mask;
} log_level;

/** Returns log level bitmask based on its string representation. */
static te_log_level
get_level_mask(const char *level_str)
{
    static const log_level levels[] = {
        {TE_LL_ERROR_STR,      TE_LL_ERROR},
        {TE_LL_WARN_STR,       TE_LL_WARN},
        {TE_LL_RING_STR,       TE_LL_RING},
        {TE_LL_INFO_STR,       TE_LL_INFO},
        {TE_LL_VERB_STR,       TE_LL_VERB},
        {TE_LL_ENTRY_EXIT_STR, TE_LL_ENTRY_EXIT},
        {TE_LL_PACKET_STR,     TE_LL_PACKET},
        {TE_LL_MI_STR,         TE_LL_MI},
        {TE_LL_CONTROL_STR,    TE_LL_CONTROL},
    };
    static const size_t    levels_num = sizeof(levels) / sizeof(levels[0]);

    size_t        i;
    const char   *end_ptr;
    const char   *ptr = level_str;
    const char   *tmp;
    ptrdiff_t     len;
    te_log_level  val = 0;

    if (level_str == NULL || strlen(level_str) == 0)
        return 0xffff;

    end_ptr = level_str + strlen(level_str);
    while (ptr < end_ptr)
    {
        tmp = strchr(ptr, ',');
        if (tmp == NULL)
            tmp = end_ptr;
        len = tmp - ptr;

        for (i = 0; i < levels_num; i++)
        {
            if ((size_t)len == strlen(levels[i].name) &&
                strncmp(ptr, levels[i].name, len) == 0)
            {
                val |= levels[i].mask;
                break;
            }
        }
        if (i == levels_num)
            WARN("Unrecognized log level '%.*s' found\n", len, ptr);

        ptr = tmp + 1;
    }

    return val;
}

/* See description in load_yaml.h */
te_errno
log_msg_filter_load_yaml(log_msg_filter *filter, yaml_document_t *doc,
                         yaml_node_t *node)
{
    yaml_node_item_t *i;

    if (node->type != YAML_SEQUENCE_NODE)
    {
        ERROR("%s: Filter root must be a sequence", __FUNCTION__);
        return TE_EINVAL;
    }

    for (i = node->data.sequence.items.start;
         i < node->data.sequence.items.top; i++)
    {
        te_errno          rc;
        yaml_node_t      *item;
        yaml_node_pair_t *pair;
        yaml_node_t      *include = NULL;
        yaml_node_t      *exclude = NULL;
        const char       *entity  = NULL;
        const char       *user    = NULL;
        te_log_level      level   = 0xffff;

        item = yaml_document_get_node(doc, *i);
        if (item->type != YAML_MAPPING_NODE)
        {
            ERROR("%s: Filter command must be a mapping", __FUNCTION__);
            return TE_EINVAL;
        }

        for (pair = item->data.mapping.pairs.start;
             pair < item->data.mapping.pairs.top; pair++)
        {
            yaml_node_t *k = yaml_document_get_node(doc, pair->key);
            yaml_node_t *v = yaml_document_get_node(doc, pair->value);
            const char  *key = te_yaml_scalar_value(k);

            if (key == NULL)
            {
                ERROR("%s: Non-scalar key in mapping", __FUNCTION__);
                return TE_EINVAL;
            }

            if (strcmp(key, "include") == 0)
                include = k;
            else if (strcmp(key, "exclude") == 0)
                exclude = k;
            else if (strcmp(key, "entity") == 0)
                entity = te_yaml_scalar_value(v);
            else if (strcmp(key, "user") == 0)
                user = te_yaml_scalar_value(v);
            else if (strcmp(key, "level") == 0)
                level = get_level_mask(te_yaml_scalar_value(v));
        }

        if (include == NULL && exclude == NULL)
        {
            ERROR("%s: Missing include/exclude directive", __FUNCTION__);
            return TE_EINVAL;
        }

        if (include != NULL && exclude != NULL)
        {
            ERROR("%s: Cannot have both include and exclude in the same rule",
                  __FUNCTION__);
            return TE_EINVAL;
        }

        if (entity == NULL && user == NULL)
        {
            rc = log_msg_filter_set_default(filter, include != NULL, level);
            if (rc != 0)
                return rc;
        }
        else if (user == NULL)
        {
            rc = log_msg_filter_add_entity(filter, include != NULL,
                                           entity, FALSE,
                                           level);
            if (rc != 0)
                return rc;
        }
        else
        {
            rc = log_msg_filter_add_user(filter, include != NULL,
                                         entity, FALSE,
                                         user, FALSE,
                                         level);
            if (rc != 0)
                return rc;
        }
    }

    return 0;
}
