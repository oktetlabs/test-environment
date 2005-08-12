/** @file
 * @brief Test API for RADIUS Server Configuration and RADIUS CSAP 
 *
 * Implementation.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Boris Misenov <Boris.Misenov@oktetlabs.ru>
 *
 * $Id$
 */

#include "te_config.h"

#include "tapi_radius.h"
#include "tapi_udp.h"
#include "conf_api.h"
#include "logger_api.h"
#include "te_errno.h"
#include "asn_usr.h"
#include "ndn.h"
#include "rcf_api.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Attribute dictionary length */
#define TAPI_RADIUS_DICT_LEN     256

/* Index for undefined entries */
#define TAPI_RADIUS_ATTR_UNKNOWN 255

#define TAPI_RADIUS_TYPE_IS_DYNAMIC(type) \
    ((type) == TAPI_RADIUS_TYPE_STRING || \
     (type) == TAPI_RADIUS_TYPE_TEXT || \
     (type) == TAPI_RADIUS_TYPE_UNKNOWN)

static uint8_t tapi_radius_dict_index[TAPI_RADIUS_DICT_LEN];

/* RADIUS attributes dictionary */
static const tapi_radius_attr_info_t tapi_radius_dict[] =
{
    { 1, "User-Name", TAPI_RADIUS_TYPE_TEXT },
    { 2, "User-Password", TAPI_RADIUS_TYPE_TEXT },
    { 3, "CHAP-Password", TAPI_RADIUS_TYPE_STRING },
    { 4, "NAS-IP-Address", TAPI_RADIUS_TYPE_ADDRESS },
    { 5, "NAS-Port", TAPI_RADIUS_TYPE_INTEGER },
    { 6, "Service-Type", TAPI_RADIUS_TYPE_INTEGER },
    { 7, "Framed-Protocol", TAPI_RADIUS_TYPE_INTEGER },
    { 8, "Framed-IP-Address", TAPI_RADIUS_TYPE_ADDRESS },
    { 9, "Framed-IP-Netmask", TAPI_RADIUS_TYPE_ADDRESS },
    { 10, "Framed-Routing", TAPI_RADIUS_TYPE_INTEGER },
    { 11, "Filter-Id", TAPI_RADIUS_TYPE_TEXT },
    { 12, "Framed-MTU", TAPI_RADIUS_TYPE_INTEGER },
    { 13, "Framed-Compression", TAPI_RADIUS_TYPE_INTEGER },
    { 14, "Login-IP-Host", TAPI_RADIUS_TYPE_ADDRESS },
    { 15, "Login-Service", TAPI_RADIUS_TYPE_INTEGER },
    { 16, "Login-TCP-Port", TAPI_RADIUS_TYPE_INTEGER },
    { 18, "Reply-Message", TAPI_RADIUS_TYPE_TEXT },
    { 19, "Callback-Number", TAPI_RADIUS_TYPE_TEXT },
    { 20, "Callback-Id", TAPI_RADIUS_TYPE_TEXT },
    { 22, "Framed-Route", TAPI_RADIUS_TYPE_TEXT },
    { 23, "Framed-IPX-Network", TAPI_RADIUS_TYPE_ADDRESS },
    { 24, "State", TAPI_RADIUS_TYPE_STRING },
    { 25, "Class", TAPI_RADIUS_TYPE_STRING },
    { 26, "Vendor-Specific", TAPI_RADIUS_TYPE_STRING },
    { 27, "Session-Timeout", TAPI_RADIUS_TYPE_INTEGER },
    { 28, "Idle-Timeout", TAPI_RADIUS_TYPE_INTEGER },
    { 29, "Termination-Action", TAPI_RADIUS_TYPE_INTEGER },
    { 30, "Called-Station-Id", TAPI_RADIUS_TYPE_TEXT },
    { 31, "Calling-Station-Id", TAPI_RADIUS_TYPE_TEXT },
    { 32, "NAS-Identifier", TAPI_RADIUS_TYPE_TEXT },
    { 33, "Proxy-State", TAPI_RADIUS_TYPE_STRING },
    { 34, "Login-LAT-Service", TAPI_RADIUS_TYPE_TEXT },
    { 35, "Login-LAT-Node", TAPI_RADIUS_TYPE_TEXT },
    { 36, "Login-LAT-Group", TAPI_RADIUS_TYPE_STRING },
    { 37, "Framed-AppleTalk-Link", TAPI_RADIUS_TYPE_INTEGER },
    { 38, "Framed-AppleTalk-Network", TAPI_RADIUS_TYPE_INTEGER },
    { 39, "Framed-AppleTalk-Zone", TAPI_RADIUS_TYPE_TEXT },
    { 40, "Acct-Status-Type", TAPI_RADIUS_TYPE_INTEGER },
    { 41, "Acct-Delay-Time", TAPI_RADIUS_TYPE_INTEGER },
    { 42, "Acct-Input-Octets", TAPI_RADIUS_TYPE_INTEGER },
    { 43, "Acct-Output-Octets", TAPI_RADIUS_TYPE_INTEGER },
    { 44, "Acct-Session-Id", TAPI_RADIUS_TYPE_TEXT },
    { 45, "Acct-Authentic", TAPI_RADIUS_TYPE_INTEGER },
    { 46, "Acct-Session-Time", TAPI_RADIUS_TYPE_INTEGER },
    { 47, "Acct-Input-Packets", TAPI_RADIUS_TYPE_INTEGER },
    { 48, "Acct-Output-Packets", TAPI_RADIUS_TYPE_INTEGER },
    { 49, "Acct-Terminate-Cause", TAPI_RADIUS_TYPE_INTEGER },
    { 50, "Acct-Multi-Session-Id", TAPI_RADIUS_TYPE_TEXT },
    { 51, "Acct-Link-Count", TAPI_RADIUS_TYPE_INTEGER },
    { 52, "Acct-Input-Gigawords", TAPI_RADIUS_TYPE_INTEGER },
    { 53, "Acct-Output-Gigawords", TAPI_RADIUS_TYPE_INTEGER },
    { 55, "Event-Timestamp", TAPI_RADIUS_TYPE_TIME },
    { 60, "CHAP-Challenge", TAPI_RADIUS_TYPE_STRING },
    { 61, "NAS-Port-Type", TAPI_RADIUS_TYPE_INTEGER },
    { 62, "Port-Limit", TAPI_RADIUS_TYPE_INTEGER },
    { 63, "Login-LAT-Port", TAPI_RADIUS_TYPE_INTEGER },
    { 68, "Acct-Tunnel-Connection", TAPI_RADIUS_TYPE_TEXT },
    { 70, "ARAP-Password", TAPI_RADIUS_TYPE_TEXT },
    { 71, "ARAP-Features", TAPI_RADIUS_TYPE_TEXT },
    { 72, "ARAP-Zone-Access", TAPI_RADIUS_TYPE_INTEGER },
    { 73, "ARAP-Security", TAPI_RADIUS_TYPE_INTEGER },
    { 74, "ARAP-Security-Data", TAPI_RADIUS_TYPE_TEXT },
    { 75, "Password-Retry", TAPI_RADIUS_TYPE_INTEGER },
    { 76, "Prompt", TAPI_RADIUS_TYPE_INTEGER },
    { 77, "Connect-Info", TAPI_RADIUS_TYPE_TEXT },
    { 78, "Configuration-Token", TAPI_RADIUS_TYPE_TEXT },
    { 79, "EAP-Message", TAPI_RADIUS_TYPE_STRING },
    { 80, "Message-Authenticator", TAPI_RADIUS_TYPE_STRING },
    { 84, "ARAP-Challenge-Response", TAPI_RADIUS_TYPE_TEXT },
    { 85, "Acct-Interim-Interval", TAPI_RADIUS_TYPE_INTEGER },
    { 87, "NAS-Port-Id", TAPI_RADIUS_TYPE_TEXT },
    { 88, "Framed-Pool", TAPI_RADIUS_TYPE_TEXT },
    { 95, "NAS-IPv6-Address", TAPI_RADIUS_TYPE_ },
    { 96, "Framed-Interface-Id", TAPI_RADIUS_TYPE_ },
    { 97, "Framed-IPv6-Prefix", TAPI_RADIUS_TYPE_STRING },
    { 98, "Login-IPv6-Host", TAPI_RADIUS_TYPE_ },
    { 99, "Framed-IPv6-Route", TAPI_RADIUS_TYPE_TEXT },
    { 100, "Framed-IPv6-Pool", TAPI_RADIUS_TYPE_TEXT },
    { 101, "Error-Cause", TAPI_RADIUS_TYPE_INTEGER },
    { 206, "Digest-Response", TAPI_RADIUS_TYPE_TEXT },
    { 207, "Digest-Attributes", TAPI_RADIUS_TYPE_STRING },
    { 500, "Fall-Through", TAPI_RADIUS_TYPE_INTEGER },
    { 502, "Exec-Program", TAPI_RADIUS_TYPE_TEXT },
    { 503, "Exec-Program-Wait", TAPI_RADIUS_TYPE_TEXT },
    { 1000, "Auth-Type", TAPI_RADIUS_TYPE_INTEGER },
    { 1001, "Menu", TAPI_RADIUS_TYPE_TEXT },
    { 1002, "Termination-Menu", TAPI_RADIUS_TYPE_TEXT },
    { 1003, "Prefix", TAPI_RADIUS_TYPE_TEXT },
    { 1004, "Suffix", TAPI_RADIUS_TYPE_TEXT },
    { 1005, "Group", TAPI_RADIUS_TYPE_TEXT },
    { 1006, "Crypt-Password", TAPI_RADIUS_TYPE_TEXT },
    { 1007, "Connect-Rate", TAPI_RADIUS_TYPE_INTEGER },
    { 1008, "Add-Prefix", TAPI_RADIUS_TYPE_TEXT },
    { 1009, "Add-Suffix", TAPI_RADIUS_TYPE_TEXT },
    { 1010, "Expiration", TAPI_RADIUS_TYPE_TIME },
    { 1011, "Autz-Type", TAPI_RADIUS_TYPE_INTEGER },
    { 1012, "Acct-Type", TAPI_RADIUS_TYPE_INTEGER },
    { 1013, "Session-Type", TAPI_RADIUS_TYPE_INTEGER },
    { 1014, "Post-Auth-Type", TAPI_RADIUS_TYPE_INTEGER },
    { 1015, "Pre-Proxy-Type", TAPI_RADIUS_TYPE_INTEGER },
    { 1016, "Post-Proxy-Type", TAPI_RADIUS_TYPE_INTEGER },
    { 1017, "Pre-Acct-Type", TAPI_RADIUS_TYPE_INTEGER },
    { 1018, "EAP-Type", TAPI_RADIUS_TYPE_INTEGER },
    { 1019, "EAP-TLS-Require-Client-Cert", TAPI_RADIUS_TYPE_INTEGER },
    { 1020, "EAP-Id", TAPI_RADIUS_TYPE_INTEGER },
    { 1021, "EAP-Code", TAPI_RADIUS_TYPE_INTEGER },
    { 1022, "EAP-MD5-Password", TAPI_RADIUS_TYPE_TEXT },
    { 1029, "User-Category", TAPI_RADIUS_TYPE_TEXT },
    { 1030, "Group-Name", TAPI_RADIUS_TYPE_TEXT },
    { 1031, "Huntgroup-Name", TAPI_RADIUS_TYPE_TEXT },
    { 1034, "Simultaneous-Use", TAPI_RADIUS_TYPE_INTEGER },
    { 1035, "Strip-User-Name", TAPI_RADIUS_TYPE_INTEGER },
    { 1040, "Hint", TAPI_RADIUS_TYPE_TEXT },
    { 1041, "Pam-Auth", TAPI_RADIUS_TYPE_TEXT },
    { 1042, "Login-Time", TAPI_RADIUS_TYPE_TEXT },
    { 1043, "Stripped-User-Name", TAPI_RADIUS_TYPE_TEXT },
    { 1044, "Current-Time", TAPI_RADIUS_TYPE_TEXT },
    { 1045, "Realm", TAPI_RADIUS_TYPE_TEXT },
    { 1046, "No-Such-Attribute", TAPI_RADIUS_TYPE_TEXT },
    { 1047, "Packet-Type", TAPI_RADIUS_TYPE_INTEGER },
    { 1048, "Proxy-To-Realm", TAPI_RADIUS_TYPE_TEXT },
    { 1049, "Replicate-To-Realm", TAPI_RADIUS_TYPE_TEXT },
    { 1050, "Acct-Session-Start-Time", TAPI_RADIUS_TYPE_TIME },
    { 1051, "Acct-Unique-Session-Id", TAPI_RADIUS_TYPE_TEXT },
    { 1052, "Client-IP-Address", TAPI_RADIUS_TYPE_ADDRESS },
    { 1053, "Ldap-UserDn", TAPI_RADIUS_TYPE_TEXT },
    { 1054, "NS-MTA-MD5-Password", TAPI_RADIUS_TYPE_TEXT },
    { 1055, "SQL-User-Name", TAPI_RADIUS_TYPE_TEXT },
    { 1057, "LM-Password", TAPI_RADIUS_TYPE_STRING },
    { 1058, "NT-Password", TAPI_RADIUS_TYPE_STRING },
    { 1059, "SMB-Account-CTRL", TAPI_RADIUS_TYPE_INTEGER },
    { 1061, "SMB-Account-CTRL-TEXT", TAPI_RADIUS_TYPE_TEXT },
    { 1062, "User-Profile", TAPI_RADIUS_TYPE_TEXT },
    { 1063, "Digest-Realm", TAPI_RADIUS_TYPE_TEXT },
    { 1064, "Digest-Nonce", TAPI_RADIUS_TYPE_TEXT },
    { 1065, "Digest-Method", TAPI_RADIUS_TYPE_TEXT },
    { 1066, "Digest-URI", TAPI_RADIUS_TYPE_TEXT },
    { 1067, "Digest-QOP", TAPI_RADIUS_TYPE_TEXT },
    { 1068, "Digest-Algorithm", TAPI_RADIUS_TYPE_TEXT },
    { 1069, "Digest-Body-Digest", TAPI_RADIUS_TYPE_TEXT },
    { 1070, "Digest-CNonce", TAPI_RADIUS_TYPE_TEXT },
    { 1071, "Digest-Nonce-Count", TAPI_RADIUS_TYPE_TEXT },
    { 1072, "Digest-User-Name", TAPI_RADIUS_TYPE_TEXT },
    { 1073, "Pool-Name", TAPI_RADIUS_TYPE_TEXT },
    { 1074, "Ldap-Group", TAPI_RADIUS_TYPE_TEXT },
    { 1075, "Module-Success-Message", TAPI_RADIUS_TYPE_TEXT },
    { 1076, "Module-Failure-Message", TAPI_RADIUS_TYPE_TEXT },
    { 1078, "Rewrite-Rule", TAPI_RADIUS_TYPE_TEXT },
    { 1079, "Sql-Group", TAPI_RADIUS_TYPE_TEXT },
    { 1080, "Response-Packet-Type", TAPI_RADIUS_TYPE_INTEGER },
    { 1081, "Packet-Dst-Port", TAPI_RADIUS_TYPE_INTEGER },
    { 1082, "MS-CHAP-Use-NTLM-Auth", TAPI_RADIUS_TYPE_INTEGER },
    { 1083, "NTLM-User-Name", TAPI_RADIUS_TYPE_TEXT },
    { 1200, "EAP-Sim-Subtype", TAPI_RADIUS_TYPE_INTEGER },
    { 1201, "EAP-Sim-Rand1", TAPI_RADIUS_TYPE_STRING },
    { 1202, "EAP-Sim-Rand2", TAPI_RADIUS_TYPE_STRING },
    { 1203, "EAP-Sim-Rand3", TAPI_RADIUS_TYPE_STRING },
    { 1204, "EAP-Sim-SRES1", TAPI_RADIUS_TYPE_STRING },
    { 1205, "EAP-Sim-SRES2", TAPI_RADIUS_TYPE_STRING },
    { 1206, "EAP-Sim-SRES3", TAPI_RADIUS_TYPE_STRING },
    { 1207, "EAP-Sim-State", TAPI_RADIUS_TYPE_INTEGER },
    { 1208, "EAP-Sim-IMSI", TAPI_RADIUS_TYPE_TEXT },
    { 1209, "EAP-Sim-HMAC", TAPI_RADIUS_TYPE_TEXT },
    { 1210, "EAP-Sim-KEY", TAPI_RADIUS_TYPE_STRING },
    { 1211, "EAP-Sim-EXTRA", TAPI_RADIUS_TYPE_STRING },
    { 1212, "EAP-Sim-KC1", TAPI_RADIUS_TYPE_STRING },
    { 1213, "EAP-Sim-KC2", TAPI_RADIUS_TYPE_STRING },
    { 1214, "EAP-Sim-KC3", TAPI_RADIUS_TYPE_STRING },
    { 1281, "EAP-Type-Identity", TAPI_RADIUS_TYPE_TEXT },
    { 1282, "EAP-Type-Notification", TAPI_RADIUS_TYPE_TEXT },
    { 1283, "EAP-Type-NAK", TAPI_RADIUS_TYPE_TEXT },
    { 1284, "EAP-Type-MD5", TAPI_RADIUS_TYPE_STRING },
    { 1285, "EAP-Type-OTP", TAPI_RADIUS_TYPE_TEXT },
    { 1286, "EAP-Type-GTC", TAPI_RADIUS_TYPE_TEXT },
    { 1297, "EAP-Type-TLS", TAPI_RADIUS_TYPE_STRING },
    { 1298, "EAP-Type-SIM", TAPI_RADIUS_TYPE_STRING },
    { 1301, "EAP-Type-LEAP", TAPI_RADIUS_TYPE_STRING },
    { 1302, "EAP-Type-SIM2", TAPI_RADIUS_TYPE_STRING },
    { 1305, "EAP-Type-TTLS", TAPI_RADIUS_TYPE_STRING },
    { 1309, "EAP-Type-PEAP", TAPI_RADIUS_TYPE_STRING },
    { 1537, "EAP-Sim-RAND", TAPI_RADIUS_TYPE_STRING },
    { 1542, "EAP-Sim-PADDING", TAPI_RADIUS_TYPE_STRING },
    { 1543, "EAP-Sim-NONCE_MT", TAPI_RADIUS_TYPE_STRING },
    { 1546, "EAP-Sim-PERMANENT_ID_REQ", TAPI_RADIUS_TYPE_STRING },
    { 1547, "EAP-Sim-MAC", TAPI_RADIUS_TYPE_STRING },
    { 1548, "EAP-Sim-NOTIFICATION", TAPI_RADIUS_TYPE_STRING },
    { 1549, "EAP-Sim-ANY_ID_REQ", TAPI_RADIUS_TYPE_STRING },
    { 1550, "EAP-Sim-IDENTITY", TAPI_RADIUS_TYPE_STRING },
    { 1551, "EAP-Sim-VERSION_LIST", TAPI_RADIUS_TYPE_STRING },
    { 1552, "EAP-Sim-SELECTED_VERSION", TAPI_RADIUS_TYPE_STRING },
    { 1553, "EAP-Sim-FULLAUTH_ID_REQ", TAPI_RADIUS_TYPE_STRING },
    { 1555, "EAP-Sim-COUNTER", TAPI_RADIUS_TYPE_STRING },
    { 1556, "EAP-Sim-COUNTER_TOO_SMALL", TAPI_RADIUS_TYPE_STRING },
    { 1557, "EAP-Sim-NONCE_S", TAPI_RADIUS_TYPE_STRING },
    { 1665, "EAP-Sim-IV", TAPI_RADIUS_TYPE_STRING },
    { 1666, "EAP-Sim-ENCR_DATA", TAPI_RADIUS_TYPE_STRING },
    { 1668, "EAP-Sim-NEXT_PSEUDONUM", TAPI_RADIUS_TYPE_STRING },
    { 1669, "EAP-Sim-NEXT_REAUTH_ID", TAPI_RADIUS_TYPE_STRING },
    { 1670, "EAP-Sim-CHECKCODE", TAPI_RADIUS_TYPE_STRING },
};

void
tapi_radius_dict_init()
{
    size_t i;

    for (i = 0; i < TAPI_RADIUS_DICT_LEN; i++)
    {
        tapi_radius_dict_index[i] = TAPI_RADIUS_ATTR_UNKNOWN;
    }
    for (i = 0; i < sizeof(tapi_radius_dict) / sizeof(tapi_radius_dict[0]);
         i++)
    {
        uint8_t id = tapi_radius_dict[i].id;

        if (tapi_radius_dict_index[id] != TAPI_RADIUS_ATTR_UNKNOWN)
        {
            WARN("%s: duplicate entry %u in RADIUS attribute dictionary",
                 __FUNCTION__, id);
        }
        tapi_radius_dict_index[id] = i;
    }
}

/**
 * Lookup RADIUS attribute dictionary entry by the attribute type
 */
const tapi_radius_attr_info_t *
tapi_radius_dict_lookup(tapi_radius_attr_type_t type)
{
    uint8_t index = tapi_radius_dict_index[type];

    if (index != TAPI_RADIUS_ATTR_UNKNOWN)
        return &tapi_radius_dict[index];

    return NULL;
}

const tapi_radius_attr_info_t *
tapi_radius_dict_lookup_by_name(const char *name)
{
    size_t i;

    for (i = 0; i < sizeof(tapi_radius_dict) / sizeof(tapi_radius_dict[0]);
         i++)
    {
        if (strcmp(name, tapi_radius_dict[i].name) == 0)
            return &tapi_radius_dict[i];
    }
    return NULL;
}

int
tapi_radius_attr_list_push(tapi_radius_attr_list_t *list,
                           const tapi_radius_attr_t *attr)
{
    tapi_radius_attr_t *p;

    p = realloc(list->attr, (list->len + 1) * sizeof(list->attr[0]));
    if (p == NULL)
    {
        ERROR("%s: failed to allocate memory for attribute", __FUNCTION__);
        return TE_ENOMEM;
    }
    memcpy(&p[list->len], attr, sizeof(p[list->len]));
    list->attr = p;
    list->len++;

    return 0;
}

/* See description in tapi_radius.h */
int
tapi_radius_attr_list_push_value(tapi_radius_attr_list_t *list,
                                 const char *name, ...)
{
    const tapi_radius_attr_info_t  *info;
    tapi_radius_attr_t              attr;
    va_list                         va;
    int                             rc = 0;

    info = tapi_radius_dict_lookup_by_name(name);
    if (info == NULL)
    {
        ERROR("%s: attribute '%s' is not found in dictionary",
              __FUNCTION__, name);
        return TE_ENOENT;
    }
    attr.type = info->id;
    attr.datatype = info->type;
    va_start(va, name);
    switch (info->type)
    {
        case TAPI_RADIUS_TYPE_ADDRESS:
        case TAPI_RADIUS_TYPE_TIME:
        case TAPI_RADIUS_TYPE_INTEGER:  /* (int) */
            attr.integer = va_arg(va, int);
            attr.len = sizeof(attr.integer);
            break;

        case TAPI_RADIUS_TYPE_STRING:   /* (uint8_t *, size_t) */
            {
                const uint8_t *p = va_arg(va, uint8_t *);
                attr.len = va_arg(va, size_t);
                attr.string = calloc(1, attr.len + 1);
                if (attr.string == NULL)
                {
                    ERROR("%s: failed to allocate memory for attribute '%s'",
                          __FUNCTION__, name);
                    rc = TE_ENOMEM;
                }
                else
                    memcpy(attr.string, p, attr.len);
            }
            break;

        case TAPI_RADIUS_TYPE_TEXT:     /* (char *) */
            {
                const char *s = va_arg(va, char *);
                attr.string = strdup(s);
                if (attr.string == NULL)
                {
                    ERROR("%s: failed to allocate memory for attribute '%s'",
                          __FUNCTION__, name);
                    rc = TE_ENOMEM;
                }
                else
                    attr.len = strlen(s);
            }
            break;
        default:
            ERROR("%s: unknown type %u for attribute",
                  __FUNCTION__, info->type);
            rc = TE_EINVAL;
    }
    va_end(va);
    tapi_radius_attr_list_push(list, &attr);

    return rc;
}

const tapi_radius_attr_t *
tapi_radius_attr_list_find(const tapi_radius_attr_list_t *list,
                           tapi_radius_attr_type_t type)
{
    size_t i;

    for (i = 0; i < list->len; i++)
        if (list->attr[i].type == type)
            return &list->attr[i];

    return NULL;
}

void
tapi_radius_attr_list_init(tapi_radius_attr_list_t *list)
{
    list->len = 0;
    list->attr = NULL;
}

void
tapi_radius_attr_list_free(tapi_radius_attr_list_t *list)
{
    size_t  i;

    for (i = 0; i < list->len; i++)
    {
        if (TAPI_RADIUS_TYPE_IS_DYNAMIC(list->attr[i].datatype))
            free(list->attr[i].string);
    }
    free(list->attr);
    list->attr = NULL;
}

int
tapi_radius_attr_list_to_string(const tapi_radius_attr_list_t *list,
                                char **str)
{
    size_t       i;
    char        *result = NULL;
    size_t       len = 0;

    assert(str != NULL);

    *str = NULL;

    for (i = 0; i < list->len; i++)
    {
        const tapi_radius_attr_t      *attr = &list->attr[i];
        const tapi_radius_attr_info_t *info;
        char                           buf[INET_ADDRSTRLEN];
        const char                    *value;
        char                          *s;
        size_t                         attr_strlen = 0;

        if ((info = tapi_radius_dict_lookup(attr->type)) == NULL)
        {
            ERROR("%s: failed to find attribute %u in RADIUS dictionary",
                  __FUNCTION__, attr->type);
            return TE_ENOENT;
        }
        assert(attr->datatype == info->type);
        switch (attr->datatype)
        {
            case TAPI_RADIUS_TYPE_INTEGER:
                snprintf(buf, sizeof(buf), "%u", attr->integer);
                value = buf;
                break;
            case TAPI_RADIUS_TYPE_ADDRESS:
                if (inet_ntop(AF_INET, &attr->integer,
                              buf, sizeof(buf)) == NULL)
                {
                    ERROR("%s: failed to convert value of attribute '%s' "
                          "to string", __FUNCTION__, info->name);
                    free(result);
                    return TE_EINVAL;
                }
                value = buf;
                break;
            case TAPI_RADIUS_TYPE_TEXT:
                value = attr->string;
                attr_strlen = 2;        /* enclosing quotes */
                break;
            default:
                WARN("%s: attribute '%s' type is unsupported, skipping",
                     __FUNCTION__, info->name);
                continue;
        }
        attr_strlen += strlen(info->name) + strlen(value) + 2;
        s = realloc(result, len + attr_strlen);
        if (s == NULL)
        {
            ERROR("%s: failed to allocate memory", __FUNCTION__);
            free(result);
            return TE_ENOMEM;
        }
        result = s;
        if (len > 0)
            strcat(result, ",");
        else
            result[0] = '\0';

        strcat(result, info->name);
        strcat(result, "=");
        if (attr->datatype == TAPI_RADIUS_TYPE_TEXT)
        {
            strcat(result, "\"");
            strcat(result, value);
            strcat(result, "\"");
        }
        else
            strcat(result, value);
        len += attr_strlen;
    }
    *str = result;
    
    return 0;
}

#if 0
void
tapi_radius_attr_list_enumerate(tapi_radius_attr_list_t *list,
                                tapi_radius_attr_cb_t callback,
                                void *user_data)
{
    size_t  i;

    if (callback == NULL)
        return;

    for (i = 0; i < list->len; i++)
    {
        callback(&list->attr[i], user_data);
    }
}

int
tapi_radius_attr_init(tapi_radius_attr_t *attr,
                      tapi_radius_type_t *data_type,
                      tapi_radius_attr_type_t attr_type,
                      const uint8_t *data, size_t data_len)
{
    tapi_radius_attr_info_t  attr_info;

    attr_info = tapi_radius_dict_lookup(attr_type);
    if (attr_info == NULL)
    {
        ERROR("%s: unknown attribute %u", __FUNCTION__, attr_type);
        return TE_EINVAL;
    }
    *data_type = attr_info->type;
    switch (attr_info->type)
    {
        case TAPI_RADIUS_TYPE_INTEGER:
        case TAPI_RADIUS_TYPE_ADDRESS:
        case TAPI_RADIUS_TYPE_TIME:
            if (data_len != sizeof(attr->integer))
            {
                ERROR("%s: invalid length of attribute %u",
                      __FUNCTION__, attr_type);
                return TE_EINVAL;
            }
            memcpy(&attr->integer, data, sizeof(attr->integer));
            attr->len = sizeof(attr->integer);
            break;

        case TAPI_RADIUS_TYPE_TEXT:
        case TAPI_RADIUS_TYPE_STRING:
            attr->string = calloc(data_len + 1, sizeof(char));
            if (attr->string == NULL)
            {
                ERROR("%s: failed to allocate memory for attribute %u",
                      __FUNCTION__, attr_type);
                return TE_ENOMEM;
            }
            memcpy(attr->string, data, data_len);
            attr->len = data_len;
            break;

        default:
            ERROR("%s: unknown type %u for attribute %u", __FUNCTION__,
                  attr_info->type, attr.type);
            return TE_EINVAL;
    }
    return 0;
}
#endif

int
tapi_radius_attr_copy(tapi_radius_attr_t *dst,
                      const tapi_radius_attr_t *src)
{
    memcpy(dst, src, sizeof(*dst));
    if (TAPI_RADIUS_TYPE_IS_DYNAMIC(src->datatype))
    {
        dst->string = calloc(1, src->len + 1);
        if (dst->string == NULL)
        {
            ERROR("%s: failed to allocate memory", __FUNCTION__);
            return TE_ENOMEM;
        }
        memcpy(dst->string, src->string, src->len);
    }
    return 0;
}

int
tapi_radius_attr_list_copy(tapi_radius_attr_list_t *dst,
                           const tapi_radius_attr_list_t *src)
{
    int    rc;
    size_t i;

    memcpy(dst, src, sizeof(*dst));
    dst->attr = (tapi_radius_attr_t *)malloc(src->len * sizeof(src->attr[0]));
    if (dst->attr == NULL)
    {
        ERROR("%s: failed to allocate memory", __FUNCTION__);
        return TE_ENOMEM;
    }
    for (i = 0; i < src->len; i++)
    {
        if ((rc = tapi_radius_attr_copy(&dst->attr[i], &src->attr[i])) != 0)
            return rc;
    }
    return 0;
}

int
tapi_radius_parse_packet(const uint8_t *data, size_t data_len,
                         tapi_radius_packet_t *packet)
{
    const uint8_t                 *p = data;
    uint16_t                       radius_len;
    tapi_radius_attr_t             attr;
    const tapi_radius_attr_info_t *attr_info;

    if (data == NULL)
        return TE_EINVAL;

    if (data_len < TAPI_RADIUS_PACKET_MIN_LEN)
    {
        ERROR("%s: data length is too small, %u bytes",
              __FUNCTION__, data_len);
        return TE_EINVAL;
    }

    packet->code = *p++;
    packet->identifier = *p++;
    memcpy(&radius_len, p, sizeof(radius_len));
    radius_len = ntohs(radius_len);
    p += sizeof(radius_len);

    if (radius_len > data_len)
    {
        ERROR("%s: buffer size (%u) is smaller than RADIUS packet length (%u)",
              __FUNCTION__, data_len, radius_len);
        return TE_EINVAL;
    }
    if (radius_len < TAPI_RADIUS_PACKET_MIN_LEN ||
        radius_len > TAPI_RADIUS_PACKET_MAX_LEN)
    {
        ERROR("%s: RADIUS packet with invalid length %d",
              __FUNCTION__, radius_len);
        return TE_EINVAL;
    }
    memcpy(packet->authenticator, p, sizeof(packet->authenticator));
    p += sizeof(packet->authenticator);

    /* Attributes */
    tapi_radius_attr_list_init(&packet->attrs);
    while ((p - data) + TAPI_RADIUS_ATTR_MIN_LEN <= radius_len)
    {
        uint32_t value;

        attr.type = *p++;
        attr.len = (*p++) - 2;
        if ((p - data) + attr.len > radius_len)
        {
            ERROR("%s: invalid RADIUS packet - attribute %u value is out "
                  "of packet data", __FUNCTION__, attr.type);
            return TE_EINVAL;
        }

        attr_info = tapi_radius_dict_lookup(attr.type);
        if (attr_info == NULL)
        {
            WARN("%s: unknown attribute %u", __FUNCTION__, attr.type);
            attr.datatype = TAPI_RADIUS_TYPE_UNKNOWN;
        }
        else
            attr.datatype = attr_info->type;

        switch (attr.datatype)
        {
            case TAPI_RADIUS_TYPE_INTEGER:
            case TAPI_RADIUS_TYPE_ADDRESS:
            case TAPI_RADIUS_TYPE_TIME:
                if (attr.len != sizeof(value))
                {
                    ERROR("%s: invalid length of attribute %u",
                          __FUNCTION__, attr.type);
                    break;
                }
                memcpy(&value, p, sizeof(value));
                attr.integer = ntohl(value);
                break;

            case TAPI_RADIUS_TYPE_TEXT:
            case TAPI_RADIUS_TYPE_STRING:
            case TAPI_RADIUS_TYPE_UNKNOWN:
                attr.string = calloc(attr.len + 1, sizeof(char));
                if (attr.string == NULL)
                {
                    ERROR("%s: failed to allocate memory for attribute %u",
                          __FUNCTION__, attr.type);
                    return TE_ENOMEM;
                }
                memcpy(attr.string, p, attr.len);
                break;

            default:
                ERROR("%s: unknown type %u for attribute %u", __FUNCTION__,
                      attr.datatype, attr.type);
                return TE_EINVAL;
        }
        p += attr.len;
        tapi_radius_attr_list_push(&packet->attrs, &attr);
    }
    return 0;
}


int
tapi_radius_csap_create(const char *ta, int sid, const char *device,
                        const in_addr_t net_addr, uint16_t port,
                        csap_handle_t *csap)
{
    return tapi_udp_ip4_eth_csap_create(ta, sid, device, NULL, NULL,
                                        net_addr, INADDR_ANY,
                                        port, 0, csap);
}

typedef struct {
    radius_callback  callback;
    void            *userdata;
} radius_cb_data_t;


void
tapi_radius_callback(const udp4_datagram *pkt, void *userdata)
{
    tapi_radius_packet_t  packet;
    radius_cb_data_t     *cb_data = (radius_cb_data_t *)userdata;
    int                   rc;

    rc = tapi_radius_parse_packet(pkt->payload, pkt->payload_len, &packet);
    if (rc != 0)
    {
        ERROR("%s: failed to parse UDP packet to %s:%u",
              __FUNCTION__, inet_ntoa(pkt->dst_addr), pkt->dst_port);
        return;
    }
    cb_data->callback(&packet, cb_data->userdata);
    tapi_radius_attr_list_free(&packet.attrs);
}

int
tapi_radius_recv_start(const char *ta, int sid, csap_handle_t csap,
                       radius_callback user_callback, void *user_data,
                       unsigned int timeout)
{
    radius_cb_data_t  *cb_data;

    UNUSED(timeout);

    cb_data = (radius_cb_data_t *)calloc(1, sizeof(radius_cb_data_t));
    if (cb_data == NULL)
    {
        ERROR("%s: failed to allocate memory", __FUNCTION__);
        return TE_ENOMEM;
    }
    cb_data->callback = user_callback;
    cb_data->userdata = user_data;

    return tapi_udp_ip4_eth_recv_start(ta, sid, csap, NULL,
                                       tapi_radius_callback, cb_data);
}



/* See the description in tapi_radius.h */
int
tapi_radius_serv_enable(const char *ta_name)
{
    assert(ta_name != NULL);

    return cfg_set_instance_fmt(CVT_INTEGER, (void *)TRUE,
                                "/agent:%s/radiusserver:", ta_name);
}

/* See the description in tapi_radius.h */
int
tapi_radius_serv_disable(const char *ta_name)
{
    assert(ta_name != NULL);

    return cfg_set_instance_fmt(CVT_INTEGER, (void *)FALSE,
                                "/agent:%s/radiusserver:", ta_name);
}

/* See the description in tapi_radius.h */
int
tapi_radius_serv_set(const char *ta_name, const tapi_radius_serv_t *cfg)
{
    struct sockaddr_in addr;

    assert(ta_name != NULL && cfg != NULL);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy(&(addr.sin_addr), &(cfg->net_addr), sizeof(addr.sin_addr));

#define RADIUS_CFG_SET_VALUE(sub_oid_, cfg_value_, err_msg_)        \
    do {                                                            \
        int rc;                                                     \
                                                                    \
        rc = cfg_set_instance_fmt(cfg_value_,                       \
                 "/agent:%s/radiusserver:/" sub_oid_ ":", ta_name); \
        if (rc != 0)                                                \
        {                                                           \
            ERROR("Cannot set RADIUS %s on '%s' Agent",             \
                  err_msg_, ta_name);                               \
            return rc;                                              \
        }                                                           \
    } while (0)

    RADIUS_CFG_SET_VALUE("auth_port", CFG_VAL(INTEGER, cfg->auth_port),
                         "Authentication Port");
    RADIUS_CFG_SET_VALUE("acct_port", CFG_VAL(INTEGER, cfg->acct_port),
                         "Accounting Port");
    RADIUS_CFG_SET_VALUE("net_addr", CFG_VAL(ADDRESS, &addr),
                         "Network Address");

#undef RADIUS_CFG_SET_VALUE

    return 0;
}

/* See the description in tapi_radius.h */
int
tapi_radius_serv_add_client(const char *ta_name,
                            const tapi_radius_clnt_t *cfg)
{
    int        rc;
    cfg_handle handle;
    char       clnt_name[INET_ADDRSTRLEN];

    assert(ta_name != NULL && cfg != NULL);

    if (cfg->secret == NULL)
    {
        ERROR("Incorrect secret value for RADIUS Client");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (inet_ntop(AF_INET, &(cfg->net_addr),
                  clnt_name, sizeof(clnt_name)) == NULL)
    {
        ERROR("Cannot convert network address of RADIUS Client into "
              "string representation");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = cfg_add_instance_fmt(&handle, CVT_NONE, NULL,
                              "/agent:%s/radiusserver:/client:%s",
                              ta_name, clnt_name);

    if (rc != 0)
    {
        ERROR("Cannot add a new RADIUS Client on '%s' Agent", ta_name);
        return rc;
    }

    /* Set secret phrase */
    return cfg_set_instance_fmt(CVT_STRING, (void *)cfg->secret,
                                "/agent:%s/radiusserver:/client:%s/secret:",
                                ta_name, clnt_name);
}

/* See the description in tapi_radius.h */
int
tapi_radius_serv_del_client(const char *ta_name,
                            const struct in_addr *net_addr)
{
    int  rc;
    char clnt_name[INET_ADDRSTRLEN];

    assert(ta_name != NULL && net_addr != NULL);

    if (inet_ntop(AF_INET, net_addr,
                  clnt_name, sizeof(clnt_name)) == NULL)
    {
        ERROR("Cannot convert network address of RADIUS Client into "
              "string representation");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if ((rc = cfg_del_instance_fmt(FALSE,
                                   "/agent:%s/radiusserver:/client:%s",
                                   ta_name, clnt_name)) != 0)
    {
        ERROR("Cannot delete RADIUS Client %s on '%s' Agent",
              clnt_name, ta_name);
    }

    return rc;
}

/* See the description in tapi_radius.h */
int
tapi_radius_serv_add_user(const char *ta_name,
                          const char *user_name,
                          te_bool acpt_user,
                          const tapi_radius_attr_list_t *check_attrs,
                          const tapi_radius_attr_list_t *acpt_attrs,
                          const tapi_radius_attr_list_t *chlg_attrs)
{
    int        rc;
    char      *attr_str;
    cfg_handle handle;

    assert(ta_name != NULL && user_name != NULL);

    if ((rc = cfg_add_instance_fmt(&handle,
                                   CFG_VAL(INTEGER, acpt_user ? 1 : 0),
                                   "/agent:%s/radiusserver:/user:%s",
                                   ta_name, user_name)) != 0)
    {
        ERROR("Failed to add RADIUS user '%s' on Agent '%s'",
              user_name, ta_name);
        return TE_RC(TE_TAPI, rc);
    }
#define ADD_ATTR_LIST(_attrs, _name, _cfg_name) \
    if (_attrs != NULL)                                                     \
    {                                                                       \
        if ((rc = tapi_radius_attr_list_to_string(_attrs, &attr_str)) != 0) \
        {                                                                   \
            ERROR("Failed to convert " _name " RADIUS attributes list "     \
                  "for user '%s' to string", user_name);                    \
            return TE_RC(TE_TAPI, rc);                                      \
        }                                                                   \
        if ((rc = cfg_set_instance_fmt(CFG_VAL(STRING, attr_str),           \
                    "/agent:%s/radiusserver:/user:%s/" _cfg_name ":",       \
                    ta_name, user_name)) != 0)                              \
        {                                                                   \
            ERROR("Failed to add " _name " RADIUS attributes list '%s'"     \
                  "for user '%s'", attr_str, user_name);                    \
            free(attr_str);                                                 \
            return TE_RC(TE_TAPI, rc);                                      \
        }                                                                   \
        free(attr_str);                                                     \
    }

    ADD_ATTR_LIST(check_attrs, "check", "check");
    ADD_ATTR_LIST(acpt_attrs, "Access-Accept", "accept-attrs");
    ADD_ATTR_LIST(chlg_attrs, "Access-Challenge", "challenge-attrs");

#undef ADD_ATTR_LIST

    return 0;
}

/* See the description in tapi_radius.h */
int
tapi_radius_serv_del_user(const char *ta_name, const char *user_name)
{
    int rc;

    if ((rc = cfg_del_instance_fmt(FALSE, "/agent:%s/radiusserver:/user:%s",
                                   ta_name, user_name)) != 0)
    {
        ERROR("Failed to remove RADIUS user '%s' from the Configurator DB",
              user_name);
    }
    return rc;
}

/* Supplicant related functions @todo Should not be here */

/* See the description in tapi_radius.h */
int
tapi_supp_set_md5(const char *ta_name, const char *if_name,
                  tapi_supp_auth_md5_info_t *info)
{
    int rc;

    /* Set MD5 related parameters */
    rc = cfg_set_instance_fmt(CVT_STRING, (void *)info->user,
                              "/agent:%s/supplicant:%s/method:%s/username:",
                              ta_name, if_name, "md5");
    if (rc != 0)
        return rc;

    rc = cfg_set_instance_fmt(CVT_STRING, (void *)info->passwd,
                              "/agent:%s/supplicant:%s/method:%s/passwd:",
                              ta_name, if_name, "md5");
    if (rc != 0)
        return rc;
    
    /* Now set current authentication method to MD5 */
    rc = cfg_set_instance_fmt(CVT_STRING, (void *)"md5",
                              "/agent:%s/supplicant:%s/cur_method:",
                              ta_name, if_name);
    return rc;
}
