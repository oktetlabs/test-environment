/** @file
 * @brief ACSE EPC API
 *
 * ACSE EPC declarations, common for both sites: ACSE process
 *      and head TA process.
 *
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS
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
 * $Id$
 */

#ifndef __TE_LIB_ACSE_EPC_H__
#define __TE_LIB_ACSE_EPC_H__

#include "rcf_common.h"
#include "te_errno.h"
#include "tarpc.h"

#include "te_cwmp.h" 
#include "te_defs.h" 
#include "acse_soapStub.h"

#ifdef __cplusplus
extern "C" {
#endif


/** Magic number, start of EPC message codes, for validity check */
#define EPC_MSG_CODE_MAGIC 0x1985

/** Magic number, for EPC configuration commands, for validity check */
#define EPC_CONFIG_MAGIC 0x1977

/** Magic number, for EPC CWMP commands, for validity check */
#define EPC_CWMP_MAGIC 0x1950


/**
 * Level of EPC configuration command: either ACS or CPE.
 */
typedef enum {
    EPC_CFG_ACS = 1, /**< ACS object configuring level */
    EPC_CFG_CPE      /**< CPE record configuring level */
} acse_cfg_level_t;

/**
 * Type of EPC configuration command.
 */
typedef enum {
    EPC_CFG_ADD,        /**< Add record on ACSE: either ACS or CPE */
    EPC_CFG_DEL,        /**< Remove record on ACSE: either ACS or CPE */
    EPC_CFG_MODIFY,     /**< Modify some parameter in record */
    EPC_CFG_OBTAIN,     /**< Obtain value of some parameter */
    EPC_CFG_LIST,       /**< Get list of objects on specified level */
} acse_cfg_op_t;

/**
 * Struct for data, related to config EPC from/to ACSE.
 * This struct should be stored in shared memory. 
 */
typedef struct {
    struct {
        unsigned         magic:16; /**< Should contain EPC_CONFIG_MAGIC */
        acse_cfg_level_t level:2;  /**< Level of config operation */
        acse_cfg_op_t    fun:4;    /**< Function to do */
    } op;               /**< Config operation code */

    char         acs[RCF_MAX_NAME]; /**< Name of ACS object */
    char         cpe[RCF_MAX_NAME]; /**< Name of CPE record */

    char         oid[RCF_MAX_ID]; /**< TE Configurator OID of leaf,
                                        which is subject of operation */ 
    union {
        char     value[RCF_MAX_VAL]; /**< operation result: single value */
        char     list[RCF_MAX_VAL];  /**< operation result: name list */
    };
} acse_epc_config_data_t;


/** CWMP operation code */
typedef enum {
    EPC_RPC_CALL = EPC_CWMP_MAGIC, /**< Call RPC on CPE */
    EPC_RPC_CHECK,        /**< Check status of sent RPC */
    EPC_CONN_REQ,         /**< Send ConnectionRequest to CPE */
    EPC_CONN_REQ_CHECK,   /**< Check status of ConnectionRequest */
    EPC_GET_INFORM,       /**< Get Inform already received from CPE */
} acse_epc_cwmp_op_t;


/**
 * Message with request/response for CWMP-related operation on ACSE.
 */
typedef struct {
    acse_epc_cwmp_op_t  op;     /**< Code of operation */

    char        acs[RCF_MAX_NAME]; /**< Name of ACS object */
    char        cpe[RCF_MAX_NAME]; /**< Name of CPE record */

    te_cwmp_rpc_cpe_t   rpc_cpe; /**< Code of RPC call to be put 
                                      into queue desired for CPE.*/

    int         index; /**< IN/OUT field for position in queue;
                            IN - for 'get/check' operation, 
                            OUT - for 'call' operation. */

    te_bool     hold_requests; /**< Flag denotes whether this call
                                    should be sent before serve 
                                    RPC from CPE. */

    union {
        void *p;
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
    } to_cpe; /**< Typed pointer to call-specific CWMP data. 
                   This field is processed only
                   in messages client->ACSE */

    union {
        acse_cr_state_t      cr_state;
        void *p;
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

    } from_cpe; /**< Typed pointer to call-specific CWMP data. 
                   This field is processed only
                   in messages ACSE->client */

    uint8_t enc_start[0]; /**< Start of space after msg header,
                               for packed data */

} acse_epc_cwmp_data_t;




/** Code of EPC message */
typedef enum {
    EPC_CONFIG_CALL = EPC_MSG_CODE_MAGIC, /**< Config, user->ACSE */
    EPC_CONFIG_RESPONSE, /**< Config response ACSE->user */
    EPC_CWMP_CALL,       /**< CWMP operation call */
    EPC_CWMP_RESPONSE,   /**< CWMP operation response */
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
    acse_msg_code_t opcode; /**< Code of operation */

    union {
        void *p;        /**< Useful pointer to data */

        acse_epc_config_data_t *cfg; /**< Config user data */
        acse_epc_cwmp_data_t   *cwmp;/**< CWMP user data   */
    }       data;    /**< In user APІ is pointer to operation specific
                          structure.  In messages really passing 
                          via pipe it is not used. */

    size_t   length; /**< Lenght of data, related to the message.
                          Significant only in messages passing 
                          via pipe between ACSE and its client.
                          Ignored as INPUT parameter in user API. */
    te_errno status; /**< Significant only in response */
    /* TODO: design simple and universal scheme for passing 
        status of operation and internal errors! */
} acse_epc_msg_t;

/**
 * Role of EPC endpoint: Server (that is ACSE itself) or 
 * Client (application, which ask ACSE to do something by CWMP, 
 * that is TA of separate simple CLI tool).
 */
typedef enum {
    ACSE_EPC_SERVER, /**< endpoint is ACSE */
    ACSE_EPC_CLIENT  /**< endpoint is user, i.d. TA or CLI tool */
} acse_epc_role_t;

/**
 * Open EPC connection. This function may be called only once
 * in process life. 
 * For SERVER, this function blocks until EPC pipe will be 
 * established, waiting for Client connection.
 *
 * @param msg_sock_name         Name of pipe socket for messages 
 *                                  or NULL for internal default.
 * @param shmem_name            Name of shared memory block, must
 *                              be same in related Server and Client.
 *                              May be NULL for internal default.
 * @param role                  EPC role, which current application plays.
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
 *
 * @param user_message          Message to be sent
 *
 * @return status code
 */
extern te_errno acse_epc_send(const acse_epc_msg_t *user_message);

/**
 * Receive message from other site in EPC connection. 
 * Field @p data will point to structure of proper type.
 * Memory for data is allocated by malloc(), and should be free'd by 
 * user. Allocated block have size, stored in field @p length 
 * in the message, it may be used for boundary checks.
 *
 * @param user_message          Location for ptr to received message
 *
 * @return status code 
 */
extern te_errno acse_epc_recv(acse_epc_msg_t **user_message);

#ifdef __cplusplus
}
#endif

#endif /* __TE_LIB_ACSE_EPC_H__ */
