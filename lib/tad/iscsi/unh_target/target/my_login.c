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
#include <netinet/tcp.h>
#include <sys/select.h>
#include <pthread.h>

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

#include "../userland_lib/my_memory.h"

#include "my_login.h"

#define tprintf(args,...) \
    do {                        \
        printf(args);           \
        printf("\n");           \
    } while(0)

/** Pointer to the device specific data */
static struct iscsi_global *devdata;

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
    if (!conn) {
        goto out1;
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
    session = (struct iscsi_session *)malloc(sizeof(struct iscsi_session));
    if (!session) {

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

    goto out;

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

out1:
out:
    return conn;
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
        tprintf("Failed to create sync pipe");
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
    devdata->param_tbl[2]->int_value = 2;
    /* chap and srp support - CHONG */
    devdata->auth_parameter.chap_local_ctx = CHAP_InitializeContext();
    devdata->auth_parameter.chap_peer_ctx = CHAP_InitializeContext();
    devdata->auth_parameter.srp_ctx = SRP_InitializeContext();

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

    my_free((void **)&devdata->param_tbl);
    my_free((void **)&devdata->auth_parameter.chap_local_ctx);
    my_free((void **)&devdata->auth_parameter.chap_peer_ctx);
    my_free((void **)&devdata->auth_parameter.srp_ctx);
    
    return 0;
}
