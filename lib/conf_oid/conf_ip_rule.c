/** @file
 * @brief ip rule configuratior objects processing
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Oleg Sadakov <Oleg.Sadakov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include "conf_ip_rule.h"

#include <arpa/inet.h>
#include <net/if.h>
#include <stddef.h>

/**
 * Initialization of @b te_conf_obj element according to the struct
 * @b te_conf_ip_rule
 *
 * @param _type         Type name for @p _field
 * @param _field        Field name
 * @param _flag_value   Flag name for specified @p _field
 * @param _opts_value   User options
 */
#define ITEM_WITH_OPTS(_type, _field, _flag_value, _opts_value)     \
    TE_CONF_OBJ_INIT(                                               \
        te_conf_ip_rule,                                            \
        &TE_CONCAT(te_conf_obj_methods_, _type),                    \
        _field,                                                     \
        TE_IP_RULE_FLAG(_flag_value),                               \
        _opts_value)

/**
 * Call @b ITEM_WITH_OPTS with @p _options equal to @c NULL
 *
 * @param _type         Type name for @p _field
 * @param _field         Field name
 * @param _flag_value    Flag name for specified @p _field
 */
#define ITEM(_type, _field, _flag_value) \
    ITEM_WITH_OPTS(_type, _field, _flag_value, NULL)

/** Maximum length of a field @b iifname and @b oifname */
static const size_t ifnamesiz = IFNAMSIZ;

/** Definition of the structure @b te_conf_ip_rule */
static const te_conf_obj ip_rule[] = {
    ITEM(uint32_t, priority,   PRIORITY),
    ITEM(uint8_t,  family,     FAMILY  ),
    ITEM(te_bool,  invert,     INVERT  ),
    ITEM(uint32_t, type,       TYPE    ),
    ITEM(uint8_t,  srclen,     SRCLEN  ),
    ITEM(uint8_t,  dstlen,     DSTLEN  ),
    ITEM(sockaddr, src,        SRC     ),
    ITEM(sockaddr, dst,        DST     ),
    ITEM(uint32_t, goto_index, GOTO    ),
    ITEM(uint8_t,  tos,        TOS     ),
    ITEM(uint32_t, fwmark,     FWMARK  ),
    ITEM(uint32_t, fwmask,     FWMASK  ),
    ITEM(uint32_t, table,      TABLE   ),
    ITEM(uint32_t, flow,       FLOW    ),

    ITEM_WITH_OPTS(str, iifname, IIFNAME, &ifnamesiz),
    ITEM_WITH_OPTS(str, oifname, OIFNAME, &ifnamesiz),
};

/* See the description in conf_ip_rule.h */
void te_conf_ip_rule_init(te_conf_ip_rule *rule)
{
    memset(rule, 0, sizeof(*rule));

    rule->family = AF_INET;
    rule->mask   = TE_IP_RULE_FLAG_FAMILY;
}

/* See the description in conf_ip_rule.h */
te_errno
te_conf_ip_rule_to_str(const te_conf_ip_rule *rule, char **str)
{
    return te_conf_obj_to_str(ip_rule, TE_ARRAY_LEN(ip_rule),
                              rule, rule->mask, str);
}

/* See the description in conf_ip_rule.h */
te_errno
te_conf_ip_rule_from_str(const char *str, uint32_t *required,
                         te_conf_ip_rule *rule)
{
    te_conf_ip_rule_init(rule);
    return te_conf_obj_from_str(ip_rule, TE_ARRAY_LEN(ip_rule),
                                str, required, rule, &rule->mask);
}

/* See the description in conf_ip_rule.h */
te_conf_obj_compare_result
te_conf_ip_rule_compare(uint32_t required, const te_conf_ip_rule *object_a,
                        const te_conf_ip_rule *object_b)
{
    return te_conf_obj_compare(ip_rule, TE_ARRAY_LEN(ip_rule), required,
                               object_a, object_a->mask,
                               object_b, object_b->mask);
}

/* See the description in conf_ip_rule.h */
void
te_conf_ip_rule_set_invert(te_conf_ip_rule *rule,
                           te_bool invert)
{
    rule->invert = invert;
    if (invert)
        rule->mask |= TE_IP_RULE_FLAG_INVERT;
    else
        rule->mask &= ~TE_IP_RULE_FLAG_INVERT;
}

/* See the description in conf_ip_rule.h */
te_bool
te_conf_ip_rule_get_invert(const te_conf_ip_rule *rule)
{
    return !!(rule->mask & TE_IP_RULE_FLAG_INVERT);
}
