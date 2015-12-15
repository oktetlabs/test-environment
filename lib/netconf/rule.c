/** @file
 * @brief Routing policy database management in netconf library
 *
 *
 * Copyright (C) 2015 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Oleg Sadakov <Oleg.Sadakov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER "Netconf rule"

#include "conf_ip_rule.h"
#include "logger_api.h"
#include "netconf.h"
#include "netconf_internal.h"
#include "te_config.h"
#include "te_defs.h"

#include "package.h"

#include <linux/fib_rules.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

/**
 * Callback of rules dump
 *
 * @param [in] h        Message header
 * @param [out]list     List of info to store
 *
 * @return              Status code (0 - success)
 */
static inline te_errno
rule_list_cb_internal(struct nlmsghdr *h, netconf_list *list)
{
    const struct rtmsg *rtm = NLMSG_DATA(h);
    struct rtattr      *rta;
    int                 len;
    netconf_rule       *rule;

    if (netconf_list_extend(list, NETCONF_NODE_RULE) != 0)
    {
        ERROR("Failed to dump rule, not enough memory");
        return TE_ENOMEM;
    }

    rule = &(list->tail->data.rule);

/**
 * Set a value field in @b rule and set a flag in field @b mask
 *
 * @param _field  Field name
 * @param _value  Value for @p _field
 * @param _flag   Flag for field @b mask
 */
#define SET_FIELD(_field, _value, _flag)        \
    do {                                        \
        if ((_value) == 0) break;               \
        rule->_field = (_value);                \
        rule->mask |= TE_IP_RULE_FLAG(_flag);   \
    } while (0)

    SET_FIELD(family, rtm->rtm_family,  FAMILY);
    SET_FIELD(dstlen, rtm->rtm_dst_len, DSTLEN);
    SET_FIELD(srclen, rtm->rtm_src_len, SRCLEN);
    SET_FIELD(tos,    rtm->rtm_tos,     TOS);
    SET_FIELD(table,  rtm->rtm_table,   TABLE);
    SET_FIELD(type,   rtm->rtm_type,    TYPE);

#undef SET_FIELD

    rta = (struct rtattr *)((char *)h + NLMSG_SPACE(sizeof(struct rtmsg)));
    len = h->nlmsg_len - NLMSG_SPACE(sizeof(struct rtmsg));

/**
 * Case statement to set a @b sockaddr field in @b rule and set a flag in
 * field @b mask
 *
 * @param _type     Value for the case statement
 * @param _field    Field name
 * @param _flag     Flag for field @b mask
 *
 * @retval TE_EINVAL    When a data size could not be determined by
 *                      address type
 */
#define CASE_SOCKADDR(_type, _field, _flag)                           \
    case _type:                                                       \
        if (RTA_PAYLOAD(rta) == sizeof(struct in_addr))               \
        {                                                             \
            memcpy(&SIN(&rule->_field)->sin_addr, RTA_DATA(rta),      \
                RTA_PAYLOAD(rta));                                    \
            SA(&rule->_field)->sa_family = AF_INET;                   \
        }                                                             \
        else if (RTA_PAYLOAD(rta) == sizeof(struct in6_addr))         \
        {                                                             \
            memcpy(&SIN6(&rule->_field)->sin6_addr, RTA_DATA(rta),    \
                RTA_PAYLOAD(rta));                                    \
            SA(&rule->_field)->sa_family = AF_INET6;                  \
        }                                                             \
        else                                                          \
        {                                                             \
            ERROR("Failed to read field (" #_field "), "              \
                  "unknown length (%d)",                              \
                  RTA_PAYLOAD(rta));                                  \
            return TE_EINVAL;                                         \
        }                                                             \
        rule->mask |= TE_IP_RULE_FLAG(_flag);                         \
        break

/**
 * Case statement to set a string field in @b rule and set a flag in
 * field @b mask
 *
 * @param _type     Value for the case statement
 * @param _field    Field name
 * @param _flag     Flag for field @b mask
 *
 * @retval TE_ENOMEM  When @p _field size is less than necessary
 */
#define CASE_STRING(_type, _field, _flag)                             \
    case _type:                                                       \
        if (RTA_PAYLOAD(rta) > sizeof(rule->_field))                  \
        {                                                             \
            ERROR("Failed to read field (" #_field  "), "             \
                  "not enough length (%d < %d)",                      \
                  sizeof(rule->_field), RTA_PAYLOAD(rta));            \
            return TE_ENOMEM;                                         \
        }                                                             \
        memcpy(&rule->_field, RTA_DATA(rta), RTA_PAYLOAD(rta));       \
        rule->mask |= TE_IP_RULE_FLAG(_flag);                         \
        break

/**
 * Case statement to set a value field with type @b uint32_t in @b rule
 * and set a flag in field @b mask
 *
 * @param _type     Value for the case statement
 * @param _field    Field name
 * @param _flag     Flag for field @b mask
 *
 * @retval TE_EINVAL  When a data size is not equal to a field size
 */
#define CASE_UINT32(_type, _field, _flag)                             \
    case _type:                                                       \
        if (RTA_PAYLOAD(rta) != sizeof(rule->_field))                 \
        {                                                             \
            ERROR("Failed to read field (" #_field  "), "             \
                  "incorrect length (%d != %d)",                      \
                  sizeof(rule->_field), RTA_PAYLOAD(rta));            \
            return TE_EINVAL;                                         \
        }                                                             \
        memcpy(&rule->_field, RTA_DATA(rta), RTA_PAYLOAD(rta));       \
        rule->mask |= TE_IP_RULE_FLAG(_flag);                         \
        break

    while (RTA_OK(rta, len))
    {
        switch (rta->rta_type)
        {
            CASE_SOCKADDR(FRA_DST,      dst,        DST);
            CASE_SOCKADDR(FRA_SRC,      src,        SRC);
/*
 * The name @b FRA_IFNAME in old kernel headers (for example 2.6.32) is
 * the member of enum, but in new headers the name is defined as
 * "define" variable.
 */
#ifdef FRA_IFNAME
            CASE_STRING  (FRA_IIFNAME,  iifname,    IIFNAME);
            CASE_STRING  (FRA_OIFNAME,  oifname,    OIFNAME);
#else
            CASE_STRING  (FRA_IFNAME,   iifname,    IIFNAME);
#endif
            CASE_UINT32  (FRA_GOTO,     goto_index, GOTO);
            CASE_UINT32  (FRA_PRIORITY, priority,   PRIORITY);
            CASE_UINT32  (FRA_FWMARK,   fwmark,     FWMARK);
            CASE_UINT32  (FRA_FWMASK,   fwmask,     FWMASK);
            CASE_UINT32  (FRA_FLOW,     flow,       FLOW);
            CASE_UINT32  (FRA_TABLE,    table,      TABLE);

            default:
                break; // Silently skip unknown field
        }

        rta = RTA_NEXT(rta, len);
    }

#undef CASE_SOCKADDR
#undef CASE_STRING
#undef CASE_UINT32

    return 0;
}

/**
 * Callback of rules dump
 *
 * @param [in] h        Message header
 * @param [out]list     List of info to store
 *
 * @return @c 0 on success, @c -1 on error (check @b errno for details).
 */
static int
rule_list_cb(struct nlmsghdr *h, netconf_list *list)
{
    te_errno result = rule_list_cb_internal(h, list);

    if (result == 0)
        return 0;

    errno = result;
    return -1;
}

/**
 * Create a new rule or delete an existing rule
 *
 * @param [in] nh       Netconf session handle
 * @param [in] cmd      Action to do
 * @param [in] rule     Rule to modify
 * @param [out]req      Request for send to netconf
 *
 * @return              Status code (@c 0 - success)
 */
static te_errno
rule_modify(netconf_handle nh, netconf_cmd cmd, const netconf_rule *rule,
            char *req)
{
    struct nlmsghdr    *h;
    struct rtmsg       *rtm;
    unsigned int        addrlen;

    /* Check required fields */
    if (rule->family != AF_INET && rule->family != AF_INET6)
    {
        ERROR("Failed to modify rule, undefined family value (%d)",
              rule->family);
        return TE_EINVAL;
    }

    if ((rule->mask & TE_IP_RULE_FLAG_DST) != 0 &&
        rule->family != rule->dst.sa_family)
    {
        ERROR("Failed to modify rule, "
              "incorrect family value in dst field (%d != %d)",
              rule->family, rule->dst.sa_family);
        return TE_EINVAL;
    }

    if ((rule->mask & TE_IP_RULE_FLAG_SRC) != 0 &&
        rule->family != rule->src.sa_family)
    {
        ERROR("Failed to modify rule, "
              "incorrect family value in src field (%d != %d)",
              rule->family, rule->src.sa_family);
        return TE_EINVAL;
    }

    switch (rule->family)
    {
        case AF_INET:   addrlen = sizeof(struct in_addr);   break;
        case AF_INET6:  addrlen = sizeof(struct in6_addr);  break;

        default:
            return TE_EINVAL;
    }

    /* Generate request */
    h = (struct nlmsghdr *)req;
    h->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    h->nlmsg_type = (cmd == NETCONF_CMD_DEL) ? RTM_DELRULE : RTM_NEWRULE;
    h->nlmsg_flags = netconf_cmd_to_flags(cmd);

    if (h->nlmsg_flags == 0)
        return TE_EINVAL;

    h->nlmsg_seq = ++nh->seq;

    /* Set parameters or some defaults if unspecified */
    rtm = NLMSG_DATA(h);

    rtm->rtm_family = rule->family;
    rtm->rtm_tos = rule->tos;

    if ((rule->mask & TE_IP_RULE_FLAG_DSTLEN) != 0)
        rtm->rtm_dst_len = rule->dstlen;
    else
        rtm->rtm_dst_len =
            ((rule->mask & TE_IP_RULE_FLAG_DST) == 0) ? 0 : 8*addrlen;

    if ((rule->mask & TE_IP_RULE_FLAG_SRCLEN) != 0)
        rtm->rtm_src_len = rule->srclen;
    else
        rtm->rtm_src_len =
            ((rule->mask & TE_IP_RULE_FLAG_SRC) == 0) ? 0 : 8*addrlen;

    if (((rule->mask & TE_IP_RULE_FLAG_TABLE) != 0) &&
        (rule->table != TE_IP_TABLE_ID_UNSPEC))
        rtm->rtm_table = rule->table;
    else
        rtm->rtm_table = TE_IP_TABLE_ID_MAIN;

    if (((rule->mask & TE_IP_RULE_FLAG_TYPE) == 0) ||
        (rule->type == TE_IP_TYPE_UNSPEC && cmd != NETCONF_CMD_DEL))
        rtm->rtm_type = RTN_UNICAST;
    else
        rtm->rtm_type = rule->type;

/**
 * Append an attribute to existing @b nlmsg if it is @b mask
 *
 * @param _type  Type of attribute
 * @param _data  Attribute data
 * @param _size  Size of @p _data
 * @param _flag  Flag for field @b mask
 */
#define NETCONF_APPEND_RTA(_type, _data, _size, _flag)                \
    do {                                                              \
        if ((rule->mask & TE_IP_RULE_FLAG(_flag)) != 0)               \
            netconf_append_rta(h, (_data), (_size), (_type));         \
    } while (0)

/**
 * Append an attribute to existing @b nlmsg if it is @b mask.
 * Size of @p _field is calculated by function @b strlen()
 *
 * @param _type     Type of attribute
 * @param _field    Field name
 * @param _flag     Flag for field @b mask
 */
#define NETCONF_APPEND_RTA_STR(_type, _field, _flag) \
    NETCONF_APPEND_RTA(_type, &rule->_field, strlen(rule->_field), _flag)

/**
 * Append an attribute to existing @b nlmsg if it is @b mask
 * Size of @p _field is calculated by function @b sizeof()
 *
 * @param _type     Type of attribute
 * @param _field    Field name
 * @param _flag     Flag for field @b mask
 */
#define NETCONF_APPEND_RTA_FIELD(_type, _field, _flag) \
    NETCONF_APPEND_RTA(_type, &rule->_field, sizeof(rule->_field), _flag)

/**
 * Append an attribute to existing @b nlmsg if it is @b mask
 * Size of @p _field is calculated by field @b sa_family
 *
 * @param _type     Type of attribute
 * @param _field    Field name
 * @param _flag     Flag for field @b mask
 */
#define NETCONF_APPEND_RTA_SOCKADDR(_type, _field, _flag)             \
    do {                                                              \
        const struct sockaddr *sa = CONST_SA(&rule->_field);          \
        switch (sa->sa_family)                                        \
        {                                                             \
            case AF_INET:                                             \
                NETCONF_APPEND_RTA(_type, &CONST_SIN(sa)->sin_addr,   \
                                   sizeof(struct in_addr), _flag);    \
                break;                                                \
                                                                      \
            case AF_INET6:                                            \
                NETCONF_APPEND_RTA(_type, &CONST_SIN6(sa)->sin6_addr, \
                                   sizeof(struct in6_addr), _flag);   \
                break;                                                \
        }                                                             \
    } while (0)

    NETCONF_APPEND_RTA_SOCKADDR(FRA_DST,      dst,        DST);
    NETCONF_APPEND_RTA_SOCKADDR(FRA_SRC,      src,        SRC);
/*
 * The name @b FRA_IFNAME in old kernel headers (for example 2.6.32) is
 * the member of enum, but in new headers the name is defined as
 * "define" variable.
 */
#ifdef FRA_IFNAME
    NETCONF_APPEND_RTA_STR     (FRA_IIFNAME,  iifname,    IIFNAME);
    NETCONF_APPEND_RTA_STR     (FRA_OIFNAME,  oifname,    OIFNAME);
#else
    NETCONF_APPEND_RTA_STR     (FRA_IFNAME,   iifname,    IIFNAME);
#endif
    NETCONF_APPEND_RTA_FIELD   (FRA_GOTO,     goto_index, GOTO);
    NETCONF_APPEND_RTA_FIELD   (FRA_PRIORITY, priority,   PRIORITY);
    NETCONF_APPEND_RTA_FIELD   (FRA_FWMARK,   fwmark,     FWMARK);
    NETCONF_APPEND_RTA_FIELD   (FRA_FWMASK,   fwmask,     FWMASK);
    NETCONF_APPEND_RTA_FIELD   (FRA_FLOW,     flow,       FLOW);
    NETCONF_APPEND_RTA_FIELD   (FRA_TABLE,    table,      TABLE);

#undef NETCONF_APPEND_RTA
#undef NETCONF_APPEND_RTA_STR
#undef NETCONF_APPEND_RTA_FIELD
#undef NETCONF_APPEND_RTA_SOCKADDR

    return 0;
}


/* See the description in netconf.h */
void
netconf_rule_init(netconf_rule *rule)
{
    te_conf_ip_rule_init(rule);
}

/* See the description in netconf.h */
void
netconf_rule_node_free(netconf_node *node)
{
    NETCONF_ASSERT(node != NULL);

    free(node);
}

/* See the description in netconf.h */
netconf_list *
netconf_rule_dump(netconf_handle nh, unsigned char family)
{
    return netconf_dump_request(nh, RTM_GETRULE, family, rule_list_cb);
}

/* See the description in netconf.h */
int
netconf_rule_modify(netconf_handle nh, netconf_cmd cmd,
                    const netconf_rule *rule)
{
    char req[NETCONF_MAX_REQ_LEN] = {0};

    errno = rule_modify(nh, cmd, rule, req);
    if (errno == 0)
        errno = netconf_talk(nh, req, sizeof(req), NULL, NULL);

    return (errno == 0) ? 0 : -1;
}
