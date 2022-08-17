/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief Open iSCSI specific stuff
 *
 * iSCSI Initiator configuration
 *
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __CYGWIN__

#define TE_LGR_USER      "Configure iSCSI"

#include "te_config.h"

#include <stddef.h>
#include <signal.h>

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta.h"
#include "logfork.h"

#include "te_sleep.h"
#include "iscsi_initiator.h"

/**
 * Sets an iSCSI parameter via `iscsiadm' tool.
 *
 * @return Status code
 * @param recid Open-iSCSI record id
 * @param param         Parameter description
 * @param target        Target-wide parameters
 * @param connection    iSCSI operational parameters
 * @param auth_data     iSCSI security parameters
 */
static te_errno
iscsi_openiscsi_set_param(const char *recid,
                          iscsi_target_param_descr_t *param,
                          iscsi_target_data_t *target,
                          iscsi_connection_data_t *connection,
                          iscsi_tgt_chap_data_t *auth_data)
{
    int rc = 0;
    static char buffer[1024];

    if (!iscsi_is_param_needed(param, target, connection, auth_data))
        return 0;

    iscsi_write_param(iscsi_put_to_buf, buffer,
                      param, target, connection, auth_data);

    RING("Setting %s to %s", param->name, buffer);
    rc = iscsi_unix_cli("iscsiadm -m node --record=%s --op=update "
                      "--name=%s --value='%s'", recid,
                      param->name, buffer);
    return TE_RC(ISCSI_AGENT_TYPE, rc);
}


/** Name of the file where `iscsid' stores its PID */
#define ISCSID_PID_FILE "/var/run/iscsid.pid"

/** Name of Open-iSCSI parameter database */
#define ISCSID_RECORD_FILE "/var/db/iscsi/node.db"

/**
 *  Start Open-iSCSI managing daemon `iscsid'.
 *  If the daemon is already running, then:
 *  - if @p force_start is TRUE, the old daemon is killed and
 *  a new one is started.
 *  - otherwise, the function just returns.
 *
 *  Both ISCSID_PID_FILE and ISCSID_RECORD_FILE are deleted.
 *  The function writes the Initiator name and alias to a file, since these
 *  two parameters cannot be configured later via `iscsiadm'.
 *  After spawning the daemon, it waits until ISCSI_PID_FILE is created,
 *  or else we might encounter problems trying to configure `iscsid' before
 *  it is ready.
 *
 *  @return Status code
 *  @param  target      Target data
 *  @param  force_start Force restarting the daemon if it's running
 */
static te_errno
iscsi_openiscsi_start_daemon(iscsi_target_data_t *target, te_bool force_start)
{
    int   rc = 0;
    FILE *name_file;
    FILE *pid_file;
    int   i;

    RING("Starting iscsid daemon");

    pid_file = fopen(ISCSID_PID_FILE, "r");

    if (pid_file != NULL)
    {
        int iscsid_pid = 0;
        if (fscanf(pid_file, "%d", &iscsid_pid) == 1)
        {
            if (force_start)
            {
                WARN("Stale iscsid (pid = %d) found, killing, iscsid_pid");
                ta_system("iscsiadm --stop");
                te_usleep(1000);
                kill(iscsid_pid, SIGKILL);
            }
            else
            {
                if (kill(iscsid_pid, 0) == 0)
                {
                    fclose(pid_file);
                    return 0;
                }
            }
        }
        fclose(pid_file);
    }
    remove(ISCSID_PID_FILE);
    remove(ISCSID_RECORD_FILE);

    name_file = fopen("/tmp/initiatorname.iscsi", "w");
    if (name_file == NULL)
    {
        rc = errno;
        ERROR("Cannot open /tmp/initiatorname.iscsi: %s",
              strerror(rc));
        return TE_OS_RC(ISCSI_AGENT_TYPE, rc);
    }
    fprintf(name_file, "InitiatorName=%s\n", target->conns[0].initiator_name);
    if (*target->conns[0].initiator_alias != '\0')
        fprintf(name_file, "InitiatorAlias=%s\n", target->conns[0].initiator_alias);
    fclose(name_file);

    iscsi_unix_cli("iscsid %s -c /dev/null -i /tmp/initiatorname.iscsi",
             iscsi_configuration()->verbosity > 0 ? "-d255" : "");

    for (i = 10; i != 0; i--)
    {
        if (access(ISCSID_PID_FILE, R_OK) == 0)
        {
            return 0;
        }
        te_usleep(1000);
    }
    ERROR("Cannot check that iscsid actually started");
    iscsi_openiscsi_stop_daemon();

    return TE_RC(ISCSI_AGENT_TYPE, TE_EFAIL);
}

/**
 * Stop Open-iSCSI managing daemon if it's running
 *
 * @return Status code
 */
te_errno
iscsi_openiscsi_stop_daemon(void)
{
    FILE *pid_file;

    RING("Stopping iscsid daemon");

    pid_file = fopen(ISCSID_PID_FILE, "r");

    if (pid_file != NULL)
    {
        int iscsid_pid = 0;
        if (fscanf(pid_file, "%d", &iscsid_pid) == 1)
        {
            if (kill(iscsid_pid, 0) == 0)
            {
                ta_system("iscsiadm --stop");
                te_usleep(1000);
                kill(iscsid_pid, SIGKILL);
            }
        }
        fclose(pid_file);
    }
    remove(ISCSID_PID_FILE);

    return 0;
}

/**
 * Set iSCSI parameters via `iscsiadm'.
 *
 * @return              Status code
 * @param  target       Target data
 */
static int
iscsi_openiscsi_set_target_params(iscsi_target_data_t *target)
{
    iscsi_target_param_descr_t *p;
#define PARAMETER(field, name, offer, type) \
    {OFFER_##offer, name, type, ISCSI_OPER_PARAM, offsetof(iscsi_connection_data_t, field), NULL, NULL}
#define TGT_PARAMETER(field, name, type) \
    {0, name, type, ISCSI_GLOBAL_PARAM, offsetof(iscsi_target_data_t, field), NULL, NULL}
#define AUTH_PARAM(field, name, predicate) \
        {0, "node.session.auth." name, TRUE, ISCSI_SECURITY_PARAM, \
         offsetof(iscsi_tgt_chap_data_t, field), NULL, predicate}

    /** Operational and security parameters */
    static iscsi_target_param_descr_t params[] =
        {
            TGT_PARAMETER(target_name, "node.name", TRUE),
            TGT_PARAMETER(target_addr, "node.conn[0].address", TRUE),
            TGT_PARAMETER(target_port, "node.conn[0].port", FALSE),
            PARAMETER(max_connections,
                      "node.session.iscsi.MaxConnections", MAX_CONNECTIONS, FALSE),
            PARAMETER(initial_r2t,
                      "node.session.iscsi.InitialR2T", INITIAL_R2T,  TRUE),
            PARAMETER(header_digest,
                      "node.conn[0].iscsi.HeaderDigest", HEADER_DIGEST,  TRUE),
            PARAMETER(data_digest,
                      "node.conn[0].iscsi.DataDigest", DATA_DIGEST,  TRUE),
            PARAMETER(immediate_data,
                      "node.session.iscsi.ImmediateData", IMMEDIATE_DATA,  TRUE),
            PARAMETER(max_recv_data_segment_length,
                      "node.conn[0].iscsi.MaxRecvDataSegmentLength",
                      MAX_RECV_DATA_SEGMENT_LENGTH,
                      FALSE),
            PARAMETER(first_burst_length,
                      "node.session.iscsi.FirstBurstLength",
                      FIRST_BURST_LENGTH,  FALSE),
            PARAMETER(max_burst_length,
                      "node.session.iscsi.MaxBurstLength",
                      MAX_BURST_LENGTH,  FALSE),
            PARAMETER(default_time2wait,
                      "node.session.iscsi.DefaultTime2Wait",
                      DEFAULT_TIME2WAIT,  FALSE),
            PARAMETER(default_time2retain,
                      "node.session.iscsi.DefaultTime2Retain",
                      DEFAULT_TIME2RETAIN,  FALSE),
            PARAMETER(max_outstanding_r2t,
                      "node.session.iscsi.MaxOutstandingr2t",
                      MAX_OUTSTANDING_R2T,  FALSE),
#if 0 /* not implemented in open iscsi */
            PARAMETER(data_pdu_in_order,
                      "node.session.iscsi.DataPDUInOrder",
                      DATA_PDU_IN_ORDER,  TRUE),
            PARAMETER(data_sequence_in_order,
                      "node.session.iscsi.DataSequenceInOrder",
                      DATA_SEQUENCE_IN_ORDER,  TRUE),
#endif
#if 0 /* not implemented in the initiator config tree */
            PARAMETER(if_marker,
                      "node.session.iscsi.IFMarker",
                      IF_MARKER,  TRUE),
            PARAMETER(of_marker,
                      "node.session.iscsi.OFMarker",
                      OF_MARKER,  TRUE),

#endif
#if 0 /* not implemented both in the initiator and the config tree */
            PARAMETER(if_mark_int,
                      "node.session.iscsi.IFMarkInt",
                      IF_MARKER,  TRUE),
            PARAMETER(of_mark_int,
                      "node.session.iscsi.OFMarkInt",
                      OF_MARKER,  TRUE),

#endif

            PARAMETER(error_recovery_level,
                      "node.session.iscsi.ERL",
                      ERROR_RECOVERY_LEVEL,  FALSE),
            AUTH_PARAM(chap, "authmethod", NULL),
            AUTH_PARAM(peer_name, "username", NULL),
            AUTH_PARAM(peer_secret, "password", NULL),
            AUTH_PARAM(local_name, "username_in", iscsi_when_tgt_auth),
            AUTH_PARAM(local_secret, "password_in", iscsi_when_tgt_auth),
            ISCSI_END_PARAM_TABLE
        };

    int   rc;

    for (p = params; p->name != NULL; p++)
    {
        if (p->offer == 0 ||
            ((target->conns[0].conf_params & p->offer) == p->offer))
        {
            if ((rc = iscsi_openiscsi_set_param(target->session_id,
                                                p, target, target->conns,
                                                &target->conns[0].chap)) != 0)
            {
                ERROR("Unable to set param %s: %r", p->name, rc);
                return rc;
            }
        }
    }
    return 0;
#undef TGT_PARAMETER
#undef PARAMETER
#undef AUTH_PARAM
}

/**
 * Allocate a record in Open-iSCSI configuration DB for
 * a given target address and port
 *
 * @return              Record ID or NULL in case of an error
 * @param data          iSCSI configuration table
 * @param target        Target address
 * @param target_port   Target port
 */
static const char *
iscsi_openiscsi_alloc_node(iscsi_initiator_data_t *data,
                           const char *target,
                           int target_port)
{
    char      buffer[80];
    FILE     *nodelist;
    int       status;

    static char recid[ISCSI_SESSION_ID_LENGTH];


    snprintf(buffer, sizeof(buffer),
             "iscsiadm %s -m node --op=new --portal=%s:%d",
             data->verbosity != 0 ? "-d255" : "",
             target, target_port);
    RING("Attempting to create a record for %s:%d", target, target_port);
    nodelist = popen(buffer, "r");
    if (nodelist == NULL)
    {
        ERROR("Unable to get the list of records: %s",
              strerror(errno));
        return NULL;
    }

    if (fgets(buffer, sizeof(buffer), nodelist) == NULL)
    {
        ERROR("EOF from iscsiadm, something's wrong");
        return NULL;
    }

    RING("Got '%s' from iscsiadm", buffer);
    status = pclose(nodelist);
    if (status != 0)
    {
        if (status < 0)
            WARN("Error while waiting for iscsiadm: %s", strerror(errno));
        else
            WARN("iscsiadm terminated abnormally wuth code %x",
                 (unsigned)status);
    }

    *recid = '\0';
    if (sscanf(buffer, "new iSCSI node record added: [%[^]]]", recid) != 1)
    {
        ERROR("Unparsable output from iscsiadm: '%s'", buffer);
        return NULL;
    }

    return recid;
}


/**
 * See iscsi_initiator.h and iscsi_initator_conn_request_thread()
 * for a complete description of the state machine involved.
 */
int
iscsi_initiator_openiscsi_set(iscsi_connection_req *req)
{
    int                     rc = -1;
    int                     rc2 = -1;
    iscsi_target_data_t    *target = iscsi_configuration()->targets + req->target_id;
    iscsi_connection_data_t *conn = target->conns + req->cid;

    if (req->status == ISCSI_CONNECTION_DOWN ||
        req->status == ISCSI_CONNECTION_REMOVED)
    {
        if (*target->session_id == '\0')
        {
            ERROR("Target %d has no associated record id", req->target_id);
            return TE_RC(ISCSI_AGENT_TYPE, TE_EINVAL);
        }
        rc = iscsi_unix_cli("iscsiadm -m node --record=%s --logout",
                          target->session_id);
        rc2 = iscsi_unix_cli("iscsiadm -m node --record=%s --op=delete",
                           target->session_id);
        if (rc != 0 || rc2 != 0)
        {
            return TE_RC(ISCSI_AGENT_TYPE, rc != 0 ? rc : rc2);
        }

        *target->session_id = '\0';
        return 0;
    }
    else if (conn->status == ISCSI_CONNECTION_DISCOVERING)
    {
        rc = iscsi_openiscsi_start_daemon(target,
                                          iscsi_configuration()->n_connections == 0);
        if (rc != 0)
            return rc;
        rc = iscsi_unix_cli("iscsiadm -d255 -m discovery -t st --portal=%s:%d",
                          target->target_addr, target->target_port);
        return TE_RC(ISCSI_AGENT_TYPE, rc);

    }
    else
    {
        rc = iscsi_openiscsi_start_daemon(target,
                                          iscsi_configuration()->n_connections == 0);
        if (rc != 0)
            return rc;

        if (*target->session_id == '\0')
        {
            const char *id = iscsi_openiscsi_alloc_node(iscsi_configuration(),
                                                        target->target_addr,
                                                        target->target_port);
            if (id == NULL)
            {
                return TE_RC(ISCSI_AGENT_TYPE, TE_ETOOMANY);
            }
            strcpy(target->session_id, id);
        }
        rc = iscsi_openiscsi_set_target_params(target);
        if (rc != 0)
        {
            return rc;
        }

        rc = iscsi_unix_cli("iscsiadm -m node --record=%s --login",
                             target->session_id);
        return TE_RC(ISCSI_AGENT_TYPE, rc);
    }

    return 0;
}

#else

#include "te_config.h"
#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_iscsi.h"
#include "iscsi_initiator.h"

te_errno
iscsi_initiator_openiscsi_set(iscsi_connection_req *req)
{
    UNUSED(req);
    return TE_RC(ISCSI_AGENT_TYPE, TE_ENOSYS);
}


#endif
