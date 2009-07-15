/** @file
 * @brief Test API - RPC
 *
 * Definition of TAPI for remote calls of some standard input/output
 * functions and useful extensions.
 *
 *
 * Copyright (C) 2005 Test Environment authors (see file AUTHORS in
 * the root directory of the distribution).
 * Copyright (c) 2005 Level5 Networks Corp.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
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
 * @author Edward Makarov <Edward.Makarov@oktetlabs.ru>
 *
 * $Id: tapi_rpc_tr069.h 43076 2007-09-24 06:48:00Z kam $
 */

#ifndef __TE_TAPI_RPC_TR069_H__
#define __TE_TAPI_RPC_TR069_H__

#include <stdio.h>

#include "rcf_rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** GetRPCMethods */
typedef struct {
    char   (*method_list)[65];
    size_t   n;
} cpe_get_rpc_methods_response_t;

/** SetParameterValues */
typedef struct {
    struct {
        char name[257];
        int  value;
    }   *parameter_list;
    char parameter_key[33];
} cpe_set_parameter_values_t;

typedef struct {
    int status;
} cpe_set_parameter_values_response_t;

/** GetParameterValues */
typedef struct {
} cpe_get_parameter_values_t;

typedef struct {
} cpe_get_parameter_values_response_t;

/** GetParameterNames */
typedef struct {
} cpe_get_parameter_names_t;

typedef struct {
} cpe_get_parameter_names_response_t;

/** SetParameterAttributes */
typedef struct {
} cpe_set_parameter_attributes_t;

typedef struct {
} cpe_set_parameter_attributes_response_t;

/** GetParameterAttributes */
typedef struct {
} cpe_get_parameter_attributes_t;

typedef struct {
} cpe_get_parameter_attributes_response_t;

/** AddObject */
typedef struct {
} cpe_add_object_t;

typedef struct {
} cpe_add_object_response_t;

/** DeleteObject */
typedef struct {
} cpe_delete_object_t;

typedef struct {
} cpe_delete_object_response_t;

/** Reboot */
typedef struct {
} cpe_reboot_t;

typedef struct {
} cpe_reboot_response_t;

/** Download */
typedef struct {
} cpe_download_t;

typedef struct {
} cpe_download_response_t;

/** Upload */
typedef struct {
} cpe_upload_t;

typedef struct {
} cpe_upload_response_t;

/** FactoryReset */
typedef struct {
} cpe_factory_reset_t;

typedef struct {
} cpe_factory_reset_response_t;

/** GetQueuedTransfers */
typedef struct {
} cpe_get_queued_transfers_t;

typedef struct {
} cpe_get_queued_transfers_response_t;

/** GetAllQueuedTransfers */
typedef struct {
} cpe_get_all_queued_transfers_t;

typedef struct {
} cpe_get_all_queued_transfers_response_t;

/** ScheduleInform */
typedef struct {
} cpe_schedule_inform_t;

typedef struct {
} cpe_schedule_inform_response_t;

/** SetVouchers */
typedef struct {
} cpe_set_vouchers_t;

typedef struct {
} cpe_set_vouchers_response_t;

/** GetOptions */
typedef struct {
} cpe_get_options_t;

typedef struct {
} cpe_get_options_response_t;

/**
 * Call CPE GetRPCMethods method.
 *
 * @param rpcs     RPC server handle
 * @param resp     CPE response to the GetRPCMethods method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_get_rpc_methods(
               rcf_rpc_server *rpcs,
               cpe_get_rpc_methods_response_t *resp);

/**
 * Call CPE SetParameterValues method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the SetParameterValues method
 * @param resp     CPE response to the SetParameterValues method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_set_parameter_values(
               rcf_rpc_server *rpcs,
               cpe_set_parameter_values_t *req,
               cpe_set_parameter_values_response_t *resp);

/**
 * Call CPE GetParameterValues method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the GetParameterValues method
 * @param resp     CPE response to the GetParameterValues method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_get_parameter_values(
               rcf_rpc_server *rpcs,
               cpe_get_parameter_values_t *req,
               cpe_get_parameter_values_response_t *resp);

/**
 * Call CPE GetParameterNames method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the GetParameterNames method
 * @param resp     CPE response to the GetParameterNames method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_get_parameter_names(
               rcf_rpc_server *rpcs,
               cpe_get_parameter_names_t *req,
               cpe_get_parameter_names_response_t *resp);

/**
 * Call CPE SetParameterAttributes method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the SetParameterAttributes method
 * @param resp     CPE response to the SetParameterAttributes method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_set_parameter_attributes(
               rcf_rpc_server *rpcs,
               cpe_set_parameter_attributes_t *req,
               cpe_set_parameter_attributes_response_t *resp);

/**
 * Call CPE GetParameterAttributes method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the GetParameterAttributes method
 * @param resp     CPE response to the GetParameterAttributes method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_get_parameter_attributes(
               rcf_rpc_server *rpcs,
               cpe_get_parameter_attributes_t *req,
               cpe_get_parameter_attributes_response_t *resp);

/**
 * Call CPE AddObject method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the AddObject method
 * @param resp     CPE response to the AddObject method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_add_object(
               rcf_rpc_server *rpcs,
               cpe_add_object_t *req,
               cpe_add_object_response_t *resp);

/**
 * Call CPE DeleteObject method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the DeleteObject method
 * @param resp     CPE response to the DeleteObject method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_delete_object(
               rcf_rpc_server *rpcs,
               cpe_delete_object_t *req,
               cpe_delete_object_response_t *resp);

/**
 * Call CPE Reboot method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the Reboot method
 * @param resp     CPE response to the Reboot method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_reboot(
               rcf_rpc_server *rpcs,
               cpe_reboot_t *req,
               cpe_reboot_response_t *resp);

/**
 * Call CPE Download method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the Download method
 * @param resp     CPE response to the Download method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_download(
               rcf_rpc_server *rpcs,
               cpe_download_t *req,
               cpe_download_response_t *resp);

/**
 * Call CPE Upload method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the Upload method
 * @param resp     CPE response to the Upload method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_upload(
               rcf_rpc_server *rpcs,
               cpe_upload_t *req,
               cpe_upload_response_t *resp);

/**
 * Call CPE FactoryReset method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the FactoryReset method
 * @param resp     CPE response to the FactoryReset method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_factory_reset(
               rcf_rpc_server *rpcs,
               cpe_factory_reset_t *req,
               cpe_factory_reset_response_t *resp);

/**
 * Call CPE GetQueuedTransfers method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the GetQueuedTransfers method
 * @param resp     CPE response to the GetQueuedTransfers method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_get_queued_transfers(
               rcf_rpc_server *rpcs,
               cpe_get_queued_transfers_t *req,
               cpe_get_queued_transfers_response_t *resp);

/**
 * Call CPE GetAllQueuedTransfers method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the GetAllQueuedTransfers method
 * @param resp     CPE response to the GetAllQueuedTransfers method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_get_all_queued_transfers(
               rcf_rpc_server *rpcs,
               cpe_get_all_queued_transfers_t *req,
               cpe_get_all_queued_transfers_response_t *resp);

/**
 * Call CPE ScheduleInform method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the ScheduleInform method
 * @param resp     CPE response to the ScheduleInform method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_schedule_inform(
               rcf_rpc_server *rpcs,
               cpe_schedule_inform_t *req,
               cpe_schedule_inform_response_t *resp);

/**
 * Call CPE SetVouchers method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the SetVouchers method
 * @param resp     CPE response to the SetVouchers method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_set_vouchers(
               rcf_rpc_server *rpcs,
               cpe_set_vouchers_t *req,
               cpe_set_vouchers_response_t *resp);

/**
 * Call CPE GetOptions method.
 *
 * @param rpcs     RPC server handle
 * @param req      ACS request for the GetOptions method
 * @param resp     CPE response to the GetOptions method
 *
 * @return Upon successful completion this function returns 0.
 *         On a SOAP error a CPE fault code is returned (9000-9019).
 *         When transaction with CPE is impossible, -1 is returned.
 */
extern int rpc_cpe_get_options(
               rcf_rpc_server *rpcs,
               cpe_get_options_t *req,
               cpe_get_options_response_t *resp);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !__TE_TAPI_RPC_TR069_H__ */
