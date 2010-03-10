
/** @file 
 * @brief CWMP data common methods implementation
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
 * $Id$
*/


#include "cwmp_data.h"

#include <string.h>

/* Do not make any alignment */
#define CWMP_PACK(_src, _msg, _item_length, _packed_length, _max_length) \
    do { \
        if ((_item_length) > (_max_length - _packed_length)) \
            return -1; \
        memcpy((_msg), (_src), (_item_length)); \
        (_packed_length) += (_item_length); \
        (_msg) += (_item_length); \
    } while(0)

/* Assume local variable 'dst', points to message start, with same type
   as pointer '_src'.
   Performs alignment of _msg and _packed_l after coping of string. 
*/
#define CWMP_PACK_STR(_src, _msg, _packed_l, _max_l, _field) \
    do { \
        size_t alig_padding; \
        size_t str_size = strlen( _src -> _field ) + 1; \
        dst-> _field = (void*)((char *)(_msg) - (char*)dst); \
        CWMP_PACK(src-> _field , msg, str_size, \
                  _packed_l, _max_l); \
        if ((alig_padding = (str_size & 3)) != 0) \
        { \
            (_msg) += 4 - alig_padding; \
            (_packed_l) += 4 - alig_padding; \
        } \
    } while (0)


ssize_t
te_cwmp_pack__DeviceIdStruct(const cwmp__DeviceIdStruct *src,
                             void *msg, size_t max_len)
{
    size_t packed_length = 0;
    cwmp__DeviceIdStruct *dst = msg;

    CWMP_PACK(src, msg, sizeof(*src), packed_length, max_len);
    
    CWMP_PACK_STR(src, msg, packed_length, max_len, Manufacturer);
    CWMP_PACK_STR(src, msg, packed_length, max_len, OUI);
    CWMP_PACK_STR(src, msg, packed_length, max_len, ProductClass); 
    CWMP_PACK_STR(src, msg, packed_length, max_len, SerialNumber);

    return packed_length;
}

ssize_t
te_cwmp_pack__Inform(const _cwmp__Inform *src, void *msg, size_t max_len)
{
    size_t packed_length = 0;
    ssize_t rc;
    _cwmp__Inform *dst = msg;

    CWMP_PACK(src, msg, sizeof(_cwmp__Inform), packed_length, max_len);

    rc = te_cwmp_pack__DeviceIdStruct(src->DeviceId, msg,
                                       max_len - packed_length);
    if (rc < 0)
        return -1;
    dst->DeviceId = (void*)((char *)msg - (char*)dst);
    packed_length += rc;
    msg += rc;


    rc = te_cwmp_pack__EventList(src->Event, msg, max_len - packed_length);
    if (rc < 0)
        return -1;
    dst->Event = (void*)((char *)msg - (char*)dst);
    packed_length += rc;
    msg += rc;

    /* TODO */
    dst->ParameterList = 0;

    return packed_length;
}


ssize_t
te_cwmp_pack__EventStruct(const cwmp__EventStruct *src,
                          void *msg, size_t max_len)
{
    size_t packed_length = 0;
    cwmp__EventStruct *dst = msg;

    CWMP_PACK(src, msg, sizeof(*src), packed_length, max_len);

    CWMP_PACK_STR(src, msg, packed_length, max_len, EventCode);
    CWMP_PACK_STR(src, msg, packed_length, max_len, CommandKey);

    return packed_length;
}


ssize_t
te_cwmp_pack__EventList(const EventList *src,
                          void *msg, size_t max_len)
{
    ssize_t rc;
    size_t packed_length = 0;
    size_t array_memlen = 0;
    EventList *dst = msg;
    int i = 0;

    CWMP_PACK(src, msg, sizeof(*src), packed_length, max_len);
    
    /* For fill links to array elements here it is convenient
       to have real pointer to the array start */
    dst->__ptrEventStruct = msg; 

    array_memlen = sizeof(void*) * src->__size;
    msg += array_memlen;
    packed_length += array_memlen;

    for (i = 0; i < src->__size; i++)
    {
        rc = te_cwmp_pack__EventStruct(src->__ptrEventStruct[i], 
                                       msg, max_len - packed_length);
        if (rc < 0)
            return -1;
        dst->__ptrEventStruct[i] = (void*)((char *)msg - (char*)dst);
        packed_length += rc;
        msg += rc;
    }
    /* Put array offset instead of real pointer */
    dst->__ptrEventStruct =
        (void*)((char *)dst->__ptrEventStruct - (char*)dst);

    return packed_length;
}




ssize_t
te_cwmp_pack__MethodList(const MethodList *src, void *msg, size_t max_len)
{
    size_t packed_length = 0;
    size_t array_memlen = 0;
    MethodList *dst = msg;
    int i = 0;

    CWMP_PACK(src, msg, sizeof(*src), packed_length, max_len);
    
    /* For fill links to array elements here it is convenient
       to have real pointer to the array start */
    dst->__ptrstring = msg; 

    array_memlen = sizeof(void*) * src->__size;
    msg += array_memlen;
    packed_length += array_memlen;

    for (i = 0; i < src->__size; i++)
    {
        CWMP_PACK_STR(src, msg, packed_length, max_len, __ptrstring[i] );
    }
    /* Put array offset instead of real pointer */
    dst->__ptrstring =
        (void*)((char *)dst->__ptrstring - (char*)dst);

    return packed_length;
}

ssize_t
te_cwmp_pack__GetRPCMethodsResponse(
        const _cwmp__GetRPCMethodsResponse *src,
        void *msg, size_t max_len)
{
    size_t packed_length = 0;
    ssize_t rc;
    _cwmp__GetRPCMethodsResponse *dst = msg;

    CWMP_PACK(src, msg, sizeof(*src), packed_length, max_len);

    rc = te_cwmp_pack__MethodList(src->MethodList_, msg,
                                  max_len - packed_length);
    if (rc < 0)
        return -1;
    dst->MethodList_ = (void*)((char *)msg - (char*)dst);
    packed_length += rc;
    msg += rc;

    return packed_length;
}

/*
 * ============= UN-PACK methods ================
 */


/* Assume function parameters 'msg' and 'max_len' */
#define CWMP_UNPACK_STRING(_field, _ofs) \
    do { \
        unsigned int f = (unsigned int)(_field); \
        if (f >= max_len) return -1; \
        if ((_ofs) < f) (_ofs) = f; \
        (_field) += (unsigned int) (msg); \
    } while (0)

ssize_t
te_cwmp_unpack__DeviceIdStruct(void *msg, size_t max_len)
{
    cwmp__DeviceIdStruct *res = msg;
    size_t ofs = 0;

    CWMP_UNPACK_STRING(res->Manufacturer, ofs);
    CWMP_UNPACK_STRING(res->OUI, ofs);
    CWMP_UNPACK_STRING(res->ProductClass, ofs);
    CWMP_UNPACK_STRING(res->SerialNumber, ofs);
    /* now 'ofs' is maximum offset from all strings */

#if 1
    ofs += strlen(((char *)msg) + ofs);
#else
    ofs += strnlen(((char *)msg) + ofs, (max_len) - ofs);
#endif

    return ofs;
}


ssize_t
te_cwmp_unpack__EventStruct(void *msg, size_t max_len)
{
    cwmp__EventStruct *res = msg;
    size_t ofs = 0;

    CWMP_UNPACK_STRING(res->EventCode,  ofs);
    CWMP_UNPACK_STRING(res->CommandKey, ofs);

#if 0
    ofs += strnlen(((char *)msg) + ofs, (max_len) - ofs);
#else
    ofs += strlen(((char *)msg) + ofs);
#endif

    return ofs;
}

/* TODO: make macro for array unpack */

ssize_t
te_cwmp_unpack__EventList(void *msg, size_t max_len)
{
    EventList *res = msg;
    unsigned int ofs;
    int i;
    ssize_t rc = 0;

    ofs = (unsigned int)(res->__ptrEventStruct);
    if (res->__size == 0 || ofs == 0)
    {
        res->__ptrEventStruct = NULL;
        return sizeof(EventList);
    }

    if (ofs >= max_len)
        return -1;

    res->__ptrEventStruct = (void *)((char *)msg + ofs);
    for (i = 0; i < res->__size; i++)
    {
        ofs = (unsigned int)(res->__ptrEventStruct[i]);
        rc = te_cwmp_unpack__EventStruct(msg + ofs, max_len - ofs);
        if (rc < 0)
            return -1;
        res->__ptrEventStruct[i] = msg + ofs;
    }
    /* now 'ofs' is offset of last element in array */
    ofs += rc;

    return ofs;
}

ssize_t
te_cwmp_unpack__ParameterValueStruct(void *msg, size_t max_len)
{
    return -1;
}

ssize_t
te_cwmp_unpack__Inform(void *msg, size_t max_len)
{
    _cwmp__Inform *ret = msg;
    unsigned int ofs;
    ssize_t rc;

    ofs = (unsigned int)ret->DeviceId;
    if (ofs != 0)
    {
        if (ofs >= max_len)
            return -1;
        rc = te_cwmp_unpack__DeviceIdStruct(msg + ofs, max_len - ofs);
        if (rc < 0)
            return -1;
        ret->DeviceId = msg + ofs;
        ofs += rc;
    }
    else
    {
        ret->DeviceId = NULL;
        ofs = sizeof(_cwmp__Inform);
    }


    ofs = (unsigned int)ret->Event;
    if (ofs >= max_len)
            return -1;
    if (ofs != 0)
    {
        rc = te_cwmp_unpack__EventList(msg + ofs, max_len - ofs);
        if (rc < 0)
            return -1;
        ret->Event = msg + ofs;
        ofs += rc;
    }
    else
        ret->Event = NULL;

    return ofs;
}


ssize_t
te_cwmp_unpack__MethodList(void *msg, size_t max_len)
{
    MethodList *res = msg;
    unsigned int ofs;
    int i;

    ofs = (unsigned int)(res->__ptrstring);
    if (res->__size == 0 || ofs == 0)
    {
        res->__ptrstring = NULL;
        return sizeof(MethodList);
    }

    if (ofs >= max_len)
        return -1;

    res->__ptrstring = (void *)((char *)msg + ofs);
    for (i = 0; i < res->__size; i++)
    {
        ofs = (unsigned int)(res->__ptrstring[i]);
        res->__ptrstring[i] = msg + ofs;
    }
    /* now 'ofs' is offset of last element in array */
    ofs += strlen(((char *)msg) + ofs);

    return ofs;
}

ssize_t
te_cwmp_unpack__GetRPCMethodsResponse(void *msg, size_t max_len)
{
    _cwmp__GetRPCMethodsResponse *res = msg;
    unsigned int ofs;
    ssize_t rc;

    ofs = (unsigned int)res->MethodList_;
    if (ofs >= max_len)
            return -1;
    if (ofs != 0)
    {
        rc = te_cwmp_unpack__MethodList(msg + ofs, max_len - ofs);
        if (rc < 0)
            return -1;
        res->MethodList_ = msg + ofs;
        ofs += rc;
    }
    else
        res->MethodList_ = NULL;

    return ofs;
}

