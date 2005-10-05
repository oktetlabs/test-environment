/*	target/target_error_rec.h 
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

#ifndef	_TARGET_ERROR_REC_H
#define	_TARGET_ERROR_REC_H

#include <inttypes.h>

#define DATA_SNACK_RETRANSMIT	0x80
#define DATA_SNACK_REJECT		0x40

struct targ_error_rec
{
	struct iscsi_conn	*curr_conn;
	struct generic_pdu	*pdu_hdr;
	struct iscsi_cmnd	*cmd;
	uint8_t			err_type;
};

/* This data structure is used for maintaining R2T cookies and also
   for queueing out-of-order dataouts received */
struct iscsi_cookie
{
	uint32_t				seq;
	int						xfer_len;
	int						offset;
	int						list_offset;
	int						list_count;
	struct iscsi_cookie 	*next;
};


/************************************************************************
*  The Main Error Recovery Handler Called from iscsi_initiator when it  *
*  finds any error.                                                     *
************************************************************************/

int targ_do_error_recovery(struct targ_error_rec *err_rec);


/************************************************************************
* This function handles the session recovery. It creates a recovery     *
* thread that will do session recovery.                                 *
************************************************************************/

void targ_session_recovery(struct iscsi_conn *current_connection);


/************************************************************************
* This thread handles the session recovery. Session Recovery will       *
* abort all pending tasks and closes all transport connection           *
* Section (6.12.4)                                                      *
************************************************************************/

int targ_recovery_thread(void *arg);


/************************************************************************
* This function handles the digest errors and sequence errors that may  *
* have caused because of previous digest errors.                        *
************************************************************************/

int targ_digest_recovery(struct targ_error_rec *err_rec);


/*******************************************************************
 * Reads the data portion of the dropped PDU due to digest errors. *
 * The data read is also ignored.                                  *
********************************************************************/

int targ_drop_pdu_data(struct iscsi_conn *curr_conn, int size);


/*********************************************************************
 * (re)start R2T Timer is used for tracking the R2T sent. This timer
 * helps re-transmit the most recent R2T sent to the initiator.
 ********************************************************************/
/* (re)start the R2T retransmit timer */
void restart_r2t_timer(struct iscsi_session *session);


/*********************************************************************
* Called by kernel when R2T periodic session timer expires.
*********************************************************************/
void deal_with_r2t_timer(unsigned long data);


/***************************************************************************
* create an R2T cookie and store the R2T details in the cookie. This cookie
* is esssential for R2T re-transmssions in error conditions.
***************************************************************************/
struct iscsi_cookie* create_r2t_cookie(struct iscsi_cmnd *cmnd);


/* free the long pending R2T cookie list after the command is completed - SAI */
void free_r2t_cookie(struct iscsi_cmnd *cmnd);


/*******************************************************************************
* This function uses the offset and data length parameter or cookie information
* to build a recovery R2T to be sent to the intiator for missing Dataouts. This
* function is called by SNACK mechanism and also by other places of the code that
* finds a discrepency in Dataouts using data sequence numbering or offsets.
********************************************************************************/
int send_recovery_r2t (struct iscsi_cmnd *cmnd, int data_offset,
						struct iscsi_cookie *cookie, struct generic_pdu *hdr);


/***********************************************************************
* This thread remains blocked at a semaphore for most part of it's time
* When a r2t timeout happens thsi thread is awakened for re-transmitting
* the r2t request.
***********************************************************************/

int iscsi_retran_thread (void *param);

void add_data_to_queue(struct iscsi_cmnd *cmd, struct iscsi_cookie *dataq);


/********************************************************************
* This function drives the out-of-order dataouts received into the 
* appropriate scatter gather buffer location maintained by scsi. The
* information about the out-of-order dataout and also the corresponding
* scatter offsets are queued for future use.
**********************************************************************/
int queue_data(struct targ_error_rec *err_rec);


/***********************************************************************
* This function is called everytime a in-sequence dataout pdu is
* received. The function verifies if there was any previous
* out-of-order dataouts for the command and updates the state variables
* accordingly.
***********************************************************************/
void search_data_q(struct iscsi_cmnd *cmd);

#endif
