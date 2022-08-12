/** @file
 * @brief Test API
 *
 * Procedures that provide access to test parameter.
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs. All rights reserved.
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
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include "te_defs.h"
#include "te_param.h"
#include "te_ethernet.h"
#include "te_units.h"
#include "te_str.h"
#include "logger_api.h"

#include "tapi_test.h"
#include "tapi_cfg.h"
#include "asn_impl.h"
#include "asn_usr.h"
#include "ndn.h"
#include "ndn_base.h"

static te_bool sigusr2_caught = FALSE;

te_bool
te_sigusr2_caught(void)
{
    return sigusr2_caught;
}

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
        if (getenv("TE_TEST_SIGUSR2_STOP") != NULL)
            exit(TE_EXIT_SIGUSR2);
        else
            sigusr2_caught = TRUE;
    }
}

/** See the description in tapi_test.h */
const char *
test_find_param(int argc, char *argv[], const char *name)
{
    int         i;
    const char *ptr = NULL;

    for (i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], name, strlen(name)) != 0)
            continue;

        ptr = argv[i] + strlen(name);

        /*
         * May be we matched another name that just has our name
         * in the beginning.
         */
        if (!isspace(*ptr) && *ptr != '=')
        {
            ptr = NULL;
            continue;
        }

        while (isspace(*ptr))
            ptr++;
        if (*(ptr++) != '=')
        {
            ERROR("Error while parsing '%s' parameter value: "
                  "cannot find '=' delimeter", name);
            return NULL;
        }

        break;
    }

    return ptr;
}

/** See the description in tapi_test.h */
const char *
test_get_param(int argc, char *argv[], const char *name)
{
    const char *ptr = NULL;
    const char *value;

    if ((ptr = test_find_param(argc, argv, name)) == NULL)
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
                     const struct param_map_entry *maps, const char *str_val,
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
        te_strlcpy(buf, "<null octet string>", sizeof(buf));
        return buf;
    }

    memset(buf, 0, sizeof(buf));

    for (i = 0; i < len; i++)
        p += sprintf(p, " 0x%02x", (unsigned int)(*oct_string++));

    return buf;
}

#define TEST_LIST_PARAM_CHUNK       8

/** See the description in tapi_test.h */
int
test_split_param_list(const char *list, char sep, char ***array_p)
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
        ptr = strchr(ptr, sep);
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
                                  (type == ndn_base_octets) ?
                                  (void *)&address_val : (void *)&int_val,
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
                addr_octets = (uint8_t *)(address_val->sa_data);

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
        WARN("No ASN configuration test parameters found");
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
        tapi_asn_param_pair *param = &creation_param[i];
        const asn_type *type;
        char *param_type = strdup(param->name);
        char *array_start = strrchr(param_type, '[');

        /* Two cases:
         * - we're substrituting an array member: a.b.[]
         * - or just a member a.b.c
         *
         * in the case of array we grab a.b and then take subtype.
         */
        if (array_start != NULL)
            *array_start = '\0';
        rc = asn_get_subtype(conf_type, &type, param_type);
        if (rc != 0)
        {
            ERROR("Failed to get subtype for %s", param_type);
            goto cleanup;
        }
        if (array_start != NULL)
            type = type->sp.subtype;

        INFO("Type for node %s found : type_name='%s'",
             param->name, type->name);
        rc = asn_parse_value_text(param->value,
                                  type,
                                  &asn_param_value, &parsed_syms);
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
        tapi_asn_param_pair *param = &change_param[i];

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

te_errno
tapi_test_args2kvpairs(int argc, char *argv[],
                       te_kvpair_h *args)
{
    int            i;
    char          *ptr = NULL;
    te_errno       rc;

    for (i = 0; i < argc; i++)
    {
        ptr = strchr(argv[i], '=');
        if (ptr == NULL)
            return TE_EINVAL;

        *ptr = '\0';

        rc = te_kvpair_add(args, argv[i], "%s", (ptr + 1));
        *ptr = '=';

        if (rc != 0)
            return rc;
    }

    return 0;
}

void
test_octet_strings2list(const char *str, unsigned int str_len,
                        uint8_t ***oct_str_list, unsigned int *list_len)
{
        char          **str_array;
        unsigned int    i;
        unsigned int    len;
        unsigned char  *oct_str;
        uint8_t       **list;

        if (str == NULL)
        {
            TEST_FAIL("%s: function input is invalid, string to "
                      "convert can't be NULL", __FUNCTION__);
        }

        len = test_split_param_list(str,
                                    TEST_LIST_PARAM_SEPARATOR,
                                    &str_array);
        if (len == 0)
            TEST_FAIL("Test parameter list returned zero parameters");

        if ((list = TE_ALLOC(len * sizeof(uint8_t *))) == NULL)
            TEST_FAIL("Failed to allocate %u bytes", len * sizeof(uint8_t *));

        for (i = 0; i < len; i++)
        {
            oct_str = test_get_octet_string_param(str_array[i], str_len);
            if (oct_str == NULL)
            {
                 TEST_FAIL("Test cannot get octet string from %s parameter",
                           str_array[i]);
            }
            list[i] = oct_str;
        }

        *list_len = len;
        *oct_str_list = list;

        free(str_array);
}

/* See description in tapi_test.h */
int
test_get_enum_param(int argc, char **argv, const char *name,
                    const struct param_map_entry *maps)
{
    const char *string_value = NULL;
    int mapped_value = 0;

    string_value = test_get_param(argc, argv, name);

    if (string_value != NULL &&
        test_map_param_value(name, maps,
                             string_value, &mapped_value) == 0)
    {
        return mapped_value;
    }

    TEST_FAIL("Enum param %s get failed", name);

    return mapped_value;
}

/* See description in tapi_test.h */
const char *
test_get_string_param(int argc, char **argv, const char *name)
{
    const char *value = NULL;

    if ((value = test_get_param(argc, argv, name)) == NULL)
    {
        TEST_FAIL("String param %s get failed", name);
    }

    return value;
}

/* See description in tapi_test.h */
char *
test_get_filename_param(int argc, char **argv, const char *name)
{
    /* argv[0] is skipped in TEST_START() */
    const char *test_path = argv[-1];
    const char *rel_path;
    char *arg0;
    char *fullpath;

    if ((rel_path = test_get_param(argc, argv, name)) == NULL)
        TEST_FAIL("Filename param '%s' get failed", name);

    if ((rel_path[0] =='\0') || (strcmp(rel_path, " ") == 0))
        return NULL;

    if (test_path[0] != '/')
        TEST_FAIL("Test path '%s' is not absolute", test_path);

    arg0 = strdup(test_path);
    if (arg0 == NULL)
        TEST_FAIL("Failed to strdup(%s)", test_path);

    fullpath = te_string_fmt("%s/%s", dirname(arg0), rel_path);
    free(arg0);

    if (fullpath == NULL)
        TEST_FAIL("Failed to make full filename");

    return fullpath;
}

static void
test_parse_range(const char *name, const char *str_val, long value_min,
                 long value_max, long *min, long *max)
{
    te_errno rc = 0;
    const char *ptr = str_val;
    char *end_ptr = NULL;

    ptr++;  /* Move ptr from opening bracket to left edge */
    rc = te_strtol_raw(ptr, &end_ptr, 0, min);
    if (rc != 0)
    {
        TEST_FAIL("Wrong range format of '%s' parameter: %s "
                  "(invalid left edge)", name, str_val);
    }
    if (*end_ptr != ',')
    {
        TEST_FAIL("Wrong range format of '%s' parameter: %s "
                  "(invalid separator)", name, str_val);
    }
    ptr = end_ptr + 1;  /* Move ptr from left edge to right edge */
    rc = te_strtol_raw(ptr, &end_ptr, 0, max);
    if (rc != 0)
    {
         TEST_FAIL("Wrong range format of '%s' parameter: %s "
                  "(invalid right edge)", name, str_val);
    }
    if (*end_ptr != ']' || *(end_ptr + 1) != '\0')
    {
         TEST_FAIL("Wrong range format of '%s' parameter: %s "
                  "(invalid closing symbol)", name, str_val);
    }
    if (*min < value_min || *min > value_max)
    {
        TEST_FAIL("The left range of '%s' parameter is too "
                  "large or too small: %s", name, str_val);
    }
    if (*max < value_min || *max > value_max)
    {
        TEST_FAIL("The right range of '%s' parameter is too "
                  "large or too small: %s", name, str_val);
    }
    if (*min > *max)
    {
        TEST_FAIL("Wrong range declaration of '%s' parameter: %s "
                  "(left edge is greater than right)", name, str_val);
    }
    if (*max - *min > RAND_MAX)
    {
        TEST_FAIL("Not supported range size of '%s' parameter: %s",
                    name, str_val);
    }
}

static int
test_get_rand_int(const char *name, const char *str_val)
{
    long min = 0;
    long max = 0;
    int value = 0;

    test_parse_range(name, str_val, INT_MIN, INT_MAX, &min, &max);
    value = (int)rand_range(min, max);
    RING("Generated int value of '%s' parameter in range [%ld,%ld] is %d",
         name, min, max, value);

    return value;
}

/* See description in tapi_test.h */
int
test_get_int_param(int argc, char **argv, const char *name)
{
    long value = 0;
    char *end_ptr = NULL;
    const char *str_val = NULL;

    str_val = test_get_param(argc, argv, name);
    if (str_val == NULL)
    {
        TEST_FAIL("Str value for name=%s was not found", name);
    }
    if (*str_val == '[')
    {
        value = test_get_rand_int(name, str_val);
    }
    else
    {
        value = strtol(str_val, &end_ptr, 0);
        if (end_ptr == str_val || *end_ptr != '\0')
        {
            TEST_FAIL("The value of '%s' parameter should be "
                  "an integer, but it is %s", name, str_val);
        }
        if (value < INT_MIN || value > INT_MAX)
        {
            TEST_FAIL("The value of '%s' parameter is too "
                  "large or too small: %s", name, str_val);
        }
    }

    return (int)value;
}

/* See description in tapi_test.h */
unsigned int
test_get_test_id(int argc, char **argv)
{
    unsigned long int value;
    const char *str_val;
    te_errno rc;

    str_val = test_get_param(argc, argv, "te_test_id");
    if (str_val == NULL)
    {
        ERROR("te_test_id parameter not found");
        return TE_LOG_ID_UNDEFINED;
    }
    rc = te_strtoul(str_val, 0, &value);
    if (rc != 0 || value > UINT_MAX)
    {
        ERROR("Cannot convert '%s' to te_test_id");
        return TE_LOG_ID_UNDEFINED;
    }

    return value;
}

static unsigned int
test_get_rand_uint(const char *name, const char *str_val)
{
    long min = 0;
    long max = 0;
    unsigned int value = 0;

    /* Right range edge greater than INT_MAX is not supported */
    test_parse_range(name, str_val, 0, INT_MAX, &min, &max);
    value = (unsigned int)rand_range(min, max);
    RING("Generated unsigned int value of '%s' parameter in range"
         "[%ld,%ld] is %u", name, min, max, value);

    return value;
}

/* See description in tapi_test.h */
unsigned int
test_get_uint_param(int argc, char **argv, const char *name)
{
    char *end_ptr = NULL;
    const char *str_val = NULL;
    unsigned long value = 0;

    str_val = test_get_param(argc, argv, name);
    CHECK_NOT_NULL(str_val);

    if (*str_val == '[')
    {
        value = test_get_rand_uint(name, str_val);
    }
    else
    {
        value = strtoul(str_val, &end_ptr, 0);
        if ((end_ptr == str_val) || (*end_ptr != '\0'))
            TEST_FAIL("Failed to convert '%s' to a number", name);

        if (value > UINT_MAX) {
            TEST_FAIL("'%s' parameter value is greater"
                      " than %u and cannot be stored in"
                      " an 'unsigned int' variable",
                      name, UINT_MAX);
        }
    }

    return (unsigned int)value;
}

/* See description in tapi_test.h */
int64_t
test_get_int64_param(int argc, char **argv, const char *name)
{
    const char *str_val = NULL;
    char *end_ptr = NULL;
    long long int value = 0;

    str_val = test_get_param(argc, argv, name);
    if (str_val == NULL)
    {
        TEST_FAIL("Failed to get int64 value for param %s", name);
    }

    value = strtoll(str_val, &end_ptr, 0);
    if ((end_ptr == str_val) || (*end_ptr != '\0'))
    {
        TEST_FAIL("The value of '%s' parameter should be "
                  "an integer, but it is %s", name,
                  str_val);
    }
    if ((value == LLONG_MAX || value == LLONG_MIN) && errno == ERANGE)
    {
        TEST_FAIL("The value of '%s' parameter is too "
                  "large or too small: %s", name, str_val);
    }

    return (int64_t)value;
}

/* See description in tapi_test.h */
double
test_get_double_param(int argc, char **argv, const char *name)
{
    const char *str_val = NULL;
    char *end_ptr = NULL;
    double value = 0.0;

    str_val = test_get_param(argc, argv, name);
    if (str_val == NULL)
    {
        TEST_FAIL("Failed to get double value for param %s", name);
    }

    value = strtod(str_val, &end_ptr);
    if (end_ptr == str_val || *end_ptr != '\0')
    {
        TEST_FAIL("The value of '%s' parameter should be "
                  "a double, but it is %s", name,
                  str_val);
    }

    return value;
}

/* See description in tapi_test.h */
double
test_get_value_unit_param(int argc, char **argv, const char *name)
{
    const char *str_val = NULL;
    te_unit     unit;

    str_val = test_get_param(argc, argv, name);
    if (str_val == NULL)
    {
        TEST_FAIL("Failed to get unit value for param %s", name);
    }

    if (te_unit_from_string(str_val, &unit) != 0)
    {
        TEST_FAIL("The value of '%s' parameter should be "
                  "convertible to double, but '%s' is not", name,
                  str_val);
    }

    return te_unit_unpack(unit);
}
