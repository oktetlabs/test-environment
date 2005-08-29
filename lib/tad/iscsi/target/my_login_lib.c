

struct iscsi_global *devdata;

int
send_async_logout(struct iscsi_conn *conn)
{
    struct async_logout *pdu = malloc(sizeof(struct async_logout));

    memset(pdu, 0, sizeof(struct async_logout));

    pdu->some = 0xc0;
    pdu->fff = 0xffffffff;

    pdu->stat_sn = htonl(conn->stat_sn);
    pdu->exp_cmd_sn = htonl(5);
    pdu->max_cmd_sn = htonl(10);

    pdu->async_event = 3;
    pdu->par2 = htons(0);
    pdu->par3 = htons(0);

    if (send(conn->conn_socket, pdu, sizeof(struct async_logout), 0) < 0)
    {
        free(pdu);
        return -1;
    }

    free(pdu);
    
    return 0;
}

/**
 * releases the connection
 */
int
iscsi_release_connection(struct iscsi_conn *conn)
{
    if (conn == NULL) {
        return -1;
    }

    /* Release socket */
    close(conn->conn_socket);
    conn->conn_socket = -1;

    TRACE(TRACE_ISCSI_FULL, "Dequeue connection conn->cid %u\n", conn->conn_id);

#if 0
    /* Dequeue connection */
    /* __ prefix should be used */
    list_del(&conn->conn_link);
    conn->session->nconn--;
#endif

    /* Free connection */
    my_free((void **)&conn->local_ip_address);
    my_free((void **)&conn->ip_address);
    my_free((void **)&conn);

    return 0;
}

/*
 * executed only by the tx thread.
*/
int
handle_logout_rsp(uint8_t *buf, struct iscsi_conn *conn, 
                  struct iscsi_session *session)
{
    uint8_t                        iscsi_hdr[ISCSI_HDR_LEN];
    struct  iscsi_targ_logout_rsp *hdr;
    int                            rc;

    printf("Handle logout request");

    memset(iscsi_hdr, 0x0, ISCSI_HDR_LEN);
    hdr = (struct iscsi_targ_logout_rsp *) iscsi_hdr;

    hdr->opcode = ISCSI_TARG_LOGOUT_RSP;

    hdr->flags |= F_BIT;

#define INIT_TASK_TAG_OFFSET 16
    hdr->init_task_tag = htonl(*((uint32_t *)(buf + INIT_TASK_TAG_OFFSET)));
#undef INIT_TASK_TAG_OFFSET

    hdr->stat_sn = htonl(conn->stat_sn++);

    /* for now, we always send response = 0, connection or session closed ok */
    hdr->exp_cmd_sn = htonl(session->exp_cmd_sn);
    hdr->max_cmd_sn = htonl(session->max_cmd_sn);

    /* connection now logged out, do not send any more commands after this */
    conn->connection_flags |= CONN_LOGGED_OUT;

    rc = send(conn->conn_socket, hdr, ISCSI_HDR_LEN, 0);
    if (rc != ISCSI_HDR_LEN)
        return -1;

    TRACE(TRACE_ISCSI, "logout response sent\n");

    return 0;
}

int
handle_login(struct iscsi_conn *conn, uint8_t *buffer)
{
    struct iscsi_init_login_cmnd *pdu = (struct iscsi_init_login_cmnd *) buffer;
    struct iscsi_session         *session;
    struct iscsi_session         *temp;
    struct iscsi_conn            *temp_conn;
    uint32_t                      when_called;
    int                           found;
    int                           retval = -1;
    struct auth_parameter_type    auth_param;

    /*chap and srp support - CHONG */
    struct parameter_type *p;
    struct parameter_type (*this_param_tbl)[MAX_CONFIG_PARAMS];
    struct parameter_type (*temp_params)[MAX_CONFIG_PARAMS];
    struct list_head      *list_ptr;
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

    if (parameter_negotiate(conn, *this_param_tbl, pdu, when_called, auth_param)
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
                tprintf("PARAM: %s = %s\n", (*this_param_tbl)[k].parameter_name,
                        (*this_param_tbl)[k].str_value);
        }
    }
    /* Bjorn Thordarson, 10-May-2004 -- end -- */

    /* chap and srp support - CHONG */
    CHAP_FinalizeContext(auth_param.chap_local_ctx);
    CHAP_FinalizeContext(auth_param.chap_peer_ctx);
    SRP_FinalizeContext(auth_param.srp_ctx);
    
    conn->stat_sn++;

    if (conn->stat_sn < 0) {
        goto out;
    }

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
        free((void**)&temp_params);
    }

    return retval;

err_conn_out:
    /* here on a fatal error detected before session is linked into devdata */
    close(conn->conn_socket);
    conn->conn_socket = 1;

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
 */
struct iscsi_conn *
build_conn_sess(int sock, struct portal_group *ptr)
{

    struct iscsi_conn *conn;
    struct iscsi_session *session;
    int addr_len;

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
    free((void**)&session->r2t_timer);

out5:
    free((void**)&session);

out4:
    free((void**)&conn->local_ip_address);

out3:
    free((void**)&conn->ip_address);

out2:
    free((void**)&conn);

out1:
    close(sock);
out:
    return conn;
}
