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
#define QUEUE_DEPTH_ALLOWED		100
#define SERIAL_BITS			31

#define WHITE_SPACE			" \t\v\f\n\r"
#define TARGETNAME_HEADER	"iqn.2004-01.com:"

/* max number of loops allowed during login negotiation */
#define LOOP_TIMES			8

/* values for "role" and "who_called" parameters */
#define INITIATOR			1
#define TARGET				2
#define MANAGEMENT			4

#define TRUE				1
#define NOT_TRUE			2

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

/* iSCSI opcodes */
/* Draft 20, Section 10.2.1.2 Opcode */

/* Initiator opcodes */
#define ISCSI_INIT_NOP_OUT		0x00
#define ISCSI_INIT_SCSI_CMND		0x01
#define ISCSI_INIT_TASK_MGMT_CMND	0x02
#define ISCSI_INIT_LOGIN_CMND		0x03
#define ISCSI_INIT_TEXT_CMND		0x04
#define ISCSI_INIT_SCSI_DATA_OUT	0x05
#define ISCSI_INIT_LOGOUT_CMND		0x06
#define ISCSI_INIT_SNACK		0x10

/* Target opcodes */
#define ISCSI_TARG_NOP_IN		0x20
#define ISCSI_TARG_SCSI_RSP		0x21
#define ISCSI_TARG_TASK_MGMT_RSP	0x22
#define ISCSI_TARG_LOGIN_RSP		0x23
#define ISCSI_TARG_TEXT_RSP		0x24
#define ISCSI_TARG_SCSI_DATA_IN		0x25
#define ISCSI_TARG_LOGOUT_RSP		0x26
#define ISCSI_TARG_R2T			0x31
#define ISCSI_TARG_ASYNC_MSG		0x32
#define ISCSI_TARG_RJT			0x3f

/* bits in connection_flags to control login and FFP */
#define USE_FLAT_SPACE_LUN		0x0001
#define USE_FULL_REPLIES		0x0002
#define USE_SECURITY_PHASE		0x0004
#define USE_OPERATIONAL_PHASE		0x0008
#define USE_HEADERDIGEST		0x0010
#define USE_DATADIGEST			0x0020
#define GOT_ACTIVITY			0x0040
#define SEND_NO_REPLY_TO_NOP_PING	0x0080	/* test 6 */
#define SEND_NO_REPLY_TO_ASYNC_LOGOUT	0x0100	/* test 7 */
#define USE_REFLECT_XKEYS		0x0200
#define CONN_LOGGED_OUT			0x0400
#define NEED_TX_WAKEUP			0x0800
#define TX_TIMER_OFF			0x1000
#define USE_ONE_KEY_PER_TEXT		0x2000
#define CONN_HIT_EOF			0x4000

/* chap support - CHONG */
#define USE_TARGET_CONFIRMATION		0x01
#define USE_BASE64			0x02

/* bits and fields in the opcode byte (always byte 0 in a pdu) */
#define ISCSI_OPCODE			0x3f
#define REPLY_BIT			0x20
#define I_BIT                          	0x40
#define OLD_X_BIT			0x80	/* used in draft 8 and before */

/* bits and fields in the flags byte (always byte 1 in a pdu) */
#define F_BIT				0x80
#define R_BIT				0x40
#define A_BIT				0x40
#define C_BIT				0x40
#define READ_TYPE_CMND			R_BIT
#define W_BIT				0x20
#define WRITE_TYPE_CMND			W_BIT
#define BRO_BIT				0x10
#define BRU_BIT				0x08
#define O_BIT				0x04
#define U_BIT				0x02
#define S_BIT                          	0x01

#define T_BIT				0x80
#define CSG				0x0C
#define CSG1				0x04
#define CSG2				0x08
#define CSG3				0x0C
#define CSG_SHIFT			2
#define NSG				0x03
#define NSG1				0x01
#define NSG2				0x02
#define NSG3				0x03

/*	Draft 20, Section 10.3.1 Flags and Task Attibutes (byte 1) */
/* SCSI Command ATTR value */
#define ISCSI_UNTAGGED			0x00
#define ISCSI_SIMPLE   	       		0x01
#define ISCSI_ORDERED          		0x02
#define ISCSI_HEAD_OF_QUEUE    		0x03
#define ISCSI_ACA              		0x04


#ifndef __SCSI_TARGET__

/* RFC 3720 Section 10.5.1 Function (field in TMF Request)
 *
 * N.B. These definitions are duplicated in target/scsi_target.h
 * because there are needed in SCSI but that target file cannot
 * include this file (iscsi_common.h)
 */
#define TMF_ABORT_TASK			1
#define TMF_ABORT_TASK_SET		2
#define TMF_CLEAR_ACA			3
#define TMF_CLEAR_TASK_SET		4
#define TMF_LUN_RESET			5
#define TMF_TARGET_WARM_RESET		6
#define TMF_TARGET_COLD_RESET		7
#define TMF_TASK_REASSIGN		8

#endif

/*	Mike Christie, mikenc@us.ibm.com */
#define ISCSI_SHUTDOWN_SIGNAL			SIGHUP
#define ISCSI_SHUTDOWN_SIGBITS			(sigmask(ISCSI_SHUTDOWN_SIGNAL))

/*	Draft 20, Section 10.13.5 Status-Class and Status-Detail */
/*	Status-Class field in Login Response */
#define STAT_CLASS_SUCCESS			0x00
#define STAT_CLASS_REDIRECTION			0x01
#define STAT_CLASS_INITIATOR			0x02
#define STAT_CLASS_TARGET			0x03

/*	Status-Detail field in Login Response */
#define STAT_DETAIL_TARG_MOVED_TEMP 		0x01
#define STAT_DETAIL_TARG_MOVED_PERM 		0x02

#define STAT_DETAIL_ERR				0x00
#define STAT_DETAIL_NOT_AUTH			0x01
#define STAT_DETAIL_NOT_ALLOWED			0x02
#define STAT_DETAIL_NOT_FOUND			0x03
#define STAT_DETAIL_TARG_REMOVED		0x04
#define STAT_DETAIL_VERSION_NOT_SUPPORTED 	0x05
#define STAT_DETAIL_TOO_MANY_CONNECTIONS 	0x06
#define STAT_DETAIL_MISSING_PARAMETER 		0x07
#define STAT_DETAIL_NOT_INCLUDED 		0x08
#define STAT_DETAIL_SESSION_TYPE 		0x09
#define STAT_DETAIL_SESSION_NOT_EXIST 		0x0a
#define STAT_DETAIL_INVALID_DURING_LOGIN	0x0b

#define STAT_DETAIL_SERVICE_UNAVAILABLE 	0x01
#define STAT_DETAIL_OUT_OF_RESOURCE 		0x02


/*	Draft 20, Section 10.6.1 Response (field in TMF Response) */
#define FUNCTION_COMPLETE           		0
#define TASK_DOES_NOT_EXIST			1
#define LUN_DOES_NOT_EXIST			2
#define TASK_STILL_ALLEGIANT			3
#define TASK_FAILOVER_NOT_SUPPORTED		4
#define TASK_MANAGEMENT_FUNCTION_NOT_SUPPORTED	5
#define FUNCTION_AUTHORIZATION_FAILED		6
#define FUNCTION_REJECTED           		255


/*	RFC 3720 Section 10.17.1 Reason (field in Reject Response) */
#define REASON_DATA_DIGEST_ERR			0x02
#define REASON_DATA_SNACK			0x03
#define REASON_PROTOCOL_ERR			0x04
#define REASON_COMMAND_NOT_SUPPORTED		0x05
#define REASON_TOO_MANY_IMMEDIATE_COMMANDS	0x06
#define REASON_TASK_IN_PROGRESS			0x07
#define REASON_INVALID_SNACK			0x08
#define REASON_INVALID_PDU_FIELD		0x09
#define REASON_OUT_OF_RESOURCES			0x0a
#define REASON_NEGOTIATION_RESET		0x0b
#define REASON_WAITING_FOR_LOGOUT		0x0c


/*	Draft 20, Section 10.14.1 Reason Code (field in Logout Request) */
#define LOGOUT_REASON				0x7f
#define LOGOUT_CLOSE_SESSION			0x00
#define LOGOUT_CLOSE_CONNECTION			0x01
#define LOGOUT_REMOVE_CONN_FOR_RECOVERY		0x02


/*	Draft 20, Section 10.16.1 Type (field in SNACK Request) */
#define SNACK_TYPE              0x0f
#define DATA_R2T_SNACK          0x00
#define STATUS_SNACK            0x01
#define DATACK_SNACK            0x02
#define R_DATA_SNACK            0x03
                                                                                

/* connection recovery related macros - SAI */
/* ErrorRecoveryLevel values */
#define SESSION_RECOVERY			0
#define DIGEST_RECOVERY				1
#define CONNECTION_RECOVERY			2

#define CONNREC_NOT_SUPPORTED			0x02
#define TIME2WAIT				0x0001
#define TIME2RETAIN				0x0005

/* enable bits in the session's snack_flag */
#define DATA_SNACK_ENABLE       0x01
#define STATUS_SNACK_ENABLE     0x02
#define DATACK_SNACK_ENABLE     0x04


/* For Text Parameter Negotiation */
/* Draft 20, Section 5.3 Login Phase
   "The default MaxRecvDataSegmentLength is used during Login."

   Draft 20, Section 12.12 MaxRecvDataSegmentLength
   "Default is 8192 bytes."
*/
#define MAX_TEXT_LEN 				8192
#define TEXT_FUDGE_LEN				512

/* SNACK for Error Recovery - SAI */
#define SNACK					0x00000030

extern struct iscsi_thread_param *iscsi_param;

/* iSCSI PDU formats */
struct iscsi_init_scsi_cmnd {
        uint8_t opcode;
	uint8_t flags;
	uint16_t rsvd1;
	uint32_t length;
	uint64_t lun;
	uint32_t init_task_tag;
	uint32_t xfer_len;
	uint32_t cmd_sn;
	uint32_t exp_stat_sn;
	uint8_t cdb[16];
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_targ_scsi_rsp {	/* Draft 20, Section 10.4 SCSI Response */
	uint8_t opcode;
	uint8_t flags;
	uint8_t response;
	uint8_t status;
	uint32_t length;
	uint64_t lun;		/* reserved */
	uint32_t init_task_tag;
	uint32_t target_xfer_tag;
	uint32_t stat_sn;
	uint32_t exp_cmd_sn;
	uint32_t max_cmd_sn;
	uint32_t exp_data_sn;
	uint32_t bidi_resid;
	uint32_t resid;
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_init_task_mgt_command {	/* Draft 20, Section 10.5 Task Management
					   Function Request */
	uint8_t opcode;
	uint8_t function;
	uint16_t rsvd1;
	uint32_t length;		/* reserved */
	uint64_t lun;
	uint32_t init_task_tag;
	uint32_t ref_task_tag;
	uint32_t cmd_sn;
	uint32_t exp_stat_sn;
	uint32_t ref_cmd_sn;
	uint32_t exp_data_sn;
	uint64_t rsvd4;
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_targ_task_mgt_response {	/* Draft 20, Section 10.6 Task Management
					   Function Response */
	uint8_t opcode;
	uint8_t flags;
	uint8_t response;
	uint8_t rsvd1;
	uint32_t length;		/* reserved */
	uint64_t lun;		/* reserved */
	uint32_t init_task_tag;
	uint32_t rsvd2;
	uint32_t stat_sn;
	uint32_t exp_cmd_sn;
	uint32_t max_cmd_sn;
	uint32_t rsvd4;
	uint64_t rsvd5;
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_init_scsi_data_out {	/* Draft 20, Section 10.7 SCSI Data-out */
	uint8_t opcode;
	uint8_t flags;
	uint16_t rsvd2;
	uint32_t length;
	uint64_t lun;
	uint32_t init_task_tag;
	uint32_t target_xfer_tag;
	uint32_t rsvd3;
	uint32_t exp_stat_sn;
	uint32_t rsvd4;
	uint32_t data_sn;
	uint32_t offset;
	uint32_t rsvd5;
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_targ_scsi_data_in {	/* Draft 20, Section 10.7 SCSI Data-in */
	uint8_t opcode;
	uint8_t flags;
	uint8_t rsvd1;
	uint8_t status;
	uint32_t length;
	uint64_t lun;
	uint32_t init_task_tag;
	uint32_t target_xfer_tag;
	uint32_t stat_sn;
	uint32_t exp_cmd_sn;
	uint32_t max_cmd_sn;
	uint32_t data_sn;
	uint32_t offset;
	uint32_t resid;
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_targ_r2t {		/* Draft 20, Section 10.8 Ready To Transfer (R2T) */
	uint8_t opcode;
	uint8_t flags;
	uint16_t rsvd2;
	uint32_t length;		/* reserved */
	uint64_t lun;
	uint32_t init_task_tag;
	uint32_t target_xfer_tag;
	uint32_t stat_sn;
	uint32_t exp_cmd_sn;
	uint32_t max_cmd_sn;
	uint32_t r2t_sn;
	uint32_t offset;
	uint32_t xfer_len;
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_targ_async_msg {	/* Draft 20, Section 10.9 Asynchronous Message */
	uint8_t opcode;
	uint8_t flags;
	uint16_t rsvd2;
	uint32_t length;
	uint64_t lun;
	uint32_t init_task_tag;	/* reserved 0xffffffff */
	uint32_t rsvd3;
	uint32_t stat_sn;
	uint32_t exp_cmd_sn;
	uint32_t max_cmd_sn;
	uint8_t async_event;
	uint8_t async_vcode;
	uint16_t parameter1;
	uint16_t parameter2;
	uint16_t parameter3;
	uint32_t rsvd5;
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_init_text_cmnd {	/* Draft 20, Section 10.10 Text Request */
	uint8_t opcode;
	uint8_t flags;
	uint16_t rsvd2;
	uint32_t length;
	uint64_t lun;
	uint32_t init_task_tag;
	uint32_t target_xfer_tag;
	uint32_t cmd_sn;
	uint32_t exp_stat_sn;
	uint64_t rsvd4;
	uint64_t rsvd5;
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_targ_text_rsp {	/* Draft 20, Section 10.11 Text Response */
	uint8_t opcode;
	uint8_t flags;
	uint16_t rsvd2;
	uint32_t length;
	uint64_t lun;
	uint32_t init_task_tag;
	uint32_t target_xfer_tag;
	uint32_t stat_sn;
	uint32_t exp_cmd_sn;
	uint32_t max_cmd_sn;
	uint32_t rsvd4;
	uint64_t rsvd5;
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_init_login_cmnd {	/* Draft 20, Section 10.12 Login Request */
	uint8_t opcode;
	uint8_t flags;
	uint8_t version_max;
	uint8_t version_min;
	uint32_t length;
	uint8_t isid[6];
	uint16_t tsih;
	uint32_t init_task_tag;
	uint16_t cid;
	uint16_t rsvd1;
	uint32_t cmd_sn;
	uint32_t exp_stat_sn;
	uint64_t rsvd2;
	uint64_t rsvd3;
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_targ_login_rsp {	/* Draft 20, Section 10.13 Login Response */
	uint8_t opcode;
	uint8_t flags;
	uint8_t version_max;
	uint8_t version_active;
	uint32_t length;
	uint8_t isid[6];
	uint16_t tsih;
	uint32_t init_task_tag;
	uint32_t rsvd1;
	uint32_t stat_sn;
	uint32_t exp_cmd_sn;
	uint32_t max_cmd_sn;
	uint8_t status_class;
	uint8_t status_detail;
	uint16_t rsvd2;
	uint64_t rsvd3;
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_init_logout_cmnd {	/* Draft 20, Section 10.14 Logout Request */
	uint8_t opcode;
	uint8_t flags;		/* reasoncode */
	uint16_t rsvd1;
	uint32_t length;
	uint64_t lun;		/* reserved */
	uint32_t init_task_tag;
	uint16_t cid;
	uint16_t rsvd2;
	uint32_t cmd_sn;
	uint32_t exp_stat_sn;
	uint64_t rsvd4;
	uint64_t rsvd5;
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_targ_logout_rsp {	/* Draft 20, Section 10.15 Logout Response */
	uint8_t opcode;
	uint8_t flags;
	uint8_t response;
	uint8_t rsvd1;
	uint32_t length;		/* reserved */
	uint64_t lun;		/* reserved */
	uint32_t init_task_tag;
	uint32_t rsvd3;
	uint32_t stat_sn;
	uint32_t exp_cmd_sn;
	uint32_t max_cmd_sn;
	uint32_t rsvd4;
	uint16_t time2wait;
	uint16_t time2retain;
	uint32_t rsvd5;
	uint32_t header_digest;	/* holds header CRC if in use */
};

/* SNACK pdu structure definition - SAI */

struct iscsi_init_snack {	/* Draft 20, Section 10.16 SNACK Request */
	uint8_t opcode;
	uint8_t flags;
	uint16_t rsvd1;
	uint32_t length;
	uint64_t lun;
	uint32_t init_task_tag;
	uint32_t target_xfer_tag;
	uint32_t rsvd2;
	uint32_t exp_stat_sn;
	uint64_t rsvd3;
	uint32_t begrun;
	uint32_t runlen;
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_targ_rjt {		/* Draft 20, Section 10.17 Reject */
	uint8_t opcode;
	uint8_t flags;
	uint8_t reason;
	uint8_t rsvd2;
	uint32_t length;
	uint64_t lun;		/* reserved */
	uint32_t init_task_tag;	/* reserved 0xffffffff */
	uint32_t rsvd4;
	uint32_t stat_sn;
	uint32_t exp_cmd_sn;
	uint32_t max_cmd_sn;
	uint32_t data_sn;
	uint64_t rsvd5;
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_init_nopout {	/* Draft 20, Section 10.18 NOP-Out */
	uint8_t opcode;
	uint8_t flags;
	uint16_t rsvd1;
	uint32_t length;
	uint64_t lun;
	uint32_t init_task_tag;
	uint32_t target_xfer_tag;
	uint32_t cmd_sn;
	uint32_t exp_stat_sn;
	uint64_t rsvd2;
	uint64_t rsvd3;
	uint32_t header_digest;	/* holds header CRC if in use */
};

struct iscsi_targ_nopin {	/* Draft 20, Section 10.19 NOP-In */
	uint8_t opcode;
	uint8_t flags;
	uint16_t rsvd1;
	uint32_t length;
	uint64_t lun;
	uint32_t init_task_tag;
	uint32_t target_xfer_tag;
	uint32_t stat_sn;
	uint32_t exp_cmd_sn;
	uint32_t max_cmd_sn;
	uint32_t rsvd2;
	uint64_t rsvd3;
	uint32_t header_digest;	/* holds header CRC if in use */
};

/* Used in Login negotiations */
struct generic_pdu {
	uint8_t opcode;		/*  0 */
	uint8_t flags;		/*  1 */
	uint8_t version_max;	/*  2 */
	uint8_t version_active;	/*  3 for login response PDU */
	uint32_t length;		/*  4 */
	uint8_t isid[6];		/*  8 */
	uint16_t tsih;		/* 14 */
	uint32_t init_task_tag;	/* 16 */
	uint16_t CID;		/* 20 */
	uint16_t rsvd1;		/* 22 */
	uint32_t cmd_sn;		/* 24 */
	uint32_t exp_stat_sn;	/* 28 */
	uint32_t max_cmd_sn;	/* 32 */
	uint8_t status_class;	/* 36 */
	uint8_t status_detail;	/* 37 */
	uint16_t rsvd2;		/* 38 */
	uint32_t offset;		/* 40 */
	uint32_t resid;		/* 44 */
	uint32_t header_digest;	/* 48 holds header CRC if in use */
	uint8_t *text;
	uint32_t text_length;
};

/* Used in generic processing of responses */
struct response_pdu {
	uint8_t opcode;
	uint8_t flags;
	uint8_t reason;		/* for reject PDU */
	uint8_t version_active;	/* for login response PDU */
	uint32_t length;
	uint64_t lun;
	uint32_t init_task_tag;
	uint32_t target_xfer_tag;
	uint32_t stat_sn;
	uint32_t exp_cmd_sn;
	uint32_t max_cmd_sn;
	uint8_t status_class;
	uint8_t status_detail;
	uint16_t rsvd2;
	uint32_t offset;
	uint32_t xfer_len;
	uint32_t header_digest;	/* holds header CRC if in use */
};

/* The format of the data returned by the SCSI REQUEST SENSE command.
 *	SCSI Primary Commands - 2 (SPC-2)
 *	ANSI Project T10/1236-D
 *	Revision 20 18 July 2001
 *	page 135
 *	7.20.1 REQUEST SENSE command introduction
 *		"Device servers shall be capable of returning 18 bytes of data
 *		in response to a REQUEST SENSE command."
 *	page 136
 *	7.20.2 Sense data format
 *		Table 102 -- Response codes 70h and 71h sense data format
 *
 *	This structure is the minimum a device can return.
 *	The file linux/drivers/scsi/scsi.h defines the maximum acceptable as:
 *		#define SCSI_SENSE_BUFFERSIZE	64
 *		unsigned char sr_sense_buffer[SCSI_SENSE_BUFFERSIZE];
 *		unsigned char sense_buffer[SCSI_SENSE_BUFFERSIZE];
 */

struct scsi_sense_data {
	uint8_t response;		/* sense_buffer[0] -- value f0h or f1h only */
	uint8_t obsolete;		/* sense_buffer[1] */
	uint8_t sense_key;		/* sense_buffer[2] -- plus FILEMARK, EOM, ILI */
	uint8_t info[4];		/* sense_buffer[3-6] */
	uint8_t asl;		/* sense_buffer[7] -- value 17-7 = 10 */
	uint8_t cmnd_spec_info[4];	/* sense_buffer[8-11] */
	uint8_t asc;		/* sense_buffer[12] */
	uint8_t ascq;		/* sense_buffer[13] */
	uint8_t f_r_unit_code;	/* sense_buffer[14] */
	uint8_t sense_specific[3];	/* sense_buffer[15-17] */
};

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

#endif
