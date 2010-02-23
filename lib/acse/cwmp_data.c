
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
 * $Id: $
*/


#include "cwmp_data.h"

#define CWMP_PACK(_src, _msg, _item_length, _packed_length, _max_length) \
    do { \
        if ((_item_length) > (_max_length - _packed_length)) \
            return TE_ESMALLBUF; \
        memcpy((_msg), (_src), (_item_length)); \
        _packed_length += (_item_length); \
        _msg += (_item_length); \
    } while(0)


te_errno
te_cwmp_pack__Inform(const _cwmp__Inform *src, void *msg, size_t max_len)
{
    size_t packed_length = 0;

#if 0
    if (max_len < sizeof(_cwmp__Inform))
        return TE_ESMALLBUF;
    memcpy(msg, src, sizeof(_cwmp__Inform));
    packed_length += sizeof(_cwmp__Inform);
    msg+= sizeof(_cwmp__Inform);
#else
    CWMP_PACK(src, msg, sizeof(_cwmp__Inform), packed_length, max_len);
#endif

    return 0;
}

_cwmp__Inform *
te_cwmp_unpack__Inform(void *msg, size_t len)
{
    return 0;
}

_cwmp__InformResponse *
te_cwmp_unpack__InformResponse(void *msg, size_t len)
{
    return 0;
}
