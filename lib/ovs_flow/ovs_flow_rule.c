/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved. */
/** @file
 * @brief OVS Flow Rule Processing Library
 *
 * Definition of the flow rule descriptor structure and the associated
 * operations.
 */

#define TE_LGR_USER      "OVS Flow Lib"

#include "te_config.h"

#include <ctype.h>
#include <assert.h>
#include <stddef.h>

#include "te_defs.h"
#include "te_errno.h"
#include "te_str.h"
#include "logger_api.h"
#include "ovs_flow_rule.h"


typedef struct ovs_flow_field_desc ovs_flow_field_desc;

/** Flow field parsing function */
typedef te_errno (*ovs_flow_field_parse)(const char *value_start,
                                         size_t value_len,
                                         const ovs_flow_field_desc *field,
                                         ovs_flow_rule* rule);

/** Flow field formatting function */
typedef void (*ovs_flow_field_format)(const ovs_flow_field_desc *field,
                                      te_string *str,
                                      ovs_flow_rule* rule);


typedef enum ovs_flow_field_props {
    OVS_FIELD_HIDE_ZERO    = 1 << 0,
    OVS_FIELD_NOT_OPENFLOW = 1 << 1,
    OVS_FIELD_FORMAT_HEX   = 1 << 2,
} ovs_flow_field_props;

/** Flow field descriptor structure used by the main flow parsing function */
typedef struct ovs_flow_field_desc {
    const char            *name;
    size_t                 name_len;
    size_t                 offset;
    ovs_flow_field_parse   parser;
    ovs_flow_field_format  formatter;
    ovs_flow_field_props   props;
} ovs_flow_field_desc;

static te_errno
parse_ovs_u64(const char *value_start, size_t value_len,
              const ovs_flow_field_desc *field, ovs_flow_rule *rule)
{
    uint64_t *res;
    char      buf[72]; /* Enough to fit any 64bit number in base >= 2 */

    if (value_len + 1 > sizeof(buf))
        return TE_EINVAL;

    memcpy(buf, value_start, value_len);
    buf[value_len] = '\0';

    res = (uint64_t *)((char *)rule + field->offset);

    return te_str_to_uint64(buf, 0, res);
}

static void
format_ovs_u64(const ovs_flow_field_desc *field, te_string *str,
               ovs_flow_rule *rule)
{
    uint64_t *value = (uint64_t *)((char *)rule + field->offset);

    if (((field->props & OVS_FIELD_HIDE_ZERO) != 0) && *value == 0)
        return;

    if ((field->props & OVS_FIELD_FORMAT_HEX) != 0)
        te_string_append(str, "%s=0x%"PRIx64, field->name, *value);
    else
        te_string_append(str, "%s=%"PRIu64, field->name, *value);
}

#define OVS_FIELD_DESC(_name, _type, _props) \
    { #_name, sizeof (#_name) - 1, offsetof(ovs_flow_rule, _name), \
      parse_ovs_##_type, format_ovs_##_type, _props }

/* All flow rule fields that need to be parsed and formatted */
static ovs_flow_field_desc field_desc[] = {
    OVS_FIELD_DESC(cookie, u64, OVS_FIELD_HIDE_ZERO | OVS_FIELD_FORMAT_HEX),
    OVS_FIELD_DESC(table, u64, OVS_FIELD_HIDE_ZERO),
    OVS_FIELD_DESC(n_packets, u64, OVS_FIELD_NOT_OPENFLOW),
    OVS_FIELD_DESC(n_bytes, u64, OVS_FIELD_NOT_OPENFLOW),
    OVS_FIELD_DESC(n_offload_packets, u64, OVS_FIELD_NOT_OPENFLOW),
    OVS_FIELD_DESC(n_offload_bytes, u64, OVS_FIELD_NOT_OPENFLOW),
};
static const unsigned int field_desc_num = sizeof(field_desc) / sizeof(field_desc[0]);
#undef OVS_FIELD_DESC

static void
trim_spaces(const char **start, const char **end)
{
    while (*start < *end && isspace(**start))
        *start += 1;

    while (*start < *end && isspace(*(*end - 1)))
        *end -= 1;
}


static void
ovs_flow_rule_str_comma_append(te_string *rule, const char *field)
{
    if (field == NULL || *field == '\0')
        return;

    if (rule->len > 0)
        te_string_append(rule, ",%s", field);
    else
        te_string_append(rule, "%s", field);
}

te_errno
ovs_flow_rule_parse(const char *rule_str, ovs_flow_rule *rule)
{
    te_errno      rc;
    te_string     body = TE_STRING_INIT;
    const char   *next_name_start;
    const char   *name_start;
    const char   *name_end;
    const char   *value_start;
    const char   *value_end;
    unsigned int  i;

    memset(rule, 0, sizeof(*rule));
    name_start = rule_str;

    do {
        name_end = strpbrk(name_start, ",=");
        if (name_end == NULL)
        {
            /* Field name is the end of the string */
            ERROR("Name: %s (%p)", name_start, name_start);
            ERROR("Can't parse OvS flow rule without actions");
            te_string_free(&body);
            return TE_RC(TE_TAPI, TE_EINVAL);
        }

        if (*name_end == ',')
        {
            value_start = NULL;
            value_end = value_start;
            next_name_start = name_end + 1;
        }
        else /* *name_end == '=' */
        {
            value_start = name_end + 1;
        }

        trim_spaces(&name_start, &name_end);

        if (name_end - name_start == sizeof("actions") - 1 &&
            strncmp(name_start, "actions", sizeof("actions") - 1) == 0)
        {
            /* Actions started, dump the rest of the body string and return */
            ovs_flow_rule_str_comma_append(&body, name_start);
            break;
        }

        if (value_start != NULL)
        {
            value_end = strpbrk(value_start, ",");
            if (value_end == NULL)
            {
                /* Field value is the end of the string */
                ERROR("Can't parse OvS flow rule without actions");
                te_string_free(&body);
                return TE_RC(TE_TAPI, TE_EINVAL);
            }
            trim_spaces(&value_start, &value_end);
            next_name_start = value_end + 1;
        }

        /*
         * Different OvS tools use different field names for table IDs.
         * Handle "table_id" when parsing, but stick to "table" everywhere else.
         */
        if (strncmp(name_start, "table_id", name_end - name_start) == 0)
        {
            name_start = "table";
            name_end = name_start + strlen(name_start);
        }

        for (i = 0; i < field_desc_num; i++)
        {
            ovs_flow_field_desc *desc = &field_desc[i];

            if (name_end - name_start == desc->name_len &&
                strncmp(name_start, desc->name, desc->name_len) == 0)
            {
                rc = desc->parser(value_start, value_end - value_start, desc, rule);
                if (rc != 0)
                {
                    te_string_free(&body);
                    return rc;
                }
                break;
            }
        }

        if (i == field_desc_num)
        {
            te_string field_str = TE_STRING_INIT;

            te_string_append(&field_str, "%.*s", name_end - name_start,
                             name_start);

            if (value_start != NULL)
            {
                te_string_append(&field_str, "=%.*s",
                                 value_end - value_start, value_start);
            }

            ovs_flow_rule_str_comma_append(&body, field_str.ptr);

            te_string_free(&field_str);
        }

        name_start = next_name_start;
    } while (true);

    rule->body = body.ptr;
    return 0;
}

void
ovs_flow_rule_fini(ovs_flow_rule *rule)
{
    free(rule->body);
    rule->body = NULL;
}

static te_errno
ovs_flow_rule_to_string_impl(ovs_flow_rule *rule, char **rule_str,
                             bool of_only)
{
    unsigned int i;
    te_string    str = TE_STRING_INIT;

    if (rule_str == NULL)
        return TE_EINVAL;

    for (i = 0; i < field_desc_num; i++)
    {
        ovs_flow_field_desc *desc = &field_desc[i];
        te_string            field_str = TE_STRING_INIT;

        if (of_only && ((desc->props & OVS_FIELD_NOT_OPENFLOW) != 0))
            continue;

        desc->formatter(desc, &field_str, rule);

        ovs_flow_rule_str_comma_append(&str, field_str.ptr);
        te_string_free(&field_str);
    }

    ovs_flow_rule_str_comma_append(&str, rule->body);

    *rule_str = str.ptr;
    return 0;
}

te_errno
ovs_flow_rule_to_string(ovs_flow_rule *rule, char **rule_str)
{
    return ovs_flow_rule_to_string_impl(rule, rule_str, false);
}

te_errno
ovs_flow_rule_to_ofctl(ovs_flow_rule *rule, char **rule_str)
{
    return ovs_flow_rule_to_string_impl(rule, rule_str, true);
}
