
/** @file
 * @brief CWMP data common methods implementation
 *
 * CWMP data exchange common methods, useful for transfer CWMP message 
 * structures, declared in cwmp_soapStub.h, between processes with 
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
 * $Id$
 */

#define TE_LGR_USER     "CWMP data utils"

#include "te_config.h"
#include "te_defs.h"
#include "logger_api.h"
#include "cwmp_data.h"
#include "acse_soapH.h"
#include "cwmp_utils.h"

#include <string.h>




/**
 * Method for syntax universal pack of string and accurate 
 * calculate length of memory, used by string, according with 
 * alignment.
 */
static inline ssize_t
te_cwmp_pack__string(char *src, void *msg, size_t max_len)
{
    char *dst = msg;
    size_t str_size;
    size_t alig_padding; 
    size_t packed_length;

    if (NULL == src)
        return 0;

    packed_length = str_size = strlen(src) + 1;

    if ((alig_padding = (str_size & 3)) != 0)
        packed_length += 4 - alig_padding; 

    if (packed_length > max_len)
        return -1;

    memcpy(dst, src, str_size);

    return packed_length;
}

/**
 * Method just for syntax universal unpack of string and accurate 
 * calculate length of memory, used by string, according with 
 * alignment.
 * No data copying is performed for UNpack. 
 */
static inline ssize_t
te_cwmp_unpack__string(void *msg, size_t max_len)
{
    char *str = msg;
    size_t str_size = strlen(str) + 1;
    size_t alig_padding = (str_size & 3); 

    if (alig_padding != 0)
        str_size += 4 - alig_padding; 

    if (str_size > max_len)
    {
        ERROR("unpack_string failed, str_size %u >= max_len %u",
              str_size, max_len);
        return -1;
    }

    return str_size;
}


static inline ssize_t
te_cwmp_pack__integer(int *src, void *msg, size_t max_len)
{
    UNUSED(max_len);
    if (NULL == src)
        return 0;

    *((int *)msg) = *src;

    return sizeof(int);
}

static inline ssize_t
te_cwmp_unpack__integer(void *msg, size_t max_len)
{
    UNUSED(msg);
    UNUSED(max_len);
    return sizeof(int);
}


static inline ssize_t
te_cwmp_pack__time(time_t *src, void *msg, size_t max_len)
{
    UNUSED(max_len);

    if (NULL == src)
        return 0;

    *((time_t *)msg) = *src;

    return sizeof(time_t);
}

static inline ssize_t
te_cwmp_unpack__time(void *msg, size_t max_len)
{
    UNUSED(msg);
    UNUSED(max_len);
    return sizeof(time_t);
}


/* 
 * Packing macros, assume the following local variables and 
 * function parameters: 
 * src, msg, max_len, dst, rc, packed_length. 
 *
 * Macro 'CWMP_PACK_COMMON_VARS' defines local variables, others 
 * should be function params. 
*/

#define CWMP_PACK_COMMON_VARS(_type) \
    size_t packed_length = 0; \
    ssize_t rc = 0; \
    _type *dst = (_type *)msg; 


/* Do not make any alignment */
#define CWMP_PACK_ROW(_item_length) \
    do { \
        if ((_item_length) > (max_len - packed_length)) \
            return -1; \
        memcpy((msg), (src), (_item_length)); \
        (packed_length) += (_item_length); \
        (msg) += (_item_length); \
    } while(0)

#define CWMP_PACK_LEAF(_f_type, _field) \
    do { \
        if (NULL == src -> _field) \
        { \
            dst-> _field = (void*)0; \
            break; \
        } \
        rc = te_cwmp_pack__ ## _f_type (src -> _field, msg, \
                                        (max_len) - (packed_length)); \
        if (rc < 0) \
            return -1; \
        if (rc > 0) \
            dst-> _field = (void*)((char *)(msg) - (char*)(dst)); \
        else \
            dst-> _field = (void*)0; \
        packed_length += rc; \
        msg += rc; \
    } while (0)

#if 0
/* Assume local variable 'dst', points to message start, with same type
   as pointer '_src'.
   Performs alignment (mod 4) of _msg and _packed_l after coping of string. 
*/
#define CWMP_PACK_STR(_src, _msg, _packed_l, _max_l, _field) \
    do { \
        size_t alig_padding; \
        size_t str_size = strlen( _src -> _field ) + 1; \
        dst-> _field = (void*)((char *)(_msg) - (char*)dst); \
        CWMP_PACK_ROW(src-> _field , msg, str_size, \
                  _packed_l, _max_l); \
        if ((alig_padding = (str_size & 3)) != 0) \
        { \
            (_msg) += 4 - alig_padding; \
            (_packed_l) += 4 - alig_padding; \
        } \
    } while (0)
#endif



ssize_t
te_cwmp_pack__DeviceIdStruct(const cwmp__DeviceIdStruct *src,
                             void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(cwmp__DeviceIdStruct);

    CWMP_PACK_ROW(sizeof(*src));
    
    CWMP_PACK_LEAF(string, Manufacturer);
    CWMP_PACK_LEAF(string, OUI);
    CWMP_PACK_LEAF(string, ProductClass); 
    CWMP_PACK_LEAF(string, SerialNumber);

    return packed_length;
}


ssize_t
te_cwmp_pack__Inform(const _cwmp__Inform *src, void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__Inform);

    CWMP_PACK_ROW(sizeof(_cwmp__Inform));

    CWMP_PACK_LEAF(DeviceIdStruct, DeviceId);
    CWMP_PACK_LEAF(EventList, Event);
    CWMP_PACK_LEAF(ParameterValueList, ParameterList);

    return packed_length;
}


ssize_t
te_cwmp_pack__EventStruct(const cwmp__EventStruct *src,
                          void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(cwmp__EventStruct);

    CWMP_PACK_ROW(sizeof(*src));

    CWMP_PACK_LEAF(string, EventCode);
    CWMP_PACK_LEAF(string, CommandKey);

    return packed_length;
}


#define CWMP_PACK_LIST_FUNC(_list_type, _elem_type) \
ssize_t \
te_cwmp_pack__ ## _list_type (const _list_type *src, \
                          void *msg, size_t max_len) \
{ \
    CWMP_PACK_COMMON_VARS(_list_type); \
    size_t array_memlen = 0; \
    int i = 0; \
 \
    CWMP_PACK_ROW(sizeof(*src)); \
     \
    /* For fill links to array elements here it is convenient \
       to have real pointer to the array start */ \
    dst->__ptr ## _elem_type = msg;  \
 \
    array_memlen = sizeof(void*) * src->__size; \
    msg += array_memlen; \
    packed_length += array_memlen; \
 \
    for (i = 0; i < src->__size; i++) \
    { \
        rc = te_cwmp_pack__ ## _elem_type(src->__ptr ## _elem_type[i],  \
                                       msg, max_len - packed_length); \
        if (rc < 0) \
            return -1; \
        dst->__ptr ## _elem_type[i] = (void*)((char *)msg - (char*)dst); \
        packed_length += rc; \
        msg += rc; \
    } \
    /* Put array offset instead of real pointer */ \
    dst->__ptr ## _elem_type = \
        (void*)((char *)dst->__ptr ## _elem_type - (char*)dst); \
 \
    return packed_length; \
}


CWMP_PACK_LIST_FUNC(EventList, EventStruct)


CWMP_PACK_LIST_FUNC(MethodList, string)

CWMP_PACK_LIST_FUNC(ParameterNames, string)

CWMP_PACK_LIST_FUNC(ParameterValueList, ParameterValueStruct)

ssize_t
te_cwmp_pack__ParameterValueStruct(const cwmp__ParameterValueStruct *src,
                                   void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(cwmp__ParameterValueStruct);

    CWMP_PACK_ROW(sizeof(*src));

    CWMP_PACK_LEAF(string, Name);
    switch (src->__type)
    {
        case SOAP_TYPE_string:
        case SOAP_TYPE_xsd__anySimpleType:
        case SOAP_TYPE_SOAP_ENC__base64:
            CWMP_PACK_LEAF(string, Value);
            break;
        case SOAP_TYPE_time:
            CWMP_PACK_LEAF(time, Value);
            break;
        default: /* integer, boolean types storen similar usual int. */
            CWMP_PACK_LEAF(integer, Value);
            break;
    }

    return packed_length;
}


ssize_t
te_cwmp_pack__GetRPCMethodsResponse(
        const _cwmp__GetRPCMethodsResponse *src,
        void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__GetRPCMethodsResponse);

    CWMP_PACK_ROW(sizeof(*src));

    CWMP_PACK_LEAF(MethodList, MethodList_);

    return packed_length;
}


ssize_t
te_cwmp_pack__SetParameterValues
    (const _cwmp__SetParameterValues *src,
     void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__SetParameterValues);

    CWMP_PACK_ROW(sizeof(_cwmp__SetParameterValues));

    CWMP_PACK_LEAF(ParameterValueList, ParameterList);
    CWMP_PACK_LEAF(string, ParameterKey);

    return packed_length;
}

ssize_t
te_cwmp_pack__SetParameterValuesResponse
    (const _cwmp__SetParameterValuesResponse *src,
     void *msg, size_t max_len)
{
    size_t packed_length = 0;

    CWMP_PACK_ROW(sizeof(*src));

    return packed_length;
}


ssize_t
te_cwmp_pack__GetParameterValues(const _cwmp__GetParameterValues *src,
                                 void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__GetParameterValues);

    CWMP_PACK_ROW(sizeof(*src));

    CWMP_PACK_LEAF(ParameterNames, ParameterNames_);

    return packed_length;
}

ssize_t
te_cwmp_pack__GetParameterValuesResponse
    (const _cwmp__GetParameterValuesResponse *src,
    void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__GetParameterValuesResponse);

    CWMP_PACK_ROW(sizeof(*src));

    CWMP_PACK_LEAF(ParameterValueList, ParameterList);

    return packed_length; 
}

ssize_t
te_cwmp_pack__GetParameterNames(const _cwmp__GetParameterNames *src,
                                void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__GetParameterNames);

    CWMP_PACK_ROW(sizeof(*src));

    if (NULL != src->ParameterPath)
    {
        dst->ParameterPath = msg;
        msg += sizeof(void*);
        packed_length += sizeof(void*);

        CWMP_PACK_LEAF(string, ParameterPath[0]);

        dst->ParameterPath = (void*)((char*)dst->ParameterPath -
                                     (char*)dst);
    }
    return packed_length;
}

ssize_t
te_cwmp_pack__GetParameterNamesResponse
            (const _cwmp__GetParameterNamesResponse *src,
            void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__GetParameterNamesResponse);

    CWMP_PACK_ROW(sizeof(*src));

    CWMP_PACK_LEAF(ParameterInfoList, ParameterList);

    return packed_length;
}

CWMP_PACK_LIST_FUNC(ParameterInfoList, ParameterInfoStruct)

ssize_t
te_cwmp_pack__ParameterInfoStruct(const cwmp__ParameterInfoStruct *src,
                                  void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(cwmp__ParameterInfoStruct);

    CWMP_PACK_ROW(sizeof(*src));

    CWMP_PACK_LEAF(string, Name);

    return packed_length;
}

ssize_t
te_cwmp_pack__Download(const _cwmp__Download *src,
                       void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__Download);

    CWMP_PACK_ROW(sizeof(*src));

    CWMP_PACK_LEAF(string, CommandKey);
    CWMP_PACK_LEAF(string, FileType);
    CWMP_PACK_LEAF(string, URL);
    CWMP_PACK_LEAF(string, Username);
    CWMP_PACK_LEAF(string, Password);
    CWMP_PACK_LEAF(string, TargetFileName);
    CWMP_PACK_LEAF(string, SuccessURL);
    CWMP_PACK_LEAF(string, FailureURL);

    return packed_length;
}

ssize_t
te_cwmp_pack__DownloadResponse(const _cwmp__DownloadResponse *src,
                               void *msg, size_t max_len)
{
    size_t packed_length = 0;

    CWMP_PACK_ROW(sizeof(*src));

    return packed_length;
}

ssize_t
te_cwmp_pack__Reboot(const _cwmp__Reboot *src, void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__Reboot);
    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(string, CommandKey);
    return packed_length;
}

ssize_t
te_cwmp_pack__FaultStruct(const cwmp__FaultStruct *src,
                          void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(cwmp__FaultStruct);
    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(string, FaultCode);
    CWMP_PACK_LEAF(string, FaultString);
    return packed_length;
}


ssize_t
te_cwmp_pack__TransferComplete(const _cwmp__TransferComplete *src,
                               void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__TransferComplete);
    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(string, CommandKey);
    CWMP_PACK_LEAF(FaultStruct, FaultStruct);
    return packed_length;
}

ssize_t
te_cwmp_pack__Fault(const _cwmp__Fault *src, void *msg, size_t max_len)
{
    size_t array_memlen = 0;
    int i = 0;
    CWMP_PACK_COMMON_VARS(_cwmp__Fault);
    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(string, FaultCode);
    CWMP_PACK_LEAF(string, FaultString);

    /* Here start some ugly hacks because for this type gSOAP generated
     * non-usual presentation of array. */

    dst->SetParameterValuesFault = msg;

    array_memlen = sizeof(struct _cwmp__Fault_SetParameterValuesFault) *
                    src->__sizeSetParameterValuesFault;
    msg += array_memlen;
    packed_length += array_memlen;

    for (i = 0; i < src->__sizeSetParameterValuesFault; i++) \
    {
        CWMP_PACK_LEAF(string, SetParameterValuesFault[i].ParameterName);
        CWMP_PACK_LEAF(string, SetParameterValuesFault[i].FaultCode);
        CWMP_PACK_LEAF(string, SetParameterValuesFault[i].FaultString);
    }
    /* Put array offset instead of real pointer */
    dst->SetParameterValuesFault = 
        (void*)((char *)dst->SetParameterValuesFault - (char*)dst);

    return packed_length;
}



ssize_t
te_cwmp_pack__AddObject(const _cwmp__AddObject *src, void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__AddObject);
    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(string, ObjectName);
    CWMP_PACK_LEAF(string, ParameterKey);
    return packed_length;
}

ssize_t
te_cwmp_pack__AddObjectResponse(const _cwmp__AddObjectResponse *src,
                                void *msg, size_t max_len)
{
    size_t packed_length = 0;
    CWMP_PACK_ROW(sizeof(*src)); 
    return packed_length;
}

ssize_t
te_cwmp_pack__DeleteObject(const _cwmp__DeleteObject *src,
                           void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__DeleteObject);
    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(string, ObjectName);
    CWMP_PACK_LEAF(string, ParameterKey);
    return packed_length;
}
ssize_t
te_cwmp_pack__DeleteObjectResponse(const _cwmp__DeleteObjectResponse *src,
                                   void *msg, size_t max_len)
{
    size_t packed_length = 0;
    CWMP_PACK_ROW(sizeof(*src)); 
    return packed_length;
}


ssize_t
te_cwmp_pack__GetOptions(const _cwmp__GetOptions *src,
                         void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__GetOptions);
    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(string, OptionName);
    return packed_length;
}

ssize_t
te_cwmp_pack__OptionStruct(const cwmp__OptionStruct *src,
                           void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(cwmp__OptionStruct);
    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(string, OptionName);

    if (NULL != src->ExpirationDate)
    {
        WARN("pack OptionStruct: TODO pack ExpirationData");
        dst->ExpirationDate = NULL;
    }
    return packed_length;
}

CWMP_PACK_LIST_FUNC(OptionList, OptionStruct)

ssize_t
te_cwmp_pack__GetOptionsResponse(const _cwmp__GetOptionsResponse *src,
                                 void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__GetOptionsResponse);
    CWMP_PACK_ROW(sizeof(*src));
    CWMP_PACK_LEAF(OptionList, OptionList_);

    return packed_length;
}

CWMP_PACK_LIST_FUNC(AccessList, string)

ssize_t
te_cwmp_pack__SetParameterAttributesStruct(
            const cwmp__SetParameterAttributesStruct *src,
            void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(cwmp__SetParameterAttributesStruct);

    CWMP_PACK_ROW(sizeof(*src));

    if (NULL != src->Name)
    {
        dst->Name = msg;
        msg += sizeof(void*);
        packed_length += sizeof(void*);

        CWMP_PACK_LEAF(string, Name[0]);

        dst->Name = (void*)((char*)dst->Name - (char*)dst);
    }
    CWMP_PACK_LEAF(AccessList, AccessList_);

    return packed_length;
}


ssize_t
te_cwmp_pack__ParameterAttributeStruct(
                const cwmp__ParameterAttributeStruct *src,
                void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(cwmp__ParameterAttributeStruct);

    CWMP_PACK_ROW(sizeof(*src));

    CWMP_PACK_LEAF(string, Name);
    CWMP_PACK_LEAF(AccessList, AccessList_);

    return packed_length;
}

CWMP_PACK_LIST_FUNC(SetParameterAttributesList, SetParameterAttributesStruct)

ssize_t
te_cwmp_pack__SetParameterAttributes(
                        const _cwmp__SetParameterAttributes *src,
                        void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__SetParameterAttributes); 
    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(SetParameterAttributesList, ParameterList); 

    return packed_length;
}

ssize_t
te_cwmp_pack__GetParameterAttributes(
                        const _cwmp__GetParameterAttributes *src,
                        void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__GetParameterAttributes); 
    CWMP_PACK_ROW(sizeof(*src));
    CWMP_PACK_LEAF(ParameterNames, ParameterNames_);
    return packed_length;
}

CWMP_PACK_LIST_FUNC(ParameterAttributeList, ParameterAttributeStruct)

ssize_t
te_cwmp_pack__GetParameterAttributesResponse(
                        const _cwmp__GetParameterAttributesResponse *src,
                        void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__GetParameterAttributesResponse); 
    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(ParameterAttributeList, ParameterList); 
    return packed_length;
}


ssize_t
te_cwmp_pack__Upload(const _cwmp__Upload *src,
                     void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__Upload);

    CWMP_PACK_ROW(sizeof(*src)); 

    CWMP_PACK_LEAF(string, CommandKey);
    CWMP_PACK_LEAF(string, FileType);
    CWMP_PACK_LEAF(string, URL);
    CWMP_PACK_LEAF(string, Username);
    CWMP_PACK_LEAF(string, Password);

    return packed_length;
}

ssize_t
te_cwmp_pack__UploadResponse(const _cwmp__UploadResponse *src,
                             void *msg, size_t max_len)
{
    size_t packed_length = 0;
    CWMP_PACK_ROW(sizeof(*src)); 
    return packed_length;
}



ssize_t
te_cwmp_pack__ScheduleInform(const _cwmp__ScheduleInform *src,
                             void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__ScheduleInform);

    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(string, CommandKey);
    return packed_length;
}



ssize_t
te_cwmp_pack__base64(const SOAP_ENC__base64 *src,
                       void *msg, size_t max_len)
{
    size_t packed_length = 0;
    size_t alig_padding = 0;
    SOAP_ENC__base64 *dst = (SOAP_ENC__base64 *)msg; 

    if (NULL == src->__ptr || 0 == src->__size)
    {
        dst->__ptr = (void*)0;
        dst->__size = 0;
        return sizeof(*src);
    }

    packed_length = sizeof(*src) + src->__size;
    dst->__size = src->__size;
    msg += sizeof(*src);

    if ((alig_padding = (src->__size & 3)) != 0)
        packed_length += 4 - alig_padding; 

    if (packed_length > max_len)
        return -1;

    dst->__ptr = (void*)((char *)(msg) - (char*)(dst));

    memcpy(msg, src->__ptr, src->__size);

    return packed_length;
}

CWMP_PACK_LIST_FUNC(VoucherList, base64)

ssize_t
te_cwmp_pack__SetVouchers(const _cwmp__SetVouchers *src,
                          void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__SetVouchers);

    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(VoucherList, VoucherList_); 
    return packed_length;
}


ssize_t
te_cwmp_pack__QueuedTransferStruct(const cwmp__QueuedTransferStruct *src,
                                   void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(cwmp__QueuedTransferStruct);

    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(string, CommandKey); 
    return packed_length;
}

CWMP_PACK_LIST_FUNC(TransferList, QueuedTransferStruct)

ssize_t
te_cwmp_pack__AllQueuedTransferStruct(const cwmp__AllQueuedTransferStruct *src,
                                      void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(cwmp__AllQueuedTransferStruct);

    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(string, CommandKey); 
    CWMP_PACK_LEAF(string, FileType); 
    CWMP_PACK_LEAF(string, TargetFileName); 
    return packed_length;
}

CWMP_PACK_LIST_FUNC(AllTransferList, AllQueuedTransferStruct)

ssize_t
te_cwmp_pack__GetQueuedTransfersResponse(
                    const _cwmp__GetQueuedTransfersResponse *src,
                    void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__GetQueuedTransfersResponse);

    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(TransferList, TransferList_); 
    return packed_length;
}

ssize_t
te_cwmp_pack__GetAllQueuedTransfersResponse(
                    const _cwmp__GetAllQueuedTransfersResponse *src,
                    void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__GetAllQueuedTransfersResponse);

    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(AllTransferList, TransferList_); 
    return packed_length;
}


ssize_t
te_cwmp_pack__AutonomousTransferComplete(
            const _cwmp__AutonomousTransferComplete *src,
            void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__AutonomousTransferComplete);

    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(string, AnnounceURL);
    CWMP_PACK_LEAF(string, TransferURL);
    CWMP_PACK_LEAF(string, FileType);
    CWMP_PACK_LEAF(string, TargetFileName);
    CWMP_PACK_LEAF(FaultStruct, FaultStruct);
    return packed_length;
}

ssize_t
te_cwmp_pack__ArgStruct(const cwmp__ArgStruct *src,
                        void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(cwmp__ArgStruct);

    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(string, Name);
    CWMP_PACK_LEAF(string, Value);
    return packed_length;
}

CWMP_PACK_LIST_FUNC(FileTypeArg, ArgStruct)

ssize_t
te_cwmp_pack__RequestDownload(const _cwmp__RequestDownload *src,
                              void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__RequestDownload);

    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(string, FileType);
    CWMP_PACK_LEAF(FileTypeArg, FileTypeArg_);
    return packed_length;
}


ssize_t
te_cwmp_pack__Kicked(const _cwmp__Kicked *src, void *msg, size_t max_len)
{
    CWMP_PACK_COMMON_VARS(_cwmp__Kicked); 
    CWMP_PACK_ROW(sizeof(*src)); 
    CWMP_PACK_LEAF(string, Command);
    CWMP_PACK_LEAF(string, Referer);
    CWMP_PACK_LEAF(string, Arg);
    CWMP_PACK_LEAF(string, Next);
    return packed_length;
}




/*
 * ============= UN-PACK methods ================
 */

/*
 * Unpack macros assume following local variables and parameters:
 * msg, max_len, res, unpack_size. Local variables are defined by 
 * macro CWMP_UNPACK_VARS.
 */


#define CWMP_UNPACK_VARS(_type) \
    _type *res = msg; \
    size_t unpack_size = sizeof(*res);

#define CWMP_UNPACK_LEAF(_leaf_type, _leaf) \
    do { \
        ssize_t rc; \
        size_t ofs = (size_t)(res-> _leaf);  \
                                        \
        if (ofs >= max_len) \
        { \
            ERROR("UNPACK_LEAF at line %d failed, ofs %u >= max_len %u", \
                  __LINE__, ofs, max_len); \
            return -1;  \
        } \
        if (0 == ofs) \
        { \
            res-> _leaf = NULL; \
            break; \
        } \
        rc = te_cwmp_unpack__ ## _leaf_type(msg + ofs, max_len - ofs); \
        if (rc < 0) \
        { \
            ERROR("UNPACK_LEAF at line %d failed, leaf subtype %s ", \
                  __LINE__, #_leaf_type); \
            return -1; \
        } \
        res-> _leaf = msg + ofs; \
        unpack_size += rc; \
    } while (0)


ssize_t
te_cwmp_unpack__DeviceIdStruct(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(cwmp__DeviceIdStruct);

    CWMP_UNPACK_LEAF(string, Manufacturer);
    CWMP_UNPACK_LEAF(string, OUI);
    CWMP_UNPACK_LEAF(string, ProductClass);
    CWMP_UNPACK_LEAF(string, SerialNumber);

    return unpack_size;
}

ssize_t
te_cwmp_unpack__EventStruct(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(cwmp__EventStruct);

    CWMP_UNPACK_LEAF(string, EventCode);
    CWMP_UNPACK_LEAF(string, CommandKey);

    return unpack_size;
}

#if 0
    if (res->__size == 0 || ofs == 0) 
#endif

#define CWMP_UNPACK_LIST_FUNC(_list_type, _elem_type) \
ssize_t \
te_cwmp_unpack__ ## _list_type (void *msg, size_t max_len) \
{ \
    _list_type *res = msg; \
    unsigned int ofs; \
    int i; \
    ssize_t rc = 0; \
 \
    ofs = (unsigned int)(res->__ptr ## _elem_type); \
    if (ofs == 0) \
    { \
        res->__ptr ## _elem_type = NULL; \
        return sizeof(_list_type); \
    } \
 \
    if (ofs >= max_len) \
        return -1; \
 \
    res->__ptr ## _elem_type = (void *)((char *)msg + ofs); \
    for (i = 0; i < res->__size; i++) \
    { \
        ofs = (unsigned int)(res->__ptr ## _elem_type[i]); \
        rc = te_cwmp_unpack__ ## _elem_type (msg + ofs, max_len - ofs); \
        if (rc < 0) \
            return -1; \
        res->__ptr ## _elem_type [i] = msg + ofs; \
    } \
    /* now 'ofs' is offset of last element in array */ \
    ofs += rc; \
 \
    return ofs; \
}

CWMP_UNPACK_LIST_FUNC(MethodList, string)

CWMP_UNPACK_LIST_FUNC(EventList, EventStruct)

CWMP_UNPACK_LIST_FUNC(ParameterInfoList, ParameterInfoStruct)

CWMP_UNPACK_LIST_FUNC(ParameterValueList, ParameterValueStruct)

CWMP_UNPACK_LIST_FUNC(ParameterNames, string)

ssize_t
te_cwmp_unpack__ParameterValueStruct(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(cwmp__ParameterValueStruct);

    CWMP_UNPACK_LEAF(string, Name);

    switch (res->__type)
    {
        case SOAP_TYPE_string:
        case SOAP_TYPE_xsd__anySimpleType:
        case SOAP_TYPE_SOAP_ENC__base64:
            CWMP_UNPACK_LEAF(string, Value);
            break;
        case SOAP_TYPE_time:
            CWMP_UNPACK_LEAF(time, Value);
            break;
        default: /* integer, boolean types storen similar usual int. */
            CWMP_UNPACK_LEAF(integer, Value);
            break;
    }

    return unpack_size;
}

ssize_t
te_cwmp_unpack__Inform(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__Inform);

    CWMP_UNPACK_LEAF(DeviceIdStruct, DeviceId);
    CWMP_UNPACK_LEAF(EventList, Event);
    CWMP_UNPACK_LEAF(ParameterValueList, ParameterList);

    return unpack_size;
}


ssize_t
te_cwmp_unpack__GetRPCMethodsResponse(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__GetRPCMethodsResponse);

    CWMP_UNPACK_LEAF(MethodList, MethodList_);

    return unpack_size;
}


ssize_t
te_cwmp_unpack__SetParameterValues(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__SetParameterValues);

    CWMP_UNPACK_LEAF(ParameterValueList, ParameterList);
    CWMP_UNPACK_LEAF(string, ParameterKey);

    return unpack_size;
}

ssize_t
te_cwmp_unpack__SetParameterValuesResponse(void *msg, size_t max_len)
{
    UNUSED(msg);
    UNUSED(max_len);
    /* no pointer to subvalues*/
    return sizeof (_cwmp__SetParameterValuesResponse);
}

ssize_t
te_cwmp_unpack__GetParameterValues(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__GetParameterValues);

    CWMP_UNPACK_LEAF(ParameterNames, ParameterNames_);

    return unpack_size;
}

ssize_t
te_cwmp_unpack__GetParameterValuesResponse(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__GetParameterValuesResponse);

    CWMP_UNPACK_LEAF(ParameterValueList, ParameterList);

    return unpack_size;
}

ssize_t
te_cwmp_unpack__GetParameterNames(void *msg, size_t max_len)
{
    size_t f;
    CWMP_UNPACK_VARS(_cwmp__GetParameterNames);

    f = (size_t)res->ParameterPath;
    if (0 == f)
    {
        res->ParameterPath = NULL;
        return unpack_size;
    }
    res->ParameterPath = msg + f;

    CWMP_UNPACK_LEAF(string, ParameterPath[0]);

    return unpack_size;
}

ssize_t
te_cwmp_unpack__GetParameterNamesResponse(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__GetParameterNamesResponse);

    CWMP_UNPACK_LEAF(ParameterInfoList, ParameterList);

    return unpack_size;
}


ssize_t
te_cwmp_unpack__ParameterInfoStruct(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(cwmp__ParameterInfoStruct);

    CWMP_UNPACK_LEAF(string, Name);

    return unpack_size;
}


ssize_t
te_cwmp_unpack__Download(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__Download);

    CWMP_UNPACK_LEAF(string, CommandKey);
    CWMP_UNPACK_LEAF(string, FileType);
    CWMP_UNPACK_LEAF(string, URL);
    CWMP_UNPACK_LEAF(string, Username);
    CWMP_UNPACK_LEAF(string, Password);
    CWMP_UNPACK_LEAF(string, TargetFileName);
    CWMP_UNPACK_LEAF(string, SuccessURL);
    CWMP_UNPACK_LEAF(string, FailureURL);

    return unpack_size;
}

ssize_t
te_cwmp_unpack__DownloadResponse(void *msg, size_t max_len)
{
    UNUSED(msg);
    UNUSED(max_len);
    /* no pointer to subvalues*/
    return sizeof(_cwmp__DownloadResponse);
}
ssize_t
te_cwmp_unpack__Reboot(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__Reboot);
    CWMP_UNPACK_LEAF(string, CommandKey);
    return unpack_size;
}


ssize_t
te_cwmp_unpack__FaultStruct(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(cwmp__FaultStruct);
    CWMP_UNPACK_LEAF(string, FaultCode);
    CWMP_UNPACK_LEAF(string, FaultString);
    return unpack_size;
}

ssize_t
te_cwmp_unpack__TransferComplete(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__TransferComplete);
    CWMP_UNPACK_LEAF(string, CommandKey);
    CWMP_UNPACK_LEAF(FaultStruct, FaultStruct);
    return unpack_size;
}


ssize_t
te_cwmp_unpack__AddObject(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__AddObject);
    CWMP_UNPACK_LEAF(string, ObjectName);
    CWMP_UNPACK_LEAF(string, ParameterKey);
    return unpack_size;
}
ssize_t
te_cwmp_unpack__AddObjectResponse(void *msg, size_t max_len)
{
    UNUSED(msg);
    UNUSED(max_len);
    return sizeof(_cwmp__AddObjectResponse);
}
ssize_t
te_cwmp_unpack__DeleteObject(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__DeleteObject);
    CWMP_UNPACK_LEAF(string, ObjectName);
    CWMP_UNPACK_LEAF(string, ParameterKey);
    return unpack_size;
}

ssize_t
te_cwmp_unpack__DeleteObjectResponse(void *msg, size_t max_len)
{
    UNUSED(msg);
    UNUSED(max_len);
    return sizeof(_cwmp__DeleteObjectResponse);
}


ssize_t
te_cwmp_unpack__GetOptions(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__GetOptions);
    CWMP_UNPACK_LEAF(string, OptionName);
    return unpack_size;
}

ssize_t
te_cwmp_unpack__OptionStruct(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(cwmp__OptionStruct);
    CWMP_UNPACK_LEAF(string, OptionName);
    return unpack_size;
}

CWMP_UNPACK_LIST_FUNC(OptionList, OptionStruct)

ssize_t
te_cwmp_unpack__GetOptionsResponse(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__GetOptionsResponse);
    CWMP_UNPACK_LEAF(OptionList, OptionList_);

    return unpack_size;
}


ssize_t
te_cwmp_unpack__Fault(void *msg, size_t max_len)
{
    size_t array_ofs = 0;
    int i = 0;
    CWMP_UNPACK_VARS(_cwmp__Fault);
    CWMP_UNPACK_LEAF(string, FaultCode);
    CWMP_UNPACK_LEAF(string, FaultString);

    /* Here start some ugly hacks because for this type gSOAP generated
     * non-usual presentation of array. */

    array_ofs = (size_t)(res->SetParameterValuesFault);
                                        \
    if (array_ofs >= max_len) return -1;
    if (0 == array_ofs || 0 == res->__sizeSetParameterValuesFault)
        res->SetParameterValuesFault = NULL;
    else
        res->SetParameterValuesFault = msg + array_ofs;

    unpack_size += sizeof(struct _cwmp__Fault_SetParameterValuesFault) *
                    res->__sizeSetParameterValuesFault;

    for (i = 0; i < res->__sizeSetParameterValuesFault; i++) \
    {
        CWMP_UNPACK_LEAF(string, SetParameterValuesFault[i].ParameterName);
        CWMP_UNPACK_LEAF(string, SetParameterValuesFault[i].FaultCode);
        CWMP_UNPACK_LEAF(string, SetParameterValuesFault[i].FaultString);
    }

    return unpack_size;
}


CWMP_UNPACK_LIST_FUNC(AccessList, string)

ssize_t
te_cwmp_unpack__SetParameterAttributesStruct(void *msg, size_t max_len)
{
    size_t f;
    CWMP_UNPACK_VARS(cwmp__SetParameterAttributesStruct);

    f = (size_t)res->Name;
    if (0 != f)
    {
        res->Name = msg + f;
        CWMP_UNPACK_LEAF(string, Name[0]);
    }
    else
        res->Name = NULL;
    CWMP_UNPACK_LEAF(AccessList, AccessList_);

    return unpack_size;
}

ssize_t
te_cwmp_unpack__ParameterAttributeStruct(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(cwmp__ParameterAttributeStruct);

    CWMP_UNPACK_LEAF(string, Name);
    CWMP_UNPACK_LEAF(AccessList, AccessList_);

    return unpack_size;
}


CWMP_UNPACK_LIST_FUNC(SetParameterAttributesList, SetParameterAttributesStruct)

ssize_t
te_cwmp_unpack__SetParameterAttributes(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__SetParameterAttributes); 
    CWMP_UNPACK_LEAF(SetParameterAttributesList, ParameterList); 
    return unpack_size;
}

ssize_t
te_cwmp_unpack__GetParameterAttributes(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__GetParameterAttributes); 
    CWMP_UNPACK_LEAF(ParameterNames, ParameterNames_);
    return unpack_size;
}

CWMP_UNPACK_LIST_FUNC(ParameterAttributeList, ParameterAttributeStruct)

ssize_t
te_cwmp_unpack__GetParameterAttributesResponse(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__GetParameterAttributesResponse); 
    CWMP_UNPACK_LEAF(ParameterAttributeList, ParameterList); 
    return unpack_size;
}


ssize_t
te_cwmp_unpack__Upload(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__Upload);

    CWMP_UNPACK_LEAF(string, CommandKey);
    CWMP_UNPACK_LEAF(string, FileType);
    CWMP_UNPACK_LEAF(string, URL);
    CWMP_UNPACK_LEAF(string, Username);
    CWMP_UNPACK_LEAF(string, Password);

    return unpack_size;
}
ssize_t
te_cwmp_unpack__UploadResponse(void *msg, size_t max_len)
{
    UNUSED(msg);
    UNUSED(max_len);
    return sizeof(_cwmp__UploadResponse);
}


ssize_t
te_cwmp_unpack__ScheduleInform(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__ScheduleInform); 
    CWMP_UNPACK_LEAF(string, CommandKey);
    return unpack_size;
}



ssize_t
te_cwmp_unpack__base64(void *msg, size_t max_len)
{
    SOAP_ENC__base64 *res = msg; 
    size_t unpack_size = sizeof(*res);
    size_t alig_padding = (res->__size & 3); 
    size_t ofs = (size_t)(res->__ptr);

    unpack_size += res->__size;
    if (alig_padding != 0)
        unpack_size += 4 - alig_padding; 

    if (ofs + res->__size >= max_len) return -1;

    if (0 == ofs)
        res->__ptr = NULL;
    else
        res->__ptr = msg + ofs;

    return unpack_size;
}


CWMP_UNPACK_LIST_FUNC(VoucherList, base64)

ssize_t
te_cwmp_unpack__SetVouchers(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__SetVouchers); 
    CWMP_UNPACK_LEAF(VoucherList, VoucherList_); 
    return unpack_size;
}



ssize_t
te_cwmp_unpack__QueuedTransferStruct(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(cwmp__QueuedTransferStruct);

    CWMP_UNPACK_LEAF(string, CommandKey); 
    return unpack_size;
}

CWMP_UNPACK_LIST_FUNC(TransferList, QueuedTransferStruct)

ssize_t
te_cwmp_unpack__GetQueuedTransfersResponse(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__GetQueuedTransfersResponse);
    CWMP_UNPACK_LEAF(TransferList, TransferList_); 
    return unpack_size;
}



ssize_t
te_cwmp_unpack__AllQueuedTransferStruct(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(cwmp__AllQueuedTransferStruct); 
    CWMP_UNPACK_LEAF(string, CommandKey); 
    CWMP_UNPACK_LEAF(string, FileType); 
    CWMP_UNPACK_LEAF(string, TargetFileName); 
    return unpack_size;
}

CWMP_UNPACK_LIST_FUNC(AllTransferList, AllQueuedTransferStruct)

ssize_t
te_cwmp_unpack__GetAllQueuedTransfersResponse(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__GetAllQueuedTransfersResponse);
    CWMP_UNPACK_LEAF(AllTransferList, TransferList_); 
    return unpack_size;
}


ssize_t
te_cwmp_unpack__AutonomousTransferComplete(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__AutonomousTransferComplete);

    CWMP_UNPACK_LEAF(string, AnnounceURL);
    CWMP_UNPACK_LEAF(string, TransferURL);
    CWMP_UNPACK_LEAF(string, FileType);
    CWMP_UNPACK_LEAF(string, TargetFileName);
    CWMP_UNPACK_LEAF(FaultStruct, FaultStruct);
    return unpack_size;
} 

ssize_t
te_cwmp_unpack__ArgStruct(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(cwmp__ArgStruct);

    CWMP_UNPACK_LEAF(string, Name);
    CWMP_UNPACK_LEAF(string, Value);
    return unpack_size;
} 

CWMP_UNPACK_LIST_FUNC(FileTypeArg, ArgStruct)

ssize_t
te_cwmp_unpack__RequestDownload(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__RequestDownload);

    CWMP_UNPACK_LEAF(string, FileType);
    CWMP_UNPACK_LEAF(FileTypeArg, FileTypeArg_);
    return unpack_size;
} 

ssize_t
te_cwmp_unpack__Kicked(void *msg, size_t max_len)
{
    CWMP_UNPACK_VARS(_cwmp__Kicked); 
    CWMP_UNPACK_LEAF(string, Command);
    CWMP_UNPACK_LEAF(string, Referer);
    CWMP_UNPACK_LEAF(string, Arg);
    CWMP_UNPACK_LEAF(string, Next);
    return unpack_size;
}







/*
 * ============= Generic utils ================
 */

/* see description in cwmp_data.h */
ssize_t
cwmp_pack_call_data(cwmp_data_to_cpe_t src,
                    te_cwmp_rpc_cpe_t rpc_cpe,
                    void *msg, size_t len)
{
    if (NULL == src.p || NULL == msg)
        return 0;

    switch (rpc_cpe)
    {
        case CWMP_RPC_set_vouchers:
            return te_cwmp_pack__SetVouchers(src.set_vouchers,
                                             msg, len);
        case CWMP_RPC_schedule_inform:
            return te_cwmp_pack__ScheduleInform(src.schedule_inform,
                                                msg, len);
        case CWMP_RPC_upload:
            return te_cwmp_pack__Upload(src.upload, msg, len);
        case CWMP_RPC_set_parameter_attributes:
            return te_cwmp_pack__SetParameterAttributes(
                            src.set_parameter_attributes,
                            msg, len);
        case CWMP_RPC_get_parameter_attributes:
            return te_cwmp_pack__GetParameterAttributes(
                            src.get_parameter_attributes,
                            msg, len);
        case CWMP_RPC_get_options:
            return te_cwmp_pack__GetOptions(
                            src.get_options,
                            msg, len);
        case CWMP_RPC_set_parameter_values:
            return te_cwmp_pack__SetParameterValues(
                            src.set_parameter_values,
                            msg, len);
        case CWMP_RPC_get_parameter_values:
            return te_cwmp_pack__GetParameterValues(
                            src.get_parameter_values,
                            msg, len);
        case CWMP_RPC_get_parameter_names:
            return te_cwmp_pack__GetParameterNames(
                            src.get_parameter_names,
                            msg, len);
        case CWMP_RPC_download:
            return te_cwmp_pack__Download(
                            src.download, msg, len);
        case CWMP_RPC_reboot:
            return te_cwmp_pack__Reboot(
                            src.reboot, msg, len); 
        case CWMP_RPC_add_object:
            return te_cwmp_pack__AddObject(
                            src.add_object, msg, len);
        case CWMP_RPC_delete_object:
            return te_cwmp_pack__DeleteObject(
                            src.delete_object, msg, len);

        case CWMP_RPC_NONE:
        case CWMP_RPC_get_rpc_methods:
        case CWMP_RPC_factory_reset:
        case CWMP_RPC_get_queued_transfers:
        case CWMP_RPC_get_all_queued_transfers:
        case CWMP_RPC_FAULT:
            /* do nothing, no data to CPE */
            return 0;
    }
    return 0;
}

/* see description in cwmp_data.h */
ssize_t
cwmp_pack_response_data(cwmp_data_from_cpe_t src,
                        te_cwmp_rpc_cpe_t rpc_cpe,
                        void *msg, size_t len)
{
    if (NULL == src.p)
        return 0;

    switch (rpc_cpe)
    {
        case CWMP_RPC_get_queued_transfers:
            return te_cwmp_pack__GetQueuedTransfersResponse(
                            src.get_queued_transfers_r,
                            msg, len);
        case CWMP_RPC_get_all_queued_transfers:
            return te_cwmp_pack__GetAllQueuedTransfersResponse(
                            src.get_all_queued_transfers_r,
                            msg, len);
        case CWMP_RPC_upload:
            return te_cwmp_pack__UploadResponse(src.upload_r, msg, len);
        case CWMP_RPC_get_parameter_attributes:
            return te_cwmp_pack__GetParameterAttributesResponse(
                            src.get_parameter_attributes_r,
                            msg, len);
        case CWMP_RPC_get_options:
            return te_cwmp_pack__GetOptionsResponse(
                            src.get_options_r, msg, len);
        case CWMP_RPC_get_rpc_methods:
            return te_cwmp_pack__GetRPCMethodsResponse(
                            src.get_rpc_methods_r, msg, len);
        case CWMP_RPC_set_parameter_values:
            return te_cwmp_pack__SetParameterValuesResponse(
                            src.set_parameter_values_r, msg, len);
        case CWMP_RPC_get_parameter_values:
            return te_cwmp_pack__GetParameterValuesResponse(
                            src.get_parameter_values_r, msg, len);
        case CWMP_RPC_get_parameter_names:
            return te_cwmp_pack__GetParameterNamesResponse(
                            src.get_parameter_names_r, msg, len);
        case CWMP_RPC_download:
            return te_cwmp_pack__DownloadResponse(
                            src.download_r, msg, len);
        case CWMP_RPC_add_object:
            return te_cwmp_pack__AddObjectResponse(
                            src.add_object_r, msg, len);
        case CWMP_RPC_delete_object:
            return te_cwmp_pack__DeleteObjectResponse(
                            src.delete_object_r, msg, len);
        case CWMP_RPC_FAULT:
            return te_cwmp_pack__Fault(src.fault, msg, len);

        /* Calls with no data */
        case CWMP_RPC_NONE:
        case CWMP_RPC_schedule_inform:
        case CWMP_RPC_set_vouchers:
        case CWMP_RPC_reboot:
        case CWMP_RPC_set_parameter_attributes:
        case CWMP_RPC_factory_reset:
            /* do nothing, no data to CPE */
            return 0;
    }
    return 0;
}



/*
 * Unpack data from message client->ACSE.
 * 
 * @param buf           Place with packed data (usually in 
 *                      local copy of transfered struct).
 * @param len           Length of packed data.
 * @param cwmp_data     Specific CWMP data with unpacked payload.
 * 
 * @return status code
 */
te_errno
cwmp_unpack_call_data(void *buf, size_t len,
                      te_cwmp_rpc_cpe_t rpc_cpe)
{
    te_errno rc = 0;

    switch (rpc_cpe)
    {
        case CWMP_RPC_set_vouchers:
            rc = (te_cwmp_unpack__SetVouchers(buf, len) > 0) ? 0 : TE_EFAIL;
            break;
            
        case CWMP_RPC_schedule_inform:
            rc = (te_cwmp_unpack__ScheduleInform(buf, len) > 0) ? 0 : TE_EFAIL;
            break;
        case CWMP_RPC_upload:
            rc = (te_cwmp_unpack__Upload(buf, len) > 0) ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_set_parameter_attributes:
            rc = (te_cwmp_unpack__SetParameterAttributes(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_get_parameter_attributes:
            rc = (te_cwmp_unpack__GetParameterAttributes(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_get_options:
            rc = (te_cwmp_unpack__GetOptions(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_set_parameter_values:
            rc = (te_cwmp_unpack__SetParameterValues(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_get_parameter_values:
            rc = (te_cwmp_unpack__GetParameterValues(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_get_parameter_names:
            rc = (te_cwmp_unpack__GetParameterNames(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_reboot:
            rc = (te_cwmp_unpack__Reboot(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_download:
            rc = (te_cwmp_unpack__Download(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_add_object:
            rc = (te_cwmp_unpack__AddObject(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_delete_object:
            rc = (te_cwmp_unpack__DeleteObject(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;

        case CWMP_RPC_NONE:
        case CWMP_RPC_get_rpc_methods:
        case CWMP_RPC_factory_reset:
        case CWMP_RPC_get_queued_transfers:
        case CWMP_RPC_get_all_queued_transfers:
        case CWMP_RPC_FAULT:
            /* do nothing, no data to CPE */
            return 0;
    }
    if (rc != 0)
        ERROR("CWMP unpack of %s failed", cwmp_rpc_cpe_string(rpc_cpe));
    return rc;
}

te_errno
cwmp_unpack_response_data(void *buf, size_t len,
                          te_cwmp_rpc_cpe_t rpc_cpe)
{
    te_errno rc = 0;

    switch (rpc_cpe)
    {
        case CWMP_RPC_get_queued_transfers:
            rc = (te_cwmp_unpack__GetQueuedTransfersResponse(buf, len) > 0)
                    ? 0 : TE_EFAIL;
            break;
        case CWMP_RPC_get_all_queued_transfers:
            rc = (te_cwmp_unpack__GetAllQueuedTransfersResponse(buf, len) > 0)
                    ? 0 : TE_EFAIL;
            break;
        case CWMP_RPC_upload:
            rc = (te_cwmp_unpack__UploadResponse(buf, len) > 0)
                    ? 0 : TE_EFAIL;
            break;
        case CWMP_RPC_get_parameter_attributes:
            rc = (te_cwmp_unpack__GetParameterAttributesResponse(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_get_options:
            rc = (te_cwmp_unpack__GetOptionsResponse(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_get_rpc_methods:
            rc = (te_cwmp_unpack__GetRPCMethodsResponse(buf, len) > 0)
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_set_parameter_values:
            rc = (te_cwmp_unpack__SetParameterValuesResponse(buf, len) > 0)
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_get_parameter_values:
            rc = (te_cwmp_unpack__GetParameterValuesResponse(buf, len) > 0)
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_get_parameter_names:
            rc = (te_cwmp_unpack__GetParameterNamesResponse(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_download:
            rc = (te_cwmp_unpack__DownloadResponse(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_FAULT:
            rc = (te_cwmp_unpack__Fault(buf, len) > 0) ? 0 : TE_EFAIL;
            break;
        case CWMP_RPC_add_object:
            rc = (te_cwmp_unpack__AddObjectResponse(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_delete_object:
            rc = (te_cwmp_unpack__DeleteObjectResponse(buf, len) > 0) 
                    ? 0 : TE_EFAIL;  
            break;

        case CWMP_RPC_NONE:
        case CWMP_RPC_schedule_inform:
        case CWMP_RPC_set_vouchers:
        case CWMP_RPC_reboot:
        case CWMP_RPC_set_parameter_attributes:
        case CWMP_RPC_factory_reset:
            /* do nothing, no data to CPE */
            return 0;
    }
    if (rc != 0)
        ERROR("EPC unpack failed");
    return rc;
}



te_errno
cwmp_pack_acs_rpc_data(cwmp_data_from_cpe_t src,
                        te_cwmp_rpc_acs_t rpc_acs,
                        void *msg, size_t len)
{
    if (NULL == src.p)
        return 0;
    switch (rpc_acs)
    {
        case CWMP_RPC_transfer_complete:
            return te_cwmp_pack__TransferComplete(src.transfer_complete,
                                                  msg, len);
        case CWMP_RPC_inform:
            return te_cwmp_pack__Inform(src.inform, msg, len);

        case CWMP_RPC_autonomous_transfer_complete: 
            return te_cwmp_pack__AutonomousTransferComplete(
                            src.aut_transfer_complete, msg, len);
        case CWMP_RPC_request_download:
            return te_cwmp_pack__RequestDownload(
                            src.request_download, msg, len); 
        case CWMP_RPC_kicked:
            return te_cwmp_pack__Kicked(src.kicked, msg, len);

        case CWMP_RPC_ACS_FAULT:
            WARN("%s(): CWMP_RPC_ACS_FAULT detected, should not be",
                 __FUNCTION__);
        case CWMP_RPC_ACS_NONE:
        case CWMP_RPC_ACS_get_rpc_methods:
            /* nothing to do */
            return 0;
    }
    return 0;
}


te_errno
cwmp_unpack_acs_rpc_data(void *buf, size_t len,
                         te_cwmp_rpc_acs_t rpc_acs)
{
    te_errno rc = 0;

    switch (rpc_acs)
    {
        case CWMP_RPC_inform:
            rc = (te_cwmp_unpack__Inform(buf, len) > 0)
                    ? 0 : TE_EFAIL;
            break;
        case CWMP_RPC_transfer_complete:
            rc = (te_cwmp_unpack__TransferComplete(buf, len) > 0)
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_autonomous_transfer_complete:
            rc = (te_cwmp_unpack__AutonomousTransferComplete(buf, len) > 0)
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_request_download:
            rc = (te_cwmp_unpack__RequestDownload(buf, len) > 0)
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_kicked:
            rc = (te_cwmp_unpack__Kicked(buf, len) > 0)
                    ? 0 : TE_EFAIL;  
            break;
        case CWMP_RPC_ACS_FAULT:
            WARN("%s(): CWMP_RPC_ACS_FAULT detected, should not be",
                 __FUNCTION__);
        case CWMP_RPC_ACS_get_rpc_methods:
        case CWMP_RPC_ACS_NONE:
            /* nothing to do */
            return 0;
    }
    return rc;
}



