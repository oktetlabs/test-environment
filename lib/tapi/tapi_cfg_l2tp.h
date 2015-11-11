/** @file
 * @brief Test API to configure L2TP.
 *
 * Definition of TAPI to configure L2TP.
 *
 *
 * Copyright (C) 2015 Test Environment authors (see file AUTHORS
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
 * @author Albert Podusenko <albert.podusenko@oktetlabs.ru>
 *
 */

#ifndef __TE_TAPI_CFG_L2TP_H__
#define __TE_TAPI_CFG_L2TP_H__

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "conf_api.h"


/**
 * Add ip range to L2TP server configuration on the Test Agent.
 *
 * @param ta            Test Agent
 * @param iprange       IP range from where server appoints 
                        an IP address to the client
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_l2tp_server_range_add(const char *ta, const char *iprange);

/**
 * Delete iprange from L2TP server configuration on the Test Agent.
 *
 * @param ta            Test Agent
 * @param iprange       IP range to remove
 *
 * @return Status code
 */
extern te_errno
tapi_cfg_l2tp_server_range_del(const char *ta, const char *iprange);

/**
 * Set a local ip of L2TP server.
 *
 * @param ta            Test Agent
 * @param local         New IP adress
 *
 * @return Status code
 */
 extern te_errno
 tapi_cfg_l2tp_server_local_set(const char *ta, const char *local);

/**
 * Get a current local ip of L2TP server.
 *
 * @param ta            Test Agent
 * @param local         Returned pointer to the current local ip
 *
 * @return Status code
 */
 extern te_errno
 tapi_cfg_l2tp_server_local_get(const char *ta, const char **local);

/**
 * Set an option of L2TP server.
 *
 * @param ta            Test Agent
 * @param kind_opt      Which file's option must be changed(pppd/xl2tpd)
 * @param px_option     New option
 *
 * @return Status code
 */
 extern te_errno
 tapi_cfg_l2tp_server_option_set(const char *ta, 
                                 const kind_opt, const char *px_option);

/**
 * Get an option of L2TP server.
 *
 * @param ta            Test Agent
 * @param kind_opt      Which file's option must be get(pppd/xl2tpd)
 * @param px_option     Returned pointer to the current option
 *
 * @return Status code
 */
 extern te_errno
 tapi_cfg_l2tp_server_option_get(const char *ta, 
                                 const kind_opt, const char **px_option);

/**
 * Add a new option to pppd.
 *
 * @param ta            Test Agent
 * @param p_option      Option to add
 *
 * @return Status code
 */
 extern te_errno
 tapi_cfg_l2tp_server_option_add(const char *ta, const char *p_option);

/**
 * Delete an option from pppd.
 *
 * @param ta            Test Agent
 * @param p_option      Option to delete
 *
 * @return Status code
 */
 extern te_errno
 tapi_cfg_l2tp_server_option_del(const char *ta, const char *p_option);

#endif /* !__TE_TAPI_CFG_L2TP_H__ */
