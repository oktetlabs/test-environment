
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
    /* TODO */
    dst->Event = 0;
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
}


cwmp__DeviceIdStruct *
te_cwmp_unpack__DeviceIdStruct(void *msg, size_t len)
{
    cwmp__DeviceIdStruct *ret = msg;

    ret->Manufacturer += (unsigned int)msg;
    ret->OUI          += (unsigned int)msg;
    ret->ProductClass += (unsigned int)msg;
    ret->SerialNumber += (unsigned int)msg;

    return ret;
}


cwmp__EventStruct *
te_cwmp_unpack__EventStruct(void *msg, size_t len)
{
    return NULL;
}

cwmp__ParameterValueStruct *
te_cwmp_unpack__ParameterValueStruct(void *msg, size_t len)
{
    return NULL;
}

_cwmp__Inform *
te_cwmp_unpack__Inform(void *msg, size_t len)
{
    _cwmp__Inform *ret = msg;
    unsigned int ofs;

    ofs = (unsigned int)ret->DeviceId;
    if (ofs != 0)
    {
        if (ofs >= len)
            return NULL;
        ret->DeviceId  = 
            te_cwmp_unpack__DeviceIdStruct(msg + ofs, len - ofs);
    }
    return ret;
}

_cwmp__InformResponse *
te_cwmp_unpack__InformResponse(void *msg, size_t len)
{
    /* TODO */
    return NULL;
}
