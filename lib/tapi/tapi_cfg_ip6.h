/** @file
 * @brief Test API to access configuration model
 *
 * Definition of API to access IPv6-related objects.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 *
 * @author Andrew Rybchenko <Andrew.Rybchenko@oktetlabs.ru>
 *
 */

#ifndef __TE_TAPI_CFG_IP6_H_
#define __TE_TAPI_CFG_IP6_H_

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "te_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_ip6 IPv6 specific configuration
 * @ingroup tapi_conf
 * @{
 */

/* IPv6 address length */
#define IPV6_ADDR_LEN    sizeof(struct in6_addr)

/**
 * Get link-local address of the interface.
 *
 * @param ta        Test Agent name
 * @parma iface     Interface name
 * @param p_addr    Location for pointer to sockaddr with IPv6
 *                  link-local address
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_ip6_get_linklocal_addr(const char *ta,
                                                const char *iface,
                                                struct sockaddr_in6
                                                    *p_addr);

/**
 * Get multicast all link-local address of the interface.
 *
 * @param ta        Test Agent name
 * @parma iface     Interface name
 * @param p_addr    Location for pointer to sockaddr with IPv6
 *                  multicast address
 *
 * @return Status code.
 */
extern te_errno tapi_cfg_ip6_get_mcastall_addr(const char *ta,
                                               const char *iface,
                                               struct sockaddr_in6 *p_addr);

/**@} <!-- END tapi_conf_ip6 --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_IP6_H_ */
