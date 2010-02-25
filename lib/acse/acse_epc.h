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

#include "rcf_common.h"
#include "te_errno.h" 
#include "tarpc.h"

#include "te_cwmp.h" 
#include "acse_soapStub.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LRPC_MMAP_AREA "/lrpc_mmap_area"
#define LRPC_ACSE_SOCK "/tmp/lrpc_acse_sock"
#define LRPC_TA_SOCK   "/tmp/lrpc_ta_sock"
#define LRPC_RPC_SOCK  "/tmp/lrpc_rpc_sock"


#if 1
/* This enum should correspond to xlat array in acse_epc_disp.c */
typedef enum { ACSE_FUN_FIRST = 0,
               ACSE_DB = ACSE_FUN_FIRST, /**< ACSE db operation: 
                                              add/delete ACS or CPE */
               ACSE_CONFIG,              /**< Modify some fields in 
                                              particular ACS or CPE */
               ACSE_CWMP,                /**< Perform CWMP operation */
               EPC_TEST_FUN,
               ACSE_FUN_LAST = EPC_TEST_FUN
} acse_fun_t;
#else
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
#endif

typedef enum {
    ACSE_PC_ACS_ADD,
    ACSE_PC_ACS_DEL,
    ACSE_PC_ACS_MODIFY,
    ACSE_PC_ACS_OBTAIN,
    ACSE_PC_ACS_LIST,
    ACSE_PC_CPE_ADD,
    ACSE_PC_CPE_DEL,
    ACSE_PC_CPE_MODIFY,
    ACSE_PC_CPE_OBTAIN,
    ACSE_PC_CPE_LIST,
    ACSE_PC_CWMP_CALL,
} acse_pc_t;


typedef struct {
    size_t       length;
    acse_pc_t    op;
    char         oid[RCF_MAX_ID]; /**< TE Configurator OID of leaf,
                                        which is subject of operation */ 
    char         acs[RCF_MAX_NAME];
    char         cpe[RCF_MAX_NAME];

    union {
        char     value[RCF_MAX_VAL];
        char     list[RCF_MAX_VAL];
    };

    te_cwmp_rpc_cpe_t      rpc_code;

    union {
        _cwmp__SetParameterValues       *set_parameter_values;
        _cwmp__GetParameterValues       *get_parameter_values;
        _cwmp__GetParameterNames        *get_parameter_names;
        _cwmp__SetParameterAttributes   *set_parameter_attributes;
        _cwmp__GetParameterAttributes   *get_parameter_attributes;
        _cwmp__AddObject                *add_object;
        _cwmp__DeleteObject             *delete_object;
        _cwmp__Reboot                   *reboot;
        _cwmp__Download                 *download;
        _cwmp__Upload                   *upload;
        _cwmp__ScheduleInform           *schedule_inform;
        _cwmp__SetVouchers              *set_vouchers;
        _cwmp__GetOptions               *get_options;
    } to_cpe;

    union {
        _cwmp__Inform                         *inform;
        _cwmp__GetRPCMethodsResponse          *get_rpc_methods_r;
        _cwmp__SetParameterValuesResponse     *set_parameter_values_r;
        _cwmp__GetParameterValuesResponse     *get_parameter_values_r;
        _cwmp__GetParameterNamesResponse      *get_parameter_names_r;
        _cwmp__GetParameterAttributes         *get_parameter_attributes_r;
        _cwmp__AddObjectResponse              *add_object_r;
        _cwmp__DeleteObjectResponse           *delete_object_r;
        _cwmp__DownloadResponse               *download_r;
        _cwmp__UploadResponse                 *upload_r;
        _cwmp__GetQueuedTransfersResponse     *get_queued_transfers_r;
        _cwmp__GetAllQueuedTransfersResponse  *get_all_queued_transfers_r;
        _cwmp__GetOptionsResponse             *get_options_r;
    } from_cpe;

    uint8_t data[0]; /* start of space after msg header, for packed data */

} acse_epc_msg_t;

typedef enum {
    ACSE_EPC_SERVER,
    ACSE_EPC_CLIENT
} acse_epc_role_t;

/**
 * Open EPC connection.
 *
 * @param msg_sock_name         Name of pipe socket for messages.
 *
 * @return status code
 */
extern te_errno acse_epc_open(const char *msg_sock_name,
                              const char *shmem_name,
                              acse_epc_role_t role);

/**
 * Close EPC connection.
 *
 */
extern te_errno acse_epc_close(void);

/**
 * Return EPC message socket fd for poll(). 
 * Do not read/write in it, use acse_epc_{send|recv}() instead.
 */
extern int acse_epc_sock(void);

/**
 * Return pointer to EPC data transfer buffer. 
 */
extern void* acse_epc_shmem(void);

extern te_errno acse_epc_send(const acse_epc_msg_t *params);

extern te_errno acse_epc_recv(acse_epc_msg_t **params);

#ifdef __cplusplus
}
#endif

#endif /* __TE_LIB_ACSE_H__ */
