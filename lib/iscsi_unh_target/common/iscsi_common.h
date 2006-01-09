/*	common/iscsi_common.h
 * 
 *	vi: set autoindent tabstop=8 shiftwidth=8 :
 *
 *	defines stuff common for both iscsi initiator and iscsi target
 *
 *	This file contains auxilliary functions for iscsi initiator 
 *	code that are responsible for dealing with error recovery.
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
 * 	written permission.
*/
#ifndef _ISCSI_COMMON_H
#define _ISCSI_COMMON_H

#include <stdint.h>
#include "te_iscsi.h"

#define ISCSI_WKP			3260	/* IANA's iSCSI WKP */
#define ISCSI_WKP_STRING		"3260"	/* IANA's iSCSI WKP as string */
#define ISCSI_SYSTEM_PORT		860	/* IANA's iSCSI system port */
#define ISCSI_SYSTEM_PORT_STRING	"860"	/* IANA's system port as strng*/
#define ISCSI_HDR_LEN			48	/* Basic Header Len */
#define ISCSI_CDB_LEN			16	/* CDB len within BHS */
#define CRC_LEN				4	/* Digest length */
#define MAX_KEY_NAME_LENGTH		63	/* max len of key name */
#define MAX_KEY_VALUE_LENGTH		255	/* max len of key val */
#define MAX_ISCSI_NAME_LENGTH		223	/* max len of iSCSI name */
#define ALL_ONES			0xFFFFFFFF	/* Reserved Value */
#define MASK_24_BITS			0xFFFFFF
#define MASK_22_BITS			0x3FFFFF
#define MASK_16_BITS			0xFFFF
#define MASK_8_BITS			0xFF
#define QUEUE_DEPTH_ALLOWED		0
#define SERIAL_BITS			31

#define WHITE_SPACE			" \t\v\f\n\r"
#define TARGETNAME_HEADER	"iqn.2004-01.com:"

/* max number of loops allowed during login negotiation */
#define LOOP_TIMES			8

/* values for "role" and "who_called" parameters */
#define INITIATOR			1
#define TARGET				2
#define MANAGEMENT			4

/** WHAT THE ... IS GOING ON HERE? ***/
/** (happily, these definitions seems to be actually unused */
/*
  #define TRUE				1
  #define NOT_TRUE			2
*/

/* iSCSI Drafts -- from now on we do only the latest draft (the standard) */
#define DRAFT20				2000
#define DEFAULT_DRAFT			DRAFT20
#define DRAFT_MULTIPLIER		100

/* iSCSI version */
#define ISCSI_MAX_VERSION		0	/* Draft 20 */
#define ISCSI_MIN_VERSION		0	/* Draft 20 */

/*************************************************************************/
/* Bits used in the "neg_info" variable and key field during Login Phase */

/* these 2 bits:
	are passed by manage in the neg_info parameter to proc_info()
	are used by proc_info()
	are never stored in param->neg_info
	are never used during initiator or target operation
		(i.e., they are only used between manage and proc_info())
*/
#define RESTORE 			0x00000001
#define FORCE				0x00000002

/* these 2 bits:
	are passed by manage in the neg_info parameter to proc_info()
	are used by proc_info()
	are stored by proc_info() directly in param->neg_info
	are tested in param->neg_info during login negotiations
	are never used in FFP
*/
#define KEY_TO_BE_NEGOTIATED	  	0x00000004
#define KEY_BREAK_CONN_OR_RJT		0x00000008

/* these 6 bits:
	are not used by manage
	are not used by proc_info()
	are set and tested in param->neg_info during login negotiations
	are never used in FFP
*/
#define KEY_SENT_TO_OTHER_SIDE		0x00000010
#define KEY_GOT_FROM_OTHER_SIDE		0x00000020
#define KEY_REPLY_OPTIONAL		0x00000040
#define KEY_IRRELEVANT			0x00000080
#define KEY_BAD				0x00000100
#define KEY_REJECT			0x00000200
#define KEY_WRONG			0x00000400
/********************************************************************/

/*	Mike Christie, mikenc@us.ibm.com */
#define ISCSI_SHUTDOWN_SIGNAL			SIGHUP
#define ISCSI_SHUTDOWN_SIGBITS			(sigmask(ISCSI_SHUTDOWN_SIGNAL))

/* values of connection-specific parameters used during FFP */
struct connection_operational_parameters {
	void *dummy;		/* at present, there are none we use */
};

/* values of session-wide parameters used during FFP */
struct session_operational_parameters {
	uint16_t MaxConnections;	/* [1..65535] */
	uint8_t InitialR2T;	/* [0,1] == [No,Yes] */
	uint8_t ImmediateData;	/* [0,1] == [No,Yes] */
	uint32_t MaxBurstLength;	/* [512..2**24-1] */
	uint32_t FirstBurstLength;	/* [512..2**24-1] */
	uint16_t DefaultTime2Wait;	/* [0..3600] */
	uint16_t DefaultTime2Retain;	/* [0..3600] */
	uint16_t MaxOutstandingR2T;	/* [1..65535] */
	uint8_t DataPDUInOrder;	/* [0,1] == [No,Yes] */
	uint8_t DataSequenceInOrder;	/* [0,1] == [No,Yes] */
	uint8_t ErrorRecoveryLevel;	/* [0..2] */
	uint8_t SessionType;	/* [0,1] == [Normal,Discovery] */
	uint8_t TargetName;	/* number at the end of the target name */
};

extern char *string_llx(uint64_t x, char *str); 

extern void print_payload(const void *buffer, int len);

extern void print_init_scsi_cmnd(struct iscsi_init_scsi_cmnd *cmd);

extern void print_targ_scsi_rsp(struct iscsi_targ_scsi_rsp *cmd);

extern void print_init_text_cmnd(struct iscsi_init_text_cmnd *cmd);

extern void print_targ_text_rsp(struct iscsi_targ_text_rsp *cmd);

extern void print_init_login_cmnd(struct iscsi_init_login_cmnd *cmd);

extern void print_targ_login_rsp(struct iscsi_targ_login_rsp *cmd);

extern void print_init_logout_cmnd(struct iscsi_init_logout_cmnd *cmd);

extern void print_targ_logout_rsp(struct iscsi_targ_logout_rsp *cmd);

extern void print_init_scsi_data_out(struct iscsi_init_scsi_data_out *cmd);

extern void print_targ_scsi_data_in(struct iscsi_targ_scsi_data_in *cmd);

extern void print_targ_rjt(struct iscsi_targ_rjt *cmd);

extern void print_init_nopout(struct iscsi_init_nopout *cmd);

extern void print_targ_nopin(struct iscsi_targ_nopin *cmd);

extern void print_targ_r2t(struct iscsi_targ_r2t *cmd);

extern void print_targ_async_msg(struct iscsi_targ_async_msg *cmd);

extern void print_init_task_mgt_command(struct iscsi_init_task_mgt_command *cmd);

extern void print_targ_task_mgt_response(struct iscsi_targ_task_mgt_response *cmd);

extern void print_init_snack(struct iscsi_init_snack *cmd);

extern void print_iscsi_command(void *cmd);

/*	returns pointer to printable name of SCSI op code */
/*	(Normally this is byte 1 of a SCSI CDB */
extern char *printable_scsi_op(uint8_t opcode, char *op_buf);

/*	returns pointer to printable name of iSCSI op code, but if this is
 *	a SCSI Command, return the printable name of the SCSI op from the cdb
 */
extern char *printable_iscsi_op(void *cmd, char *op_buf);

/*
 * Returns 1 if any luns for this target are in use.
 * Returns 0 otherwise.
 */
extern int target_in_use(uint32_t target_id);

struct scatterlist 
{
    uint32_t length;
    uint8_t *address;
};

#endif
