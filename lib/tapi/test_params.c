/** @file
 * @brief Test API
 *
 * Procedures that provide access to test parameter.
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 *
 * @author Oleg Kravtsov <Oleg.Kravtsov@oktetlabs.ru>
 *
 * $Id$
 */

#define TE_LGR_USER     "TAPI Params"

#include "te_config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#include "te_defs.h"
#include "te_param.h"
#include "te_ethernet.h"
#include "logger_api.h"

#include "tapi_test.h"
#include "tapi_cfg.h"
#include "asn_impl.h"
#include "asn_usr.h"
#include "ndn.h"
#include "ndn_base.h"


/** Global variable with entity name for logging */
DEFINE_LGR_ENTITY("(unknown)");


/* See description in tapi_test.h */
void
te_test_sig_handler(int signum)
{
    if (signum == SIGINT)
    {
        UNUSED(signum);
        exit(TE_EXIT_SIGINT);
    }
    else if (signum == SIGUSR1)
    {
        TEST_FAIL("Test is killed by SIGUSR1");
    }
    else if (signum == SIGUSR2)
    {
        if (getenv("TE_TEST_SIGUSR2_VERDICT") != NULL)
            RING_VERDICT("Test is stopped by SIGUSR2");
        else
            exit(TE_EXIT_SIGINT);
    }
}


/** See the description in tapi_test.h */
const char *
test_get_param(int argc, char *argv[], const char *name)
{
    int         i;
    const char *ptr = NULL;
    const char *value;

    for (i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], name, strlen(name)) != 0)
            continue;

        ptr = argv[i] + strlen(name);
        while (isspace(*ptr))
            ptr++;
        if (*(ptr++) != '=')
        {
            ERROR("Error while parsing '%s' parameter value: "
                  "cannot find '=' delimeter", name);
            return NULL;
        }
        while (isspace(*ptr))
            ptr++;

        break;
    }
    if (ptr == NULL)
    {
        WARN("There is no '%s' parameter specified", name);
        return NULL;
    }

    /* we've found required parameter */
    INFO("Parameter %s has value '%s'", name, ptr);

    if (strncmp(ptr, TEST_ARG_VAR_PREFIX,
                strlen(TEST_ARG_VAR_PREFIX)) == 0)
    {
        /*
         * it's in fact a reference to a variable, we should
         * form a name of corresponding shell environment
         * variable and call getenv() to get it
         */
        char env_name[128];
        int  env_size = sizeof(env_name);

        te_var_name2env(ptr, env_name, env_size);

        value = getenv(env_name);
    }
    else
        /* normal value */
        value = ptr;

    return value;
}

/** See the description in tapi_test.h */
int
test_map_param_value(const char *var_name,
                     struct param_map_entry *maps, const char *str_val,
                     int *num_val)
{
    int i;

    for (i = 0; maps[i].str_val != NULL; i++)
    {
        if (strcmp(str_val, maps[i].str_val) == 0)
        {
            *num_val = maps[i].num_val;
            return 0;
        }
    }

    {
        char buf[2048];

        snprintf(buf, sizeof(buf),
                 "'%s' parameter has incorrect value '%s'. "
                 "It can have the following value: {",
                 var_name, str_val);

        for (i = 0; maps[i].str_val != NULL; i++)
        {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                     "'%s', ", maps[i].str_val);
        }
        if (i > 0)
        {
            /* Delete comma after the last item in the list */
            buf[strlen(buf) - 2] = '\0';
        }
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "}");

        ERROR(buf);
    }

    return -1;
}

/** See the description in tapi_test.h */
uint8_t *
test_get_octet_string_param(const char *str_val, size_t len)
{
    uint8_t        *oct_string, *ptr;
    const char     *nptr, *endptr;
    unsigned int    i = 0;

    if (str_val == NULL)
    {
        ERROR("Invalid parameter: NULL pointer");
        return NULL;
    }

    oct_string = (uint8_t *)calloc(len, sizeof(uint8_t));
    if (oct_string == NULL)
    {
        ERROR("Error in memory allocation");
        return NULL;
    }
    ptr = oct_string;
    nptr = NULL;
    endptr = str_val;

    while ((nptr != endptr) && (endptr) && (i < len))
    {
        /* How should I fight args list ?*/
        if (*endptr == ':')
            endptr++;
        nptr = endptr;
        *ptr++ = (unsigned char)strtol(nptr, (char **)&endptr, 16);
        i++;
    }

    if (*endptr != '\0')
    {
        ERROR("Error in parsing octet string %s or bad given length %u",
              str_val, len);
        free(oct_string);
        return NULL;
    }

    if (i != len)
    {
        ERROR("Bad given length %u for octet string %s", len, str_val);
        free(oct_string);
        return NULL;
    }

    return oct_string;
}

/** See the description in tapi_test.h */
const char*
print_octet_string(const uint8_t *oct_string, size_t len)
{
    static char     buf[256];

    char           *p = buf;
    unsigned int    i;

    if (oct_string == NULL || len == 0)
    {
        strncpy(buf, "<null octet string>", sizeof(buf));
        return buf;
    }

    memset(buf, 0, sizeof(buf));

    for (i = 0; i < len; i++)
        p += sprintf(p, " 0x%02x", (unsigned int)(*oct_string++));

    return buf;
}

#define TEST_LIST_PARAM_CHUNK       8
#define TEST_LIST_PARAM_SEPARATOR   ','

/** See the description in tapi_test.h */
int
test_split_param_list(const char *list, char ***array_p)
{
    char  **array = NULL;
    char   *ptr = strdup(list);
    char    size = 0;
    int     length = 0;

    while (1)
    {
        /* Allocate memory if necessary */
        if (length <= size)
        {
            array = realloc(array, 
                            (size + TEST_LIST_PARAM_CHUNK) * 
                            sizeof(char *));
            if (array == NULL)
                return 0;
            memset((char *)array + size, 0, 
                   TEST_LIST_PARAM_CHUNK * sizeof(char *));
            size += TEST_LIST_PARAM_CHUNK;
        }

        /* get next value from the list */
        array[length] = ptr;
        length++;

        /* Find next value */
        ptr = strchr(ptr, TEST_LIST_PARAM_SEPARATOR);
        if (ptr == NULL)
        {
            /* No new separators */
            *array_p = array;
            return length;
        }

        /* Replace separator by end of string and skip all spaces */
        *ptr = '\0';
        ptr++;
        while (isspace(*ptr))
            ptr++;
    }
    /* Unreachable */
}

#define BUFLEN 512
/*
 * Structure to store (parameter name, value) pair
 */
typedef struct tapi_asn_param_pair {
    char *name;
    char *value;
} tapi_asn_param_pair;

/**
 * Function parses value and converts it to ASN value.
 * It also handles cfg links.
 *
 * @param ns    Preallocated buffer for updated value
 */
te_errno
tapi_asn_param_value_parse(char              *pwd,
                           char             **s,
                           const asn_type    *type,
                           asn_value        **parsed_val,
                           int               *parsed_syms,
                           char              *ns)
{
    te_errno rc;

    if (type == ndn_base_octets)
    {
        /*
         * Check and convert if necessary human-readable notation
         * 10.0.0.1 to ASN standard notation '0a 00 00 01'H
         */
        char *p;
        struct in_addr addr;


        p = strchr(*s, '.');
        if (p != NULL)
        {
            /* We need a conversion */
            if (inet_aton(*s, &addr) == 0)
            {
                ERROR("Failed to parse IP address '%s'", *s);
                rc = TE_EFAULT;
                return rc;
            }

            snprintf(ns, BUFLEN, "'%02x %02x %02x %02x'H",
                     (addr.s_addr) & 0xff,
                     (addr.s_addr >> 8) & 0xff,
                     (addr.s_addr >> 16) & 0xff,
                     (addr.s_addr >> 24) & 0xff);

            *s = ns;
        }
    }

    INFO("%s: called, pwd='%s' %d", __FUNCTION__, pwd, strlen(pwd));
    /* in case we deal with cfg link */
    if (tapi_is_cfg_link(*s))
    {
        int                 int_val = 0;
        struct sockaddr    *address_val = NULL;
        uint8_t            *addr_octets;
        int                 pwd_offset = strlen(pwd);
        char               *c = (*s + strlen(TAPI_CFG_LINK_PREFIX));
        int                 add;

        /* *c = ('/' | '.' | 'letter') */
        if (*c == '/')
            pwd_offset = 0;

        add = snprintf(pwd + pwd_offset, BUFLEN - pwd_offset,
                       "/%s", c + (*c == '.' ? 2 : *c == '/' ? 1 : 0));

        if (*c != '.')
            pwd_offset += add;

        rc = cfg_get_instance_fmt(NULL,
                                  (void*)((type == ndn_base_octets) ?
                                  &address_val : &int_val),
                                  "%s", pwd);
        pwd[pwd_offset] = '\0';
        if (rc != 0)
        {
            ERROR("%s: bad cfg link '%s' (pwd='%s') given: %r",
                  __FUNCTION__, *s, pwd, rc);
            return rc;
        }

        if (type == ndn_base_octets)
        {
            /* Fixme: looks ugly */
            if (strstr(pwd, "link_addr") != NULL ||
                address_val->sa_family == AF_LOCAL)
            {
                addr_octets = address_val->sa_data;

                snprintf(
                        ns, BUFLEN,
                        "'%02x %02x %02x %02x %02x %02x'H",
                        addr_octets[0], addr_octets[1], addr_octets[2],
                        addr_octets[3], addr_octets[4], addr_octets[5]);
            }
            else if (address_val->sa_family == AF_INET6)
            {
                addr_octets = SIN6(address_val)->sin6_addr.s6_addr;

                snprintf(
                        ns, BUFLEN,
                        "'"
                        "%02x %02x %02x %02x "
                        "%02x %02x %02x %02x "
                        "%02x %02x %02x %02x "
                        "%02x %02x %02x %02x "
                        "'H",
                        addr_octets[0], addr_octets[1],
                        addr_octets[2], addr_octets[3],
                        addr_octets[4], addr_octets[5],
                        addr_octets[6], addr_octets[7],
                        addr_octets[8], addr_octets[9],
                        addr_octets[10], addr_octets[11],
                        addr_octets[12], addr_octets[13],
                        addr_octets[14], addr_octets[15]);
            }
            else
            {
                /*
                 * FIXME: address is not AF_LOCAL, not AF_INET6
                 * we expect it to be AF_INET.
                 */
                snprintf(ns, BUFLEN, "'%02x %02x %02x %02x'H",
                         (SIN(address_val)->sin_addr.s_addr) & 0xff,
                         (SIN(address_val)->sin_addr.s_addr >> 8) & 0xff,
                         (SIN(address_val)->sin_addr.s_addr >> 16) & 0xff,
                         (SIN(address_val)->sin_addr.s_addr >> 24) & 0xff);
            }
        }
        else
        {
            snprintf(ns, BUFLEN, "%d", int_val);
        }

        free(address_val);
        *s = ns;
    }

    rc = asn_parse_value_text(*s, type, parsed_val, parsed_syms);

    return rc;
}


/* See description in tapi_test.h */
te_errno
tapi_asn_params_get(int argc, char **argv, const char *conf_prefix,
                    const asn_type *conf_type, asn_value *conf_value)
{
    te_errno        rc = 0;
    int             i;
    const char     *type_suffix = ".type";
    char           *name = NULL;
    char           *value = NULL;
    char           *p = NULL;
    int             parsed_syms;
    asn_value      *asn_param_value;
    int             index;
    const asn_type *asn_param_type;
    char            asn_path[BUFLEN];
    unsigned int    asn_path_len = BUFLEN;
    char            temp_buf[BUFLEN];

    tapi_asn_param_pair *param = NULL;

    /* Sets for different kind of parameters */
    tapi_asn_param_pair *creation_param = NULL;
    tapi_asn_param_pair *change_param = NULL;
    int                  creation_param_count = 0;
    int                  change_param_count = 0;

    char pwd[BUFLEN];    /* filled with zeroes on init */

    /* should be all filled with zeroes */
    memset(pwd, 0, sizeof(pwd));

    creation_param = malloc(argc * sizeof(tapi_asn_param_pair));
    if (creation_param == NULL)
    {
        rc = TE_ENOMEM;
        goto cleanup;
    }

    change_param = malloc(argc * sizeof(tapi_asn_param_pair));
    if (change_param == NULL)
    {
        rc = TE_ENOMEM;
        goto cleanup;
    }

    /*
     * Separate parameters to several sets:
     *  - Object creation (type specification) - ends with '.type'
     *  - Parameters change
     *  - Other non-ASN parameter
     */
    for (i = 0; i < argc; i++)
    {
        if (0 != strncmp(argv[i], conf_prefix, strlen(conf_prefix)))
            continue;

        name = malloc(strlen(argv[i]) + 1);
        strcpy(name, argv[i] + strlen(conf_prefix));
        value = strchr(name, '=');
        *value = '\0';
        value++;
        p = strstr(name, type_suffix);
        if ((p != NULL) &&
            (unsigned)(p - name) == strlen(name) - strlen(type_suffix) &&
            strcmp(value, "INVALID") != 0)
        {
            /* This is creation */
            creation_param[creation_param_count].name = name;
            creation_param[creation_param_count].name[strlen(name) -
                strlen(type_suffix)] = '\0';
            creation_param[creation_param_count].value = value;
            creation_param_count++;
        }
        else
        {
            if (strcmp(value, "INVALID") != 0)
            {
                /* This is parameter change */
                change_param[change_param_count].name = name;
                change_param[change_param_count].value = value;
                change_param_count++;
            }
        }
    }

    if (creation_param_count + change_param_count == 0)
    {
        ERROR("No ASN configuration test parameters found");
    }
    else
    {
        RING("Found %d ASN configuration parameters",
             creation_param_count + change_param_count);
    }

    /* Process creation parameters */
    INFO("%s: process creation parameters", __FUNCTION__);
    for (i = 0; i < creation_param_count; i++)
    {
        unsigned int subtype_id;

        param = &creation_param[i];
        RING("%s: name='%s' value='%s'",
             __FUNCTION__, param->name, param->value);

        for (subtype_id = 0; subtype_id < conf_type->len; subtype_id++)
        {
            const asn_named_entry_t *entry =
                &conf_type->sp.named_entries[subtype_id];

            RING("Iterate entry %s", entry->name);

            if (strncmp(param->name, entry->name, strlen(entry->name)) == 0)
            {
                if (entry->type->syntax != SEQUENCE_OF)
                {
                    ERROR("Syntax of %s is not SEQUENCE_OF", entry->name);
                    rc = TE_EINVAL;
                    break;
                }
                rc = asn_parse_value_text(param->value,
                                          &entry->type->sp.subtype[0],
                                          &asn_param_value, &parsed_syms);
                break;
            }
        }
        if (subtype_id >= conf_type->len)
        {
            RING("Failed to find matching entry for %s", param->name);
            rc = TE_RC(TE_TAPI, TE_EINVAL);
        }

        if (rc != 0)
        {
            ERROR("Failed to parse creation param #%d, value='%s'."
                  "\nError at '%s'",
                  i, param->value,
                 param->value + parsed_syms);
            goto cleanup;
        }

        rc = asn_insert_value_extended_path(conf_value,
                                            param->name,
                                            asn_param_value,
                                            &index);
        if (rc != 0)
        {
            ERROR("Failed to insert parameter #%d into ASN configuration, "
                  "rc=%r", i, rc);
            goto cleanup;
        }
    }

    RING("Creation params processed");

    /* Process parameter change */
    for (i = 0; i < change_param_count; i++)
    {
        param = &change_param[i];
#if 0
        RING("%s: name='%s' value='%s'", __FUNCTION__,
             param->name, param->value);
#endif
        rc = asn_path_from_extended(conf_value,
                                    param->name,
                                    asn_path,
                                    asn_path_len,
                                    TRUE);
        if (rc != 0)
        {
            ERROR("Failed to convert extended path to normal, path='%s', "
                  "rc=%r", param->name, rc);
            goto cleanup;
        }

        rc = asn_get_subtype(conf_type, &asn_param_type, asn_path);
        if (rc != 0)
        {
            ERROR("Failed to get subtype for path '%s', rc=%r",
                  asn_path, rc);
            goto cleanup;
        }

        /* internal function call, NOT ASN! */
        rc = tapi_asn_param_value_parse(pwd, &param->value, asn_param_type,
                                        &asn_param_value, &parsed_syms,
                                        temp_buf);
        if (rc != 0)
        {
            ERROR("Failed to parse ASN value '%s'."
                  "\nError at '%s', rc=%r",
                  param->value, param->value + parsed_syms, rc);
            goto cleanup;
        }

        rc = asn_put_descendent(conf_value,
                                asn_param_value,
                                asn_path);
        if (rc != 0)
        {
            ERROR("Failed to add item into configuration tree at '%s', "
                  "rc=%r", asn_path, rc);
            goto cleanup;
        }
    }


    INFO("Changed params processed");

cleanup:
    if (creation_param != NULL)
    {
        for (i = 0; i < creation_param_count; i++)
        {
            if (creation_param[i].name != NULL)
                free(creation_param[i].name);
        }

        free(creation_param);
    }
    if (change_param != NULL)
    {
        for (i = 0; i < change_param_count; i++)
        {
            if (change_param[i].name != NULL)
                free(change_param[i].name);
        }

        free(change_param);
    }

    if (rc == 0)
    {
        RING("ASN parameters parsed succesfully");
    }
    else
    {
        ERROR("Failed to parse ASN parameters, rc=%r", rc);
    }

    return rc;
}

