/** @file
 * @brief Proteos, TAD file protocol, NDN.
 *
 * Definitions of ASN.1 types for NDN for file protocol. 
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * @author Konstantin Abramenko <konst@oktetlabs.ru>
 *
 * $Id$
 */

#include "asn_impl.h"
#include "ndn_internal.h"
#include "ndn_file.h"


static asn_named_entry_t _ndn_file_message_ne_array [] = 
{
    { "line", &ndn_data_unit_char_string_s }
};

asn_type ndn_file_message_s =
{
    "File-Message", {PRIVATE, 100}, SEQUENCE, 
    sizeof(_ndn_file_message_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_file_message_ne_array}
};

asn_type_p ndn_file_message = &ndn_file_message_s;








static asn_named_entry_t _ndn_file_csap_ne_array [] = 
{
    { "filename", &ndn_data_unit_char_string_s },
    { "mode",     &ndn_data_unit_char_string_s }
};

asn_type ndn_file_csap_s =
{
    "File-CSAP", {PRIVATE, 101}, SEQUENCE, 
    sizeof(_ndn_file_csap_ne_array)/sizeof(asn_named_entry_t),
    {_ndn_file_csap_ne_array}
};

asn_type_p ndn_file_csap = &ndn_file_csap_s;



