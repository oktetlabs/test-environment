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
#include <scsi_cmnd.h>
#include <scsi_request.h>
#include "iscsi_target.h"
#include "iscsi_portal_group.h"
#include "target_error_rec.h"

#include <my_memory.h>

#include "my_login.h"

/* 
 * Declaration of send/recv methods from iSCSI TAD, see 
 * tad_iscsi_{impl.h|stack.c}
 */
extern int iscsi_tad_recv(int csap, uint8_t *buffer, size_t len);
extern int iscsi_tad_send(int csap, uint8_t *buffer, size_t len);

/** Pointer to the device specific data */
struct iscsi_global *devdata;

static void check_queued_cmnd(struct iscsi_session *session);
static void update_after_read(struct generic_pdu *hdr, struct iscsi_cmnd *cmnd);
static int check_cmd_sn(struct iscsi_cmnd *cmnd, void *ptr, struct iscsi_session *session,
                        uint32_t increment);
static int ack_sent_cmnds(struct iscsi_conn *conn, struct iscsi_cmnd *cmnd,
                          uint32_t exp_stat_sn, te_bool add_cmnd_to_queue);

int fill_iovec(struct iovec *iov, int p, int niov,
               struct scatterlist *st_list, int *offset, uint32_t data);
int find_iovec_needed(uint32_t data_len, struct scatterlist *st_list, uint32_t offset);

/*
 * RDR
 * allocate and zero out a new struct iscsi_cmnd.
 * Returns pointer to the new struct if all ok, else NULL after error message
 */
struct iscsi_cmnd *
get_new_cmnd( void )
{
	struct iscsi_cmnd *cmnd;

	cmnd = calloc(1, sizeof(*cmnd));

	if (cmnd) 
    {
		cmnd->state = ISCSI_NEW_CMND;
	}
	return cmnd;
}


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
    free(conn->local_ip_address);                                    
    free(conn->ip_address);                                          
#endif                                                                            
    free(conn);
    return 0;                                                                     
}

static void
free_data_list(struct iscsi_cmnd *cmnd)
{
    struct data_list *data;

    while ((data = cmnd->unsolicited_data_head)) {
        cmnd->unsolicited_data_head = data->next;
        free(data->buffer);
        free(data);
    }
}

static int
iscsi_recv_iov (int csap, struct iovec *iov, int niov)
{
    int total = 0;
    int received;
    
    for (received = 1; 
         niov != 0 && received != 0; 
         niov--, iov++, total += received)
    {
        received = iscsi_tad_recv(csap, iov->iov_base, iov->iov_len);
        if (received < 0)
            return received;
    }
    return total;
}

/*
 * iscsi_rx_data: will receive a fixed amount of data.
 * INPUT: struct iscsi_conn (what connection), struct iovec, number of iovecs,
 * total amount of data in bytes
 * OUTPUT: > 0 total bytes read if everything is okay
 * 	       < 0 if trouble (-ECONNRESET means end-of-file)
 *		   = 0 for PAYLOAD_DIGERR
 */
int
iscsi_rx_data(struct iscsi_conn *conn, struct iovec *iov, int niov, int data)
{
	int msg_iovlen;
	int rx_loop, total_rx;
	uint32_t checksum, data_crc;
	int i;
	struct iovec *iov_copy, *iov_ptr;

	total_rx = 0;

	TRACE(TRACE_ENTER_LEAVE, "Enter iscsi_rx_data, niov %d, data %d\n",
		  niov, data);

	if (!conn->conn_socket) {
		TRACE_ERROR("Transport endpoint is not connected\n");
		total_rx = -ENOTCONN;		/* Transport endpoint is not connected */
		goto out;
	}

	iov_copy = malloc(niov * sizeof(struct iovec));
	if (iov_copy == NULL) {
		total_rx = -ENOMEM;
		goto out;
	}

	while (total_rx < data) {
		/* get a clean copy of the original io vector to work with */
		memcpy(iov_copy, iov, niov * sizeof(struct iovec));
		msg_iovlen = niov;
		iov_ptr = iov_copy;

		if ((rx_loop = total_rx)) {
			/* have done only partial read, recalculate iov and niov -- cdeng */
			TRACE(TRACE_ISCSI,
				  "iscsi_rx_data: data %d, received so far %d, recompute iov\n",
				  data, total_rx);
			do {
				if (iov_ptr->iov_len <= (unsigned)rx_loop) {
					rx_loop -= iov_ptr->iov_len;
					iov_ptr++;
					msg_iovlen--;
				} else {
					iov_ptr->iov_base += rx_loop;
					iov_ptr->iov_len -= rx_loop;
					rx_loop = 0;
				}
			} while (rx_loop);
		}

		rx_loop = iscsi_recv_iov(conn->conn_socket, iov_ptr, msg_iovlen);

		/* this receive from initiator broke the silence */
        conn->control &= ~SILENCE_BIT;

		if (rx_loop <= 0) {
			free(iov_copy);
			total_rx = -ECONNRESET;		/* Connection reset by peer */
			goto out;
		}

		total_rx += rx_loop;
		TRACE(TRACE_DEBUG, "iscsi_rx_data: rx_loop %d total_rx %d\n", rx_loop,
			  total_rx);
	}

	free(iov_copy);

	if (niov > 1 && conn->data_crc) {
		/* this really is for a data segment, and data digests are in effect */
		data_crc = 0;
		for (i = 0; i < niov - 1; i++)
			do_crc(iov[i].iov_base, iov[i].iov_len, &data_crc);
		checksum = *(uint32_t *) (iov[niov - 1].iov_base);
		if (checksum != data_crc) {
			TRACE_ERROR("Got data crc 0x%08x, expected 0x%08x\n",
						ntohl(checksum), ntohl(data_crc));
			/* Data Digest Error Recovery - SAI */
			total_rx = PAYLOAD_DIGERR;
			goto out;
		} else {
			TRACE(TRACE_ISCSI_FULL, "Got data crc 0x%08x\n", ntohl(checksum));
		}
	}
out:
	TRACE(TRACE_ENTER_LEAVE, "Leave iscsi_rx_data, total_rx %d\n", total_rx);

	return total_rx;
}


/* read data segment from cmd's connection into a single buffer 
 * that is newly allocated to hold bufsize bytes of data
 * returns > 0 total number of bytes read (including pad & crc) if all ok,
 *		   = 0 if recovered from payload digest error ok
 *		   < 0 error
 * if ok, sets result to newly allocated buffer, else leaves result unchanged
 */
static int
read_single_data_seg(uint8_t *buffer, struct iscsi_cmnd *cmd,
					 int bufsize, char **result)
{
	struct iovec iov[3];
	int size, niov = 1, padding, err;
	uint32_t digest, pad_bytes;
	char *data_buf;
	struct targ_error_rec err_rec;

	size = bufsize;

	padding = (-size) & 3;
	if (padding) {
		iov[niov].iov_base = &pad_bytes;
		iov[niov].iov_len = padding;
		size += padding;
		niov++;
	}

	if (cmd->conn->data_crc) {
		iov[niov].iov_base = &digest;
		iov[niov].iov_len = CRC_LEN;
		size += CRC_LEN;
		niov++;
	}

	if ((data_buf = malloc(bufsize))) {
		iov[0].iov_base = data_buf;
		iov[0].iov_len = bufsize;

		err = iscsi_rx_data(cmd->conn, iov, niov, size);

		if (err != size) {
			/* Payload Digest Error Recovery - SAI */
			if (err == PAYLOAD_DIGERR) {
				TRACE(TRACE_ERROR_RECOVERY,
					  "Start payload digest error recovery\n");
				err_rec.curr_conn = cmd->conn;
				err_rec.pdu_hdr = (struct generic_pdu *)buffer;
				err_rec.cmd = cmd;
				err_rec.err_type = PAYLOAD_DIGERR;
				err = targ_do_error_recovery(&err_rec);
			}
			ZFREE(data_buf);
		} else {
			*result = data_buf;
		}
	} else {
		err = -1;
	}
	return err;
}


/* 
 * executed only by the rx thread.
 * cdeng: called by handle_cmnd and handle_data if the 
 * command is out of order.  Store the data into data_list in 
 * iSCSI layer rather than the st_list in SCSI layer.
 * Returns 0 if all ok, -1 on any error (no memory for buffers).
 */

static int
save_unsolicited_data(struct iscsi_cmnd *cmnd, uint32_t offset,
					  struct generic_pdu *hdr)
{
	int err;
	struct data_list *data;
	uint32_t total_length, length;

	TRACE(TRACE_ENTER_LEAVE, "Enter save_unsolicited_data\n");

	if ((total_length = hdr->length) == 0)
		return 0;

	TRACE(TRACE_ISCSI, "Save_unsolicited_data: offset %u, length %u\n", offset,
		  total_length);

	while ((length = total_length)) {
		if (length > MAX_MALLOC_SIZE)
			length = MAX_MALLOC_SIZE;

		/* receive length bytes of data */
		data = malloc(sizeof(struct data_list));
		if (data == NULL)
			return -1;

		data->offset = offset;
		data->length = length;

		/* receive unsolicited data into a newly-allocated buffer */
		err = read_single_data_seg((uint8_t *)hdr, cmnd, length, &data->buffer);
		if (err <= 0) {
			free(data);
			return err;
		}

		/* add the new buffer to end of list of unsolicted data buffers */
		data->next = NULL;
		if (cmnd->unsolicited_data_head)
			cmnd->unsolicited_data_tail->next = data;
		else
			cmnd->unsolicited_data_head = data;
		cmnd->unsolicited_data_tail = data;

		offset += length;
		total_length -= length;
		}

	update_after_read(hdr, cmnd);

	TRACE(TRACE_ENTER_LEAVE, "Leaving save_unsolicited_data\n");
	return 0;
}

/*
 * executed only by the rx thread.
 * read data segment for command into list of buffers at given offset
 * this can be either immediate data in a write, or the payload of a DataIn
 * returns > 0 if ok, < 0 if error, = 0 if error recovery completed ok
 */
static int
read_list_data_seg(struct generic_pdu *hdr, struct iscsi_cmnd *cmd,
				   struct scatterlist *st_list, int offset)
{
	struct targ_error_rec err_rec;
	struct iovec *iov;
	int size, niov, padding, err, sgindex;
	uint32_t digest, pad_bytes;

	size = hdr->length;
	niov = find_iovec_needed(size, st_list, offset);
	if (niov <= 0) {
		TRACE_ERROR("Trouble in find_iovec_needed\n");
		return -1;
	}

	/* allocate 2 extra iov slots for possible padding and crc */
	iov = malloc((niov + 2) * sizeof(struct iovec));
	if (!iov) {
		return -1;
	}

	/* gives back number of st_list elements used */
	sgindex = fill_iovec(iov, 0, niov, st_list, &offset, size);

	padding = (-size) & 3;
	if (padding) {
		iov[niov].iov_base = &pad_bytes;
		iov[niov].iov_len = padding;
		niov++;
		size += padding;
	}

	if (cmd->conn->data_crc) {
		iov[niov].iov_base = &digest;
		iov[niov].iov_len = CRC_LEN;
		niov++;
		size += CRC_LEN;
	}

	err = iscsi_rx_data(cmd->conn, iov, niov, size);

	if (err == size) {
		/* we received everything we expected to receive
		 * Store scatter list count and offset for recovery purposes - SAI
		 */
		cmd->scatter_list_count += sgindex;
		cmd->scatter_list_offset = offset;
		update_after_read(hdr, cmd);
	} else {
		/* Payload Digest Error Recovery - SAI */
		if (err == PAYLOAD_DIGERR) {
			TRACE(TRACE_ERROR_RECOVERY,
				  "Start payload digest error recovery\n");
			err_rec.curr_conn = cmd->conn;
			err_rec.pdu_hdr = hdr;
			err_rec.cmd = cmd;
			err_rec.err_type = PAYLOAD_DIGERR;
			err = targ_do_error_recovery(&err_rec);
		}
	}
	free(iov);
	return err;
}


/* called after reading unsolicited data attached to a WRITE SCSI command pdu,
 * or solicited data attached to a DataOut pdu.
 */
static void __attribute__ ((no_instrument_function))
update_after_read(struct generic_pdu *hdr, struct iscsi_cmnd *cmnd)
{
	cmnd->data_done += hdr->length;
	cmnd->immediate_data_present = 0;
	if (hdr->flags & F_BIT) {
		/* end of this (solicited or unsolicited) sequence,
		 * reset the counters for the next solicited burst (if any)
		 */
		cmnd->data_sn = 0;
		cmnd->unsolicited_data_present = 0;
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
        free(session->r2t_timer);
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
        free(cmnd->ping_data);
        free(cmnd);
    }

    /* free connections */
    list_for_each_safe(list_ptr, list_temp, &session->conn_list) {
        conn = list_entry(list_ptr, struct iscsi_conn, conn_link);
        TRACE(TRACE_ISCSI, "releasing connection %d\n", (int)
              conn->conn_id);

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
    free(session->session_params);

    free(session->oper_param);

    free(session);

    return 0;
}

/*
 * executed only by midlevel's target_process_thread as part of a call-back
 * via iscsi_xmit_response() or iscsi_rdy_to_xfer().
 * search_iscsi_cmnd: This goes through individual sessions and then
 * through the command queues for that session to look for a match to
 * a given command.
 * INPUT: Target_Scsi_Cmnd
 * OUTPUT: struct iscsi_cmnd if it finds one, NULL otherwise
 *		   if return value is not NULL, result_sess has been filled in with
 *							the session, and the session->cmnd_sem is locked
 */
static struct iscsi_cmnd * __attribute__ ((no_instrument_function))
search_iscsi_cmnd(Target_Scsi_Cmnd * cmnd, struct iscsi_session **result_sess)
{
	struct iscsi_cmnd *cmd = NULL;
	struct iscsi_session *session;
	struct iscsi_global *host;
	struct list_head *list_ptr;

	if (!cmnd) {
		TRACE_ERROR("Cannot search for a NULL command\n");
		return NULL;
	}

	host = (struct iscsi_global *) cmnd->device->dev_specific;

	/* non-destructive access to session lists */

    pthread_mutex_lock(&host->session_read_mutex);
    host->session_readers++;
    pthread_mutex_unlock(&host->session_read_mutex);

    pthread_mutex_lock(&host->session_mutex);
	list_for_each(list_ptr, &host->session_list) {
		session = list_entry(list_ptr, struct iscsi_session, sess_link);

        pthread_mutex_lock(&session->cmnd_mutex);
		for (cmd = session->cmnd_list; cmd != NULL; cmd = cmd->next) {
			if (cmd->cmnd == cmnd) {
				*result_sess = session;
                /*** NOTE: The cmnd_mutex is released in the CALLER!!! ***/
                /*** Blame the original target authors, not me - A.A.  ***/
				goto search_iscsi_cmnd_out;
			}
		}
        pthread_mutex_unlock(&session->cmnd_mutex);
	}

search_iscsi_cmnd_out:

    pthread_mutex_unlock(&host->session_mutex);
    pthread_mutex_lock(&host->session_read_mutex);
	--host->session_readers;
    pthread_mutex_unlock(&host->session_read_mutex);

	return cmd;
}

/*
 * search_iscsi_message: This goes through individual sessions and then
 * through the command queues for that session to look for a match to
 * a given message.
 * INPUT: Target_Scsi_Message
 * OUTPUT: struct iscsi_cmnd if it finds one, NULL otherwise
 */
static struct iscsi_cmnd *search_task_mgt_command(Target_Scsi_Message * message) {
	struct iscsi_cmnd *related_task_mgt_command = NULL;
	struct iscsi_session *related_session;
	struct iscsi_global *host;
	struct list_head *list_ptr;

	if (!message) {
		TRACE_ERROR("Cannot search for a NULL command\n");
		return NULL;
	}

	host = (struct iscsi_global *) message->device->dev_specific;

	/* non-destructive access to session lists */
    pthread_mutex_lock(&host->session_read_mutex);
    host->session_readers++;
    pthread_mutex_unlock(&host->session_read_mutex);

    pthread_mutex_lock(&host->session_mutex);
	list_for_each(list_ptr, &host->session_list) {
		related_session = list_entry(list_ptr, struct iscsi_session, sess_link);

        pthread_mutex_lock(&related_session->cmnd_mutex);

		for (related_task_mgt_command = related_session->cmnd_list;
			 related_task_mgt_command != NULL;
			 related_task_mgt_command = related_task_mgt_command->next) {
			if (related_task_mgt_command->message == message) {
                pthread_mutex_unlock(&related_session->cmnd_mutex);
				goto out;
			}
		}

        pthread_mutex_unlock(&related_session->cmnd_mutex);
	}

out:
    pthread_mutex_unlock(&host->session_mutex);

    pthread_mutex_lock(&host->session_read_mutex);
	--host->session_readers;
    pthread_mutex_unlock(&host->session_read_mutex);

	return related_task_mgt_command;
}


/*
 * search_tags: This goes through individual sessions and then
 * through the command queues for that session to look for a match to
 * a given set of tags.
 * INPUT: connection, initiator and target tag
 * OUTPUT: struct iscsi_cmnd if it finds one, NULL otherwise
 */
static struct iscsi_cmnd * __attribute__ ((no_instrument_function))
search_tags(struct iscsi_conn *conn, uint32_t init_task_tag, 
            uint32_t target_xfer_tag, int dumpall)
{
	struct iscsi_cmnd *cmd = NULL;
	struct iscsi_session *session;

	if (!conn) {
		TRACE_ERROR(" Cannot search a NULL connection\n");
		return NULL;
	}

	/* get session from conn structure directly to support multiple sessions */
	session = conn->session;

    pthread_mutex_lock(&session->cmnd_mutex);

	if (dumpall || TRACE_TEST(TRACE_DEBUG)) {
		for (cmd = session->cmnd_list; cmd != NULL; cmd = cmd->next) {
			TRACE(TRACE_DEBUG,
                  "scsi cmnd %p opcode 0x%02x init_task_tag %d target_xfer_tag"
				   " %d data_done %d xfer length %d stat_sn %u state %u\n",
				   cmd->cmnd, cmd->opcode_byte,
				   cmd->init_task_tag, cmd->target_xfer_tag,
				   cmd->data_done, cmd->data_length, cmd->stat_sn, cmd->state);
		}
	}

	for (cmd = session->cmnd_list; cmd != NULL; cmd = cmd->next) {
		if ((cmd->init_task_tag == init_task_tag)
			&& ((cmd->target_xfer_tag == target_xfer_tag)
					  || (target_xfer_tag == ALL_ONES))) {
			TRACE(TRACE_DEBUG, "Search found the command\n");
			TRACE(TRACE_DEBUG,
				  "scsi cmnd %p, init_task_tag %d target_xfer_tag %d"
				  " data_done %d xfer length %d\n", cmd->cmnd,
				  cmd->init_task_tag, cmd->target_xfer_tag, cmd->data_done,
				  cmd->data_length);

            break;
		}
	}

    pthread_mutex_unlock(&session->cmnd_mutex);

	return cmd;
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
    int data_len, k;
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

    TRACE(TRACE_ISCSI, "login response sent\n");

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
    pthread_mutex_lock(&host->session_mutex);

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
                "existing session tsih %u\n",
              conn->cid, pdu->tsih);

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

    pthread_mutex_unlock(&host->session_mutex);

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
        free(temp_params);
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

    goto out;
}


/*
 * executed only by the rx thread.
 * called when a Logout Request has just been received.
 * INPUT: connection, session and the buffer containing the logout pdu
 * OUTPUT: 0 if everything is okay, < 0 if there is trouble
 */
static int
handle_logout(struct iscsi_conn *conn,
			  struct iscsi_session *session,
			  uint8_t *buffer)
{
	struct iscsi_init_logout_cmnd *pdu =
		(struct iscsi_init_logout_cmnd *) buffer;
	struct iscsi_cmnd *cmnd;
	int err;

	if (TRACE_TEST(TRACE_ISCSI_FULL))
		print_init_logout_cmnd(pdu);

	pdu->length = ntohl(pdu->length);
	pdu->init_task_tag = ntohl(pdu->init_task_tag);
	pdu->cid = ntohs(pdu->cid);
	pdu->cmd_sn = ntohl(pdu->cmd_sn);
	pdu->exp_stat_sn = ntohl(pdu->exp_stat_sn);

	TRACE(TRACE_VERBOSE, "Logout ITT %u, CmdSN %u, reason %u, cid %u\n",
			pdu->init_task_tag, pdu->cmd_sn, pdu->flags & LOGOUT_REASON,
			pdu->cid);

	if ((cmnd = get_new_cmnd()) == NULL) {
		return -1;
	}

	cmnd->conn = conn;
	cmnd->session = session;
	cmnd->opcode_byte = pdu->opcode;
	cmnd->data_length = pdu->length;
	cmnd->init_task_tag = pdu->init_task_tag;
	cmnd->cmd_sn = pdu->cmd_sn;
	cmnd->stat_sn = pdu->exp_stat_sn;

	if (pdu->length > 0) {
		/* read length bytes of text data into a newly-allocated buffer */
		TRACE_ERROR("Data attached to logout request, CmdSN %u, ExpCmdSN "
					"%u, ITT %u, opcode 0x%02x\n", 
					cmnd->cmd_sn, session->exp_cmd_sn,
					cmnd->init_task_tag, cmnd->opcode_byte);
		err = read_single_data_seg(buffer, cmnd, pdu->length, &cmnd->ping_data);
		if (err <= 0) {
			free(cmnd);
			return err;
		}
	}

    pthread_mutex_lock(&session->cmnd_mutex);
	err = check_cmd_sn(cmnd, pdu, session, 1);
    pthread_mutex_lock(&session->cmnd_mutex);

	if (err < 0) {	
		/* out of range, silently ignore it */
		TRACE_ERROR("ignoring out of range CmdSN %u, ExpCmdSN %u, ITT %u, "
					"opcode 0x%02x\n", 
					cmnd->cmd_sn, session->exp_cmd_sn,
					cmnd->init_task_tag, cmnd->opcode_byte);
		ack_sent_cmnds(conn, cmnd, pdu->exp_stat_sn, 0);
		free(cmnd->ping_data);
		free(cmnd);
	} else {
		if (err == 0) {		
			/* do immediate command or expected non-immediate command now */
			cmnd->state = ISCSI_LOGOUT;
		} else {
			/* within range but out of order, queue it for later */
			cmnd->state = ISCSI_QUEUE_OTHER;
		}

		ack_sent_cmnds(conn, cmnd, pdu->exp_stat_sn, 1);
	}
	return 0;
}


/*
 * generate the next TTT in a session
 * must be called with session->cmnd_mutex lock held
 */
static inline uint32_t __attribute__ ((no_instrument_function))
generate_next_ttt(struct iscsi_session* session)
{
	uint32_t retval;

	retval = session->cmnd_id++;
	if (session->cmnd_id == 0) {
		/* just assigned -1 to retval, which is reserved
		 * so do it again, since 0 is not reserved
		 */
		retval = session->cmnd_id++;
	}
	return retval;
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
    pthread_mutex_init(&conn->text_in_progress_mutex, NULL);

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

    pthread_mutex_init(&session->cmnd_mutex, NULL);
    sem_init(&session->retran_sem, 0,0);
    sem_init(&session->thr_kill_sem,0 ,0);

    return conn;

out7:
    free(session->session_params);

    printf("\n 1 \n");
out6:
    TRACE(TRACE_DEBUG, "Releasing R2T timer %p for session %p\n",
          session->r2t_timer, session);
    free(session->r2t_timer);

    free(session);

out4:
    free(conn);

    return NULL;
}

/*
 * executed only by the tx thread.
 * iscsi_tx_reject: this function transmits a reject to the Initiator
 * the attached 48-byte data segment is the header of the rejected PDU
 * OUTPUT: 0 if everything is okay, < 0 if there is trouble
 */
int
iscsi_tx_rjt(struct iscsi_conn *conn, uint8_t *bad_hdr, uint8_t reason)
{
	int err = 0;
	uint8_t iscsi_hdr[ISCSI_HDR_LEN];
	struct iscsi_targ_rjt *hdr;

	memset(iscsi_hdr, 0x0, ISCSI_HDR_LEN);
	hdr = (struct iscsi_targ_rjt *) iscsi_hdr;

	hdr->opcode = ISCSI_TARG_RJT;
	hdr->flags |= F_BIT;
	hdr->reason = reason;
	hdr->length = htonl(ISCSI_HDR_LEN);
	/* init task tag must be all ones - new 18_04 SAI */
	hdr->init_task_tag = ALL_ONES;

	hdr->stat_sn = htonl(conn->stat_sn++);

	if (conn->session) {
		hdr->exp_cmd_sn = htonl(conn->session->exp_cmd_sn);
		hdr->max_cmd_sn = htonl(conn->session->max_cmd_sn);
	}

	if (send_hdr_plus_1_data(conn, iscsi_hdr, bad_hdr, ISCSI_HDR_LEN) < 0) {
		err = -1;
		goto out;
	}

	TRACE_WARNING("Send Reject\n");

	if (TRACE_TEST(TRACE_ISCSI_FULL))
		print_targ_rjt(hdr);

out:
	return err;
}

/**************************************************************************/
/*                            handle response                             */
/**************************************************************************/

#define UNDERFLOW_FLAG	0x0001
#define OVERFLOW_FLAG	0x0002
#define SEND_SENSE_FLAG	0x0004
#define LAST_SEQ_FLAG	0x0010

static uint32_t
do_command_status(struct iscsi_cmnd *cmnd, Scsi_Request *req,
				  int *data_left, int *residual_count)
{
	int transfer = 0;
	unsigned data_length_left;
	uint32_t flags = 0;

	data_length_left = req->sr_bufflen;

	TRACE(TRACE_DEBUG, "Sense: %02x %02x %02x %02x %02x %02x %02x %02x\n",
		  req->sr_sense_buffer[0], req->sr_sense_buffer[1],
		  req->sr_sense_buffer[2], req->sr_sense_buffer[3],
		  req->sr_sense_buffer[4], req->sr_sense_buffer[5],
		  req->sr_sense_buffer[6], req->sr_sense_buffer[7]);

	if ((req->sr_sense_buffer[0] & 0x7e) == 0x70) {
		/* current or deferred error -- else, we don't send the sense buffer */
		flags |= SEND_SENSE_FLAG;

		if (req->sr_sense_buffer[0] & 0x80) {
			/* bytes 3 thru 6 contain valid data */
			transfer = (req->sr_sense_buffer[3] << 24) |
				(req->sr_sense_buffer[4] << 16) |
				(req->sr_sense_buffer[5] << 8) | req->sr_sense_buffer[6];
			TRACE(TRACE_DEBUG, "information in sense data: %d\n", transfer);
		}

		if ((req->sr_sense_buffer[2] & 0x20) != 0) {		/* ILI bit set */
				data_length_left -= transfer;
		}
	} else if (req->sr_command) {
		/* ensure correct DataSegmentLength and ResidualCount */
		TRACE(TRACE_DEBUG, "data_length_left %d, sr_command->resid %d\n",
				data_length_left, req->sr_command->resid);
		data_length_left -= req->sr_command->resid;
	}

	/* check overflow or underflow problem, cdeng 3/24/02 */
	if (data_length_left > cmnd->data_length) {
		*residual_count = data_length_left - cmnd->data_length;
		data_length_left = cmnd->data_length;
		flags |= OVERFLOW_FLAG;
	} else if (data_length_left < cmnd->data_length) {
		*residual_count = cmnd->data_length - data_length_left;
		flags |= UNDERFLOW_FLAG;
	}

	TRACE(TRACE_DEBUG,"data_length_left %d, residual_count %d, flags 0x%08x\n",
		  data_length_left, *residual_count, flags);

	*data_left = data_length_left;
	return flags;
}


/*
 * executed only by the tx thread.
 * called when it is ok to send a Text Response pdu for this command.
 * (i.e., when cmnd state is ISCSI_SEND_TEXT_RESPONSE)
 */
static int
handle_discovery_rsp(struct iscsi_cmnd *cmnd,
					 struct iscsi_conn *conn,
					 struct iscsi_session *session)
{
	uint8_t iscsi_hdr[ISCSI_HDR_LEN];
	struct iscsi_targ_text_rsp *hdr;
	unsigned size, offset;
    int retval = 0;
	int next_state = ISCSI_SENT;
	struct iscsi_cmnd *next_in_progress = NULL;
	char *ptr;


	pthread_mutex_lock(&conn->text_in_progress_mutex);

	/* under protection of this lock, make sure command has not been reset */
	if (cmnd->state != ISCSI_SEND_TEXT_RESPONSE
		|| conn->text_in_progress != cmnd)
		goto out;

	/* get offset to, and amount of, data remaining to send back to initiator */
	offset = cmnd->data_done;
	size = cmnd->data_length - offset;
	ptr = cmnd->ping_data + offset;

	memset(iscsi_hdr, 0x0, ISCSI_HDR_LEN);
	hdr = (struct iscsi_targ_text_rsp *)iscsi_hdr;

	hdr->opcode = ISCSI_TARG_TEXT_RSP;

	if (size > 0 && conn->connection_flags & USE_ONE_KEY_PER_TEXT) {
		/* send just one key per text response (for testing) */
		size = strlen(ptr) + 1;
		cmnd->data_done += size;
		next_state = ISCSI_BLOCKED_SENDING_TEXT;
		next_in_progress = cmnd;
	} else if (size > conn->max_send_length) {
		/* reply too big for one pdu, more text reply pdus follow this one */
		size = conn->max_send_length;
		cmnd->data_done += size;
		hdr->flags |= C_BIT;
		next_state = ISCSI_BLOCKED_SENDING_TEXT;
		next_in_progress = cmnd;
	} else if (cmnd->command_flags & F_BIT) {
		/* this is our last reply and initiator has no more to send */
		hdr->flags |= F_BIT;
		cmnd->target_xfer_tag = hdr->target_xfer_tag = ALL_ONES;
		next_state = ISCSI_SENT;
		next_in_progress = NULL;
	} else {
		/* this is our last reply but initiator has more to send */
		cmnd->data_length = 0;			/* reset for next Request PDU */
		cmnd->in_progress_buffer = cmnd->ping_data;
		cmnd->ping_data = NULL;
		next_state = ISCSI_AWAIT_MORE_TEXT;
		next_in_progress = cmnd;
	}
	hdr->length = htonl(size);
	hdr->init_task_tag = htonl(cmnd->init_task_tag);
	hdr->stat_sn = htonl(conn->stat_sn++);

    pthread_mutex_lock(&session->cmnd_mutex);

	/* generate next TTT if we expect initiator to send another text request */
	if (next_in_progress) {
		cmnd->target_xfer_tag = generate_next_ttt(session);
		hdr->target_xfer_tag = htonl(cmnd->target_xfer_tag);
	}

	hdr->exp_cmd_sn = htonl(session->exp_cmd_sn);
	hdr->max_cmd_sn = htonl(session->max_cmd_sn);
	if (!(cmnd->opcode_byte & I_BIT)) {
		/* the text command pdu was not immediate, CmdSN advances */
		session->max_cmd_sn++;
	}
	cmnd->state = next_state;
    pthread_mutex_unlock(&session->cmnd_mutex);

	conn->text_in_progress = next_in_progress;

	if (send_hdr_plus_1_data(conn, iscsi_hdr, ptr, size) < 0) {
		goto out1;
	}

	TRACE(TRACE_ISCSI, "text response sent, ITT %u\n",
		  cmnd->init_task_tag);
	if (TRACE_TEST(TRACE_ISCSI_FULL))
		print_targ_text_rsp(hdr);

out:
    pthread_mutex_unlock(&conn->text_in_progress_mutex);
	return retval;

out1:
	cmnd->state = ISCSI_DEQUEUE;
	cmnd->init_task_tag = ALL_ONES;
	conn->text_in_progress = NULL;
	retval = -1;
	goto out;
}

/*
 * executed only by the tx thread.
 * Called only when cmnd state is ISCSI_ASK_FOR_MORE_TEXT.
 * Sends back an empty text response so initiator can send more text data.
 * This text response pdu has C=0 F=0 DSL=0 ITT=from request TTT=newly generated
 */
static int
ask_for_more_text(struct iscsi_cmnd *cmnd,
				  struct iscsi_conn *conn,
				  struct iscsi_session *session)
{
	uint8_t iscsi_hdr[ISCSI_HDR_LEN];
	struct iscsi_targ_text_rsp *hdr;
	int retval = 0;


    pthread_mutex_lock(&conn->text_in_progress_mutex);

	/* under protection of this lock, make sure command has not been reset */
	if (cmnd->state != ISCSI_ASK_FOR_MORE_TEXT
		|| conn->text_in_progress != cmnd)
		goto out1;

	memset(iscsi_hdr, 0x0, ISCSI_HDR_LEN);
	hdr = (struct iscsi_targ_text_rsp *)iscsi_hdr;

	hdr->opcode = ISCSI_TARG_TEXT_RSP;

	hdr->init_task_tag = htonl(cmnd->init_task_tag);
	hdr->stat_sn = htonl(conn->stat_sn++);

    pthread_mutex_lock(&session->cmnd_mutex);

	/* generate the next TTT */
	cmnd->target_xfer_tag = generate_next_ttt(session);
	hdr->target_xfer_tag = htonl(cmnd->target_xfer_tag);

	hdr->exp_cmd_sn = htonl(session->exp_cmd_sn);
	hdr->max_cmd_sn = htonl(session->max_cmd_sn);
	if (!(cmnd->opcode_byte & I_BIT)) {
		/* the text command pdu was not immediate, CmdSN advances */
		session->max_cmd_sn++;
	}
    pthread_mutex_unlock(&session->cmnd_mutex);

	cmnd->state = ISCSI_AWAIT_MORE_TEXT;

	if (send_hdr_plus_1_data(conn, iscsi_hdr, NULL, 0) < 0) {
		retval = -1;
		goto out1;
	}

	TRACE(TRACE_ISCSI, "text response sent, ITT %u\n", 
		  cmnd->init_task_tag);
	if (TRACE_TEST(TRACE_ISCSI_FULL))
		print_targ_text_rsp(hdr);

out1:
    pthread_mutex_unlock(&conn->text_in_progress_mutex);
	return retval;
}

/*
 * executed only by the tx thread.
*/
static int
handle_logout_rsp(struct iscsi_cmnd *cmnd,
				  struct iscsi_conn *conn,
				  struct iscsi_session *session)
{
	uint8_t iscsi_hdr[ISCSI_HDR_LEN];
	struct iscsi_targ_logout_rsp *hdr;

	memset(iscsi_hdr, 0x0, ISCSI_HDR_LEN);
	hdr = (struct iscsi_targ_logout_rsp *) iscsi_hdr;

	hdr->opcode = ISCSI_TARG_LOGOUT_RSP;

	hdr->flags |= F_BIT;

	/* for now, we always send response = 0, connection or session closed ok */

	hdr->init_task_tag = htonl(cmnd->init_task_tag);
	hdr->stat_sn = htonl(conn->stat_sn++);

	pthread_mutex_lock(&session->cmnd_mutex);
	hdr->exp_cmd_sn = htonl(session->exp_cmd_sn);
	hdr->max_cmd_sn = htonl(session->max_cmd_sn);
	if (!(cmnd->opcode_byte & I_BIT)) {
		/* the logout command pdu was not immediate, CmdSN advances */
		session->max_cmd_sn++;
	}
	pthread_mutex_unlock(&session->cmnd_mutex);

	cmnd->state = ISCSI_SENT;

	/* connection now logged out, do not send any more commands after this */
	conn->connection_flags |= CONN_LOGGED_OUT;

	if (send_hdr_only(conn, iscsi_hdr) < 0) {
		return -1;
	}

	TRACE(TRACE_ISCSI, "logout response sent\n");

	if (TRACE_TEST(TRACE_ISCSI_FULL))
		print_targ_logout_rsp(hdr);

	return 0;
}

/*
 * executed only by the tx thread.
 * sends a NopIn PDU to initiator
 * either in response to an earlier NopOut ping from initiator
 * or as a new NopIn ping generated by the target
 */
static int
handle_nopin(struct iscsi_cmnd *cmnd,
			 struct iscsi_conn *conn,
			 struct iscsi_session *session)
{
	uint8_t iscsi_hdr[ISCSI_HDR_LEN];
	struct iscsi_targ_nopin *hdr;

	memset(iscsi_hdr, 0x0, ISCSI_HDR_LEN);
	hdr = (struct iscsi_targ_nopin *) iscsi_hdr;

	hdr->opcode = ISCSI_TARG_NOP_IN;

	hdr->flags |= F_BIT;

	hdr->length = htonl(cmnd->data_length);
	hdr->init_task_tag = htonl(cmnd->init_task_tag);
	hdr->target_xfer_tag = htonl(cmnd->target_xfer_tag);

	/* Draft 20, Section 10.19.2 StatSN
	 * "The StatSN field will always contain the next StatSN. However,
	 * when the Initiator Task Tag is set to 0xffffffff StatSN for the
	 * connection is not advanced after this PDU is sent."
	 */
	hdr->stat_sn = htonl(conn->stat_sn);
	if (cmnd->init_task_tag != ALL_ONES)
		conn->stat_sn++;

    pthread_mutex_lock(&session->cmnd_mutex);	
    hdr->exp_cmd_sn = htonl(session->exp_cmd_sn);
	hdr->max_cmd_sn = htonl(session->max_cmd_sn);
	if (!(cmnd->opcode_byte & I_BIT)) {
		/* the nopout command pdu was not immediate, CmdSN advances */
		session->max_cmd_sn++;
	}
	pthread_mutex_unlock(&session->cmnd_mutex);

	if (cmnd->target_xfer_tag == ALL_ONES) {
		/* target not expecting a reply from initiator */
		cmnd->state = ISCSI_SENT;
	}

	if (send_hdr_plus_1_data(conn, iscsi_hdr, cmnd->ping_data,
													cmnd->data_length) < 0) {
		return -1;
	}

	TRACE(TRACE_DEBUG, "sent NopIn CmdSN %u, ExpCmdSN %u, ITT %u "
						"opcode 0x%02x, state %u\n",
						cmnd->cmd_sn, session->exp_cmd_sn,
						cmnd->init_task_tag, cmnd->opcode_byte, cmnd->state);

	TRACE(TRACE_ISCSI, "nopin sent\n");

	if (TRACE_TEST(TRACE_ISCSI_FULL))
		print_targ_nopin(hdr);

	ZFREE(cmnd->ping_data);

	return 0;
}

/*
 * executed only by the tx thread.
 * iscsi_dequeue: this function frees up a command's allocated memory
 * after it has been removed from the session list (before the call)
 * INPUT: command which has to be freed
 */
static void
iscsi_dequeue(struct iscsi_cmnd *cmnd, struct iscsi_conn *conn)
{
	TRACE(TRACE_DEBUG, "free cmnd with ITT %u\n", cmnd->init_task_tag);

	if (cmnd->cmnd != NULL) {
		if (scsi_target_done(cmnd->cmnd) < 0) {
			TRACE_ERROR("scsi_target_done returned an error\n");
		}
	}

	/* if this command was the text command currently in progress on this
	 * connection, unblock the connection so it can process more text commands.
	 */
	pthread_mutex_lock(&conn->text_in_progress_mutex);
    if (conn->text_in_progress == cmnd)
        conn->text_in_progress = NULL;
    pthread_mutex_unlock(&conn->text_in_progress_mutex);

	/* Free the R2T cookie if any - SAI */
	free_r2t_cookie(cmnd);
	free_data_list(cmnd);
	free(cmnd->in_progress_buffer);
	free(cmnd->ping_data);
	free(cmnd);
}

static inline uint32_t __attribute__ ((no_instrument_function))
get_sglen(struct scatterlist *st_list)
{
    return st_list->length;
}

static inline uint8_t * __attribute__ ((no_instrument_function))
get_sgbuf(struct scatterlist *st_list)
{
    return st_list->address;
}

/* skip thru sg list to appropriate offset for data buffer
 * i is index to sg list item
 * offset is total number of bytes to skip
 * returns number of bytes remaining to be skipped in ith sg list item
 */
static uint32_t __attribute__ ((no_instrument_function))
skip_thru_sg_list(struct scatterlist *st_list, uint32_t *i, uint32_t offset)
{
	uint32_t sglen;

	*i = 0;							/* index to current sg list item */
	while (offset > 0) {
		sglen = get_sglen(st_list);
		if (offset < sglen) {
			/* copying starts at "offset" bytes in ith sg list item */
			break;
		}
		/* need to skip over this entire sg list item */
		offset -= sglen;
		st_list++;
		*i += 1;
	}
	return offset;
}

/*
 * find_iovec_needed: this function returns the number of iovecs that
 * are needed to receive some data
 * INPUT: amount of data needed, pointer to scattergather list (from the
 * 	  point where the data is to start for this round, offset within
 * 	  the first scattergather element - in case it was not used up
 * 	  completely
 * OUTPUT: number of iovectors needed, else < 0 if there is trouble
 */
int __attribute__ ((no_instrument_function))
find_iovec_needed(uint32_t data_len, struct scatterlist *st_list, uint32_t offset)
{
	int i = 0;
	uint32_t sglen;

	while (data_len) {
		sglen = get_sglen(st_list);
		if (data_len > (sglen - offset))
			data_len -= (sglen - offset);
		else
			data_len = 0;

		i++;
		st_list++;
		offset = 0;
	}

	return (i);
}

/*
 * fill_iovec: This function fills in a given iovec structure so as to
 * reflect a given scattergather list upto the length that is required
 * No checks are made on the length on the iovec given except that it
 * needs to be greater than the size needed. Also, the scatterlist is
 * also not checked. The integer offset upon return reflects the posn of
 * the offset in the last scatterlist that was used for this function
 * INPUT: struct iovec, no. of iovecs, scatterlist, ptr to offset within
 * 	  the first element, and the total length required
 * OUTPUT: how many st_list elements are used, < 0 if there is trouble
 */
int __attribute__ ((no_instrument_function))
fill_iovec(struct iovec *iov, int p, int niov,
		   struct scatterlist *st_list, int *offset, uint32_t data)
{
	int count = 0;
	uint32_t sglen;

	TRACE(TRACE_DEBUG, "offset: %d, data_len: %d\n", *offset, data);

	iov += p;
	while (data > 0 && p < niov) {
		iov->iov_base = (void *)(get_sgbuf(st_list) + (*offset));
		sglen = get_sglen(st_list);
		if ((sglen - *offset) > data) {
			/* more space left in this st list item than iovector can use */
			iov->iov_len = data;
			*offset += data;
		} else {
			/* iovector uses all the space in this st list item */
			iov->iov_len = sglen - *offset;
			*offset = 0;
			count++;
		}

		data -= iov->iov_len;

		TRACE(TRACE_DEBUG, "iov %p, p %d, iov_base %p, iov_len %d\n", iov,
			  p, iov->iov_base, iov->iov_len);

		p++;
		iov++;
		st_list++;
	}

	return count;
}

/* 
 * executed only by the tx thread
 * called directly when state is ISCSI_RESEND_STATUS
 * and called indirectly via handle_iscsi_done() when state is ISCSI_DONE
 * to send ISCSI Response PDU, with status information if needed.
 * Separated Data-In from Status Response PDU sending for Status 
 * Error Recovery - SAI 
 */
static int
send_iscsi_response(struct iscsi_cmnd *cmnd,
					struct iscsi_conn *conn,
					struct iscsi_session *session)
{
	struct iscsi_targ_scsi_rsp *rsp;
	Scsi_Request *req;
	uint8_t iscsi_hdr[ISCSI_HDR_LEN];
	int data_length_left = 0;
	struct sense_data_buf {
		uint16_t	len;						/* added by iscsi standard */
		uint8_t	data[SCSI_SENSE_BUFFERSIZE];/* direct from scsi */
		} sense_data = {0, {0}};
	uint32_t flags = 0;
	int residual_count = 0;

	TRACE(TRACE_DEBUG, "send_scsi_response\n");

	memset(iscsi_hdr, 0x0, ISCSI_HDR_LEN);
	rsp = (struct iscsi_targ_scsi_rsp *)iscsi_hdr;
	rsp->opcode = ISCSI_TARG_SCSI_RSP;
	rsp->flags |= F_BIT;
	rsp->init_task_tag = htonl(cmnd->init_task_tag);

	req = cmnd->cmnd->req;
	if ((req->sr_data_direction == SCSI_DATA_READ)
									&& host_byte(req->sr_result) == DID_OK) {
		flags = do_command_status(cmnd,req,&data_length_left,&residual_count);
		rsp->exp_data_sn = htonl(cmnd->data_sn);
	}

	/* cdeng 3/24/02 */
	if (flags & OVERFLOW_FLAG) {
		rsp->flags |= O_BIT;
		rsp->resid = htonl(residual_count);
	} else if (flags & UNDERFLOW_FLAG) {
		rsp->flags |= U_BIT;
		rsp->resid = htonl(residual_count);
	}

	/* RDR - do NOT assign and increment stat_sn on a retransmission */
	if (!cmnd->retransmit_flg) {
		cmnd->stat_sn = conn->stat_sn++;
	}
	rsp->stat_sn = htonl(cmnd->stat_sn);
	rsp->exp_cmd_sn = htonl(session->exp_cmd_sn);
	rsp->max_cmd_sn = htonl(session->max_cmd_sn);

	if (flags & SEND_SENSE_FLAG) {
		/* sense data has to be sent as part of SCSI Response pdu */
		rsp->status = CHECK_CONDITION << 1;	/* why does Linux do this shift? */
		sense_data.len = SCSI_SENSE_BUFFERSIZE;
		if (flags & UNDERFLOW_FLAG) {
			TRACE(TRACE_DEBUG, "underflow is found\n");
			memset(sense_data.data, 0x0, SCSI_SENSE_BUFFERSIZE);
			sense_data.data[0] = 0xf0;		/* scsi valid bit, code 70h */
			sense_data.data[2] = 0x20;		/* scsi ILI bit */
			sense_data.data[7] = 0x07;		/* scsi additional length = 7 */
			memcpy(sense_data.data + 3, req->sr_sense_buffer + 3, 4);
		} else {
			TRACE(TRACE_DEBUG, "sense key 0x%x\n",
				  req->sr_sense_buffer[2] & 0xf);
			memcpy(sense_data.data,req->sr_sense_buffer,SCSI_SENSE_BUFFERSIZE);
		}
		sense_data.len += 2;	/* for the extra iscsi 2-byte length header */
	}
	rsp->length = htonl(sense_data.len);

	TRACE(TRACE_DEBUG, "send_iscsi_response: sending status for cmnd_rn %.8x "
		  "init_task_tag %.8x target_xfer_tag %.8x\n", cmnd->cmd_sn,
		  cmnd->init_task_tag, cmnd->target_xfer_tag);

	cmnd->retransmit_flg = 0;
	cmnd->state = ISCSI_SENT;
	if (send_hdr_plus_1_data(conn, iscsi_hdr, &sense_data, sense_data.len) < 0){
		return -1;
	}

	TRACE(TRACE_ISCSI, "scsi response sent, ITT %u\n",
		  cmnd->init_task_tag);

	if (TRACE_TEST(TRACE_ISCSI_FULL))
		print_targ_scsi_rsp(rsp);

	return 0;
}

/*
 * executed only by the tx thread when state is ISCSI_DONE.
 * called only from handle_iscsi_done() on a READ-type command.
 * sends all the DataIn pdus for this command to the initiator. 
 *
 * During this sending:
 *	data_length_left	number of bytes left to send in following sequences.
 *	seq_length			number of bytes left to send in this sequence.
 *	seq_low_byte		offset to lowest byte in this sequence.
 *	seq_limit_byte		offset to highest byte plus one in this sequence.
 *	pdu_offset			offset to first byte in this pdu.
 *	data_payload_length	number of data bytes to send in this pdu.
 *	total_data_length	total number of all bytes to send in this pdu
 */
static int
send_read_data(struct iscsi_cmnd *cmnd,
			   struct iscsi_conn *conn,
			   struct iscsi_session *session,
			   int *phase_collapse)
{
	int err = 0;
	uint32_t flags;
	struct iscsi_targ_scsi_data_in *hdr;
	struct scatterlist *st_list;
	int residual_count;
	unsigned prevsn;
	struct iovec *iov;
	int niov;
	Scsi_Request *req;
	uint8_t iscsi_hdr[ISCSI_HDR_LEN];
	uint32_t pad_bytes;
	int padding;
	unsigned data_length_left;
	unsigned seq_length;
	int seq_low_byte, seq_limit_byte;
	int pdu_offset;
	int data_payload_length;
	int total_data_length;

/* Daren Hayward, darenh@4bridgeworks.com */
#if defined(MANGLE_INQUIRY_DATA)
	int miov, siov;
#endif

	TRACE(TRACE_DEBUG, "sending sense data for cmnd_sn "
		  "%u, ITT %u, TTT %u\n", cmnd->cmd_sn, cmnd->init_task_tag,
		  cmnd->target_xfer_tag);

	req = cmnd->cmnd->req;
	flags = do_command_status(cmnd, req, &data_length_left, &residual_count);

	/* Store the previous data sequence for error recovery - SAI */
	prevsn = cmnd->prev_data_sn;
	if (!cmnd->retransmit_flg)
		cmnd->prev_data_sn = cmnd->data_sn;

	/* one of these is changed before use, depending on DataSequenceInOrder */
	seq_low_byte = 0;
	seq_limit_byte = data_length_left;

	/* once around this loop for each input sequence sent to initiator */
	while (data_length_left > 0) {
		if (data_length_left <= session->oper_param->MaxBurstLength) {
			/* all remaining data fits in this last sequence */
			seq_length = data_length_left;
			flags |= LAST_SEQ_FLAG;
		} else {
			/* another sequence will follow this one */
			seq_length = session->oper_param->MaxBurstLength;
		}
		data_length_left -= seq_length;

		if (session->oper_param->DataSequenceInOrder)
			seq_limit_byte = seq_low_byte + seq_length;	/* in order */
		else
			seq_low_byte = seq_limit_byte - seq_length;	/* reverse order */

		if (session->oper_param->DataPDUInOrder)
			pdu_offset = seq_low_byte;					/* in order */
		else
			pdu_offset = seq_limit_byte;				/* reverse order */

		/* once around this loop for each pdu in this sequence */
		while (seq_length > 0) {
			TRACE(TRACE_DEBUG, "data_length_left: %d, seq_length: %d\n",
				  data_length_left, seq_length);

			memset(iscsi_hdr, 0x0, ISCSI_HDR_LEN);
			hdr = (struct iscsi_targ_scsi_data_in *) iscsi_hdr;

			hdr->opcode = ISCSI_TARG_SCSI_DATA_IN;
			hdr->target_xfer_tag = ALL_ONES;

			if (seq_length > conn->max_send_length) {
				data_payload_length = conn->max_send_length;
			} else {
				/* this is the last data-in pdu in this sequence */
				data_payload_length = seq_length;
				hdr->flags |= F_BIT;

				if ((session->targ_snack_flg & DATACK_SNACK_ENABLE)
							&& session->oper_param->ErrorRecoveryLevel > 0) {
					/* set A bit on each data-in pdu that ends a sequence.
					 * (only legal at ErrorRecoveryLevel 1 or 2)
					 *
					 * Draft 20, Section 10.7.4 Target Transfer Tag and LUN
					 *
					 * On incoming data, the Target Transfer Tag and LUN
					 * MUST be provided by the target if the A bit is set
					 * to 1; otherwise they are reserved.
					 */
					hdr->flags |=A_BIT;
					hdr->target_xfer_tag = htonl(cmnd->target_xfer_tag);
					hdr->lun = cmnd->lun;
					TRACE(TRACE_ERROR_RECOVERY,
						  "Send DataIn, A=1, ITT %u, TTT %u, dataSN %u\n",
						  cmnd->init_task_tag, cmnd->target_xfer_tag,
						  cmnd->data_sn);
				}

				if ((flags & LAST_SEQ_FLAG) && !(flags & SEND_SENSE_FLAG)) {
					/* this is also the last data-in pdu for this command and
					 * we are not sending any sense data, try phase collapse.
					 * 0 never collapse, 1 always collapse, 
					 * 2 collapse this time, -1 do not collapse this time
					 */
					if (devdata->phase_collapse > 0) {
						hdr->flags |= S_BIT;
						*phase_collapse = 1;

						if (devdata->phase_collapse == 2)
							devdata->phase_collapse = -1;
					} else if (devdata->phase_collapse == -1)
						devdata->phase_collapse = 2;
				}
			}

			if (!session->oper_param->DataPDUInOrder)
				pdu_offset -= data_payload_length;		/* reverse order */

			hdr->length = htonl(data_payload_length);
			hdr->init_task_tag = htonl(cmnd->init_task_tag);

			if (hdr->flags & S_BIT) {
				/* RDR - already assigned and incremented stat_sn before 
				 * a retransmission
				 */
				if (!cmnd->retransmit_flg) {
					cmnd->stat_sn = conn->stat_sn++;
				}
				hdr->stat_sn = htonl(cmnd->stat_sn);
			}

			hdr->exp_cmd_sn = htonl(session->exp_cmd_sn);
			hdr->max_cmd_sn = htonl(session->max_cmd_sn);

			/* re-transmit only the requested Data PDU - SAI */
			if (!cmnd->retransmit_flg)
				hdr->data_sn = htonl(cmnd->data_sn++);
			else
				hdr->data_sn = htonl(prevsn);

			hdr->offset = htonl(pdu_offset);

			st_list = (struct scatterlist *)cmnd->cmnd->req->sr_buffer;

			/* if anything out of order, need to skip through command's
			 * scatter-gather list to find start of this pdu
			 */
			if (!session->oper_param->DataPDUInOrder
								|| !session->oper_param->DataSequenceInOrder) {
				cmnd->scatter_list_offset = skip_thru_sg_list(st_list,
													&cmnd->scatter_list_count,
													pdu_offset);
			}

			st_list += cmnd->scatter_list_count;

			TRACE(TRACE_DEBUG,
				  "offset into sg entry %d, offset into sg list" " %d\n",
				  cmnd->scatter_list_offset, cmnd->scatter_list_count);

			/* get number of iovec slots needed to send this DataIn payload */
			niov = find_iovec_needed(data_payload_length, st_list,
									 cmnd->scatter_list_offset);
			if (niov <= 0) {
				TRACE_ERROR("Trouble in find_iovec_needed\n");
				err = -1;
				goto out;
			}

			if (session->oper_param->DataPDUInOrder)
				pdu_offset += data_payload_length;	/* in order */

			/* now add on number of additional iovec slots this pdu needs */
			niov++;					/* add one for header */
			total_data_length = data_payload_length + ISCSI_HDR_LEN;
			/* Daren Hayward, darenh@4bridgeworks.com */
			#if defined(MANGLE_INQUIRY_DATA)
				siov = 1;
			#endif

			if (conn->hdr_crc) {
				niov++;				/* add one for hdr_crc */
				total_data_length += CRC_LEN;
				/* Daren Hayward, darenh@4bridgeworks.com */
				#if defined(MANGLE_INQUIRY_DATA)
					siov++;
				#endif
			}

			/* Daren Hayward, darenh@4bridgeworks.com */
			#if defined(MANGLE_INQUIRY_DATA)
				miov = niov;
			#endif

			padding = (-data_payload_length) & 3;
			if (padding) {
				niov++;				/* add one for padding */
				total_data_length += padding;
			}

			if (conn->data_crc) {
				niov++;				/* add one for data_crc */
				total_data_length += CRC_LEN;
			}

			iov = malloc(niov * sizeof(struct iovec));
			if (!iov) {
				TRACE(TRACE_DEBUG, "handle_iscsi_done: No space for iov\n");
				err = -1;
				goto out;
			}

			/* now go back and fill in the iovec slots */
			iov[0].iov_base = iscsi_hdr;
			iov[0].iov_len = ISCSI_HDR_LEN;

			cmnd->scatter_list_count +=
						fill_iovec(iov, 1 + conn->hdr_crc, niov, st_list,
								   &cmnd->scatter_list_offset,
								   data_payload_length);

/* Daren Hayward, darenh@4bridgeworks.com */
#if defined(MANGLE_INQUIRY_DATA)
			if (cmnd->cdb[0] == INQUIRY
								&& mangle_inquiry_data(cmnd, iov, siov, miov)) {
				err = -1;
				goto out;
			}
#endif

			if (padding) {
				iov[niov - 1 - conn->data_crc].iov_base = &pad_bytes;
				iov[niov - 1 - conn->data_crc].iov_len = padding;
				TRACE(TRACE_DEBUG, "padding attached: %d bytes\n", padding);
			}

			if (conn->data_crc) {
					iov[niov - 1].iov_len = CRC_LEN;
			}

			/* skip data PDU re-transmissions that are not asked for - SAI*/
			if (cmnd->retransmit_flg) {
				if (prevsn < cmnd->startsn || prevsn > cmnd->endsn)
					goto skip_transmission;
				TRACE_WARNING("Retransmitting DataIn, ITT %u, DataSN %u,"
							  " Buffer Offset %u\n",
							  cmnd->init_task_tag,
							  prevsn, ntohl(hdr->offset));
			}

			/* stop sending more data_in pdus in case command was aborted */
			if (cmnd->state == ISCSI_DEQUEUE) {
				err = 0;
				goto out1;
			}

			err = iscsi_tx_data(conn, iov, niov, total_data_length);

			if (err != total_data_length) {
				TRACE_ERROR("Trouble in iscsi_tx_data, expected %d "
							"bytes, got %d\n",
					 		total_data_length, err);
				err = -1;
				goto out1;
			}

			if (TRACE_TEST(TRACE_ISCSI_FULL)) {
				TRACE(TRACE_ISCSI_FULL, "DataIn sent, offset %u\n",
					  htonl(hdr->offset));
				print_targ_scsi_data_in(hdr);
			}

			/* Added for error recovery - SAI */
skip_transmission:

			ZFREE(iov);

			/* Increment the retransmitted data sequence number - SAI */
			prevsn++;

			seq_length -= data_payload_length;

			TRACE(TRACE_DEBUG,
				  "data sent %d data left in seq %d sg_list_offset %d "
				  "sg_entry_offset %d\n", err, seq_length,
				  cmnd->scatter_list_count, cmnd->scatter_list_offset);
		}

		/* end of this sequence of DataIn pdus, set up for next seq (if any) */
		if (session->oper_param->DataSequenceInOrder)
			seq_low_byte = seq_limit_byte;		/* in order */
		else
			seq_limit_byte = seq_low_byte;		/* reverse order */
	}

out:
	TRACE(TRACE_ENTER_LEAVE, "Leave send_read_data, err = %d\n", err);
	return err;
out1:
	free(iov);
	goto out;
}

/*
 * executed only by the tx thread when state is ISCSI_DONE.
 * handle_iscsi_done: This is responsible for building the data (if any)
 * and response headers and transmitting them to the correct initiator.
 * The command state is set to DEQUEUE to reflect that it is done
 * executing
 * INPUT: command that is done.
 * OUTPUT: 0 if everything is okay, < 0 if there is trouble
 */
static int
handle_iscsi_done(struct iscsi_cmnd *cmnd,
				  struct iscsi_conn *conn,
				  struct iscsi_session *session)
{
	Scsi_Request *req;
	int err = 0;
	int phase_collapse = 0;

	TRACE(TRACE_ENTER_LEAVE, "Enter handle_iscsi_done, cmnd %p, state %d\n",
		  cmnd, cmnd->state);

	if (cmnd->cmnd == NULL) {
		TRACE_ERROR("cmnd is NULL for cmnd %p\n", cmnd);
		err = -1;
		goto out;
	}

	req = cmnd->cmnd->req;

	if (req == NULL) {
		TRACE_ERROR("req is NULL for cmnd %p\n", cmnd);
		err = -1;
		goto out;
	}

	if ((req->sr_data_direction == SCSI_DATA_READ)
									&& host_byte(req->sr_result) == DID_OK) {
			err = send_read_data(cmnd, conn, session, &phase_collapse);
			if (err < 0)
				goto out;
	}

	if (!(cmnd->opcode_byte & I_BIT)) {
		/* the scsi command pdu was not immediate, CmdSN advances */
        pthread_mutex_lock(&session->cmnd_mutex);
		session->max_cmd_sn++;
		pthread_mutex_unlock(&session->cmnd_mutex);
	}

	err = 0;

	/* Separated Response for Error Recovery - SAI */
	if (cmnd->retransmit_flg) {
		cmnd->retransmit_flg = 0;
		cmnd->state = ISCSI_SENT;
	} else {
		if (phase_collapse == 0)
			err = send_iscsi_response(cmnd, conn, session);
		else
			cmnd->state = ISCSI_SENT;
	}

	/* in case there are any out-of-order commands that are now in-order */
	check_queued_cmnd(session);

out:
	TRACE(TRACE_ENTER_LEAVE, "Leave handle_iscsi_done, err = %d\n", err);
	return err;
}


/*
 * iscsi_tx_thread: This thread is responsible for the transmission of
 * responses and/or status information to the initiator
 */
int
iscsi_tx (struct iscsi_conn *conn)
{
	struct iscsi_session *session;
	struct iscsi_cmnd *cmnd, *prev_cmnd;
	int count, skipover;

    session = conn->session;
    
    skipover = 0;				/* don't skip anything yet */
    
#if 0
    if (test_and_clear_bit(NEED_NOPIN_BIT, &conn->control)) {
        /* bit was set, either first time through
         * or timer expired after an interval of silence from initiator
         */
        if (!timer_pending(&conn->nop_timer)) {
            /* first time thru here -- timer not going yet, start it up */
            restart_nop_timer(conn);
        } else {
            /* not first time thru here, timer must have expired */
            if ((count = atomic_read(&conn->outstanding_nopins))) {
                TRACE_ERROR("No reply to %u outstanding NopIns\n",
                            count);
                if (count >= MAX_OUTSTANDING_NOPINS) {
                    TRACE_ERROR("Max outstanding NopIns exceeded\n");
                    goto iscsi_tx_thread_out;
                }
                TRACE_SET(save_mask | TRACE_DEBUG | TRACE_ISCSI
                          | TRACE_ISCSI_FULL | TRACE_ENTER_LEAVE);
            }
            if (generate_nopin(conn, session) < 0)
                goto iscsi_tx_thread_out;
            atomic_inc(&conn->outstanding_nopins);
        }
    }
#endif

restart_after_dequeue:	/* back here after dequeueing a command from the list,
						 * at which time skipover will be set so we can skip
						 * ahead to our place in the list (which may have been
						 * changed by other tx threads while we were busy).
						 */
		/* lock the session-wide list of commands */
    pthread_mutex_lock(&session->cmnd_mutex);

    /* look at and count each command belonging to this session */
    count = 0;
    prev_cmnd = NULL;
    cmnd = session->cmnd_list;
    while (cmnd != NULL) {
        TRACE(TRACE_DEBUG, "pick up cmnd %p\n", cmnd);
        if (cmnd->conn == conn && ++count >= skipover) {
            /* this command is for this connection, handle it */
            if (!(conn->connection_flags & CONN_LOGGED_OUT)) {
				TRACE(TRACE_DEBUG, "handle cmnd no. %d, ITT %u, "
					  "opcode 0x%02x, state %d\n",
					  count, cmnd->init_task_tag,
					  cmnd->opcode_byte, cmnd->state);
				TRACE(TRACE_DEBUG, "ImmData %u, UnsolData %u, data_len %u, "
					  "data_done %u, r2t_data %d\n",
					  cmnd->immediate_data_present,
					  cmnd->unsolicited_data_present,
					  cmnd->data_length,
					  cmnd->data_done,
					  cmnd->r2t_data);
				switch (cmnd->state) 
                {
                    case ISCSI_SEND_TEXT_RESPONSE:
					{
                        pthread_mutex_unlock(&session->cmnd_mutex);
						if (handle_discovery_rsp(cmnd, conn, session) < 0) {
							TRACE_ERROR("Trouble in handle_discovery_rsp\n");
							goto iscsi_tx_thread_out;
						}
                        pthread_mutex_lock(&session->cmnd_mutex);
						break;
					}

                    case ISCSI_ASK_FOR_MORE_TEXT:
					{
                        pthread_mutex_unlock(&session->cmnd_mutex);
						if (ask_for_more_text(cmnd, conn, session) < 0) {
							TRACE_ERROR("Trouble in ask_for_more_text\n");
							goto iscsi_tx_thread_out;
						}
                        pthread_mutex_lock(&session->cmnd_mutex);
						break;
					}

                    case ISCSI_LOGOUT:
					{
						/* take this command out of the session-wide list */
						if (prev_cmnd == NULL)
							session->cmnd_list = cmnd->next;
						else
							prev_cmnd->next = cmnd->next;
                        pthread_mutex_unlock(&session->cmnd_mutex);
						if (handle_logout_rsp(cmnd, conn, session) < 0) {
							TRACE_ERROR("Trouble in handle_logout_rsp\n");
						}
						/* always exit the tx thread after a logout response */
						iscsi_dequeue(cmnd, conn);
						goto iscsi_tx_thread_out;
					}

                    case ISCSI_PING:
					{
                        pthread_mutex_unlock(&session->cmnd_mutex);
						if (handle_nopin(cmnd, conn, session) < 0) {
							TRACE_ERROR("Trouble in handle_nopin\n");
							goto iscsi_tx_thread_out;
						}
                        pthread_mutex_lock(&session->cmnd_mutex);
						break;
					}

                    case ISCSI_DONE:
					{
                        pthread_mutex_unlock(&session->cmnd_mutex);
						if (handle_iscsi_done(cmnd, conn, session) < 0) {
							TRACE_ERROR("Trouble in handle_iscsi_done\n");
							goto iscsi_tx_thread_out;
						}
                        pthread_mutex_lock(&session->cmnd_mutex);
						break;
					}
                    
                    case ISCSI_RESEND_STATUS:
					{
                        pthread_mutex_unlock(&session->cmnd_mutex);
						if (send_iscsi_response(cmnd, conn, session) < 0) {
							TRACE_ERROR("Trouble in send_iscsi_response\n");
							goto iscsi_tx_thread_out;
						}
                        pthread_mutex_lock(&session->cmnd_mutex);
						break;
					}

                    case ISCSI_MGT_FN_DONE:
					{
                        pthread_mutex_unlock(&session->cmnd_mutex);
#if 0
						if (handle_iscsi_mgt_fn_done(cmnd, conn, session) < 0)
#endif
                        {
							TRACE_ERROR("Trouble in iscsi_mgt_fn_done\n");
							goto iscsi_tx_thread_out;
						}
                        pthread_mutex_lock(&session->cmnd_mutex);
						break;
					}

                    case ISCSI_BUFFER_RDY:
					{
                        pthread_mutex_unlock(&session->cmnd_mutex);
#if 0
						if (iscsi_tx_r2t(cmnd, conn, session) < 0) 
#endif
                        {
							TRACE_ERROR("Trouble in iscsi_tx_r2t\n");
							goto iscsi_tx_thread_out;
						}
                        pthread_mutex_lock(&session->cmnd_mutex);
						break;
					}

                    case ISCSI_DEQUEUE:
					{
						TRACE(TRACE_DEBUG, "dequeue command, ITT %u, CmndSN %u,"
							  " count %u, skipover %u\n", cmnd->init_task_tag,
							  cmnd->cmd_sn, count, skipover);
						/* take this command out of the session-wide list */
						if (prev_cmnd == NULL)
							session->cmnd_list = cmnd->next;
						else
							prev_cmnd->next = cmnd->next;
                        pthread_mutex_unlock(&session->cmnd_mutex);
						iscsi_dequeue(cmnd, conn);
						skipover = count;
						goto restart_after_dequeue;
					}

                    case ISCSI_QUEUE_CMND_RDY:
					{
#if 0
						if (send_unsolicited_data(cmnd, conn, session) < 0) 
#endif
                        {
							TRACE_ERROR("Trouble in send_unsolicited_data\n");
                            pthread_mutex_unlock(&session->cmnd_mutex);
							goto iscsi_tx_thread_out;
						}
						break;
					}

                    case ISCSI_QUEUE_CMND:
                    case ISCSI_QUEUE_OTHER:
                    case ISCSI_CMND_RECEIVED:
                    case ISCSI_NEW_CMND:
                    case ISCSI_SENT:
                    case ISCSI_NOPIN_SENT:
                    case ISCSI_ALL_R2TS_SENT:
                    case ISCSI_IMMEDIATE_DATA_IN:
                    case ISCSI_UNSOLICITED_DATA_IN:
                    case ISCSI_DATA_IN:
                    case ISCSI_BLOCKED_SENDING_TEXT:
                    case ISCSI_AWAIT_MORE_TEXT:
					{
						/* Not much to do */
						break;
					}

                    default:		/* NEVER HERE */
					{
                        pthread_mutex_unlock(&session->cmnd_mutex);
						TRACE_ERROR("Unknown command state %u\n", cmnd->state);
						goto iscsi_tx_thread_out;
					}
				} /* switch */
            }
        } /* if */
        prev_cmnd = cmnd;
        cmnd = cmnd->next;
    } /* while */

    /* unlock the session-wide list of commands */
    pthread_mutex_unlock(&session->cmnd_mutex);

    TRACE(TRACE_DEBUG, "handled %d commands\n", count);

iscsi_tx_thread_out:
	return 0;
}

/*
 * executed only by midlevel's target_process_thread as a call-back.
 * iscsi_xmit_response: responsible for transmitting data and scsi
 * response
 */
int
iscsi_xmit_response(Target_Scsi_Cmnd * cmnd)
{
	struct iscsi_cmnd *cmd;
	struct iscsi_session *session;

	cmd = search_iscsi_cmnd(cmnd, &session);

	if (!cmd) {
		TRACE_ERROR("iscsi_xmit_response could not find cmnd %d\n",
					cmnd->id);
		return -1;
	}

	/* bump next expected cmd sequence number if necessary */
	session->exp_cmd_sn += cmd->cmd_sn_increment;
	cmd->cmd_sn_increment = 0;

	cmd->state = ISCSI_DONE;

	TRACE(TRACE_ISCSI, "CmdSN %u ITT %u done by target, ExpCmdSN %u\n",
					  cmd->cmd_sn, cmd->init_task_tag, session->exp_cmd_sn);
    /** The mutex has been locked in `search_iscsi_cmnd' **/
	pthread_mutex_unlock(&session->cmnd_mutex);

    iscsi_tx(cmd->conn);

	return 0;
}

/*
 * executed only by midlevel's target_process_thread as a call-back.
 * iscsi_rdy_to_xfer: function changes the state of the command
 * and wakes up the tx thread to send an R2T to the Initiator if needed
 */
int
iscsi_rdy_to_xfer(Target_Scsi_Cmnd * cmnd)
{
	struct iscsi_cmnd *cmd;
	struct iscsi_session *session;

	cmd = search_iscsi_cmnd(cmnd, &session);

	if (!cmd) {
		TRACE_ERROR("iscsi_rdy_to_xfer could not find cmnd %d\n",
                    cmnd->id);
		return -1;
	}

	/* bump next expected cmd sequence number if necessary */
	session->exp_cmd_sn += cmd->cmd_sn_increment;
	cmd->cmd_sn_increment = 0;

	/* allow r2t generation (if any) for this command */
	cmd->r2t_data = cmd->r2t_data_total;

	if (cmd->data_length == 0)
		/* this command expects no data from initiator, so it is done */
		cmd->state = ISCSI_DONE;
	else if (cmd->state == ISCSI_QUEUE_CMND)
		/* this command is queued, now it is queued and ready to receive */
		cmd->state = ISCSI_QUEUE_CMND_RDY;
	else {
		/* this command is ready to receive data from initiator */
		if (cmd->state != ISCSI_NEW_CMND) {
			TRACE_ERROR("iscsi_rdy_to_xfer for CmdSN %u ITT %u opcode 0x%02x"
						" expected state %u, got state %u, setting state %u\n",
						cmd->cmd_sn, cmd->init_task_tag,
						cmd->opcode_byte, ISCSI_NEW_CMND, cmd->state,
						ISCSI_BUFFER_RDY);
		}
		cmd->state = ISCSI_BUFFER_RDY;
	}

    /** the mutex has been locked in `search_iscsi_cmnd' **/
    pthread_mutex_unlock(&session->cmnd_mutex);

	/* tell iscsi rx thread that midlevel buffers are now ready */
	TRACE(TRACE_SEM, "iscsi_rdy_to_xfer: unblocking unsolicited_data_sem\n");
	sem_post(&cmd->unsolicited_data_sem);

    iscsi_tx(cmd->conn);

	return 0;
}

/*
 * iscsi_task_mgt_fn_done:
 *
 */
void
iscsi_task_mgt_fn_done(Target_Scsi_Message * msg)
{
	struct iscsi_cmnd *related_command;

	related_command = search_task_mgt_command(msg);

	if (!related_command) {
		TRACE_ERROR("Could not find msg %d\n", msg->message);
		return;
	}

	related_command->state = ISCSI_MGT_FN_DONE;

	/* wake up the transmit thread */
    iscsi_tx(related_command->conn);

	return;
}


/* computes the abort response when the referenced task is not found:
 * RFC 3720 section 10.6.1 Response
 * For the ABORT TASK function,
 * ...
 * "b) If the Referenced Task Tag does not identify an existing task,
 * but if the CmdSN indicated by the RefCmdSN field in the Task
 * Management function request is within the valid CmdSN window and less
 * than the CmdSN of the Task Management function request itself,
 * then targets must consider the CmdSN received and return the
 * 'Function complete' response.
 * "c) If the Referenced Task Tag does not identify an existing task
 * and if the CmdSN indicated by the RefCmdSN field in the Task
 * Management function request is outside the valid CmdSN window,
 * then targets must return the 'Task does not exist' response.
 */
static int __attribute__ ((no_instrument_function))
get_abort_response( struct iscsi_session *session,
					struct iscsi_cmnd *cmnd)
{
	int delta, retval = TASK_DOES_NOT_EXIST;

    pthread_mutex_lock(&session->cmnd_mutex);

	delta = session->max_cmd_sn - cmnd->ref_cmd_sn;
	if (delta < 0) {
		/* ref_cmd_sn is greater than MaxCmdSN, so outside valid CmdSN window */
		goto out_unlock;
	}

	delta = cmnd->ref_cmd_sn - session->exp_cmd_sn;
	if (delta < 0) {
		/* ref_cmd_sn is less than ExpCmdSN, so outside valid CmdSN window */
		goto out_unlock;
	}

	delta = cmnd->cmd_sn - cmnd->ref_cmd_sn;
	if (delta <= 0) {
		/* ref_cmd_sn is NOT less than cmd_sn */
		goto out_unlock;
	}

	/* target must consider the CmdSN received, and return FUNCTION_COMPLETE */
	session->exp_cmd_sn++;
	retval = FUNCTION_COMPLETE;

out_unlock:
    pthread_mutex_unlock(&session->cmnd_mutex);

	return retval;
}


static void __attribute__ ((no_instrument_function))
do_task_mgt(struct iscsi_conn *conn,
			struct iscsi_cmnd *cmnd)
{
	struct iscsi_cmnd *ref_command;

	if (cmnd->ref_function == TMF_ABORT_TASK) {
		ref_command = search_tags(conn, cmnd->ref_task_tag, ALL_ONES, 1);
		if (ref_command == NULL) {
			/* can't find command with this ITT, check the RefCmdSN field */
			TRACE_ERROR("No command with ITT %u\n",
						cmnd->ref_task_tag);
			cmnd->response = get_abort_response(conn->session, cmnd);
			cmnd->state = ISCSI_MGT_FN_DONE;
		} else {
			TRACE_ERROR("Aborting opcode 0x%02x, ITT %u, xfer_len %u, "
						"data_done %u, r2t_data %d, r2t_sn %u, state %u\n",
						ref_command->opcode_byte,
						ref_command->init_task_tag,
						ref_command->data_length,
						ref_command->data_done,
						ref_command->r2t_data,
						ref_command->r2t_sn,
						ref_command->state);
			cmnd->message = rx_task_mgmt_fn(conn->dev->device,
                                            cmnd->ref_function, ref_command->cmnd);
			if (!cmnd->message) {
				cmnd->response = FUNCTION_REJECTED;
				cmnd->state = ISCSI_MGT_FN_DONE;
			}
		}
	} else {
		/* we don't deal with this task management function (yet) */
		cmnd->response = TASK_MANAGEMENT_FUNCTION_NOT_SUPPORTED;
		cmnd->state = ISCSI_MGT_FN_DONE;
	}
}



/*
 * executed only by the rx thread.
 * Allocates new reject item, fills it in with header of rejected pdu,
 * and enqueues it for tx thread on reject_list of this connection.
 */
int
enqueue_reject(struct iscsi_conn *conn, uint8_t reason)
{
    return iscsi_tx_rjt(conn, conn->bad_hdr, reason);
}

/* called when out-of-order queued commands (except for SCSI Commands)
 * become in-order and have to be "delivered"
 */
static void __attribute__ ((no_instrument_function))
deliver_queue_other(struct iscsi_cmnd *cmnd, struct iscsi_session *session)
{
	uint8_t opcode;

	opcode = cmnd->opcode_byte & ISCSI_OPCODE;
	if (opcode == ISCSI_INIT_NOP_OUT) {
		/* this was an out-of-order NopIn */
		if (cmnd->init_task_tag == ALL_ONES) {
			/* NopIn not used as a ping request, done with it now */
			TRACE(TRACE_DEBUG, "Freeing NopIn, ITT 0x%08x, CmdSN %u\n",
					cmnd->init_task_tag, cmnd->cmd_sn);
			/* NopOut pdu was not immediate, CmdSN was advanced */
			session->max_cmd_sn++;
			ZFREE(cmnd->ping_data);
			cmnd->state = ISCSI_DEQUEUE;
		} else {
			cmnd->state = ISCSI_PING;
			TRACE(TRACE_DEBUG, "ping back CmdSN %u, ExpCmdSN %u, ITT %u"
							   " opcode 0x%02x, state %u, data_length "
							   "%u\n",
								cmnd->cmd_sn, session->exp_cmd_sn,
								cmnd->init_task_tag, cmnd->opcode_byte,
								cmnd->state, cmnd->data_length);
		}
	} else if (opcode == ISCSI_INIT_TEXT_CMND) {
		/* this was an out-of-order Text Request */
#if 0
		do_text_request(cmnd, cmnd->conn, session);
#endif
	} else if (opcode == ISCSI_INIT_LOGOUT_CMND) {
		/* this was an out-of-order Logout Request */
		cmnd->state = ISCSI_LOGOUT;
	} else if (opcode == ISCSI_INIT_TASK_MGMT_CMND) {
		/* this was an out-of-order Task Management Request */
		do_task_mgt(cmnd->conn, cmnd);
	} else {
		/* should never happen */
		TRACE_ERROR("Unexpected queue other ITT 0x%08x, CmdSN %u, "
					"opcode 0x%02x\n", 
					cmnd->init_task_tag, cmnd->cmd_sn, cmnd->opcode_byte);
	}
}

/*	cdeng: called by any rx_thread or rx_thread.
 *	Checks session's command queue to see if any out-of-order command is now
 *	in order.  If so, and it is a SCSI command,
 *	call rx_cmnd for it as the same as the handle_cmnd does
 */
static void
check_queued_cmnd(struct iscsi_session *session)
{
	struct iscsi_init_scsi_cmnd *pdu;
	struct iscsi_cmnd *temp, *prev = NULL;

	/* need to restart search loop whenever ExpCmdSN is incremented,
	 * which happens when we find and deliver a previously out-of-order
	 * command because it is now in order.
	 */
restart:
	/* lock the session-wide list of commands while searching it */
	pthread_mutex_lock(&session->cmnd_mutex);

	for (temp = session->cmnd_list; temp != NULL; temp = temp->next) {
		if (temp->cmd_sn == session->exp_cmd_sn
			&& !(temp->opcode_byte & I_BIT)
			&& temp->state != ISCSI_DEQUEUE) {
			TRACE(TRACE_ISCSI, "CmdSN %u ITT %u now in order\n",
				  temp->cmd_sn, temp->init_task_tag);

			/* ok now to deliver the command to the target in proper order */
			if (temp->state == ISCSI_QUEUE_OTHER) {
				/* this is non-SCSI command, deliver it to us, not target */
				session->exp_cmd_sn++;
                pthread_mutex_unlock(&session->cmnd_mutex);
				deliver_queue_other(temp, session);
				goto restart;				/* exp_cmd_sn has been bumped */
			} else if (!temp->cmd_sn_increment) {
				/* this was an out-of-order (non-immediate) SCSI Command that
				 * is now in-order, deliver it to target and mark it
				 * until delivery is confirmed */
				temp->cmd_sn_increment = 1;
				pthread_mutex_unlock(&session->cmnd_mutex);
				TRACE(TRACE_ISCSI, "delivering CmdSN %u ITT %u to target\n",
					  temp->cmd_sn, temp->init_task_tag);
				pdu = (struct iscsi_init_scsi_cmnd *)temp->hdr;
				rx_cmnd(temp->conn->dev->device,
						session->oper_param->TargetName, pdu->lun, pdu->cdb,
						ISCSI_CDB_LEN, pdu->xfer_len, pdu->flags, &temp->cmnd);

				if (temp->cmnd)					/* rx_cmnd worked ok */
					goto restart;				/* exp_cmd_sn may be bumped */

				TRACE_ERROR("rx_cmnd returned NULL, ITT %u\n",
							pdu->init_task_tag);

                pthread_mutex_lock(&session->cmnd_mutex);
				/* have to bump this on error */
				session->exp_cmd_sn += temp->cmd_sn_increment;
				temp->cmd_sn_increment = 0;
                pthread_mutex_unlock(&session->cmnd_mutex);
                return;
			}
		}
		prev = temp;
	}
	pthread_mutex_unlock(&session->cmnd_mutex);
	return;
}

/*	Draft 20, Section 3.2.2.1 Command Numbering and Acknowledging
 *	...
 *	For non-immediate commands, the CmdSN field can take any value from
 *	ExpCmdSN to MaxCmdSN inclusive.  The target MUST silently ignore
 *	any non-immediate command outside of this range or non-immediate
 *	duplicates within the range.  The CmdSN carried by immediate commands
 *	may lie outside the ExpCmdSN to MaxCmdSN range.
 */
/*	RDR: check that cmd_sn is within the range [ExpCmdSN .. MaxCmdSN]
 *	returns 0 if equal to exp_cmd_sn (and exp_cmd_sn is updated)
 *			1 if greater than exp_cmd_sn (exp_cmd_sn not updated)
 *			-1 if outside the range (and hence should be ignored)
 *			0 for all immediate commands, regardless of cmd_sn value
 *  when called, session->cmnd_mutex is locked so access to session-wide
 *  max_cmd_sn and exp_cmd_sn fields is atomic.
 */
static int
check_cmd_sn(struct iscsi_cmnd *cmnd, void *ptr, struct iscsi_session *session,
			 uint32_t increment)
{
	struct generic_pdu *pdu = ptr;
	int delta;

	if (!(pdu->opcode & I_BIT)) {
		/* this is an non-immediate command */
		delta = session->max_cmd_sn - pdu->cmd_sn;
		if (delta < 0) {		/* cmd_sn is greater than MaxCmdSN */
			return -1;
		}

		delta = pdu->cmd_sn - session->exp_cmd_sn;
		if (delta < 0) {		/* cmd_sn is less than ExpCmdSN */
			return -1;
		}

		if (delta > 0) {		/* cmd_sn is in range, but out of order */
			return 1;
		}

		/* non-immediate command is in expected order */
		if (increment) {
			session->exp_cmd_sn++;
		} else {
			/* exp_cmd_sn is not incremented.
			 * This only happens on in-order, non-immediate SCSI command pdus,
			 * which don't bump exp_cmd_sn until after midlevel confirms
			 * command was delivered.  Set cmd_sn_increment to tell
			 * check_queued_cmnd() not to process this command once it has been
			 * queued!
			 */
			cmnd->cmd_sn_increment = 1;			/* increment exp_cmd_sn later */
		}
	}

	return 0;
}

/*
 * executed only by the rx thread.
 * find commands for this connection already in the queue that have already
 * sent everything and are being ACKed by this command in order to mark them
 * for dequeueing.
 * Then adds this command to end of queue if flag is set.
 * Finally, if any command's state was changed, or if this command was added
 * to the queue, wake up the tx thread.
 * NOTE: cmnd MAY be NULL, and if it is, add_cmnd_to_queue MUST be 0!
 * returns 0 on success, -1 on error.
 */
static int
ack_sent_cmnds(struct iscsi_conn *conn,
               struct iscsi_cmnd *cmnd,
               uint32_t exp_stat_sn,
               te_bool add_cmnd_to_queue)
{
	struct iscsi_cmnd *temp;
	int delta, count;
    te_bool changed_something = add_cmnd_to_queue;

	pthread_mutex_lock(&conn->session->cmnd_mutex);

	count = 0;
	for (temp = conn->session->cmnd_list; temp != NULL; temp = temp->next) {
		if (temp->conn == conn) {
			count++;
			if (temp->state == ISCSI_SENT) {
				delta = temp->stat_sn - exp_stat_sn;
				if (delta < 0) {
					TRACE(TRACE_DEBUG, "set dequeue command statsn %d, "
						  "received exp_stat_sn %d, command state %d\n",
						  temp->stat_sn, exp_stat_sn, temp->state);
					temp->state = ISCSI_DEQUEUE;
					changed_something = TRUE;
				}
			}
		}

		if (temp->next == NULL)
			break;
	}

	if (add_cmnd_to_queue) {
		/* Add this command to the queue */
		TRACE(TRACE_DEBUG, "add command %p to queue, ITT %u, CmdSN %u, "
			  "state %d, count %d\n", cmnd, cmnd->init_task_tag,
			  cmnd->cmd_sn, cmnd->state, count);
		count++;
		if (temp)
			temp->next = cmnd;
		else
			conn->session->cmnd_list = cmnd;
	}

    pthread_mutex_unlock(&conn->session->cmnd_mutex);

	/* tell the tx thread it has something to do */
	if (changed_something) {
        iscsi_tx(conn);
	}

	return 0;
}


/*
 * executed only by the rx thread.
 * called only by handle_cmnd()
 * when called, err is +1 if cmnd is in range but out of order
 *				err is -1 if cmnd is out of range and should be ignored
 * returns	0 if command is queued ok or freed because it was out of range
 *			-1 if there was an error (no memory to save immediate data)
 */
static int
out_of_order_cmnd(struct iscsi_conn *conn,
				  struct iscsi_session *session,
				  uint8_t *buffer,
				  struct iscsi_cmnd *cmnd, int err)
{
	struct iscsi_init_scsi_cmnd *hdr = (struct iscsi_init_scsi_cmnd *) buffer;
	int retval;

	/* store "length" bytes of immediate data attached to this command */
	if ((retval = save_unsolicited_data(cmnd, 0, (struct generic_pdu *)hdr))) {
		free(cmnd);
		goto out;
	}

	memcpy(cmnd->hdr, buffer, ISCSI_HDR_LEN);
	cmnd->state = ISCSI_QUEUE_CMND;

	/* last param is 0 (don't queue) if cmnd is out of range (err == -1) */
	ack_sent_cmnds(conn, cmnd, hdr->exp_stat_sn, err+1);

	if (err < 0) {	
		/* out of range, silently ignore it */
		TRACE_ERROR("ignoring out of range CmdSN %u, ExpCmdSN %u, ITT %u, "
					"opcode 0x%02x\n", 
					cmnd->cmd_sn, session->exp_cmd_sn,
					cmnd->init_task_tag, cmnd->opcode_byte);
		/* command was not put into queue */
		free_data_list(cmnd);
		free(cmnd);
	} else {		
		/* within range but out of order, we queued it for later (above) */
		TRACE(TRACE_ISCSI, "out of order CmdSN %u bigger than ExpCmdSN %u\n",
			  cmnd->cmd_sn, cmnd->session->exp_cmd_sn);
	}
out:
	return retval;
}


static int
handle_cmnd(struct iscsi_conn *conn,
			struct iscsi_session *session,
			uint8_t *buffer)
{
	struct iscsi_init_scsi_cmnd *pdu = (struct iscsi_init_scsi_cmnd *) buffer;
	struct iscsi_cmnd *cmnd;
	int err;

	TRACE(TRACE_ENTER_LEAVE, "Enter handle_cmnd\n");

	if (TRACE_TEST(TRACE_ISCSI_FULL))
		print_init_scsi_cmnd(pdu);

	pdu->length = ntohl(pdu->length);
	pdu->init_task_tag = ntohl(pdu->init_task_tag);
	pdu->xfer_len = ntohl(pdu->xfer_len);
	pdu->cmd_sn = ntohl(pdu->cmd_sn);
	pdu->exp_stat_sn = ntohl(pdu->exp_stat_sn);

	/* check data length, immediate data should not bigger than 
	 * MaxRecvPDULength or FirstBurstLength
	 */
	if (pdu->length > conn->max_recv_length
		|| pdu->length > session->oper_param->FirstBurstLength) {
		TRACE_WARNING("DSL %u too big\n", pdu->length);
	}

	if( (cmnd = get_new_cmnd()) == NULL) {
		err = -1;
		goto out;
	}

	cmnd->conn = conn;
	cmnd->session = session;
	cmnd->opcode_byte = pdu->opcode;
	cmnd->init_task_tag = pdu->init_task_tag;
	cmnd->data_length = pdu->xfer_len;
	cmnd->lun = pdu->lun;
	cmnd->cdb[0] = pdu->cdb[0];
	cmnd->cdb[4] = pdu->cdb[4];
	cmnd->cmd_sn = pdu->cmd_sn;
	cmnd->first_burst_len = pdu->length;
	cmnd->stat_sn = pdu->exp_stat_sn;

	if (pdu->length)
		/* there is data attached to this PDU, it must be immediate data */
		cmnd->immediate_data_present = 1;

	if (!(pdu->flags & F_BIT)) {
		/* F bit not set, so unsolicited DataIn PDUs must follow this PDU */
		cmnd->unsolicited_data_present = 1;
		/* we solicit all data that might follow a full first burst
		 * NOTE: this subtraction can produce a negative value!
		 */
		cmnd->r2t_data_total = cmnd->data_length
								- session->oper_param->FirstBurstLength;
	} else {
		/* no unsolicited DataIn PDUs, we solicit all data except immediate */
		cmnd->r2t_data_total = cmnd->data_length - pdu->length;
	}

	/* Retransmit Flag for Error Recovery - SAI */

	/* error recovery ver ref18_04 - SAI */

	sem_init(&cmnd->unsolicited_data_sem, 0, 1);

	/* check cmd_sn for command ordering */
    pthread_mutex_lock(&session->cmnd_mutex);

	/* Last parameter is 0 because we cannot bump the exp_cmd_sn for the session
	 * until this command has been delivered to the mid-level in proper order.
	 * (bumping exp_cmd_sn might let other commands jump ahead of this)
	 * Also causes command list to become blocked if this command is in order.
	 */
	err = check_cmd_sn(cmnd, pdu, session, 0);

	/* generate the next TTT, in case we need it */
	cmnd->target_xfer_tag = generate_next_ttt(session);
    pthread_mutex_unlock(&session->cmnd_mutex);

	if (err) {
		/* command received is not in expected CmdSN order */
		err = out_of_order_cmnd(conn, session, buffer, cmnd, err);
		/* cmnd has now been queued or freed, as appropriate */
		goto out;
	}

	/* else command is immediate or (is in-order and cmnd_list is blocked) */

	TRACE(TRACE_DEBUG, "unsolicited_data_present %d, err %d, flags "
		  "0x%02x\n", cmnd->unsolicited_data_present, err, pdu->flags);

	ack_sent_cmnds(conn, cmnd, pdu->exp_stat_sn, 1);

	/* pass this command on to the midlevel so it can set up the buffers */
	rx_cmnd(conn->dev->device, session->oper_param->TargetName,
			pdu->lun, pdu->cdb, ISCSI_CDB_LEN, pdu->xfer_len,
			pdu->flags, &cmnd->cmnd);

	if (!(cmnd->cmnd)) {
		TRACE_ERROR("rx_cmnd returned NULL, ITT %u\n",
					cmnd->init_task_tag);
		err = -1;
		pthread_mutex_lock(&session->cmnd_mutex);
			/* bump exp_cmd_sn as needed */
        session->exp_cmd_sn += cmnd->cmd_sn_increment;
        cmnd->cmd_sn_increment = 0;
		pthread_mutex_unlock(&session->cmnd_mutex);
		goto out2;
	}

	if (cmnd->immediate_data_present) {
		/* this WRITE pdu has immediate data attached,
		 * wait for midlevel to set up buffers so we can
		 * read it directly into midlevel buffers
		 */
		TRACE(TRACE_SEM, "Blocked on unsolicited_data_sem\n");
		sem_wait(&cmnd->unsolicited_data_sem);
		TRACE(TRACE_SEM, "Unblocked on unsolicited_data_sem\n");

		/* state MUST now be ISCSI_BUFFER_RDY !*/
		if (cmnd->state != ISCSI_BUFFER_RDY) {
			TRACE_ERROR("got cmnd->state %u, expected %u\n",
						cmnd->state, ISCSI_BUFFER_RDY);
		}

		/* now read immediate data directly into midlevel buffers */
		err = read_list_data_seg((struct generic_pdu *) pdu, cmnd,
				(struct scatterlist *) cmnd->cmnd->req->sr_buffer, 0);
		if (err <= 0) {
			if (err) {
				TRACE_ERROR("read_list_data_seg returned error %d\n", err);
			} /* else recovered error */
			goto out2;
		}

		if (!session->oper_param->DataPDUInOrder) {
			/* immediate data should always begin from offset 0 */
			cmnd->seq_range_list.offset = 0;
			cmnd->pdu_range_list.offset = 0;
			cmnd->pdu_range_list.limit = pdu->length;
			merge_offset_length(&cmnd->pdu_range_list, 0, pdu->length);
		}

		if (!session->oper_param->DataSequenceInOrder
			&& !cmnd->unsolicited_data_present
			&& cmnd->data_done < cmnd->data_length) {
			merge_offset_length(&cmnd->seq_range_list, 0, pdu->length);
		}

		if (cmnd->data_done >= cmnd->data_length) {
			/* all the data was sent as immediate, there will be no
			 * unsolicited or solicited data to follow, therefore no need
			 * to generate any R2Ts.
			 * the WRITE is finished
			 */
			if (!session->oper_param->DataPDUInOrder) {
				check_range_list_complete(&cmnd->pdu_range_list);
				free_range_list(&cmnd->pdu_range_list);
			}

			TRACE(TRACE_DEBUG, "%d received for cmnd %p\n", pdu->length, cmnd);

            pthread_mutex_lock(&session->cmnd_mutex);
            cmnd->state = ISCSI_DATA_IN;
            err = scsi_rx_data(cmnd->cmnd);
            pthread_mutex_unlock(&session->cmnd_mutex);

			if (err < 0) {
				TRACE_ERROR("scsi_rx_data returned an error\n");
				goto out2;
			}
		} 
	}

out:
	TRACE(TRACE_ENTER_LEAVE, "Leave handle_cmnd, err = %d\n", err);
	return err;

out2:
	/* here for errors that are detected after cmnd is added to queue */
	cmnd->state = ISCSI_DEQUEUE;
	goto out;
    return 0;
}


/*
 * executed only by the rx thread.
 * called when a Task Management Request has just been received.
 * INPUT: connection, session and the buffer containing the TM pdu
 * OUTPUT: 0 if everything is okay, < 0 if there is trouble
 */
static int
handle_task_mgt_command(struct iscsi_conn *conn,
						struct iscsi_session *session,
						uint8_t *buffer)
{
	struct iscsi_init_task_mgt_command *pdu
						= (struct iscsi_init_task_mgt_command *)buffer;
	struct iscsi_cmnd *cmnd;
	int err = 0;
	uint32_t save_trace;

	/* turn on (almost) all tracing while processing a TM command */
	TRACE_GET(save_trace);
	TRACE_SET(TRACE_ALL
				& ~(TRACE_NET | TRACE_BUF | TRACE_MY_MEMORY | TRACE_TIMERS));

	if (TRACE_TEST(TRACE_ISCSI_FULL))
		print_init_task_mgt_command(pdu);

	pdu->init_task_tag = ntohl(pdu->init_task_tag);
	pdu->ref_task_tag = ntohl(pdu->ref_task_tag);
	pdu->cmd_sn = ntohl(pdu->cmd_sn);
	pdu->exp_stat_sn = ntohl(pdu->exp_stat_sn);
	pdu->ref_cmd_sn = ntohl(pdu->ref_cmd_sn);
	pdu->exp_data_sn = ntohl(pdu->exp_data_sn);

	TRACE_ERROR("Got TM Req, ITT %u, RTT %u, CmdSN %u, ref CmdSN %u, "
				"ExpCmdSN %u, ExpStatSN %u\n",
				pdu->init_task_tag, pdu->ref_task_tag,
				pdu->cmd_sn, pdu->ref_cmd_sn, session->exp_cmd_sn,
				pdu->exp_stat_sn);

	if (!(pdu->function & F_BIT)) {
		TRACE_ERROR("task mgt request with F_BIT 0\n");
	}

	pdu->function = pdu->function & 0x7f;

	/* RFC 3720 Section 10.5.4 Referenced Task Tag
	 * "The Initiator (sic -- should be Referenced) Task Tag of the task to be
	 * aborted for the ABORT TASK function or reassigned for the TASK REASSIGN
	 * function.  For all the other functions this field MUST be set to the
	 * reserved value 0xffffffff."
	 */
	if ((pdu->function != TMF_ABORT_TASK)
		&& (pdu->function != TMF_TASK_REASSIGN)
		&& (pdu->ref_task_tag != ALL_ONES)) {
		TRACE_ERROR("task mgt request with RTT "
					"%u, should be 0x%08x for function %d\n",
					pdu->ref_task_tag, ALL_ONES, pdu->function);
	}

	if( (cmnd = get_new_cmnd()) == NULL) {
		err = -1;
		goto out;
	}

	cmnd->conn = conn;
	cmnd->session = session;
	cmnd->opcode_byte = pdu->opcode;
	cmnd->init_task_tag = pdu->init_task_tag;
	cmnd->ref_task_tag = pdu->ref_task_tag;
	cmnd->ref_cmd_sn = pdu->ref_cmd_sn;
	cmnd->ref_function = pdu->function;
	cmnd->cmd_sn = pdu->cmd_sn;
	cmnd->stat_sn = pdu->exp_stat_sn;
	cmnd->response = FUNCTION_COMPLETE;

	if (pdu->length > 0) {
		/* read length bytes of text data into a newly-allocated buffer */
		TRACE_ERROR("Data attached to task mgt request, CmdSN %u, ExpCmdSN "
					"%u, ITT %u, opcode 0x%02x\n", 
					cmnd->cmd_sn, session->exp_cmd_sn,
					cmnd->init_task_tag, cmnd->opcode_byte);
		err = read_single_data_seg(buffer, cmnd, pdu->length, &cmnd->ping_data);
		if (err <= 0) {
			ZFREE(cmnd);
			goto out;
		}
	}

    pthread_mutex_lock(&session->cmnd_mutex);
    err = check_cmd_sn(cmnd, pdu, session, 1);
    pthread_mutex_unlock(&session->cmnd_mutex);

	if (err < 0) {	
		/* out of range, silently ignore it */
		TRACE_ERROR("Dropping out of range task mgt request CmdSN %u, "
					"ExpCmdSN %u\n", 
					pdu->cmd_sn, session->exp_cmd_sn);
		ack_sent_cmnds(conn, cmnd, pdu->exp_stat_sn, 0);
		free(cmnd->ping_data);
		ZFREE(cmnd);
	} else {
		if (err > 0) {		
			/* within range but out of order, queue it for later */
			cmnd->state = ISCSI_QUEUE_OTHER;
		} else {
			/* is immediate command or expected non-immediate command */
			do_task_mgt(conn, cmnd);
		}

		ack_sent_cmnds(conn, cmnd, pdu->exp_stat_sn, 1);
	}
	err = 0;
out:
	/* restore tracing to what it was when we came in here */
	TRACE_SET(save_trace);

	return err;
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
    if (scsi_target_init() != 0)
    {
        TRACE_ERROR("Can't initialize SCSI target");
        return -1;
    }
    
    /* devdata init */
    devdata = malloc(sizeof(struct iscsi_global));

    memset(devdata, 0, sizeof(struct iscsi_global));

    INIT_LIST_HEAD(&devdata->session_list);
    INIT_LIST_HEAD(&devdata->bad_session_list);

    pthread_mutex_init(&devdata->session_mutex, NULL);
    pthread_mutex_init(&devdata->session_read_mutex, NULL);
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

	devdata->device = make_target_front_end();

	if (!devdata->device) {
		TRACE_ERROR("Device registration failed\n");
        return -2;
	}

	devdata->device->dev_specific = (void *) devdata;

	TRACE(TRACE_DEBUG, "Registration complete\n");

    return 0;
}


/* 
 * iscsi_rx_thread: This thread is responsible for receiving iSCSI PDUs
 * and messages from the Initiator and then figuring out what to do with
 * them
 */
void *
iscsi_server_rx_thread(void *param)
{
	struct iscsi_conn *conn;
	struct targ_error_rec err_rec;
	uint8_t buffer[ISCSI_HDR_LEN];		/* read 48-byte PDU header in here */
	uint32_t digest;					/* read 4-byte header digest in here */
	uint32_t hdr_crc;					/* compute PDU header digest in here */
	uint32_t opcode;					/* extract 6-bit PDU opcode into here */
	uint32_t local_itt;				/* convert 4-byte PDU ITT into here */
	int err;
    te_bool terminate = FALSE;

    iscsi_param = (iscsi_target_thread_params_t *)param;

    /****************/ 

    conn = build_conn_sess(iscsi_param->send_recv_csap, iscsi_portal_groups);

    if (conn == NULL)
    {
        TRACE_ERROR("Error init connection\n");
        return NULL;
    }

	/* receive loop */
	while (!terminate) 
    {
		/* receive iSCSI header */
		err = iscsi_recv_msg(conn->conn_socket, ISCSI_HDR_LEN, buffer, 
                             conn->connection_flags);
		if (err != ISCSI_HDR_LEN)
        {
            TRACE_ERROR("Cannot read iSCSI header: %d", err);
            break;
        }

		TRACE_BUFFER(TRACE_BUF, buffer, ISCSI_HDR_LEN,
					 " Got PDU header\n");

		opcode = buffer[0] & ISCSI_OPCODE;

		local_itt = ntohl(((struct generic_pdu *)buffer)->init_task_tag);

		if (!(conn->session->tsih)) {
			/* not in full feature phase yet, only accept Login request pdus */
			if (opcode != ISCSI_INIT_LOGIN_CMND) {
				TRACE_ERROR("ITT %u has opcode 0x%02x, expected 0x%02x\n",
							local_itt, opcode,
							ISCSI_INIT_LOGIN_CMND);
				/* MUST immediately terminate the connection */
                break;
			}

			TRACE(TRACE_ISCSI, "Got login request, ITT %u\n",
				  local_itt);

			if (handle_login(conn, buffer) < 0) {
				TRACE_ERROR("Trouble in handle_login\n");
                break;
			}
			continue;
		}

		/* keep a copy of this header in case we need it in a later reject */
		memcpy(conn->bad_hdr, buffer, ISCSI_HDR_LEN);

		/* connection belongs to session that is in full feature phase */
		if (conn->hdr_crc) {
			/* read and check header digest on this pdu */
			TRACE(TRACE_DEBUG, "iscsi_rx_thread: Header digest check\n");
			hdr_crc = 0;
			do_crc(buffer, ISCSI_HDR_LEN, &hdr_crc);

            if ((err = iscsi_recv_msg(conn->conn_socket, CRC_LEN, 
                                      (char *)&digest, 
                                      conn->connection_flags)) != CRC_LEN) 
                break;

			if (hdr_crc != digest) 
            {
				TRACE_ERROR("Expected header crc 0x%08x, got 0x%08x\n",
							ntohl(hdr_crc), ntohl(digest));
				/* Error Recovery Code Added by SAI */
				TRACE(TRACE_ERROR_RECOVERY,
					  "Start header digest error recovery\n");
				err_rec.curr_conn = conn;
				err_rec.pdu_hdr = (struct generic_pdu *)buffer;
				err_rec.pdu_hdr->length = ntohl(err_rec.pdu_hdr->length);
				err_rec.err_type = HEADER_DIGERR;
				if ((err = targ_do_error_recovery(&err_rec)) < 0)
                    break;
				continue;
			} 
            else 
            {
				TRACE(TRACE_ISCSI_FULL, "Got header crc 0x%08x\n",
					  ntohl(digest));
			}
		}

		switch (opcode) 
        {
            case ISCSI_INIT_LOGIN_CMND:
			{
				/* got Login request while in full feature phase */
				TRACE_ERROR(" Got login request ITT %u in full feature "
							"phase\n", local_itt);
                terminate = TRUE;
                continue;
			}
#if 0
            case ISCSI_INIT_TEXT_CMND:
			{
				TRACE(TRACE_ISCSI, "Got text request, ITT %u\n",
					  local_itt);
                
				if (handle_text_request(conn, conn->session, buffer) < 0) {
					TRACE_ERROR("Trouble in handle_text_request, ITT %u\n",
								local_itt);
                    terminate = TRUE;
                    continue;
				}

				break;
			}
#endif

            case ISCSI_INIT_SCSI_CMND:
			{
				TRACE(TRACE_ISCSI, "Got SCSI command, CmdSN %u, ITT %u\n",
					  ntohl(((struct generic_pdu *)buffer)->cmd_sn),
					  local_itt);

				if (handle_cmnd(conn, conn->session, buffer) < 0) {
					TRACE_ERROR("Trouble in handle_cmnd, ITT %u\n",
								local_itt);
                    terminate = TRUE;
                    continue;
				}

				break;
			}

#if 0
            case ISCSI_INIT_SCSI_DATA_OUT:
            {
				TRACE(TRACE_ISCSI, "Got data-out, ITT %u, offset %u\n",
					  local_itt,
					  ntohl(((struct generic_pdu *) buffer)->offset));
                
				if (handle_data(conn, conn->session, buffer) < 0) {
					TRACE_ERROR("Trouble in handle_data, ITT %u\n",
								local_itt);
                    terminate = TRUE;
                    continue;
				}
                
				break;
			}
#endif

            case ISCSI_INIT_TASK_MGMT_CMND:
			{
				TRACE(TRACE_ISCSI,
					  "Got task mgt command, ITT %u\n",
					  local_itt);
                
				if (handle_task_mgt_command(conn, conn->session, buffer) < 0) {
					TRACE_ERROR("Trouble in handle_task_mgt_cmnd, ITT %u\n",
								local_itt);
                    terminate = TRUE;
                    continue;
				}

				break;
			}

            case ISCSI_INIT_LOGOUT_CMND:
			{
				TRACE(TRACE_ISCSI, "Got logout request, ITT %u\n",
					  local_itt);
                
				if (handle_logout(conn, conn->session, buffer) < 0) {
					TRACE_ERROR("Trouble in handle_logout, ITT %u\n",
								local_itt);
                    terminate = TRUE;
                    continue;
				}

				break;
			}

#if 0
            case ISCSI_INIT_NOP_OUT:
			{
				TRACE(TRACE_ISCSI, "Got NOP_OUT, ITT %u\n",
					  local_itt);

				if (handle_nopout(conn, conn->session, buffer) < 0) {
					TRACE_ERROR("Trouble in handle_nopout, ITT %u\n",
								local_itt);
                    terminate = TRUE;
                    continue;
				}

				break;
			}

			/* SNACK Handling by Target - SAI */
            case ISCSI_INIT_SNACK:
			{
				TRACE(TRACE_ERROR_RECOVERY, "Got SNACK Request\n");

				if (handle_snack(conn, conn->session, buffer) < 0) {
					TRACE_ERROR("Trouble in handle_snack\n");
                    terminate = TRUE;
                    continue;
				}

				break;
			}
#endif

            case ISCSI_TARG_NOP_IN:
            case ISCSI_TARG_SCSI_RSP:
            case ISCSI_TARG_TASK_MGMT_RSP:
            case ISCSI_TARG_LOGIN_RSP:
            case ISCSI_TARG_TEXT_RSP:
            case ISCSI_TARG_SCSI_DATA_IN:
            case ISCSI_TARG_LOGOUT_RSP:
            case ISCSI_TARG_R2T:
            case ISCSI_TARG_ASYNC_MSG:
            case ISCSI_TARG_RJT:
			{
				TRACE_ERROR("Initiator sent a target opcode %.2x, ITT %u\n",
							opcode, local_itt);
				/*****
				TRACE_SET(TRACE_DEBUG | TRACE_ISCSI | TRACE_ERROR_RECOVERY
							  | TRACE_ISCSI_FULL | TRACE_ENTER_LEAVE);
				*****/

				/* Send a Reject PDU and escalate to session recovery */
				enqueue_reject(conn, REASON_PROTOCOL_ERR);
				targ_session_recovery(conn);
                terminate = TRUE;
                continue;
			}

            default:
			{
				TRACE_ERROR("Got unknown opcode %.2x, ITT %u\n",
							opcode, local_itt);
				/*****
				TRACE_SET(TRACE_DEBUG | TRACE_ISCSI | TRACE_ERROR_RECOVERY
							  | TRACE_ISCSI_FULL | TRACE_ENTER_LEAVE);
				*****/

				/* Send a Reject PDU and escalate to session recovery */
				enqueue_reject(conn, REASON_COMMAND_NOT_SUPPORTED);
				targ_session_recovery(conn);
                terminate = TRUE;
                continue;
			}
		} /* switch */

		/* in case there are any out-of-order commands that are now in-order */
		check_queued_cmnd(conn->session);

	} /* while(!terminate) */


    iscsi_release_connection(conn);
	return NULL;
}
