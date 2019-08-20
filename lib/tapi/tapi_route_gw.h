/** @file
 * @brief Gateway host configuration API
 *
 * @defgroup ts_tapi_route_gw Control network channel using a gateway
 * @ingroup te_ts_tapi
 * @{
 *
 * Macros and functions for gateway configuration to be used in tests.
 * "Gateway" here is the third host which forwards packets between
 * two testing hosts not connected directly.
 * The header must be included from test sources only. It is allowed
 * to use the macros only from @b main() function of the test.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Dmitry Izbitsky <Dmitry.Izbitsky@oktetlabs.ru>
 */

#ifndef __TE_TAPI_ROUTE_GW_H__
#define __TE_TAPI_ROUTE_GW_H__

#include "te_config.h"

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif

#include "te_defs.h"
#include "te_errno.h"
#include "te_stdint.h"
#include "conf_api.h"
#include "logger_api.h"
#include "rcf_api.h"

#include "tapi_sockaddr.h"
#include "tapi_cfg_base.h"
#include "tapi_cfg.h"

/**
 * Structure describing gateway connecting IUT and Tester hosts.
 */
typedef struct tapi_route_gateway {
    char iut_ta[RCF_MAX_NAME];                /**< TA on IUT. */
    char tst_ta[RCF_MAX_NAME];                /**< TA on Tester. */
    char gw_ta[RCF_MAX_NAME];                 /**< TA on gateway. */

    const struct if_nameindex  *iut_if;       /**< Network interface on
                                                   IUT. */
    const struct if_nameindex  *tst_if;       /**< Network interface on
                                                   Tester. */
    const struct if_nameindex  *gw_iut_if;    /**< Network interface on
                                                   gateway connected to
                                                   IUT. */
    const struct if_nameindex  *gw_tst_if;    /**< Network interface on
                                                   gateway connected to
                                                   Tester. */

    const struct sockaddr *iut_addr;          /**< IUT network address. */
    const struct sockaddr *tst_addr;          /**< Tester network
                                                   address. */
    const struct sockaddr *gw_iut_addr;       /**< Network address on
                                                   gw_iut_if interface. */
    const struct sockaddr *gw_tst_addr;       /**< Network address on
                                                   gw_tst_if interface. */
    const struct sockaddr *alien_link_addr;   /**< Alien link address. */
} tapi_route_gateway;

/**
 * Declare test parameters related to gateway configuration.
 */
#define TAPI_DECLARE_ROUTE_GATEWAY_PARAMS \
    rcf_rpc_server *pco_iut;                      \
    rcf_rpc_server *pco_tst;                      \
    rcf_rpc_server *pco_gw;                       \
                                                  \
    const struct if_nameindex  *tst_if;           \
    const struct if_nameindex  *iut_if;           \
    const struct if_nameindex  *gw_tst_if;        \
    const struct if_nameindex  *gw_iut_if;        \
                                                  \
    const struct sockaddr *iut_addr;              \
    const struct sockaddr *tst_addr;              \
    const struct sockaddr *gw_iut_addr;           \
    const struct sockaddr *gw_tst_addr;           \
    const struct sockaddr *alien_link_addr;

/**
 * Get test parameters related to gateway configuration.
 */
#define TAPI_GET_ROUTE_GATEWAY_PARAMS \
    TEST_GET_PCO(pco_iut);                        \
    TEST_GET_PCO(pco_tst);                        \
    TEST_GET_PCO(pco_gw);                         \
    TEST_GET_IF(tst_if);                          \
    TEST_GET_IF(iut_if);                          \
    TEST_GET_IF(gw_tst_if);                       \
    TEST_GET_IF(gw_iut_if);                       \
    TEST_GET_ADDR(pco_iut, iut_addr);             \
    TEST_GET_ADDR(pco_tst, tst_addr);             \
    TEST_GET_ADDR(pco_gw, gw_iut_addr);           \
    TEST_GET_ADDR(pco_gw, gw_tst_addr);           \
    TEST_GET_LINK_ADDR(alien_link_addr);

/**
 * Initialize gateway structure.
 *
 * @param gw_     Gateway structure.
 */
#define TAPI_INIT_ROUTE_GATEWAY(gw_) \
    CHECK_RC(tapi_route_gateway_init(                                   \
                            &gw_, pco_iut->ta, pco_tst->ta, pco_gw->ta, \
                            iut_if, tst_if, gw_iut_if, gw_tst_if,       \
                            iut_addr, tst_addr,                         \
                            gw_iut_addr, gw_tst_addr, alien_link_addr))

/**
 * Add dynamic entry to neighbour table.
 *
 * @param ta_src            Source TA.
 * @param ifname_src        Source interface name.
 * @param ta_dest           Destination TA.
 * @param ifname_dest       Destination interface name.
 * @param addr_dest         Destination address.
 * @param link_addr_dest    New destination MAC or @c NULL.
 *                          If @c NULL link address of @p ta_dest
 *                          interface will be used.
 *
 * @note Netlink does not allow to make address resolution
 *       at a reasonable time if it does not know where to send
 *       the request.
 *       Therefore the dst MAC address is required.
 *
 * @return Status code.
 */
extern te_errno tapi_add_dynamic_arp(const char *ta_src,
                                     const char *ifname_src,
                                     const char *ta_dest,
                                     const char *ifname_dest,
                                     const struct sockaddr *addr_dest,
                                     const struct sockaddr *link_addr_dest);


/**
 * Add static entry to neighbour table with fake or actual MAC address.
 *
 * @param ta_src            Source TA.
 * @param ifname_src        Source interface name.
 * @param ta_dest           Destination TA.
 * @param ifname_dest       Destination interface name.
 * @param addr_dest         Destination address.
 * @param link_addr_dest    New destination MAC or @c NULL.
 *                          If @c NULL link address of @p ta_dest
 *                          interface will be used.
 *
 * @return Status code.
 */
extern te_errno tapi_add_static_arp(const char *ta_src,
                                    const char *ifname_src,
                                    const char *ta_dest,
                                    const char *ifname_dest,
                                    const struct sockaddr *addr_dest,
                                    const struct sockaddr *link_addr_dest);


/**
 * Update entry in the neighbour table.
 *
 * @param ta_src            Source TA.
 * @param ifname_src        Source interface name.
 * @param ta_dest           Destination TA.
 * @param ifname_dest       Destination interface name.
 * @param addr_dest         Destination address.
 * @param link_addr_dest    New destination MAC or @c NULL.
 * @param is_static         If the new ARP row should be static.
 *
 * @return Status code.
 *
 * @sa tapi_add_static_arp, tapi_add_dynmaic_arp
 */
extern te_errno tapi_update_arp(const char *ta_src,
                                const char *ifname_src,
                                const char *ta_dest,
                                const char *ifname_dest,
                                const struct sockaddr *addr_dest,
                                const void *link_addr_dest,
                                 te_bool is_static);

/**
 * Remove existing ARP table entry, wait for a while, check that it
 * did not reappear automatically. If it did, try to remove it again
 * a few times before giving up.
 *
 * @param ta            Test Agent name.
 * @param if_name       Interface name.
 * @param net_addr      IP address.
 *
 * @return Status code.
 */
extern te_errno tapi_remove_arp(const char *ta,
                                const char *if_name,
                                const struct sockaddr *net_addr);

/**
 * Initialize gateway structure.
 *
 * @param gw                Pointer to gateway structure.
 * @param iut_ta            TA on IUT.
 * @param tst_ta            TA on Tester.
 * @param gw_ta             TA on gateway.
 * @param iut_if            Network interface on IUT.
 * @param tst_if            Network interface on Tester.
 * @param gw_iut_if         Network interface on gateway connected to IUT.
 * @param gw_tst_if         Network interface on gateway connected to
 *                          Tester.
 * @param iut_addr          Network address on IUT.
 * @param tst_addr          Network address on Tester.
 * @param gw_iut_addr       Network address on @p gw_iut_if.
 * @param gw_tst_addr       Network address on @p gw_tst_if.
 * @param alien_link_addr   Alien ethernet address.
 *
 * @return Status code.
 */
extern te_errno tapi_route_gateway_init(
                              tapi_route_gateway *gw,
                              const char *iut_ta,
                              const char *tst_ta,
                              const char *gw_ta,
                              const struct if_nameindex *iut_if,
                              const struct if_nameindex *tst_if,
                              const struct if_nameindex *gw_iut_if,
                              const struct if_nameindex *gw_tst_if,
                              const struct sockaddr *iut_addr,
                              const struct sockaddr *tst_addr,
                              const struct sockaddr *gw_iut_addr,
                              const struct sockaddr *gw_tst_addr,
                              const struct sockaddr *alien_link_addr);

/**
 * Configure connection via gateway.
 *
 * @param gateway       Gateway description.
 *
 * @return Status code.
 */
extern te_errno tapi_route_gateway_configure(tapi_route_gateway *gw);

/**
 * Enable or disable IPv4 or IPv6 forwarding on gateway.
 *
 * @param gateway       Gateway description.
 *
 * @return Status code.
 */
extern te_errno tapi_route_gateway_set_forwarding(tapi_route_gateway *gw,
                                              te_bool enabled);

/**
 * Break connection from gateway to IUT.
 *
 * @param gateway       Gateway description.
 *
 * @return Status code.
 */
extern te_errno tapi_route_gateway_break_gw_iut(tapi_route_gateway *gw);

/**
 * Repair connection from gateway to IUT.
 *
 * @param gateway       Gateway description.
 *
 * @return Status code.
 */
extern te_errno tapi_route_gateway_repair_gw_iut(tapi_route_gateway *gw);

/**
 * Break connection from gateway to Tester.
 *
 * @param gateway       Gateway description.
 *
 * @return Status code.
 */
extern te_errno tapi_route_gateway_break_gw_tst(tapi_route_gateway *gw);

/**
 * Repair connection from gateway to Tester.
 *
 * @param gateway       Gateway description.
 *
 * @return Status code.
 */
extern te_errno tapi_route_gateway_repair_gw_tst(tapi_route_gateway *gw);

/**
 * Break connection from IUT to gateway.
 *
 * @param gateway       Gateway description.
 *
 * @return Status code.
 */
extern te_errno tapi_route_gateway_break_iut_gw(tapi_route_gateway *gw);

/**
 * Repair connection from IUT to gateway.
 *
 * @param gateway       Gateway description.
 *
 * @return Status code.
 */
extern te_errno tapi_route_gateway_repair_iut_gw(tapi_route_gateway *gw);

/**
 * Break connection from Tester to gateway.
 *
 * @param gateway       Gateway description.
 *
 * @return Status code.
 */
extern te_errno tapi_route_gateway_break_tst_gw(tapi_route_gateway *gw);

/**
 * Repair connection from Tester to gateway.
 *
 * @param gateway       Gateway description.
 *
 * @return Status code.
 */
extern te_errno tapi_route_gateway_repair_tst_gw(tapi_route_gateway *gw);

/**
 * Down up all interfaces that were in gateway connection.
 *
 * @param gateway       Gateway description.
 *
 * @note Caller should take care about wait for the interfaces to be raised.
 *
 * @return Status code.
 */
extern te_errno tapi_route_gateway_down_up_ifaces(tapi_route_gateway *gw);

#endif /* !__TE_TAPI_ROUTE_GW_H__ */

/**@} <!-- END ts_tapi_route_gw --> */
