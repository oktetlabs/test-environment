/*	target/iscsi_target.h

	vi: set autoindent tabstop=8 shiftwidth=8 :

	header file for the iSCSI Target

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

#ifndef	_ISCSI_TARGET_H
#define	_ISCSI_TARGET_H

/* #include "scsi_target.h" */

#include "../common/iscsi_common.h"
#include "../common/debug.h"
#include "../common/range.h"
#include "../common/crc.h"
#include "../common/tcp_utilities.h"

/* chap and srp support - CHONG */
#include "../security/misc/misc_func.h"
#include "../security/chap/chap.h"
#include "../security/srp/srp.h"

#include "../common/text_param.h"
#include "../common/target_negotiate.h"
#include "scsi_target.h"
#include "iscsi_portal_group.h"


struct data_list
{
	uint32_t offset;
	uint32_t length;
	char *buffer;
	struct data_list *next;
};

/* values for "state" in struct iscsi_cmnd */
#define ISCSI_CMND_RECEIVED			1
#define ISCSI_NEW_CMND				2
#define ISCSI_BUFFER_RDY			3
#define ISCSI_DONE				4
#define ISCSI_SENT				5
#define ISCSI_DEQUEUE				6
#define ISCSI_ALL_R2TS_SENT			7
#define ISCSI_IMMEDIATE_DATA_IN			8
#define ISCSI_UNSOLICITED_DATA_IN		9
#define ISCSI_DATA_IN				10
#define ISCSI_MGT_FN_DONE			11
#define ISCSI_SEND_TEXT_RESPONSE		12
#define ISCSI_LOGOUT				13
#define ISCSI_PING				14
#define ISCSI_QUEUE_CMND			15
#define ISCSI_QUEUE_CMND_RDY			16
#define ISCSI_QUEUE_OTHER			17
#define ISCSI_NOPIN_SENT			18
#define ISCSI_RESEND_STATUS			19
#define ISCSI_ASK_FOR_MORE_TEXT			20
#define ISCSI_AWAIT_MORE_TEXT			21
#define ISCSI_BLOCKED_SENDING_TEXT		22

extern struct iscsi_global *devdata;

extern int	iscsi_detect		(Scsi_Target_Template*);
extern int	iscsi_release		(Scsi_Target_Device*);
extern int	iscsi_xmit_response	(Target_Scsi_Cmnd*);
extern int	iscsi_rdy_to_xfer	(Target_Scsi_Cmnd*);
extern int 	iscsi_proc_info		(char *, char **, off_t, int, int, int);
extern void 	iscsi_task_mgt_fn_done 	(Target_Scsi_Message *msg);

extern int	iscsi_server_thread (void*);
extern int	iscsi_rx_thread	(void*);
extern int	iscsi_tx_thread (void*);

    
#define PDU_SENSE_LENGTH_SIZE 2
#define SENSE_STRUCTURE_SIZE 18


/*
 * executed only by the rx thread, in spite of the tx in the name!
 * iscsi_tx_reject: this function transmits a reject to the Initiator
 * the attached 48-byte data segment is the header of the rejected PDU
 * OUTPUT: 0 if everything is okay, < 0 if there is trouble
 */
int
iscsi_tx_rjt(struct iscsi_conn *conn, uint8_t *bad_hdr, uint8_t reason);

/*
 * executed only by the rx thread.
 * Allocates new reject item, fills it in with header of rejected pdu,
 * and enqueues it for tx thread on reject_list of this connection.
 */
int
enqueue_reject(struct iscsi_conn *conn, uint8_t reason);

/*
 * iscsi_rx_data: will receive a fixed amount of data.
 * INPUT: struct iscsi_conn (what connection), struct iovec, number of iovecs,
 * total amount of data in bytes
 * OUTPUT: int total bytes read if everything is okay
 * 	       < 0 if trouble
 */
extern int iscsi_rx_data (struct iscsi_conn *conn, struct iovec *iov, int niov, int data);


/*
 * iscsi_tx_data: will transmit a fixed amount of data.
 * INPUT: struct iscsi_conn (what connection), struct iovec, number of iovecs,
 * total amount of data in bytes,
 * OUTPUT: int total bytes read if everything is okay
 * 	       < 0 if trouble
 */
extern int iscsi_tx_data (struct iscsi_conn *conn, struct iovec *iov, int niov, int data);
				
#endif
