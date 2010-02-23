/** @file 
 * @brief CWMP data common methods
 * 
 * CWMP data exchange common methods, useful for transfer CWMP message
 * structures, declared in acse_soapStub.h, between processes with 
 * different address spaces.
 *
 *
 * Copyright (C) 2010 Test Environment authors (see file AUTHORS
 * in the root directory of the distribution). 
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version. 
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details. 
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *
 * @author Konstantin Abramenko <Konstantin.Abramenko@oktetlabs.ru>
 *
 * $Id: $
*/

#ifndef __TE_CWMP_DATA__H__
#define __TE_CWMP_DATA__H__

#include "te_errno.h"
#include "acse_soapStub.h"

#ifdef __cplusplus
extern "C" {
#endif


extern te_errno te_cwmp_pack__FaultStruct(const cwmp__FaultStruct *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__DeviceIdStruct(const cwmp__DeviceIdStruct *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__EventStruct(const cwmp__EventStruct *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__ParameterValueStruct(const cwmp__ParameterValueStruct *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__ParameterInfoStruct(const cwmp__ParameterInfoStruct *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__SetParameterAttributesStruct(const cwmp__SetParameterAttributesStruct *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__ParameterAttributeStruct(const cwmp__ParameterAttributeStruct *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__QueuedTransferStruct(const cwmp__QueuedTransferStruct *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__AllQueuedTransferStruct(const cwmp__AllQueuedTransferStruct *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__OptionStruct(const cwmp__OptionStruct *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__ArgStruct(const cwmp__ArgStruct *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__Fault(const _cwmp__Fault *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__GetRPCMethods(const _cwmp__GetRPCMethods *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__GetRPCMethodsResponse(const _cwmp__GetRPCMethodsResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__SetParameterValues(const _cwmp__SetParameterValues *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__SetParameterValuesResponse(const _cwmp__SetParameterValuesResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__GetParameterValues(const _cwmp__GetParameterValues *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__GetParameterValuesResponse(const _cwmp__GetParameterValuesResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__GetParameterNames(const _cwmp__GetParameterNames *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__GetParameterNamesResponse(const _cwmp__GetParameterNamesResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__SetParameterAttributes(const _cwmp__SetParameterAttributes *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__SetParameterAttributesResponse(const _cwmp__SetParameterAttributesResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__GetParameterAttributes(const _cwmp__GetParameterAttributes *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__GetParameterAttributesResponse(const _cwmp__GetParameterAttributesResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__AddObject(const _cwmp__AddObject *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__AddObjectResponse(const _cwmp__AddObjectResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__DeleteObject(const _cwmp__DeleteObject *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__DeleteObjectResponse(const _cwmp__DeleteObjectResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__Download(const _cwmp__Download *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__DownloadResponse(const _cwmp__DownloadResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__Reboot(const _cwmp__Reboot *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__RebootResponse(const _cwmp__RebootResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__GetQueuedTransfers(const _cwmp__GetQueuedTransfers *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__GetQueuedTransfersResponse(const _cwmp__GetQueuedTransfersResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__ScheduleInform(const _cwmp__ScheduleInform *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__ScheduleInformResponse(const _cwmp__ScheduleInformResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__SetVouchers(const _cwmp__SetVouchers *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__SetVouchersResponse(const _cwmp__SetVouchersResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__GetOptions(const _cwmp__GetOptions *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__GetOptionsResponse(const _cwmp__GetOptionsResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__Upload(const _cwmp__Upload *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__UploadResponse(const _cwmp__UploadResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__FactoryReset(const _cwmp__FactoryReset *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__FactoryResetResponse(const _cwmp__FactoryResetResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__GetAllQueuedTransfers(const _cwmp__GetAllQueuedTransfers *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__GetAllQueuedTransfersResponse(const _cwmp__GetAllQueuedTransfersResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__Inform(const _cwmp__Inform *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__InformResponse(const _cwmp__InformResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__TransferComplete(const _cwmp__TransferComplete *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__TransferCompleteResponse(const _cwmp__TransferCompleteResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__AutonomousTransferComplete(const _cwmp__AutonomousTransferComplete *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__AutonomousTransferCompleteResponse(const _cwmp__AutonomousTransferCompleteResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__Kicked(const _cwmp__Kicked *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__KickedResponse(const _cwmp__KickedResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__RequestDownload(const _cwmp__RequestDownload *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__RequestDownloadResponse(const _cwmp__RequestDownloadResponse *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__ID(const _cwmp__ID *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__HoldRequests(const _cwmp__HoldRequests *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__MethodList(const MethodList *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__EventList(const EventList *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__ParameterValueList(const ParameterValueList *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__ParameterInfoList(const ParameterInfoList *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__ParameterNames(const ParameterNames *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__AccessList(const AccessList *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__SetParameterAttributesList(const SetParameterAttributesList *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__ParameterAttributeList(const ParameterAttributeList *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__TransferList(const TransferList *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__AllTransferList(const AllTransferList *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__VoucherList(const VoucherList *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__OptionList(const OptionList *src, void *msg, size_t len);
extern te_errno te_cwmp_pack__FileTypeArg(const FileTypeArg *src, void *msg, size_t len);


extern cwmp__FaultStruct * te_cwmp_unpack__FaultStruct(void *msg, size_t len);
extern cwmp__DeviceIdStruct * te_cwmp_unpack__DeviceIdStruct(void *msg, size_t len);
extern cwmp__EventStruct * te_cwmp_unpack__EventStruct(void *msg, size_t len);
extern cwmp__ParameterValueStruct * te_cwmp_unpack__ParameterValueStruct(void *msg, size_t len);
extern cwmp__ParameterInfoStruct * te_cwmp_unpack__ParameterInfoStruct(void *msg, size_t len);
extern cwmp__SetParameterAttributesStruct * te_cwmp_unpack__SetParameterAttributesStruct(void *msg, size_t len);
extern cwmp__ParameterAttributeStruct * te_cwmp_unpack__ParameterAttributeStruct(void *msg, size_t len);
extern cwmp__QueuedTransferStruct * te_cwmp_unpack__QueuedTransferStruct(void *msg, size_t len);
extern cwmp__AllQueuedTransferStruct * te_cwmp_unpack__AllQueuedTransferStruct(void *msg, size_t len);
extern cwmp__OptionStruct * te_cwmp_unpack__OptionStruct(void *msg, size_t len);
extern cwmp__ArgStruct * te_cwmp_unpack__ArgStruct(void *msg, size_t len);
extern _cwmp__Fault * te_cwmp_unpack__Fault(void *msg, size_t len);
extern _cwmp__GetRPCMethods * te_cwmp_unpack__GetRPCMethods(void *msg, size_t len);
extern _cwmp__GetRPCMethodsResponse * te_cwmp_unpack__GetRPCMethodsResponse(void *msg, size_t len);
extern _cwmp__SetParameterValues * te_cwmp_unpack__SetParameterValues(void *msg, size_t len);
extern _cwmp__SetParameterValuesResponse * te_cwmp_unpack__SetParameterValuesResponse(void *msg, size_t len);
extern _cwmp__GetParameterValues * te_cwmp_unpack__GetParameterValues(void *msg, size_t len);
extern _cwmp__GetParameterValuesResponse * te_cwmp_unpack__GetParameterValuesResponse(void *msg, size_t len);
extern _cwmp__GetParameterNames * te_cwmp_unpack__GetParameterNames(void *msg, size_t len);
extern _cwmp__GetParameterNamesResponse * te_cwmp_unpack__GetParameterNamesResponse(void *msg, size_t len);
extern _cwmp__SetParameterAttributes * te_cwmp_unpack__SetParameterAttributes(void *msg, size_t len);
extern _cwmp__SetParameterAttributesResponse * te_cwmp_unpack__SetParameterAttributesResponse(void *msg, size_t len);
extern _cwmp__GetParameterAttributes * te_cwmp_unpack__GetParameterAttributes(void *msg, size_t len);
extern _cwmp__GetParameterAttributesResponse * te_cwmp_unpack__GetParameterAttributesResponse(void *msg, size_t len);
extern _cwmp__AddObject * te_cwmp_unpack__AddObject(void *msg, size_t len);
extern _cwmp__AddObjectResponse * te_cwmp_unpack__AddObjectResponse(void *msg, size_t len);
extern _cwmp__DeleteObject * te_cwmp_unpack__DeleteObject(void *msg, size_t len);
extern _cwmp__DeleteObjectResponse * te_cwmp_unpack__DeleteObjectResponse(void *msg, size_t len);
extern _cwmp__Download * te_cwmp_unpack__Download(void *msg, size_t len);
extern _cwmp__DownloadResponse * te_cwmp_unpack__DownloadResponse(void *msg, size_t len);
extern _cwmp__Reboot * te_cwmp_unpack__Reboot(void *msg, size_t len);
extern _cwmp__RebootResponse * te_cwmp_unpack__RebootResponse(void *msg, size_t len);
extern _cwmp__GetQueuedTransfers * te_cwmp_unpack__GetQueuedTransfers(void *msg, size_t len);
extern _cwmp__GetQueuedTransfersResponse * te_cwmp_unpack__GetQueuedTransfersResponse(void *msg, size_t len);
extern _cwmp__ScheduleInform * te_cwmp_unpack__ScheduleInform(void *msg, size_t len);
extern _cwmp__ScheduleInformResponse * te_cwmp_unpack__ScheduleInformResponse(void *msg, size_t len);
extern _cwmp__SetVouchers * te_cwmp_unpack__SetVouchers(void *msg, size_t len);
extern _cwmp__SetVouchersResponse * te_cwmp_unpack__SetVouchersResponse(void *msg, size_t len);
extern _cwmp__GetOptions * te_cwmp_unpack__GetOptions(void *msg, size_t len);
extern _cwmp__GetOptionsResponse * te_cwmp_unpack__GetOptionsResponse(void *msg, size_t len);
extern _cwmp__Upload * te_cwmp_unpack__Upload(void *msg, size_t len);
extern _cwmp__UploadResponse * te_cwmp_unpack__UploadResponse(void *msg, size_t len);
extern _cwmp__FactoryReset * te_cwmp_unpack__FactoryReset(void *msg, size_t len);
extern _cwmp__FactoryResetResponse * te_cwmp_unpack__FactoryResetResponse(void *msg, size_t len);
extern _cwmp__GetAllQueuedTransfers * te_cwmp_unpack__GetAllQueuedTransfers(void *msg, size_t len);
extern _cwmp__GetAllQueuedTransfersResponse * te_cwmp_unpack__GetAllQueuedTransfersResponse(void *msg, size_t len);
extern _cwmp__Inform * te_cwmp_unpack__Inform(void *msg, size_t len);
extern _cwmp__InformResponse * te_cwmp_unpack__InformResponse(void *msg, size_t len);
extern _cwmp__TransferComplete * te_cwmp_unpack__TransferComplete(void *msg, size_t len);
extern _cwmp__TransferCompleteResponse * te_cwmp_unpack__TransferCompleteResponse(void *msg, size_t len);
extern _cwmp__AutonomousTransferComplete * te_cwmp_unpack__AutonomousTransferComplete(void *msg, size_t len);
extern _cwmp__AutonomousTransferCompleteResponse * te_cwmp_unpack__AutonomousTransferCompleteResponse(void *msg, size_t len);
extern _cwmp__Kicked * te_cwmp_unpack__Kicked(void *msg, size_t len);
extern _cwmp__KickedResponse * te_cwmp_unpack__KickedResponse(void *msg, size_t len);
extern _cwmp__RequestDownload * te_cwmp_unpack__RequestDownload(void *msg, size_t len);
extern _cwmp__RequestDownloadResponse * te_cwmp_unpack__RequestDownloadResponse(void *msg, size_t len);
extern _cwmp__ID * te_cwmp_unpack__ID(void *msg, size_t len);
extern _cwmp__HoldRequests * te_cwmp_unpack__HoldRequests(void *msg, size_t len);
extern MethodList * te_cwmp_unpack__MethodList(void *msg, size_t len);
extern EventList * te_cwmp_unpack__EventList(void *msg, size_t len);
extern ParameterValueList * te_cwmp_unpack__ParameterValueList(void *msg, size_t len);
extern ParameterInfoList * te_cwmp_unpack__ParameterInfoList(void *msg, size_t len);
extern ParameterNames * te_cwmp_unpack__ParameterNames(void *msg, size_t len);
extern AccessList * te_cwmp_unpack__AccessList(void *msg, size_t len);
extern SetParameterAttributesList * te_cwmp_unpack__SetParameterAttributesList(void *msg, size_t len);
extern ParameterAttributeList * te_cwmp_unpack__ParameterAttributeList(void *msg, size_t len);
extern TransferList * te_cwmp_unpack__TransferList(void *msg, size_t len);
extern AllTransferList * te_cwmp_unpack__AllTransferList(void *msg, size_t len);
extern VoucherList * te_cwmp_unpack__VoucherList(void *msg, size_t len);
extern OptionList * te_cwmp_unpack__OptionList(void *msg, size_t len);
extern FileTypeArg * te_cwmp_unpack__FileTypeArg(void *msg, size_t len);

#ifdef __cplusplus
}
#endif
#endif /* __TE_CWMP_DATA__H__ */
