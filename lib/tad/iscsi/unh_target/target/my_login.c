/** @file
 * @brief Test Environment: 
 *
 * iSCSI target emulator (based on the UNH kernel iSCSI target)
 * 
 * Copyright (C) 2001-2004 InterOperability Lab (IOL)
 *                         University of New Hampshier (UNH)
 *                         Durham, NH 03824
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * The name IOL and/or UNH may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * Author: Konstantin Ushakov <Konstantin.Ushakov@oktetlabs.ru>
 * Author: Artem Andreev      <Artem.Andreev@oktetlabs.ru>
 *
 * $Id$
 */

#include <te_config.h>
#include <te_defs.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <pthread.h>

#include "../common/list.h"
#include <iscsi_common.h>
#include "../common/debug.h"
#include <range.h>
#include <crc.h>
#include <tcp_utilities.h>
#include <misc_func.h>
#include <chap.h>
#include <srp.h>

#include <text_param.h>
#include <target_negotiate.h>
#include "scsi_target.h"
#include "iscsi_target.h"
#include "iscsi_portal_group.h"

#include <my_memory.h>

#include "my_login.h"

/** Pointer to the device specific data */
struct iscsi_global *devdata;

static int                                                                        
iscsi_release_connection(struct iscsi_conn *conn)                                 
{
    if (conn == NULL) { 
        return -1;                                                                
    }                                                                             
    /* Release socket */                                                          
    conn->conn_socket = -1;                                                       
    TRACE(TRACE_ISCSI_FULL, "Dequeue connection conn->cid %u\n", conn->conn_id);  
#if 0                                                                             
    /* Dequeue connection */                                                      
    /* __ prefix should be used */                                                
    list_del(&conn->conn_link);                                                   
    conn->session->nconn--;                                                       
    /* Free connection */                                                         
    my_free((void **)&conn->local_ip_address);                                    
    my_free((void **)&conn->ip_address);                                          
#endif                                                                            
    my_free(((void *)&conn));
    return 0;                                                                     
}

static void
free_data_list(struct iscsi_cmnd *cmnd)
{
    struct data_list *data;

    while ((data = cmnd->unsolicited_data_head)) {
        cmnd->unsolicited_data_head = data->next;
        my_free((void **)&data->buffer);
        my_free((void **)&data);
    }
}


/*
 * iscsi_release_session: This function is responsible for closing out
 * a session and removing it from whatever list it is on.
 * host->session_sem MUST be locked before this routine is called.
 * INPUT: session to be released
 * OUTPUT: 0 if success, < 0 if there is trouble
 */
int
iscsi_release_session(struct iscsi_session *session)
{
    struct iscsi_cmnd *cmnd;
    struct iscsi_conn *conn;
    struct list_head *list_ptr, *list_temp;

    if (!session) {
        TRACE_ERROR("Cannot release a NULL session\n");
        return -1;
    }

    if (TRACE_TEST(TRACE_ISCSI)) {
        print_isid_tsih_message(session, "Release session with ");
    }

    TRACE(TRACE_DEBUG, "Releasing R2T timer %p for session %p\n",
          session->r2t_timer, session);

    /* Delete r2t timer - SAI */
    if (session->r2t_timer) {
#if 0
        TRACE(TRACE_DEBUG, "Deleting r2t timer %p\n", session->r2t_timer);
        del_timer_sync(session->r2t_timer);
        TRACE(TRACE_DEBUG, "Deleted r2t timer\n");
        my_free((void**)&session->r2t_timer);
#endif
    }

    /* free commands */
    while ((cmnd = session->cmnd_list)) {
        session->cmnd_list = cmnd->next;

        if (cmnd->cmnd != NULL) {
            if (scsi_release(cmnd->cmnd) < 0) {
                TRACE_ERROR("Trouble releasing command, opcode 0x%02x, "
                            "ITT %u, state 0x%x\n", 
                            cmnd->opcode_byte, cmnd->init_task_tag,
                            cmnd->state);
            }
        }
        /* free data_list if any, cdeng */
        free_data_list(cmnd);
        my_free((void**)&cmnd->ping_data);
        my_free((void**)&cmnd);
    }

    /* free connections */
    list_for_each_safe(list_ptr, list_temp, &session->conn_list) {
        conn = list_entry(list_ptr, struct iscsi_conn, conn_link);
        TRACE(TRACE_ISCSI, "iscsi%d: releasing connection %d\n", (int)
              session->devdata->device->id, conn->conn_id);

        if (iscsi_release_connection(conn) < 0) {
            TRACE_ERROR("Trouble releasing connection\n");
        }
    }

    /* dequeue session if it is linked into some list */
    if (!list_empty(&session->sess_link)) {
        list_del(&session->sess_link);

        /* error recovery ver new 18_04 - SAI */
        if (session->retran_thread) {
#if 0
            /*  Mike Christie mikenc@us.ibm.com */
            send_sig(ISCSI_SHUTDOWN_SIGNAL, session->retran_thread, 1);
            sem_wait(&session->thr_kill_sem);
#endif
        }
    }

    /* free session structures */
    my_free((void**)&session->session_params);

    my_free((void**)&session->oper_param);

    my_free((void**)&session);

    return 0;
}

/*
 * executed only by the tx thread.
 * iscsi_tx_data: will transmit a fixed-size PDU of any type.
 * INPUT: struct iscsi_conn (what connection), struct iovec, number of iovecs,
 * total amount of data in bytes,
 * iovec[0] must be the 48-byte PDU header.
 * iovec[1] (if header digests are in use, calculate it in this function)
 * iovec[...] the data
 * iovec[niov-1] (if data digests are in use, calculate it in this function)
 * OUTPUT: int total bytes written if everything is okay
 *         < 0 if trouble
 */
int
iscsi_tx_data(struct iscsi_conn *conn, struct iovec *iov, int niov, int data)
{

# ifdef DEBUG_DATA
    int i, j;
    struct iovec *debug_iov;
    uint8_t *to_print;
# endif

    int total_tx, tx_loop;
    uint32_t hdr_crc, data_crc;
    int data_len, k, i;
    struct iovec *ioptr;
    uint8_t *buffer;
    struct generic_pdu *pdu;

    if (!conn->conn_socket) {
        TRACE_ERROR("NULL conn_socket\n");
        return -1;
    }
# ifdef DEBUG_DATA
    TRACE(TRACE_DEBUG, "iscsi_tx_data: iovlen %d\n", niov);

    debug_iov = iov;

    for (i = 0; i < niov; i++) {
        to_print = (uint8_t *) debug_iov->iov_base;

        for (j = 0; ((j < debug_iov->iov_len) && (j < 64)); j++) {
            TRACE(TRACE_DEBUG, "%.2x ", to_print[j]);

            if (((j + 1) % 16) == 0)
                TRACE(TRACE_DEBUG, "\n");
            else if (((j + 1) % 4) == 0)
                TRACE(TRACE_DEBUG, "    ");
        }

        TRACE(TRACE_DEBUG, "\n");

        debug_iov++;
    }
# endif

    /* compute optional header digest */
    if (conn->hdr_crc) {
        hdr_crc = 0;
        do_crc(iov[0].iov_base, ISCSI_HDR_LEN, &hdr_crc);

        iov[1].iov_base = &hdr_crc;
        iov[1].iov_len = CRC_LEN;
        TRACE(TRACE_ISCSI_FULL, "Send header crc 0x%08x\n", ntohl(hdr_crc));
    }

    /* compute optional data digest */
    if (conn->data_crc && niov > conn->hdr_crc + 2) {
        data_len = 0;
        data_crc = 0;
        for (k = conn->hdr_crc + 1; k < niov - 1; k++) {
            do_crc(iov[k].iov_base, iov[k].iov_len, &data_crc);
            data_len += iov[k].iov_len;
        }

        iov[niov - 1].iov_base = &data_crc;
        iov[niov - 1].iov_len = CRC_LEN;
        TRACE(TRACE_ISCSI_FULL, "Send data len %d, data crc 0x%08x\n",
              data_len, ntohl(data_crc));
    }


    buffer = malloc(data);
    if (buffer == NULL)
        return -1;

    total_tx = 0;
    while (total_tx < data) {
        TRACE(TRACE_DEBUG, "iscsi_tx_data: niov %d, data %d, total_tx %d\n",
              niov, data, total_tx);

        tx_loop = iscsi_tad_send(conn->conn_socket, buffer, (data - total_tx));

        if (tx_loop <= 0) {
            pdu = (struct generic_pdu *)iov[0].iov_base;
            TRACE_ERROR("sock_sendmsg error %d, total_tx %d, data %d, niov "
                        "%d, op 0x%02x, flags 0x%02x, ITT %u\n",
                        tx_loop, total_tx, data, niov,
                        pdu->opcode, pdu->flags,
                        ntohl(pdu->init_task_tag));
            free(buffer);
            return tx_loop;
        }

        total_tx += tx_loop;
        TRACE(TRACE_DEBUG, "iscsi_tx_data: tx_loop %d total_tx %d\n", tx_loop,
              total_tx);
    }

    return total_tx;
}

/*
 * executed only by the tx thread.
 * RDR
 */
static int
send_hdr_plus_1_data(struct iscsi_conn *conn, void *iscsi_hdr,
                     void *data_buf, int data_len)
{
    struct iovec iov[5];
    int niov, total_size, padding, retval;
    int pad_bytes = 0;
    
    /* set up the header in the first iov slot */
    iov[0].iov_base = iscsi_hdr;
    iov[0].iov_len = ISCSI_HDR_LEN;
    total_size = ISCSI_HDR_LEN;
    niov = 1;                   /* one for the header */

    if (conn->hdr_crc) {
        /* set up the header digest in the second iov slot */
        iov[niov].iov_len = CRC_LEN;
        total_size += CRC_LEN;
        niov++;                 /* one for header digest */
    }

    if (data_len) {
        /* set up the data in the next iov slot */
        iov[niov].iov_base = data_buf;
        iov[niov].iov_len = data_len;
        total_size += data_len;
        niov++;                     /* one for data */

        padding = (-data_len) & 3;
        if (padding) {
            /* set up the data padding in the next iov slot */
            iov[niov].iov_base = &pad_bytes;
            iov[niov].iov_len = padding;
            total_size += padding;
            niov++;                 /* one for padding */
            TRACE(TRACE_DEBUG, "padding attached: %d bytes\n", padding);
        }

        if (conn->data_crc) {
            /* set up the data digest in the next iov slot */
            iov[niov].iov_len = CRC_LEN;
            total_size += CRC_LEN;
            niov++;                 /* one for data digest */
        }
    }

    retval = iscsi_tx_data(conn, iov, niov, total_size);

    if (retval != total_size) {
        TRACE_ERROR("Trouble in iscsi_tx_data, expected %d bytes, got %d\n",
                    total_size, retval);
        retval = -1;
    }

    return retval;
}

/*
 * executed only by the tx thread.
 * RDR
 */
static inline int
send_hdr_only(struct iscsi_conn *conn, void *iscsi_hdr)
{
    return send_hdr_plus_1_data(conn, iscsi_hdr, NULL, 0);
}


/*
 * This is executed only by the rx thread, NOT the tx thread as name implies!!!
 * During a login, this is called by the rx thread to send a login reject pdu.
 * The status_class parameter MUST not be zero.
 * Therefore, conn->session will not be valid when called by rx thread.
 * relevant parts of the standard are:
 *
 * Draft 20 section 5.3.1 Login Phase Start
 *  "-Login Response with Login reject. This is an immediate rejection from
 *  the target that causes the connection to terminate and the session to
 *  terminate if this is the first (or only) connection of a new session.
 *  The T bit and the CSG and NSG fields are reserved."
 *
 * Draft 20 section 10.13.1 Version-max
 *  "All Login responses within the Login Phase MUST carry the same
 *  Version-max."
 *
 * Draft 20 section 10.13.2 Version-active
 *  "All Login responses within the Login Phase MUST carry the same
 *  Version-active."
 *
 * Draft 20 section 10.13.3 TSIH
 *  "With the exception of the Login Final-Response in a new session,
 *  this field should be set to the TSIH provided by the initiator in
 *  the Login Request."
 *
 * Draft 20 section 10.13.4 StatSN
 *  "This field is only valid if the Status-Class is 0."
 *
 * Draft 20 section 10.13.5 Status-Class and Status-Detail
 *  "0 - Success - indicates that the iSCSI target successfully received,
 *  "understood, and accepted the request. The numbering fields (StatSN,
 *  ExpCmdSN, MaxCmdSN) are only valid if Status-Class is 0."
 */
static int
iscsi_tx_login_reject(struct iscsi_conn *conn,
                   struct iscsi_init_login_cmnd *pdu,
                   uint8_t status_class, uint8_t status_detail)
{
    uint8_t iscsi_hdr[ISCSI_HDR_LEN];
    struct iscsi_targ_login_rsp *hdr;

    /* most fields in the Login reject header are zero (reserved) */
    memset(iscsi_hdr, 0x0, ISCSI_HDR_LEN);
    hdr = (struct iscsi_targ_login_rsp *) iscsi_hdr;

    hdr->opcode = ISCSI_TARG_LOGIN_RSP;

    /* the T bit and CSG and NSG fields are reserved on a Login reject */

    hdr->version_max = ISCSI_MAX_VERSION;
    hdr->version_active = ISCSI_MIN_VERSION;

    /* there is no data attached to this login reject */

    memcpy(hdr->isid, pdu->isid, 6);
    hdr->tsih = htons(pdu->tsih);
    hdr->init_task_tag = htonl(pdu->init_task_tag);

    /* The numbering fields (StatSN, ExpCmdSn, MaxCmdSN) are only valid if
     * Status-Class is 0, which it is not on a Login reject */

    hdr->status_class = status_class;
    hdr->status_detail = status_detail;

    if (send_hdr_only(conn, iscsi_hdr) < 0) {
        return -1;
    }

    TRACE(TRACE_ISCSI, "%s login response sent\n", current->comm);

    if (TRACE_TEST(TRACE_ISCSI_FULL))
        print_targ_login_rsp(hdr);

    return 0;
}


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
static int
handle_login(struct iscsi_conn *conn, uint8_t *buffer)
{
    struct iscsi_init_login_cmnd *pdu = (struct iscsi_init_login_cmnd *) buffer;
    struct iscsi_session         *session;
    uint32_t                      when_called = 0;
    int                           retval = -1;
    struct auth_parameter_type    auth_param;

    /*chap and srp support - CHONG */
    struct parameter_type *p;
    struct parameter_type (*this_param_tbl)[MAX_CONFIG_PARAMS];
    struct parameter_type (*temp_params)[MAX_CONFIG_PARAMS];
    struct iscsi_global   *host;

    if (TRACE_TEST(TRACE_ISCSI_FULL))
        print_init_login_cmnd(pdu);

    /* this will be non-NULL if the parameter table should be freed when done */
    temp_params = NULL;

    if ((session = conn->session) == NULL) {
        /* should never happen */
        goto out;
    }

    if ((host = session->devdata) == NULL) {
        /* should never happen */
        goto out;
    }

    /* always use clean copy of configured parameter table for negotiations */
    this_param_tbl = session->session_params;

    pdu->length = ntohl(pdu->length);
    pdu->tsih = ntohs(pdu->tsih);
    pdu->init_task_tag = ntohl(pdu->init_task_tag);
    pdu->cid = ntohs(pdu->cid);
    pdu->cmd_sn = ntohl(pdu->cmd_sn);
    pdu->exp_stat_sn = ntohl(pdu->exp_stat_sn);

    /* destructive access to session lists */
    if (sem_wait(&host->session_sem))
        goto err_conn_out;

    if (pdu->tsih == 0) {
        /* a new session, the iscsi_session structure is already set up */
        conn->cid = pdu->cid;
        conn->stat_sn = pdu->exp_stat_sn;
        session->cmd_sn = pdu->cmd_sn;
        session->exp_cmd_sn = pdu->cmd_sn;
        session->max_cmd_sn = pdu->cmd_sn + QUEUE_DEPTH_ALLOWED;

        memcpy(session->isid, pdu->isid, 6);

        /* set up the operational parameters from the global structure */
        set_session_parameters(session->oper_param, *session->session_params);

        /* add new session to the end of the global session list
         * with tsih = 0 to indicate login not finished
         */
        list_add_tail(&session->sess_link, &host->session_list);

        /* set flags to allow keys for new session (leading) login */
        when_called = LEADING_ONLY | INITIAL_ONLY | ALL;
    }
    else
    {
        te_bool               found = FALSE;
        struct list_head     *list_ptr;
        struct iscsi_session *temp;
        struct iscsi_conn    *temp_conn;

        /* existing session, check through the session list to find it */
        list_for_each(list_ptr, &conn->dev->session_list) {
            session = list_entry(list_ptr, struct iscsi_session, sess_link);
            if (session->tsih == pdu->tsih) {
                found = TRUE;
                break;
            }
        }

        if (!found) {
            TRACE_ERROR("No existing session with TSIH %u, "
                        "terminate this connection\n",
                        pdu->tsih);
            goto err_conn_out;
        }

        if (conn->portal_group_tag != session->portal_group_tag) {
            TRACE_ERROR("Portal group tag %u for new connection does "
                        "not match portal group tag %u of session\n",
                        conn->portal_group_tag,
                        session->portal_group_tag);

            iscsi_tx_login_reject(conn, pdu, STAT_CLASS_INITIATOR,
                                  STAT_DETAIL_NOT_INCLUDED);
            goto err_conn_out;
        }

        /* check isid */
        if (memcmp(pdu->isid, session->isid, 6) != 0) {
            TRACE_ERROR("The session has a different ISID, "
                        "terminate the connection\n");

            iscsi_tx_login_reject(conn, pdu, STAT_CLASS_INITIATOR,
                                  STAT_DETAIL_ERR);
            goto err_conn_out;
        }

        conn->cid = pdu->cid;
        conn->stat_sn = pdu->exp_stat_sn;

        /* check cid, and if it already exists then release old connection */
        list_for_each(list_ptr, &session->conn_list) {
            temp_conn = list_entry(list_ptr, struct iscsi_conn, conn_link);
            if (temp_conn->cid == conn->cid) {
                TRACE(TRACE_ISCSI, "connection reinstatement with cid %u\n",
                      conn->cid);

                if (iscsi_release_connection(temp_conn) < 0) {
                    TRACE_ERROR("Error releasing connection\n");
                }
                break;
            }
        }

        TRACE(TRACE_ISCSI, "new connection cid %u attached to "
                "existing session tsih %u, IP 0x%08x\n",
              conn->cid, pdu->tsih,
              ((struct sockaddr_in *)conn->ip_address)->sin_addr.s_addr);

        /* add new connection to end of connection list for existing session */
        temp = conn->session;
        conn->session = session;
        list_del(&conn->conn_link);
        temp->nconn = 0;
        list_add_tail(&conn->conn_link, &session->conn_list);
        session->nconn++;

        /* use clean parameter table for negotiations, free it later */
        temp_params = this_param_tbl;
        temp->session_params = NULL;

        /* free up the no-longer-needed session structure */
        iscsi_release_session(temp);

        /* set back the leading-only keys if these keys were set to
         * " KEY_TO_BE_NEGOTIATED" in the leading connection negotation.*/
        reset_parameter_table(*this_param_tbl);

        /* set flags to allow keys for new connection (only) login */
        when_called = INITIAL_ONLY | ALL;
    }

    sem_post(&host->session_sem);
    auth_param.auth_flags = 0;
    auth_param.chap_local_ctx =
        CHAP_CloneContext(conn->dev->auth_parameter.chap_local_ctx);
    auth_param.chap_peer_ctx =
        CHAP_CloneContext(conn->dev->auth_parameter.chap_peer_ctx);
    auth_param.srp_ctx = SRP_CloneContext(conn->dev->auth_parameter.srp_ctx);

    if ((p = find_flag_parameter(TARGETPORTALGROUPTAG_FLAG,
                                 *conn->session->session_params))) {
        p->int_value = conn->session->portal_group_tag;
    }

    if (parameter_negotiate(conn, *this_param_tbl, pdu, when_called, 
                            auth_param)
        < 0) {

        /* chap and srp support - CHONG */
        CHAP_FinalizeContext(auth_param.chap_local_ctx);
        CHAP_FinalizeContext(auth_param.chap_peer_ctx);
        SRP_FinalizeContext(auth_param.srp_ctx);
        goto out;
    }

    /* Bjorn Thordarson, 10-May-2004 -- start -- */
    if (TRACE_TEST(TRACE_ISCSI_FULL)) {
        int k;

        for (k = 0; k < MAX_CONFIG_PARAMS ; k++) {
            if ((*this_param_tbl)[k].parameter_name != NULL &&
                (*this_param_tbl)[k].str_value != NULL)
                printf("PARAM: %s = %s\n", 
                       (*this_param_tbl)[k].parameter_name,
                       (*this_param_tbl)[k].str_value);
        }
    }
    /* Bjorn Thordarson, 10-May-2004 -- end -- */

    /* chap and srp support - CHONG */
    CHAP_FinalizeContext(auth_param.chap_local_ctx);
    CHAP_FinalizeContext(auth_param.chap_peer_ctx);
    SRP_FinalizeContext(auth_param.srp_ctx);
    
    conn->stat_sn++;


    /* set the operational parameters to the negotiated value */
    if (pdu->tsih == 0) {
        /* this is a new connection in a new session */
        set_session_parameters(session->oper_param, *session->session_params);
    }

    /* we are now in Full Feature Phase */
    conn->hdr_crc = (conn->connection_flags & USE_HEADERDIGEST) ? 1 : 0;
    conn->data_crc = (conn->connection_flags & USE_DATADIGEST) ? 1 : 0;

    /* all ok */
    retval = 0;

out:
    /* here when nothing further can or needs to be done
     * to the session or connection structs
     */
    if (temp_params) {
        param_tbl_uncpy(*temp_params);
        free((void*)&temp_params);
    }

    return retval;

err_conn_out:
    /* here on a fatal error detected before session is linked into devdata */
    conn->conn_socket = -1;

    /* put this session onto end of "bad-session" list for cleanup later */
    session = conn->session;
    TRACE(TRACE_DEBUG, "add to list bad session %p, conn %p\n",
          session, conn);

    list_add_tail(&session->sess_link, &host->bad_session_list);

    sem_post(&host->session_sem);

    goto out;
}

/*
 * allocates all the structures necessary for a new connection and a
 * new session.  If later we find out that this connection belongs to
 * an existing session, the session structure alloced here will be
 * freed.  Its just easier to do everything here in one place, and it
 * makes it easier to recover from errors if everything is all set
 * up
 * (i.e., there are no partial structures)
 * returns NULL if anything fails, in which case everything is
 * freed
 * pointer to new iscsi_conn, which points to new
 * iscsi_sess, if all ok
 *
 * @param sock     The socket returned by the accept() function
 * @param ptr      Portal group structure pointer.
 */
static struct iscsi_conn *
build_conn_sess(int sock, struct portal_group *ptr)
{
    struct iscsi_conn *conn;
    struct iscsi_session *session;

    conn = (struct iscsi_conn *)malloc(sizeof(struct iscsi_conn));
    if (!conn) 
    {
        return NULL;
    }

    TRACE(TRACE_DEBUG, "new conn %p for sock %p\n", conn, sock);
    memset(conn, 0, sizeof(struct iscsi_conn));

    INIT_LIST_HEAD(&conn->conn_link);

    INIT_LIST_HEAD(&conn->reject_list);
    sem_init(&conn->reject_sem, 0, 1);

    conn->active = 1;
    conn->conn_id = ++devdata->conn_id;
    conn->conn_socket = sock;
    conn->dev = devdata;
    conn->max_send_length = 8192;
    conn->max_recv_length = 8192;
    conn->portal_group_tag = ptr->tag;
    conn->connection_flags = devdata->force;
    sem_init(&conn->kill_rx_sem, 0, 0);
    sem_init(&conn->kill_tx_sem, 0, 0);
    sem_init(&conn->text_in_progress_sem, 0, 1);

#if 0
    if ((conn->ip_address = malloc(sizeof(struct sockaddr))) == NULL) {
        goto out2;
    }
    addr_len = ptr->ip_length;        
    if (getsockname(sock, conn->ip_address, &addr_len) < 0) {
        goto out3;
    }

    if ((conn->local_ip_address = malloc(sizeof(struct sockaddr))) == NULL) {

        goto out3;
    }

    addr_len = ptr->ip_length;
    /* value-result parameter */
    if (getsockname(sock, conn->local_ip_address, &addr_len) < 0) {
        goto out4;
    }
#endif
    session = malloc(sizeof(struct iscsi_session));
    if (!session) 
    {

        goto out4;
    }

    memset(session, 0, sizeof(struct iscsi_session));

    INIT_LIST_HEAD(&session->sess_link);
    INIT_LIST_HEAD(&session->conn_list);
    list_add_tail(&conn->conn_link, &session->conn_list);

    conn->session = session;
    session->nconn = 1;
    session->devdata = devdata;
    session->portal_group_tag = ptr->tag;
    session->version_max = ISCSI_MAX_VERSION;
    session->version_min = ISCSI_MIN_VERSION;

    if (!(session->session_params = malloc(MAX_CONFIG_PARAMS *
                                           sizeof(struct parameter_type)))) {

        goto out6;
    }

    if (!(session->oper_param = 
          malloc(MAX_CONFIG_PARAMS *
                 sizeof(struct session_operational_parameters)))) {

        goto out7;
    }

    /* copy the parameters information from the global structure */
    param_tbl_cpy(*session->session_params, *devdata->param_tbl);
    session->r2t_period = devdata->r2t_period;

    /* Store SNACK flags as part of Session - SAI */
    session->targ_snack_flg = devdata->targ_snack_flg;

    sem_init(&session->cmnd_sem, 0,1);
    sem_init(&session->retran_sem, 0,0);
    sem_init(&session->thr_kill_sem,0 ,0);

    return conn;

out7:
    free((void**)&session->session_params);

    printf("\n 1 \n");
out6:
    TRACE(TRACE_DEBUG, "Releasing R2T timer %p for session %p\n",
          session->r2t_timer, session);
    free((void*)&session->r2t_timer);

    free((void*)&session);

out4:
    free((void*)&conn->local_ip_address);

    free((void*)&conn->ip_address);

    free((void*)&conn);

    return NULL;
}

void
print_char(char c)
{

    printf("\n| %d | %d | %d | %d | %d | %d | %d | %d |\n",
           (c % 256 - c % 128) / 128,
           (c % 128 - c % 64) / 64,
           (c % 64 - c % 32) / 32,
           (c % 32 - c % 16) / 16,
           (c % 16 - c % 8) / 8,
           (c % 8 - c % 4) / 4,
           (c % 4 - c % 2) / 2,
           (c % 2)
          );
}

int 
create_socket_pair(int *pipe)
{
    int rc;
    
    rc = socketpair(AF_LOCAL, SOCK_STREAM, 0, pipe);
    if (rc != 0)
    {
        fputs("Failed to create sync pipe\n", stderr);
        return 1;
    }

    return 0;
}

#define ISCSI_DEFAULT_PORT 3260


int
iscsi_server_init(void)
{
    /* devdata init */
    devdata = malloc(sizeof(struct iscsi_global));

    memset(devdata, 0, sizeof(struct iscsi_global));

    INIT_LIST_HEAD(&devdata->session_list);
    INIT_LIST_HEAD(&devdata->bad_session_list);

    sem_init(&devdata->session_sem, 0, 1);
    sem_init(&devdata->session_read_mutex, 0, 1);
    sem_init(&devdata->server_sem, 0, 0);

    devdata->param_tbl = malloc(MAX_CONFIG_PARAMS *
                                sizeof(struct parameter_type));
    if (!(devdata->param_tbl)) {

        return -1;
    }

    /* Copy the default parameters */
    param_tbl_init(*devdata->param_tbl);
    /* chap and srp support - CHONG */
    devdata->auth_parameter.chap_local_ctx = CHAP_InitializeContext();
    devdata->auth_parameter.chap_peer_ctx = CHAP_InitializeContext();
    devdata->auth_parameter.srp_ctx = SRP_InitializeContext();

	/* setup the security key hash table */
	setup_security_hash_table();

	TRACE(TRACE_DEBUG, "iSCSI initialization completed\n");

#if 0
	devdata->device = register_target_front_end(tmpt);

	if (!devdata->device) {
		TRACE_ERROR("%s Device registration failed\n", current->comm);
        bring_down_portals();
        return -2;
	}

	devdata->device->dev_specific = (void *) devdata;

	TRACE(TRACE_DEBUG, "Registration complete\n");
#endif

    return 0;
}

void *
iscsi_server_rx_thread(void *arg)
{ 
    struct iscsi_conn *conn;

    char  buf[100];

    iscsi_param = (struct iscsi_thread_param *)arg;

    /****************/ 

    conn = build_conn_sess(iscsi_param->send_recv_csap, iscsi_portal_groups);
    
    /***/
    iscsi_recv_msg(iscsi_param->send_recv_csap, 48, buf, 0);
    /***/

    handle_login(conn, buf);

    iscsi_release_connection(conn);
/*
    my_free((void **)&devdata->param_tbl);
    my_free((void **)&devdata->auth_parameter.chap_local_ctx);
    my_free((void **)&devdata->auth_parameter.chap_peer_ctx);
    my_free((void **)&devdata->auth_parameter.srp_ctx);
  */  
    return 0;
}
