/** @file
 * @brief ACSE EPC API
 *
 * ACSE EPC declarations, common for both sites: ACSE process
 *      and head TA process.
 *
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * Copyright (C) 2004-2018 OKTET Labs. All rights reserved.
 *
 *
 *
 */

#ifndef __TE_LIB_ACSE_EPC_H__
#define __TE_LIB_ACSE_EPC_H__

#include "rcf_common.h"
#include "te_errno.h"
#include "tarpc.h"

#include "te_cwmp.h"
#include "te_defs.h"
#include "cwmp_soapStub.h"

#ifdef __cplusplus
extern "C" {
#endif


/** Magic number, start of EPC message codes, for validity check */
#define EPC_MSG_CODE_MAGIC 0x1985

/** Magic number, for EPC configuration commands, for validity check */
#define EPC_CONFIG_MAGIC 0x1977

/** Magic number, for EPC CWMP commands, for validity check */
#define EPC_CWMP_MAGIC 0x1950

/** Maximum length of config pipe name */
#define EPC_MAX_PATH 128

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
    /* TODO: level seems unnecessary here, it could be obtained by
            presense of CPE name. Think about it, may be remove it? */
    struct {
        unsigned         magic:16; /**< Should contain EPC_CONFIG_MAGIC */
        acse_cfg_level_t level:2;  /**< Level of config operation */
        acse_cfg_op_t    fun:4;    /**< Function to do */
    } op;               /**< Config operation code */

    te_errno     status;/**< Status of operation (in response).*/

    char         acs[RCF_MAX_NAME]; /**< Name of ACS object */
    char         cpe[RCF_MAX_NAME]; /**< Name of CPE record */

    char         oid[RCF_MAX_ID];   /**< TE Configurator OID of leaf,
                                         which is subject of operation */
    union {
        char     value[RCF_MAX_VAL];/**< operation result: single value */
        char     list[RCF_MAX_VAL]; /**< operation result: name list */
    };
} acse_epc_config_data_t;


/** CWMP operation code */
typedef enum {
    EPC_RPC_CALL = EPC_CWMP_MAGIC, /**< Call RPC on CPE.
                                If sync mode is ON and RPC code
                                is 'CWMP_RPC_NONE',
                                CWMP session will be terminated. */
    EPC_RPC_CHECK,        /**< Check status of sent RPC and get response or
                               get received from CPE the ACS RPC. */
    EPC_CONN_REQ,         /**< Send ConnectionRequest to CPE */
    EPC_CONN_REQ_CHECK,   /**< Check status of ConnectionRequest */
    EPC_GET_INFORM,       /**< Get Inform already received from CPE */
    EPC_HTTP_RESP,        /**< Send particular HTTP response to CPE.
                                HTTP code passed in to_cpe union,
                                string (if needed) passed in enc_start,
                                as usual zero-terminated character string.*/
} acse_epc_cwmp_op_t;


/**
 * Message with request/response for CWMP-related operation on ACSE.
 */
typedef struct {
    acse_epc_cwmp_op_t  op;     /**< Code of operation */

    te_errno    status;/**< Status of operation (in response).*/

    char        acs[RCF_MAX_NAME]; /**< Name of ACS object */
    char        cpe[RCF_MAX_NAME]; /**< Name of CPE record */

    te_cwmp_rpc_cpe_t   rpc_cpe; /**< Code of RPC call to be put
                                      into queue desired for CPE,
                                      or already sent to CPE. */

    te_cwmp_rpc_acs_t   rpc_acs; /**< Code of RPC catched from CPE.
                                    If is not CWMP_RPC_ACS_NONE and
                                    if op == EPC_RPC_CHECK,
                                    message is request to find
                                    received RPC from CPE.*/


    acse_request_id_t    request_id; /**< IN/OUT field for position
                                            in queue;
                                        IN - for 'get/check' operation,
                                        OUT - for 'call' operation. */

    cwmp_data_to_cpe_t   to_cpe;   /**< RPC specific CWMP data.
                                        This field is processed only
                                        in messages client->ACSE */

    cwmp_data_from_cpe_t from_cpe; /**< RPC specific CWMP data.
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

#if 0

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


    size_t    length;/**< Lenght of data, related to the message
                          and passed through shared memory.
                          Significant only in messages passing
                          via pipe between ACSE and its client.
                          Ignored as INPUT parameter in user API. */
    te_errno  status;/**< Status of operation (in response). */
    /* TODO: design simple and universal scheme for passing
        status of operation and internal errors! */

    union {
#if 1
        void *p;     /**< Syntax convenient pointer to user data */
#endif
        acse_epc_config_data_t *cfg; /**< Config user data */
        acse_epc_cwmp_data_t   *cwmp;/**< CWMP user data   */
    }         data;  /**< In user API is a pointer to operation specific
                          structure.  In messages really passing
                          via pipe it is not used. */
    uint8_t cfg_begin[0];
} acse_epc_msg_t;
#endif

/**
 * Role of EPC endpoint: Server (that is ACSE itself) or
 * Client (application, which ask ACSE to do something by CWMP,
 * that is TA of separate simple CLI tool).
 */
typedef enum {
    ACSE_EPC_SERVER,     /**< endpoint is ACSE */
    ACSE_EPC_CFG_CLIENT, /**< endpoint is user, i.d. TA or CLI tool */
    ACSE_EPC_OP_CLIENT,  /**< endpoint is user, i.d. TA or CLI tool */
} acse_epc_role_t;

typedef struct {
    acse_epc_role_t     role;
    int                 fd_in;
    int                 fd_out;
} epc_site_t;

/**
 * Methods to access EPC role flag, which should be implemented
 * by user of EPC, that is, it depends on the application design
 * which ACSE built in.
 */
extern void epc_role_set(acse_epc_role_t);
extern acse_epc_role_t epc_role_get(void);


static inline const char *
cwmp_epc_cfg_op_string(acse_cfg_op_t op)
{
    switch (op)
    {
        case EPC_CFG_ADD:       return "Add";
        case EPC_CFG_DEL:       return "Delete";
        case EPC_CFG_MODIFY:    return "Modify";
        case EPC_CFG_OBTAIN:    return "Obtain";
        case EPC_CFG_LIST:      return "List";
    }
    return "unknown";
}

static inline const char *
cwmp_epc_cwmp_op_string(acse_epc_cwmp_op_t op)
{
    switch (op)
    {
        case EPC_RPC_CALL:      return "RPC call";
        case EPC_RPC_CHECK:     return "RPC check";
        case EPC_CONN_REQ:      return "Conn.Req.";
        case EPC_CONN_REQ_CHECK:return "Conn.Req. check";
        case EPC_GET_INFORM:    return "Get Inform";
        case EPC_HTTP_RESP:     return "HTTP Response";
    }
    return "unknown";
}
/**
 * Init EPC server. This function may be called only once
 * in process life; it creates the unix socket with unique name,
 * based on the PID. This name should be passed to the client.
 *
 * Besides, it openes two internal pipes to inform about TR RPCs,
 * which would be placed into the "shared" memory, and allocates
 * this shared memory, placing pointer to global var @p epc_shmem.
 *
 * @param cfg_sock_name         Place for the name of unix
 *                              socket created, should have at least
 *                              EPC_MAX_PATH bytes. (OUT)
 * @param listen_sock           socket for listening EPC connection (OUT)
 *
 * @return status code
 */
extern te_errno acse_epc_init(char *cfg_sock_name, int *listen_sock);

/**
 * Open EPC Config connection from TA to ACSE.
 *
 * @param cfg_sock_name         Name of unix socket for messages
 *
 * @return status code
 */
extern te_errno acse_epc_connect(const char *cfg_sock_name);

/**
 * Close EPC Config connection from TA to ACSE.
 */
extern void acse_epc_close(void);

/**
 * Return EPC message socket fd for poll().
 * Do not read/write in it, use acse_epc_{send|recv}() instead.
 */
extern int acse_epc_socket(void);

/**
 * Check current EPC config pipe.
 *
 * @return TE_ENOTCONN if pipe is closed.
 */
extern te_errno acse_epc_check(void);

#if 0
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
 * @param message          Location for received message
 *
 * @return status code
 */
extern te_errno acse_epc_recv(acse_epc_msg_t *message);
#endif

/**
 * Send CFG message to other site in EPC connection.
 *
 * @param msg          Message to be sent
 *
 * @return status code
 */
extern te_errno acse_epc_conf_send(const acse_epc_config_data_t *msg);

/**
 * Receive CFG message from other site in EPC connection.
 *
 * @param msg         Location for received message
 *
 * @return status code
 */
extern te_errno acse_epc_conf_recv(acse_epc_config_data_t *msg);

/**
 * Send CWMP message to other site in EPC connection.
 * Field data must point to structure of proper type.
 *
 * @param user_message          Message to be sent
 *
 * @return status code
 */
extern te_errno acse_epc_cwmp_send(epc_site_t *s,
                                   const acse_epc_cwmp_data_t *msg);

/**
 * Receive CWMP message from other site in EPC connection.
 * Memory for data is allocated by malloc(), and should be free'd by
 * user. Allocated block have size, stored in field @p length
 * in the message, it may be used for boundary checks.
 *
 * @param message          Location for received message
 *
 * @return status code
 */
extern te_errno acse_epc_cwmp_recv(epc_site_t *s,
                                   acse_epc_cwmp_data_t **cwmp_data,
                                   size_t *d_len);

/**
 * Pack data for message client->ACSE.
 *
 * @param buf           Place for packed data (usually in
 *                      shared memory segment).
 * @param len           Length of memory area for packed data.
 * @param cwmp_data     User-provided struct with data to be sent.
 *
 * @return      -1 on error, 0 if no data presented,
 *              or length of used memory block in @p buf.
 */
extern ssize_t epc_pack_call_data(void *buf, size_t len,
                                  const acse_epc_cwmp_data_t *cwmp_data);

/**
 * Pack data for message ACSE->client.
 *
 * @param buf           Place for packed data (usually in
 *                      shared memory segment).
 * @param len           Length of memory area for packed data.
 * @param cwmp_data     User-provided struct with data to be sent.
 *
 * @return      -1 on error, 0 if no data presented,
 *              or length of used memory block in @p buf.
 */
extern ssize_t epc_pack_response_data(void *buf, size_t len,
                                  const acse_epc_cwmp_data_t *cwmp_data);


/*
 * Unpack data from message client->ACSE.
 *
 * @param buf           Place with packed data (usually in
 *                      local copy of transfered struct).
 * @param len           Length of packed data.
 * @param cwmp_data     Specific CWMP data with unpacked payload.
 *
 * @return status code
 */
extern te_errno epc_unpack_call_data(void *buf, size_t len,
                                     acse_epc_cwmp_data_t *cwmp_data);


/**
 * Unpack data from message ACSE->client.
 *
 * @param buf           Place with packed data (usually in
 *                      local copy of transfered struct).
 * @param len           Length of memory area with packed data.
 * @param cwmp_data     Specific CWMP data with unpacked payload.
 *
 * @return status code
 */
extern te_errno epc_unpack_response_data(void *buf, size_t len,
                                         acse_epc_cwmp_data_t *cwmp_data);



#ifdef __cplusplus
}
#endif

#endif /* __TE_LIB_ACSE_EPC_H__ */
