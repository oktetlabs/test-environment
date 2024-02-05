/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023 OKTET Labs Ltd. All rights reserved. */
/** @file
 * @brief TAPI to manage Cisco TRex
 *
 * TAPI to manage *Cisco TRex*.
 */

#define TE_LGR_USER "TAPI CISCO TREX"

#include <signal.h>
#include <regex.h>
#include <netinet/in.h>

#include "tapi_trex.h"
#include "tapi_file.h"
#include "tapi_job_opt.h"
#include "tapi_cfg_pci.h"
#include "tapi_cfg_base.h"
#include "tapi_rpc_unistd.h"
#include "tapi_job_internal.h"

#include "te_str.h"
#include "te_bufs.h"
#include "te_enum.h"
#include "te_file.h"
#include "te_alloc.h"
#include "te_units.h"
#include "te_expand.h"
#include "te_kvpair.h"
#include "te_mi_log.h"
#include "te_sockaddr.h"

#include "conf_oid.h"

/** TRex dummy interface name. */
#define TAPI_TREX_DUMMY "dummy"

/** TRex ASTF config path format string. */
#define TAPI_TREX_ASTF_CONF_FMT "/tmp/astf%s%s.json"

/** TRex YAML config with PCI-based interfaces path format string. */
#define TAPI_TREX_CFG_YAML_FMT "/tmp/%s.yaml"

/** TRex default timeout. */
#define TAPI_TREX_TIMEOUT_MS 10000

/** TRex interface description. */
struct tapi_trex_interface {
    /**
     * Interface to use.
     * It can be PCI address, interface name or @p TAPI_TREX_DUMMY.
     */
    char *if_name;
    /** If @c TRUE then interface will be bound. */
    te_bool need_to_bind;
};

/** TRex YAML config file template. */
static const char *default_trex_cfg =
"- port_limit      : ${#IFACES}\n"
"  version         : 2\n"
"  interfaces: [${IFACES[, ]}]\n"
"  low_end: true\n"
"  port_info:\n"
"${PORTINFO_IP*    - ip${COLON} ${PORTINFO_IP[${}]}\n"
"      default_gw${COLON} ${PORTINFO_DEFAULT_GW[${}]}\n}";

/** Mapping of possible values for tapi_trex_opt::verbose option. */
static const te_enum_map tapi_trex_verbose_mapping[] = {
    {.name = "1", .value = TAPI_TREX_VERBOSE_MODE_MIN},
    {.name = "3", .value = TAPI_TREX_VERBOSE_MODE_MAX},
    TE_ENUM_MAP_END
};

/* Possible TRex command line arguments. */
static const tapi_job_opt_bind trex_args_binds[] = TAPI_JOB_OPT_SET(
    TAPI_JOB_OPT_DUMMY("-f"),
    TAPI_JOB_OPT_DUMMY("default.py"),
    TAPI_JOB_OPT_DUMMY("--astf"),
    TAPI_JOB_OPT_BOOL("--astf-server-only", tapi_trex_opt, astf_server_only),
    TAPI_JOB_OPT_UINT_T("-c", FALSE, NULL, tapi_trex_opt, n_threads),
    TAPI_JOB_OPT_BOOL("--tso-disable", tapi_trex_opt, tso_disable),
    TAPI_JOB_OPT_BOOL("--lro-disable", tapi_trex_opt, lro_disable),
    TAPI_JOB_OPT_DOUBLE("-d", FALSE, NULL, tapi_trex_opt, duration),
    TAPI_JOB_OPT_BOOL("--flip", tapi_trex_opt, asymmetric_traffic_flow),
    TAPI_JOB_OPT_BOOL("--hdrh", tapi_trex_opt, use_hdr_histograms),
    TAPI_JOB_OPT_BOOL("--ipv6", tapi_trex_opt, ipv6),
    TAPI_JOB_OPT_UINT_T("-m", FALSE, NULL, tapi_trex_opt, rate_multiplier),
    TAPI_JOB_OPT_BOOL("--nc", tapi_trex_opt, force_close_at_end),
    TAPI_JOB_OPT_BOOL("--no-flow-control-change", tapi_trex_opt,
                      enable_flow_control),
    TAPI_JOB_OPT_BOOL("--no-watchdog", tapi_trex_opt, no_watchdog),
    TAPI_JOB_OPT_BOOL("--rt", tapi_trex_opt, use_realtime_prio),
    TAPI_JOB_OPT_BOOL("-pubd", tapi_trex_opt, no_monitors),
    TAPI_JOB_OPT_BOOL("--queue-drop", tapi_trex_opt, dont_resend_pkts),
    TAPI_JOB_OPT_BOOL("--sleeps", tapi_trex_opt, use_sleep),
    TAPI_JOB_OPT_ENUM("-v", FALSE, tapi_trex_opt, verbose,
                      tapi_trex_verbose_mapping),
    TAPI_JOB_OPT_UINT_T("-w", FALSE, NULL, tapi_trex_opt, init_wait_sec),
    TAPI_JOB_OPT_STRING("--prefix", FALSE, tapi_trex_opt, instance_prefix)
);

/* Default values of TRex common configuration. */
#define TAPI_TREX_COMMON_CONFIG_DEFAULT \
    {                                           \
        .ip_range_beg = NULL,                   \
        .ip_range_end = NULL,                   \
        .ip_offset = NULL,                      \
        .port = TAPI_JOB_OPT_UINT_UNDEF,        \
        .payload = NULL,                        \
        .interface = NULL,                      \
        .ip = NULL,                             \
        .gw = NULL                              \
     }

/* Default values of TRex client configuration. */
const struct tapi_trex_client_config tapi_trex_client_config_default = {
    .common = TAPI_TREX_COMMON_CONFIG_DEFAULT
};

/* Default values of TRex server configuration. */
const struct tapi_trex_server_config tapi_trex_server_config_default = {
    .common = TAPI_TREX_COMMON_CONFIG_DEFAULT
};

/* Default values of TRex command line arguments. */
const tapi_trex_opt tapi_trex_default_opt = {
    .astf_server_only = FALSE,
    .n_threads = TAPI_JOB_OPT_UINT_UNDEF,
    .tso_disable = FALSE,
    .lro_disable = FALSE,
    .duration = TAPI_JOB_OPT_DOUBLE_UNDEF,
    .asymmetric_traffic_flow = FALSE,
    .use_hdr_histograms = FALSE,
    .ipv6 = FALSE,
    .rate_multiplier = TAPI_JOB_OPT_UINT_UNDEF,
    .force_close_at_end = FALSE,
    .enable_flow_control = FALSE,
    .no_watchdog = FALSE,
    .use_realtime_prio = FALSE,
    .no_monitors = FALSE,
    .dont_resend_pkts = FALSE,
    .use_sleep = FALSE,
    .verbose  = TAPI_TREX_VERBOSE_NONE,
    .init_wait_sec = TAPI_JOB_OPT_UINT_UNDEF,
    .instance_prefix = NULL,
    .clients = NULL,
    .servers = NULL,
    .astf_vars = NULL,
    .driver = NULL,
    .astf_template = NULL,
    .trex_exec = NULL
};

static void
tapi_trex_gen_clients_astf_conf(tapi_trex_client_config **clients,
                                te_kvpair_h *kvpairs)
{
    size_t i;

    if (clients == NULL || kvpairs == NULL)
        return;

    for (i = 0; clients[i] != NULL; i++)
    {
        const tapi_trex_client_config *client = clients[i];

        const char *payload = client->common.payload != NULL ?
                              client->common.payload :
                              TAPI_TREX_DEFAULT_CLIENT_HTTP_PAYLOAD;

        char *ip_range_beg = te_ip2str(client->common.ip_range_beg);
        char *ip_range_end = te_ip2str(client->common.ip_range_end);
        char *ip_offset = te_ip2str(client->common.ip_offset);

        unsigned port = client->common.port.defined ?
                        client->common.port.value : 80;

        te_kvpair_push(kvpairs, "CLIENT_HTTP", "%s", payload);

        te_kvpair_push(kvpairs, "CLIENT_IP_START", "%s",
                       ip_range_beg != NULL ? ip_range_beg : "0.0.0.0");
        te_kvpair_push(kvpairs, "CLIENT_IP_END", "%s",
                       ip_range_end != NULL ? ip_range_end : "0.0.0.0");
        te_kvpair_push(kvpairs, "CLIENT_IP_OFFSET", "%s",
                       ip_offset != NULL ? ip_offset : "0.0.0.0");

        te_kvpair_push(kvpairs, "CLIENT_IP_PORT", "%u", port);

        free(ip_range_beg);
        free(ip_range_end);
        free(ip_offset);
    }
}

static void
tapi_trex_gen_servers_astf_conf(tapi_trex_server_config **servers,
                                te_kvpair_h *kvpairs)
{
    size_t i;

    if (servers == NULL || kvpairs == NULL)
        return;

    for (i = 0; servers[i] != NULL; i++)
    {
        const tapi_trex_server_config *server = servers[i];

        const char *payload = server->common.payload != NULL ?
                              server->common.payload :
                              TAPI_TREX_DEFAULT_SERVER_HTTP_PAYLOAD;

        char *ip_range_beg = te_ip2str(server->common.ip_range_beg);
        char *ip_range_end = te_ip2str(server->common.ip_range_end);
        char *ip_offset = te_ip2str(server->common.ip_offset);

        unsigned port = server->common.port.defined ?
                        server->common.port.value : 80;

        te_kvpair_push(kvpairs, "SERVER_HTTP", "%s", payload);

        te_kvpair_push(kvpairs, "SERVER_IP_START", "%s",
                       ip_range_beg != NULL ? ip_range_beg : "0.0.0.0");
        te_kvpair_push(kvpairs, "SERVER_IP_END", "%s",
                       ip_range_end != NULL ? ip_range_end : "0.0.0.0");
        te_kvpair_push(kvpairs, "SERVER_IP_OFFSET", "%s",
                       ip_offset != NULL ? ip_offset : "0.0.0.0");

        te_kvpair_push(kvpairs, "SERVER_IP_PORT", "%u", port);

        free(ip_range_beg);
        free(ip_range_end);
        free(ip_offset);
    }
}

/**
 * Generate TRex ASTF config file.
 *
 * @param[in] ta    Name of the Test Agent.
 * @param[in] opt   TRex options.
 *
 * @return Status code.
 */
static te_errno
tapi_trex_gen_astf_config(const char *ta, const tapi_trex_opt *opt)
{
    te_errno rc;
    te_string template = TE_STRING_INIT;

    char astf_json_path[RCF_MAX_PATH];
    te_bool prefix_is_empty = te_str_is_null_or_empty(opt->instance_prefix);

    te_kvpair_h kvpairs;
    te_kvpair_init(&kvpairs);

    rc = te_string_append(&template, "%s", opt->astf_template);
    if (rc != 0)
    {
        rc = TE_RC(TE_TAPI, rc);
        ERROR("Failed to read TRex ASTF template: %r", rc);
        goto cleanup;
    }

    tapi_trex_gen_clients_astf_conf(opt->clients, &kvpairs);
    tapi_trex_gen_servers_astf_conf(opt->servers, &kvpairs);

    if (opt->astf_vars != NULL)
        te_kvpairs_copy(&kvpairs, opt->astf_vars);

    rc = te_snprintf(astf_json_path, sizeof(astf_json_path),
                     TAPI_TREX_ASTF_CONF_FMT,
                     prefix_is_empty ? "" : "-",
                     prefix_is_empty ? "" : opt->instance_prefix);
    if (rc != 0)
    {
        rc = TE_RC_UPSTREAM(TE_TAPI, rc);
        ERROR("Failed to generate TRex ASTF config file name: %r", rc);
        goto cleanup;
    }

    rc = tapi_file_expand_kvpairs(ta, template.ptr, NULL, &kvpairs,
                                  astf_json_path);
    if (rc != 0)
    {
        ERROR("Failed to create TRex ASTF config '%s': %r", astf_json_path, rc);
        goto cleanup;
    }

cleanup:

    te_string_free(&template);
    te_kvpair_fini(&kvpairs);

    return rc;
}

/**
 * Initialize @p tapi_trex_interface structure.
 *
 * @param[in] name  Name of the interface, PCI address, or @c NULL.
 * @param[in] bind  Whether it's necessary to bind the interface.
 *
 * @return Allocated @p tapi_trex_interface structure.
 *
 * @note Function result should be tapi_trex_interface_free()'d.
 *
 * @note @p name can be:
 *              - interface name, e.g. @c 'eth0'
 *              - PCI address, e.g. @c '0000:00:04.0'
 *              - @c NULL
 */
static tapi_trex_interface *
tapi_trex_interface_init(const char *name, te_bool bind)
{
    tapi_trex_interface *interface = TE_ALLOC(sizeof(tapi_trex_interface));

    if (te_str_is_null_or_empty(name))
    {
        interface->if_name = NULL;
        return interface;
    }

    interface->if_name = strdup(name);
    interface->need_to_bind = bind;

    return interface;
}

/**
 * Bind PCI port for DPDK usage.
 *
 * @param[in] ta        Name of the Test Agent.
 * @param[in] pci_addr  PCI device address.
 * @param[in] driver    Driver name to be bound to PCI.
 *                      Should be @c NULL if interface is already bound.
 *
 * @return Status code.
 */
static te_errno
tapi_trex_bind_pci_addr(const char *ta,
                        const char *pci_addr,
                        const char *driver)
{
    te_errno rc;
    cfg_handle handle;

    char *oid = NULL;
    char *pci = NULL;

    if (driver == NULL)
        return 0;

    if (ta == NULL || pci_addr == NULL)
    {
        ERROR("TA name or PCI address can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = cfg_find_fmt(&handle, "/agent:%s/hardware:/pci:/device:%s",
                      ta, pci_addr);
    if (rc != 0)
    {
        ERROR("Failed to find PCI OID: %r", rc);
        goto cleanup;
    }

    rc = cfg_get_oid_str(handle, &oid);
    if (rc != 0)
    {
        ERROR("Failed to get PCI OID from handle: %r", rc);
        goto cleanup;
    }

    rc = tapi_cfg_pci_bind_driver(oid, driver);
    if (rc != 0)
    {
        ERROR("Failed to bind '%s' using driver '%s': %r", oid, driver, rc);
        goto cleanup;
    }

cleanup:

    free(oid);
    free(pci);

    return rc;
}

/**
 * Return PCI address of @p ta's @p interface.
 *
 * @param ta            Test Agent name.
 * @param interface     Interface name (e.g. @c 'eth0').
 *
 * @return Allocated PCI addr (e.g. @c "0000:00:04.0").
 *
 * @note Result of the function should be free()'d.
 */
static char *
tapi_trex_get_pci_addr(const char *ta, const char *interface)
{
    te_errno rc;

    char *pci_oid = NULL;
    char *iface = NULL;

    cfg_handle *pci_handles = NULL;
    char *pci_addr = NULL;

    unsigned int pci_n;
    unsigned int pci_i;

    rc = cfg_find_pattern_fmt(&pci_n, &pci_handles,
                              "/agent:%s/hardware:/pci:/device:*", ta);
    if (rc != 0)
    {
        ERROR("Failed to get %s's PCI devices: %r", ta, rc);
        goto cleanup;
    }

    for (pci_i = 0; pci_i < pci_n; pci_i++)
    {
        rc = cfg_get_oid_str(pci_handles[pci_i], &pci_oid);
        if (rc != 0)
        {
            ERROR("Failed to get PCI OID from handle: %r", rc);
            goto cleanup;
        }

        rc = cfg_get_instance_string_fmt(&iface, "%s/net:", pci_oid);
        if (rc != 0)
        {
            ERROR("Failed to get %s's network interface OID: %r", ta, rc);
            goto cleanup;
        }

        if (strcmp(iface, interface) == 0)
        {
            cfg_oid *oid = cfg_convert_oid_str(pci_oid);
            rc = tapi_cfg_pci_addr_by_oid(oid, &pci_addr);
            free(oid);
            if (rc != 0)
            {
                ERROR("Failed to get PCI address by PCI device OID: %r", rc);
                goto cleanup;
            }

            goto cleanup;
        }

        free(pci_oid);
        free(iface);

        pci_oid = NULL;
        iface = NULL;
    }

cleanup:

    free(pci_handles);
    free(pci_oid);
    free(iface);

    return pci_addr;
}

/**
 * Convert @c '/agent/hardware/pci/vendor/device/instance' to
 * @c '/agent/hardware/pci/device' OID format.
 *
 * @param pci_vendor_oid PCI device OID in vendor format.
 *
 * @return Allocated PCI device OID or @c NULL.
 *
 * @note Function result should be free()'d.
 */
static cfg_oid*
tapi_trex_interface_vendor2device_oid(const cfg_oid *pci_vendor_oid)
{
    te_errno rc;

    cfg_oid *pci_dev_oid = NULL;

    char *pci_dev = NULL;
    char *pci_vendor = cfg_convert_oid(pci_vendor_oid);

    if (pci_vendor == NULL)
    {
        ERROR("Failed to convert OID to string");
        goto cleanup;
    }

    rc = tapi_cfg_pci_resolve_device_oid(&pci_dev, "%s", pci_vendor);
    if (rc != 0)
    {
        ERROR("Failed to resolve PCI vendor OID '%s': %r", pci_vendor, rc);
        goto cleanup;
    }

    pci_dev_oid = cfg_convert_oid_str(pci_dev);
    if (pci_dev_oid == NULL)
    {
        ERROR("Failed to convert '%s' to OID format", pci_dev);
        goto cleanup;
    }

cleanup:

    free(pci_vendor);
    free(pci_dev);

    return pci_dev_oid;
}

enum tapi_trex_oid_type {
    TAPI_TREX_OID_TYPE_INTERFACE = 0,
    TAPI_TREX_OID_TYPE_DEVICE,
    TAPI_TREX_OID_TYPE_VENDOR,
};

/* Actions that just store a chosen alternative into a provided location. */
static te_errno
action_oid_interface(const char *inst_oid, const cfg_oid *parsed_oid, void *ctx)
{
    tapi_trex_interface **iface = ctx;
    cfg_oid *oid = cfg_convert_oid_str(inst_oid);

    char *ta = cfg_oid_get_inst_name(oid, 1);
    char *name = cfg_oid_get_inst_name(oid, 2);
    char *pci_addr = tapi_trex_get_pci_addr(ta, name);;

    UNUSED(parsed_oid);

    *iface = tapi_trex_interface_init(pci_addr, TRUE);

    free(pci_addr);
    free(name);
    free(ta);

    return 0;
}

static te_errno
action_oid_device(const char *inst_oid, const cfg_oid *parsed_oid, void *ctx)
{
    te_errno rc;
    char *pci_addr;

    tapi_trex_interface **iface = ctx;
    cfg_oid *oid = cfg_convert_oid_str(inst_oid);

    UNUSED(parsed_oid);

    rc = tapi_cfg_pci_addr_by_oid(oid, &pci_addr);
    if (rc != 0)
    {
        ERROR("Failed to get PCI address: %r", rc);
        return rc;
    }

    *iface = tapi_trex_interface_init(pci_addr, TRUE);

    free(pci_addr);
    free(oid);

    return 0;
}

static te_errno
action_oid_vendor(const char *inst_oid, const cfg_oid *parsed_oid, void *ctx)
{
    te_errno rc;

    cfg_oid *oid = cfg_convert_oid_str(inst_oid);
    tapi_trex_interface **iface = ctx;

    char *pci_addr;
    cfg_oid *pci_dev_oid;

    UNUSED(parsed_oid);

    pci_dev_oid = tapi_trex_interface_vendor2device_oid(oid);
    if (pci_dev_oid == NULL)
    {
        ERROR("Failed to convert vendor OID to device OID");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = tapi_cfg_pci_addr_by_oid(pci_dev_oid, &pci_addr);
    if (rc != 0)
    {
        ERROR("Failed to get PCI address: %r", rc);
        return rc;
    }

    *iface = tapi_trex_interface_init(pci_addr, TRUE);

    free(pci_dev_oid);
    free(pci_addr);
    free(oid);

    return 0;
}

static const cfg_oid object_oid_interface = CFG_OBJ_OID_LITERAL(
    {"agent"}, {"interface"}
);

static const cfg_oid object_oid_device = CFG_OBJ_OID_LITERAL(
    {"agent"}, {"hardware"}, {"pci"}, {"device"}
);

static const cfg_oid object_oid_vendor = CFG_OBJ_OID_LITERAL(
    {"agent"}, {"hardware"}, {"pci"}, {"vendor"}, {"device"}, {"instance"}
);

/* See description in tapi_trex.h */
tapi_trex_interface *
tapi_trex_interface_init_oid(te_bool use_kernel_interface,
                             const char *oid_fmt, ...)
{
    te_errno rc;

    tapi_trex_interface *interface;
    char oid[CFG_OID_MAX];
    va_list ap;

    static const cfg_oid_rule oid_rules[] = {
        {
            .object_oid = &object_oid_interface,
            .match_prefix = FALSE,
            .action = action_oid_interface
        },
        {
            .object_oid = &object_oid_device,
            .match_prefix = FALSE,
            .action = action_oid_device
        },
        {
            .object_oid = &object_oid_vendor,
            .match_prefix = FALSE,
            .action = action_oid_vendor
        },
        CFG_OID_RULE_END
    };

    va_start(ap, oid_fmt);
    rc = te_vsnprintf(oid, sizeof(oid), oid_fmt, ap);
    va_end(ap);

    if (rc != 0)
        return NULL;

    if (use_kernel_interface)
    {
        cfg_oid *oid_cfg = cfg_convert_oid_str(oid);
        char *name = cfg_oid_get_inst_name(oid_cfg, 2);

        interface = TE_ALLOC(sizeof(tapi_trex_interface));
        *interface = (tapi_trex_interface){ .if_name = name,
                                            .need_to_bind = FALSE };
        cfg_free_oid(oid_cfg);
    }
    else
    {
        rc = cfg_oid_dispatch(oid_rules, oid, &interface);
        if (rc != 0)
        {
            ERROR("Failed to read unknown OID format: %r", rc);
            return NULL;
        }
    }

    return interface;
}

/* See description in tapi_trex.h */
void
tapi_trex_interface_free(tapi_trex_interface *interface)
{
    if (interface == NULL)
        return;

    free(interface->if_name);
    free(interface);
}

/**
 * Set up lists of ips and gws for TRex YAML config file and bind DPDK ports.
 *
 * @param[in] ta        Name of the Test Agent.
 * @param[in] driver    Driver name to be bound to PCI.
 * @param[in] interface TRex interface name.
 * @param[in] ip        IP address of @p iface.
 * @param[in] gw        GW address of @p iface.
 * @param[in] kvpairs   Allocated kvpairs with list of interfaces.
 *
 * @return Status code.
 */
static te_errno
tapi_trex_setup_port(const char *ta,
                     const char *driver,
                     const tapi_trex_interface *interface,
                     const struct sockaddr *ip,
                     const struct sockaddr *gw,
                     te_kvpair_h *kvpairs)
{
    te_errno rc;

    const char *iface;
    te_bool need_to_bind;

    char *ip_addr = te_ip2str(ip);
    char *gw_addr = te_ip2str(gw);

    if (interface == NULL || te_str_is_null_or_empty(interface->if_name))
    {
        need_to_bind = FALSE;
        iface = TAPI_TREX_DUMMY;
    }
    else
    {
        need_to_bind = interface->need_to_bind;
        iface = interface->if_name;
    }

    /*
     * Interface must be in quotes (e.g. '0000:01:00.0'),
     * as Cisco TRex scripts cannot correctly recognize
     * some interfaces without quotes.
     */
    te_kvpair_push(kvpairs, "IFACES", "'%s'", iface);
    te_kvpair_push(kvpairs, "PORTINFO_IP", "%s",
                   ip_addr != NULL ? ip_addr : "0.0.0.0");
    te_kvpair_push(kvpairs, "PORTINFO_DEFAULT_GW", "%s",
                   gw_addr != NULL ? gw_addr : "0.0.0.0");

    free(ip_addr);
    free(gw_addr);

    if (need_to_bind)
    {
        rc = tapi_trex_bind_pci_addr(ta, iface, driver);
        if (rc != 0)
        {
            ERROR("Failed to bind '%s' PCI interface on TA '%s': %r",
                  iface, ta, rc);
            return rc;
        }
    }

    return 0;
}

/**
 * Generate TRex YAML config file and bind DPDK ports if needed.
 *
 * @param[in] ta                Name of the Test Agent.
 * @param[in] opt               TRex options.
 * @param[in] yaml_config_path  TRex YAML config path.
 *
 * @return Status code.
 */
static te_errno
tapi_trex_gen_yaml_config(const char *ta, const tapi_trex_opt *opt,
                          const char *yaml_config_path)
{
    te_errno rc;

    te_vec clients = TE_VEC_INIT(const tapi_trex_client_config *);
    te_vec servers = TE_VEC_INIT(const tapi_trex_server_config *);

    const tapi_trex_client_config *client =
        (opt->clients != NULL ? opt->clients[0] : NULL);
    const tapi_trex_server_config *server =
        (opt->servers != NULL ? opt->servers[0] : NULL);

    te_string ifaces = TE_STRING_INIT;
    te_string ips = TE_STRING_INIT;

    size_t nics_n;

    te_kvpair_h kvpairs;
    te_kvpair_init(&kvpairs);
    te_kvpair_push(&kvpairs, "COLON", "%s", ":");

    for (nics_n = 0;
         (client = (client != NULL ? opt->clients[nics_n] : client)) != NULL ||
         (server = (server != NULL ? opt->servers[nics_n] : server)) != NULL;
         nics_n++)
    {
        TE_VEC_APPEND(&clients, client);
        TE_VEC_APPEND(&servers, server);
    }

    for (nics_n = te_vec_size(&clients); nics_n != 0; nics_n -= 1)
    {
        client = TE_VEC_GET(const tapi_trex_client_config *, &clients, nics_n - 1);
        server = TE_VEC_GET(const tapi_trex_server_config *, &servers, nics_n - 1);

        client = (client != NULL ? client : &tapi_trex_client_config_default);
        server = (server != NULL ? server : &tapi_trex_server_config_default);

        rc = tapi_trex_setup_port(ta, opt->driver,
                                  server->common.interface,
                                  server->common.ip,
                                  server->common.gw,
                                  &kvpairs);
        if (rc != 0)
            goto cleanup;

        rc = tapi_trex_setup_port(ta, opt->driver,
                                  client->common.interface,
                                  client->common.ip,
                                  client->common.gw,
                                  &kvpairs);
        if (rc != 0)
            goto cleanup;
    }

    te_vec_free(&clients);
    te_vec_free(&servers);

    rc = tapi_file_expand_kvpairs(ta, default_trex_cfg, NULL,
                                  &kvpairs, yaml_config_path);
    if (rc != 0)
        ERROR("Failed to create TRex config '%s'", yaml_config_path);

cleanup:

    te_string_free(&ifaces);
    te_string_free(&ips);

    te_kvpair_fini(&kvpairs);

    return rc;
}

/**
 * Configure DPDK on TA side and create TRex configuration files.
 *
 * @param[in] ta                Name of the Test Agent.
 * @param[in] opt               TRex options.
 * @param[in] yaml_config_path  TRex YAML config path.
 *
 * @return Status code.
 */
static te_errno
tapi_trex_configure(const char *ta, const tapi_trex_opt *opt,
                    const char *yaml_config_path)
{
    te_errno rc;

    rc = tapi_trex_gen_astf_config(ta, opt);
    if (rc != 0)
        return rc;

    rc = tapi_trex_gen_yaml_config(ta, opt, yaml_config_path);
    if (rc != 0)
        return rc;

    return 0;
}

/**
 * Set up TRex YAML config file path and add it into TRex arguments.
 *
 * @param[in,out] args  TRex command line arguments.
 *
 * @return Allocated TRex YAML config file path.
 */
static char *
tapi_trex_setup_yaml_config_path_setup(te_vec *args)
{
    char *yaml_config_filename = te_make_spec_buf(RCF_RPC_NAME_LEN / 2,
                                                  RCF_RPC_NAME_LEN / 2,
                                                  NULL, TE_FILL_SPEC_C_ID);

    char *yaml_config_path = te_string_fmt(TAPI_TREX_CFG_YAML_FMT,
                                           yaml_config_filename);

    free(yaml_config_filename);

    if (te_vec_size(args) > 1)
        te_vec_remove_index(args, te_vec_size(args) - 1);

    te_vec_append_str_fmt(args, "%s", "--cfg");
    te_vec_append_str_fmt(args, "%s", yaml_config_path);

    TE_VEC_APPEND_RVALUE(args, const char *, NULL);

    return yaml_config_path;
}

/* See description in tapi_trex.h */
te_errno
tapi_trex_create(tapi_job_factory_t *factory,
                 const tapi_trex_opt *opt,
                 tapi_trex_app **app)
{
    te_errno rc;

    te_vec args = TE_VEC_INIT(char *);
    tapi_trex_app *new_app;

    char *workdir;
    char *yaml_config_path;

    if (factory == NULL)
    {
        ERROR("TRex factory to create job can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (opt == NULL)
    {
        ERROR("TRex opt to create job can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (app == NULL)
    {
        ERROR("TRex app to create job can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (opt->trex_exec == NULL)
    {
        ERROR("TRex exec path can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = tapi_job_opt_build_args(opt->trex_exec, trex_args_binds, opt, &args);
    if (rc != 0)
    {
        ERROR("Failed to build TRex job arguments: %r", rc);
        return rc;
    }

    yaml_config_path = tapi_trex_setup_yaml_config_path_setup(&args);
    if (yaml_config_path == NULL)
    {
        ERROR("Failed to create TRex YAML config file name");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    new_app = TE_ALLOC(sizeof(*new_app));

    rc = tapi_job_simple_create(factory,
                        &(tapi_job_simple_desc_t){
                            .program    = opt->trex_exec,
                            .argv       = (const char **)args.data.ptr,
                            .job_loc    = &new_app->job,
                            .stdout_loc = &new_app->out_chs[0],
                            .stderr_loc = &new_app->out_chs[1],
                            .filters    = TAPI_JOB_SIMPLE_FILTERS(
                                {
                                    .use_stdout = TRUE,
                                    .readable = TRUE,
                                    .re = "Total\\-Tx\\s+\\:\\s+"
                                          "([0-9]+\\.[0-9]{2}\\s.)bps",
                                    .extract = 1,
                                    .filter_var = &new_app->total_tx_filter,
                                },
                                {
                                    .use_stdout = TRUE,
                                    .readable = TRUE,
                                    .re = "Total\\-Rx\\s+\\:\\s+"
                                          "([0-9]+\\.[0-9]{2}\\s.)bps",
                                    .extract = 1,
                                    .filter_var = &new_app->total_rx_filter,
                                },
                                {
                                    .use_stdout = TRUE,
                                    .readable = TRUE,
                                    .re = "Total\\-CPS\\s+\\:\\s+"
                                          "([0-9]+\\.[0-9]{2}\\s.)cps",
                                    .extract = 1,
                                    .filter_var = &new_app->total_cps_filter,
                                },
                                {
                                   .use_stdout  = TRUE,
                                   .readable    = FALSE,
                                   .log_level   = TE_LL_RING,
                                   .filter_name = "TRex stdout"
                                },
                                {
                                   .use_stderr  = TRUE,
                                   .readable    = FALSE,
                                   .log_level   = TE_LL_WARN,
                                   .filter_name = "TRex stderr"
                                }
                            )
                        });
    if (rc != 0)
    {
        ERROR("Failed to create '%s' job: %r", opt->trex_exec, rc);
        te_vec_deep_free(&args);
        free(yaml_config_path);
        free(new_app);
        return rc;
    }

    workdir = te_dirname(opt->trex_exec);
    rc = tapi_job_set_workdir(new_app->job, workdir);
    if (rc != 0)
    {
        ERROR("Failed to set TRex working directory '%s': %r", workdir, rc);
        te_vec_deep_free(&args);
        free(yaml_config_path);
        free(workdir);
        free(new_app);
        return rc;
    }
    free(workdir);

    rc = tapi_trex_configure(tapi_job_factory_ta(factory), opt,
                             yaml_config_path);
    if (rc != 0)
    {
        ERROR("Failed to configure TRex environment: %r", rc);
        te_vec_deep_free(&args);
        free(yaml_config_path);
        free(new_app);
        return rc;
    }

    *app = new_app;
    te_vec_deep_free(&args);
    free(yaml_config_path);

    return 0;
}

/* See description in tapi_trex.h */
te_errno
tapi_trex_start(const tapi_trex_app *app)
{
    if (app == NULL)
    {
        ERROR("TRex app to start job can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return tapi_job_start(app->job);
}

/* See description in tapi_trex.h */
te_errno
tapi_trex_wait(const tapi_trex_app *app, int timeout_ms)
{
    te_errno rc;
    tapi_job_status_t status;

    if (app == NULL)
    {
        ERROR("TRex app to wait job can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = tapi_job_wait(app->job, timeout_ms, &status);
    if (rc != 0)
    {
        if (TE_RC_GET_ERROR(rc) == TE_EINPROGRESS)
            RING("Job was still in process at the end of the wait");

        return rc;
    }

    TAPI_JOB_CHECK_STATUS(status);
    return 0;
}

/* See description in tapi_trex.h */
te_errno
tapi_trex_stop(const tapi_trex_app *app)
{
    if (app == NULL)
    {
        ERROR("TRex app to stop job can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return tapi_job_stop(app->job, SIGTERM, TAPI_TREX_TIMEOUT_MS);
}

/* See description in tapi_trex.h */
te_errno
tapi_trex_kill(const tapi_trex_app *app, int signum)
{
    if (app == NULL)
    {
        ERROR("TRex app to kill job can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return tapi_job_kill(app->job, signum);
}

/**
 * Free TAPI TRex clients.
 *
 * @param nics  TAPI TRex clients.
 */
static void
tapi_trex_destroy_clients(tapi_trex_client_config *clients[])
{
    size_t clients_n;

    if (clients == NULL)
        return;

    for(clients_n = 0; clients[clients_n] != NULL; clients_n++)
        tapi_trex_interface_free(clients[clients_n]->common.interface);
}

/**
 * Free TAPI TRex servers.
 *
 * @param nics  TAPI TRex servers.
 */
static void
tapi_trex_destroy_servers(tapi_trex_server_config *servers[])
{
    size_t servers_n;

    if (servers == NULL)
        return;

    for(servers_n = 0; servers[servers_n] != NULL; servers_n++)
        tapi_trex_interface_free(servers[servers_n]->common.interface);
}

/* See description in tapi_trex.h */
te_errno
tapi_trex_destroy(const char *ta, tapi_trex_app *app, tapi_trex_opt *opt)
{
    te_errno rc;

    if (app == NULL)
        return 0;

    rc = tapi_job_destroy(app->job, TAPI_TREX_TIMEOUT_MS);
    if (rc != 0)
        ERROR("Failed to destroy TRex job: %r", rc);

    free(app);

    tapi_trex_destroy_clients(opt->clients);
    tapi_trex_destroy_servers(opt->servers);

    return rc;
}

/** Data units starting from bits. */
static const te_unit_list bin_units = {
    .scale = 1024,
    .start_pow = 0,
    .units = (const char * const[]){ "  ", " K", " M", " G", " T", NULL },
};

/**
 * Calculate average value of filtered @p units from filter.
 *
 * @param[in]  filter   Filter for reading.
 * @param[in]  units    Unit type.
 *
 * @return Average value of filtered @p units.
 */
static double
get_avg_from_filter(tapi_job_channel_t *filter, const te_unit_list *units)
{
    te_errno rc;
    tapi_job_buffer_t *bufs = NULL;

    unsigned bufs_n = 0;
    unsigned num;

    double total = 0.0;
    double val = 0.0;

    rc = tapi_job_receive_many(TAPI_JOB_CHANNEL_SET(filter),
                               TAPI_TREX_TIMEOUT_MS,
                               &bufs, &bufs_n);
    if (rc != 0)
    {
        ERROR("Failed to read data from filter: %r", rc);
        goto cleanup;
    }

    if (bufs_n < 2)
    {
        WARN("Too little data to calculate the average value");
        goto cleanup;
    }

    for (num = 0; num < bufs_n; num++)
    {
        if (bufs[num].eos)
            break;

        rc = te_unit_list_value_from_string(bufs[num].data.ptr, units, &val);
        if (rc != 0)
        {
            ERROR("Failed to convert value '%s': %r", bufs[num].data.ptr, rc);
            goto cleanup;
        }

        total += val;
    }

cleanup:

    tapi_job_buffers_free(bufs, bufs_n);

    if (rc != 0)
        return 0.0;

    return (total / num);
}

/* See description in tapi_trex.h */
te_errno
tapi_trex_get_report(tapi_trex_app *app, tapi_trex_report *report)
{
    if (app == NULL)
    {
        ERROR("TRex app to get job report can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (report == NULL)
    {
        ERROR("TRex report to get job report can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    report->avg_rx = get_avg_from_filter(app->total_tx_filter, &bin_units);
    report->avg_tx = get_avg_from_filter(app->total_rx_filter, &bin_units);
    report->avg_cps = get_avg_from_filter(app->total_cps_filter, &bin_units);

    return 0;
}

/* See description in tapi_trex.h */
te_errno
tapi_trex_report_mi_log(const tapi_trex_report *report)
{
    te_errno rc;
    te_mi_logger *logger;

    if (report == NULL)
    {
        ERROR("TRex report to write log can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    rc = te_mi_logger_meas_create("trex", &logger);
    if (rc != 0)
    {
        ERROR("Failed to create MI logger, error: %r", rc);
        return rc;
    }

    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_THROUGHPUT, "Average Tx",
                          TE_MI_MEAS_AGGR_SINGLE, report->avg_tx,
                          TE_MI_MEAS_MULTIPLIER_PLAIN);
    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_THROUGHPUT, "Average Rx",
                          TE_MI_MEAS_AGGR_SINGLE, report->avg_rx,
                          TE_MI_MEAS_MULTIPLIER_PLAIN);
    te_mi_logger_add_meas(logger, NULL, TE_MI_MEAS_THROUGHPUT, "Average CPS",
                          TE_MI_MEAS_AGGR_SINGLE, report->avg_rx,
                          TE_MI_MEAS_MULTIPLIER_PLAIN);

    te_mi_logger_destroy(logger);
    return 0;
}

/* See description in tapi_trex.h */
te_errno
tapi_trex_destroy_report(tapi_trex_report *report)
{
    if (report == NULL)
    {
        ERROR("TRex report to destroy can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    return 0;
}
