/** @file
 * @brief Test API to configure PPPoE.
 *
 * Definition of API to configure PPPoE.
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights served.
 *
 * 
 *
 *
 * @author Igor Labutin <Igor.Labutin@oktetlabs.ru>
 *
 * $Id: tapi_cfg_dhcp.h 32571 2006-10-17 13:43:12Z edward $
 */

#ifndef __TE_TAPI_CFG_PPPOE_H__
#define __TE_TAPI_CFG_PPPOE_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "conf_api.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tapi_conf_pppoe PPPoE Server configuration
 * @ingroup tapi_conf
 * @{
 */

/**
 * Add interface to PPPoE server configuration on the Test Agent.
 *
 * @param ta            Test Agent
 * @param ifname        Interface to listen on
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_pppoe_server_if_add(const char *ta, const char *ifname);

/**
 * Delete interface from PPPoE server configuration on the Test Agent.
 *
 * @param ta            Test Agent
 * @param ifname        Interface to remove
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_pppoe_server_if_del(const char *ta, const char *ifname);

/**
 * Configure subnet of PPPoE server to allocate local and remote addresses.
 *
 * @param ta            Test Agent
 * @param subnet        String value of subnet XXX.XXX.XXX.XXX/prefix
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_pppoe_server_subnet_set(const char *ta, const char *subnet);

/**
 * Get subnet of PPPoE server to allocate local and remote addresses.
 *
 * @param ta            Test Agent
 * @param subnet_p      Returned pointer to subnet value
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_pppoe_server_subnet_get(const char *ta, const char **subnet_p);

/**
 * Get local IP address of PPPoE server.
 *
 * @param[in]  ta           Test agent name.
 * @param[out] addr         Local IP address. Note, it should be freed with
 *                          free(3) when it is no longer needed.
 */
extern void tapi_cfg_pppoe_server_laddr_get(const char *ta,
                                            struct sockaddr **addr);

/**
 * Get starting remote IP address of PPPoE server.
 *
 * @param[in]  ta           Test agent name.
 * @param[out] addr         Starting remote IP address. Note, it should be freed
 *                          with free(3) when it is no longer needed.
 */
extern void tapi_cfg_pppoe_server_raddr_get(const char *ta,
                                            struct sockaddr **addr);

/**@} <!-- END tapi_conf_pppoe --> */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_TAPI_CFG_PPPOE_H__ */
