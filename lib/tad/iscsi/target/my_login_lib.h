/**
 * This program is my software !!!!!!!!!!!
 *
 * Don't even read it, or you'll die in a week.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/select.h>

#include "../common/list.h"
#include "../common/iscsi_common.h"
#include "../common/debug.h"
#include "../common/range.h"
#include "../common/crc.h"
#include "../common/tcp_utilities.h"
#include "../security/misc/misc_func.h"
#include "../security/chap/chap.h"
#include "../security/srp/srp.h"

#include "../common/text_param.h"
#include "../common/target_negotiate.h"
#include "scsi_target.h"
#include "iscsi_portal_group.h"
#include "../common/default_param.c"

#define tprintf(args,...) \
    do {                        \
        printf(args);           \
        printf("\n");           \
    } while(0)

/** Pointer to the device specific data */
extern struct iscsi_global *devdata;

/** 
 * Structure of the Asynchronous Logout message.
 *
 * Fields description could be found in rfc3270.
 */
struct async_logout {
    uint8_t  some;
    uint8_t  resvd1[3];

    uint8_t  total_length;
    uint8_t  data_length[3];
    uint64_t resvd2;

    uint32_t fff;
    uint32_t recvd3;
    uint32_t stat_sn;
    uint32_t exp_cmd_sn;
    uint32_t max_cmd_sn;
    
    uint8_t  async_event;
    uint8_t  async_event_vcode;
    
    uint16_t par1;
    uint16_t par2;
    uint16_t par3;

    uint32_t recvd4;
};

/**
 * Function sends Asynchronous Logout to the Initiator.
 * It indicates, that the Target will drop all the connections
 * of the session.
 *
 * @param conn     Connection via which the request should be sent.
 */
extern int
send_async_logout(struct iscsi_conn *conn);

/**
 * Releases the connection and related structures.
 *
 * @param conn     Connection to be released.
 */
extern int
iscsi_release_connection(struct iscsi_conn *conn);

/**
 * Handles logout request and sends the logout response.
 * 
 * @param buf      The logout request.
 * @param conn     The connection via which the request was received and
 *                 via which the response should be sent.
 * @param session  The session which owns the connection.
 */
extern int
handle_logout_rsp(uint8_t *buf, struct iscsi_conn *conn, 
                  struct iscsi_session *session);

/**
 * Handles the Login Request from the Initiator. Security parameters 
 * negotiation and operational parameters negotiation is made inside
 * this function.
 *
 * @param conn     The connection via which the Login Request was received.
 * @param buffer   The request BHS (ussually 48 bytes). The rest of the
 *                 message is still unread at the entry point of the
 *                 function.
 */
extern int
handle_login(struct iscsi_conn *conn, uint8_t *buffer);

/**
 * Creates the session and connection structures when the TCP connection is
 * established.
 *
 * @param sock     The socket returned by the accept() function
 * @param ptr      Portal group structure pointer.
 */
extern struct iscsi_conn *
build_conn_sess(int sock, struct portal_group *ptr);
