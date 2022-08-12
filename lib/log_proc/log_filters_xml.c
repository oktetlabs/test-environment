/** @file
 * @brief Log processing
 *
 * XML parsing for log filters.
 *
 * Copyright (C) 2020-2022 OKTET Labs. All rights reserved.
 *
 */

#define TE_LGR_USER "Log processing"

#include "te_config.h"

#include <libxml/parser.h>

#include "log_filters_xml.h"
#include "logger_api.h"
#include "te_str.h"

/* Extract an XML attribute, ignore empty strings */
static xmlChar *
get_xml_prop(xmlNodePtr node, const char *name)
{
    xmlChar *val;

    val = xmlGetProp(node, (const xmlChar *)name);
    if (val != NULL && xmlStrcmp(val, (const xmlChar *)"") == 0)
    {
        xmlFree(val);
        return NULL;
    }

    return val;
}

/* See description in load_xml.h */
te_errno
log_branch_filter_load_xml(log_branch_filter *filter, xmlNodePtr filter_node)
{
    te_errno  rc;
    te_bool   include;
    xmlChar  *path;

    if (filter_node == NULL)
        return 0;

    filter_node = filter_node->xmlChildrenNode;
    while (filter_node != NULL)
    {
        if (xmlStrcmp(filter_node->name, (const xmlChar *)"include") != 0 &&
            xmlStrcmp(filter_node->name, (const xmlChar *)"exclude") != 0)
        {
            filter_node = filter_node->next;
            continue;
        }

        include = xmlStrcmp(filter_node->name, (const xmlChar *)"include") == 0;

        path = get_xml_prop(filter_node, "path");

        rc = log_branch_filter_add(filter, (const char *)path, include);

        xmlFree(path);

        if (rc != 0)
            return rc;

        filter_node = filter_node->next;
    }

    return 0;
}

/* See description in load_xml.h */
te_errno
log_duration_filter_load_xml(log_duration_filter *filter, xmlNodePtr filter_node)
{
    te_bool   include;
    xmlChar  *node_str, *min_str, *max_str;
    uint32_t  min, max;
    te_errno  rc;

    if (filter_node == NULL)
        return 0;

    filter_node = filter_node->xmlChildrenNode;
    while (filter_node != NULL)
    {
        if (xmlStrcmp(filter_node->name, (const xmlChar *)"include") != 0 &&
            xmlStrcmp(filter_node->name, (const xmlChar *)"exclude") != 0)
        {
            filter_node = filter_node->next;
            continue;
        }

        include = xmlStrcmp(filter_node->name, (const xmlChar *)"include") == 0;

        node_str = get_xml_prop(filter_node, "node");
        min_str  = get_xml_prop(filter_node, "min");
        max_str  = get_xml_prop(filter_node, "max");

        min = 0;
        max = UINT32_MAX;

        if (min_str != NULL)
        {
            rc = te_strtoui((const char *)min_str, 10, &min);
            if (rc != 0)
            {
                ERROR("Invalid value of 'min' in duration filter: %r", rc);
                rc = TE_EINVAL;
            }
        }

        if (rc == 0 && max_str != NULL)
        {
            rc = te_strtoui((const char *)max_str, 10, &max);
            if (rc != 0)
            {
                ERROR("Invalid value of 'max' in duration filter: %r", rc);
                rc = TE_EINVAL;
            }
        }

        if (rc == 0)
        {
            if (min >= max)
                ERROR("'min' value should be less than 'max' value");
            else
                rc = log_duration_filter_add(filter, (const char *)node_str,
                                             min, max, include);
        }

        xmlFree(node_str);
        xmlFree(min_str);
        xmlFree(max_str);

        if (rc != 0)
            return rc;

        filter_node = filter_node->next;
    }

    return 0;
}

/**
 * Returns log level bit mask based on string value of it.
 *
 * @param level_str  Log level in string representation
 *
 * @return Log level as a bit mask.
 */
static te_log_level
get_level_mask(const char *level_str)
{
    const char   *end_ptr;
    const char   *ptr = level_str;
    char         *tmp;
    uint32_t      len;
    te_log_level  val = 0;
    te_log_level  prev_val;

    if (level_str == NULL || strlen(level_str) == 0)
        return 0xffff;

    end_ptr = level_str + strlen(level_str);

    while (ptr != NULL)
    {
        tmp = strchr(ptr, ',');
        if (tmp == NULL)
            len = end_ptr - ptr;
        else
            len = tmp - ptr;

#define CHECK_SET_BIT(level_) \
        do {                                       \
            if (prev_val == val &&                 \
                strncmp(ptr, #level_, len) == 0 && \
                strlen(#level_) == len)            \
            {                                      \
                if ((ptr = tmp) != NULL)           \
                    ptr++;                         \
                                                   \
                val |= TE_LL_ ## level_;           \
            }                                      \
        } while (0)

        prev_val = val;

        CHECK_SET_BIT(ERROR);
        CHECK_SET_BIT(WARN);
        CHECK_SET_BIT(RING);
        CHECK_SET_BIT(INFO);
        CHECK_SET_BIT(VERB);
        CHECK_SET_BIT(ENTRY_EXIT);
        CHECK_SET_BIT(PACKET);
        CHECK_SET_BIT(MI);
        CHECK_SET_BIT(CONTROL);

#undef CHECK_SET_BIT

        if (prev_val == val)
        {
            char buf[len + 1];

            memcpy(buf, ptr, len);
            buf[len] = '\0';

            ERROR("Unrecognized log level '%s' found", buf);
        }
    }

    return val;
}

/* Extract level bitmask from an XML node */
static te_log_level
parse_level_mask(xmlNodePtr node)
{
    xmlChar      *level;
    te_log_level  mask;

    level = get_xml_prop(node, "level");

    if (level != NULL)
    {
        mask = get_level_mask((const char *)level);
        xmlFree(level);
    }
    else
    {
        mask = 0xffff;
    }

    return mask;
}

/* See description in load_xml.h */
te_errno
log_msg_filter_load_xml(log_msg_filter *filter, xmlNodePtr filter_node)
{
    te_bool       regex;
    xmlChar      *match_prop;
    te_errno      rc;

    if (filter_node == NULL)
        /* Empty file, nothing to do */
        return 0;

    regex = FALSE;
    match_prop = xmlGetProp(filter_node, (const xmlChar *)"match");
    if (match_prop)
    {
        if (xmlStrcmp(match_prop, (const xmlChar *)"regexp") == 0)
            regex = TRUE;

        xmlFree(match_prop);
    }

    filter_node = filter_node->xmlChildrenNode;
    while (filter_node != NULL)
    {
        te_bool       include;
        xmlChar      *entity = NULL;
        te_log_level  level = 0xffff;
        xmlNodePtr    user;

        if (xmlStrcmp(filter_node->name, (const xmlChar *)"include") != 0 &&
            xmlStrcmp(filter_node->name, (const xmlChar *)"exclude") != 0)
        {
           filter_node = filter_node->next;
           continue;
        }

        include = xmlStrcmp(filter_node->name, (const xmlChar *)"include") == 0;

        entity = get_xml_prop(filter_node, "entity");

        level = parse_level_mask(filter_node);

        user = filter_node->xmlChildrenNode;

        if (entity == NULL && user == NULL)
        {
            rc = log_msg_filter_set_default(filter, include, level);
            if (rc != 0)
                return rc;
        }
        else
        {
            if (user == NULL)
            {
                rc = log_msg_filter_add_entity(filter, include,
                                               (const char *)entity, regex,
                                               level);
                if (rc != 0)
                    return rc;
            }
            else
            {
                while (user != NULL)
                {
                    te_log_level  user_level;
                    xmlChar      *name;

                    if (xmlStrcmp(user->name, (const xmlChar *)"user") != 0)
                    {
                        user = user->next;
                        continue;
                    }

                    user_level = parse_level_mask(user);
                    if (user_level == 0xffff)
                        user_level = level;

                    name = xmlGetProp(user, (const xmlChar *)"name");
                    if (name == NULL || xmlStrcmp(name, (const xmlChar *)"") == 0)
                        return TE_EINVAL;

                    rc = log_msg_filter_add_user(filter, include,
                                                 (const char *)entity, regex,
                                                 (const char *)name, regex,
                                                 user_level);
                    if (rc != 0)
                        return rc;

                    user = user->next;
                }
            }
        }

        filter_node = filter_node->next;
    }

    return 0;
}
