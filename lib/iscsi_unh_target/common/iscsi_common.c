/*	common/iscsi_common.c
 *
 * 	Contains functions common for both iscsi initiator and iscsi target
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

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <netinet/in.h>

#include <scsi/scsi.h>

#include "iscsi_common.h"
#include "debug.h"


/*	routine to convert a 64-bit value in network byte order
	into a printable string
*/
char *
string_llx(uint64_t x, char *str)
{
	int i;
	static char convert[] = "0123456789abcdef";
	char *ptr;
	uint32_t c;
	union {
		uint64_t x;
		uint8_t c[8];
	} tmp;

	tmp.x = x;
	ptr = str;
	*ptr++ = '0';
	*ptr++ = 'x';
	for (i = 0; i < 8; i++) {
		c = tmp.c[i];
		*ptr++ = convert[c >> 4];
		*ptr++ = convert[c & 0xf];
	}
	*ptr = '\0';
	return str;
}

void
print_payload(const void *buffer, int len)
{
    char format[80] = "";
    char *fptr;
    int i;

    TRACE(TRACE_DEBUG, "iSCSI Payload of length %d: ", len);
    for (i = 0, fptr = format; i < len; i++)
    {
        sprintf(fptr, "%2.2x ", ((const uint8_t *)buffer)[i]);
        fptr += 3;
        if (fptr >= format + sizeof(format))
        {
            RING("%s", format);
            fptr = format;
            memset(format, 0, sizeof(format));
        }
    }
    if (fptr != format)
        TRACE(TRACE_DEBUG, "%s", format);
}

static void
print_rsvd_u8(int n, uint8_t rsvd)
{
	if (rsvd != 0)
		TRACE(TRACE_DEBUG, "    rsvd%d: 0x%.2x", n, rsvd);
}

static void
print_rsvd_u16(int n, uint16_t rsvd)
{
	if (rsvd != 0)
		TRACE(TRACE_DEBUG, "    rsvd%d: 0x%.4x", n, rsvd);
}

static void
print_rsvd_u32(int n, uint32_t rsvd)
{
	if (rsvd != 0)
		TRACE(TRACE_DEBUG, "    rsvd%d: 0x%.8x", n, rsvd);
}

static void
print_rsvd_u64(int n, uint64_t rsvd)
{
	char string[8];

	if (rsvd != 0ll)
		TRACE(TRACE_DEBUG, "    rsvd%d: %s", n, string_llx(rsvd, string));
}

static const char *
map_opcode_to_name(uint8_t opcode)
{
    switch(opcode)
    {
        case ISCSI_INIT_NOP_OUT: return "INIT_NOP_OUT";
        case ISCSI_INIT_SCSI_CMND: return "INIT_SCSI_CMND";
        case ISCSI_INIT_TASK_MGMT_CMND: return "INIT_TASK_MGMT_CMND";
        case ISCSI_INIT_LOGIN_CMND: return "INIT_LOGIN_CMND";
        case ISCSI_INIT_TEXT_CMND: return "INIT_TEXT_CMND";
        case ISCSI_INIT_SCSI_DATA_OUT: return "INIT_SCSI_DATA_OUT";
        case ISCSI_INIT_LOGOUT_CMND: return "INIT_LOGOUT_CMND";
        case ISCSI_INIT_SNACK: return "INIT_SNACK";
            
/* Target opcodes */
        case ISCSI_TARG_NOP_IN: return "TARG_NOP_IN";
        case ISCSI_TARG_SCSI_RSP: return "TARG_SCSI_RSP";
        case ISCSI_TARG_TASK_MGMT_RSP: return "TARG_TASK_MGMT_RSP";
        case ISCSI_TARG_LOGIN_RSP: return "TARG_LOGIN_RSP";
        case ISCSI_TARG_TEXT_RSP: return "TARG_TEXT_RSP";
        case ISCSI_TARG_SCSI_DATA_IN: return "TARG_SCSI_DATA_IN";
        case ISCSI_TARG_LOGOUT_RSP: return "TARG_LOGOUT_RSP";
        case ISCSI_TARG_R2T: return "TARG_R2T";
        case ISCSI_TARG_ASYNC_MSG: return "TARG_ASYNC_MSG";
        case ISCSI_TARG_RJT: return "TARG_RJT";
        default:
        {
            static char buf[32];
            sprintf(buf, "Unknown opcode %2.2X", opcode);
            return buf;
        }
    }
}

static void
print_opcode(uint8_t opcode)
{
	TRACE(TRACE_DEBUG, "    Opcode: %s,  I: %d", 
          map_opcode_to_name(opcode & ISCSI_OPCODE),
          (opcode & I_BIT) != 0);
}

static void
print_flags(uint8_t flags)
{
	TRACE(TRACE_DEBUG, "    flags: 0x%.2x", flags);
}

static void
print_version(char *which, uint8_t version)
{
	TRACE(TRACE_DEBUG, "    Version%s: 0x%.2x", which, version);
}

static void
print_response(uint8_t response)
{
	if (response != 0)
		TRACE(TRACE_DEBUG, "    Response: 0x%.2x", response);
}

static void
print_status(uint8_t status)
{
	if (status != 0)
		TRACE(TRACE_DEBUG, "    Status: 0x%.2x", status);
}

static void
print_lun(uint64_t lun)
{
	char string[20];

	if (lun != 0ll)
		TRACE(TRACE_DEBUG, "    LUN: %s", string_llx(lun, string));
}

static void
print_isid_tsih(uint8_t isid[6], uint16_t tsih)
{
	TRACE(TRACE_DEBUG, "    ISID: 0x%.2x %.2x %.2x %.2x %.2x %.2x",
	       isid[0], isid[1], isid[2], isid[3], isid[4], isid[5]);
	TRACE(TRACE_DEBUG, "    TSIH: %u", ntohs(tsih));
}

static void
print_dsl(uint32_t length)
{
	if (length != 0)
		TRACE(TRACE_DEBUG, "    DSL: %u", ntohl(length));
}

static void
print_itt(uint32_t init_task_tag)
{
	if (init_task_tag == ALL_ONES)
		TRACE(TRACE_DEBUG, "    ITT: 0x%08x", init_task_tag);
	else
		TRACE(TRACE_DEBUG, "    ITT: %u", ntohl(init_task_tag));
}

static void
print_ttt(uint32_t target_xfer_tag)
{
	if (target_xfer_tag == ALL_ONES)
		TRACE(TRACE_DEBUG, "    TTT: 0x%08x", target_xfer_tag);
	else
		TRACE(TRACE_DEBUG, "    TTT: %u", ntohl(target_xfer_tag));
}

static void
print_cid(uint16_t cid)
{
	TRACE(TRACE_DEBUG, "    CID: %u", ntohs(cid));
}

static void
print_expstatsn(uint32_t exp_stat_sn)
{
	if (exp_stat_sn != 0)
		TRACE(TRACE_DEBUG, "    ExpStatSN: %u", ntohl(exp_stat_sn));
}

static void
print_cmdsn_expstatsn(uint32_t cmd_sn, uint32_t exp_stat_sn)
{
	TRACE(TRACE_DEBUG, "    CmdSN: %u", ntohl(cmd_sn));
	print_expstatsn(exp_stat_sn);
}

static void
print_statsn_exp_max(uint32_t stat_sn, uint32_t exp_cmd_sn, uint32_t max_cmd_sn)
{
	if (stat_sn != 0)
		TRACE(TRACE_DEBUG, "    StatSN: %u", ntohl(stat_sn));
	TRACE(TRACE_DEBUG, "    ExpCmdSN: %u", ntohl(exp_cmd_sn));
	TRACE(TRACE_DEBUG, "    MaxCmdSN: %u", ntohl(max_cmd_sn));
}

static void
print_residual(uint32_t resid)
{
	if (resid != 0)
		TRACE(TRACE_DEBUG, "    ResidualCount: %u", ntohl(resid));
}

static void
print_datasn(uint32_t data_sn)
{
	if (data_sn != 0)
		TRACE(TRACE_DEBUG, "    DataSN: %u", ntohl(data_sn));
}

static void
print_offset(uint32_t offset)
{
	if (offset != 0)
		TRACE(TRACE_DEBUG, "    BufferOffset: %u", ntohl(offset));
}

static void
print_rtt(uint32_t ref_task_tag)
{
	if (ref_task_tag != 0)
		TRACE(TRACE_DEBUG, "    RTT: %u", ntohl(ref_task_tag));
}

static void
print_exp_data_sn(uint32_t exp_data_sn)
{
	if (exp_data_sn != 0)
		TRACE(TRACE_DEBUG, "    ExpDataSN: %u", ntohl(exp_data_sn));
}

static void
print_begrun(uint32_t begrun)
{
	TRACE(TRACE_DEBUG, "    BegRun: %u", ntohl(begrun));
}

static void
print_runlen(uint32_t runlen)
{
	TRACE(TRACE_DEBUG, "    RunLength: %u", ntohl(runlen));
}

void
print_init_scsi_cmnd(struct iscsi_init_scsi_cmnd *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	print_rsvd_u16(1, cmd->rsvd1);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	TRACE(TRACE_DEBUG, "    EDTL: %u", ntohl(cmd->xfer_len));
	print_cmdsn_expstatsn(cmd->cmd_sn, cmd->exp_stat_sn);
	VERB
	    ("    CDB: 0x%.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x "
	     "%.2x %.2x %.2x %.2x %.2x\n", cmd->cdb[0], cmd->cdb[1],
	     cmd->cdb[2], cmd->cdb[3], cmd->cdb[4], cmd->cdb[5], cmd->cdb[6],
	     cmd->cdb[7], cmd->cdb[8], cmd->cdb[9], cmd->cdb[10], cmd->cdb[11],
	     cmd->cdb[12], cmd->cdb[13], cmd->cdb[14], cmd->cdb[15]);
}

void
print_targ_scsi_rsp(struct iscsi_targ_scsi_rsp *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	print_response(cmd->response);
	print_status(cmd->status);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	print_ttt(cmd->target_xfer_tag);
	print_statsn_exp_max(cmd->stat_sn, cmd->exp_cmd_sn, cmd->max_cmd_sn);
	print_exp_data_sn(cmd->exp_data_sn);
	if (cmd->bidi_resid != 0)
		TRACE(TRACE_DEBUG, "    BidiResidualCount: %u",
		       ntohl(cmd->bidi_resid));
	print_residual(cmd->resid);
}

void
print_init_text_cmnd(struct iscsi_init_text_cmnd *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	print_rsvd_u16(2, cmd->rsvd2);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	print_ttt(cmd->target_xfer_tag);
	print_cmdsn_expstatsn(cmd->cmd_sn, cmd->exp_stat_sn);
	print_rsvd_u64(4, cmd->rsvd4);
	print_rsvd_u64(5, cmd->rsvd5);
}

void
print_targ_text_rsp(struct iscsi_targ_text_rsp *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	print_rsvd_u16(2, cmd->rsvd2);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	print_ttt(cmd->target_xfer_tag);
	print_statsn_exp_max(cmd->stat_sn, cmd->exp_cmd_sn, cmd->max_cmd_sn);
	print_rsvd_u32(4, cmd->rsvd4);
	print_rsvd_u64(5, cmd->rsvd5);
}

void
print_init_login_cmnd(struct iscsi_init_login_cmnd *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	print_version("Max", cmd->version_max);
	print_version("Min", cmd->version_min);
	print_dsl(cmd->length);
	print_isid_tsih(cmd->isid, cmd->tsih);
	print_itt(cmd->init_task_tag);
	print_cid(cmd->cid);
	print_rsvd_u16(1, cmd->rsvd1);
	print_cmdsn_expstatsn(cmd->cmd_sn, cmd->exp_stat_sn);
	print_rsvd_u64(2, cmd->rsvd2);
	print_rsvd_u64(3, cmd->rsvd3);
}

void
print_targ_login_rsp(struct iscsi_targ_login_rsp *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	print_version("Max", cmd->version_max);
	print_version("Active", cmd->version_active);
	print_dsl(cmd->length);
	print_isid_tsih(cmd->isid, cmd->tsih);
	print_itt(cmd->init_task_tag);
	print_rsvd_u32(1, cmd->rsvd1);
	print_statsn_exp_max(cmd->stat_sn, cmd->exp_cmd_sn, cmd->max_cmd_sn);
	if (cmd->status_class != 0)
		TRACE(TRACE_DEBUG, "    StatusClass: 0x%.2x", cmd->status_class);
	if (cmd->status_detail != 0)
		TRACE(TRACE_DEBUG, "    StatusDetail: 0x%.2x", cmd->status_detail);
	print_rsvd_u16(2, cmd->rsvd2);
	print_rsvd_u64(3, cmd->rsvd3);
}

void
print_init_logout_cmnd(struct iscsi_init_logout_cmnd *cmd)
{
	print_opcode(cmd->opcode);
	TRACE(TRACE_DEBUG, "reasoncod: 0x%.2x", cmd->reason);
	print_rsvd_u16(1, cmd->rsvd1);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	print_cid(cmd->cid);
	print_rsvd_u16(2, cmd->rsvd2);
	print_cmdsn_expstatsn(cmd->cmd_sn, cmd->exp_stat_sn);
	print_rsvd_u64(4, cmd->rsvd4);
	print_rsvd_u64(5, cmd->rsvd5);
}

void
print_targ_logout_rsp(struct iscsi_targ_logout_rsp *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	print_response(cmd->response);
	print_rsvd_u8(1, cmd->rsvd1);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	print_rsvd_u32(3, cmd->rsvd3);
	print_statsn_exp_max(cmd->stat_sn, cmd->exp_cmd_sn, cmd->max_cmd_sn);
	print_rsvd_u32(4, cmd->rsvd4);
	TRACE(TRACE_DEBUG, "    Time2Wait: 0x%.8x", ntohs(cmd->time2wait));
	TRACE(TRACE_DEBUG, "    Tm2Retain: 0x%.8x", ntohs(cmd->time2retain));
	print_rsvd_u32(5, cmd->rsvd5);
}

void
print_init_scsi_data_out(struct iscsi_init_scsi_data_out *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	print_rsvd_u16(2, cmd->rsvd2);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	print_ttt(cmd->target_xfer_tag);
	print_rsvd_u32(3, cmd->rsvd3);
	print_expstatsn(cmd->exp_stat_sn);
	print_rsvd_u32(4, cmd->rsvd4);
	print_datasn(cmd->data_sn);
	print_offset(cmd->offset);
	print_rsvd_u32(5, cmd->rsvd5);
}

void
print_targ_scsi_data_in(struct iscsi_targ_scsi_data_in *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	print_rsvd_u8(1, cmd->rsvd1);
	print_status(cmd->status);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	print_ttt(cmd->target_xfer_tag);
	print_statsn_exp_max(cmd->stat_sn, cmd->exp_cmd_sn, cmd->max_cmd_sn);
	print_datasn(cmd->data_sn);
	print_offset(cmd->offset);
	print_residual(cmd->resid);
}

void
print_targ_rjt(struct iscsi_targ_rjt *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	if (cmd->reason != 0)
		TRACE(TRACE_DEBUG, "    Reason: 0x%.2x", cmd->reason);
	print_rsvd_u8(2, cmd->rsvd2);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	print_rsvd_u32(4, cmd->rsvd4);
	print_statsn_exp_max(cmd->stat_sn, cmd->exp_cmd_sn, cmd->max_cmd_sn);
	print_datasn(cmd->data_sn);
	print_rsvd_u64(4, cmd->rsvd4);
	print_rsvd_u64(5, cmd->rsvd5);
}

void
print_init_nopout(struct iscsi_init_nopout *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	print_rsvd_u16(1, cmd->rsvd1);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	print_ttt(cmd->target_xfer_tag);
	print_cmdsn_expstatsn(cmd->cmd_sn, cmd->exp_stat_sn);
	print_rsvd_u64(2, cmd->rsvd2);
	print_rsvd_u64(3, cmd->rsvd3);
}

void
print_targ_nopin(struct iscsi_targ_nopin *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	print_rsvd_u16(1, cmd->rsvd1);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	print_ttt(cmd->target_xfer_tag);
	print_statsn_exp_max(cmd->stat_sn, cmd->exp_cmd_sn, cmd->max_cmd_sn);
	print_rsvd_u32(2, cmd->rsvd2);
	print_rsvd_u64(3, cmd->rsvd3);
}

void
print_targ_r2t(struct iscsi_targ_r2t *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	print_rsvd_u16(2, cmd->rsvd2);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	print_ttt(cmd->target_xfer_tag);
	print_statsn_exp_max(cmd->stat_sn, cmd->exp_cmd_sn, cmd->max_cmd_sn);
	TRACE(TRACE_DEBUG, "    R2TSN: %u", ntohl(cmd->r2t_sn));
	print_offset(cmd->offset);
	TRACE(TRACE_DEBUG, "    DDTL: %u", ntohl(cmd->xfer_len));
}

void
print_targ_async_msg(struct iscsi_targ_async_msg *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	print_rsvd_u16(2, cmd->rsvd2);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	print_rsvd_u32(3, cmd->rsvd3);
	print_statsn_exp_max(cmd->stat_sn, cmd->exp_cmd_sn, cmd->max_cmd_sn);
	TRACE(TRACE_DEBUG, "AsyncEvnt: %u", cmd->async_event);
	TRACE(TRACE_DEBUG, "AsyncVCod: %u", cmd->async_vcode);
	if (cmd->parameter1 != 0)
		TRACE(TRACE_DEBUG, "   Param1: %u", ntohs(cmd->parameter1));
	if (cmd->parameter2 != 0)
		TRACE(TRACE_DEBUG, "   Param2: %u", ntohs(cmd->parameter2));
	if (cmd->parameter3 != 0)
		TRACE(TRACE_DEBUG, "   Param3: %u", ntohs(cmd->parameter3));
	print_rsvd_u32(5, cmd->rsvd5);
}

void
print_init_task_mgt_command(struct iscsi_init_task_mgt_command *cmd)
{
	print_opcode(cmd->opcode);
	TRACE(TRACE_DEBUG, "    Function: 0x%.2x", cmd->function);
	print_rsvd_u16(1, cmd->rsvd1);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	print_rtt(cmd->ref_task_tag);
	print_cmdsn_expstatsn(cmd->cmd_sn, cmd->exp_stat_sn);
	if (cmd->ref_cmd_sn != 0)
		TRACE(TRACE_DEBUG, "    RefCmdSN: %u", ntohl(cmd->ref_cmd_sn));
	print_exp_data_sn(cmd->exp_data_sn);
	print_rsvd_u64(4, cmd->rsvd4);
}

void
print_targ_task_mgt_response(struct iscsi_targ_task_mgt_response *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	print_response(cmd->response);
	print_rsvd_u8(1, cmd->rsvd1);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	print_rsvd_u32(2, cmd->rsvd2);
	print_statsn_exp_max(cmd->stat_sn, cmd->exp_cmd_sn, cmd->max_cmd_sn);
	print_rsvd_u32(4, cmd->rsvd4);
	print_rsvd_u64(5, cmd->rsvd5);
}

void
print_init_snack(struct iscsi_init_snack *cmd)
{
	print_opcode(cmd->opcode);
	print_flags(cmd->flags);
	print_rsvd_u16(1, cmd->rsvd1);
	print_dsl(cmd->length);
	print_lun(cmd->lun);
	print_itt(cmd->init_task_tag);
	print_ttt(cmd->target_xfer_tag);
	print_rsvd_u32(2, cmd->rsvd2);
	print_expstatsn(cmd->exp_stat_sn);
	print_rsvd_u64(3, cmd->rsvd3);
	print_begrun(cmd->begrun);
	print_runlen(cmd->runlen);
}

void
print_iscsi_command(void *cmd)
{
	struct generic_pdu *pdu = cmd;

	switch (pdu->opcode & ISCSI_OPCODE) {
	case ISCSI_INIT_NOP_OUT:
		print_init_nopout((struct iscsi_init_nopout *)cmd);
		break;
	case ISCSI_INIT_SCSI_CMND:
		print_init_scsi_cmnd((struct iscsi_init_scsi_cmnd *)cmd);
		break;
	case ISCSI_INIT_TASK_MGMT_CMND:
		print_init_task_mgt_command(
				(struct iscsi_init_task_mgt_command *)cmd);
		break;
	case ISCSI_INIT_LOGIN_CMND:
		print_init_login_cmnd((struct iscsi_init_login_cmnd *)cmd);
		break;
	case ISCSI_INIT_TEXT_CMND:
		print_init_text_cmnd((struct iscsi_init_text_cmnd *)cmd);
		break;
	case ISCSI_INIT_SCSI_DATA_OUT:
		print_init_scsi_data_out((struct iscsi_init_scsi_data_out*)cmd);
		break;
	case ISCSI_INIT_LOGOUT_CMND:
		print_init_logout_cmnd((struct iscsi_init_logout_cmnd *)cmd);
		break;
	case ISCSI_INIT_SNACK:
		print_init_snack((struct iscsi_init_snack *)cmd);
		break;
	case ISCSI_TARG_NOP_IN:
		print_targ_nopin((struct iscsi_targ_nopin *)cmd);
		break;
	case ISCSI_TARG_SCSI_RSP:
		print_targ_scsi_rsp((struct iscsi_targ_scsi_rsp *)cmd);
		break;
	case ISCSI_TARG_TASK_MGMT_RSP:
		print_targ_task_mgt_response(
				(struct iscsi_targ_task_mgt_response *)cmd);
		break;
	case ISCSI_TARG_LOGIN_RSP:
		print_targ_login_rsp((struct iscsi_targ_login_rsp *)cmd);
		break;
	case ISCSI_TARG_TEXT_RSP:
		print_targ_text_rsp((struct iscsi_targ_text_rsp *)cmd);
		break;
	case ISCSI_TARG_SCSI_DATA_IN:
		print_targ_scsi_data_in((struct iscsi_targ_scsi_data_in *)cmd);
		break;
	case ISCSI_TARG_LOGOUT_RSP:
		print_targ_logout_rsp((struct iscsi_targ_logout_rsp *)cmd);
		break;
	case ISCSI_TARG_R2T:
		print_targ_r2t((struct iscsi_targ_r2t *)cmd);
		break;
	case ISCSI_TARG_ASYNC_MSG:
		print_targ_async_msg((struct iscsi_targ_async_msg *)cmd);
		break;
	case ISCSI_TARG_RJT:
		print_targ_rjt((struct iscsi_targ_rjt *)cmd);
		break;
	default:	/* an illegal opcode! */
		print_opcode(pdu->opcode);
		print_flags(pdu->flags);
		print_dsl(pdu->length);
		print_itt(pdu->init_task_tag);
		break;
	}	/* switch */
}

/*	returns pointer to printable name of SCSI op code */
/*	(Normally this is byte 1 of a SCSI CDB */
char *
printable_scsi_op(uint8_t opcode, char *op_buf)
{
	char *result = "";

	/*      opcode symbols defined in <scsi/scsi.h> */
	/*      this table includes only those symbols we have actually seen
	   being sent to/from a vendor */
	switch (opcode) {
	case TEST_UNIT_READY:	/* 0x00 */
		result = "TEST_UNIT_READY";
		break;
	case REZERO_UNIT:	/* 0x01 */
		result = "REWIND";
		break;
	case REQUEST_SENSE:	/* 0x03 */
		result = "REQUEST_SENSE";
		break;
	case READ_BLOCK_LIMITS:	/* 0x05 */
		result = "READ_BLOCK_LIMITS";
		break;
	case READ_6:		/* 0x08 */
		result = "READ_6";
		break;
	case WRITE_6:		/* 0x0a */
		result = "WRITE_6";
		break;
	case WRITE_FILEMARKS:	/* 0x10 */
		result = "WRITE_FILEMARKS";
		break;
	case INQUIRY:		/* 0x12 */
		result = "INQUIRY";
		break;
	case MODE_SENSE:	/* 0x1a */
		result = "MODE_SENSE";
		break;
	case READ_CAPACITY:	/* 0x25 */
		result = "READ_CAPACITY";
		break;
	case READ_10:		/* 0x28 */
		result = "READ_10";
		break;
	case WRITE_10:		/* 0x2a */
		result = "WRITE_10";
		break;
	case READ_12:		/* 0xa8 */
		result = "READ_12";
		break;
	case WRITE_12:		/* 0xaa */
		result = "WRITE_12";
		break;
	default:
		sprintf(op_buf, "CDBop 0x%02x", opcode);
		result = op_buf;
		break;
	}			/* switch */
	return result;
}

/*	returns pointer to printable name of iSCSI op code, but if this is
 *	a SCSI Command, return the printable name of the SCSI op from the cdb
 */
char *
printable_iscsi_op(void *cmd, char *op_buf)
{
	struct generic_pdu *pdu = cmd;
	char *result = "";

	switch (pdu->opcode & ISCSI_OPCODE) {
	case ISCSI_INIT_NOP_OUT:
		result = "NopOut";
		break;
	case ISCSI_INIT_SCSI_CMND:
		result = printable_scsi_op(((struct iscsi_init_scsi_cmnd *)cmd)
							->cdb[0], op_buf);
		break;
	case ISCSI_INIT_TASK_MGMT_CMND:
		result = "Task MGMT Req";
		break;
	case ISCSI_INIT_LOGIN_CMND:
		result = "Login";
		break;
	case ISCSI_INIT_TEXT_CMND:
		result = "Text Req";
		break;
	case ISCSI_INIT_SCSI_DATA_OUT:
		result = "DataOut";
		break;
	case ISCSI_INIT_LOGOUT_CMND:
		result = "Logout";
		break;
	case ISCSI_INIT_SNACK:
		result = "SNACK";
		break;
	case ISCSI_TARG_NOP_IN:
		result = "NopIn";
		break;
	case ISCSI_TARG_SCSI_RSP:
		result = "SCSI Rsp";
		break;
	case ISCSI_TARG_TASK_MGMT_RSP:
		result = "Task MGMT Rsp";
		break;
	case ISCSI_TARG_LOGIN_RSP:
		result = "Login Rsp";
		break;
	case ISCSI_TARG_TEXT_RSP:
		result = "Text Rsp";
		break;
	case ISCSI_TARG_SCSI_DATA_IN:
		result = "DataIn";
		break;
	case ISCSI_TARG_LOGOUT_RSP:
		result = "Logout Rsp";
		break;
	case ISCSI_TARG_R2T:
		result = "R2T";
		break;
	case ISCSI_TARG_ASYNC_MSG:
		result = "ASYNC Message";
		break;
	case ISCSI_TARG_RJT:
		result = "Reject";
		break;
	default:	/* an illegal opcode! */
		sprintf(op_buf, "bad op 0x%02x", pdu->opcode & ISCSI_OPCODE);
		result = op_buf;
		break;
	}	/* switch */
	return result;
}

int target_in_use(uint32_t target_id)
{
    if (target_id == 0)
        return 1;

    return 0;
}
