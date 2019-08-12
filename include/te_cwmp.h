/** @file
 * @brief Test Environment CWMP support
 *
 * Common definitions for CPE WAN Management Protocol (TR-069).
 *
 *
 * Copyright (C) 2003-2018 OKTET Labs. All rights reserved.
 *
 * 
 *
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#ifndef __TE_CWMP_H__
#define __TE_CWMP_H__

#include "te_stdint.h"

/* gSOAP generated structs */
#include "cwmp_soapStub.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Primitive SOAP type constants, copied here from acse_coapH.h
 * for convenient compile.
 */
#ifndef SOAP_TYPE_int
#define SOAP_TYPE_int (1)
#endif

#ifndef SOAP_TYPE_byte
#define SOAP_TYPE_byte (2)
#endif

#ifndef SOAP_TYPE_string
#define SOAP_TYPE_string (3)
#endif

#ifndef SOAP_TYPE_SOAP_ENC__base64
#define SOAP_TYPE_SOAP_ENC__base64 (6)
#endif

#ifndef SOAP_TYPE_unsignedInt
#define SOAP_TYPE_unsignedInt (7)
#endif

#ifndef SOAP_TYPE_unsignedByte
#define SOAP_TYPE_unsignedByte (8)
#endif

#ifndef SOAP_TYPE_xsd__boolean
#define SOAP_TYPE_xsd__boolean (12)
#endif

#ifndef SOAP_TYPE_boolean
#define SOAP_TYPE_boolean (12)
#endif

#ifndef SOAP_TYPE_time
#define SOAP_TYPE_time (99)
#endif


#define NULL_FROM_CPE ((cwmp_data_from_cpe_t)NULL)
#define PTR_FROM_CPE(ptr_) ((cwmp_data_from_cpe_t *) &ptr_)

#define NULL_TO_CPE ((cwmp_data_to_cpe_t)NULL)
#define PTR_TO_CPE(ptr_) ((cwmp_data_to_cpe_t) ptr_)


/*
 * Typedef synonyms independent from gSOAP tool.
 */

typedef struct  cwmp__FaultStruct  cwmp_fault_struct_t;
typedef struct  cwmp__DeviceIdStruct  cwmp_device_id_struct_t;
typedef struct  cwmp__EventStruct  cwmp_event_struct_t;
typedef struct  cwmp__ParameterValueStruct
                    cwmp_parameter_value_struct_t;
typedef struct  cwmp__ParameterInfoStruct
                    cwmp_parameter_info_struct_t;
typedef struct  cwmp__SetParameterAttributesStruct
                    cwmp_set_parameter_attributes_struct_t;
typedef struct  cwmp__ParameterAttributeStruct
                    cwmp_parameter_attribute_struct_t;
typedef struct  cwmp__QueuedTransferStruct
                    cwmp_queued_transfer_struct_t;
typedef struct  cwmp__AllQueuedTransferStruct
                    cwmp_all_queued_transfer_struct_t;
typedef struct  cwmp__OptionStruct  cwmp_option_struct_t;
typedef struct  cwmp__ArgStruct  cwmp_arg_struct_t;
typedef struct  _cwmp__Fault  cwmp_fault_t;
typedef struct  _cwmp__GetRPCMethods  cwmp_get_rpc_methods_t;
typedef struct  _cwmp__GetRPCMethodsResponse
                    cwmp_get_rpc_methods_response_t;
typedef struct  _cwmp__SetParameterValues
                    cwmp_set_parameter_values_t;
typedef struct  _cwmp__SetParameterValuesResponse
                    cwmp_set_parameter_values_response_t;
typedef struct  _cwmp__GetParameterValues
                    cwmp_get_parameter_values_t;
typedef struct  _cwmp__GetParameterValuesResponse
                    cwmp_get_parameter_values_response_t;
typedef struct  _cwmp__GetParameterNames
                    cwmp_get_parameter_names_t;
typedef struct  _cwmp__GetParameterNamesResponse
                    cwmp_get_parameter_names_response_t;
typedef struct  _cwmp__SetParameterAttributes
                    cwmp_set_parameter_attributes_t;
typedef struct  _cwmp__SetParameterAttributesResponse
                    cwmp_set_parameter_attributes_response_t;
typedef struct  _cwmp__GetParameterAttributes
                    cwmp_get_parameter_attributes_t;
typedef struct  _cwmp__GetParameterAttributesResponse
                    cwmp_get_parameter_attributes_response_t;
typedef struct  _cwmp__AddObject  cwmp_add_object_t;
typedef struct  _cwmp__AddObjectResponse  cwmp_add_object_response_t;
typedef struct  _cwmp__DeleteObject  cwmp_delete_object_t;
typedef struct  _cwmp__DeleteObjectResponse  cwmp_delete_object_response_t;
typedef struct  _cwmp__Download  cwmp_download_t;
typedef struct  _cwmp__DownloadResponse  cwmp_download_response_t;
typedef struct  _cwmp__Reboot  cwmp_reboot_t;
typedef struct  _cwmp__RebootResponse  cwmp_reboot_response_t;
typedef struct  _cwmp__GetQueuedTransfers  cwmp_get_queued_transfers_t;
typedef struct  _cwmp__GetQueuedTransfersResponse
                    cwmp_get_queued_transfers_response_t;
typedef struct  _cwmp__ScheduleInform  cwmp_schedule_inform_t;
typedef struct  _cwmp__ScheduleInformResponse
                    cwmp_schedule_inform_response_t;
typedef struct  _cwmp__SetVouchers  cwmp_set_vouchers_t;
typedef struct  _cwmp__SetVouchersResponse
                    cwmp_set_vouchers_response_t;
typedef struct  _cwmp__GetOptions  cwmp_get_options_t;
typedef struct  _cwmp__GetOptionsResponse
                    cwmp_get_options_response_t;
typedef struct  _cwmp__Upload  cwmp_upload_t;
typedef struct  _cwmp__UploadResponse  cwmp_upload_response_t;
typedef struct  _cwmp__FactoryReset  cwmp_factory_reset_t;
typedef struct  _cwmp__FactoryResetResponse
                    cwmp_factory_reset_response_t;
typedef struct  _cwmp__GetAllQueuedTransfers
                    cwmp_get_all_queued_transfers_t;
typedef struct  _cwmp__GetAllQueuedTransfersResponse
                    cwmp_get_all_queued_transfers_response_t;
typedef struct  _cwmp__Inform  cwmp_inform_t;
typedef struct  _cwmp__InformResponse  cwmp_inform_response_t;
typedef struct  _cwmp__TransferComplete  cwmp_transfer_complete_t;
typedef struct  _cwmp__TransferCompleteResponse
                    cwmp_transfer_complete_response_t;
typedef struct  _cwmp__AutonomousTransferComplete
                    cwmp_autonomous_transfer_complete_t;
typedef struct  _cwmp__AutonomousTransferCompleteResponse
                    cwmp_autonomous_transfer_complete_response_t;
typedef struct  _cwmp__Kicked  cwmp_kicked_t;
typedef struct  _cwmp__KickedResponse  cwmp_kicked_response_t;
typedef struct  _cwmp__RequestDownload  cwmp_request_download_t;
typedef struct  _cwmp__RequestDownloadResponse
                    cwmp_request_download_response_t;
typedef struct  MethodList  cwmp_method_list_t;
typedef struct  EventList  cwmp_event_list_t;
typedef struct  ParameterValueList  cwmp_parameter_value_list_t;
typedef struct  ParameterInfoList  cwmp_parameter_info_list_t;
typedef struct  ParameterNames  cwmp_parameter_names_t;
typedef struct  AccessList  cwmp_access_list_t;
typedef struct  SetParameterAttributesList
                    cwmp_set_parameter_attributes_list_t;
typedef struct  ParameterAttributeList  cwmp_parameter_attribute_list_t;
typedef struct  TransferList  cwmp_transfer_list_t;
typedef struct  AllTransferList  cwmp_all_transfer_list_t;
typedef struct  VoucherList  cwmp_voucher_list_t;
typedef struct  OptionList  cwmp_option_list_t;
typedef struct  FileTypeArg  cwmp_file_type_arg_t;
typedef struct  _cwmp__ID  cwmp_id_t;
typedef struct  _cwmp__HoldRequests  cwmp_hold_requests_t;




/**
 * This enumeration contains types of RPC calls, which
 * could be called on CPE via CWMP, according with
 * Table 5 in [TR-069].
 */
typedef enum {
    CWMP_RPC_NONE = 0,
    CWMP_RPC_get_rpc_methods,
    CWMP_RPC_set_parameter_values,
    CWMP_RPC_get_parameter_values,
    CWMP_RPC_get_parameter_names,
    CWMP_RPC_set_parameter_attributes,
    CWMP_RPC_get_parameter_attributes,
    CWMP_RPC_add_object,
    CWMP_RPC_delete_object,
    CWMP_RPC_reboot,
    CWMP_RPC_download,
    CWMP_RPC_upload,
    CWMP_RPC_factory_reset,
    CWMP_RPC_get_queued_transfers,
    CWMP_RPC_get_all_queued_transfers,
    CWMP_RPC_schedule_inform,
    CWMP_RPC_set_vouchers,
    CWMP_RPC_get_options,
    CWMP_RPC_FAULT,
} te_cwmp_rpc_cpe_t;

/**
 * This enumeration contains types of RPC calls, which
 * could be called on ACS via CWMP, according with
 * Table 5 in [TR-069].
 */
typedef enum {
    CWMP_RPC_ACS_NONE = 0,
    CWMP_RPC_ACS_get_rpc_methods,
    CWMP_RPC_inform,
    CWMP_RPC_transfer_complete,
    CWMP_RPC_autonomous_transfer_complete,
    CWMP_RPC_request_download,
    CWMP_RPC_kicked,
    CWMP_RPC_ACS_FAULT,
} te_cwmp_rpc_acs_t;

/**
 * This enumeration contains types of Events, which
 * may occur in Inform from CPE to ACS, according with
 * Table 7 in [TR-069].
 */
typedef enum {
    CWMP_EV_0_BOOTSTRAP,
    CWMP_EV_1_BOOT,
    CWMP_EV_2_PERIODIC,
    CWMP_EV_3_SCHEDULED,
    CWMP_EV_4_VALUE_CHANGED,
    CWMP_EV_5_KICKED,
    CWMP_EV_6_CONNECTION_REQUEST,
    CWMP_EV_7_TRANSFER_COMPLETE,
    CWMP_EV_8_DIAGNOSTICS_COMPLETE,
    CWMP_EV_9_REQUEST_DOWNLOAD,
    CWMP_EV_10_AUTONOMOUS_TRANSFER_COMPLETE,
    CWMP_EV_M_Reboot,
    CWMP_EV_M_ScheduleInform,
    CWMP_EV_M_Download,
    CWMP_EV_M_Upload
} te_cwmp_event_code_t;




/** State of Connection Request to CPE. */
typedef enum acse_cr_state_t {
    CR_NONE = 1,       /**< No Conn.Req operation was inited */
    CR_WAIT_AUTH = 2,  /**< Connection Request started, but waiting
                            for successful authenticate. */
    CR_DONE = 4,       /**< Connection Request was sent and gets
                            successful HTTP response.
                            Swith back to CR_NONE after receive Inform
                            with EventCode  = @c CONNECTION REQUEST*/
    CR_ERROR = 8,      /**< Connection Request was sent and gets
                            HTTP error.
                            Switch back to CR_NONE after read
                            Conn.Req. status by EPC */
} acse_cr_state_t;

/** CWMP Session states */
typedef enum {
    CWMP_NOP = 1,      /**< No any TCP activity: neither active
                            connection, nor listening for incoming ones.*/
    CWMP_LISTEN = 2,   /**< Listening for incoming HTTP connection.  */
    CWMP_WAIT_AUTH = 4,/**< TCP connection established, first HTTP
                            request received, but not authenicated,
                            response with our WWW-Authenticate is sent.*/
    CWMP_SERVE = 8,    /**< CWMP session established, waiting for
                            incoming SOAP RPC requests from CPE.      */
    CWMP_WAIT_RESPONSE = 16,/**< CWMP session established, SOAP RPC
                            is sent to the CPE, waiting for response. */
    CWMP_PENDING = 32, /**< CWMP session established, waiting for
                            RPC to be sent on CPE, from EPC.          */
    CWMP_SEND_FILE = 64,/**< HTTP connection, sending reponse to GET,
                             received for particular ACS. */
    CWMP_CLOSE = 128,   /**< Session should be closed. */
    CWMP_SUSPENDED = 256,/**<CWMP session established, but TCP connection
                             was terminated, waiting for new one. */
} cwmp_sess_state_t;

/**< Typed pointer to call-specific CWMP data from ACS to CPE. */
typedef union {
        int http_code;
        void *p;
        cwmp_set_parameter_values_t      *set_parameter_values;
        cwmp_get_parameter_values_t      *get_parameter_values;
        cwmp_get_parameter_names_t       *get_parameter_names;
        cwmp_set_parameter_attributes_t  *set_parameter_attributes;
        cwmp_get_parameter_attributes_t  *get_parameter_attributes;
        cwmp_add_object_t                *add_object;
        cwmp_delete_object_t             *delete_object;
        cwmp_reboot_t                    *reboot;
        cwmp_download_t                  *download;
        cwmp_upload_t                    *upload;
        cwmp_schedule_inform_t           *schedule_inform;
        cwmp_set_vouchers_t              *set_vouchers;
        cwmp_get_options_t               *get_options;
        cwmp_fault_t                     *fault;
    } cwmp_data_to_cpe_t;

/**< Typed pointer to call-specific CWMP data from CPE to ACS. */
typedef union {
        acse_cr_state_t      cr_state;

        void *p;
        cwmp_inform_t                        *inform;
        cwmp_fault_t                         *fault;
        cwmp_transfer_complete_t             *transfer_complete;
        cwmp_autonomous_transfer_complete_t  *aut_transfer_complete;
        cwmp_kicked_t                        *kicked;
        cwmp_request_download_t              *request_download;
        cwmp_get_rpc_methods_response_t      *get_rpc_methods_r;
        cwmp_set_parameter_values_response_t *set_parameter_values_r;
        cwmp_get_parameter_values_response_t *get_parameter_values_r;
        cwmp_get_parameter_names_response_t  *get_parameter_names_r;
        cwmp_get_parameter_attributes_response_t
                                             *get_parameter_attributes_r;
        cwmp_add_object_response_t           *add_object_r;
        cwmp_delete_object_response_t        *delete_object_r;
        cwmp_download_response_t             *download_r;
        cwmp_upload_response_t               *upload_r;
        cwmp_get_queued_transfers_response_t *get_queued_transfers_r;
        cwmp_get_all_queued_transfers_response_t
                                             *get_all_queued_transfers_r;
        cwmp_get_options_response_t          *get_options_r;
    } cwmp_data_from_cpe_t;

/**
 * Type for ACSE CWMP request identifier in queue.
 */
typedef uint32_t acse_request_id_t;

/**
 * Notification Attrubute values
 */
typedef enum {
    CWMP_NOTIF_OFF = 0,
    CWMP_NOTIF_PASSIVE = 1,
    CWMP_NOTIF_ACTIVE = 2
} cwmp_notification_t;

/**
 * Data Model for TR-069
 */
typedef enum {
    TE_CWMP_DM_TR098,   /**< Internet Gateway Device Data Model */
    TE_CWMP_DM_TR181,   /**< Device Data Model */
} te_cwmp_datamodel_t;

/** Default timeout for CWMP in seconds */
enum {
    CWMP_TIMEOUT = 30,
};

/**
 * Convert internal value of CWMP CPE RPC type to string, for printing.
 */
static inline const char *
cwmp_rpc_cpe_string(te_cwmp_rpc_cpe_t r)
{
    switch (r)
    {
        case CWMP_RPC_NONE:             return "NONE";
        case CWMP_RPC_get_rpc_methods:  return "GetRPCMethods";
        case CWMP_RPC_set_parameter_values: return "SetParameterValues";
        case CWMP_RPC_get_parameter_values: return "GetParameterValues";
        case CWMP_RPC_get_parameter_names:  return "GetParameterNames";
        case CWMP_RPC_set_parameter_attributes:
                                        return "SetParameterAttributes";
        case CWMP_RPC_get_parameter_attributes:
                                        return "GetParameterAttributes";
        case CWMP_RPC_add_object:       return "AddObject";
        case CWMP_RPC_delete_object:    return "DeleteObject";
        case CWMP_RPC_reboot:           return "Reboot";
        case CWMP_RPC_download:         return "Download";
        case CWMP_RPC_upload:           return "Upload";
        case CWMP_RPC_factory_reset:    return "FactoryReset";
        case CWMP_RPC_get_queued_transfers:
                                        return "GetQueuedTransfers";
        case CWMP_RPC_get_all_queued_transfers:
                                        return "GetAllQueuedTransfers";
        case CWMP_RPC_schedule_inform:  return "ScheduleInform";
        case CWMP_RPC_set_vouchers:     return "SetVouchers";
        case CWMP_RPC_get_options:      return "GetOptions";
        case CWMP_RPC_FAULT:            return "Fault";
    }
    return "unknown";
}

/**
 * Convert internal value of CWMP ACS RPC type to string, for printing.
 */
static inline const char *
cwmp_rpc_acs_string(te_cwmp_rpc_acs_t r)
{
    switch (r)
    {
        case CWMP_RPC_ACS_NONE:         return "NONE";
        case CWMP_RPC_ACS_get_rpc_methods: return "GetRPCMethods";
        case CWMP_RPC_inform:           return "Inform";
        case CWMP_RPC_transfer_complete:return "TransferComplete";
        case CWMP_RPC_autonomous_transfer_complete:
                                        return "AutonomousTransferComplete";
        case CWMP_RPC_request_download: return "RequestDownload";
        case CWMP_RPC_kicked:           return "Kicked";
        case CWMP_RPC_ACS_FAULT:        return "Fault";
    }
    return "unknown";
}

static inline cwmp_notification_t
cwmp_notification_parse(const char *label)
{
    if (label == NULL)
        return CWMP_NOTIF_OFF;
    if (strcasecmp(label, "passive") == 0)
        return CWMP_NOTIF_PASSIVE;
    if (strcasecmp(label, "active") == 0)
        return CWMP_NOTIF_ACTIVE;
    return CWMP_NOTIF_OFF;
}

static inline const char *
cwmp_notification_string(cwmp_notification_t n)
{
    switch (n)
    {
        case CWMP_NOTIF_OFF:     return "off";
        case CWMP_NOTIF_PASSIVE: return "passive";
        case CWMP_NOTIF_ACTIVE:  return "active";
    }
    return "unknown";
}
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*__TE_CWMP_H__ */
