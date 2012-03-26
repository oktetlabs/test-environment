/** @file
 * @brief Dummy functions for Test Agent sniffers calls.
 *
 * Copyright (C) 2004-2012 Test Environment authors (see file AUTHORS
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
 * @author Andrey Dmitrov <Andrey.Dmitrov@oktetlabs.ru>
 *
 * $Id: $
 */

#include "te_config.h"
#include "rcf_pch_internal.h"
#include "te_errno.h"

/**
 * Dummy function for rcf_ch_get_sniffers call. Used when the agent does not
 * support the sniffer framework.
 * 
 * @return (TE_RCF_PCH, TE_ENOPROTOOPT)     value means the protocol is
 *                                          not supported
 */
te_errno
rcf_ch_get_sniffers(struct rcf_comm_connection *handle, char *cbuf, 
                    size_t buflen, size_t answer_plen, 
                    const char *sniff_id_str)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(sniff_id_str);
    return TE_RC(TE_RCF_PCH, TE_ENOPROTOOPT);
}

/**
 * Dummy function for rcf_ch_get_snif_dump call. Used when the agent does
 * not support the sniffer framework.
 * 
 * @return (TE_RCF_PCH, TE_ENOPROTOOPT)     value means the protocol is
 *                                          not supported
 */
te_errno
rcf_ch_get_snif_dump(struct rcf_comm_connection *handle, char *cbuf, 
                    size_t buflen, size_t answer_plen, 
                    const char *sniff_id_str)
{
    UNUSED(handle);
    UNUSED(cbuf);
    UNUSED(buflen);
    UNUSED(answer_plen);
    UNUSED(sniff_id_str);
    return TE_RC(TE_RCF_PCH, TE_ENOPROTOOPT);
}
