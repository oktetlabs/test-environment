/** @file
 * @brief Unix Test Agent: IPv6 router advertsement daemon radvd
 *
 * IPv6 router advertisement daemon radvd control code
 *
 * Copyright (C) 2011 Test Environment authors (see file AUTHORS
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
 * @author Konstantin G. Petrov <Konstantin.Petrov@oktetlabs.ru>
 *
 * $Id$
 */

#ifdef WITH_RADVD

#include "conf_daemons.h"
#include "radvd.h"

/*** Common variables ***/
#define COMMAND_BUFLEN  1024
static char cmd_buf[COMMAND_BUFLEN];

/*** Status variables and status control methods ***/
/* radvd admin status */
static te_bool radvd_started = FALSE;

/* Changed flag for radvd configuration */
static te_bool radvd_changed = FALSE;

/* This list keeps all managed radvd settings! */
static TAILQ_HEAD(te_radvd_interfaces, te_radvd_interface) interfaces;

/*
 * 1) Return current value of global flad 'radvd_initilaized',
 * 2) initialize queue 'interfaces' and modify flag 'radvd_initilaized'
 *    if required;
 */
static te_bool
radvd_init_check(te_bool initilaize)
{
    static te_bool  radvd_initialized = FALSE;
    te_bool         retval;

    retval = radvd_initialized;
    if (initilaize && !radvd_initialized)
    {
        TAILQ_INIT(&interfaces);

        radvd_initialized = TRUE;
    }

    return retval;
}

/*** Common utilities ***/
/*** Value convert utilities ***/
static te_radvd_optcode
te_radvd_str2optcode(const char *optstr)
{
    te_radvd_optcode    retval = OPTCODE_UNDEF;

    if (optstr == NULL) return retval;
#define CHECK_OPTSTR(_optcode) \
    else if (strcmp(optstr, OPTNAME_##_optcode) == 0) \
        retval = OPTCODE_##_optcode
    /* interface specific options */
    CHECK_OPTSTR(IF_IGNOREIFMISSING);
    CHECK_OPTSTR(IF_ADVSENDADVERT);
    CHECK_OPTSTR(IF_UNICASTONLY);
    CHECK_OPTSTR(IF_MAXRTRADVINTERVAL);
    CHECK_OPTSTR(IF_MINRTRADVINTERVAL);
    CHECK_OPTSTR(IF_MINDELAYBETWEENRAS);
    CHECK_OPTSTR(IF_ADVMANAGEDFLAG);
    CHECK_OPTSTR(IF_ADVLINKMTU);
    CHECK_OPTSTR(IF_ADVREACHABLETIME);
    CHECK_OPTSTR(IF_ADVRETRANSTIMER);
    CHECK_OPTSTR(IF_ADVCURHOPLIMIT);
    CHECK_OPTSTR(IF_ADVDEFAULTLIFETIME);
    CHECK_OPTSTR(IF_ADVDEFAULTPREFERENCE);
    CHECK_OPTSTR(IF_ADVSOURCELLADDRESS);
    CHECK_OPTSTR(IF_ADVHOMEAGENTFLAG);
    CHECK_OPTSTR(IF_ADVHOMEAGENTINFO);
    CHECK_OPTSTR(IF_HOMEAGENTLIFETIME);
    CHECK_OPTSTR(IF_HOMEAGENTPREFERENCE);
    CHECK_OPTSTR(IF_ADVMOBRTRSUPPORTFLAG);
    CHECK_OPTSTR(IF_ADVINTERVALOPT);
    /* prefix specific options */
    CHECK_OPTSTR(PREFIX_ADVONLINK);
    CHECK_OPTSTR(PREFIX_ADVAUTONOMOUS);
    CHECK_OPTSTR(PREFIX_ADVROUTERADDR);
    CHECK_OPTSTR(PREFIX_ADVVALIDLIFETIME);
    CHECK_OPTSTR(PREFIX_ADVPREFERREDLIFETIME);
    CHECK_OPTSTR(PREFIX_BASE6TO4INTERFACE);
    /* route specific options */
    CHECK_OPTSTR(ROUTE_ADVROUTELIFETIME);
    CHECK_OPTSTR(ROUTE_ADVROUTEPREFERENCE);
    /* rdnss specific options */
    CHECK_OPTSTR(RDNSS_ADVRDNSSPREFERENCE);
    CHECK_OPTSTR(RDNSS_ADVRDNSSOPEN);
    CHECK_OPTSTR(RDNSS_ADVRDNSSLIFETIME);
#undef CHECK_OPTSTR
    return retval;
}

static const char *
te_radvd_optcode2str(te_radvd_optcode optcode)
{
    if (optcode == OPTCODE_UNDEF)
        return NULL;

    switch (optcode)
    {
#define OPTCODE_CASE(_optcode) \
        case OPTCODE_##_optcode:  return OPTNAME_##_optcode
        /* Interface option */
        OPTCODE_CASE(IF_IGNOREIFMISSING);
        OPTCODE_CASE(IF_ADVSENDADVERT);
        OPTCODE_CASE(IF_UNICASTONLY);
        OPTCODE_CASE(IF_MAXRTRADVINTERVAL);
        OPTCODE_CASE(IF_MINRTRADVINTERVAL);
        OPTCODE_CASE(IF_MINDELAYBETWEENRAS);
        OPTCODE_CASE(IF_ADVMANAGEDFLAG);
        OPTCODE_CASE(IF_ADVLINKMTU);
        OPTCODE_CASE(IF_ADVREACHABLETIME);
        OPTCODE_CASE(IF_ADVRETRANSTIMER);
        OPTCODE_CASE(IF_ADVCURHOPLIMIT);
        OPTCODE_CASE(IF_ADVDEFAULTLIFETIME);
        OPTCODE_CASE(IF_ADVDEFAULTPREFERENCE);
        OPTCODE_CASE(IF_ADVSOURCELLADDRESS);
        OPTCODE_CASE(IF_ADVHOMEAGENTFLAG);
        OPTCODE_CASE(IF_ADVHOMEAGENTINFO);
        OPTCODE_CASE(IF_HOMEAGENTLIFETIME);
        OPTCODE_CASE(IF_HOMEAGENTPREFERENCE);
        OPTCODE_CASE(IF_ADVMOBRTRSUPPORTFLAG);
        OPTCODE_CASE(IF_ADVINTERVALOPT);
        /* prefix options */
        OPTCODE_CASE(PREFIX_ADVONLINK);
        OPTCODE_CASE(PREFIX_ADVAUTONOMOUS);
        OPTCODE_CASE(PREFIX_ADVROUTERADDR);
        OPTCODE_CASE(PREFIX_ADVVALIDLIFETIME);
        OPTCODE_CASE(PREFIX_ADVPREFERREDLIFETIME);
        OPTCODE_CASE(PREFIX_BASE6TO4INTERFACE);
        /* route options */
        OPTCODE_CASE(ROUTE_ADVROUTELIFETIME);
        OPTCODE_CASE(ROUTE_ADVROUTEPREFERENCE);
        /* RDNSS options */
        OPTCODE_CASE(RDNSS_ADVRDNSSPREFERENCE);
        OPTCODE_CASE(RDNSS_ADVRDNSSOPEN);
        OPTCODE_CASE(RDNSS_ADVRDNSSLIFETIME);
#undef OPTCODE_CASE
        default: return NULL;
    }
}

static te_radvd_opttype
te_radvd_optcode2opttype(te_radvd_optcode optcode)
{
    switch (optcode)
    {
        case OPTCODE_PREFIX_BASE6TO4INTERFACE:
                                        return OPTTYPE_STRING;

        case OPTCODE_IF_ADVDEFAULTPREFERENCE:
        case OPTCODE_ROUTE_ADVROUTEPREFERENCE:
                                        return OPTTYPE_PREFERENCE;

        case OPTCODE_IF_IGNOREIFMISSING:
        case OPTCODE_IF_ADVSENDADVERT:
        case OPTCODE_IF_UNICASTONLY:
        case OPTCODE_IF_ADVMANAGEDFLAG:
        case OPTCODE_IF_ADVSOURCELLADDRESS:
        case OPTCODE_IF_ADVHOMEAGENTFLAG:
        case OPTCODE_IF_ADVHOMEAGENTINFO:
        case OPTCODE_IF_ADVMOBRTRSUPPORTFLAG:
        case OPTCODE_IF_ADVINTERVALOPT:
        case OPTCODE_PREFIX_ADVONLINK:
        case OPTCODE_PREFIX_ADVAUTONOMOUS:
        case OPTCODE_PREFIX_ADVROUTERADDR:
        case OPTCODE_RDNSS_ADVRDNSSOPEN:
                                        return OPTTYPE_BOOLEAN;

        case OPTCODE_IF_MAXRTRADVINTERVAL:
        case OPTCODE_IF_MINRTRADVINTERVAL:
        case OPTCODE_IF_MINDELAYBETWEENRAS:
        case OPTCODE_IF_ADVLINKMTU:
        case OPTCODE_IF_ADVREACHABLETIME:
        case OPTCODE_IF_ADVRETRANSTIMER:
        case OPTCODE_IF_ADVCURHOPLIMIT:
        case OPTCODE_IF_ADVDEFAULTLIFETIME:
        case OPTCODE_IF_HOMEAGENTLIFETIME:
        case OPTCODE_IF_HOMEAGENTPREFERENCE:
        case OPTCODE_PREFIX_ADVVALIDLIFETIME:
        case OPTCODE_PREFIX_ADVPREFERREDLIFETIME:
        case OPTCODE_ROUTE_ADVROUTELIFETIME:
        case OPTCODE_RDNSS_ADVRDNSSPREFERENCE:
        case OPTCODE_RDNSS_ADVRDNSSLIFETIME:
                                        return OPTTYPE_INTEGER;
        case OPTCODE_UNDEF:
        default:
                                        return OPTTYPE_UNDEF;
    }
}

static te_radvd_opttype
te_radvd_str2opttype(char *optstr)
{
    return te_radvd_optcode2opttype(te_radvd_str2optcode(optstr));
}

static te_radvd_optgroup
te_radvd_optcode2optgroup(te_radvd_optcode optcode)
{
    if (optcode == OPTCODE_UNDEF)
        return OPTGROUP_UNDEF;

    switch (optcode)
    {
        case OPTCODE_IF_IGNOREIFMISSING:
        case OPTCODE_IF_ADVSENDADVERT:
        case OPTCODE_IF_UNICASTONLY:
        case OPTCODE_IF_MAXRTRADVINTERVAL:
        case OPTCODE_IF_MINRTRADVINTERVAL:
        case OPTCODE_IF_MINDELAYBETWEENRAS:
        case OPTCODE_IF_ADVMANAGEDFLAG:
        case OPTCODE_IF_ADVLINKMTU:
        case OPTCODE_IF_ADVREACHABLETIME:
        case OPTCODE_IF_ADVRETRANSTIMER:
        case OPTCODE_IF_ADVCURHOPLIMIT:
        case OPTCODE_IF_ADVDEFAULTLIFETIME:
        case OPTCODE_IF_ADVDEFAULTPREFERENCE:
        case OPTCODE_IF_ADVSOURCELLADDRESS:
        case OPTCODE_IF_ADVHOMEAGENTFLAG:
        case OPTCODE_IF_ADVHOMEAGENTINFO:
        case OPTCODE_IF_HOMEAGENTLIFETIME:
        case OPTCODE_IF_HOMEAGENTPREFERENCE:
        case OPTCODE_IF_ADVMOBRTRSUPPORTFLAG:
        case OPTCODE_IF_ADVINTERVALOPT:
                                    return OPTGROUP_INTERFACE;

        /* prefix options */
        case OPTCODE_PREFIX_ADVONLINK:
        case OPTCODE_PREFIX_ADVAUTONOMOUS:
        case OPTCODE_PREFIX_ADVROUTERADDR:
        case OPTCODE_PREFIX_ADVVALIDLIFETIME:
        case OPTCODE_PREFIX_ADVPREFERREDLIFETIME:
        case OPTCODE_PREFIX_BASE6TO4INTERFACE:
                                    return OPTGROUP_PREFIX;

        /* route options */
        case OPTCODE_ROUTE_ADVROUTELIFETIME:
        case OPTCODE_ROUTE_ADVROUTEPREFERENCE:
                                    return OPTGROUP_ROUTE;

        /* RDNSS options */
        case OPTCODE_RDNSS_ADVRDNSSPREFERENCE:
        case OPTCODE_RDNSS_ADVRDNSSOPEN:
        case OPTCODE_RDNSS_ADVRDNSSLIFETIME:
                                    return OPTGROUP_RDNSS;

        default:
                                    return OPTGROUP_UNDEF;
    }
}

static te_radvd_optgroup
te_radvd_str2optgroup(const char *optstr)
{
    return te_radvd_optcode2optgroup(te_radvd_str2optcode(optstr));
}

static preference_optval
preference_str2optval(const char *preference_str)
{
    preference_optval   retval = PREFERENCE_UNDEF;

    if (preference_str == NULL)
        return retval;
#define CHECK_PREFERENCE_STR(_preference) \
    else if (strcmp(preference_str, PREFERENCE_NAME_##_preference) == 0) \
        retval = PREFERENCE_##_preference
    CHECK_PREFERENCE_STR(LOW);
    CHECK_PREFERENCE_STR(MEDIUM);
    CHECK_PREFERENCE_STR(HIGH);
#undef CHECK_PREFERENCE_STR

    return retval;
}

static const char *
preference_optval2str(preference_optval preference)
{
    if (preference == PREFERENCE_UNDEF)
        return "";

    switch (preference)
    {
#define CHECK_PREFERENCE_CODE(_code) \
        case PREFERENCE_##_code: return PREFERENCE_NAME_##_code
        CHECK_PREFERENCE_CODE(LOW);
        CHECK_PREFERENCE_CODE(MEDIUM);
        CHECK_PREFERENCE_CODE(HIGH);
#undef CHECK_PREFERENCE_CODE
        default:
                return "";
    }
}

static te_errno
te_radvd_option2str(char *str, te_radvd_option *option)
{
    te_radvd_opttype    type;

    if (str == NULL || option == NULL)
        return TE_EINVAL;

    switch (type = te_radvd_optcode2opttype(option->code))
    {
        case OPTTYPE_BOOLEAN:
            sprintf(str, (option->boolean) ? "on" : "off");
            break;
        case OPTTYPE_PREFERENCE:
            sprintf(str, preference_optval2str(option->preference));
            break;
        case OPTTYPE_INTEGER:
            if (option->integer == -1)
                sprintf(str, "infinity");
            else
                sprintf(str, "%d", option->integer);
            break;
        case OPTTYPE_STRING:
            sprintf(str, "%s", option->string);
            break;
        default:
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    return 0;
}

static te_errno
te_radvd_str2option(te_radvd_option *option, const char *value)
{
    te_radvd_opttype type;

    if (option == NULL ||
        value == NULL ||
        strlen(value) == 0)
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    switch (type = te_radvd_optcode2opttype(option->code))
    {
        case OPTTYPE_BOOLEAN:
            if (strcmp(value, "on") == 0)
            {
                option->boolean = TRUE;
            }
            else if (strcmp(value, "off") == 0)
            {
                option->boolean = FALSE;
            }
            else
            {
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
            }
            break;
        case OPTTYPE_PREFERENCE:
            if ((option->preference =
                    preference_str2optval(value)) == PREFERENCE_UNDEF)
            {
                return TE_RC(TE_TA_UNIX, TE_EINVAL);
            }
            break;
        case OPTTYPE_INTEGER:
            if (strcmp(value, "infinity") == 0)
            {
                option->integer = -1;
            }
            else
            {
                if (sscanf(value, "%d", &option->integer) != 1)
                    return TE_RC(TE_TA_UNIX, TE_EFAULT);
            }
            break;
        case OPTTYPE_STRING:
            free(option->string);
            if ((option->string = strdup(value)) == NULL)
                return TE_RC(TE_TA_UNIX, TE_ENOMEM);
            break;
        default:
            return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    return 0;
}

/*** Search utilities ***/
/*** Look through specified interfaces to find one with given name ***/
te_radvd_interface *
find_interface(const char *ifname)
{
    te_radvd_interface     *radvd_if;

    if (ifname == NULL || strlen(ifname) == 0)
        return NULL;

    TAILQ_FOREACH(radvd_if, &interfaces, links)
    {
        if (strcmp(radvd_if->name, ifname) == 0) break;
    }

    return radvd_if;
}

/*** Look through the list of options to find one with given name ***/
te_radvd_option *
find_option(void *parent, const char *optname)
{
    te_radvd_option    *option;

    if (parent == NULL || optname == NULL || strlen(optname) == 0)
        return NULL;

    TAILQ_FOREACH(option, &((te_radvd_named_optlist *)parent)->options,
                  links)
    {
        if (strcmp(option->name, optname) == 0) break;
    }

    return option;
}

/*** Look through the list of IPv6 addrs to find one with given name ***/
te_radvd_ip6_addr *
find_addr(void *parent, const char *addrname)
{
    te_radvd_ip6_addr  *addr;

    if (parent == NULL || addrname == NULL || strlen(addrname) == 0)
        return NULL;

    TAILQ_FOREACH(addr, &((te_radvd_named_optlist *)parent)->addrs, links)
    {
        if (strcmp(addr->name, addrname) == 0) break;
    }

    return addr;
}

/*
 * Functions
 * find_preficess(te_radvd_interface *radvd_if, const char *name)
 * find_routes(te_radvd_interface *, const char *)
 * find_rdnss(te_radvd_interface *, const char *)
 * to look through lists of prefices, subnets and routes
 * and find unit with given name.
 */
#define FIND_SUBNET_VALUE(_field_name) \
static te_radvd_subnet *                                            \
find_##_field_name(te_radvd_interface *radvd_if, const char *name)  \
{                                                                   \
    te_radvd_subnet   *subnet;                                      \
                                                                    \
    if (radvd_if == NULL || name == NULL || strlen(name) == 0)      \
        return NULL;                                                \
                                                                    \
    TAILQ_FOREACH(subnet, &radvd_if->_field_name, links)            \
    {                                                               \
        if (strcmp(subnet->name, name) == 0)                        \
            break;                                                  \
    }                                                               \
                                                                    \
    return subnet;                                                  \
}

FIND_SUBNET_VALUE(prefices)

FIND_SUBNET_VALUE(routes)

FIND_SUBNET_VALUE(rdnss)
#undef FIND_SUBNET_VALUE

/*** Allocated memory management ***/
static te_radvd_option *
new_option(const char *optname)
{
    te_radvd_option *option;

    if ((option = calloc(1, sizeof(option))) != NULL)
    {
        if ((option->name = strdup(optname)) == NULL)
        {
            free(option);
            option = NULL;
        }
    }

    return option;
}

static void
free_option(te_radvd_option *opt_ptr)
{
    if (opt_ptr == NULL)
        return;

    if (opt_ptr->name != NULL)
        free(opt_ptr->name);

    if (te_radvd_optcode2opttype(opt_ptr->code) == OPTTYPE_STRING &&
        opt_ptr->string != NULL)
    {
        free(opt_ptr->string);
    }

    free(opt_ptr);
}

static te_radvd_ip6_addr *
new_ip6_addr(const char *name)
{
    te_radvd_ip6_addr *addr;

    if ((addr = calloc(1, sizeof(*addr))) != NULL)
    {
        if ((addr->name = strdup(name)) == NULL)
        {
            free(addr);
            return NULL;
        }
    }

    return addr;
}

static void
free_ip6_addr(te_radvd_ip6_addr *addr_ptr)
{
    if (addr_ptr == NULL)
        return;

    if (addr_ptr->name != NULL)
        free(addr_ptr->name);

    free(addr_ptr);
}

static void
free_optlist(void *parent)
{
    te_radvd_option *ptr;

    while ((ptr =
            TAILQ_FIRST(&((te_radvd_named_optlist *)parent)->options))
                                                                != NULL)
    {
        TAILQ_REMOVE(&((te_radvd_named_optlist *)parent)->options,
                     ptr, links);
        free_option(ptr);
    }
}

static void
free_addrlist(void *parent)
{
    te_radvd_ip6_addr *ptr;

    while ((ptr =
            TAILQ_FIRST(&((te_radvd_named_optlist *)parent)->addrs))
                                                                != NULL)
    {
        TAILQ_REMOVE(&((te_radvd_named_optlist *)parent)->addrs,
                     ptr, links);
        free_ip6_addr(ptr);
    }
}

/*
 * Tricks to simplify managing:
 * 1.1) list of clients in interface specification
 * 1.2) list of options in inreface specification
 * 2.1) address/prefix in prefix specification
 * 2.2) list of options in prefix specificetion
 * 3.1) address/prefix in route specification
 * 3.2) list of options in route specification
 * 4.1) list of addresses in RDNSS specification
 * 4.2) list of options in RDNSS specification
 *
 * 1) The same strucrure te_radvd_subnet is used to
 *    keep prefix, route and RDNSS specifications
 * 2) Field 'addrs' in the structure te_radvd_subnet points
 *    to the list of structures te_radvd_ip6_addr. Structure
 *    te_radvd_ip6_addr can keep IPv6 address in the forms of
 *    string and in6_addr structure. It can keep integer prefix
 *    value too.
 *    In case of subnet or route specification list of addresses
 *    contains only one element which keeps integer prefix value and
 *    IPv6 address.
 *    In case of RDNSS specification this list keeps one or more IPv6
 *    addresses.
 * 3) Structures te_radvd_interface, te_radvd_subnet and
 *    te_radvd_named_optlist have the same head
 *    ...{
 *          char                                   *name;
 *          TAILQ_HEAD( , te_radvd_option)          options;
 *          TAILQ_HEAD( , te_radvd_ip6_addr)        addrs;
 *          ...
 *          ...
 *    } ...
 *    functions managing list of options and addresses are called with
 *    void *ptr parameter and use it as (te_radvd_named_optlist *)ptr
 * 4) Function new_subnet allocs one element in the list 'addrs'
 *    and fills subnet->name and addr->name with duplication
 *    of the same string value 'name' (parameter of the function).
 *    This makes managing with prefix and route specifications more
 *    convinient.
 */
static te_radvd_subnet *
new_subnet(const char *name)
{
    te_radvd_subnet    *subnet;
    te_radvd_ip6_addr  *addr;

    if ((subnet = calloc(1, sizeof(*subnet))) != NULL)
    {
        if ((subnet->name = strdup(name)) == NULL)
        {
            free(subnet);
            return NULL;
        }

        TAILQ_INIT(&subnet->options);
        TAILQ_INIT(&subnet->addrs);

        /* Trick. See above */
        if ((addr = calloc(1, sizeof(*addr))) == NULL)
        {
            free(subnet->name);
            free(subnet);
            return NULL;
        }

        if ((addr->name = strdup(name)) == NULL)
        {
            free(addr);
            free(subnet->name);
            free(subnet);
            return NULL;
        }
        TAILQ_INSERT_TAIL(&subnet->addrs, addr, links);
        /* */
    }

    return subnet;
}

static void
free_subnet(te_radvd_subnet *ptr)
{
    if (ptr->name != NULL)
        free(ptr->name);

    free_optlist(ptr);
    free_addrlist(ptr);

    free(ptr);
}

static void
free_interface(te_radvd_interface *radvd_if)
{
    te_radvd_subnet *ptr;

    if (radvd_if->name != NULL)
        free(radvd_if->name);

    free_optlist(radvd_if);
    free_addrlist(radvd_if);

#define FREE_SUBNETS(_listname) \
    while ((ptr = TAILQ_FIRST(&radvd_if->_listname)) != NULL) {             \
        TAILQ_REMOVE(&radvd_if->_listname, (te_radvd_subnet *)ptr, links);  \
        free_subnet(ptr);                                                   \
    }
    FREE_SUBNETS(prefices)

    FREE_SUBNETS(routes)

    FREE_SUBNETS(rdnss)
#undef FREE_SUBNETS

    free(radvd_if);
}

/*** radvd managing utilities ***/
/*** Is radvd running? ***/
#define FIND_PID_FILE \
    do {                                            \
        TE_SPRINTF(cmd_buf, TE_RADVD_FIND_CMD);     \
        rc = ta_system(cmd_buf);                    \
    } while (0)
static te_bool
ds_radvd_is_run(void)
{
    int     rc = 0;

    FIND_PID_FILE;
    if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
        return FALSE;

    sprintf(cmd_buf,
            PS_ALL_PID_ARGS
            " | grep $(cat " TE_RADVD_PID_FILENAME ") | "
            "grep -q " TE_RADVD_EXECUTABLE_FILENAME " >/dev/null 2>&1");

    rc = ta_system(cmd_buf);

    return !(rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0);
}

/*** Stop radvd server ***/
static te_errno
ds_radvd_stop(void)
{
    int     rc = 0;

    ENTRY("%s()", __FUNCTION__);

    /* Check if tester's radvd pid file exists. Return success if not. */
    FIND_PID_FILE;
    if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
        return 0;

    /* We've found pid file. Try to kill process with given PID. */
    TE_SPRINTF(cmd_buf, TE_RADVD_STOP_CMD);
    rc = ta_system(cmd_buf);
    if (rc < 0 || !WIFEXITED(rc) || WEXITSTATUS(rc) != 0)
    {
        ERROR("Command '%s' failed, rc=%r", cmd_buf, rc);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}
#undef FIND_PID_FILE

/*** Save configuration to the file ***/
static int
ds_radvd_save_conf(void)
{
    FILE                                       *f = NULL;
    te_radvd_interface                         *interface;
    te_radvd_subnet                            *subnet;
    te_bool                                     empty_cfg;

    INFO("%s()", __FUNCTION__);

    if ((f = fopen(TE_RADVD_CONF_FILENAME, "w"))  == NULL)
    {
        ERROR("Failed to open '"
              TE_RADVD_CONF_FILENAME
              "' for writing: %s", strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    empty_cfg = TRUE;
    TAILQ_FOREACH(interface, &interfaces, links)
    {
        const char         *ignore_if_missing = IGNOREIFMISSING_DFLT;
        const char         *adv_send_advert = ADVSENDADVERT_DFLT;
        int                 max_rtr_adv_interval = MAXRTRADVINTERVAL_DFLT;
        te_radvd_option    *option;
        te_radvd_ip6_addr  *addr;
        te_bool             no_prefices;
        /*
         * Detect wrong interface configuration:
         */
        if (interface->name == NULL || strlen(interface->name) == 0)
            continue;
        /*
         * Valid interface specification must have at leas one valid
         * prefix specification;
         */
        no_prefices = TRUE;
        TAILQ_FOREACH(subnet, &interface->prefices, links)
        {
#define VALIDATE_SUBNET \
            if (subnet->name == NULL ||                         \
                strlen(subnet->name) == 0 ||                    \
                (addr = TAILQ_FIRST(&subnet->addrs)) == NULL || \
                addr->name == NULL ||                           \
                strlen(addr->name) == 0 ||                      \
                addr->prefix < 0 ||                             \
                addr->prefix > 128) continue
            VALIDATE_SUBNET;

            /* At least one valid prefix specification found */
            no_prefices = FALSE;
            break;
        }

        if (no_prefices)
            continue;

        fprintf(f, IF_CFG_HEAD, interface->name);

        /* interface options */
        ignore_if_missing = IGNOREIFMISSING_DFLT;
        adv_send_advert = ADVSENDADVERT_DFLT;
        max_rtr_adv_interval = MAXRTRADVINTERVAL_DFLT;
        TAILQ_FOREACH(option, &interface->options, links)
        {
            switch (option->code)
            {
                case OPTCODE_IF_IGNOREIFMISSING:
                    ignore_if_missing = (option->boolean) ? "on" : "off";
                    break;
                case OPTCODE_IF_ADVSENDADVERT:
                    adv_send_advert = (option->boolean) ? "on" : "off";
                    break;
                case OPTCODE_IF_MAXRTRADVINTERVAL:
                    max_rtr_adv_interval = option->integer;
                    break;
                default:
                    /* FIXME unrecognised option is skipped silently */
                    if (te_radvd_option2str(cmd_buf, option) == 0)
                    {
                        fprintf(f,
                                CFG_BLOCK_SPACE "%s" " %s;\n",
                                option->name, cmd_buf);
                    }
            }
        }

        /* interface options with defaults */
        fprintf(f,
                CFG_BLOCK_SPACE OPTNAME_IF_IGNOREIFMISSING " %s;\n",
                ignore_if_missing);
        fprintf(f,
                CFG_BLOCK_SPACE OPTNAME_IF_ADVSENDADVERT " %s;\n",
                adv_send_advert);
        fprintf(f,
                CFG_BLOCK_SPACE OPTNAME_IF_MAXRTRADVINTERVAL " %d;\n",
                max_rtr_adv_interval);

        /*
         * List of subnets. We have already verified it to
         * be have at least one valid specification */
        TAILQ_FOREACH(subnet, &interface->prefices, links)
        {
            const char *adv_on_link;
            const char *adv_autonomous;
            VALIDATE_SUBNET;

            fprintf(f, PREFIX_BLOCK_HEAD,
                    addr->name, addr->prefix);

            adv_on_link = ADVONLINK_DFLT;
            adv_autonomous = ADVAUTONOMOUS_DFLT;
            /* subnet options */
            TAILQ_FOREACH(option, &subnet->options, links)
            {
                switch (option->code)
                {
                    case OPTCODE_PREFIX_ADVONLINK:
                        adv_on_link = (option->boolean) ? "on" : "off";
                        break;
                    case OPTCODE_PREFIX_ADVAUTONOMOUS:
                        adv_autonomous = (option->boolean) ? "on" : "off";
                        break;
                    default:
                        /* FIXME unrecognised option is skipped silently */
                        if (te_radvd_option2str(cmd_buf, option) == 0)
                        {
                            fprintf(f,
                                    CFG_BLOCK_SPACE
                                    CFG_BLOCK_SPACE
                                    "%s" " %s;\n",
                                    option->name, cmd_buf);
                        }
                }
            }

            /* subnet options with defaults */
            fprintf(f,
                    CFG_BLOCK_SPACE
                    CFG_BLOCK_SPACE
                    OPTNAME_PREFIX_ADVONLINK
                    " %s;\n",
                    adv_on_link);
            fprintf(f,
                    CFG_BLOCK_SPACE
                    CFG_BLOCK_SPACE
                    OPTNAME_PREFIX_ADVAUTONOMOUS
                    " %s;\n",
                    adv_autonomous);

            fprintf(f, CFG_BLOCK_TAIL);
        }

        /* list of routes */
        TAILQ_FOREACH(subnet, &interface->routes, links)
        {
            VALIDATE_SUBNET;
#undef VALIDATE_SUBNET

            fprintf(f, ROUTE_BLOCK_HEAD,
                    addr->name, addr->prefix);

            /* route options */
#define FILL_SUBNET_OPTIONS \
            TAILQ_FOREACH(option, &subnet->options, links)              \
                /* FIXME unrecognised option is skipped silently */     \
                if (te_radvd_option2str(cmd_buf, option) == 0)          \
                    fprintf(f,                                          \
                            CFG_BLOCK_SPACE                             \
                            CFG_BLOCK_SPACE                             \
                            "%s" " %s;\n",                            \
                            option->name, cmd_buf)
            FILL_SUBNET_OPTIONS;

            fprintf(f, CFG_BLOCK_TAIL);
        }

        /* list of RDNSS */
        TAILQ_FOREACH(subnet, &interface->rdnss, links)
        {
            if (subnet->name == NULL || strlen(subnet->name) == 0)
                continue;

            fprintf(f, RDNSS_BLOCK_HEAD, subnet->name);

            /* RDNSS options */
            FILL_SUBNET_OPTIONS;
#undef FILL_SUBNET_OPTIONS
            fprintf(f, CFG_BLOCK_TAIL);
        }

        /* list of clients */
        if (TAILQ_FIRST(&interface->addrs) != NULL)
        {
            te_bool no_clients;

            no_clients = TRUE;
            TAILQ_FOREACH(addr, &interface->addrs, links)
            {
#define VALIDATE_ADDR \
                if(addr->name == NULL || strlen(addr->name) == 0) continue
                VALIDATE_ADDR;

                no_clients = FALSE;
                break;
            }

            if (!no_clients)
            {
                fprintf(f, CLIENTS_BLOCK_HEAD);
                TAILQ_FOREACH(addr, &interface->addrs, links)
                {
                    VALIDATE_ADDR;
#undef VALIDATE_ADDR

                    fprintf(f,
                            CFG_BLOCK_SPACE
                            CFG_BLOCK_SPACE
                            "%s;\n", addr->name);
                }
                fprintf(f, CFG_BLOCK_TAIL);
            }
        }

        fprintf(f, IF_CFG_TAIL);

        empty_cfg = FALSE;
    }

    if (empty_cfg)
    {
        /* Noting was configured or wrong configuration */
        fprintf(f, TE_RADVD_EMPTY_CFG);
    }

    if (fsync(fileno(f)) != 0)
    {
        int err = errno;

        ERROR("%s(): fsync() failed: %s", __FUNCTION__, strerror(err));
        (void)fclose(f);
        return TE_OS_RC(TE_TA_UNIX, err);
    }

    if (fclose(f) != 0)
    {
        ERROR("%s(): fclose() failed: %s", __FUNCTION__, strerror(errno));
        return TE_OS_RC(TE_TA_UNIX, errno);
    }

    return 0;
}

/*** start radvd server ***/
static te_errno
ds_radvd_start(void)
{
    te_errno rc = 0;

    ENTRY("%s()", __FUNCTION__);
    INFO("%s()", __FUNCTION__);

    if ((rc = ds_radvd_save_conf()) != 0)
    {
        ERROR("Failed to save radvd configuration file");
        return rc;
    }

    TE_SPRINTF(cmd_buf, TE_RADVD_START_CMD);
    if (ta_system(cmd_buf) != 0)
    {
        ERROR("Failed to start radvd, command '%s'", cmd_buf);
        return TE_RC(TE_TA_UNIX, TE_ESHCMD);
    }

    return 0;
}

/*** Configurator methods ***/

/*
 * Subtree /agent/
 */
/*** Node radvd methods ***/
/* Get radvd state on/off */
static te_errno
ds_radvd_get(unsigned int gid, const char *oid, char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    INFO("%s()", __FUNCTION__);

    radvd_init_check(TRUE);

    strcpy(value, ds_radvd_is_run() ? "1" : "0");

    return 0;
}

/* Set radvd state on/off */
static te_errno
ds_radvd_set(unsigned int gid, const char *oid, const char *value)
{
    UNUSED(gid);
    UNUSED(oid);

    ENTRY("%s(): value=%s", __FUNCTION__, value);

    INFO("%s()", __FUNCTION__);

    radvd_init_check(TRUE);

    radvd_started = (strcmp(value, "1") == 0);
    if (radvd_started != ds_radvd_is_run())
    {
        radvd_changed = TRUE;
    }

    return 0;
}

/* Turn radvd on/off and modify configuration */
static te_errno
ds_radvd_commit(unsigned int gid, const char *oid)
{
    te_errno rc = 0;

    UNUSED(gid);
    UNUSED(oid);
    ENTRY("%s()", __FUNCTION__);
    INFO("%s()", __FUNCTION__);

    radvd_init_check(TRUE);

    /*
     * We don't need to change state of radvd:
     * The current state is the same as desired.
     */
    if (!radvd_changed)
        return 0;

    /* Stop radvd */
    if (ds_radvd_is_run())
    {
        if ((rc = ds_radvd_stop()) != 0)
        {
            if (ds_radvd_is_run())
            {
                ERROR("Failed to stop radvd");
                return rc;
            }
        }
    }

    /* (Re)start radvd if necessary */
    if (radvd_started)
    {
        if ((rc = ds_radvd_start()) != 0)
        {
            ERROR("Failed to start radvd");
            return rc;
        }
    }

    radvd_changed = FALSE;

    return rc;
}

/*
 * Subtree /agent/radvd/
 */
/*
 * Configurator methods use this macro to check if interface with given
 * name exists in configuration structure. Value _expect_to_find
 * defines action.
 *
 * _expect_to_find  interface_exists
 * TRUE             TRUE            method goes further
 * FALSE            FALSE           -//-
 * TRUE             FALSE           method returns with TE_ENOENT
 * FALSE            TRUE            method returns with TE_EEXIST
 */
#define FIND_RADVD_IF(_expect_to_find) \
    if (((radvd_if = find_interface(ifname)) != NULL &&                 \
                                        !_expect_to_find) ||            \
        (radvd_if == NULL &&                                            \
                                        _expect_to_find))               \
        return TE_RC(TE_TA_UNIX,                                        \
                     (_expect_to_find) ? TE_ENOENT : TE_EEXIST)

#define LIST_UNITS(_head, _element) \
    do {                                                                \
        *cmd_buf = '\0';                                                \
        TAILQ_FOREACH((_element), &(_head), links)                      \
        {                                                               \
            sprintf(cmd_buf + strlen(cmd_buf), "%s ",                   \
                    (_element)->name);                                  \
        }                                                               \
                                                                        \
        return (*list = strdup(cmd_buf)) == NULL ?                      \
                                    TE_RC(TE_TA_UNIX, TE_ENOMEM) : 0;   \
    } while (0)
/*** Node radvd/interface methods ***/
static te_errno
ds_interface_add(unsigned int gid, const char *oid,
                 const char *value, const char *radvd,
                 const char *ifname)
{
    te_radvd_interface     *radvd_if;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(FALSE);

    if ((radvd_if = calloc(1, sizeof(*radvd_if))) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((radvd_if->name = strdup(ifname)) == NULL)
    {
        free(radvd_if);
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);
    }

    TAILQ_INIT(&radvd_if->options);
    TAILQ_INIT(&radvd_if->addrs);
    TAILQ_INIT(&radvd_if->prefices);
    TAILQ_INIT(&radvd_if->routes);
    TAILQ_INIT(&radvd_if->rdnss);

    TAILQ_INSERT_TAIL(&interfaces, radvd_if, links);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_interface_del(unsigned int gid, const char *oid,
                 const char *radvd, const char *ifname)
{
    te_radvd_interface  *radvd_if;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    TAILQ_REMOVE(&interfaces, radvd_if, links);

    free_interface(radvd_if);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_interface_list(unsigned int gid, const char *oid, char **list,
                  const char *radvd)
{
    te_radvd_interface  *radvd_if;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    LIST_UNITS(interfaces, radvd_if);
}

/*
 * Subtree /agent/radvd/interface/
 */
/*** node radvd/interface/options methods ***/
static te_errno
ds_interface_option_get(unsigned int gid, const char *oid,
                        char *value,
                        const char *radvd,
                        const char *ifname,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_option    *option;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((option = find_option(radvd_if, optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return te_radvd_option2str(value, option);
}

static te_errno
ds_interface_option_set(unsigned int gid, const char *oid,
                        const char *value,
                        const char *radvd,
                        const char *ifname,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_option    *option;
    te_errno            retval;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    FIND_RADVD_IF(TRUE);

    if ((option = find_option(radvd_if, optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((retval = te_radvd_str2option(option, value)) == 0)
        radvd_changed = TRUE;

    return retval;
}

static te_errno
ds_interface_option_add(unsigned int gid, const char *oid,
                        const char *value,
                        const char *radvd,
                        const char *ifname,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_option    *option;
    te_errno            retval;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    if (optname == NULL || value == NULL ||
        strlen(optname) == 0 || strlen(value) == 0)
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    FIND_RADVD_IF(TRUE);

    if ((option = find_option(radvd_if, optname)) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (te_radvd_str2optgroup(optname) != OPTGROUP_INTERFACE)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if ((option = new_option(optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((retval = te_radvd_str2option(option, value)) != 0)
    {
        free_option(option);
        return TE_RC(TE_TA_UNIX, retval);
    }

    TAILQ_INSERT_TAIL(&radvd_if->options, option, links);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_interface_option_del(unsigned int gid, const char *oid,
                        const char *radvd,
                        const char *ifname,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_option    *option;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    FIND_RADVD_IF(TRUE);

    if ((option = find_option(radvd_if, optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TAILQ_REMOVE(&radvd_if->options, option, links);

    free_option(option);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_interface_option_list(unsigned int gid, const char *oid,
                         char **list,
                        const char *radvd,
                        const char *ifname)
{
    te_radvd_interface *radvd_if;
    te_radvd_option    *option;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    FIND_RADVD_IF(TRUE);

    LIST_UNITS(radvd_if->options, option);
}

/*** node radvd/interface/prefix methods ***/
static te_errno
ds_prefix_add(unsigned int gid, const char *oid,
                        const char *value,
                        const char *radvd,
                        const char *ifname,
                        const char *prefix_name)
{
    te_radvd_interface     *radvd_if;
    te_radvd_subnet        *prefix;
    te_radvd_ip6_addr      *addr;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((prefix = find_prefices(radvd_if, prefix_name)) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if ((prefix = new_subnet(prefix_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    addr = TAILQ_FIRST(&prefix->addrs);

    if (sscanf(value, "%d", &addr->prefix) != 1 ||
        inet_pton(AF_INET6, addr->name, &addr->addr) != 1)
    {
        free_subnet(prefix);
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    TAILQ_INSERT_TAIL(&radvd_if->prefices, prefix, links);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_prefix_del(unsigned int gid, const char *oid,
                        const char *radvd,
                        const char *ifname,
                        const char *prefix_name)
{
    te_radvd_interface     *radvd_if;
    te_radvd_subnet        *prefix;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((prefix = find_prefices(radvd_if, prefix_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TAILQ_REMOVE(&radvd_if->prefices, prefix, links);

    free_subnet(prefix);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_prefix_list(unsigned int gid, const char *oid,
                         char **list,
                        const char *radvd,
                        const char *ifname)
{
    te_radvd_interface     *radvd_if;
    te_radvd_subnet        *prefix;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    LIST_UNITS(radvd_if->prefices, prefix);
}

/*** node radvd/interface/route methods ***/
static te_errno
ds_route_add(unsigned int gid, const char *oid,
                        const char *value,
                        const char *radvd,
                        const char *ifname,
                        const char *route_name)
{
    te_radvd_interface     *radvd_if;
    te_radvd_subnet        *route;
    te_radvd_ip6_addr      *addr;
    te_errno                retval;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((route = find_routes(radvd_if, route_name)) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if ((route = new_subnet(route_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    addr = TAILQ_FIRST(&route->addrs);

    if (sscanf(value, "%d", &addr->prefix) != 1 ||
        inet_pton(AF_INET6, addr->name, &addr->addr) != 1)
    {
        free_subnet(route);
        return TE_RC(TE_TA_UNIX, retval);
    }

    TAILQ_INSERT_TAIL(&radvd_if->routes, route, links);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_route_del(unsigned int gid, const char *oid,
                        const char *radvd,
                        const char *ifname,
                        const char *route_name)
{
    te_radvd_interface     *radvd_if;
    te_radvd_subnet        *route;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((route = find_routes(radvd_if, route_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TAILQ_REMOVE(&radvd_if->routes, route, links);

    free_subnet(route);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_route_list(unsigned int gid, const char *oid,
                         char **list,
                        const char *radvd,
                        const char *ifname)
{
    te_radvd_interface     *radvd_if;
    te_radvd_subnet        *route;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    LIST_UNITS(radvd_if->routes, route);
}

/*** node radvd/interface/rdnss methods ***/
static te_errno
ds_rdnss_add(unsigned int gid, const char *oid,
                        const char *value,
                        const char *radvd,
                        const char *ifname,
                        const char *rdnss_name)
{
    te_radvd_interface     *radvd_if;
    te_radvd_subnet        *rdnss;
    te_radvd_ip6_addr      *addr;
    char                   *token;
    te_errno                retval;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);
    UNUSED(value);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((rdnss = find_rdnss(radvd_if, rdnss_name)) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if ((rdnss = new_subnet(rdnss_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    /*
     * Function new_subnet allocates one ip6 address in the list
     * subnet->addrs after creating new subnet structure. Fields
     * subnet->name and addr->name are created
     * by duplicating the same string (argument of the function new_subnet).
     * Motivation of this trick see in the function new_subnet code.
     */
    free_addrlist(rdnss);

    do {
        token = strtok(rdnss->name, " ");

        retval = 0;
        while (token != NULL) {
            do {
                if ((addr = new_ip6_addr(token)) == NULL)
                {
                    retval = TE_ENOMEM;
                    break;
                }

                if (inet_pton(AF_INET6, token, &addr->addr) != 1)
                {
                    free_ip6_addr(addr);
                    retval = TE_EINVAL;
                }
            } while (0);

            if (retval != 0)
                break;

            TAILQ_INSERT_TAIL(&rdnss->addrs, addr, links);

            token = strtok(NULL, " ");
        }

        if (retval != 0)
            break;

        /*
         * We need it because function next_token modifies
         * content of memory pointed by rdnss->name
         */
        strcpy(rdnss->name, rdnss_name);
    } while (0);

    if (retval != 0)
    {
        free_subnet(rdnss);
        return TE_RC(TE_TA_UNIX, retval);
    }

    TAILQ_INSERT_TAIL(&radvd_if->rdnss, rdnss, links);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_rdnss_del(unsigned int gid, const char *oid,
                        const char *radvd,
                        const char *ifname,
                        const char *rdnss_name)
{
    te_radvd_interface     *radvd_if;
    te_radvd_subnet        *rdnss;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((rdnss = find_rdnss(radvd_if, rdnss_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TAILQ_REMOVE(&radvd_if->rdnss, rdnss, links);

    free_subnet(rdnss);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_rdnss_list(unsigned int gid, const char *oid,
                         char **list,
                        const char *radvd,
                        const char *ifname)
{
    te_radvd_interface     *radvd_if;
    te_radvd_subnet        *rdnss;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    LIST_UNITS(radvd_if->rdnss, rdnss);
}

/*** node radvd/interface/clients methods ***/
/*
 * Node 'clients' has no name. Its value looks like
 * string ""|IPv6[' 'IPv6[' 'IPv6[...]]] (empty string
 * or list of IPv6 addresses in string format).
 * In structure te_radvd_interface this value is keeped in the
 * field 'clients'. It looks like list of of structures
 * te_radvd_ip6_addr. Functions get/set makes convertion between
 * string and list
 */
static te_errno
ds_clients_get(unsigned int gid, const char *oid,
               char *value,
               const char *radvd,
               const char *ifname)
{
    te_radvd_interface *radvd_if;
    te_radvd_ip6_addr  *client;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    *value = '\0';
    TAILQ_FOREACH(client, &radvd_if->addrs, links)
    {
        sprintf(value + strlen(value), "%s ",  client->name);
    }

    return 0;
}

static te_errno
ds_clients_set(unsigned int gid, const char *oid,
               const char *value,
               const char *radvd,
               const char *ifname)
{
    te_radvd_interface *radvd_if;
    te_radvd_ip6_addr  *addr;
    char               *clients;
    char               *token;
    te_errno            retval;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    /* Firt cleanup existing list of clients */
    free_addrlist(radvd_if);

    /*
     * Extract IPv6 address substrings from 'value' and create new
     * list of clients
     */
    if ((clients = strdup(value)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    token = strtok(clients, " ");

    retval = 0;
    while (token != NULL) {
        do {
            if ((addr = new_ip6_addr(token)) == NULL)
            {
                retval = TE_ENOMEM;
                break;
            }

            if (inet_pton(AF_INET6, token, &addr->addr) != 1)
            {
                free_ip6_addr(addr);
                retval = TE_EINVAL;
            }
        } while (0);

        if (retval != 0)
            break;

        TAILQ_INSERT_TAIL(&radvd_if->addrs, addr, links);

        token = strtok(NULL, " ");
    }

    if (retval != 0)
    {
        free_addrlist(radvd_if);
        free(clients);
        return TE_RC(TE_TA_UNIX, retval);
    }

    radvd_changed = TRUE;

    return 0;
}

/*
 * Subtree /agent/radvd/interface/prefix
 */
/*** Node /agent/radvd/interface/prefix/options methods ***/
static te_errno
ds_prefix_option_get(unsigned int gid, const char *oid,
                        char *value,
                        const char *radvd,
                        const char *ifname,
                        const char *prefix_name,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_subnet    *prefix;
    te_radvd_option    *option;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((prefix = find_prefices(radvd_if, prefix_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((option = find_option(prefix, optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return te_radvd_option2str(value, option);
}

static te_errno
ds_prefix_option_set(unsigned int gid, const char *oid,
                        const char *value,
                        const char *radvd,
                        const char *ifname,
                        const char *prefix_name,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_subnet    *prefix;
    te_radvd_option    *option;
    te_errno            retval;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((prefix = find_prefices(radvd_if, prefix_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((option = find_option(prefix, optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((retval = te_radvd_str2option(option, value)) == 0)
        radvd_changed = TRUE;

    return retval;
}

static te_errno
ds_prefix_option_add(unsigned int gid, const char *oid,
                        const char *value,
                        const char *radvd,
                        const char *ifname,
                        const char *prefix_name,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_subnet    *prefix;
    te_radvd_option    *option;
    te_errno            retval;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    if (optname == NULL || value == NULL ||
        strlen(optname) == 0 || strlen(value) == 0)
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    FIND_RADVD_IF(TRUE);

    if ((prefix = find_prefices(radvd_if, prefix_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((option = find_option(prefix, optname)) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (te_radvd_str2optgroup(optname) != OPTGROUP_PREFIX)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if ((option = new_option(optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((retval = te_radvd_str2option(option, value)) != 0)
    {
        free_option(option);
        return TE_RC(TE_TA_UNIX, retval);
    }

    TAILQ_INSERT_TAIL(&prefix->options, option, links);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_prefix_option_del(unsigned int gid, const char *oid,
                        const char *radvd,
                        const char *ifname,
                        const char *prefix_name,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_subnet    *prefix;
    te_radvd_option    *option;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((prefix = find_prefices(radvd_if, prefix_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((option = find_option(prefix, optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TAILQ_REMOVE(&prefix->options, option, links);

    free_option(option);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_prefix_option_list(unsigned int gid, const char *oid,
                         char **list,
                        const char *radvd,
                        const char *ifname,
                        const char *prefix_name)
{
    te_radvd_interface *radvd_if;
    te_radvd_subnet    *prefix;
    te_radvd_option    *option;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    FIND_RADVD_IF(TRUE);

    if ((prefix = find_prefices(radvd_if, prefix_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_UNITS(prefix->options, option);
}

/*
 * Subtree /agent/radvd/interface/route
 */
/*** Node /agent/radvd/interface/route/options methods ***/
static te_errno
ds_route_option_get(unsigned int gid, const char *oid,
                        char *value,
                        const char *radvd,
                        const char *ifname,
                        const char *route_name,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_subnet    *route;
    te_radvd_option    *option;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((route = find_routes(radvd_if, route_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((option = find_option(route, optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return te_radvd_option2str(value, option);
}

static te_errno
ds_route_option_set(unsigned int gid, const char *oid,
                        const char *value,
                        const char *radvd,
                        const char *ifname,
                        const char *route_name,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_subnet    *route;
    te_radvd_option    *option;
    te_errno            retval;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((route = find_routes(radvd_if, route_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((option = find_option(route, optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((retval = te_radvd_str2option(option, value)) == 0)
        radvd_changed = TRUE;

    return retval;
}

static te_errno
ds_route_option_add(unsigned int gid, const char *oid,
                        const char *value,
                        const char *radvd,
                        const char *ifname,
                        const char *route_name,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_subnet    *route;
    te_radvd_option    *option;
    te_errno            retval;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    if (optname == NULL || value == NULL ||
        strlen(optname) == 0 || strlen(value) == 0)
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    FIND_RADVD_IF(TRUE);

    if ((route = find_routes(radvd_if, route_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((option = find_option(route, optname)) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (te_radvd_str2optgroup(optname) != OPTGROUP_ROUTE)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if ((option = new_option(optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((retval = te_radvd_str2option(option, value)) != 0)
    {
        free_option(option);
        return TE_RC(TE_TA_UNIX, retval);
    }

    TAILQ_INSERT_TAIL(&route->options, option, links);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_route_option_del(unsigned int gid, const char *oid,
                        const char *radvd,
                        const char *ifname,
                        const char *route_name,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_subnet    *route;
    te_radvd_option    *option;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((route = find_routes(radvd_if, route_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((option = find_option(route, optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TAILQ_REMOVE(&route->options, option, links);

    free_option(option);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_route_option_list(unsigned int gid, const char *oid,
                         char **list,
                        const char *radvd,
                        const char *ifname,
                        const char *route_name)
{
    te_radvd_interface *radvd_if;
    te_radvd_subnet    *route;
    te_radvd_option    *option;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    FIND_RADVD_IF(TRUE);

    if ((route = find_routes(radvd_if, route_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_UNITS(route->options, option);
}

/*
 * Subtree /agent/radvd/interface/rdnss
 */
/*** Node /agent/radvd/interface/rdnss/options methods ***/
static te_errno
ds_rdnss_option_get(unsigned int gid, const char *oid,
                        char *value,
                        const char *radvd,
                        const char *ifname,
                        const char *rdnss_name,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_subnet    *rdnss;
    te_radvd_option    *option;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((rdnss = find_rdnss(radvd_if, rdnss_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((option = find_option(rdnss, optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return te_radvd_option2str(value, option);
}

static te_errno
ds_rdnss_option_set(unsigned int gid, const char *oid,
                        const char *value,
                        const char *radvd,
                        const char *ifname,
                        const char *rdnss_name,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_subnet    *rdnss;
    te_radvd_option    *option;
    te_errno            retval;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((rdnss = find_rdnss(radvd_if, rdnss_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((option = find_option(rdnss, optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((retval = te_radvd_str2option(option, value)) == 0)
        radvd_changed = TRUE;

    return retval;
}

static te_errno
ds_rdnss_option_add(unsigned int gid, const char *oid,
                        const char *value,
                        const char *radvd,
                        const char *ifname,
                        const char *rdnss_name,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_subnet    *rdnss;
    te_radvd_option    *option;
    te_errno            retval;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    if (optname == NULL || value == NULL ||
        strlen(optname) == 0 || strlen(value) == 0)
    {
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    FIND_RADVD_IF(TRUE);

    if ((rdnss = find_rdnss(radvd_if, rdnss_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((option = find_option(rdnss, optname)) != NULL)
        return TE_RC(TE_TA_UNIX, TE_EEXIST);

    if (te_radvd_str2optgroup(optname) != OPTGROUP_RDNSS)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

    if ((option = new_option(optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    if ((retval = te_radvd_str2option(option, value)) != 0)
    {
        free_option(option);
        return TE_RC(TE_TA_UNIX, retval);
    }

    TAILQ_INSERT_TAIL(&rdnss->options, option, links);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_rdnss_option_del(unsigned int gid, const char *oid,
                        const char *radvd,
                        const char *ifname,
                        const char *rdnss_name,
                        const char *optname)
{
    te_radvd_interface *radvd_if;
    te_radvd_subnet    *rdnss;
    te_radvd_option    *option;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    radvd_init_check(TRUE);

    FIND_RADVD_IF(TRUE);

    if ((rdnss = find_rdnss(radvd_if, rdnss_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((option = find_option(rdnss, optname)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    TAILQ_REMOVE(&rdnss->options, option, links);

    free_option(option);

    radvd_changed = TRUE;

    return 0;
}

static te_errno
ds_rdnss_option_list(unsigned int gid, const char *oid,
                         char **list,
                        const char *radvd,
                        const char *ifname,
                        const char *rdnss_name)
{
    te_radvd_interface *radvd_if;
    te_radvd_subnet    *rdnss;
    te_radvd_option    *option;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(radvd);

    FIND_RADVD_IF(TRUE);

    if ((rdnss = find_rdnss(radvd_if, rdnss_name)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    LIST_UNITS(rdnss->options, option);
}

/*** Configuration subtree ***/
/* radvd subtree layout relations: left - son, down - brother
 *
 * radvd - inteface - option
 *                      |
 *                    prefix - option
 *                      |
 *                    route - option
 *                      |
 *                    rdnss - option
 *                      |
 *                    clients
 */
/*** prefix, route and rdnss options ***/
/* No need son definition: options to be end node everywhere */
#define OPTIONS_NODE(_father, _brother) \
static rcf_pch_cfg_object node_ds_##_father##_options =             \
    { "option", 0, NULL, _brother,                                 \
      (rcf_ch_cfg_get)ds_##_father##_option_get,                    \
      (rcf_ch_cfg_set)ds_##_father##_option_set,                    \
      (rcf_ch_cfg_add)ds_##_father##_option_add,                    \
      (rcf_ch_cfg_del)ds_##_father##_option_del,                    \
      (rcf_ch_cfg_list)ds_##_father##_option_list, NULL, NULL }
    OPTIONS_NODE(prefix, NULL);
    OPTIONS_NODE(route, NULL);
    OPTIONS_NODE(rdnss, NULL);

/*** interface options, subnet, route, rdnss and clients nodes ***/
static rcf_pch_cfg_object node_ds_clients =
    { "clients", 0, NULL, NULL,
      (rcf_ch_cfg_get)ds_clients_get,
      (rcf_ch_cfg_set)ds_clients_set,
      NULL, NULL, NULL, NULL, NULL };

#define INTERFACE_NODE(_name, _brother) \
static rcf_pch_cfg_object node_ds_##_name =                         \
    { #_name, 0, &node_ds_##_name##_options, _brother,              \
      NULL, NULL,                                                   \
      (rcf_ch_cfg_add)ds_##_name##_add,                             \
      (rcf_ch_cfg_del)ds_##_name##_del,                             \
      (rcf_ch_cfg_list)ds_##_name##_list, NULL, NULL }
    INTERFACE_NODE(rdnss, &node_ds_clients);
    INTERFACE_NODE(route, &node_ds_rdnss);
    INTERFACE_NODE(prefix, &node_ds_route);
    OPTIONS_NODE(interface, &node_ds_prefix);
/*** interface node ***/
/*
 * radvd configuration looks like sequence of uniformal records
 *
 * interface <ifname>
 * {
 *      <interface settings>
 * }
 *
 * Each record represents interface being served with its specific
 * service settings. Strictly one record per one interface.
 *
 * Functions ds_radvd_inteface_* do not parse radvd configuration file.
 * configuration records are represented in the list named 'iterfaces'
 * (see above). Configuration file is created/modified with given contents
 * of the list 'interfaces' when function radvd_commit is called.
 */
    INTERFACE_NODE(interface, NULL);
#undef INTERFACE_NODE
#undef OPTIONS_NODE

/*** subtree root ***/
/*
 * Node to control fadvd server execution and (re)configuration
 *
 * 1) ds_radvd_get:     Find running radvd executable started by tester
 * 2) ds_radvd_set:     Postponed start/stop of radvd. Real start/stop
 *                      in ds_radvd_commit.
 * 3) ds_radvd_commit:  Real start/stop/restart and reconfiguring radvd
 */
static rcf_pch_cfg_object node_ds_radvd =
    { "radvd", 0,
      &node_ds_interface, NULL,
      (rcf_ch_cfg_get)ds_radvd_get,
      (rcf_ch_cfg_set)ds_radvd_set,
      NULL, NULL, NULL,
      (rcf_ch_cfg_commit)ds_radvd_commit, NULL };

/*** radvd grab and release functions ***/
te_errno
radvd_grab(const char *name)
{
    int rc = 0;

    UNUSED(name);

    radvd_init_check(TRUE);

    if (ds_radvd_is_run())
    {
        if ((rc = ds_radvd_stop()) != 0)
        {
            if (ds_radvd_is_run())
            {
                ERROR("Failed to stop radvd server");
                return rc;
            }
        }
    }

    radvd_started = FALSE;
    radvd_changed = FALSE;

    if ((rc = rcf_pch_add_node("/agent", &node_ds_radvd)) != 0)
        return rc;

    return 0;
}

te_errno
radvd_release(const char *name)
{
    te_radvd_interface     *radvd_if;
    te_errno                rc = 0;

    UNUSED(name);

    if (!radvd_init_check(FALSE))
        return 0;

    if ((rc = rcf_pch_del_node(&node_ds_radvd)) != 0)
        return rc;

    if (unlink(TE_RADVD_CONF_FILENAME) != 0 && errno != ENOENT)
    {
        ERROR("Failed to delete radvd configuration file '"
              TE_RADVD_CONF_FILENAME
              "': %s", strerror(errno));
    }

    while ((radvd_if = TAILQ_FIRST(&interfaces)) != NULL)
    {
        TAILQ_REMOVE(&interfaces, radvd_if, links);
        free_interface(radvd_if);
    }

    return 0;
}

#endif /* WITH_RADVD */
