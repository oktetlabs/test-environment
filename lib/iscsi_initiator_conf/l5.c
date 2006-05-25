/** @file
 * @brief Level5-specific stuff
 *
 * iSCSI Initiator configuration
 *
 *
 * Copyright (C) 2004 Test Environment authors (see file AUTHORS
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
 * @author Artem Andreev <Artem.Andreev@oktetlabs.ru>
 *
 */

#ifndef __CYGWIN__

#define TE_LGR_USER      "Configure iSCSI"

#include "te_config.h"
#include "package.h"

#include <stddef.h>

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta.h"
#include "logfork.h"

#include "iscsi_initiator.h"

/****** L5 Initiator specific stuff  ****/

static void
iscsi_l5_write_param(FILE *destination, 
                     iscsi_target_param_descr_t *param,
                     iscsi_target_data_t *tgt_data,
                     iscsi_connection_data_t *conn_data,
                     iscsi_tgt_chap_data_t *auth_data)
{
    const char *n;

    if (!iscsi_is_param_needed(param, tgt_data, conn_data, auth_data))
        return;

    for (n = param->name; *n != '\0'; n++)
    {
        if (*n != '_')
            fputc(*n, destination);
    }
    fputs(": ", destination);
    iscsi_write_param(iscsi_write_to_file, destination, param, 
                      tgt_data, conn_data, auth_data);
    fputc('\n', destination);
}

static int
iscsi_l5_write_target_params(FILE *destination, 
                             iscsi_target_data_t *target)
{
    iscsi_target_param_descr_t *p;
    iscsi_connection_data_t    *connection = target->conns;

#define PARAMETER(field, offer, type) \
    {OFFER_##offer, #field, type, ISCSI_OPER_PARAM, \
     offsetof(iscsi_connection_data_t, field), NULL, NULL}
#define GPARAMETER(name, field, type) \
    {0, name, type, ISCSI_GLOBAL_PARAM, \
     offsetof(iscsi_target_data_t, field), NULL, NULL}
#define AUTH_PARAM(field, name, type, predicate) \
     {0, name, type, ISCSI_SECURITY_PARAM, \
      offsetof(iscsi_tgt_chap_data_t, field), NULL, predicate}
#define CONSTANT(name, field, type, predicate) \
    {0, name, type, ISCSI_FIXED_PARAM, \
    offsetof(iscsi_constant_t, field), NULL, predicate}


    static iscsi_target_param_descr_t session_params[] =
        {
            PARAMETER(max_connections, MAX_CONNECTIONS,  FALSE),
            PARAMETER(initial_r2t, INITIAL_R2T,  TRUE),
            PARAMETER(immediate_data, IMMEDIATE_DATA,  TRUE),
            PARAMETER(first_burst_length, FIRST_BURST_LENGTH,  FALSE),
            PARAMETER(max_burst_length, MAX_BURST_LENGTH,  FALSE),
            PARAMETER(default_time2wait, DEFAULT_TIME2WAIT,  FALSE),
            PARAMETER(default_time2retain, DEFAULT_TIME2RETAIN,  FALSE),
            PARAMETER(max_outstanding_r2t, MAX_OUTSTANDING_R2T,  FALSE),
            PARAMETER(data_pdu_in_order, DATA_PDU_IN_ORDER,  TRUE),
            PARAMETER(data_sequence_in_order, DATA_SEQUENCE_IN_ORDER,  TRUE),
            PARAMETER(error_recovery_level, ERROR_RECOVERY_LEVEL,  FALSE),
            AUTH_PARAM(local_name, "TargetCHAPName", TRUE, iscsi_when_tgt_auth),
            AUTH_PARAM(local_secret, "TargetCHAPSecret", TRUE, iscsi_when_tgt_auth),
            ISCSI_END_PARAM_TABLE
        };
    static iscsi_target_param_descr_t connection_params[] =
        {
            GPARAMETER("Host", target_addr, TRUE),
            GPARAMETER("Port", target_port, FALSE),
            PARAMETER(header_digest, HEADER_DIGEST,  TRUE),
            PARAMETER(data_digest, DATA_DIGEST,  TRUE),
            PARAMETER(max_recv_data_segment_length, MAX_RECV_DATA_SEGMENT_LENGTH,  
                      FALSE),
            AUTH_PARAM(chap, "AuthMethod", TRUE, iscsi_when_not_tgt_auth),
            CONSTANT("AuthMethod", l5_tgt_auth, TRUE, iscsi_when_tgt_auth),
            AUTH_PARAM(peer_name, "CHAPName", TRUE, iscsi_when_chap),
            AUTH_PARAM(peer_secret, "CHAPSecret", TRUE, iscsi_when_chap),
            ISCSI_END_PARAM_TABLE
        };

    for (p = session_params; p->name != NULL; p++)
    {
#if 0
        if ((connection->conf_params & p->offer) == p->offer)
        {
#endif
            iscsi_l5_write_param(destination, p, 
                                 target, connection, &connection->chap);
#if 0
        }
#endif
    }
    /** Other authentication parameters are not supported by
     *  L5 initiator, both on the level of script and on 
     *  the level of ioctls
     */

    for (; connection < target->conns + MAX_CONNECTIONS_NUMBER;
         connection++)
    {
        if (connection->status != ISCSI_CONNECTION_REMOVED)
        {
            fprintf(destination, "\n\n[target%d_conn%d]\n",
                    target->target_id,
                    (int)(connection - target->conns));

            for (p = connection_params; p->name != NULL; p++)
            {
#if 0
                if ((connection->conf_params & p->offer) == p->offer)
                {
#endif
                    iscsi_l5_write_param(destination, p, 
                                         target, connection, &connection->chap);
#if 0
                }
#endif
            }
        }
    }
    return 0;
#undef CONSTANT
#undef PARAMETER
#undef GPARAMETER
#undef AUTH_PARAM
}

static int
iscsi_l5_write_config(iscsi_initiator_data_t *iscsi_data)
{
    static char filename[MAX_CMD_SIZE];
    FILE *destination;
    iscsi_target_data_t *target;
    int conn_no;
    te_bool is_first;
    
    snprintf(filename, sizeof(filename),
             "%s/configs/te",
             *iscsi_data->script_path != '\0' ? 
             iscsi_data->script_path : ".");
    destination = fopen(filename, "w");
    if (destination == NULL)
    {
        ERROR("Cannot open '%s' for writing: %s",
              filename, strerror(errno));
        return TE_OS_RC(ISCSI_AGENT_TYPE, errno);
    }
             
    target = iscsi_data->targets;
    if (target->target_id < 0)
    {
        ERROR("No targets configured");
        return TE_RC(ISCSI_AGENT_TYPE, TE_ENOENT);
    }
    /** NOTE: L5 Initator seems to be unable 
     *  to have different Initator names for
     *  different Targets/Connections.
     *  So we just using the first one
     */
    fprintf(destination, "[INITIATOR]\n"
                         "Name: %s\n"
                         "Targets:",
            target->conns[0].initiator_name);
    is_first = TRUE;
    for (; target < iscsi_data->targets + MAX_TARGETS_NUMBER; target++)
    {
        if (target->target_id >= 0)
        {
            target->is_active = FALSE;
            for (conn_no = 0; conn_no < MAX_CONNECTIONS_NUMBER; conn_no++)
            {
                if (target->conns[conn_no].status != ISCSI_CONNECTION_REMOVED)
                {
                    target->is_active = TRUE;
                    break;
                }
            }
            if (target->is_active)
            {
                fprintf(destination, "%s target%d", 
                        is_first ? "" : ",",
                        target->target_id);
                is_first = FALSE;
            }
        }
    }

    target = iscsi_data->targets;
    for (; target < iscsi_data->targets + MAX_TARGETS_NUMBER; target++)
    {
        if (target->target_id >= 0 && target->is_active)
        {
            fprintf(destination, 
                    "\n\n[target%d]\n"
                    "TargetName: %s\n"
                    "Connections: ",
                    target->target_id,
                    target->target_name);
            is_first = TRUE;
            for (conn_no = 0; conn_no < MAX_CONNECTIONS_NUMBER; conn_no++)
            {
                if (target->conns[conn_no].status != ISCSI_CONNECTION_REMOVED)
                {
                    fprintf(destination, "%s target%d_conn%d",
                            is_first ? "" : ",",
                            target->target_id,
                            conn_no);
                    is_first = FALSE;
                }
            }
            fputs("\n\n", destination);
            iscsi_l5_write_target_params(destination, target);
        }
    }
    fclose(destination);
    ta_system_ex("cat %s", filename);
    return ta_system_ex("cd %s; ./iscsi_setconfig te", 
                           iscsi_configuration()->script_path);
}

int
iscsi_initiator_l5_set(iscsi_connection_req *req)
{
    int                  rc = -1;
    int                  former_status;
    iscsi_target_data_t *target = iscsi_configuration()->targets + req->target_id;
    iscsi_connection_data_t *conn = target->conns + req->cid;
    
    switch (req->status)
    {
        case ISCSI_CONNECTION_DOWN:
        case ISCSI_CONNECTION_REMOVED:
            
            if (strcmp(conn->session_type, "Discovery") != 0)
            {
                rc = ta_system_ex("cd %s; ./iscsi_stopconns %s target%d_conn%d", 
                                  iscsi_configuration()->script_path,
                                  iscsi_configuration()->verbosity != 0 ? "-v" : "",
                                  req->target_id, req->cid);
                if (rc != 0)
                {
                    ERROR("Unable to stop initiator connection %d, %d, "
                          "status = %d", 
                          req->target_id, req->cid, rc);
                    return TE_RC(ISCSI_AGENT_TYPE, TE_ESHCMD);
                }
            }
            break;
        case ISCSI_CONNECTION_UP:
            if (iscsi_configuration()->n_connections == 0)
            {
                rc = iscsi_l5_write_config(iscsi_configuration());
                if (rc != 0)
                    return rc;
            }

            if (strcmp(conn->session_type, "Discovery") != 0)
            {
                rc = ta_system_ex("cd %s; ./iscsi_startconns %s target%d_conn%d", 
                                  iscsi_configuration()->script_path,
                                  iscsi_configuration()->verbosity != 0 ? "-v" : "",
                                  req->target_id, req->cid);
            }
            else
            {
                if (target->number_of_open_connections == 0)
                {
                    rc = ta_system_ex("cd %s; ./iscsi_discover te", 
                                         iscsi_configuration()->script_path);
                    if (rc != 0)
                        rc = TE_RC(ISCSI_AGENT_TYPE, TE_ESHCMD);
                }
                else
                {
                    WARN("Discovery session already in progress");
                    rc = TE_RC(ISCSI_AGENT_TYPE, TE_EINPROGRESS);
                }
            }

            if (rc != 0)
            {
                ERROR("Unable to start initiator connection %d, %d",
                      req->target_id, req->cid);
                return TE_RC(ISCSI_AGENT_TYPE, rc);
            }
            break;
        default:
            ERROR("Invalid operational code %d", req->status);
            return TE_RC(ISCSI_AGENT_TYPE, TE_EINVAL);
    }
    return 0;
}


#endif
