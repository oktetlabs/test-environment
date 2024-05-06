/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2023-2024 OKTET Labs Ltd. All rights reserved. */
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

/** TRex m_traffic_duration filter for client (1) and server (2). */
#define TAPI_TREX_M_TRAFF_DUR_FLT \
    "\\s+m_traffic_duration\\s+\\|\\s+([0-9]+\\.[0-9]{2})\\s+sec\\s+\\|" \
    "\\s+([0-9]+\\.[0-9]{2})\\s+sec\\s+\\|\\s+measured traffic duration"

#define TAPI_TREX_PORT_STAT_ONE_COUNTER_FLT "\\s+\\|\\s+([0-9]+)"
#define TAPI_TREX_PORT_STAT_TIME_FLT "\\s+:\\s+([0-9]+\\.[0-9]+)\\s+sec"

typedef enum tapi_trex_val_type {
    TAPI_TREX_VAL_TYPE_UINT64,
    TAPI_TREX_VAL_TYPE_DOUBLE,
} tapi_trex_val_type;

typedef struct tapi_trex_port_stat_type {
    const char *name;
    tapi_trex_val_type type;
    size_t offset;
} tapi_trex_port_stat_type;

static const tapi_trex_port_stat_type port_stat_types[] = {
    [TAPI_TREX_PORT_STAT_OPKTS] = {
        .name = "opackets",
        .type = TAPI_TREX_VAL_TYPE_UINT64,
        .offset = offsetof(struct tapi_trex_port_stat, opackets) },
    [TAPI_TREX_PORT_STAT_OBYTES] = {
        .name = "obytes",
        .type = TAPI_TREX_VAL_TYPE_UINT64,
        .offset = offsetof(struct tapi_trex_port_stat, obytes) },
    [TAPI_TREX_PORT_STAT_IPKTS] = {
        .name = "ipackets",
        .type = TAPI_TREX_VAL_TYPE_UINT64,
        .offset = offsetof(struct tapi_trex_port_stat, ipackets) },
    [TAPI_TREX_PORT_STAT_IBYTES] = {
        .name = "ibytes",
        .type = TAPI_TREX_VAL_TYPE_UINT64,
        .offset = offsetof(struct tapi_trex_port_stat, ibytes) },
    [TAPI_TREX_PORT_STAT_IERRS] = {
        .name = "ierrors",
        .type = TAPI_TREX_VAL_TYPE_UINT64,
        .offset = offsetof(struct tapi_trex_port_stat, ierrors) },
    [TAPI_TREX_PORT_STAT_OERRS] = {
        .name = "oerrors",
        .type = TAPI_TREX_VAL_TYPE_UINT64,
        .offset = offsetof(struct tapi_trex_port_stat, oerrors) },
    [TAPI_TREX_PORT_STAT_CURRENT_TIME] = {
        .name = "current time",
        .type = TAPI_TREX_VAL_TYPE_DOUBLE,
        .offset = offsetof(struct tapi_trex_port_stat, curr_time) },
    [TAPI_TREX_PORT_STAT_TEST_DURATION] = {
        .name = "test duration",
        .type = TAPI_TREX_VAL_TYPE_DOUBLE,
        .offset = offsetof(struct tapi_trex_port_stat, test_duration) },
};

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
"  port_info:\n"
"${PORTINFO_IP*    - ip${COLON} ${PORTINFO_IP[${}]}\n"
"      default_gw${COLON} ${PORTINFO_DEFAULT_GW[${}]}\n}"
"${PORTINFO_DST_MAC*    - dest_mac${COLON} ${PORTINFO_DST_MAC[${}]}\n"
"      src_mac${COLON} ${PORTINFO_SRC_MAC[${}]}\n}";

/** Mapping of possible values for tapi_trex_opt::verbose option. */
static const te_enum_map tapi_trex_verbose_mapping[] = {
    {.name = "1", .value = TAPI_TREX_VERBOSE_MODE_MIN},
    {.name = "3", .value = TAPI_TREX_VERBOSE_MODE_MAX},
    TE_ENUM_MAP_END
};

/** Mapping of possible values for tapi_trex_opt::iom option. */
static const te_enum_map tapi_trex_iom_mapping[] = {
    {.name = "0", .value = TAPI_TREX_IOM_SILENT},
    {.name = "1", .value = TAPI_TREX_IOM_NORMAL},
    {.name = "2", .value = TAPI_TREX_IOM_SHORT},
    TE_ENUM_MAP_END
};

/** Mapping of possible values for tapi_trex_opt::*-so options. */
static const te_enum_map tapi_trex_so_mapping[] = {
    {.name = "--mlx4-so", .value = TAPI_TREX_SO_MLX4},
    {.name = "--mlx5-so", .value = TAPI_TREX_SO_MLX5},
    {.name = "--mlx4-so --mlx5-so", .value = TAPI_TREX_SO_MLX4_MLX5},
    {.name = "--ntacc-so", .value = TAPI_TREX_SO_NTACC},
    {.name = "--bnxt-so", .value = TAPI_TREX_SO_BNXT},
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
    TAPI_JOB_OPT_ENUM("--iom", FALSE, tapi_trex_opt, iom,
                      tapi_trex_iom_mapping),
    TAPI_JOB_OPT_ENUM(NULL, FALSE, tapi_trex_opt, so, tapi_trex_so_mapping),
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
    .stdout_log_level = TE_LL_RING,
    .stderr_log_level = TE_LL_WARN,
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
    .iom = TAPI_TREX_IOM_NONE,
    .so = TAPI_TREX_SO_NONE,
    .init_wait_sec = TAPI_JOB_OPT_UINT_UNDEF,
    .instance_prefix = NULL,
    .clients = NULL,
    .servers = NULL,
    .astf_vars = NULL,
    .driver = NULL,
    .cfg_template = NULL,
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
 * @param[in] ip        IP address (or MAC) of @p iface.
 * @param[in] gw        GW address (or MAC) of @p iface.
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
    te_string src = TE_STRING_INIT;
    te_string dst = TE_STRING_INIT;

    if (ip != NULL)
    {
        rc = te_netaddr2te_str(ip, &src);
        if (rc != 0)
        {
            ERROR("%s:%d te_netaddr2te_str() returned unexpected result: %r",
                  __FILE__, __LINE__, rc);
            return rc;
        }
    }

    if (gw != NULL)
    {
        rc = te_netaddr2te_str(gw, &dst);
        if (rc != 0)
        {
            ERROR("%s:%d te_netaddr2te_str() returned unexpected result: %r",
                  __FILE__, __LINE__, rc);
            return rc;
        }
    }

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
    if (ip == NULL || ip->sa_family != AF_LOCAL)
    {
        te_kvpair_push(kvpairs, "PORTINFO_IP", "%s",
                       ip != NULL ? src.ptr : "0.0.0.0");
    }
    else
    {
        te_kvpair_push(kvpairs, "PORTINFO_SRC_MAC", "%s", src.ptr);
    }

    if (gw == NULL || gw->sa_family != AF_LOCAL)
    {
        te_kvpair_push(kvpairs, "PORTINFO_DEFAULT_GW", "%s",
                       gw != NULL ? dst.ptr : "0.0.0.0");
    }
    else
    {
        te_kvpair_push(kvpairs, "PORTINFO_DST_MAC", "%s", dst.ptr);
    }

    te_string_free(&src);
    te_string_free(&dst);

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
    const char *cfg_templ;

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

    cfg_templ = opt->cfg_template != NULL ? opt->cfg_template : default_trex_cfg;
    rc = tapi_file_expand_kvpairs(ta, cfg_templ, NULL,
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

/** Return the number of ports actually used by all clients and servers. */
static unsigned int
tapi_trex_ports_count(const tapi_trex_opt *opt)
{
    unsigned int count = 0;
    tapi_trex_client_config **clt;
    tapi_trex_server_config **srv;

    for (clt = opt->clients; *clt != NULL; clt++)
    {
        if ((*clt)->common.interface != NULL)
            count++;
    }

    for (srv = opt->servers; *srv != NULL; srv++)
    {
        if ((*srv)->common.interface != NULL)
            count++;
    }

    return count;
}

/**
 * Attach (dynamically) filters to capture per port statistics.
 * It makes sense to do this when the '--iom 1' option is enabled.
 */
static te_errno
tapi_trex_port_stat_flts_attach(tapi_trex_app *app, const tapi_trex_opt *opt)
{
    te_errno rc;
    unsigned int i;
    unsigned int j;
    unsigned int n_flts;
    unsigned int n_ports;
    tapi_trex_port_stat_flt *flt;
    tapi_trex_port_stat_flt **flts = NULL;
    te_string buf = TE_STRING_INIT;
    te_string int_flts = TE_STRING_INIT;

    n_ports = tapi_trex_ports_count(opt);
    /* The integer filters depend on the number of the ports used */
    for (i = 0; i < n_ports; i++)
        te_string_append(&int_flts, "%s", TAPI_TREX_PORT_STAT_ONE_COUNTER_FLT);

    n_flts = TE_ARRAY_LEN(port_stat_types) * n_ports;
    flts = TE_ALLOC(sizeof(*flts) * (n_flts + 1 /* NULL terminated */));

    for (i = 0; i < TE_ARRAY_LEN(port_stat_types); i++)
    {
        for (j = 0; j < n_ports; j++)
        {
            flt = TE_ALLOC(sizeof(*flt));
            flt->param = i;
            flt->index = j;

            te_string_reset(&buf);
            te_string_append(&buf, "port %u %s", flt->index,
                             port_stat_types[flt->param].name);
            rc = tapi_job_attach_filter(TAPI_JOB_CHANNEL_SET(app->out_chs[0]),
                                        buf.ptr, TRUE, 0, &flt->filter);
            if (rc != 0)
            {
                ERROR("%s() failed to attach '%s' filter: %r", __func__,
                      buf.ptr, rc);
                goto cleanup;
            }

            te_string_reset(&buf);
            switch (flt->param) {
                case TAPI_TREX_PORT_STAT_CURRENT_TIME:
                case TAPI_TREX_PORT_STAT_TEST_DURATION:
                    te_string_append(&buf, "%s%s",
                                     port_stat_types[flt->param].name,
                                     TAPI_TREX_PORT_STAT_TIME_FLT);
                    rc = tapi_job_filter_add_regexp(flt->filter,
                                                    buf.ptr, 1);
                    break;

                default:
                    te_string_append(&buf, "%s%s",
                                     port_stat_types[flt->param].name,
                                     int_flts.ptr);
                    rc = tapi_job_filter_add_regexp(flt->filter,
                                                    buf.ptr, flt->index + 1);
                    break;
            }
            if (rc != 0)
            {
                ERROR("%s() failed to add regexp '%s': %r", __func__,
                      buf.ptr, rc);
                goto cleanup;
            }
            flts[i * n_ports + j] = flt;
        }
    }
    app->per_port_stat_flts.flts = flts;
    app->per_port_stat_flts.n_ports = n_ports;

cleanup:
    te_string_free(&buf);
    te_string_free(&int_flts);
    if (rc != 0 && flts != NULL)
    {
        for (i = 0; flts[i] != NULL; i++)
            free(flts[i]);
        free(flts);
    }
    return rc;
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
                                    .use_stdout = TRUE,
                                    .readable = TRUE,
                                    .re = "Total-tx-pkt\\s+:\\s+([0-9]+)\\s+pkts",
                                    .extract = 1,
                                    .filter_var = &new_app->total_tx_pkt_filter,
                                },
                                {
                                    .use_stdout = TRUE,
                                    .readable = TRUE,
                                    .re = "Total-rx-pkt\\s+:\\s+([0-9]+)\\s+pkts",
                                    .extract = 1,
                                    .filter_var = &new_app->total_rx_pkt_filter,
                                },
                                {
                                    .use_stdout = TRUE,
                                    .readable = TRUE,
                                    .re = TAPI_TREX_M_TRAFF_DUR_FLT,
                                    .extract = 1,
                                    .filter_var = &new_app->m_traff_dur_cl_flt,
                                },
                                {
                                    .use_stdout = TRUE,
                                    .readable = TRUE,
                                    .re = TAPI_TREX_M_TRAFF_DUR_FLT,
                                    .extract = 2,
                                    .filter_var = &new_app->m_traff_dur_srv_flt,
                                },
                                {
                                   .use_stdout  = TRUE,
                                   .readable    = TRUE,
                                   .log_level   = opt->stdout_log_level,
                                   .filter_name = "TRex stdout",
                                   .filter_var = &new_app->std_out,
                                },
                                {
                                   .use_stderr  = TRUE,
                                   .readable    = TRUE,
                                   .log_level   = opt->stderr_log_level,
                                   .filter_name = "TRex stderr",
                                   .filter_var = &new_app->std_err,
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

    if (opt->iom == TAPI_TREX_IOM_NORMAL)
    {
        rc = tapi_trex_port_stat_flts_attach(new_app, opt);
        if (rc != 0)
        {
            te_vec_deep_free(&args);
            free(yaml_config_path);
            free(new_app);
            return rc;
        }
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

/**
 * Get content of a single message.
 *
 * @param filter[in]    from where to read the message
 * @param str[out]      where to save the content
 *
 * @return Status code.
 */
static te_errno
get_single_str(tapi_job_channel_t *filter, te_string *str)
{
    te_errno rc;

    rc = tapi_job_receive_single(filter, str, TAPI_TREX_TIMEOUT_MS);
    if (rc != 0)
    {
        ERROR("%s:%d tapi_job_receive_single returned unexpected result: %r",
              __FILE__, __LINE__, rc);
    }
    return rc;
}

/**
 * Get a single unit64_t value.
 *
 * @param filter[in]    from where to read the message
 * @param value[out]    where to save the value
 *
 * @return Status code.
 */
static te_errno
get_single_uint64(tapi_job_channel_t *filter, uint64_t *value)
{
    te_errno rc;
    te_string str = TE_STRING_INIT;

    rc = get_single_str(filter, &str);

    if (rc == 0)
        rc = te_str_to_uint64(str.ptr, 10, value);

    te_string_free(&str);
    return rc;
}

/**
 * Get a single double value.
 *
 * @param filter[in]    from where to read the message
 * @param value[out]    where to save the value
 *
 * @return Status code.
 */
static te_errno
get_single_double(tapi_job_channel_t *filter, double *value)
{
    te_errno rc;
    te_string str = TE_STRING_INIT;

    rc = get_single_str(filter, &str);

    if (rc == 0)
        rc = te_strtod(str.ptr, value);

    te_string_free(&str);
    return rc;
}

/** Convert a string to a number. */
static te_errno
tapi_trex_str_to_val(const char *str, tapi_trex_val_type type, void *val)
{
    switch (type) {
        case TAPI_TREX_VAL_TYPE_UINT64:
            return te_str_to_uint64(str, 10, val);
            break;

        case TAPI_TREX_VAL_TYPE_DOUBLE:
            return te_strtod(str, val);
            break;

        default:
            return TE_RC(TE_TAPI, TE_EINVAL);
    }
}

/** Fill in the report with port statistics data. */
static te_errno
tapi_trex_get_report_port_stat(tapi_trex_app *app, tapi_trex_report *report)
{
    te_errno rc = 0;
    unsigned int i;
    unsigned int res_len;
    unsigned int n_bufs = 0;
    unsigned int n_ports;
    tapi_trex_port_stat_flt **flt;
    tapi_job_buffer_t *bufs = NULL;
    tapi_trex_port_stat **res = NULL;

    n_ports = app->per_port_stat_flts.n_ports;

    for (flt = app->per_port_stat_flts.flts; *flt != NULL; flt++)
    {
        rc = tapi_job_receive_many(TAPI_JOB_CHANNEL_SET((*flt)->filter),
                                   TAPI_TREX_TIMEOUT_MS, &bufs, &n_bufs);
        if (rc != 0)
        {
            ERROR("%() failed to read data from filter: %r", __func__, rc);
            goto cleanup;
        }

        if (res == NULL)
        {
            /* the set of messages ends with eos. */
            res_len = n_bufs;
            if (res_len < 2)
            {
                ERROR("%s() there are no per port statistics", __func__);
                goto cleanup;
            }

            res = TE_ALLOC(sizeof(*res) * res_len);
            for (i = 0; i < (res_len - 1 /* NULL terminated */); i++)
            {
                res[i] = TE_ALLOC(sizeof(**res) * n_ports);
            }
        }

        if (n_bufs != res_len)
        {
            ERROR("%s() unexpected len of buffers %u, expected %u",
                  __func__, n_bufs, res_len);
            rc = TE_RC(TE_TAPI, TE_ERANGE);
            goto cleanup;
        }
        for (i = 0; i < n_bufs; i++)
        {
            if (i == n_bufs - 1)
            {
                if (!bufs[i].eos)
                {
                    ERROR("%s() the expected eos was not found (%s)",
                          __func__, bufs[i].data.ptr);
                    rc = TE_RC(TE_TAPI, TE_ERANGE);
                    goto cleanup;
                }
                break;
            }
            else if (bufs[i].eos)
            {
                ERROR("%s() the unexpected eos was not found (%s)", __func__);
                rc = TE_RC(TE_TAPI, TE_EINVAL);
                goto cleanup;
            }
            rc = tapi_trex_str_to_val(bufs[i].data.ptr, port_stat_types[(*flt)->param].type,
                                      (void *)&res[i][(*flt)->index] + port_stat_types[(*flt)->param].offset);
            if (rc != 0)
                goto cleanup;
        }

        tapi_job_buffers_free(bufs, n_bufs);
        bufs = NULL;
    }
    report->per_port_stat.ports = res;
    report->per_port_stat.n_ports = n_ports;

cleanup:
    tapi_job_buffers_free(bufs, n_bufs);
    return rc;
}

/* See description in tapi_trex.h */
te_errno
tapi_trex_get_report(tapi_trex_app *app, tapi_trex_report *report)
{
    te_errno rc = 0;

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

    report->avg_rx = get_avg_from_filter(app->total_rx_filter, &bin_units);
    report->avg_tx = get_avg_from_filter(app->total_tx_filter, &bin_units);
    report->avg_cps = get_avg_from_filter(app->total_cps_filter, &bin_units);

    rc = get_single_uint64(app->total_tx_pkt_filter, &report->tx_pkts);
    if (rc != 0)
        return rc;

    rc = get_single_uint64(app->total_rx_pkt_filter, &report->rx_pkts);
    if (rc != 0)
        return rc;

    rc = get_single_double(app->m_traff_dur_cl_flt, &report->m_traff_dur_cl);
    if (rc != 0)
        return rc;

    rc = get_single_double(app->m_traff_dur_srv_flt, &report->m_traff_dur_srv);
    if (rc != 0)
        return rc;

    if (app->per_port_stat_flts.flts != NULL)
        rc = tapi_trex_get_report_port_stat(app, report);

    return rc;
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
    tapi_trex_port_stat **it;

    if (report == NULL)
    {
        ERROR("TRex report to destroy can't be NULL");
        return TE_RC(TE_TAPI, TE_EINVAL);
    }

    if (report->per_port_stat.ports != NULL)
    {
        for (it = report->per_port_stat.ports; *it != NULL; it++)
            free(*it);

        free(report->per_port_stat.ports);
    }

    return 0;
}

static void
tapi_trex_val_to_double(void *ptr, tapi_trex_val_type type, double *out)
{
    switch (type) {
        case TAPI_TREX_VAL_TYPE_UINT64:
            *out = 1. * (*(uint64_t*)ptr);
            break;

        case TAPI_TREX_VAL_TYPE_DOUBLE:
            *out = *(double*)ptr;
            break;
    }
}

/* See description in tapi_trex.h */
void
tapi_trex_port_stat_param_series_get(tapi_trex_report *report,
                          tapi_trex_port_stat_enum param, unsigned int index,
                          te_bool absolute_value, te_bool by_time,
                          double **vals, unsigned int *n_vals)
{
    size_t offset;
    unsigned int i;
    unsigned int count = 0;
    double *res = NULL;
    tapi_trex_val_type val_type;
    tapi_trex_port_stat **ports;
    tapi_trex_port_stat **it;

    assert(report != NULL);

    offset = port_stat_types[param].offset;
    val_type = port_stat_types[param].type;

    ports = report->per_port_stat.ports;
    if (ports == NULL)
        goto out;

    for (it = ports, count = 0; *it != NULL; it++)
        count++;

    res = TE_ALLOC(sizeof(*res) * count);

    for (i = 0; i < count; i++)
    {
        void *pval1;
        void *pval2;
        double val1;
        double val2;
        double time_diff;

        pval2 = (void *)&ports[i][index] + offset;
        if (absolute_value)
        {
            tapi_trex_val_to_double(pval2, val_type, &res[i]);
        }
        else
        {
            tapi_trex_val_to_double(pval2, val_type, &val2);

            if (i > 0)
            {
                pval1 = (void *)&ports[i - 1][index] + offset;
                tapi_trex_val_to_double(pval1, val_type, &val1);
                time_diff = ports[i][index].curr_time -
                                    ports[i - 1][index].curr_time;
            }
            else
            {
                /*
                 * When calculating the difference in values, assume
                 * that the parameter values were originally zeros.
                 */
                val1 = 0;
                time_diff = ports[i][index].curr_time;
            }

            res[i] = val2 - val1;
            if (by_time)
                res[i] /= time_diff;
        }
    }

out:
    *vals = res;
    *n_vals = count;
}

/* Callback function for qsort. */
static int
tapi_trex_compare_doubles_max(const void *a, const void *b)
{
    double *val1 = (double *)a;
    double *val2 = (double *)b;

    if (*val1 < *val2)
        return 1;
    else if (*val1 > *val2)
        return -1;
    else
        return 0;
}

/* See description in tapi_trex.h */
te_errno
tapi_trex_port_stat_median_get(tapi_trex_report *report,
                            tapi_trex_port_stat_enum param, unsigned int index,
                            double time_start, double time_end, double *median)
{
    te_errno rc = 0;
    double *vals;
    double *meds;
    unsigned int i;
    unsigned int n_vals;
    unsigned int n_meds;
    unsigned int i_start = UINT_MAX;
    unsigned int i_end = UINT_MAX;

    tapi_trex_port_stat_time_series_get(report, &vals, &n_vals);
    if (vals == NULL)
    {
        ERROR("%s() interface statistics do not contain data (is the --iom 1 "
              "option used?)", __func__);
        return TE_RC(TE_TAPI, TE_ENODATA);
    }
    assert(time_start < time_end);

    for (i = 0; i < n_vals - 1; i++)
    {
        if (vals[i] < time_start && time_start < vals[i + 1])
            i_start = i;

        if (vals[i] < time_end && time_end < vals[i + 1])
            i_end = i;
    }

    if (i_start == UINT_MAX)
    {
        rc = TE_RC(TE_TAPI, TE_ERANGE);
        WARN("%s() cannot find start time -gt %f, using first element",
             __func__, time_start);
        i_start = 0;
    }
    if (i_end == UINT_MAX)
    {
        rc = TE_RC(TE_TAPI, TE_ERANGE);
        WARN("%s() cannot find end time -gt %f, using last element",
             __func__, time_end);
        i_end = n_vals - 1;
    }
    free(vals);

    n_meds = i_end - i_start + 1;
    tapi_trex_port_stat_param_series_by_time_get(report, param, index,
                                                 &vals, &n_vals);
    assert(n_meds < n_vals);

    meds = TE_ALLOC(sizeof(*meds) * n_meds);
    memcpy(meds, &vals[i_start], sizeof(*meds) * n_meds);
    free(vals);

    qsort(meds, n_meds, sizeof(*meds), &tapi_trex_compare_doubles_max);
    *median = meds[n_meds / 2];
    free(meds);

    return rc;
}
