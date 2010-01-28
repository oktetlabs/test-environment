/** @file
 * @brief ACSE API
 *
 * ACSE declarations, common for both sites: ACSE process
 *      and head TA process.
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
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_LIB_ACSE_H__
#define __TE_LIB_ACSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define LRPC_MMAP_AREA "/lrpc_mmap_area"
#define LRPC_ACSE_SOCK "/tmp/lrpc_acse_sock"
#define LRPC_TA_SOCK   "/tmp/lrpc_ta_sock"
#define LRPC_RPC_SOCK  "/tmp/lrpc_rpc_sock"

#include "rcf_common.h"
#include "tarpc.h"

/* This enum should correspond to xlat array in acse_lrpc.c */
typedef enum { acse_fun_first = 1,
               acse_acs_add_fun = acse_fun_first,
               acse_acs_del_fun, acse_acs_list_fun,
               acs_url_get_fun, acs_url_set_fun,
               acs_cert_get_fun, acs_cert_set_fun,
               acs_user_get_fun, acs_user_set_fun,
               acs_pass_get_fun, acs_pass_set_fun,
               acs_enabled_get_fun, acs_enabled_set_fun,
               acs_ssl_get_fun, acs_ssl_set_fun,
               acs_port_get_fun, acs_port_set_fun,
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
               session_state_get_fun,
               session_target_state_get_fun, session_target_state_set_fun,
               session_enabled_get_fun, session_enabled_set_fun,
               session_hold_requests_get_fun, session_hold_requests_set_fun,
               fun_cpe_get_rpc_methods,
               fun_cpe_set_parameter_values,
               fun_cpe_get_parameter_values,
               fun_cpe_get_parameter_names,
               fun_cpe_set_parameter_attributes,
               fun_cpe_get_parameter_attributes,
               fun_cpe_add_object,
               fun_cpe_delete_object,
               fun_cpe_reboot,
               fun_cpe_download,
               fun_cpe_upload,
               fun_cpe_factory_reset,
               fun_cpe_get_queued_transfers,
               fun_cpe_get_all_queued_transfers,
               fun_cpe_schedule_inform,
               fun_cpe_set_vouchers,
               fun_cpe_get_options,
               rpc_test_fun,
               acse_fun_last = rpc_test_fun
} acse_fun_t;

typedef struct {
    unsigned int acse;
    unsigned int gid;
    char         oid[RCF_MAX_ID];

    union {
        char     value[RCF_MAX_VAL];
        char     list[RCF_MAX_VAL];
    };

    char         acs[RCF_MAX_NAME];
    char         cpe[RCF_MAX_NAME];

    union {
        /** GetRPCMethods output */
        struct {
            tarpc_string64_t list[32];
            tarpc_uint       len;
        } method_list;
    };
} params_t;

/**
 * Check fd for acceptance by select call
 *
 * @param fd            File descriptor
 *
 * @return              0 if OK, otherwise -1
 */
extern int check_fd(int fd);

/**
 * ACSE main loop
 *
 * @param rfd           Local RPC read endpoint
 * @param wfd           Local RPC write endpoint
 */
extern void acse_loop(params_t *params, int sock);

#ifdef __cplusplus
}
#endif

#endif /* __TE_LIB_ACSE_H__ */
