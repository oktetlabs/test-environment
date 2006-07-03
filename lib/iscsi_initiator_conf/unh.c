/** @file
 * @brief UNH iSCSI specific stuff
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

#include "te_defs.h"
#include "te_errno.h"
#include "logger_api.h"
#include "logger_ta.h"
#include "logfork.h"

#include "iscsi_initiator.h"

/**
 * Checks return code of the configuration
 * of the Initiator. If it is not zero calls return;
 *
 * @param rc_         return code
 * @param parameter_  name of the parameter which was configured
 */
#define CHECK_SHELL_CONFIG_RC(rc_, parameter_) \
    do {                                                                    \
        if (rc_ != 0)                                                       \
        {                                                                   \
            ERROR("Setting %s parameter failed, rc=%d",                     \
                  (parameter_), (rc_));                                     \
            return (rc_);                                                   \
        }                                                                   \
    } while (0)

/** Format of the set command for UNH Initiator */
static const char *conf_iscsi_unh_set_fmt =
"iscsi_manage init set%s %s=%s target=%d host=%d";

/** Format of the set int value command for UNH Initiator */
static const char *conf_iscsi_unh_set_int_fmt =
"iscsi_manage init set%s %s=%d target=%d host=%d";

/** Format of the force command for UNH Initiator */
static const char *conf_iscsi_unh_force_fmt =
"iscsi_manage init force %s=%s target=%d host=%d";

/** 
 * Format of the force string command for UNH Initiator.
 * The string value is written in notation : "...".
 */
static const char *conf_iscsi_unh_force_string_fmt =
"iscsi_manage init force %s=\"%s\" target=%d host=%d";

/** Format of the set int value command for UNH Initiator */
static const char *conf_iscsi_unh_force_int_fmt =
"iscsi_manage init force %s=%d target=%d host=%d";

/** Format of the force for flag command for UNH Initiator */
static const char *conf_iscsi_unh_force_flag_fmt =
"iscsi_manage init force %s target=%d host=%d";

#define SHOULD_OFFER(offer_, param_) \
    (((offer_) & (param_)) == (param_))

/**
 * Sets parameter of the UNH Initiator.
 *
 * @param param_            Name of the parameter
 * @param value_            New value of the parameter
 * @param target_id_        ID of the target for which the parameter
 *                          is set
 */
#define ISCSI_UNH_SET_UNNEGOTIATED(param_, value_, target_id_)      \
    do {                                                            \
        VERB("ISCSI_UNH_SET_UNNEGOTIATED(%s,%p,%d)\n",              \
             param_, value_, target_id_);                           \
        CHECK_SHELL_CONFIG_RC(                                      \
            iscsi_unix_cli(conf_iscsi_unh_set_fmt,                    \
                         "",                                        \
                         (param_),                                  \
                         (value_), (target_id_),                    \
                         iscsi_configuration()->host_bus_adapter),  \
            (param_));                                              \
    } while (0)

/**
 * Sets parameter of the UNH Initiator.
 *
 * @param param_            Name of the parameter
 * @param value_            New value of the parameter
 * @param target_id_        ID of the target for which the parameter
 *                          is set
 * @param mask_             Bit mask for the parameter
 * @param offered_params_   Set of params that should be offered by the
 *                          Initiator during the login phase.
 */
#define ISCSI_UNH_SET(param_, value_, target_id_, mask_, offered_params_)   \
    do {                                                                    \
        VERB("ISCSI_UNH_SET(%s,%p,%d)\n",                                   \
             param_, value_, target_id_);                                   \
        CHECK_SHELL_CONFIG_RC(                                              \
            iscsi_unix_cli(conf_iscsi_unh_set_fmt,                            \
                         SHOULD_OFFER((offered_params_), (mask_)) ? "":"p", \
                         (param_),                                          \
                         (value_), (target_id_),                            \
                         iscsi_configuration()->host_bus_adapter),          \
            (param_));                                                      \
    } while (0)

/**
 * Sets int parameter of the UNH Initiator.
 *
 * @param param_     Name of the parameter
 * @param value_     New value of the parameter
 * @param target_id_ ID of the target for which the parameter
 *                   is set.
 * @param mask_             Bit mask for the parameter
 * @param offered_params_   Set of params that should be offered by the
 *                          Initiator during the login phase.
 */
#define ISCSI_UNH_SET_INT(param_, value_, target_id_, mask_, offered_params_)   \
    do {                                                                        \
        VERB("ISCSI_UNH_SET_INT(%s,%d,%d)\n",                                   \
             param_, value_, target_id_);                                       \
        CHECK_SHELL_CONFIG_RC(                                                  \
            iscsi_unix_cli(conf_iscsi_unh_set_int_fmt,                            \
                         SHOULD_OFFER((offered_params_), (mask_)) ? "":"p",     \
                         (param_),                                              \
                         (value_), (target_id_),                                \
                         iscsi_configuration()->host_bus_adapter),              \
            (param_));                                                          \
    } while (0)

/**
 * "Forces" parameter of the UNH Initiator.
 * Is used for the security parameter due to the UNH notation.
 *
 * @param param_     Name of the parameter
 * @param value_     New value of the parameter
 * @param target_id_ ID of the target for which the parameter
 *                   is set.
 */
#define ISCSI_UNH_FORCE(param_, value_, target_id_, info_)          \
    do {                                                            \
        VERB("ISCSI_UNH_FORCE(%s,%p,%d)\n",                         \
             param_, value_, target_id_);                           \
        CHECK_SHELL_CONFIG_RC(                                      \
            iscsi_unix_cli(conf_iscsi_unh_force_fmt, (param_),        \
                         (value_), (target_id_),                    \
                         iscsi_configuration()->host_bus_adapter),  \
            (info_));                                               \
    } while (0)

/**
 * "Forces" string parameter of the UNH Initiator.
 * Is called when the parameter should be passed in '"'.
 * Is used for the security parameter due to the UNH notation.
 *
 * @param param_     Name of the parameter
 * @param value_     New value of the parameter
 * @param target_id_ ID of the target for which the parameter
 *                   is set.
 */
#define ISCSI_UNH_FORCE_STRING(param_, value_, target_id_, info_)   \
    do {                                                            \
        VERB("ISCSI_UNH_FORCE(%s,%p,%d)\n",                         \
             param_, value_, target_id_);                           \
        CHECK_SHELL_CONFIG_RC(                                      \
            iscsi_unix_cli(conf_iscsi_unh_force_string_fmt, (param_), \
                         (value_), (target_id_),                    \
                         iscsi_configuration()->host_bus_adapter),  \
            (info_));                                               \
    } while (0)

/**
 * "Forces" int parameter of the UNH Initiator.
 * Is used for the security parameter due to the UNH notation.
 *
 * @param param_     Name of the parameter
 * @param value_     New value of the parameter
 * @param target_id_ ID of the target for which the parameter
 *                   is set.
 */
#define ISCSI_UNH_FORCE_INT(param_, value_, target_id_, info_)      \
    do {                                                            \
        VERB("ISCSI_UNH_FORCE_INT(%s,%d,%d)\n",                     \
             param_, value_, target_id_);                           \
        CHECK_SHELL_CONFIG_RC(                                      \
            iscsi_unix_cli(conf_iscsi_unh_force_int_fmt, (param_),    \
                         (value_), (target_id_),                    \
                         iscsi_configuration()->host_bus_adapter),  \
            (info_));                                               \
    } while (0)

/**
 * "Forces" flag parameter of the UNH Initiator.
 * Is used for the security parameter due to the UNH notation.
 *
 * @param param_     Name of the parameter
 * @param value_     New value of the parameter
 * @param target_id_ ID of the target for which the parameter
 *                   is set.
 */
#define ISCSI_UNH_FORCE_FLAG(flag_, target_id_, info_)              \
    do {                                                            \
        VERB("ISCSI_UNH_FORCE_FLAG(%s,%d)\n",                       \
             flag_, target_id_);                                    \
        CHECK_SHELL_CONFIG_RC(                                      \
            iscsi_unix_cli(conf_iscsi_unh_force_flag_fmt, (flag_),    \
                         (target_id_),                              \
                         iscsi_configuration()->host_bus_adapter),  \
            (info_));                                               \
    } while (0)

/**
 * See iscsi_initiator.h and iscsi_initator_conn_request_thread()
 * for a complete description of the state machine involved. 
 */
te_errno
iscsi_initiator_unh_set(iscsi_connection_req *req)
{
    int                      rc = 0;
    iscsi_target_data_t     *target;
    iscsi_connection_data_t *conn;

    int                     offer;

    target = &iscsi_configuration()->targets[req->target_id];
    conn = &target->conns[req->cid];
    
    offer = conn->conf_params;

    VERB("Current number of open connections: %d", 
          target->number_of_open_connections);
    
    if (req->status == ISCSI_CONNECTION_DOWN || 
        req->status == ISCSI_CONNECTION_REMOVED)
    {
        rc = iscsi_unix_cli("iscsi_config down cid=%d target=%d host=%d",
                          req->cid, req->target_id, 
                          iscsi_configuration()->host_bus_adapter);

        if (rc != 0)
        {
            ERROR("Failed to close the connection "
                  "with CID = %d", req->cid);
            return TE_RC(ISCSI_AGENT_TYPE, EINVAL);
        }
    }
    else
    {
        /* We should open new connection */
        /* 1: configurating the Initiator */
        
        CHECK_SHELL_CONFIG_RC(
            iscsi_unix_cli("iscsi_manage init restore target=%d host=%d",
                         req->target_id, iscsi_configuration()->host_bus_adapter),
            "Restoring");
        
        if (strcmp(conn->session_type, "Normal") == 0)
            ISCSI_UNH_SET_UNNEGOTIATED("TargetName", target->target_name, req->target_id);

        /**
         *  The connection is the leading connection of a session,
         *  so configure all session-wide parameters
         */
        if (req->cid == 0)
        {
            /**
             * Some parameters are only meaningful for Normal sessions, 
             * but not Discovery sessions.
             */
            if (strcmp(conn->session_type, "Normal") == 0)
            {
                ISCSI_UNH_SET_INT("MaxConnections", conn->max_connections,
                                  req->target_id, OFFER_MAX_CONNECTIONS, offer);
                ISCSI_UNH_SET("InitialR2T", conn->initial_r2t, req->target_id, 
                              OFFER_INITIAL_R2T, offer);
                ISCSI_UNH_SET("ImmediateData", conn->immediate_data, req->target_id,
                              OFFER_IMMEDIATE_DATA, offer);
                ISCSI_UNH_SET_INT("MaxBurstLength", 
                                  conn->max_burst_length, req->target_id,
                                  OFFER_MAX_BURST_LENGTH, offer);
                ISCSI_UNH_SET_INT("FirstBurstLength", 
                                  conn->first_burst_length, req->target_id,
                                  OFFER_FIRST_BURST_LENGTH, offer);
                ISCSI_UNH_SET_INT("MaxOutstandingR2T", 
                                  conn->max_outstanding_r2t, req->target_id,
                                  OFFER_MAX_OUTSTANDING_R2T, offer);
                ISCSI_UNH_SET("DataPDUInOrder", 
                              conn->data_pdu_in_order, req->target_id,
                              OFFER_DATA_PDU_IN_ORDER, offer);
                ISCSI_UNH_SET("DataSequenceInOrder", 
                              conn->data_sequence_in_order, req->target_id,
                              OFFER_DATA_SEQUENCE_IN_ORDER, offer);
            }
            
            ISCSI_UNH_SET("HeaderDigest", conn->header_digest, req->target_id,
                          OFFER_HEADER_DIGEST, offer);
            
            ISCSI_UNH_SET("DataDigest", conn->data_digest, req->target_id,
                          OFFER_DATA_DIGEST, offer);
            
            ISCSI_UNH_SET_INT("MaxRecvDataSegmentLength", 
                              conn->max_recv_data_segment_length,
                              req->target_id, OFFER_MAX_RECV_DATA_SEGMENT_LENGTH,
                              offer);
            
            ISCSI_UNH_SET_INT("DefaultTime2Wait", 
                              conn->default_time2wait, req->target_id,
                              OFFER_DEFAULT_TIME2WAIT, offer);
            
            ISCSI_UNH_SET_INT("DefaultTime2Retain", 
                              conn->default_time2retain, req->target_id,
                              OFFER_DEFAULT_TIME2RETAIN, offer);
            
            ISCSI_UNH_SET_INT("ErrorRecoveryLevel", 
                              conn->error_recovery_level, req->target_id,
                              OFFER_ERROR_RECOVERY_LEVEL, offer);
            
            ISCSI_UNH_SET_UNNEGOTIATED("SessionType", conn->session_type, 
                                       req->target_id);
            
            
            
        }
        ISCSI_UNH_SET_UNNEGOTIATED("AuthMethod", conn->chap.chap, req->target_id);
        
        /* Target' CHAP */
        if (conn->chap.need_target_auth)
        {
            ISCSI_UNH_FORCE_FLAG("t", req->target_id,
                                 "Target Authentication");
        }
        
        ISCSI_UNH_FORCE_STRING("px", conn->chap.peer_secret, req->target_id,
                               "Peer Secret");
        
        ISCSI_UNH_FORCE("ln", conn->chap.local_name, req->target_id,
                        "Local Name");
        
        if (conn->chap.enc_fmt == BASE_64)
            ISCSI_UNH_FORCE_FLAG("b", req->target_id,
                                 "Encoding Format");
        
        ISCSI_UNH_FORCE_INT("cl", conn->chap.challenge_length, req->target_id,
                            "Challenge Length");
        
        ISCSI_UNH_FORCE("pn", conn->chap.peer_name, req->target_id,
                        "Peer Name");
        
        ISCSI_UNH_FORCE_STRING("lx", conn->chap.local_secret, req->target_id,
                               "Local Secret");
        
        ISCSI_UNH_FORCE_INT("sch", 1, req->target_id,
                            "Load-balancing"); /* Turning on round-robin */
        
        /* Initiator itself */
        ISCSI_UNH_SET_UNNEGOTIATED("InitiatorName", 
                                   conn->initiator_name,
                                   req->target_id);
#if 0
        ISCSI_UNH_SET_UNNEGOTIATED("InitiatorAlias", 
                                   conn->initiator_alias, 
                                   target_id);
#endif
        /* Now the connection should be opened */
        rc = iscsi_unix_cli("iscsi_config up ip=%s port=%d "
                          "cid=%d target=%d host=%d lun=%d",
                          target->target_addr,
                          target->target_port,
                          req->cid, req->target_id, 
                          iscsi_configuration()->host_bus_adapter,
                          ISCSI_DEFAULT_LUN_NUMBER);
        if (rc != 0)
        {
            ERROR("Failed to establish connection with cid=%d", req->cid);
            return TE_RC(ISCSI_AGENT_TYPE, rc);
        }
    }
    return rc;
}

#else /* __CYGWIN__ */

#include "te_config.h"
#include "package.h"
#include "te_defs.h"
#include "te_stdint.h"
#include "te_errno.h"
#include "te_iscsi.h"
#include "iscsi_initiator.h"

te_errno
iscsi_initiator_unh_set(iscsi_connection_req *req)
{
    UNUSED(req);
    return TE_RC(ISCSI_AGENT_TYPE, TE_ENOSYS);
}

#endif /* __CYGWIN__ */
