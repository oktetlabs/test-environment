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

#define EPC_MMAP_AREA "/lrpc_mmap_area"
#define EPC_ACSE_SOCK "/tmp/lrpc_acse_sock"
#define EPC_TA_SOCK   "/tmp/lrpc_ta_sock"
#define EPC_RPC_SOCK  "/tmp/lrpc_rpc_sock"


#if 0
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
#endif

#define EPC_MSG_CODE_MAGIC 0x1985
#define EPC_CONFIG_MAGIC 0x1977
#define EPC_CWMP_MAGIC 0x1950


typedef enum {
    EPC_CONFIG_CALL = EPC_MSG_CODE_MAGIC,
    EPC_CONFIG_RESPONSE,
    EPC_CWMP_CALL,
    EPC_CWMP_RESPONSE,
} acse_msg_code_t;


/**
 * Struct for message exchange between ACSE and its client via AF_UNIX
 * pipe. 
 * All large data passed through shared memory, but not via this connection.
 * ACSE must answer to request as soon as possible. 
 * ACSE may read/write from/to shared memory only between receive this 
 * request and answer response. 
 * All operations with shared memory is hidden behind API, defined in this
 * file (and implemented in acse_epc.c and cwmp_data.c).
 */
typedef struct {
    acse_msg_code_t  opcode; /**< Code of operation */
    void            *data;   /**< In user APІ is pointer to:
                                either @acse_epc_config_data_t,
                                or @acse_epc_cwmp_data_t.
                                In messages really passing via pipe 
                                is not used. */
    size_t           length; /**< Lenght of data, related to the message.
                                Ignored as INPUT parameter in user API. */
} acse_epc_msg_t;

typedef enum {
    EPC_CFG_ACS = 1,
    EPC_CFG_CPE
} acse_cfg_level_t;

typedef enum {
    EPC_CFG_ADD,
    EPC_CFG_DEL,
    EPC_CFG_MODIFY,
    EPC_CFG_OBTAIN,
    EPC_CFG_LIST,
} acse_cfg_op_t;

/**
 * Struct for data, related to config EPC from/to ACSE.
 * This struct should be stored in shared memory. 
 */
typedef struct {
    struct {
        unsigned         magic:16; /**< Should store EPC_CONFIG_MAGIC */
        acse_cfg_level_t level:2;
        acse_cfg_op_t    fun:4;
    } op;

    char         oid[RCF_MAX_ID]; /**< TE Configurator OID of leaf,
                                        which is subject of operation */ 
    char         acs[RCF_MAX_NAME];
    char         cpe[RCF_MAX_NAME];

    union {
        char     value[RCF_MAX_VAL];
        char     list[RCF_MAX_VAL];
    };
} acse_epc_config_data_t;


typedef enum {
    EPC_RPC_CALL = EPC_CWMP_MAGIC,
    EPC_RPC_CHECK,
    EPC_CONN_REQ,
    EPC_CONN_REQ_CHECK,
    EPC_GET_INFORM,
} acse_epc_cwmp_op_t;

typedef struct {
    acse_epc_cwmp_op_t  op;

    char        acs[RCF_MAX_NAME];
    char        cpe[RCF_MAX_NAME];

    te_cwmp_rpc_cpe_t   rpc_cpe; /**< Code of RPC call to be put 
                                      into queue desired for CPE.*/

    int         index; /**< IN/OUT field for position in queue */

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

} acse_epc_cwmp_data_t;

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
 * Send message to other site in EPC connection. 
 * Field data must point to structure of proper type.
 */
extern te_errno acse_epc_send(const acse_epc_msg_t *user_message);

/**
 * Receive message from other site in EPC connection. 
 * Field @p data will point to structure of proper type.
 * Memory for data is allocated by malloc(), and should be free'd by 
 * user. Allocated block have size, stored in field @p length 
 * in the message, it may be used for boundary checks.
 */
extern te_errno acse_epc_recv(acse_epc_msg_t **user_message);

#ifdef __cplusplus
}
#endif

#endif /* __TE_LIB_ACSE_H__ */
