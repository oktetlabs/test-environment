/** @file
 * @brief Unix Test Agent: IPv6 router advertisement daemon radvd
 *
 * Definitions used in code to control IPv6 router
 * advertisement daemon radvd.
 *
 * Copyright (C) 2011 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution).
 *
 * @author Konstantin G. Petrov <Konstantin.Petrov@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_TA_UNIX_RADVD_H__
#define __TE_TA_UNIX_RADVD_H__

#include "te_defs.h"
#include "te_queue.h"

/* radvd executable filename and radvd username */
#define TE_RADVD_EXECUTABLE_FILENAME    "/usr/sbin/radvd"
#define TE_RADVD_USERNAME               "root"

/* Files used by tester to control radvd */
#define TE_RADVD_CONF_FILENAME          "/tmp/te.radvd.conf"
#define TE_RADVD_PID_FILENAME           "/tmp/te.radvd.pid"

/*
 * Console commands used by configurator to control radvd
 *
 * 1) start radvd
 */
#define TE_RADVD_START_CMD              TE_RADVD_EXECUTABLE_FILENAME    \
                                        " -C "  TE_RADVD_CONF_FILENAME  \
                                        " -p "  TE_RADVD_PID_FILENAME   \
                                        " -u "  TE_RADVD_USERNAME       \
                                        " 1>&2 2>/dev/null"
/* 2) find cmd */
#define TE_RADVD_FIND_CMD               "cat "                          \
                                        TE_RADVD_PID_FILENAME           \
                                        " 2>/dev/null 1>/dev/null"
/* 2) stop radvd */
#define TE_RADVD_STOP_CMD               "kill `cat "                    \
                                        TE_RADVD_PID_FILENAME           \
                                        "`"
/* 3) read config file and restart with new settings */
#define TE_RADVD_RESTART_CMD            "kill -s HUP `cat "             \
                                        TE_RADVD_PID_FILENAME           \
                                        "`"

/* Option names */
/* 1) radvd interface configuration options */
#define OPTNAME_IF_IGNOREIFMISSING          "IgnoreIfMissing"
#define OPTNAME_IF_ADVSENDADVERT            "AdvSendAdvert"
#define OPTNAME_IF_UNICASTONLY              "UnicastOnly"
#define OPTNAME_IF_MAXRTRADVINTERVAL        "MaxRtrAdvInterval"
#define OPTNAME_IF_MINRTRADVINTERVAL        "MinRtrAdvInterval"
#define OPTNAME_IF_MINDELAYBETWEENRAS       "MinDelayBetweenRAs"
#define OPTNAME_IF_ADVMANAGEDFLAG           "AdvManagedFlag"
#define OPTNAME_IF_ADVLINKMTU               "AdvLinkMTU"
#define OPTNAME_IF_ADVREACHABLETIME         "AdvReachableTime"
#define OPTNAME_IF_ADVRETRANSTIMER          "AdvRetransTimer"
#define OPTNAME_IF_ADVCURHOPLIMIT           "AdvCurHopLimit"
#define OPTNAME_IF_ADVDEFAULTLIFETIME       "AdvDeafultLifetime"
#define OPTNAME_IF_ADVDEFAULTPREFERENCE     "AdvDeafultPreference"
#define OPTNAME_IF_ADVSOURCELLADDRESS       "AdvSourceLLAddress"
#define OPTNAME_IF_ADVHOMEAGENTFLAG         "AdvHomeAgentFlag"
#define OPTNAME_IF_ADVHOMEAGENTINFO         "AdvHomeAgentInfo"
#define OPTNAME_IF_HOMEAGENTLIFETIME        "HomeAgentLifetime"
#define OPTNAME_IF_HOMEAGENTPREFERENCE      "HomeAgentPreference"
#define OPTNAME_IF_ADVMOBRTRSUPPORTFLAG     "AdvMobRtrSupportFlag"
#define OPTNAME_IF_ADVINTERVALOPT           "AdvIntervalOpt"
/* 2) radvd prefix specific options */
#define OPTNAME_PREFIX_ADVONLINK            "AdvOnLink"
#define OPTNAME_PREFIX_ADVAUTONOMOUS        "AdvAutonomous"
#define OPTNAME_PREFIX_ADVROUTERADDR        "AdvRouterAddr"
#define OPTNAME_PREFIX_ADVVALIDLIFETIME     "AdvValidLifetime"
#define OPTNAME_PREFIX_ADVPREFERREDLIFETIME "AdvPreferredLifetime"
#define OPTNAME_PREFIX_BASE6TO4INTERFACE    "Base6to4Interface"
/* 3) radvd route specific options */
#define OPTNAME_ROUTE_ADVROUTELIFETIME      "AdvRouteLifetime"
#define OPTNAME_ROUTE_ADVROUTEPREFERENCE    "AdvRoutePreference"
/* 4) radvd RDNSS specific options */
#define OPTNAME_RDNSS_ADVRDNSSPREFERENCE    "AdvRDNSSPreference"
#define OPTNAME_RDNSS_ADVRDNSSOPEN          "AdvRDNSSOpen"
#define OPTNAME_RDNSS_ADVRDNSSLIFETIME      "AdvRDNSSLifetime"

/* names of preference enumerated value variants */
#define PREFERENCE_NAME_LOW     "low"
#define PREFERENCE_NAME_MEDIUM  "medium"
#define PREFERENCE_NAME_HIGH    "high"

/* configuration file blocks */
/* 1) radvd interface configuration block */
#define IF_CFG_HEAD             "interface %s\n{\n"
#define IF_CFG_TAIL             "};\n"
#define CFG_BLOCK_SPACE         "    "
#define CFG_BLOCK_TAIL          CFG_BLOCK_SPACE "};\n"
/* 2) radvd prefix configuration block header */
#define PREFIX_BLOCK_HEAD \
                                CFG_BLOCK_SPACE                         \
                                "prefix %s/%d\n" /* IPv6 addr/prefix */ \
                                CFG_BLOCK_SPACE                         \
                                "{\n"
/* 3) radvd route configuration block header */
#define ROUTE_BLOCK_HEAD \
                                CFG_BLOCK_SPACE                         \
                                "route %s/%d\n" /* IPv6 addr/prefix*/   \
                                CFG_BLOCK_SPACE                         \
                                "{\n"
/* 4) radvd RDNSS configuration block header */
#define RDNSS_BLOCK_HEAD \
                                CFG_BLOCK_SPACE                         \
                                "RDNSS %s\n" /* list of IPv6 addrs */   \
                                CFG_BLOCK_SPACE                         \
                                "{\n"
/* 5) radvd clients block header */
#define CLIENTS_BLOCK_HEAD      CFG_BLOCK_SPACE "clients {\n"

/* Configuration defaults */
#define IGNOREIFMISSING_DFLT    "on"
#define ADVSENDADVERT_DFLT      "on"
#define MAXRTRADVINTERVAL_DFLT  60 /*
                                    * Experimentally defined value to make
                                    * dynamic IPv6 connection wake up in
                                    * reasonable time <~ 1 minute
                                    */

/* radvd prefix default options */
#define ADVONLINK_DFLT          "on"
#define ADVAUTONOMOUS_DFLT      "on"

/*
 * Contents of confguration file used to start radvd with no
 * configuration settings defined in configuration tree.
 * Use as configuration string in functions like printf/sprintf.
 */
#define TE_RADVD_EMPTY_CFG      "interface .\n{\n"                      \
                                CFG_BLOCK_SPACE "IgnoreIfMissing on;\n" \
                                IF_CFG_TAIL

/*** typedefs ***/
/*** Option codes: to distinguish options by their codes ***/
typedef enum {
    /* Undefined */
    OPTCODE_UNDEF = 0,  /* option_name = "foobar"|""|NULL */

    /* Interface option */
    OPTCODE_IF_IGNOREIFMISSING,
    OPTCODE_IF_ADVSENDADVERT,
    OPTCODE_IF_UNICASTONLY,
    OPTCODE_IF_MAXRTRADVINTERVAL,
    OPTCODE_IF_MINRTRADVINTERVAL,
    OPTCODE_IF_MINDELAYBETWEENRAS,
    OPTCODE_IF_ADVMANAGEDFLAG,
    OPTCODE_IF_ADVLINKMTU,
    OPTCODE_IF_ADVREACHABLETIME,
    OPTCODE_IF_ADVRETRANSTIMER,
    OPTCODE_IF_ADVCURHOPLIMIT,
    OPTCODE_IF_ADVDEFAULTLIFETIME,
    OPTCODE_IF_ADVDEFAULTPREFERENCE,
    OPTCODE_IF_ADVSOURCELLADDRESS,
    OPTCODE_IF_ADVHOMEAGENTFLAG,
    OPTCODE_IF_ADVHOMEAGENTINFO,
    OPTCODE_IF_HOMEAGENTLIFETIME,
    OPTCODE_IF_HOMEAGENTPREFERENCE,
    OPTCODE_IF_ADVMOBRTRSUPPORTFLAG,
    OPTCODE_IF_ADVINTERVALOPT,

    /* prefix options */
    OPTCODE_PREFIX_ADVONLINK,
    OPTCODE_PREFIX_ADVAUTONOMOUS,
    OPTCODE_PREFIX_ADVROUTERADDR,
    OPTCODE_PREFIX_ADVVALIDLIFETIME,
    OPTCODE_PREFIX_ADVPREFERREDLIFETIME,
    OPTCODE_PREFIX_BASE6TO4INTERFACE,

    /* route options */
    OPTCODE_ROUTE_ADVROUTELIFETIME,
    OPTCODE_ROUTE_ADVROUTEPREFERENCE,

    /* RDNSS options */
    OPTCODE_RDNSS_ADVRDNSSPREFERENCE,
    OPTCODE_RDNSS_ADVRDNSSOPEN,
    OPTCODE_RDNSS_ADVRDNSSLIFETIME
} te_radvd_optcode;

/*** Option types: specify how to manage option value ***/
typedef enum {
    OPTTYPE_UNDEF = 0,      /* do not know what is it */
    OPTTYPE_BOOLEAN,        /* "on"|"off" */
    OPTTYPE_PREFERENCE,     /* "low"|"medium"|"high" */
    OPTTYPE_INTEGER,        /* "infinity"|<str_value_scanned_to_integer> */
    OPTTYPE_STRING          /* <string_value> */
} te_radvd_opttype;

/*** Option groups: specify group of settings given option belongs to ***/
typedef enum {
    OPTGROUP_UNDEF = 0,     /* do not know what is it */
    OPTGROUP_INTERFACE,     /* interface options */
    OPTGROUP_PREFIX,        /* prefix options */
    OPTGROUP_ROUTE,         /* route options */
    OPTGROUP_RDNSS          /* RDNSS options */
} te_radvd_optgroup;

/*** Preference value: code representing medium option choice ***/
typedef enum {
    PREFERENCE_UNDEF = 0,   /* "foobar"|""|NULL */
    PREFERENCE_LOW,         /* "low" */
    PREFERENCE_MEDIUM,      /* "medium" */
    PREFERENCE_HIGH         /* "high" */
} preference_optval;

/*** To keep lists of options ***/
typedef struct te_radvd_option {
    char                           *name;

    TAILQ_ENTRY(te_radvd_option)    links;

    te_radvd_optcode                code;
    union {
        te_bool                     boolean;
        preference_optval           preference;
        int                         integer;
        char                       *string;
    };
} te_radvd_option;

/*** To keep lists of IPv6 addresses ***/
typedef struct te_radvd_ip6_addr {
    char                                                   *name;

    TAILQ_ENTRY(te_radvd_ip6_addr)                          links;

    struct in6_addr                                         addr;
    int                                                     prefix;
} te_radvd_ip6_addr;

/*
 * Used to manage lists of options and addresses
 * in interface|subnet|route|rdnss specifications
 */
typedef struct te_radvd_named_oplist {
    char                                                   *name;
    TAILQ_HEAD(optlist, te_radvd_option)                    options;
    TAILQ_HEAD(addrlist, te_radvd_ip6_addr)                 addrs;
} te_radvd_named_optlist;

/*** To keep interface subnet|route|rdnss specifications ***/
typedef struct te_radvd_subnet {
    char                                                   *name;
    TAILQ_HEAD(subnet_options, te_radvd_option)             options;
    TAILQ_HEAD(subnet_addrs, te_radvd_ip6_addr)             addrs;

    TAILQ_ENTRY(te_radvd_subnet)                            links;
} te_radvd_subnet;

/*** To keep interface specifications ***/
typedef struct te_radvd_interface {
    char                                                   *name;
    TAILQ_HEAD(interface_options, te_radvd_option)          options;
    TAILQ_HEAD(interface_clients, te_radvd_ip6_addr)        addrs;

    TAILQ_ENTRY(te_radvd_interface)                         links;

    TAILQ_HEAD(interface_prefices, te_radvd_subnet)         prefices;
    TAILQ_HEAD(interface_routes, te_radvd_subnet)           routes;
    TAILQ_HEAD(interface_rdnss, te_radvd_subnet)            rdnss;
} te_radvd_interface;

#endif /* !__TE_TA_UNIX_RADVD_H__ */
