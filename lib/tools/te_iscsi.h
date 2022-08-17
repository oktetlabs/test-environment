/* SPDX-License-Identifier: Apache-2.0 */
/** @file
 * @brief TE iSCSI stuff
 *
 * @defgroup te_tools_te_iscsi iSCSI
 * @ingroup te_tools
 * @{
 *
 * Functions and constant declared here can be used bothe on the agent
 * and in tests.
 *
 * Copyright (C) 2004-2022 OKTET Labs Ltd. All rights reserved.
 */

#ifndef __TE_ISCSI_H__
#define __TE_ISCSI_H__

#define ISCSI_HDR_LEN 48    /* Basic Header Len */

/** iSCSI opcodes */
/** RFC 3720, Section 10.2.1.2 Opcode */

/** Initiator opcodes */
enum iscsi_initiator_opcode {
    ISCSI_INIT_NOP_OUT        = 0x00,
    ISCSI_INIT_SCSI_CMND      = 0x01,
    ISCSI_INIT_TASK_MGMT_CMND = 0x02,
    ISCSI_INIT_LOGIN_CMND     = 0x03,
    ISCSI_INIT_TEXT_CMND      = 0x04,
    ISCSI_INIT_SCSI_DATA_OUT  = 0x05,
    ISCSI_INIT_LOGOUT_CMND    = 0x06,
    ISCSI_INIT_SNACK          = 0x10
};

/** Target opcodes */
enum iscsi_target_opcodes {
    ISCSI_TARG_NOP_IN        = 0x20,
    ISCSI_TARG_SCSI_RSP      = 0x21,
    ISCSI_TARG_TASK_MGMT_RSP = 0x22,
    ISCSI_TARG_LOGIN_RSP     = 0x23,
    ISCSI_TARG_TEXT_RSP      = 0x24,
    ISCSI_TARG_SCSI_DATA_IN  = 0x25,
    ISCSI_TARG_LOGOUT_RSP    = 0x26,
    ISCSI_TARG_R2T           = 0x31,
    ISCSI_TARG_ASYNC_MSG     = 0x32,
    ISCSI_TARG_RJT           = 0x3f
};

/** Logout responses */
enum iscsi_logout_responses {
    ISCSI_LOGOUT_RESPONSE_OK = 0x00,
    ISCSI_LOGOUT_RESPONSE_NO_CID = 0x01,
    ISCSI_LOGOUT_RESPONSE_CONN_RECOVERY_NOT_SUPPORTED = 0x02,
    ISCSI_LOGOUT_RESPONSE_CLEANUP_FAILED = 0x03,
};

static inline int
iscsi_logout_resp_str2int(const char *resp)
{
    if (strcmp(resp, "NO_CID") == 0)
        return ISCSI_LOGOUT_RESPONSE_NO_CID;

    if (strcmp(resp, "CONN_RECOVERY_NOT_SUPPORTED") == 0)
        return ISCSI_LOGOUT_RESPONSE_CONN_RECOVERY_NOT_SUPPORTED;

    if (strcmp(resp, "OK") == 0)
        return ISCSI_LOGOUT_RESPONSE_OK;

    return ISCSI_LOGOUT_RESPONSE_CLEANUP_FAILED;
}

#define USE_FLAT_SPACE_LUN            0x0001
#define USE_FULL_REPLIES              0x0002
#define USE_SECURITY_PHASE            0x0004
#define USE_OPERATIONAL_PHASE         0x0008
#define USE_HEADERDIGEST              0x0010
#define USE_DATADIGEST                0x0020
#define GOT_ACTIVITY                  0x0040
#define SEND_NO_REPLY_TO_NOP_PING     0x0080
#define SEND_NO_REPLY_TO_ASYNC_LOGOUT 0x0100
#define USE_REFLECT_XKEYS             0x0200
#define CONN_LOGGED_OUT               0x0400
#define NEED_TX_WAKEUP                0x0800
#define TX_TIMER_OFF                  0x1000
#define USE_ONE_KEY_PER_TEXT          0x2000
#define CONN_HIT_EOF                  0x4000

#define USE_TARGET_CONFIRMATION 0x01
#define USE_BASE64              0x02


#define ISCSI_OPCODE 0x3f
#define REPLY_BIT    0x20
#define I_BIT        0x40
#define OLD_X_BIT    0x80

#define F_BIT           0x80
#define R_BIT           0x40
#define A_BIT           0x40
#define C_BIT           0x40
#define READ_TYPE_CMND  R_BIT
#define W_BIT           0x20
#define WRITE_TYPE_CMND W_BIT
#define BRO_BIT         0x10
#define BRU_BIT         0x08
#define O_BIT           0x04
#define U_BIT           0x02
#define S_BIT           0x01

#define T_BIT     0x80
#define CSG       0x0C
#define CSG1      0x04
#define CSG2      0x08
#define CSG3      0x0C
#define CSG_SHIFT 2
#define NSG       0x03
#define NSG1      0x01
#define NSG2      0x02
#define NSG3      0x03

/**
 * Enum of the login states.
 */
typedef enum {
    LOGIN_STAGE_SECURITY      = 0,
    LOGIN_STAGE_OPERATIONAL   = 1,
    LOGIN_STAGE_FULLFEATURE   = 3,
} iscsi_login_stage_t;


/** RFC 3720, Section 10.3.1 Flags and Task Attibutes (byte 1) */
/** SCSI Command ATTR value */

enum iscsi_command_attr_values {
    ISCSI_UNTAGGED      = 0x00,
    ISCSI_SIMPLE        = 0x01,
    ISCSI_ORDERED       = 0x02,
    ISCSI_HEAD_OF_QUEUE = 0x03,
    ISCSI_ACA           = 0x04
};


/** RFC 3720 Section 10.5.1 Function (field in TMF Request) */
enum iscsi_tmf_task_codes {
    TMF_ABORT_TASK        = 1,
    TMF_ABORT_TASK_SET    = 2,
    TMF_CLEAR_ACA         = 3,
    TMF_CLEAR_TASK_SET    = 4,
    TMF_LUN_RESET         = 5,
    TMF_TARGET_WARM_RESET = 6,
    TMF_TARGET_COLD_RESET = 7,
    TMF_TASK_REASSIGN     = 8
};

/** RFC 3720, Section 10.13.5 Status-Class and Status-Detail */

/** Status-Class field in Login Response */
enum iscsi_status_classes {
    STAT_CLASS_SUCCESS     = 0x00,
    STAT_CLASS_REDIRECTION = 0x01,
    STAT_CLASS_INITIATOR   = 0x02,
    STAT_CLASS_TARGET      = 0x03
};

/** Status-Detail field in Login Response */
enum iscsi_status_details {
    STAT_DETAIL_TARG_MOVED_TEMP       = 0x01,
    STAT_DETAIL_TARG_MOVED_PERM       = 0x02,
    STAT_DETAIL_ERR                   = 0x00,
    STAT_DETAIL_NOT_AUTH              = 0x01,
    STAT_DETAIL_NOT_ALLOWED           = 0x02,
    STAT_DETAIL_NOT_FOUND             = 0x03,
    STAT_DETAIL_TARG_REMOVED          = 0x04,
    STAT_DETAIL_VERSION_NOT_SUPPORTED = 0x05,
    STAT_DETAIL_TOO_MANY_CONNECTIONS  = 0x06,
    STAT_DETAIL_MISSING_PARAMETER     = 0x07,
    STAT_DETAIL_NOT_INCLUDED          = 0x08,
    STAT_DETAIL_SESSION_TYPE          = 0x09,
    STAT_DETAIL_SESSION_NOT_EXIST     = 0x0a,
    STAT_DETAIL_INVALID_DURING_LOGIN  = 0x0b,
    STAT_DETAIL_SERVICE_UNAVAILABLE   = 0x01,
    STAT_DETAIL_OUT_OF_RESOURCE       = 0x02
};

/** RFC 3720, Section 10.6.1 Response (field in TMF Response) */
enum iscsi_tmf_responses {
    FUNCTION_COMPLETE                      = 0,
    TASK_DOES_NOT_EXIST                    = 1,
    LUN_DOES_NOT_EXIST                     = 2,
    TASK_STILL_ALLEGIANT                   = 3,
    TASK_FAILOVER_NOT_SUPPORTED            = 4,
    TASK_MANAGEMENT_FUNCTION_NOT_SUPPORTED = 5,
    FUNCTION_AUTHORIZATION_FAILED          = 6,
    FUNCTION_REJECTED                      = 255,
};

/** RFC 3720 Section 10.17.1 Reason (field in Reject Response) */
enum iscsi_reject_reasons {
    REASON_DATA_DIGEST_ERR             = 0x02,
    REASON_DATA_SNACK                  = 0x03,
    REASON_PROTOCOL_ERR                = 0x04,
    REASON_COMMAND_NOT_SUPPORTED       = 0x05,
    REASON_TOO_MANY_IMMEDIATE_COMMANDS = 0x06,
    REASON_TASK_IN_PROGRESS            = 0x07,
    REASON_INVALID_SNACK               = 0x08,
    REASON_INVALID_PDU_FIELD           = 0x09,
    REASON_OUT_OF_RESOURCES            = 0x0a,
    REASON_NEGOTIATION_RESET           = 0x0b,
    REASON_WAITING_FOR_LOGOUT          = 0x0c,
};

/** RFC 3720, Section 10.14.1 Reason Code (field in Logout Request) */
enum iscsi_logout_reasons {
    LOGOUT_REASON                   = 0x7f,
    LOGOUT_CLOSE_SESSION            = 0x00,
    LOGOUT_CLOSE_CONNECTION         = 0x01,
    LOGOUT_REMOVE_CONN_FOR_RECOVERY = 0x02,
};


/** RFC 3720, Section 10.16.1 Type (field in SNACK Request) */
enum iscsi_snack_types {
    SNACK_TYPE     = 0x0f,
    DATA_R2T_SNACK = 0x00,
    STATUS_SNACK   = 0x01,
    DATACK_SNACK   = 0x02,
    R_DATA_SNACK   = 0x03,
};

/** ErrorRecoveryLevel values */
enum iscsi_error_recovery_levels {
    SESSION_RECOVERY    = 0,
    DIGEST_RECOVERY     = 1,
    CONNECTION_RECOVERY = 2,
};

#define CONNREC_NOT_SUPPORTED   0x02
#define TIME2WAIT               0x0001
#define TIME2RETAIN             0x0005

#define DATA_SNACK_ENABLE       0x01
#define STATUS_SNACK_ENABLE     0x02
#define DATACK_SNACK_ENABLE     0x04


/** RFC 3720, Section 5.3 Login Phase
 *   "The default MaxRecvDataSegmentLength is used during Login."
 *
 *  RFC 3720, Section 12.12 MaxRecvDataSegmentLength
 *  "Default is 8192 bytes."
 */
#define MAX_TEXT_LEN                8192
#define TEXT_FUDGE_LEN              512

/** SNACK for Error Recovery */
#define SNACK                   0x00000030

/**
 * Parameters for iSCSI target emulator thread
 */
typedef struct {
    int send_recv_sock;
    int custom_id;
    int reject;
} iscsi_target_thread_params_t;

/* parse layout */
/* byteorder: network; */

/* name: ISCSI_BHS; */
/* accessors: bhs_; */
/* mostly_reserved: yes; */
struct generic_pdu {
    /* bits: 0-5; reader: yes; writer: yes; */
    uint8_t   opcode;
    /* name: F_BIT; bit: 7; reader: yes; writer: yes; */
    /* bittype: te_bool; */
    /*-*/
    /* name: I_BIT; bit: 6; reader: yes; writer: yes; */
    /* bittype: te_bool; */
    /*-*/
    uint8_t   flags;
    uint8_t   version_max;
    uint8_t   version_active;
    /* name: data_segment_length; */
    /* bits: 0-23; reader: yes; writer: yes; */
    uint32_t  length;
    /* name: total_ahs_length; */
    /* bits: 24-31; reader: yes; writer: yes; */
    /*-*/
    uint8_t   isid[6];
    /* reader: yes; writer: yes; */
    uint16_t  tsih;
    /* reader: yes; writer: yes; */
    uint32_t  init_task_tag;
    uint16_t  CID;
    uint16_t  rsvd1;
    uint32_t  cmd_sn;
    /* reader: yes; writer: yes; */
    uint32_t  exp_stat_sn;
    uint32_t  max_cmd_sn;
    uint8_t   status_class;
    uint8_t   status_detail;
    uint16_t  rsvd2;
    uint32_t  offset;
    uint32_t  resid;
    /* ignore: yes; */
    uint32_t  header_digest;
    uint8_t  *text;
    uint32_t  text_length;
} __attribute__((packed));

/* name: ISCSI_BHS; */
/* mostly_reserved: yes; */
struct response_pdu {
    uint8_t  opcode;
    uint8_t  flags;
    uint8_t  reason;
    uint8_t  version_active;
    uint32_t length;
    uint64_t lun;
    uint32_t init_task_tag;
    uint32_t target_xfer_tag;
    /* reader: yes; writer: yes; */
    uint32_t stat_sn;
    uint32_t exp_cmd_sn;
    uint32_t max_cmd_sn;
    uint8_t  status_class;
    uint8_t  status_detail;
    uint16_t rsvd2;
    uint32_t offset;
    uint32_t xfer_len;
    uint32_t header_digest;
} __attribute__((packed));


/** iSCSI PDU formats */
/* ignore: yes; */
struct iscsi_init_scsi_cmnd {
    uint8_t  opcode;
    uint8_t  flags;
    /* reserved: yes; */
    uint16_t rsvd1;
    uint32_t length;
    uint64_t lun;
    uint32_t init_task_tag;
    uint32_t xfer_len;
    uint32_t cmd_sn;
    uint32_t exp_stat_sn;
    uint8_t  cdb[16];
    uint32_t header_digest;
} __attribute__((packed));

/** RFC 3720, Section 10.4 SCSI Response */

/* accessors: bhs_scsi_; */
/* name: ISCSI_BHS_SCSI; */
/* mostly_reserved: yes; */
struct iscsi_targ_scsi_rsp {
    uint8_t  opcode;
    uint8_t  flags;
    uint8_t  response;
    /* reader: yes; writer: yes; */
    uint8_t  status;
    uint32_t length;
    uint64_t lun;
    uint32_t init_task_tag;
    uint32_t target_xfer_tag;
    uint32_t stat_sn;
    uint32_t exp_cmd_sn;
    uint32_t max_cmd_sn;
    uint32_t exp_data_sn;
    uint32_t bidi_resid;
    uint32_t resid;
    uint32_t header_digest;
} __attribute__((packed));

/** RFC 3720, Section 10.5 Task Management Function Request */
/* ignore: yes; */
struct iscsi_init_task_mgt_command {
    uint8_t  opcode;
    uint8_t  function;
    /* reserved: yes; */
    uint16_t rsvd1;
    uint32_t length;
    uint64_t lun;
    uint32_t init_task_tag;
    uint32_t ref_task_tag;
    uint32_t cmd_sn;
    uint32_t exp_stat_sn;
    uint32_t ref_cmd_sn;
    uint32_t exp_data_sn;
    uint64_t rsvd4;
    uint32_t header_digest;
} __attribute__((packed));

/** RFC 3720, Section 10.6 Task Management Function Response */
/* ignore: yes; */
struct iscsi_targ_task_mgt_response {
    uint8_t  opcode;
    uint8_t  flags;
    uint8_t  response;
    /* reserved: yes; */
    uint8_t  rsvd1;
    uint32_t length;
    uint64_t lun;
    uint32_t init_task_tag;
    uint32_t rsvd2;
    uint32_t stat_sn;
    uint32_t exp_cmd_sn;
    uint32_t max_cmd_sn;
    uint32_t rsvd4;
    uint64_t rsvd5;
    uint32_t header_digest;
} __attribute__((packed));

/** RFC 3720, Section 10.7 SCSI Data-out */
/* ignore: yes; */
struct iscsi_init_scsi_data_out {
    uint8_t  opcode;
    uint8_t  flags;
    /* reserved: yes; */
    uint16_t rsvd2;
    uint32_t length;
    uint64_t lun;
    uint32_t init_task_tag;
    uint32_t target_xfer_tag;
    /* reserved: yes; */
    uint32_t rsvd3;
    uint32_t exp_stat_sn;
    /* reserved: yes; */
    uint32_t rsvd4;
    uint32_t data_sn;
    uint32_t offset;
    /* reserved: yes; */
    uint32_t rsvd5;
    uint32_t header_digest;
} __attribute__((packed));

/** RFC 3720, Section 10.7 SCSI Data-in */
/* ignore: yes; */
struct iscsi_targ_scsi_data_in {
    uint8_t  opcode;
    uint8_t  flags;
    /* reserved: yes; */
    uint8_t  rsvd1;
    uint8_t  status;
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
    uint32_t header_digest;
} __attribute__((packed));

/** RFC 3720, Section 10.8 Ready To Transfer (R2T) */
/* ignore: yes; */
struct iscsi_targ_r2t {
    uint8_t  opcode;
    uint8_t  flags;
    /* reserved: yes; */
    uint16_t rsvd2;
    uint32_t length;
    uint64_t lun;
    uint32_t init_task_tag;
    uint32_t target_xfer_tag;
    uint32_t stat_sn;
    uint32_t exp_cmd_sn;
    uint32_t max_cmd_sn;
    uint32_t r2t_sn;
    uint32_t offset;
    uint32_t xfer_len;
    uint32_t header_digest;
} __attribute__((packed));

/** RFC 3720, Section 10.9 Asynchronous Message */
/* ignore: yes; */
struct iscsi_targ_async_msg {
    uint8_t  opcode;
    uint8_t  flags;
    /* reserved: yes; */
    uint16_t rsvd2;
    uint32_t length;
    uint64_t lun;
    uint32_t init_task_tag;
    /* reserved: yes; */
    uint32_t rsvd3;
    uint32_t stat_sn;
    uint32_t exp_cmd_sn;
    uint32_t max_cmd_sn;
    uint8_t  async_event;
    uint8_t  async_vcode;
    uint16_t parameter1;
    uint16_t parameter2;
    uint16_t parameter3;
    /* reserved: yes; */
    uint32_t rsvd5;
    uint32_t header_digest;
} __attribute__((packed));

enum iscsi_targ_async_msg_events {
    ISCSI_ASYNC_SCSI_EVENT,
    ISCSI_ASYNC_LOGOUT_REQUEST,
    ISCSI_ASYNC_DROP_CONNECTION,
    ISCSI_ASYNC_DROP_ALL,
    ISCSI_ASYNC_RENEGOTIATE,
    ISCSI_ASYNC_VENDOR = 255
};

/** RFC 3720, Section 10.10 Text Request */
/* ignore: yes; */
struct iscsi_init_text_cmnd {
    uint8_t  opcode;
    uint8_t  flags;
    /* reserved: yes; */
    uint16_t rsvd2;
    uint32_t length;
    uint64_t lun;
    uint32_t init_task_tag;
    uint32_t target_xfer_tag;
    uint32_t cmd_sn;
    uint32_t exp_stat_sn;
    /* reserved: yes; */
    uint64_t rsvd4;
    /* reserved: yes; */
    uint64_t rsvd5;
    uint32_t header_digest;
} __attribute__((packed));

/**< RFC 3720, Section 10.11 Text Response */
/* ignore: yes; */
struct iscsi_targ_text_rsp {
    uint8_t  opcode;
    uint8_t  flags;
    /* reserved: yes; */
    uint16_t rsvd2;
    uint32_t length;
    uint64_t lun;
    uint32_t init_task_tag;
    uint32_t target_xfer_tag;
    uint32_t stat_sn;
    uint32_t exp_cmd_sn;
    uint32_t max_cmd_sn;
    /* reserved: yes; */
    uint32_t rsvd4;
    /* reserved: yes; */
    uint64_t rsvd5;
    uint32_t header_digest;
} __attribute__((packed));

/** RFC 3720, Section 10.12 Login Request */
struct iscsi_init_login_cmnd {
    uint8_t  opcode;
    uint8_t  flags;
    uint8_t  version_max;
    uint8_t  version_min;
    uint32_t length;
    uint8_t  isid[6];
    uint16_t tsih;
    uint32_t init_task_tag;
    uint16_t cid;
    /* reserved: yes; */
    uint16_t rsvd1;
    uint32_t cmd_sn;
    uint32_t exp_stat_sn;
    /* reserved: yes; */
    uint64_t rsvd2;
    /* reserved: yes; */
    uint64_t rsvd3;
    uint32_t header_digest;
} __attribute__((packed));

/** RFC 3720, Section 10.13 Login Response */
struct iscsi_targ_login_rsp {
    uint8_t  opcode;
    uint8_t  flags;
    uint8_t  version_max;
    uint8_t  version_active;
    uint32_t length;
    uint8_t  isid[6];
    uint16_t tsih;
    uint32_t init_task_tag;
    /* reserved: yes; */
    uint32_t rsvd1;
    uint32_t stat_sn;
    uint32_t exp_cmd_sn;
    uint32_t max_cmd_sn;
    uint8_t  status_class;
    uint8_t  status_detail;
    /* reserved: yes; */
    uint16_t rsvd2;
    /* reserved: yes; */
    uint64_t rsvd3;
    uint32_t header_digest;
} __attribute__((packed));

/** RFC 3720, Section 10.14 Logout Request */
struct iscsi_init_logout_cmnd {
    uint8_t  opcode;
    uint8_t  reason;
    /* reserved: yes; */
    uint16_t rsvd1;
    uint32_t length;
    uint64_t lun;
    uint32_t init_task_tag;
    uint16_t cid;
    /* reserved: yes; */
    uint16_t rsvd2;
    uint32_t cmd_sn;
    uint32_t exp_stat_sn;
    /* reserved: yes; */
    uint64_t rsvd4;
    /* reserved: yes; */
    uint64_t rsvd5;
    uint32_t header_digest;
} __attribute__((packed));

/** RFC 3720, Section 10.15 Logout Response */
struct iscsi_targ_logout_rsp {
    uint8_t  opcode;
    uint8_t  flags;
    uint8_t  response;
    /* reserved: yes; */
    uint8_t  rsvd1;
    uint32_t length;
    uint64_t lun;
    uint32_t init_task_tag;
    /* reserved: yes; */
    uint32_t rsvd3;
    uint32_t stat_sn;
    uint32_t exp_cmd_sn;
    uint32_t max_cmd_sn;
    /* reserved: yes; */
    uint32_t rsvd4;
    uint16_t time2wait;
    uint16_t time2retain;
    /* reserved: yes; */
    uint32_t rsvd5;
    uint32_t header_digest;
} __attribute__((packed));

/** RFC 3720, Section 10.16 SNACK Request */
struct iscsi_init_snack {
    uint8_t  opcode;
    uint8_t  flags;
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
    uint32_t header_digest;
} __attribute__((packed));

/** RFC 3720, Section 10.17 Reject */
struct iscsi_targ_rjt {
    uint8_t  opcode;
    uint8_t  flags;
    uint8_t  reason;
    uint8_t  rsvd2;
    uint32_t length;
    uint64_t lun;
    uint32_t init_task_tag;
    uint32_t rsvd4;
    uint32_t stat_sn;
    uint32_t exp_cmd_sn;
    uint32_t max_cmd_sn;
    uint32_t data_sn;
    uint64_t rsvd5;
    uint32_t header_digest;
} __attribute__((packed));

/** RFC 3720, Section 10.18 NOP-Out */
struct iscsi_init_nopout {
    uint8_t  opcode;
    uint8_t  flags;
    uint16_t rsvd1;
    uint32_t length;
    uint64_t lun;
    uint32_t init_task_tag;
    uint32_t target_xfer_tag;
    uint32_t cmd_sn;
    uint32_t exp_stat_sn;
    uint64_t rsvd2;
    uint64_t rsvd3;
    uint32_t header_digest;
} __attribute__((packed));

/** RFC 3720, Section 10.19 NOP-In */
struct iscsi_targ_nopin {
    uint8_t  opcode;
    uint8_t  flags;
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
    uint32_t header_digest;
} __attribute__((packed));


/** The format of the data returned by the SCSI REQUEST SENSE command.
 *  SCSI Primary Commands - 2 (SPC-2)
 *  ANSI Project T10/1236-D
 *  Revision 20 18 July 2001
 *  page 135
 *  7.20.1 REQUEST SENSE command introduction
 *      "Device servers shall be capable of returning 18 bytes of data
 *      in response to a REQUEST SENSE command."
 *  page 136
 *  7.20.2 Sense data format
 *      Table 102 -- Response codes 70h and 71h sense data format
 *
 *  This structure is the minimum a device can return.
 *  The file linux/drivers/scsi/scsi.h defines the maximum acceptable as:
 *      #define SCSI_SENSE_BUFFERSIZE   64
 *      unsigned char sr_sense_buffer[SCSI_SENSE_BUFFERSIZE];
 *      unsigned char sense_buffer[SCSI_SENSE_BUFFERSIZE];
 */

struct scsi_fixed_sense_data
{
    uint8_t  response;
    uint8_t  obsolete;
    uint8_t  sense_key_and_flags;
    uint32_t information;
    uint8_t  additional_length;
    uint32_t csi;
    uint8_t  asc;
    uint8_t  ascq;
    uint8_t  fruc;
    uint8_t  sks[3];
} __attribute__ ((packed));

/* end parse layout */

/*
 *  SCSI Architecture Model (SAM) Status codes. Taken from SAM-3 draft
 *  T10/1561-D Revision 4 Draft dated 7th November 2002.
 */
#define SAM_STAT_GOOD            0x00
#define SAM_STAT_CHECK_CONDITION 0x02
#define SAM_STAT_CONDITION_MET   0x04
#define SAM_STAT_BUSY            0x08
#define SAM_STAT_INTERMEDIATE    0x10
#define SAM_STAT_INTERMEDIATE_CONDITION_MET 0x14
#define SAM_STAT_RESERVATION_CONFLICT 0x18
#define SAM_STAT_COMMAND_TERMINATED 0x22    /* obsolete in SAM-3 */
#define SAM_STAT_TASK_SET_FULL   0x28
#define SAM_STAT_ACA_ACTIVE      0x30
#define SAM_STAT_TASK_ABORTED    0x40


/**
 * List of default parameters, used during initialization of
 * the target_data
 */
#define ISCSI_DEFAULT_TARGET_NAME                  "iqn.2004-01.com:0"
#define ISCSI_DEFAULT_MAX_CONNECTIONS              1
#define ISCSI_DEFAULT_INITIAL_R2T                  "Yes"
#define ISCSI_DEFAULT_HEADER_DIGEST                "None"
#define ISCSI_DEFAULT_DATA_DIGEST                  "None"
#define ISCSI_DEFAULT_IMMEDIATE_DATA               "Yes"
#define ISCSI_DEFAULT_MAX_RECV_DATA_SEGMENT_LENGTH 8192
#define ISCSI_DEFAULT_FIRST_BURST_LENGTH           65536
#define ISCSI_DEFAULT_MAX_BURST_LENGTH             262144
#define ISCSI_DEFAULT_ISCSI_DEFAULT_TIME2WAIT            2
#define ISCSI_DEFAULT_ISCSI_DEFAULT_TIME2RETAIN          20
#define ISCSI_DEFAULT_MAX_OUTSTANDING_R2T          1
#define ISCSI_DEFAULT_DATA_PDU_IN_ORDER            "Yes"
#define ISCSI_DEFAULT_DATA_SEQUENCE_IN_ORDER       "Yes"
#define ISCSI_DEFAULT_ERROR_RECOVERY_LEVEL         0
#define ISCSI_DEFAULT_SESSION_TYPE                 "Normal"
#define ISCSI_DEFAULT_CHAP                         "None"
#define ISCSI_DEFAULT_CHALLENGE_LENGTH             256
#define ISCSI_DEFAULT_INITIATOR_NAME \
    "iqn.1999-11.edu.unh.iol.iscsi-initiator"
#define ISCSI_DEFAULT_INITIATOR_ALIAS             "UNH"

/**
 * Flags:
 * the following assumption holds:
 * if parameter of the local Initiator structure
 * was untouched than it should not be synchronized
 * with the Initiator. Than the Initiator uses the
 * default parameter and MAY NOT offer the parameter
 * during the negotiations.
 */
#define OFFER_0                                 0
#define OFFER_MAX_CONNECTIONS                   (1 << 0)
#define OFFER_INITIAL_R2T                       (1 << 1)
#define OFFER_HEADER_DIGEST                     (1 << 2)
#define OFFER_DATA_DIGEST                       (1 << 3)
#define OFFER_IMMEDIATE_DATA                    (1 << 4)
#define OFFER_MAX_RECV_DATA_SEGMENT_LENGTH      (1 << 5)
#define OFFER_MAX_BURST_LENGTH                  (1 << 6)
#define OFFER_FIRST_BURST_LENGTH                (1 << 7)
#define OFFER_DEFAULT_TIME2WAIT                 (1 << 8)
#define OFFER_DEFAULT_TIME2RETAIN               (1 << 9)
#define OFFER_MAX_OUTSTANDING_R2T               (1 << 10)
#define OFFER_DATA_PDU_IN_ORDER                 (1 << 11)
#define OFFER_DATA_SEQUENCE_IN_ORDER            (1 << 12)
#define OFFER_ERROR_RECOVERY_LEVEL              (1 << 13)


/**
 * Connection status (up, down or non-existant.
 * Note: it is essential that ISCSI_CONNECTION_DOWN == 0
 * and ISCSI_CONNECTION_UP == 1, or things will break.
 */
typedef enum iscsi_connection_status
{
    ISCSI_CONNECTION_REMOVED = -1, /**< Connection is removed */
    ISCSI_CONNECTION_DOWN,         /**< Connection is down */
    ISCSI_CONNECTION_ESTABLISHING, /**< Connection is being established */
    ISCSI_CONNECTION_WAITING_DEVICE, /**< Connection is up but
                                      *  a corresponding SCSI device is not
                                      *  yet ready */
    ISCSI_CONNECTION_UP,           /**< Connection is up and ready */
    ISCSI_CONNECTION_CLOSING,      /**< Connection is being closed */
    ISCSI_CONNECTION_ABNORMAL,     /**< Connection is in abnormal state */
    ISCSI_CONNECTION_RECOVER_DOWN, /**< Connection needs cleanup, then down */
    ISCSI_CONNECTION_RECOVER_UP,   /**< Connection needs cleanup, then up */
    ISCSI_CONNECTION_DISCOVERING   /**< Doing discovery session */
} iscsi_connection_status;

/**
 * If the parameters is session wide, than this value should be passed
 * to the set parameter function as a cid
 */
#define ISCSI_ALL_CONNECTIONS -1

/**
 * SCSI block size for iSCSI
 */
#define    ISCSI_SCSI_BLOCKSIZE 512

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* !__TE_ISCSI_H__ */
/**@} <!-- END te_tools_te_iscsi --> */
