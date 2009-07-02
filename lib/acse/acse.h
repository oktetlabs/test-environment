/** @file
 * @brief ACSE API
 *
 * ACSE declarations.
 *
 *
 * Copyright (C) 2004-2006 Test Environment authors (see file AUTHORS
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
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 *
 * $Id: acse.h 31892 2006-09-26 07:12:12Z arybchik $
 */

#ifndef __TE_LIB_ACSE_H__
#define __TE_LIB_ACSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rcf_common.h"

/* This enum should correspond to xlat array in acse_lrpc.c */
typedef enum { acse_fun_first = 1,
               acse_acs_add_fun = acse_fun_first,
               acse_acs_del_fun, acse_acs_list_fun,
               acs_url_get_fun, acs_url_set_fun,
               acs_cert_get_fun, acs_cert_set_fun,
               acs_user_get_fun, acs_user_set_fun,
               acs_pass_get_fun, acs_pass_set_fun,
               acs_cpe_add_fun, acs_cpe_del_fun, acs_cpe_list_fun,
               cpe_ip_addr_get_fun, cpe_ip_addr_set_fun,
               cpe_url_get_fun, cpe_url_set_fun,
               cpe_cert_get_fun, cpe_cert_set_fun,
               cpe_user_get_fun, cpe_user_set_fun,
               cpe_pass_get_fun, cpe_pass_set_fun,
               device_id_manufacturer_get_fun,
               device_id_oui_get_fun,
               device_id_product_class_get_fun,
               device_id_serial_number_get_fun,
               acse_session_list_fun,
               session_link_get_fun, session_state_get_fun,
               session_target_state_get_fun, session_target_state_set_fun,
               session_enabled_get_fun, session_enabled_set_fun,
               session_hold_requests_get_fun, session_hold_requests_set_fun,
               acse_fun_last = session_hold_requests_set_fun
} acse_fun_t;

typedef struct {
    unsigned int gid;
    char         oid[RCF_MAX_ID];

    union {
        char     value[RCF_MAX_VAL];
        char     list[RCF_MAX_VAL];
    };

    union {
        char     acs[RCF_MAX_NAME];
        char     session[RCF_MAX_NAME];
    };

    char         cpe[RCF_MAX_NAME];
} params_t;

/**
 * ACSE main loop
 *
 * @param rfd           Local RPC read endpoint
 * @param wfd           Local RPC write endpoint
 */
extern void acse_loop(params_t *params, int rd_fd, int wr_fd);

#ifdef __cplusplus
}
#endif

#endif /* __TE_LIB_ACSE_H__ */
