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

#include "scsi_target.h"

#include "../common/iscsi_common.h"
#include "../common/debug.h"
#include "../common/range.h"
#include "../common/crc.h"
#include "../common/tcp_utilities.h"

/* chap and srp support - CHONG */
#include "../security/misc_func.h"
#include "../security/chap.h"
#include "../security/srp.h"

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

/* stores everything related to a SCSI command received */
struct iscsi_cmnd
{
    /* next: pointer to the next command in the list */
    struct iscsi_cmnd	*next;
    
    /* conn: connection on which this command was received */
    struct iscsi_conn	*conn;
    
    /* session: session on which this command was received */
    struct iscsi_session	*session;
    
    /* unsolicited_data_sem: to control receiving immediate data */
    sem_t    	unsolicited_data_sem;
    
    /* state: execution state of the command */
    uint8_t			state;
    
    /* response: task_mgt_function response */
    uint8_t			response;
    
    /* flag for errorRecovery of r2t */
    uint8_t 			recovery_r2t;
    
    /* flags from command pdu */
    uint8_t			command_flags;
    
    /* ping_data: data get from nopout and send for nopin */
    char			*ping_data;
    
    /* in_progress_buffer: accumulated data during text negotiations */
    char			*in_progress_buffer;
    
    /* cmnd: cmnd corresponding to this struct */
    Target_Scsi_Cmnd	*cmnd;
    
    /* message: message corresponding to this struct */
    Target_Scsi_Message 	*message;
    
    /* immediate_data_present: */
    uint8_t    		immediate_data_present;
    
    /* unsolicited_data_present: */
    uint8_t    		unsolicited_data_present;

    /* Data PDU re-transmission on Data SNACK - SAI */
    uint8_t                  retransmit_flg;
    
    /* opcode_byte: command's opcode byte (including I bit) */
    uint8_t                  opcode_byte;
    
    /* data_length: total number of bytes of data to be transferred by command */
    uint32_t			data_length;
    
    /* first_burst_size: data associated with the command */
    uint32_t			first_burst_len;
    
    /* max_burst_size: data associated with one R2T */
    uint32_t			next_burst_len;
    
    /* data_done: number of bytes of data transferred so far in a command */
    uint32_t			data_done;
    
    /* r2t_data_total: total amount of data to be solicited by r2ts */
    int			r2t_data_total;
    
    /* r2t_data: data_length_left for the next r2t -- can go negative! */
    int			r2t_data;
    
    /* scatter_list_offset:when processing a WRITE command, is byte offset in
     *			 current sg list item of where to start storing next
     *			 data byte from a DataOut pdu if pdus, seqs are in order
     *			 when processing a READ command, is byte offset in
     *			 current sg list iemt of where to start sending next
     *			 data byte in a DataOut pdu if pdus, seqs are in order
     */
    uint32_t           	scatter_list_offset;
    
    /* scatter_entry_count:when processing a WRITE command, is index into
     *			 current sg list of item of where to start storing next
     *			 data byte from a DataOut pdu if pdus, seqs are in order
     *			 when processing a READ command, is byte offset in
     *			 current sg list item of where to start sending next
     *			 data byte in a DataOut pdu if pdus, seqs are in order
     */
    uint32_t           	scatter_list_count;
    
    /* init_task_tag: Initiator Tag */
    uint32_t			init_task_tag;
    
    /* target_xfer_tag: Target Assigned Tag */
    uint32_t			target_xfer_tag;
    
    /* ref_task_tag: Referenced Task Tag used when aborting tasks*/
    uint32_t           	ref_task_tag;
    
    /* ref_cmd_sn: Referenced command sn used when aborting command*/
    uint32_t           	ref_cmd_sn;
    
    /* ref_function: Referenced function for task management */
    uint32_t			ref_function;
    
    /* ref_task_lun: Referenced Task lun used when aborting luns*/
    uint64_t           	ref_task_lun;

    /* lun and cdb: as their name mean */
    uint64_t   lun;
    uint8_t    cdb[16];	
    
    /* cmd_sn of this command and stat_sn of this command response */
    uint32_t			cmd_sn;
    uint32_t			stat_sn;
    uint32_t			cmd_sn_increment;   /* used by SCSI Command pdus only */
    
    /* Command WIDE COUNTERS: data_sn and r2t_sn */
    uint32_t			data_sn;
    uint32_t			r2t_sn;
    uint32_t			outstanding_r2t;

    /* for out-of-order sequence and DataOut PDUs */
    struct order_range      seq_range_list;
    struct order_range      pdu_range_list;
    
    /* for out-of-order command */
    uint8_t			 hdr[ISCSI_HDR_LEN];
    struct data_list	*unsolicited_data_head;
    struct data_list	*unsolicited_data_tail;
    
    /* Added r2t cookie details - SAI */
    struct iscsi_cookie	*first_r2t_cookie;
    struct iscsi_cookie	*last_r2t_cookie;
    
    /* queue out-of-order data pdus - SAI */
    struct iscsi_cookie	*first_data_q;
    struct iscsi_cookie	*last_data_q;
    
    /* Added timestamp for r2t retransmissions - SAI */
    time_t	timestamp;

    /* Store the previously sent data_sn, used when retransmit_flg set */
    uint32_t		prev_data_sn;
    
    /* Store the retransmission limits for data_sn set by a SNACK,
     * used when retransmit_flg set */
    uint32_t		startsn, endsn;
    
};


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

extern int iscsi_xmit_response(Target_Scsi_Cmnd * cmnd);
extern int iscsi_rdy_to_xfer(Target_Scsi_Cmnd * cmnd);
extern void iscsi_task_mgt_fn_done(Target_Scsi_Message * msg);
				
#endif
