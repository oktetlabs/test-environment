/*	target/target_error_rec.c 

    vi: set autoindent tabstop=4 shiftwidth=4 :

*/

/*
	Copyright (C) 2001-2003 InterOperability Lab (IOL)
	University of New Hampshier (UNH)
	Durham, NH 03824

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
	USA.

	The name of IOL and/or UNH may not be used to endorse or promote products
	derived from this software without specific prior written permission.
*/

#include <te_config.h>

#include "iscsi_target.h"
#include "target_error_rec.h"

/************************************************************************
 *  The Main Error Recovery Handler Called from iscsi_initiator when it *
 *  finds any error.                                                    *
 *  Returns 0 if recovery completed ok, -1 otherwise                    *
 ************************************************************************/

int
targ_do_error_recovery(struct targ_error_rec *err_rec)
{

	int retval = 0;
	struct iscsi_conn *curr_conn;

	TRACE(DEBUG, "Entering targ_do_error_recovery\n");

	if (err_rec == NULL) {
		TRACE_ERROR("Error Recovery structure is NULL\n");
		retval = -1;
		goto out;
	}

	if (err_rec->curr_conn == NULL) {
		TRACE_ERROR("NULL Connection Pointer in Error Recovery structure\n");
		retval = -1;
		goto out;
	}

	curr_conn = err_rec->curr_conn;

	switch (curr_conn->session->oper_param->ErrorRecoveryLevel) {
        case SESSION_RECOVERY:
            targ_session_recovery(curr_conn);
            break;
        case DIGEST_RECOVERY:
            retval = targ_digest_recovery(err_rec);
            break;
        default:
            TRACE_ERROR("ErrorRecoveryLevel %u Not Implemented yet\n",
                        curr_conn->session->oper_param->ErrorRecoveryLevel);
            retval = -1;
            break;
	}

out:
	TRACE(DEBUG, "Leaving targ_do_error_recovery, retval %d\n",
		retval);
	return retval;
}

/************************************************************************
* This function handles the session recovery. It creates a recovery     *
* thread that will do session recovery.                                 *
************************************************************************/

void
targ_session_recovery(struct iscsi_conn *current_connection)
{
	struct iscsi_session *sess;

	TRACE(DEBUG, "Enter targ_session_recovery\n");

	TRACE(NORMAL,
		  "Executing Target Session Recovery - cancelling Receive Thread\n");

	sess = current_connection->session;

    iscsi_release_session(sess);

	TRACE(DEBUG, "Leave targ_session_recovery\n");

	return;
}

/************************************************************************
 * This function handles the digest errors and sequence errors that may *
 * have caused because of previous digest errors.                       *
 * Returns 0 if recovery is completed correctly, -1 otherwise           *
 ************************************************************************/

int
targ_digest_recovery(struct targ_error_rec *err_rec)
{
	int retval = 0, data_length = 0;
	int size = 0, err_type = 0, data_offset = 0;
	unsigned int opcode = 0;
	struct iscsi_conn *curr_conn;
	struct iscsi_init_scsi_data_out *data_out_hdr;
	struct iscsi_cmnd *cmd;

	curr_conn = err_rec->curr_conn;
	err_type = err_rec->err_type;
	cmd = err_rec->cmd;

	TRACE(DEBUG, "Entering targ_digest_recovery\n");

	if (err_rec->pdu_hdr == NULL) {
		TRACE_ERROR("iscsi NULL PDU Header\n");
		retval = -1;
		goto digest_out;
	} else {
		opcode = err_rec->pdu_hdr->opcode;
		size = err_rec->pdu_hdr->length;
		if (size > MASK_24_BITS) {
			TRACE_ERROR("TotalAHSLength 0x%02x, expected 0x00\n",
						size >> 24);
			retval = -1;
			goto digest_out;
		}
	}

	if (err_type == HEADER_DIGERR) {
		if (size > 0) {
			retval = targ_drop_pdu_data(curr_conn, size);
		}
		goto digest_out;
	}

	if (opcode == ISCSI_INIT_SCSI_DATA_OUT) {
		if (cmd == NULL) {
			retval = -1;
			goto digest_out;
		}

		if (err_type == PAYLOAD_DIGERR) {
			enqueue_reject(curr_conn, REASON_DATA_DIGEST_ERR);
			retval = send_recovery_r2t(cmd, cmd->data_done, NULL,
						   err_rec->pdu_hdr);
		} else if (err_type == SEQUENCE_ERR) {
			/* error recovery ver ref18_04 - SAI */
			/* if there is a sequence error and the arrived data 
			 * seq. indicates lost packets, queue the received 
			 * packets and request re-transmission of lost packets 
			 */
			queue_data(err_rec);

			data_length = cmd->data_length - cmd->data_done;
			data_offset = cmd->data_length - data_length;

			data_out_hdr = (struct iscsi_init_scsi_data_out *) err_rec->pdu_hdr;
			size = (data_out_hdr->offset - data_offset);

			retval = send_recovery_r2t(cmd, data_offset, NULL,
						   err_rec->pdu_hdr);

		} else {
			TRACE_ERROR("Unknown err_type %d for DataOut recovery\n",
						err_type);
			retval = -1;
		}

	} else if (opcode == ISCSI_INIT_SCSI_CMND) {
		if (err_type == PAYLOAD_DIGERR) {
			/* digest error in immediate data attached to write */
			enqueue_reject(curr_conn, REASON_DATA_DIGEST_ERR);
		} else {
			TRACE_ERROR("Unknown err_type %d for SCSI_CMND recovery\n",
						err_type);
			retval = -1;
		}
	} else {
		TRACE_ERROR("nknown err_type %d for recovery\n", 
					err_type);
		retval = -1;
	}

digest_out:

	TRACE(DEBUG, "Leaving targ_digest_recovery, retval %d\n",
		retval);

	return retval;
}

/*******************************************************************
 * Reads the data portion of the dropped PDU due to digest errors. *
 * The data read is also ignored.                                  *
********************************************************************/

int
targ_drop_pdu_data(struct iscsi_conn *curr_conn, int size)
{
    char *buffer = malloc(size);
    int retval;
	int padding = 0;

    if (!buffer) {
        TRACE_ERROR("Malloc Error: Unable to drop the data PDU\n");
        return ENOMEM;
    }
    
	padding = -(size) & 3;
	size += padding;

	if (curr_conn->data_crc == 1) {
		size += CRC_LEN;
	}

    retval = iscsi_recv_msg(curr_conn->conn_socket, 
                            size, buffer, curr_conn->connection_flags);
    free(buffer);
    return retval;
}

/***************************************************************************
* create an R2T cookie and store the R2T details in the cookie. This cookie
* is esssential for R2T re-transmssions in error conditions.
***************************************************************************/
struct iscsi_cookie *
create_r2t_cookie(struct iscsi_cmnd *cmnd)
{
	struct iscsi_cookie *cookie = NULL;

	TRACE(DEBUG, "Enter create_r2t_cookie\n");

	cookie = malloc(sizeof(struct iscsi_cookie));
	if (cookie == NULL) {
		TRACE_ERROR("Malloc problem in creating R2T cookie\n");
		goto end_iscsi_cookie;
	}
	cookie->next = NULL;

	if (cmnd->first_r2t_cookie == NULL) {
		cmnd->first_r2t_cookie = cmnd->last_r2t_cookie = cookie;
	} else {
		cmnd->last_r2t_cookie->next = cookie;
		cmnd->last_r2t_cookie = cookie;
	}

end_iscsi_cookie:

	TRACE(DEBUG, "Leave create_r2t_cookie\n");

	return cookie;
}

/* free the long pending R2T cookie list after the command is completed - SAI */
void
free_r2t_cookie(struct iscsi_cmnd *cmnd)
{
	struct iscsi_cookie *cookie = cmnd->first_r2t_cookie;
	struct iscsi_cookie *tmp_cookie = NULL;

	TRACE(DEBUG, "Enter free_r2t_cookie\n");

	while (cookie) {
		tmp_cookie = cookie;
		cookie = cookie->next;
		free(tmp_cookie);
	}

	TRACE(DEBUG, "Leave free_r2t_cookie\n");

	return;

}


/*******************************************************************************
* This function uses the offset and data length parameter or cookie information
* to build a recovery R2T to be sent to the intiator for missing Dataouts. This
* function is called by SNACK mechanism and also by other places in the code
* that find a discrepency in Dataouts using data sequence numbering or offsets.
* with regard to R2TSN updating for recovery R2Ts:
*
* RFC 3720 Section 2.1. Definitions
* "- Recovery R2T: An R2T generated by a target upon detecting the loss
*    of one or more Data-Out PDUs through one of the following means: a
*    digest error, a sequence error, or a sequence reception timeout.  A
*    recovery R2T carries the next unused R2TSN, but requests all or
*    part of the data burst that an earlier R2T (with a lower R2TSN) had
*    already requested."
********************************************************************************/
int
send_recovery_r2t(struct iscsi_cmnd *cmnd, int data_offset,
		  struct iscsi_cookie *cookie, struct generic_pdu *hdr)
{
	int data_length;
	int max_burst_len;
	int err = 0;

	TRACE(DEBUG, "Enter send_recovery_r2t, recovery_r2t %u\n",
	      cmnd->recovery_r2t);

	if (cmnd->recovery_r2t) {
		/* don't send recovery_r2t for another error */
		if (hdr->flags & F_BIT) {
			/* last DataOut in sequence to be recovered */
			cmnd->recovery_r2t = 2;
			cmnd->next_burst_len = 0;
			cmnd->data_sn = 0;
		}
		goto out;
	}

	max_burst_len = cmnd->conn->session->oper_param->MaxBurstLength;
	data_length = cmnd->data_length - data_offset;

	if (!cookie) {
		cmnd->r2t_data = data_length;
		cookie = create_r2t_cookie(cmnd);
		if (!cookie) {
			err = -1;
			goto out;
		}
		cmnd->startsn = cmnd->endsn = cmnd->r2t_sn;
		cookie->seq = cmnd->r2t_sn++;	/* recovery R2T stays in R2T sequence */
		cookie->offset = data_offset;
		cookie->xfer_len = (data_length <= max_burst_len)
					? data_length : max_burst_len;
	} else {
		data_offset = cookie->offset;
		cmnd->r2t_data = cmnd->data_length - data_offset;
		cmnd->r2t_sn = cookie->seq;
		cmnd->startsn = cmnd->endsn = cmnd->r2t_sn;
		cmnd->outstanding_r2t--;
	}

	TRACE_WARNING("Send recovery R2T, ITT %u R2TSN %u Buffer Offset %d\n",
				   cmnd->init_task_tag, cmnd->r2t_sn,
				   data_offset);
	if (hdr->flags & F_BIT) {
		cmnd->recovery_r2t = 2;
		cmnd->next_burst_len = 0;
		cmnd->data_sn = 0;
	} else {
		cmnd->recovery_r2t = 1;
	}
	cmnd->state = ISCSI_BUFFER_RDY;

out:
	TRACE(DEBUG, "Leave send_recovery_r2t, recovery_r2t %u, "
	      "err = %d\n", cmnd->recovery_r2t, err);
	return err;
}

/***********************************************************************
* This thread remains blocked on a semaphore for most of the time.
* When a r2t timeout happens this thread is awakened for re-transmitting
* any r2t requests that have not generated any activity in a long time.
***********************************************************************/

void *
iscsi_retran_thread(void *param)
{
    UNUSED(param);
	struct iscsi_session *session = (struct iscsi_session *) param;
	struct iscsi_cmnd *cmnd;

    for (;;)
    {
        
        sleep(session->r2t_period);
        pthread_testcancel();

		/* lock the session-wide list of commands */
        pthread_mutex_lock(&session->cmnd_mutex);
		for (cmnd = session->cmnd_list; cmnd; cmnd = cmnd->next) {
			if (cmnd->outstanding_r2t > 0
			   && time(NULL) > (time_t)(cmnd->timestamp + session->r2t_period)
			   && cmnd->state == (uint8_t)ISCSI_BUFFER_RDY
			   && !cmnd->retransmit_flg) {
				/* this is a WRITE command with outstanding
				 * R2Ts and no activity for a while.  Try
				 * retransmitting the last R2T.
				 */
				TRACE(VERBOSE,
				      "activity timeout, ITT %u, "
				      "recovery_r2t %u\n",
				      cmnd->init_task_tag,
				      cmnd->recovery_r2t);
				if (cmnd->recovery_r2t) {
					if (cmnd->recovery_r2t == 1)
						cmnd->recovery_r2t = 2;
				} else {
					cmnd->retransmit_flg = 1;
					cmnd->endsn = cmnd->startsn
							= cmnd->r2t_sn - 1;
				}
				/* signal the tx thread to do the rexmit */
                iscsi_tx(cmnd->conn);
			}
		}
        pthread_mutex_unlock(&session->cmnd_mutex);
	}
	return 0;
}

void
add_data_to_queue(struct iscsi_cmnd *cmd, struct iscsi_cookie *dataq)
{
	TRACE(DEBUG, "Entering add_data_to_queue\n");

	if (!cmd->first_data_q) {
		cmd->first_data_q = dataq;
		cmd->last_data_q = dataq;
	} else {
		cmd->last_data_q->next = dataq;
		cmd->last_data_q = cmd->last_data_q->next;
	}

	TRACE(DEBUG, "Leaving add_data_to_queue\n");

	return;
}

/********************************************************************
* This function drives the out-of-order dataouts received into the 
* appropriate scatter gather buffer location maintained by scsi. The
* information about the out-of-order dataout and also the corresponding
* scatter offsets are queued for future use.
**********************************************************************/
int
queue_data(struct targ_error_rec *err_rec)
{
    UNUSED(err_rec);
#if 0
	int offset = 0, size = 0, list_offset = 0;
	int count = 0;
	int niov, padding, err, retval = 0, data_crc_len = 0;
	uint32_t data_sn, digest, pad_bytes;

	struct iscsi_conn *conn = NULL;
	struct iscsi_cmnd *cmd;
	struct iscsi_init_scsi_data_out *hdr;
	struct iscsi_cookie *dataq = NULL;
	struct scatterlist *st_list;
	struct iovec *iov = NULL;

	TRACE(DEBUG, "Entering queue_data\n");

	cmd = err_rec->cmd;
	hdr = (struct iscsi_init_scsi_data_out *) err_rec->pdu_hdr;

	list_offset = offset = hdr->offset;
	data_sn = hdr->data_sn;
	size = hdr->length;
	conn = err_rec->curr_conn;

	st_list = (struct scatterlist *) cmd->cmnd->req->sr_buffer;

	while (st_list[count].length <= list_offset) {
		list_offset -= st_list[count].length;
		count++;
	}

	st_list += count;
	niov = find_iovec_needed(size, st_list, list_offset);

	if (niov < 0) {
		TRACE_ERROR("queue data: find_iovec_needed returned an error\n");
		return -1;
	}

	padding = -(size) & 3;
	if (padding)
		niov++;

	if (conn->data_crc == 1)
		niov++;

	iov = (struct iovec *) malloc(niov * sizeof(struct iovec));

	if (!iov) {
		TRACE_ERROR("queue_data: Could not malloc iov\n");
		return -1;
	}

	TRACE(DEBUG, "queue_data: no. of iovec needed %d\n", niov);

	retval = fill_iovec(iov, 0, niov, st_list, &list_offset, size);

	if (padding) {
		iov[niov - 1 - conn->data_crc].iov_base = &pad_bytes;
		iov[niov - 1 - conn->data_crc].iov_len = padding;
	}

	if (conn->data_crc == 1) {
		data_crc_len = CRC_LEN;
		iov[niov - 1].iov_base = &digest;
		iov[niov - 1].iov_len = CRC_LEN;
	}

	err = iscsi_rx_data(cmd->conn, iov, niov, size + padding + data_crc_len);

	if (err != (size + padding + data_crc_len)) {
		TRACE_ERROR("queue_data: Trouble in iscsi_rx_data\n");
		if (err == PAYLOAD_DIGERR)
			return err;

	}

	dataq = malloc(sizeof(struct iscsi_cookie));

	if (!dataq) {
		TRACE_ERROR("add_data_to_queue: Malloc returned an error\n");
		return -1;
	}

	dataq->offset = offset;
	dataq->xfer_len = size;
	dataq->list_offset = list_offset;
	dataq->list_count = retval;
	dataq->next = NULL;

	add_data_to_queue(cmd, dataq);

	TRACE(DEBUG, "Leaving queue_data\n");
#endif

	return 0;
}

/***********************************************************************
* This function is called everytime a in-sequence dataout pdu is
* received. The function verifies if there was any previous
* out-of-order dataouts for the command and updates the state variables
* accordingly.
***********************************************************************/
void
search_data_q(struct iscsi_cmnd *cmd)
{
	int unhook_flg = 0;
	struct iscsi_cookie *dataq, *prev, *tmp_q;

	TRACE(DEBUG, "Entering search_data_q\n");

	if (!cmd->first_data_q)
		return;

	dataq = prev = cmd->first_data_q;

	while (dataq) {
		if (cmd->data_done > (uint32_t)dataq->offset)
			unhook_flg = 1;
		else if (cmd->data_done == (uint32_t)dataq->offset) {
			cmd->scatter_list_count += dataq->list_count;
			cmd->scatter_list_offset = dataq->list_offset;
			cmd->data_done += dataq->xfer_len;
			unhook_flg = 1;
		}

		if (unhook_flg) {
			/* unhook data node */
			if (prev == cmd->first_data_q) {
				cmd->first_data_q = dataq->next;
				prev = dataq->next;
			} else if (dataq == cmd->last_data_q)
				cmd->last_data_q = prev;
			else
				prev->next = dataq->next;

			tmp_q = dataq;
			dataq = dataq->next;
			free(tmp_q);
			unhook_flg = 0;
		} else {
			prev = dataq;
			dataq = dataq->next;
		}

		if (cmd->data_done >= cmd->data_length)
			break;
	}

	TRACE(DEBUG, "Leaving search_data_q\n");
}
