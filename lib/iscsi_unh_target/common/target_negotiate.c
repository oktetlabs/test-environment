/*	common/target_negotiate.c
 *
 *	vi: set autoindent tabstop=4 shiftwidth=4 :
 *
 *	This defines the functions used in the Login phase
 *	by the iSCSI target for parameter negotiation
 *
 *	Copyright (C) 2001-2004 InterOperability Lab (IOL)
 *	University of New Hampshire (UNH)
 *	Durham, NH 03824
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2, or (at your option)
 *	any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *	USA.
 *
 *	The name of IOL and/or UNH may not be used to endorse or promote
 *	products derived from this software without specific prior
 *  written permission.
 */
/*
 * Converted to  indent -kr -i4 -ts4 -sob -l80 -ss -bs -psl. 07/25/2003
 * TO Do. IMO . jpd
 * 1. target_security_negotiate(), target_parameter_negotiate() and
 *    scan_input_and_process have too many parameters both as 
 *    formal and auotomatic.
 *
 *    May want to consider passing structures. Also the functions
 *    are long. Consider resturctureing into smaller functions
 *    for each state. This is critical code that has been well tested
 *    so the risk is obvious. 
 */

#include <te_config.h>
#include <te_defs.h>

#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "list.h"

#include "my_memory.h"
#include "iscsi_common.h"
#include <iscsi_custom.h>
#include "debug.h"
#include "crc.h"

/* chap and srp support - CHONG */
#include "../security/chap.h"
#include "../security/srp.h"
#include "../security/misc_func.h"


#include "text_param.h"
#include "target_negotiate.h"

static int
iscsi_send_msg_ex(struct iscsi_conn *conn, int sock,
                  struct generic_pdu *outputpdu)
{
    unsigned split_pdu = iscsi_get_custom_value(conn->custom, "split_pdu_at");
    struct generic_pdu inputpdu;

    if (split_pdu == 0 || outputpdu->text_length <= split_pdu)
    {
        if (iscsi_send_msg(sock, outputpdu, conn->connection_flags) < 0) {
            TRACE(TRACE_DEBUG, "iscsi_send_msg failed");
            return -1;
        } else {
            conn->stat_sn++;
        }
    }
    else
    {
        unsigned total; 
        
        TRACE(TRACE_DEBUG, "Splitting PDU %d at %d", outputpdu->text_length, split_pdu);
        for (total = outputpdu->text_length; total > 0; total -= split_pdu)
        {
            if (total > split_pdu)
            {
                outputpdu->text_length = split_pdu;
                outputpdu->flags      |= C_BIT;
            }
            else
            {
                outputpdu->text_length = total;
                outputpdu->flags &= ~C_BIT;
            }
            if (iscsi_send_msg(sock, outputpdu, conn->connection_flags) < 0) 
            {
                TRACE(TRACE_DEBUG, "iscsi_send_msg failed");
                return -1;
            }
            else 
            {
                conn->stat_sn++;
                if ((outputpdu->flags & C_BIT) == C_BIT)
                {
                    TRACE(TRACE_DEBUG, "Waiting for a continuation request");
                    outputpdu->cmd_sn = htonl(conn->stat_sn + 1);
                    memmove(outputpdu->text, outputpdu->text + outputpdu->text_length,
                            total - outputpdu->text_length);
                    if (iscsi_recv_msg(sock, ISCSI_HDR_LEN, (char *) &inputpdu,
                                       conn->connection_flags) < 0) 
                    {
                        TRACE_ERROR("iscsi_recv_msg failed");
                        return -1;
                    }
                    if (inputpdu.length != 0)
                    {
                        TRACE_ERROR("An initiator sent a non-empty Login Request");
                        return -1;
                    }
                }
            }
        }
    }
    return 0;
}

/*************************************************************************
 *This function sends a login response with proper status code when error
 * occurs in target during login phase
 *
 ************************************************************************/

static int
login_reject(struct iscsi_conn *conn, int class, int detail,
			 struct generic_pdu *outputpdu)
{

	TRACE(TRACE_ENTER_LEAVE, "Enter login_reject");

	outputpdu->status_class = class;
	outputpdu->status_detail = detail;

	/* Draft 20, Section 5.3.1 Login Phase Start
	   "-Login Response with Login Reject.  This is an immediate rejection
	   from the target that causes the connection to terminate and the session
	   to terminate if this is the first (or only) connection of a new
	   session.  The T bit and the CSG and NSG fields are reserved."
	 */
	outputpdu->flags &= ~CSG;	/* set the CSN to zero */
	outputpdu->flags &= ~NSG;	/* set the NSG to zero */
	outputpdu->flags &= ~T_BIT;

	/* Draft 20, Section 10.13.4 StatSN (for Login Response)
	   "This field is valid only if Status-Class is 0."
	 */

	outputpdu->cmd_sn = 0;
	outputpdu->exp_stat_sn = 0;
	outputpdu->max_cmd_sn = 0;

	/* send out the outputpdu which never has any data attached to it */
	outputpdu->text_length = 0;
	if (iscsi_send_msg(conn->conn_socket, outputpdu, conn->connection_flags) <
		0) {
		TRACE(TRACE_DEBUG, "iscsi_send_msg failed");
		return -1;
	}

	TRACE(TRACE_ENTER_LEAVE, "Leave login_reject");

	return 0;
}

/*******************************************************************
 *This function checks every login request except the first login for
 * new connection or session
 *
 *******************************************************************/
static int
check_other_login(struct iscsi_conn *conn, int correct_CSG,
				  struct generic_pdu *inputpdu, struct generic_pdu *outputpdu)
{
	int retval = 0;

	TRACE(TRACE_ENTER_LEAVE, "Enter check_other_login");

	/* check if the login has proper version set */
	if ((inputpdu->version_max > ISCSI_MAX_VERSION) ||
		(inputpdu->version_max < inputpdu->version_active)) {
		/* protocol error  */
		TRACE_ERROR("Bad version_max %d\n", inputpdu->version_max);
		login_reject(conn, STAT_CLASS_INITIATOR,
					 STAT_DETAIL_VERSION_NOT_SUPPORTED, outputpdu);
		retval = -1;

	}

	TRACE(TRACE_ENTER_LEAVE, "Leave check_other_login, retval %d", retval);

        UNUSED(correct_CSG);

	return retval;
}

/*****************************************************************************
 *  This function checks if the first login has InitiatorName, and
 *  TargetName if SessionType is Normal (Discovery does not need TargetName)
 *  Send login reject if any parameter is missing.
 *
 ****************************************************************************/
static int
check_flags(struct iscsi_conn *conn, uint64_t login_flags,
			struct generic_pdu *outputpdu,
			struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS])
{
	int err = 1;
	struct parameter_type *ptr;

	TRACE(TRACE_ENTER_LEAVE, "Enter check_flags");

	if (!(login_flags & INITIATORNAME_FLAG)) {
		TRACE_ERROR("Initiator name not given in initial login\n");
		err = -1;
	} else {
		ptr = find_flag_parameter(TARGETPORTALGROUPTAG_FLAG, p_param_tbl);
		if (!(login_flags & TARGETNAME_FLAG)) {
			/* TargetName not included in first login pdu by initiator */
			if (login_flags & DISCOVERY_FLAG) {
				/* Discovery session, do not return TargetPortalGroupTag */
				if (ptr) {
					ptr->neg_info &= ~KEY_TO_BE_NEGOTIATED;
				}
			} else {
				/* Normal session with no TargetName is bad */
				TRACE_ERROR
				("Target name not given in initial login to NORMAL session\n");
				err = -1;
			}
		} else {
			/* TargetName was in first login pdu from initiator
			 * RFC 3720 Section 2.1 Definitions
			 * "- SSID (Session ID):
			 * ...
			 * The TargetPortalGroupTag key must also be returned by the target
			 * as a confirmation during connection establishment when
			 * TargetName is given."
			 */
			if (ptr) {
				/* force sending of TargetPortalGroupTag
				 * (correct value already set up when connection set up)
				 */
				ptr->neg_info |= KEY_TO_BE_NEGOTIATED;
			}
		}
	}

	/* send a login reject if an error was seen */
	if (err < 0) {
		login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_MISSING_PARAMETER,
					 outputpdu);
	}

	TRACE(TRACE_ENTER_LEAVE, "Leave check_flags, err = %d", err);

	return err;
}

/****************************************************************
 * This function checks the first login in a new connection
 *
 ****************************************************************/

static int
check_first_login(struct iscsi_conn *conn,
				  struct generic_pdu *inputpdu, struct generic_pdu *outputpdu)
{
	int retval = 0;

	TRACE(TRACE_ENTER_LEAVE, "Enter check_first_login");

	/* check that this new connection does not exceed MaxConnections
	 * except for the following:
	 * Draft 20 Section 5.3.4 Connection Reinstatement
	 * "Targets MUST support opening a second connection even when they do
	 * not support multiple connections in Full Feature Phase if
	 * ErrorRecoveryLevel is 2 and SHOULD support opening a second
	 * connection if ErrorRecoveryLevel is less than 2."
	 */
	if (conn->session->nconn > conn->session->oper_param->MaxConnections
		&& conn->session->nconn > 2) {
		TRACE_ERROR("current nconn %d > MaxConnections %d\n",
					conn->session->nconn,
					conn->session->oper_param->MaxConnections);
		login_reject(conn, STAT_CLASS_INITIATOR,
					 STAT_DETAIL_TOO_MANY_CONNECTIONS, outputpdu);
		retval = -1;
		goto out;
	}

	/* check versions */
	if (inputpdu->version_active != conn->session->version_min) {
		TRACE_ERROR("unsupported version %d, terminate the connection\n",
					inputpdu->version_active);

		login_reject(conn, STAT_CLASS_INITIATOR,
					 STAT_DETAIL_VERSION_NOT_SUPPORTED, outputpdu);
		retval = -1;
		goto out;
	}

	conn->session->version_active = conn->session->version_max;

out:
	TRACE(TRACE_ENTER_LEAVE, "Leave check_first_login, retval %d", retval);

	return retval;
}

void __attribute__ ((no_instrument_function))
print_isid_tsih_message(struct iscsi_session *session, char *message)
{
	int i;
	char buffer[32], *ptr;
	
	ptr = buffer;
	for (i = 0; i < 6; i++) {
		ptr += sprintf(ptr, " %02x", session->isid[i]);
	}
	printf("%sISID%s TSIH %u\n", message, buffer, session->tsih);
}

static void __attribute__ ((no_instrument_function))
finalize_new_session(struct iscsi_session *session)
{
	struct iscsi_global *host;
	struct parameter_type *sess_iname;
	struct parameter_type *sess_tname;
	struct parameter_type *sess_pname;
	uint16_t new_tsih;

	if ((host = session->devdata) == NULL) {
		/* should never happen */
		goto out;
	}

	if ((sess_iname = find_flag_parameter(INITIATORNAME_FLAG,
										  *session->session_params)) == NULL) {
		/* should never happen */
		goto out;
	}

	if ((sess_tname = find_flag_parameter(TARGETNAME_FLAG,
										  *session->session_params)) == NULL) {
		/* should never happen */
		goto out;
	}

	if ((sess_pname = find_flag_parameter(TARGETPORTALGROUPTAG_FLAG,
										  *session->session_params)) == NULL) {
		/* should never happen */
		goto out;
	}

	/* assign this new session a new tsih which is not zero */
	if ((new_tsih = ++host->ntsih) == 0)
		new_tsih = ++host->ntsih;

	/* ok to use this tsih, it is unique */
	session->tsih = new_tsih;
#if 0
	print_isid_tsih_message(session, "Created session with ");
#endif

out:
	return;
}

static int
target_check_login(struct iscsi_conn *conn,
				   struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
				   struct generic_pdu *inputpdu,
				   struct generic_pdu *outputpdu,
				   uint32_t when_called,
				   int noperational,
				   uint64_t *login_flags,
				   int *count, struct unknown_key **unknown_key_list)
{
	int *max_send_length = &conn->max_send_length;
	int sock = conn->conn_socket;
	int retval = 0;
	int correct_CSG = (inputpdu->flags & CSG) >> CSG_SHIFT;
	int add_length;
	int what_to_process;

	TRACE(TRACE_DEBUG, "Enter target_check_login");

	if ((inputpdu->opcode & ISCSI_OPCODE) != ISCSI_INIT_LOGIN_CMND) {
		/* opcode just received is not a Login, but is should be! */
		TRACE_ERROR("invalid opcode 0x%02x during login\n",
					inputpdu->opcode & ISCSI_OPCODE);
		login_reject(conn, STAT_CLASS_INITIATOR,
					 STAT_DETAIL_INVALID_DURING_LOGIN, outputpdu);
		retval = -1;
		goto out;
	}

	TRACE(TRACE_ISCSI, "Got Login command, CSG %d, NSG %d, T %d",
		  (inputpdu->flags & CSG) >> CSG_SHIFT, inputpdu->flags & NSG,
		  (inputpdu->flags & T_BIT) >> 7);

    print_init_login_cmnd((struct iscsi_init_login_cmnd *)inputpdu);

	/* check if the login has I bit set */
	if (!(inputpdu->opcode & I_BIT)) {
		/* protocol error  */
		TRACE_ERROR("login request I bit not set!\n");

		login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_ERR, outputpdu);
		retval = -1;
		goto out;
	}

	/* check if the input CSG is valid */
	if ((inputpdu->flags & CSG) >= CSG2) {
		TRACE_ERROR("Invalid CSG %d should be 0 or 1\n",
					(inputpdu->flags & CSG) >> CSG_SHIFT);
		login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_ERR, outputpdu);
		retval = -1;
		goto out;
	}

	/* check that initiator and target agree on the current stage */
	if (((inputpdu->flags & CSG) >> CSG_SHIFT) != correct_CSG) {
		/* protocol error */
		TRACE_ERROR("Initiator state is %d, expected %d\n",
					(inputpdu->flags & CSG) >> CSG_SHIFT, correct_CSG);
		login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_ERR, outputpdu);
		retval = -1;
		goto out;
	}

	/* check if the input NSG is valid:
	   NSG can never be 2,
	   NSG must be 0 if the T bit is 0,
	   NSG must be greater than CSG if T bit is 1.
	 */
	if (inputpdu->flags & T_BIT) {
		/* T bit is set, NSG must be valid */
		if ((inputpdu->flags & NSG) == NSG2 ||
		    (inputpdu->flags & NSG) <= ((inputpdu->flags & CSG) >> CSG_SHIFT)) {
			TRACE_ERROR("invalid NSG %d\n", inputpdu->flags & NSG);
			login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_ERR, outputpdu);
			retval = -1;
			goto out;
		}
	} else {
		/* T bit is not set, NSG should be 0 */
		if (inputpdu->flags & NSG) {
			TRACE_WARNING("T bit is 0 but NSG = %d, should be 0 (ignored)\n",
							inputpdu->flags & NSG);
		}
	}

	outputpdu->flags &= ~(CSG | NSG | T_BIT);
	outputpdu->flags |= (inputpdu->flags & CSG);

	/* RFC 3720 Section 10.12.3 CSG and NSG
	 * "The next stage value is only valid when the T bit is 1;
	 * otherwise, it is reserved."
	 */
	if ((inputpdu->flags & T_BIT) &&
        iscsi_get_custom_value(conn->custom, "disable_t_bit") == 0)
    {
		outputpdu->flags |= (inputpdu->flags & NSG) | T_BIT;
	}

	if (*login_flags & FIRST_FLAG) {	
		/* this is the first login pdu of this login phase */
		retval = check_first_login(conn, inputpdu, outputpdu);
	} else {
		/* this is not the first login pdu of this login phase */
		retval = check_other_login(conn, correct_CSG, inputpdu, outputpdu);
	}
	if (retval < 0)
		goto out;

	if ((outputpdu->flags & CSG) == 0) {
		/* now in security parameter negotiation stage */
		what_to_process = SECURITY_PARAM | INFORMATIONAL_PARAM;
		if (noperational > 0) {
			/* we want to offer operational keys, force next stage to 1 */
			inputpdu->flags &= ~NSG;
			inputpdu->flags |= NSG1;
		}
	} else {					
		/* now in operational parameter negotiation stage */
		what_to_process = OPERATIONAL_PARAM | INFORMATIONAL_PARAM;
	}

	*count += 1;				/* to avoid infinite loops */
	if (*count >= LOOP_TIMES) {
		TRACE_ERROR("Infinite loop in parameter negotiations\n");
		login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_ERR, outputpdu);
		retval = -1;
		goto out;
	}

	/* process each key attached to input */
	add_length = scan_input_and_process(sock, p_param_tbl,
										what_to_process,
										TARGETNAME_FLAG | INITIATORNAME_FLAG |
										SESSIONTYPE_FLAG, TARGET,
										max_send_length, when_called, inputpdu,
										outputpdu, conn->connection_flags,
										login_flags, unknown_key_list);
	if (add_length < 0) {
        TRACE_ERROR("Cannot scan keys");
		login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_ERR, outputpdu);
		retval = add_length;
		goto out;
	}

	/* check that initial login includes initiator name, target name */
	if ((*login_flags) & FIRST_FLAG) {
		if (check_flags(conn, *login_flags, outputpdu, p_param_tbl) < 0) {
			retval = -1;
			goto out;
		}
	}

	outputpdu->text_length = add_length;

	/* scan through our table and make any offers */
	add_length = scan_table_and_process(sock, p_param_tbl,
										what_to_process, 0,
										TARGET,
										inputpdu, outputpdu,
										conn->connection_flags, login_flags);

	if (add_length < 0) {
        ERROR("Cannot scan keys in our table");
		login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_ERR, outputpdu);
		retval = add_length;
		goto out;
	}
	outputpdu->text_length += add_length;

	/*  after finishing attaching all keys to output */
	if (outputpdu->flags & T_BIT) {	
		/* check whether all parameters that were sent in this stage
		 * have received their responses 
		 */
		if (check_neg_responses(p_param_tbl, 0) < 0) {
			if (*count < LOOP_TIMES - 1) {	
				/* reset the T bit to be 0 for more negotiations */
				outputpdu->flags &= (~T_BIT);
				outputpdu->flags &= (~NSG);
			} else {			
				/* It seems the initiator doesn't reply the key */
				/* error */
				TRACE_ERROR("Target didn't receive all the responses\n");
				login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_ERR,
							 outputpdu);
				retval = -1;
				goto out;
			}
		} else {
			/* output.NSG = input.NSG, we will change to the next state now */
			outputpdu->flags &= (~NSG);
			outputpdu->flags |= inputpdu->flags & NSG;
		}
	} else {
		/* not changing the state */
		outputpdu->flags &= (~NSG);
	}

	outputpdu->status_class = 0;
	outputpdu->status_detail = 0;

	outputpdu->cmd_sn = htonl(conn->stat_sn + 1);
	outputpdu->exp_stat_sn = htonl(conn->session->exp_cmd_sn);
	outputpdu->max_cmd_sn = htonl(conn->session->max_cmd_sn);

	/* For a new session TSIH MUST be set by the target in the final
	   response and MUST be 0 otherwise */
	if (((inputpdu->tsih) == 0) && (!(inputpdu->flags & T_BIT)
									|| (inputpdu->flags & NSG) != NSG3
									|| !(outputpdu->flags & T_BIT)
									|| (outputpdu->flags & NSG) != NSG3)) {
		outputpdu->tsih = 0;
	} else {
		/* last LoginResponse in new session,
		 * check for session reinstatement and assign a new tsih to session
		 */
		if (inputpdu->tsih == 0)
			finalize_new_session(conn->session);
		outputpdu->tsih = htons(conn->session->tsih);
	}

out:
	TRACE(TRACE_DEBUG, "Leave target_check_login, retval %d", retval);

	return retval;
}

/*	states to drive authentication steps during security phase */
enum security_steps {
	ss_initial,
	ss_find_chap_a, 
	ss_find_chap_n_r, 
	ss_find_chap_i_c,
	ss_find_srp_u, 
	ss_find_srp_a_g, 
	ss_find_srp_m,
	ss_done, 
	ss_leave,
	ss_error
};

/*	returns 0 on success, -1 on error */
int
no_security_key_allowed(struct iscsi_conn *conn,
						struct generic_pdu *outputpdu,
						struct unknown_key *unknown_key_list)
{
	int retval = 0;
	struct unknown_key *key;

	for (key = unknown_key_list; key != NULL; key = key->next) {
		if (is_securitykey(key->keyname)) {
			print_not_allowed_security_key(key);
			retval = -1;
			login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_ERR,
						 outputpdu);
			goto out;
		}
	}
  out:
	return retval;
}

void
attach_key_int(struct generic_pdu *outputpdu, char *key_name, int key_int)
{
	int tmp_len;

	tmp_len = sprintf(outputpdu->text + outputpdu->text_length, "%s=%d",
					  key_name, key_int);
	TRACE(TRACE_ISCSI, "attach key %s",
		  outputpdu->text + outputpdu->text_length);
	outputpdu->text_length += tmp_len + 1;
}

void
attach_key_string(struct generic_pdu *outputpdu, char *key_name,
				  char *key_string)
{
	int tmp_len;

	tmp_len = sprintf(outputpdu->text + outputpdu->text_length, "%s=%s",
					  key_name, key_string);
	TRACE(TRACE_ISCSI, "attach key %s",
		  outputpdu->text + outputpdu->text_length);
	outputpdu->text_length += tmp_len + 1;
}

void
check_authmethod(struct parameter_type *auth_p,
				 struct generic_pdu *outputpdu,
				 enum security_steps *security_step)
{
	if (IS_KEY_GOT_FROM_OTHER_SIDE(auth_p->neg_info)) {	
		/* initiator has sent AuthMethod offer/reply, check it */
		if (!strcmp(auth_p->str_value, CHAP))
			*security_step = ss_find_chap_a;
		else if (!strcmp(auth_p->str_value, SRP))
			*security_step = ss_find_srp_u;
		else if (outputpdu->flags & T_BIT)
			*security_step = ss_leave;
		else
			*security_step = ss_done;
	} else if (outputpdu->flags & T_BIT)
		*security_step = ss_leave;
	/* else leave security_step value unchanged */
}

static int
target_security_negotiate(struct iscsi_conn *conn,
						  struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
						  struct generic_pdu *inputpdu,
						  struct generic_pdu *outputpdu,
						  uint32_t when_called,
						  uint64_t *login_flags,
						  int noperational,
						  struct auth_parameter_type auth_param,
						  struct unknown_key **unknown_key_list)
{
	int sock = conn->conn_socket;
	int retval = 0;
	int padding = 0;
	struct parameter_type *auth_p;
	char *dummy_string;
	struct unknown_key *key, *key_next, **key_prev;
	int count = 0;
	uint32_t neg_flags = 0, got_bitmask;
	char *chap_r = NULL;
	char *chap_c = NULL;
	int chap_a = 0;
	int chap_i = 0;
	char *chap_n;
	uint32_t value;
	enum security_steps security_step = ss_initial;
	int target_auth = 0;
	char *srp_grouplist = NULL;
	char *srp_s = NULL;
	char *srp_b = NULL;
	char *srp_u = NULL;
	char *srp_hm;

	ALLOCATE_MAX_TEXT_LEN(dummy_string);

	TRACE(TRACE_DEBUG, "Entering target security negotiate");
	if ((auth_p = find_flag_parameter(AUTHMETHOD_FLAG, p_param_tbl)) == NULL) {
		/* should NEVER happen */
		TRACE_ERROR("AuthMethod parameter not found\n");
		retval = -1;
		goto out;
	}
	if (target_check_login(conn,
						   p_param_tbl,
						   inputpdu,
						   outputpdu,
						   when_called,
						   noperational,
						   login_flags,
						   &count, unknown_key_list) < 0) {
		TRACE_ERROR("check login failed\n");
		retval = -1;
		goto out;
	}

	if (no_security_key_allowed(conn, outputpdu, *unknown_key_list))
		goto out;

	if (iscsi_send_msg_ex(conn, sock, outputpdu) < 0) 
    {
		TRACE(TRACE_DEBUG, "iscsi_send_msg failed");
		retval = -1;
		goto out;
	}

	outputpdu->text_length = 0;
	*login_flags &= ~FIRST_FLAG;
	check_authmethod(auth_p, outputpdu, &security_step);

	while ((outputpdu->flags & NSG) != NSG3) {
		if (iscsi_recv_msg(sock, ISCSI_HDR_LEN, (char *) inputpdu,
						   conn->connection_flags) < 0) {
			TRACE(TRACE_DEBUG, "iscsi_recv_msg failed");
			retval = -1;
			goto out;
		}

		inputpdu->text_length = ntohl(inputpdu->length);
		if (inputpdu->text_length > 0) {
			padding = (-inputpdu->text_length) & 3;
			if (inputpdu->text_length < MAX_TEXT_LEN) {
				if (iscsi_recv_msg(sock, inputpdu->text_length + padding,
							   	inputpdu->text, conn->connection_flags) < 0) {
					TRACE(TRACE_DEBUG, "iscsi_recv_msg failed");
					retval = -1;
					goto out;
				}
			} else {
				TRACE_ERROR
					("DSL %u greater than default MaxRecvDataSegmentLength %d\n",
				 	inputpdu->text_length, MAX_TEXT_LEN);
				retval = -1;
				goto out;
			}
		}

		/*  exit now if what we just read should not be in security stage */
		if (security_step == ss_leave)
			goto out;

		if (target_check_login(conn,
							   p_param_tbl,
							   inputpdu,
							   outputpdu,
							   when_called,
							   noperational,
							   login_flags,
							   &count, unknown_key_list) < 0) {
			TRACE_ERROR("check login failed\n");
			retval = -1;
			goto out;
		}

		TRACE(TRACE_ISCSI_FULL, "Target switch on security_step %d",
			  security_step);

		switch (security_step) {
		case ss_initial:
			/* looking to find AuthMethod=CHAP */
			if (no_security_key_allowed(conn, outputpdu, *unknown_key_list))
				goto out;

			check_authmethod(auth_p, outputpdu, &security_step);
			break;

		case ss_find_chap_a:
			/* looking to find CHAP_A */
			for (key = *(key_prev = unknown_key_list); key != NULL;
				 key = key_next) {
				key_next = key->next;
				if (!(got_bitmask = print_bad_security_key(key))) {
					key_prev = &key->next;
				} else {
					/* have a security key, is it allowed in this step? */
					if (got_bitmask == GOT_CHAP_A) {
						if (check_step_key(key, &neg_flags, GOT_CHAP_A)
							|| (chap_a =
								CHAP_SelectAlgorithm(key->keyvalue)) <= 0) {
							TRACE_ERROR("unable to select algorithm\n");
							retval = -1;
							login_reject(conn, STAT_CLASS_INITIATOR,
										 STAT_DETAIL_ERR, outputpdu);
							goto out;
						}
						TRACE(TRACE_ISCSI, "algorithm selected is %d",
							  chap_a);
						CHAP_SetAlgorithm(chap_a, auth_param.chap_local_ctx);
						CHAP_SetAlgorithm(chap_a, auth_param.chap_peer_ctx);
						security_step = ss_find_chap_n_r;
					} else {
						print_not_allowed_security_key(key);
						retval = -1;
						login_reject(conn, STAT_CLASS_INITIATOR,
									 STAT_DETAIL_ERR, outputpdu);
						goto out;
					}
					/* now delete this key from the list */
					free(key->keyvalue);
					free(key->keyname);
					ZFREE(key);
					*key_prev = key_next;
				}
			}

			if (neg_flags == GOT_CHAP_A) {
				attach_key_int(outputpdu, CHAP_A, chap_a);

				/*  generate id number and challenge string to send to him */
                chap_i = iscsi_get_custom_value(conn->custom, "CHAP_I");
                if (chap_i != 0)
                    CHAP_SetIdentifier(chap_i, auth_param.chap_local_ctx);
                else
                    chap_i = CHAP_GetIdentifier(auth_param.chap_local_ctx);
				attach_key_int(outputpdu, CHAP_I, chap_i);

				chap_c = CHAP_GetChallenge(auth_param.chap_local_ctx);
				if (chap_c == NULL) {
					TRACE_ERROR("CHAP exchange failed\n");
					goto out;
				}
				attach_key_string(outputpdu, CHAP_C, chap_c);

				/*  done with printable string version of our challenge */
				ZFREE(chap_c);
			}
			break;

		case ss_find_chap_n_r:
			/*  looking for CHAP_N, CHAP_R and maybe CHAP_I and CHAP_C */
			for (key = *(key_prev = unknown_key_list); key != NULL;
				 key = key_next) {
				key_next = key->next;
				if (!(got_bitmask = print_bad_security_key(key))) {
					key_prev = &key->next;
				} else {
					/* have a security key, is it allowed in this step? */
					if (got_bitmask == GOT_CHAP_N) {
						if (check_step_key(key, &neg_flags, GOT_CHAP_N)
							|| auth_param.chap_local_ctx->name == NULL
							|| strcmp(key->keyvalue,
									  auth_param.chap_local_ctx->name)) {
							TRACE_ERROR("security authentication failed\n");
                                                        TRACE_ERROR("key is %s->%s", key->keyname, key->keyvalue);
                                                        if (auth_param.chap_local_ctx->name == NULL)
                                                            TRACE_ERROR("name is null");
                                                        if (strcmp(key->keyvalue, auth_param.chap_local_ctx->name))
                                                            TRACE_ERROR("strcmp failed, %s, %s",
                                                                        key->keyvalue, auth_param.chap_local_ctx->name);
                                                        
							retval = -1;
							login_reject(conn, STAT_CLASS_INITIATOR,
										 STAT_DETAIL_NOT_AUTH, outputpdu);
							goto out;
						}
					} else if (got_bitmask == GOT_CHAP_R) {
						if (check_step_key(key, &neg_flags, GOT_CHAP_R)
							|| ((chap_r = malloc(strlen(key->keyvalue) + 1))) == NULL)
						{
							retval = -1;
							login_reject(conn, STAT_CLASS_INITIATOR,
										 STAT_DETAIL_NOT_AUTH, outputpdu);
							goto out;
						}
						strcpy(chap_r, key->keyvalue);
					} else if (got_bitmask == GOT_CHAP_I) {
						if (check_step_key_number(key, &neg_flags, GOT_CHAP_I,
												  255, &value)) {
							retval = -1;
							login_reject(conn, STAT_CLASS_INITIATOR,
										 STAT_DETAIL_ERR, outputpdu);
							goto out;
						}
                        chap_i = value; 
					} else if (got_bitmask == GOT_CHAP_C) {
						if (check_step_key(key, &neg_flags, GOT_CHAP_C)
							|| (chap_c = malloc(strlen(key->keyvalue) + 1)) == NULL)
						{
							retval = -1;
							login_reject(conn, STAT_CLASS_INITIATOR,
										 STAT_DETAIL_NOT_AUTH, outputpdu);
							goto out;
						}
						strcpy(chap_c, key->keyvalue);
					} else {
						print_not_allowed_security_key(key);
						retval = -1;
						login_reject(conn, STAT_CLASS_INITIATOR,
									 STAT_DETAIL_ERR, outputpdu);
						goto out;
					}
					/* now delete this key from the list */
					ZFREE(key->keyvalue);
					ZFREE(key->keyname);
					ZFREE(key);
					*key_prev = key_next;
				}
			}
			if ((neg_flags & (GOT_CHAP_A | GOT_CHAP_N | GOT_CHAP_R))
				== (GOT_CHAP_A | GOT_CHAP_N | GOT_CHAP_R)) {
					/* got both CHAP_N and CHAP_R, check response */
				if (CHAP_CheckResponse(chap_r, MAX_CHAP_BINARY_LENGTH,
									   auth_param.chap_local_ctx) <= 0) {
					TRACE_ERROR("security authentication failed\n");
					retval = -1;
					login_reject(conn, STAT_CLASS_INITIATOR,
								 STAT_DETAIL_NOT_AUTH, outputpdu);
					goto out;
				}
				/*  finished with his response to our challenge */
				ZFREE(chap_r);
			}
			if (neg_flags == (GOT_CHAP_A | GOT_CHAP_N | 
				GOT_CHAP_R | GOT_CHAP_I | GOT_CHAP_C)) {	
					/* got both CHAP_I and CHAP_C from initiator */
				if (!CHAP_CheckChallenge(chap_c, auth_param.chap_local_ctx)) {
					TRACE_ERROR("CHAP_C from Initiator duplicates one "
								"previously generated by target\n");
					retval = -1;
					login_reject(conn, STAT_CLASS_INITIATOR,
								 STAT_DETAIL_NOT_AUTH, outputpdu);
					goto out;
				}
				if ((chap_r = CHAP_GetResponse(chap_i, chap_c,
											   MAX_CHAP_BINARY_LENGTH,
											   auth_param.chap_peer_ctx)) ==
					NULL) {
					TRACE_ERROR("CHAP_R to Initiator cannot be generated\n");
					retval = -1;
					login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_ERR,
								 outputpdu);
					goto out;
				}
				if ((chap_n = CHAP_GetName(auth_param.chap_peer_ctx)) == NULL) {
					TRACE_ERROR("CHAP_N to Initiator not configured\n");
					retval = -1;
					login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_ERR,
								 outputpdu);
					goto out;
				}
				attach_key_string(outputpdu, CHAP_N, chap_n);

				/*  finished with our name to him */
				ZFREE(chap_n);

				attach_key_string(outputpdu, CHAP_R, chap_r);

				if (outputpdu->flags & T_BIT)
					security_step = ss_leave;
				else
					security_step = ss_done;
			} else {
				if (outputpdu->flags & T_BIT)
					security_step = ss_leave;
				else
					security_step = ss_find_chap_i_c;
			}
			break;

		case ss_find_srp_u:
			/* looking for SRP_U and TargetAuth */
			for (key = *(key_prev = unknown_key_list); key != NULL;
				 key = key_next) {
				key_next = key->next;
				if (!(got_bitmask = print_bad_security_key(key))) {
					key_prev = &key->next;
				} else {		
					/* have a security key, is it allowed in this step? */
					if (got_bitmask == GOT_SRP_U) {
						if (check_step_key(key, &neg_flags, GOT_SRP_U)
							|| !(srp_u =
								 SRP_Initiator_GetUsername(auth_param.
														   srp_ctx))) {
							TRACE_ERROR("unable to set SRP user name\n");
							retval = -1;
							login_reject(conn, STAT_CLASS_INITIATOR,
										 STAT_DETAIL_NOT_AUTH, outputpdu);
							goto out;
						}
						if (strcmp(srp_u, key->keyvalue)) {
							TRACE_ERROR("got SRP user name %s, expected %s\n",
										key->keyvalue, srp_u);
							retval = -1;
							login_reject(conn, STAT_CLASS_INITIATOR,
										 STAT_DETAIL_NOT_AUTH, outputpdu);
							goto out;
						}

					} else if (got_bitmask == GOT_SRP_TARGETAUTH) {
						if (check_step_key(key, &neg_flags, 
											GOT_SRP_TARGETAUTH)) {
							retval = -1;
							login_reject(conn, STAT_CLASS_INITIATOR,
										 STAT_DETAIL_NOT_AUTH, outputpdu);
							goto out;
						}
						if (!strcmp(key->keyvalue, YES))
							target_auth = 1;
						else if (!strcmp(key->keyvalue, NO))
							target_auth = 0;
						else {
							TRACE_ERROR("illegal value in %s=%s\n",
										key->keyname, key->keyvalue);
						}
					} else {
						print_not_allowed_security_key(key);
						retval = -1;
						login_reject(conn, STAT_CLASS_INITIATOR,
									 STAT_DETAIL_ERR, outputpdu);
						goto out;
					}

					/* now delete this key from the list */
					ZFREE(key->keyvalue);
					ZFREE(key->keyname);
					ZFREE(key);
					*key_prev = key_next;
				}
			}

			if ((neg_flags & (GOT_SRP_U | GOT_SRP_TARGETAUTH)) ==
						(GOT_SRP_U | GOT_SRP_TARGETAUTH)) {	
							/* got both SRP_U and SRP_TargetAuth,
							 * time to send SRP_GROUP and SRP_s 
							 */
				if (!(srp_grouplist =
					  SRP_Target_GetGroupList(auth_param.srp_ctx))) {
					TRACE_ERROR("unable to get SRP Group list\n");
					retval = -1;
					login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_ERR,
								 outputpdu);
					goto out;
				}
				attach_key_string(outputpdu, SRP_GROUP, srp_grouplist);

				if (!(srp_s = SRP_Target_GetSalt(auth_param.srp_ctx))) {
					TRACE_ERROR("unable to get SRP salt\n");
					retval = -1;
					login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_ERR,
								 outputpdu);
					goto out;
				}
				attach_key_string(outputpdu, SRP_S, srp_s);
				security_step = ss_find_srp_a_g;
			}
			break;

		case ss_find_srp_a_g:
			/* looking for SRP_A and SRP_GROUP */
			for (key = *(key_prev = unknown_key_list); key != NULL;
				 key = key_next) {
				key_next = key->next;
				if (!(got_bitmask = print_bad_security_key(key))) {
					key_prev = &key->next;
				} else {		
					/* have a security key, is it allowed in this step? */
					if (got_bitmask == GOT_SRP_A) {
						if (check_step_key(key, &neg_flags, GOT_SRP_A)
							|| !SRP_Target_SetA(key->keyvalue,
												MAX_SRP_BINARY_LENGTH,
												auth_param.srp_ctx)) {
							TRACE_ERROR("unable to set SRP_A\n");
							retval = -1;
							login_reject(conn, STAT_CLASS_INITIATOR,
										 STAT_DETAIL_NOT_AUTH, outputpdu);
							goto out;
						}
					} else if (got_bitmask == GOT_SRP_GROUP) {
						if (check_step_key(key, &neg_flags, GOT_SRP_GROUP)
							|| !SRP_SetSRPGroup(key->keyvalue,
												auth_param.srp_ctx)) {
							TRACE_ERROR("unable to set SRP group %s\n",
										key->keyvalue);
							retval = -1;
							login_reject(conn, STAT_CLASS_INITIATOR,
										 STAT_DETAIL_NOT_AUTH, outputpdu);
							goto out;
						}
					} else {
						print_not_allowed_security_key(key);
						retval = -1;
						login_reject(conn, STAT_CLASS_INITIATOR,
									 STAT_DETAIL_ERR, outputpdu);
						goto out;
					}

					/* now delete this key from the list */
					ZFREE(key->keyvalue);
					ZFREE(key->keyname);
					ZFREE(key);
					*key_prev = key_next;
				}
			}
			if ((neg_flags & (GOT_SRP_A | GOT_SRP_GROUP)) == 
					(GOT_SRP_A | GOT_SRP_GROUP)) {	
						/* got both SRP_A and SRP_GROUP, time to send SRP_B */
				if (!(srp_b = SRP_Target_GetB(auth_param.srp_ctx))) {
					TRACE_ERROR("unable to get SRP_B\n");
					retval = -1;
					login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_ERR,
								 outputpdu);
					goto out;
				}
				attach_key_string(outputpdu, SRP_B, srp_b);
				security_step = ss_find_srp_m;
			}
			break;

		case ss_find_srp_m:
			/* looking for SRP_M */
			for (key = *(key_prev = unknown_key_list); key != NULL;
				 key = key_next) {
				key_next = key->next;
				if (!(got_bitmask = print_bad_security_key(key))) {
					key_prev = &key->next;
				} else {
					/* have a security key, is it allowed in this step? */
					if (got_bitmask == GOT_SRP_M) {
						if (check_step_key(key, &neg_flags, GOT_SRP_M)
							|| !SRP_Target_SetM(key->keyvalue,
												MAX_SRP_BINARY_LENGTH,
												auth_param.srp_ctx)) {
							TRACE_ERROR("Authentication Failure\n");
							retval = -1;
							login_reject(conn, STAT_CLASS_INITIATOR,
										 STAT_DETAIL_NOT_AUTH, outputpdu);
							goto out;
						}
					} else {
						print_not_allowed_security_key(key);
						retval = -1;
						login_reject(conn, STAT_CLASS_INITIATOR,
									 STAT_DETAIL_ERR, outputpdu);
						goto out;
					}

					/* now delete this key from the list */
					ZFREE(key->keyvalue);
					ZFREE(key->keyname);
					ZFREE(key);
					*key_prev = key_next;
				}
			}
			if (neg_flags & GOT_SRP_M) {
				if (target_auth) {	
					/* previously got TargetAuth=Yes, send SRP_HM now */
					if (!(srp_hm = SRP_Target_GetHM(auth_param.srp_ctx))) {
						TRACE_ERROR("unable to get SRP_HM\n");
						retval = -1;
						login_reject(conn, STAT_CLASS_INITIATOR,
									 STAT_DETAIL_ERR, outputpdu);
						goto out;
					}
					attach_key_string(outputpdu, SRP_HM, srp_hm);
					ZFREE(srp_hm);
				}
				if (outputpdu->flags & T_BIT)
					security_step = ss_leave;
				else
					security_step = ss_done;
			}
			break;

		case ss_done:
			/* done with authentication, additional keys are illegal */
			if (no_security_key_allowed(conn, outputpdu, *unknown_key_list))
				goto out;
			if (outputpdu->flags & T_BIT)
				security_step = ss_leave;
			break;

		default:
			/* should NEVER happen */
			{
				TRACE_ERROR("Unknown security step %d\n", security_step);
				goto out;
			}

		}						/* switch */


        if (iscsi_send_msg_ex(conn, sock, outputpdu) < 0) 
        {
            TRACE(TRACE_DEBUG, "iscsi_send_msg failed");
            retval = -1;
            goto out;
        }

		outputpdu->text_length = 0;
	}

	/* end up security phase */

  out:
	ZFREE(chap_r);
	ZFREE(chap_c);
	ZFREE(srp_grouplist);
	ZFREE(srp_s);
	ZFREE(srp_b);
	ZFREE(srp_u);

	FREE_STRING(dummy_string);
	TRACE(TRACE_DEBUG, "Leaving target security negotiate");
	return retval;
}

/*
 * This function performs parameter negotiation on target side    
 * arguments: socket id, table of session parameters    
 * length to be read from the socket in case of first  
 * login command sent from the initiator to the target.
*/

static int
target_parameter_negotiate(struct iscsi_conn *conn, 
						struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
						struct generic_pdu *inputpdu, 
						struct generic_pdu *outputpdu, 
						uint32_t when_called, 
						struct auth_parameter_type auth_param,	
						struct unknown_key **unknown_key_list)
{
	int sock = conn->conn_socket;
	int retval = 0, count;
	int padding, nsecurity, ninformational, noperational;
	int correct_CSG;
	uint64_t login_flags = FIRST_FLAG;

	TRACE(TRACE_ENTER_LEAVE, "Entering target_parameter_negotiate");

	/* find out number of each type of key we want to offer */
	scan_table_and_count(p_param_tbl, &nsecurity, &ninformational,
						 &noperational);

	/* Include the padding bytes too  */
	padding = (-inputpdu->text_length) & 3;

	/* Get the input data - received parameter request */
	if (inputpdu->text_length > 0) {
		if (iscsi_recv_msg
			(sock, inputpdu->text_length + padding, inputpdu->text,
			 conn->connection_flags) < 0) {
			TRACE(TRACE_DEBUG, "iscsi_recv_msg failed");
			return -1;
		}
	}

	/* target always starts in the same state as initiator */
	correct_CSG = (inputpdu->flags & CSG) >> CSG_SHIFT;

	if ((nsecurity > 0 && (inputpdu->flags & CSG) != 0 )) {
        TRACE_ERROR("Incorrect CSG");
		login_reject(conn, STAT_CLASS_INITIATOR, STAT_DETAIL_ERR, outputpdu);
		return -1;
	}

	/* chap and srp support - CHONG */
	if ((inputpdu->flags & CSG) == 0) {
		if ((retval = target_security_negotiate(conn,
												p_param_tbl,
												inputpdu,
												outputpdu,
												when_called,
												&login_flags,
												noperational,
												auth_param,
												unknown_key_list)) < 0)
			goto out;

		correct_CSG = (inputpdu->flags & CSG) >> CSG_SHIFT;
	}

	/* see if already in Full Feature Phase */
	if ((outputpdu->flags & NSG) == NSG3)
		goto out;

	/* use count to control the loop times for login negotiation */
	count = 0;
	do {
		if (target_check_login(conn,
							   p_param_tbl,
							   inputpdu,
							   outputpdu,
							   when_called,
							   noperational,
							   &login_flags,
							   &count, unknown_key_list) < 0) {
			TRACE_ERROR("check login failed\n");
			retval = -1;
			goto out;
		}

		/* send out the output pdu */

        if (iscsi_send_msg_ex(conn, sock, outputpdu) < 0) 
        {
            TRACE(TRACE_DEBUG, "iscsi_send_msg failed");
            retval = -1;
            goto out;
        }

		/* after sending the output pdu, reset its length to 0 */
		outputpdu->text_length = 0;

		login_flags &= ~FIRST_FLAG;

		if (outputpdu->flags & T_BIT) {	
			/* both sides change state now after target sends a reply */

			/* Enable digests at end of login phase.
			 * RFC 3720 Section 12.1 HeaderDigest and DataDigest
			 * "When the Initiator and Target agree on a digest, this digest
			 * MUST be used for every PDU in Full Feature Phase."
			 */
			if ((outputpdu->flags & NSG) == NSG3) {	
				/* enable digests now */
				set_digestflags(p_param_tbl, &conn->connection_flags);

				/* go to full feature phase, end of login phase */
				goto out;
			}

			/* change to a new (operational) state */
			outputpdu->flags &= (~CSG);
			outputpdu->flags |= ((inputpdu->flags & NSG) << CSG_SHIFT);
		}

		/* keep the last outputpdu.CSG, since that is the state we are in now */
		correct_CSG = (outputpdu->flags & CSG) >> CSG_SHIFT;

		/*  wait for input from initiator  */
		if (iscsi_recv_msg(sock, ISCSI_HDR_LEN, (char *) inputpdu,
						   conn->connection_flags) < 0) {
			TRACE(TRACE_DEBUG, "iscsi_recv_msg failed");
			return -1;
		}

        print_init_login_cmnd((struct iscsi_init_login_cmnd *) inputpdu);

		/*  Also get the text associated with it  */
		inputpdu->text_length = ntohl(inputpdu->length);

		/* Include the padding bytes too */
		padding = -inputpdu->text_length & 3;

		if (inputpdu->text_length <= MAX_TEXT_LEN) {
			if (iscsi_recv_msg(sock, inputpdu->text_length + padding,
							   inputpdu->text, conn->connection_flags) < 0) {
				TRACE(TRACE_DEBUG, "iscsi_recv_msg failed");
				retval = -1;
				goto out;
			}
		} else {
			TRACE_ERROR
				("DSL %u greater than default MaxRecvDataSegmentLength %d\n",
				 inputpdu->text_length, MAX_TEXT_LEN);
			retval = -1;
			goto out;
		}

	} while (1);

out:
	TRACE(TRACE_ENTER_LEAVE, "Leave target_parameter_negotiate, retval %d",
		  retval);
	return retval;
}

int
parameter_negotiate(struct iscsi_conn *conn,
					struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
					struct iscsi_init_login_cmnd *loginpdu,
					uint32_t when_called,
					struct auth_parameter_type auth_param)
{
	int retval = 0;
	struct generic_pdu *inputpdu, *outputpdu;
	struct unknown_key *uptr, *unknown_key_list = NULL;

	TRACE(TRACE_ENTER_LEAVE, "Enter parameter_negotiate");

	if ((inputpdu =
		 malloc(sizeof (struct generic_pdu))) == NULL) {
		retval = -1;
		goto out;
	}

	if ((outputpdu =
		malloc(sizeof (struct generic_pdu))) == NULL) {
		ZFREE(inputpdu);
		retval = -1;
		goto out;
	}

	/* Get memory for text portion of inputpdu and outputpdu */
	if ((inputpdu->text = malloc(MAX_TEXT_LEN)) == NULL) {
		ZFREE(outputpdu);
		ZFREE(inputpdu);
		retval = -1;
		goto out;
	}

	if ((outputpdu->text = malloc(MAX_TEXT_LEN)) == NULL) {
		ZFREE(inputpdu->text);
		ZFREE(outputpdu);
		ZFREE(inputpdu);
		retval = -1;
		goto out;
	}

	/* fill inputpdu with copy of login pdu header received from initiator */
	memcpy(inputpdu, loginpdu, ISCSI_HDR_LEN);
	inputpdu->text_length = loginpdu->length;	/* still have to read this! */

	/* reset outputpdu to all 0 so reserved fields will be 0 */
	memset(outputpdu, 0, ISCSI_HDR_LEN);
	outputpdu->text_length = 0;

	/* set the initiator task tag */
	outputpdu->init_task_tag = htonl(loginpdu->init_task_tag);

	/*set the ISID and TSIH for this session */
	memcpy(outputpdu->isid, conn->session->isid, 6);
	outputpdu->tsih = htons(conn->session->tsih);

	/* Set opcode */
	outputpdu->opcode = ISCSI_TARG_LOGIN_RSP;

	/* set the version_max and version_active for the session  */
	outputpdu->version_max = conn->session->version_max;
	outputpdu->version_active = conn->session->version_min;

	retval = target_parameter_negotiate(conn,
										p_param_tbl,
										inputpdu,
										outputpdu,
										when_called,
										auth_param,
										&unknown_key_list);

	/* Draft 20, Section 5.2 Text Mode Negotiation
	   "Some parameters may be subject to integrity rules (e.g., parameter-x
	   must not exceed parameter-y or parameter-u not 1 implies parameter-v
	   be Yes).  Whenever required, integrity rules are specified with the
	   keys.  Checking for compliance with the integrity rule must only be
	   performed after all the parameters are available (the existent and the
	   newly negotiated).  An iSCSI target MUST perform integrity checking
	   before the new parameters take effect.  An initiator MAY perform
	   integrity checking."
	 */
	check_integrity_rules(p_param_tbl, inputpdu->tsih);

	/*  if the target sent a MaxRecvDataSegmentLength (draft 13)
	   value to the target, we have to now start using it in FFP 
	 */
	set_connection_recv_length(p_param_tbl, &conn->max_recv_length);

	/* free up memory allocated for any unknown keys we received */
	while (unknown_key_list != NULL) {
		uptr = unknown_key_list->next;
		ZFREE(unknown_key_list->keyvalue);
		ZFREE(unknown_key_list->keyname);
		ZFREE(unknown_key_list);
		unknown_key_list = uptr;
	}

	/* Free all allocated memory */
	ZFREE(outputpdu->text);
	ZFREE(inputpdu->text);
	ZFREE(outputpdu);
	ZFREE(inputpdu);

out:
	TRACE(TRACE_ENTER_LEAVE, "Leave parameter_negotiate, retval %d", retval);
	return retval;
}

/* turn off the KEY_TO_BE_NEGOTIATED flag in leading-only keys
 * (so they can be used in negotations for a 2nd, 3rd, ... connection
 * in an existing session).
 */
void
reset_parameter_table(struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS])
{
	struct parameter_type *p = NULL;
	int i;

	TRACE(TRACE_ENTER_LEAVE, "Enter reset_parameter_table");

	for (i = 0, p = p_param_tbl; i < MAX_CONFIG_PARAMS; i++, p++) {
		if (IS_LEADING_ONLY(p->type))
			p->neg_info &= ~KEY_TO_BE_NEGOTIATED;
	}

	TRACE(TRACE_ENTER_LEAVE, "Leave reset_parameter_table");
}

/* Ming Zhang, mingz@ele.uri.edu */
#ifdef K26
EXPORT_SYMBOL(reset_parameter_table);
EXPORT_SYMBOL(parameter_negotiate);
#endif
