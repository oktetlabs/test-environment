/*	common/target_negotiate.h
 * 
 * 	This defines the functions used in the Login phase
 * 	by the iSCSI target for parameter negotiation
 *
 *	vi: set autoindent tabstop=8 shiftwidth=4 :
 *
 *	This file contains auxilliary functions for iscsi initiator 
 *	code that are responsible for dealing with error recovery.
 *
 *	Copyright (C) 2001-2003 InterOperability Lab (IOL)
 *	University of New Hampshier (UNH)
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
 * 	written permission.
*/
#ifndef	_TARGET_NEGOTIATE_H
#define	_TARGET_NEGOTIATE_H

#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

#include "iscsi_common.h"
#include "list.h"
#include "text_param.h"
#include <iscsi_custom.h>

/* names of bit numbers in the iscsi_conn control byte */
#define SILENCE_BIT		0
#define NEED_NOPIN_BIT		1

/* maximum number of unreplied-to NopIns to send to initiator before aborting */
#define MAX_OUTSTANDING_NOPINS	4

/* maximum number of different IP_address:port listening sockets and threads
 * (used as size of iscsi_portal_groups[])
 */
#define MAX_PORTAL	32

/* queue item to send a reject pdu back to initiator */
struct reject_item {
	struct list_head	reject_link;
	uint8_t			bad_header[ISCSI_HDR_LEN];
	uint8_t			reason;
};

/* global struct to store properties corresponding to an iscsi target */
struct iscsi_global
{
  /* conn_id: id assigned to connections within session */
  uint32_t			conn_id;

  /* ntsih: id assigned to target sessions */
  uint16_t			ntsih;

  /* Data and Status snack support - SAI */
  uint8_t                  targ_snack_flg;

  /* phase-collapse setting - cdeng */
  signed char		phase_collapse;
  
  /* session_list: list of sessions */
  struct list_head	session_list;

  /* bad_session_list: list of sessions that failed during startup */
  struct list_head	bad_session_list;

  /* session_sem: semaphore to add and remove sessions from both session lists
   *	this semaphore also controls access to session->conn_list
   */
  pthread_mutex_t session_mutex;

  /* session_read_mutex: semaphore to read (no changes) both session lists
   *	as well as the session->conn_list
   */
  pthread_mutex_t session_read_mutex;

  /* session_readers: counts number of current readers of the session lists */
  uint32_t			session_readers;

  /* server_thr: task struct for the server thread */
  struct task_struct	*server_thr[MAX_PORTAL];

  /* server_socket: socket on which the server listens */
  struct socket		*server_socket[MAX_PORTAL];

  /* server_sem: semaphore for killing the server thread */
  sem_t	server_sem;

  /* device: device returned by the STML */
  struct Scsi_Target_Device *device;

  /* force: manageable bits to initialize connection flags */
  uint32_t			force;

  /* nop_period: timeout period to send periodic NopIns to initiator */
  uint32_t			nop_period;
        
  /* Added r2t retransmit period - SAI */
  uint32_t			r2t_period;

  /* text parameters accepted by the target */
  struct parameter_type   (*param_tbl)[MAX_CONFIG_PARAMS];

  /* chap parameter - CHONG */
  struct auth_parameter_type auth_parameter;
};

/* stores everything related to an iscsi connection on the target */
struct iscsi_conn {
	/* conn_link: structure for threading this conn onto lists */
	struct list_head conn_link;

	/* conn_id: the id for this connection */
	int conn_id;

	/* cid: connection id assigned by the Initiator */
        uint16_t cid;

	/* portal_group_tag: target_portal_group_tag for the connection */
	uint16_t portal_group_tag;

	/* conn_socket: socket used by this connection */
	int conn_socket;

	/* session: what session does this connection belong to */
	struct iscsi_session *session;

	/* dev: device on which this connection was received */
	struct iscsi_global *dev;

	/* rx_thread: task struct for rx_thread */
	struct task_struct *rx_thread;

	/* tx_thread: task struct for tx_thread */
	struct task_struct *tx_thread;

	/* tx_sem: semaphore to control operation of tx_thread */
	sem_t tx_sem;

	/* semaphores to kill rx and tx threads */
	sem_t kill_rx_sem;
	sem_t kill_tx_sem;

	/* reject_list: list of rejects to be sent back to initiator */
	struct list_head	reject_list;

	/* reject_sem: semaphore to add/remove reject items */
	sem_t reject_sem;

	/* bad_hdr: complete header of bad PDU for reject */
	uint8_t bad_hdr[ISCSI_HDR_LEN];

	/* hdr_crc: 1 if header digest in use, else 0 */
	int hdr_crc;

	/* data_crc: 1 if data digest in use, else 0 */
	int data_crc;

	/* active: 1 if connection is in use, else 0 */
	uint8_t active;

	/* control: bits to control tx thread wakeups, sending NopIn pings */
	unsigned long control;

	/* connection_flags: bits to control stuff during login and FFP */
	uint32_t connection_flags;

	/* text_in_progress: ptr to text command in-progress, else NULL */
	void *text_in_progress;

	/* text_in_progress_sem: semaphore to add/remove text_in_progress */
	pthread_mutex_t text_in_progress_mutex;

	/* 
	 * Connection WIDE COUNTERS: stat_sn
	 */
	uint32_t stat_sn;
	uint32_t max_send_length;	/*initiator's MaxRecvPDULength */
	uint32_t max_recv_length;	/*target's MaxRecvPDULength */

    iscsi_custom_data *custom;
};

/* stores everything related to an iscsi session on the target */
struct iscsi_session {
	/* sess_link: structure for threading this session onto lists */
	struct list_head sess_link;

	/* isid: id assigned to a session by the Initiator */
	uint8_t isid[6];

	/* SNACK flags for Error Recovery - SAI */
	uint8_t targ_snack_flg;

	/* tsih: id assigned to a session by the Target */
	uint16_t tsih;

	/* portal_group_tag: target_portal_group_tag for the session */
	uint16_t portal_group_tag;

	/* cmnd_id: these are the values assigned as target_tags */
	uint32_t cmnd_id;

	/* cmnd_list: list of commands received within session */
	struct iscsi_cmnd *cmnd_list;

	/* conn_sem: semaphore to add/remove commands and reject items */
	pthread_mutex_t cmnd_mutex;
    pthread_mutexattr_t cmnd_mutex_recursive;

	/* nconn: no of active connections */
	int nconn;

	/* conn_list: list of connections within session
	 *	access to this list is under protection of the session_sem
	 *	it can be read only when session_list is read protected
	 *	it can be written only when session_list is write protected
	 */
	struct list_head conn_list;

	/* devdata: pointer to the device specific data */
	struct iscsi_global *devdata;

	/* Added the r2t retransmit timer - SAI */
	uint32_t r2t_period;

	/* error recovery ver ref18_04 */
	pthread_t retran_thread;
    te_bool   has_retran_thread;
	sem_t thr_kill_sem;

	/* For parameters */
	struct parameter_type (*session_params)[MAX_CONFIG_PARAMS];
	struct session_operational_parameters *oper_param;

	/* version identifiers */
	uint8_t version_min;
	uint8_t version_max;
	uint8_t version_active;

	/*
	 * SESSION WIDE COUNTERS: exp_cmd_sn and max_cmd_sn
	 */
	uint32_t cmd_sn;
	uint32_t exp_cmd_sn;
	uint32_t max_cmd_sn;

	/* command ordering variables - SAI */
	sem_t cmd_order_sem;
	struct order_cmd *cmd_order_head;
	struct order_cmd *cmd_order_tail;
};


void __attribute__ ((no_instrument_function))
print_isid_tsih_message(struct iscsi_session *session, char *message);


int parameter_negotiate(struct iscsi_conn *conn,
			struct parameter_type p_param_tbl[MAX_CONFIG_PARAMS],
			struct iscsi_init_login_cmnd *loginpdu,
			uint32_t when_called,
			struct auth_parameter_type auth_param);


/* set back the leading only keys if these keys were set to be
 * " KEY_TO_BE_NEGOTIATED" in the leading connection negotation.*/
void reset_parameter_table(struct parameter_type
			   p_param_tbl[MAX_CONFIG_PARAMS]);


/*
 * iscsi_release_session: This function is responsible for closing out
 * a session and removing it from whatever list it is on.
 * INPUT: session to be released
 * OUTPUT: 0 if success, < 0 if there is trouble
 */
int
iscsi_release_session(struct iscsi_session *session);

#endif
