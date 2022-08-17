/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Windows Test Agent
 *
 * Windows TA configuring support
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#define TE_LGR_USER     "Windows Conf"

#include "windows.h"
#include "w32api/iphlpapi.h"
#undef _SYS_TYPES_FD_SET
#include <w32api/winsock2.h>
#include "te_defs.h"
#include "iprtrmib.h"

#undef ERROR

#include "te_errno.h"
#include "te_defs.h"
#include "logger_api.h"
#include "comm_agent.h"
#include "rcf_ch_api.h"
#include "rcf_pch.h"
#include "rcf_pch_ta_cfg.h"
#include "cs_common.h"

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#include "te_ethernet_phy.h"

/* TA name pointer */
extern char *ta_name;

/* Auxiliary buffer */
static char  buf[2048 * 32] = {0, };

/** Route is direct "local interface" in terms of RFC 1354 */
#define FORW_TYPE_LOCAL  3

/** Route is indirect "remote destination" in terms of RFC 1354 */
#define FORW_TYPE_REMOTE 4

/** Static ARP entry */
#define ARP_STATIC      4

/** Dynamic ARP entry */
#define ARP_DYNAMIC     3

/** Fast conversion of the network mask to prefix */
#define MASK2PREFIX(mask, prefix)            \
    switch (ntohl(mask))                     \
    {                                        \
        case 0x0: prefix = 0; break;         \
        case 0x80000000: prefix = 1; break;  \
        case 0xc0000000: prefix = 2; break;  \
        case 0xe0000000: prefix = 3; break;  \
        case 0xf0000000: prefix = 4; break;  \
        case 0xf8000000: prefix = 5; break;  \
        case 0xfc000000: prefix = 6; break;  \
        case 0xfe000000: prefix = 7; break;  \
        case 0xff000000: prefix = 8; break;  \
        case 0xff800000: prefix = 9; break;  \
        case 0xffc00000: prefix = 10; break; \
        case 0xffe00000: prefix = 11; break; \
        case 0xfff00000: prefix = 12; break; \
        case 0xfff80000: prefix = 13; break; \
        case 0xfffc0000: prefix = 14; break; \
        case 0xfffe0000: prefix = 15; break; \
        case 0xffff0000: prefix = 16; break; \
        case 0xffff8000: prefix = 17; break; \
        case 0xffffc000: prefix = 18; break; \
        case 0xffffe000: prefix = 19; break; \
        case 0xfffff000: prefix = 20; break; \
        case 0xfffff800: prefix = 21; break; \
        case 0xfffffc00: prefix = 22; break; \
        case 0xfffffe00: prefix = 23; break; \
        case 0xffffff00: prefix = 24; break; \
        case 0xffffff80: prefix = 25; break; \
        case 0xffffffc0: prefix = 26; break; \
        case 0xffffffe0: prefix = 27; break; \
        case 0xfffffff0: prefix = 28; break; \
        case 0xfffffff8: prefix = 29; break; \
        case 0xfffffffc: prefix = 30; break; \
        case 0xfffffffe: prefix = 31; break; \
        case 0xffffffff: prefix = 32; break; \
         /* Error indication */              \
        default: prefix = 33; break;         \
    }

/** Fast conversion of the prefix to network mask */
#define PREFIX2MASK(prefix) htonl(prefix == 0 ? 0 : (~0) << (32 - (prefix)))

/* TA name pointer */
extern char *ta_name;

#define METRIC_DEFAULT  20

/* Version of the driver */
#define DRIVER_VERSION_UNKNOWN 0
#define DRIVER_VERSION_2_1 1
#define DRIVER_VERSION_2_2 2
#define DRIVER_VERSION_2_3 3

/*
 * Access routines prototypes (comply to procedure types
 * specified in rcf_ch_api.h).
 */
static te_errno interface_list(unsigned int, const char *,
                               const char *, char **);

static te_errno net_addr_add(unsigned int, const char *, const char *,
                             const char *, const char *);
static te_errno net_addr_del(unsigned int, const char *,
                             const char *, const char *);
static te_errno net_addr_list(unsigned int, const char *,
                              const char *, char **,
                              const char *);

static te_errno prefix_get(unsigned int, const char *, char *,
                           const char *, const char *);
static te_errno prefix_set(unsigned int, const char *, const char *,
                           const char *, const char *);


static te_errno broadcast_get(unsigned int, const char *, char *,
                              const char *, const char *);
static te_errno broadcast_set(unsigned int, const char *, const char *,
                              const char *, const char *);

static te_errno link_addr_get(unsigned int, const char *, char *,
                              const char *);

static te_errno bcast_link_addr_get(unsigned int, const char *,
                                    char *, const char *);

static te_errno ifindex_get(unsigned int, const char *, char *,
                            const char *);

static te_errno status_get(unsigned int, const char *, char *,
                           const char *);
static te_errno status_set(unsigned int, const char *, const char *,
                           const char *);

static te_errno promisc_get(unsigned int, const char *, char *,
                            const char *);
static te_errno promisc_set(unsigned int, const char *, const char *,
                            const char *);

static te_errno mtu_get(unsigned int, const char *, char *,
                        const char *);
static te_errno mtu_set(unsigned int, const char *, const char *,
                        const char *);

static te_errno neigh_state_get(unsigned int, const char *, char *,
                                const char *, const char *);

static te_errno neigh_get(unsigned int, const char *, char *,
                          const char *, const char *);
static te_errno neigh_set(unsigned int, const char *, const char *,
                          const char *, const char *);
static te_errno neigh_add(unsigned int, const char *, const char *,
                          const char *, const char *);
static te_errno neigh_find(const char *, const char *, const char *,
                          char *);
static te_errno neigh_del(unsigned int, const char *,
                          const char *, const char *);
static te_errno neigh_list(unsigned int, const char *,
                           const char *, char **,
                           const char *);

//Multicast
static te_errno mcast_link_addr_add(unsigned int, const char *,
                                    const char *, const char *,
                                    const char *);
static te_errno mcast_link_addr_del(unsigned int, const char *,
                                    const char *, const char *);
static te_errno mcast_link_addr_list(unsigned int, const char *,
                                     const char *, char **,
                                     const char *);

/* VLANS */
static te_errno vlan_ifname_get(unsigned int , const char *,
                                char *, const char *, const char *);
static te_errno vlans_list(unsigned int, const char *,
                           const char *, char **, const char *);
static te_errno vlans_add(unsigned int, const char *, const char *,
                          const char *, const char *);
static te_errno vlans_del(unsigned int, const char *, const char *,
                          const char *);
static int set_vlan_reg(const char *ifname, int vlan_id);
static int remove_vlan_reg(const char *ifname, int vlan_id);

//PHY support
extern te_errno ta_unix_conf_phy_init();

static te_errno phy_state_get(unsigned int, const char *, char *,
                              const char *);

static te_errno phy_speed_get(unsigned int, const char *, char *,
                              const char *);
static te_errno phy_speed_set(unsigned int, const char *, const char *,
                              const char *);

static te_errno phy_autoneg_get(unsigned int, const char *, char *,
                                const char *);
static te_errno phy_autoneg_set(unsigned int, const char *, const char *,
                                const char *);

static te_errno phy_duplex_get(unsigned int, const char *, char *,
                               const char *);
static te_errno phy_duplex_set(unsigned int, const char *, const char *,
                               const char *);

static te_errno phy_commit(unsigned int, const cfg_oid *);

static int get_settings_path(char *path);

static int phy_parameters_get(const char *ifname);

static int phy_parameters_set(const char *ifname);

static int get_driver_version();

char * ifindex2frname(DWORD ifindex);
static DWORD frname2ifindex(const char *ifname);

static unsigned int speed_duplex_state = 0;
static unsigned int speed_duplex_to_set = 0;
static rcf_pch_cfg_object node_phy;

//Multicast

/*
 * This is a bit of hack - there are same handlers for static and dynamic
 * branches, handler discovers dynamic subtree by presence of
 * "dynamic" in OID. But list method does not contain the last subid.
 *
 * FIXME: now there is an argument for the last subid.
 */
static te_errno
neigh_dynamic_list(unsigned int gid, const char *oid,
                   const char *sub_id, char **list,
                   const char *ifname)
{
    UNUSED(oid);
    UNUSED(sub_id);

    return neigh_list(gid, "dynamic", sub_id, list, ifname);
}

static te_errno route_dev_get(unsigned int, const char *, char *,
                              const char *);
static te_errno route_dev_set(unsigned int, const char *, const char *,
                              const char *);
static te_errno route_add(unsigned int, const char *, const char *,
                          const char *);
static te_errno route_del(unsigned int, const char *,
                          const char *);
static te_errno route_get(unsigned int, const char *, char *,
                          const char *);
static te_errno route_set(unsigned int, const char *, const char *,
                          const char *);
static te_errno route_list(unsigned int, const char *, const char *,
                           char **);

static te_errno route_commit(unsigned int, const cfg_oid *);

/*
 * Access routines prototypes (comply to procedure types
 * specified in rcf_ch_api.h).
 */
static te_errno env_get(unsigned int, const char *, char *,
                        const char *);
static te_errno env_set(unsigned int, const char *, const char *,
                        const char *);
static te_errno env_add(unsigned int, const char *, const char *,
                        const char *);
static te_errno env_del(unsigned int, const char *,
                        const char *);
static te_errno env_list(unsigned int, const char *,
                         const char *, char **);

/** Environment variables hidden in list operation */
static const char * const env_hidden[] = {
    "SSH_CLIENT",
    "SSH_CONNECTION",
    "SUDO_COMMAND",
    "TE_RPC_PORT",
    "TE_LOG_PORT",
    "TARPC_DL_NAME",
    "TCE_CONNECTION",
    "TZ",
    "_",
    "CYGWIN"
};

static te_errno uname_get(unsigned int, const char *, char *);

/** win32 statistics */
#ifndef ENABLE_IFCONFIG_STATS
#define ENABLE_IFCONFIG_STATS
#endif

#ifndef ENABLE_NET_SNMP_STATS
#define ENABLE_NET_SNMP_STATS
#endif

typedef struct ndis_stats_s {
  uint64_t gen_broadcast_bytes_rcv;
  uint64_t gen_broadcast_bytes_xmit;
  uint64_t gen_broadcast_frames_rcv;
  uint64_t gen_broadcast_frames_xmit;
  uint64_t gen_directed_bytes_rcv;
  uint64_t gen_directed_bytes_xmit;
  uint64_t gen_directed_frames_rcv;
  uint64_t gen_directed_frames_xmit;
  uint64_t gen_multicast_bytes_rcv;
  uint64_t gen_multicast_bytes_xmit;
  uint64_t gen_multicast_frames_rcv;
  uint64_t gen_multicast_frames_xmit;
  uint64_t gen_rcv_crc_error;
  uint64_t gen_rcv_error;
  uint64_t gen_rcv_no_buffer;
  uint64_t gen_rcv_ok;
  uint64_t gen_xmit_error;
  uint64_t gen_xmit_ok;
  uint64_t eth_rcv_error_alignment;
  uint64_t eth_rcv_overrun;
  uint64_t eth_xmit_heartbeat_failure;
  uint64_t eth_xmit_late_collisions;
  uint64_t eth_xmit_max_collisions;
  uint64_t eth_xmit_more_collisions;
  uint64_t eth_xmit_deferred;
  uint64_t eth_xmit_one_collision;
  uint64_t eth_xmit_times_crs_lost;
  uint64_t eth_xmit_underrun;
  uint64_t gen_transmit_queue_length;
  ULONG gen_link_speed;
} ndis_stats;

typedef struct if_stats {
    uint64_t      in_octets;
    uint64_t      in_ucast_pkts;
    uint64_t      in_nucast_pkts;
    uint64_t      in_discards;
    uint64_t      in_errors;
    uint64_t      in_unknown_protos;
    uint64_t      out_octets;
    uint64_t      out_ucast_pkts;
    uint64_t      out_nucast_pkts;
    uint64_t      out_discards;
    uint64_t      out_errors;
} if_stats;

/* VLANS */
#define MAX_VLANS 0xfff
#define TAG_PRI_ONLY 0x1000
#define TAG_VLAN_ONLY 0x2000
static int vlans_2_1_buffer[5];
static size_t n_2_1_vlans = 0;

static te_errno if_stats_get(const char *ifname, if_stats *stats,
                             ndis_stats *raw_stats);

#ifdef ENABLE_IFCONFIG_STATS
extern te_errno ta_win32_conf_net_if_stats_init();
#endif

#ifdef ENABLE_NET_SNMP_STATS
extern te_errno ta_win32_conf_net_snmp_stats_init();
#endif

/* WMI Support */
#ifdef _WIN32

#ifndef ENABLE_WMI_SUPPORT
#define ENABLE_WMI_SUPPORT
#endif

#ifdef ENABLE_WMI_SUPPORT
typedef int (*t_wmi_init_wbem_objs)(void);
typedef int (*t_wmi_uninit_wbem_objs)(void);
typedef te_errno (*t_wmi_get_vlan_list)(char * , DWORD **, int *);
typedef char * (*t_wmi_get_frname_by_vlanid)(DWORD vlanid);
typedef DWORD (*t_wmi_get_vlanid_by_frname)(const char *ifname);
typedef te_errno (*t_wmi_add_vlan)(DWORD vlan_id, te_bool priority);
typedef te_errno (*t_wmi_del_vlan)(DWORD vlan_id);
typedef int(*t_wmi_mtu_set)(const char* frname, int value);


/* Defining function pointers to imported functions*/
#define GEN_IMP_FUNC_PTR(_fname) \
t_##_fname p##_fname;
GEN_IMP_FUNC_PTR(wmi_init_wbem_objs);
GEN_IMP_FUNC_PTR(wmi_uninit_wbem_objs);
GEN_IMP_FUNC_PTR(wmi_get_vlan_list);
GEN_IMP_FUNC_PTR(wmi_get_frname_by_vlanid);
GEN_IMP_FUNC_PTR(wmi_get_vlanid_by_frname);
GEN_IMP_FUNC_PTR(wmi_add_vlan);
GEN_IMP_FUNC_PTR(wmi_del_vlan);
GEN_IMP_FUNC_PTR(wmi_mtu_set);
#undef GEN_IMP_FUNC_PTR

static te_bool wmi_imported = FALSE;

/** Function used to initalize function poiners with respective
  * function addresses from talib.
  * If import was successfull it marks wmi_imported as TRUE.
  *
  * After successful import , pwmi_* funcs can be used as usual.
  */
static te_bool wmi_init_func_imports(void)
{
    wmi_imported = TRUE;
#define IMPORT_FUNC(_fname)                                   \
    do {                                                      \
        if (!wmi_imported)                                    \
            break;                                            \
        if ((p##_fname = rcf_ch_symbol_addr(#_fname, TRUE)) == 0)\
        {                                                     \
            ERROR("No %s function exported. "                  \
                 "WMI support will be disabled", #_fname);    \
            wmi_imported = FALSE;                             \
        }                                                     \
    } while (0)

    IMPORT_FUNC(wmi_init_wbem_objs);
    IMPORT_FUNC(wmi_uninit_wbem_objs);
    IMPORT_FUNC(wmi_get_vlan_list);
    IMPORT_FUNC(wmi_get_frname_by_vlanid);
    IMPORT_FUNC(wmi_get_vlanid_by_frname);
    IMPORT_FUNC(wmi_add_vlan);
    IMPORT_FUNC(wmi_del_vlan);
    IMPORT_FUNC(wmi_mtu_set);

    return wmi_imported;
#undef IMPORT_FUNC
}
#endif //ENABLE_WMI_SUPPORT

#endif //_WIN32

/* win32 Test Agent configuration tree */


static rcf_pch_cfg_object node_route;

RCF_PCH_CFG_NODE_RWC(node_route_dev, "dev", NULL, NULL,
                     route_dev_get, route_dev_set, &node_route);
static rcf_pch_cfg_object node_route =
    { "route", 0, &node_route_dev, NULL,
      (rcf_ch_cfg_get)route_get, (rcf_ch_cfg_set)route_set,
      (rcf_ch_cfg_add)route_add, (rcf_ch_cfg_del)route_del,
      (rcf_ch_cfg_list)route_list,
      (rcf_ch_cfg_commit)route_commit, NULL, NULL};

RCF_PCH_CFG_NODE_RO(node_neigh_state, "state",
                    NULL, NULL,
                    (rcf_ch_cfg_list)neigh_state_get);

static rcf_pch_cfg_object node_neigh_dynamic =
    { "neigh_dynamic", 0, &node_neigh_state, NULL,
      (rcf_ch_cfg_get)neigh_get, (rcf_ch_cfg_set)neigh_set,
      (rcf_ch_cfg_add)neigh_add, (rcf_ch_cfg_del)neigh_del,
      (rcf_ch_cfg_list)neigh_dynamic_list, NULL, NULL, NULL};

static rcf_pch_cfg_object node_neigh_static =
    { "neigh_static", 0, NULL, &node_neigh_dynamic,
      (rcf_ch_cfg_get)neigh_get, (rcf_ch_cfg_set)neigh_set,
      (rcf_ch_cfg_add)neigh_add, (rcf_ch_cfg_del)neigh_del,
      (rcf_ch_cfg_list)neigh_list, NULL, NULL, NULL};

//Multicast
static rcf_pch_cfg_object node_mcast_link_addr =
 { "mcast_link_addr", 0, NULL, &node_neigh_static,
  NULL, NULL, (rcf_ch_cfg_add)mcast_link_addr_add,
  (rcf_ch_cfg_del)mcast_link_addr_del,
  (rcf_ch_cfg_list)mcast_link_addr_list, NULL, NULL, NULL};
//

/* VLANS */
RCF_PCH_CFG_NODE_RO(node_vl_ifname, "ifname", NULL, NULL,
                    vlan_ifname_get);
RCF_PCH_CFG_NODE_COLLECTION(node_vlans, "vlans",
                            &node_vl_ifname, &node_mcast_link_addr,
                            vlans_add, vlans_del,
                            vlans_list, NULL);

RCF_PCH_CFG_NODE_RW(node_promisc, "promisc", NULL, &node_vlans,
                    promisc_get, promisc_set);

RCF_PCH_CFG_NODE_RW(node_status, "status", NULL, &node_promisc,
                    status_get, status_set);

RCF_PCH_CFG_NODE_RW(node_mtu, "mtu", NULL, &node_status,
                    mtu_get, mtu_set);

RCF_PCH_CFG_NODE_RO(node_bcast_link_addr, "bcast_link_addr", NULL,
                    &node_mtu, bcast_link_addr_get);

RCF_PCH_CFG_NODE_RO(node_link_addr, "link_addr", NULL,
                    &node_bcast_link_addr,
                    link_addr_get);

RCF_PCH_CFG_NODE_RW(node_broadcast, "broadcast", NULL, NULL,
                    broadcast_get, broadcast_set);
RCF_PCH_CFG_NODE_RW(node_prefix, "prefix", NULL, &node_broadcast,
                    prefix_get, prefix_set);

static rcf_pch_cfg_object node_net_addr =
    { "net_addr", 0, &node_prefix, &node_link_addr,
      (rcf_ch_cfg_get)prefix_get, (rcf_ch_cfg_set)prefix_set,
      (rcf_ch_cfg_add)net_addr_add, (rcf_ch_cfg_del)net_addr_del,
      (rcf_ch_cfg_list)net_addr_list, NULL, NULL, NULL };

RCF_PCH_CFG_NODE_RO(node_ifindex, "index", NULL, &node_net_addr,
                    ifindex_get);

RCF_PCH_CFG_NODE_COLLECTION(node_interface, "interface",
                            &node_ifindex, &node_route,
                            NULL, NULL, interface_list, NULL);

static rcf_pch_cfg_object node_env =
    { "env", 0, NULL, &node_interface,
      (rcf_ch_cfg_get)env_get, (rcf_ch_cfg_set)env_set,
      (rcf_ch_cfg_add)env_add, (rcf_ch_cfg_del)env_del,
      (rcf_ch_cfg_list)env_list, NULL, NULL, NULL };

RCF_PCH_CFG_NODE_RO(node_uname, "uname", NULL, &node_env, uname_get);

const char *te_lockdir = "/tmp";

/** Mapping of EF ports to interface indices */
static DWORD ef_index[2] = { 0, 0 };
static char ef_regpath[2][1024];

/*
   Structure to save manualy added arp list entries
*/

typedef struct neigh_st_entry_s{
  MIB_IPNETROW val;
  struct neigh_st_list_entry_s *next;
}neigh_st_entry, *pneigh_st_entry;

static pneigh_st_entry gl_st_list_head = NULL;


/* Adds an entry to the static ARP entries list */
static void add_static_neigh(MIB_IPNETROW *to_add)
{
  pneigh_st_entry* p;
  pneigh_st_entry new_entry;

  new_entry = (pneigh_st_entry) malloc(sizeof(neigh_st_entry));
  p = &gl_st_list_head;

  while(*p != NULL){
    if (((*p)->val.dwAddr == to_add->dwAddr)&&
        ((*p)->val.dwType == to_add->dwType)&&
        ((*p)->val.dwIndex == to_add->dwIndex))
    {
        memcpy(&((*p)->val.bPhysAddr), &(to_add->bPhysAddr),
               MAXLEN_PHYSADDR);
        return;
    }
    p = &((*p)->next);
  }

  *p = new_entry;
  (*p)->next = NULL;
  memcpy(&((*p)->val), to_add, sizeof(MIB_IPNETROW));
}

/* Remove an entry from the static ARP entries list */
static te_errno
delete_neigh_st_entry(MIB_IPNETROW* ip_entry){
  pneigh_st_entry *p;
  pneigh_st_entry t;

  p = &gl_st_list_head;
  while(*p != NULL){
    if (((*p)->val.dwAddr == ip_entry->dwAddr)&&
        ((*p)->val.dwType == ip_entry->dwType)&&
        ((*p)->val.dwIndex == ip_entry->dwIndex) )
    {
      t = *p;
      *p = (*p)->next;
      free(t);
      return 0;
    }
    p = &((*p)->next);
  }
  return TE_RC(TE_TA_WIN32, TE_ENOENT);
}

/*

  Return a list of the static entries in ARP cache,
  i.e. only the entries we've entered manually

*/
static te_errno
neigh_st_list(char** list, const char* ifname)
{
    char mac[30];
    DWORD ifindex = ifname2ifindex(ifname);
    char* t = buf;
    pneigh_st_entry p;

    buf[0] = 0;
    for (p = gl_st_list_head; p != NULL; p = p->next)
    {
        if (p->val.dwPhysAddrLen != 6 ||
            p->val.dwIndex != ifindex ||
            p->val.dwAddr == 0xFFFFFFFF ||
            neigh_find("static", ifname,
                       inet_ntoa(*(struct in_addr *) &(p->val.dwAddr)),
                       mac) != 0)
        {

            continue;

        }
        t += snprintf(t, sizeof(buf) - (t - buf), "%s ",
                      inet_ntoa(*(struct in_addr *)
                                    &(p->val.dwAddr)));
    }


    if ((*list = strdup(buf)) == NULL)
      return TE_RC(TE_TA_WIN32, TE_ENOMEM);

    return 0;
}

/*
   Flushes static ARP entries list.
*/
void flush_neigh_st_list()
{
    pneigh_st_entry p1, p2 = NULL;


    for (p1 = gl_st_list_head; p1 != NULL; p1 = p1->next) {
        if (p2 != NULL)
            free(p2);
        p2 = p1;
    }

}

/* Convert wide string to usual one */
static char *
w2a(WCHAR *str)
{
    static char buf[256];
    BOOL        b;

    WideCharToMultiByte(CP_ACP, 0, str, -1, buf, sizeof(buf), "-", &b);

    return buf;
}

static int
intfdata2file(const char *prefix, int efindex, int ifindex,
              const char *guid, const unsigned char *mac,
              const char *regpath, int efvlan)
{
  FILE *F;
  char filename[100];

  if (strncmp(prefix, "ef", 2) == 0)
  {
    if (efvlan > 0)
      sprintf(filename, "/tmp/efdata_%d.%d", efindex + 1, efvlan);
    else
      sprintf(filename, "/tmp/efdata_%d", efindex + 1);
  }
  else
  {
    sprintf(filename, "/tmp/intfdata_%d", ifindex);
  }

  F = fopen(filename, "wt");
  if (NULL == F)
  {
    return -1;
  }
  fprintf(F, "%d\n%s\n", ifindex, guid);
  fprintf(F, "%02x:%02x:%02x:%02x:%02x:%02x\n",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  fprintf(F, "%s\n", regpath);
  fclose(F);

  return 0;
}

static te_errno
efport2ifindex(void)
{
/* Path to network components in the registry */
#define NET_PATH        "SYSTEM\\CurrentControlSet\\Control\\Class\\" \
                        "{4D36E972-E325-11CE-BFC1-08002bE10318}"
#define ENV_PATH        "SYSTEM\\CurrentControlSet\\Control\\" \
                        "Session Manager\\Environment"

#define NDIS_EFAB       "dev_c101_ndis_"
#define NDIS_SF_2_1         "sfe_ndis_"
//#define NDIS_SF_2_2         "ndis_basic"
#define NDIS_SF_2_2     "{c641c770-faac-44ed-9c73-48d1b5e59200}"
#define BUFSIZE         512
#define AMOUNT_OF_GUIDS         5

    HKEY key, subkey;

    static char subkey_name[BUFSIZE];
    static char subkey_path[BUFSIZE];
    static char value[BUFSIZE];

    static char guid1[AMOUNT_OF_GUIDS][BUFSIZE];
    static char guid2[AMOUNT_OF_GUIDS][BUFSIZE];
    static char guid_2_2[AMOUNT_OF_GUIDS][BUFSIZE];
    static char guid1_regpath[AMOUNT_OF_GUIDS][BUFSIZE];
    static char guid2_regpath[AMOUNT_OF_GUIDS][BUFSIZE];
    static char guid_2_2_regpath[AMOUNT_OF_GUIDS][BUFSIZE];
    static int driver_type_reported = 0;

    static int guid1_amount = 0, guid2_amount = 0, guid_2_2_amount = 0;
    static int guids_found = 0;

    DWORD subkey_size;
    DWORD value_size = BUFSIZE;

    FILETIME tmp;
    ULONG    size = 0;
    DWORD    rc;

    PIP_INTERFACE_INFO iftable;
    PIP_ADAPTER_INFO  adapters, info;

    static unsigned char mac1[6], mac2[6];
    static char mac_str[100];

    char *ifname = NULL;
    int efvlan = 0;

    char driver_type[BUFSIZE];

    int i, j;
    unsigned int old_ef_index[2];
    int guid1_found_index, guid2_found_index;

    driver_type[0] = 0;

    /* Querying environment variable TE_USE_EFAB_DRIVER value */

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, ENV_PATH, 0,
                     KEY_READ, &key) != ERROR_SUCCESS)
    {
        ERROR("RegOpenKeyEx() failed with errno %lu", GetLastError());
        return TE_RC(TE_TA_WIN32, TE_EFAULT);
    }
    RegQueryValueEx(key, "TE_USE_EFAB_DRIVER",
                            NULL, NULL, driver_type,
                            &value_size);
    RegCloseKey(key);

    if (driver_type[0] != 0)
    {
      int t = atoi(driver_type);
      if (t)
      {
        strcpy(driver_type, NDIS_EFAB);
        if (!driver_type_reported)
        {
          RING("Efab drivers will be used to resolve ef* interfaces,"
               " if any");
        }
      }
      else
      {
        strcpy(driver_type, NDIS_SF_2_1);
        if (!driver_type_reported)
        {
          RING("Vendor drivers will be used to resolve ef* interfaces,"
               " if any");
        }
      }
    }
    else
    {
      strcpy(driver_type, NDIS_SF_2_1);
      if (!driver_type_reported)
      {
        RING("Vendor drivers will be used to resolve ef* interfaces, "
             "if any");
      }
    }
    driver_type_reported = 1;

    if (!guids_found)
    {
        /* Obtaining interface indexes */
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, NET_PATH, 0,
                         KEY_READ, &key) != ERROR_SUCCESS)
        {
            ERROR("RegOpenKeyEx() failed with errno %lu", GetLastError());
            return TE_RC(TE_TA_WIN32, TE_EFAULT);
        }

        for (i = 0, subkey_size = value_size = BUFSIZE;
             RegEnumKeyEx(key, i, subkey_name, &subkey_size,
                          NULL, NULL, NULL, &tmp) != ERROR_NO_MORE_ITEMS;
             i++, subkey_size = value_size = BUFSIZE)
        {
            sprintf(subkey_path, "%s\\%s", NET_PATH, subkey_name);
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subkey_path,
                             0, KEY_READ, &subkey) != ERROR_SUCCESS)
            {
               continue;
            }

            if (RegQueryValueEx(subkey, "MatchingDeviceId",
                                NULL, NULL, (unsigned char *)value,
                                &value_size) != ERROR_SUCCESS)
            {
                /* Field with device ID is absent, may its virtual device */
                RegCloseKey(subkey);
                continue;
            }

            if ((strstr(value, driver_type) != NULL))
            {
                char driver[BUFSIZE];
                unsigned char *guid, *guid_regpath;

                strcpy(driver, driver_type);
                strcat(driver, "0");

                if (strstr(value, driver) != NULL)
                {
                  guid = guid1[guid1_amount];
                  guid_regpath = guid1_regpath[guid1_amount];
                  guid1_amount++;
                }
                else
                {
                  guid = guid2[guid2_amount];
                  guid_regpath = guid2_regpath[guid2_amount];
                  guid2_amount++;
                }

                value_size = BUFSIZE;
                strcpy(guid_regpath, subkey_path);

                if (RegQueryValueEx(subkey, "NetCfgInstanceId",
                                    NULL, NULL, guid, &value_size) != 0)
                {
                    ERROR("RegQueryValueEx(%s) failed with errno %u",
                          subkey_path, GetLastError());
                    RegCloseKey(subkey);
                    RegCloseKey(key);
                    return TE_RC(TE_TA_WIN32, TE_EFAULT);
                }
            }
            else /* Try to find path to SF 2.2 driver */
            if ((strstr(value, NDIS_SF_2_2) != NULL))
            {
                unsigned char *guid, *guid_regpath;
                value_size = BUFSIZE;

                guid = guid1[guid_2_2_amount];
                guid_regpath = guid1_regpath[guid_2_2_amount];
                guid_2_2_amount++;

                strcpy(guid_regpath, subkey_path);
                if (RegQueryValueEx(subkey, "NetCfgInstanceId",
                                    NULL, NULL, guid, &value_size) != 0)
                {
                    ERROR("RegQueryValueEx(%s) failed with errno %u",
                          subkey_path, GetLastError());
                    RegCloseKey(subkey);
                    RegCloseKey(key);
                    return TE_RC(TE_TA_WIN32, TE_EFAULT);
                }
            }
            RegCloseKey(subkey);
        }
        RegCloseKey(key);

        if (guid1_amount == 0 || guid2_amount == 0)
            if (guid_2_2_amount == 0)
            {
                return 0;
            }

        guids_found = 1;
    }
    //RING("SFCResolve: guid1='%s', guid2='%s'", guid1[0], guid2[0]);

    if ((iftable = (PIP_INTERFACE_INFO)malloc(sizeof(*iftable))) == NULL)
        return TE_RC(TE_TA_WIN32, TE_ENOMEM);

    if (GetInterfaceInfo(iftable, &size) == ERROR_INSUFFICIENT_BUFFER)
    {
        PIP_INTERFACE_INFO new_table =
            (PIP_INTERFACE_INFO)realloc(iftable, size);

        if (new_table == NULL)
        {
            free(iftable);
            return TE_RC(TE_TA_WIN32, TE_ENOMEM);
        }

        iftable = new_table;
    }
    else
    {
        free(iftable);
        ERROR("GetInterfaceInfo() failed");
        return TE_RC(TE_TA_WIN32, TE_ENOMEM);
    }

    if (GetInterfaceInfo(iftable, &size) != NO_ERROR)
    {
        ERROR("GetInterfaceInfo() failed");
        return TE_RC(TE_TA_WIN32, TE_ENOMEM);
    }

    old_ef_index[0] = ef_index[0];
    old_ef_index[1] = ef_index[1];
    ef_index[0] = 0;
    ef_index[1] = 0;
    guid1_found_index = -1;
    guid2_found_index = -1;
    //RING("SFCResolve: numadapters=%d", iftable->NumAdapters);
    for (i = 0; i < iftable->NumAdapters; i++)
    {
        for(j = 0; j < guid1_amount; j++)
        {
          if (strstr(w2a(iftable->Adapter[i].Name), guid1[j]) != NULL)
          {
            ef_index[0] = iftable->Adapter[i].Index;
            guid1_found_index = j;
          }
        }
        for(j = 0; j < guid2_amount; j++)
        {
          if (strstr(w2a(iftable->Adapter[i].Name), guid2[j]) != NULL)
          {
            ef_index[1] = iftable->Adapter[i].Index;
            guid2_found_index = j;
          }
        }
        /* Try to find index for 2.2 version driver */
        for(j = 0; j < guid_2_2_amount; j++)
        {
          if (strstr(w2a(iftable->Adapter[i].Name), guid1[j]) != NULL)
          {
            ef_index[0] = iftable->Adapter[i].Index;
            guid1_found_index = j;
          }
        }
    }
    free(iftable);

    GetAdaptersInfo(NULL, &size);
    adapters = (PIP_ADAPTER_INFO)malloc(size);

    if ((rc = GetAdaptersInfo(adapters, &size)) != NO_ERROR)
    {
        ERROR("GetAdaptersInfo failed, error %d", rc);
        free(adapters);
        return TE_RC(TE_TA_WIN32, TE_ENOMEM);
    }

    for (info = adapters; info != NULL; info = info->Next)
    {
      ifname = ifindex2ifname(info->Index);
      if (ifname == NULL)
      {
        ERROR("Can't get ifname for ifindex=%d", info->Index);
      }
      if ((ifname != NULL) &&
          (strncmp(ifname, "ef", 2) == 0))
      {
        if (strstr(ifname, ".") != NULL)
        {
          sscanf(strstr(ifname, ".") + 1, "%d", &efvlan);
          intfdata2file("ef", 0, info->Index, info->AdapterName,
                        info->Address, "", efvlan);
        }
      }
      else
        intfdata2file("intf", -1, info->Index, info->AdapterName,
                      info->Address, "", 0);
#if 0
      if ((guid_2_2_found_index >= 0) && (ef_index[0] == info->Index))
      {
        memcpy(mac1, info->Address, 6);
      }
#endif
      if ((guid1_found_index >= 0) && (ef_index[0] == info->Index))
      {
        memcpy(mac1, info->Address, 6);
      }
      if ((guid2_found_index >= 0) && (ef_index[1] == info->Index))
      {
        memcpy(mac2, info->Address, 6);
      }
    }
    free(adapters);
#if 0
    if (ef_index_2_2 > 0)
    {
        if (old_ef_index_2_2 != ef_index_2_2)
        {
            strcpy(ef_regpath[0], guid_2_2_regpath[guid_2_2_found_index]);
            intfdata2file("ef", 0, ef_index_2_2,
                          guid_2_2[guid_2_2_found_index],
                          mac1, ef_regpath[0], 0);
            sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                    mac1[0], mac1[1], mac1[2],
                    mac1[3], mac1[4], mac1[5]);
            RING("Interface index for EF port 1: %d, "
                 "MAC: %s, regpath: %s", ef_index_2_2, mac_str,
                 ef_regpath[0]);
        }
    }
    else
    {
        if (old_ef_index_2_2 != 0)
        {
            RING("Can't find index for EF port 1");
        }
    }
#endif
    if (ef_index[0] > 0)
    {
        if (old_ef_index[0] != ef_index[0])
        {
            strcpy(ef_regpath[0], guid1_regpath[guid1_found_index]);
            intfdata2file("ef", 0, ef_index[0],
                          guid1[guid1_found_index], mac1, ef_regpath[0], 0);
            sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                    mac1[0], mac1[1], mac1[2],
                    mac1[3], mac1[4], mac1[5]);
            RING("Interface index for EF port 1: %d, "
                 "MAC: %s, regpath: %s", ef_index[0], mac_str,
                 ef_regpath[0]);
        }
    }
    else
    {
        if (old_ef_index[0] != 0)
        {
            RING("Can't find index for EF port 1");
        }
    }

    if (ef_index[1] > 0)
    {
        if (old_ef_index[1] != ef_index[1])
        {
            strcpy(ef_regpath[1], guid2_regpath[guid2_found_index]);
            intfdata2file("ef", 1, ef_index[1],
                          guid2[guid2_found_index], mac2, ef_regpath[1], 0);
            sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
                    mac2[0], mac2[1], mac2[2],
                    mac2[3], mac2[4], mac2[5]);
            RING("Interface index for EF port 2: %d, "
                 "MAC: %s, regpath: %s", ef_index[1], mac_str,
                 ef_regpath[1]);
        }
    }
    else
    {
        if (old_ef_index[1] != 0)
        {
            RING("Can't find index for EF port 2");
        }
    }

    return 0;

#undef AMOUNT_OF_GUIDS
#undef BUFSIZE
#undef NET_PATH
#undef NDIS_SF_2_1
#undef NDIS_SF_2_2
#undef NDIS_EFAB
}

static MIB_IFROW if_entry;

/** Convert interface name to interface index */
static DWORD
ifname2ifindex(const char *ifname)
{
    char   *s;
    char   *tmp;
    te_bool ef = FALSE;
    DWORD   index, vlan_id;
    char *friendly_name;

    if (ifname == NULL)
        return 0;

    if (strcmp_start("intf", ifname) == 0)
        s = (char *)ifname + strlen("intf");
    else if (strcmp_start("ef", ifname) == 0)
    {
        if (strcmp_start("ef1.", ifname) == 0)
        {
            /* Interface is VLAN */
            s = (char *)ifname + strlen("ef1.");
            vlan_id = strtol(s, &tmp, 10);
            if (tmp == s || *tmp != 0)
                return 0;
            if (!wmi_imported)
                return 0;
            friendly_name = pwmi_get_frname_by_vlanid(vlan_id);
            if (friendly_name != NULL)
            {
                index = frname2ifindex(friendly_name);
                if (index > 0)
                    return index;
                else
                    return 0;
            }
            else
                return 0;
        }
        else
        {
            s = (char *)ifname + strlen("ef");
            ef = TRUE;
        }
    }
    else
        return 0;
    index = strtol(s, &tmp, 10);
    if (tmp == s || *tmp != 0)
        return 0;

    if (!ef)
        return index;

    if (index < 1 || index > 2)
        return 0;

    efport2ifindex();

    return ef_index[index - 1];
}

/** Convert interface index to interface name */
char *
ifindex2ifname(DWORD ifindex)
{
    static char ifname[16];
    char *friendly_name;
    DWORD vlan_id = -1;

    friendly_name = ifindex2frname(ifindex);
    if (friendly_name == NULL)
    {
        ERROR("ifindex2frname failed");
        return NULL;
    }
    if (strcmp_start("Vendor Virtual", friendly_name) == 0)
    {
        if (!wmi_imported)
        {
            return NULL;
        }
        vlan_id = pwmi_get_vlanid_by_frname(friendly_name);
        if (vlan_id > 0)
        {
            sprintf(ifname, "ef1.%d", vlan_id);
            return ifname;
        }
        else
        {
            ERROR("wmi_get_vlanid_by_name failed");
            return NULL;
        }
    }

    if (ef_index[0] == ifindex)
        sprintf(ifname, "ef1");
    else if (ef_index[1] == ifindex)
        sprintf(ifname, "ef2");
    else
        sprintf(ifname, "intf%u", (unsigned int)ifindex);

    return ifname;
}

/** Update information in if_entry. Local variable ifname should exist */
#define GET_IF_ENTRY \
    do {                                                        \
        if ((if_entry.dwIndex = ifname2ifindex(ifname)) == 0 || \
            GetIfEntry(&if_entry) != 0)                         \
            return TE_RC(TE_TA_WIN32, TE_ENOENT);               \
    } while (0)

#define RETURN_BUF

/**
 * Allocate memory and get some SNMP-like table; variable table of the
 * type _type should be defined.
 */
#define GET_TABLE(_type, _func) \
    do {                                                                \
        DWORD size = 0, rc;                                             \
                                                                        \
        if ((table = (_type *)malloc(sizeof(_type))) == NULL)           \
            return TE_RC(TE_TA_WIN32, TE_ENOMEM);                       \
                                                                        \
        if ((rc = _func(table, &size, 0)) == ERROR_INSUFFICIENT_BUFFER) \
        {                                                               \
            _type *new_table = (_type *)realloc(table, size);           \
                                                                        \
            if (new_table == NULL)                                      \
            {                                                           \
                free(table);                                            \
                return TE_RC(TE_TA_WIN32, TE_ENOMEM);                   \
            }                                                           \
            table = new_table;                                          \
        }                                                               \
        else if (rc == NO_ERROR)                                        \
        {                                                               \
            free(table);                                                \
            table = NULL;                                               \
        }                                                               \
        else                                                            \
        {                                                               \
            ERROR("%s failed, error %d", #_func, rc);                   \
            free(table);                                                \
            return TE_RC(TE_TA_WIN32, TE_EWIN);                         \
        }                                                               \
                                                                        \
        if ((rc = _func(table, &size, 0)) != NO_ERROR)                  \
        {                                                               \
            ERROR("%s failed, error %d", #_func, rc);                   \
            free(table);                                                \
            return TE_RC(TE_TA_WIN32, TE_EWIN);                         \
        }                                                               \
                                                                        \
        if (table->dwNumEntries == 0)                                   \
        {                                                               \
            free(table);                                                \
            table = NULL;                                               \
        }                                                               \
    } while (0)


/** Find an interface for destination IP */
static int
find_ifindex(DWORD addr, DWORD *ifindex)
{
    MIB_IPFORWARDTABLE *table;

    DWORD index = 0;
    DWORD mask_max = 0;
    int   i;

    GET_TABLE(MIB_IPFORWARDTABLE, GetIpForwardTable);

    if (table == NULL)
        return TE_RC(TE_TA_WIN32, TE_ENOENT);

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if ((table->table[i].dwForwardDest &
             table->table[i].dwForwardMask) !=
            (addr &  table->table[i].dwForwardMask))
        {
            continue;
        }

        if (ntohl(table->table[i].dwForwardMask) > mask_max || index == 0)
        {
            mask_max = table->table[i].dwForwardMask;
            index = table->table[i].dwForwardIfIndex;
            if (mask_max == 0xFFFFFFFF)
                break;
        }
    }
    free(table);

    if (index == 0)
        return TE_RC(TE_TA_WIN32, TE_ENOENT);

    *ifindex = index;

    return 0;
}

/**
 * Initialize base configuration.
 *
 * @return Status code.
 */
static inline te_errno
ta_win32_conf_base_init(void)
{
    return rcf_pch_add_node("/agent", &node_uname);
}

/* See the description in lib/rcfpch/rcf_ch_api.h */
int
rcf_ch_conf_init()
{
    static te_bool init = FALSE;

    if (!init)
    {
#ifdef ENABLE_WMI_SUPPORT
        if (get_driver_version() >= DRIVER_VERSION_2_2)
          wmi_init_func_imports();
#endif

        if (efport2ifindex() != 0)
            return -1;

        if (ta_win32_conf_base_init() != 0)
            return -1;

#ifdef RCF_RPC
        rcf_pch_rpc_init(NULL);
#endif
#ifdef WITH_ISCSI
        if (iscsi_initiator_conf_init() != 0)
            return -1;
#endif

#ifdef ENABLE_WMI_SUPPORT
        if (wmi_imported)
            pwmi_init_wbem_objs();
#endif
        rcf_pch_rsrc_init();
        rcf_pch_rsrc_info("/agent/interface",
                          rcf_pch_rsrc_grab_dummy,
                          rcf_pch_rsrc_release_dummy);

#ifdef ENABLE_IFCONFIG_STATS
        if (ta_win32_conf_net_if_stats_init() != 0)
            return -1;
#endif
#ifdef ENABLE_NET_SNMP_STATS
        if (ta_win32_conf_net_snmp_stats_init() != 0)
            return -1;
#endif

        /* Initialize configurator PHY support */
        if (ta_unix_conf_phy_init() != 0)
            return -1;

        init = TRUE;
    }

    return 0;
}

/**
 * Get Test Agent name.
 *
 * @return name pointer
 */
const char *
rcf_ch_conf_agent()
{
    return ta_name;
}

/**
 * Release resources allocated for configuration support.
 */
void
rcf_ch_conf_fini()
{
}

/**
 * Get instance list for object "agent/interface".
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          location for the list pointer
 *
 * @return status code
 * @retval 0            success
 * @retval TE_ENOMEM       cannot allocate memory
 */
static te_errno
interface_list(unsigned int gid, const char *oid,
               const char *sub_id, char **list)
{
//    MIB_IFTABLE *table;
    IP_ADAPTER_INFO *ifinfo, *cur_if;
    DWORD size = 0, rc;
    char        *s = buf;
    int          i;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    efport2ifindex();

    ifinfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
    size = sizeof(IP_ADAPTER_INFO);
    if (ifinfo == NULL)
    {
      return TE_RC(TE_TA_WIN32, TE_ENOMEM);
    }

    rc = GetAdaptersInfo(ifinfo, &size);
    if (rc == ERROR_BUFFER_OVERFLOW)
    {
      free(ifinfo);
      ifinfo = (IP_ADAPTER_INFO *)malloc(size);
      if (ifinfo == NULL)
      {
        return TE_RC(TE_TA_WIN32, TE_ENOMEM);
      }
    }
    else if (rc != ERROR_SUCCESS)
    {
      ERROR("%s failed, error %d", "GetAdaptersInfo", rc);
      free(ifinfo);
      return TE_RC(TE_TA_WIN32, TE_EWIN);
    }

    rc = GetAdaptersInfo(ifinfo, &size);
    if (rc != ERROR_SUCCESS)
    {
      ERROR("%s failed, error %d", "GetAdaptersInfo", rc);
      free(ifinfo);
      return TE_RC(TE_TA_WIN32, TE_EWIN);
    }

    buf[0] = 0;
    cur_if = ifinfo;
    while (cur_if != NULL)
    {
      s += snprintf(s, sizeof(buf) - (s - buf),
                    "%s ", ifindex2ifname(cur_if->Index));
      cur_if = cur_if->Next;
    }

    free(ifinfo);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_WIN32, TE_ENOMEM);


    return 0;
}


char *
ifindex2frname(DWORD ifindex)
{
    static char friendly_name[100] = "";
    DWORD retval = 0;
    MIB_IFROW *info;
    DWORD        size = 0;
    int          i;
    MIB_IFTABLE *iftable;

#if 0
    MIB_IFROW table;
    table.dwIndex = ifindex;

    retval = GetIfEntry(&table);

    if (retval == 0)
    {
        sprintf(friendly_name, "%s", table.bDescr);
        goto success;
    }
#endif

    iftable = (MIB_IFTABLE *)malloc(sizeof(MIB_IFTABLE));

    retval = GetIfTable(iftable, &size, 0);
    iftable = (MIB_IFTABLE *)realloc(iftable, size);
    retval = GetIfTable(iftable, &size, 0);


    for (i = 0; i < iftable->dwNumEntries; i++)
    {
        info = iftable->table + i;
        if (info->dwIndex == ifindex)
        {
            snprintf(friendly_name, TE_ARRAY_LEN(friendly_name),
                     "%s", info->bDescr);
            goto success;
        }

    }

    ERROR("Friendly name not found for interface %d", ifindex);
    return NULL;

success:
    free(iftable);
    return friendly_name;
}

static DWORD
frname2ifindex(const char *ifname)
{
    DWORD retval = 0;
    MIB_IFROW *info;
    DWORD        size = 0;
    int          i;
    MIB_IFTABLE *iftable;
    DWORD ifindex = -1;

    iftable = (MIB_IFTABLE *)malloc(sizeof(MIB_IFTABLE));

    retval = GetIfTable(iftable, &size, 0);
    iftable = (MIB_IFTABLE *)realloc(iftable, size);
    retval = GetIfTable(iftable, &size, 0);

    for (i = 0; i < iftable->dwNumEntries; i++)
    {
        info = iftable->table + i;
        if (strcmp(info->bDescr, ifname) == 0)
        {
            ifindex = info->dwIndex;
            goto success;
        }

    }

success:
    free(iftable);
    return ifindex;

}

/** Convert interface name to interface index */
static int
name2index(const char *ifname, DWORD *ifindex)
{
    GET_IF_ENTRY;

    *ifindex = if_entry.dwIndex;

    return 0;
}

/**
 * Get index of the interface.
 *
 * @param gid           request group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         location for interface index
 * @param ifname        name of the interface (like "eth0")
 *
 * @return status code
 */
static te_errno
ifindex_get(unsigned int gid, const char *oid, char *value,
            const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    GET_IF_ENTRY;

    sprintf(value, "%lu", if_entry.dwIndex);

    return 0;
}

/** Check, if IP address exist for the current if_entry */
static int
ip_addr_exist(DWORD addr, MIB_IPADDRROW *data)
{
    MIB_IPADDRTABLE *table;
    int              i;

    if (addr == 0)
    {
      RING("skip 0.0.0.0 address");
      return TE_RC(TE_TA_WIN32, TE_ENOENT);
    }

    GET_TABLE(MIB_IPADDRTABLE, GetIpAddrTable);

    if (table == NULL)
        return TE_RC(TE_TA_WIN32, TE_ENOENT);

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if (table->table[i].dwIndex == if_entry.dwIndex &&
            table->table[i].dwAddr == addr)
        {
            if (data != NULL)
                *data = table->table[i];
            free(table);
            return 0;
        }
    }

    free(table);

    return TE_RC(TE_TA_WIN32, TE_ENOENT);
}

/** Parse address and fill mask by specified or default value */
static int
get_addr_mask(const char *addr, const char *value, DWORD *p_a, DWORD *p_m)
{
    DWORD a;
    int   prefix;
    char *end;

    if ((a = inet_addr(addr)) == INADDR_NONE ||
        (a & 0xe0000000) == 0xe0000000)
        return TE_RC(TE_TA_WIN32, TE_EINVAL);

    prefix = strtol(value, &end, 10);
    if (value == end || *end != 0)
    {
        ERROR("Invalid value '%s' of prefix length", value);
        return TE_RC(TE_TA_WIN32, TE_EFMT);
    }

    if (prefix > 32)
    {
        ERROR("Invalid prefix '%s' to be set", value);
        return TE_RC(TE_TA_WIN32, TE_EINVAL);
    }

    if (prefix != 0)
        *p_m = PREFIX2MASK(prefix);
    else
        *p_m = (a & htonl(0x80000000)) == 0 ? htonl(0xFF000000) :
               (a & htonl(0xC0000000)) == htonl(0x80000000) ?
               htonl(0xFFFF0000) : htonl(0xFFFFFF00);
     *p_a = a;

     return 0;
}

/** Check that IPv4 address is assigned to specified interface */
static int
check_address(const char *addr, DWORD if_index)
{
    MIB_IPADDRTABLE *table;
    char ifname[20], tmpaddr[30];
    int i;

    int   found = 0;

    sprintf(ifname, ifindex2ifname(if_index));
    GET_IF_ENTRY;
    GET_TABLE(MIB_IPADDRTABLE, GetIpAddrTable);
    if (table == NULL)
    {
      return 0;
    }

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if (table->table[i].dwIndex != if_index)
            continue;
        if (table->table[i].dwAddr == 0)
        {
            RING("skip 0.0.0.0 address");
            continue;
        }

        snprintf(tmpaddr, sizeof(tmpaddr), "%s",
                                    inet_ntoa(*(struct in_addr *)
                                    &(table->table[i].dwAddr)));
        if (strcmp(addr, tmpaddr) == 0)
        {
          found = 1;
          break;
        }
    }
    free(table);

    return found;
}

/**
 * Configure IPv4 address for the interface.
 * If the address does not exist, alias interface is created.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value string (unused)
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return status code
 */
static te_errno
net_addr_add(unsigned int gid, const char *oid, const char *value,
             const char *ifname, const char *addr)
{
#define TIME_TO_WAIT 15 // Time to wait until address appears
                        // on interface
    DWORD a, m;

    ULONG nte_context;
    ULONG nte_instance;

    int rc, i;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = get_addr_mask(addr, value, &a, &m)) != 0)
        return rc;

    GET_IF_ENTRY;

    if ((rc = AddIPAddress(*(IPAddr *)&a, *(IPAddr *)&m, if_entry.dwIndex,
                            &nte_context, &nte_instance)) != NO_ERROR)
    {
      if (rc == ERROR_DUP_DOMAINNAME)
      {
        WARN("AddIpAddress() failed, error ERROR_DUP_DOMAINNAME, addr %s",
             addr);
        return 0;
      }
      if (rc == ERROR_OBJECT_ALREADY_EXISTS)
      {
        WARN("AddIpAddress() failed, error "
             "ERROR_OBJECT_ALREADY_EXISTS, addr %s",
             addr);
        return 0;
      }
      ERROR("AddIpAddress() failed, error %d, addr %s", rc, addr);
      return TE_RC(TE_TA_WIN32, TE_EWIN);
    }

    for (i = 0; i < TIME_TO_WAIT; i++)
    {
      if (check_address(addr, if_entry.dwIndex))
      {
        break;
      }
      sleep(1);
    }
    if (i == TIME_TO_WAIT)
    {
      WARN("AddIpAddress(): IP address didn't appear on interface "
           "after %d seconds", TIME_TO_WAIT);
    }
    else
    {
      RING("AddIpAddress: Address appeared on interface after "
          "%d seconds", i);
    }

    return 0;
}

/**
 * Deletes DHCP address
 *
 */

static te_errno
net_addr_del_dhcp(unsigned int dwIndex)
{
  PIP_INTERFACE_INFO table;
  DWORD size = 0;
  DWORD i;
  int   rc;

  GetInterfaceInfo(NULL, &size);
  table = (PIP_INTERFACE_INFO)malloc(size);

  if ((rc = GetInterfaceInfo(table, &size)) != NO_ERROR)
  {
      ERROR("GetInterfaceInfo failed, error %d", rc);
      free(table);
      return TE_RC(TE_TA_WIN32, TE_EWIN);
  }

  for (i = 0; i < table->NumAdapters; i++)
  {
     if (dwIndex == table->Adapter[i].Index)
     {
        WARN("Try to delete DHCP address adapter ID = %d\n", dwIndex);
        if ((rc = IpReleaseAddress(&table->Adapter[i])) != NO_ERROR)
        {
           ERROR("IpReleaseAddress() failed; error %d, adapterid = %d\n",
                 rc,  table->Adapter[i].Index);
           free(table);
           return TE_RC(TE_TA_WIN32, TE_EWIN);;
        }
       free(table);
       return 0;
     }
  }
  free(table);
  return -1;
}

/**
 * Clear interface address of the down interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return status code
 */

static te_errno
net_addr_del(unsigned int gid, const char *oid,
             const char *ifname, const char *addr)
{
    PIP_ADAPTER_INFO table, info;
    DWORD            size = 0;

    DWORD a;
    int   rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((a = inet_addr(addr)) == INADDR_NONE)
        return TE_RC(TE_TA_WIN32, TE_EINVAL);

    GET_IF_ENTRY;

    GetAdaptersInfo(NULL, &size);
    table = (PIP_ADAPTER_INFO)malloc(size);

    if ((rc = GetAdaptersInfo(table, &size)) != NO_ERROR)
    {
        ERROR("GetAdaptersInfo failed, error %d", rc);
        free(table);
        return TE_RC(TE_TA_WIN32, TE_EWIN);
    }

    for (info = table; info != NULL; info = info->Next)
    {
       IP_ADDR_STRING *addrlist;

       if (info->Index != if_entry.dwIndex)
           continue;

       for (addrlist = &(info->IpAddressList);
            addrlist != NULL;
            addrlist = addrlist->Next)
        {
            if (strcmp(addr, addrlist->IpAddress.String) == 0)
            {
                if ((rc = DeleteIPAddress(addrlist->Context)) != NO_ERROR)
                {
                   WARN("DeleteIPAddress() failed; error %d, addr = %s\n",
                         rc, addr);
                   if (rc == ERROR_GEN_FAILURE)
                   {
                     rc = net_addr_del_dhcp(info->Index);
                     free(table);
                     return rc;
                   }
                   free(table);
                   return TE_RC(TE_TA_WIN32, TE_EWIN);;
                }
                free(table);
                return 0;
            }
        }
    }
    free(table);

    return TE_RC(TE_TA_WIN32, TE_ENOENT);
}

/**
 * Get instance list for object "agent/interface/net_addr".
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param sub_id        ID of the object to be listed
 * @param list          location for the list pointer
 * @param ifname        interface name
 *
 * @return status code
 * @retval 0                    success
 * @retval TE_ENOENT            no such instance
 * @retval TE_ENOMEM               cannot allocate memory
 */
static te_errno
net_addr_list(unsigned int gid, const char *oid,
              const char *sub_id, char **list,
              const char *ifname)
{
    MIB_IPADDRTABLE *table;
    int              i;
    char            *s = buf;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    GET_IF_ENTRY;
    GET_TABLE(MIB_IPADDRTABLE, GetIpAddrTable);

    if (table == NULL)
    {
        if ((*list = strdup(" ")) == NULL)
            return TE_RC(TE_TA_WIN32, TE_ENOMEM);
        else
            return 0;
    }

    buf[0] = 0;

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if (table->table[i].dwIndex != if_entry.dwIndex)
            continue;
        if (table->table[i].dwAddr == 0)
        {
            RING("skip 0.0.0.0 address");
            continue;
        }

        s += snprintf(s, sizeof(buf) - (s - buf), "%s ",
                      inet_ntoa(*(struct in_addr *)
                                    &(table->table[i].dwAddr)));
    }

    free(table);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_WIN32, TE_ENOMEM);

    return 0;
}

/**
 * Get netmask (prefix) of the interface address.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         netmask location (netmask is presented in dotted
 *                      notation)
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return status code
 */
static te_errno
prefix_get(unsigned int gid, const char *oid, char *value,
            const char *ifname, const char *addr)
{
    MIB_IPADDRROW data;
    DWORD         a;
    int           rc;
    int           prefix;

    UNUSED(gid);
    UNUSED(oid);

    if ((a = inet_addr(addr)) == INADDR_NONE)
    {
        if (strcmp(addr, "255.255.255.255") != 0)
            return TE_RC(TE_TA_WIN32, TE_EINVAL);
    }

    GET_IF_ENTRY;

    if ((rc = ip_addr_exist(a, &data)) != 0)
        return rc;

    MASK2PREFIX(data.dwMask, prefix);

    snprintf(value, RCF_MAX_VAL, "%d", prefix);

    return 0;
}

/**
 * Change netmask (prefix) of the interface address.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         pointer to the new network mask in dotted notation
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return status code
 */
static te_errno
prefix_set(unsigned int gid, const char *oid, const char *value,
            const char *ifname, const char *addr)
{
    DWORD a, m;
    int   rc;

    ULONG nte_context;
    ULONG nte_instance;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = get_addr_mask(addr, value, &a, &m)) != 0)
        return rc;

    if ((a = inet_addr(addr)) == INADDR_NONE)
        return TE_RC(TE_TA_WIN32, TE_EINVAL);

    if ((rc = net_addr_del(0, NULL, ifname, addr)) != 0)
        return rc;

    if ((rc = AddIPAddress(*(IPAddr *)&a, *(IPAddr *)&m,
                           if_entry.dwIndex, &nte_context,
                           &nte_instance)) != NO_ERROR)
    {
        ERROR("AddIpAddr() failed, error %d", rc);
        return TE_RC(TE_TA_WIN32, TE_EWIN);
    }

    return 0;
}

/**
 * Get broadcast address of the interface address.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         netmask location (netmask is presented in dotted
 *                      notation)
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return status code
 */
static te_errno
broadcast_get(unsigned int gid, const char *oid, char *value,
              const char *ifname, const char *addr)
{
    MIB_IPADDRROW data;
    DWORD         a;
    DWORD         b;
    int           rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((a = inet_addr(addr)) == INADDR_NONE)
    {
        if (strcmp(addr, "255.255.255.255") != 0)
            return TE_RC(TE_TA_WIN32, TE_EINVAL);
    }

    GET_IF_ENTRY;

    if ((rc = ip_addr_exist(a, &data)) != 0)
        return rc;

    b = (~data.dwMask) | (a & data.dwMask);

    snprintf(value, RCF_MAX_VAL, "%s", inet_ntoa(*(struct in_addr *)&b));

    return 0;
}

/**
 * Change broadcast address of the interface address - does nothing.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         pointer to the new network mask in dotted notation
 * @param ifname        name of the interface (like "eth0")
 * @param addr          IPv4 address in dotted notation
 *
 * @return status code
 */
static te_errno
broadcast_set(unsigned int gid, const char *oid, const char *value,
              const char *ifname, const char *addr)
{
    MIB_IPADDRROW data;
    DWORD         a;
    int           rc;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);

    if ((a = inet_addr(addr)) == INADDR_NONE)
    {
        if (strcmp(addr, "255.255.255.255") != 0)
            return TE_RC(TE_TA_WIN32, TE_EINVAL);
    }

    GET_IF_ENTRY;

    if ((rc = ip_addr_exist(a, &data)) != 0)
        return rc;

    return 0;
}

/**
 * Get hardware address of the interface.
 * Only MAC addresses are supported now.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         location for hardware address (address is returned
 *                      as XX:XX:XX:XX:XX:XX)
 * @param ifname        name of the interface (like "eth0")
 *
 * @return status code
 */
static te_errno
link_addr_get(unsigned int gid, const char *oid, char *value,
              const char *ifname)
{
    uint8_t *ptr = if_entry.bPhysAddr;

    UNUSED(gid);
    UNUSED(oid);

    GET_IF_ENTRY;

    if (if_entry.dwPhysAddrLen != 6)
        snprintf(value, RCF_MAX_VAL, "00:00:00:00:00:00");
    else
        snprintf(value, RCF_MAX_VAL, "%02x:%02x:%02x:%02x:%02x:%02x",
                 ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);

    return 0;
}


/**
 * Get broadcast hardware address of the interface.
 * Because we can not manipulate broadcast MAC addresses on windows
 * the ff:ff:ff:ff:ff:ff is returned for test purposes.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         broadcast hardware address (it should be
 *                      provided as XX:XX:XX:XX:XX:XX string)
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
bcast_link_addr_get(unsigned int gid, const char *oid,
                    char *value, const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    GET_IF_ENTRY;

    if (value == NULL)
    {
        ERROR("A buffer for broadcast link layer address is not provided");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    snprintf(value, RCF_MAX_VAL, "ff:ff:ff:ff:ff:ff");
    return 0;
}

/* FIXME: Temporary workaround to force mtu_get return the value
 *        that was passed to mtu_set
 */
struct mtu_entry_t
{
  char if_name[20];
  unsigned int mtu;
};
static struct mtu_entry_t mtus[20];

/**
 * Get MTU of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return status code
 */
static te_errno
mtu_get(unsigned int gid, const char *oid, char *value,
        const char *ifname)
{
//    int i, if_index = -1, free_index = -1;

    UNUSED(gid);
    UNUSED(oid);

    GET_IF_ENTRY;

/*
    for (i = 0; i < 20; i++)
    {
      if (strcmp(mtus[i].if_name, ifname) == 0)
      {
        if_index = i;
        break;
      }
      if (mtus[i].if_name[0] == 0 && free_index == -1)
      {
        free_index = i;
      }
    }
    if (if_index == -1 && free_index != -1)
    {
      if_index = free_index;
      strcpy(mtus[if_index].if_name, ifname);
      mtus[if_index].mtu = if_entry.dwMtu;
    }
    sprintf(value, "%lu", mtus[if_index].mtu);
*/

    sprintf(value, "%lu", if_entry.dwMtu);
    return 0;
}

/**
 * Change MTU of the interface.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface (like "eth0")
 *
 * @return              Status code
 */
static te_errno
mtu_set(unsigned int gid, const char *oid, const char *value,
        const char *ifname)
{
//    char     *tmp;
    te_errno  rc = 0;
//    long      mtu;
//    int i, if_index = -1, free_index = -1;
    unsigned char szCommand[256];

#define ETHERNET_HEADER_LEN 14
#define VLAN_HEADER_LEN 4

    UNUSED(gid);
    UNUSED(oid);
    GET_IF_ENTRY;

    if (NULL == strstr(ifname, "ef"))
    {
        ERROR("Tried to set MTU on non-testable adapter.");
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    }

    if (get_driver_version() >= DRIVER_VERSION_2_2)
    {
      int     mtu;
      char*   frname;

      mtu = atoi(value);
      if (!wmi_imported)
      {
        ERROR("WMI functions were not imported.");
        return TE_RC(TE_TA_UNIX, TE_EFAULT);
      }

      frname = ifindex2frname(if_entry.dwIndex);
      if (frname == NULL)
      {
        ERROR("Failed to retrieve adapter friendly name.");
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
      }

      // We pass to the driver the maximum total size,
      // which is frame length + ethernet and vlan header.
      // VLAN tagging is enabled by default on V2.3 so we
      // have to add 18 to the MU input value.

      if (pwmi_mtu_set(frname, mtu + ETHERNET_HEADER_LEN + VLAN_HEADER_LEN))
        return TE_RC(TE_TA_UNIX, TE_EFAULT);

      return 0;
    }
    else
    {
      snprintf(szCommand, sizeof(szCommand),
          "./sish_client.exe "
          "--server=127.0.0.1 "
          "--command=\`cygpath -w \$PWD\`\\\\windows_layer2_manage.exe "
          "--args=\"set mtu %s\"", value);
      //    sprintf(szCommand, "./windows_layer2_mtu.exe %s", value);
      printf("szCommand = %s\n", szCommand);
      system(szCommand);
    }
/*    mtu = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);
    for (i = 0; i < 20; i++)
    {
      if (strcmp(mtus[i].if_name, ifname) == 0)
      {
        if_index = i;
        break;
      }
      if (mtus[i].if_name[0] == 0 && free_index == -1)
      {
        free_index = i;
      }
    }
    if (if_index == -1 && free_index != -1)
    {
      if_index = free_index;
      strcpy(mtus[if_index].if_name, ifname);
      mtus[if_index].mtu = 1500;
    }
    mtus[if_index].mtu = mtu;
*/

#undef ETHERNET_HEADER_LEN
#undef VLAN_HEADER_LEN

#if 0 /* UNIX implementation */
    if ((rc = CHECK_INTERFACE(ifname)) != 0)
        return TE_RC(TE_TA_UNIX, rc);

    mtu = strtol(value, &tmp, 10);
    if (tmp == value || *tmp != 0)
        return TE_RC(TE_TA_UNIX, TE_EINVAL);

#if (defined(SIOCGIFMTU)  && defined(HAVE_STRUCT_IFREQ_IFR_MTU))   || \
    (defined(SIOCGLIFMTU) && defined(HAVE_STRUCT_LIFREQ_LIFR_MTU))
    req.my_ifr_mtu = mtu;
    strcpy(req.my_ifr_name, ifname);
    if (ioctl(cfg_socket, MY_SIOCSIFMTU, (int)&req) != 0)
    {
        rc = TE_OS_RC(TE_TA_UNIX, errno);

        if (errno == EBUSY)
        {
            char status[2];

            /* Try to down interface */
            if (status_get(0, NULL, status, ifname) == 0 &&
                *status == '1' && status_set(0, NULL, "0", ifname) == 0)
            {
                int rc1;

                WARN("Interface '%s' is pushed down/up to set a new MTU",
                     ifname);

                if (ioctl(cfg_socket, MY_SIOCSIFMTU, (int)&req) == 0)
                {
                    rc = 0;
                }

                if ((rc1 = status_set(0, NULL, "1", ifname)) != 0)
                {
                    ERROR("Failed to up interface after changing of mtu "
                          "error %r", rc1);
                    return rc1;
                }
            }
        }
    }
#else
    rc = TE_RC(TE_TA_UNIX, TE_ENOSYS);
#endif

    if (rc != 0)
        ERROR("ioctl(SIOCSIFMTU) failed: %r", rc);
#endif /* UNIX implementation*/

    return rc;
}

/**
 * Get status of the interface ("0" - down or "1" - up).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return status code
 */
static te_errno
status_get(unsigned int gid, const char *oid, char *value,
           const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    GET_IF_ENTRY;

    sprintf(value, "%d",
            if_entry.dwOperStatus == MIB_IF_OPER_STATUS_CONNECTED ||
            if_entry.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL ?
                1 : 0);

    return 0;
}

/**
 * Change status of the interface. If virtual interface is put to down
 * state, it is de-installed and information about it is stored in the
 * list of down interfaces.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface (like "eth0")
 *
 * @return status code
 */
static te_errno
status_set(unsigned int gid, const char *oid, const char *value,
           const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    GET_IF_ENTRY;

    if (strcmp(value, "0") == 0)
        if_entry.dwAdminStatus = MIB_IF_ADMIN_STATUS_DOWN;
    else if (strcmp(value, "1") == 0)
        if_entry.dwAdminStatus = MIB_IF_ADMIN_STATUS_UP;
    else
        return TE_RC(TE_TA_WIN32, TE_EINVAL);

    if (SetIfEntry(&if_entry) != 0)
        return TE_RC(TE_TA_WIN32, TE_ENOENT);

    return 0;
}


/**
 * Get promiscuous mode of the interface ("0" - disabled or "1" - enabled).
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value location
 * @param ifname        name of the interface (like "eth0")
 *
 * @return status code
 */
static te_errno
promisc_get(unsigned int gid, const char *oid, char *value,
            const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    /* TODO */
    *value = '0';
    value[1] = '\0';
    UNUSED(ifname);

    return 0;
}

/**
 * Change status of the interface. If virtual interface is put to down
 * state, it is de-installed and information about it is stored in the
 * list of down interfaces.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface (like "eth0")
 *
 * @return status code
 */
static te_errno
promisc_set(unsigned int gid, const char *oid, const char *value,
            const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);

    /* TODO */
    if (strcmp(value, "0") == 0)
        return 0;
    UNUSED(ifname);

    return TE_RC(TE_TA_WIN32, TE_EOPNOTSUPP);
}


/** Find neighbour entry and return its parameters */
static te_errno
neigh_find(const char *oid, const char *ifname, const char *addr,
           char *mac)
{
    MIB_IPNETTABLE *table;
    DWORD           a;
    int             i;
    DWORD           type = strstr(oid, "dynamic") == NULL ?
                           ARP_STATIC : ARP_DYNAMIC;
    DWORD           ifindex = ifname2ifindex(ifname);

    if (ifindex == 0)
        return TE_RC(TE_TA_WIN32, TE_ENOENT);

    if ((a = inet_addr(addr)) == INADDR_NONE)
        return TE_RC(TE_TA_WIN32, TE_EINVAL);

    GET_TABLE(MIB_IPNETTABLE, GetIpNetTable);
    if (table == NULL)
        return TE_RC(TE_TA_WIN32, TE_ENOENT);

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if (a == table->table[i].dwAddr && table->table[i].dwType == type)
        {
            uint8_t *ptr = table->table[i].bPhysAddr;

            if (table->table[i].dwIndex != ifindex)
                continue;

            if (table->table[i].dwPhysAddrLen != 6)
            {
                free(table);
                return TE_RC(TE_TA_WIN32, TE_ENOENT);
            }
            if (mac != NULL)
                snprintf(mac, RCF_MAX_VAL, "%02x:%02x:%02x:%02x:%02x:%02x",
                         ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
            free(table);
            return 0;
        }
    }

    free(table);

    return TE_RC(TE_TA_WIN32, TE_ENOENT);
}

/**
 * Get neighbour entry state.
 *
 * @param gid            group identifier (unused)
 * @param oid            full object instence identifier (unused)
 * @param value          location for the value
 *                       (XX:XX:XX:XX:XX:XX is returned)
 * @param ifname         interface name
 * @param addr           IP address in human notation
 *
 * @return Status code
 */
te_errno
neigh_state_get(unsigned int gid, const char *oid, char *value,
                const char *ifname, const char *addr)
{
    int  rc;

    UNUSED(gid);

    if (strstr(oid, "dynamic") == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    if ((rc = neigh_find(oid, ifname, addr, NULL)) != 0)
        return rc;

    sprintf(value, "%u", CS_NEIGH_REACHABLE);

    return 0;
}

/**
 * Get neighbour entry value (hardware address corresponding to IP).
 *
 * @param gid            group identifier (unused)
 * @param oid            full object instence identifier (unused)
 * @param value          location for the value
 *                       (XX:XX:XX:XX:XX:XX is returned)
 * @param ifname         interface name
 * @param addr           IP address in human notation
 *
 * @return Status code
 */
static te_errno
neigh_get(unsigned int gid, const char *oid, char *value,
          const char *ifname, const char *addr)
{
    UNUSED(gid);

    return neigh_find(oid, ifname, addr, value);
}


/**
 * Change already existing neighbour entry.
 *
 * @param gid            group identifier
 * @param oid            full object instence identifier (unused)
 * @param value          new value pointer ("XX:XX:XX:XX:XX:XX")
 * @param ifname         interface name
 * @param addr           IP address in human notation
 *
 * @return Status code
 */
static te_errno
neigh_set(unsigned int gid, const char *oid, const char *value,
          const char *ifname, const char *addr)
{
    UNUSED(gid);

    if (neigh_find(oid, ifname, addr, NULL) != 0)
        return TE_RC(TE_TA_UNIX, TE_ENOENT);

    return neigh_add(gid, oid, value, ifname, addr);
}

/**
 * Add a new neighbour entry.
 *
 * @param gid            group identifier (unused)
 * @param oid            full object instence identifier (unused)
 * @param value          new entry value pointer ("XX:XX:XX:XX:XX:XX")
 * @param ifname         interface name
 * @param addr           IP address in human notation
 *
 * @return Status code
 */
static te_errno
neigh_add(unsigned int gid, const char *oid, const char *value,
          const char *ifname, const char *addr)
{
    MIB_IPNETROW   entry;
    int            int_mac[6];
    int            rc;
    int            i;

    UNUSED(gid);
    UNUSED(oid);

    /* Don't check the existence of the entry */
//    if (neigh_find(oid, ifname, addr, NULL) == 0)
//        return TE_RC(TE_TA_WIN32, TE_EEXIST);

    memset(&entry, 0, sizeof(entry));

    entry.dwType = strstr(oid, "dynamic") != NULL ? ARP_DYNAMIC
                                                  : ARP_STATIC;

    if ((entry.dwIndex =  ifname2ifindex(ifname)) == 0)
        return TE_RC(TE_TA_WIN32, TE_ENOENT);

    if (sscanf(value, "%2x:%2x:%2x:%2x:%2x:%2x%s", int_mac, int_mac + 1,
            int_mac + 2, int_mac + 3, int_mac + 4, int_mac + 5, buf) != 6)
    {
        return TE_RC(TE_TA_WIN32, TE_EINVAL);
    }

    for (i = 0; i < 6; i++)
        entry.bPhysAddr[i] = (unsigned char)int_mac[i];

    entry.dwAddr = inet_addr(addr);
    entry.dwPhysAddrLen = 6;
    if ((rc = CreateIpNetEntry(&entry)) != NO_ERROR)
    {
        ERROR("CreateIpNetEntry() failed, error %d", rc);
        return TE_RC(TE_TA_WIN32, TE_EWIN);
    }
    if (entry.dwType == ARP_STATIC) add_static_neigh(&entry);

    return 0;
}

/**
 * Delete neighbour entry.
 *
 * @param gid            group identifier (unused)
 * @param oid            full object instence identifier (unused)
 * @param ifname         interface name
 * @param addr           IP address in human notation
 *
 * @return Status code
 */
static te_errno
neigh_del(unsigned int gid, const char *oid, const char *ifname,
          const char *addr)
{
    MIB_IPNETTABLE *table;
    int             i;
    DWORD           a;
    int             rc;
    DWORD           type = strstr(oid, "dynamic") == NULL ? ARP_STATIC
                                                          : ARP_DYNAMIC;
    te_bool         found = FALSE;
    DWORD           ifindex = ifname2ifindex(ifname);

    UNUSED(gid);
    UNUSED(oid);

    if (ifindex == 0)
        return TE_RC(TE_TA_WIN32, TE_ENOENT);

    if ((a = inet_addr(addr)) == INADDR_NONE)
        return TE_RC(TE_TA_WIN32, TE_EINVAL);

    GET_TABLE(MIB_IPNETTABLE, GetIpNetTable);
    if (table == NULL)
        return TE_RC(TE_TA_WIN32, TE_ENOENT);

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if (table->table[i].dwAddr == a && table->table[i].dwType == type &&
            table->table[i].dwIndex == ifindex)
        {
            if (((rc = DeleteIpNetEntry(table->table + i)) != 0)||
               (delete_neigh_st_entry(table->table + i)))
            {
                if (type == ARP_STATIC)
                {
                    ERROR("DeleteIpNetEntry() failed, error %d", rc);
                    free(table);
                    return TE_RC(TE_TA_WIN32, TE_EWIN);
                }
            }
            found = TRUE;
            /* Continue to delete entries on other interfaces */
        }
    }
    free(table);
    return (!found && type == ARP_STATIC) ?
           TE_RC(TE_TA_WIN32, TE_ENOENT) : 0;
}

/**
 * Get instance list for object "agent/arp" and "agent/volatile/arp".
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          location for the list pointer
 * @param ifname        interface name
 *
 * @return Status code
 *
 * Note: For "static" requests only the entries added by neigh_add
 *       will be returned. It is done in order to avoid tests failures
 *       due to unwanted arbitrary changes in ARP cache on Vista.
 */
static te_errno
neigh_list(unsigned int gid, const char *oid,
           const char *sub_id, char **list,
           const char *ifname)
{
    MIB_IPNETTABLE *table;
    int             i;
    char           *s = buf;
    DWORD           type = strstr(oid, "dynamic") == NULL ? ARP_STATIC
                                                          : ARP_DYNAMIC;
    DWORD           ifindex = ifname2ifindex(ifname);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    if (ifindex == 0)
        return TE_RC(TE_TA_WIN32, TE_ENOENT);
    if (type == ARP_STATIC)
        return neigh_st_list(list, ifname);

    GET_TABLE(MIB_IPNETTABLE, GetIpNetTable);
    if (table == NULL)
    {
        if ((*list = strdup(" ")) == NULL)
            return TE_RC(TE_TA_WIN32, TE_ENOMEM);
        else
            return 0;
    }

    buf[0] = 0;

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        if (table->table[i].dwPhysAddrLen != 6 ||
            table->table[i].dwType != type ||
            table->table[i].dwIndex != ifindex ||
            table->table[i].dwAddr == 0xFFFFFFFF /* FIXME */)
        {
            continue;
        }
        s += snprintf(s, sizeof(buf) - (s - buf), "%s ",
                      inet_ntoa(*(struct in_addr *)
                                    &(table->table[i].dwAddr)));
    }

    free(table);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_WIN32, TE_ENOMEM);

    return 0;
}

/******************************************************************/
/* Implementation of /agent/route subtree                         */
/******************************************************************/

/**
 * Convert system-independent route info data structure to
 * Win32-specific MIB_IPFORWARDROW data structure.
 *
 * @param rt_info TA portable route info
 * @param rt      Win32-specific data structure (OUT)
 *
 * @return Status code.
 */
static te_errno
rt_info2ipforw(const ta_rt_info_t *rt_info, MIB_IPFORWARDROW *rt)
{
    int rc;

    if ((rt_info->flags & TA_RT_INFO_FLG_GW) == 0 &&
        (rt_info->flags & TA_RT_INFO_FLG_IF) == 0)
    {
        ERROR("Incorrect flags %x for rt_info %x", rt_info->flags);
        return TE_RC(TE_TA_WIN32, TE_EINVAL);
    }

    rt->dwForwardDest = SIN(&(rt_info->dst))->sin_addr.s_addr;
    rt->dwForwardNextHop = SIN(&(rt_info->gw))->sin_addr.s_addr;
    rt->dwForwardMask = PREFIX2MASK(rt_info->prefix);
    rt->dwForwardType =
        ((rt_info->flags & TA_RT_INFO_FLG_GW) != 0) ?
        FORW_TYPE_REMOTE : FORW_TYPE_LOCAL;

    rt->dwForwardMetric1 =
        ((rt_info->flags & TA_RT_INFO_FLG_METRIC) != 0) ?
        rt_info->metric : METRIC_DEFAULT;

    if (rt->dwForwardNextHop == 0)
        rt->dwForwardNextHop = rt->dwForwardDest;

    rt->dwForwardProto = 3;

    if ((rt_info->flags & TA_RT_INFO_FLG_IF) != 0)
    {
        rc = name2index(rt_info->ifname, &(rt->dwForwardIfIndex));
        rt->dwForwardNextHop = rt->dwForwardDest;
    }
    else
    {
        /* Use Next Hop address to define outgoing interface */
        rc = find_ifindex(rt->dwForwardNextHop, &rt->dwForwardIfIndex);
    }

    return rc;
}

/**
 * Get route attributes.
 *
 * @param route    route instance name: see doc/cm_cm_base.xml
 *                 for the format
 * @param rt_info  route related information (OUT)
 * @param rt       route related information in win32 format (OUT)
 *
 * @return status code
 */
static int
route_find(const char *route, ta_rt_info_t *rt_info, MIB_IPFORWARDROW *rt)
{
    MIB_IPFORWARDTABLE *table;
    DWORD               route_addr;
    int                 rc;
    int                 i;

    if ((rc = ta_rt_parse_inst_name(route, rt_info)) != 0)
        return TE_RC(TE_TA_WIN32, rc);

    GET_TABLE(MIB_IPFORWARDTABLE, GetIpForwardTable);
    if (table == NULL)
        return TE_RC(TE_TA_WIN32, TE_ENOENT);
    route_addr = SIN(&(rt_info->dst))->sin_addr.s_addr;

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        uint32_t p;

        if (table->table[i].dwForwardType != FORW_TYPE_LOCAL &&
            table->table[i].dwForwardType != FORW_TYPE_REMOTE)
        {
            continue;
        }

        MASK2PREFIX(table->table[i].dwForwardMask, p);
        if (table->table[i].dwForwardDest != route_addr ||
            (p != rt_info->prefix) ||
            ((rt_info->flags & TA_RT_INFO_FLG_METRIC) != 0 &&
             (table->table[i].dwForwardMetric1 != rt_info->metric)))
        {
            continue;
        }

        if (table->table[i].dwForwardIfIndex != 0)
        {
            rt_info->flags |= TA_RT_INFO_FLG_IF;
            sprintf(rt_info->ifname,
                    ifindex2ifname(table->table[i].dwForwardIfIndex));
        }
        if (table->table[i].dwForwardNextHop != 0)
        {
            rt_info->flags |= TA_RT_INFO_FLG_GW;
            SIN(&rt_info->gw)->sin_family = AF_INET;
            SIN(&rt_info->gw)->sin_addr.s_addr =
                table->table[i].dwForwardNextHop;
        }

        if (rt != NULL)
            *rt = table->table[i];

        /*
         * win32 agent supports only a limited set of route attributes.
         */
        free(table);
        return 0;
    }

    free(table);

    return TE_RC(TE_TA_WIN32, TE_ENOENT);
}

/**
 * Load all route-specific attributes into route object.
 *
 * @param obj  Object to be uploaded
 *
 * @return 0 in case of success, or error code on failure.
 */
static int
route_load_attrs(ta_cfg_obj_t *obj)
{
    ta_rt_info_t rt_info;
    int          rc;
    char         val[128];

    if ((rc = route_find(obj->name, &rt_info, NULL)) != 0)
        return rc;

    snprintf(val, sizeof(val), "%s", rt_info.ifname);
    if ((rt_info.flags & TA_RT_INFO_FLG_IF) != 0 &&
        (rc = ta_obj_attr_set(obj , "dev", val)) != 0)
    {
        return rc;
    }

    return 0;
}

static te_errno
route_dev_get(unsigned int gid, const char *oid,
              char *value, const char *route)
{
    int          rc;
    ta_rt_info_t rt_info;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = route_find(route, &rt_info, NULL)) != 0)
        return rc;

    sprintf(value, "%s", rt_info.ifname);
    return 0;
}

static te_errno
route_dev_set(unsigned int gid, const char *oid,
              const char *value, const char *route)
{
    UNUSED(oid);

    return ta_obj_set(TA_OBJ_TYPE_ROUTE, route, "dev",
                      value, gid, route_load_attrs);
}

/**
 * Add a new route.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value string (unused)
 * @param route         route instance name: see doc/cm_cm_base.xml
 *                      for the format
 *
 * @return status code
 */
static te_errno
route_add(unsigned int gid, const char *oid, const char *value,
          const char *route)
{
    UNUSED(oid);

    return ta_obj_add(TA_OBJ_TYPE_ROUTE, route, value, gid,
                      NULL, NULL, NULL);
}

/**
 * Delete a route.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param route         route instance name: see doc/cm_cm_base.xml
 *                      for the format
 *
 * @return status code
 */
static te_errno
route_del(unsigned int gid, const char *oid, const char *route)
{
    UNUSED(oid);

    return ta_obj_del(TA_OBJ_TYPE_ROUTE, route, NULL, NULL, gid, NULL);
}

/** Get the value of the route.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value string (unused)
 * @param route         route instance name: see doc/cm_cm_base.xml
 *                      for the format
 *
 * @return status code
 */
static te_errno
route_get(unsigned int gid, const char *oid, char *value,
          const char *route_name)
{
    ta_rt_info_t  attr;
    int           rc;
    char         *addr;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = route_find(route_name, &attr, NULL)) != 0)
    {
        ERROR("Route %s cannot be found", route_name);
        return rc;
    }

    if (attr.dst.ss_family == AF_INET)
    {
        addr = inet_ntoa(SIN(&attr.gw)->sin_addr);
        memcpy(value, addr, strlen(addr));
    }
    else
    {
        ERROR("Unexpected destination address family: %d",
               attr.dst.ss_family);
        return TE_RC(TE_TA_WIN32, TE_EINVAL);
    }

    return 0;
}

/** Set new value for the route.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param value         value string (unused)
 * @param route         route instance name: see doc/cm_cm_base.xml
 *                      for the format
 *
 * @return status code
 */
static te_errno
route_set(unsigned int gid, const char *oid, const char *value,
          const char *route_name)
{
    UNUSED(oid);

    return ta_obj_value_set(TA_OBJ_TYPE_ROUTE, route_name, value, gid);
}

/**
 * Get instance list for object "agent/route".
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          location for the list pointer
 *
 * @return status code
 * @retval 0                    success
 * @retval TE_ENOENT            no such instance
 * @retval TE_ENOMEM               cannot allocate memory
 */
static te_errno
route_list(unsigned int gid, const char *oid,
           const char *sub_id, char **list)
{
    MIB_IPFORWARDTABLE *table;
    int                 i;
    char               *end_ptr = buf + sizeof(buf);
    char               *ptr = buf;

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    GET_TABLE(MIB_IPFORWARDTABLE, GetIpForwardTable);
    if (table == NULL)
    {
        if ((*list = strdup(" ")) == NULL)
            return TE_RC(TE_TA_WIN32, TE_ENOMEM);
        else
            return 0;
    }

    buf[0] = '\0';

    for (i = 0; i < (int)table->dwNumEntries; i++)
    {
        int prefix;

        if ((/*table->table[i].dwForwardType != FORW_TYPE_LOCAL &&*/
             table->table[i].dwForwardType != FORW_TYPE_REMOTE) ||
             table->table[i].dwForwardDest == 0xFFFFFFFF ||
             (table->table[i].dwForwardMask == 0xFFFFFFFF &&
              table->table[i].dwForwardDest != htonl(INADDR_LOOPBACK) &&
              table->table[i].dwForwardNextHop == htonl(INADDR_LOOPBACK)))
        {
            continue;
        }

        MASK2PREFIX(table->table[i].dwForwardMask, prefix);
        snprintf(ptr, end_ptr - ptr, "%s|%d",
                 inet_ntoa(*(struct in_addr *)
                     &(table->table[i].dwForwardDest)), prefix);
        ptr += strlen(ptr);
        if (table->table[i].dwForwardMetric1 != METRIC_DEFAULT)
        {
            snprintf(ptr, end_ptr - ptr, ",metric=%ld",
                     table->table[i].dwForwardMetric1);
        }

        ptr += strlen(ptr);
        snprintf(ptr, end_ptr - ptr, " ");
        ptr += strlen(ptr);
    }

    free(table);

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_WIN32, TE_ENOMEM);

    return 0;
}

/**
 * Commit changes made for the route.
 *
 * @param gid           group identifier (unused)
 * @param p_oid         object instence data structure
 *
 * @return status code
 */
static te_errno
route_commit(unsigned int gid, const cfg_oid *p_oid)
{
    const char       *route;
    ta_cfg_obj_t     *obj;
    ta_rt_info_t      rt_info;
    int               rc;
    MIB_IPFORWARDROW  rt;

    ta_cfg_obj_action_e obj_action;

    UNUSED(gid);
    ENTRY("%s", route);

    memset(&rt, 0, sizeof(rt));

    route = ((cfg_inst_subid *)(p_oid->ids))[p_oid->len - 1].name;

    if ((obj = ta_obj_find(TA_OBJ_TYPE_ROUTE, route, gid)) == NULL)
    {
        WARN("Commit for %s route which has not been updated", route);
        return 0;
    }

    if ((rc = ta_rt_parse_obj(obj, &rt_info)) != 0)
    {
        ta_obj_free(obj);
        return rc;
    }

    if ((obj_action = obj->action) == TA_CFG_OBJ_DELETE ||
        obj_action == TA_CFG_OBJ_SET)
    {
        rc = route_find(obj->name, &rt_info, &rt);
    }
    ta_obj_free(obj);
    if (rc != 0)
        return rc;

    switch (obj_action)
    {
        case TA_CFG_OBJ_DELETE:
        case TA_CFG_OBJ_SET:
        {
            if ((rc = DeleteIpForwardEntry(&rt)) != 0)
            {
                ERROR("DeleteIpForwardEntry() failed, error %d", rc);
                return TE_RC(TE_TA_WIN32, TE_EWIN);
            }
            if (obj_action == TA_CFG_OBJ_DELETE)
                break;
            /* FALLTHROUGH */
        }

        case TA_CFG_OBJ_CREATE:
        {
            if ((rc = rt_info2ipforw(&rt_info, &rt)) != 0)
            {
                ERROR("Failed to convert route to "
                      "MIB_IPFORWARDROW data structure");
                return TE_RC(TE_TA_WIN32, TE_EWIN);
            }

            /* Add or set operation */
            if ((rc = CreateIpForwardEntry(&rt)) != NO_ERROR)
            {
                ERROR("CreateIpForwardEntry() failed, error %d", rc);
                return TE_RC(TE_TA_WIN32, TE_EWIN);
            }
            break;
        }

        default:
            ERROR("Unknown object action specified %d", obj_action);
            return TE_RC(TE_TA_WIN32, TE_EINVAL);
    }

    return 0;
}

/**
 * Is Environment variable with such name hidden?
 *
 * @param name      Variable name
 * @param name_len  -1, if @a name is a NUL-terminated string;
 *                  >= 0, if length of the @a name is @a name_len
 */
static te_bool
env_is_hidden(const char *name, int name_len)
{
    unsigned int    i;

    for (i = 0; i < sizeof(env_hidden) / sizeof(env_hidden[0]); ++i)
    {
        if (memcmp(env_hidden[i], name,
                   (name_len < 0) ? strlen(name) : (size_t)name_len) == 0)
            return TRUE;
    }
    return FALSE;
}

/**
 * Get Environment variable value.
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param value     Location for the value (OUT)
 * @param name      Variable name
 *
 * @return status code
 */
static te_errno
env_get(unsigned int gid, const char *oid, char *value,
        const char *name)
{
    const char *tmp = getenv(name);

    UNUSED(gid);
    UNUSED(oid);

    if (tmp == NULL)
    {
        static char buf[RCF_MAX_VAL];

        *buf = 0;
        GetEnvironmentVariable(name, buf, sizeof(buf));
        if (*buf != 0)
            tmp = buf;
    }

    if (!env_is_hidden(name, -1) && (tmp != NULL))
    {
        if (strlen(tmp) >= RCF_MAX_VAL)
            WARN("Environment variable '%s' value truncated", name);
        snprintf(value, RCF_MAX_VAL, "%s", tmp);
        return 0;
    }
    else
    {
        return TE_RC(TE_TA_UNIX, TE_ENOENT);
    }
}

/**
 * Change already existing Environment variable.
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param value     New value to set
 * @param name      Variable name
 *
 * @return status code
 */
static te_errno
env_set(unsigned int gid, const char *oid, const char *value,
        const char *name)
{
    UNUSED(gid);
    UNUSED(oid);

    if (env_is_hidden(name, -1))
        return TE_RC(TE_TA_UNIX, TE_EPERM);

    setenv(name, value, 1);
    SetEnvironmentVariable(name, value);

    return 0;
}

/**
 * Add a new Environment variable.
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param value     Value
 * @param name      Variable name
 *
 * @return status code
 */
static te_errno
env_add(unsigned int gid, const char *oid, const char *value,
        const char *name)
{
    UNUSED(gid);
    UNUSED(oid);

    if (env_is_hidden(name, -1))
        return TE_RC(TE_TA_WIN32, TE_EPERM);

    setenv(name, value, 1);
    SetEnvironmentVariable(name, value);

    return 0;
}

/**
 * Delete Environment variable.
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param name      Variable name
 *
 * @return status code
 */
static te_errno
env_del(unsigned int gid, const char *oid, const char *name)
{
    te_errno rc;

    UNUSED(gid);
    UNUSED(oid);

    if ((rc = env_get(0, NULL, buf, name)) != 0)
        return rc;

    SetEnvironmentVariable(name, NULL);
    unsetenv(name);

    return 0;
}

/**
 * Get instance list for object "/agent/env".
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param sub_id    ID of the object to be listed (unused)
 * @param list      Location for the list pointer
 *
 * @return status code
 */
static te_errno
env_list(unsigned int gid, const char *oid,
         const char *sub_id, char **list)
{
    char **env;

    char   *ptr = buf;
    char   *buf_end = buf + sizeof(buf);

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    if (environ == NULL)
        return 0;

    *ptr = '\0';
    for (env = environ; *env != NULL; ++env)
    {
        char    *s = strchr(*env, '=');
        ssize_t  name_len;

        if (s == NULL)
        {
            ERROR("Invalid Environment entry format: %s", *env);
            return TE_RC(TE_TA_UNIX, TE_EFMT);
        }
        name_len = s - *env;
        if (env_is_hidden(*env, name_len))
            continue;

        if (ptr != buf)
            *ptr++ = ' ';
        if ((buf_end - ptr) <= name_len)
        {
            ERROR("Too small buffer for the list of Environment "
                  "variables");
            return TE_RC(TE_TA_UNIX, TE_ESMALLBUF);
        }
        memcpy(ptr, *env, name_len);
        ptr += name_len;
        *ptr = '\0';
    }

    if ((*list = strdup(buf)) == NULL)
        return TE_RC(TE_TA_UNIX, TE_ENOMEM);

    return 0;
}

/**
 * Get agent uname value.
 *
 * @param gid       Request's group identifier (unused)
 * @param oid       Full object instance identifier (unused)
 * @param value     Location for the value (OUT)
 * @param name      Variable name
 *
 * @return Status code
 */
static te_errno
uname_get(unsigned int gid, const char *oid, char *value)
{
#ifdef HAVE_SYS_UTSNAME_H
    struct utsname val;

    UNUSED(gid);
    UNUSED(oid);

    if (uname(&val) == 0)
    {
        if (strlen(val.sysname) >= RCF_MAX_VAL)
            ERROR("System uname '%s' truncated", val.sysname);
        snprintf(value, RCF_MAX_VAL, "%s", val.sysname);
        return 0;
    }
    else
    {
        ERROR("Failed to call uname()");
        return TE_OS_RC(TE_TA_UNIX, errno);
    }
#else
#warning "uname access method is not implemented"
    return "Microsoft Windows";
#endif
}

/*
 * win32 Test Agent network statistics configuration tree.
 */

#define STATS_IFTABLE_COUNTER_GET(_counter_, _field_) \
static te_errno net_if_stats_##_counter_##_get(unsigned int gid_,       \
                                               const char  *oid_,       \
                                               char        *value_,     \
                                               const char  *ifname)     \
{                                                                       \
    int        rc = 0;                                                  \
    if_stats   stats;                                                   \
                                                                        \
    UNUSED(gid_);                                                       \
    UNUSED(oid_);                                                       \
    /* Get statistics via GetIfEntry function */                        \
    GET_IF_ENTRY;                                                       \
    stats.in_octets = (uint64_t)if_entry.dwInOctets;                    \
    stats.in_ucast_pkts = (uint64_t)if_entry.dwInUcastPkts;             \
    stats.in_nucast_pkts = (uint64_t)if_entry.dwInNUcastPkts;           \
    stats.in_discards = (uint64_t)if_entry.dwInDiscards;                \
    stats.in_errors = (uint64_t)if_entry.dwInErrors;                    \
    stats.in_unknown_protos = (uint64_t)if_entry.dwInUnknownProtos;     \
    stats.out_octets = (uint64_t)if_entry.dwOutOctets;                  \
    stats.out_ucast_pkts = (uint64_t)if_entry.dwOutUcastPkts;           \
    stats.out_nucast_pkts = (uint64_t)if_entry.dwOutNUcastPkts;         \
    stats.out_discards = (uint64_t)if_entry.dwOutDiscards;              \
    stats.out_errors = (uint64_t)if_entry.dwOutErrors;                  \
                                                                        \
    /* Try to get statistics from wrapper. In case of success           \
     * several values from stats would be overwritten */                \
    if_stats_get((ifname), &stats, NULL);                               \
                                                                        \
    snprintf((value_), RCF_MAX_VAL, "%llu", stats. _field_);            \
                                                                        \
    VERB("dev_counter_get(dev_name=%s, counter=%s) returns %s",         \
         ifname, #_counter_, value_);                                   \
                                                                        \
    return rc;                                                          \
}


STATS_IFTABLE_COUNTER_GET(in_octets, in_octets);
STATS_IFTABLE_COUNTER_GET(in_ucast_pkts, in_ucast_pkts);
STATS_IFTABLE_COUNTER_GET(in_nucast_pkts, in_nucast_pkts);
STATS_IFTABLE_COUNTER_GET(in_discards, in_discards);
STATS_IFTABLE_COUNTER_GET(in_errors, in_errors);
STATS_IFTABLE_COUNTER_GET(in_unknown_protos, in_unknown_protos);
STATS_IFTABLE_COUNTER_GET(out_octets, out_octets);
STATS_IFTABLE_COUNTER_GET(out_ucast_pkts, out_ucast_pkts);
STATS_IFTABLE_COUNTER_GET(out_nucast_pkts, out_nucast_pkts);
STATS_IFTABLE_COUNTER_GET(out_discards, out_discards);
STATS_IFTABLE_COUNTER_GET(out_errors, out_errors);

#undef STATS_IFTABLE_COUNTER_GET

#define STATS_NET_SNMP_IPV4_COUNTER_GET(_counter_, _field_)     \
static te_errno                                                 \
net_snmp_ipv4_stats_##_counter_##_get(unsigned int gid_,        \
                                      const char  *oid_,        \
                                      char        *value_)      \
{                                                               \
    int         rc = 0;                                         \
    MIB_IPSTATS *table = NULL;                                  \
                                                                \
    UNUSED(gid_);                                               \
    UNUSED(oid_);                                               \
    if ((table = malloc(sizeof(MIB_IPSTATS))) == NULL   )       \
        return TE_RC(TE_TA_WIN32, TE_ENOMEM);                   \
                                                                \
    if ((rc = GetIpStatistics(table)) != NO_ERROR)              \
    {                                                           \
        ERROR("GetIpStatistics failed, error %d", rc);          \
        free(table);                                            \
        return TE_RC(TE_TA_WIN32, TE_EWIN);                     \
    }                                                           \
                                                                \
    snprintf((value_), RCF_MAX_VAL, "%lu",                      \
             table->_field_);                                   \
                                                                \
    VERB("net_snmp_ipv4_counter_get(counter=%s) returns %s",    \
         #_counter_, value_);                                   \
                                                                \
    free(table);                                                \
    return 0;                                                   \
}

STATS_NET_SNMP_IPV4_COUNTER_GET(in_recvs, dwInReceives);
STATS_NET_SNMP_IPV4_COUNTER_GET(in_hdr_errs, dwInHdrErrors);
STATS_NET_SNMP_IPV4_COUNTER_GET(in_addr_errs, dwInAddrErrors);
STATS_NET_SNMP_IPV4_COUNTER_GET(forw_dgrams, dwForwDatagrams);
STATS_NET_SNMP_IPV4_COUNTER_GET(in_unknown_protos, dwInUnknownProtos);
STATS_NET_SNMP_IPV4_COUNTER_GET(in_discards, dwInDiscards);
STATS_NET_SNMP_IPV4_COUNTER_GET(in_delivers, dwInDelivers);
STATS_NET_SNMP_IPV4_COUNTER_GET(out_requests, dwOutRequests);
STATS_NET_SNMP_IPV4_COUNTER_GET(out_discards, dwOutDiscards);
STATS_NET_SNMP_IPV4_COUNTER_GET(out_no_routes, dwOutNoRoutes);
STATS_NET_SNMP_IPV4_COUNTER_GET(reasm_timeout, dwReasmTimeout);
STATS_NET_SNMP_IPV4_COUNTER_GET(reasm_reqds, dwReasmReqds);
STATS_NET_SNMP_IPV4_COUNTER_GET(reasm_oks, dwReasmOks);
STATS_NET_SNMP_IPV4_COUNTER_GET(reasm_fails, dwReasmFails);
STATS_NET_SNMP_IPV4_COUNTER_GET(frag_oks, dwFragOks);
STATS_NET_SNMP_IPV4_COUNTER_GET(frag_fails, dwFragFails);
STATS_NET_SNMP_IPV4_COUNTER_GET(frag_creates, dwFragCreates);

#undef STATS_NET_SNMP_IPV4_COUNTER_GET

#define STATS_NET_SNMP_ICMP_COUNTER_GET(_counter_, _field_) \
static te_errno                                                 \
net_snmp_icmp_stats_ ## _counter_ ## _get(unsigned int gid_,    \
                                          const char  *oid_,    \
                                          char        *value_)  \
{                                                               \
    int         rc = 0;                                         \
    MIB_ICMP *table = NULL;                                     \
                                                                \
    UNUSED(gid_);                                               \
    UNUSED(oid_);                                               \
    if ((table = malloc(sizeof(MIB_ICMP))) == NULL   )          \
        return TE_RC(TE_TA_WIN32, TE_ENOMEM);                   \
                                                                \
    if ((rc = GetIcmpStatistics(table)) != NO_ERROR)            \
    {                                                           \
        ERROR("GetIcmpStatistics failed, error %d", rc);        \
        free(table);                                            \
        return TE_RC(TE_TA_WIN32, TE_EWIN);                     \
    }                                                           \
                                                                \
    snprintf((value_), RCF_MAX_VAL, "%lu",                      \
             table->stats._field_);                             \
                                                                \
    VERB("net_snmp_icmp_counter_get(counter=%s) returns %s",    \
         #_counter_, value_);                                   \
                                                                \
    free(table);                                                \
    return 0;                                                   \
}

STATS_NET_SNMP_ICMP_COUNTER_GET(in_msgs, icmpInStats.dwMsgs);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_errs, icmpInStats.dwErrors);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_dest_unreachs,
                                icmpInStats.dwDestUnreachs);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_time_excds, icmpInStats.dwTimeExcds);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_parm_probs, icmpInStats.dwParmProbs);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_src_quenchs, icmpInStats.dwSrcQuenchs);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_redirects, icmpInStats.dwRedirects);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_echos, icmpInStats.dwEchos);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_echo_reps, icmpInStats.dwEchoReps);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_timestamps, icmpInStats.dwTimestamps);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_timestamp_reps,
                                icmpInStats.dwTimestampReps);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_addr_masks, icmpInStats.dwAddrMasks);
STATS_NET_SNMP_ICMP_COUNTER_GET(in_addr_mask_reps,
                                icmpInStats.dwAddrMaskReps);

STATS_NET_SNMP_ICMP_COUNTER_GET(out_msgs, icmpOutStats.dwMsgs);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_errs, icmpOutStats.dwErrors);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_dest_unreachs,
                                icmpOutStats.dwDestUnreachs);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_time_excds, icmpOutStats.dwTimeExcds);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_parm_probs, icmpOutStats.dwParmProbs);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_src_quenchs, icmpOutStats.dwSrcQuenchs);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_redirects, icmpOutStats.dwRedirects);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_echos, icmpOutStats.dwEchos);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_echo_reps, icmpOutStats.dwEchoReps);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_timestamps, icmpOutStats.dwTimestamps);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_timestamp_reps,
                                icmpOutStats.dwTimestampReps);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_addr_masks, icmpOutStats.dwAddrMasks);
STATS_NET_SNMP_ICMP_COUNTER_GET(out_addr_mask_reps,
                                icmpOutStats.dwAddrMaskReps);

#undef STATS_NET_SNMP_ICMP_COUNTER_GET

/* counters from MIB_IFROW structure */

RCF_PCH_CFG_NODE_RO(node_stats_net_if_in_octets, "in_octets",
                    NULL, NULL, net_if_stats_in_octets_get);

#define STATS_NET_IF_ATTR(_name_, _next) \
    RCF_PCH_CFG_NODE_RO(node_stats_net_if_##_name_, #_name_, \
                        NULL, &node_stats_net_if_##_next,    \
                        net_if_stats_##_name_##_get);

STATS_NET_IF_ATTR(in_ucast_pkts, in_octets);
STATS_NET_IF_ATTR(in_nucast_pkts, in_ucast_pkts);
STATS_NET_IF_ATTR(in_discards, in_nucast_pkts);
STATS_NET_IF_ATTR(in_errors, in_discards);
STATS_NET_IF_ATTR(in_unknown_protos, in_errors);
STATS_NET_IF_ATTR(out_octets, in_unknown_protos);
STATS_NET_IF_ATTR(out_ucast_pkts, out_octets);
STATS_NET_IF_ATTR(out_nucast_pkts, out_ucast_pkts);
STATS_NET_IF_ATTR(out_discards, out_nucast_pkts);
STATS_NET_IF_ATTR(out_errors, out_discards);

#undef STATS_NET_IF_ATTR
/* counters from MIB_IPSTATS structure */

RCF_PCH_CFG_NODE_RO(node_stats_net_snmp_ipv4_in_recvs, "ipv4_in_recvs",
                    NULL, NULL, net_snmp_ipv4_stats_in_recvs_get);

#define STATS_NET_SNMP_IPV4_ATTR(_name_, _next) \
    RCF_PCH_CFG_NODE_RO(node_stats_net_snmp_ipv4_##_name_,          \
                        "ipv4_" #_name_,                            \
                        NULL, &node_stats_net_snmp_ipv4_##_next,    \
                        net_snmp_ipv4_stats_##_name_##_get);

STATS_NET_SNMP_IPV4_ATTR(in_hdr_errs, in_recvs);
STATS_NET_SNMP_IPV4_ATTR(in_addr_errs, in_hdr_errs);
STATS_NET_SNMP_IPV4_ATTR(forw_dgrams, in_addr_errs);
STATS_NET_SNMP_IPV4_ATTR(in_unknown_protos, forw_dgrams);
STATS_NET_SNMP_IPV4_ATTR(in_discards, in_unknown_protos);
STATS_NET_SNMP_IPV4_ATTR(in_delivers, in_discards);
STATS_NET_SNMP_IPV4_ATTR(out_requests, in_delivers);
STATS_NET_SNMP_IPV4_ATTR(out_discards, out_requests);
STATS_NET_SNMP_IPV4_ATTR(out_no_routes, out_discards);
STATS_NET_SNMP_IPV4_ATTR(reasm_timeout, out_no_routes);
STATS_NET_SNMP_IPV4_ATTR(reasm_reqds, reasm_timeout);
STATS_NET_SNMP_IPV4_ATTR(reasm_oks, reasm_reqds);
STATS_NET_SNMP_IPV4_ATTR(reasm_fails, reasm_oks);
STATS_NET_SNMP_IPV4_ATTR(frag_oks, reasm_fails);
STATS_NET_SNMP_IPV4_ATTR(frag_fails, frag_oks);
STATS_NET_SNMP_IPV4_ATTR(frag_creates, frag_fails);


/* counters from MIB_ICMP structure */

RCF_PCH_CFG_NODE_RO(node_stats_net_snmp_icmp_in_msgs, "icmp_in_msgs",
                    NULL, &node_stats_net_snmp_ipv4_frag_creates,
                    net_snmp_icmp_stats_in_msgs_get);

#define STATS_NET_SNMP_ICMP_ATTR(_name_, _next) \
    RCF_PCH_CFG_NODE_RO(node_stats_net_snmp_icmp_##_name_,          \
                        "icmp_" #_name_,                            \
                        NULL, &node_stats_net_snmp_icmp_##_next,    \
                        net_snmp_icmp_stats_##_name_##_get);

STATS_NET_SNMP_ICMP_ATTR(in_errs, in_msgs);
STATS_NET_SNMP_ICMP_ATTR(in_dest_unreachs, in_errs);
STATS_NET_SNMP_ICMP_ATTR(in_time_excds, in_dest_unreachs);
STATS_NET_SNMP_ICMP_ATTR(in_parm_probs, in_time_excds);
STATS_NET_SNMP_ICMP_ATTR(in_src_quenchs, in_parm_probs);
STATS_NET_SNMP_ICMP_ATTR(in_redirects, in_src_quenchs);
STATS_NET_SNMP_ICMP_ATTR(in_echos, in_redirects);
STATS_NET_SNMP_ICMP_ATTR(in_echo_reps, in_echos);
STATS_NET_SNMP_ICMP_ATTR(in_timestamps, in_echo_reps);
STATS_NET_SNMP_ICMP_ATTR(in_timestamp_reps, in_timestamps);
STATS_NET_SNMP_ICMP_ATTR(in_addr_masks, in_timestamp_reps);
STATS_NET_SNMP_ICMP_ATTR(in_addr_mask_reps, in_addr_masks);

STATS_NET_SNMP_ICMP_ATTR(out_msgs, in_addr_mask_reps);
STATS_NET_SNMP_ICMP_ATTR(out_errs, out_msgs);
STATS_NET_SNMP_ICMP_ATTR(out_dest_unreachs, out_errs);
STATS_NET_SNMP_ICMP_ATTR(out_time_excds, out_dest_unreachs);
STATS_NET_SNMP_ICMP_ATTR(out_parm_probs, out_time_excds);
STATS_NET_SNMP_ICMP_ATTR(out_src_quenchs, out_parm_probs);
STATS_NET_SNMP_ICMP_ATTR(out_redirects, out_src_quenchs);
STATS_NET_SNMP_ICMP_ATTR(out_echos, out_redirects);
STATS_NET_SNMP_ICMP_ATTR(out_echo_reps, out_echos);
STATS_NET_SNMP_ICMP_ATTR(out_timestamps, out_echo_reps);
STATS_NET_SNMP_ICMP_ATTR(out_timestamp_reps, out_timestamps);
STATS_NET_SNMP_ICMP_ATTR(out_addr_masks, out_timestamp_reps);
STATS_NET_SNMP_ICMP_ATTR(out_addr_mask_reps, out_addr_masks);

RCF_PCH_CFG_NODE_NA(node_net_if_stats, "stats",
                    &node_stats_net_if_out_errors,
                    NULL);

RCF_PCH_CFG_NODE_NA(node_net_snmp_stats, "stats",
                    &node_stats_net_snmp_icmp_out_addr_mask_reps,
                    NULL);


te_errno
ta_win32_conf_net_snmp_stats_init(void)
{

    return rcf_pch_add_node("/agent", &node_net_snmp_stats);
}

te_errno
ta_win32_conf_net_if_stats_init(void)
{

    return rcf_pch_add_node("/agent/interface", &node_net_if_stats);
}


// Multicast

#define DRV_TYPE 40000
#define METHOD_BUFFERED                 0
#define FILE_ANY_ACCESS                 0

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define NDIS_IOCTL_BASE   (0x800 | 0x100)

#define NDIS_IOCTL(code) \
    CTL_CODE( DRV_TYPE, NDIS_IOCTL_BASE + code, \
              METHOD_BUFFERED, FILE_ANY_ACCESS  )

#define KRX_ADD_MULTICAST_ADDR     NDIS_IOCTL(7)
#define KRX_DEL_MULTICAST_ADDR     NDIS_IOCTL(8)
#define KRX_GET_MULTICAST_LIST     NDIS_IOCTL(9)

#define KSTAT_GET     NDIS_IOCTL(18)

#define WRAPPER_DEVICE_NAME  "\\\\.\\olwrapper"
#define WRAPPER_DEVFILE_NAME "\\\\.\\olwrapper"

static te_errno
mcast_link_addr_add(unsigned int gid, const char *oid,
                    const char *value, const char *ifname, const char *addr)
{

    HANDLE dev = CreateFile(WRAPPER_DEVFILE_NAME, GENERIC_READ |
                            GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    int rc;
    DWORD bytes_returned = 0;
    unsigned char addr6[6];

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(value);
    if (strstr(ifname, "ef") == NULL)
    {
      if (dev != INVALID_HANDLE_VALUE)
        CloseHandle(dev);
      return 0;
    }
    UNUSED(ifname);

    if (INVALID_HANDLE_VALUE == dev)
    {
      return TE_RC(TE_TA_WIN32, TE_EOPNOTSUPP);
    }
    sscanf(addr, "%x:%x:%x:%x:%x:%x", &addr6[0], &addr6[1], &addr6[2],
                                      &addr6[3], &addr6[4], &addr6[5]);
    if (!DeviceIoControl(dev, KRX_ADD_MULTICAST_ADDR, addr6, 6, NULL, 0,
                         &bytes_returned, NULL))
    {
      rc = GetLastError();
      WARN("DeviceIoControl failed with errno=%d", GetLastError());
      CloseHandle(dev);
      return -2;
    }
    CloseHandle(dev);
    return 0;
}

static te_errno
mcast_link_addr_del(unsigned int gid, const char *oid, const char *ifname,
                    const char *addr)
{

    HANDLE dev = CreateFile(WRAPPER_DEVFILE_NAME, GENERIC_READ |
                            GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    int rc;
    DWORD bytes_returned = 0;
    unsigned char addr6[6];

    UNUSED(gid);
    UNUSED(oid);
    if (strstr(ifname, "ef") == NULL)
    {
      if (dev != INVALID_HANDLE_VALUE)
        CloseHandle(dev);
      return 0;
    }

    if (INVALID_HANDLE_VALUE == dev)
    {
      return TE_RC(TE_TA_WIN32, TE_EOPNOTSUPP);
    }

    sscanf(addr, "%x:%x:%x:%x:%x:%x", &addr6[0], &addr6[1], &addr6[2],
                                      &addr6[3], &addr6[4], &addr6[5]);
    if (!DeviceIoControl(dev, KRX_DEL_MULTICAST_ADDR, addr6, 6, NULL, 0,
                         &bytes_returned, NULL))
    {
      rc = GetLastError();
      WARN("DeviceIoControl failed with errno=%d", GetLastError());
      CloseHandle(dev);
      return -2;
    }
    CloseHandle(dev);
    return 0;
}

static te_errno
mcast_link_addr_list(unsigned int gid, const char *oid,
                     const char *sub_id, char **list,
                     const char *ifname)
{

    HANDLE dev = CreateFile(WRAPPER_DEVFILE_NAME, GENERIC_READ |
                            GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    int rc;
    DWORD bytes_returned = 0;
    unsigned int i;
    unsigned char *buf = malloc(1024 * sizeof(unsigned char));
    unsigned char *ret = malloc(1024 * sizeof(unsigned char));

    UNUSED(gid);
    UNUSED(oid);
    UNUSED(sub_id);

    if (strstr(ifname, "ef") == NULL)
    {
      sprintf(ret, " ");
      *list = ret;
      free(buf);
      if (dev != INVALID_HANDLE_VALUE)
        CloseHandle(dev);
      return 0;
    }

    if (INVALID_HANDLE_VALUE == dev)
    {
      free(buf);
      free(ret);
      return TE_RC(TE_TA_WIN32, TE_ENOENT);
    }

    memset(ret, 0, 1024 * sizeof(char));
    if (!DeviceIoControl(dev, KRX_GET_MULTICAST_LIST,
                        (void *)buf, 1024, (void *)buf, 1024,
                         &bytes_returned, NULL))
    {
      rc = GetLastError();
      WARN("DeviceIoControl failed with errno=%d", GetLastError());
      free(buf);
      free(ret);
      CloseHandle(dev);
      return -2;
    }
    CloseHandle(dev);
    for (i = 0; i < bytes_returned / 6; i++)
    {
      sprintf(&ret[i*18],"%02x:%02x:%02x:%02x:%02x:%02x ", buf[i*6],
              buf[i*6+1], buf[i*6+2], buf[i*6+3], buf[i*6+4], buf[i*6+5]);
    }

    free(buf);

    *list = ret;
    return 0;
}

// Multicast

// Ndis statistics

static te_errno
if_stats_get(const char *ifname, if_stats *stats, ndis_stats *raw_stats)
{
    int   rc = 0;

    ndis_stats ndstats;
    DWORD bytes_returned = 0;

    HANDLE dev = CreateFile(WRAPPER_DEVFILE_NAME, GENERIC_READ |
                            GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (INVALID_HANDLE_VALUE == dev)
    {
#if 0
        WARN("Wrapper not found. Cannot get statistics.");
#endif
        return -1;
    }

    if (strstr(ifname, "ef") == NULL)
    {
        CloseHandle(dev);
        return -1;
    }

    memset(&ndstats, 0, sizeof(ndstats));

    if (!DeviceIoControl(dev, KSTAT_GET, NULL, 0,
                         (void *)&ndstats, sizeof(ndstats),
                         &bytes_returned, NULL))
    {
        rc = GetLastError();
        WARN("DeviceIoControl failed with errno=%d", GetLastError());
        CloseHandle(dev);
        return -2;
    }
    CloseHandle(dev);

    if (raw_stats != NULL)
    {
        memcpy(raw_stats, &ndstats, sizeof(ndstats));
    }

    if (stats != NULL)
    {
        stats->in_octets = ndstats.gen_broadcast_bytes_rcv;
        stats->in_ucast_pkts = ndstats.gen_directed_frames_rcv;
        stats->in_nucast_pkts = ndstats.gen_broadcast_frames_rcv +
                                ndstats.gen_multicast_frames_rcv;
        stats->in_discards = ndstats.gen_rcv_error +
                             ndstats.gen_rcv_no_buffer;
        stats->in_errors = ndstats.gen_rcv_error;
#if 0
        stats->in_unknown_protos = 0;
#endif
        stats->out_octets = ndstats.gen_broadcast_bytes_xmit;
        stats->out_ucast_pkts = ndstats.gen_directed_frames_xmit;
        stats->out_nucast_pkts = ndstats.gen_broadcast_frames_xmit +
                                 ndstats.gen_multicast_frames_xmit;
#if 0
        stats->out_discards = 0;
#endif
        stats->out_errors = ndstats.gen_xmit_error;
    }
    return 0;
}

//PHY support.
RCF_PCH_CFG_NODE_RO(node_phy_state, "state", NULL, NULL,
                    phy_state_get);

/* TODO: Should write separate methods for oper/admin values */
RCF_PCH_CFG_NODE_RO(node_phy_speed_oper, "speed_oper", NULL,
                    &node_phy_state, phy_speed_get);

RCF_PCH_CFG_NODE_RWC(node_phy_speed_admin, "speed_admin", NULL,
                     &node_phy_speed_oper,
                     phy_speed_get, phy_speed_set, &node_phy);

RCF_PCH_CFG_NODE_RO(node_phy_duplex_oper, "duplex_oper", NULL,
                    &node_phy_speed_admin, phy_duplex_get);

RCF_PCH_CFG_NODE_RWC(node_phy_duplex_admin, "duplex_admin", NULL,
                     &node_phy_duplex_oper,
                     phy_duplex_get, phy_duplex_set, &node_phy);

RCF_PCH_CFG_NODE_RO(node_phy_autoneg_oper, "autoneg_oper", NULL,
                     &node_phy_duplex_admin, phy_autoneg_get);

RCF_PCH_CFG_NODE_RWC(node_phy_autoneg_admin, "autoneg_admin", NULL,
                     &node_phy_autoneg_oper,
                     phy_autoneg_get, phy_autoneg_set, &node_phy);

RCF_PCH_CFG_NODE_NA_COMMIT(node_phy, "phy", &node_phy_autoneg_admin,
                           NULL, phy_commit);

/**
 * Get PHY state value.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         location of value
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_state_get(unsigned int gid, const char *oid, char *value,
              const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    UNUSED(ifname);
    int state = -1;

    snprintf(value, RCF_MAX_VAL, "%d", state);

    return 0;
}

/**
 * Get PHY current speed value.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         location of value
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_speed_get(unsigned int gid, const char *oid, char *value,
              const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);


    int speed = -1;
    ndis_stats stats;
    if(if_stats_get( ifname, NULL, &stats) == 0)
    {
        speed = (int)(stats.gen_link_speed / 10000);
    }

    snprintf(value, RCF_MAX_VAL, "%d", speed);

    return 0;
}

/**
 * Change speed value.
 *
 * Autonegatiation state should be turned off before
 * change the values of duplex and speed PHY parameters.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_speed_set(unsigned int gid, const char *oid, const char *value,
              const char *ifname)
{
    UNUSED(oid);
    UNUSED(gid);

    int speed;
    int rc;
    /* Get value provided by caller */
    sscanf(value, "%d", &speed);
    if (strcmp(ifname, "ef1") == 0)
    {
        if (speed_duplex_to_set == 0)
            speed_duplex_to_set = 1; /* Assume full duplex */
        else
        {
            /* resetting current speed to zero retaining duplex state */
            speed_duplex_to_set &= 1;
            speed_duplex_to_set = 1 - speed_duplex_to_set;
        }
        switch (speed)
        {
           case 10:
               speed_duplex_to_set += 1;
               break;
           case 100:
               speed_duplex_to_set += 3;
               break;
           case 1000:
               speed_duplex_to_set += 5;
               break;
           case 10000:
               speed_duplex_to_set += 7;
               break;
        }
    }
    else
    {
        ERROR("change speed state is only supported on ef1");
        return TE_RC(TE_TA_WIN32, TE_EOPNOTSUPP);
    }
    return 0;
}

/**
 * Get PHY autonegotiation state.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         location of value
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_autoneg_get(unsigned int gid, const char *oid, char *value,
                const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    int rc;
    int state = -1;

    if (strcmp(ifname, "ef1") == 0)
    {
        if ((rc = phy_parameters_get(ifname)) == 0)
        {
            if (speed_duplex_state == 0)
                state = TE_PHY_AUTONEG_ON;
            else
                state = TE_PHY_AUTONEG_OFF;
        }
        else
        {
            ERROR("failed to get autoneg state");
            return TE_RC(TE_TA_WIN32, rc);
        }
    }
    snprintf(value, RCF_MAX_VAL, "%d", state);

    return 0;
}

/**
 * Change autonegatiation state.
 * Possible values: 0 - autonegatiation off
 *                  1 - autonegatiation on
 *
 * Autonegatiation state should turn off before
 * change the values of duplex and speed PHY parameters.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_autoneg_set(unsigned int gid, const char *oid, const char *value,
                const char *ifname)
{
    UNUSED(oid);
    UNUSED(gid);

    int rc;
    ndis_stats stats;
    int autoneg = -1;
    int speed = -1;

    sscanf(value, "%d", &autoneg);
    if (autoneg != TE_PHY_AUTONEG_ON && autoneg != TE_PHY_AUTONEG_OFF)
    {
        ERROR("cannot set unknown autonegotiation state: %s", value);
        return TE_RC(TE_TA_WIN32, TE_EINVAL);
    }
    if (strcmp(ifname, "ef1") == 0)
    {

        if (autoneg == TE_PHY_AUTONEG_ON)
        {
            speed_duplex_to_set = 0;
        }
        else
        if (autoneg == TE_PHY_AUTONEG_OFF)
        {
            if (speed_duplex_to_set == 0)
            {
                /**
                  * setting to current speed and full duplex because
                  * we cannot get duplex state from interface
                  */
                if((rc = if_stats_get(ifname, NULL, &stats)) != 0)
                {
                    ERROR("failed to get link speed");
                    return TE_OS_RC(TE_TA_WIN32, rc);
                }
                speed = (int)(stats.gen_link_speed / 10000);
                switch (speed)
                {
                    case 10:
                        speed_duplex_to_set = 2;
                        break;
                    case 100:
                        speed_duplex_to_set = 4;
                        break;
                    case 1000:
                        speed_duplex_to_set = 6;
                        break;
                    case 10000:
                        speed_duplex_to_set = 8;
                        break;
                    default:
                        WARN("Unknown speed value %d, setting to 1000",
                             speed);
                        speed_duplex_to_set = 6;
                        break;
                }
            }
        }
    }
    else
    {
        ERROR("changing autonegotiation state is only supported on ef1");
        return TE_RC(TE_TA_WIN32, TE_EOPNOTSUPP);
    }

    return 0;
}

/**
 * Get PHY duplex state.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         location of value
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_duplex_get(unsigned int gid, const char *oid, char *value,
               const char *ifname)
{
    UNUSED(gid);
    UNUSED(oid);
    int rc;

    if (strcmp(ifname, "ef1") == 0)
    {
        if ((rc = phy_parameters_get(ifname)) == 0)
        {
            if (speed_duplex_state != 0)
                sprintf(value, "%s", ((speed_duplex_state % 2) == 0)?
                                      TE_PHY_DUPLEX_STRING_FULL:
                                      TE_PHY_DUPLEX_STRING_HALF);
            else
                sprintf(value, "%s", TE_PHY_DUPLEX_STRING_UNKNOWN);

            return 0;
        }
        else
        {
            ERROR("failed to get duplex state");
            return TE_RC(TE_TA_WIN32, rc);
        }
    }
    sprintf(value, "not supported");

    return 0;
}

/**
 * Get duplex state by name string
 *
 * @param name          Duplex name string
 *
 * @return DUPLEX_FULL, DUPLEX_HALF, DUPLEX_UNKNOWN or -1 if
 *         duplex name string does not recognized
 */
static inline int
phy_get_duplex_by_name(const char *name)
{
    if (name == NULL)
        return -1;

    if (strcasecmp(name, TE_PHY_DUPLEX_STRING_HALF) == 0)
        return TE_PHY_DUPLEX_HALF;

    if (strcasecmp(name, TE_PHY_DUPLEX_STRING_FULL) == 0)
        return TE_PHY_DUPLEX_FULL;

    if (strcasecmp(name, TE_PHY_DUPLEX_STRING_UNKNOWN) == 0)
        return TE_PHY_DUPLEX_UNKNOWN;

    return -1;
}

/**
 * Change duplex state.
 * Possible values: 0 - half duplex
 *                  1 - full duplex
 *
 * Autonegatiation state should be turned off before
 * change the values of duplex and speed PHY parameters.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instance identifier (unused)
 * @param value         new value pointer
 * @param ifname        name of the interface
 *
 * @return              Status code
 */
static te_errno
phy_duplex_set(unsigned int gid, const char *oid, const char *value,
           const char *ifname)
{
    UNUSED(oid);
    UNUSED(gid);

    int rc;
    int duplex = -1;

    /* Get value provided by caller */
    duplex = phy_get_duplex_by_name(value);
    if (duplex != TE_PHY_DUPLEX_HALF &&
        duplex != TE_PHY_DUPLEX_FULL &&
        duplex != TE_PHY_DUPLEX_UNKNOWN)
    {
        ERROR("cannot set unknown duplex state: %s", value);
        return TE_RC(TE_TA_WIN32, TE_EINVAL);
    }
    if (duplex == TE_PHY_DUPLEX_UNKNOWN)
    {
        WARN("Skipped setting \"unknown\" duplex state");
        return 0;
    }
    if (strcmp(ifname, "ef1") == 0)
    {
        if (speed_duplex_to_set == 0) /* Will set by default 1Gbit speed */
        {
            speed_duplex_to_set = (duplex == TE_PHY_DUPLEX_FULL)?
                                  6 : 5;
        } else
        {
            /* Last bit of speed_duplex_to_set set to 1 means half-duplex */
            if (((speed_duplex_to_set & 1) == 0) && /* full duplex */
                duplex == TE_PHY_DUPLEX_HALF)
            {
                speed_duplex_to_set -= 1;
            }
            else if (((speed_duplex_to_set & 1) == 1) && /* half duplex */
                     duplex == TE_PHY_DUPLEX_FULL)
            {
                speed_duplex_to_set += 1;
            }
        }
    }
    else
    {
        ERROR("change duplex state is only supported on ef1");
        return TE_RC(TE_TA_WIN32, TE_EOPNOTSUPP);
    }

    return 0;
}

/**
 * Apply locally stored changes.
 *
 * @param gid           GID
 * @param p_oid         Pointer to the OID
 *
 * @return              Status code
 */
static te_errno
phy_commit(unsigned int gid, const cfg_oid *p_oid)
{
    UNUSED(gid);
    char *ifname;
    int rc;

    /* Extract interface name */
    ifname = CFG_OID_GET_INST_NAME(p_oid, 2);

    if (strcmp(ifname, "ef1") == 0)
    {
        if ((rc = phy_parameters_set(ifname)) != 0)
        {
            ERROR("failed to set phy parameters");
            return TE_RC(TE_TA_WIN32, rc);
        }
    }
    else
    {
        ERROR("change speed/duplex/autoneg state is only supported on ef1");
        return TE_RC(TE_TA_WIN32, TE_EOPNOTSUPP);
    }
    return 0;
}

/**
 * Initialize PHY subtree
 */
extern te_errno
ta_unix_conf_phy_init(void)
{
    return rcf_pch_add_node("/agent/interface", &node_phy);
    if (phy_parameters_get("ef1") != 0)
        WARN("Phy params can't be received for ef1");
    speed_duplex_to_set = speed_duplex_state;
}

#define NET_PATH        "SYSTEM\\CurrentControlSet\\Control\\Class\\" \
                        "{4D36E972-E325-11CE-BFC1-08002bE10318}"
//#define NDIS_SF_0_2_2         "ndis_basic"
#define NDIS_SF_0_2_2   "{c641c770-faac-44ed-9c73-48d1b5e59200}"
#define NDIS_SF_0_2_1         "sfe_ndis_0"
#define BUFSIZE_REG         256
#define SPEED_DUPLEX_NAME TEXT("*SpeedDuplex")
#define DRIVER_VERSION_NAME TEXT("DriverVersion")

static int get_settings_path(char *path)
{
#ifdef _WIN32
    HKEY key, subkey;
    static char subkey_name[BUFSIZE_REG];
    static char subkey_path[BUFSIZE_REG];
    static char value[BUFSIZE_REG];
    DWORD subkey_size;
    DWORD value_size = BUFSIZE_REG;

    DWORD  ret = -1;
    int i;
    FILETIME tmp;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, NET_PATH, 0,
                     KEY_READ, &key) != ERROR_SUCCESS)
    {
        VERB("%s: RegOpenKeyEx() failed with errno %lu\n",
             __FUNCTION__, GetLastError());
        return -1;
    }

    for (i = 0, subkey_size = value_size = BUFSIZE_REG;
         RegEnumKeyEx(key, i, subkey_name, &subkey_size,
                      NULL, NULL, NULL, &tmp) != ERROR_NO_MORE_ITEMS;
         i++, subkey_size = value_size = BUFSIZE_REG)
    {
        sprintf(subkey_path, "%s\\%s", NET_PATH, subkey_name);
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subkey_path,
                         0, KEY_READ, &subkey) != ERROR_SUCCESS)
        {
            continue;
        }

        if (RegQueryValueEx(subkey, "MatchingDeviceId",
                            NULL, NULL, (unsigned char *)value,
                            &value_size) != ERROR_SUCCESS)
        {
            /* Field with device ID is absent, may its virtual device */
            RegCloseKey(subkey);
            continue;
        }
        if ((strstr(value, NDIS_SF_0_2_2) != NULL))
        {
            strcpy(path, subkey_path);
            ret = 0;
            RegCloseKey(subkey);
            break;
        }
        if ((strstr(value, NDIS_SF_0_2_1) != NULL))
        {
            strcpy(path, subkey_path);
            ret = 0;
            RegCloseKey(subkey);
            break;
        }
        RegCloseKey(subkey);
    }
    RegCloseKey(key);
    return ret;
#else
    UNUSED(path);
    return 0;
#endif
}

static int get_driver_version()
{
    int err;
    HKEY hkKey;
    static char path[BUFSIZE_REG];
    char driver_version_value[20];
    unsigned int dwTemp;

    if (get_settings_path(path) != 0)
    {
      err = -1;
      ERROR("Failed to find NDIS port 0 entry");
      return TE_RC(TE_TA_WIN32, err);
    }

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0,
                    KEY_READ, &hkKey) == ERROR_SUCCESS)
    {
        dwTemp = sizeof(driver_version_value);
        if (RegQueryValueEx(hkKey, DRIVER_VERSION_NAME,
                            NULL, NULL, (BYTE *)driver_version_value,
                            &dwTemp) != ERROR_SUCCESS)
        {
            err = GetLastError();
            ERROR("Failed to get DriverVersion, err = %d", err);
            RegCloseKey(hkKey);
            return DRIVER_VERSION_UNKNOWN;
        }
    }
    else
    {
        err = GetLastError();
        WARN("Failed to open NDIS registry key, err = %d", err);
        return DRIVER_VERSION_UNKNOWN;
    }
    RegCloseKey(hkKey);

    if (strncmp(driver_version_value, "2.1", 3) == 0)
    {
        return DRIVER_VERSION_2_1;
    }
    else
    if (strncmp(driver_version_value, "2.2", 3) == 0)
    {
        return DRIVER_VERSION_2_2;
    } else
    if (strncmp(driver_version_value, "2.3", 3) == 0)
    {
        return DRIVER_VERSION_2_3;
    }

    return DRIVER_VERSION_UNKNOWN;
}

static int phy_parameters_get(const char *ifname)
{
#ifdef _WIN32
    int err;
    HKEY hkKey;
    static char path[BUFSIZE_REG];
    char speed_duplex_value[10];
    unsigned int dwTemp;

    if (strcmp(ifname, "ef1") != 0)
    {
        err = -1;
        ERROR("Wrong interface name %s, only ef1 is supported", ifname);
        return TE_RC(TE_TA_WIN32, err);
    }

    if (get_settings_path(path) != 0)
    {
        err = -1;
        WARN("Failed to find NDIS port 0 entry");
        return TE_RC(TE_TA_WIN32, err);
    }

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0,
                    KEY_READ | KEY_WRITE, &hkKey) == ERROR_SUCCESS)
    {
        dwTemp = sizeof(speed_duplex_value);
        if (RegQueryValueEx(hkKey, SPEED_DUPLEX_NAME,
                            NULL, NULL, (BYTE *)speed_duplex_value,
                            &dwTemp) == ERROR_SUCCESS)
        {
            sscanf(speed_duplex_value, "%d", &speed_duplex_state);
        }
        else
        {
            err = GetLastError();
            ERROR("Failed to get *SpeedDuplex, err = %d", err);
            RegCloseKey(hkKey);
            return TE_RC(TE_TA_WIN32, err);
        }
    }
    else
    {
        err = GetLastError();
        WARN("Failed to get open NDIS registry key, err = %d", err);
        return TE_RC(TE_TA_WIN32, err);
    }
    RegCloseKey(hkKey);
    return 0;
#else
    UNUSED(ifname);
    return 0;
#endif
}

static int phy_parameters_set(const char *ifname)
{
#ifdef _WIN32
    int err;
    HKEY hkKey;
    static char path[BUFSIZE_REG];
    char speed_duplex_value[10];

    if (strcmp(ifname, "ef1") != 0)
    {
        err = 2;
        ERROR("Wrong interface name %s, only ef1 is supported", ifname);
        return TE_RC(TE_TA_WIN32, err);
    }

    if (get_settings_path(path) != 0)
    {
        err = -1;
        ERROR("Failed to find NDIS port 0 entry");
        return TE_RC(TE_TA_WIN32, err);
    }

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0,
                    KEY_READ | KEY_WRITE, &hkKey) == ERROR_SUCCESS)
    {
        sprintf(speed_duplex_value, "%d", speed_duplex_to_set);
        if (RegSetValueEx(hkKey, SPEED_DUPLEX_NAME, 0, REG_SZ,
                          (CONST BYTE *)speed_duplex_value,
                          strlen(speed_duplex_value)) != ERROR_SUCCESS)
        {
            err = GetLastError();
            ERROR("Failed to set *SpeedDuplex, err = %d", err);
            RegCloseKey(hkKey);
            return TE_RC(TE_TA_WIN32, err);
        }
    }
    else
    {
        err = GetLastError();
        ERROR("Failed to get open NDIS registry key, err = %d, path = %s",
            err, path);
        return TE_RC(TE_TA_WIN32, err);
    }
    RegCloseKey(hkKey);
    return 0;
#else
    UNUSED(ifname);
    return 0;
#endif
}
#undef NET_PATH
#undef NDIS_SF_0_2_1
#undef NDIS_SF_0_2_2
#undef BUFSIZE_REG
#undef SPEED_DUPLEX_NAME

/**
 * Get VLAN ifname
 *
 * @param gid           request group identifier (unused)
 * @param oid           full object instence identifier
 * @param value         location for interface name
 * @param ifname        name of the interface
 * @param vid           name of the vlan (decimal integer)
 *
 * @return status
 */
static te_errno
vlan_ifname_get(unsigned int gid, const char *oid, char *value,
                const char *ifname, const char *vid)
{
    int vlan_id = atoi(vid);

    VERB("%s: gid=%u oid='%s', ifname = '%s', vid %d",
         __FUNCTION__, gid, oid, ifname,  vlan_id);

    if (get_driver_version() >= DRIVER_VERSION_2_2)
    {
        sprintf(value, "%s.%s", ifname, vid);
    }
    else /* 2.1 */
    {
        sprintf(value, "%s", ifname);
    }
    return 0;
}

/**
 * Get instance list for object "agent/interface/vlans".
 *
 * @param gid           group identifier (unused)
 * @param oid           full identifier of the father instance
 * @param sub_id        ID of the object to be listed (unused)
 * @param list          location for the list pointer
 *
 * @return              Status code
 * @retval 0            success
 * @retval TE_ENOMEM       cannot allocate memory
 */
static te_errno
vlans_list(unsigned int gid, const char *oid,
           const char *sub_id, char **list,
           const char *ifname)
{
    size_t i;
    char *b;
    char *frname;
    DWORD *vid_list;
    int rc, count, count_skipped = 0;

    UNUSED(sub_id);

    if (strstr(ifname, "ef")== NULL)
    {
        VERB("%s: gid=%u oid='%s', ifname %s, num vlans %d",
        __FUNCTION__, gid, oid, ifname, 0);

       *list = NULL;
        return 0;
    }

    if (get_driver_version() >= DRIVER_VERSION_2_2)
    {
        if (strstr(ifname, ".") != NULL)
        {
            VERB("%s: gid=%u oid='%s', ifname %s, num vlans %d",
                 __FUNCTION__, gid, oid, ifname, 0);
            *list = NULL;
            return 0;
        }
        if (!wmi_imported)
        {
            *list = NULL;
            return 0;
        }
        frname = ifindex2frname(ifname2ifindex(ifname));
        if (frname == NULL)
        {
            ERROR("ifindex2frname failed");
            return 0;
        }
        rc = pwmi_get_vlan_list(frname, &vid_list, &count);

        if (rc != 0)
        {
            ERROR("Getting vlan list by WMI failed, rc=%d", rc);
            *list = NULL;
            return 0;
        }
        if (vid_list == NULL || count == 0)
        {
            *list = NULL;
            return 0;
        }
                           /* max digits in VLAN id + space */
        b = *list = malloc(count * 6  + 1);

        for (i = 0; i < count; i++)
        {
            /* Exclude special vlanids from list */
            if (vid_list[i] == 0 || vid_list[i] == 4095)
            {
                WARN("Special vlan id %d skipped", vid_list[i]);
                count_skipped++;
                continue;
            }
            b += sprintf(b, "%d ", vid_list[i]);
        }
        if (count_skipped == count)
        {
            free(b);
            *list = NULL;
        }
        return 0;

    }
    else
    {
        VERB("%s: gid=%u oid='%s', ifname %s, num vlans %d",
             __FUNCTION__, gid, oid, ifname, n_2_1_vlans);

        if (n_2_1_vlans == 0)
        {
            *list = NULL;
            return 0;
        }
                           /* max digits in VLAN id + space */
        b = *list = malloc(n_2_1_vlans * 6 + 1);
        if (*list == NULL)
            return TE_RC(TE_TA_UNIX, TE_ENOMEM);

        for (i = 0; i < n_2_1_vlans; i++)
            b += sprintf(b, "%d ", vlans_2_1_buffer[i]);

        return 0;
    }
}

/**
 * Add link to VLAN Ethernet device.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier
 * @param value         value string
 * @param ifname        device name, over it VLAN should be added
 * @param vid_str       VLAN id string, decimal notation
 *
 * @return              Status code
 */
static te_errno
vlans_add(unsigned int gid, const char *oid, const char *value,
              const char *ifname, const char *vid_str)
{
    int vid = atoi(vid_str);
    int rc = 0;
    te_bool priority = TRUE;

    UNUSED(value);

    VERB("%s: gid=%u oid='%s', vid %s, ifname %s",
         __FUNCTION__, gid, oid, vid_str, ifname);

    if (strstr(ifname,"ef")== NULL)
    {
        ERROR("Only ef* windows interfaces support VLANS");
        return TE_RC(TE_TA_WIN32, EINVAL);
    }

    if (get_driver_version() >= DRIVER_VERSION_2_2)
    {
        if (vid & TAG_PRI_ONLY)
        {
            if (vid != TAG_PRI_ONLY)
                WARN("Vlan id has been set to 0 in Priority only mode");
            vid = TAG_PRI_ONLY;
            /*priority = TRUE;*/
        }
        else if (vid & TAG_VLAN_ONLY)
            priority = FALSE;

        if (!wmi_imported)
            return TE_RC(TE_TA_WIN32, TE_EFAULT);
        rc = pwmi_add_vlan(vid & MAX_VLANS, priority);
        if (rc != 0)
        {
            ERROR("Failed to set VLAN via WMI");
            return TE_RC(TE_TA_WIN32, TE_EFAULT);
        }
    }
    else
    {
        if (n_2_1_vlans == 1)
        {
            ERROR("VLAN interface is already set on %s", ifname);
            return TE_RC(TE_TA_WIN32, EINVAL);
        }
        rc = set_vlan_reg("ef1", vid);
        if (rc != 0)
        {
            ERROR("Failed to physically set VLAN");
            return TE_RC(TE_TA_WIN32, TE_EFAULT);
        }
        vlans_2_1_buffer[n_2_1_vlans] = vid;
        n_2_1_vlans += 1;
    }
    return 0;
}

/**
 * Delete link to VLAN Ethernet device.
 *
 * @param gid           group identifier (unused)
 * @param oid           full object instence identifier (unused)
 * @param ifname        VLAN device name: ef*
 *
 * @return              Status code
 */
static te_errno
vlans_del(unsigned int gid, const char *oid, const char *ifname,
          const char *vid_str)
{
    int vid = atoi(vid_str);
    int rc = 0;

    WARN("%s: gid=%u oid='%s', vid %s, ifname %s",
         __FUNCTION__, gid, oid, vid_str, ifname);

    if (strstr(ifname,"ef")== NULL)
    {
        ERROR("Only ef* windows interfaces support VLANS");
        return TE_RC(TE_TA_WIN32, EINVAL);
    }

    if (get_driver_version() >= DRIVER_VERSION_2_2)
    {
        if (!wmi_imported)
            return TE_RC(TE_TA_WIN32, EINVAL);
        rc = pwmi_del_vlan(vid & MAX_VLANS);
        if (rc != 0)
        {
            ERROR("Failed to remove VLAN via WMI");
            return TE_RC(TE_TA_WIN32, TE_EFAULT);
        }
    }
    else
    {
        if (n_2_1_vlans == 0)
        {
            ERROR("VLAN interface are not set "
                  "on %s, cannot delete" , ifname);
            return TE_RC(TE_TA_WIN32, EINVAL);
        }

        if (vlans_2_1_buffer[n_2_1_vlans] != vid)
        {
            WARN("Trying to delete VLAN with VLAN id=, still deleting",vid);
        }
        rc = remove_vlan_reg("ef1", vid);
        if (rc != 0)
        {
            ERROR("Failed to physically remove VLAN");
            return TE_RC(TE_TA_WIN32, TE_EFAULT);
        }
        n_2_1_vlans -= 1;
    }
    return 0;
}

static int
set_vlan_reg(const char *ifname, int vlan_id)
{
    int vlan_mode = 3;
    char buffer[RCF_MAX_PATH + 1];
    RING("Setting %d VLAN on '%s'", vlan_id, ifname);
    if (strcmp(ifname, "ef1") != 0)
    {
      ERROR("Wrong interface name %s, only ef1 is appropriate", ifname);
      return -1;
    }
    if (vlan_id & TAG_PRI_ONLY)
    {
        if (vlan_id != TAG_PRI_ONLY)
            WARN("Vlan id has been set to 0 in Priority only mode");
        vlan_id = TAG_PRI_ONLY;
        vlan_mode = 1;
    }
    else if (vlan_id & TAG_VLAN_ONLY)
        vlan_mode = 2;

    snprintf(buffer, sizeof(buffer),
             "./sish_client.exe "
             "--server=127.0.0.1 "
             "--command=\`cygpath -w \$PWD\`\\\\windows_layer2_manage.exe "
             "--args=\"set vlanid %d vlantagging %d\"",
             (vlan_id & MAX_VLANS), vlan_mode);
    if (system(buffer) == 0)
        return 0;
    else
    {
        errno = ENXIO;
        return -1;
    }
}

static int
remove_vlan_reg(const char *ifname, int vlan_id)
{
    char buffer[RCF_MAX_PATH + 1];
    RING("Deleting %d VLAN on '%s'",vlan_id, ifname);
    if (strcmp(ifname, "ef1") != 0)
    {
      ERROR("Wrong interface name %s, only ef1 is appropriate", ifname);
      return -1;
    }
    snprintf(buffer, sizeof(buffer),
             "./sish_client.exe "
             "--server=127.0.0.1 "
             "--command=\`cygpath -w \$PWD\`\\\\windows_layer2_manage.exe "
             "--args=\"set vlanid %d vlantagging 0\"",
             (vlan_id & MAX_VLANS));
    if (system(buffer) == 0)
        return 0;
    else
    {
        errno = ENXIO;
        return -1;
    }
}
